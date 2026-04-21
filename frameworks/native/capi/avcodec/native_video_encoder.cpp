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

#include <cstdint>
#include <list>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_trace.h"
#include "avcodec_video_encoder.h"
#include "buffer/avsharedmemory.h"
#include "buffer_utils.h"
#include "common/native_mfmagic.h"
#include "native_avbuffer.h"
#include "native_avcodec_base.h"
#include "native_avcodec_videoencoder.h"
#include "native_avmagic.h"
#include "native_window.h"
#include "hiappevent_util.h"
#include "api_metrics_util.h"
#include "preprocessor_encoder.h"
#include "video_encoder_object.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "NativeVideoEncoder"};
constexpr size_t MAX_LENGTH = 255;
bool IsValidVideoEncoderMagic(AVMagic magic)
{
    return magic == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER ||
                    OHOS::MediaAVCodec::PreprocessorEncoder::IsPreprocEncoderMagic(magic);
}
}

namespace OHOS {
namespace MediaAVCodec {

#ifdef __cplusplus
extern "C" {
#endif

struct OH_AVCodec *OH_VideoEncoder_CreateByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "Mime is nullptr!");
    CHECK_AND_RETURN_RET_LOG(strlen(mime) < MAX_LENGTH, nullptr, "Mime is too long!");

    static AppEventReporter appEventReporter = AppEventReporter();
    ApiInvokeRecorder apiInvokeRecorder("OH_VideoEncoder_CreateByMime", appEventReporter);

    std::shared_ptr<AVCodecVideoEncoder> videoEncoder = VideoEncoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, nullptr, "Video encoder create by mime failed");

    struct VideoEncoderObject *object = new (std::nothrow) VideoEncoderObject(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Video encoder create by mime failed");

    return object;
}

struct OH_AVCodec *OH_VideoEncoder_CreateByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "Name is nullptr!");
    CHECK_AND_RETURN_RET_LOG(strlen(name) < MAX_LENGTH, nullptr, "Name is too long!");

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_CreateByName.Time", TIMES);
    static AppEventReporter appEventReporter = AppEventReporter();
    ApiInvokeRecorder apiInvokeRecorder("OH_VideoEncoder_CreateByName", appEventReporter);

    std::shared_ptr<AVCodecVideoEncoder> videoEncoder = VideoEncoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, nullptr, "Video encoder create by name failed");

    struct VideoEncoderObject *object = new (std::nothrow) VideoEncoderObject(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Video encoder create by name failed");

    return object;
}

