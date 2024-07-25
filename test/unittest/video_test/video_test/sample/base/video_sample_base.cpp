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
#include "external_window.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"
#include "sample_helper.h"

namespace {
using namespace std::string_literals;
using namespace std::chrono_literals;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoSampleBase"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
VideoSampleBase::~VideoSampleBase()
{
    StartRelease();
    if (releaseThread_ && releaseThread_->joinable()) {
        releaseThread_->join();
    }
}

int32_t VideoSampleBase::Create(SampleInfo sampleInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(context_ == nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    
    context_ = std::make_shared<SampleContext>();
    context_->sampleInfo = std::make_shared<SampleInfo>(sampleInfo);
    auto &info = *context_->sampleInfo;
    auto &videoCodec = context_->videoCodec_;

    dataProducer_ = DataProducerFactory::CreateDataProducer(info.dataProducerInfo.dataProducerType);
    CHECK_AND_RETURN_RET_LOG(dataProducer_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create data producer failed");
    int32_t ret = dataProducer_->Init(context_->sampleInfo);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Data producer init failed");
    
    videoCodec = VideoCodecFactory::CreateVideoCodec(info.codecType, info.codecRunMode);
    CHECK_AND_RETURN_RET_LOG(videoCodec != nullptr, AVCODEC_SAMPLE_ERR_ERROR,
        "Create video encoder failed, no memory");
    ret = videoCodec->Create(info.codecMime, info.codecType & 0b1);  // 0b1: software codec mask
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create video codec failed");

    ret = Init();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Init failed");
    PrintSampleInfo(info);
    
    ret = videoCodec->Config(info, reinterpret_cast<uintptr_t *>(context_.get()));
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Video codec config failed");

    releaseThread_ = nullptr;
    AVCODEC_LOGI("Succeed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoSampleBase::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    
    auto &videoCodec = context_->videoCodec_;
    CHECK_AND_RETURN_RET_LOG(videoCodec != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");

    int32_t ret = videoCodec->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Codec start failed");

    ret = StartThread();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Codec thread start failed");

    AVCODEC_LOGI("Succeed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoSampleBase::Init()
{
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoSampleBase::StartThread()
{
    return AVCODEC_SAMPLE_ERR_OK;
}

void VideoSampleBase::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (inputThread_ && inputThread_->joinable()) {
        inputThread_->join();
    }
    if (outputThread_ && outputThread_->joinable()) {
        outputThread_->join();
    }
    inputThread_ = nullptr;
    outputThread_ = nullptr;
    context_ = nullptr;
    dataProducer_ = nullptr;
    outputFile_ = nullptr;

    AVCODEC_LOGI("Succeed");
    doneCond_.notify_all();
}

void VideoSampleBase::StartRelease()
{
    if (releaseThread_ == nullptr) {
        AVCODEC_LOGI("Start to release");
        releaseThread_ = std::make_unique<std::thread>(&VideoSampleBase::Release, this);
    }
}

void VideoSampleBase::DumpOutput(const CodecBufferInfo &bufferInfo)
{
    auto &info = *context_->sampleInfo;
    CHECK_AND_RETURN(info.needDumpOutput);

    if (outputFile_ == nullptr) {
        auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (info.outputFilePath.empty()) {
            if (!(info.codecType & 0b10)) {  // 0b10: Video encoder mask
                info.outputFilePath = "VideoDecoderOut_"s + ToString(info.pixelFormat) + "_" +
                    std::to_string(info.videoWidth) + "_" + std::to_string(info.videoHeight) + "_" +
                    std::to_string(time) + ".yuv";
            } else {
                info.outputFilePath = "VideoEncoderOut_"s + std::to_string(time) + ".bin";
            }
        }
        
        outputFile_ = std::make_unique<std::ofstream>(info.outputFilePath, std::ios::out | std::ios::trunc);
        if (!outputFile_->is_open()) {
            outputFile_ = nullptr;
            AVCODEC_LOGE("Output file open failed");
            return;
        }
    }

    uint8_t *bufferAddr = nullptr;
    if (bufferInfo.bufferAddr != nullptr) {
        bufferAddr = bufferInfo.bufferAddr;
    } else {
        bufferAddr = static_cast<uint8_t>(info.codecRunMode) & 0b10 ?    // 0b10: AVBuffer mode mask
                        OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer)) :
                        OH_AVMemory_GetAddr(reinterpret_cast<OH_AVMemory *>(bufferInfo.buffer));
    }

    CHECK_AND_RETURN_LOG(bufferAddr != nullptr, "Buffer is nullptr");
    if (!(info.codecType & 0b10)) {   // 0b10: Video encoder mask
        WriteOutputFileWithStrideYUV420(bufferAddr, bufferInfo.attr.size);
    } else {
        outputFile_->write(reinterpret_cast<char *>(bufferAddr), bufferInfo.attr.size);
    }
}

void VideoSampleBase::WriteOutputFileWithStrideYUV420(uint8_t *bufferAddr, uint32_t size)
{
    CHECK_AND_RETURN_LOG(bufferAddr != nullptr, "Buffer is nullptr");
    auto &info = *context_->sampleInfo;
    constexpr int8_t yuvSampleRatio = 2;
    int32_t videoWidth = info.videoWidth *
        ((info.codecMime == OH_AVCODEC_MIMETYPE_VIDEO_HEVC && info.profile == HEVC_PROFILE_MAIN_10) ? 2 : 1);
    int32_t &videoStrideWidth = info.videoStrideWidth;

    // copy Y
    for (int32_t row = 0; row < info.videoHeight; row++) {
        outputFile_->write(reinterpret_cast<char *>(bufferAddr), videoWidth);
        bufferAddr += videoStrideWidth;
    }
    bufferAddr += (info.videoSliceHeight - info.videoHeight) * videoStrideWidth;

    // copy UV
    for (int32_t row = 0; row < (info.videoHeight / yuvSampleRatio); row++) {
        outputFile_->write(reinterpret_cast<char *>(bufferAddr), videoWidth);
        bufferAddr += videoStrideWidth;
    }
}

void VideoSampleBase::PushEosFrame()
{
    auto bufferInfoOpt = context_->inputBufferQueue.DequeueBuffer();
    CHECK_AND_RETURN(bufferInfoOpt != std::nullopt);
    auto &bufferInfo = bufferInfoOpt.value();

    bufferInfo.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;

    (void)context_->videoCodec_->PushInput(bufferInfo);
}
} // Sample
} // MediaAVCodec
} // OHOS