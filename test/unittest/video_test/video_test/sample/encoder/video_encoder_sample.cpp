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
#include <cmath>
#include "external_window.h"
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

int32_t VideoEncoderSample::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(context_ && context_->sampleInfo, AVCODEC_SAMPLE_ERR_ERROR, "Context is nullptr");
    bool syncMode = (*context_->sampleInfo).syncMode;
    auto &info = *context_->sampleInfo;
    if (static_cast<uint8_t>(info.codecRunMode) & 0b01) {  // 0b01: Buffer mode mask
        auto format = context_->videoCodec->GetFormat();
        OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_STRIDE, &info.videoStrideWidth);
        OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_SLICE_HEIGHT, &info.videoSliceHeight);
        if (!syncMode) {
            inputThread_ = std::make_unique<std::thread>(&VideoEncoderSample::BufferInputThread, this);
        }
    } else {
        auto window = context_->windowWrapper->GetWindow().get();
        (void)OH_NativeWindow_NativeWindowHandleOpt(window, SET_BUFFER_GEOMETRY,
            info.videoWidth, info.videoHeight);
        (void)OH_NativeWindow_NativeWindowHandleOpt(window, SET_FORMAT,
            ToGraphicPixelFormat(info.pixelFormat, info.profile));

        OHNativeWindowBuffer *buffer = nullptr;
        int fenceFd = -1;
        int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(window, &buffer, &fenceFd);
        CHECK_AND_RETURN_RET_LOG(ret == 0, AVCODEC_SAMPLE_ERR_ERROR, "RequestBuffer failed, ret: %{public}d", ret);
        OH_NativeBuffer *nativeBuffer = nullptr;
        OH_NativeBuffer_FromNativeWindowBuffer(buffer, &nativeBuffer);
        OH_NativeBuffer_Config config;
        OH_NativeBuffer_GetConfig(nativeBuffer, &config);
 
        info.videoStrideWidth = config.stride;
        info.videoSliceHeight = info.videoHeight;
        OH_NativeWindow_NativeWindowAbortBuffer(window, buffer);
        if (!syncMode) {
            inputThread_ = std::make_unique<std::thread>(&VideoEncoderSample::SurfaceInputThread, this);
        }
    }
    AVCODEC_LOGI("Resolution: %{public}d*%{public}d => %{public}d*%{public}d",
        info.videoWidth, info.videoHeight, info.videoStrideWidth, info.videoSliceHeight);
    if (!syncMode) {
        outputThread_ = std::make_unique<std::thread>(&VideoEncoderSample::OutputThread, this);
        CHECK_AND_RETURN_RET_LOG(inputThread_->joinable() && outputThread_->joinable(),
            AVCODEC_SAMPLE_ERR_ERROR, "Create thread failed");
    } else {
        syncThread_ = std::make_unique<std::thread>(&VideoEncoderSample::SyncThread, this);
        CHECK_AND_RETURN_RET_LOG(syncThread_, AVCODEC_SAMPLE_ERR_ERROR, "Create sync thread failed");
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

void VideoEncoderSample::SetRoiByNativebuf(OH_NativeBuffer *nativeBuffer)
{
    if (nativeBuffer != nullptr && context_->sampleInfo->enableRoiByNativebuf) {
        std::string roiInfo;
        if (context_->inputStreamByNativebuf.is_open() &&
            std::getline(context_->inputStreamByNativebuf, roiInfo) && roiInfo != "") {
            if (roiInfo == "clear") {
                roiInfo = "";
            }
            (void)OH_NativeBuffer_SetMetadataValue(nativeBuffer, OH_REGION_OF_INTEREST_METADATA,
                roiInfo.size(), reinterpret_cast<uint8_t*>(const_cast<char*>(roiInfo.c_str())));
        }
    }
}

void VideoEncoderSample::SurfaceInputThread()
{
    OHNativeWindowBuffer *buffer = nullptr;
    int fenceFd = -1;
    auto &info = *context_->sampleInfo;
    OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("SampleWorkTime", FAKE_POINTER(this));
    while (true) {
        uint32_t frameCount = context_->inputBufferQueue.IncFrameCount();
        uint64_t pts = static_cast<uint64_t>(frameCount) *
            ((info.frameInterval == 0) ? 1 : info.frameInterval) * 1000; // 1000: 1ms to us
        auto window = context_->windowWrapper->GetWindow().get();
        (void)OH_NativeWindow_NativeWindowHandleOpt(window, SET_UI_TIMESTAMP, pts);
        int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(window, &buffer, &fenceFd);
        CHECK_AND_CONTINUE_LOG(ret == 0, "RequestBuffer failed, ret: %{public}d", ret);
        OH_NativeBuffer *nativeBuffer;
        ret = OH_NativeBuffer_FromNativeWindowBuffer(buffer, &nativeBuffer);
        CHECK_AND_BREAK_LOG(ret == 0, "Convert from window buffer failed, thread out");
        SetRoiByNativebuf(nativeBuffer);
        void *virAddr = nullptr;
        ret = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
        CHECK_AND_BREAK_LOG(ret == 0, "Map native buffer failed, thread out");
        uint8_t *bufferAddr = static_cast<uint8_t *>(virAddr);
        CodecBufferInfo bufferInfo(bufferAddr);
        ret = dataProducer_->ReadSample(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Read EOS frame, thread out");
        ret = OH_NativeBuffer_Unmap(nativeBuffer);
        CHECK_AND_BREAK_LOG(ret == 0, "Unmap native buffer failed, thread out");

        ThreadSleep(info.threadSleepMode == THREAD_SLEEP_MODE_INPUT_SLEEP, info.frameInterval);

        AVCodecTrace::TraceBegin("OH::Frame", pts);
        ret = OH_NativeWindow_NativeWindowFlushBuffer(window, buffer, fenceFd, {nullptr, 0});
        CHECK_AND_BREAK_LOG(ret == 0, "Read frame failed, thread out");

        buffer = nullptr;
    }
    OH_NativeWindow_DestroyNativeWindowBuffer(buffer);
    context_->videoCodec->PushInput(eosBufferInfo);
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->inputBufferQueue.GetFrameCount());
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
        if (bufferInfo.attr.size > 0) {
            DumpOutput(bufferInfo);
        }
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Catch EOS frame, thread out");

        ThreadSleep(info.threadSleepMode == THREAD_SLEEP_MODE_OUTPUT_SLEEP, info.frameInterval);

        int32_t ret = context_->videoCodec->FreeOutput(bufferInfo.bufferIndex);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Decoder output thread out");
    }
    OHOS::MediaAVCodec::AVCodecTrace::TraceEnd("SampleWorkTime", FAKE_POINTER(this));
    OHOS::MediaAVCodec::AVCodecTrace::CounterTrace("SampleFrameCount", context_->outputBufferQueue.GetFrameCount());
    NotifySampleDone();
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->outputBufferQueue.GetFrameCount());
}

void VideoEncoderSample::SyncThread()
{
    OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("SampleWorkTime", FAKE_POINTER(this));
    OHNativeWindowBuffer *buffer = nullptr;
    int fenceFd = -1;
    auto &info = *context_->sampleInfo;
    bool outputDone = false;
    bool inputDone = false;
    uint32_t index;
    int32_t err = 0; // 0: ok 1: eos -1: error
    int32_t ret;
    int32_t outFrameCount = 0;
    int32_t inFrameCount = 0;
    while (!outputDone && err == 0) {
        if (!inputDone) {
            if (static_cast<uint8_t>(info.codecRunMode) & 0b01) {  // 0b01: Buffer mode mask
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
            } else {
                inFrameCount++;
                uint64_t pts = static_cast<uint64_t>(inFrameCount) *
                    ((info.frameInterval == 0) ? 1 : info.frameInterval) * 1000; // 1000: 1ms to us
                auto window = context_->windowWrapper->GetWindow().get();
                (void)OH_NativeWindow_NativeWindowHandleOpt(window, SET_UI_TIMESTAMP, pts);
                ret = OH_NativeWindow_NativeWindowRequestBuffer(window, &buffer, &fenceFd);
                CHECK_AND_CONTINUE_LOG(ret == 0, "RequestBuffer failed, ret: %{public}d", ret);

                BufferHandle* bufferHandle = OH_NativeWindow_GetBufferHandleFromNative(buffer);
                CHECK_AND_BREAK_LOG(bufferHandle != nullptr, "Get buffer handle failed, thread out");
                uint8_t *bufferAddr = static_cast<uint8_t *>(mmap(bufferHandle->virAddr, bufferHandle->size,
                    PROT_READ | PROT_WRITE, MAP_SHARED, bufferHandle->fd, 0));
                CHECK_AND_BREAK_LOG(bufferAddr != MAP_FAILED, "Map native window buffer failed, thread out");

                CodecBufferInfo bufferInfo(bufferAddr);
                ret = dataProducer_->ReadSample(bufferInfo);
                CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");
                CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Read EOS frame, thread out");
                ret = munmap(bufferAddr, bufferHandle->size);
                CHECK_AND_BREAK_LOG(ret != -1, "Unmap buffer failed, thread out");

                AVCodecTrace::TraceBegin("OH::Frame", pts);
                ret = OH_NativeWindow_NativeWindowFlushBuffer(window, buffer, fenceFd, {nullptr, 0});
                CHECK_AND_BREAK_LOG(ret == 0, "Read frame failed, thread out");

                buffer = nullptr;
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