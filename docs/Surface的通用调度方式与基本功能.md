# Surface 的通用调度方式与基本功能

## 一、Surface 是什么

Surface 是 OpenHarmony 图形子系统提供的**跨进程帧缓冲传输通道**，本质是一个生产者-消费者队列。

```
┌──────────┐                       ┌──────────────┐                       ┌──────────┐
│  生产者   │     ProducerSurface   │  BufferQueue  │    ConsumerSurface    │  消费者   │
│ (填帧端)  │ ─────────────────────→│   (核心队列)  │←───────────────────── │ (取帧端)  │
└──────────┘    运行在生产者进程     └──────────────┘   运行在消费者进程      └──────────┘
```

- **BufferQueue** 是核心，管理 buffer 的分配、流转、回收
- **ProducerSurface** 是生产者端代理，通过 IPC（IBufferProducer）与 BufferQueue 交互
- **ConsumerSurface** 是消费者端代理，直接持有 BufferQueue 的引用

## 二、Surface 的创建

Surface 有两种创建方式，分别对应消费者端和生产者端：

```cpp
// 1. 消费者端创建（同时创建 BufferQueue）
sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("mySurface");
// 内部: new BufferQueue(name) → new BufferQueueProducer(queue) → new BufferQueueConsumer(queue)

// 2. 生产者端创建（从消费者端获取 IBufferProducer 代理）
sptr<IBufferProducer> producer = consumerSurface->GetProducer();  // 跨进程传递
sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);
// 内部: new ProducerSurface(producer) → 通过 IPC 与 BufferQueue 通信
```

**创建关系图**：

```
消费者进程                                    生产者进程
───────────                                  ──────────
ConsumerSurface::Create("mySurface")
    │
    ├─ BufferQueue("mySurface")         ←── 核心对象，管理所有 buffer
    │     ├─ BufferQueueProducer        ←── Stub（服务端桩），暴露给生产者
    │     └─ BufferQueueConsumer        ←── 代理，消费者直接用
    │
    └─ consumerSurface->GetProducer()  ←── 返回 BufferQueueProducer (stub) 的引用
         │
         │  (跨进程传递 IBufferProducer——IPC 框架将 stub 序列化为 handle)
         │
         ▼
Surface::CreateSurfaceAsProducer(producer)
    └─ ProducerSurface(producer)        ←── 持有 BpBufferProducer (proxy)，通过 IPC 调用 stub
```

### 2.1 一个 consumer 可以被多个进程引用

`GetProducer()` 返回的是 `BufferQueueProducer`（stub）的引用。当它通过 IPC 传递到其他进程时，IPC 框架会在每个接收进程中创建一个 **BpBufferProducer（proxy）**，都指向消费者进程中的**同一个 stub**。因此，**多个进程可以各自创建自己的 ProducerSurface**，它们都对应同一个 ConsumerSurface：

```
                         消费者进程
                    ┌────────────────────────┐
                    │ BufferQueueProducer    │  ←── 唯一的 stub
                    │      (connectedPid_)   │
                    └─────┬──────────┬───────┘
                          │          │
                     IPC handle    IPC handle  (同一个 handle)
                          │          │
         ┌────────────────┘          └────────────────┐
         ▼                                            ▼
   进程 A                                         进程 B
   ┌─────────────────┐                           ┌─────────────────┐
   │ BpBufferProducer │                           │ BpBufferProducer │
   │  (proxy A)       │                           │  (proxy B)       │
   └────────┬─────────┘                           └────────┬─────────┘
            │                                               │
            ▼                                               ▼
   ProducerSurface-A                               ProducerSurface-B
   (创建成功 ✓)                                     (创建成功 ✓)
```

**但创建成功不等于可以生产帧**——Connect 的排他机制确保同一时刻只有一个进程能写帧（见第六节）。

> **IPC 模型要点**：Stub 是服务端对象，Proxy 是客户端句柄。一个 stub 可以被多个进程持有 proxy 引用，这是 Binder 类 IPC 的基本能力。每个进程内部，`ProcessSkeleton` 缓存了 `descriptor → proxy` 的映射（`insert_or_assign`），保证同一进程内同一个 stub 只有一个 proxy；但不同进程各自有独立的 `ProcessSkeleton` 实例，各自创建自己的 proxy 对象。

## 三、BufferQueue 内部结构

BufferQueue 是 Surface 的核心，维护三份数据：

### 3.1 BufferQueueCache — 所有 buffer 的元信息

```cpp
// bufferQueueCache_ 是一个 map，key 是 buffer 的序列号
std::map<uint32_t, BufferElement> bufferQueueCache_;

struct BufferElement {
    sptr<SurfaceBuffer> buffer;     // buffer 本体（像素数据的引用）
    BufferState state;              // 当前状态
    bool isDeleting;                // 是否标记删除
    BufferRequestConfig config;     // 分配时的配置（宽高、格式、usage）
    sptr<SyncFence> fence;          // 同步栅栏
    int64_t timestamp;              // 帧时间戳
    std::vector<Rect> damages;      // 脏区
    ...
};
```

### 3.2 freeList — 空闲 buffer 列表

```cpp
std::list<uint32_t> freeList_;  // 可被生产者 RequestBuffer 的 buffer 序列号
```

### 3.3 dirtyList — 已填充帧列表

```cpp
std::list<uint32_t> dirtyList_;  // 已 FlushBuffer，等待消费者 AcquireBuffer 的序列号
```

### 3.4 Buffer 状态机

一个 buffer 在生命周期中的状态流转：

```
                          生产者操作                      消费者操作
                          ─────────                      ─────────

  RELEASED ───── RequestBuffer() ────→ REQUESTED
      ↑                                         │
      │                                    填充像素数据
      │                                         │
      │                                    FlushBuffer()
      │                                         │
      │                                         ▼
   freeList_                              FLUSHED (加入 dirtyList_)
      ↑                                         │
      │                                    AcquireBuffer()
      │                                         │
  ReleaseBuffer()                               ▼
      │                                    ACQUIRED (消费者持有中)
      │                                         │
      └─────────────────────────────────────────┘
```

| 状态 | 含义 | 所在列表 |
|---|---|---|
| `RELEASED` | 空闲，可被生产者请求 | freeList_ |
| `ATTACHED` | 外部挂载，等待生产者使用 | 无（不在任何列表） |
| `REQUESTED` | 生产者正在写入 | 无（从 freeList 取出后不在任何列表） |
| `FLUSHED` | 已填充完成，等待消费 | dirtyList_ |
| `ACQUIRED` | 消费者正在使用 | 无（从 dirtyList 取出后不在任何列表） |

## 四、核心 API —— 帧循环

### 4.1 生产者端

#### RequestBuffer — 请求一个 buffer

```
RequestBuffer(config) → 遍历 freeList_ 找 config 匹配的 buffer，复用
                         如果 freeList_ 有 buffer 但 config 不匹配且队列已满 → Realloc 替换该 buffer
                         如果 freeList_ 有 buffer 但 config 不匹配且队列未满 → 不复用，走新分配
                         如果 freeList_ 为空且未达上限，则分配新 buffer
                         如果队列已满且无空闲 buffer，阻塞等待（由 config.timeout 决定）
```

#### FlushBuffer — 提交已填充的帧

```
FlushBuffer(seq, fence, config) → 状态变为 FLUSHED，加入 dirtyList_
                                   通知消费者: listener->OnBufferAvailable()
```

#### CancelBuffer — 取消已请求的 buffer

