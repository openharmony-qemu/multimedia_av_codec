# 视频编解码资源回收和抢占机制

## 1. 核心结论

Android 视频编解码抢占不是后台定期扫描，也不是达到某个内存阈值后主动执行，而是下面这条同步闭环：

```text
MediaCodec 创建、configure 或 start
        ↓
底层返回 NO_MEMORY
        ↓
MediaCodec 向 ResourceManagerService 请求回收
        ↓
先处理 secure 共存冲突
        ↓ 无冲突
按“pending removal → 跨进程优先级 → 同进程 importance”选择一个目标
        ↓
目标 MediaCodec 被 release
        ↓
申请方恢复状态并重试原操作
        ↓
成功则结束；仍为 NO_MEMORY 则再次回收；达到上限后返回失败
```

抢占的本质是：**底层先报告资源不足，资源管理器再根据申请方声明的资源类型寻找可牺牲的 codec，释放后由申请方重试。**

## 2. 完整抢占逻辑链

### 2.1 第一步：判断是否进入抢占流程

只有创建、configure 或 start 最终返回 `NO_MEMORY`，`MediaCodec` 才发起资源回收。参数非法、调用状态错误、格式不支持及其他非 `NO_MEMORY` 错误不会进入这条逻辑链。

| 失败阶段 | 申请方提交给资源管理器的资源请求 | 回收成功后重试内容 |
| --- | --- | --- |
| 创建 codec | secure/non-secure codec 类型，以及硬件/软件、音频/视频/图片子类型 | 重新创建底层 component |
| configure | codec 类型和图形内存 | reset，恢复 callback，重新 configure |
| start | codec 类型和图形内存 | reset，恢复 callback，重新 configure，再重新 start |

configure/start 中携带的图形内存值为占位值 `1`，用于寻找持有相关资源的实例，不表示框架知道实际缺少 1 字节，也不证明失败一定由图形内存不足导致。

这里不上报“具体缺少多少图形内存”有两个原因。第一，底层返回给 `MediaCodec` 的只是 `NO_MEMORY`，并没有标准接口说明真实缺口是图形 buffer、线性 buffer、厂商内部池还是硬件上下文。第二，start 失败时即使框架能根据宽高和 buffer 数估算图形 buffer 规模，也只是估算本 codec 可能占用多少，不等于底层本次申请失败的精确缺口；厂商内部 buffer、对齐、额外参考帧、secure 内存和 Surface/allocator 的真实分配策略都可能不在框架可见范围内。因此当前代码明确使用 `GraphicMemoryResource(1)` 作为类型占位，回收路径暂不按申请方 buffer size 做容量规划。

MediaCodec 对创建、configure、start 最多执行 3 次：第 1 次是原始调用，后面最多再重新执行 2 次。create/start 通常是在下一次执行前先回收资源；configure 则是在每次返回 `NO_MEMORY` 后立即请求回收，因此第 3 次 configure 失败后也可能再触发一次回收，但不会继续执行第 4 次 configure。最终对应用可见的结果是：成功则返回成功，3 次执行后仍失败则返回最终错误。

### 2.2 第二步：先裁决 secure 共存冲突

资源管理器收到回收请求后，首先检查设备声明的 secure codec 共存能力。这一步优先于 pending removal、普通资源匹配和单实例优先级选择。

回收请求从 `MediaCodec` 进入服务端后的接口参数和代码调用链见第 5、6 节。本节先描述策略顺序。

| 新申请 | 设备限制 | 冲突集合 |
| --- | --- | --- |
| secure codec | 不支持多个 secure codec | 已有相同 codec 子类型的 secure codec |
| secure codec | 不支持 secure 与 non-secure 共存 | 已有相同 codec 子类型的 non-secure codec |
| non-secure codec | 不支持 secure 与 non-secure 共存 | 已有相同 codec 子类型的 secure codec |

对冲突集合执行统一裁决：

1. 跳过申请者自身。
2. 逐个比较申请进程与冲突实例所属进程的 OOM 优先级。
3. 只有申请进程优先级严格高于冲突实例，冲突实例才可回收。
4. 只要存在一个同级或更高优先级的冲突实例，整次 secure 冲突抢占立即失败，已经收集的目标清空，不做部分回收。
5. 如果全部冲突实例都低于申请进程优先级，则把全部冲突实例作为目标，一次性回收。

因此 secure 冲突不是“从候选者中选一个最大实例”，而是：

```text
存在共存冲突
    ├─ 冲突集合中有任一实例不可抢占 → 整体失败
    └─ 全部实例都可抢占             → 全部回收
```

secure 冲突处理完成并产生目标后，不再进入普通单实例搜索。只有没有 secure 共存冲突时，才继续下一步。

### 2.3 第三步：建立普通回收候选集合

普通候选不是所有 codec，而是资源登记记录能够匹配本次请求中任一相关资源范围的客户端。对视频 codec 请求，服务端会先把以下范围内的 client 汇总并去重，形成一个候选全集 `clients`：

1. 持有请求中关联资源的实例。当前 create/configure/start 的 `NO_MEMORY` 回收请求里，视频/图片 codec 通常只带图形内存这一类关联资源；Codec2 系统资源可以登记在已有 client 的 `ResourceList` 中，但这条回收请求路径不一定把它作为“缺少的资源”传入。关联资源层匹配的是关联资源本身的 `type`，例如 `kGraphicMemory`，同时用申请方 codec 主资源的 `subType` 作为 `primarySubType` 过滤目标 client 的主 codec 类别；这一层不要求目标 client 的 secure/non-secure codec `type` 与申请方一致。
2. 持有与申请方相同 secure 属性、相同 codec 子类型的实例。
3. 持有另一 secure 属性、相同 codec 子类型的实例，作为前两类找不到目标时的最后降级选择。

例如，请求是 `secure + hardware video codec + graphic memory`，其中 `primarySubType = kHwVideoCodec`，候选搜索顺序可以概括为：

```text
第一层：持有 graphic memory，并且主 codec 类别匹配 kHwVideoCodec 或同硬件类别的客户端；不要求 secure/non-secure type 相同
第二层：持有 secure + hardware video codec 的客户端
第三层：持有 non-secure + hardware video codec 的客户端
```

因此第一层可能包含 `secure + hardware video + graphic memory`，也可能包含 `non-secure + hardware video + graphic memory`，代码上还可能包含同属硬件类别的图片 codec client；它不是按 `kSecureCodec + kHwVideoCodec` 这个 codec 主资源做严格匹配。候选全集只说明这些客户端“可能释放相关资源”，不保证每个 client 都持有当前策略正在尝试的那一个资源项，也不保证释放一个实例后一定满足实际缺口。

核心原因是：框架并不知道底层 `NO_MEMORY` 的真实缺口到底是什么。它只能根据当前阶段构造“可能相关”的资源请求。第 2、3 类是降级目标，不要求目标 client 同时持有第 1 类中的关联资源。也就是说，如果真实缺的是图形内存、Codec2 系统资源或厂商内部资源，但回收请求里没有精确表达该缺口，资源管理器仍可能退回去释放同类 codec 实例。这种释放可能释放厂商内部上下文、硬件 codec 槽位或未精确登记的资源，但也可能无法解决真实资源不足；是否有效只能通过申请方后续重试确认。

### 2.4 第四步：按固定策略顺序选择目标

候选准备完成后，普通抢占严格按以下顺序寻找目标，前一级成功后立即停止，不再比较后面的策略。

```text
优先级 1：申请者同进程的 pending removal 实例
    ↓ 没找到
优先级 2：其他进程中严格低优先级的实例
    ↓ 没找到
优先级 3：申请者同进程中 importance 更低的实例（新版机制）
    ↓ 没找到
本轮回收失败
```

#### 优先级 1：同进程 pending removal

资源管理器按申请请求中的资源顺序查找同进程 pending removal 实例。找到第一个资源类型存在匹配实例时，从这些实例中选择该资源 `value` 最大的一个，并立即返回。

这一层不比较进程 OOM 优先级和 codec importance，因为目标与申请方属于同一进程且已经进入释放流程。

#### 优先级 2：跨进程 OOM 优先级

如果没有可用的 pending removal 实例，执行跨进程选择。2.3 节的 `clients` 是多个资源范围汇总后的候选全集；这里是在这个全集内，按当前正在尝试的资源项选择最终目标：

1. 在候选全集中查找持有当前资源项的客户端。
2. 读取申请进程和这些客户端所属进程的 OOM 优先级。
3. 选择其中进程优先级最低的进程，即 OOM score 数值最大的进程。
4. 如果该进程的优先级没有严格低于申请进程，则当前资源项不可抢占，继续尝试下一项资源或下一层资源范围。
5. 如果允许抢占，在该进程内选择当前资源项 `value` 最大的客户端。
6. 选出一个客户端后立即结束目标搜索。

其完整降级顺序是：

```text
关联资源匹配范围、相同 codec 子类型
    ├─ 找到严格低优先级进程 → 选该进程内 value 最大的客户端
    └─ 未找到
         ↓
相同 secure 属性、相同 codec 子类型
    ├─ 找到严格低优先级进程 → 选该进程内 value 最大的客户端
    └─ 未找到
         ↓
另一 secure 属性、相同 codec 子类型
    ├─ 找到严格低优先级进程 → 选该进程内 value 最大的客户端
    └─ 未找到 → 进入同进程 importance 策略
```

