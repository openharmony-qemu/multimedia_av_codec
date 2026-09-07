# Open GOP 接口设计调研

> 调研范围：仅 Android（MediaCodec / MediaFormat）与 iOS（VideoToolbox）原生编解码接口。用途：为 OpenHarmony av_codec 编码侧是否补充 open GOP 能力提供对标参考。

## 1. 背景与术语

GOP（Group of Pictures）是视频编码中以关键帧为首的一组连续图像。按关键帧是否打破前向参考链，分为两类：

| 类型 | 关键帧 NAL | 是否允许后续帧引用上一 GOP | 随机接入 | 压缩效率 |
| --- | --- | --- | --- | --- |
| Closed GOP | IDR | 否（IDR 重置 DPB） | 任意 IDR 可独立解码 | 略低 |
| Open GOP | non-IDR I 帧（H.264）/ CRA（H.265） | 是（RASL 帧可跨 GOP 引用） | 仅 CRA 处接入时需丢弃 RASL | 略高 |

> H.264 用 non-IDR I 帧实现 open GOP、IDR 实现 closed GOP；H.265 用 CRA 实现 open GOP、IDR 实现 closed GOP。按 x265/ffmpeg 等主流工程口径，closed GOP 即以 IDR 为边界（IDR 重置 DPB），CRA（non-IDR）一律归为 open GOP。H.265 的 RADL/RASL 是 CRA/IDR 之后 leading picture 的细分（RASL 跨 GOP 引用 CRA 之前帧、RADL 不引用），不影响"IDR=closed、CRA=open"的主流划分。

## 2. av_codec 新增 API 设计（open GOP）

> 目标：对标 iOS `kVTCompressionPropertyKey_AllowOpenGOP`（见 3 节），在 OpenHarmony av_codec 编码侧补齐 open/closed GOP 控制能力。设计严格遵循现有 B 帧的"key + capability feature"双件模式（`OH_MD_KEY_VIDEO_ENCODER_ENABLE_B_FRAME` + `VIDEO_ENCODER_B_FRAME`）。

### 2.1 新增 Key（仅 configure 阶段）

在 `interfaces/kits/c/native_avcodec_base.h` 新增 key 声明，对齐 B 帧 key 的注释格式：

**英文版**（对齐现有 B 帧 key 注释风格）：

```c
/**
 * @brief Key to enable open GOP encoding, value type is int32_t (0 or 1):
 * 1 is open GOP (key frame is allowed to be non-IDR(H.264)/CRA(H.265), subsequent frames can reference the previous GOP),
 * 0 is closed GOP (key frame is IDR, subsequent frames cannot reference the previous GOP).
 *
 * This is an optional key that applies only to video encoder, default is 0 (closed GOP).\n
 * Open GOP usually needs to be used with B-frame reordering ({@link OH_MD_KEY_VIDEO_ENCODER_ENABLE_B_FRAME})
 * to achieve significant compression gain.\n
 * For unsupported platforms, configuring this key will have no effect.\n
 * Platform capability can be checked via {@link OH_AVCapability_IsFeatureSupported} with
 * {@link OH_AVCapabilityFeature::VIDEO_ENCODER_OPEN_GOP}.\n
 * It's only used in configuration phase.\n
 *
 * @since 26.1.0
 */
extern const char *OH_MD_KEY_VIDEO_ENCODER_ENABLE_OPEN_GOP;
```

**中文版**：

```c
/**
 * @brief 开启 open GOP 编码的 key，值类型为 int32_t (0 或 1)：
 * 1 为 open GOP（关键帧允许为 non-IDR(H.264)/CRA(H.265)，后续帧可跨 GOP 引用），
 * 0 为 closed GOP（关键帧为 IDR，后续帧不可跨 GOP 引用）。
 *
 * 该 key 为可选 key，仅用于视频编码器，默认值为 0（closed GOP）。\n
 * open GOP 通常需配合 B 帧重排（{@link OH_MD_KEY_VIDEO_ENCODER_ENABLE_B_FRAME}）才有显著压缩收益。\n
 * 不支持的平台配置该 key 不生效。\n
 * 平台能力可通过 {@link OH_AVCapability_IsFeatureSupported} 配合
 * {@link OH_AVCapabilityFeature::VIDEO_ENCODER_OPEN_GOP} 查询。\n
 * 仅在配置阶段生效。\n
 *
 * @since 26.1.0
 */
extern const char *OH_MD_KEY_VIDEO_ENCODER_ENABLE_OPEN_GOP;
```