```
CancelBuffer(seq) → 状态回退为 RELEASED，归还 freeList_
                    （生产者请求了但决定不用）
```

### 4.2 消费者端

#### AcquireBuffer — 取出一帧

```
AcquireBuffer() → 从 dirtyList_ 取出最早的帧，状态变为 ACQUIRED
                  返回 sptr<SurfaceBuffer> + fence + timestamp + damages
```

#### ReleaseBuffer — 释放已消费的帧

```
ReleaseBuffer(buffer, fence) → 状态变为 RELEASED，归还 freeList_
                               通知生产者: OnReleaseFunc / OnBufferReleased
                               唤醒可能在等待 RequestBuffer 的生产者
```

### 4.3 完整帧循环

```
生产者                           BufferQueue                          消费者
──────                           ──────────                          ──────

RequestBuffer() ──────────────→ 从 freeList 取 buffer
  ←──────────── 返回 sptr<SurfaceBuffer>

填充像素数据...

FlushBuffer() ────────────────→ 加入 dirtyList_
                                OnBufferAvailable() ──────────────→ 收到通知

                                                                   AcquireBuffer() ──→ 从 dirtyList 取帧
                                                                   ←── 返回 buffer + fence
                                                                   显示/处理...

                                                                   ReleaseBuffer() ──→ 归还 freeList
                                OnBufferReleased() ←────────── 通知生产者

RequestBuffer() ──────────────→ 从 freeList 取 buffer（复用刚归还的）
  ...
```

**关键点**：buffer 在队列中循环流转，不会被反复分配/释放。`RequestBuffer` 拿到的通常是上一轮 `ReleaseBuffer` 归还的同一个 buffer。

## 五、Buffer 管理机制

### 5.1 QueueSize — 队列容量

```cpp
SetQueueSize(uint32_t queueSize)  // 设置 buffer slot 数量
GetQueueSize()                     // 获取当前容量
```

- 队列最多持有 `queueSize` 个 buffer
- 当 freeList 为空且已用 buffer 数 < queueSize 时，`RequestBuffer` 会触发新 buffer 分配
- 当已用 buffer 数 = queueSize 时，`RequestBuffer` 阻塞等待消费者释放

### 5.2 BufferQueue 中的 buffer 不要求一致

BufferQueue 的 freeList 中可以同时存在**不同配置**（宽高、格式、usage）的 buffer。每个 buffer 在 `bufferQueueCache_` 中记录了分配时的 `BufferRequestConfig`：

```cpp
struct BufferElement {
    sptr<SurfaceBuffer> buffer;
    BufferRequestConfig config;   // ← 每个 buffer 独立记录自己的配置
    ...
};
```

**这意味着**：QueueSize 定义的是 slot 数量上限，而不是"所有 buffer 必须同规格"。BufferQueue 是一个异构的 buffer 池。

### 5.3 RequestBuffer 遇到配置不一致时的行为

当生产者调用 `RequestBuffer(config)` 时，BufferQueue 的选择策略如下：

**第一步：优先找配置完全匹配的 buffer**

```cpp
GSError BufferQueue::PopFromFreeListLocked(sptr<SurfaceBuffer> &buffer, const BufferRequestConfig &config)
{
    // 遍历 freeList，找 config 完全一致的 buffer
    for (auto it = freeList_.begin(); it != freeList_.end(); it++) {
        if (mapIter->second.config == config) {
            buffer = mapIter->second.buffer;
            freeList_.erase(it);
            return GSERROR_OK;   // 匹配成功，直接复用
        }
    }
    // 没找到匹配的，判断是否返回一个不匹配的 buffer（走 Realloc）还是返回空（走新分配）
    if (freeList_.empty() || GetUsedSize() < bufferQueueSize_ - detachReserveSlotNum_) {
        buffer = nullptr;
        return GSERROR_NO_BUFFER;  // ← 队列未满，返回空，走新分配
    }
    // 队列已满，返回 freeList 首个不匹配的 buffer，走 Realloc
    buffer = bufferQueueCache_[freeList_.front()].buffer;
    freeList_.pop_front();
    return GSERROR_OK;
}
```

这里的关键判断是 `GetUsedSize() < bufferQueueSize_`：
- **队列未满**：返回 `NO_BUFFER`（buffer = nullptr），后续走 `SetupNewBufferLocked` → **新增**一个 buffer，不删除任何旧 buffer
- **队列已满**：返回 freeList 首个 buffer（即使 config 不匹配），后续走 `ReuseBuffer` → **Realloc** 替换该 buffer

**为什么区分？** 队列未满时，直接新增一个 buffer 比替换旧 buffer 更高效——不需要释放旧内存。只有队列满了（不能再加 slot），才需要用 Realloc "以新换旧"。

**第二步（队列未满）：新增分配——SetupNewBufferLocked**

```cpp
GSError BufferQueue::SetupNewBufferLocked(...)
{
    GSError ret = AllocBuffer(buffer, nullptr, updateConfig, lock);  // 分配新 buffer
    // → bufferQueueCache_[新 sequence] = ele
    // → 旧 buffer 不受影响
}
```

队列中 buffer 数量 +1，旧 buffer 仍在 freeList 中等待后续匹配。

**第二步（队列已满）：对取出的那一个 buffer 做重分配——Realloc**

`ReuseBuffer` 判断 `needRealloc = (config != 缓存 config)`，然后调用 `ReallocBufferLocked` **只操作从 freeList 取出的那一个 buffer**：

```cpp
GSError BufferQueue::ReallocBufferLocked(...)
{
    // 1. 读取该 buffer 的 isBufferNeedRealloc 标记，决定是否等 fence
    bool isBufferNeedRealloc = bufferQueueCache_[retval.sequence].isBufferNeedRealloc;
    if (isBufferNeedRealloc && fence != nullptr) {
        fence->Wait(3000);  // 等消费者释放该 buffer
    }

    // 2. 只删除这一个旧 buffer
    DeleteBufferInCacheNoWaitForAllocatingState(retval.sequence);
    //    → bufferQueueCache_.erase(旧 sequence)
    //    → deletingList_.push_back(旧 sequence)

    // 3. 分配一个新 buffer，加入 cache
    AllocBuffer(buffer, config);
    bufferQueueCache_[新 sequence].config = config;
}
```

**不会**遍历删除所有旧配置的 buffer。其他 buffer 仍在 freeList / dirtyList 中原封不动。

**时序总结——队列已满时逐个 Realloc**（queueSize=3，从 1080p 切到 720p）：

```
初始: freeList=[seq1(1080p), seq2(1080p), seq3(1080p)], 队列已满

第1次 RequestBuffer(720p):
  → 遍历 freeList 无匹配 → 队列已满 → 取 seq1 → Realloc → 删 seq1，分配 seq4(720p)
  → freeList=[seq2(1080p), seq3(1080p)]  ← seq2、seq3 不动

第2次 RequestBuffer(720p):
  → 遍历 freeList 无匹配 → 队列已满 → 取 seq2 → Realloc → 删 seq2，分配 seq5(720p)
  → freeList=[seq3(1080p)]  ← seq3 不动

第3次 RequestBuffer(720p):
  → 遍历 freeList 无匹配 → 队列已满 → 取 seq3 → Realloc → 删 seq3，分配 seq6(720p)
  → freeList=[]  ← 全部替换完成

此后 RequestBuffer(720p) → 直接匹配复用，不再 Realloc
```

**时序总结——队列未满时新增分配**（queueSize=5，当前 3 个 1080p buffer）：

