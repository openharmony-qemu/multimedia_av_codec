# HttpMediaDownloader（HTTP媒体下载器）

## 实体概念

| 子维度 | 内容 |
| --- | -- |
| 实体名称 | HttpMediaDownloader（HTTP媒体下载器） |
| 实体定义 | 处理普通 HTTP/HTTPS 单文件流式下载的 MediaDownloader 实现，支持 Range 请求、Seek、双缓冲模式（RingBuffer/CacheMediaChunkBuffer） |
| 核心特征 | 双缓冲模式：小文件用 RingBuffer（isRingBuffer_=true），大文件用 CacheMediaChunkBuffer；CacheBuffer 模式下通过 HandleDownloadWaterLine 做下载流控（90% 暂停/低于阈值恢复）；支持多码率 HTTP 流（PlayMediaStream 列表）；ABR 自适应码率切换 |
| 类型/分类 | MediaDownloader 子类 |

## 上下文与场景

| 子维度 | 内容 |
| --- | -- |
| 使用场景 | MP4/MKV 等单文件 HTTP 渐进式下载播放；多码率 HTTP 流自适应切换 |
| 角色与参与方 | Downloader（下层）：执行 HTTP 请求；Callback（上层）：通知缓冲状态 |
| 生命周期 | 由 HttpSourcePlugin 创建 → Open → Read 循环 → Close → 销毁 |
| 交互流程 | 1. Open URL → 2. Downloader 发起 HTTP 请求 → 3. 数据回调 SaveData 写入缓冲区 → 4. Read 从缓冲区读取 → 5. CacheBuffer 模式下 HandleDownloadWaterLine 控制下载流控 |
| 状态流转 | IDLE → OPENING → READING → SEEKING → CLOSED |

## 规格与约束

| 子维度 | 内容 |
| --- | -- |
| 系统限制 | 依赖服务器支持 Range 请求才能 Seek |
| 业务规则 | isRingBuffer_ 根据 contentLength 决定缓冲模式；CacheBuffer 模式下 canWrite_ 由 HandleDownloadWaterLine 控制（缓冲达 90% 暂停写入，低于阈值恢复）；Seek 时 AUDIO_SEEK_WATER_LINE_ABOVE(16KB) 控制切换下载位置阈值 |
| 性能约束 | Read 循环超时 60s（LOOP_TIMEOUT）；首次缓冲等待首帧到达 |

## 知识关联

| 子维度 | 内容 |
| --- | -- |
| 上层依赖 | HttpSourcePlugin（通过 DownloadMonitor 间接调用） |
| 下游影响 | Downloader、RingBuffer、CacheMediaChunkBufferImpl |
| 平级关联 | HlsMediaDownloader、DashMediaDownloader |

## 数据模型

| 子维度 | 内容 |
| --- | -- |
| 核心数据结构 | ringBuffer_(RingBuffer)、cacheMediaBuffer_(CacheMediaChunkBufferImpl)、downloader_(Downloader)、downloadRequest_(DownloadRequest)、totalBufferSize_(缓冲区总大小) |
| 字段定义 | isRingBuffer_: 缓冲模式标志；canWrite_: 下载流控写入开关（HandleDownloadWaterLine 控制）；readOffset_/writeOffset_: 读写偏移；cachedDuration_: 已缓冲时长 |
| 数据存储方式 | 媒体数据存储在内存缓冲区（RingBuffer 或 CacheMediaChunkBuffer） |
| 数据流转路径 | libcurl → RxBodyData → SaveData → RingBuffer/CacheBuffer → Read → Demuxer |

## 代码与符号

| 子维度 | 内容 |
| --- | -- |
| 核心API/类 | `HttpMediaDownloader`（http/http_media_downloader.h）|
| 核心符号映射 | Open → 发起 HTTP 请求；Read → ReadDelegate → ReadRingBuffer/HandleCacheBuffer；HandleCacheBuffer → ReadCacheBuffer + HandleDownloadWaterLine；SeekToPos → SeekRingBuffer/SeekCacheBuffer |
