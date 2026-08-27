# Surface 的生命周期管理

## 一、整体架构

Surface 是解码器和 RS（Render Service）之间的帧传输通道，本质上是一个生产者-消费者模型：

```
┌──────────┐                     ┌──────────────┐                     ┌──────────┐
│  解码器   │ ──── sptr<Surface> ──→ │  Surface     │ ←──── sptr<Surface> ──── │   RS     │
│ (生产者)  │    持有 ProducerSurface │ (BufferQueue)│     持有 ConsumerSurface  │ (消费者)  │
└──────────┘                     └──────────────┘                     └──────────┘
```

- 解码器持有 **ProducerSurface**（生产者端），通过它填帧
- RS 持有 **ConsumerSurface**（消费者端），通过它取帧显示
- 两端共享同一个 **BufferQueue**，由 BufferQueue 管理 buffer 的流转和状态

## 二、Surface 生命周期总览

Surface 的生命周期与解码器的状态机绑定，经历以下阶段：

```
创建/传入 ──→ 注册监听 ──→ 分配Buffer ──→ 帧循环 ──→ 清理缓存 ──→ 释放
  (1)          (2)          (3)          (4)         (5)          (6)
```

### 生命周期全景图

```
App 调用 SetOutputSurface(surface)
    │
    ▼
┌─ 1. Surface 传入 ──────────────────────────────────────────────────────────────┐
│  CodecServer → codecBase_->SetOutputSurface(surface)                          │
│  解码器存储 sptr<Surface>（引用计数+1）                                        │
│    [SW解码器] sInfo_.surface = surface                                         │
│    [HW解码器] currSurface_ = SurfaceItem(surface, ...)                         │
└───────────────────────────────────────────────────────────────────────────────┘
    │
    ▼
┌─ 2. 注册 Release 监听 ───────────────────────────────────────────────────────┐
│  SurfaceTools::RegisterReleaseListener(instanceId, surface, callback, type)   │
│    ├ surface->RegisterReleaseListener(callback)   // 注册回调                  │
│    ├ surface->SetSurfaceSourceType(VIDEO)         // 标记为视频源              │
│    └ surfaceProducerMap_[surfaceId] = instanceId  // 记录归属关系              │
│                                                                                │
│  回调作用：当消费者(RS)用完一帧释放时，通知解码器回收该 buffer                  │
│    [SW解码器] OnReleaseFunc → BufferReleasedByConsumer() → 请求新buffer       │
│    [HW解码器] OnReleaseFuncWithSequenceAndFence → GET_BUFFER_FROM_SURFACE     │
└───────────────────────────────────────────────────────────────────────────────┘
    │
    ▼
┌─ 3. 分配 Buffer ─────────────────────────────────────────────────────────────┐
│  Connect() → CleanCache() → SetQueueSize(N) → [Disconnect()]                 │
│                                                                                │
│  为每个 buffer slot:                                                          │
│    SurfaceBuffer::Create() + Alloc()   // 分配 SurfaceBuffer 内存             │
│    AttachBufferToQueue(surfaceBuffer)  // 挂载到 BufferQueue                   │
│                                                                                │
│  BufferQueue 内部状态：                                                        │
│    bufferQueueCache_[seq] = { SurfaceBuffer, state=ATTACHED }                 │
│    freeList_  = [所有已分配的 seq]     // 可被生产者请求的 buffer 列表          │
│    dirtyList_ = []                    // 已 Flush 等待消费者 Acquire 的列表     │
└───────────────────────────────────────────────────────────────────────────────┘
    │
    ▼
┌─ 4. 帧循环（正常工作阶段）──────────────────────────────────────────────────┐
│                                                                                │
│  生产者端 (解码器):                                                           │
│    RequestBuffer() → state=REQUESTED → 填充像素 → FlushBuffer() → state=FLUSHED│
│                                           加入 dirtyList_                     │
│                                                                                │
│  消费者端 (RS):                                                               │
│    AcquireBuffer() → 从 dirtyList_ 取出 → state=ACQUIRED → 送显示             │
│    ReleaseBuffer() → 归还 → state=RELEASED → 加入 freeList_                   │
│                     → 触发 OnReleaseFunc 回调 → 解码器回收 buffer             │
│                                                                                │
│  循环: freeList_ → RequestBuffer → 填充 → FlushBuffer → dirtyList_           │
│       → AcquireBuffer → 显示 → ReleaseBuffer → freeList_                     │
└───────────────────────────────────────────────────────────────────────────────┘
    │
    ▼
┌─ 5. 清理缓存 ───────────────────────────────────────────────────────────────┐
│  根据 stop/release 的场景选择：                                               │
│                                                                                │
│  Stop 时:                                                                     │
│    pushBlankBufferOnShutdown_=true  → CleanCache(true)  → RS收OnGoBackground  │
│    pushBlankBufferOnShutdown_=false → CleanCache(false) → RS收OnCleanCache    │
│                                                                                │
│  Release 时:                                                                  │
│    ReleaseSurface(instanceId, surface, cleanAll, abadon=true)                 │
│    ├ CleanCache(cleanAll)                                                      │
│    ├ UnRegisterReleaseListener()     // 注销回调                               │
│    ├ SetSurfaceSourceType(DEFAULT)   // 重置源类型                             │
│    └ surfaceProducerMap_.erase(id)   // 移除归属记录                           │
└───────────────────────────────────────────────────────────────────────────────┘
    │
    ▼
┌─ 6. 释放 Surface ──────────────────────────────────────────────────────────┐
│  解码器置空自己的 sptr<Surface>：                                              │
│    [SW解码器] sInfo_.surface = nullptr                                        │
│    [HW解码器] currSurface_.Release() → surface_ = nullptr                     │
│                                                                                │
│  CodecServer 也释放：                                                         │
│    codecBase_ = nullptr  → 解码器对象销毁 → 其持有的 sptr 全部释放            │
│                                                                                │
│  当所有 sptr<Surface> 引用计数归零 → Surface 对象被销毁                       │
└───────────────────────────────────────────────────────────────────────────────┘
```

