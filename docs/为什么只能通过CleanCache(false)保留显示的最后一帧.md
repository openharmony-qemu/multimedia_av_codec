# 为什么只能通过 CleanCache(false) 保留显示的最后一帧

## 问题背景

为了支持显示最后一帧，解码器停止时调用 `CleanCache(false)` 保存 RS 正在显示的那一帧。疑问是：

- 如果 RS 持有该帧的 sptr，是不是就不需要 `CleanCache(false)` 了？
- RS 能不能自己根据收到的信号决定是否保留 buffer_？

## 一、帧显示的基本模型

```
┌──────────┐    RequestBuffer     ┌──────────────┐    AcquireBuffer    ┌──────────┐
│  解码器   │ ──填充像素数据───→  │  Surface     │ ────────────────→  │   RS     │
│ (生产者)  │    FlushBuffer      │ (BufferQueue)│                    │ (消费者)  │
└──────────┘                     └──────────────┘                     └──────────┘
```

- **解码器**是生产者，往 Surface 里填帧
- **RS**（Render Service）是消费者，从 Surface 里取帧，送屏幕合成显示
- Surface 内部是一个 BufferQueue，管理 buffer 的流转

### RS 内部怎么持有"当前正在显示的帧"

RS 有一个 `RSSurfaceHandler`，里面存了两个关键成员：

```cpp
class RSSurfaceHandler {
    SurfaceBufferEntry buffer_;     // 当前帧
    SurfaceBufferEntry preBuffer_;  // 上一帧
};

struct SurfaceBufferEntry {
    sptr<SurfaceBuffer> buffer;     // 核心：指向实际像素数据的智能指针
    sptr<SyncFence>     acquireFence;
    Rect                damageRect;
    int64_t             timestamp;
    ...
};
```

RS **渲染时只看 `buffer_.buffer` 这一个入口**。它有值则显示对应画面；它为 null 则没有内容，黑屏。

### buffer_.buffer 从哪来

RS 调用 `consumer->AcquireBuffer(returnValue)` 从 Surface 取帧，返回值里包含 `sptr<SurfaceBuffer> buffer`，然后直接赋值：

```
AcquireBuffer 返回 sptr<SurfaceBuffer>
    → 组装成 SurfaceBufferEntry
    → RSSurfaceHandler::ConsumeAndUpdateBuffer()
        → ConsumeAndUpdateBufferInner()
            → UpdateBuffer()
                → buffer_.buffer = buffer;   // 直接 sptr 赋值，指向同一个对象
```

**`buffer_.buffer` 就是从 Surface AcquireBuffer 出来的那个 sptr，是同一个 SurfaceBuffer 对象的引用。**

## 二、解码器停止时的问题

解码器停止（播放结束/用户 Stop）后，不再往 Surface 填新帧。此时需要清理 Surface 的资源，但有个矛盾：

- **要清理**：Surface 里还缓存着若干 buffer slot，解码器持有的 Surface 引用需要释放
- **要保留**：RS 正在显示的最后一帧不能丢，否则画面变黑

这就引出了 `CleanCache` 这个 API。

## 三、CleanCache(cleanAll) 做了什么

`CleanCache` 是 Surface 提供的缓存清理接口，参数 `cleanAll` 决定了两种完全不同的行为。

### 3.1 CleanCache(true) —— 全清理，画面变黑

```
解码器调用 CleanCache(true)
         │
         ▼
    ┌─────────────────────────────────────────────────┐
    │  BufferQueue 层 (Surface 服务端)                  │
    │                                                  │
    │  1. 通知 RS: listener->OnGoBackground()          │
    │                                                  │
    │  2. 清空所有队列:                                 │
    │     bufferQueueCache_.clear()                     │
    │     freeList_.clear()                             │
    │     dirtyList_.clear()                            │
    └──────────────────┬──────────────────────────────┘
                       │
                       ▼
    ┌─────────────────────────────────────────────────┐
    │  RS 收到 OnGoBackground() 后:                    │
    │                                                  │
    │  surfaceHandler->CleanCache():                    │
    │      buffer_.Reset()    → buffer_.buffer = null  │  ★ 当前帧置空
    │      preBuffer_.Reset() → preBuffer_.buffer = null│
    │                                                  │
    │  node->UpdateBufferInfo(nullptr, ...)             │  ★ 渲染信息归空
    │  node->SetContentDirty()                          │  ★ 标记需要重绘
    │                                                  │
    │  结果: GetBuffer() 返回 null → 黑屏              │
    └─────────────────────────────────────────────────┘
```

**关键**：`OnGoBackground` 让 RS 把 `buffer_.buffer` 置空了。RS 的渲染管线看不到任何帧，画面变黑。

### 3.2 CleanCache(false) —— 清理队列，但保留当前帧

```
解码器调用 CleanCache(false)
         │
         ▼
    ┌──────────────────────────────────────────────────────┐
    │  BufferQueue 层 (Surface 服务端)                       │
    │                                                       │
    │  1. 通知 RS: listener->OnCleanCache(&bufSeqNum)       │
    │                                                       │
    │  2. 标记最后一帧:                                      │
    │     MarkBufferReclaimableByIdLocked(bufSeqNum)        │
    │     → ioctl(fd, DMA_BUF_SET_TYPE, "last_buffer")     │  ★ 告诉内核这是最后一帧
    │                                                       │
    │  3. 清空所有队列:                                      │
    │     bufferQueueCache_.clear()                          │
    │     freeList_.clear()                                  │
    │     dirtyList_.clear()                                 │
    └──────────────────────┬────────────────────────────────┘
                           │
                           ▼
    ┌──────────────────────────────────────────────────────┐
    │  RS 收到 OnCleanCache(&bufSeqNum) 后:                  │
    │                                                       │
    │  1. 把当前帧的 seqNum 写回给 BufferQueue:              │
    │     *bufSeqNum = curBuffer->GetSeqNum()               │
    │     → 让 BufferQueue 知道哪帧是"最后显示帧"            │
    │                                                       │
    │  2. 只清 preBuffer，不清当前 buffer:                   │
    │     surfaceHandler->ResetPreBuffer()                   │  ★ 只清上一帧
    │     // 注意：没有调用 buffer_.Reset()                  │  ★ 当前帧保留！
    │                                                       │
    │  3. 清理硬件合成层缓存:                                 │
    │     CleanLayerBufferCache()                            │
    │                                                       │
    │  结果: GetBuffer() 仍返回最后一帧 → 画面保持            │
    └──────────────────────────────────────────────────────┘
```

