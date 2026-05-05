# FFmpeg 与 MPEG4 Demuxer 读帧重试机制梳理

## 范围

本文梳理当前 `av_codec` 分支中 FFmpeg demuxer plugin 与 MPEG4 demuxer plugin 在读帧链路上的重试、等待和超时行为，重点覆盖：

- 同步读帧：`ReadSample(trackId, buffer)` / `GetNextSampleSize(trackId, size)`
- 异步读帧：`ReadSample(trackId, buffer, timeout)` / `ReadSampleZeroCopy(trackId, buffer, timeout)` / `GetNextSampleSize(trackId, size, timeout)`
- 直接数据读取：`DataSource::ReadAt()` 返回 `Status::ERROR_AGAIN` 后的处理
- 初始化/探测/分片解析中与读数据相关的重试

## 关键结论

- FFmpeg 的核心弱网重试集中在 `AVReadPacket()`，同步和异步最终都会经由 `av_read_frame()` 触发该回调，但 `InvokerType::READ` 只在异步接口里设置，因此同步读帧与异步读帧的 `ERROR_AGAIN` 行为并不完全相同。
- MPEG4 的核心弱网重试集中在 `ReadSampleData()`，同步和异步共用同一套 `ReadAt()` 重试参数：最多 10 次，每次等待 5 ms。
- FFmpeg 异步读帧对 `ERROR_AGAIN` 是“线程内持续等待/重试”，不会直接把 `ERROR_AGAIN` 暴露给异步接口调用者；调用者只会因接口 `timeout` 等不到缓存而得到 `ERROR_WAIT_TIMEOUT`。
- MPEG4 异步读帧当前已对齐 FFmpeg 异步语义：`ReadSampleData()` 返回 `ERROR_AGAIN` 后，异步读线程保持存活并继续下一轮读取，不再终止 `readThread_`。
- `timeout` 参数只控制异步接口等待缓存的时间，不等价于底层 `ReadAt()` 的重试总时长。

## 常量与等待参数

| 模块 | 位置 | 常量/参数 | 值 | 含义 |
| --- | --- | --- | --- | --- |
| FFmpeg | `ffmpeg_demuxer_thread.cpp` | `AV_READ_PACKET_RETRY_UPPER_LIMIT` | 9 | 异步 READ 场景下，连续 `ERROR_AGAIN` 达到该阈值后转为条件变量等待 |
| FFmpeg | `ffmpeg_demuxer_thread.cpp` | `AV_READ_PACKET_SLEEP_TIME` | 50 ms | 异步 READ 场景下每次短重试等待时间 |
| FFmpeg | `ffmpeg_demuxer_thread.cpp` | `AV_READ_PACKET_NON_READ_RETRY_UPPER_LIMIT` | 10 | INIT/SEEK/非 READ 场景下 bounded retry 上限 |
| FFmpeg | `ffmpeg_demuxer_thread.cpp` | `AV_READ_PACKET_NON_READ_SLEEP_TIME` | 10 ms | INIT/SEEK/非 READ 场景下 bounded retry 等待时间 |
| FFmpeg | `ffmpeg_demuxer_plugin.cpp` | `PROBE_READ_RETRY_TIMES` | 10 | 探测读取 `ReadProbeData()` 的重试上限 |
| FFmpeg | `ffmpeg_demuxer_plugin.cpp` | `PROBE_READ_SLEEP_TIME_US` | 10,000 us | 探测读取每次等待时间 |
| FFmpeg | `ffmpeg_demuxer_plugin.cpp` | `SETTIMER_TIMEOUT` | 5 s | XCollie 卡顿日志定时器，不是读数据重试超时 |
| FFmpeg | `IOContext::readTimeoutMs` | 运行时设置 | 默认 0 | INIT/SEEK 非 READ 路径可选总超时；0 表示不启用 |
| MPEG4 | `mpeg4_demuxer_plugin.cpp` | `READ_RETRY_TIMES` | 10 | `ReadSampleData()` 对 `ReadAt()` 的重试上限 |
| MPEG4 | `mpeg4_demuxer_plugin.cpp` | `READ_RETRY_SLEEP_TIME_US` | 5,000 us | `ReadSampleData()` 每次重试等待时间 |
| MPEG4/Common | `demuxer_data_reader.cpp` | `READ_RETRY_TIMES` | 10 | box/header 读取 `DemuxerDataReader::ReadUintData()` 的重试上限 |
| MPEG4/Common | `demuxer_data_reader.cpp` | `READ_RETRY_SLEEP_TIME_US` | 5,000 us | box/header 读取每次等待时间 |
| MPEG4 | `mpeg4_demuxer_plugin.cpp` | `PROBE_SIZE` | 5,000,000 | sniff/probe 建议读取范围，不是重试次数 |

