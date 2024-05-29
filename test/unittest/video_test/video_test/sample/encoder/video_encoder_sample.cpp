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
using namespace std::chrono_literals;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoEncoderSample"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
int32_t VideoEncoderSample::Init()
{
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderSample::StartThread()
{
    inputThread_ = (static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b01) ?  // 0b01: Buffer mode mask
        std::make_unique<std::thread>(&VideoEncoderSample::BufferInputThread, this) :
        std::make_unique<std::thread>(&VideoEncoderSample::SurfaceInputThread, this);
    outputThread_ = std::make_unique<std::thread>(&VideoEncoderSample::OutputThread, this);
    if (inputThread_ == nullptr || outputThread_ == nullptr) {
        AVCODEC_LOGE("Create thread failed");
        StartRelease();
        return AVCODEC_SAMPLE_ERR_ERROR;
    }
    return AVCODEC_SAMPLE_ERR_OK;
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

        ThreadSleep(sampleInfo_.threadSleepMode == THREAD_SLEEP_MODE_INPUT_SLEEP);

        ret = videoCodec_->PushInputData(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Push data failed, thread out");
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Push EOS frame, thread out");
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
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Push EOS frame, thread out");
        ret = munmap(bufferAddr, bufferHandle->size);
        CHECK_AND_BREAK_LOG(ret != -1, "Unmap buffer failed, thread out");

        ThreadSleep(sampleInfo_.threadSleepMode == THREAD_SLEEP_MODE_INPUT_SLEEP);

        AVCodecTrace::TraceBegin("OH::Frame", pts);
        ret = OH_NativeWindow_NativeWindowFlushBuffer(sampleInfo_.window, buffer, fenceFd, {nullptr, 0});
        CHECK_AND_BREAK_LOG(ret == 0, "Read frame failed, thread out");

        buffer = nullptr;
    }
    OH_NativeWindow_DestroyNativeWindowBuffer(buffer);
    videoCodec_->PushInputData(eosBufferInfo);
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
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Catch EOS frame, thread out");
        context_->outputFrameCount_++;
        AVCODEC_LOGV("Out buffer count: %{public}u, size: %{public}d, flag: %{public}u, pts: %{public}" PRId64,
            context_->outputFrameCount_, bufferInfo.attr.size, bufferInfo.attr.flags, bufferInfo.attr.pts);
        lock.unlock();

        DumpOutput(bufferInfo);
        ThreadSleep(sampleInfo_.threadSleepMode == THREAD_SLEEP_MODE_OUTPUT_SLEEP);

        int32_t ret = videoCodec_->FreeOutputData(bufferInfo.bufferIndex);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Encoder output thread out");
    }
    AVCodecTrace::TraceEnd("SampleWorkTime", FAKE_POINTER(this));
    AVCodecTrace::CounterTrace("SampleFrameCount", context_->outputFrameCount_);
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->outputFrameCount_);
    StartRelease();
}
} // Sample
} // MediaAVCodec
} // OHOS