```
初始: freeList=[seq1(1080p), seq2(1080p), seq3(1080p)], 队列未满(3/5)

第1次 RequestBuffer(720p):
  → 遍历 freeList 无匹配 → 队列未满 → 返回空 → SetupNewBufferLocked
  → 分配 seq4(720p)，旧 buffer 不动
  → freeList=[seq1(1080p), seq2(1080p), seq3(1080p)], cache 多了一个 720p

第2次 RequestBuffer(720p):
  → 遍历 freeList → 匹配 seq4(720p) → 直接复用 ✓
```

**关键**：
- 队列未满时，config 不匹配不会 Realloc，而是新增分配——更高效，旧 buffer 不浪费
- 队列已满时，config 不匹配才会 Realloc 逐个替换；过渡期间队列中同时存在新旧配置的 buffer

**第三步：特殊场景——Usage 回滚**

如果新旧 config 仅在 `rollbackableUsage_`（默认 `BUFFER_USAGE_AUXILLARY_BUFFER0`）这一项 usage 位上有差异，则不重分配，直接复用：

```cpp
bool BufferQueue::IsBufferUsageNeedRollback(const BufferRequestConfig &config, BufferRequestConfig cacheConfig)
{
    if ((config.usage ^ cacheConfig.usage) != rollbackableUsage_) {
        return false;  // 差异不只是 rollbackableUsage，需要 realloc
    }
    cacheConfig.usage = config.usage;
    return config == cacheConfig;  // 只差 rollbackableUsage，可以回滚复用
}
```

**时序总结**：

```
RequestBuffer(1920x1080, RGBA, GPU_R)
  → freeList 中找到 1920x1080 RGBA GPU_R 的 buffer → 直接复用 ✓

RequestBuffer(1280x720, RGBA, GPU_R)
  → freeList 中只有 1920x1080 的 buffer → config 不匹配
  → 取出该 buffer 做 Realloc → 删该旧 buffer，分配新 1280x720 buffer ✓
  → 其余 1080p buffer 不受影响

RequestBuffer(1920x1080, RGBA, GPU_R | AUX_BUF)
  → freeList 中有 1920x1080 RGBA GPU_R 的 buffer
  → 差异仅为 AUX_BUF 位（rollbackableUsage_） → 回滚复用，不重分配 ✓
```

### 5.4 SetBufferReallocFlag — 标记待重分配

```cpp
surface->SetBufferReallocFlag(bool flag)
```

当设为 `true` 时，遍历 `bufferQueueCache_`，将**每个** buffer 的 `isBufferNeedRealloc` 标记设为 true：

```cpp
GSError BufferQueue::SetBufferReallocFlag(bool flag)
{
    for (auto mapIter = bufferQueueCache_.begin(); mapIter != bufferQueueCache_.end(); ++mapIter) {
        mapIter->second.isBufferNeedRealloc = flag;  // 只是设标记，不释放不分配
    }
}
```

**调用后不会立即释放任何 buffer**。标记的效果体现在后续 Realloc 时：

- **标记为 true 时**：`ReallocBufferLocked` 释放旧 buffer 前会等其 fence 信号（最多 3 秒），确保消费者已用完；同时将旧 buffer 作为 `previousBuffer` 传给 `AllocBuffer`，供新分配参考
- **标记为 false 时**（默认）：Realloc 不等 fence，不传旧 buffer

**注意**：`isBufferNeedRealloc` 标记只在 Realloc 路径中被读取。如果后续 `RequestBuffer` 的 config 与旧 buffer 的 config 一致（`needRealloc = false`），不会进入 `ReallocBufferLocked`，标记不会被检查，buffer 直接复用。

**典型场景**：分辨率切换前，生产者调用 `SetBufferReallocFlag(true)`，然后以新分辨率调用 `RequestBuffer`。队列中的旧 buffer 会在帧循环中逐个被 Realloc 替换（见 5.3 时序），每次替换前等 fence 保证消费者已释放。

### 5.5 AttachBuffer / DetachBuffer — 外部挂载/卸载

先理解外部挂载解决什么问题。

**正常帧循环**中，生产者获取 buffer 的唯一途径是 `RequestBuffer`——由 BufferQueue 分配并管理 buffer 的生命周期。但有些场景下，生产者**已经自己分配好了 buffer**（如视频解码器通过 HDI/OMX 分配的 output buffer），不希望再走 RequestBuffer 的动态分配流程。挂载接口就是为这种场景设计的。

**正常帧循环 vs 挂载帧循环**：

```
正常帧循环:
  RequestBuffer → 得到 buffer（BufferQueue 分配） → 填充 → FlushBuffer
  ↑ 生产者不拥有 buffer 的分配权，由 BufferQueue 决定何时分配

挂载帧循环:
  外部分配 buffer → 挂载到队列 → 填充 → FlushBuffer
  ↑ 生产者自己分配 buffer，跳过 RequestBuffer 的分配步骤
```

BufferQueue 提供两套挂载/卸载接口，**生产者和消费者均可调用**：

```cpp
// 挂载——将外部分配的 buffer 注册到 BufferQueue
AttachBufferToQueue(sptr<SurfaceBuffer> buffer)     // 一步到位：挂入即持有
AttachBuffer(sptr<SurfaceBuffer>& buffer)           // 两步操作：先挂入，再认领

// 卸载——将 buffer 从 BufferQueue 移除
DetachBufferFromQueue(sptr<SurfaceBuffer> buffer)   // 对应 AttachBufferToQueue
DetachBuffer(sptr<SurfaceBuffer>& buffer)           // 对应 AttachBuffer
```

**两套接口的核心区别**：

| | AttachBufferToQueue（一步到位） | AttachBuffer（两步操作） |
|---|---|---|
| 挂入后的状态 | 根据调用者：生产者→REQUESTED，消费者→ACQUIRED | 统一为 ATTACHED（无论谁调用） |
| 调用者是否"持有"buffer | 是，可直接进入帧循环 | 否，需要再执行一步操作 |
| 队列满时 | 直接失败 `BUFFER_QUEUE_FULL` | 尝试淘汰 freeList/dirtyList 中的 buffer 腾位 |
| buffer 已在 cache 中 | 直接失败 `BUFFER_IS_INCACHE` | 等待该 buffer 变为 RELEASED 再改为 ATTACHED（带 timeout） |
| C API 映射 | `OH_NativeWindow_NativeWindowAttachBuffer` 内部走此接口 | 无对应 C API |

一句话总结：AttachBufferToQueue 把"挂入"和"持有"合为一步；AttachBuffer 把它们拆成两步（挂入 → 中转 → 进入循环）。

### 5.6 AttachBufferToQueue — 一步到位的挂载

`AttachBufferToQueue` 将 buffer 挂入 BufferQueue 后，**根据调用者身份直接设为可用状态**，不需要额外操作即可进入帧循环。

```cpp
GSError BufferQueue::AttachBufferToQueueLocked(sptr<SurfaceBuffer> buffer, InvokerType invokerType)
{
    if (GetUsedSize() >= bufferQueueSize_) {
        return SURFACE_ERROR_BUFFER_QUEUE_FULL;  // ← 队列满直接失败，不淘汰
    }
    if (bufferQueueCache_.find(sequence) != bufferQueueCache_.end()) {
        return SURFACE_ERROR_BUFFER_IS_INCACHE;  // ← 已在 cache 中直接失败
    }
    BufferElement ele = { .buffer = buffer, ... };
    if (invokerType == InvokerType::PRODUCER_INVOKER) {
        ele.state = BUFFER_STATE_REQUESTED;  // ← 生产者：已持有，可直接 FlushBuffer
    } else {
        ele.state = BUFFER_STATE_ACQUIRED;   // ← 消费者：已持有，可直接 ReleaseBuffer
        if (detachReserveSlotNum_ > 0) { detachReserveSlotNum_--; }
    }
    bufferQueueCache_[sequence] = ele;       // 加入缓存，但不加入 freeList
    return GSERROR_OK;
}
```

