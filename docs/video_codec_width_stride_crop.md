# 视频编解码宽高、跨距与裁剪信息说明

## 目录

- [1. 概述](#1-概述)
- [2. 基本概念](#2-基本概念)
  - [2.1 视频宽高 (Width/Height)](#21-视频宽高-widthheight)
  - [2.2 跨距 (Stride)](#22-跨距-stride)
  - [2.3 切片高度 (Slice Height)](#23-切片高度-slice-height)
  - [2.4 有效画面尺寸 (Picture Width/Height)](#24-有效画面尺寸-picture-widthheight)
  - [2.5 裁剪矩形 (Crop Rectangle)](#25-裁剪矩形-crop-rectangle)
- [3. 各尺寸之间的关系](#3-各尺寸之间的关系)
  - [3.1 内存布局示意](#31-内存布局示意)
  - [3.2 YUV 420 格式示例](#32-yuv-420-格式示例)
- [4. API 参考](#4-api-参考)
  - [4.1 解码器输出格式参数](#41-解码器输出格式参数)
  - [4.2 编码器预处理裁剪参数](#42-编码器预处理裁剪参数)
  - [4.3 数据结构定义](#43-数据结构定义)
- [5. 使用示例](#5-使用示例)
  - [5.1 获取解码输出帧的尺寸信息](#51-获取解码输出帧的尺寸信息)
  - [5.2 配置编码器裁剪参数](#52-配置编码器裁剪参数)
- [6. 注意事项](#6-注意事项)
  - [6.1 内存对齐要求](#61-内存对齐要求)
  - [6.2 裁剪参数使用限制](#62-裁剪参数使用限制)
  - [6.3 格式变化处理](#63-格式变化处理)
- [7. 常见问题](#7-常见问题)

---

## 1. 概述

在视频编解码过程中，涉及多种不同的尺寸概念，包括视频的实际宽高、内存布局的跨距、有效画面尺寸以及裁剪区域等。正确理解这些概念及其相互关系，对于视频数据的正确处理、内存管理和图像渲染至关重要。

本文档基于 OpenHarmony av_codec 部件的实现，详细说明视频编解码中宽高、跨距与裁剪的相关概念、API 接口及使用注意事项。

---

## 2. 基本概念

### 2.1 视频宽高 (Width/Height)

**定义**：视频帧的逻辑宽度和高度，即视频内容的实际分辨率。

- **Key**: `OH_MD_KEY_WIDTH` / `OH_MD_KEY_HEIGHT`
- **值类型**: `int32_t`
- **说明**: 表示视频帧的期望或实际分辨率，通常用于配置编解码器。

```c
/* Key for video width, value type is int32_t */
extern const char *OH_MD_KEY_WIDTH;
/* Key for video height, value type is int32_t */
extern const char *OH_MD_KEY_HEIGHT;
```

**使用场景**：
- 配置编码器输入分辨率
- 配置解码器输出分辨率
- 查询编解码器能力范围

### 2.2 跨距 (Stride)

**定义**：视频缓冲区布局的行跨距（行增量），表示相邻两行起始位置之间的字节差值。

- **Key**: `OH_MD_KEY_VIDEO_STRIDE`
- **值类型**: `int32_t`
- **说明**: 对于 YUV 420 格式，跨距对应于 Y 分量平面；U 和 V 平面的跨距可根据色彩格式计算得出。

```c
/**
 * @brief Key for describing the stride of the video buffer layout, value type is int32_t. 
 * Stride (or row increment) is the difference between the index of a pixel and that of 
 * the pixel directly underneath. For YUV 420 formats, the stride corresponds to the Y plane; 
 * the stride of the U and V planes can be calculated based on the color format, though it 
 * is generally undefined and depends on the device and release.
 *
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 12
 */
extern const char *OH_MD_KEY_VIDEO_STRIDE;
```

**为什么需要 Stride？**

由于硬件对齐要求，实际分配的内存宽度通常大于或等于视频的逻辑宽度。Stride 保证了每行数据在内存中的起始位置满足特定的对齐要求（如 16 字节、32 字节或 64 字节对齐）。

**计算公式**：
```
Stride >= Width × 每像素字节数
Stride 通常为对齐单位的整数倍
```

### 2.3 切片高度 (Slice Height)

**定义**：多平面 (YUV) 视频缓冲区布局的平面高度，也称为垂直跨距。

- **Key**: `OH_MD_KEY_VIDEO_SLICE_HEIGHT`
- **值类型**: `int32_t`
- **说明**: 表示从 Y 平面顶部到 U 平面顶部需要跳过的行数。U 平面的偏移量本质上为 `sliceHeight × stride`。

```c
/**
 * @brief Key for describing the plane height of a multi-planar (YUV) video buffer layout, 
 * value type is int32_t.
 * Slice height (or plane height/vertical stride) is the number of rows that must be skipped 
 * to get from the top of the Y plane to the top of the U plane in the buffer. In essence 
 * the offset of the U plane is sliceHeight * stride. The height of the U/V planes can be 
 * calculated based on the color format, though it is generally undefined and depends on 
 * the device and release.
 *
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 12
 */
extern const char *OH_MD_KEY_VIDEO_SLICE_HEIGHT;
```

**为什么需要 Slice Height？**

与 Stride 类似，Slice Height 用于满足垂直方向的内存对齐要求。对于 YUV 420 格式，U/V 平面通常从 Y 平面之后开始存储，Slice Height 决定了 Y 平面的实际占用行数。

### 2.4 有效画面尺寸 (Picture Width/Height)

**定义**：视频中有效画面的宽度和高度。

- **Key**: `OH_MD_KEY_VIDEO_PIC_WIDTH` / `OH_MD_KEY_VIDEO_PIC_HEIGHT`
- **值类型**: `int32_t`
- **说明**: 表示视频帧中实际有效的图像区域尺寸，可能与分配的缓冲区尺寸不同。

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

### 2.5 裁剪矩形 (Crop Rectangle)

**定义**：用于指定视频帧中需要显示或编码的矩形区域。

#### 解码器裁剪参数

解码器输出的裁剪参数，用于指示有效画面在缓冲区中的位置。

| Key | 说明 |
|-----|------|
| `OH_MD_KEY_VIDEO_CROP_LEFT` | 裁剪矩形最左列的 x 坐标（从 0 开始） |
| `OH_MD_KEY_VIDEO_CROP_RIGHT` | 裁剪矩形最右列的 x 坐标（从 0 开始） |
| `OH_MD_KEY_VIDEO_CROP_TOP` | 裁剪矩形最顶行的 y 坐标（从 0 开始） |
| `OH_MD_KEY_VIDEO_CROP_BOTTOM` | 裁剪矩形最底行的 y 坐标（从 0 开始） |

```c
/**
 * @brief Key for describing the left-coordinate (x) of the crop rectangle, value type is int32_t.
 * This is the left-most column included in the crop frame, where column indices start at 0.
 * @since 12
 */
extern const char *OH_MD_KEY_VIDEO_CROP_LEFT;

/**
 * @brief Key for describing the right-coordinate (x) of the crop rectangle, value type is int32_t.
 * This is the right-most column included in the crop frame, where column indices start at 0.
 * @since 12
 */
extern const char *OH_MD_KEY_VIDEO_CROP_RIGHT;

/**
 * @brief Key for describing the top-coordinate (y) of the crop rectangle, value type is int32_t.
 * This is the top-most row included in the crop frame, where row indices start at 0.
 * @since 12
 */
extern const char *OH_MD_KEY_VIDEO_CROP_TOP;

/**
 * @brief Key for describing the bottom-coordinate (y) of the crop rectangle, value type is int32_t.
 * This is the bottom-most row included in the crop frame, where row indices start at 0.
 * @since 12
 */
extern const char *OH_MD_KEY_VIDEO_CROP_BOTTOM;
```

**裁剪尺寸计算**：
```
裁剪宽度  = right - left + 1
裁剪高度 = bottom - top + 1
```

#### 编码器预处理裁剪参数

编码器输入预处理阶段的裁剪参数，用于指定仅编码输入帧的特定区域。

| Key | 说明 |
|-----|------|
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT` | 编码器预处理裁剪左边界 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT` | 编码器预处理裁剪右边界 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP` | 编码器预处理裁剪上边界 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM` | 编码器预处理裁剪下边界 |

```c
/**
 * @brief Key for describing the left-coordinate (x) of the crop rectangle in video encoder preprocess,
 * value type is int32_t.
 *
 * The value represents the left-most column included in the crop frame, where column indices start at 0.
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
extern const char *OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP;
extern const char *OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT;
extern const char *OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM;
```

---

## 3. 各尺寸之间的关系

### 3.1 内存布局示意

```
┌─────────────────────────────────────────┐
│         分配的缓冲区 (Stride × SliceHeight)        │
│                                         │
│  ┌───────────────────────────┐          │
│  │                           │          │
│  │   有效画面区域             │          │
│  │   (Crop Rect)             │          │
│  │                           │          │
│  │   宽度 = right - left + 1  │          │
│  │   高度 = bottom - top + 1  │          │
│  │                           │          │
│  └───────────────────────────┘          │
│                                         │
│  ←──────── Stride ────────→             │
│  ├──────── Width ───┤                   │
└─────────────────────────────────────────┘
         ↑                    ↑
       left                 right
```

### 3.2 YUV 420 格式示例

以 NV12 格式（YUV 420 半平面格式）为例：

```
内存布局：
┌───────────────────────────────────────┐
│ Y 平面                                 │
│ (Stride × SliceHeight 字节)            │
│                                       │
│ ┌─────────────────────┐               │
│ │ 有效 Y 数据          │               │
│ │ (Width × Height)    │               │
│ └─────────────────────┘               │
│                                       │
├───────────────────────────────────────┤
│ UV 平面 (交错存储)                      │
│ (Stride × (SliceHeight / 2) 字节)     │
│                                       │
│ ┌─────────────────────┐               │
│ │ 有效 UV 数据         │               │
│ │ (Width × Height/2)  │               │
│ └─────────────────────┘               │
└───────────────────────────────────────┘

尺寸关系：
- Stride >= Width（通常向上对齐到 16 或 32 的倍数）
- SliceHeight >= Height（通常向上对齐到 16 或 32 的倍数）
- U/V 平面宽度 = Stride / 2（对于 420 格式）
- U/V 平面高度 = SliceHeight / 2（对于 420 格式）
```

**尺寸关系总结**：

| 参数 | 说明 | 典型关系 |
|------|------|----------|
| Width | 视频逻辑宽度 | ≤ Stride |
| Height | 视频逻辑高度 | ≤ SliceHeight |
| Stride | 内存行跨距 | ≥ Width，对齐到硬件要求 |
| SliceHeight | 内存平面高度 | ≥ Height，对齐到硬件要求 |
| Crop Width | 有效画面宽度 | = right - left + 1 ≤ Width |
| Crop Height | 有效画面高度 | = bottom - top + 1 ≤ Height |

---

## 4. API 参考

### 4.1 解码器输出格式参数

解码器输出格式变更时，可通过 `OH_MD_KEY_VIDEO_*` 系列 Key 获取以下信息：

| Key | 类型 | 说明 |
|-----|------|------|
| `OH_MD_KEY_WIDTH` | int32_t | 视频宽度 |
| `OH_MD_KEY_HEIGHT` | int32_t | 视频高度 |
| `OH_MD_KEY_VIDEO_STRIDE` | int32_t | Y 平面行跨距（字节） |
| `OH_MD_KEY_VIDEO_SLICE_HEIGHT` | int32_t | Y 平面高度（行） |
| `OH_MD_KEY_VIDEO_PIC_WIDTH` | int32_t | 有效画面宽度 |
| `OH_MD_KEY_VIDEO_PIC_HEIGHT` | int32_t | 有效画面高度 |
| `OH_MD_KEY_VIDEO_CROP_LEFT` | int32_t | 裁剪左边界 |
| `OH_MD_KEY_VIDEO_CROP_RIGHT` | int32_t | 裁剪右边界 |
| `OH_MD_KEY_VIDEO_CROP_TOP` | int32_t | 裁剪上边界 |
| `OH_MD_KEY_VIDEO_CROP_BOTTOM` | int32_t | 裁剪下边界 |

### 4.2 编码器预处理裁剪参数

配置编码器输入预处理时，可使用以下 Key 设置裁剪区域：

| Key | 类型 | 说明 |
|-----|------|------|
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT` | int32_t | 裁剪左边界 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT` | int32_t | 裁剪右边界 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP` | int32_t | 裁剪上边界 |
| `OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM` | int32_t | 裁剪下边界 |

### 4.3 数据结构定义

#### ImgSize 结构体

用于表示图像尺寸：

```cpp
struct ImgSize {
    int32_t width;
    int32_t height;

    ImgSize() : width(0), height(0) {}

    ImgSize(const int32_t &width, const int32_t &height)
    {
        this->width = width;
        this->height = height;
    }

    bool operator<(const ImgSize &p) const
    {
        return (width < p.width) || (width == p.width && height < p.height);
    }
};
```

#### CropRect 结构体

用于表示裁剪矩形区域：

```cpp
struct CropRect {
    int32_t top{0};
    int32_t left{0};
    int32_t bottom{0};
    int32_t right{0};
};
```

#### CropConfig 结构体

用于编码器预处理裁剪配置：

```cpp
struct CropConfig {
    int32_t left{0};
    int32_t right{0};
    int32_t top{0};
    int32_t bottom{0};
    int32_t width{0};   // = right - left + 1
    int32_t height{0};  // = bottom - top + 1
    bool enabled{false};
    
    explicit CropConfig(const CropParams &params)
        : left(params.left),
          right(params.right),
          top(params.top),
          bottom(params.bottom),
          width(params.right - params.left + 1),
          height(params.bottom - params.top + 1),
          enabled(params.result == ConfigResult::ENABLED)
    {}
};
```

---

## 5. 使用示例

### 5.1 获取解码输出帧的尺寸信息

```cpp
// 获取解码器输出格式信息
Format format;
int32_t ret = decoder->GetOutputFormat(format);
if (ret != AVCS_ERR_OK) {
    // 处理错误
    return;
}

// 获取视频宽高
int32_t width = 0;
int32_t height = 0;
format.GetIntValue(OH_MD_KEY_WIDTH, width);
format.GetIntValue(OH_MD_KEY_HEIGHT, height);

// 获取跨距和切片高度
int32_t stride = 0;
int32_t sliceHeight = 0;
format.GetIntValue(OH_MD_KEY_VIDEO_STRIDE, stride);
format.GetIntValue(OH_MD_KEY_VIDEO_SLICE_HEIGHT, sliceHeight);

// 获取有效画面尺寸
int32_t picWidth = 0;
int32_t picHeight = 0;
format.GetIntValue(OH_MD_KEY_VIDEO_PIC_WIDTH, picWidth);
format.GetIntValue(OH_MD_KEY_VIDEO_PIC_HEIGHT, picHeight);

// 获取裁剪区域
int32_t cropLeft = 0, cropRight = 0, cropTop = 0, cropBottom = 0;
format.GetIntValue(OH_MD_KEY_VIDEO_CROP_LEFT, cropLeft);
format.GetIntValue(OH_MD_KEY_VIDEO_CROP_RIGHT, cropRight);
format.GetIntValue(OH_MD_KEY_VIDEO_CROP_TOP, cropTop);
format.GetIntValue(OH_MD_KEY_VIDEO_CROP_BOTTOM, cropBottom);

// 计算裁剪后的实际尺寸
int32_t cropWidth = cropRight - cropLeft + 1;
int32_t cropHeight = cropBottom - cropTop + 1;

printf("Video Resolution: %dx%d\n", width, height);
printf("Buffer Layout: stride=%d, sliceHeight=%d\n", stride, sliceHeight);
printf("Valid Picture: %dx%d\n", picWidth, picHeight);
printf("Crop Rect: [%d, %d, %d, %d] -> %dx%d\n", 
       cropLeft, cropTop, cropRight, cropBottom, cropWidth, cropHeight);
```

### 5.2 配置编码器裁剪参数

```cpp
// 创建编码器配置格式
Format format;

// 设置基本编码参数
format.PutIntValue(OH_MD_KEY_WIDTH, 1920);
format.PutIntValue(OH_MD_KEY_HEIGHT, 1080);
format.PutStringValue(OH_MD_KEY_MIME, "video/avc");

// 配置编码器预处理裁剪区域
// 裁剪区域: 从 (100, 50) 到 (1700, 950)
// 裁剪后尺寸: 1601 x 901
format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, 100);
format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, 50);
format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, 1700);
format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, 950);

// 配置编码器
int32_t ret = encoder->Configure(format, surface);
if (ret != AVCS_ERR_OK) {
    // 处理配置错误
    return;
}

// 启用裁剪后，编码器将仅编码裁剪区域内的数据
```

---

## 6. 注意事项

### 6.1 内存对齐要求

1. **Stride 对齐**：实际分配的 Stride 通常大于或等于视频宽度，并向上对齐到硬件要求的边界（常见的对齐单位有 16、32、64 字节）。

2. **SliceHeight 对齐**：与 Stride 类似，SliceHeight 也会向上对齐以满足硬件垂直对齐要求。

3. **RGB 格式特殊处理**：对于 RGBA 格式，Stride 的计算方式为：
   ```cpp
   // RGBA 格式，每个像素 4 字节
   stride = width * 4;  // RGBA_COLINC = 4
   ```

4. **YUV 420 格式**：U/V 平面的 Stride 通常为 Y 平面 Stride 的一半。

### 6.2 裁剪参数使用限制

1. **必须完整配置**：裁剪的 left、top、right、bottom 四个参数必须同时配置，不可单独设置。

2. **禁用裁剪**：当 left、top、right、bottom 全部为 0 时，裁剪功能处于禁用状态。

3. **边界验证**：裁剪参数必须满足以下条件：
   ```
   0 <= left < right <= encoderConfig.width
   0 <= top < bottom <= encoderConfig.height
   ```

4. **与降采样互斥**：裁剪参数不能与降采样 (downsampling) 参数同时使用。

5. **默认裁剪值**：解码器在未显式设置裁剪参数时，默认值为：
   ```cpp
   cropLeft = 0;
   cropTop = 0;
   cropRight = width - 1;
   cropBottom = height - 1;
   ```

### 6.3 格式变化处理

1. **动态格式变化**：视频流可能在运行过程中发生分辨率变化，此时需要重新获取 Stride、SliceHeight 和裁剪参数。

2. **格式变化检测**：解码器在检测到格式变化时，会更新相关参数：
   ```cpp
   if (formatChanged) {
       // 更新 stride
       int32_t stride = GetSurfaceBufferStride(buffer);
       format.PutIntValue(OH_MD_KEY_VIDEO_STRIDE, stride);
       
       // 更新宽高
       format.PutIntValue(OH_MD_KEY_WIDTH, width_);
       format.PutIntValue(OH_MD_KEY_HEIGHT, height_);
       
       // 更新 slice height
       format.PutIntValue(OH_MD_KEY_VIDEO_SLICE_HEIGHT, height_);
       
       // 更新有效画面尺寸
       format.PutIntValue(OH_MD_KEY_VIDEO_PIC_WIDTH, width_);
       format.PutIntValue(OH_MD_KEY_VIDEO_PIC_HEIGHT, height_);
   }
   ```

3. **缓冲区重新分配**：格式变化可能导致需要重新分配输出缓冲区。

---

## 7. 常见问题

### Q1: 为什么 Stride 不等于 Width？

**A**: Stride 是内存中的行跨距，由于硬件对齐要求（如 16/32/64 字节对齐），实际分配的内存宽度通常大于视频的逻辑宽度。这样可以提高内存访问效率，满足 GPU、DSP 等硬件模块的对齐要求。

### Q2: 如何计算 YUV 420 格式的实际缓冲区大小？

**A**: 对于 NV12 等 YUV 420 格式：
```
缓冲区大小 = Stride × SliceHeight × 3 / 2
```
其中：
- Y 平面大小 = Stride × SliceHeight
- UV 平面大小 = Stride × SliceHeight / 2

### Q3: Crop Right/Bottom 为什么是包含边界的？

**A**: 裁剪矩形的 right 和 bottom 坐标是包含在内的（inclusive），即有效像素范围是 `[left, right]` 和 `[top, bottom]`。因此裁剪宽度/高度的计算需要加 1：
```
cropWidth = right - left + 1
cropHeight = bottom - top + 1
```

### Q4: 解码器输出的 Crop 参数有什么作用？

**A**: 某些视频编码标准（如 H.264/HEVC）允许在比特流中指定裁剪信息，用于去除编码时添加的填充像素。解码器会将这些裁剪信息通过 `OH_MD_KEY_VIDEO_CROP_*` 参数输出，渲染时应仅显示裁剪区域内的内容。

### Q5: 编码器预处理裁剪与解码器裁剪有什么区别？

**A**: 
- **解码器裁剪**：是输出参数，表示解码后有效画面的区域，由解码器根据比特流信息自动设置。
- **编码器预处理裁剪**：是输入参数，由调用者配置，用于指定仅编码输入帧的特定区域，裁剪后的区域将作为编码器的有效输入。

### Q6: 何时需要重新获取 Stride 和 SliceHeight？

**A**: 以下情况需要重新获取：
1. 解码器输出格式发生变化（分辨率切换）
2. 重新配置编解码器后
3. 首次获取输出格式时

---

## 附录：相关头文件

| 文件路径 | 说明 |
|----------|------|
| `interfaces/kits/c/native_avcodec_base.h` | 公共 API 定义，包含所有 Key 的声明 |
| `interfaces/inner_api/native/avcodec_info.h` | 内部 API，包含 ImgSize 等结构体定义 |
| `frameworks/native/capi/avcodec/preprocessor.h` | 预处理器相关结构体定义 |
| `frameworks/native/avcodec/pre_processing/fast_kits_interface/fast_kits_interface.h` | CropRect 结构体定义 |
