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
int32_t VideoDecoderSample::Init()
{
    auto &info = *context_->sampleInfo;
    if (!(info.codecRunMode & 0b01)) { // 0b01: Buffer mode mask
        switch (info.codecConsumerType) {
            case CODEC_COMSUMER_TYPE_DEFAULT:
                context_->windowWrapper =
                    WindowManager::GetInstance().CreateWindowWrapper(SampleWindowType::NATIVE_IMAGE);
                break;
#ifdef SAMPLE_BUILD_TO_EXECUTOR
            case CODEC_COMSUMER_TYPE_DECODER_RENDER_OUTPUT:
                context_->windowWrapper = WindowManager::GetInstance().CreateWindowWrapper(SampleWindowType::ROSEN);
                break;
#endif
            default:
                AVCODEC_LOGE("Not supported codec consumer type: %{public}d", info.codecConsumerType);
                break;
        }
        CHECK_AND_RETURN_RET_LOG(context_->windowWrapper != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create window failed!");
    }
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoderSample::Prepare()
{
    if ((*context_->sampleInfo).syncMode) {
        syncThread_ = std::make_unique<std::thread>(&VideoDecoderSample::SyncThread, this);
        CHECK_AND_RETURN_RET_LOG(syncThread_, AVCODEC_SAMPLE_ERR_ERROR, "Create sync thread failed");
    } else {
        inputThread_ = std::make_unique<std::thread>(&VideoDecoderSample::InputThread, this);
        outputThread_ = std::make_unique<std::thread>(&VideoDecoderSample::OutputThread, this);
        CHECK_AND_RETURN_RET_LOG(inputThread_ && outputThread_, AVCODEC_SAMPLE_ERR_ERROR, "Create thread failed");
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
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Read EOS frame, thread out");
        AVCODEC_LOGV("In buffer count: %{public}u, size: %{public}d, flag: %{public}u, pts: %{public}" PRId64,
            context_->inputBufferQueue.GetFrameCount(),
            bufferInfo.attr.size, bufferInfo.attr.flags, bufferInfo.attr.pts);

        ThreadSleep(info.threadSleepMode == THREAD_SLEEP_MODE_INPUT_SLEEP, info.frameInterval);

        ret = context_->videoCodec->PushInput(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Push data failed, thread out");
    }
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->inputBufferQueue.GetFrameCount());
    PushEosFrame();
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

        int32_t ret = context_->videoCodec->FreeOutput(bufferInfo.bufferIndex);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Decoder output thread out");
    }
    OHOS::MediaAVCodec::AVCodecTrace::TraceEnd("SampleWorkTime", FAKE_POINTER(this));
    OHOS::MediaAVCodec::AVCodecTrace::CounterTrace("SampleFrameCount", context_->outputBufferQueue.GetFrameCount());
    NotifySampleDone();
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->outputBufferQueue.GetFrameCount());
}

void VideoDecoderSample::SyncThread()
{
    OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("SampleWorkTime", FAKE_POINTER(this));
    auto &info = *context_->sampleInfo;
    bool outputDone = false;
    bool inputDone = false;
    uint32_t index;
    int32_t err = 0; // 0: ok 1: eos -1: error
    int32_t ret;
    int32_t outFrameCount = 0;
    while (!outputDone && err == 0) {
        if (!inputDone) {
            ret = context_->videoCodec->QueryInput(index, 0);
            switch (ret) {
                case AV_ERR_OK: {
                    auto bufferInfoOpt = context_->videoCodec->GetInput(index);
                    if (bufferInfoOpt == std::nullopt) {
                        // 异常处理
                        err = -1;
                        break;
                    }
                    auto &bufferInfo = bufferInfoOpt.value();
                    ret = dataProducer_->ReadSample(bufferInfo);
                    if (bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
                        inputDone = 1;
                    }
                    ret = context_->videoCodec->PushInput(bufferInfo);
                    if (ret != AVCODEC_SAMPLE_ERR_OK) {
                        // 异常处理
                        err = -1;
                        break;
                    }
                    break;
                }
                case AV_ERR_TRY_AGAIN_LATER: {
                    break;
                }
                default: {
                    err = -1;
                    break;
                }
            }
        }
        if (!outputDone) {
            ret = context_->videoCodec->QueryOutput(index, 0);
            switch (ret) {
                case AV_ERR_OK: {
                    auto bufferInfoOpt = context_->videoCodec->GetOutput(index);
                    if (bufferInfoOpt == std::nullopt) {
                        // 异常处理
                        err = -1;
                        break;
                    }
                    auto &bufferInfo = bufferInfoOpt.value();
                    if (bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
                        outputDone = 1;
                        err = 1;
                    } else {
                        DumpOutput(bufferInfo);
                    }
                    ret = context_->videoCodec->FreeOutput(bufferInfo.bufferIndex);
                    if (ret != AVCODEC_SAMPLE_ERR_OK) {
                        // 异常处理
                        err = -1;
                        break;
                    }
                    outFrameCount++;
                    break;
                }
                case AV_ERR_TRY_AGAIN_LATER: {
                    break;
                }
                case AV_ERR_STREAM_CHANGED: {
                    auto format = context_->videoCodec->GetFormat();
                    auto originVideoWidth = info.videoWidth;
                    auto originVideoHeight = info.videoHeight;
                    OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_PIC_WIDTH, &info.videoWidth);
                    OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_PIC_HEIGHT, &info.videoHeight);
                    OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_STRIDE, &info.videoStrideWidth);
                    OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_SLICE_HEIGHT, &info.videoSliceHeight);
                    auto &videoSliceHeight = info.videoSliceHeight;
                    videoSliceHeight = videoSliceHeight == 0 ? info.videoHeight : videoSliceHeight;
                    AVCODEC_LOGW("Resolution: %{public}d*%{public}d => %{public}d*%{public}d(%{public}d*%{public}d)",
                        originVideoWidth, originVideoHeight, info.videoWidth, info.videoHeight,
                        info.videoStrideWidth, info.videoSliceHeight);
                    break;
                }
                default: {
                    err = -1;
                    break;
                }
            }
        }
        ThreadSleep(info.threadSleepMode == THREAD_SLEEP_MODE_OUTPUT_SLEEP, info.frameInterval);
    }
    OHOS::MediaAVCodec::AVCodecTrace::TraceEnd("SampleWorkTime", FAKE_POINTER(this));
    OHOS::MediaAVCodec::AVCodecTrace::CounterTrace("SampleFrameCount", outFrameCount);
    NotifySampleDone();
    AVCODEC_LOGI("Exit, frame count: %{public}u", outFrameCount);
}
} // Sample
} // MediaAVCodec
} // OHOS