## 三、各阶段详解

### 3.1 Surface 传入

App 层通过 CodecServer 将 Surface 传给解码器：

```cpp
// CodecServer::SetOutputSurface
int32_t CodecServer::SetOutputSurface(sptr<Surface> surface)
{
    ret = codecBase_->SetOutputSurface(surface);
    surfaceId_ = surface->GetUniqueId();    // 只记 ID，不持 sptr
    isSurfaceMode_ = (ret == AVCS_ERR_OK);
}
```

解码器内部存储方式：

| 解码器 | 存储位置 | 类型 |
|---|---|---|
| SW 解码器 | `sInfo_.surface` | `SurfaceControl::surface` (`sptr<Surface>`) |
| HW 解码器 | `currSurface_.surface_` | `SurfaceItem::surface_` (`sptr<Surface>`) |

**关键点**：CodecServer 只记录 `surfaceId_`（uint64_t），不直接持有 `sptr<Surface>`。需要时通过 `SurfaceUtils::GetInstance()->GetSurface(surfaceId_)` 重新获取。解码器是 `sptr<Surface>` 的实际持有者。

### 3.2 注册 Release 监听

这是 Surface 生命周期的关键机制——**buffer 回收通知**。

```cpp
// SurfaceTools::RegisterReleaseListener
bool SurfaceTools::RegisterReleaseListener(int32_t instanceId, sptr<Surface> surface,
    OnReleaseFunc callback, OHSurfaceSource type)
{
    surface->RegisterReleaseListener(callback);      // 注册到 Surface
    surface->SetSurfaceSourceType(type);             // 标记为 VIDEO 源
    surfaceProducerMap_[surfaceId] = instanceId;     // 记录归属
}
```

**为什么要注册**：解码器把一帧 FlushBuffer 给 Surface 后，就失去了对 buffer 的直接控制。当 RS 消费完这帧调用 `ReleaseBuffer` 时，Surface 通过这个回调通知解码器"这个 buffer 可以回收了"，解码器才能重新用它解码下一帧。

```
解码器 ──FlushBuffer──→ Surface ──AcquireBuffer──→ RS 显示
                                              │
                         ReleaseBuffer ←──────┘ (RS 用完了)
                                              │
                         OnReleaseFunc ──────→ 解码器回收 buffer
```