OH_AVErrCode OH_VideoEncoder_Destroy(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");
    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_Destroy.Time", TIMES);
    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto *preprocEnc = reinterpret_cast<OHOS::MediaAVCodec::PreprocessorEncoder *>(codec);
        (void)preprocEnc->Release();
        if (!OHOS::MediaAVCodec::PreprocessorEncoder::IsPrimaryEncoderMagic(codec->magic_)) {
            preprocEnc->DetachFromPrimaryEncoder();
            delete preprocEnc;
            return AV_ERR_OK;
        }

        auto secondaryEnc = preprocEnc->DetachSecondaryEncoder();
        if (secondaryEnc) {
            delete secondaryEnc;
        }
        delete preprocEnc;
        return AV_ERR_OK;
    }

    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER,
                             AV_ERR_INVALID_VAL, "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);

    if (videoEncObj != nullptr && videoEncObj->videoEncoder_ != nullptr) {
        int32_t ret = videoEncObj->videoEncoder_->Release();
        videoEncObj->StopCallback();
        videoEncObj->ClearBufferList();
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGE("Video encoder destroy failed!");
            delete codec;
            return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
        }
    } else {
        AVCODEC_LOGD("Video encoder is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Configure(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "Format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL, "Format magic error!");

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_Configure.Time", TIMES);
    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        int32_t ret = preprocEnc->Configure(format->format_);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "PreprocessorEncoder configure failed!");
        return AV_ERR_OK;
    }
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t bitrateMode = -1;
    if (OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, &bitrateMode) &&
        bitrateMode == SQR) {
        int64_t bitrate;
        int64_t maxBitrate;
        bool bitrateExist = OH_AVFormat_GetLongValue(format, OH_MD_KEY_BITRATE, &bitrate);
        bool maxBitrateExist = OH_AVFormat_GetLongValue(format, OH_MD_KEY_MAX_BITRATE, &maxBitrate);
        if (bitrateExist && !maxBitrateExist) {
            AVCODEC_LOGW("In SQR bitrate mode, param %{public}s is not set, param %{public}s will be used instead",
                OH_MD_KEY_MAX_BITRATE, OH_MD_KEY_BITRATE);
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_MAX_BITRATE, bitrate);
        }
    }

    int32_t ret = videoEncObj->videoEncoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder configure failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Prepare(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");

    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        return AV_ERR_OK;
    }

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_Prepare.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder prepare failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Start(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_Start.Time", TIMES);
    static AppEventReporter appEventReporter = AppEventReporter();
    ApiInvokeRecorder apiInvokeRecorder("OH_VideoEncoder_Start", appEventReporter);

    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        auto ret = preprocEnc->Start();
        apiInvokeRecorder.SetErrorCode(ret);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "PreprocessorEncoder start failed!");
        return AV_ERR_OK;
    }

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Start();
    apiInvokeRecorder.SetErrorCode(AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "Video encoder start failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Stop(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");

    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        auto ret = preprocEnc->Stop();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "PreprocessorEncoder stop failed!");
        return AV_ERR_OK;
    }

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_Stop.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Stop();
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Video encoder stop failed");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    videoEncObj->ClearBufferList();
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Flush(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");

    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        auto ret = preprocEnc->Flush();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "PreprocessorEncoder flush failed!");
        return AV_ERR_OK;
    }

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_Flush.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Flush();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder flush failed!");
    videoEncObj->ClearBufferList();
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Reset(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");

    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        auto ret = preprocEnc->Reset();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "PreprocessorEncoder reset failed!");
        return AV_ERR_OK;
    }

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_Reset.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Reset();
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Video encoder reset failed");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    videoEncObj->ClearBufferList();
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_GetSurface(OH_AVCodec *codec, OHNativeWindow **window)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr && window != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(!PreprocessorEncoder::IsSecondaryEncoderMagic(codec->magic_), AV_ERR_OPERATE_NOT_PERMIT,
                             "Secondary encoder is not permitted to call GetSurface! Use primary encoder instead.");

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_GetSurface.Time", TIMES);
    if (PreprocessorEncoder::IsPrimaryEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        auto surface = preprocEnc->GetSurface();
        CHECK_AND_RETURN_RET_LOG(surface != nullptr, AV_ERR_OPERATE_NOT_PERMIT,
                                 "PreprocessorEncoder get surface failed!");
        *window = CreateNativeWindowFromSurface(&surface);
        CHECK_AND_RETURN_RET_LOG(*window != nullptr, AV_ERR_INVALID_VAL, "Video encoder get surface failed!");
        return AV_ERR_OK;
    }

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    OHOS::sptr<OHOS::Surface> surface = videoEncObj->videoEncoder_->CreateInputSurface();
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, AV_ERR_OPERATE_NOT_PERMIT, "Video encoder get surface failed!");

    *window = CreateNativeWindowFromSurface(&surface);
    CHECK_AND_RETURN_RET_LOG(*window != nullptr, AV_ERR_INVALID_VAL, "Video encoder get surface failed!");
    videoEncObj->isInputSurfaceMode_ = true;

    return AV_ERR_OK;
}

OH_AVFormat *OH_VideoEncoder_GetOutputDescription(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), nullptr, "Codec magic error!");

    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        return preprocEnc->GetOutputDescription();
    }

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_GetOutputDescription.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video encoder is nullptr!");

    Format format;
    int32_t ret = videoEncObj->videoEncoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Video encoder get output description failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Video encoder get output description failed!");
    avFormat->format_ = format;

    return avFormat;
}

