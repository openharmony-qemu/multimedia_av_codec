# 术语表
> 只说明模型不知道的术语，常识类不要写

## HttpSourcePlugin（HTTP源插件）
：媒体引擎的HTTP网络数据源插件，类型为**实体**。继承自 SourcePlugin，负责根据URL协议类型（HTTP/HLS/DASH）创建对应的 MediaDownloader 实例，统一提供 Read/Seek/码率切换等接口给上层 Demuxer 调用。完整信息指向 [HttpSourcePlugin 实体文档](entities/http-source-plugin.md)

## MediaDownloader（媒体下载器）
：协议无关的下载抽象基类，类型为**实体**。定义了 Open/Close/Read/Seek/码率选择等纯虚接口，HttpMediaDownloader、HlsMediaDownloader、DashMediaDownloader 分别实现对应协议的下载逻辑。完整信息指向 [MediaDownloader 实体文档](entities/media-downloader.md)

## HttpMediaDownloader（HTTP媒体下载器）
：处理普通 HTTP/HTTPS 单文件流式下载的 MediaDownloader 实现，类型为**实体**。支持 RingBuffer 和 CacheMediaChunkBuffer 双缓冲模式，实现 Range 请求、Seek。完整信息指向 [HttpMediaDownloader 实体文档](entities/http-media-downloader.md)

## HlsMediaDownloader（HLS媒体下载器）
：处理 HLS（HTTP Live Streaming）协议流式下载的 MediaDownloader 实现，类型为**实体**。管理多 Segment 类型（视频/音频/字幕）的独立 HlsSegmentManager，支持 Master Playlist 解析、码率切换、fMP4 头读取。完整信息指向 [HlsMediaDownloader 实体文档](entities/hls-media-downloader.md)

## DashMediaDownloader（DASH媒体下载器）
：处理 DASH（Dynamic Adaptive Streaming over HTTP）协议流式下载的 MediaDownloader 实现，类型为**实体**。通过 DashMpdDownloader 解析 MPD 清单，管理多 DashSegmentDownloader 实例，支持 Period 切换、码率平滑切换、音轨/字幕轨选择。完整信息指向 [DashMediaDownloader 实体文档](entities/dash-media-downloader.md)

## Downloader（下载器）
：底层 HTTP 请求执行器，类型为**实体**。封装 libcurl 网络请求，管理 DownloadRequest 请求队列，在独立 Task 线程中循环执行 HTTP 下载，通过回调将响应数据传递给上层

## DownloadRequest（下载请求）
：单次 HTTP 请求的上下文对象，类型为**实体**。携带 URL、Range 参数、HeaderInfo、数据保存回调，跟踪请求状态（EOS/重试/错误码）

## DownloadMonitor（下载监控器）
：MediaDownloader 的装饰器，类型为**实体**。包装真实 MediaDownloader，监控下载状态，实现自动重试、网络错误码映射、错误事件上报

## HlsSegmentManager（HLS分片管理器）
：HLS 单流类型（视频/音频/字幕）的分片下载管理器，类型为**实体**。管理 Playlist 下载、分片队列调度、AES 解密、CacheMediaChunkBuffer 缓冲管理。完整信息指向 [HLS分片管理技术文档](technologies/hls-segment-management.md)

## DashMpdDownloader（DASH MPD下载器）
：DASH 协议的 MPD（Media Presentation Description）清单解析与分片调度模块，类型为**实体**。负责下载 MPD 文件、解析 Period/AdaptationSet/Representation 层级结构、提供分片信息给 DashSegmentDownloader

## DashSegmentDownloader（DASH分片下载器）
：DASH 协议单流的分片下载执行器，类型为**实体**。管理 RingBuffer、Init Segment、Media Segment 的下载与缓冲，支持码率切换时的缓冲清理

## StreamProtocolSniffer（流协议嗅探器）
：通过数据头部特征识别流协议类型的工具类，类型为**技术**。根据 `#EXTM3U` 识别 HLS，根据 `<?xml` + `MPD` 识别 DASH，其余识别为 HTTP

## CacheMediaChunkBuffer（缓存分块缓冲区）
：基于 Fragment + Chunk 二级结构的可寻址媒体缓冲区，类型为**技术**。支持按 offset 随机读写、LRU 淘汰、Fragment 分裂与合并，为 HLS/DASH 提供缓存管理。完整信息指向 [缓存缓冲区技术文档](technologies/cache-buffer-management.md)

## AesDecryptor（AES解密器）
：AES-128-CBC 解密工具类，类型为**技术**。支持密钥和 IV 动态更新，用于 HLS 加密分片的实时解密

## RingBuffer（环形缓冲区）
：定长循环缓冲区，类型为**技术**。用于 HTTP 单文件流和 DASH 分片数据的缓冲，支持阻塞式读写和 Seek 操作

## PlayListChangeCallback（播放清单变更回调）
：Playlist 变更通知接口，类型为**概念**。定义 OnMasterReady/OnPlayListChanged/OnDrmInfoChanged/OnTimedMetaDataChanged 四个回调，HlsSegmentManager 和 DashMediaDownloader 实现此接口接收 Playlist 更新

## MediaSourceLoaderCombinations（媒体源加载器组合）
：应用侧数据加载适配器，类型为**实体**。封装 IMediaSourceLoader 和 MediaSourceLoadingRequest，使 Downloader 可通过应用侧回调（而非 libcurl）获取数据，用于 DRM 等场景

## MPD（Media Presentation Description）
：DASH 协议的清单描述文件，类型为**概念**。XML 格式，描述 Period/AdaptationSet/Representation/Segment 等层级结构

## M3U8
：HLS 协议的播放清单文件格式，类型为**概念**。分为 Master Playlist（多码率索引）和 Media Playlist（分片列表），通过 `#EXTM3U` 标签标识
