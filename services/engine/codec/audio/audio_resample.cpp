/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "audio_resample.h"
#include "avcodec_log.h"
#include "securec.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AvCodec-AudioResample"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
int32_t AudioResample::Init(const ResamplePara& resamplePara)
{
    resamplePara_ = resamplePara;
    if (resamplePara_.bitsPerSample != 8 && resamplePara_.bitsPerSample != 24) { // 8 24
        auto destFrameSize = av_samples_get_buffer_size(nullptr, resamplePara_.channels,
                                                        resamplePara_.destSamplesPerFrame, resamplePara_.destFmt, 0);
        resampleCache_.reserve(destFrameSize);
        resampleChannelAddr_.reserve(resamplePara_.channels);
        auto tmp = resampleChannelAddr_.data();
        av_samples_fill_arrays(tmp, nullptr, resampleCache_.data(), resamplePara_.channels,
                               resamplePara_.destSamplesPerFrame, resamplePara_.destFmt, 0);
        auto swrContext = swr_alloc();
        if (swrContext == nullptr) {
            AVCODEC_LOGE("cannot allocate swr context");
            return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
        }
        swrContext = swr_alloc_set_opts(swrContext, resamplePara_.channelLayout, resamplePara_.destFmt,
                                        resamplePara_.sampleRate, resamplePara_.channelLayout,
                                        resamplePara_.srcFmt, resamplePara_.sampleRate, 0, nullptr);
        if (swr_init(swrContext) != 0) {
            AVCODEC_LOGE("swr init error");
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
        swrCtx_ = std::shared_ptr<SwrContext>(swrContext, [](SwrContext *ptr) {
            if (ptr) {
                swr_free(&ptr);
            }
        });
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioResample::Convert(const uint8_t* srcBuffer, const size_t srcLength, uint8_t*& destBuffer,
                               size_t& destLength)
{
    size_t lineSize = srcLength / resamplePara_.channels;
    std::vector<const uint8_t*> tmpInput(resamplePara_.channels);
    tmpInput[0] = srcBuffer;
    if (av_sample_fmt_is_planar(resamplePara_.srcFmt)) {
        for (size_t i = 1; i < tmpInput.size(); ++i) {
            tmpInput[i] = tmpInput[i-1] + lineSize;
        }
    }
    auto samples = lineSize / av_get_bytes_per_sample(resamplePara_.srcFmt);
    auto res = swr_convert(swrCtx_.get(), resampleChannelAddr_.data(), resamplePara_.destSamplesPerFrame,
                           tmpInput.data(), samples);
    if (res < 0) {
        AVCODEC_LOGE("resample input failed");
        destLength = 0;
    } else {
        destBuffer = resampleCache_.data();
        destLength = res * av_get_bytes_per_sample(resamplePara_.destFmt) * resamplePara_.channels;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS
