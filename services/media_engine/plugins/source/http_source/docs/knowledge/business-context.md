# 业务背景

## 应用定位
### 核心职责
本模块是 HarmonyOS 媒体引擎的 **HTTP 网络数据源插件** ，为 **Demuxer（解封装器）** 提供 **HTTP/HLS/DASH 协议的网络流数据读取能力**，是 **HarmonyOS 播放器** 在网络流场景的关键数据通路

### 核心价值
- 支持三种主流网络流协议：HTTP 渐进式下载、HLS 自适应流、DASH 自适应流
- 提供统一的 Read/Seek/码率切换接口，对上层 Demuxer 屏蔽协议差异
- 支持自适应码率切换（ABR），根据网络带宽动态选择最优画质
- 支持 HLS AES-128 加密分片解密和 DASH DRM 信息上报

### 职责边界（本模块不负责）
- 不负责音视频解码（由 Codec 模块负责）
- 不负责容器格式解析（由 Demuxer 模块负责）
- 不负责播放控制和状态机（由 Player 模块负责）
- 不负责 UI 渲染（由 ACE 框架负责）
- 不负责底层网络连接管理（由 libcurl 和 NetworkClient 负责）

## 系统上下文

### 外部系统
- **Demuxer（解封装器）**：上层调用方，通过 SourcePlugin 接口读取数据
- **libcurl**：底层 HTTP 客户端库，提供网络请求能力
- **OpenSSL**：提供 AES-128-CBC 解密能力（HLS 加密分片）
- **libxml2**：提供 XML 解析能力（DASH MPD 清单解析）
- **HiLog**：系统日志框架
- **HiTrace**：系统打点追踪框架
- **NetworkManager**：网络连接状态管理

### 交互上下文
```
用户 → Player → Demuxer → HttpSourcePlugin → DownloadMonitor → MediaDownloader → Downloader → libcurl → 服务器
                                                                               ↘ AppClient → MediaSourceLoader → 应用侧
```
**交互说明**：
| 交互路径 | 方向 | 交互内容 | 交互意图 |
| ---- | -- | ---- | ---- |
| Demuxer ↔ HttpSourcePlugin | ↔ | Read 数据、Seek、码率查询/切换 | 获取网络流数据并控制播放位置 |
| HttpSourcePlugin → DownloadMonitor | → | 委托协议相关下载操作（DownloadMonitor 装饰真实 MediaDownloader） | 按协议类型分发下载任务，透明添加重试和监控 |
| DownloadMonitor → MediaDownloader | → | 代理调用真实下载器 | 执行协议相关下载 |
| MediaDownloader → Downloader | → | 创建和管理 HTTP 请求 | 执行实际网络 I/O |
| Downloader → HttpCurlClient | → | HTTP 请求/响应 | 通过 libcurl 底层网络通信 |
| Downloader → AppClient | → | 应用侧数据请求 | DRM 等需要应用侧参与的数据获取（AppClient 与 HttpCurlClient 同为 NetworkClient 实现，二选一） |
| MediaDownloader → Callback | → | 缓冲状态/错误/码率变更事件 | 通知上层播放状态变化 |
| DashMpdDownloader → libxml2 | → | MPD XML 解析 | 解析 DASH 清单 |
| HlsSegmentManager → AesDecryptor | → | AES 解密 | 解密 HLS 加密分片 |
