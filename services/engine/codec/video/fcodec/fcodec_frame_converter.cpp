/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "fcodec_frame_converter.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
namespace {
constexpr int32_t FRAME_CONVERT_ALIGN = 16;
} // namespace

FCodecFrameConverter::~FCodecFrameConverter()
{
    if (isAllocated_ && data_[0] != nullptr) {
        av_free(data_[0]);
    }
}
int32_t FCodecFrameConverter::Init(int32_t srcWidth, int32_t srcHeight, AVPixelFormat srcFmt,
                                   int32_t dstWidth, int32_t dstHeight, AVPixelFormat dstFmt)
{
    srcHeight_ = srcHeight;
    swsCtx_ = std::shared_ptr<SwsContext>(
        sws_getContext(srcWidth, srcHeight, srcFmt, dstWidth, dstHeight, dstFmt,
                       SWS_FAST_BILINEAR, nullptr, nullptr, nullptr),
        [](SwsContext *ptr) {
            if (ptr != nullptr) {
                sws_freeContext(ptr);
            }
        });
    if (swsCtx_ == nullptr) {
        return AVCS_ERR_UNKNOWN;
    }
    int32_t ret = av_image_alloc(data_, lineSize_, dstWidth, dstHeight, dstFmt, FRAME_CONVERT_ALIGN);
    isAllocated_ = (ret >= 0);
    return isAllocated_ ? AVCS_ERR_OK : AVCS_ERR_UNKNOWN;
}

int32_t FCodecFrameConverter::Convert(uint8_t **srcData, const int32_t *srcLineSize)
{
    if (swsCtx_ == nullptr || srcData == nullptr) {
        return AVCS_ERR_INVALID_VAL;
    }
    int32_t res = sws_scale(swsCtx_.get(), srcData, srcLineSize, 0, srcHeight_, data_, lineSize_);
    return res < 0 ? AVCS_ERR_UNKNOWN : AVCS_ERR_OK;
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