## FFmpeg 同步读帧链路

### 入口与调用链

同步入口：

- `FFmpegDemuxerPlugin::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)`
- `FFmpegDemuxerPlugin::GetNextSampleSize(uint32_t trackId, int32_t& size)`

主要调用链：

```text
ReadSample()
  -> WaitForCacheReady(trackId)
    -> ReadPacketToCacheQueue(trackId)
      -> AVReadFrameLimit(pkt)
        -> av_read_frame(formatContext_, pkt)
          -> AVReadPacket(opaque, buf, bufSize)
            -> dataSource->ReadAt(offset, buffer, bufSize)
```

### 同步 ERROR_AGAIN 行为

- `AVReadPacket()` 是 FFmpeg 读取 `DataSource` 的统一入口。
- 当 `DataSource::ReadAt()` 返回 `ERROR_AGAIN` 且没有读到有效数据时，进入 `HandleReadAgain()`。
- 同步 `ReadSample()` 本身没有把 `ioContext_.invokerType` 设置为 `InvokerType::READ`，因此底层通常走 `HandleNonReadAgain()` 的非 READ 分支。
- 非 READ 分支会按 `AV_READ_PACKET_NON_READ_RETRY_UPPER_LIMIT` 做 bounded retry：
  - `AVReadPacketStopState::UNSET` 或普通非 READ：最多 10 次，每次睡眠 10 ms。
  - INIT/SEEK 且 `AVReadPacketStopState::FALSE`：每次睡眠 50 ms，并可受 `readTimeoutMs` 约束。
- 重试耗尽后 `AVReadPacket()` 返回错误，`HandleReadFrameResult()` 若检测到 `ioContext_.retry == true`，会 `ResetContext()` 并返回 `Status::ERROR_AGAIN`。
- `FrameReady()` 将 `Status::ERROR_AGAIN` 视为未就绪并返回 false，最终同步接口向调用者返回 `ERROR_AGAIN`。

### 同步等待特点

- 同步接口没有用户传入的 `timeout`。
- `SETTIMER_TIMEOUT = 5s` 是 XCollie 日志/监控定时器，用于发现卡顿，不会主动中断读帧。
- 同步读帧在 cache 未就绪时会持续读包，直到读到目标 track 的 sample、EOS 或错误。

## FFmpeg 异步读帧链路

### 入口与调用链

异步入口：

- `FFmpegDemuxerPlugin::ReadSample(trackId, sample, timeout)`
- `FFmpegDemuxerPlugin::ReadSampleZeroCopy(trackId, sample, timeout)`
- `FFmpegDemuxerPlugin::GetNextSampleSize(trackId, size, timeout)`

主要调用链：

```text
ReadSample(..., timeout)
  -> 设置 ioContext_.invokerType = InvokerType::READ
  -> 启动 FFmpegReadLoop()
  -> WaitForLoop(trackId, timeout)

FFmpegReadLoop()
  -> ReadAndProcessFrame()
    -> AVReadFrameLimit(pkt)
      -> av_read_frame()
        -> AVReadPacket()
          -> dataSource->ReadAt()
```

### 异步接口 timeout 行为

- 若目标 track cache 为空且 `timeout == 0`，立即返回 `Status::ERROR_WAIT_TIMEOUT`。
- 若启动了读线程但 `WaitForLoop()` 在 `timeout` 毫秒内没有等到目标 track cache 满足条件，返回 `Status::ERROR_WAIT_TIMEOUT`。
- `WaitForLoop()` 的等待条件只关注 cache 是否就绪；它不直接控制底层 `ReadAt()` 的重试次数。

### 异步 ERROR_AGAIN 行为

- 异步入口会设置 `ioContext_.invokerType = InvokerType::READ`。
- `AVReadPacket()` 收到 `ERROR_AGAIN` 且没有读到数据时：
  - `tryCount` 为 1 到 8 时走短重试，每次睡眠 50 ms。
  - 第 9 次进入 `readCbCv_` 条件变量等待。
  - 等待由外部读请求、状态变化或销毁等路径唤醒，唤醒后 `tryCount` 清零继续读。
