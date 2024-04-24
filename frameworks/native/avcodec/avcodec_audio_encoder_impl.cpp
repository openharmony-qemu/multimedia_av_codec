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

#include "avcodec_audio_encoder_impl.h"
#include "i_avcodec_service.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "codec_server.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AVCodecAudioEncoderImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<AVCodecAudioEncoder> AudioEncoderFactory::CreateByMime(const std::string &mime)
{
    AVCODEC_SYNC_TRACE;
    std::shared_ptr<AVCodecAudioEncoderImpl> impl = std::make_shared<AVCodecAudioEncoderImpl>();

    int32_t ret = impl->Init(AVCODEC_TYPE_AUDIO_ENCODER, true, mime);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "failed to init AVCodecAudioEncoderImpl");

    return impl;
}

std::shared_ptr<AVCodecAudioEncoder> AudioEncoderFactory::CreateByName(const std::string &name)
{
    AVCODEC_SYNC_TRACE;
    std::shared_ptr<AVCodecAudioEncoderImpl> impl = std::make_shared<AVCodecAudioEncoderImpl>();

    int32_t ret = impl->Init(AVCODEC_TYPE_AUDIO_ENCODER, false, name);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "failed to init AVCodecAudioEncoderImpl");

    return impl;
}

int32_t AVCodecAudioEncoderImpl::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    AVCODEC_SYNC_TRACE;
    Format format;
    codecService_ = CodecServer::Create();
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_UNKNOWN, "failed to create codec service");

    return codecService_->Init(type, isMimeType, name, *format.GetMeta());
}

AVCodecAudioEncoderImpl::AVCodecAudioEncoderImpl()
{
    AVCODEC_LOGD("AVCodecAudioEncoderImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecAudioEncoderImpl::~AVCodecAudioEncoderImpl()
{
    codecService_ = nullptr;
    AVCODEC_LOGD("AVCodecAudioEncoderImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecAudioEncoderImpl::Configure(const Format &format)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->Configure(format);
}

int32_t AVCodecAudioEncoderImpl::Prepare()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    return AVCS_ERR_OK;
}

int32_t AVCodecAudioEncoderImpl::Start()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->Start();
}

int32_t AVCodecAudioEncoderImpl::Stop()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->Stop();
}

int32_t AVCodecAudioEncoderImpl::Flush()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->Flush();
}

int32_t AVCodecAudioEncoderImpl::Reset()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->Reset();
}

int32_t AVCodecAudioEncoderImpl::Release()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->Release();
}

int32_t AVCodecAudioEncoderImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->QueueInputBuffer(index, info, flag);
}

int32_t AVCodecAudioEncoderImpl::GetOutputFormat(Format &format)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->GetOutputFormat(format);
}

int32_t AVCodecAudioEncoderImpl::ReleaseOutputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->ReleaseOutputBuffer(index);
}

int32_t AVCodecAudioEncoderImpl::SetParameter(const Format &format)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->SetParameter(format);
}

int32_t AVCodecAudioEncoderImpl::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "service died");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_INVALID_VAL, "callback is nullptr");
    return codecService_->SetCallback(callback);
}
} // namespace MediaAVCodec
} // namespace OHOS