**为什么不加入 freeList？** 因为调用者已经"持有"了 buffer，buffer 应该直接进入帧循环的中间状态（REQUESTED 或 ACQUIRED），不需要再被 RequestBuffer/AcquireBuffer "取走"。

#### 生产者调 AttachBufferToQueue（av_codec 核心用法）

视频解码器最常用的模式：启动阶段将所有 output buffer 挂入队列，直接进入帧循环。

```
解码器启动阶段:
  1. 通过 HDI/OMX AllocateBuffer 分配所有 output buffer
  2. 逐个 AttachBufferToQueue(surfaceBuffer)
     → 每个 buffer 状态变为 REQUESTED，生产者直接持有

解码循环:
  3. 填充像素数据到已持有的 buffer
  4. FlushBuffer(surfaceBuffer, fence, config)   ← 直接提交，不需要 RequestBuffer
  5. 消费者 AcquireBuffer → 消费 → ReleaseBuffer → buffer 回到 freeList
  6. 下一帧：RequestBuffer 从 freeList 取回同一 buffer（复用），继续循环
```

#### 消费者调 AttachBufferToQueue

消费者侧挂载时 buffer 状态设为 ACQUIRED：

```
消费者 AttachBufferToQueue → state = ACQUIRED（消费者已持有）
    │
    ▼  消费者 ReleaseBuffer
state = RELEASED → freeList
    │
    ▼  生产者 RequestBuffer
正常帧循环
```

#### DetachBufferFromQueue — 从队列中卸载

与 AttachBufferToQueue 配对使用，按调用者身份校验当前状态：

```cpp
if (invokerType == InvokerType::PRODUCER_INVOKER) {
    if (state != BUFFER_STATE_REQUESTED)
        return SURFACE_ERROR_BUFFER_STATE_INVALID;
    bufferQueueCache_.erase(sequence);  // ← 直接从 cache 移除
} else {
    if (state != BUFFER_STATE_ACQUIRED)
        return SURFACE_ERROR_BUFFER_STATE_INVALID;
    DeleteBufferInCache(sequence, lock);
    if (isReserveSlot) { detachReserveSlotNum_++; }
}
```

#### av_codec 典型场景

**Surface 切换**（解码器换 Surface）：

```
  1. 新 Surface → Connect() → CleanCache()     ← 清空新 Surface
  2. 旧 Surface → 逐个 DetachBufferFromQueue()  ← 从旧 Surface 卸载
  3. 新 Surface → 逐个 AttachBufferToQueue()     ← 挂到新 Surface
  4. 旧 Surface → CleanCache() → Disconnect()    ← 彻底释放旧 Surface
```

**Buffer 替换**（PreprocessorManager）：

```
  1. DetachBufferFromQueue(oldBuffer)             ← 卸载旧 buffer
  2. AttachBufferToQueue(newBuffer)               ← 挂载新 buffer
  3. 第 2 步失败时 → AttachBufferToQueue(oldBuffer)  ← 回滚
```

### 5.7 AttachBuffer — 两步操作的挂载

`AttachBuffer` 将 buffer 注册到 BufferQueue，但**不赋予任何一方持有权**。无论谁调用，buffer 统一进入 **ATTACHED** 状态——已加入缓存，但不被任何一方持有。调用者需要再执行一步操作才能进入帧循环。

```cpp
GSError BufferQueue::AttachBuffer(sptr<SurfaceBuffer> &buffer, int32_t timeOut)
{
    auto mapIter = bufferQueueCache_.find(sequence);
    if (mapIter != bufferQueueCache_.end()) {
        // buffer 已在 cache 中：等它变为 RELEASED 再改为 ATTACHED（带 timeout）
        return AttachBufferUpdateStatus(lock, sequence, timeOut, mapIter);
    }
    BufferElement ele = {
        .buffer = buffer,
        .state = BUFFER_STATE_ATTACHED,   // ← 不区分调用者，统一为 ATTACHED
        .config = { .width = ..., .height = ..., ... },
    };
    if (usedSize >= queueSize) {
        // 队列满时：尝试淘汰 freeList/dirtyList 中的 buffer 腾位
        int32_t freeSize = dirtyList_.size() + freeList_.size();
        if (freeSize >= usedSize - queueSize + 1) {
            DeleteBuffersLocked(usedSize - queueSize + 1, lock);
            bufferQueueCache_[sequence] = ele;
        } else {
            return GSERROR_OUT_OF_RANGE;
        }
    } else {
        bufferQueueCache_[sequence] = ele;
    }
    return GSERROR_OK;
}
```

#### ATTACHED 状态的意义

ATTACHED 是**挂载中转站**：buffer 已在 `bufferQueueCache_` 中，但**不在 freeList 也不在 dirtyList 上**——"已注册进队列，但未被任何一方认领"。

与 REQUESTED / ACQUIRED 的根本区别：
- REQUESTED / ACQUIRED：明确表示"某一方持有这个 buffer，可以直接对它操作"
- ATTACHED：表示"buffer 在队列里，但**无人持有**"

正因为无人持有，才需要额外一步操作来让某一方"认领"它，进入正常帧循环。

#### ATTACHED buffer 如何进入帧循环

有三种路径，各有对应的状态校验支持：

```
AttachBuffer 挂入 → state = ATTACHED
    │
    ├─ ReleaseBuffer（ConsumerSurface 调用）
    │     └─→ RELEASED → freeList → 生产者 RequestBuffer → 正常帧循环
    │
    ├─ CancelBuffer（ProducerSurface 调用）
    │     └─→ RELEASED → freeList → 生产者 RequestBuffer → 正常帧循环
    │
    └─ FlushBuffer（ProducerSurface 调用）
          └─→ FLUSHED → dirtyList → 消费者 AcquireBuffer → 消费 → ReleaseBuffer → freeList
```

三种操作的状态校验均明确接受 ATTACHED：

```cpp
// ReleaseBufferLocked
if (state != BUFFER_STATE_ACQUIRED && state != BUFFER_STATE_ATTACHED)
    return SURFACE_ERROR_BUFFER_STATE_INVALID;  // ← ATTACHED 可通过

// CancelBufferLocked
if (state != BUFFER_STATE_REQUESTED && state != BUFFER_STATE_ATTACHED)
    return SURFACE_ERROR_BUFFER_STATE_INVALID;  // ← ATTACHED 可通过

// CheckBufferQueueCacheLocked（FlushBuffer 前置校验）
if (state != BUFFER_STATE_REQUESTED && state != BUFFER_STATE_ATTACHED)
    return SURFACE_ERROR_BUFFER_STATE_INVALID;  // ← ATTACHED 可通过
```

#### DetachBuffer — 与 AttachBuffer 配对的卸载

`DetachBuffer` 只接受 **REQUESTED** 或 **ACQUIRED** 状态，**不接受 ATTACHED**：