- 因此 FFmpeg 异步 READ 场景不是固定次数失败后退出，而是“短重试 + 条件变量等待 + 再重试”的可恢复循环。
- `ERROR_AGAIN` 通常不会作为异步接口返回值暴露给调用者；调用者更常见得到的是 `ERROR_WAIT_TIMEOUT`，后续再次调用可继续等待同一个读线程产出 cache。

### 异步线程异常传播

- 真正不可恢复错误会写入 `readLoopStatus_`，`WaitForLoop()` 被唤醒后返回该错误。
- EOS 会向所有 track cache 写 EOS sample，并把 `readLoopStatus_` 置为 `END_OF_STREAM`。

## MPEG4 同步读帧链路

### 入口与调用链

同步入口：

- `MPEG4DemuxerPlugin::ReadSample(trackId, outBuffer)`
- `MPEG4DemuxerPlugin::GetNextSampleSize(trackId, sampleSize)`

`ReadSample()` 主要调用链：

```text
ReadSample()
  -> GetSampleBySeekableStatus(seekable_, trackId, mpeg4Sample, outBuffer)
    -> GetSeekableSampleInfo() 或 GetUnseekableSampleInfo()
    -> ReadSampleData(sample, pos, size, trackId)
      -> dataSource_->ReadAt(pos, buffer, bufferSize)
  -> WriteBuffer()
```

### 同步 ERROR_AGAIN 行为

- `ReadSampleData()` 内部统一处理 `DataSource::ReadAt()` 的 `ERROR_AGAIN`。
- 重试条件是 `ret == Status::ERROR_AGAIN && count < READ_RETRY_TIMES`。
- 每次未读满目标 sample size 时会 `count++` 并 `usleep(5000)`。
- 最多 10 次重试，单次 `ReadSampleData()` 的等待约为 50 ms 量级。
- 如果最终仍然没有有效数据，则返回 `Status::ERROR_AGAIN`。
- 同步 `ReadSample()` 不再额外吞掉或循环 `ERROR_AGAIN`，因此 `ERROR_AGAIN` 会向调用者返回。

### GetNextSampleSize 同步行为

- 当前 MPEG4 同步 `GetNextSampleSize()` 主要基于已解析 sample table 的 `currentSampleIndex` 计算 size。
- 它本身不触发 sample data 的 `ReadAt()`，因此没有读帧数据层的 `ERROR_AGAIN` 重试。

## MPEG4 异步读帧链路

### 入口与调用链

异步入口：

- `MPEG4DemuxerPlugin::ReadSample(trackId, outBuffer, timeout)`
- `MPEG4DemuxerPlugin::ReadSampleZeroCopy(trackId, outBuffer, timeout)`
- `MPEG4DemuxerPlugin::GetNextSampleSize(trackId, sampleSize, timeout)`

主要调用链：

```text
ReadSample(..., timeout)
  -> 启动 MPEG4ReadLoop()
  -> WaitForLoop(trackId, timeout)

MPEG4ReadLoop()
  -> ReadAndCacheSample()
    -> ReadSampleToCache(trackId)
      -> GetUnseekableSampleInfo(currentTrackId, pos, size)
      -> ReadSampleData(sample, pos, size, currentTrackId)
        -> dataSource_->ReadAt(pos, buffer, bufferSize)
      -> AddSampleToCacheQueue()
```

### 异步接口 timeout 行为

- 若目标 track cache 为空且 `timeout == 0`，立即返回 `Status::ERROR_WAIT_TIMEOUT`。
- 若读线程已启动但 `WaitForLoop()` 在 `timeout` 毫秒内没有等到目标 track cache 或读线程异常状态，返回 `Status::ERROR_WAIT_TIMEOUT`。
- `timeout` 只控制调用线程等待 cache 的时长，不改变 `ReadSampleData()` 的 10 次、5 ms 重试参数。

### 异步 ERROR_AGAIN 行为

- `ReadSampleToCache()` 调用 `ReadSampleData()`，当 `ReadSampleData()` 返回 `ERROR_AGAIN` 时会把该状态返回给 `ReadAndCacheSample()`。
- 当前对齐后的 `ReadAndCacheSample()` 行为：
  - `ret == Status::ERROR_AGAIN` 时，将 `readLoopStatus_` 保持为 `Status::OK`。
  - 返回 `true`，让 `MPEG4ReadLoop()` 继续下一轮读取。
  - 不向 cache 写错误，不终止 `readThread_`。
