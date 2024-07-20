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

#include "video_encoder.h"
#include <unordered_map>
#include "external_window.h"
#include "av_codec_sample_error.h"
#include "av_codec_sample_log.h"
#include "codec_callback.h"
#include "native_window.h"
#include "sample_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoEncoder"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
int32_t VideoEncoder::Create(const std::string &codecMime, bool isSoftware)
{
    auto codecName = GetCodecName(codecMime, true, isSoftware);
    CHECK_AND_RETURN_RET_LOG(!codecName.empty(), AVCODEC_SAMPLE_ERR_ERROR,
        "Codec not supported, mime: %{public}s, software: %{public}d", codecMime.c_str(), isSoftware);
    
    codec_ = std::shared_ptr<OH_AVCodec>(
        OH_VideoEncoder_CreateByName(codecName.c_str()), OH_VideoEncoder_Destroy);
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create failed");

    AVCODEC_LOGI("Succeed, codec name: %{public}s", codecName.c_str());
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoder::Config(SampleInfo &sampleInfo, uintptr_t * const sampleContext)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Encoder is null");
    CHECK_AND_RETURN_RET_LOG(sampleContext != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Invalid param: sampleContext");

    // Configure video encoder
    int32_t ret = Configure(sampleInfo);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Configure failed");
    
    // GetSurface from video encoder
    ret = GetSurface(sampleInfo);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Get surface failed");

    // SetCallback for video encoder
    ret = SetCallback(sampleContext);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Set callback failed");

    // Prepare video encoder
    ret = OH_VideoEncoder_Prepare(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Prepare failed, ret: %{public}d", ret);

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoder::Start()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Encoder is null");

    int ret = OH_VideoEncoder_Start(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Start failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoder::Flush()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Encoder is null");

    int ret = OH_VideoEncoder_Flush(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Flush failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoder::Stop()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Encoder is null");

    int32_t ret = OH_VideoEncoder_Stop(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Stop failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoder::Reset()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Encoder is null");

    int32_t ret = OH_VideoEncoder_Reset(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Reset failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

OH_AVFormat *VideoEncoder::GetFormat()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, nullptr, "Decoder is null");
    return OH_VideoEncoder_GetInputDescription(codec_.get());
}

int32_t VideoEncoder::NotifyEndOfStream()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Encoder is null");

    int32_t ret = OH_VideoEncoder_NotifyEndOfStream(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR,
        "Notify end of stream failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoder::Configure(const SampleInfo &sampleInfo)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "AVFormat create failed");

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, sampleInfo.videoWidth);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, sampleInfo.videoHeight);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, sampleInfo.frameRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, sampleInfo.bitrateMode);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, sampleInfo.bitrate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, sampleInfo.pixelFormat);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, sampleInfo.profile);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, sampleInfo.iFrameInterval);
    
    int ret = OH_VideoEncoder_Configure(codec_.get(), format);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Config failed, ret: %{public}d", ret);

    OH_AVFormat_Destroy(format);
    format = nullptr;
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoder::GetSurface(SampleInfo &sampleInfo)
{
    // 0b01: buffer mode mask
    CHECK_AND_RETURN_RET(!(static_cast<uint8_t>(sampleInfo.codecRunMode) & 0b01), AVCODEC_SAMPLE_ERR_OK);

    NativeWindow *window = nullptr;
    int32_t ret = OH_VideoEncoder_GetSurface(codec_.get(), &window);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK && window, AVCODEC_SAMPLE_ERR_ERROR,
        "Get surface failed, ret: %{public}d", ret);
    (void)OH_NativeWindow_NativeWindowHandleOpt(window, SET_BUFFER_GEOMETRY,
        sampleInfo.videoWidth, sampleInfo.videoHeight);
    (void)OH_NativeWindow_NativeWindowHandleOpt(window, SET_USAGE, 16425);      // 16425: Window usage
    (void)OH_NativeWindow_NativeWindowHandleOpt(window, SET_FORMAT,
        ToGraphicPixelFormat(sampleInfo.pixelFormat, sampleInfo.profile));

    if (sampleInfo.encoderSurfaceMaxInputBuffer >= 0) {
        window->surface->SetQueueSize(sampleInfo.encoderSurfaceMaxInputBuffer);
    }
    sampleInfo.window = std::shared_ptr<NativeWindow>(window, OH_NativeWindow_DestroyNativeWindow);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderAPI10::FreeOutput(uint32_t bufferIndex)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Encoder is null");

    int32_t ret = OH_VideoEncoder_FreeOutputData(codec_.get(), bufferIndex);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR,
        "Free output data failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderAPI10::SetCallback(uintptr_t *const sampleContext)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Encoder is null");

    int32_t ret = OH_VideoEncoder_SetCallback(codec_.get(), AVCodecAsyncCallback, sampleContext);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Set callback failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderAPI10Buffer::PushInput(CodecBufferInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int32_t ret = OH_VideoEncoder_PushInputData(codec_.get(), info.bufferIndex, info.attr);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Push input data failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderAPI10Surface::PushInput(CodecBufferInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");
    CHECK_AND_RETURN_RET_LOG(info.attr.flags == AVCODEC_BUFFER_FLAGS_EOS, AVCODEC_SAMPLE_ERR_ERROR,
        "Not EOS frame but called notify EOS");

    int32_t ret = OH_VideoEncoder_NotifyEndOfStream(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR,
        "Notify EOS failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderAPI11::FreeOutput(uint32_t bufferIndex)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Encoder is null");

    int32_t ret = OH_VideoEncoder_FreeOutputBuffer(codec_.get(), bufferIndex);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR,
        "Free output buffer failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderAPI11::SetCallback(uintptr_t *const sampleContext)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Encoder is null");
    
    int32_t ret = OH_VideoEncoder_RegisterCallback(codec_.get(), AVCodecCallback, sampleContext);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Set callback failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderAPI11Buffer::PushInput(CodecBufferInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int32_t ret = OH_AVBuffer_SetBufferAttr(reinterpret_cast<OH_AVBuffer *>(info.buffer), &info.attr);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Set avbuffer attr failed");
    ret = OH_VideoEncoder_PushInputBuffer(codec_.get(), info.bufferIndex);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Push input buffer failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderAPI11Surface::PushInput(CodecBufferInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");
    CHECK_AND_RETURN_RET_LOG(info.attr.flags == AVCODEC_BUFFER_FLAGS_EOS, AVCODEC_SAMPLE_ERR_ERROR,
        "Not EOS frame but called notify EOS");

    int32_t ret = OH_VideoEncoder_NotifyEndOfStream(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR,
        "Notify EOS failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}
} // Sample
} // MediaAVCodec
} // OHOS