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
#include "refbase.h"
#include "surface/window.h"
#include "surface.h"

#include "ui/rs_surface_node.h"
#include "window_option.h"

#include "avcodec_trace.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"
#include "sample_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoDecoderSample"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
VideoDecoderSample::~VideoDecoderSample()
{
    if (rosenWindow_) {
        rosenWindow_->Destroy();
        rosenWindow_ = nullptr;
    }
}

int32_t VideoDecoderSample::Init()
{
    auto &info = *context_->sampleInfo;
    if (!(info.codecRunMode & 0b01)) { // 0b01: Buffer mode mask
        int32_t ret = CreateWindow(info.window);
        CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create window failed");
    }
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoderSample::StartThread()
{
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

void VideoDecoderSample::OutputThread()
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

int32_t VideoDecoderSample::CreateWindow(std::shared_ptr<NativeWindow> &window)
{
    sptr<OHOS::Surface> surfaceProducer;
    if (context_->sampleInfo->codecConsumerType == CODEC_COMSUMER_TYPE_DEFAULT) {
        surfaceConsumer_ = OHOS::Surface::CreateSurfaceAsConsumer("VideoCodecDemo");
        OHOS::sptr<OHOS::IBufferConsumerListener> listener = this;
        surfaceConsumer_->RegisterConsumerListener(listener);
        auto producer = surfaceConsumer_->GetProducer();
        surfaceProducer = OHOS::Surface::CreateSurfaceAsProducer(producer);
    } else if (context_->sampleInfo->codecConsumerType == CODEC_COMSUMER_TYPE_DECODER_RENDER_OUTPUT) {
        sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
        option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_FLOAT);
        option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FULLSCREEN);
        rosenWindow_ = Rosen::Window::Create("VideoCodecDemo", option);
        CHECK_AND_RETURN_RET_LOG(rosenWindow_ != nullptr && rosenWindow_->GetSurfaceNode() != nullptr,
            AVCODEC_SAMPLE_ERR_ERROR, "Create display window failed");
        rosenWindow_->SetTurnScreenOn(!rosenWindow_->IsTurnScreenOn());
        rosenWindow_->SetKeepScreenOn(true);
        rosenWindow_->Show();
        surfaceProducer = rosenWindow_->GetSurfaceNode()->GetSurface();
    }
    window = std::shared_ptr<NativeWindow>(reinterpret_cast<NativeWindow *>(
        CreateNativeWindowFromSurface(&surfaceProducer)), [](NativeWindow *window) -> void { (void)window; });
    CHECK_AND_RETURN_RET_LOG(window != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create window failed!");

    return AVCODEC_SAMPLE_ERR_OK;
}

void VideoDecoderSample::OnBufferAvailable()
{
    OHOS::sptr<OHOS::SurfaceBuffer> buffer;
    int64_t timestamp = 0;
    OHOS::Rect damage = {};
    int32_t flushFence;
    surfaceConsumer_->AcquireBuffer(buffer, flushFence, timestamp, damage);

    if (context_->sampleInfo->needDumpOutput) {
        CodecBufferInfo bufferInfo(reinterpret_cast<uint8_t *>(buffer->GetVirAddr()), buffer->GetSize());
        DumpOutput(bufferInfo);
    }
    surfaceConsumer_->ReleaseBuffer(buffer, flushFence);
}
} // Sample
} // MediaAVCodec
} // OHOS