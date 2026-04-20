/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "preprocessor_encoder.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "media_core.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "PreprocessorEncoder"};
constexpr size_t MAX_LENGTH = 255;
} // namespace

namespace OHOS {
namespace MediaAVCodec {

// Reverse mapping: GraphicPixelFormat (Surface/Buffer) → VideoPixelFormat (Codec internal)
// Returns true on success, false if format is unknown.
// Mapping table derived from Preprocessor::ConvertVideoPixelFormat2GraphicPixelFormat (preprocessor.cpp:406)
static bool GraphicPixelFmtToVideoPixelFmt(GraphicPixelFormat graphicFmt, VideoPixelFormat& outVideoFmt)
{
    switch (graphicFmt) {
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P:
            outVideoFmt = VideoPixelFormat::YUVI420;
            return true;
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888:
            outVideoFmt = VideoPixelFormat::RGBA;
            return true;
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
            outVideoFmt = VideoPixelFormat::NV12;
            return true;
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
            outVideoFmt = VideoPixelFormat::NV21;
            return true;
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_1010102:
            outVideoFmt = VideoPixelFormat::RGBA1010102;
            return true;
        // 10-bit formats (HEVC Main10 profile)
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010:
            outVideoFmt = VideoPixelFormat::NV12;  // P010 maps to NV12 in codec context
            return true;
        default:
            return false;
    }
}

} // namespace MediaAVCodec
} // namespace OHOS

namespace OHOS {
namespace MediaAVCodec {

bool PreprocessorEncoder::IsPrimaryEncoderMagic(AVMagic magic)
{
    return magic == AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER;
}

bool PreprocessorEncoder::IsSecondaryEncoderMagic(AVMagic magic)
{
    return magic == AVMagic::AVCODEC_MAGIC_SECONDARY_VIDEO_ENCODER;
}

bool PreprocessorEncoder::IsPreprocEncoderMagic(AVMagic magic)
{
    return IsPrimaryEncoderMagic(magic) || IsSecondaryEncoderMagic(magic);
}

bool PreprocessorEncoder::IsValidPreprocEncoderMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, false, "Mime is nullptr!");
    size_t len = strnlen(mime, MAX_LENGTH);
    CHECK_AND_RETURN_RET_LOG(len > 0 && len <= MAX_LENGTH, false, "Mime is too long!");
    std::string mimeStr(mime);
    return mimeStr == OH_AVCODEC_MIMETYPE_VIDEO_HEVC || mimeStr == OH_AVCODEC_MIMETYPE_VIDEO_AVC;
}

std::string PreprocessorEncoder::GenerateEncoderId(const std::string &prefix)
{
    static std::atomic<uint64_t> counter{0};
    uint64_t id = counter.fetch_add(1);
    return prefix + "_" + std::to_string(id);
}

PreprocessorEncoder::PreprocessorEncoder(AVMagic magic) : OH_AVCodec(magic)
{
    AVCODEC_LOGI("PreprocessorEncoder created");
}

PreprocessorEncoder::~PreprocessorEncoder()
{
    DetachPeerEncoder();
    UnregisterEncoderFromManager();
    AVCODEC_LOGI("PreprocessorEncoder destroyed");
}

