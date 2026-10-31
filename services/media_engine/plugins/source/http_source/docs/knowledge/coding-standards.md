# 编码铁律

## 通用编码铁律

- **禁止**在下载回调中执行阻塞操作，采用异步投递到 Task 线程处理（SaveData 仅写缓冲区，不等待播放线程）
- **禁止**在多线程场景下直接读写共享缓冲区，采用 Mutex + ConditionVariable 保护（RingBuffer 用 cvReadWrite_，CacheBuffer 用 mutex_）
- **禁止**在 HttpSourcePlugin::Read 中持有锁的同时调用 MediaDownloader 的阻塞方法，采用先查询再读取的模式
- **禁止**硬编码缓冲区大小常量，采用 PlayStrategy 配置或 HlsSegmentManager 的 MIN/MAX_BUFFER_SIZE 常量
- **禁止**在日志中输出完整 URL（含查询参数可能含 Token），采用脱敏处理或仅输出域名路径部分
- **禁止**忽略 DownloadRequest 的错误码，采用检查 clientError_/serverError_ 并走重试或上报流程
- **禁止**在析构函数中执行耗时操作，采用 Stop → 异步 Close → 等待线程退出

## 安全编码铁律

- **禁止**信任从 libcurl 回调中获取的 HTTP 响应数据长度，采用按实际写入长度校验
- **禁止**将未校验的 URL 直接传递给 HTTP 请求，采用 IsAllowedProtocol 黑名单校验（阻止 file:// 等本地协议）
- **禁止**在 AES 解密中泄露密钥或 IV 到日志，采用仅输出密钥索引或长度
- **禁止**使用 -fno-stack-protector，采用 -fstack-protector-all 编译选项
- **禁止**在符号导出版本脚本（.map 文件）中导出内部符号，采用仅导出插件注册接口
- **禁止**对 int64 整型字段做无守卫算术（加减乘、类型转换），采用 safe_arithmetic.h 的 SafeAdd/SafeSub/SafeMul/SafeIntCast，溢出时返回 false 并由调用方决定降级路径
- **禁止**在 SafeIntCast 中直接比较 `src < numeric_limits<Dst>::min()`，当 Src 无符号、Dst 有符号时 min() 负数会隐式提升为大无符号值，使有效范围内的 src 恒触发溢出误报，采用按 `is_signed_v` 分支派到 SafeIntCastSameSign/SignedToUnsigned/UnsignedToSigned 三个 helper
- **禁止**函数内部多种错误路径返回不同哨兵值（如越界返 -1、溢出返 INT64_MAX），调用方用单一 `< 0` 守卫会漏检正值哨兵，采用统一错误哨兵（GetMediaSegmentNumberSeq/GetTimelineSegmentNumberSeq 越界与溢出均返 -1）
- **禁止**把 DASH/HLS 内部纳秒时长（int64_t）直接传入期望毫秒的上层接口，采用显式 `/ MS_2_NS` 缩放（如 NotifyDateRanges 的 duration/start 转 AVTimedMetaData 时）

## 时长单位铁律

### 内部媒体时长统一为纳秒（int64_t/uint64_t ns）

以下内部字段一律纳秒，禁止新增毫秒/秒级媒体时长字段：

| 模块 | 字段 | 类型 | 单位 | 定义位置 |
| --- | --- | --- | --- | --- |
| DASH | `DashSegment::duration_` | uint64_t | ns | dash/dash_common.h:94 |
| DASH | `DashStreamDescription::duration_` | uint64_t | ns | dash/dash_common.h:212 |
| DASH | `DashDurationSegInfo::segDurationNs/totalDuration` | uint64_t | ns | dash/dash_common.h:172-175 |
| DASH | `DashTimelineEntry::startTime/duration` | uint64_t | timescale→ns（解析时 ×S_2_NS） | dash/dash_common.h:161-162 |
| DASH | `SidxEntry::durationNs` | uint64_t | ns | dash/dash_common.h:182 |
| DASH | `SEGMENT_DURATION_DELTA_NS` | uint64_t | ns | dash/dash_mpd_downloader.cpp:47 |
| HLS | `M3U8Segment::duration_` | uint64_t | ns | hls/m3u8.h:52 |
| HLS | `PlayInfo::duration_` | uint64_t | ns | hls/playlist_downloader.h:37 |
| HLS | `DateRange::duration/start` | int64_t | ns | hls/hls_playlist_downloader.cpp:632 |
| HLS | `M3U8::GetDuration()` | uint64_t | ns | hls/m3u8.h:103 |
| 公共 | `MIN_SEGMENT_DURATION_NS` | uint64_t | ns | utils/time_utils.h:57 |

### 对外接口单位清单（跨边界须显式缩放）