这里不是先在所有候选中选择全局占用最大的 codec。顺序是**先确定最低优先级进程，再在该进程内选择匹配资源值最大的 codec**。

#### 优先级 3：同进程 codec importance

新版资源管理器默认在跨进程策略找不到目标后，继续检查申请者同进程的其他 codec：

1. 排除申请者自身和其他进程客户端。
2. 只保留 `importance` 数值严格大于申请者的客户端，即重要性更低的客户端。
3. 按“关联资源 → 相同 secure 类型 → 另一 secure 类型”的相同范围逐层查找。
4. 在首个可用范围内选择匹配资源 `value` 最大的客户端。
5. importance 相同或更重要的同进程 codec 不允许被抢占。

### 2.5 第五步：执行回收

选出目标后，资源管理器通过目标的 `IResourceManagerClient` 调用 `MediaCodec::reclaim()`，最终执行 release，销毁 OMX node 或 Codec2 component。

| 目标状态 | 回收行为 |
| --- | --- |
| 没有应用持有的输入/输出 buffer | 直接 release |
| 仍有应用持有的 buffer | 首次非强制回收返回 `WOULD_BLOCK` |
| 等待约 0.5 秒后仍未归还 | 再次强制回收，即使 buffer 仍在应用手中也释放 codec |
| 目标客户端已经消失 | 视为资源已经释放，跳过该客户端 |
| 目标回收调用失败 | 本轮回收失败，并清理失效的资源管理记录 |

secure 冲突可能包含多个目标，资源管理器依次回收；普通场景通常只选择一个目标。

### 2.6 第六步：申请方恢复并重试

目标释放成功后，发起申请的 `MediaCodec` 重试原操作：

| 原失败点 | 内部恢复路径 |
| --- | --- |
| 创建失败 | 直接重新申请底层 component |
| configure 失败 | reset 回到已初始化状态，恢复异步 callback，重新 configure |
| start 失败 | reset，恢复异步 callback，使用保存的参数重新 configure，再重新 start |

重试结果处理：

```text
重试成功                    → 整个操作成功
仍返回 NO_MEMORY 且还能回收 → 进入下一轮目标选择和回收
找不到目标或回收失败        → 向应用返回当前失败
返回其他错误                → 不再抢占，直接返回该错误
达到最多执行次数            → 向应用返回最终失败
```

## 3. 一张图看清全部优先级

```text
[创建/configure/start 返回 NO_MEMORY]
                    |
                    v
       [检查 secure codec 共存冲突]
          |                     |
       有冲突                 无冲突
          |                     |
          v                     v
[是否所有冲突实例都严格]   [同进程是否有匹配资源的
[低于申请进程优先级？]      pending removal 实例？]
     |             |              |          |
    否             是            是          否
     |             |              |          |
     v             v              v          v
 [整体失败]   [全部冲突实例] [选 value 最大] [跨进程策略]
                  |             |             |
                  |             |      [按资源范围逐层查找]
                  |             |             |
                  |             |      [先选 OOM 优先级最低进程]
                  |             |             |
                  |             |      [再选进程内 value 最大实例]
                  |             |          |          |
                  |             |        找到        未找到
                  |             |          |          |
                  |             |          |    [同进程 importance]
                  |             |          |          |
                  |             |          |    [仅选更不重要实例]
                  |             |          |          |
                  +-------------+----------+-----找到--+
                                                |
                                                v
                                      [release 目标 codec]
                                                |
                                                v
                                      [申请方恢复并重试]
```

## 4. 同类资源和匹配规则说明

### 4.1 codec 资源如何判断同类

codec 资源必须同时匹配 `type` 和 `subType`：

- `type`：`kSecureCodec` 或 `kNonSecureCodec`。
- `subType`：`kHwVideoCodec`、`kSwVideoCodec`、`kHwAudioCodec`、`kSwAudioCodec`、`kHwImageCodec` 或 `kSwImageCodec`。

例如：

| 申请资源 | 是否同类 | 原因 |
| --- | --- | --- |
| non-secure + HW video | non-secure + HW video：是 | type 和 subType 都相同 |
| non-secure + HW video | secure + HW video：否 | secure 属性不同，只能作为降级搜索范围 |
| non-secure + HW video | non-secure + SW video：否 | 硬件/软件子类型不同 |
| non-secure + HW video | non-secure + HW audio：否 | 视频/音频子类型不同 |

### 4.2 非 codec 资源如何判断同类

图形内存、DRM session、CPU boost 等非 codec 资源主要按 `type` 匹配，普通资源选择不再用 audio/video、hardware/software 子类型细分。

关联资源匹配还会检查持有者的主 codec 类别。当前实现将硬件视频/图片 codec 视为硬件类别；其余子类型按非硬件类别处理。它的作用是避免为了硬件视频 codec 请求，优先选到只持有图形内存但 codec 类别无关的客户端。这个检查使用申请方 codec 主资源的 `subType` 作为 `primarySubType`，只约束主 codec 类别或硬件类别，不要求目标 client 的 secure/non-secure codec `type` 与申请方一致。

### 4.3 `value` 的基本作用

`value` 只在已经确定策略层级、资源项和目标进程后，用于在同一进程内选择当前资源项占用最大的 client。它不是全局排序条件，也不会跨不同资源类型比较。

| 资源 | `value` 常见含义 |
| --- | --- |
| codec 资源 | 实例计数，通常为 1 |
| 图形内存 | 框架估算的图形内存字节数 |
| Codec2 扩展系统资源 | component store 上报的资源数量 |

图形内存的估算模型在 `MediaCodec::getGraphicBufferSize()` 中：

```text
graphicMemoryValue = (输入端 buffer 数 + 输出端 buffer 数) * width * height * 3 / 2
```

这里按 YUV420 的 1.5 bytes/pixel 粗略估算，源码注释也说明这只是估算，并不是从 ACodec 或 allocator 查询真实 GraphicBuffer 大小。configure/start 发起回收请求时使用的 `GraphicMemoryResource(1)` 也不是这个估算值；真实估算值通常在 start 完成后登记。

Codec2 扩展系统资源的 `value` 来自 component store 上报的 `C2ResourcesNeededTuning`：

- `CONST` 资源直接使用 `amount`。
- `PER_FRAME` 资源按 `amount * frameRate` 换算成静态数量。
- `PER_INPUT_BLOCK` 和 `PER_OUTPUT_BLOCK` 当前也临时按 per-frame 方式处理。

ResourceList 中相同 `type/subType/id` 的资源会合并，新增时累加，更新时覆盖，移除时扣减或删除。因此看到的 `value` 是资源管理器当前登记的计数或估算值，不是实时向底层查询的精确占用。

## 5. 从回收接口到 ResourceList 匹配

### 5.1 回收接口长什么样

`MediaCodec` 发起回收时调用资源管理服务的 AIDL 接口：

```cpp
boolean reclaimResource(
    in ClientInfoParcel clientInfo,
    in MediaResourceParcel[] resources);
```

其中：

- `clientInfo` 表示申请资源失败的当前 codec 客户端，包括 pid、uid、clientId、importance 等信息。
- `resources` 表示本次申请方认为相关的资源类型列表，不是精确缺口列表。

`MediaResourceParcel` 的核心字段是：

```cpp
MediaResourceType type;
MediaResourceSubType subType;
byte[] id;
long value;
```

因此申请方告知服务端的不是“我缺多少资源”，而是：

```text
我这次申请这些类型的资源失败了，请按这些资源类型寻找可回收目标。
```

### 5.2 不同失败阶段传入什么资源

| 失败阶段 | 传入 `resources` | 含义 |
| --- | --- | --- |
| 创建 codec | `CodecResource(secure, codecSubType)` | 需要对应 secure 属性、硬件/软件、音频/视频/图片类别的 codec 实例 |
| configure | `CodecResource(...)`，视频/图片还带 `GraphicMemoryResource(1)` | codec 配置失败，可能与 codec 实例或图形内存相关 |
| start | `CodecResource(...)`，视频/图片还带 `GraphicMemoryResource(1)` | 启动或初始 buffer 分配失败，可能与 codec 实例或图形内存相关 |

以普通硬件视频解码 start 失败为例，传入服务端的资源列表大致是：

```text
resources[0]
  type    = kNonSecureCodec
  subType = kHwVideoCodec
  value   = 1

resources[1]
  type    = kGraphicMemory
  subType = kUnspecifiedSubType
  value   = 1
```

这里 `resources[1].value = 1` 只是类型占位，不表示真实缺 1 字节图形内存。

### 5.3 服务端如何用 ResourceList 匹配

资源管理器内部保存的是每个 codec 客户端的资源登记：

```text
pid
└── clientId
    └── ResourceList
        ├── kNonSecureCodec / kHwVideoCodec / value=1
        ├── kGraphicMemory / value=xxx
        └── Codec2 扩展资源 / value=yyy
```

服务端收到回收请求后，不会修改申请方的资源，也不会直接按申请方的 `value` 计算缺口；它会拿请求中的每个 `MediaResourceParcel` 去匹配其他客户端的 `ResourceList`。

需要区分两类资源：

