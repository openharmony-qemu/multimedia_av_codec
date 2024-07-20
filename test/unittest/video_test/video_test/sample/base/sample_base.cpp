/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "sample_base.h"
#include "video_decoder_sample.h"
#include "video_encoder_sample.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"
#include "yuv_viewer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "SampleBase"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
int32_t SampleBase::WaitForDone()
{
    AVCODEC_LOGI("In");
    std::unique_lock<std::mutex> lock(mutex_);
    doneCond_.wait(lock);
    AVCODEC_LOGI("Done");
    return AVCODEC_SAMPLE_ERR_OK;
}

std::shared_ptr<SampleBase> SampleFactory::CreateSample(const SampleInfo &info)
{
    std::shared_ptr<SampleBase> sampleBase;

    switch (info.sampleType) {
        case SampleType::VIDEO_SAMPLE:
            sampleBase = (info.codecType & 0b10) ? // 0b10: Video encoder mask
                std::static_pointer_cast<SampleBase>(std::make_shared<VideoEncoderSample>()) :
                std::static_pointer_cast<SampleBase>(std::make_shared<VideoDecoderSample>());
            break;
        case SampleType::YUV_VIEWER:
            sampleBase = std::static_pointer_cast<SampleBase>(std::make_shared<YuvViewer>());
            break;
        default:
            AVCODEC_LOGE("Not supported sample type, type: %{public}d", info.sampleType);
            break;
    }
    return sampleBase;
}
} // Sample
} // MediaAVCodec
} // OHOS