两种回调类型的区别：

| 类型 | 使用者 | 参数 | 作用 |
|---|---|---|---|
| `OnReleaseFunc` | SW 解码器 | `sptr<SurfaceBuffer>&` | 直接拿到 buffer，请求新 buffer 入队 |
| `OnReleaseFuncWithSequenceAndFence` | HW 解码器 | `uint32_t sequence, sptr<SyncFence>` | 拿到序号+fence，等 fence 后异步通知 OMX |

### 3.3 分配 Buffer

在解码器 Start 时，需要为 Surface 分配指定数量的 buffer slot：

```
步骤 1: Connect()        // 建立生产者连接（CleanCache 需要先 Connect）
步骤 2: CleanCache()     // 清空旧缓存
步骤 3: SetQueueSize(N)  // 设置 buffer 数量
步骤 4: [Disconnect()]   // SW解码器断开（后续通过 AttachBuffer 重新连）
步骤 5: 逐个分配 SurfaceBuffer 并挂载到队列
```

**Connect / Disconnect 的作用**：

- `Connect()` 让当前进程成为 Surface 的生产者，之后才能调用 `CleanCache()`
- SW 解码器用完就 `Disconnect()`，后续通过 `AttachBufferToQueue` 挂载 buffer
- HW 解码器 Connect 后不断开，保持生产者连接

Buffer 挂载方式：

```cpp
// SW 解码器
surface->AttachBufferToQueue(surfaceBuffer);   // 把预分配的 buffer 挂到 BufferQueue

// HW 解码器（LPP 模式）
currSurface_.surface_->AttachBufferToQueue(surfaceBuffer);
```

### 3.4 帧循环

这是 Surface 正常工作时的核心循环，涉及 BufferQueue 内部的 buffer 状态流转：

```
                    生产者操作                    消费者操作
                    ─────────                    ─────────
  RELEASED ──── RequestBuffer() ────→ REQUESTED
      ↑                                       │
      │                                  FlushBuffer()
      │                                       │
      │                                       ▼
  freeList_                              FLUSHED (在 dirtyList_ 中)
      ↑                                       │
      │                                  AcquireBuffer()
      │                                       │
  ReleaseBuffer()                              ▼
      │                                   ACQUIRED (RS 持有，正在显示)
      │                                       │
      └───────────────────────────────────────┘
```

BufferQueue 内部的三个关键数据结构：

| 数据结构 | 内容 | 作用 |
|---|---|---|
| `bufferQueueCache_` | `map<seq, BufferElement>` | 所有 buffer 的元信息（sptr、状态、fence 等） |
| `freeList_` | `list<seq>` | 可被生产者 RequestBuffer 的 buffer 序列号 |
| `dirtyList_` | `list<seq>` | 已 Flush 等待消费者 AcquireBuffer 的序列号 |

解码器侧的 buffer 归属追踪：

| 解码器 | 追踪方式 | 状态枚举 |
|---|---|---|
| SW 解码器 | `CodecBuffer::owner_` | `OWNED_BY_CODEC`, `OWNED_BY_USER`, `OWNED_BY_SURFACE` |
| HW 解码器 | `BufferInfo::owner` | `OWNED_BY_US`, `OWNED_BY_USER`, `OWNED_BY_OMX`, `OWNED_BY_SURFACE` |

### 3.5 清理缓存

解码器停止时需要清理 Surface 缓存，但清理方式取决于意图：

#### Stop 场景

```cpp
// CodecServer::Stop
if (pushBlankBufferOnShutdown_) {
    // 需要黑屏（如用户主动 Stop）
    SurfaceTools::CleanCache(instanceId_, surface, true);   // → OnGoBackground → RS 清掉 buffer_
} else {
    // 不需要黑屏（如 EOS 正常结束）
    SurfaceTools::CleanCache(instanceId_, surface, false);  // → OnCleanCache → RS 保留 buffer_
}
```

#### Release 场景

```cpp
// CodecServer::Release
SurfaceTools::ReleaseSurface(instanceId_, surface, pushBlankBufferOnShutdown_, true);
```

`ReleaseSurface` 比 `CleanCache` 做了更多事情：

