# 编码铁律

## 通用编码铁律

- **禁止**在下载回调中执行阻塞操作，采用异步投递到 Task 线程处理（SaveData 仅写缓冲区，不等待播放线程）
- **禁止**在多线程场景下直接读写共享缓冲区，采用 Mutex + ConditionVariable 保护（RingBuffer 用 cvReadWrite_，CacheBuffer 用 mutex_）
- **禁止**在 HttpSourcePlugin::Read 中持有锁的同时调用 MediaDownloader 的阻塞方法，采用先查询再读取的模式
- **禁止**硬编码缓冲区大小常量，采用 PlayStrategy 配置或 HlsSegmentManager 的 MIN/MAX_BUFFER_SIZE 常量
- **禁止**在日志中输出完整 URL（含查询参数可能含 Token），采用脱敏处理或仅输出域名路径部分
- **禁止**忽略 DownloadRequest 的错误码，采用检查 clientError_/serverError_ 并走重试或上报流程
- **禁止**在析构函数中执行耗时操作，采用 Stop → 异步 Close → 等待线程退出

## 安全编码铁律

- **禁止**信任从 libcurl 回调中获取的 HTTP 响应数据长度，采用按实际写入长度校验
- **禁止**将未校验的 URL 直接传递给 HTTP 请求，采用 IsAllowedProtocol 黑名单校验（阻止 file:// 等本地协议）
- **禁止**在 AES 解密中泄露密钥或 IV 到日志，采用仅输出密钥索引或长度
- **禁止**使用 -fno-stack-protector，采用 -fstack-protector-all 编译选项
- **禁止**在符号导出版本脚本（.map 文件）中导出内部符号，采用仅导出插件注册接口

## 性能编码铁律

- **禁止**在 Read 主路径上执行同步磁盘 I/O 或同步网络请求，采用预加载 + 缓冲区策略
- **禁止**在每次 Read 调用时创建新的 DownloadRequest，采用复用或队列管理
- **禁止**在 libcurl RxBodyData 回调中分配大内存，采用预分配缓冲区或直接写入 RingBuffer/CacheBuffer
- **禁止**在 Seek 操作中重新下载已缓冲的数据，采用先检查缓冲区命中再发起网络请求

## 内存编码铁律

- **禁止**注册回调后不在析构时反注册，采用 Close/Stop 中统一清理 Downloader、DownloadRequest、Task
- **禁止**CacheMediaChunkBuffer 的 Fragment 和 Chunk 内存泄漏，采用 Clear/Reset 时释放所有 FragmentCacheBuffer 和 freeChunks_
- **禁止**AES 解密缓冲区越界，采用 singleBlock_（16字节）+ decryptBuffer_（256字节）定长缓冲，写入前校验长度
- **禁止**HlsSegmentManager 的分片下载队列无限增长，采用 backPlayList_ 容量限制和已播放分片清理
- **禁止**Downloader 的请求队列无限增长，采用 BlockingQueue 有界队列

## 并发编码铁律

- **禁止**跨线程访问 Downloader 的 currentRequest_ 不加锁，采用 downloadRequestMutex_ 保护
- **禁止**在 Hold 住 mutex_ 的同时调用阻塞等待，采用 ConditionVariable 的 wait/wait_for 并在回调中重新检查条件
- **禁止**DashSegmentDownloader 的 segmentList_ 跨线程无锁访问，采用 segmentMutex_ 保护
- **禁止**HlsPlayListDownloader 的 master_ 和 currentVariant_ 跨线程无锁访问，采用 masterMutex_ 和 currentVariantMutex_ 保护