- 调用线程若在本次 `timeout` 内等不到 sample，会收到 `ERROR_WAIT_TIMEOUT`。
- 下一次异步读接口调用会继续等待同一个读线程后续重试产出的 sample。

### 分片解析 ERROR_AGAIN

- `GetUnseekableSampleInfo()` 在无 sample 且需要解析下一个 moof 时调用 `ParseMoof()`。
- 如果 `ParseMoof()` 返回 `ERROR_AGAIN`，`GetUnseekableSampleInfo()` 会直接返回 `ERROR_AGAIN`。
- 异步路径下该 `ERROR_AGAIN` 也会经 `ReadSampleToCache()` 传到 `ReadAndCacheSample()`，按可恢复状态继续循环。
- 同步路径下该 `ERROR_AGAIN` 会返回给调用者。

## MPEG4 header/box 读取重试

MPEG4 `SetDataSource()` / `ParseMoof()` / sniff 相关解析通过 `DemuxerDataReader` 读取 box/header 数据：

```text
DemuxerDataReader::ReadUintData()
  -> dataSource_->ReadAt(offset, bufferInfo, size)
```

该层参数：

- 最多 10 次。
- 每次等待 5 ms。
- 重试耗尽后，如果仍未读到足够数据，返回 `ERROR_AGAIN` 或原始错误。

这层属于元数据/box 解析重试，与 sample data 的 `ReadSampleData()` 重试参数相同但实现位置不同。

## FFmpeg 探测读取重试

FFmpeg plugin set 阶段的 `ReadProbeData()` 对探测数据读取有独立重试：

- 最多 10 次。
- 每次等待 10 ms。
- `ret == OK` 但未读到数据时，最终转换为 `ERROR_AGAIN`。
- 非 `OK` 且非 `ERROR_AGAIN` 的错误会立即返回。

这层主要用于格式探测，不属于常规读帧 cache 生产路径。

## 主线程与异步读子线程协同

说明：当前 FFmpeg/MPEG4 demuxer 异步读帧链路未使用真正的 semaphore 类型；这里的“信号量/信号”主要由 `std::condition_variable`、mutex 和 atomic 标志位共同表达。

### 共同角色

- 主线程：调用 `ReadSample(..., timeout)`、`ReadSampleZeroCopy(..., timeout)` 或 `GetNextSampleSize(..., timeout)` 的线程。它负责指定当前目标 `trackId_`，必要时启动读子线程，然后等待目标 track 的 cache 可用。
- 异步读子线程：`FFmpegReadLoop()` 或 `MPEG4ReadLoop()`。它负责从底层数据源读取 packet/sample，写入 `cacheQueue_`，并通过条件变量通知主线程。
- cache：主线程和子线程之间的核心同步对象。主线程等待 cache “足够可读”，子线程在 cache “已有数据或暂停”时停止继续读，避免无限预读。

### FFmpeg 使用的同步对象

| 对象 | 类型 | 主要使用方 | 作用 |
| --- | --- | --- | --- |
| `readCacheCv_` | `condition_variable` | 主线程等待，子线程通知 | 子线程写入 cache、EOS 或线程结束后通知主线程 |
| `readLoopCv_` | `condition_variable` | 子线程等待，主线程通知 | 子线程因 cache 已有数据/暂停进入 WAITING 后，由主线程或状态变化唤醒 |
| `readCbCv_` | static `condition_variable` | `AVReadPacket()` 等待，主线程/销毁线程通知 | FFmpeg 特有，用于 `ERROR_AGAIN` 达到阈值后让 `AVReadPacket()` 阻塞等待下一次读请求或状态变化 |
| `seekWaitCv_` | `condition_variable` | seek/flush 等控制流程等待 | 等待读线程进入 WAITING 或 NOT_STARTED，避免和读线程并发修改状态 |
| `threadState_` | atomic enum | 主线程/子线程 | 标识 NOT_STARTED/READING/WAITING |
| `threadReady_` | atomic bool | 主线程置位，子线程消费 | 主线程要求 WAITING 状态的读线程恢复读 |
| `isWaitingForReadThread_` | atomic bool | 主线程置位，子线程观察 | 标识当前有主线程正在等待读线程产出 cache |
| `ioContext_.readCbReady` | atomic bool | 主线程置位，`AVReadPacket()` 消费 | 唤醒卡在 `readCbCv_` 的 FFmpeg 底层读回调 |
| `ioContext_.invokerType` | atomic enum | 主线程/控制线程/子线程 | 标识 READ/SEEK/FLUSH/DESTORY 等调用状态，影响读线程退出和 `ERROR_AGAIN` 重试策略 |