| 类别 | 含义 |
| --- | --- |
| 回收请求里的 `resources[]` | 申请方本次告诉资源管理器“可能缺少或相关”的资源类型。create 只带 codec 主资源；configure/start 的视频或图片路径通常带 codec 主资源和 `GraphicMemoryResource(1)` |
| 已有 client 的 `ResourceList` | 已运行 codec 实际登记过的资源，可能包含 codec 主资源、图形内存、Codec2 component store 上报的扩展系统资源等 |

也就是说，Codec2 扩展系统资源可以出现在已有 client 的 `ResourceList` 中，用于描述该 client 已占用什么；但当前 create/configure/start 失败发起的普通回收请求，通常并不会把这些 Codec2 扩展系统资源作为“本次缺少的资源”一起传给资源管理器，除非代码路径显式把它们放进 `resources[]`。

匹配规则可以概括为：

| 请求资源 | ResourceList 匹配方式 |
| --- | --- |
| codec 资源 | 必须同时匹配 `type` 和 `subType`，例如 `kNonSecureCodec + kHwVideoCodec` |
| 图形内存等非 codec 资源 | 主要匹配 `type`，例如是否持有 `kGraphicMemory` |
| 关联资源匹配 | 除了匹配附属资源，还要求持有者的主 codec 类别与申请方匹配，例如硬件视频请求优先匹配硬件 codec 客户端；这一层不要求目标 codec 的 secure/non-secure `type` 与申请方一致 |
| secure 冲突 | 不按普通单项最大值匹配，而是按 secure/non-secure 共存策略收集冲突集合 |

### 5.4 多资源请求的匹配顺序

对 codec 请求，代码行为可以概括为：

| 策略 | 资源尝试顺序 | 说明 |
| --- | --- | --- |
| pending removal | 按 `resources[]` 原顺序 | 通常先看 `resources[0]` codec 资源，再看图形内存等后续资源 |
| 跨进程 OOM 优先级 | 先 `resources[1..]`，再 `resources[0]`，最后另一 secure 类型 | `resources[1..]` 是本次请求携带的关联资源；当前 create/configure/start 视频路径通常是图形内存占位资源 |
| 同进程 importance | 与跨进程策略相同 | 只在申请者同进程内选择 importance 更低的 client |

“关联资源”不是 `MediaResourceParcel` 字段，而是数组约定：当 `resources[0]` 是 codec 主资源时，`resources[1]` 及后续资源会被按顺序当作关联资源尝试。

如果请求包含多个关联资源，服务端不会比较它们谁的 `value` 更大，也不会要求目标 client 同时拥有所有关联资源。它按数组顺序逐项尝试：当前资源项能找到可抢占目标就停止；找不到才继续下一项。

例如 `resources[1] = kGraphicMemory`、`resources[2] = Codec2ResourceX`。如果图形内存层已找到 client A，就不会继续比较 Codec2ResourceX 层的 client B。只有同一个资源项内部才比较 `value`，不同资源类型的 `value` 没有统一量纲。

### 5.5 选中 clientId 后回收什么

`ResourceList` 用于选目标，但执行回收的单位是 `clientId` 对应的整个 `MediaCodec` 实例。

也就是说，不存在只释放某个资源项的流程：

```text
不会只释放 kGraphicMemory，保留 kNonSecureCodec。
不会只释放 Codec2 扩展资源，保留 codec component。
```

实际流程是：

```text
根据 ResourceList 中的某项资源选中 clientId
        ↓
调用该 clientId 对应的 IResourceManagerClient::reclaimResource()
        ↓
目标 MediaCodec 执行 release
        ↓
OMX node 或 Codec2 component 被销毁
        ↓
该 clientId 下的整个 ResourceList 随客户端移除而注销
```

因此，“按图形内存选中目标”只表示图形内存资源参与了目标选择；最终释放的是整个 codec 实例及其所有登记资源。

## 6. 代码实现如何对应主流程

本节对应第 2 节的逻辑链，说明每一步在代码中由谁完成。阅读时可以把它理解为“主流程的实现映照”，不是新的策略。

### 6.1 逻辑步骤与代码位置

| 第 2 节逻辑步骤 | 主要代码 | 作用 |
| --- | --- | --- |
| 进入抢占流程 | `MediaCodec::init()`、`configure()`、`start()` | 捕获 `NO_MEMORY`，构造 `resources[]` |
| 调用回收接口 | `ResourceManagerServiceProxy::reclaimResource()` | 调用 `IResourceManagerService::reclaimResource(clientInfo, resources)` |
| 服务端入口 | `ResourceManagerService::reclaimResource()` | 检查请求是否为空，调用目标选择，最后执行回收 |
| secure 冲突裁决 | `DefaultResourceModel::getAllClients()` | 先处理 secure/non-secure 共存约束，冲突时批量收集目标或整体失败 |
| 普通候选收集 | `DefaultResourceModel::getCodecClients()`、`ResourceTracker::getAllClients()` | 按关联资源、主 codec 资源、另一 secure 类型收集候选全集 `clients` |
| pending removal 优先 | `ResourceManagerServiceNew::getClientForResource_l()` | 先查同进程 pending removal，找到则直接生成 `targetClients` |
| 跨进程优先级选择 | `ProcessPriorityReclaimPolicy::getClients()` | 在候选中选 OOM 优先级最低进程，再选该进程内当前资源项 `value` 最大的 client |
| 同进程 importance 选择 | `ClientImportanceReclaimPolicy::getClients()` | 跨进程失败后，在同进程中选 importance 更低的 client |
| 执行回收 | `ResourceManagerService::reclaimUnconditionallyFrom()`、`MediaCodec::reclaim()` | 对 `targetClients` 调用 release，释放整个 codec 实例 |
| 申请方重试 | `MediaCodec::init()`、`configure()`、`start()` 循环 | 回收后重新执行原操作，最多执行 3 次 |

这张表只是索引。下面几节用关键代码把“请求如何进来、候选如何生成、目标如何挑、最后如何释放”串起来。

### 6.2 申请方如何构造 `resources[]`

创建 codec 失败时只传 codec 主资源：

```cpp
resources.push_back(MediaResource::CodecResource(
        secureCodec,
        toMediaResourceSubType(mIsHardware, mDomain)));
```

configure/start 失败时，视频或图片 codec 还会带图形内存资源：

```cpp
resources.push_back(MediaResource::CodecResource(
        mFlags & kFlagIsSecure,
        toMediaResourceSubType(mIsHardware, mDomain)));

resources.push_back(MediaResource::GraphicMemoryResource(1));
```

这里 `resources[0]` 是 codec 主资源，后续资源是与本次 codec 请求相关的资源。`GraphicMemoryResource(1)` 是类型占位，不是实际缺口。

创建阶段的重试循环直接围绕 `reclaimResource()` 展开：

```cpp
// MediaCodec.cpp
std::vector<MediaResourceParcel> resources;
resources.push_back(MediaResource::CodecResource(secureCodec,
                                                 toMediaResourceSubType(mIsHardware, mDomain)));

for (int i = 0; i <= kMaxRetry; ++i) {
    if (i > 0) {
        if (!mResourceManagerProxy->reclaimResource(resources)) {
            break;
        }
    }

    err = PostAndAwaitResponse(msg, &response);
    if (!isResourceError(err)) {
        break;
    }
}
```

configure/start 的写法类似，只是 configure/start 在视频或图片域里会额外塞入 `GraphicMemoryResource(1)`，并且 start 成功后还会把图形内存和系统资源登记到 RM。

真正跨进程调用资源管理服务的是 `ResourceManagerServiceProxy::reclaimResource()`：

```cpp
// MediaCodec.cpp
bool MediaCodec::ResourceManagerServiceProxy::reclaimResource(
        const std::vector<MediaResourceParcel> &resources) {
    std::shared_ptr<IResourceManagerService> service = getService_l();
    if (service == nullptr) {
        return false;
    }
    bool success;
    Status status = service->reclaimResource(getClientInfo(), resources, &success);
    return status.isOk() && success;
}
```

这里传给服务端的是 `getClientInfo()` 和刚构造好的 `resources[]`。`getClientInfo()` 带有 pid、uid、clientId、name、importance 等申请方身份信息；`resources[]` 带有本次失败相关的资源类型。

注意：`mRequiredResourceInfo` 中的 Codec2 扩展系统资源主要用于 start 成功后的资源登记和后续更新，不等同于 create/configure/start 失败时一定会作为回收请求参数传入。当前这几条 `NO_MEMORY` 回收请求路径里，代码明确构造的是 codec 主资源，视频/图片再加图形内存占位资源。

### 6.3 服务端如何从请求变成候选

服务端先把请求包装成 `ReclaimRequestInfo`，再调用 `DefaultResourceModel::getAllClients()`。这一步输出的是候选全集 `clients`，不是最终回收目标。

服务端入口先判断请求是否为空，然后调用 `getTargetClients()`，最后才执行回收：

```cpp
// ResourceManagerService.cpp
Status ResourceManagerService::reclaimResource(const ClientInfoParcel& clientInfo,
        const std::vector<MediaResourceParcel>& resources, bool* _aidl_return) {
    *_aidl_return = false;

    if (resources.empty()) {
        return Status::ok();
    }

    std::vector<ClientInfo> targetClients;
    if (getTargetClients(clientInfo, resources, targetClients)) {
        *_aidl_return = reclaimUnconditionallyFrom(targetClients);
    } else {
        targetClients.clear();
    }

    pushReclaimAtom(clientInfo, targetClients, *_aidl_return);
    return Status::ok();
}
```

