# 缓存缓冲区管理（CacheMediaChunkBuffer）

## 解决的问题（Problem）

| 子维度 | 内容 |
| --- | -- |
| 业务痛点 | HLS/DASH 的分片数据需按 offset 随机访问（Seek），但 RingBuffer 是线性循环结构，无法高效支持按 offset 寻址和部分回收；HTTP 大文件下载也需要灵活的缓冲区管理 |
| 技术目标 | 建立 **按 offset 寻址的分块缓存系统**，支持随机读写、已读数据回收、Fragment 级别的管理，为 HTTP/HLS/DASH 提供统一的缓冲管理 |
| 适用场景 | HLS 分片数据的缓冲与 Seek；DASH 分片数据的缓冲与码率切换后缓冲清理；HTTP 大文件的分段缓存 |

## 核心概念与术语（Concepts）

| 子维度 | 内容 |
| --- | -- |
| 关键术语 | **CacheChunk**：固定大小的数据块（包含 chunkSize/dataLength/offset/data[]）；**FragmentCacheBuffer**：一组连续 Chunk 组成的逻辑片段（对应一个分片或一段数据）；**LRU 淘汰**：基于访问时间的 Fragment 淘汰策略 |
| 角色定义 | **CacheMediaChunkBufferImpl**：核心实现类，管理 Fragment 列表和空闲 Chunk 池；**CacheMediaChunkBuffer**：对外接口类（Pimpl 模式）；**CacheMediaChunkBufferHlsImpl**：HLS 专化实现，重写 GetFreeCacheChunk/SplitFragmentCacheBuffer |
| 核心对象 | fragmentCacheBuffer_(FragmentCacheBufferList)：Fragment 双向链表；freeChunks_(CacheChunkList)：空闲 Chunk 池；lruCache_(LruCache)：Fragment LRU 索引 |

## 原理（Principles）

| 子维度 | 内容 |
| --- | -- |
| 核心机制 | **Fragment + Chunk 二级结构**：数据按 Fragment 组织（对应一个下载分片），每个 Fragment 内按 Chunk 划分（固定大小）。空闲 Chunk 池预分配，写入时从池中获取 Chunk 挂到 Fragment 下，读取时按 offset 查找 Fragment → 查找 Chunk |
| 架构组成 | CacheMediaChunkBufferImpl 维护：fragmentCacheBuffer_(Fragment链表)、freeChunks_(空闲Chunk池)、readPos_/writePos_(读写游标)、lruCache_(Fragment查找加速) |
| 关键流程 | 1. Init(totalBuffSize, chunkSize) → 预分配 bufferAddr_ + 计算 chunkMaxNum_ → 2. Write(data, offset, size) → GetFreeCacheChunk 获取空闲 Chunk → 写入数据 → AddFragmentCacheBuffer 挂载到 Fragment → 3. Read(ptr, offset, size) → GetOffsetFragmentCache 查找 Fragment → GetOffsetChunkCache 查找 Chunk → 拷贝数据 |
| 数据/控制流 | 写入：数据 → GetFreeCacheChunk → Chunk 填充 → AddFragmentCacheBuffer → Fragment 链表；读取：offset → LRU/线性查找 Fragment → 线性查找 Chunk → 数据拷贝 |

### Fragment 生命周期

```
创建（AddFragmentCacheBuffer）→ 写入中 → 写入完成
                                    ↓
读取中 → 访问时间更新（LRU）→ 已读完毕（DeleteHasReadFragmentCacheBuffer）
                                    ↓
回收（Chunk 归还 freeChunks_）→ Fragment 从链表移除
```

### 已读数据回收

1. Fragment 的 totalReadSize 超过阈值 → 标记为可回收
2. 新 Write 需要空闲 Chunk 但 freeChunks_ 为空 → DeleteHasReadFragmentCacheBuffer 回收最久未读的 Fragment
3. 回收的 Chunk 归还 freeChunks_，Fragment 从链表移除

## 使用方式（Usage）

| 子维度 | 内容 |
| --- | -- |
| 接入条件 | 缓冲区大小和 Chunk 大小已确定 |
| 配置步骤 | 1. 创建 CacheMediaChunkBuffer → 2. Init(totalBuffSize, chunkSize) → 3. Write/Read 操作 |
| 核心API | `Init(totalBuffSize, chunkSize)` — 初始化缓冲区；`Write(ptr, offset, writeSize)` — 写入数据；`Read(ptr, offset, readSize)` — 读取数据；`Seek(offset)` — 移动读游标；`ClearFragmentBeforeOffset(offset)` — 清理已读数据 |
| 典型流程 | 1. Init(4MB, 16KB) → 预分配 4MB 内存，256 个 Chunk → 2. 分片1 数据到达 → Write → 创建 Fragment1 → 3. 分片2 数据到达 → Write → 创建 Fragment2 → 4. 播放线程 Read → 从 Fragment1 读取 → 5. Fragment1 读完 → ClearFragmentBeforeOffset 回收 |

## 代码关联（Code）

| 子维度 | 内容 |
| --- | -- |
| 核心类/接口 | `CacheMediaChunkBufferImpl`（utils/media_cached_buffer.h）；`CacheMediaChunkBuffer`（同文件）；`CacheMediaChunkBufferHlsImpl`（同文件） |
| 关键配置文件 | 无独立配置，参数通过 Init 传入 |
| 代码结构 | utils/media_cached_buffer.cpp — 缓冲区核心实现；utils/lru_cache.h — LRU 缓存模板 |

## 约束与限制（Constraints）

| 子维度 | 内容 |
| --- | -- |
| 系统限制 | 缓冲区总大小在 Init 时固定，不可动态扩展；Chunk 大小不可变 |
| 环境依赖 | 依赖 LruCache 模板做 Fragment 查找加速 |
| 性能约束 | Fragment 线性查找（find_if）在 Fragment 数量多时可能较慢，LRU 缓存缓解；大 offset 跨度（isLargeOffsetSpan_）影响回收策略 |
| 安全约束 | Write 不做越界检查（调用方保证 offset 合法）；Chunk 数据用 memcpy_s 拷贝 |
