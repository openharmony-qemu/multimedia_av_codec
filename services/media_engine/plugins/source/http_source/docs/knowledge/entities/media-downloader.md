# MediaDownloader（媒体下载器）

## 实体概念

| 子维度 | 内容 |
| --- | -- |
| 实体名称 | MediaDownloader（媒体下载器抽象基类） |
| 实体定义 | 协议无关的媒体下载抽象接口，定义 Open/Close/Read/Seek/码率切换等纯虚方法，HttpMediaDownloader/HlsMediaDownloader/DashMediaDownloader 分别实现对应协议 |
| 核心特征 | 纯虚接口类；大量虚方法提供默认实现（日志提示未实现）；定义 ReadDataInfo 结构体传递读取上下文 |
| 类型/分类 | 抽象基类 |

## 上下文与场景

| 子维度 | 内容 |
| --- | -- |
| 使用场景 | HttpSourcePlugin 持有 MediaDownloader 指针，通过多态调用协议相关实现 |
| 角色与参与方 | HttpSourcePlugin（调用方）：通过基类指针调用；子类（实现方）：实现协议相关逻辑 |
| 生命周期 | 随 HttpSourcePlugin::SetSource 创建 → 随 Close/Reset 销毁 |
| 交互流程 | 1. HttpSourcePlugin 创建具体子类 → 2. 调用 Init → 3. 调用 Open → 4. 循环调用 Read → 5. 调用 Close |
| 状态流转 | CREATED → INITIALIZED → OPENED → READING/SEEKING → CLOSED |

## 规格与约束

| 子维度 | 内容 |
| --- | -- |
| 系统限制 | 必须由 HttpSourcePlugin 管理，不可独立使用 |
| 业务规则 | 子类必须实现所有纯虚方法（Init/Open/Close/Read/SetCallback/SetInterruptState 等）；可选方法有默认空实现 |
| 性能约束 | Read 方法在播放线程调用，不可长时间阻塞 |

## 知识关联

| 子维度 | 内容 |
| --- | -- |
| 上层依赖 | HttpSourcePlugin、DownloadMonitor |
| 下游影响 | HttpMediaDownloader、HlsMediaDownloader、DashMediaDownloader |
| 平级关联 | Downloader（底层 HTTP 执行器） |

## 数据模型

| 子维度 | 内容 |
| --- | -- |
| 核心数据结构 | ReadDataInfo{streamId_, ffmpegOffset, wantReadLength_, realReadLength_, isEos_} |
| 数据流转路径 | HttpSourcePlugin → MediaDownloader::Read → 具体子类实现 → 返回数据 |

## 代码与符号

| 子维度 | 内容 |
| --- | -- |
| 核心API/类 | `MediaDownloader`（media_downloader.h）|
| 核心符号映射 | Read → 各子类 Read 实现；SeekToTime → HlsMediaDownloader/DashMediaDownloader 实现 |
