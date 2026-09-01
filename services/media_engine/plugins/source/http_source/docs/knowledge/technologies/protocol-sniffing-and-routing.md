# 协议嗅探与路由

## 解决的问题（Problem）

| 子维度 | 内容 |
| --- | -- |
| 业务痛点 | HttpSourcePlugin 需要根据流协议类型创建不同的 MediaDownloader（HttpMediaDownloader/HlsMediaDownloader/DashMediaDownloader），但协议判断有多种来源：URL 特征、mimeType、数据内容签名，需统一理解路由逻辑 |
| 技术目标 | 明确 **HttpSourcePlugin 的协议路由决策链** 和 **StreamProtocolSniffer 的独立嗅探能力** 两部分，前者基于 URL/mimeType 做快速判断，后者基于数据内容做精确识别 |
| 适用场景 | SetSource 时自动选择下载器；外部模块需要根据数据内容判断流协议类型 |

## 核心概念与术语（Concepts）

| 子维度 | 内容 |
| --- | -- |
| 关键术语 | **SetDownloaderBySource**：HttpSourcePlugin 的协议路由入口方法；**IsDash**：URL 含 `.mpd` 或 `type=mpd` 判断；**CheckIsM3U8Uri**：URL 含 `.m3u8` 或 `=m3u8` 判断；**StreamProtocolSniffer**：基于数据签名的协议嗅探工具 |
| 角色定义 | **SetDownloaderBySource**：路由决策者，按优先级判断协议并创建 Downloader；**StreamProtocolSniffer**：独立嗅探工具，供外部模块使用（不参与 SetDownloaderBySource 路由） |
| 核心对象 | DASH_LIST = {".mpd", "type=mpd"}；LOWER_M3U8 = "m3u8"；HLS_SIGNATURE = "#EXTM3U"；DASH_XML_SIGNATURE = "<?xml" + DASH_MPD_SIGNATURE = "MPD" |

## 原理（Principles）

| 子维度 | 内容 |
| --- | -- |
| 核心机制 | **URL/mimeType 多级判断路由**：SetDownloaderBySource 按优先级依次检查 IsDash()、IsSeekToTimeSupported()（含 CheckIsM3U8Uri）、http 前缀、mimeType，命中即创建对应 Downloader |
| 嗅探规则（StreamProtocolSniffer） | 1. 数据以 `#EXTM3U` 开头 → HLS；2. 数据包含 `<?xml` 且包含 `MPD` → DASH；3. 其他 → HTTP |
| 架构组成 | 路由逻辑在 HttpSourcePlugin::SetDownloaderBySource 中；StreamProtocolSniffer 为独立静态工具类，导出 C 风格函数供外部调用 |

### 实际路由流程（SetDownloaderBySource，http_source_plugin.cpp:323-354）

```
SetSource(source) → PlayStrategyInit(source)  // 提取 uri_, httpHeader_, mimeType_, playStrategy_
                  → InitSourceLoader(source)  // 初始化 MediaSourceLoaderCombinations
                  → SetDownloaderBySource(source)
                      │
                      ├─ 1. IsDash() 为 true？
                      │     → URI 包含 ".mpd" 或 "type=mpd"
                      │     → CreateDashDownloader() → DownloadMonitor(DashMediaDownloader)
                      │     → delayReady_ = false
                      │
                      ├─ 2. IsSeekToTimeSupported() 为 true 且 mimeType_ != APPLICATION_M3U8？
                      │     → IsSeekToTimeSupported() 内部：
                      │         CheckIsM3U8Uri() 为 true（URI 含 .m3u8 或 =m3u8）
                      │         或 URI 含 ".mpd"
                      │     → CreateHlsDownloader() → DownloadMonitor(HlsMediaDownloader)
                      │     → delayReady_ = false
                      │
                      ├─ 3. uri_ 以 "http" 开头？
                      │     → InitHttpSource(source) → DownloadMonitor(HttpMediaDownloader)
                      │     → 若 source 含 PlayMediaStream 列表则按码率排序取最低码率 URL
                      │
                      ├─ 设置 PlayStrategy
                      │
                      └─ 4. mimeType_ == APPLICATION_M3U8？
                            → 创建 DownloadMonitor(HlsMediaDownloader(mimeType_))
                            → 覆盖之前设置的 Downloader
```

### StreamProtocolSniffer 的实际使用场景

StreamProtocolSniffer **不参与** SetDownloaderBySource 的路由决策。它是独立的协议嗅探工具，导出 C 风格接口供外部模块使用：

- `GetMinSniffBufferSize()` — 获取最小嗅探数据量（256 字节）
- `SniffStreamProtocolFromData(data, size)` — 根据数据内容判断协议类型

StreamResourceParser 在嗅探结果基础上进一步解析子资源 URL 列表。

## 使用方式（Usage）

| 子维度 | 内容 |
| --- | -- |
| 接入条件 | HttpSourcePlugin：通过 SetSource 传入 MediaSource；StreamProtocolSniffer：至少 256 字节流数据 |
| 核心API | `HttpSourcePlugin::SetSource` — 自动路由协议并创建 Downloader；`StreamProtocolSniffer::SniffStreamProtocolFromData(data, size)` — 独立嗅探协议类型 |
| 典型流程 | 1. 应用调用 AVSource_CreateWithURI("http://xxx/playlist.m3u8") → 2. HttpSourcePlugin::SetSource → 3. CheckIsM3U8Uri=true → 4. CreateHlsDownloader → 5. 开始下载 |

## 代码关联（Code）

| 子维度 | 内容 |
| --- | -- |
| 核心类/接口 | `HttpSourcePlugin::SetDownloaderBySource`（http_source_plugin.cpp:323）；`IsDash`（http_source_plugin.cpp:773）；`CheckIsM3U8Uri`（http_source_plugin.cpp:691）；`StreamProtocolSniffer`（http_source_plugin_stream_protocol_sniffer.h） |
| 代码结构 | http_source_plugin.cpp — 协议路由决策；http_source_plugin_stream_protocol_sniffer.cpp — 数据签名嗅探；http_source_plugin_stream_resource_parser.cpp — 子资源解析 |

## 约束与限制（Constraints）

| 子维度 | 内容 |
| --- | -- |
| 系统限制 | SetDownloaderBySource 基于 URL 字符串匹配，非标准 URL 后缀可能误判；mimeType_==APPLICATION_M3U8 会覆盖之前的 Downloader |
| 环境依赖 | StreamProtocolSniffer 需先下载至少 256 字节数据 |
| 性能约束 | URL 字符串匹配为 O(n)，开销极小；StreamProtocolSniffer 为 O(n) 扫描，通常 256 字节 |
