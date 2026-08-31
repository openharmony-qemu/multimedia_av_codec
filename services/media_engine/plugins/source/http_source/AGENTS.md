# HttpSourcePlugin AGENTS.md

## 基本信息

本模块是 HarmonyOS 媒体引擎的 HTTP 网络数据源插件，为 Demuxer 提供 HTTP/HLS/DASH 协议的网络流数据读取能力。主要功能包括：
- 根据 URL 特征和 mimeType 自动路由协议类型，创建对应的 MediaDownloader
- 提供统一的 Read/Seek/码率切换接口，对上层 Demuxer 屏蔽协议差异
- 支持自适应码率切换（ABR），根据网络带宽动态选择最优画质
- 支持 HLS AES-128 加密分片解密和 DASH DRM 信息上报

## 架构及依赖

> 详细的架构设计及约束参见 [architecture.md](docs/knowledge/architecture.md)

### 核心架构

采用策略模式 + 装饰器模式的分层架构：

1. **插件入口层**：
- HttpSourcePlugin：继承 SourcePlugin，基于 URL 字符串匹配（IsDash/CheckIsM3U8Uri）和 mimeType 判断协议类型，创建对应的 MediaDownloader

2. **监控层**：
- DownloadMonitor：装饰器，包装真实 MediaDownloader，透明添加重试和监控能力

3. **协议实现层**：
- HttpMediaDownloader：普通 HTTP/HTTPS 单文件流式下载
- HlsMediaDownloader：HLS 协议全流程（Playlist 解析、分片调度、AES 解密、码率切换）
- DashMediaDownloader：DASH 协议全流程（MPD 解析、Period 切换、码率/轨切换）

4. **下载基础设施层**：
- Downloader：HTTP 请求执行器，管理 DownloadRequest 请求队列
- NetworkClient：网络客户端抽象（HttpCurlClient 封装 libcurl，AppClient 封装 MediaSourceLoader）

5. **工具层**：
- RingBuffer / CacheMediaChunkBuffer：双模式缓冲区
- AesDecryptor：AES-128-CBC 解密
- WriteBitrateCaculator：码率计算
- StreamProtocolSniffer：独立协议嗅探工具（不参与内部路由）
- StreamResourceParser：子资源解析

### 协议路由决策链

SetDownloaderBySource 按优先级判断协议类型：

1. IsDash() → DashMediaDownloader
2. IsSeekToTimeSupported() + mimeType 非 APPLICATION_M3U8 → HlsMediaDownloader
3. uri_ 以 "http" 开头 → HttpMediaDownloader
4. mimeType_ == APPLICATION_M3U8 → 覆盖为 HlsMediaDownloader

### 数据流

1. **HTTP 数据流**：libcurl → RxBodyData → SaveBodyData → DataSaveFunc → RingBuffer/CacheBuffer → HandleDownloadWaterLine 流控 → Read
2. **HLS 数据流**：libcurl → HlsSegmentManager::SaveData → (AES解密) → CacheBuffer → DownloadThrottle/Resume 流控 → Read
3. **DASH 数据流**：libcurl → DashSegmentDownloader::SaveData → RingBuffer → Read

## 目录结构

```
http_source/
├── http_source_plugin.h          # 插件入口头文件
├── http_source_plugin.cpp        # 插件入口实现（含协议路由逻辑）
├── http_source_plugin_stream_protocol_sniffer.h   # 协议嗅探器（独立工具）
├── http_source_plugin_stream_protocol_sniffer.cpp
├── http_source_plugin_stream_resource_parser.h    # 子资源解析器
├── http_source_plugin_stream_resource_parser.cpp
├── media_downloader.h            # MediaDownloader 抽象基类
├── BUILD.gn                      # 构建配置
├── libmedia_plugin_HttpSource.map # 符号导出控制
├── docs/
│   └── knowledge/                # 知识库
│       ├── glossary.md           # 术语表
│       ├── business-context.md   # 业务背景
│       ├── architecture.md       # 架构设计及约束
│       ├── coding-standards.md   # 编码铁律
│       ├── entities/             # 实体知识
│       │   ├── http-source-plugin.md
│       │   ├── media-downloader.md
│       │   ├── http-media-downloader.md
│       │   ├── hls-media-downloader.md
│       │   └── dash-media-downloader.md
│       └── technologies/         # 技术知识
│           ├── protocol-sniffing-and-routing.md
│           ├── hls-segment-management.md
│           ├── cache-buffer-management.md
│           ├── download-retry-and-monitor.md
│           ├── adaptive-bitrate-switching.md
│           └── dash-mpd-parsing.md
├── http/                         # HTTP 单文件下载实现
│   ├── http_media_downloader.h
│   └── http_media_downloader.cpp
├── hls/                          # HLS 协议实现
│   ├── hls_media_downloader.h
│   ├── hls_media_downloader.cpp
│   ├── hls_segment_manager.h
│   ├── hls_segment_manager.cpp
│   ├── hls_playlist_downloader.h
│   ├── hls_playlist_downloader.cpp
│   ├── m3u8.h                    # M3U8 数据模型
│   └── hls_tags.h                # HLS 标签解析
├── dash/                         # DASH 协议实现
│   ├── dash_media_downloader.h
│   ├── dash_media_downloader.cpp
│   ├── dash_mpd_downloader.h
│   ├── dash_mpd_downloader.cpp
│   ├── dash_segment_downloader.h
│   ├── dash_segment_downloader.cpp
│   ├── dash_common.h             # DASH 公共数据结构
│   ├── include/
│   │   ├── mpd_parser/           # MPD 解析器头文件
│   │   └── utils/                # DASH 工具头文件
│   └── mpd_parser/               # MPD 解析器实现
├── download/                     # 下载基础设施
│   ├── downloader.h              # HTTP 请求执行器
│   ├── downloader.cpp
│   ├── media_source_loading_request.h
│   ├── media_source_loading_request.cpp
│   ├── app_client.h              # AppClient（MediaSourceLoader 适配）
│   └── network_client/           # NetworkClient 抽象及实现
├── monitor/                      # 下载监控装饰器
│   ├── download_monitor.h
│   └── download_monitor.cpp
├── utils/                        # 工具集
│   ├── media_cached_buffer.h     # CacheMediaChunkBuffer 缓冲区
│   ├── aes_decryptor.h           # AES 解密器
│   ├── storage_usage_util.h      # 存储空间检测
│   ├── thread_safe_container.h   # 线程安全容器
│   └── write_bitrate_caculator.h # 码率计算器
├── base64/                       # Base64 编解码
└── xml/                          # XML 解析辅助
```