### FFmpeg 主线程等待条件

主线程进入 `WaitForLoop(trackId, timeout)` 后，先判断 `ShouldWaitForRead(trackId)`：

```text
NeedCombineFrame(trackId) && cacheQueue_.GetCacheSize(trackId) <= 1
或
!NeedCombineFrame(trackId) && !cacheQueue_.HasCache(trackId)
```

含义：

- 对需要组帧的 track，cache 只有 0 或 1 个 packet 还不够，需要继续等待。
- 对普通 track，cache 为空就需要等待。

如果需要等待：

- 设置 `isWaitingForReadThread_ = true`。
- 如果子线程正在 READING，主线程会设置 `ioContext_.readCbReady = true` 并通知 `readCbCv_`，用于唤醒可能卡在 FFmpeg `AVReadPacket()` 弱网等待里的子线程。
- 如果子线程处于 WAITING，主线程会设置 `threadReady_ = true` 并通知 `readLoopCv_`，让子线程恢复读取。
- 主线程随后在 `readCacheCv_.wait_for(timeout)` 上阻塞。

主线程解除阻塞的条件：

```text
!ShouldWaitForRead(trackId)
```

也就是目标 track cache 已满足读取条件。若 `timeout` 到期仍不满足：

- 如果 `readLoopStatus_ == OK`，返回 `ERROR_WAIT_TIMEOUT`。
- 如果 `readLoopStatus_ != OK`，返回读线程记录的异常状态。

### FFmpeg 子线程等待条件

`FFmpegReadLoop()` 每轮读取前会检查 `NeedWaitForRead(trackId)`：

```text
(cacheQueue_.HasCache(trackId) || isPauseReadPacket_) &&
ioContext_.invokerType != DESTORY
```

含义：

- 当前目标 track 已经有 cache，继续预读意义不大，进入等待。
- 或者上层调用了 Pause，也进入等待。
- DESTORY 状态不等待，直接走退出。

子线程进入 `HandleReadWait()` 后：

- 设置 `threadState_ = WAITING`。
- 通知 `seekWaitCv_`，让 seek/flush 等控制流程知道读线程已安全停住。
- 在 `readLoopCv_` 上阻塞。

子线程解除阻塞的条件：

```text
threadReady_
或 ioContext_.invokerType == DESTORY
或 (!cacheQueue_.HasCache(trackId) && !isPauseReadPacket_)
或 (isWaitingForReadThread_ && cacheQueue_.GetCacheSize(trackId) <= 1)
```

其中最后一个条件是 FFmpeg 组帧场景的特殊处理：即使 cache 里已有 1 个 packet，只要主线程正在等待读线程，并且组帧还不够，就允许子线程继续读取补齐 cache。

### FFmpeg ERROR_AGAIN 等待协同

FFmpeg 还有一层特殊协同发生在 `AVReadPacket()` 内部：

- `DataSource::ReadAt()` 连续返回 `ERROR_AGAIN` 且没有读到数据时，前 8 次每次睡眠 50 ms。
- 第 9 次进入 `readCbCv_.wait()`。
- 等待条件是：

```text
ioContext->readCbReady
或 ioContext->invokerType != READ
```

主线程下一次进入异步读接口并发现子线程仍在 READING 时，会设置 `readCbReady = true` 并通知 `readCbCv_`。这使得弱网恢复后，卡在底层读回调里的子线程能够继续尝试读取，而不是一直沉睡。

### MPEG4 使用的同步对象

