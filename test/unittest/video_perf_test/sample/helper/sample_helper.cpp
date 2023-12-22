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

#include "sample_helper.h"
#include <unordered_map>
#include "video_perf_test_sample_base.h"
#include "video_decoder_perf_test_sample.h"
#include "video_encoder_perf_test_sample.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SampleHelper"};

const std::unordered_map<OHOS::MediaAVCodec::Sample::CodecRunMode, std::string> RUN_MODE_TO_STRING = {
    {OHOS::MediaAVCodec::Sample::CodecRunMode::BUFFER_AVBUFFER, "Buffer AVBuffer"},  
    {OHOS::MediaAVCodec::Sample::CodecRunMode::BUFFER_SHARED_MEMORY, "Buffer SharedMemory"},  
    {OHOS::MediaAVCodec::Sample::CodecRunMode::SURFACE_ORIGIN, "Surface Origin"},  
    {OHOS::MediaAVCodec::Sample::CodecRunMode::SURFACE_AVBUFFER, "Surface AVBuffer"},  
};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
int32_t RunSample(const SampleInfo &sampleInfo)
{
    AVCODEC_LOGI("====== Video sample config ======");
    AVCODEC_LOGI("codec run mode: %{public}s", RUN_MODE_TO_STRING.at(sampleInfo.codecRunMode).c_str());
    AVCODEC_LOGI("input file: %{public}s", sampleInfo.inputFilePath.c_str());
    AVCODEC_LOGI("mime: %{public}s, %{public}d*%{public}d, %{public}.1ffps, %{public}.2fMbps, interval: %{public}dms",
        sampleInfo.codecMime.c_str(), sampleInfo.videoWidth, sampleInfo.videoHeight, sampleInfo.frameRate,
        static_cast<double>(sampleInfo.bitrate) / 1024 / 1024, sampleInfo.frameInterval); // 1024: precision
    AVCODEC_LOGI("====== Video sample config ======");

    std::unique_ptr<VideoPerfTestSampleBase> sample = sampleInfo.codecType == CodecType::VIDEO_DECODER ?
        static_cast<std::unique_ptr<VideoPerfTestSampleBase>>(std::make_unique<VideoDecoderPerfTestSample>()) :
        static_cast<std::unique_ptr<VideoPerfTestSampleBase>>(std::make_unique<VideoEncoderPerfTestSample>());

    int32_t ret = sample->Create(sampleInfo);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Create failed");
    ret = sample->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Start failed");
    ret = sample->WaitForDone();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Wait for done failed");
    return AVCODEC_SAMPLE_ERR_OK;
}
} // Sample
} // MediaAVCodec
} // OHOS