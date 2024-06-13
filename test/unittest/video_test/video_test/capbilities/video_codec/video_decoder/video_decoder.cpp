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

#include "video_decoder.h"
#include "av_codec_sample_error.h"
#include "av_codec_sample_log.h"
#include "codec_callback.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoDecoder"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
int32_t VideoDecoder::Create(const std::string &codecMime, bool isSoftware)
{
    auto codecName = GetCodecName(codecMime, false, isSoftware);
    CHECK_AND_RETURN_RET_LOG(!codecName.empty(), AVCODEC_SAMPLE_ERR_ERROR,
        "Codec not supported, mime: %{public}s, software: %{public}d", codecMime.c_str(), isSoftware);
    
    codec_ = std::shared_ptr<OH_AVCodec>(
        OH_VideoDecoder_CreateByName(codecName.c_str()), OH_VideoDecoder_Destroy);
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create failed");

    AVCODEC_LOGI("Succeed, codec name: %{public}s", codecName.c_str());
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Config(SampleInfo &sampleInfo, SampleContext * const sampleContext)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");
    CHECK_AND_RETURN_RET_LOG(sampleContext != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Invalid param: codecUserData");
    runMode_ = sampleInfo.codecRunMode;

    // Configure video decoder
    int32_t ret = Configure(sampleInfo);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Configure failed");

    // SetSurface from video decoder
    if (sampleInfo.window != nullptr) {
        ret = OH_VideoDecoder_SetSurface(codec_.get(), sampleInfo.window);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK && sampleInfo.window, AVCODEC_SAMPLE_ERR_ERROR,
            "Set surface failed, ret: %{public}d", ret);
    }

    // SetCallback for video decoder
    ret = SetCallback(sampleContext);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR,
        "Set callback failed, ret: %{public}d", ret);

    // Prepare video decoder
    ret = OH_VideoDecoder_Prepare(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Prepare failed, ret: %{public}d", ret);

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Start()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int ret = OH_VideoDecoder_Start(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Start failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Flush()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int ret = OH_VideoDecoder_Flush(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Flush failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Stop()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int32_t ret = OH_VideoDecoder_Stop(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Stop failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Reset()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int32_t ret = OH_VideoDecoder_Reset(codec_.get());
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Reset failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::PushInputData(CodecBufferInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int32_t ret = AV_ERR_OK;
    if (runMode_ & 0b10) { // 0b10: AVBuffer mode mask
        ret = OH_AVBuffer_SetBufferAttr(reinterpret_cast<OH_AVBuffer *>(info.buffer), &info.attr);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Set avbuffer attr failed");
        ret = OH_VideoDecoder_PushInputBuffer(codec_.get(), info.bufferIndex);
    } else {
        ret = OH_VideoDecoder_PushInputData(codec_.get(), info.bufferIndex, info.attr);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Push input data failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::FreeOutputData(uint32_t bufferIndex)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");
    
    int32_t ret = AVCODEC_SAMPLE_ERR_OK;
    if (runMode_ & 0b10) { // 0b10: AVBuffer mode mask
        ret = !(runMode_ & 0b01) ?
            OH_VideoDecoder_RenderOutputBuffer(codec_.get(), bufferIndex) :
            OH_VideoDecoder_FreeOutputBuffer(codec_.get(), bufferIndex);
    } else {
        ret = !(runMode_ & 0b01) ?
            OH_VideoDecoder_RenderOutputData(codec_.get(), bufferIndex) :
            OH_VideoDecoder_FreeOutputData(codec_.get(), bufferIndex);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Free output data failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::SetCallback(SampleContext *codecUserData)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");
    int32_t ret = AV_ERR_OK;
    if (runMode_ & 0b10) { // 0b10: AVBuffer mode mask
        ret = OH_VideoDecoder_RegisterCallback(codec_.get(), AVCodecCallback, codecUserData);
    } else {
        ret = OH_VideoDecoder_SetCallback(codec_.get(), AVCodecAsyncCallback, codecUserData);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Set callback failed, ret: %{public}d", ret);

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Configure(const SampleInfo &sampleInfo)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "AVFormat create failed");

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, sampleInfo.videoWidth);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, sampleInfo.videoHeight);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, sampleInfo.frameRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, sampleInfo.pixelFormat);

    if (sampleInfo.videoHeight < sampleInfo.videoWidth) {
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, 270);   // rotate 270°
    }
    
    int ret = OH_VideoDecoder_Configure(codec_.get(), format);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Config failed, ret: %{public}d", ret);
    OH_AVFormat_Destroy(format);
    format = nullptr;

    return AVCODEC_SAMPLE_ERR_OK;
}
} // Sample
} // MediaAVCodec
} // OHOS