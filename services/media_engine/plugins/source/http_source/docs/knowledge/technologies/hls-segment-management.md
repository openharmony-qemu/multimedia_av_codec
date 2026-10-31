# HLS 分片管理（HlsSegmentManager）

## 解决的问题（Problem）

| 子维度 | 内容 |
| --- | -- |
| 业务痛点 | HLS 流由独立的小分片（.ts/.m4s）组成，每个分片需单独下载、可能加密、需按序缓冲，且支持 Seek 时需快速定位分片，传统顺序下载无法满足 |
| 技术目标 | 建立 **分片级下载调度器**，管理 Playlist 更新、分片下载队列、AES 解密流水线，实现 HLS 分片的按需下载和无缝播放 |
| 适用场景 | HLS 点播 Seek 后定位分片；HLS 直播持续拉取新分片；加密分片实时解密；码率切换时分片队列重建 |

## 核心概念与术语（Concepts）

| 子维度 | 内容 |
| --- | -- |
| 关键术语 | **PlayInfo**：分片下载描述（url/duration/offset/keyIndex）；**PlayListChangeCallback**：Playlist 变更通知接口；**HlsSegmentType**：分片类型枚举（SEG_VIDEO/SEG_AUDIO/SEG_SUBTITLE） |
| 角色定义 | **HlsSegmentManager**：单流类型分片管理器，实现 PlayListChangeCallback；**HlsPlayListDownloader**：M3U8 下载与解析；**Downloader**：HTTP 请求执行 |
| 核心对象 | playList_(BlockingQueue<PlayInfo>)：待下载分片队列；cacheMediaBuffer_(CacheMediaChunkBufferImpl)：数据缓冲区 |

## 原理（Principles）

| 子维度 | 内容 |
| --- | -- |
| 核心机制 | **生产者-消费者模型**：HlsPlayListDownloader 解析 M3U8 后将 PlayInfo 推入 playList_ 队列，HlsSegmentManager 从队列取出 PlayInfo 创建 DownloadRequest 下载，数据通过 SaveData 写入 CacheBuffer |
| 架构组成 | HlsSegmentManager 持有：playlistDownloader_(Playlist 下载)、downloader_(分片下载)、cacheMediaBuffer_(缓冲区)、aesDecryptor_(解密器) |
| 关键流程 | 1. OnPlayListChanged 回调触发 → 2. PushPlayInfo 推入 playList_ → 3. 下载线程从 playList_ 取出 PlayInfo → 4. PutRequestIntoDownloader 发起下载 → 5. SaveData 写入缓冲区（若加密先解密）→ 6. Read 从缓冲区读取 |
| 数据/控制流 | Playlist更新 → PlayInfo队列 → Downloader → SaveData → (AES解密) → CacheBuffer → Read |

### AES 解密流水线

```
下载加密数据 → SaveEncryptData → singleBlock_ 填充（16字节对齐）
                                → DecryptData → AES-128-CBC 解密
                                → decryptBuffer_（256字节批量解密）
                                → SaveCacheBufferData → 写入 CacheBuffer
```

### 缓冲区满处理与下载流控

1. 缓冲区满 → CacheBufferFullLoop 等待消费
2. 缓冲区可用空间低于 GetDownloadThrottleThreshold(25% 总内存) → isNeedResume_=true → 暂停写入
3. Read 消费后可用空间超过 GetDownloadResumeThreshold(50% 总内存) → isNeedResume_=false → 恢复下载
4. SourceLoader 模式下 writeOffset_-readOffset_ 低于 GetsourceLoaderClearThreshold(30% 总内存) → ClearMiddleReadFragment 回收已读 Fragment

## 使用方式（Usage）

| 子维度 | 内容 |
| --- | -- |
| 接入条件 | M3U8 URL 已获取；HlsMediaDownloader 已创建 |
| 配置步骤 | 1. 创建 HlsSegmentManager(expectBufferDuration, userDefinedDuration) → 2. Init() → 3. Open(url, httpHeader) → 4. SetCallback 设置事件回调 |
| 核心API | `HlsSegmentManager::Open` — 打开 M3U8 并开始下载；`HlsSegmentManager::Read` — 从缓冲区读取数据；`HlsSegmentManager::SeekToTime` — Seek 到指定时间；`HlsSegmentManager::SelectBitRate` — 切换码率 |
| 典型流程 | 1. 用户点击播放 → 2. Open(master.m3u8) → 3. 解析 Master → 选择 Variant → 4. 下载 Media Playlist → 5. 逐个下载分片 → 6. 解密（如需）→ 7. 写入缓冲 → 8. Read 返回数据 |

## 代码关联（Code）

| 子维度 | 内容 |
| --- | -- |
| 核心类/接口 | `HlsSegmentManager`（hls/hls_segment_manager.h）；`PlayListChangeCallback`（hls/playlist_downloader.h） |
| 关键配置文件 | 无独立配置，缓冲参数通过构造函数传入 |
| 代码结构 | hls/hls_segment_manager.cpp — 分片调度与缓冲管理；hls/hls_playlist_downloader.cpp — M3U8 解析；hls/m3u8.cpp — M3U8 数据模型 |

## 约束与限制（Constraints）

| 子维度 | 内容 |
| --- | -- |
| 系统限制 | AES 解密仅支持 AES-128-CBC 模式；fMP4 头数据需在 X-MAP 标签中指定 |
| 环境依赖 | 依赖 Downloader 执行 HTTP 请求；依赖 AesDecryptor 执行解密；依赖 CacheMediaChunkBufferImpl 做缓冲管理 |
| 性能约束 | 解密缓冲区固定 256 字节（AES_DECRYPT_LEN）；分片队列默认无上限（由 Playlist 内容决定）；CopyFragmentInfo 在 NotifyListChange 循环中按分片数调用，url 拼接须用 `reserve(2*INT64_MAX_DIGITS + URL_SEPARATORS + uri.size())` + `append` 预分配，禁止 `std::to_string + operator+` 链式拼接产生多次堆分配 |
| 安全约束 | 密钥通过 HTTPS 下载；IV 可为显式或隐式（序列号） |
| 时长单位约束 | **HLS fragment duration 内部统一纳秒（int64_t ns）**；dateRange->duration/start 为 ns，传入期望毫秒的 AVTimedMetaData 时必须 `/ MS_2_NS` 显式缩放，禁止隐式截断 |