### 3.3 对比总结

| | `CleanCache(true)` | `CleanCache(false)` |
|---|---|---|
| **RS 收到的信号** | `OnGoBackground()` | `OnCleanCache(bufSeqNum)` |
| **RS 对当前帧 `buffer_`** | `buffer_.Reset()` → **置空** → 黑屏 | **不动** → 最后一帧继续显示 |
| **RS 对上一帧 `preBuffer_`** | `preBuffer_.Reset()` → 清掉 | `ResetPreBuffer()` → 清掉 |
| **Surface 队列清理** | 全部清空 | 全部清空（队列都清了，结果一样） |
| **DMA-BUF "last_buffer" 标记** | 不标记 | `ioctl(DMA_BUF_SET_TYPE, "last_buffer")` |
| **硬件合成层缓存** | 不清理 | `CleanLayerBufferCache()` |
| **语义** | "生产者走了，你可以全放" | "生产者走了，但当前帧请留着继续显示" |

**核心区别就一行**：`OnCleanCache` 不会调 `buffer_.Reset()`，`OnGoBackground` 会。

## 四、为什么 RS 自己持有 sptr 不够

可能的想法：如果 RS 的其他地方也持有同一帧的 `sptr<SurfaceBuffer>`，buffer 内存就不会被释放，RS 不就能继续显示吗？

**不能。** 因为 RS 的渲染管线只认 `buffer_.buffer` 这一个入口：

```cpp
// RS 渲染时取帧的唯一入口
const sptr<SurfaceBuffer> GetBuffer() const {
    return buffer_.buffer;    // 如果这里是 null，就渲染不出任何东西
}
```

即使别的组件持有 sptr 让 buffer 内存活着，但 `OnGoBackground` 把 `buffer_.buffer` 设成了 null，渲染管线看到的仍然是"没有帧"，**画面照样黑屏**。

```
                        buffer_.buffer = nullptr  (OnGoBackground 设的)
                               │
                               ▼
    RS 渲染管线 ──GetBuffer()──→ null ──→ 黑屏

    其他组件持有的 sptr ──→ buffer 内存还活着，但没人在用
```

**sptr 保活的是内存，不是渲染管线的引用入口。** 两者是不同层面的问题。

## 五、为什么 RS 不能自己判断"要不要保留 buffer_"

假设改成：RS 收到 `OnGoBackground` 后，自己决定是否保留 `buffer_`。问题是——**RS 不知道该不该保留**：

| 场景 | 生产者(解码器)知道的 | RS 知道的 | 该不该保留？ |
|---|---|---|---|
| 播放到 EOS 结束 | 知道正常结束 | 只知道"生产者走了" | 保留 |
| Surface 动态切换 | 知道要换 surface | 只知道"生产者走了" | 保留旧帧 |
| App 退到后台 | 知道应用不可见 | 只知道"生产者走了" | 释放省内存 |
| 解码异常/出错 | 知道出错了 | 只知道"生产者走了" | 释放 |
| 用户主动 Stop | 知道用户要停 | 只知道"生产者走了" | 释放 |

同一个 `OnGoBackground` 信号，5 种场景有 2 种该保留、3 种不该保留。RS 没有区分这些场景的信息，无法做出正确决策。

**只有生产者（解码器）知道意图。** `CleanCache(false)` 就是生产者把意图编码进信号的方式：

- 调 `CleanCache(false)` → 告诉 RS "我要走了，但请保留当前帧继续显示"
- 调 `CleanCache(true)` → 告诉 RS "我要走了，请全部清掉"

## 六、完整结论

```
为什么只能通过 CleanCache(false) 保留最后一帧？

因为"保留最后一帧"不是一个内存问题，而是一个协调问题：

1. RS 渲染只看 buffer_.buffer 这一个入口
   → 谁把它设 null，谁就决定了画面变黑

2. OnGoBackground 会把 buffer_.buffer 设 null
   → 这是 RS 的既有逻辑，不能改（其他场景确实需要全清）

3. RS 自己无法判断该不该保留
   → 只有生产者知道意图

4. CleanCache(false) 通过发送 OnCleanCache（而非 OnGoBackground）
   → 让 RS 不清 buffer_，同时做好配套工作（DMA-BUF标记、硬件缓存清理）

整条链路是：
   生产者的意图（保留最后一帧）
       ↓ 编码为
   CleanCache(false)
       ↓ 产生
   OnCleanCache 信号
       ↓ 让 RS
   不调 buffer_.Reset()
       ↓ 结果
   buffer_.buffer 仍指向最后一帧 → 画面保持
```

如果把决策权交给 RS，就等于让不知情方做判断，且需要额外补上 DMA-BUF 标记和硬件缓存清理等配套逻辑——本质上是在 RS 侧重新发明一套已经由 `CleanCache(false)` 一条调用搞定的事情。