对应在 `frameworks/native/capi/avcodec/native_avcodec_base.cpp` 定义字符串值：

```cpp
const char *OH_MD_KEY_VIDEO_ENCODER_ENABLE_OPEN_GOP = "video_encoder_enable_open_gop";
```

### 2.2 新增能力查询 Feature

在 `interfaces/kits/c/native_avcapability.h` 的 `OH_AVCapabilityFeature` 枚举新增：

**英文版**（对齐现有 feature 注释风格）：

```c
/** Feature for codec supports open GOP encoding. It is only used in video encoder.
* @since <next>
*/
VIDEO_ENCODER_OPEN_GOP = 11,
```

**中文版**：

```c
/** 编解码器支持 open GOP 编码的特性。仅用于视频编码器。
* @since <next>
*/
VIDEO_ENCODER_OPEN_GOP = 11,
```

要点：
- **枚举值**接续现有最大值（`VIDEO_ENCODER_PREPROC_CROP = 10`），新增 `= 11`。
- **复用现有查询函数，不新增专门查询接口**，对齐 B 帧的现有查询模式：
  - `OH_AVCapability_IsFeatureSupported(capability, VIDEO_ENCODER_OPEN_GOP)`：判断当前编码器是否支持 open GOP。
  - `OH_AVCapability_GetFeatureProperties(capability, VIDEO_ENCODER_OPEN_GOP)`：返回 `OH_AVFormat*`，可携带附加属性（如平台默认 open/closed）；若无需附加属性，返回空 format。
- 查询链路与 B 帧一致：`IsFeatureSupported` → `GetFeatureProperties` → `OH_AVFormat_GetIntValue`。

### 2.3 使用流程（C 伪代码）

```c
// 1. 查询能力：判断当前 HEVC 编码器是否支持 open GOP
OH_AVCapability *cap = OH_AVCodec_GetCapability("video/hevc", true);
bool openGopSupported = OH_AVCapability_IsFeatureSupported(cap, VIDEO_ENCODER_OPEN_GOP);

// 2. 构建 configure 参数（key 仅在 configure 阶段生效）
OH_AVFormat *format = OH_AVFormat_Create();
OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, 1920);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 1080);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, 1000);  // GOP 间隔（单位毫秒）
if (openGopSupported) {
    // 开启 open GOP（建议配合 B 帧重排，open GOP 收益才显著，见 7 节）
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_B_FRAME, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_OPEN_GOP, 1);
}
// 3. configure（此处仅展示 key 设置，编码器实例创建 + configure 调用略）
OH_AVFormat_Destroy(format);
```

### 2.4 设计要点与对标

| 维度 | 本设计（av_codec） | iOS 对标 |
| --- | --- | --- |
| 控制开关 | `OH_MD_KEY_VIDEO_ENCODER_ENABLE_OPEN_GOP` (0/1) | `kVTCompressionPropertyKey_AllowOpenGOP` (bool) |
| 配置时机 | 仅 configure 阶段 | session 创建后 |
| 与 B 帧开关关系 | 独立两个 key（对齐 iOS 两个独立 property） | `AllowOpenGOP` 与 `AllowFrameReordering` 独立 |
| 能力查询 | `IsFeatureSupported(VIDEO_ENCODER_OPEN_GOP)` | 无显式查询（文档未标注默认值） |
| 默认值 | 0（closed，保证随机接入兼容） | 文档未标注（经验通常 open） |

### 2.5 实现侧要点（供编码器实现参考）

- **H.264 编码器**：open=1 时，GOP 边界 I 帧用 non-IDR（不重置 DPB）；open=0 时用 IDR。
- **H.265 编码器**：open=1 时，GOP 边界用 CRA（允许 RASL 跨 GOP 引用）；open=0 时用 IDR。
- **参考 AOSP 软编现状**（见 4.2 节）：HEVC 软编默认已 open（`i4_max_cra_open_gop_period` 非零），AVC 软编默认 closed。实现时需把该 key 接到对应底层参数。
- **Demuxer 侧**已具备 open GOP seek 处理（`services/media_engine/modules/demuxer/media_demuxer.cpp` 的 `skipOpenGopUnrefFrameCnt`），编码侧补齐后端到端即可闭环。

## 3. iOS — VideoToolbox

