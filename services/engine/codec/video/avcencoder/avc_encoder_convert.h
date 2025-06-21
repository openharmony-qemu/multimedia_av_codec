/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AVC_ENCODER_CONVERT_H
#define AVC_ENCODER_CONVERT_H

#include <cstdint>

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {

#ifdef __cplusplus
extern "C" {
#endif

constexpr int32_t RGB_COLINC = 3;
constexpr int32_t RGBA_COLINC = 4;

typedef enum {
    RANGE_UNSPECIFIED = 0,
    RANGE_FULL,
    RANGE_LIMITED,
} COLOR_RANGE;

typedef enum {
    MATRIX_UNSPECIFIED = 0,
    MATRIX_BT709,
    MATRIX_FCC47_73_682,
    MATRIX_BT601,
    MATRIX_240M,
    MATRIX_BT2020,
    MATRIX_BT2020_CONSTANT,
} COLOR_MATRIX;

struct RgbImageData {
    uint8_t *data = nullptr;
    int32_t stride = 0;
    COLOR_MATRIX matrix = MATRIX_UNSPECIFIED;
    COLOR_RANGE range = RANGE_UNSPECIFIED;
    int32_t bytesPerPixel = 0;
};

struct YuvImageData {
    uint8_t *data = nullptr;
    int32_t stride = 0;
    int32_t uvOffset = 0;
};

#if defined(ARMV8)
int32_t ConvertRgbToYuv420Neon(uint8_t *dstData, int32_t width, int32_t height,
    int32_t bufferSize, RgbImageData &rgbData);
#endif

int32_t ConvertRgbToYuv420(uint8_t *dstData, int32_t width, int32_t height,
    int32_t bufferSize, RgbImageData &rgbData);

int32_t ConvertNv12ToYuv420(uint8_t *dstData, int32_t width, int32_t height,
    int32_t bufferSize, YuvImageData &yuvData);

int32_t ConvertNv21ToYuv420(uint8_t *dstData, int32_t width, int32_t height,
    int32_t bufferSize, YuvImageData &yuvData);

#ifdef __cplusplus
};
#endif
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVC_ENCODER_CONVERT_H