# DASH MPD 解析与 Period 管理

## 解决的问题（Problem）

| 子维度 | 内容 |
| --- | -- |
| 业务痛点 | DASH 的 MPD 清单采用 XML 格式，层级复杂（MPD → Period → AdaptationSet → Representation → Segment），不同组合方式（SegmentTemplate/SegmentList/SegmentBase/BaseURL）导致分片信息提取逻辑差异大 |
| 技术目标 | 建立 **分层 MPD 解析架构**，将 Period/AdaptationSet/Representation 的解析职责分离到独立 Manager，统一处理 SegmentTemplate/SegmentList/SegmentBase 三种分片描述方式 |
| 适用场景 | DASH 点播/直播 MPD 解析；多 Period 内容切换；Sidx 索引解析；HDR/SDR 流识别 |

## 核心概念与术语（Concepts）

| 子维度 | 内容 |
| --- | -- |
| 关键术语 | **MPD**：Media Presentation Description，DASH 清单文件；**Period**：时间段，一个独立的媒体段落；**AdaptationSet**：自适应集，同一 Period 内可互相切换的编码流集合；**Representation**：表示，一个具体的编码版本（码率/分辨率/编码格式） |
| 角色定义 | **DashMpdParser**：MPD XML 解析入口；**DashMpdManager**：MPD 顶层管理；**DashPeriodManager**：Period 级解析；**DashAdptSetManager**：AdaptationSet 级解析；**DashRepresentationManager**：Representation 级解析 |
| 核心对象 | mpdInfo_(DashMpdInfo)：解析后的 MPD 结构化数据；streamDescriptions_(vector<DashStreamDescription>)：所有可用流的描述 |

## 原理（Principles）

| 子维度 | 内容 |
| --- | -- |
| 核心机制 | **分层解析 + 职责链**：DashMpdParser 负责 XML → 对象树的初步解析，DashMpdManager/PeriodManager/AdptSetManager/RepresentationManager 逐层细化，每层 Manager 负责自身层级的结构解析和下级 Manager 的创建 |
| 架构组成 | DashMpdDownloader 持有：mpdParser_(XML解析)、mpdManager_(MPD管理)、periodManager_(Period管理)、adptSetManager_(AdaptationSet管理)、representationManager_(Representation管理) |
| 关键流程 | 1. DoOpen 下载 MPD → 2. ParseManifest → mpdParser_->ParseMpd → 3. GetStreamsInfoInMpd → 遍历 Period → 4. GetStreamsInfoInPeriod → 遍历 AdaptationSet → 5. GetStreamsInfoInAdptSet → 遍历 Representation → 6. GetSegmentsInRepresentation → 按分片描述方式提取分片 |
| 数据/控制流 | MPD XML → DashMpdParser → DashMpdInfo → DashMpdManager → PeriodManager → AdptSetManager → RepresentationManager → DashStreamDescription + DashSegment |

### 分片描述方式处理

| 方式 | 解析方法 | 适用场景 |
| -- | -- | -- |
| SegmentTemplate + SegmentTimeline | GetSegmentsWithTmpltTimelineStatic | 直播/点播，分片时长不固定 |
| SegmentTemplate + Duration | GetSegmentsWithTmpltDurationStatic | 点播，分片时长固定 |
| SegmentTemplate + media 属性 | GetSegmentsWithTmpltStatic | 点播，带编号模板 |
| SegmentList | GetSegmentsWithSegList | 点播，分片 URL 显式列出 |
| SegmentBase + Sidx | GetSegmentsWithSegBase + ParseSidx | 点播，Sidx 索引框 |
| BaseURL | GetSegmentsWithBaseUrl | 单文件 DASH |

### Period 切换

```
播放推进 → 检测当前 Period 将结束
                ↓
FindTargetPeriodIndex → 计算目标 Period
                ↓
SwitchStreamIfNeeded → 切换 Representation
                ↓
新 Period 的分片开始下载 → 解码器可能需要重启
```

## 使用方式（Usage）

| 子维度 | 内容 |
| --- | -- |
| 接入条件 | MPD URL 已获取 |
| 核心API | `DashMpdDownloader::Open(url)` — 下载并解析 MPD；`DashMpdDownloader::GetStreamInfo` — 获取所有流信息；`DashMpdDownloader::GetNextSegmentByStreamId` — 获取下一个分片；`DashMpdDownloader::SeekToTs` — Seek 到指定时间 |
| 典型流程 | 1. Open(mpd.xml) → 2. 下载 MPD → 3. 解析 Period/AdaptationSet/Representation → 4. 选择默认流 → 5. 获取分片列表 → 6. 开始下载 |

## 代码关联（Code）

| 子维度 | 内容 |
| --- | -- |
| 核心类/接口 | `DashMpdDownloader`（dash/dash_mpd_downloader.h）；`DashMpdParser`（dash/include/mpd_parser/dash_mpd_parser.h）；`DashMpdManager`（dash/include/mpd_parser/dash_mpd_manager.h） |
| 代码结构 | dash/dash_mpd_downloader.cpp — MPD 下载与调度；dash/mpd_parser/ — MPD 解析器群 |

## 约束与限制（Constraints）

| 子维度 | 内容 |
| --- | -- |
| 系统限制 | libxml2 解析大 MPD 文件可能消耗较多内存；Sidx 解析需额外 HTTP Range 请求 |
| 环境依赖 | 依赖 libxml2 做 XML 解析；依赖 Downloader 做 MPD 和 Sidx 下载 |
| 性能约束 | MPD 解析为同步操作，复杂 MPD 可能耗时数百毫秒 |
| 安全约束 | MPD 中的 URL 需校验协议合法性（阻止 file:// 等本地协议）；DRM PSSH 数据需正确提取 |