OH_AVErrCode PreprocessorEncoder::CreatePrimary(const char *mime, OH_AVCodec **codec)
{
    auto encoder = std::make_unique<PreprocessorEncoder>(AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER);
    CHECK_AND_RETURN_RET_LOG(encoder != nullptr, AV_ERR_NO_MEMORY, "PreprocessorEncoder create failed!");

    OH_AVErrCode ret = encoder->InitAsPrimary(mime);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "InitAsPrimary failed!");

    *codec = encoder.release();
    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::CreateSecondary(OH_AVCodec *primary, OH_AVCodec **secondary)
{
    auto *primaryEnc = reinterpret_cast<PreprocessorEncoder *>(primary);
    CHECK_AND_RETURN_RET_LOG(primaryEnc->magic_ == AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Primary magic error!");

    CHECK_AND_RETURN_RET_LOG(primaryEnc->secondary_ == nullptr, AV_ERR_OPERATE_NOT_PERMIT,
                             "Secondary encoder already exists!");

    auto encoder = std::make_unique<PreprocessorEncoder>(AVMagic::AVCODEC_MAGIC_SECONDARY_VIDEO_ENCODER);
    CHECK_AND_RETURN_RET_LOG(encoder != nullptr, AV_ERR_NO_MEMORY, "PreprocessorEncoder create failed!");

    OH_AVErrCode ret = encoder->InitAsSecondary(primaryEnc);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "InitAsSecondary failed!");

    *secondary = encoder.release();
    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::InitAsPrimary(const char *mime)
{
    isPrimary_ = true;
    mime_ = mime;
    encoderId_ = GenerateEncoderId("primary");

    auto videoEncoder = VideoEncoderFactory::CreateByMime(mime_);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, AV_ERR_NO_MEMORY, "VideoEncoder create failed!");

    videoEncoderObject_ = std::make_shared<VideoEncoderObject>(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(videoEncoderObject_ != nullptr, AV_ERR_NO_MEMORY, "VideoEncoderObject create failed!");

    preprocMgr_ = std::make_shared<PreprocessorManager>();
    CHECK_AND_RETURN_RET_LOG(preprocMgr_ != nullptr, AV_ERR_NO_MEMORY, "PreprocessorManager create failed!");

    preprocMgr_->CreateSharedSurface();

    auto preprocessor = std::make_shared<Preprocessor>(mime);
    CHECK_AND_RETURN_RET_LOG(preprocessor != nullptr, AV_ERR_NO_MEMORY, "Preprocessor create failed!");
    preprocessors_[encoderId_] = preprocessor;

    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::InitAsSecondary(PreprocessorEncoder *primary)
{
    isPrimary_ = false;
    mime_ = primary->mime_;
    encoderId_ = GenerateEncoderId("secondary");

    auto videoEncoder = VideoEncoderFactory::CreateByMime(mime_);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, AV_ERR_NO_MEMORY, "VideoEncoder create failed!");

    videoEncoderObject_ = std::make_shared<VideoEncoderObject>(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(videoEncoderObject_ != nullptr, AV_ERR_NO_MEMORY, "VideoEncoderObject create failed!");

    preprocMgr_ = primary->preprocMgr_;

    auto preprocessor = std::make_shared<Preprocessor>(mime_);
    CHECK_AND_RETURN_RET_LOG(preprocessor != nullptr, AV_ERR_NO_MEMORY, "Preprocessor create failed!");
    preprocessors_[encoderId_] = preprocessor;

    primary_ = primary;
    primary_->secondary_ = this;

    return AV_ERR_OK;
}

void PreprocessorEncoder::CreateSharedSurface()
{
    if (preprocMgr_) {
        preprocMgr_->CreateSharedSurface();
    }
}

sptr<Surface> PreprocessorEncoder::GetSharedSurface()
{
    if (preprocMgr_) {
        return preprocMgr_->GetSharedSurface();
    }
    return nullptr;
}

void PreprocessorEncoder::RegisterEncoderToManager()
{
    if (preprocMgr_) {
        auto preprocessor = preprocessors_[encoderId_];
        preprocMgr_->RegisterEncoder(encoderId_, preprocessor, encoderSurface_);
    }
}

void PreprocessorEncoder::UnregisterEncoderFromManager()
{
    if (preprocMgr_) {
        preprocMgr_->UnregisterEncoder(encoderId_);
    }
}

void PreprocessorEncoder::DetachPeerEncoder()
{
    if (isPrimary_) {
        auto *secondary = secondary_;
        secondary_ = nullptr;
        if (secondary != nullptr && secondary->primary_ == this) {
            secondary->primary_ = nullptr;
        }
        return;
    }

    DetachFromPrimaryEncoder();
}

PreprocessorEncoder* PreprocessorEncoder::DetachSecondaryEncoder()
{
    auto *secondary = secondary_;
    secondary_ = nullptr;
    if (secondary != nullptr && secondary->primary_ == this) {
        secondary->primary_ = nullptr;
    }
    return secondary;
}

void PreprocessorEncoder::DetachFromPrimaryEncoder()
{
    auto *primary = primary_;
    primary_ = nullptr;
    if (primary != nullptr && primary->secondary_ == this) {
        primary->secondary_ = nullptr;
    }
}

OH_AVErrCode PreprocessorEncoder::RegisterCallback(struct OH_AVCodecCallback callback, void *userData)
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        if (videoEncoderObject_->callback_ != nullptr) {
            videoEncoderObject_->callback_->UpdateCallback(callback, userData);
        } else {
            videoEncoderObject_->callback_ = std::make_shared<NativeVideoEncoderCallback>(this, callback, userData);
        }
        int32_t ret = videoEncoderObject_->videoEncoder_->SetCallback(
            std::static_pointer_cast<MediaCodecCallback>(videoEncoderObject_->callback_));
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Video encoder register callback failed!");
    }
    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::Configure(const Media::Format &format)
{
    int32_t ret = ConfigureVideoEncoder(format);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Configure video encoder failed!");

    ret = ConfigurePreprocessor(format);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Configure preprocessor failed!");

    if (!isPrimary_ && primary_ != nullptr) {
        auto secondaryPreprocessor = GetPreprocessor();
        auto primaryPreprocessor = primary_->GetPreprocessor();
        if (secondaryPreprocessor && primaryPreprocessor) {
            GraphicPixelFormat secFmt = secondaryPreprocessor->GetConfiguredPixelFormat();
            GraphicPixelFormat priFmt = primaryPreprocessor->GetConfiguredPixelFormat();
            if (secFmt != priFmt && priFmt != GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT) {
                AVCODEC_LOGE("PixelFormat mismatch: primary=%{public}d, secondary=%{public}d",
                                static_cast<int32_t>(priFmt), static_cast<int32_t>(secFmt));
                return AVCSErrorToOHAVErrCode(AVCS_ERR_INVALID_VAL);
            }
        }
    }
    ret = CreateEncoderSurface();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "Create internal surface failed!");

    RegisterEncoderToManager();

    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::SetParameter(const Media::Format &format)
{
    auto preprocessor = GetPreprocessor();
    if (preprocessor) {
        int32_t ret = preprocessor->Configure(format);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "PreprocessorEncoder set preprocessor parameter failed!");
    }

    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->SetParameter(format);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "PreprocessorEncoder set parameter failed!");
    }
    return AV_ERR_OK;
}