```cpp
// DetachBuffer 的状态校验
if (state == BUFFER_STATE_REQUESTED) {
    // 生产者卸载
} else if (state == BUFFER_STATE_ACQUIRED) {
    // 消费者卸载
} else {
    return GSERROR_NO_ENTRY;  // ← ATTACHED 状态不能 DetachBuffer！
}
```

如果要卸载 ATTACHED 的 buffer，必须先通过 ReleaseBuffer / CancelBuffer / FlushBuffer 让它离开 ATTACHED，等进入 RELEASED→ACQUIRED 等正常状态后才能 DetachBuffer。

### 5.8 五种状态的完整关系

| 状态 | 含义 | 在 freeList | 进入方式 | 下一步 |
|---|---|---|---|---|
| `RELEASED` | 空闲，可被 RequestBuffer 选取 | **是** | ReleaseBuffer 归还 | RequestBuffer → REQUESTED |
| `ATTACHED` | 已挂载到队列但未被任何一方持有 | **否** | `AttachBuffer` 挂入 | ReleaseBuffer(消费者) / CancelBuffer(生产者) → RELEASED；或 FlushBuffer(生产者) → FLUSHED |
| `REQUESTED` | 生产者已持有 | **否** | RequestBuffer 取走 / AttachBufferToQueue(生产者) 直接持有 | FlushBuffer → FLUSHED |
| `FLUSHED` | 已填充，等待消费 | **否** | FlushBuffer 提交 | AcquireBuffer → ACQUIRED |
| `ACQUIRED` | 消费者正在使用 | **否** | AcquireBuffer 取走 / AttachBufferToQueue(消费者) 直接持有 | ReleaseBuffer → RELEASED |

### 5.9 AttachBuffer 的 buffer 不受队列已有配置约束

`AttachBuffer` / `AttachBufferToQueue` 直接将外部 buffer 挂入队列，**不校验**该 buffer 与已有 buffer 的配置是否一致：

```cpp
// AttachBufferToQueueLocked 中，直接从 buffer 本身读取配置
BufferElement ele = {
    .buffer = buffer,
    .config = buffer->GetBufferRequestConfig(),  // ← 用 buffer 自身的宽高/格式/usage
};
bufferQueueCache_[sequence] = ele;  // 直接加入缓存
```

这意味着：
- 队列中可以同时存在 1920x1080 和 1280x720 的 buffer
- 队列中可以同时存在不同 format、不同 usage 的 buffer
- 挂载后，这些 buffer 会按自身 config 参与 `PopFromFreeListLocked` 的匹配选择

**AttachBuffer 的 buffer 会否被 Realloc？**

会。只要 buffer 进入 freeList（通过 ReleaseBuffer / CancelBuffer → RELEASED），后续 `RequestBuffer` 如果 config 不匹配，就会走 Realloc 路径替换它——与正常分配的 buffer 没有区别。

但 `PreAllocBuffers` 预分配的 buffer 有特殊标记 `isPreAllocBuffer = true`：

```cpp
// PreAllocBuffers 分配的 buffer
BufferElement ele = {
    .state = BUFFER_STATE_RELEASED,
    .config = config,
    .isPreAllocBuffer = true,  // ← 特殊标记
};
bufferQueueCache_[sequence] = ele;
freeList_.push_back(sequence);  // 直接进 freeList，不需要 CancelBuffer
```

Realloc 删除 `isPreAllocBuffer = true` 的 buffer 时，只从 cache 和 freeList 中移除，**不加入 deletingList_**（不需要通知 RenderService）：

```cpp
void BufferQueue::DeleteBufferInCacheNoWaitForAllocatingState(uint32_t sequence)
{
    if (it->second.isPreAllocBuffer) {
        bufferQueueCache_.erase(it);       // 直接删
        DeleteFreeListCacheLocked(sequence);
        return;                             // ← 不走 deletingList_
    }
    OnBufferDeleteForRS(sequence);          // 通知 RS
    bufferQueueCache_.erase(it);
    deletingList_.push_back(sequence);       // ← 普通 buffer 走这里
}
```

**AttachBuffer 挂入已有 buffer 时的特殊行为**：

```cpp
// AttachBuffer: 如果 buffer 已在 cache 中（之前 RequestBuffer 分配的）
auto mapIter = bufferQueueCache_.find(sequence);
if (mapIter != bufferQueueCache_.end()) {
    return AttachBufferUpdateStatus(lock, sequence, timeOut, mapIter);  // 不重新挂载，只改状态
}
```

如果 buffer 已经在 `bufferQueueCache_` 中（即它是队列之前分配的），`AttachBuffer` 不创建新条目，只是等其状态变为 RELEASED 后改为 ATTACHED。

**AttachBuffer 队列已满时的行为**：

```cpp
if (usedSize >= queueSize) {
    int32_t freeSize = dirtyList_.size() + freeList_.size();
    if (freeSize >= usedSize - queueSize + 1) {
        DeleteBuffersLocked(usedSize - queueSize + 1, lock);  // 淘汰旧 buffer 腾位置
        bufferQueueCache_[sequence] = ele;
    } else {
        return GSERROR_OUT_OF_RANGE;  // 没有可淘汰的，失败
    }
}
```

队列满时，如果 freeList/dirtyList 中有足够的 buffer 可淘汰，则先淘汰再挂入；否则返回 `OUT_OF_RANGE`。

### 5.10 RequestAndDetachBuffer / AttachAndFlushBuffer — 复合接口

```cpp
// 请求一个 buffer 并立即从队列中卸载（生产者完全拥有该 buffer）
surface->RequestAndDetachBuffer(buffer, fence, config)

// 将持有的 buffer 挂回队列并提交（与 AttachBufferToQueue + FlushBuffer 等价）
surface->AttachAndFlushBuffer(buffer, fence, config, needMap)
```

这对接口用于跨 Surface 的零拷贝传递：从源 Surface 取出 buffer，传递到目标 Surface 挂入并提交，避免额外的内存拷贝。

### 5.11 GetBufferCacheConfig — 查询已有 buffer 的配置

```cpp
surface->GetBufferCacheConfig(buffer, config)
```

返回指定 buffer 在 `bufferQueueCache_` 中记录的 config。生产者可在 `AttachBuffer` 前用此接口查询队列中已有 buffer 的规格，以决定是否需要按相同规格预分配。

### 5.12 Buffer 分配

`RequestBuffer` 在 freeList 为空或配置不匹配时会触发分配：

```cpp
GSError BufferQueue::AllocBuffer(sptr<SurfaceBuffer> &buffer, ...)
{
    sptr<SurfaceBuffer> bufferImpl = new SurfaceBufferImpl();  // 创建 SurfaceBuffer
    bufferImpl->Alloc(config);                                  // 分配 DMA 内存
    bufferImpl->Map();                                          // 映射到用户空间
    ioctl(bufferHandle->fd, DMA_BUF_SET_NAME_A, pid);          // 给 DMA-BUF 命名（调试用）
    bufferQueueCache_[sequence] = ele;                          // 加入缓存
}
```

分配的 buffer 在 `bufferQueueCache_` 中持续存在，直到被 Realloc 替换、队列缩容或 CleanCache 清理。

## 六、连接管理 —— Connect / Disconnect

### 6.1 核心规则：BufferQueue 在帧流转层面是 1:1 排他的

虽然多个进程可以各自创建 ProducerSurface（见 2.1 节），**但 BufferQueueProducer 在任意时刻只允许一个 producer 进程进行帧操作**。这是通过 `connectedPid_` 实现的：

```cpp
// BufferQueueProducer 内部只记录一个 PID
int32_t connectedPid_ = 0;
```