因此服务端主入口本身不做复杂选择，它只负责把“找目标”和“执行回收”串起来。新版服务的目标选择入口在 `ResourceManagerServiceNew::getTargetClients()`。

`DefaultResourceModel::getAllClients()` 先处理 secure/non-secure 共存冲突；没有共存冲突时，codec 请求才进入普通候选收集：

```cpp
// DefaultResourceModel.cpp
switch (reclimRequestInfo.mResources[0].type) {
case MediaResource::Type::kSecureCodec:
case MediaResource::Type::kNonSecureCodec:
    // Handling Codec resource reclaim
    return getCodecClients(reclimRequestInfo, clients);
case MediaResource::Type::kGraphicMemory:
case MediaResource::Type::kDrmSession:
    mediaResource.id = reclimRequestInfo.mResources[0].id;
    mediaResource.value = reclimRequestInfo.mResources[0].value;
    return mResourceTracker->getAllClients(resourceRequestInfo, clients);
default:
    break;
}
```

也就是说，本文讨论的视频 codec 回收通常走 `getCodecClients()`，因为 `resources[0]` 是 `kSecureCodec` 或 `kNonSecureCodec`；`kGraphicMemory` 只是 `resources[1]` 之后的关联资源，不是本次请求的主资源。

`getCodecClients()` 的代码直接对应第 2.3 节的三层候选范围：

```cpp
// DefaultResourceModel.cpp
MediaResource::SubType primarySubType = reclimRequestInfo.mResources[0].subType;
for (size_t index = 1; index < reclimRequestInfo.mResources.size(); index++) {
    mediaResource.type = reclimRequestInfo.mResources[index].type;
    mediaResource.subType = reclimRequestInfo.mResources[index].subType;
    mResourceTracker->getAllClients(resourceRequestInfo, clients, primarySubType);
}

mediaResource.type = reclimRequestInfo.mResources[0].type;
mediaResource.subType = reclimRequestInfo.mResources[0].subType;
mResourceTracker->getAllClients(resourceRequestInfo, clients);

MediaResourceType otherType =
    (reclimRequestInfo.mResources[0].type == MediaResource::Type::kSecureCodec) ?
    MediaResource::Type::kNonSecureCodec : MediaResource::Type::kSecureCodec;
mediaResource.type = otherType;
mResourceTracker->getAllClients(resourceRequestInfo, clients);
```

这段代码里有三个关键点：

1. `primarySubType` 的来源就是 `resources[0].subType`，也就是申请方 codec 主资源的子类型。例如硬件视频是 `kHwVideoCodec`，软件视频是 `kSwVideoCodec`。
2. `for (index = 1; ...)` 先遍历关联资源，例如 `kGraphicMemory`。这一层调用 `getAllClients(..., primarySubType)`，所以不是只看谁持有图形内存，还要看目标 client 的主 codec 类别是否与申请方相关。
3. 后两次 `getAllClients()` 没有传 `primarySubType`，分别收集相同 secure 属性、相同 `subType` 的主 codec client，以及另一 secure 属性、相同 `subType` 的主 codec client。

对于 `secure + hardware video codec + graphic memory`，这段代码实际展开为：

```text
第一层：getAllClients(kGraphicMemory, kUnspecifiedSubType, primarySubType = kHwVideoCodec)
第二层：getAllClients(kSecureCodec, kHwVideoCodec)
第三层：getAllClients(kNonSecureCodec, kHwVideoCodec)
```

第一层为什么不要求 secure/non-secure `type` 相同，要看 `ResourceTracker` 的匹配函数：

```cpp
// ResourceTracker.cpp
static bool hasResourceType(MediaResource::Type type, MediaResource::SubType subType,
                            const ResourceList& resources, MediaResource::SubType primarySubType) {
    bool foundResource = false;
    bool matchedPrimary =
        (primarySubType == MediaResource::SubType::kUnspecifiedSubType) ?  true : false;
    for (const MediaResourceParcel& res : resources.getResources()) {
        if (hasResourceType(type, subType, res)) {
            foundResource = true;
        } else if (res.subType == primarySubType) {
            matchedPrimary = true;
        } else if (isHwCodec(res.subType) == isHwCodec(primarySubType)) {
            matchedPrimary = true;
        }
        if (matchedPrimary && foundResource) {
            return true;
        }
    }
    return false;
}
```

这段代码要求同一个 client 的 `ResourceList` 同时满足两个条件：

1. `foundResource = true`：持有当前正在查的资源，例如 `kGraphicMemory`。
2. `matchedPrimary = true`：持有与 `primarySubType` 相同或同硬件类别的 codec 子类型。

其中 `hasResourceType(type, subType, res)` 对非 codec 资源主要比较 `type`，所以第一层查 `kGraphicMemory` 时，不会同时要求这个 client 还持有 `kNonSecureCodec`。secure/non-secure 的严格区分发生在第二层和第三层的 codec 主资源匹配中。

还要注意，`clients` 是三层范围汇总并去重后的候选全集。它不是最终目标，也不表示每个 client 同时满足三层条件；后面的回收策略会再按同样的资源尝试顺序从 `clients` 中选择 `targetClients`。

### 6.4 服务端如何从候选变成目标

候选全集 `clients` 准备好后，`getClientForResource_l()` 按第 2.4 节的顺序选择 `targetClients`：

```cpp
// ResourceManagerServiceNew.cpp
for (const MediaResourceParcel& resource : reclaimRequestInfo.mResources) {
    if (mResourceTracker->getBiggestClientPendingRemoval(callingPid, resource.type,
                                                         resource.subType, targetClient)) {
        targetClients.emplace_back(targetClient);
        return;
    }
}

for (std::unique_ptr<IReclaimPolicy>& reclaimPolicy : mReclaimPolicies) {
    if (reclaimPolicy->getClients(reclaimRequestInfo, clients, targetClients)) {
        return;
    }
}
```

第一段先查同进程 pending removal，并且按 `resources[]` 原顺序查。第二段按 `setReclaimPolicy()` 注册顺序执行策略：先 `ProcessPriorityReclaimPolicy`，再 `ClientImportanceReclaimPolicy`。

跨进程 OOM 优先级策略的三层资源尝试顺序在 `ProcessPriorityReclaimPolicy::getClients()` 中：

```cpp
// ProcessPriorityReclaimPolicy.cpp
MediaResource::Type type = reclaimRequestInfo.mResources[0].type;
MediaResource::SubType subType = reclaimRequestInfo.mResources[0].subType;
MediaResource::SubType primarySubType = subType;

for (size_t index = 1; !found && (index < reclaimRequestInfo.mResources.size()); index++) {
    MediaResource::Type type = reclaimRequestInfo.mResources[index].type;
    MediaResource::SubType subType = reclaimRequestInfo.mResources[index].subType;
    found = getBiggestClientFromLowestPriority(callingPid, callingPriority,
                                               type, subType, primarySubType,
                                               clients, clientInfo, lowestPriority);
}

if (!found) {
    found = getBiggestClientFromLowestPriority(callingPid, callingPriority,
                                               type, subType,
                                               MediaResource::SubType::kUnspecifiedSubType,
                                               clients, clientInfo, lowestPriority);
}

if (!found) {
    MediaResourceType otherType = (type == MediaResource::Type::kSecureCodec) ?
        MediaResource::Type::kNonSecureCodec : MediaResource::Type::kSecureCodec;
    if (!getBiggestClientFromLowestPriority(callingPid, callingPriority,
                                            otherType, subType,
                                            MediaResource::SubType::kUnspecifiedSubType,
                                            clients, clientInfo, lowestPriority)) {
        return false;
    }
}
```

这段代码说明资源层级优先于进程优先级的全局比较。也就是说，不是先从所有候选中找最低优先级进程，再看它属于哪层资源；而是先尝试关联资源层，关联资源层找不到可抢占目标时，才尝试相同 secure 属性的主 codec 层，再尝试另一 secure 属性的主 codec 层。

每一层内部的选择逻辑在 `getBiggestClientFromLowestPriority()`：

```cpp
// ProcessPriorityReclaimPolicy.cpp
if (!mResourceTracker->getLowestPriorityPid(type, subType, primarySubType, clients,
                                            lowestPriorityPid, lowestPriority)) {
    return false;
}

if (lowestPriority <= callingPriority) {
    return false;
}

return mResourceTracker->getBiggestClient(lowestPriorityPid, type, subType,
                                          clients, targetClient, primarySubType);
```

它对应第 2.4 节的跨进程规则：

```text
当前资源层
    ↓ 从 clients 中筛出持有当前资源项、且 primarySubType 符合的 client
选这些 client 所属进程中 OOM 优先级最低的进程
    ↓ 如果该进程没有严格低于申请进程，则本层失败
在该进程内选当前资源项 value 最大的 client
    ↓ 找到后立即停止，不再尝试后面的资源层
```

同进程 importance 策略复用相同的资源层级顺序，只是约束换成“同一进程，且目标 client 的 importance 数值严格大于申请者”：