```
CleanCache(cleanAll):        ReleaseSurface(instanceId, surface, cleanAll, abadon):
  ├ CleanCache(cleanAll)       ├ CleanCache(cleanAll)
                                ├ UnRegisterReleaseListener()    // 注销回收回调
                                ├ SetSurfaceSourceType(DEFAULT)  // 重置源类型
                                └ surfaceProducerMap_.erase(id)  // 移除归属记录 (abadon=true)
```

#### HW 解码器的 OnClearBufferPool

在 OMX 状态切换回 Loaded 时触发：

```cpp
// HDecoder::OnClearBufferPool
void HDecoder::OnClearBufferPool(OMX_DIRTYPE portIndex)
{
    if ((portIndex == OMX_DirOutput) && currSurface_.surface_) {
        SurfaceTools::CleanCache(instanceId_, currSurface_.surface_, false);
    }
}
```

这里用的是 `CleanCache(false)`，因为此时只是清理 buffer pool，不是完全释放 Surface。

### 3.6 释放 Surface

释放时，解码器置空自己持有的 `sptr<Surface>`：

#### SW 解码器

```cpp
void VideoDecoder::ReleaseResource()
{
    if (sInfo_.surface != nullptr) {
        sInfo_.surface->SetTransform(GRAPHIC_ROTATE_NONE);  // 恢复初始 transform
        UnRegisterListenerToSurface(sInfo_.surface);         // 注销监听
        StopRequestSurfaceBufferThread();
    }
    sInfo_.surface = nullptr;   // 释放 sptr<Surface>
}
```

#### HW 解码器

```cpp
void HDecoder::SurfaceItem::Release(bool cleanAll)
{
    if (surface_) {
        surface_->SetTransform(originalTransform_);                     // 恢复初始 transform
        SurfaceTools::ReleaseSurface(instanceId_, surface_, cleanAll); // 清理+注销
        surface_ = nullptr;                                             // 释放 sptr<Surface>
    }
}
```

#### CodecServer

```cpp
int32_t CodecServer::Release()
{
    int32_t ret = codecBase_->Release();  // 触发上述解码器释放流程

    if (isSurfaceMode_ && codecType_ == AVCODEC_TYPE_VIDEO_DECODER) {
        SurfaceTools::ReleaseSurface(instanceId_, surface, pushBlankBufferOnShutdown_, true);
        // abadon=true: 从 surfaceProducerMap_ 中移除
    }

    codecBase_ = nullptr;  // 解码器对象销毁，其持有的所有 sptr 全部释放
}
```

**Surface 对象何时被销毁**：当所有持有 `sptr<Surface>` 的引用计数归零时（解码器的 + CodecServer 间接获取的 + SurfaceUtils 缓存的）。这是 OpenHarmony sptr 智能指针的标准行为。

## 四、Surface 动态切换

Surface 支持在解码器运行中动态切换（如画中画、窗口迁移）：

```
新 Surface 注册 → 旧 Surface 清理 → Buffer 迁移 → 帧恢复
```

### SW 解码器切换流程

```cpp
// RenderSurface::SwitchBetweenSurface
newSurface->Connect();                    // 连接新 Surface
newSurface->CleanCache();                 // 清空新 Surface
newSurface->Disconnect();                 // 断开（后续通过 AttachBuffer 重连）

// 将所有 output buffer 挂载到新 Surface
for (buffer : outputBuffers) {
    newSurface->AttachBufferToQueue(buffer);
}

sInfo_.surface = newSurface;              // 切换到新 Surface

// 将 owned-by-surface 的 buffer Flush 到新 Surface
// ...

UnRegisterListenerToSurface(curSurface);  // 注销旧 Surface 的监听
curSurface->CleanCache(true);             // 清理旧 Surface（→ OnGoBackground → 黑屏）
```

### HW 解码器切换流程