### 3.1 关键结论

**✅ 唯一在公开 API 显式提供 open GOP 控制的平台。**

通过 `kVTCompressionPropertyKey_AllowOpenGOP`（iOS 12.0+ / macOS 10.14+）控制。

| Property Key | 类型 | 可用版本 | 官方原文 |
| --- | --- | --- | --- |
| **`kVTCompressionPropertyKey_AllowOpenGOP`** ⭐ | Boolean | iOS 12.0+ / macOS 10.14+ | "Enables Open GOP (Group Of Pictures) encoding." |

- `AllowOpenGOP = true`：key frame 允许为 non-IDR（H.264）/ CRA（H.265），后续帧可跨 GOP 引用 → open GOP。
- `AllowOpenGOP = false`：所有 key frame 为 IDR，后续帧不可跨 GOP 引用 → closed GOP。
- 默认值 Apple 文档未显式标注（经验上通常为 true），需真机实测确认。

> Apple API 层面仍用 "key frame" 统称，不显式暴露 IDR/CRA NAL 类型给应用，但通过该开关间接控制。

### 3.2 接口伪代码（Swift）

```swift
// ===== 创建压缩会话 =====
var session: VTCompressionSession?
VTCompressionSessionCreate(
    allocator: kCFAllocatorDefault,
    width: 1920, height: 1080,
    codecType: kCMVideoCodecType_H264,
    encoderSpecification: nil,
    imageBufferAttributes: nil,
    compressedDataAllocator: nil,
    outputHandler: { _, _, sampleBuffer in
        // 编码后的 CMSampleBuffer 在此回调
    },
    refcon: nil,
    &session
)

// ⭐ open GOP 开关：true = 允许 non-IDR I 帧，后续帧可跨 GOP 引用（压缩效率更高）
VTSessionSetProperty(session!,
    property: kVTCompressionPropertyKey_AllowOpenGOP, value: true)

VTCompressionSessionPrepareToEncodeFrames(session!)
```

```swift
// ===== Closed GOP 模式（直播/切片场景，便于任意点随机接入） =====
VTSessionSetProperty(session!,
    property: kVTCompressionPropertyKey_AllowOpenGOP, value: false)
// 所有 key frame 均为 IDR，后续帧不可跨 GOP 引用
```

## 4. Android — MediaCodec / MediaFormat

### 4.1 关键结论

**❌ 无 open/closed GOP 显式开关。**

> 市面上 `KEY_GOP_SIZE`、`KEY_GOP_USAGE`、`KEY_IDR_INTERVAL` 等名称来自厂商私有扩展（`vendor.custom` 前缀或 OMX 层），不在公开 Java API 中。

**但源码分析表明，AOSP 软件编码器内部存在 open GOP 的默认行为差异（见 4.2 节）。**

### 4.2 AOSP 软件编码器源码分析——默认行为

虽然公开 API 无开关，但 AOSP `frameworks/av` 源码显示，不同软件编码器在内部实现上对 open GOP 的默认处理不同：

#### 4.2.1 HEVC 软编 `c2.android.hevc.encoder`——默认配置了 open GOP

`C2SoftHevcEnc.cpp:717-718` 将 sync frame period 直接写入 open GOP 参数：

```cpp
// C2SoftHevcEnc.cpp:717-718
mEncParams.s_coding_tools_prms.i4_max_i_open_gop_period = mIDRInterval;
mEncParams.s_coding_tools_prms.i4_max_cra_open_gop_period = mIInterval;
```

- 参数名显式包含 `open_gop`，CRA（Clean Random Access）是 HEVC open GOP 的标志帧类型。
- `mIDRInterval` 和 `mIInterval` 默认均来自 `getSyncFramePeriod_l()`（`C2SoftHevcEnc.cpp:824-825`），默认值为 1000000μs ≈ 1 秒（`C2SoftHevcEnc.cpp:206-207`），换算后约 30 帧（30fps）。
- 默认 GOP `flexCount=0`（`C2SoftHevcEnc.cpp:95-96`），ParseGop 不会被触发，上述值不会被覆盖。
- **结论：HEVC 软编默认配置了 open GOP。**

#### 4.2.2 AVC 软编 `c2.android.avc.encoder`——默认是 closed GOP

`C2SoftAvcEnc.cpp:1013-1014` 使用 libIve API 设置 GOP 参数：

