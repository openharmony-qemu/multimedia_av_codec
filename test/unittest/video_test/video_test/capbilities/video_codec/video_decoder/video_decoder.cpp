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
VideoDecoder::~VideoDecoder()
{
    Release();
}

int32_t VideoDecoder::Create(const std::string &codecMime, bool isSoftware)
{
    (void)isSoftware;
    if (isSoftware) {
        codec_ = OH_VideoDecoder_CreateByName("OH.Media.Codec.Decoder.Video.AVC");
    } else {
        codec_ = OH_VideoDecoder_CreateByMime(codecMime.c_str());
    }
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Config(SampleInfo &sampleInfo, CodecUserData *codecUserData)
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");
    CHECK_AND_RETURN_RET_LOG(codecUserData != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Invalid param: codecUserData");
    runMode_ = sampleInfo.codecRunMode;

    // Configure video decoder
    int32_t ret = Configure(sampleInfo);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Configure failed");

    // SetSurface from video decoder
    if (sampleInfo.window != nullptr) {
        ret = OH_VideoDecoder_SetSurface(codec_, sampleInfo.window);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK && sampleInfo.window, AVCODEC_SAMPLE_ERR_ERROR,
            "Set surface failed, ret: %{public}d", ret);
    }

    // SetCallback for video decoder
    ret = SetCallback(codecUserData);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR,
        "Set callback failed, ret: %{public}d", ret);

    // Prepare video decoder
    ret = OH_VideoDecoder_Prepare(codec_);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Prepare failed, ret: %{public}d", ret);

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Start()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int ret = OH_VideoDecoder_Start(codec_);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Start failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Flush()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int ret = OH_VideoDecoder_Flush(codec_);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Flush failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Stop()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int32_t ret = OH_VideoDecoder_Stop(codec_);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Stop failed, ret: %{public}d", ret);
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Reset()
{
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Decoder is null");

    int32_t ret = OH_VideoDecoder_Reset(codec_);
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
        ret = OH_VideoDecoder_PushInputBuffer(codec_, info.bufferIndex);
    } else {
        ret = OH_VideoDecoder_PushInputData(codec_, info.bufferIndex, info.attr);
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
            OH_VideoDecoder_RenderOutputBuffer(codec_, bufferIndex) :
            OH_VideoDecoder_FreeOutputBuffer(codec_, bufferIndex);
    } else {
        ret = !(runMode_ & 0b01) ?
            OH_VideoDecoder_RenderOutputData(codec_, bufferIndex) :
            OH_VideoDecoder_FreeOutputData(codec_, bufferIndex);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Free output data failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::Release()
{
    if (codec_ != nullptr) {
        OH_VideoDecoder_Destroy(codec_);
        codec_ = nullptr;
    }
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoder::SetCallback(CodecUserData *codecUserData)
{
    int32_t ret = AV_ERR_OK;
    if (runMode_ & 0b10) { // 0b10: AVBuffer mode mask
        ret = OH_VideoDecoder_RegisterCallback(codec_, AVCodecCallback, codecUserData);
    } else {
        ret = OH_VideoDecoder_SetCallback(codec_, AVCodecAsyncCallback, codecUserData);
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
    
    int ret = OH_VideoDecoder_Configure(codec_, format);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Config failed, ret: %{public}d", ret);
    OH_AVFormat_Destroy(format);
    format = nullptr;

    return AVCODEC_SAMPLE_ERR_OK;
}
} // Sample
} // MediaAVCodec
} // OHOS