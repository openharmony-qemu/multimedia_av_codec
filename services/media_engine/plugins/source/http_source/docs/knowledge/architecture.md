# 架构设计及约束

## 架构设计
### 设计原则
| 原则 | 描述 | 理由 |
| -- | -- | -- |
| 策略模式协议分发 | HttpSourcePlugin 基于 URL 字符串匹配（IsDash/CheckIsM3U8Uri）和 mimeType 判断协议类型，按策略创建对应 MediaDownloader 实例 | 新增协议只需新增 MediaDownloader 子类，不修改插件框架 |
| 装饰器监控 | DownloadMonitor 装饰 MediaDownloader，透明添加重试和监控能力 | 监控逻辑与下载逻辑解耦，可独立演进 |
| 缓冲策略抽象 | 双缓冲模式（RingBuffer/CacheMediaChunkBuffer）由各 MediaDownloader 按协议特性选择 | HTTP 单文件用 RingBuffer 高效寻址，HLS/DASH 分片用 CacheMediaChunkBuffer 灵活管理 |
| 回调驱动 | 数据到达通过 DataSaveFunc 回调写入缓冲区，状态变更通过 Callback/StatusCallback 通知上层 | 解耦网络线程与播放线程，避免阻塞 |
| 多流并行 | HLS/DASH 的视频/音频/字幕流独立下载和管理 | 多流互不阻塞，支持独立码率选择和轨切换 |

### 逻辑架构
```
┌──────────────────────────────────────────────────────┐
│                    上层（Demuxer）                      │
└──────────────┬───────────────────────────────────────┘
               │ SourcePlugin 接口
┌──────────────▼───────────────────────────────────────┐
│              HttpSourcePlugin                          │
│  ┌─────────────────┐ ┌────────────────────────────┐  │
│  │URL匹配路由      │ │StreamResourceParser        │  │
│  │(IsDash/         │ │(子资源解析)                 │  │
│  │ CheckIsM3U8Uri/ │ │                            │  │
│  │ mimeType)       │ │                            │  │
│  └─────────────────┘ └────────────────────────────┘  │
│  注：StreamProtocolSniffer 为独立嗅探工具，不参与内部路由  │
└──────────────┬───────────────────────────────────────┘
               │
┌──────────────▼───────────────────────────────────────┐
│            DownloadMonitor（装饰器）                    │
│  ┌──────────────────────────────────────────────────┐ │
│  │          MediaDownloader（策略接口）                │ │
│  │  ┌──────────────┐ ┌────────────┐ ┌────────────┐ │ │
│  │  │HttpMedia     │ │HlsMedia    │ │DashMedia    │ │ │
│  │  │Downloader    │ │Downloader  │ │Downloader   │ │ │
│  │  └──────┬───────┘ └─────┬──────┘ └──────┬──────┘ │ │
│  └─────────┼───────────────┼───────────────┼────────┘ │
└────────────┼───────────────┼───────────────┼──────────┘
             │               │               │
     ┌───────▼───────┐ ┌─────▼──────┐ ┌──────▼───────────┐
     │Downloader     │ │HlsSegment  │ │DashMpdDownloader │
     │+RingBuffer/   │ │Manager×N   │ │+DashSegmentDown- │
     │CacheBuffer    │ │+PlaylistDL │ │loader×N           │
     └───────┬───────┘ └─────┬──────┘ └──────┬───────────┘
             │               │               │
     ┌───────▼───────────────▼───────────────▼───────────┐
     │              Downloader（HTTP请求执行器）            │
     │  ┌──────────────────┐  ┌───────────────────────┐  │
     │  │HttpCurlClient    │  │AppClient              │  │
     │  │(libcurl)         │  │(MediaSourceLoader)    │  │
     │  └──────────────────┘  └───────────────────────┘  │
     └───────────────────────────────────────────────────┘
```

### 模块职责
| 模块 | 类型 | 模块职责 | 其他 |
| -- | -- | ---- | ---- |
| http_source_plugin | 插件入口 | 实现 SourcePlugin 接口，URL特征匹配路由与 MediaDownloader 创建/切换 | 核心类：HttpSourcePlugin |
| http | HTTP下载 | 普通 HTTP/HTTPS 单文件流式下载，双缓冲模式，CacheBuffer 模式下下载流控 | 核心类：HttpMediaDownloader |
| hls | HLS下载 | HLS 协议全流程：Playlist 解析、分片调度、AES 解密、码率切换 | 核心类：HlsMediaDownloader、HlsSegmentManager |
| dash | DASH下载 | DASH 协议全流程：MPD 解析、Period 切换、码率/轨切换 | 核心类：DashMediaDownloader、DashMpdDownloader、DashSegmentDownloader |
| download | 下载基础设施 | HTTP 请求执行、请求队列管理、应用侧数据加载 | 核心类：Downloader、DownloadRequest、AppClient |
| monitor | 下载监控 | 重试策略、错误码映射、网络错误事件上报 | 核心类：DownloadMonitor |
| utils | 工具集 | 缓存缓冲区、AES 解密、存储空间检测、线程安全容器、码率计算 | |
| base64 | Base64编解码 | Base64 编解码工具 | |
| xml | XML解析 | XML 元素与解析器（DASH MPD 使用 libxml2，此处为辅助） | |

