# 自适应码率切换（ABR）

## 解决的问题（Problem）

| 子维度 | 内容 |
| --- | -- |
| 业务痛点 | 网络带宽波动时，固定码率播放会导致频繁卡顿（码率过高）或画质浪费（码率过低），需要根据实时带宽动态选择最优码率 |
| 技术目标 | 建立 **自适应码率切换（ABR）机制**，根据实时下载速度和缓冲状态自动选择合适的码率，同时在手动切换码率时平滑过渡不中断播放 |
| 适用场景 | 弱网环境自动降码率；带宽恢复自动升码率；用户手动选择码率；多码率流初始码率选择 |

## 核心概念与术语（Concepts）

| 子维度 | 内容 |
| --- | -- |
| 关键术语 | **ABR（Adaptive Bitrate）**：自适应码率选择算法；**Smooth Switch**：平滑切换（等待当前分片完成再切换）；**Auto Switch**：自动切换模式 |
| 角色定义 | **HttpMediaDownloader**：HTTP 多码率流的 ABR 实现；**HlsSegmentManager**：HLS 分片级码率切换；**DashMediaDownloader**：DASH 的 PreparedAction 码率切换机制 |
| 核心对象 | writeBitrateCaculator_(WriteBitrateCaculator)：实时写入码率计算器；isAutoSelectBitrate_：ABR 开关标志 |

## 原理（Principles）

| 子维度 | 内容 |
| --- | -- |
| 核心机制 | **带宽估计算法**：通过统计下载速度（downloadSpeed_）、已缓冲时长（cachedDuration_）和播放码率（readBitrate_）计算目标码率。ABR 模式下定期检查是否需要切换 |
| HTTP ABR 流程 | 1. CalcAverageSpeed 计算平均下载速度 → 2. CalcDesiredBitRate 计算目标码率 → 3. CheckAutoSelectBitrate 判断是否满足切换条件 → 4. SelectBitRate 执行切换 |
| HLS ABR 流程 | 1. HlsPlayListDownloader::GetBitRates 获取可用码率列表 → 2. HlsSegmentManager::AutoSelectBitrate 判断是否切换 → 3. 等待当前分片下载完成 → 4. 切换到新 Variant → 5. 重新下载新分片 |
| DASH ABR 流程 | 1. DashMpdDownloader::GetBitRates 获取可用码率 → 2. DashMediaDownloader::CheckAutoSelectBitrate 检查是否切换 → 3. 设置 DashPreparedAction → 4. DoPreparedAction 在合适时机执行切换 |

### DASH PreparedAction 机制

```
码率切换请求 → 设置 preparedAction_.preparedBitrateParam_
                        ↓
当前分片下载完成 → DoPreparedAction
                        ↓
DoPreparedSwitchBitrate → CleanSegmentBuffer → 切换到新 Representation
                        ↓
SeekInternal → 从新码率的对应时间点继续下载
```

### HLS 码率切换流程

```
SelectBitRate → isSelectingBitrate_ = true
                        ↓
当前分片下载完成 → HlsPlayListDownloader::SelectBitRate
                        ↓
切换 currentVariant_ → 新 Variant 的 Playlist
                        ↓
新分片开始下载 → isSelectingBitrate_ = false
```

## 使用方式（Usage）

| 子维度 | 内容 |
| --- | -- |
| 接入条件 | 流支持多码率（HLS Master Playlist / DASH 多 Representation） |
| 核心API | `SelectBitRate(bitRate)` — 手动选择码率；`AutoSelectBitRate(bitRate)` — 触发 ABR 切换；`SetIsTriggerAutoMode(isAuto)` — 开启/关闭 ABR 自动模式 |
| 典型流程 | 1. 播放器启动 → 默认选择中等码率 → 2. ABR 检测到带宽充足 → 自动切换到高码率 → 3. 网络变差 → ABR 切换到低码率 → 4. 用户手动选择最高码率 → 关闭 ABR |

## 代码关联（Code）

| 子维度 | 内容 |
| --- | -- |
| 核心类/接口 | HTTP: `HttpMediaDownloader`（http/http_media_downloader.h）；HLS: `HlsSegmentManager::AutoSelectBitrate`（hls/hls_segment_manager.h）；DASH: `DashMediaDownloader::SelectBitRate/DoPreparedAction`（dash/dash_media_downloader.h） |
| 辅助类 | `WriteBitrateCaculator`（utils/write_bitrate_caculator.h） |

## 约束与限制（Constraints）

| 子维度 | 内容 |
| --- | -- |
| 系统限制 | ABR 算法基于下载速度估算，在带宽剧烈波动时可能误判 |
| 环境依赖 | 依赖 WriteBitrateCaculator 的码率统计准确性 |
| 性能约束 | HLS 码率切换延迟取决于当前分片剩余时长；DASH 平滑切换需等待当前分片完成 |
| 安全约束 | 码率切换不应导致播放位置偏移；切换后缓冲区需正确清理 |
