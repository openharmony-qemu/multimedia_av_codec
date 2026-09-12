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

#ifndef FCODEC_FRAME_CONVERTER_H
#define FCODEC_FRAME_CONVERTER_H

#include <cstdint>
#include <memory>
extern "C" {
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
class FCodecFrameConverter {
public:
    FCodecFrameConverter() = default;
    FCodecFrameConverter(const FCodecFrameConverter &) = delete;
    FCodecFrameConverter &operator=(const FCodecFrameConverter &) = delete;
    ~FCodecFrameConverter();
    int32_t Init(int32_t w, int32_t h, AVPixelFormat srcFmt, AVPixelFormat dstFmt);
    int32_t Prepare(int32_t w, int32_t h, AVPixelFormat srcFmt, AVPixelFormat dstFmt);
    int32_t Convert(uint8_t **srcData, const int32_t *srcLineSize);

    uint8_t **GetData()
    {
        return data_;
    }

    int32_t *GetLineSize()
    {
        return lineSize_;
    }

private:
    int32_t width_ = 0;
    int32_t height_ = 0;
    AVPixelFormat srcFmt_ = AVPixelFormat::AV_PIX_FMT_NONE;
    AVPixelFormat dstFmt_ = AVPixelFormat::AV_PIX_FMT_NONE;
    bool isAllocated_ = false;
    uint8_t *data_[AV_NUM_DATA_POINTERS] = {nullptr};
    int32_t lineSize_[AV_NUM_DATA_POINTERS] = {0};
    std::shared_ptr<SwsContext> swsCtx_ = nullptr;
};
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FCODEC_FRAME_CONVERTER_H
