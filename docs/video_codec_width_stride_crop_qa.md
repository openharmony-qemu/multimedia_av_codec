# 视频编解码宽高、跨距与裁剪 Key 使用答疑

## 目录

- [1. Key 速查表](#1-key-速查表)
- [2. 各 Key 含义详解](#2-各-key-含义详解)
  - [2.1 OH_MD_KEY_WIDTH / OH_MD_KEY_HEIGHT](#21-oh_md_key_width--oh_md_key_height)
  - [2.2 OH_MD_KEY_VIDEO_STRIDE](#22-oh_md_key_video_stride)
  - [2.3 OH_MD_KEY_VIDEO_SLICE_HEIGHT](#23-oh_md_key_video_slice_height)
  - [2.4 OH_MD_KEY_VIDEO_PIC_WIDTH / OH_MD_KEY_VIDEO_PIC_HEIGHT](#24-oh_md_key_video_pic_width--oh_md_key_video_pic_height)
  - [2.5 OH_MD_KEY_VIDEO_CROP_LEFT/TOP/RIGHT/BOTTOM](#25-oh_md_key_video_crop_lefttoprightbottom)
  - [2.6 OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT/TOP/RIGHT/BOTTOM](#26-oh_md_key_video_encoder_preproc_crop_lefttoprightbottom)
- [3. 接口调用 Key 使用指南](#3-接口调用-key-使用指南)
  - [3.1 解码器 Configure](#31-解码器-configure)
  - [3.2 解码器 GetOutputFormat](#32-解码器-getoutputformat)
  - [3.3 编码器 Configure](#33-编码器-configure)
  - [3.4 编码器 GetInputFormat](#34-编码器-getinputformat)
  - [3.5 编码器 GetOutputFormat](#35-编码器-getoutputformat)
  - [3.6 预处理器 Configure](#36-预处理器-configure)
- [4. 典型场景调用示例](#4-典型场景调用示例)
  - [4.1 解码器分辨率变化处理](#41-解码器分辨率变化处理)
  - [4.2 编码器输入缓冲区准备](#42-编码器输入缓冲区准备)
  - [4.3 编码器预处理裁剪配置](#43-编码器预处理裁剪配置)
- [5. 常见疑问解答](#5-常见疑问解答)

---

## 1. Key 速查表

| Key 名称 | 字符串值 | 数据类型 | 主要用途 | 解码器使用接口 | 编码器使用接口 |
|----------|----------|----------|----------|----------------|----------------|
| `OH_MD_KEY_WIDTH` | `"width"` | int32_t | 视频逻辑宽度 | **Configure**（设置，必需） | **Configure**（设置，必需）<br>**GetInputFormat**（获取）<br>**GetOutputFormat**（获取） |
| `OH_MD_KEY_HEIGHT` | `"height"` | int32_t | 视频逻辑高度 | **Configure**（设置，必需） | **Configure**（设置，必需）<br>**GetInputFormat**（获取）<br>**GetOutputFormat**（获取） |
| `OH_MD_KEY_VIDEO_STRIDE` | `"stride"` | int32_t | 缓冲区行跨距（字节） | **GetOutputFormat**（获取） | **GetInputFormat**（获取） |
| `OH_MD_KEY_VIDEO_SLICE_HEIGHT` | `"video_slice_height"` | int32_t | Y 平面高度（行） | **GetOutputFormat**（获取） | **GetInputFormat**（获取） |
| `OH_MD_KEY_VIDEO_PIC_WIDTH` | `"video_picture_width"` | int32_t | 有效画面宽度 | **GetOutputFormat**（获取） | **GetOutputFormat**（获取） |
| `OH_MD_KEY_VIDEO_PIC_HEIGHT` | `"video_picture_height"` | int32_t | 有效画面高度 | **GetOutputFormat**（获取） | **GetOutputFormat**（获取） |
| `OH_MD_KEY_VIDEO_CROP_LEFT` | `"video_crop_left"` | int32_t | 解码输出裁剪左边界 | **GetOutputFormat**（获取） | 不适用 |
| `OH_MD_KEY_VIDEO_CROP_TOP` | `"video_crop_top"` | int32_t | 解码输出裁剪上边界 | **GetOutputFormat**（获取） | 不适用 |
| `OH_MD_KEY_VIDEO_CROP_RIGHT` | `"video_crop_right"` | int32_t | 解码输出裁剪右边界 | **GetOutputFormat**（获取） | 不适用 |
| `OH_MD_KEY_VIDEO_CROP_BOTTOM` | `"video_crop_bottom"` | int32_t | 解码输出裁剪下边界 | **GetOutputFormat**（获取） | 不适用 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT` | `"video_encoder_preproc_crop_left"` | int32_t | 编码预处理裁剪左边界 | 不适用 | **Preprocessor Configure**（设置）<br>**GetInputDescription**（获取） |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP` | `"video_encoder_preproc_crop_top"` | int32_t | 编码预处理裁剪上边界 | 不适用 | **Preprocessor Configure**（设置）<br>**GetInputDescription**（获取） |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT` | `"video_encoder_preproc_crop_right"` | int32_t | 编码预处理裁剪右边界 | 不适用 | **Preprocessor Configure**（设置）<br>**GetInputDescription**（获取） |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM` | `"video_encoder_preproc_crop_bottom"` | int32_t | 编码预处理裁剪下边界 | 不适用 | **Preprocessor Configure**（设置）<br>**GetInputDescription**（获取） |

---

## 2. 各 Key 含义详解

### 2.1 OH_MD_KEY_WIDTH / OH_MD_KEY_HEIGHT

**字符串值**：`"width"` / `"height"`

**具体含义**：

这两个 Key 表示视频的**逻辑分辨率**，即你期望编解码器处理的画面宽高。

- 在 **Configure 阶段**：由调用者设置，告诉编解码器要处理多大分辨率的视频
- 在 **GetOutputFormat 阶段**：由编解码器返回，但**该用法已废弃 (deprecated)**，建议使用 `VIDEO_PIC_WIDTH/HEIGHT` 替代

**典型值**：1920、1280、640 等

**代码定义位置**：
- 头文件：`interfaces/kits/c/native_avcodec_base.h`
- 实现文件：`frameworks/native/capi/avcodec/native_avcodec_base.cpp:84-85`

```c
/* Key for video width, value type is int32_t */
extern const char *OH_MD_KEY_WIDTH;
/* Key for video height, value type is int32_t */
extern const char *OH_MD_KEY_HEIGHT;
```

**内部等价定义**：
```cpp
// interfaces/inner_api/native/media_description.h:81,86
static constexpr std::string_view MD_KEY_WIDTH = "width";
static constexpr std::string_view MD_KEY_HEIGHT = "height";
```

---

### 2.2 OH_MD_KEY_VIDEO_STRIDE

**字符串值**：`"stride"`

**具体含义**：

Stride 表示视频缓冲区中**每一行数据占用的字节数**，也称为行跨距或行增量。

**关键点**：
- Stride **不等于** Width × 每像素字节数，因为硬件通常要求内存对齐
- Stride 通常向上对齐到 16、32 或 64 字节的整数倍
- 对于 YUV 420 格式，Stride 对应 Y 分量的行跨距
- U/V 分量的 Stride 可根据色彩格式推算（通常为 Y Stride 的一半）

**为什么需要 Stride？**

假设视频宽度为 1920，NV12 格式每像素 1 字节（Y 平面）：
- 理论每行字节数 = 1920
- 若硬件要求 32 字节对齐，则实际 Stride = 1920（已对齐）
- 若视频宽度为 1921，则 Stride = 1952（向上对齐到 32 的倍数）

**代码定义位置**：
- 头文件：`interfaces/kits/c/native_avcodec_base.h:1031-1040`

```c
/**
 * @brief Key for describing the stride of the video buffer layout, value type is int32_t. 
 * Stride (or row increment) is the difference between the index of a pixel and that of 
 * the pixel directly underneath. For YUV 420 formats, the stride corresponds to the Y plane; 
 * the stride of the U and V planes can be calculated based on the color format.
 *
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 12
 */
extern const char *OH_MD_KEY_VIDEO_STRIDE;
```

**Stride 计算示例**（来自 `avc_encoder.cpp:1088-1091`）：
```cpp
if (srcPixelFmt_ == VideoPixelFormat::RGBA) {
    // RGBA 格式：每像素 4 字节
    format.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, encWidth_ * RGBA_COLINC);
} else {
    // YUV 格式：每像素 1 字节（Y 平面）
    format.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, encWidth_);
}
```

---

### 2.3 OH_MD_KEY_VIDEO_SLICE_HEIGHT

**字符串值**：`"video_slice_height"`

**具体含义**：

Slice Height 表示 YUV 多平面缓冲区中 **Y 平面的实际高度（行数）**，也称为垂直跨距或平面高度。

**关键点**：
- Slice Height **不等于** 视频的 Height，因为垂直方向也需要内存对齐
- 对于 YUV 420 格式，U/V 平面紧跟在 Y 平面之后存储
- U 平面的起始偏移 = `sliceHeight × stride`

**为什么需要 Slice Height？**

假设视频高度为 1080：
- 理论 Y 平面行数 = 1080
- 若硬件要求垂直方向 32 行对齐，则实际 SliceHeight = 1088

**内存布局示意**：
```
缓冲区起始
    │
    ▼
┌───────────────────────┐ ← Y 平面起始
│                       │
│   Y 数据区域          │
│   (Stride × SliceHeight 字节)
│                       │
├───────────────────────┤ ← U/V 平面起始（偏移 = SliceHeight × Stride）
│                       │
│   UV 数据区域          │
│   (Stride × SliceHeight/2 字节)
│                       │
└───────────────────────┘
```

**代码定义位置**：
- 头文件：`interfaces/kits/c/native_avcodec_base.h:1041-1051`

```c
/**
 * @brief Key for describing the plane height of a multi-planar (YUV) video buffer layout, 
 * value type is int32_t.
 * Slice height (or plane height/vertical stride) is the number of rows that must be skipped 
 * to get from the top of the Y plane to the top of the U plane in the buffer. In essence 
 * the offset of the U plane is sliceHeight * stride.
 *
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 12
 */
extern const char *OH_MD_KEY_VIDEO_SLICE_HEIGHT;
```

**SliceHeight 计算示例**（来自 `hdecoder.cpp:686-689`）：
```cpp
// 通过 U 平面的 offset 反推 sliceHeight
int32_t stride = surfaceBuffer->GetStride();
int32_t sliceHeight = static_cast<int32_t>(
    static_cast<int64_t>(planes->planes[1].offset) / stride);
outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_SLICE_HEIGHT, sliceHeight);
```

---

### 2.4 OH_MD_KEY_VIDEO_PIC_WIDTH / OH_MD_KEY_VIDEO_PIC_HEIGHT

**字符串值**：`"video_picture_width"` / `"video_picture_height"`

**具体含义**：

这两个 Key 表示视频帧中**有效画面的实际宽高**，即去除填充、裁剪后真正需要显示或处理的图像区域尺寸。

**关键点**：
- 这是**推荐使用**的获取实际画面尺寸的方式
- 可能与缓冲区尺寸（Stride/SliceHeight）不同
- 可能与逻辑宽高（WIDTH/HEIGHT）不同（当存在裁剪时）

**与 WIDTH/HEIGHT 的区别**：
- `WIDTH/HEIGHT`：配置时指定的期望分辨率
- `PIC_WIDTH/PIC_HEIGHT`：解码或编码后实际有效的画面尺寸

**代码定义位置**：
- 头文件：`interfaces/kits/c/native_avcodec_base.h:1052-1069`

```c
/**
 * @brief Key for describing the valid picture width of the video, value type is int32_t.
 */
extern const char *OH_MD_KEY_VIDEO_PIC_WIDTH;
/**
 * @brief Key for describing the valid picture height of the video, value type is int32_t.
 */
extern const char *OH_MD_KEY_VIDEO_PIC_HEIGHT;
```

---

### 2.5 OH_MD_KEY_VIDEO_CROP_LEFT/TOP/RIGHT/BOTTOM

**字符串值**：
- `"video_crop_left"`
- `"video_crop_top"`
- `"video_crop_right"`
- `"video_crop_bottom"`

**具体含义**：

这四个 Key 共同定义了解码输出帧中的**裁剪矩形区域**，用于指示有效画面在缓冲区中的位置。

**坐标说明**：
- `(left, top)`：裁剪矩形左上角坐标
- `(right, bottom)`：裁剪矩形右下角坐标
- 坐标从 0 开始计数
- **right 和 bottom 是包含边界的**（inclusive）

**裁剪尺寸计算**：
```
裁剪宽度  = right - left + 1
裁剪高度 = bottom - top + 1
```

**默认值**（来自 `video_decoder.cpp:622-628`）：
```cpp
// 未显式设置时的默认值
format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_RIGHT, width_-1);
format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_BOTTOM, height_-1);
format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_LEFT, 0);
format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_TOP, 0);
```

**代码定义位置**：
- 头文件：`interfaces/kits/c/native_avcodec_base.h:998-1030`

```c
/**
 * @brief Key for describing the left-coordinate (x) of the crop rectangle, value type is int32_t.
 * This is the left-most column included in the crop frame, where column indices start at 0.
 * This key is only used for video decoding.
 * @since 12
 */
extern const char *OH_MD_KEY_VIDEO_CROP_LEFT;
// ... TOP, RIGHT, BOTTOM 类似定义
```

**OMX 获取 Crop 示例**（来自 `hdecoder.cpp:269-303`）：
```cpp
void HDecoder::GetCropFromOmx(uint32_t w, uint32_t h, OHOS::Rect& damage)
{
    OMX_CONFIG_RECTTYPE rect;
    GetParameter(OMX_IndexConfigCommonOutputCrop, rect, true);
    
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_LEFT, rect.nLeft);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_TOP, rect.nTop);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_RIGHT,
        static_cast<int32_t>(rect.nLeft + rect.nWidth) - 1);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_BOTTOM,
        static_cast<int32_t>(rect.nTop + rect.nHeight) - 1);
}
```

---

### 2.6 OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT/TOP/RIGHT/BOTTOM

**字符串值**：
- `"video_encoder_preproc_crop_left"`
- `"video_encoder_preproc_crop_top"`
- `"video_encoder_preproc_crop_right"`
- `"video_encoder_preproc_crop_bottom"`

**具体含义**：

这四个 Key 用于配置**编码器输入预处理阶段的裁剪区域**，指定仅编码输入帧的特定区域。

**使用限制**（来自 API 注释）：
1. left、top、right、bottom **必须同时配置**，不可单独设置
2. 当四个值**全部为 0 时，裁剪功能禁用**
3. **不能与降采样 (downsampling) 参数同时使用**
4. 启用裁剪后，编码器**仅编码裁剪区域内的数据**

**边界验证条件**（来自 `preprocessor.cpp:199-202`）：
```
0 <= left < right <= encoderConfig.width
0 <= top < bottom <= encoderConfig.height
```

**代码定义位置**：
- 头文件：`interfaces/kits/c/native_avcodec_base.h:857-925`

```c
/**
 * @brief Key for describing the left-coordinate (x) of the crop rectangle in video encoder preprocess,
 * value type is int32_t.
 *
 * The Caller must use "left, top, right, bottom" together to define the crop rectangle.
 * (left, top) is the coordinate of the top-left corner of the crop rectangle.
 * (right, bottom) is the coordinate of the bottom-right corner of the crop rectangle.
 * The width and height of the crop rectangle can be calculated as:
 * - width = right - left + 1
 * - height = bottom - top + 1
 *
 * Using restrictions:
 * 1. Crop left, top, right, bottom must be configured together.
 * 2. When crop left, top, right, bottom are all 0, the crop is disabled.
 * 3. Cannot be used together with downsampling parameters.
 * 6. When crop is enabled, the encoder will only encode the cropped area of the input frame.
 *
 * @since 26.0.0
 */
extern const char *OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT;
// ... TOP, RIGHT, BOTTOM 类似定义
```

---

## 3. 接口调用 Key 使用指南

### 3.1 解码器 Configure

**调用时机**：创建解码器后，启动解码前

**需要设置的 Key**：

| Key | 是否必需 | 说明 |
|-----|----------|------|
| `OH_MD_KEY_WIDTH` | **是** | 期望解码输出的视频宽度 |
| `OH_MD_KEY_HEIGHT` | **是** | 期望解码输出的视频高度 |
| `OH_MD_KEY_PIXEL_FORMAT` | 否 | 输出像素格式（如 NV12） |

**不需要设置的 Key**：
- `VIDEO_STRIDE`、`VIDEO_SLICE_HEIGHT`、`VIDEO_PIC_WIDTH/HEIGHT`、`VIDEO_CROP_*` 等均由解码器内部设置，调用者无需配置

**调用示例**（来自 `avcodec_video_decoder_demo.cpp:108-113`）：
```cpp
OH_AVFormat *format = OH_AVFormat_Create();
OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, 1920);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 1080);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT,
                        static_cast<int32_t>(VideoPixelFormat::NV12));
OH_AVCodecCodec_Configure(codec, format);
OH_AVFormat_Destroy(format);
```

**内部处理逻辑**（来自 `video_decoder.cpp:221-271`）：
```cpp
int32_t VideoDecoder::Configure(const Format &format)
{
    // 设置默认值
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    
    // 遍历调用者传入的 format，提取 width/height
    for (auto &it : format.GetFormatMap()) {
        if (it.first == MediaDescriptionKey::MD_KEY_WIDTH) {
            ConfigurelWidthAndHeight(format, it.first, true);
        } else if (it.first == MediaDescriptionKey::MD_KEY_HEIGHT) {
            ConfigurelWidthAndHeight(format, it.first, false);
        }
    }
    
    // 最终获取配置的宽高
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width_);
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height_);
}
```

---

### 3.2 解码器 GetOutputFormat

**调用时机**：
- 首次启动解码后，获取输出格式信息
- 收到 `AV_ERR_STREAM_CHANGED` 事件时，分辨率发生变化

**可获取的 Key**：

| Key | 说明 | 推荐度 |
|-----|------|--------|
| `OH_MD_KEY_VIDEO_PIC_WIDTH` | 有效画面宽度 | **推荐** |
| `OH_MD_KEY_VIDEO_PIC_HEIGHT` | 有效画面高度 | **推荐** |
| `OH_MD_KEY_VIDEO_STRIDE` | 缓冲区行跨距 | **推荐** |
| `OH_MD_KEY_VIDEO_SLICE_HEIGHT` | Y 平面高度 | **推荐** |
| `OH_MD_KEY_VIDEO_CROP_LEFT` | 裁剪左边界 | 需要时使用 |
| `OH_MD_KEY_VIDEO_CROP_TOP` | 裁剪上边界 | 需要时使用 |
| `OH_MD_KEY_VIDEO_CROP_RIGHT` | 裁剪右边界 | 需要时使用 |
| `OH_MD_KEY_VIDEO_CROP_BOTTOM` | 裁剪下边界 | 需要时使用 |
| `OH_MD_KEY_WIDTH` | 视频宽度 | 已废弃，不推荐 |
| `OH_MD_KEY_HEIGHT` | 视频高度 | 已废弃，不推荐 |

**调用示例**（来自 `video_decoder_sample.cpp:186-199`）：
```cpp
case AV_ERR_STREAM_CHANGED: {
    // 分辨率变化，重新获取格式信息
    auto format = context_->videoCodec->GetFormat();
    
    auto originVideoWidth = info.videoWidth;
    auto originVideoHeight = info.videoHeight;
    
    // 获取新的有效画面尺寸
    OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_PIC_WIDTH, &info.videoWidth);
    OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_PIC_HEIGHT, &info.videoHeight);
    
    // 获取缓冲区布局信息
    OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_STRIDE, &info.videoStrideWidth);
    OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_SLICE_HEIGHT, &info.videoSliceHeight);
    
    AVCODEC_LOGW("Resolution: %{public}d*%{public}d => %{public}d*%{public}d(%{public}d*%{public}d)",
        originVideoWidth, originVideoHeight, 
        info.videoWidth, info.videoHeight,
        info.videoStrideWidth, info.videoSliceHeight);
    break;
}
```

**内部设置逻辑**（来自 `video_decoder.cpp:696-710`）：
```cpp
// 当 stride 未设置或格式发生变化时
if (!format_.ContainKey(OHOS::Media::Tag::VIDEO_STRIDE) || formatChanged) {
    int32_t stride = GetSurfaceBufferStride(buffers_[INDEX_OUTPUT][index]);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, stride);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width_);      // deprecated
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height_);    // deprecated
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_SLICE_HEIGHT, height_);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, width_);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, height_);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_RIGHT, width_-1);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_BOTTOM, height_-1);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_LEFT, 0);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_TOP, 0);
    callback_->OnOutputFormatChanged(format_);
}
```

---

### 3.3 编码器 Configure

**调用时机**：创建编码器后，启动编码前

**需要设置的 Key**：

| Key | 是否必需 | 说明 |
|-----|----------|------|
| `OH_MD_KEY_WIDTH` | **是** | 期望编码的视频宽度 |
| `OH_MD_KEY_HEIGHT` | **是** | 期望编码的视频高度 |
| `OH_MD_KEY_MIME` | **是** | 编码格式（如 "video/avc"） |
| `OH_MD_KEY_BIT_RATE` | 否 | 目标码率 |
| `OH_MD_KEY_FRAME_RATE` | 否 | 帧率 |
| `OH_MD_KEY_PIXEL_FORMAT` | 否 | 输入像素格式 |

**调用示例**：
```cpp
OH_AVFormat *format = OH_AVFormat_Create();
OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, 1920);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 1080);
OH_AVFormat_SetStringValue(format, OH_MD_KEY_MIME, "video/avc");
OH_AVFormat_SetIntValue(format, OH_MD_KEY_BIT_RATE, 5000000);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, 30);
OH_AVCodecCodec_Configure(codec, format);
OH_AVFormat_Destroy(format);
```

**内部处理逻辑**（来自 `avc_encoder.cpp:416-464`）：
```cpp
int32_t AvcEncoder::Configure(const Format &format)
{
    // 设置默认值
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    
    // 遍历调用者传入的 format
    for (auto &it : format.GetFormatMap()) {
        if (it.first == MediaDescriptionKey::MD_KEY_WIDTH) {
            ConfigureDefaultVal(format, it.first, VIDEO_MIN_SIZE, VIDEO_MAX_WIDTH_SIZE);
        } else if (it.first == MediaDescriptionKey::MD_KEY_HEIGHT) {
            ConfigureDefaultVal(format, it.first, VIDEO_MIN_SIZE, VIDEO_MAX_HEIGHT_SIZE);
        }
        // ... 处理其他 key
    }
}
```

---

### 3.4 编码器 GetInputFormat

**调用时机**：编码器 Configure 之后，准备输入缓冲区前

**可获取的 Key**：

| Key | 说明 | 用途 |
|-----|------|------|
| `OH_MD_KEY_WIDTH` | 编码输入宽度 | 确认输入分辨率 |
| `OH_MD_KEY_HEIGHT` | 编码输入高度 | 确认输入分辨率 |
| `OH_MD_KEY_VIDEO_STRIDE` | 输入缓冲区行跨距 | **计算缓冲区大小** |
| `OH_MD_KEY_VIDEO_SLICE_HEIGHT` | Y 平面高度 | **计算缓冲区大小** |
| `OH_MD_KEY_PIXEL_FORMAT` | 输入像素格式 | 确认输入格式 |

**调用示例**（来自 `video_encoder_sample.cpp:44-75`）：
```cpp
// 方式一：通过 GetInputFormat 获取
auto format = context_->videoCodec->GetFormat();
OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_STRIDE, &info.videoStrideWidth);
OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_SLICE_HEIGHT, &info.videoSliceHeight);

// 方式二：通过 NativeBuffer 配置获取
OH_NativeBuffer *nativeBuffer = OH_NativeBuffer_Create(width, height, ...);
OH_NativeBuffer_Configuration config;
OH_NativeBuffer_GetConfig(nativeBuffer, &config);
info.videoStrideWidth = config.stride;
info.videoSliceHeight = info.videoHeight;
```

**内部设置逻辑**（来自 `avc_encoder.cpp:1076-1091`）：
```cpp
int32_t AvcEncoder::GetInputFormat(Format &format)
{
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, encWidth_);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, encHeight_);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, ...);
    
    // 根据像素格式计算 stride
    if (srcPixelFmt_ == VideoPixelFormat::RGBA) {
        format.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, encWidth_ * RGBA_COLINC);
    } else {
        format.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, encWidth_);
    }
}
```

---

### 3.5 编码器 GetOutputFormat

**调用时机**：编码器启动后，获取编码输出格式信息

**可获取的 Key**：

| Key | 说明 |
|-----|------|
| `OH_MD_KEY_WIDTH` | 编码输出视频宽度 |
| `OH_MD_KEY_HEIGHT` | 编码输出视频高度 |
| `OH_MD_KEY_VIDEO_PIC_WIDTH` | 有效画面宽度 |
| `OH_MD_KEY_VIDEO_PIC_HEIGHT` | 有效画面高度 |

**内部设置逻辑**（来自 `avc_encoder.cpp:1100-1103`）：
```cpp
int32_t AvcEncoder::GetOutputFormat(Format &format)
{
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, encWidth_);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, encHeight_);
    format.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, encWidth_);
    format.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, encHeight_);
}
```

---

### 3.6 预处理器 Configure

**调用时机**：使用编码器预处理功能时，配置裁剪、降采样等参数

**需要设置的 Key**：

| Key | 是否必需 | 说明 |
|-----|----------|------|
| `OH_MD_KEY_VIDEO_WIDTH` | **是** | 输入缓冲区宽度 |
| `OH_MD_KEY_VIDEO_HEIGHT` | **是** | 输入缓冲区高度 |
| `OH_MD_KEY_VIDEO_GRAPHIC_PIXEL_FORMAT` | **是** | 输入像素格式 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT` | 否 | 裁剪左边界 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP` | 否 | 裁剪上边界 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT` | 否 | 裁剪右边界 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM` | 否 | 裁剪下边界 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH` | 否 | 降采样目标宽度 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT` | 否 | 降采样目标高度 |

**调用示例**：
```cpp
OH_AVFormat *format = OH_AVFormat_Create();

// 设置输入格式
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_WIDTH, 1920);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_HEIGHT, 1080);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_GRAPHIC_PIXEL_FORMAT, 
                        static_cast<int32_t>(GraphicPixelFormat::NV12));

// 配置裁剪区域（可选）
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, 100);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, 50);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, 1700);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, 950);

Preprocessor_Configure(preprocessor, format);
OH_AVFormat_Destroy(format);
```

**内部验证逻辑**（来自 `preprocessor.cpp:175-224`）：
```cpp
int32_t Preprocessor::ValidateCrop(const Media::Format &format, CropParams &params)
{
    params.result = ConfigResult::NOT_SET;
    int32_t left{0}, right{0}, top{0}, bottom{0};
    
    bool hasLeft = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, left);
    bool hasRight = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, right);
    bool hasTop = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, top);
    bool hasBottom = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, bottom);

    bool hasAny = hasLeft || hasRight || hasTop || hasBottom;
    bool hasAll = hasLeft && hasRight && hasTop && hasBottom;

    if (!hasAny) { return AVCS_ERR_OK; }              // 未配置，跳过
    if (hasAny && !hasAll) { return AVCS_ERR_INVALID_VAL; }  // 必须四个值都配置

    // 计算裁剪宽高
    int32_t cropWidth = right - left + 1;
    int32_t cropHeight = bottom - top + 1;
    
    // 验证边界
    if (!(0 <= left && left < right && right <= encoderConfig_.width && 
          0 <= top && top < bottom && bottom <= encoderConfig_.height)) {
        return AVCS_ERR_INVALID_VAL;
    }
    
    params.left = left; params.right = right;
    params.top = top; params.bottom = bottom;
    params.result = ConfigResult::ENABLED;
    return AVCS_ERR_OK;
}
```

---

## 4. 典型场景调用示例

### 4.1 解码器分辨率变化处理

```cpp
// 注册回调处理分辨率变化
void OnErrorCallback(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    if (errorCode == AV_ERR_STREAM_CHANGED) {
        // 分辨率发生变化，重新获取格式信息
        OH_AVFormat *format = OH_AVCodec_GetOutputFormat(codec);
        
        VideoInfo *info = static_cast<VideoInfo*>(userData);
        
        // 获取新的有效画面尺寸（推荐使用 PIC_WIDTH/HEIGHT）
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_PIC_WIDTH, &info->videoWidth);
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_PIC_HEIGHT, &info->videoHeight);
        
        // 获取缓冲区布局信息
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_STRIDE, &info->videoStrideWidth);
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_SLICE_HEIGHT, &info->videoSliceHeight);
        
        // 如需精确渲染，获取裁剪信息
        int32_t cropLeft, cropTop, cropRight, cropBottom;
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_CROP_LEFT, &cropLeft);
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_CROP_TOP, &cropTop);
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_CROP_RIGHT, &cropRight);
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_CROP_BOTTOM, &cropBottom);
        
        // 计算实际显示尺寸
        info->displayWidth = cropRight - cropLeft + 1;
        info->displayHeight = cropBottom - cropTop + 1;
        
        // 重新分配渲染缓冲区等...
        
        OH_AVFormat_Destroy(format);
    }
}
```

### 4.2 编码器输入缓冲区准备

```cpp
// 配置编码器后，准备输入缓冲区
int32_t PrepareEncoderInput(OH_AVCodec *encoder, VideoInputBuffer *bufInfo)
{
    // 获取编码器输入格式
    OH_AVFormat *format = OH_AVCodec_GetInputFormat(encoder);
    
    // 获取 stride 和 sliceHeight，用于计算缓冲区大小
    int32_t stride = 0;
    int32_t sliceHeight = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_STRIDE, &stride);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_SLICE_HEIGHT, &sliceHeight);
    
    // 获取编码分辨率
    int32_t width = 0;
    int32_t height = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_WIDTH, &width);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_HEIGHT, &height);
    
    // 对于 NV12 格式，计算缓冲区大小
    // Y 平面: stride × sliceHeight
    // UV 平面: stride × sliceHeight / 2
    bufInfo->bufferSize = stride * sliceHeight * 3 / 2;
    bufInfo->stride = stride;
    bufInfo->sliceHeight = sliceHeight;
    bufInfo->width = width;
    bufInfo->height = height;
    
    // 分配缓冲区
    bufInfo->buffer = malloc(bufInfo->bufferSize);
    
    OH_AVFormat_Destroy(format);
    return 0;
}
```

### 4.3 编码器预处理裁剪配置

```cpp
// 配置编码器预处理裁剪
int32_t ConfigureEncoderPreprocessCrop(OH_Preprocessor *preprocessor, 
                                        int32_t inputWidth, int32_t inputHeight,
                                        int32_t cropLeft, int32_t cropTop,
                                        int32_t cropRight, int32_t cropBottom)
{
    // 验证裁剪参数
    if (cropLeft < 0 || cropTop < 0 || 
        cropRight >= inputWidth || cropBottom >= inputHeight ||
        cropLeft >= cropRight || cropTop >= cropBottom) {
        return -1;  // 参数无效
    }
    
    OH_AVFormat *format = OH_AVFormat_Create();
    
    // 设置输入格式
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_WIDTH, inputWidth);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_HEIGHT, inputHeight);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_GRAPHIC_PIXEL_FORMAT,
                            static_cast<int32_t>(GraphicPixelFormat::NV12));
    
    // 设置裁剪参数（必须四个值一起设置）
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, cropLeft);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, cropTop);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, cropRight);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, cropBottom);
    
    int32_t ret = OH_Preprocessor_Configure(preprocessor, format);
    
    OH_AVFormat_Destroy(format);
    return ret;
}
```

---

## 5. 常见疑问解答

### Q1: WIDTH/HEIGHT 和 PIC_WIDTH/PIC_HEIGHT 有什么区别？我该用哪个？

**A**:
- **WIDTH/HEIGHT**：是配置参数，表示你**期望**编解码器处理的分辨率
- **PIC_WIDTH/PIC_HEIGHT**：是输出参数，表示编解码器**实际**输出的有效画面尺寸

**使用建议**：
- Configure 时：设置 `WIDTH/HEIGHT`
- GetOutputFormat 时：读取 `PIC_WIDTH/PIC_HEIGHT`（而非 WIDTH/HEIGHT，因为后者已废弃）

---

### Q2: 为什么 Stride 不等于 Width？

**A**: Stride 是内存中的行跨距，由于硬件对齐要求，实际分配的内存宽度通常大于视频的逻辑宽度。

例如：
- 视频宽度 = 1920，NV12 格式
- 若硬件要求 32 字节对齐，Stride 可能 = 1920（已对齐）或 1952（向上对齐）

**获取正确 Stride 的方式**：通过 `GetInputFormat` 或 `GetOutputFormat` 获取，不要自行计算。

---

### Q3: 如何计算 YUV 420 格式的缓冲区大小？

**A**: 使用 Stride 和 SliceHeight 计算：

```
缓冲区大小 = Stride × SliceHeight × 3 / 2
```

分解：
- Y 平面大小 = Stride × SliceHeight
- UV 平面大小 = Stride × SliceHeight / 2

**注意**：不要使用 Width × Height 计算，因为实际分配的缓冲区可能更大。

---

### Q4: Crop 的 right/bottom 为什么是包含边界的？

**A**: 这是 API 设计约定，right 和 bottom 坐标是包含在内的（inclusive）。

计算裁剪尺寸时需要加 1：
```
cropWidth = right - left + 1
cropHeight = bottom - top + 1
```

例如：left=0, right=1919 表示宽度为 1920 像素（0 到 1919 共 1920 个像素）。

---

### Q5: 解码器 Crop 参数和编码器预处理 Crop 参数有什么区别？

**A**:

| 特性 | 解码器 Crop | 编码器预处理 Crop |
|------|-------------|-------------------|
| Key 前缀 | `VIDEO_CROP_*` | `VIDEO_ENCODER_PREPROC_CROP_*` |
| 方向 | 输出参数（解码器返回） | 输入参数（调用者设置） |
| 用途 | 指示有效画面位置 | 指定仅编码特定区域 |
| 设置时机 | 无需设置，自动获取 | Configure 时配置 |

---

### Q6: 什么时候需要重新获取 Stride 和 SliceHeight？

**A**: 以下情况需要重新获取：

1. **解码器分辨率变化**：收到 `AV_ERR_STREAM_CHANGED` 事件时
2. **首次启动后**：解码器/编码器启动后首次获取格式信息时
3. **重新 Configure 后**：重新配置编解码器参数后

---

### Q7: 编码器预处理裁剪参数可以单独设置吗？

**A**: **不可以**。left、top、right、bottom 四个参数必须同时设置，否则验证会失败。

```cpp
// 错误：只设置部分参数
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, 100);
// 结果：验证失败，返回 AVCS_ERR_INVALID_VAL

// 正确：四个参数一起设置
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, 100);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, 50);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, 1700);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, 950);
```

---

### Q8: 如何禁用编码器预处理裁剪？

**A**: 将 left、top、right、bottom 全部设置为 0，或不设置这些参数。

```cpp
// 方式一：不设置裁剪参数（默认禁用）
OH_AVFormat *format = OH_AVFormat_Create();
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_WIDTH, 1920);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_HEIGHT, 1080);
// 不设置 CROP_* 参数，裁剪默认禁用

// 方式二：显式设置为 0
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, 0);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, 0);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, 0);
OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, 0);
```