Connect/Disconnect 的本质不是"允许多个 producer 同时连接"，而是**管理"当前谁拥有写帧权限"的顺序切换**。

### 6.2 Connect — 建立排他连接

```cpp
surface->Connect()   // 建立生产者连接
```

Connect 的核心逻辑（源码简化）：

```cpp
GSError BufferQueueProducer::Connect()
{
    auto callingPid = GetCallingPid();              // IPC 框架自动获取调用者 PID
    if (connectedPid_ != 0 && connectedPid_ != callingPid) {
        return SURFACE_ERROR_CONSUMER_IS_CONNECTED; // 已有其他进程连接，拒绝
    }
    if (isDisconnectStrictly_) {
        return GSERROR_CONSUMER_DISCONNECTED;        // 严格断开状态，拒绝
    }
    SetConnectedPidLocked(callingPid);              // 记录当前连接的 PID
    return SURFACE_ERROR_OK;
}
```

**行为总结**：

| 当前状态 | 调用者 PID | 结果 |
|---|---|---|
| `connectedPid_ == 0`（无人连接） | 任意 | 成功，`connectedPid_` 设为调用者 PID |
| `connectedPid_ == 调用者 PID`（自己已连） | 同一进程 | 成功（幂等，不拒绝自己） |
| `connectedPid_ ≠ 调用者 PID`（他人已连） | 不同进程 | 失败 `CONSUMER_IS_CONNECTED` |

**自动 Connect**：`RequestBuffer()` 内部会隐式调用 `Connect()`，因此第一个调用 `RequestBuffer` 的进程会自动建立连接，后续其他进程的 `RequestBuffer` 会被拒绝。

### 6.3 CheckConnectLocked — 操作前的身份校验

所有受保护的 producer 操作（RequestBuffer、CleanCache、GoBackground 等）在执行前都会通过 `CheckConnectLocked()` 校验调用者身份：

```cpp
GSError BufferQueueProducer::CheckConnectLocked()
{
    if (connectedPid_ == 0) {
        return SURFACE_ERROR_CONSUMER_DISCONNECTED;  // 没有任何连接
    }
    if (connectedPid_ != GetCallingPid()) {
        return SURFACE_ERROR_CONSUMER_IS_CONNECTED;   // 不是当前连接者
    }
    return GSERROR_OK;
}
```

### 6.4 多进程场景的时序

```
  进程 A                                    BufferQueueProducer                          进程 B
  ──────                                    ───────────────────                          ──────

  RequestBuffer() ───→ Connect(PID_A) ───→ connectedPid_ = PID_A ───→ 成功 ✓
                          (自动 Connect)
                       RequestBuffer() ──→ 返回 buffer

                                                                                      RequestBuffer() ──→ Connect(PID_B)
                                                                                       ──→ PID_B ≠ PID_A
                                                                                       ──→ CONSUMER_IS_CONNECTED ✗

  Disconnect() ─────→ CheckConnect(PID_A) ─→ 通过
                       connectedPid_ = 0
                       CleanCache(false)

                                                                                      RequestBuffer() ──→ Connect(PID_B)
                                                                                       ──→ connectedPid_ = 0
                                                                                       ──→ connectedPid_ = PID_B ──→ 成功 ✓
```

**关键**：进程 A Disconnect 后，进程 B 才能 Connect 成功——这是**顺序交接**，不是并发。

### 6.5 Disconnect — 断开连接

```cpp
surface->Disconnect(uint32_t *bufSeqNum = nullptr)
```

- 校验调用者 PID 必须与 `connectedPid_` 匹配（只有当前连接者才能断开）
- 将 `connectedPid_` 置为 0，释放连接槽
- 调用 `CleanCache(false)`，清理缓存并保留 last_buffer 语义
- `bufSeqNum` 输出参数：返回最后一个 Flush 过的 buffer 序列号

### 6.6 ConnectStrictly / DisconnectStrictly — 严格模式

```cpp
surface->ConnectStrictly()     // 解除严格断开状态
surface->DisconnectStrictly()  // 进入严格断开状态
```

与普通 Connect/Disconnect 的区别：

| | Connect / Disconnect | ConnectStrictly / DisconnectStrictly |
|---|---|---|
| 校验方式 | 按 PID 区分调用者 | 不区分 PID，看全局标志 `isDisconnectStrictly_` |
| Disconnect 效果 | 当前连接者断开，其他进程可重新连接 | 设置全局标志，**任何进程** RequestBuffer 都失败 |
| 典型场景 | 生产者正常退出/切换 | Surface 被销毁前彻底封死，防止残留 producer 继续写帧 |

### 6.7 Producer 进程死亡时的清理

BufferQueueProducer 通过 `DeathRecipient` 监控当前连接的 producer 进程：

```cpp
void BufferQueueProducer::OnBufferProducerRemoteDied()
{
    SetConnectedPidLocked(0);          // 释放连接
    bufferQueue_->CleanCache(false);   // 清理缓存
}
```

当 producer 进程异常退出时，IPC 框架回调 `OnRemoteDied`，自动释放连接，使新 producer 可以接替。

> **注意**：`token_`（DeathRecipient 绑定的 IRemoteObject）只有一个。新 producer 注册 token 时，旧 token 的死亡监控会被替换。系统只关心"当前连接的那个 producer 有没有死"。

### 6.8 Consumer 侧不管理多个 Producer

`BufferQueueProducer`（stub 端）维护的所有状态都是**面向单个 producer** 的：

| 字段 | 类型 | 说明 |
|---|---|---|
| `connectedPid_` | `int32_t` | 只记一个 PID |
| `token_` | `sptr<IRemoteObject>` | 只跟踪一个 DeathRecipient |
| `producerListener_` | `sptr<IProducerListener>` | 只有一个 release 回调 |
| `isDisconnectStrictly_` | `bool` | 全局开关 |

**不存在任何"多个 producer proxy"的管理结构。** Consumer 侧不知道也不关心有几个 BpBufferProducer 存在——那是 IPC 框架的事。唯一有多 producer 痕迹的是 `propertyChangeListeners_`（按 `producerId` 注册的 map），但这仅用于属性变更通知（如 transformHint），不走 buffer 帧流转路径。

### 6.9 Connect 存在的意义

既然始终是 1:1，为什么需要 Connect 机制？

| 目的 | 说明 |
|---|---|
| **排他控制** | 保证同一时刻只有一个进程写帧，避免 buffer 状态混乱 |
| **顺序交接** | 进程 A Disconnect → 进程 B Connect，实现 producer 切换 |
| **访问隔离** | 防止无关进程往别人的 Surface 里写帧 |
| **死亡清理** | 监控当前 producer 的存亡，异常退出时自动释放 |
| **自动 Connect** | RequestBuffer 隐式调用 Connect，零配置即可开始写帧 |
| **Disconnect 时的 last_buffer** | 断开时通过 CleanCache(false) 保留最后一帧继续显示 |

## 七、监听回调机制

Surface 有两类回调，分别面向消费者和生产者：

### 7.1 消费者监听（IBufferConsumerListener）

消费者注册到 Surface，接收队列事件通知：

```cpp
class IBufferConsumerListener {
    virtual void OnBufferAvailable() = 0;    // 有新帧可消费（dirtyList 非空）
    virtual void OnGoBackground() {};         // 生产者退到后台
    virtual void OnCleanCache(uint32_t *bufSeqNum) {};  // 生产者清理缓存（保留当前帧语义）
    virtual void OnTunnelHandleChange() {};   // Tunnel 句柄变化
    virtual void OnTransformChange() {};      // Transform 变化
    virtual void OnDropBuffer() {};           // 有 buffer 被丢弃
};
```