sptr<Surface> PreprocessorEncoder::GetSurface()
{
    return GetSharedSurface();
}

int32_t PreprocessorEncoder::ConfigureVideoEncoder(const Media::Format &format)
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t bitrateMode = -1;
        bool bitrateModeExist = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, bitrateMode);
        if (bitrateModeExist && bitrateMode == SQR) {
            int64_t bitrate;
            int64_t maxBitrate;
            bool bitrateExist = format.GetLongValue(OH_MD_KEY_BITRATE, bitrate);
            bool maxBitrateExist = format.GetLongValue(OH_MD_KEY_MAX_BITRATE, maxBitrate);
            if (bitrateExist && !maxBitrateExist) {
                AVCODEC_LOGW("In SQR bitrate mode, param %{public}s is not set, param %{public}s will be used instead",
                             OH_MD_KEY_MAX_BITRATE, OH_MD_KEY_BITRATE);
                Media::Format formatTemp = format;
                formatTemp.PutLongValue(OH_MD_KEY_MAX_BITRATE, bitrate);
                return videoEncoderObject_->videoEncoder_->Configure(formatTemp);
            }
        }
        int32_t ret = videoEncoderObject_->videoEncoder_->Configure(format);
        return ret;
    }
    return AVCS_ERR_OK;
}

int32_t PreprocessorEncoder::ConfigurePreprocessor(const Media::Format &format)
{
    auto preprocessor = GetPreprocessor();
    if (preprocessor) {
        return preprocessor->Configure(format);
    }
    return AVCS_ERR_OK;
}

int32_t PreprocessorEncoder::CreateEncoderSurface()
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        auto surface = videoEncoderObject_->videoEncoder_->CreateInputSurface();
        if (surface == nullptr) {
            return AVCS_ERR_NO_MEMORY;
        }

        encoderSurface_ = surface;
    }

    return AVCS_ERR_OK;
}

std::shared_ptr<Preprocessor> PreprocessorEncoder::GetPreprocessor()
{
    auto it = preprocessors_.find(encoderId_);
    if (it != preprocessors_.end()) {
        return it->second;
    }
    return nullptr;
}