| 接口/结构体 | 字段或参数 | 期望单位 | 内部→对外缩放 | 缩放点 |
| --- | --- | --- | --- | --- |
| `MediaDownloader::GetDuration()` | 返回值 int64_t | **纳秒** | 无需缩放（直接返回内部 ns） | hls/hls_playlist_downloader.cpp:281 |
| `MediaDownloader::SeekToTime(int64_t seekTime)` | seekTime | **纳秒** | 无需缩放（入参即 ns） | media_downloader.h:94 |
| `AVTimedMetaData::duration/start` | int64_t | **毫秒** | 内部 ns `/ MS_2_NS` | hls/hls_playlist_downloader.cpp:633-634；dash/dash_media_downloader.cpp:1292-1293 |
| `M3U8::GetLiveUpdateGap()` 除法结果 | size_t | **微秒** | 内部 ns `/ US_2_NS` | hls/hls_playlist_downloader.cpp:1110 |
| `SourceStatisticsReportInfo::playStrategyDuration_` | uint32_t | **毫秒**（来自 PlayStrategy，非内部 ns） | — | dfx/avcodec_sysevent.h:110 |
| Sniffer `timeoutMs`（SniffResult.elapsedMs/decisionMs/firstDataMs/cleanupMs/remainingMs） | int32_t/int64_t | **毫秒**（墙钟耗时，非媒体时长） | — | sniffer/sniffer_coordinator.h:51-84 |

### 内部"毫秒"字段白名单（不纳入 ns 统一）

以下字段是**墙钟统计耗时**或**超时参数**，并非媒体时长，保持毫秒：

| 字段 | 单位 | 来源 | 说明 |
| --- | --- | --- | --- |
| `HlsSegmentManager::downloadDuringTime_` | ms | `steadyClock_.ElapsedMilliseconds()` 差值 | 累计有效下载墙钟耗时 |
| `HttpMediaDownloader::downloadDuringTime_` | ms | 同上 | HTTP 渐进下载墙钟耗时 |
| `DashSegmentDownloader::downloadDuringTime_` | ms | 同上 | DASH 分片下载墙钟耗时 |
| `totalDownloadDuringTime_`（各处） | ms | 累加上述 | 总下载墙钟耗时 |
| Sniffer 各 `*Ms` 字段 | ms | `ElapsedMilliseconds()` | 嗅探决策墙钟耗时 |
| `DownloadRequest`/`Downloader::Download` 的 `waitMs/timeoutMs` | ms | 调用方传入 | 网络/等待超时参数 |

### 单位换算铁律

- **秒→纳秒必须用 SafeMul**：MPD 中秒级 duration 乘 S_2_NS 时用 `SafeMul(sec, S_2_NS, mulRes)` 守护 int64 溢出（dash_mpd_downloader.cpp 多处）
- **ns→ms 须显式 `/ MS_2_NS`**：内部 ns → AVTimedMetaData 等毫秒接口，禁止隐式截断
- **ns→μs 须显式 `/ US_2_NS`**：内部 ns → GetLiveUpdateGap 等微秒接口
- **禁止反向引入 ms→ns 隐式放大**：新增媒体时长字段直接定义 ns，不要先存 ms 再 ×MS_2_NS
- **测试夹具常量须同步缩放**：生产代码 ms→ns 后，单测中的小整数常量必须乘 MS_2_NS，否则断言差数量级或空容器 SIGSEGV

## 性能编码铁律

- **禁止**在 Read 主路径上执行同步磁盘 I/O 或同步网络请求，采用预加载 + 缓冲区策略
- **禁止**在每次 Read 调用时创建新的 DownloadRequest，采用复用或队列管理
- **禁止**在 libcurl RxBodyData 回调中分配大内存，采用预分配缓冲区或直接写入 RingBuffer/CacheBuffer
- **禁止**在 Seek 操作中重新下载已缓冲的数据，采用先检查缓冲区命中再发起网络请求

## 内存编码铁律

- **禁止**注册回调后不在析构时反注册，采用 Close/Stop 中统一清理 Downloader、DownloadRequest、Task
- **禁止**CacheMediaChunkBuffer 的 Fragment 和 Chunk 内存泄漏，采用 Clear/Reset 时释放所有 FragmentCacheBuffer 和 freeChunks_
- **禁止**AES 解密缓冲区越界，采用 singleBlock_（16字节）+ decryptBuffer_（256字节）定长缓冲，写入前校验长度
- **禁止**HlsSegmentManager 的分片下载队列无限增长，采用 backPlayList_ 容量限制和已播放分片清理
- **禁止**Downloader 的请求队列无限增长，采用 BlockingQueue 有界队列

## 并发编码铁律

- **禁止**跨线程访问 Downloader 的 currentRequest_ 不加锁，采用 downloadRequestMutex_ 保护
- **禁止**在 Hold 住 mutex_ 的同时调用阻塞等待，采用 ConditionVariable 的 wait/wait_for 并在回调中重新检查条件
- **禁止**DashSegmentDownloader 的 segmentList_ 跨线程无锁访问，采用 segmentMutex_ 保护
- **禁止**HlsPlayListDownloader 的 master_ 和 currentVariant_ 跨线程无锁访问，采用 masterMutex_ 和 currentVariantMutex_ 保护