```cpp
// ClientImportanceReclaimPolicy.cpp
MediaResource::SubType primarySubType = subType;
for (size_t index = 1; !found && (index < reclaimRequestInfo.mResources.size()); index++) {
    MediaResource::Type type = reclaimRequestInfo.mResources[index].type;
    MediaResource::SubType subType = reclaimRequestInfo.mResources[index].subType;
    found = mResourceTracker->getLeastImportantBiggestClient(
        callingPid, callingImportance,
        type, subType, primarySubType,
        clients, targetClient);
}

if (!found) {
    found = mResourceTracker->getLeastImportantBiggestClient(
        callingPid, callingImportance,
        type, subType, MediaResource::SubType::kUnspecifiedSubType,
        clients, targetClient);
}
```

因此 6.3 和 6.4 合起来看，完整普通选择链是：候选收集先把三层可能相关的 client 汇总到 `clients`；真正选目标时再从第一层开始逐层尝试。每一层内部才比较进程 OOM 优先级或同进程 importance，最后只在已选定进程或同进程范围内按当前资源项 `value` 选最大 client。

### 6.5 候选、目标和释放单位

服务端有两个关键列表：

```text
clients       = 候选全集，只说明可能相关
targetClients = 最终回收目标
```

`clients` 由资源匹配产生，`targetClients` 由优先级策略产生。只有 `targetClients` 会进入回收执行。

`ResourceManagerServiceNew::getTargetClients()` 先用 `getAllClients()` 得到候选，再进入 `getClientForResource_l()` 选目标：

```cpp
// ResourceManagerServiceNew.cpp
ReclaimRequestInfo reclaimRequestInfo{callingPid, clientInfo.id, callingImportance, resources};
std::vector<ClientInfo> clients;
if (!mDefaultResourceModel->getAllClients(reclaimRequestInfo, clients)) {
    if (clients.empty()) {
        return false;
    }
    targetClients = std::move(clients);
} else {
    getClientForResource_l(reclaimRequestInfo, clients, targetClients);
}
```

如果 `getAllClients()` 返回 `false` 但 `clients` 非空，表示这是 secure 冲突场景，此时不再走普通优先级策略，而是把整批冲突 client 作为回收目标。

普通场景下，最终执行时调用：

```cpp
reclaimUnconditionallyFrom(targetClients)
```

对应实现是逐个 client 调 `IResourceManagerClient::reclaimResource()`：

```cpp
// ResourceManagerService.cpp
for (const ClientInfo& targetClient : targetClients) {
    std::shared_ptr<IResourceManagerClient> client = nullptr;
    {
        std::scoped_lock lock{mLock};
        client = getClient_l(targetClient.mPid, targetClient.mClientId);
    }
    if (client == nullptr) {
        continue;
    }
    bool success;
    Status status = client->reclaimResource(&success);
    if (!status.isOk() || !success) {
        break;
    }
}
```

目标端进入 `MediaCodec::reclaim()`，最终走 `kWhatRelease`，释放整个 codec 实例，而不是只释放某个资源项。

```cpp
// MediaCodec.cpp
status_t MediaCodec::reclaim(bool force) {
    sp<AMessage> msg = new AMessage(kWhatRelease, this);
    msg->setInt32("reclaimed", 1);
    msg->setInt32("force", force ? 1 : 0);

    sp<AMessage> response;
    status_t ret = PostAndAwaitResponse(msg, &response);
    if (ret == -ENOENT) {
        ret = OK;
    }
    return ret;
}
```

因此，`ResourceList` 只用于选择目标；一旦选中 `clientId`，释放的是该 client 对应的整个 MediaCodec，以及它下面登记的全部资源。

### 6.6 资源登记和运行中配置变化

start 成功后，`MediaCodec` 会把实际运行期可估算的资源登记到资源管理器。视频/图片 codec 的图形内存登记发生在 `kWhatStartCompleted`：

```cpp
// MediaCodec.cpp
std::vector<MediaResourceParcel> resources;
if (mDomain == DOMAIN_VIDEO || mDomain == DOMAIN_IMAGE) {
    resources.push_back(
            MediaResource::GraphicMemoryResource(getGraphicBufferSize()));
}
if (android::media::codec::codec_availability() &&
    android::media::codec::codec_availability_support()) {
    for (const InstanceResourceInfo& resource : *resourcesLocked) {
        resources.push_back(getMediaResourceParcel(resource));
    }
}
if (!resources.empty()) {
    mResourceManagerProxy->addResource(resources);
}
```

这里的 `getGraphicBufferSize()` 是估算值，不是 allocator 返回的真实 GraphicBuffer 总大小：

```cpp
// MediaCodec.cpp
uint64_t MediaCodec::getGraphicBufferSize() {
    if (mDomain != DOMAIN_VIDEO && mDomain != DOMAIN_IMAGE) {
        return 0;
    }

    uint64_t size = 0;
    size_t portNum = sizeof(mPortBuffers) / sizeof((mPortBuffers)[0]);
    for (size_t i = 0; i < portNum; ++i) {
        size += mPortBuffers[i].size() * mWidth * mHeight * 3 / 2;
    }
    return size;
}
```

Codec2 availability 打开时，还会把 component store 声明的系统资源转换成 `MediaResourceParcel`，并在后续变化时调用 `updateResourceUsage()`：

```cpp
// MediaCodec.cpp，摘录省略版
bool MediaCodec::getRequiredSystemResources() {
    oldResources = *resourcesLocked;
    newResources = mCodec->getRequiredSystemResources();
    newResources = computeDynamicResources(newResources);
    *resourcesLocked = newResources;
    if (!oldResources.empty()) {
        updateResourceUsage(oldResources, newResources);
    }
}
```

运行中分辨率变化时，代码会更新 `mWidth/mHeight`，并通过 `notifyClientConfigChanged()` 通知 RM 侧更新 client 配置和 importance：

```cpp
// MediaCodec.cpp
if (resolutionChanged) {
    ClientConfigParcel clientConfig;
    initClientConfigParcel(clientConfig);
    mResourceManagerProxy->notifyClientConfigChanged(clientConfig);
}
```

新版资源管理器收到配置变化通知后，主要更新 `ResourceTracker` 中记录的 client importance，然后把配置变化交给 metrics：

```cpp
// ResourceManagerServiceNew.cpp
Status ResourceManagerServiceNew::notifyClientConfigChanged(
        const ClientConfigParcel& clientConfig) {
    {
        std::scoped_lock lock{mLock};
        mResourceTracker->updateClientImportance(clientConfig.clientInfo);
    }
    return ResourceManagerService::notifyClientConfigChanged(clientConfig);
}
```

因此运行中配置变化路径应分开看：

```text
start 成功
    ↓
登记 GraphicMemoryResource(getGraphicBufferSize())
    ↓
登记 Codec2 required system resources（如果功能打开）

output format changed / 分辨率变化
    ↓
更新 mWidth / mHeight
    ↓
notifyClientConfigChanged(clientConfig)
    ↓
RM 更新 client 配置/importance 和 metrics

output format changed / 帧率变化
    ↓
getRequiredSystemResources()
    ↓
必要时通过 updateResourceUsage() 更新 Codec2 系统资源登记
```

这里不能简单说“变分辨率一定会更新图形内存登记”。当前代码明确在 start 成功时登记 `GraphicMemoryResource(getGraphicBufferSize())`；运行中分辨率变化会更新 `mWidth/mHeight` 并通知配置变化，但图形内存登记是否随分辨率变化同步更新，要看是否另有资源更新路径触发。Codec2 系统资源则在帧率变化并重新计算 required resources 时，可能通过 `updateResourceUsage()` 更新登记值。

这条路径仍然是**资源登记或配置更新**，不是 create/configure/start 的 `NO_MEMORY` 回收链。当前实现没有在运行中重分配失败时自动发起同样的抢占重试。

如果变分辨率过程中底层重新分配 output buffer、图形内存或 Codec2 buffer 失败，当前 `MediaCodec` 代码没有像 create/configure/start 那样主动调用 `ResourceManagerService::reclaimResource()` 再自动重试。资源回收触发点集中在创建、configure、start 返回 `NO_MEMORY` 的路径；运行中重分配失败通常由底层 codec 报错，向应用走错误回调或 fatal error 流程。

## 7. 资源登记补充说明

资源登记是资源管理器的选择依据，不是实际硬件和内存对象。核心数据为：

```text
PID
└── clientId
    ├── uid、codec 名称、importance、pendingRemoval
    └── ResourceList
        └── MediaResourceParcel { type, subType, id, value }
```

视频 codec 的典型登记包括：

- component 分配成功后登记 secure/non-secure codec 和硬件/软件视频子类型，`value` 通常为 1。
- start 完成后登记估算图形内存。
- 支持 Codec2 availability 管控时，登记 component 声明的额外系统资源。
- release 完成、客户端移除或进程死亡后删除记录。

`pending removal` 表示客户端已进入异步释放流程，但底层 component 尚未释放完、资源记录也尚未删除。它不是正常运行态，也不代表资源已经释放，因此同进程后续申请会优先清理它。

## 8. Secure 冲突为什么需要单独处理

secure codec 可能使用受保护内存、安全视频路径、TEE/固件安全上下文及厂商专用硬件通道。设备通过资源管理策略声明：

- 是否允许多个 secure codec 同时存在。
- 是否允许 secure 和 non-secure codec 同时存在。

框架不推导具体硬件原因，只执行设备声明的共存约束。之所以必须批量处理，是因为只释放部分冲突实例后，剩余实例仍会违反共存约束；所以它采用“全部冲突实例都可抢占才全部回收，否则整体失败”的事务式判断。

