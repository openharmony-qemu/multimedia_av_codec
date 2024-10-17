/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include "base64_utils.h"
#include "common/log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_STREAM_SOURCE, "HiStreamer" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
constexpr uint32_t BASE64_DATA_MULTIPLE = 4;
constexpr uint32_t BASE64_BASE_UNIT_OF_CONVERSION = 3;
constexpr uint8_t MAX_BASE64_DECODE_NUM = 128;
/**
 * base64 decoding table
 */
static const uint8_t BASE64_DECODE_TABLE[] = {
        // 16 per row
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 1 - 16
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 17 - 32
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 63,  // 33 - 48
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0,  // 49 - 64
        0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,  // 65 - 80
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0,  // 81 - 96
        0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  // 97 - 112
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0  // 113 - 128
};

/**
 * @brief Base64Decode base64 decoding
 * @param src data to be decoded
 * @param srcSize The size of the data to be decoded
 * @param dest decoded output data
 * @param destSize The size of the output data after decoding
 * @return bool true: success false: invalid parameter
 * Note: The size of the decoded data must be greater than 4 and be a multiple of 4
 */
bool Base64Utils::Base64Decode(const uint8_t *src, uint32_t srcSize, uint8_t *dest, uint32_t *destSize)
{
    if ((src == nullptr) || (srcSize == 0) || (dest == nullptr) || (destSize == nullptr) || (srcSize > *destSize)) {
        MEDIA_LOG_E("parameter is err");
        return false;
    }
    if ((srcSize < BASE64_DATA_MULTIPLE) || (srcSize % BASE64_DATA_MULTIPLE != 0)) {
        MEDIA_LOG_E("srcSize is err");
        return false;
    }

    uint32_t i;
    uint32_t j;
    // Calculate decoded string length
    uint32_t len = srcSize / BASE64_DATA_MULTIPLE * BASE64_BASE_UNIT_OF_CONVERSION;
    if (src[srcSize - 1] == '=') { // 1:last one
        len--;
    }
    if (src[srcSize - 2] == '=') { // 2:second to last
        len--;
    }

    for (i = 0, j = 0; i < srcSize;
         i += BASE64_DATA_MULTIPLE, j += BASE64_BASE_UNIT_OF_CONVERSION) {
        if (src[i] >= MAX_BASE64_DECODE_NUM || src[i + 1] >= MAX_BASE64_DECODE_NUM ||
            src[i + 2] >= MAX_BASE64_DECODE_NUM || src[i + 3] >= MAX_BASE64_DECODE_NUM) { // 2&3 src index offset
            MEDIA_LOG_E("src data is err");
            return false;
        }
        dest[j] = (BASE64_DECODE_TABLE[src[i]] << 2) | (BASE64_DECODE_TABLE[src[i + 1]] >> 4); // 2&4bits move
        dest[j + 1] = (BASE64_DECODE_TABLE[src[i + 1]] << 4) | // 4:4bits moved
                      (BASE64_DECODE_TABLE[src[i + 2]] >> 2); // 2:index 2:2bits moved
        dest[j + 2] = (BASE64_DECODE_TABLE[src[i + 2]] << 6) | // 2:index 6:6bits moved
                      (BASE64_DECODE_TABLE[src[i + 3]]); // 3:index
    }
    *destSize = len;
    return true;
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS