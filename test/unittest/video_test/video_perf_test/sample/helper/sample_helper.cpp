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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "SampleHelper"};

const std::unordered_map<OHOS::MediaAVCodec::Sample::CodecType, std::string> CODEC_TYPE_TO_STRING = {
    {OHOS::MediaAVCodec::Sample::CodecType::VIDEO_DECODER, "Decoder"},
    {OHOS::MediaAVCodec::Sample::CodecType::VIDEO_ENCODER, "Encoder"},
};

const std::unordered_map<OHOS::MediaAVCodec::Sample::CodecRunMode, std::string> RUN_MODE_TO_STRING = {
    {OHOS::MediaAVCodec::Sample::CodecRunMode::BUFFER_AVBUFFER,         "Buffer AVBuffer"},
    {OHOS::MediaAVCodec::Sample::CodecRunMode::BUFFER_SHARED_MEMORY,    "Buffer SharedMemory"},
    {OHOS::MediaAVCodec::Sample::CodecRunMode::SURFACE_ORIGIN,          "Surface Origin"},
    {OHOS::MediaAVCodec::Sample::CodecRunMode::SURFACE_AVBUFFER,        "Surface AVBuffer"},
};

const std::unordered_map<bool, std::string> BOOL_TO_STRING = {
    {false, "false"},
    {true,  "true"}
};

const std::unordered_map<OH_AVPixelFormat, std::string> PIXEL_FORMAT_TO_STRING = {
    {AV_PIXEL_FORMAT_YUVI420,           "YUVI420"},
    {AV_PIXEL_FORMAT_NV12,              "NV12"},
    {AV_PIXEL_FORMAT_NV21,              "NV21"},
    {AV_PIXEL_FORMAT_SURFACE_FORMAT,    "SURFACE_FORMAT"},
    {AV_PIXEL_FORMAT_RGBA,              "RGBA"},
};

void PrintSampleInfo(const OHOS::MediaAVCodec::Sample::SampleInfo &info)
{
    AVCODEC_LOGI("====== Video sample config ======");
    AVCODEC_LOGI("codec type: %{public}s, codec run mode: %{public}s, max frames: %{public}u",
        CODEC_TYPE_TO_STRING.at(info.codecType).c_str(), RUN_MODE_TO_STRING.at(info.codecRunMode).c_str(),
        info.maxFrames);
    AVCODEC_LOGI("input file: %{public}s", info.inputFilePath.c_str());
    AVCODEC_LOGI("mime: %{public}s, %{public}d*%{public}d, %{public}.1ffps, %{public}.2fMbps, pixel format: %{public}s",
        info.codecMime.c_str(), info.videoWidth, info.videoHeight, info.frameRate,
        static_cast<double>(info.bitrate) / 1024 / 1024, // 1024: precision
        OHOS::MediaAVCodec::Sample::ToString(static_cast<OH_AVPixelFormat>(info.pixelFormat)).c_str());
    AVCODEC_LOGI("interval: %{public}dms, HDR vivid: %{public}s, dump output: %{public}s",
        info.frameInterval, BOOL_TO_STRING.at(info.isHDRVivid).c_str(), BOOL_TO_STRING.at(info.needDumpOutput).c_str());
    AVCODEC_LOGI("====== Video sample config ======");
}
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
std::string ToString(OH_AVPixelFormat pixelFormat)
{
    std::string ret;
    auto iter = PIXEL_FORMAT_TO_STRING.find(pixelFormat);
    if (iter != PIXEL_FORMAT_TO_STRING.end()) {
        ret = PIXEL_FORMAT_TO_STRING.at(pixelFormat);
    }
    return ret;
}

int32_t RunSample(const SampleInfo &info)
{
    PrintSampleInfo(info);

    std::unique_ptr<VideoPerfTestSampleBase> sample = info.codecType == CodecType::VIDEO_DECODER ?
        static_cast<std::unique_ptr<VideoPerfTestSampleBase>>(std::make_unique<VideoDecoderPerfTestSample>()) :
        static_cast<std::unique_ptr<VideoPerfTestSampleBase>>(std::make_unique<VideoEncoderPerfTestSample>());

    int32_t ret = sample->Create(info);
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