## 9. 资源不足的真实含义

`NO_MEMORY` 是 MediaCodec 用于触发回收的统一资源错误，不等于只发生了物理内存不足。可能来源包括：

- 硬件 codec 实例数达到上限。
- secure 会话或安全硬件上下文达到上限。
- 图形 buffer、线性 buffer、共享内存或厂商内存池不足。
- 厂商硬件通道、固件上下文等内部资源不足。
- 其他被 OMX、Codec2 或厂商 HAL 映射为 `NO_MEMORY` 的申请失败。

这些来源并不都能在资源登记中找到一一对应的标准类型。资源管理器能看到的是 `MediaResourceParcel { type, subType, id, value }`，其中 `type` 的内建范围只有 secure codec、non-secure codec、graphic memory、CPU boost、battery、DRM session，以及 Codec2 component store 预留的软/硬件扩展资源类型。

| 可能的 `NO_MEMORY` 来源 | 登记中是否有对应资源类型 | 说明 |
| --- | --- | --- |
| 硬件 codec 实例数达到上限 | 有 | 登记为 `kSecureCodec` 或 `kNonSecureCodec`，`subType` 为 `kHwVideoCodec`、`kHwAudioCodec` 等，`value` 通常为实例数 1 |
| secure codec 实例数达到上限 | 有 | 登记为 `kSecureCodec`，再通过 `subType` 区分音频、视频、图片和硬件/软件 |
| secure/non-secure 共存限制 | 有，但不是独立资源类型 | 通过 `kSecureCodec`、`kNonSecureCodec` 加设备策略判断冲突，不存在一个单独的 `secure session` 标准资源类型 |
| DRM session 不足 | 有 | 登记为 `kDrmSession`，`id` 可携带 session 标识 |
| 图形 buffer 或图形内存不足 | 有 | 登记为 `kGraphicMemory`，`value` 表示估算图形内存字节数；configure/start 发起回收请求时使用占位值 1 做候选筛选 |
| 线性 buffer、共享内存、普通 heap 不足 | 通常没有标准独立类型 | 如果最终表现为 `NO_MEMORY`，框架仍可能按 codec 和图形内存请求回收；但登记项不能精确表达“线性内存缺口” |
| Codec2 component store 声明的系统资源不足 | 可以有 | 通过 `kSwResourceTypeMin..kSwResourceTypeMax` 或 `kHwResourceTypeMin..kHwResourceTypeMax` 预留范围表达，具体资源 ID 由 component store 报告 |
| 厂商硬件通道、固件上下文、内部池不足 | 不一定有 | 只有厂商通过 Codec2 system resources 暴露时才有扩展资源类型；否则框架只能看到底层返回 `NO_MEMORY` |
| 其他被 HAL 映射为 `NO_MEMORY` 的失败 | 不一定有 | 取决于底层是否把该资源登记或声明给框架 |

因此，`NO_MEMORY` 的真实原因集合大于资源管理器可见的登记类型集合。资源管理器通常不能仅根据 `NO_MEMORY` 判断真实耗尽项，只能根据申请方声明的资源类型选择可能有效的释放目标。

当前机制也不会因为 codec 服务进程超过某个 RSS 或历史内存峰值而主动回收 codec。峰值并发像素数用于指标统计，不是抢占阈值。系统内存压力只有最终表现为 `NO_MEMORY` 时才进入普通回收链；若服务进程直接死亡，则进入 `DEAD_OBJECT` 链路。

## 10. 应用侧感知

### 10.1 申请方

创建、configure 或 start 内部回收和重试成功时，对应用总体透明：应用只看到原接口最终成功，但可能观察到调用耗时增加。重试失败时，应用收到最终错误，再决定重新创建 codec、降低规格或切换软件 codec。

### 10.2 被抢占方

| 模式 | 应用侧表现 |
| --- | --- |
| 同步模式 | 后续等待请求和部分 buffer 操作返回 `DEAD_OBJECT` |
| Java 异步模式 | 收到 fatal `MediaCodec.CodecException`，错误语义为 `ERROR_RECLAIMED` |
| NDK 异步模式 | 通过 `AMediaCodecOnAsyncNotifyCallback::onAsyncError` 回调上报；native 层 `DEAD_OBJECT` 会被 NDK wrapper 映射为 `AMEDIACODEC_ERROR_RECLAIMED` |

fatal 回调后该 codec 不可继续处理数据。后续再调用其他接口没有统一错误码：部分接口会返回 reclaimed/dead object，NDK 层通常映射为 `AMEDIACODEC_ERROR_RECLAIMED`；部分接口会先命中状态检查，返回 `INVALID_OPERATION` 或对应 NDK 错误；`release` 则允许完成。应用应释放旧对象并重新创建 codec。

## 11. 正常释放与异常清理

| 场景 | 行为 |
| --- | --- |
| `stop()` | 停止处理数据并归还部分动态资源，但通常保留底层 codec component |
| `release()` | 销毁 OMX node 或 Codec2 component，并注销客户端资源 |
| 异步 release | 先标记 pending removal，底层释放完成后删除记录 |
| 客户端进程死亡 | Binder death 删除资源登记；底层资源由进程退出和服务端清理 |
| OMX/Codec2 服务死亡 | 当前 codec 收到 `DEAD_OBJECT`，不属于按优先级抢占 |

`stop()` 不能视为完整释放 codec 实例；`release()` 才是完整释放。

## 12. 回收时应用侧 buffer 的内存生命周期

### 12.1 应用持有的是裸指针，不持有引用计数

应用通过 NDK `AMediaCodec_getInputBuffer` / Java `MediaCodec.getInputBuffer` 拿到的是裸指针（`uint8_t*` 或 `ByteBuffer`），不是 `sp<MediaCodecBuffer>`。函数返回后栈上的 `sp<>` 立即析构，应用本身不持有任何引用计数。

裸指针能否继续访问，完全取决于底层 mmap 是否存活。mmap 由 dmabuf fd 维持，fd 的生命周期由 `MediaCodecBuffer` 内部的 `shared_ptr<C2LinearBlock>` → `C2LinearAllocation` 链持有。只要 `MediaCodecBuffer` 不析构，mmap 就在。

维持 `MediaCodecBuffer` 存活的 `sp<>` 引用主要有两处：

| 引用持有者 | 说明 |
| --- | --- |
| `mPortBuffers[port][idx].mData` | MediaCodec 自身持有的主引用 |
| `mImpl.mBuffers[i].clientBuffer` | CCodecBuffers 内部持有的引用 |

input LinearBlock 由应用进程内的 `C2BlockPool` + 本地 `C2Allocator` 分配，mmap 在应用进程内执行。mediaserver / codec2 服务进程持有同一 fd 的另一份独立 mmap，两进程的映射互不影响——服务进程释放 component 只撤销服务进程的映射，不撤销应用进程的映射。

### 12.2 完整回收流程

本节从入口到 buffer 释放，逐步追踪代码。整个流程的核心是：**先通知应用、等 0.5s、再强制释放**。

#### 入口：ResourceManagerClient::reclaimResource

资源管理器选好目标后通过 binder 调用目标 MediaCodec 的 `reclaimResource()`。"先试非强制、等 0.5s、再强制"的逻辑在客户端进程的 `ResourceManagerClient` 中完成：

```cpp
// MediaCodec.cpp:327
static const int kMaxReclaimWaitTimeInUs = 500000;  // 0.5s

// MediaCodec.cpp:343
Status reclaimResource(bool* _aidl_return) override {
    sp<MediaCodec> codec = mMediaCodec.promote();
    if (codec == NULL) { /* codec 已销毁，删记录并返回 */ }

    status_t err = codec->reclaim();                    // ① 第一次，force=false
    if (err == WOULD_BLOCK) {
        usleep(kMaxReclaimWaitTimeInUs);                 // ② 等待 0.5s
        err = codec->reclaim(true /* force */);          // ③ 第二次，force=true
    }
    *_aidl_return = (err == OK);
    return Status::ok();
}
```

`MediaCodec::reclaim(bool force)` 投递 `kWhatRelease` 消息并同步等待回复：

```cpp
// MediaCodec.cpp:3488
status_t MediaCodec::reclaim(bool force) {
    sp<AMessage> msg = new AMessage(kWhatRelease, this);
    msg->setInt32("reclaimed", 1);
    msg->setInt32("force", force ? 1 : 0);
    sp<AMessage> response;
    return PostAndAwaitResponse(msg, &response);
}
```

#### hasPendingBuffer：判断应用是否还有未归还的 buffer

```cpp
// MediaCodec.cpp:3478
bool MediaCodec::hasPendingBuffer(int portIndex) {
    return std::any_of(
            mPortBuffers[portIndex].begin(), mPortBuffers[portIndex].end(),
            [](const BufferInfo &info) { return info.mOwnedByClient; });
}

bool MediaCodec::hasPendingBuffer() {
    return hasPendingBuffer(kPortIndexInput) || hasPendingBuffer(kPortIndexOutput);
}
```

`mOwnedByClient` 表示 buffer slot 当前被应用持有（dequeue 取走但还没归还）。只要有任一个 `mOwnedByClient=true`，`hasPendingBuffer()` 返回 `true`。

#### 第一次 reclaim（force=false）：通知应用，不释放

