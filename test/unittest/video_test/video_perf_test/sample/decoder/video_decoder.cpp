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
#include "sample_callback.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoDecoder"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
VideoDecoder::~VideoDecoder()
{
    Release();
}

int32_t VideoDecoder::Create(const std::string &codecMime)
{
    decoder_ = OH_VideoDecoder_CreateByMime(codecMime.c_str());
    CHECK_AND_RETURN_RET_LOG(decoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Config(const SampleInfo &sampleInfo, CodecUserData *codecUserData)
{
    CHECK_AND_RETURN_RET_LOG(decoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");
    CHECK_AND_RETURN_RET_LOG(codecUserData != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Invalid param: codecUserData");
    isAVBufferMode_ = (static_cast<uint8_t>(sampleInfo.codecRunMode) & 0b10);  // ob10: AVBuffer mode mask

    // Configure video decoder
    int32_t ret = Configure(sampleInfo);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Configure failed");

    // SetSurface from video decoder
    if (sampleInfo.window != nullptr) {
        ret = OH_VideoDecoder_SetSurface(decoder_, sampleInfo.window);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK && sampleInfo.window, AVCODEC_SAMPLE_ERR_ERROR,
            "Set surface failed, ret: %{public}d", ret);
    }

    // SetCallback for video decoder
    ret = SetCallback(codecUserData);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR,
        "Set callback failed, ret: %{public}d", ret);

    // Prepare video decoder
    ret = OH_VideoDecoder_Prepare(decoder_);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Prepare failed, ret: %{public}d", ret);

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Start()
{
    CHECK_AND_RETURN_RET_LOG(decoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int ret = OH_VideoDecoder_Start(decoder_);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Start failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::PushInputData(CodecBufferInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(decoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int32_t ret = AV_ERR_OK;
    if (isAVBufferMode_) {
        ret = OH_AVBuffer_SetBufferAttr(reinterpret_cast<OH_AVBuffer *>(info.buffer), &info.attr);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Set avbuffer attr failed");
        ret = OH_VideoDecoder_PushInputBuffer(decoder_, info.bufferIndex);
    } else {
        ret = OH_VideoDecoder_PushInputData(decoder_, info.bufferIndex, info.attr);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Push input data failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::FreeOutputData(uint32_t bufferIndex, bool renderOutput)
{
    CHECK_AND_RETURN_RET_LOG(decoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");
    
    int32_t ret = AVCODEC_SAMPLE_ERR_OK;
    if (isAVBufferMode_) {
        ret = renderOutput ? OH_VideoDecoder_RenderOutputBuffer(decoder_, bufferIndex) :
            OH_VideoDecoder_FreeOutputBuffer(decoder_, bufferIndex);
    } else {
        ret = renderOutput ? OH_VideoDecoder_RenderOutputData(decoder_, bufferIndex) :
            OH_VideoDecoder_FreeOutputData(decoder_, bufferIndex);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Free output data failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Stop()
{
    CHECK_AND_RETURN_RET_LOG(decoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int ret = OH_VideoDecoder_Flush(decoder_);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Flush failed, ret: %{public}d", ret);

    ret = OH_VideoDecoder_Stop(decoder_);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Stop failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Release()
{
    if (decoder_ != nullptr) {
        OH_VideoDecoder_Destroy(decoder_);
        decoder_ = nullptr;
    }
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::SetCallback(CodecUserData *codecUserData)
{
    int32_t ret = AV_ERR_OK;
    if (isAVBufferMode_) {
        ret = OH_VideoDecoder_RegisterCallback(decoder_,
            {SampleCallback::OnCodecError, SampleCallback::OnCodecFormatChange,
             SampleCallback::OnNeedInputBuffer, SampleCallback::OnNewOutputBuffer}, codecUserData);
    } else {
        ret = OH_VideoDecoder_SetCallback(decoder_,
            {SampleCallback::OnCodecError, SampleCallback::OnCodecFormatChange,
             SampleCallback::OnInputBufferAvailable, SampleCallback::OnOutputBufferAvailable}, codecUserData);
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
    
    int ret = OH_VideoDecoder_Configure(decoder_, format);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Config failed, ret: %{public}d", ret);
    OH_AVFormat_Destroy(format);
    format = nullptr;

    return AVCODEC_SAMPLE_ERR_OK;
}
} // Sample
} // MediaAVCodec
} // OHOS