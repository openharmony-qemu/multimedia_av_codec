# 下载重试与监控（DownloadMonitor）

## 解决的问题（Problem）

| 子维度 | 内容 |
| --- | -- |
| 业务痛点 | 网络环境不稳定导致 HTTP 请求频繁失败（连接超时、DNS 解析失败、SSL 错误、服务器 5xx 等），直接上报错误会导致播放中断，影响用户体验 |
| 技术目标 | 建立 **下载监控与自动重试机制**，在可恢复的网络错误场景下透明重试，在不可恢复错误时精准映射并上报业务错误码 |
| 适用场景 | 网络波动导致下载中断；DNS 解析暂时失败；服务器临时不可用；SSL 证书校验失败 |

## 核心概念与术语（Concepts）

| 子维度 | 内容 |
| --- | -- |
| 关键术语 | **DownloadMonitor**：MediaDownloader 的装饰器，透明添加重试和监控能力；**clientErrorCodeMap_**：libcurl 错误码到业务错误码的映射表；**serverErrorCodeMap_**：HTTP 状态码到业务错误码的映射表 |
| 角色定义 | **DownloadMonitor**：装饰器角色，持有真实 MediaDownloader；**Downloader**：被监控的底层下载器 |
| 核心对象 | retryTasks_(list<RetryRequest>)：待重试请求队列；task_(Task)：监控线程 |

## 原理（Principles）

| 子维度 | 内容 |
| --- | -- |
| 核心机制 | **装饰器模式 + 监控线程**：DownloadMonitor 实现所有 MediaDownloader 接口，内部委托给真实 downloader_ 执行，同时启动 HttpMonitorLoop 监控线程定期检查下载状态 |
| 架构组成 | DownloadMonitor 持有：downloader_(真实 MediaDownloader)、retryTasks_(重试队列)、task_(监控 Task)、clientErrorCodeMap_/serverErrorCodeMap_(错误码映射) |
| 关键流程 | 1. 下载请求发送 → 2. OnDownloadStatus 回调触发 → 3. NeedRetry 判断是否可重试 → 4. 可重试：加入 retryTasks_ → 监控线程延迟重试 → 5. 不可重试：ReportErrorAndClose 映射错误码 → NotifyError 上报 |
| 数据/控制流 | 下载失败 → OnDownloadStatus → NeedRetry? → 是：retryTasks_ → 延迟重试；否：错误码映射 → 上报 |

### 错误码映射策略

| 错误源 | 映射规则 | 示例 |
| -- | -- | -- |
| libcurl 错误码 | clientErrorCodeMap_ 映射 | -6 → MSERR_IO_SSL_SERVER_CERT_UNTRUSTED |
| HTTP 状态码 | serverErrorCodeMap_ 映射 | 404 → MSERR_IO_RESOURE_NOT_FOUND |
| 重试限制 | CheckRetryLimitReached 判断 | 超过 HeaderInfo::maxRetryTimes(100) 不再重试 |

### 监控线程循环

1. HttpMonitorLoop 每秒检查一次
2. 检查 retryTasks_ 是否有待重试请求
3. 检查 isPlaying_ 状态和 lastReadTime_
4. 执行重试或超时清理

## 使用方式（Usage）

| 子维度 | 内容 |
| --- | -- |
| 接入条件 | 由 HttpSourcePlugin 在创建 MediaDownloader 时自动包装，无需外部手动创建 |
| 配置步骤 | 由 HttpSourcePlugin 内部的 CreateXxxDownloader 方法自动完成：1. 创建真实 MediaDownloader → 2. 用 DownloadMonitor 包装 → 3. 存入 downloader_ 成员 |
| 核心API | `DownloadMonitor(downloader)` — 构造并包装真实下载器；所有 MediaDownloader 接口透明代理 |
| 典型流程 | 1. 下载请求失败（DNS 超时）→ 2. OnDownloadStatus 回调 → 3. NeedRetry=true → 4. 加入 retryTasks_ → 5. 监控线程 3s 后重试 → 6. 重试成功 → 数据继续写入 |

## 代码关联（Code）

| 子维度 | 内容 |
| --- | -- |
| 核心类/接口 | `DownloadMonitor`（monitor/download_monitor.h） |
| 代码结构 | monitor/download_monitor.cpp — 监控与重试逻辑 |

## 约束与限制（Constraints）

| 子维度 | 内容 |
| --- | -- |
| 系统限制 | 装饰器模式增加一层间接调用；错误码映射表需与 libcurl 版本和 HTTP 规范保持同步 |
| 环境依赖 | 依赖 Task 线程执行监控循环 |
| 性能约束 | 监控线程每秒唤醒一次，增加系统负载；重试队列无限长可能导致内存增长 |
| 安全约束 | 错误码映射需准确，避免将安全相关错误（如 SSL 证书错误）映射为可重试错误 |