## 构建

本模块以动态库形式构建（media_plugin_HttpSource），由媒体引擎插件框架加载。

## 知识库使用规范

项目知识存放在 `docs/knowledge/` 目录下，按以下规则加载：

### 启动时加载

- **glossary.md** — 术语表，提供所有业务术语的简要定义和 entity 文件指引
- **business-context.md** — 业务背景，涵盖产品定位、核心场景、系统上下文、外部依赖

### 按需加载

- **architecture.md** — 架构设计及约束，在涉及模块设计、架构变更、新增功能开发时加载
- **coding-standards.md** — 编码铁律，在编写或审查代码时加载

### 术语查询流程

遇到不了解的术语词汇时：

1. 优先在 **glossary.md** 中查询
2. 若 glossary 提供了 entity 链接，按需从 **entities/** 目录加载术语的详细介绍
3. 若 glossary 和 entities 中均无该术语，查阅 `docs/` 下的参考文档

## 知识索引

| 场景 | 先读 |
|------|------|
| 理解业务术语 | docs/knowledge/glossary.md |
| 理解模块定位与外部依赖 | docs/knowledge/business-context.md |
| 模块设计 / 架构变更 / 新增功能开发 | docs/knowledge/architecture.md |
| 编码前必读 | docs/knowledge/coding-standards.md |
| HttpSourcePlugin 插件入口细节 | docs/knowledge/entities/http-source-plugin.md |
| MediaDownloader 抽象基类设计 | docs/knowledge/entities/media-downloader.md |
| HTTP 单文件下载实现 | docs/knowledge/entities/http-media-downloader.md |
| HLS 协议下载实现 | docs/knowledge/entities/hls-media-downloader.md |
| DASH 协议下载实现 | docs/knowledge/entities/dash-media-downloader.md |
| 协议路由与嗅探机制 | docs/knowledge/technologies/protocol-sniffing-and-routing.md |
| HLS 分片管理机制 | docs/knowledge/technologies/hls-segment-management.md |
| 缓冲区管理机制 | docs/knowledge/technologies/cache-buffer-management.md |
| 下载重试与监控机制 | docs/knowledge/technologies/download-retry-and-monitor.md |
| 自适应码率切换（ABR） | docs/knowledge/technologies/adaptive-bitrate-switching.md |
| DASH MPD 解析与 Period 管理 | docs/knowledge/technologies/dash-mpd-parsing.md |

## 项目宪法

1. **URL 匹配是协议路由的唯一决策方式**：SetDownloaderBySource 基于 URL 字符串匹配（IsDash/CheckIsM3U8Uri）和 mimeType 判断协议类型，严禁在路由决策中引入 StreamProtocolSniffer 数据嗅探。
2. **DownloadMonitor 是必经装饰层**：所有 MediaDownloader 实例必须被 DownloadMonitor 装饰后才存入 downloader_，由 CreateXxxDownloader 内部自动完成，严禁绕过装饰器直接使用裸 MediaDownloader。
3. **IsAllowedProtocol 是黑名单机制**：仅阻止 file:// 等本地协议，不是白名单，严禁将其理解为仅允许 http/https/ftps。
4. **缓冲区选择由协议决定**：HTTP 单文件和 DASH 分片用 RingBuffer，HLS 分片和 HTTP 大文件用 CacheMediaChunkBuffer，严禁混用。
5. **码率切换必须平滑**：HLS 等待当前分片完成再切换 Variant，DASH 通过 PreparedAction 机制在分片边界切换，严禁在分片下载中途切换导致数据断裂。
6. **编码规范强制遵守**：编写或修改代码时必须遵守 `docs/knowledge/coding-standards.md` 中的编码铁律。
