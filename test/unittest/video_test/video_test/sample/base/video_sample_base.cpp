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

#include "video_sample_base.h"
#include <chrono>
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"
#include "sample_helper.h"
#include "video_decoder_sample.h"
#include "video_encoder_sample.h"

namespace {
using namespace std::string_literals;
using namespace std::chrono_literals;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoSampleBase"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
std::shared_ptr<VideoSampleBase> VideoSampleFactory::CreateVideoSample(CodecType type)
{
    return (type & 0b10) ? // 0b10: Video encoder mask
        std::static_pointer_cast<VideoSampleBase>(std::make_shared<VideoEncoderSample>()) :
        std::static_pointer_cast<VideoSampleBase>(std::make_shared<VideoDecoderSample>());
}

VideoSampleBase::~VideoSampleBase()
{
    StartRelease();
    if (releaseThread_ && releaseThread_->joinable()) {
        releaseThread_->join();
    }
}

int32_t VideoSampleBase::WaitForDone()
{
    AVCODEC_LOGI("In");
    std::unique_lock<std::mutex> lock(mutex_);
    doneCond_.wait(lock);
    AVCODEC_LOGI("Done");
    return AVCODEC_SAMPLE_ERR_OK;
}

void VideoSampleBase::Release()
{
}

void VideoSampleBase::StartRelease()
{
    if (releaseThread_ == nullptr) {
        AVCODEC_LOGI("Start to release");
        releaseThread_ = std::make_unique<std::thread>(&VideoSampleBase::Release, this);
    }
}

void VideoSampleBase::ThreadSleep()
{
    if (sampleInfo_.frameInterval <= 0) {
        return;
    }

    thread_local auto lastPushTime = std::chrono::system_clock::now();
    auto beforeSleepTime = std::chrono::system_clock::now();
    std::this_thread::sleep_until(lastPushTime + std::chrono::milliseconds(sampleInfo_.frameInterval));
    lastPushTime = std::chrono::system_clock::now();

    AVCODEC_LOGV("Sleep time: %{public}2.2fms",
        static_cast<std::chrono::duration<double, std::milli>>(lastPushTime - beforeSleepTime).count());
}

void VideoSampleBase::DumpOutput(const CodecBufferInfo &bufferInfo)
{
    if (!sampleInfo_.needDumpOutput) {
        return;
    }

    if (outputFile_ == nullptr) {
        auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (sampleInfo_.outputFilePath.empty()) {
            if (sampleInfo_.codecType & 0b10) {  // 0b10: Video decoder mask
                sampleInfo_.outputFilePath = "VideoDecoderOut_"s + ToString(sampleInfo_.pixelFormat) + "_" +
                    std::to_string(sampleInfo_.videoWidth) + "_" + std::to_string(sampleInfo_.videoHeight) + "_" +
                    std::to_string(time) + ".yuv";
            } else {
                sampleInfo_.outputFilePath = "VideoEncoderOut_"s +
                    sampleInfo_.codecMime + "_" + std::to_string(time) + ".bin";
            }
        }
        
        outputFile_ = std::make_unique<std::ofstream>(sampleInfo_.outputFilePath, std::ios::out | std::ios::trunc);
        if (!outputFile_->is_open()) {
            outputFile_ = nullptr;
        }
    }

    uint8_t *bufferAddr = nullptr;
    if (bufferInfo.bufferAddr != nullptr) {
        bufferAddr = bufferInfo.bufferAddr;
    } else {
        bufferAddr = static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b10 ?    // 0b10: AVBuffer mode mask
                        OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer)) :
                        OH_AVMemory_GetAddr(reinterpret_cast<OH_AVMemory *>(bufferInfo.buffer));
    }

    outputFile_->write(reinterpret_cast<char *>(bufferAddr), bufferInfo.attr.size);
}
} // Sample
} // MediaAVCodec
} // OHOS