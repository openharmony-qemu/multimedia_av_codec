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
#include <iostream>
#include <unordered_map>
#include <unistd.h>
#include "sample_base.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"
#include "syspara/parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "SampleHelper"};
constexpr std::string_view DEVICE_SAMPLE_RUN_TIMES_SYS_PARAM_KEY = "OHOS.Media.AVCodecSample.DeviceSampleRunTimes";
constexpr int32_t MAX_PAUSE_TIME = 60;

const std::unordered_map<OHOS::MediaAVCodec::Sample::CodecType, std::string> CODEC_TYPE_TO_STRING = {
    {OHOS::MediaAVCodec::Sample::CodecType::VIDEO_HW_DECODER, "Hardware Decoder"},
    {OHOS::MediaAVCodec::Sample::CodecType::VIDEO_SW_DECODER, "Software Decoder"},
    {OHOS::MediaAVCodec::Sample::CodecType::VIDEO_HW_ENCODER, "Hardware Encoder"},
    {OHOS::MediaAVCodec::Sample::CodecType::VIDEO_SW_ENCODER, "Software Encoder"},
};

const std::unordered_map<OHOS::MediaAVCodec::Sample::CodecRunMode, std::string> RUN_MODE_TO_STRING = {
    {OHOS::MediaAVCodec::Sample::CodecRunMode::API10_SURFACE, "Surface API10"},
    {OHOS::MediaAVCodec::Sample::CodecRunMode::API10_BUFFER,  "Buffer API10"},
    {OHOS::MediaAVCodec::Sample::CodecRunMode::API11_BUFFER,  "Buffer API11"},
    {OHOS::MediaAVCodec::Sample::CodecRunMode::API11_SURFACE, "Surface API11"},
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

inline void Pause(int32_t sleepTime)
{
    CHECK_AND_RETURN(sleepTime > 0);

    if (sleepTime > MAX_PAUSE_TIME) {
        std::cout << "Press enter to continue...";
        std::cin.get();
        std::cin.clear();
    } else {
        std::cout << "Pause " << sleepTime << " seconds and continue..." << std::endl;
        sleep(sleepTime);
    }
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
    std::shared_ptr<SampleBase> sample = SampleFactory::CreateSample(info);

    Pause(info.pauseBeforeRunSample);

    int32_t ret = sample->Create(info);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Create failed");
    ret = sample->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Start failed");
    ret = sample->WaitForDone();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Wait for done failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

void PrintSampleInfo(const SampleInfo &info)
{
    int32_t deviceSampleRunTimes = system::GetIntParameter(DEVICE_SAMPLE_RUN_TIMES_SYS_PARAM_KEY.data(), 0);
    deviceSampleRunTimes++;
    (void)system::SetParameter(DEVICE_SAMPLE_RUN_TIMES_SYS_PARAM_KEY.data(), std::to_string(deviceSampleRunTimes));

    PrintProgress(info.sampleRepeatTimes, 0);

    AVCODEC_LOGI("This device has run %{public}d times.", deviceSampleRunTimes);
    AVCODEC_LOGI("====== Video sample config ======");
    AVCODEC_LOGI("codec type: %{public}s, codec run mode: %{public}s, max frames: %{public}u",
        CODEC_TYPE_TO_STRING.at(info.codecType).c_str(), RUN_MODE_TO_STRING.at(info.codecRunMode).c_str(),
        info.maxFrames);
    AVCODEC_LOGI("input file: %{public}s", info.inputFilePath.c_str());
    AVCODEC_LOGI("mime: %{public}s, %{public}d*%{public}d, %{public}.1ffps, %{public}.2fMbps, pixel format: %{public}s",
        info.codecMime.c_str(), info.videoWidth, info.videoHeight, info.frameRate,
        static_cast<double>(info.bitrate) / 1024 / 1024, // 1024: precision
        OHOS::MediaAVCodec::Sample::ToString(static_cast<OH_AVPixelFormat>(info.pixelFormat)).c_str());
    AVCODEC_LOGI("interval: %{public}dms, dump output: %{public}s",
        info.frameInterval, BOOL_TO_STRING.at(info.needDumpOutput).c_str());
    AVCODEC_LOGI("====== Video sample config ======");
}

void PrintProgress(int32_t times, int32_t frames)
{
    std::cout << "\r\033[K" << "Repeat times left: " << times << ", frames: " << frames  << " " << std::flush;
}
} // Sample
} // MediaAVCodec
} // OHOS