注册方式：

```cpp
sptr<IBufferConsumerListener> listener = new MyListener();
consumerSurface->RegisterConsumerListener(listener);
```

**OnBufferAvailable 是最核心的回调**：生产者 `FlushBuffer` 后触发，通知消费者"有新帧了，来取"。

### 7.2 生产者 Release 监听

生产者注册，接收 buffer 被消费者释放的通知：

```cpp
// 三种回调类型，按需选择：
using OnReleaseFunc = std::function<GSError(sptr<SurfaceBuffer>&)>;
using OnReleaseFuncWithFence = std::function<GSError(const sptr<SurfaceBuffer>&, const sptr<SyncFence>&)>;
using OnReleaseFuncWithSequenceAndFence = std::function<GSError(uint32_t sequence, const sptr<SyncFence>&)>;

surface->RegisterReleaseListener(callback);
```

**触发时机**：消费者调用 `ReleaseBuffer` 后，BufferQueue 通过 IPC 回调生产者。

| 回调类型 | 参数 | 适用场景 |
|---|---|---|
| `OnReleaseFunc` | 整个 `sptr<SurfaceBuffer>` | 需要直接操作 buffer 对象 |
| `OnReleaseFuncWithFence` | buffer + release fence | 需要等 GPU 完成后再复用 |
| `OnReleaseFuncWithSequenceAndFence` | 序列号 + fence | 高性能路径，避免传递 sptr 的 IPC 开销 |

注销：

```cpp
surface->UnRegisterReleaseListener();
```

## 八、缓存清理 —— CleanCache / GoBackground

### 8.1 CleanCache

```cpp
surface->CleanCache(bool cleanAll = false)
```

清理 Surface 的所有缓存，但通过参数区分对消费者的通知方式：

**cleanAll = true（全清理）**：
- 通知消费者：`listener->OnGoBackground()` — 告诉消费者"生产者走了，全清"
- 清空 bufferQueueCache_、freeList_、dirtyList_、deletingList_
- **语义**：生产者不再活跃，消费者应释放所有资源

**cleanAll = false（保留当前帧语义）**：
- 通知消费者：`listener->OnCleanCache(&bufSeqNum)` — 告诉消费者"清缓存，但保留当前帧"
- 消费者回调中将当前帧的 seqNum 写回 `bufSeqNum`
- 调用 `MarkBufferReclaimableByIdLocked` → `ioctl(fd, DMA_BUF_SET_TYPE, "last_buffer")` — 标记 DMA-BUF 为最后一帧
- 同样清空 bufferQueueCache_、freeList_、dirtyList_、deletingList_
- **语义**：生产者走了，但当前帧请留着继续显示

**注意**：调用 `CleanCache` 前必须先 `Connect()`，否则无效。

### 8.2 GoBackground

```cpp
surface->GoBackground()
```

与 `CleanCache(true)` 效果类似，但语义更明确——生产者主动声明"我要退到后台了"：

- 通知消费者：`listener->OnGoBackground()`
- 清空所有缓存
- 通知生产者端 preCacheBuffer 置空

### 8.3 对比

| | `CleanCache(true)` | `CleanCache(false)` | `GoBackground()` |
|---|---|---|---|
| 消费者通知 | `OnGoBackground()` | `OnCleanCache(bufSeqNum)` | `OnGoBackground()` |
| 队列清理 | 全清 | 全清 | 全清 |
| DMA-BUF last_buffer 标记 | 否 | 是 | 否 |
| 生产者端 preCache | 置空 | 保留最后帧 2 轮 | 置空 |
| 语义 | 全清理，黑屏 | 清理但保留当前帧 | 生产者退到后台 |

## 九、Surface 的属性配置

Surface 提供一组属性，生产者和消费者都可以设置：

| 属性 | 设置方法 | 作用 |
|---|---|---|
| 队列容量 | `SetQueueSize(uint32_t)` | 控制 buffer slot 数量 |
| 默认宽高 | `SetDefaultWidthAndHeight(int32_t, int32_t)` | 新分配 buffer 的默认尺寸 |
| 默认 Usage | `SetDefaultUsage(uint64_t)` | buffer 的使用方式（CPU 读、GPU 写等） |
| Transform | `SetTransform(GraphicTransformType)` | 旋转/镜像变换 |
| ScalingMode | `SetScalingMode(ScalingMode)` | 缩放模式 |
| 元数据 | `SetMetaData / SetMetaDataSet` | HDR 等 per-frame 元数据 |
| Tunnel 句柄 | `SetTunnelHandle` | 直通显示路径 |
| 源类型 | `SetSurfaceSourceType(OHSurfaceSource)` | 标记来源（VIDEO / CAMERA / DEFAULT） |
| Buffer Hold | `SetBufferHold(bool)` | 消费者是否暂存当前帧不立即释放 |
| 全局 Alpha | `SetGlobalAlpha(int32_t)` | 全局透明度 |

## 十、Surface 对象的生命周期

Surface 继承自 `RefBase`，生命周期由 `sptr` 引用计数管理：

```
创建 → 使用 → 释放
```

### 10.1 ProducerSurface

```
Surface::CreateSurfaceAsProducer(producer)
    → new ProducerSurface(producer)
    → 加入 SurfaceUtils 缓存

~ProducerSurface()
    → Disconnect()                    // 断开连接
    → 从 SurfaceUtils 缓存中移除
```

### 10.2 ConsumerSurface

```
Surface::CreateSurfaceAsConsumer(name)
    → new ConsumerSurface(name)
    → new BufferQueue(name)
    → new BufferQueueProducer(queue)
    → new BufferQueueConsumer(queue)

~ConsumerSurface()
    → consumer_->OnConsumerDied()     // 通知 BufferQueue 消费者已死
    → consumer_->SetStatus(false)     // 标记队列不可用
    → consumer_ = nullptr
    → producer_ = nullptr             // 引用计数-1，可能触发 BufferQueue 销毁
```

**BufferQueue 的销毁时机**：当 BufferQueueProducer 和 BufferQueueConsumer 的引用计数都归零时（即 ConsumerSurface 被销毁），BufferQueue 随之销毁。

### 10.3 引用关系

```
                    ConsumerSurface
                    ┌─────────────┐
                    │ consumer_ ──┼──→ BufferQueueConsumer ──→ BufferQueue
                    │ producer_ ──┼──→ BufferQueueProducer ──→ BufferQueue
                    └─────────────┘        (持有 sptr<BufferQueue>)

                    ProducerSurface
                    ┌─────────────┐
                    │ producer_ ──┼──→ IBufferProducer (IPC 代理)
                    └─────────────┘        (不直接持有 BufferQueue)

                    SurfaceUtils (全局缓存)
                    ┌─────────────┐
                    │ surfaceMap_ │──→ surfaceId → sptr<Surface>
                    └─────────────┘
```

**关键点**：
- ProducerSurface 通过 IPC 与 BufferQueue 交互，不直接持有 BufferQueue 引用
- ConsumerSurface 直接持有 BufferQueue（通过 Producer/Consumer 两个代理）
- SurfaceUtils 全局缓存了所有 Surface 的引用，防止过早销毁

## 十一、跨进程通信

Surface 的生产者和消费者通常运行在不同进程（如解码器在 app 进程，RS 在 render_service 进程）。通信通过 OpenHarmony 的 IPC 机制（`IRemoteBroker`）实现。

