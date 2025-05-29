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

#include "avcodec_video_encoder_impl.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_trace.h"
#include "i_avcodec_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecVideoEncoderImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<AVCodecVideoEncoder> VideoEncoderFactory::CreateByMime(const std::string &mime)
{
    std::shared_ptr<AVCodecVideoEncoder> impl = nullptr;
    Format format;

    int32_t ret = CreateByMime(mime, format, impl);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK || impl != nullptr, nullptr,
                             "AVCodec video encoder impl init failed, %{public}s",
                             AVCSErrorToString(static_cast<AVCodecServiceErrCode>(ret)).c_str());

    return impl;
}

std::shared_ptr<AVCodecVideoEncoder> VideoEncoderFactory::CreateByName(const std::string &name)
{
    std::shared_ptr<AVCodecVideoEncoder> impl = nullptr;
    Format format;

    int32_t ret = CreateByName(name, format, impl);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK || impl != nullptr, nullptr,
                             "AVCodec video encoder impl init failed, %{public}s",
                             AVCSErrorToString(static_cast<AVCodecServiceErrCode>(ret)).c_str());

    return impl;
}

int32_t VideoEncoderFactory::CreateByMime(const std::string &mime, Format &format,
                                          std::shared_ptr<AVCodecVideoEncoder> &encoder)
{
    auto impl = std::make_shared<AVCodecVideoEncoderImpl>();

    int32_t ret = impl->Init(AVCODEC_TYPE_VIDEO_ENCODER, true, mime, format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "AVCodec video encoder impl init failed, %{public}s",
                             AVCSErrorToString(static_cast<AVCodecServiceErrCode>(ret)).c_str());

    encoder = impl;
    return AVCS_ERR_OK;
}

int32_t VideoEncoderFactory::CreateByName(const std::string &name, Format &format,
                                          std::shared_ptr<AVCodecVideoEncoder> &encoder)
{
    auto impl = std::make_shared<AVCodecVideoEncoderImpl>();

    int32_t ret = impl->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, name, format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "AVCodec video encoder impl init failed, %{public}s",
                             AVCSErrorToString(static_cast<AVCodecServiceErrCode>(ret)).c_str());

    encoder = impl;
    return AVCS_ERR_OK;
}

int32_t AVCodecVideoEncoderImpl::Init(AVCodecType type, bool isMimeType, const std::string &name, Format &format)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;

    int32_t ret = AVCodecServiceFactory::GetInstance().CreateCodecService(codecClient_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, ret, "Codec client create failed");
    ret = codecClient_->Init(type, isMimeType, name, *format.GetMeta());
    this->SetTag(codecClient_->GetTag()); // execute with CodecServiceProxy initialization completed
    return ret;
}

AVCodecVideoEncoderImpl::AVCodecVideoEncoderImpl()
{
    AVCODEC_LOGD_WITH_TAG("AVCodecVideoEncoderImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecVideoEncoderImpl::~AVCodecVideoEncoderImpl()
{
    if (codecClient_ != nullptr) {
        (void)AVCodecServiceFactory::GetInstance().DestroyCodecService(codecClient_);
        codecClient_ = nullptr;
    }
    AVCODEC_LOGD_WITH_TAG("AVCodecVideoEncoderImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecVideoEncoderImpl::Configure(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->Configure(format);
}

int32_t AVCodecVideoEncoderImpl::SetCustomBuffer(std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->SetCustomBuffer(buffer);
}

int32_t AVCodecVideoEncoderImpl::Prepare()
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return AVCS_ERR_OK;
}

int32_t AVCodecVideoEncoderImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->Start();
}

int32_t AVCodecVideoEncoderImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->Stop();
}

int32_t AVCodecVideoEncoderImpl::Flush()
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->Flush();
}

int32_t AVCodecVideoEncoderImpl::NotifyEos()
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->NotifyEos();
}

int32_t AVCodecVideoEncoderImpl::Reset()
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->Reset();
}

int32_t AVCodecVideoEncoderImpl::Release()
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->Release();
}

sptr<Surface> AVCodecVideoEncoderImpl::CreateInputSurface()
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, nullptr, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->CreateInputSurface();
}

int32_t AVCodecVideoEncoderImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->QueueInputBuffer(index, info, flag);
}

int32_t AVCodecVideoEncoderImpl::QueueInputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->QueueInputBuffer(index);
}

int32_t AVCodecVideoEncoderImpl::QueueInputParameter(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->QueueInputParameter(index);
}

int32_t AVCodecVideoEncoderImpl::GetOutputFormat(Format &format)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->GetOutputFormat(format);
}

int32_t AVCodecVideoEncoderImpl::ReleaseOutputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->ReleaseOutputBuffer(index);
}

int32_t AVCodecVideoEncoderImpl::QueryInputBuffer(uint32_t &index, int64_t timeoutUs)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE_WITH_TAG;
    return codecClient_->QueryInputBuffer(index, timeoutUs);
}

int32_t AVCodecVideoEncoderImpl::QueryOutputBuffer(uint32_t &index, int64_t timeoutUs)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE_WITH_TAG;
    return codecClient_->QueryOutputBuffer(index, timeoutUs);
}

std::shared_ptr<AVBuffer> AVCodecVideoEncoderImpl::GetInputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, nullptr, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE_WITH_TAG;
    return codecClient_->GetInputBuffer(index);
}

std::shared_ptr<AVBuffer> AVCodecVideoEncoderImpl::GetOutputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, nullptr, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE_WITH_TAG;
    return codecClient_->GetOutputBuffer(index);
}

int32_t AVCodecVideoEncoderImpl::SetParameter(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->SetParameter(format);
}

int32_t AVCodecVideoEncoderImpl::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callback != nullptr, AVCS_ERR_INVALID_VAL, "Callback is nullptr");
    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->SetCallback(callback);
}

int32_t AVCodecVideoEncoderImpl::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callback != nullptr, AVCS_ERR_INVALID_VAL, "callback is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->SetCallback(callback);
}

int32_t AVCodecVideoEncoderImpl::SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callback != nullptr, AVCS_ERR_INVALID_VAL, "callback is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->SetCallback(callback);
}

int32_t AVCodecVideoEncoderImpl::SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callback != nullptr, AVCS_ERR_INVALID_VAL, "callback is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->SetCallback(callback);
}

int32_t AVCodecVideoEncoderImpl::GetInputFormat(Format &format)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecClient_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    return codecClient_->GetInputFormat(format);
}
} // namespace MediaAVCodec
} // namespace OHOS
