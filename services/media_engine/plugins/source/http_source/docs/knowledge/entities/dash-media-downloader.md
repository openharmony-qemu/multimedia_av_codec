# DashMediaDownloader（DASH媒体下载器）

## 实体概念

| 子维度 | 内容 |
| --- | -- |
| 实体名称 | DashMediaDownloader（DASH媒体下载器） |
| 实体定义 | 处理 DASH（Dynamic Adaptive Streaming over HTTP）协议流式下载的 MediaDownloader 实现，通过 DashMpdDownloader 解析 MPD 清单，管理多 DashSegmentDownloader 实例，支持 Period 切换、码率平滑切换、音轨/字幕轨选择 |
| 核心特征 | 实现 DashMpdCallback 接收 MPD 解析事件；DashPreparedAction 管理待执行的码率/轨切换；ThreadSafeContainer 管理多 DashSegmentDownloader 实例；支持 Period 切换和 Pending 切换队列 |
| 类型/分类 | MediaDownloader 子类 + DashMpdCallback 实现者 |

## 上下文与场景

| 子维度 | 内容 |
| --- | -- |
| 使用场景 | DASH 点播/直播播放；多码率自适应切换；HDR/SDR 切换；多音轨/字幕轨选择；多 Period 内容（如广告插入） |
| 角色与参与方 | DashMpdDownloader（下层）：解析 MPD 并提供分片信息；DashSegmentDownloader（下层）：执行分片下载 |
| 生命周期 | 由 HttpSourcePlugin 创建 → Init → Open → MPD 解析 → 分片下载 → Read 循环 → Close → 销毁 |
| 交互流程 | 1. Open MPD URL → 2. DashMpdDownloader 下载并解析 MPD → 3. OnMpdInfoUpdate(PARSE_OK) → 4. 创建 DashSegmentDownloader → 5. GetNextSegmentByStreamId 获取分片 → 6. 下载并写入 RingBuffer → 7. Read 读取 |
| 状态流转 | IDLE → MPD_DOWNLOADING → MPD_PARSING → STREAM_INITIALIZING → SEGMENT_DOWNLOADING → READING |

## 规格与约束

| 子维度 | 内容 |
| --- | -- |
| 系统限制 | DASH 直播需 MPD 支持 SegmentTemplate + SegmentTimeline；HDR 切换需 AdaptationSet 包含 HDR 描述 |
| 业务规则 | 码率切换可配置为等待当前分片完成后执行（waitSegmentFinish_）或立即执行；Period 切换时需同步切换所有流 |
| 性能约束 | MPD 解析耗时取决于 Period/AdaptationSet/Representation 数量；Sidx 解析需额外 HTTP Range 请求 |

## 知识关联

| 子维度 | 内容 |
| --- | -- |
| 上层依赖 | HttpSourcePlugin、DownloadMonitor |
| 下游影响 | DashMpdDownloader、DashSegmentDownloader |
| 平级关联 | HttpMediaDownloader、HlsMediaDownloader |
| 概念对比 | DashMediaDownloader vs HlsMediaDownloader：DASH 用 MPD XML 清单 + Period 层级，HLS 用 M3U8 文本清单 + VariantStream |

## 数据模型

| 子维度 | 内容 |
| --- | -- |
| 核心数据结构 | mpdDownloader_(DashMpdDownloader)、segmentDownloaders_(ThreadSafeContainer<vector<DashSegmentDownloader>>)、bitrateParam_(DashMpdBitrateParam)、trackParam_(DashMpdTrackParam)、preparedAction_(DashPreparedAction) |
| 数据流转路径 | MPD URL → DashMpdDownloader → ParseManifest → GetStreamInfo → Create DashSegmentDownloader → GetNextSegment → Download → RingBuffer → Read |

## 代码与符号

| 子维度 | 内容 |
| --- | -- |
| 核心API/类 | `DashMediaDownloader`（dash/dash_media_downloader.h）|
| 核心符号映射 | Open → mpdDownloader_->Open；Read → segmentDownloaders_→Read；SeekToTime → SeekInternal；OnMpdInfoUpdate → 处理 MPD 解析事件 |
