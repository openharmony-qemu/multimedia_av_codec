# HlsMediaDownloader（HLS媒体下载器）

## 实体概念

| 子维度 | 内容 |
| --- | -- |
| 实体名称 | HlsMediaDownloader（HLS媒体下载器） |
| 实体定义 | 处理 HLS（HTTP Live Streaming）协议流式下载的 MediaDownloader 实现，管理多 Segment 类型（视频/音频/字幕）的独立 HlsSegmentManager，支持 Master Playlist 解析、码率切换、fMP4 头读取 |
| 核心特征 | 三路独立 SegmentManager（videoSegManager_/audioSegManager_/subtitlesSegManager_）；PlayListChangeCallback 接收 Playlist 变更通知；支持 AES-128 加密分片解密；支持 fMP4 和 ByteRange 模式 |
| 类型/分类 | MediaDownloader 子类 |

## 上下文与场景

| 子维度 | 内容 |
| --- | -- |
| 使用场景 | HLS 点播/直播播放；多码率自适应切换；加密流解密播放；fMP4 格式 HLS 播放 |
| 角色与参与方 | HlsSegmentManager（下层）：管理单流分片下载；HlsPlayListDownloader（下层）：解析 M3U8 清单 |
| 生命周期 | 由 HttpSourcePlugin 创建 → Init → Open URL → Read 循环 → Close → 销毁 |
| 交互流程 | 1. Open M3U8 URL → 2. HlsPlayListDownloader 解析 Master/Media Playlist → 3. OnMasterReady 创建 SegmentManager → 4. HlsSegmentManager 下载分片 → 5. Read 从 videoSegManager_ 读取 |
| 状态流转 | IDLE → OPENING → PLAYLIST_PARSING → SEGMENT_DOWNLOADING → READING |

## 规格与约束

| 子维度 | 内容 |
| --- | -- |
| 系统限制 | HLS 直播不支持 Seek（除非是时移流）；fMP4 模式需 X-MAP 标签 |
| 业务规则 | 多码率切换时先完成当前分片下载再切换；音频/字幕流延迟到 OnMasterReady 后创建 |
| 性能约束 | 分片切换延迟取决于当前分片下载进度；直播刷新间隔由 targetDuration_ 决定 |

## 知识关联

| 子维度 | 内容 |
| --- | -- |
| 上层依赖 | HttpSourcePlugin、DownloadMonitor |
| 下游影响 | HlsSegmentManager（视频/音频/字幕）、HlsPlayListDownloader |
| 平级关联 | HttpMediaDownloader、DashMediaDownloader |
| 概念对比 | HlsMediaDownloader vs DashMediaDownloader：HLS 用 M3U8 文本清单 + 分片，DASH 用 MPD XML 清单 + Period/AdaptationSet 层级 |

## 数据模型

| 子维度 | 内容 |
| --- | -- |
| 核心数据结构 | videoSegManager_(HlsSegmentManager)、audioSegManager_(HlsSegmentManager)、subtitlesSegManager_(HlsSegmentManager)、bufferingFlag_(缓冲状态位图) |
| 数据流转路径 | M3U8 → HlsPlayListDownloader → OnPlayListChanged → HlsSegmentManager::PutRequestIntoDownloader → Downloader → SaveData → CacheBuffer → Read |

## 代码与符号

| 子维度 | 内容 |
| --- | -- |
| 核心API/类 | `HlsMediaDownloader`（hls/hls_media_downloader.h）|
| 核心符号映射 | Open → videoSegManager_->Open；Read → videoSegManager_->Read；SeekToTime → 各 SegManager->SeekToTime |