OH_AVErrCode OH_VideoEncoder_FreeOutputData(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(!PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_), AV_ERR_OPERATE_NOT_PERMIT,
                             "RegisterParameterCallback is not supported for preprocessor encoders!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");
    CHECK_AND_RETURN_RET_LOG(videoEncObj->callback_ != nullptr, AV_ERR_INVALID_STATE,
                             "The callback of OH_AVMemory is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder free output data failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_FreeOutputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");

    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        auto ret = preprocEnc->ReleaseOutputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "PreprocessorEncoder free output buffer failed!");
        return AV_ERR_OK;
    }

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_FreeOutputBuffer.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");
    CHECK_AND_RETURN_RET_LOG(!videoEncObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
                             "Not support the callback of OH_AVMemory!");

    int32_t ret = videoEncObj->videoEncoder_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder free output data failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_NotifyEndOfStream(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");

    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        auto ret = preprocEnc->NotifyEos();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "PreprocessorEncoder notify eos failed!");
        return AV_ERR_OK;
    }

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_NotifyEndOfStream.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->NotifyEos();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder notify end of stream failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_SetParameter(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "Format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL, "Format magic error!");

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_SetParameter.Time", TIMES);
    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        CHECK_AND_RETURN_RET_LOG(preprocEnc != nullptr && preprocEnc->GetPreprocessor() != nullptr, AV_ERR_OK,
                                 "PreprocessorEncoder prepare skipped: no preprocessor!");
        return AV_ERR_OK;
    }

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder set parameter failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_SetCallback(struct OH_AVCodec *codec, struct OH_AVCodecAsyncCallback callback,
                                         void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(!PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_), AV_ERR_OPERATE_NOT_PERMIT,
                             "RegisterParameterCallback is not supported for preprocessor encoders!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    if (videoEncObj->callback_ != nullptr) {
        videoEncObj->callback_->UpdateCallback(callback, userData);
    } else {
        videoEncObj->callback_ = std::make_shared<NativeVideoEncoderCallback>(codec, callback, userData);
    }
    int32_t ret =
        videoEncObj->videoEncoder_->SetCallback(std::static_pointer_cast<AVCodecCallback>(videoEncObj->callback_));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder set callback failed!");
    videoEncObj->isSetMemoryCallback_ = true;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_RegisterCallback(struct OH_AVCodec *codec, struct OH_AVCodecCallback callback,
                                              void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(callback.onNewOutputBuffer != nullptr, AV_ERR_INVALID_VAL,
                             "Callback onNewOutputBuffer is nullptr");

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_RegisterCallback.Time", TIMES);
    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        auto ret = preprocEnc->RegisterCallback(callback, userData);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "PreprocessorEncoder reset failed!");
        return AV_ERR_OK;
    }

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    if (videoEncObj->callback_ != nullptr) {
        videoEncObj->callback_->UpdateCallback(callback, userData);
    } else {
        videoEncObj->callback_ = std::make_shared<NativeVideoEncoderCallback>(codec, callback, userData);
    }
    int32_t ret =
        videoEncObj->videoEncoder_->SetCallback(std::static_pointer_cast<MediaCodecCallback>(videoEncObj->callback_));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder register callback failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_RegisterParameterCallback(OH_AVCodec *codec,
                                                       OH_VideoEncoder_OnNeedInputParameter onInputParameter,
                                                       void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(!PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_), AV_ERR_OPERATE_NOT_PERMIT,
                             "RegisterParameterCallback is not supported for preprocessor encoders!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(onInputParameter != nullptr, AV_ERR_INVALID_VAL, "Callback onInputParameter is nullptr");

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_RegisterParameterCallback.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    if (videoEncObj->callback_ != nullptr) {
        videoEncObj->callback_->UpdateCallback(onInputParameter, userData);
    } else {
        videoEncObj->callback_ = std::make_shared<NativeVideoEncoderCallback>(codec, onInputParameter, userData);
    }
    int32_t ret = videoEncObj->videoEncoder_->SetCallback(
        std::static_pointer_cast<MediaCodecParameterCallback>(videoEncObj->callback_));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder register parameter callback failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_PushInputData(struct OH_AVCodec *codec, uint32_t index, OH_AVCodecBufferAttr attr)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_), AV_ERR_OPERATE_NOT_PERMIT,
        "Buffer mode is not supported for preprocessor encoders!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");
    CHECK_AND_RETURN_RET_LOGD(attr.size >= 0, AV_ERR_INVALID_VAL, "Invalid buffer size!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(videoEncObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
        "The callback of OH_AVMemory is nullptr!");

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.pts;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    enum AVCodecBufferFlag bufferFlag = static_cast<enum AVCodecBufferFlag>(attr.flags);

    int32_t ret = videoEncObj->videoEncoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOGD(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "Video encoder push input data failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_PushInputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_), AV_ERR_OPERATE_NOT_PERMIT,
        "Buffer mode is not supported for preprocessor encoders!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "magic error!");

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_PushInputBuffer.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!videoEncObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
        "Not support the callback of OH_AVMemory!");
    int32_t ret = videoEncObj->videoEncoder_->QueueInputBuffer(index);
    CHECK_AND_RETURN_RET_LOGD(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "videoEncoder QueueInputBuffer failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_PushInputParameter(OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_), AV_ERR_OPERATE_NOT_PERMIT,
        "Buffer mode is not supported for preprocessor encoders!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "magic error!");

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_PushInputParameter.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->QueueInputParameter(index);
    CHECK_AND_RETURN_RET_LOGD(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "videoEncoder QueueInputParameter failed!");
    return AV_ERR_OK;
}

OH_AVFormat *OH_VideoEncoder_GetInputDescription(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), nullptr, "Codec magic error!");

    if (PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_)) {
        auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
        return preprocEnc->GetInputDescription();
    }

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_GetInputDescription.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video encoder is nullptr!");

    Format format;
    int32_t ret = videoEncObj->videoEncoder_->GetInputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Video encoder get input description failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Video encoder get input description failed!");
    avFormat->format_ = format;

    return avFormat;
}