### 11.1 Stub / Proxy 模型

OpenHarmony IPC 采用 Binder 模型，核心概念是 Stub（服务端桩）和 Proxy（客户端代理）：

```
服务端进程                                    客户端进程
───────────                                  ───────────
                                             ┌───────────────────────┐
                                             │ IPCObjectProxy         │
                                             │   handle = 5           │
                                             │   SendRequest(code,..) │
                                             └──────────┬────────────┘
                                                        │
                                                        │ IPC 驱动根据 handle=5 路由
                                                        ▼
┌───────────────────────┐                   ┌───────────────────────┐
│ IPCObjectStub          │                   │ IPC 驱动 / 内核        │
│   OnRemoteRequest()    │←──────────────────│   handle → stub 映射   │
└───────────────────────┘                   └───────────────────────┘
```

- **Stub**（`IRemoteStub`）：服务端对象，处理实际的请求逻辑。在消费者进程中，`BufferQueueProducer` 就是一个 Stub
- **Proxy**（`IRemoteProxy`）：客户端对象，将方法调用序列化后通过 IPC 驱动发送给 Stub。在生产者进程中，`BpBufferProducer` 就是 Proxy
- **Handle**：内核 IPC 驱动分配的编号，标识一个 Stub。Proxy 内部记录 handle，驱动据此将请求路由到正确的 Stub

### 11.2 一个 Stub 可以有多个 Proxy

这是 Binder 类 IPC 的基本能力，也是理解 Surface 多进程行为的关键：

```
                    进程 A: IPCObjectProxy (handle=5)
                    ┌──────────────────────────────┐
                    │  SendRequest(code, data, ...) │
                    └──────────┬───────────────────┘
                               │ IPC 驱动根据 handle=5 路由
                               ▼
            ┌──────────────────────────────────────┐
            │   服务端进程: IPCObjectStub            │
            │   (BufferQueueProducer)               │
            │   OnRemoteRequest(code, data, ...)    │
            └──────────────────────────────────────┘
                               ▲
                               │ IPC 驱动根据 handle=5 路由（同一个 handle）
                    ┌──────────┴───────────────────┐
                    │  进程 B: IPCObjectProxy (handle=5) │
                    │  SendRequest(code, data, ...)      │
                    └────────────────────────────────────┘
```

- **跨进程**：每个进程有独立的 `ProcessSkeleton` 实例和地址空间，各自创建自己的 Proxy 对象，都指向同一个 Stub
- **进程内**：`ProcessSkeleton::objects_`（`descriptor → wptr<IRemoteObject>`）使用 `insert_or_assign`，保证同一进程内同一个 Stub 只缓存一个 Proxy

### 11.3 Surface 中的 IPC 链路

将 11.1、11.2 的模型套用到 Surface：

```
  进程 A                               消费者进程                               进程 B
  ┌──────────────────┐                 ┌─────────────────────┐                ┌──────────────────┐
  │ ProducerSurface-A │                │ ConsumerSurface      │                │ ProducerSurface-B │
  │    producer_ ─────┼──→ BpBuffer    │    producer_ ────────┼── BufferQueue  │    producer_ ─────┼──→ BpBuffer
  │                   │    Producer-A  │         ↓            │   Producer     │                   │    Producer-B
  └──────────────────┘    (proxy)      │    BufferQueueProducer│    (stub)     └──────────────────┘    (proxy)
                                        │    connectedPid_     │
                                        └─────────────────────┘
                                               ↓
                                        ┌─────────────────────┐
                                        │ BufferQueue          │
                                        │  freeList / dirtyList│
                                        │  bufferQueueCache    │
                                        └─────────────────────┘
```

- Consumer 进程中只有一个 `BufferQueueProducer`（stub）
- 进程 A 和进程 B 各自有一个 `BpBufferProducer`（proxy），通过 IPC 框架路由到同一个 stub
- **两个 ProducerSurface 都能创建成功**，都能向 stub 发送 IPC 请求
- **但 Connect 的 PID 校验**确保只有第一个 Connect 的进程能通过帧操作，后来的被拒绝（见第六节）

### 11.4 IBufferProducer 定义的 IPC 接口

`IBufferProducer` 定义了所有生产者操作，这些调用都通过 IPC 从 proxy 传递到 stub：

```
RequestBuffer / CancelBuffer / FlushBuffer   ← 帧流转
AttachBuffer / DetachBuffer                  ← buffer 管理
CleanCache / GoBackground                    ← 缓存清理
Connect / Disconnect                         ← 连接管理
RegisterReleaseListener / UnRegisterReleaseListener  ← 回调注册
SetTransform / SetScalingMode / ...          ← 属性设置
```

## 十二、同步栅栏（SyncFence）

帧循环中每一步都伴随 fence，确保生产者和消费者对 buffer 的访问不冲突：

```
RequestBuffer 返回 acquireFence ──→ 生产者等 fence 信号后再写入
FlushBuffer 传入 releaseFence ──→ 消费者等 fence 信号后才能读取
ReleaseBuffer 传入 fence ──→ 生产者等 fence 信号后才能复用
```

**fence 的本质**是一个文件描述符（fd），由内核的 DMA-BUF 或 GPU 驱动设置信号。当 GPU 完成写入后，acquireFence 被 signal；当显示完成读取后，releaseFence 被 signal。

## 十三、总结

Surface 作为一个通用的帧传输通道，核心设计可归纳为：

1. **BufferQueue 是调度中心**：管理 buffer 的状态、分配、回收，三个列表（freeList / dirtyList / bufferQueueCache）驱动流转
2. **生产者-消费者解耦**：通过 IPC 隔离两端进程，通过回调（OnBufferAvailable / OnBufferReleased）实现异步通知
3. **buffer 循环复用**：buffer 在队列中循环流转，避免反复分配/释放的开销
4. **fence 同步**：确保跨进程、跨硬件（CPU/GPU/显示器）的访问安全
5. **Connect/Disconnect 实现 1:1 排他连接**：任意时刻只允许一个 producer 进程写帧，通过 `connectedPid_` 校验 PID 实现；多个进程可以各自创建 ProducerSurface（一个 stub 可有多个 proxy），但只有第一个 Connect 的进程能通过帧操作
6. **Connect 的本质是顺序交接**：进程 A Disconnect → 进程 B Connect，实现 producer 切换，而非支持多 producer 并发；同时提供死亡监控和 last_buffer 语义
7. **IPC Stub/Proxy 模型**：BufferQueueProducer 是唯一的 Stub（服务端），每个远程进程各有一个 BpBufferProducer Proxy（客户端），通过 IPC 驱动按 handle 路由到同一个 Stub；Consumer 侧不管理多个 Proxy，只跟踪一个 `connectedPid_`
8. **Buffer 异构池 + AttachBufferToQueue 预挂载**：BufferQueue 不要求所有 buffer 配置一致；RequestBuffer 队列未满时 config 不匹配走新增分配，已满时走 Realloc 逐个替换；视频解码器通过 AttachBufferToQueue 将外部分配的 buffer 直接以 REQUESTED 状态挂入，跳过 RequestBuffer 直接 FlushBuffer，实现零 RequestBuffer 的帧循环
9. **CleanCache 区分语义**：`cleanAll=true` 全清理（黑屏），`cleanAll=false` 保留当前帧
9. **引用计数管理生命周期**：Surface 继承 RefBase，sptr 归零时自动销毁