```cpp
// MediaCodec.cpp:5659  kWhatRelease handler
int32_t reclaimed = 0;
msg->findInt32("reclaimed", &reclaimed);
if (reclaimed) {
    if (!mReleasedByResourceManager) {                   // 首次进入
        if (mFlags & kFlagIsAsync) {
            onError(DEAD_OBJECT, ACTION_CODE_FATAL);    // ④ 投递 ERROR_RECLAIMED
        }
        mReleasedByResourceManager = true;
    }

    int32_t force = 0;
    msg->findInt32("force", &force);
    if (!force && hasPendingBuffer()) {                  // ⑤ force=false 且有未归还 buffer
        response->setInt32("err", WOULD_BLOCK);          // ⑥ 返回 WOULD_BLOCK
        response->postReply(replyID);
        break;                                            // 退出，不执行后续释放
    }
}
// —— 以下在 force=false 且 hasPendingBuffer 时不执行 ——
setState(RELEASING);
mCodec->initiateShutdown(false);
returnBuffersToCodec(reclaimed);
```

`onError` 异步投递消息到 looper：

```cpp
// MediaCodec.cpp:7406
void MediaCodec::onError(status_t err, int32_t actionCode, const char *detail) {
    if (mCallback != NULL) {
        sp<AMessage> msg = mCallback->dup();
        msg->setInt32("callbackID", CB_ERROR);
        msg->setInt32("err", err);
        msg->post();                                      // 异步投递
    }
}
```

第一次 reclaim 结束时：
- `ERROR_RECLAIMED` 已异步投递给应用
- **没有**调用 `returnBuffersToCodec`、`initiateShutdown`
- buffer 完全有效
- 返回 `WOULD_BLOCK`

#### 0.5s 等待窗口

`ResourceManagerClient` 收到 `WOULD_BLOCK` 后执行 `usleep(500000)`。这 0.5s 内应用收到 `ERROR_RECLAIMED` 回调，应停止访问 buffer。此时 buffer 仍然有效，codec 尚未开始释放。

#### 第二次 reclaim（force=true）：实际释放

第二次进入 `kWhatRelease` handler，`mReleasedByResourceManager` 已为 `true`（跳过 `onError`），`force=true`（跳过 `WOULD_BLOCK` 检查），继续执行：

```cpp
// MediaCodec.cpp:5796
setState(RELEASING);                                      // ⑫
mCodec->initiateShutdown(false);                          // ⑬ 异步通知 HAL 释放
returnBuffersToCodec(reclaimed);                          // ⑭ 第 1 遍，isReclaim=true
```

#### returnBuffersToCodec 的核心判断

```cpp
// MediaCodec.cpp:6592-6618
void MediaCodec::returnBuffersToCodecOnPort(int32_t portIndex, bool isReclaim) {
    for (size_t i = 0; i < mPortBuffers[portIndex].size(); ++i) {
        BufferInfo *info = &mPortBuffers[portIndex][i];
        if (info->mData != nullptr) {
            sp<MediaCodecBuffer> buffer = info->mData;   // 临时拷贝，refcount+1
            if (isReclaim && info->mOwnedByClient) {
                // —— 应用还持有的 buffer ——
                // 故意不清 info->mData —— 保留引用
            } else {
                // —— 应用不持有的 buffer ——
                info->mOwnedByClient = false;
                info->mData.clear();                     // 清除引用，refcount-1
            }
            mBufferChannel->discardBuffer(buffer);         // 两种情况都调用
        }
    }
    mAvailPortBuffers[portIndex].clear();
}
```

**只有 `isReclaim==true` 且 `mOwnedByClient==true` 时才保留**；否则清除。但无论保留还是清除，都会调用 `discardBuffer`。

#### discardBuffer：通知 codec 收回 buffer

`discardBuffer` 是 `ACodecBufferChannel`（或 `CCodecBufferChannel`）上的方法，作用是**异步通知 codec 收回某个 buffer**：

```cpp
// ACodecBufferChannel.cpp:497-517
status_t ACodecBufferChannel::discardBuffer(const sp<MediaCodecBuffer> &buffer) {
    // 1. 在 buffer channel 自己的数组里找这个 buffer
    auto array = std::atomic_load(&mInputBuffers);  // 或 mOutputBuffers
    BufferInfoIterator it = findClientBuffer(array, buffer);
    if (it == array->end()) { return -ENOENT; }

    // 2. 构造消息：input 用 mInputBufferFilled，output 用 mOutputBufferDrained
    sp<AMessage> msg = input ? mInputBufferFilled->dup() : mOutputBufferDrained->dup();

    // 3. 携带 codec 侧的 buffer 引用、buffer-id、discarded 标志
    msg->setObject("buffer", it->mCodecBuffer);   // codec 侧的 buffer 引用
    msg->setInt32("buffer-id", it->mBufferId);
    msg->setInt32("discarded", true);              // 标记为"丢弃"模式

    // 4. 异步投递给 ACodec
    msg->post();   // 异步！不阻塞
    return OK;
}
```

关键点：
- 消息里携带的是 **codec 侧的 buffer 引用**（`it->mCodecBuffer`），不是 MediaCodec 侧的 `info->mData`。
- `msg->post()` 是异步投递，不阻塞。ACodec 在自己的 looper 上处理这条消息。
- 对于 Codec2 架构（`CCodecBufferChannel`），`discardBuffer` 的逻辑类似——通知 codec 侧收回 buffer。

#### ACodec 处理 discardBuffer：KEEP_BUFFERS 模式

ACodec 收到消息后进入 `onInputBufferFilled`（input 侧）或 `onOutputBufferDrained`（output 侧）：

```cpp
// ACodec.cpp:6285-6337  BaseState::onInputBufferFilled
int32_t discarded = 0;
if (msg->findInt32("discarded", &discarded) && discarded) {
    mode = KEEP_BUFFERS;   // 丢弃模式：只收下，不重新提交
}
info->mStatus = BufferInfo::OWNED_BY_US;  // codec 收回所有权
info->mData = buffer;

switch (mode) {
    case KEEP_BUFFERS:
        break;  // 什么都不做：
               //   - 不调用 postFillThisBuffer
               //   - 不把 buffer 发回给 MediaCodec
               //   - 只静静收下
    case RESUBMIT_BUFFERS:
        postFillThisBuffer(info);  // 正常模式才会发回 MediaCodec
        break;
}
```

Output 侧（`onOutputBufferDrained`，`ACodec.cpp:6802-6968`）逻辑相同：`discarded=true` 时不渲染，走 `KEEP_BUFFERS` 模式收下。

**`discarded=true` → `KEEP_BUFFERS` 模式**：ACodec 把 buffer 收回（`OWNED_BY_US`），但**不重新提交给 OMX 组件，也不发回 MediaCodec**。这一步是 codec 侧收回 buffer 所有权的核心机制。

#### freeBuffersOnPort：清空 buffer channel 引用

当 ACodec 收回所有 buffer（`allYourBuffersAreBelongToUs()` 为 true）后，进入 `ExecutingToIdleState::changeStateIfWeOwnAllBuffers()`：

```cpp
// ACodec.cpp:8916-8922
if (mComponentNowIdle && mCodec->allYourBuffersAreBelongToUs()) {
    mCodec->mOMXNode->sendCommand(OMX_CommandStateSet, OMX_StateLoaded);
    mCodec->freeBuffersOnPort(kPortIndexInput);
    mCodec->freeBuffersOnPort(kPortIndexOutput);
}
```

```cpp
// ACodec.cpp:1656-1674
status_t ACodec::freeBuffersOnPort(OMX_U32 portIndex) {
    // ★ 关键：清空 buffer channel 的数组
    if (portIndex == kPortIndexInput)
        mBufferChannel->setInputBufferArray({});   // 清空 mInputBuffers
    else
        mBufferChannel->setOutputBufferArray({});  // 清空 mOutputBuffers

    for (size_t i = mBuffers[portIndex].size(); i > 0;) {
        i--;
        freeBuffer(portIndex, i);  // 调用 OMX freeBuffer 释放底层资源
    }
}
```

`setInputBufferArray({})` / `setOutputBufferArray({})` 替换 buffer channel 的整个数组，旧数组中每个 `BufferInfo` 的 `mClientBuffer`（`sp<MediaCodecBuffer>`）被释放。此时如果 MediaCodec 侧的 `info->mData` 也已 clear，`MediaCodecBuffer` 引用计数归零 → 内存释放 → 应用裸指针悬空。**正因为 `info->mData` 未 clear，buffer 对象仍然活着。**

对于 Codec2 架构，`CCodecBuffers` 也有类似的引用持有和清理路径。

#### 第 1 遍（isReclaim=true）：保留应用持有的 buffer

在 `kWhatRelease` 中调用，`isReclaim=true`。对 `mOwnedByClient=true` 的 buffer：跳过 `mData.clear()`，`mPortBuffers` 引用仍为 1，buffer 存活。此时 HAL 正在异步释放，保留引用避免应用正在写时 crash。

#### 第 2 遍（isReclaim=false）：释放所有 buffer

HAL 释放完成后投递 `kWhatReleaseCompleted`：

```cpp
// MediaCodec.cpp:5102
case kWhatReleaseCompleted:
{
    setState(UNINITIALIZED);    // ← 触发第 2 遍
    // ...
}
```

