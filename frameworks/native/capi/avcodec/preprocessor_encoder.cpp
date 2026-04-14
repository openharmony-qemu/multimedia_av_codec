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
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "media_core.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "PreprocessorEncoder"};
constexpr size_t MAX_LENGTH = 255;
// constexpr int32_t BUFFER_REQUEST_STRIDE_ALIGNMENT = 32;
}

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

bool PreprocessorEncoder::IsValidPreprocEncoderMime(const char* mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, false, "Mime is nullptr!");
    size_t len = strnlen(mime, MAX_LENGTH);
    CHECK_AND_RETURN_RET_LOG(len > 0 && len <= MAX_LENGTH, false, "Mime is too long!");
    std::string mimeStr(mime);
    return mimeStr == OH_AVCODEC_MIMETYPE_VIDEO_HEVC || mimeStr == OH_AVCODEC_MIMETYPE_VIDEO_AVC;
}

std::string PreprocessorEncoder::GenerateEncoderId(const std::string& prefix)
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
    UnregisterEncoderFromManager();
    if (preprocMgr_) {
        preprocMgr_->Stop();
    }
    AVCODEC_LOGI("PreprocessorEncoder destroyed");
}

OH_AVErrCode PreprocessorEncoder::CreatePrimary(const char* mime, OH_AVCodec** codec)
{
    auto encoder = std::make_unique<PreprocessorEncoder>(AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER);
    CHECK_AND_RETURN_RET_LOG(encoder != nullptr, AV_ERR_NO_MEMORY, "PreprocessorEncoder create failed!");

    OH_AVErrCode ret = encoder->InitAsPrimary(mime);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "InitAsPrimary failed!");

    *codec = encoder.release();
    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::CreateSecondary(OH_AVCodec* primary, OH_AVCodec** secondary)
{
    auto* primaryEnc = reinterpret_cast<PreprocessorEncoder*>(primary);
    CHECK_AND_RETURN_RET_LOG(primaryEnc->magic_ == AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER,
                             AV_ERR_INVALID_VAL, "Primary magic error!");

    CHECK_AND_RETURN_RET_LOG(primaryEnc->secondary_ == nullptr, AV_ERR_OPERATE_NOT_PERMIT,
                             "Secondary encoder already exists!");

    auto encoder = std::make_unique<PreprocessorEncoder>(AVMagic::AVCODEC_MAGIC_SECONDARY_VIDEO_ENCODER);
    CHECK_AND_RETURN_RET_LOG(encoder != nullptr, AV_ERR_NO_MEMORY, "PreprocessorEncoder create failed!");

    OH_AVErrCode ret = encoder->InitAsSecondary(primaryEnc);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "InitAsSecondary failed!");

    *secondary = encoder.release();
    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::InitAsPrimary(const char* mime)
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

OH_AVErrCode PreprocessorEncoder::InitAsSecondary(PreprocessorEncoder* primary)
{
    isPrimary_ = false;
    mime_ = primary->mime_;
    encoderId_ = GenerateEncoderId("secondary");

    primary_ = primary;
    primary_->secondary_ = this;

    auto videoEncoder = VideoEncoderFactory::CreateByMime(mime_);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, AV_ERR_NO_MEMORY, "VideoEncoder create failed!");

    videoEncoderObject_ = std::make_shared<VideoEncoderObject>(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(videoEncoderObject_ != nullptr, AV_ERR_NO_MEMORY, "VideoEncoderObject create failed!");

    preprocMgr_ = primary->preprocMgr_;

    auto preprocessor = std::make_shared<Preprocessor>(mime_);
    CHECK_AND_RETURN_RET_LOG(preprocessor != nullptr, AV_ERR_NO_MEMORY, "Preprocessor create failed!");
    preprocessors_[encoderId_] = preprocessor;

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

OH_AVErrCode PreprocessorEncoder::RegisterCallback(struct OH_AVCodecCallback callback, void* userData)
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        if (videoEncoderObject_->callback_ != nullptr) {
            videoEncoderObject_->callback_->UpdateCallback(callback, userData);
        } else {
            videoEncoderObject_->callback_ = std::make_shared<NativeVideoEncoderCallback>(videoEncoderObject_.get(),
                                                                                          callback,
                                                                                          userData);
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

    ret = CreateEncoderSurface();
    CHECK_AND_RETURN_RET_LOG(ret != AVCS_ERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "Create internal surface failed!");

    RegisterEncoderToManager();

    return AV_ERR_OK;
}

sptr<Surface> PreprocessorEncoder::GetSurface()
{
    return GetSharedSurface();
}

int32_t PreprocessorEncoder::ConfigureVideoEncoder(const Media::Format& format)
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->Configure(format);
        return ret;
    }
    return AVCS_ERR_OK;
}

int32_t PreprocessorEncoder::ConfigurePreprocessor(const Media::Format& format)
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

        // auto preprocessor = GetPreprocessor();
        // if (preprocessor)
        // {
        //     int32_t queueSize = surface->GetQueueSize();
        //     for (int32_t i = 0; i < queueSize; i++)
        //     {
        //         sptr<BufferRequestConfig> config = new BufferRequestConfig();
        //         config->width = preprocessor->GetConfiguredWidth();
        //         config->height = preprocessor->GetConfiguredHeight();
        //         config->strideAlignment = BUFFER_REQUEST_STRIDE_ALIGNMENT;
        //         config->format = static_cast<int32_t>(preprocessor->GetConfiguredPixelFormat());
        //         config->timeout = 0;

        //         sptr<SurfaceBuffer> buffer{nullptr};
        //         int32_t fence;
        //         GSError ret = surface->RequestBuffer(buffer, fence, config);
        //         if (ret == GSERROR_OK && buffer != nullptr)
        //         {
        //             writableBufferQueue_.push(buffer);
        //         }
        //     }
        // }

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

    if (preprocMgr_) {
        int32_t ret = preprocMgr_->Start();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "PreprocessorManager start failed!");

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

OH_AVErrCode PreprocessorEncoder::NotifyEos()
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->NotifyEos();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Preprocessor notify eos failed!");
    }

    if (preprocMgr_) {
        preprocMgr_->SetEncoderRunning(encoderId_, false);
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

} // namespace MediaAVCodec
} // namespace OHOS