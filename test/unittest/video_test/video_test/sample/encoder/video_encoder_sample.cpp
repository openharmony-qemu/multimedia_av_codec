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

#include "video_encoder_sample.h"
#include <unistd.h>
#include <chrono>
#include <memory>
#include <sys/mman.h>
#include "external_window.h"
#include "native_buffer_inner.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"
#include "avcodec_trace.h"

namespace {
using namespace std::string_literals;
using namespace std::chrono_literals;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoEncoderSample"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
VideoEncoderSample::~VideoEncoderSample()
{
    StartRelease();
    if (releaseThread_ && releaseThread_->joinable()) {
        releaseThread_->join();
    }
}

int32_t VideoEncoderSample::Create(SampleInfo sampleInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(videoEncoder_ == nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    
    sampleInfo_ = sampleInfo;
    
    videoEncoder_ = std::make_unique<VideoEncoder>();
    CHECK_AND_RETURN_RET_LOG(videoEncoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR,
        "Create video encoder failed, no memory");

    int32_t ret = videoEncoder_->Create(sampleInfo_.codecMime);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create video encoder failed");

    dataProducer_ = DataProducerFactory::CreateDataProducer(sampleInfo_.dataProducerInfo);
    CHECK_AND_RETURN_RET_LOG(dataProducer_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create data producer failed");
    
    context_ = new CodecUserData;
    context_->sampleInfo = &sampleInfo_;
    ret = videoEncoder_->Config(sampleInfo_, context_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Encoder config failed");

    releaseThread_ = nullptr;
    AVCODEC_LOGI("Succeed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderSample::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(videoEncoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");

    int32_t ret = dataProducer_->Init(sampleInfo_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Data producer init failed");

    ret = videoEncoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Encoder start failed");

    inputThread_ = (static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b01) ?  // 0b01: Buffer mode mask
        std::make_unique<std::thread>(&VideoEncoderSample::BufferInputThread, this) :
        std::make_unique<std::thread>(&VideoEncoderSample::SurfaceInputThread, this);
    outputThread_ = std::make_unique<std::thread>(&VideoEncoderSample::OutputThread, this);
    if (inputThread_ == nullptr || outputThread_ == nullptr) {
        AVCODEC_LOGE("Create thread failed");
        StartRelease();
        return AVCODEC_SAMPLE_ERR_ERROR;
    }

    AVCODEC_LOGI("Succeed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderSample::WaitForDone()
{
    AVCODEC_LOGI("In");
    std::unique_lock<std::mutex> lock(mutex_);
    doneCond_.wait(lock);
    AVCODEC_LOGI("Done");
    return AVCODEC_SAMPLE_ERR_OK;
}

void VideoEncoderSample::StartRelease()
{
    if (releaseThread_ == nullptr) {
        AVCODEC_LOGI("Start release VideoEncoderSample");
        releaseThread_ = std::make_unique<std::thread>(&VideoEncoderSample::Release, this);
    }
}

void VideoEncoderSample::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (inputThread_ && inputThread_->joinable()) {
        inputThread_->join();
    }
    if (outputThread_ && outputThread_->joinable()) {
        outputThread_->join();
    }
    if (videoEncoder_ != nullptr) {
        videoEncoder_->Release();
    }
    inputThread_.reset();
    outputThread_.reset();
    videoEncoder_.reset();

    if (sampleInfo_.window != nullptr) {
        OH_NativeWindow_DestroyNativeWindow(sampleInfo_.window);
        sampleInfo_.window = nullptr;
    }
    if (context_ != nullptr) {
        delete context_;
        context_ = nullptr;
    }
    if (dataProducer_ != nullptr) {
        dataProducer_.reset();
    }
    if (outputFile_ != nullptr) {
        outputFile_.reset();
    }

    AVCODEC_LOGI("Succeed");
    doneCond_.notify_all();
}

void VideoEncoderSample::BufferInputThread()
{
    OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("SampleWorkTime", FAKE_POINTER(this));
    while (true) {
        std::unique_lock<std::mutex> lock(context_->inputMutex_);
        bool condRet = context_->inputCond_.wait_for(lock, 5s,
            [this]() { return !context_->inputBufferInfoQueue_.empty(); });
        CHECK_AND_CONTINUE_LOG(!context_->inputBufferInfoQueue_.empty(),
            "Buffer queue is empty, continue, cond ret: %{public}d", condRet);

        CodecBufferInfo bufferInfo = context_->inputBufferInfoQueue_.front();
        context_->inputBufferInfoQueue_.pop();
        context_->inputFrameCount_++;
        lock.unlock();

        bufferInfo.attr.pts = context_->inputFrameCount_ *
            ((sampleInfo_.frameInterval == 0) ? 1 : sampleInfo_.frameInterval) * 1000; // 1000: 1ms to us
        
        int32_t ret = dataProducer_->ReadSample(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");

        ThreadSleep();

        ret = videoEncoder_->PushInputData(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Push data failed, thread out");
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Catch EOS, thread out");
    }
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->inputFrameCount_);
    StartRelease();
}

void VideoEncoderSample::SurfaceInputThread()
{
    OHNativeWindowBuffer *buffer = nullptr;
    OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("SampleWorkTime", FAKE_POINTER(this));
    while (true) {
        context_->inputFrameCount_++;
        uint64_t pts = context_->inputFrameCount_ *
            ((sampleInfo_.frameInterval == 0) ? 1 : sampleInfo_.frameInterval) * 1000; // 1000: 1ms to us
        (void)OH_NativeWindow_NativeWindowHandleOpt(sampleInfo_.window, SET_UI_TIMESTAMP, pts);
        int fenceFd = -1;
        int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(sampleInfo_.window, &buffer, &fenceFd);
        CHECK_AND_CONTINUE_LOG(ret == 0, "RequestBuffer failed, ret: %{public}d", ret);

        BufferHandle* bufferHandle = OH_NativeWindow_GetBufferHandleFromNative(buffer);
        CHECK_AND_BREAK_LOG(bufferHandle != nullptr, "Get buffer handle failed, thread out");
        uint8_t *bufferAddr = static_cast<uint8_t *>(mmap(bufferHandle->virAddr, bufferHandle->size,
            PROT_READ | PROT_WRITE, MAP_SHARED, bufferHandle->fd, 0));
        CHECK_AND_BREAK_LOG(bufferAddr != MAP_FAILED, "Map native window buffer failed, thread out");

        CodecBufferInfo bufferInfo(bufferAddr);
        ret = dataProducer_->ReadSample(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Catch EOS, thread out");
        ret = munmap(bufferAddr, bufferHandle->size);
        CHECK_AND_BREAK_LOG(ret != -1, "Unmap buffer failed, thread out");

        ThreadSleep();

        AddSurfaceInputTrace(pts);
        ret = OH_NativeWindow_NativeWindowFlushBuffer(sampleInfo_.window, buffer, fenceFd, {nullptr, 0});
        CHECK_AND_BREAK_LOG(ret == 0, "Read frame failed, thread out");

        buffer = nullptr;
    }
    if (buffer != nullptr) {
        OH_NativeWindow_DestroyNativeWindowBuffer(buffer);
    }
    videoEncoder_->NotifyEndOfStream();
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->inputFrameCount_);
    StartRelease();
}

void VideoEncoderSample::OutputThread()
{
    while (true) {
        std::unique_lock<std::mutex> lock(context_->outputMutex_);
        bool condRet = context_->outputCond_.wait_for(lock, 5s,
            [this]() { return !context_->outputBufferInfoQueue_.empty(); });
        CHECK_AND_CONTINUE_LOG(!context_->outputBufferInfoQueue_.empty(),
            "Buffer queue is empty, continue, cond ret: %{public}d", condRet);

        CodecBufferInfo bufferInfo = context_->outputBufferInfoQueue_.front();
        context_->outputBufferInfoQueue_.pop();
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Catch EOS, thread out");
        context_->outputFrameCount_++;
        AVCODEC_LOGV("Out buffer count: %{public}u, size: %{public}d, flag: %{public}u, pts: %{public}" PRId64,
            context_->outputFrameCount_, bufferInfo.attr.size, bufferInfo.attr.flags, bufferInfo.attr.pts);
        lock.unlock();

        DumpOutput(bufferInfo);

        int32_t ret = videoEncoder_->FreeOutputData(bufferInfo.bufferIndex);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Encoder output thread out");
    }
    OHOS::MediaAVCodec::AVCodecTrace::TraceEnd("SampleWorkTime", FAKE_POINTER(this));
    OHOS::MediaAVCodec::AVCodecTrace::CounterTrace("SampleFrameCount", context_->outputFrameCount_);
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->outputFrameCount_);
    StartRelease();
}

inline void VideoEncoderSample::AddSurfaceInputTrace(uint64_t pts)
{
    OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("OH::Frame", pts);
}

void VideoEncoderSample::DumpOutput(const CodecBufferInfo &bufferInfo)
{
    if (!sampleInfo_.needDumpOutput) {
        return;
    }

    if (outputFile_ == nullptr) {
        auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        outputFile_ = std::make_unique<std::ofstream>("VideoEncoderOut_"s + std::to_string(time) + ".bin",
            std::ios::out | std::ios::trunc);
        if (!outputFile_->is_open()) {
            outputFile_ = nullptr;
        }
    }
    auto bufferAddr = static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b10 ?    // 0b10: AVBuffer mode mask
                      OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer)) :
                      OH_AVMemory_GetAddr(reinterpret_cast<OH_AVMemory *>(bufferInfo.buffer));
    outputFile_->write(reinterpret_cast<char *>(bufferAddr), bufferInfo.attr.size);
}
} // Sample
} // MediaAVCodec
} // OHOS