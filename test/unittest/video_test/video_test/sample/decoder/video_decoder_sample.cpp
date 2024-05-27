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

#include "video_decoder_sample.h"
#include <chrono>
#include "refbase.h"
#include "surface/window.h"
#include "surface.h"

#include "ui/rs_surface_node.h"
#include "window_option.h"
#include "wm/window.h"

#include "avcodec_trace.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"

namespace {
using namespace std::chrono_literals;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoDecoderSample"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
int32_t VideoDecoderSample::Init()
{
    if (!(sampleInfo_.codecRunMode & 0b01)) { // 0b01: Buffer mode mask
        int32_t ret = CreateWindow(sampleInfo_.window);
        CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create window failed");
    }
    inputThread_ = std::make_unique<std::thread>(&VideoDecoderSample::InputThread, this);
    outputThread_ = std::make_unique<std::thread>(&VideoDecoderSample::OutputThread, this);
    if (inputThread_ == nullptr || outputThread_ == nullptr) {
        AVCODEC_LOGE("Create thread failed");
        StartRelease();
        return AVCODEC_SAMPLE_ERR_ERROR;
    }

    return AVCODEC_SAMPLE_ERR_OK;
}

void VideoDecoderSample::InputThread()
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

        int32_t ret = dataProducer_->ReadSample(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");
        AVCODEC_LOGV("In buffer count: %{public}u, size: %{public}d, flag: %{public}u, pts: %{public}" PRId64,
            context_->inputFrameCount_, bufferInfo.attr.size, bufferInfo.attr.flags, bufferInfo.attr.pts);

        ThreadSleep(sampleInfo_.threadSleepMode == THREAD_SLEEP_MODE_INPUT_SLEEP);

        ret = videoCodec_->PushInputData(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Push data failed, thread out");
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Push EOS frame, thread out");
    }
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->inputFrameCount_);
    StartRelease();
}

void VideoDecoderSample::OutputThread()
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
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Decoder output thread out");
    }
    OHOS::MediaAVCodec::AVCodecTrace::TraceEnd("SampleWorkTime", FAKE_POINTER(this));
    OHOS::MediaAVCodec::AVCodecTrace::CounterTrace("SampleFrameCount", context_->outputFrameCount_);
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->outputFrameCount_);
    StartRelease();
}

int32_t VideoDecoderSample::CreateWindow(OHNativeWindow *&window)
{
    if (sampleInfo_.codecComsumerType == CODEC_COMSUMER_TYPE_DEFAULT) {
        surfaceConsumer_ = OHOS::Surface::CreateSurfaceAsConsumer("VideoCodecDemo");
        OHOS::sptr<OHOS::IBufferConsumerListener> listener = this;
        surfaceConsumer_->RegisterConsumerListener(listener);
        auto producer = surfaceConsumer_->GetProducer();
        auto surfaceProducer = OHOS::Surface::CreateSurfaceAsProducer(producer);
        window = CreateNativeWindowFromSurface(&surfaceProducer);
        CHECK_AND_RETURN_RET_LOG(window != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create window failed!");
    } else if (sampleInfo_.codecComsumerType == CODEC_COMSUMER_TYPE_DECODER_RENDER_OUTPUT) {
        sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
        option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_FLOAT);
        option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FULLSCREEN);
        sptr<Rosen::Window> rosenWindow = Rosen::Window::Create("VideoCodecDemo", option);
        CHECK_AND_RETURN_RET_LOG(rosenWindow != nullptr && rosenWindow->GetSurfaceNode() != nullptr,
            AVCODEC_SAMPLE_ERR_ERROR, "Create display window failed");
        rosenWindow->SetTurnScreenOn(!rosenWindow->IsTurnScreenOn());
        rosenWindow->SetKeepScreenOn(true);
        rosenWindow->Show();
        surfaceConsumer_ = rosenWindow->GetSurfaceNode()->GetSurface();
        window = CreateNativeWindowFromSurface(&surfaceConsumer_);
    }

    return AVCODEC_SAMPLE_ERR_OK;
}

void VideoDecoderSample::OnBufferAvailable()
{
    OHOS::sptr<OHOS::SurfaceBuffer> buffer;
    int32_t flushFence;
    surfaceConsumer_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);

    if (sampleInfo_.needDumpOutput) {
        CodecBufferInfo bufferInfo(reinterpret_cast<uint8_t *>(buffer->GetVirAddr()), buffer->GetSize());
        DumpOutput(bufferInfo);
    }
    surfaceConsumer_->ReleaseBuffer(buffer, flushFence);
}
} // Sample
} // MediaAVCodec
} // OHOS