```cpp
// MediaCodec.cpp:6566
if (newState == UNINITIALIZED) {
    // return any straggling buffers, e.g. if we got here on an error
    returnBuffersToCodec();       // 无参数，默认 isReclaim=false
}
```

这次 `isReclaim=false`，对 `mOwnedByClient=true` 的 buffer 也走 else 分支，执行 `info->mData.clear()`。refcount 归零 → `LinearBlockBuffer` 析构 → `shared_ptr<C2LinearBlock>` 释放 → `C2LinearAllocation` 析构 → dmabuf fd `close` → **mmap `munmap`** → 应用裸指针变为悬垂指针。

`setState(UNINITIALIZED)` 的这段是无条件保护逻辑：不论如何进入 UNINITIALIZED 都清理残留 buffer，不区分回收原因。

#### 时序总结

```text
T0  应用 dequeue + getInputBuffer，开始写 buffer
    ↓
T1  第一次 reclaim（force=false）
    → onError 投递 ERROR_RECLAIMED（位于 hasPendingBuffer 检查之前）
    → hasPendingBuffer()=true → 返回 WOULD_BLOCK
    → 不执行 returnBuffersToCodec / initiateShutdown
    → buffer 完全有效
    ↓
T2  应用收到 ERROR_RECLAIMED 回调 → 应停止访问 buffer
    ↓
T3  usleep(500000) 等待 0.5s（buffer 仍有效）
    ↓
T4  第二次 reclaim（force=true）
    → 跳过 onError（已投递）、跳过 WOULD_BLOCK 检查
    → setState(RELEASING) + initiateShutdown + returnBuffersToCodec(true)
    → 第 1 遍：isReclaim=true → 保留 mOwnedByClient 的 buffer
    ↓
T5  HAL 释放完成 → kWhatReleaseCompleted
    → setState(UNINITIALIZED) → returnBuffersToCodec(false)
    → 第 2 遍：isReclaim=false → 清除所有 mData → munmap
    → 应用裸指针变为悬垂指针
    ↓
T6  应用若仍写 buffer → crash
```

### 12.3 设计含义

整个回收流程有两层安全机制：

1. **第一层：提前通知**。`ERROR_RECLAIMED` 在第一次 reclaim（force=false）的最开头投递，此时 buffer 完全有效。资源管理器收到 `WOULD_BLOCK` 后等待 0.5s，给应用响应回调并停止访问的时间。

2. **第二层：force reclaim 的两遍 `returnBuffersToCodec`**。第一遍（`isReclaim=true`）在 HAL 释放进行中保留应用持有的 buffer；第二遍（`isReclaim=false`）在 `kWhatReleaseCompleted` 中无条件清除。

结果是：**应用在收到 `ERROR_RECLAIMED` 时 buffer 仍然有效，拥有至少 0.5s 的窗口停止访问**。在 force reclaim 的 `kWhatReleaseCompleted` 处理完毕之前停止访问，就不会 crash。

reclaim 后调 `release()` API 走空分支（`mState==UNINITIALIZED && mReleasedByResourceManager` 时直接回复 OK），不会再清理 buffer。对象析构时 `mPortBuffers` 中的 `mData` 已被清零，只是销毁空数组。

#### 为什么第一遍不释放：保守设计选择

`isReclaim` 的本质是**把释放推迟到 codec 确认完全关闭后**，而非 shutdown 过程中：

| 释放时机 | 第 1 遍就 clear | 第 2 遍 clear（实际代码） |
| --- | --- | --- |
| 内存何时释放 | `freeBuffersOnPort` 清空 buffer channel 引用时（shutdown 过程中） | `setState(UNINITIALIZED)` 后（codec 完全关闭后） |
| 应用裸指针 | shutdown 过程中悬空 | codec 关闭后悬空 |
| codec 是否仍可能访问内存 | 是（shutdown 进行中） | 否 |

两遍 clear 对应用裸指针的 use-after-free 风险**本质相同**——都是把最后的引用释放掉，应用指针就悬空。`isReclaim` 不能消除风险，只是把风险窗口从"异步 shutdown 过程中"推迟到"shutdown 确认完成后"，缩到最小。

由于 `initiateShutdown` 是异步的，shutdown 过程中 `freeBuffersOnPort` 何时清空 buffer channel 引用不可预测。如果第 1 遍就 clear 了 `info->mData`，到 `freeBuffersOnPort` 时引用计数可能立即归零——此刻 codec 可能还在处理最后一帧。保留 `info->mData` 到第 2 遍，保证了整个 shutdown 过程中 buffer 对象始终存活。

### 12.4 discardBuffer 与 ERROR_RECLAIMED 的关系

两者是 reclaim 流程中**面向不同对象的通知**，没有直接交互：

| | 面向 | 时机 | 作用 |
| --- | --- | --- | --- |
| `ERROR_RECLAIMED`/`DEAD_OBJECT` | 应用 | 第一次 `reclaim(false)`，`MediaCodec.cpp:5665` | 告诉应用"你的 codec 被回收了" |
| `discardBuffer` | codec | 第二次 `reclaim(true)`，`returnBuffersToCodec` 内，`MediaCodec.cpp:5801` | 告诉 codec"把这些 buffer 收回去" |

- `ERROR_RECLAIMED` 是应用侧的错误码（NDK 层将 `DEAD_OBJECT` 翻译为 `AMEDIACODEC_ERROR_RECLAIMED = 1101`），应用收到后应停止使用该 codec 并调用 `release()`。
- `discardBuffer` 是 codec 侧的 buffer 回收机制，通过异步消息让 ACodec/Codec2 收回应用未归还的 buffer。两者发生在不同阶段、面向不同对象。

### 12.5 与鸿蒙 C 接口对比

| 对比项 | Android MediaCodec | 鸿蒙 av_codec（C 接口） |
| --- | --- | --- |
| 给应用的句柄 | `uint8_t*` / `ByteBuffer` 裸指针 | `OH_AVBuffer*` 裸指针 |
| 框架侧引用维持 | 有，`mPortBuffers` 持有 `sp<>` | 通常无 |
| buffer 何时释放 | `kWhatReleaseCompleted` 处理时（不是对象析构时） | 若实现为"回收即释放"，立即悬空 |
| 应用可响应的窗口 | ≥0.5s（ERROR_RECLAIMED 提前通知 + WOULD_BLOCK 等待） | 通常无窗口 |
| crash 风险 | 0.5s 内未停止访问 buffer 会 crash | 回收时直接 free 会 crash |

两者本质相同：回收完成后继续访问 buffer 都会 crash。区别在于 Android 有明确的安全窗口——应用在 buffer 释放前就收到 `ERROR_RECLAIMED`，拥有至少 0.5s 停止访问。

裸指针 C 接口要避免 crash，需要：
1. 回收流程强制要求应用先归还所有 buffer（或回调通知停止访问）后再释放；或
2. 回收时保留应用已持有 buffer 的底层内存，等应用 release 后再释放（代价是内存泄漏）。

## 13. 机制边界

- 普通抢占通常一次只释放一个 codec，不做精确容量规划；由于框架不知道真实缺口，释放后仍可能不满足申请方需求，最终依赖重试结果判断是否有效。
- 本文主流程覆盖创建、configure、start 返回 `NO_MEMORY` 后的回收重试。变分辨率等运行中重分配失败时，当前 `MediaCodec` 不会主动调用资源管理服务回收其他 codec 后再自动重试，通常直接由底层 codec 报错并进入错误回调或 fatal error 路径。
- secure 共存冲突可能一次释放多个 codec，但要求全部目标都满足严格优先级条件。
- 后台进程没有单独的自动回收规则；它只是通常拥有较低进程优先级，因此资源不足时更容易被选中。
- 资源登记值是计数或估算，不保证与真实可释放容量一致。
- `stop()` 后厂商 HAL 是否释放部分内部硬件资源取决于具体实现，框架仍保留 codec component。

## 14. 主要代码位置

本文主逻辑以当前源码中的新版 `ResourceManagerServiceNew` 默认策略为准。旧版服务同样先处理 secure 冲突，再按图形内存、同类型 codec、另一 secure 类型 codec 逐级查找跨进程目标，但没有最后的同进程 importance 策略。

| 逻辑 | 文件 |
| --- | --- |
| 创建、configure、start 的回收重试 | `media/libstagefright/MediaCodec.cpp` |
| 回收目标执行和强制释放 | `media/libstagefright/MediaCodec.cpp`、`services/mediaresourcemanager/ResourceManagerService.cpp` |
| 新版目标选择总入口 | `services/mediaresourcemanager/ResourceManagerServiceNew.cpp` |
| secure 冲突和普通候选集合 | `services/mediaresourcemanager/DefaultResourceModel.cpp` |
| 跨进程 OOM 优先级策略 | `services/mediaresourcemanager/ProcessPriorityReclaimPolicy.cpp` |
| 同进程 importance 策略 | `services/mediaresourcemanager/ClientImportanceReclaimPolicy.cpp` |
| 资源匹配和最大客户端选择 | `services/mediaresourcemanager/ResourceTracker.cpp` |
| 资源类型及登记结构 | `services/mediaresourcemanager/aidl/android/media/MediaResourceParcel.aidl`、`MediaResourceType.aidl`、`MediaResourceSubType.aidl` |
| OMX codec 释放 | `media/libstagefright/ACodec.cpp` |
| Codec2 codec 释放 | `media/codec2/sfplugin/CCodec.cpp` |
