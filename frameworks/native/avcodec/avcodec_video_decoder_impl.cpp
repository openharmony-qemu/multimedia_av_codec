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

#include "avcodec_video_decoder_impl.h"
#include "avcodec_trace.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "common/log.h"
#include "i_avcodec_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecVideoDecoderImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<AVCodecVideoDecoder> VideoDecoderFactory::CreateByMime(const std::string &mime)
{
    std::shared_ptr<AVCodecVideoDecoder> impl = nullptr;
    Format format;

    int32_t ret = CreateByMime(mime, format, impl);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK || impl != nullptr, nullptr,
        "AVCodec video decoder impl init failed, %{public}s",
        AVCSErrorToString(static_cast<AVCodecServiceErrCode>(ret)).c_str());

    return impl;
}

std::shared_ptr<AVCodecVideoDecoder> VideoDecoderFactory::CreateByName(const std::string &name)
{
    std::shared_ptr<AVCodecVideoDecoder> impl = nullptr;
    Format format;

    int32_t ret = CreateByName(name, format, impl);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK || impl != nullptr, nullptr,
        "AVCodec video decoder impl init failed, %{public}s",
        AVCSErrorToString(static_cast<AVCodecServiceErrCode>(ret)).c_str());

    return impl;
}

int32_t VideoDecoderFactory::CreateByMime(const std::string &mime,
                                          Format &format, std::shared_ptr<AVCodecVideoDecoder> &decoder)
{
    auto impl = std::make_shared<AVCodecVideoDecoderImpl>();

    int32_t ret = impl->Init(AVCODEC_TYPE_VIDEO_DECODER, true, mime, format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "AVCodec video decoder impl init failed, %{public}s",
                             AVCSErrorToString(static_cast<AVCodecServiceErrCode>(ret)).c_str());

    decoder = impl;
    return AVCS_ERR_OK;
}

int32_t VideoDecoderFactory::CreateByName(const std::string &name,
                                          Format &format, std::shared_ptr<AVCodecVideoDecoder> &decoder)
{
    auto impl = std::make_shared<AVCodecVideoDecoderImpl>();

    int32_t ret = impl->Init(AVCODEC_TYPE_VIDEO_DECODER, false, name, format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "AVCodec video decoder impl init failed, %{public}s",
                             AVCSErrorToString(static_cast<AVCodecServiceErrCode>(ret)).c_str());

    decoder = impl;
    return AVCS_ERR_OK;
}

int32_t AVCodecVideoDecoderImpl::Init(AVCodecType type, bool isMimeType, const std::string &name, Format &format)
{
    AVCODEC_SYNC_TRACE;

    int32_t ret = AVCodecServiceFactory::GetInstance().CreateCodecService(codecClient_);
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, ret, "Codec client create failed");

    return codecClient_->Init(type, isMimeType, name, *format.GetMeta());
}

AVCodecVideoDecoderImpl::AVCodecVideoDecoderImpl()
{
    AVCODEC_LOGD("AVCodecVideoDecoderImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecVideoDecoderImpl::~AVCodecVideoDecoderImpl()
{
    if (codecClient_ != nullptr) {
        (void)AVCodecServiceFactory::GetInstance().DestroyCodecService(codecClient_);
        codecClient_ = nullptr;
    }
    AVCODEC_LOGD("AVCodecVideoDecoderImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecVideoDecoderImpl::Configure(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->Configure(format);
}

int32_t AVCodecVideoDecoderImpl::Prepare()
{
    return AVCS_ERR_OK;
}

int32_t AVCodecVideoDecoderImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->Start();
}

int32_t AVCodecVideoDecoderImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->Stop();
}

int32_t AVCodecVideoDecoderImpl::Flush()
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->Flush();
}

int32_t AVCodecVideoDecoderImpl::Reset()
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->Reset();
}

int32_t AVCodecVideoDecoderImpl::Release()
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->Release();
}

int32_t AVCodecVideoDecoderImpl::SetOutputSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I("AVCodecVideoDecoderImpl::SetOutputSurface");
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->SetOutputSurface(surface);
}

int32_t AVCodecVideoDecoderImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->QueueInputBuffer(index, info, flag);
}

int32_t AVCodecVideoDecoderImpl::QueueInputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->QueueInputBuffer(index);
}

int32_t AVCodecVideoDecoderImpl::GetOutputFormat(Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->GetOutputFormat(format);
}

int32_t AVCodecVideoDecoderImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->ReleaseOutputBuffer(index, render);
}

int32_t AVCodecVideoDecoderImpl::SetParameter(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->SetParameter(format);
}

int32_t AVCodecVideoDecoderImpl::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_INVALID_VAL, "Callback is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->SetCallback(callback);
}

int32_t AVCodecVideoDecoderImpl::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_INVALID_VAL, "Callback is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->SetCallback(callback);
}

#ifdef SUPPORT_DRM
int32_t AVCodecVideoDecoderImpl::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
    const bool svpFlag)
{
    AVCODEC_LOGI("AVCodecVideoDecoderImpl SetDecryptConfig proxy");
    CHECK_AND_RETURN_RET_LOG(codecClient_ != nullptr,
        AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");
    CHECK_AND_RETURN_RET_LOG(keySessionProxy != nullptr,
        AVCS_ERR_INVALID_OPERATION, "keySessionProxy is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecClient_->SetDecryptConfig(keySessionProxy, svpFlag);
}
#endif
} // namespace MediaAVCodec
} // namespace OHOS