OH_AVErrCode OH_VideoEncoder_QueryInputBuffer(struct OH_AVCodec *codec, uint32_t *index, int64_t timeoutUs)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_), AV_ERR_OPERATE_NOT_PERMIT,
        "Synchronous mode is not supported for preprocessor encoders!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    ApiMetricsReporter("AVCodecKit.OH_VideoEncoder_QueryInputBuffer.Time", TIMES);
    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");
    int32_t ret = videoEncObj->videoEncoder_->QueryInputBuffer(*index, timeoutUs);
    switch (ret) {
        case AVCS_ERR_TRY_AGAIN:
            return AV_ERR_TRY_AGAIN_LATER;
        case AVCS_ERR_OK:
            return AV_ERR_OK;
        default:
            AVCODEC_LOGD("Video encoder query input data failed!");
    }
    return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
}

OH_AVErrCode OH_VideoEncoder_QueryOutputBuffer(struct OH_AVCodec *codec, uint32_t *index, int64_t timeoutUs)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_), AV_ERR_OPERATE_NOT_PERMIT,
        "Synchronous mode is not supported for preprocessor encoders!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");
    int32_t ret = videoEncObj->videoEncoder_->QueryOutputBuffer(*index, timeoutUs);
    switch (ret) {
        case AVCS_ERR_OK:
            return AV_ERR_OK;
        case AVCS_ERR_TRY_AGAIN:
            return AV_ERR_TRY_AGAIN_LATER;
        case AVCS_ERR_STREAM_CHANGED:
            return AV_ERR_STREAM_CHANGED;
        default:
            AVCODEC_LOGD("Video encoder query output data failed!");
    }
    return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
}

OH_AVBuffer *OH_VideoEncoder_GetInputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_), nullptr,
        "Synchronous mode is not supported for preprocessor encoders!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, nullptr, "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video encoder is nullptr!");

    std::shared_ptr<AVBuffer> buffer = videoEncObj->videoEncoder_->GetInputBuffer(index);
    CHECK_AND_RETURN_RET_LOGD(buffer != nullptr, nullptr, "Buffer is nullptr, idx:%{public}u", index);

    return videoEncObj->GetTransData(index, buffer, false);
}

OH_AVBuffer *OH_VideoEncoder_GetOutputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!PreprocessorEncoder::IsPreprocEncoderMagic(codec->magic_), nullptr,
        "Synchronous mode is not supported for preprocessor encoders!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, nullptr, "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video encoder is nullptr!");

    std::shared_ptr<AVBuffer> buffer = videoEncObj->videoEncoder_->GetOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOGD(buffer != nullptr, nullptr, "Buffer is nullptr, idx:%{public}u", index);

    return videoEncObj->GetTransData(index, buffer, true);
}

OH_AVErrCode OH_VideoEncoder_IsValid(OH_AVCodec *codec, bool *isValid)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidVideoEncoderMagic(codec->magic_), AV_ERR_INVALID_VAL, "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(isValid != nullptr, AV_ERR_INVALID_VAL, "Input isValid is nullptr!");
    *isValid = true;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_CreatePrimaryWithPreproc(const char *mime, OH_AVCodec **codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr! It must not be nullptr!");
    CHECK_AND_RETURN_RET_LOG(*codec == nullptr, AV_ERR_INVALID_VAL,
        "The pointer stored in codec is not nullptr! It must be nullptr!");
    CHECK_AND_RETURN_RET_LOG(PreprocessorEncoder::IsValidPreprocEncoderMime(mime), AV_ERR_INVALID_VAL,
        "Invalid mime, only OH_AVCODEC_MIMETYPE_VIDEO_HEVC and OH_AVCODEC_MIMETYPE_VIDEO_AVC are supported!");

    auto ret = PreprocessorEncoder::CreatePrimary(mime, codec);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Create primary codec failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_CreateSecondaryFromPrimary(OH_AVCodec *primary, OH_AVCodec **codec)
{
    CHECK_AND_RETURN_RET_LOG(primary != nullptr, AV_ERR_INVALID_VAL, "Primary is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL,
        "Codec is nullptr! It must not be nullptr!");
    CHECK_AND_RETURN_RET_LOG(*codec == nullptr, AV_ERR_INVALID_VAL,
        "The pointer stored in codec is not nullptr! It must be nullptr!");
    CHECK_AND_RETURN_RET_LOG(PreprocessorEncoder::IsPrimaryEncoderMagic(primary->magic_), AV_ERR_INVALID_VAL,
        "Primary magic error!");

    auto ret = PreprocessorEncoder::CreateSecondary(primary, codec);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Create secondary codec failed!");
    return AV_ERR_OK;
}

#ifdef __cplusplus
}
#endif

} // namespace MediaAVCodec
} // namespace OHOS