OH_AVErrCode PreprocessorEncoder::Start()
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->Start();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Video encoder start failed!");
    }
    auto preprocessor = GetPreprocessor();
    if (preprocessor) {
        preprocessor->FlushTimeStamp();
    }
    if (preprocMgr_) {
        preprocMgr_->SetEncoderRunning(encoderId_, true);
    }

    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::Stop()
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->Stop();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Video encoder stop failed!");
    }

    if (preprocMgr_) {
        preprocMgr_->SetEncoderRunning(encoderId_, false);
    }

    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::Flush()
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->Flush();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Video encoder flush failed!");
    }

    if (preprocMgr_) {
        preprocMgr_->SetEncoderRunning(encoderId_, false);
    }

    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::Reset()
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->Reset();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Video encoder reset failed!");
    }

    if (preprocMgr_) {
        preprocMgr_->SetEncoderRunning(encoderId_, false);
    }

    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::Release()
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->Release();
        videoEncoderObject_->StopCallback();
        videoEncoderObject_->ClearBufferList();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "PreprocessorEncoder release failed!");
    }

    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::NotifyEos()
{
    if (preprocMgr_) {
        std::shared_ptr<VideoEncoderObject> videoEncoderObject = videoEncoderObject_;
        std::string encoderId = encoderId_;
        bool requestRet = preprocMgr_->RequestNotifyEos(encoderId_, [videoEncoderObject, encoderId]() {
            if (!(videoEncoderObject && videoEncoderObject->videoEncoder_)) {
                AVCODEC_LOGE("Video encoder is nullptr when async notify eos: %{public}s", encoderId.c_str());
                return;
            }
            int32_t ret = videoEncoderObject->videoEncoder_->NotifyEos();
            if (ret != AV_ERR_OK) {
                AVCODEC_LOGE("Async notify eos failed for %{public}s, ret=%{public}d", encoderId.c_str(), ret);
            }
        });
        CHECK_AND_RETURN_RET_LOG(requestRet, AV_ERR_INVALID_STATE, "Request async notify eos failed!");
        return AV_ERR_OK;
    }

    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::ReleaseOutputBuffer(uint32_t index)
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->ReleaseOutputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Preprocessor release output buffer failed!");
    }
    return AV_ERR_OK;
}

OH_AVFormat* PreprocessorEncoder::GetOutputDescription()
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        Format format;
        int32_t ret = videoEncoderObject_->videoEncoder_->GetOutputFormat(format);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr,
                                 "PreprocessorEncoder get output description failed!");

        OH_AVFormat* avFormat = OH_AVFormat_Create();
        CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "PreprocessorEncoder create output format failed!");
        avFormat->format_ = format;
        return avFormat;
    }
    return nullptr;
}

OH_AVFormat* PreprocessorEncoder::GetInputDescription()
{
    auto preprocessor = GetPreprocessor();
    CHECK_AND_RETURN_RET_LOG(preprocessor != nullptr, nullptr, "Preprocessor not found for GetInputDescription!");

    Format format;
    // Step 1: Obtain underlying encoder InputDescription (width, height, pixel_format, stride, etc.)
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        Format encoderFormat;
        int32_t ret = videoEncoderObject_->videoEncoder_->GetInputFormat(encoderFormat);
        if (ret == AVCS_ERR_OK) {
            format = encoderFormat;  // Copy entire encoder format as base
        }
    }
    // Step 2: Override input data fields from PreprocessorManager buffer (width, height, pixel_format, stride)
    if (preprocMgr_) {
        PreprocessorManager::BufferDataInfo bufInfo;
        if (preprocMgr_->PeekPendingBufferData(encoderId_, bufInfo)) {
            format.PutIntValue(Media::Tag::VIDEO_WIDTH, bufInfo.width);
            format.PutIntValue(Media::Tag::VIDEO_HEIGHT, bufInfo.height);
            format.PutIntValue(OHOS::Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT, bufInfo.format);
            VideoPixelFormat mappedFmt;
            if (GraphicPixelFmtToVideoPixelFmt(static_cast<GraphicPixelFormat>(bufInfo.format), mappedFmt)) {
                format.PutIntValue(Media::Tag::VIDEO_PIXEL_FORMAT, static_cast<int32_t>(mappedFmt));
            }
            if (bufInfo.stride > 0) {
                format.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, bufInfo.stride);
            }
        }
    }
    // Step 3: Append preprocessing metadata (always)
    if (preprocessor->IsCropEnabled()) {
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, preprocessor->GetCropLeft());
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, preprocessor->GetCropRight());
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, preprocessor->GetCropTop());
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, preprocessor->GetCropBottom());
    }
    if (preprocessor->IsDownsamplingEnabled()) {
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH, preprocessor->GetDownsamplingWidth());
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT, preprocessor->GetDownsamplingHeight());
    }

    OH_AVFormat* avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "PreprocessorEncoder create input format failed!");
    avFormat->format_ = format;
    return avFormat;
}

} // namespace MediaAVCodec
} // namespace OHOS
