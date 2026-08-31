# HttpSourcePlugin（HTTP源插件）

## 实体概念

| 子维度 | 内容 |
| --- | -- |
| 实体名称 | HttpSourcePlugin（HTTP源插件） |
| 实体定义 | 媒体引擎 SourcePlugin 的 HTTP 网络数据源实现，根据 URL 特征和 mimeType 判断协议类型，创建对应的 MediaDownloader（均被 DownloadMonitor 装饰），为 Demuxer 提供统一的 Read/Seek/码率切换接口 |
| 核心特征 | 继承 SourcePlugin；基于 URL 字符串匹配（IsDash/CheckIsM3U8Uri）和 mimeType 判断协议；所有 MediaDownloader 都被 DownloadMonitor 装饰后才存入 downloader_；支持 redirect 后重新创建 Downloader |
| 类型/分类 | SourcePlugin 实现，动态库形式加载（media_plugin_HttpSource） |

## 上下文与场景

| 子维度 | 内容 |
| --- | -- |
| 使用场景 | 网络流播放：点播（HTTP/HLS/DASH）、直播（HLS Live/DASH Live）、自适应码率切换、多码率 HTTP 流 |
| 角色与参与方 | Demuxer（上层）：调用 Read/Seek 接口；DownloadMonitor（中层）：装饰 MediaDownloader 提供重试和监控；MediaDownloader 子类（下层）：执行协议相关下载；PlayStrategy：播放策略配置（缓冲时长等） |
| 生命周期 | 插件注册(HttpSourceRegister) → Init → SetSource → Read 循环 → Stop → Deinit |
| 交互流程 | 1. SetSource 传入 MediaSource → 2. IsAllowedProtocol 校验 URL → 3. PlayStrategyInit 提取 uri_/httpHeader_/mimeType_/playStrategy_ → 4. SetDownloaderBySource 按优先级路由：IsDash()→DashMediaDownloader，CheckIsM3U8Uri()→HlsMediaDownloader，http前缀→HttpMediaDownloader，mimeType_==APPLICATION_M3U8→覆盖为 HlsMediaDownloader → 5. Downloader->Open(uri_, httpHeader_) → 6. Demuxer 调用 Read 获取数据 |
| 状态流转 | CREATED → SET_SOURCE（创建 Downloader + Open）→ READING → SEEKING → STOPPED → DEINIT |
| 异常处理路径 | URL 协议不允许(file://) → 返回 ERROR_INVALID_OPERATION；Open 失败 → 返回 ERROR_UNKNOWN；Seek 超出范围超过5次 → 回调 CLIENT_ERROR/TIME_OUT；Read 超时 → isInterruptNeeded_ 中断 |

## 规格与约束

| 子维度 | 内容 |
| --- | -- |
| 系统限制 | 必须以动态库形式加载到媒体引擎插件框架；同一实例不允许重复 SetSource（GetDownloader() != nullptr 时返回 ERROR_INVALID_OPERATION） |
| 业务规则 | 协议类型在 SetDownloaderBySource 中按优先级判断：DASH > HLS > HTTP > mimeType覆盖；mimeType_==APPLICATION_M3U8 会覆盖之前已创建的 Downloader；SetSource 可能因 redirect 重新调用 InitSourcePlugin（第二次使用 redirectUrl_）；Seek 支持依赖服务器 Accept-Range |
| 安全与隐私约束 | URL 必须通过 IsAllowedProtocol 校验；禁止 file:// 等本地协议；redirect URL 也要通过协议校验 |
| 性能约束 | Read 延迟受缓冲区状态影响；首次 Open 需等待 Header 信息返回；delayReady_ 控制 Prepare 是否返回 ERROR_DELAY_READY |

## 知识关联

| 子维度 | 内容 |
| --- | -- |
| 上层依赖 | SourcePlugin 接口定义、Demuxer 调用、MediaSource（携带 URL/Header/mimeType/PlayStrategy/SourceLoader） |
| 下游影响 | DownloadMonitor → MediaDownloader 子类（HttpMediaDownloader、HlsMediaDownloader、DashMediaDownloader） |
| 平级关联 | StreamProtocolSniffer（独立嗅探工具，不参与内部路由）、StreamResourceParser（子资源解析） |
| 概念对比 | HttpSourcePlugin vs FileSourcePlugin：前者处理网络流，后者处理本地文件 |

## 数据模型

| 子维度 | 内容 |
| --- | -- |
| 核心数据结构 | downloader_(MediaDownloader，被 DownloadMonitor 装饰)、downloaderMutex_(shared_mutex 保护 downloader_)、callback_(weak_ptr<Callback>)、isInterruptNeeded_(atomic<bool>)、loaderCombinations_(MediaSourceLoaderCombinations)、playStrategy_(PlayStrategy)、delayReady_(bool)、uri_(string)、httpHeader_(map)、mimeType_(string)、redirectUrl_(string)、reportInfo_(SourceStatisticsReportInfo) |
| 数据存储方式 | 运行时状态保存在内存中 |
| 数据流转路径 | Demuxer → Read → GetDownloader()(shared_lock) → downloader_->Read → 缓冲区 → 返回数据 |

## 代码与符号

| 子维度 | 内容 |
| --- | -- |
| 核心API/类 | `HttpSourcePlugin`（http_source_plugin.h）|
| 核心符号映射 | SetSource → InitSourcePlugin → SetDownloaderBySource → CreateXxxDownloader；协议路由 → IsDash/CheckIsM3U8Uri/IsSeekToTimeSupported；redirect → GetCurUrl + 二次 InitSourcePlugin |
| 关键配置文件 | BUILD.gn（构建配置）、libmedia_plugin_HttpSource.map（符号导出控制） |