### 技术选型
| 类别 | 技术选型 | 说明 |
| -- | ---- | ---- |
| HTTP 客户端 | libcurl | 工业级 HTTP 客户端库，支持 Range/Redirect/HTTPS |
| AES 解密 | OpenSSL AES-128-CBC | HLS 加密分片的实时解密 |
| XML 解析 | libxml2 | DASH MPD 清单解析 |
| 缓冲区 | RingBuffer + CacheMediaChunkBuffer | 双模式适配不同协议的数据访问模式 |
| 日志 | HiLog | 统一 Domain：STREAM_SOURCE |
| 打点 | HiTrace | 关键路径打点追踪 |
| 线程模型 | Task + BlockingQueue | 下载线程独立于播放线程 |

## 架构约束
### 基础设施
| 基础设施 | 功能说明 | 关键接口描述 |
| -- | ---- | ---- |
| 日志 | HiLog 统一日志输出 | MEDIA_LOG_I/W/E/F，Domain: STREAM_SOURCE |
| 打点 | HiTrace 关键路径追踪 | SourceStat / DownloadStat 打点 |
| 存储 | StorageUsageUtil 磁盘空间检测 | GetFreeSize/GetTotalSize/HasEnoughStorage |
| 网络 | libcurl HTTP 请求 | HttpCurlClient 封装 curl easy/multi |
| 加密 | OpenSSL AES-128-CBC | AesDecryptor 封装加解密操作 |

### 线程模型
- **播放线程**：Demuxer 调用 HttpSourcePlugin::Read，从缓冲区读取数据
- **下载线程**：Downloader::HttpDownloadLoop 在 Task 线程中循环执行 HTTP 请求
- **Playlist 更新线程**：HLS/DASH 的 Playlist/MPD 周期性更新在独立 Task 线程中执行
- **Monitor 线程**：DownloadMonitor 的 HttpMonitorLoop 周期性检查下载状态
- **线程同步**：播放线程与下载线程通过 Mutex + ConditionVariable 同步缓冲区读写

### 数据流约束
- **HTTP 数据流**：libcurl → RxBodyData 回调 → SaveBodyData → DataSaveFunc → 缓冲区写入 → HandleDownloadWaterLine 流控 → Read 返回
- **HLS 数据流**：libcurl → HlsSegmentManager::SaveData → (AES解密) → CacheBuffer 写入 → GetDownloadThrottleThreshold/GetDownloadResumeThreshold 流控 → Read 读取
- **DASH 数据流**：libcurl → DashSegmentDownloader::SaveData → RingBuffer 写入 → Read 读取

### 非功能设计
#### 性能
| 性能场景 | 实现方案 |
| ---- | ---- |
| 缓冲区首次填充 | 支持 SetInitialBufferSize 设置初始缓冲大小 |
| 大文件 Seek | HTTP Range 请求实现快速跳转，CacheBuffer 支持 ClearFragmentBeforeOffset |
| 弱网码率切换 | ABR 自动根据下载速度计算目标码率，HLS/DASH 支持平滑切换 |
| 下载速率统计 | WriteBitrateCaculator 实时计算写入码率，支撑 ABR 决策 |

#### 内存
| 内存场景 | 实现方案 |
| ---- | ---- |
| 缓冲区大小控制 | RingBuffer 定长、CacheMediaChunkBuffer 按 Duration 动态分配 |
| 已读数据回收 | CacheMediaChunkBuffer 基于 LRU 淘汰已读 Fragment |
| AES 解密缓冲 | HlsSegmentManager 使用 256 字节（AES_DECRYPT_LEN）解密缓冲区 |
| 分片信息管理 | HLS 分片信息随播放推进清理，DASH Segment 列表按 Period 管理 |

#### 安全与隐私
| 安全与隐私场景 | 实现方案 |
| ---- | ---- |
| URL 协议黑名单 | IsAllowedProtocol 黑名单校验，阻止 file:// 等本地协议（非白名单机制） |
| AES 密钥安全 | 密钥通过 HTTPS 下载，内存中不持久化明文密钥 |
| DRM 信息上报 | HLS/DASH 的 DRM PSSH 信息通过回调上报给上层 DRM 模块 |
| 符号导出控制 | -fvisibility=hidden + version_script 控制导出符号 |