| 对象 | 类型 | 主要使用方 | 作用 |
| --- | --- | --- | --- |
| `readCacheCv_` | `condition_variable` | 主线程等待，子线程通知 | 子线程写入 cache、EOS、线程结束或读循环推进时通知主线程 |
| `readLoopCv_` | `condition_variable` | 子线程等待，主线程通知 | 子线程因 cache 已有数据/暂停进入 WAITING 后，由主线程或状态变化唤醒 |
| `seekWaitCv_` | `condition_variable` | seek/flush 等控制流程等待 | 等待读线程进入 WAITING 或 NOT_STARTED |
| `threadState_` | atomic enum | 主线程/子线程 | 标识 NOT_STARTED/READING/WAITING |
| `threadReady_` | atomic bool | 主线程置位，子线程消费 | 主线程要求 WAITING 状态的读线程恢复读 |
| `isWaitingForReadThread_` | atomic bool | 主线程置位，子线程观察 | 标识当前有主线程正在等待读线程产出 cache |
| `stopReadThread_` | atomic bool | 控制线程/子线程 | 控制读线程退出 |
| `readLoopStatus_` | atomic Status | 子线程写，主线程读 | 传播 EOS 或不可恢复错误；当前 `ERROR_AGAIN` 不写入异常状态 |

### MPEG4 主线程等待条件

主线程进入 `WaitForLoop(trackId, timeout)` 后，判断 `ShouldWaitForRead(trackId)`：

```text
!cacheQueue_.HasCache(trackId)
```

也就是目标 track cache 为空时需要等待。

如果需要等待：

- 设置 `isWaitingForReadThread_ = true`。
- 如果子线程处于 WAITING，主线程设置 `threadReady_ = true` 并通知 `readLoopCv_`。
- 主线程在 `readCacheCv_.wait_for(timeout)` 上阻塞。

主线程解除阻塞的条件：

```text
!ShouldWaitForRead(trackId)
或 readLoopStatus_ != OK
```

也就是目标 track cache 可读，或读线程报告了 EOS/不可恢复异常。若 `timeout` 到期：

- 如果 `readLoopStatus_ == OK`，返回 `ERROR_WAIT_TIMEOUT`。
- 如果 `readLoopStatus_ != OK`，返回读线程记录的异常状态。

### MPEG4 子线程等待条件

`MPEG4ReadLoop()` 每轮读取前检查 `NeedWaitForRead(trackId)`：

```text
(cacheQueue_.HasCache(trackId) || isPauseReadPacket_) &&
!stopReadThread_
```

含义：

- 当前目标 track 已经有 cache，子线程暂停预读。
- 或者上层调用了 Pause，子线程暂停。
- 如果已经要求停止，则不进入等待，直接走退出。

子线程进入 `HandleReadWait()` 后：

- 设置 `threadState_ = WAITING`。
- 通知 `seekWaitCv_`。
- 在 `readLoopCv_` 上阻塞。

子线程解除阻塞的条件：

```text
threadReady_
或 stopReadThread_
或 (!cacheQueue_.HasCache(trackId) && !isPauseReadPacket_)
或 isWaitingForReadThread_
```

与 FFmpeg 相比，MPEG4 没有 `readCbCv_` 这一层，因为 MPEG4 不把 `DataSource::ReadAt()` 包在 FFmpeg 的 `AVReadPacket()` 回调里；`ReadSampleData()` 返回 `ERROR_AGAIN` 后由 `MPEG4ReadLoop()` 外层继续下一轮读取。

### 主子线程协同对比

| 维度 | FFmpeg 异步 | MPEG4 异步 |
| --- | --- | --- |
| 主线程等待 cache 的条件 | 普通 track 无 cache；组帧 track cache size <= 1 | 目标 track 无 cache |
| 主线程等待对象 | `readCacheCv_.wait_for(timeout)` | `readCacheCv_.wait_for(timeout)` |
| 主线程 timeout 返回 | cache 未就绪且 `readLoopStatus_ == OK` 时返回 `ERROR_WAIT_TIMEOUT` | cache 未就绪且 `readLoopStatus_ == OK` 时返回 `ERROR_WAIT_TIMEOUT` |
| 子线程暂停预读条件 | 目标 track 有 cache 或 pause，且不是 DESTORY | 目标 track 有 cache 或 pause，且未 stop |
| 子线程等待对象 | `readLoopCv_.wait()` | `readLoopCv_.wait()` |
| 主线程唤醒 WAITING 子线程 | 设置 `threadReady_ = true`，notify `readLoopCv_` | 设置 `threadReady_ = true`，notify `readLoopCv_` |
| 子线程通知 cache 可读 | notify `readCacheCv_` | notify `readCacheCv_` |
| 弱网 ERROR_AGAIN 额外等待层 | 有，`readCbCv_` 阻塞 `AVReadPacket()` | 无，`ERROR_AGAIN` 回到读循环外层继续下一轮 |
| 控制流程等待读线程稳定 | `seekWaitCv_` 等 WAITING/NOT_STARTED | `seekWaitCv_` 等 WAITING/NOT_STARTED |
| 不可恢复错误传播 | `readLoopStatus_` | `readLoopStatus_` |