```cpp
// HDecoder::OnSetOutputSurfaceWhenRunning
RegisterListenerToSurface(newSurface);                     // 注册新 Surface 监听
ClearSurfaceAndSetQueueSize(newSurface, outBufferCnt_);    // 清理+设置大小

// 将 buffer 挂载到新 Surface
for (bufferInfo : outputBufferPool_) {
    newSurface->AttachBufferToQueue(bufferInfo.surfaceBuffer);
}

SurfaceItem oldSurface = currSurface_;                     // 保存旧 Surface 引用
currSurface_ = SurfaceItem(newSurface, ...);               // 切换

oldSurface.Release(true);  // CleanCache(true) → OnGoBackground → 旧画面黑屏
                           // UnRegisterReleaseListener → 重置源类型
                           // surface_ = nullptr → 释放旧 sptr
```

**注意**：Surface 切换时，旧 Surface 调 `CleanCache(true)`（黑屏），新 Surface 正常接收帧。这意味着切换过程中旧画面会短暂黑屏，直到新 Surface 收到第一帧。

## 五、Surface 引用关系总结

```
┌─────────────────────────────────────────────────────────────────┐
│                        sptr<Surface> 引用关系                    │
│                                                                  │
│  App 层                                                          │
│    └─ 持有 sptr<Surface>（通过 SetOutputSurface 传入）            │
│                                                                  │
│  CodecServer                                                     │
│    └─ surfaceId_ (uint64_t，仅记 ID，不持 sptr)                   │
│    └─ 需要时通过 SurfaceUtils::GetSurface(surfaceId_) 获取        │
│                                                                  │
│  解码器                                                          │
│    ├─ [SW] sInfo_.surface (sptr<Surface>)                        │
│    └─ [HW] currSurface_.surface_ (sptr<Surface>)                 │
│                                                                  │
│  SurfaceTools                                                    │
│    └─ surfaceProducerMap_[id] = instanceId (仅记归属，不持 sptr)  │
│                                                                  │
│  SurfaceUtils                                                    │
│    └─ 内部缓存 surfaceId → sptr<Surface> 映射                    │
│                                                                  │
│  Surface 内部 (BufferQueue)                                      │
│    └─ bufferQueueCache_[seq].buffer (sptr<SurfaceBuffer>)        │
│       每个 buffer slot 持有一个 SurfaceBuffer 的引用              │
│                                                                  │
│  RS (ConsumerSurface)                                            │
│    └─ RSSurfaceHandler::buffer_.buffer (sptr<SurfaceBuffer>)     │
│       当前正在显示的帧的引用                                      │
│    └─ RSSurfaceHandler::preBuffer_.buffer (sptr<SurfaceBuffer>)  │
│       上一帧的引用                                                │
└─────────────────────────────────────────────────────────────────┘
```

**关键区别**：`sptr<Surface>` 是对 Surface 对象本身的引用，`sptr<SurfaceBuffer>` 是对 Surface 内部某个 buffer 帧数据的引用。Surface 的生命周期由 `sptr<Surface>` 的引用计数决定，帧数据的生命周期由 `sptr<SurfaceBuffer>` 的引用计数决定。

## 六、生命周期中的关键接口

| 接口 | 作用 | 调用时机 |
|---|---|---|
| `RegisterReleaseListener` | 注册 buffer 回收回调 | SetOutputSurface 时 |
| `UnRegisterReleaseListener` | 注销 buffer 回收回调 | Release 时 |
| `Connect` | 建立生产者连接 | 分配/切换 Surface 前 |
| `Disconnect` | 断开生产者连接 | SW 解码器分配完成后 |
| `AttachBufferToQueue` | 将预分配的 buffer 挂到队列 | Start 分配 buffer 时 |
| `RequestBuffer` | 从 freeList_ 请求一个 buffer | 每次解码前 |
| `FlushBuffer` | 将填好的帧放入 dirtyList_ | 解码完一帧后 |
| `AcquireBuffer` | 从 dirtyList_ 取出一帧 | RS 消费帧时 |
| `ReleaseBuffer` | 归还 buffer 到 freeList_ | RS 显示完后 |
| `CleanCache(false)` | 清队列但保留当前帧 | EOS 等需保留最后一帧时 |
| `CleanCache(true)` | 全清理，画面变黑 | 用户 Stop / 异常退出时 |
| `SetSurfaceSourceType` | 标记/重置 Surface 来源类型 | 注册/释放时 |
| `SetQueueSize` | 设置 buffer slot 数量 | Start 分配 buffer 时 |