```cpp
// C2SoftAvcEnc.cpp:1013-1014
s_gop_params_ip.u4_i_frm_interval = mIInterval;
s_gop_params_ip.u4_idr_frm_interval = mIDRInterval;
```

- `C2SoftAvcEnc.h` 定义了 `DEFAULT_I_INTERVAL=30`、`DEFAULT_IDR_INTERVAL=1000`（30 ≠ 1000，看似 open GOP），但在 `initEncoder()` 中被覆盖为同一个值：

```cpp
// C2SoftAvcEnc.cpp:1177-1178
mIInterval = mIntf->getSyncFramePeriod_l();
mIDRInterval = mIntf->getSyncFramePeriod_l();
```

- 二者相等 → 每个 I 帧都是 IDR 帧 → **closed GOP**。
- `DEFAULT_I_INTERVAL` / `DEFAULT_IDR_INTERVAL` 仅在构造函数 `initEncParams()` 中赋值（`C2SoftAvcEnc.cpp:665-666`），随后被 `initEncoder()` 覆盖，不生效。
- **结论：AVC 软编默认是 closed GOP。**

#### 4.2.3 硬件编码器——厂商决定

高通 `OMX.qcom.video.encoder.*`、三星 `OMX.Exynos.*` 等硬件编码器实现不在此仓库中，无法从源码判断。实际行为取决于厂商 HAL 实现。

#### 4.2.4 小结

| 编码器 | 组件名 | 默认 GOP 类型 | 依据 |
| --- | --- | --- | --- |
| HEVC 软编 | `c2.android.hevc.encoder` | **Open GOP** | `i4_max_i_open_gop_period` / `i4_max_cra_open_gop_period` 设为非零值 |
| AVC 软编 | `c2.android.avc.encoder` | **Closed GOP** | `u4_i_frm_interval == u4_idr_frm_interval` |
| 硬编 | `OMX.*.encoder.*` | 厂商决定 | 源码不可见 |

## 5. 横向对比

| 维度 | iOS VideoToolbox | Android MediaFormat |
| --- | --- | --- |
| **open/closed GOP 显式开关** | ✅ `kVTCompressionPropertyKey_AllowOpenGOP`（iOS 12+） | ❌ 无 |
| **默认 open 还是 closed** | 文档未标注（经验上通常 open） | 软编 HEVC = open，软编 AVC = closed（源码分析），硬编 = 厂商决定 |
| **控制粒度** | 布尔开关，间接控制 key frame 是否 IDR | 无法控制 |

## 6. 关键链接

**iOS VideoToolbox**
- ⭐ AllowOpenGOP：https://developer.apple.com/documentation/videotoolbox/kvtcompressionpropertykey_allowopengop
- Compression Properties：https://developer.apple.com/documentation/videotoolbox/compression-properties

**Android**
- MediaFormat：https://developer.android.com/reference/android/media/MediaFormat
- AOSP 源码：https://github.com/aosp-mirror/platform_frameworks_base/blob/master/media/java/android/media/MediaFormat.java

## 7. 知识补充：open GOP 收益为何主要来自 B 帧

> 本节澄清一个常见误读："open GOP 是否仅 B 帧才用、P 帧不支持"。结论：**P 帧也能跨 GOP 引用，open GOP 对 P 帧有效但收益相对小；收益主要来自 B 帧**。根源不在"参考帧质量高低"，而在 B 帧重排产生的"跨 GOP 边界帧"在 closed GOP 下会被切断前向参考，而 P 帧没有这个结构性问题。

### 7.1 B 帧重排必然产生"跨 GOP 边界帧"

B 帧双向预测，需要后向参考帧先解码，故**解码顺序 ≠ 显示顺序**。在 GOP 边界处会出现一类特殊帧——显示顺序属于前一 GOP 末尾、解码顺序属于新 GOP 开头：

```
显示顺序: ... P(n-1)  Bn | I(n+1)  B(n+2) ...
解码顺序: ... P(n-1) I(n+1)  Bn  ...            （Bn 被重排到 I(n+1) 之后解码）
```

`Bn` 显示在前一 GOP 末尾，但要等新 GOP 的 `I(n+1)` 解码后才能解码。它的前向参考指向上一 GOP（如 `P(n-1)`），后向参考指向本 GOP（`I(n+1)`）。这类帧在 H.265 中即 CRA 之后的 **RASL**（显示在 CRA 之前、引用 CRA 之前帧的 leading picture）。