## 横向对比

| 维度 | FFmpeg 同步 | FFmpeg 异步 | MPEG4 同步 | MPEG4 异步 |
| --- | --- | --- | --- | --- |
| 用户接口 timeout | 无 | 有，控制等待 cache | 无 | 有，控制等待 cache |
| 底层 ReadAt 重试入口 | `AVReadPacket()` | `AVReadPacket()` | `ReadSampleData()` | `ReadSampleData()` |
| 底层重试次数 | 非 READ 通常最多 10 次 | READ 场景短重试 9 次后条件变量等待，可继续循环 | 最多 10 次 | 单轮最多 10 次，返回 `ERROR_AGAIN` 后线程继续下一轮 |
| 底层等待时间 | 非 READ 10 ms 或 INIT/SEEK FALSE 状态 50 ms | 50 ms，达到阈值后条件变量等待 | 5 ms | 5 ms |
| ERROR_AGAIN 是否返回给读帧调用者 | 可能返回 | 通常不直接返回，常表现为 `ERROR_WAIT_TIMEOUT` | 会返回 | 不直接终止线程，调用者可能先收到 `ERROR_WAIT_TIMEOUT` |
| 弱网恢复模型 | 调用者需要再次调用 | 读线程存活，后续调用继续等 cache | 调用者需要再次调用 | 读线程存活，后续调用继续等 cache |
| EOS 处理 | 写 cache 后返回 EOS sample | 写所有 track EOS cache | 同步直接设置 EOS sample 或返回 EOS | 写所有 selected track EOS cache |
| 线程终止条件 | 无异步线程 | DESTORY/不可恢复错误 | 无异步线程 | stop/不可恢复错误；`ERROR_AGAIN` 不终止 |

## 对齐情况与风险点

### 已对齐

- MPEG4 异步路径已将 `ERROR_AGAIN` 作为可恢复状态处理，行为与 FFmpeg 异步读帧“线程保持存活、调用侧按 timeout 等待 cache”的模型一致。
- MPEG4 异步接口返回语义与 FFmpeg 更接近：弱网下单次调用可返回 `ERROR_WAIT_TIMEOUT`，下一次调用继续等待并可成功读帧。

### 仍存在差异

- FFmpeg 异步 READ 在 `AVReadPacket()` 内部有“8 次 50 ms 短重试 + 第 9 次条件变量等待”的长生命周期模型；MPEG4 是 `ReadSampleData()` 单轮“10 次 5 ms”后把 `ERROR_AGAIN` 返回到读线程外层，再由读线程下一轮继续调用。
- FFmpeg 有 `readTimeoutMs` 支持 INIT/SEEK 非 READ 场景总超时；MPEG4 当前没有对应的 INIT/SEEK 总超时控制，主要依赖每层固定次数重试。
- FFmpeg 同步读帧可能通过 `HandleReadFrameResult()` 返回 `ERROR_AGAIN` 并触发 `ResetContext()`；MPEG4 同步读帧直接把 `ReadSampleData()` 的 `ERROR_AGAIN` 返回给调用者。
- FFmpeg 探测读取等待为 10 ms，MPEG4 box/header 读取等待为 5 ms。

## 建议测试覆盖

- MPEG4 异步弱网全量读取对比 FFmpeg：验证弱网 `ERROR_AGAIN` 后最终帧数、关键帧、PTS 与 FFmpeg 一致。
- MPEG4 异步短 timeout 场景：第一次 `ReadSample(..., timeout)` 返回 `ERROR_WAIT_TIMEOUT`，下一次重试成功返回 `OK`。
- MPEG4 同步弱网场景：验证 `ReadSample()` 在 `ReadSampleData()` 重试耗尽后返回 `ERROR_AGAIN`，调用者再次调用可恢复。
- MPEG4 分片 moof 弱网场景：验证 `ParseMoof()` 返回 `ERROR_AGAIN` 时，异步读线程不退出。
- FFmpeg/MPEG4 init/seek 弱网场景：分别验证初始化、seek、读帧三类 invoker/路径下错误码和恢复语义。
