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
#include <memory>
#include <sys/mman.h>
#include "external_window.h"
#include "native_buffer_inner.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"
#include "avcodec_trace.h"
#include "sample_utils.h"

namespace {
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
    inputThread_ = (static_cast<uint8_t>(context_->sampleInfo->codecRunMode) & 0b01) ?  // 0b01: Buffer mode mask
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
    auto &info = *context_->sampleInfo;
    while (true) {
        auto bufferInfoOpt = context_->inputBufferQueue.DequeueBuffer();
        CHECK_AND_CONTINUE_LOG(bufferInfoOpt != std::nullopt, "Buffer queue is empty, try dequeue again");
        auto &bufferInfo = bufferInfoOpt.value();

        int32_t ret = dataProducer_->ReadSample(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");
        AVCODEC_LOGV("In buffer count: %{public}u, size: %{public}d, flag: %{public}u, pts: %{public}" PRId64,
            context_->inputBufferQueue.GetFrameCount(),
            bufferInfo.attr.size, bufferInfo.attr.flags, bufferInfo.attr.pts);

        ThreadSleep(info.threadSleepMode == THREAD_SLEEP_MODE_INPUT_SLEEP, info.frameInterval);

        ret = context_->videoCodec_->PushInput(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Push data failed, thread out");
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Push EOS frame, thread out");
    }
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->inputBufferQueue.GetFrameCount());
    PushEosFrame();
    StartRelease();
}

void VideoEncoderSample::SurfaceInputThread()
{
    OHNativeWindowBuffer *buffer = nullptr;
    OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("SampleWorkTime", FAKE_POINTER(this));
    auto &info = *context_->sampleInfo;
    while (true) {
        uint32_t frameCount = context_->inputBufferQueue.IncFrameCount();
        uint64_t pts = static_cast<uint64_t>(frameCount) *
            ((info.frameInterval == 0) ? 1 : info.frameInterval) * 1000; // 1000: 1ms to us
        (void)OH_NativeWindow_NativeWindowHandleOpt(info.window.get(), SET_UI_TIMESTAMP, pts);
        int fenceFd = -1;
        int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(info.window.get(), &buffer, &fenceFd);
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

        ThreadSleep(info.threadSleepMode == THREAD_SLEEP_MODE_INPUT_SLEEP, info.frameInterval);

        AVCodecTrace::TraceBegin("OH::Frame", pts);
        ret = OH_NativeWindow_NativeWindowFlushBuffer(info.window.get(), buffer, fenceFd, {nullptr, 0});
        CHECK_AND_BREAK_LOG(ret == 0, "Read frame failed, thread out");

        buffer = nullptr;
    }
    OH_NativeWindow_DestroyNativeWindowBuffer(buffer);
    context_->videoCodec_->PushInput(eosBufferInfo);
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->inputBufferQueue.GetFrameCount());
    StartRelease();
}

void VideoEncoderSample::OutputThread()
{
    auto &info = *context_->sampleInfo;
    while (true) {
        auto bufferInfoOpt = context_->outputBufferQueue.DequeueBuffer();
        CHECK_AND_CONTINUE_LOG(bufferInfoOpt != std::nullopt, "Buffer queue is empty, try dequeue again");
        auto &bufferInfo = bufferInfoOpt.value();
        AVCODEC_LOGV("Out buffer count: %{public}u, size: %{public}d, flag: %{public}u, pts: %{public}" PRId64,
            context_->outputBufferQueue.GetFrameCount(),
            bufferInfo.attr.size, bufferInfo.attr.flags, bufferInfo.attr.pts);
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Catch EOS frame, thread out");

        DumpOutput(bufferInfo);
        ThreadSleep(info.threadSleepMode == THREAD_SLEEP_MODE_OUTPUT_SLEEP, info.frameInterval);

        int32_t ret = context_->videoCodec_->FreeOutput(bufferInfo.bufferIndex);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Decoder output thread out");
    }
    OHOS::MediaAVCodec::AVCodecTrace::TraceEnd("SampleWorkTime", FAKE_POINTER(this));
    OHOS::MediaAVCodec::AVCodecTrace::CounterTrace("SampleFrameCount", context_->outputBufferQueue.GetFrameCount());
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->outputBufferQueue.GetFrameCount());
}
} // Sample
} // MediaAVCodec
} // OHOS