### 7.2 Closed GOP 切断跨边界 B 帧的前向参考

closed GOP 用 IDR 作为 GOP 边界。IDR 重置 DPB（解码图像缓冲），清空所有参考帧，直接切断跨边界 B 帧的前向参考：

- `Bn` 解码时，DPB 里已没有前一 GOP 的帧（被 IDR 清空）→ 前向参考丢失 → B 帧从双向退化为单向后向。
- 因此 closed GOP 中，跨 GOP 边界的 B 帧要么不被放置（边界处改用 P 帧），要么被强制为不引用前一 GOP 的形式（H.265 的 RADL，只引用本 GOP 帧）。

> H.265 标准明确：IDR 之后只能跟 RADL（不引用 IDR 之前帧），不能跟 RASL。这是 closed GOP 禁止跨边界 B 帧引用前一 GOP 的标准依据。

无论哪种处理，都损失了边界处"B 帧双向引用前一 GOP"的压缩优势。

### 7.3 Open GOP 解禁跨边界 B 帧

open GOP 用 CRA（H.265）/ non-IDR I 帧（H.264）作为 GOP 边界，**不重置 DPB**。跨边界 B 帧能维持完整双向引用：

- 前向引用：上一 GOP 末帧（如 `P(n-1)`，时间相邻、相关性强）
- 后向引用：本 GOP 的 CRA / non-IDR I 帧

这就是 RASL。open GOP 让 GOP 边界处也能用双向 B 帧压缩，这是 open GOP 收益的主要来源。

### 7.4 P 帧为何收益相对小

P 帧单向预测，**显示顺序 = 解码顺序**，不存在"显示在前 GOP、解码在新 GOP"的跨边界 P 帧。P 帧总是在本 GOP 内解码、引用本 GOP 帧：

- closed GOP（IDR 边界）：边界后第一个 P 帧引用本 GOP 的 IDR（帧内编码、质量高），P 帧正常工作，没有被"剥夺"什么。
- open GOP（non-IDR 边界）：边界后第一个 P 帧可引用本 GOP 的 I 帧，也可额外引用上一 GOP 末帧（多一个前向参考选项）。

差异在于性质：

| | closed GOP 下的处境 | open GOP 的收益 |
| --- | --- | --- |
| **B 帧（跨边界）** | IDR 重置 DPB 切断前向参考 → 禁止 RASL，只允许 RADL 或改用 P 帧 | 解禁 RASL，边界处恢复双向压缩（**雪中送炭**） |
| **P 帧** | 无跨边界场景，正常引用本 GOP 帧 | 边界后第一帧多一个前向参考选项（**锦上添花**） |

B 帧是 closed GOP **禁止一整类帧存在**，open GOP **解禁**之；P 帧在 closed GOP 下本就正常工作，open GOP 只是给边界后第一帧**多一个可选参考**。

### 7.5 收益范围限定（避免误读）

- **GOP 内部 B 帧**在 closed / open 下都正常双向引用本 GOP 帧，不受 open GOP 影响。open GOP 解禁的只是"跨 GOP 边界"的 B 帧。
- 因此 open GOP 的收益是**GOP 边界处**的局部收益，不是全局 B 帧收益。
- P 帧的收益是边界后第一帧"多一个参考选项"的边际增益，通常较小（具体取决于内容与 GOP 长度）。
- **P 帧不是"不支持 open GOP"**：它同样能跨 GOP 前向引用，只是没有 B 帧那种被 closed GOP 结构性切断的问题，故增益相对小。

### 7.6 对 H.264 同样成立

H.264 没有 CRA / RASL / RADL 这些 NAL 类型，但机制等价：

- non-IDR I 帧不重置 DPB → 跨边界 B 帧可前向引用上一 GOP → open GOP。
- IDR 重置 DPB → 跨边界 B 帧无法引用上一 GOP → closed GOP。

iOS `kVTCompressionPropertyKey_AllowOpenGOP` 对 H.264 和 HEVC 编码均适用，上述论证对两者都成立。

### 7.7 结论速览

> 一句话：open GOP 收益主要来自 B 帧，**不是因为 P 帧不能跨 GOP 引用（它能），而是 open GOP 解决的是 B 帧重排特有的"跨边界帧在 closed GOP 下被切断前向参考"问题，P 帧没有这个结构性问题**。
