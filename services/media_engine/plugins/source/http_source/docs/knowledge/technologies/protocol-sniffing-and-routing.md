# 协议嗅探与路由：独立探测后重新下载

## 当前行为

SetDownloaderBySource 根据选定资源的内容创建 HTTP、HLS 或 DASH 下载器。SnifferCoordinator 只负责独立探测，结束后清理请求及缓冲。正式下载经 DownloadMonitor::Open 从头开始，不接管探测会话或探测数据。

## 来源与识别

| 来源或特征 | 行为 |
| --- | --- |
| 网络来源 | 普通 GET，取得可靠结果后主动结束，不依赖服务端 Range 支持 |
| 本地 APPLICATION_M3U8 + fd:// | 共享 FdSource 校验 URI 和 offset/size，用 pread 探测，正式读取保留本地路径 |
| EXTM3U 签名 | HLS |
| MPD 根元素 | DASH；支持 XML 声明、注释、doctype 和 namespace 前缀 |
| 合法 FLV 文件头 | HTTP 并携带内部 FLV 标记，用于正式下载缓冲选择 |
| 其他内容 | 普通 HTTP；未完成的 XML 前缀继续有限读取，不按 URL 后缀猜测 |

初始读取目标为 256B，前缀足够明确时立即结束；长 XML 前导最多保留 16KiB。自然结束的短清单按实际数据判断，不强制要求 256B。对外 HTTP/HLS/DASH 枚举和 C 函数保留兼容。

## 生命周期与线程

探测使用独立 Downloader。单一单调时钟截止时间与成功、超时、取消、失败终态由 SnifferCoordinator 管理，WaitResult 独立等待到期，dataReceive 同时检查时间。客户端不接收嗅探专用控制对象。主动返回 0 前先发布终态；随后 response=23 只保留诊断，不覆盖主动终态、不触发重试。未主动结束且没有同时上报 302 时，23 仍按真实写失败处理。到期不得以部分前缀兜底成功。

控制路径在协调锁之外调用 Downloader::ForceClose，通过既有 Close(true)/Deinit 发起中断，等待 Task 当前 Job 结束后再清理请求、Loader UUID 和探测缓冲，正式初始化前释放协调器。Curl 同步请求期间由 Job 的局部 Downloader 强引用保活；AppClient 的 RespondHeader/RespondData/FinishLoading 遵守同一用户已确认的生命周期契约。Coordinator 的数据、状态和完成回调采用弱指针 lambda，不强持有业务目标。

Downloader 的 clientMutex_ 使用 shared_mutex，GetClient 在读锁内返回 shared_ptr 快照，SetClient 在写锁内交换指针；锁外调用客户端接口并释放被替换实例。Curl/AppClient 不新增整请求锁或整回调锁，Downloader 不使用整段 ForceClose 锁；探测关闭由协调器串行收尾。通用取消标志保留，既有控制入口使用客户端快照。AppClient 的 Loader Read 恢复到原状态锁内执行，沿用原异步响应约定。HttpSourcePlugin 的 activeSniffer 同样通过 getter/setter 和读写锁取得或替换快照，Stop 及旧对象释放均在锁外。重定向和瞬态重试均先关闭旧探测，再新建尝试，截止时间不重置，前缀不拼接。取消或超时不得初始化正式下载器。

WaitResult 在停止并清理 Downloader 后返回分类结果。下载回调不执行同步 Pause/Stop。Downloader 不含 probeOnly_、探测专用构造参数、HTTP 状态过滤或强制完成通知，统一使用普通下载器的响应、重定向、头部、EOS 与析构路径。普通 chunked EOS 仅置标记而不通知 DownloadDone，短的未识别内容会等到协调器截止；HTTP 错误响应体可能进入内容识别。关闭期间被普通逻辑抑制的响应通知，其错误码在协调器收尾后从请求对象读取。

## 探测阶段的 302 兼容边界

SnifferCoordinator 仅把 Downloader 上报的 serverError=302 作为显式跳转通知；不自行解析 HTTP 状态行或 Location，不修改通用 Downloader、HttpCurlClient 或其他三方网络库的重定向行为。旧 Downloader 可能把不同跳转状态归一化为 302，因此该值不保证是服务器原始状态码。

尚在 Probing 且未到期时，302 通知优先于短写 23 进入协调等待流程；成功、超时、取消等已发布终态不被迟到通知覆盖。等待线程先在状态锁外 ForceClose 旧请求，之后读取旧请求已解析的 URL 和 Location。目标缺失则失败；否则再次检查终态和截止时间，清空前缀并新建探测请求，保留请求头和应用身份。最多重建八次，始终共用同一截止时间。

本轮不保证消除 Curl 自动跟随与旧显式跳转叠加造成的额外 GET，也不能恢复旧 Downloader 已覆盖的原始错误码。正式下载仍使用选定资源地址并沿用既有流程，探测跳转地址和会话不移交。

## 正式下载

1. 按播放策略选择实际地址，再嗅探该地址。
2. 创建下载器和 DownloadMonitor，配置回调、策略、UID 等上下文。
3. 配置媒体流信息并发送 STREAM_UPDATE，再调用 Open。
4. IsHls、IsSeekToTimeSupported 及 HLS Pause/Resume 使用保存的实际协议类型。

HTTP-FLV 正式请求新建会话，完整消费该请求自己的头部和后续媒体字节；探测字节全部丢弃。FLV 文件头表示容器类型，直播状态结合正式响应长度判断。

## 代价与范围

通常增加一次小数据探测请求及连接开销。原 AdoptFromSniffer、FeedManifest、AdoptMpdDownloader 等接管接口已移除，会话移交延期重新设计。父版本已有的外层错误返回及播放器错误上报问题暂缓，不作为本轮修复的一部分。

## 验证

重点验证独立探测与正式请求返回不同内容时，正式读取仍与正式响应逐字节一致并读到真实 EOF；同时覆盖短清单、无后缀协议、本地 FD、重定向、请求头、超时、取消和应用 Loader。仅比较对象非空或保存的清单文本不足以证明路由及解析正确。
