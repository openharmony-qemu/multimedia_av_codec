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
#include "libavutil/mem.h"
#include "libswscale/swscale.h"
}
#include "avcodec_errors.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
// fcodec 内部 RAII 转换器：直接持有 SwsContext 与输出缓冲，析构统一释放，仅 fcodec 使用。
class FCodecFrameConverter {
public:
    ~FCodecFrameConverter();
    int32_t Init(int32_t srcWidth, int32_t srcHeight, AVPixelFormat srcFmt, int32_t dstWidth, int32_t dstHeight,
                 AVPixelFormat dstFmt);
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
    std::shared_ptr<SwsContext> swsCtx_ = nullptr;
    int32_t srcHeight_ = 0;
    uint8_t *data_[AV_NUM_DATA_POINTERS] = {nullptr};
    int32_t lineSize_[AV_NUM_DATA_POINTERS] = {0};
    bool isAllocated_ = false;
};
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FCODEC_FRAME_CONVERTER_H
