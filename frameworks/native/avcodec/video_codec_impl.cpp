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

#include "video_codec_impl.h"
#include "i_avcodec_service.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avcodec_dfx.h"
#include "media_codec_server.h"
#include "avcodec_list.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media::Plugin;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoCodecImpl"};
}

std::shared_ptr<VideoCodec> VideoCodecFactory::CreateByMime(bool isEncoder, const std::string &mime)
{
    AVCODEC_SYNC_TRACE;

    std::shared_ptr<VideoCodecImpl> impl = std::make_shared<VideoCodecImpl>();

    int32_t ret = impl->Init(isEncoder, true, mime);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "AVCodec video decoder impl init failed");

    return impl;
}

std::shared_ptr<VideoCodec> VideoCodecFactory::CreateByName(const std::string &name)
{
    AVCODEC_SYNC_TRACE;

    std::shared_ptr<VideoCodecImpl> impl = std::make_shared<VideoCodecImpl>();

    int32_t ret = impl->Init(false, false, name);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "AVCodec video decoder impl init failed");

    return impl;
}

VideoEncoderImpl::VideoEncoderImpl()
{
    AVCODEC_LOGD("VideoEncoderImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoEncoderImpl::~VideoEncoderImpl()
{
    if (codecService_ != nullptr) {
        (void)AVCodecServiceFactory::GetInstance().DestroyMediaCodecService(codecService_);
        codecService_ = nullptr;
    }
    AVCODEC_LOGD("VideoEncoderImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t VideoCodecImpl::Init(bool isEncoder, bool isMimeType, const std::string &name)
{
    AVCODEC_SYNC_TRACE;
    codecService_ = AVCodecServiceFactory::GetInstance().CreateMediaCodecService();
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr,
        AVCS_ERR_INVALID_OPERATION, "Codec service create failed, name: %{public}s", name.c_str());

    return codecService_->Init(isEncoder, isMimeType, name);
}

int32_t VideoCodecImpl::Release()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->Release();
}

int32_t VideoCodecImpl::SetCallback(const std::shared_ptr<VideoCodecCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->SetCallback(callback);
}

int32_t VideoCodecImpl::Configure(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->Configure(format);
}

int32_t VideoCodecImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->Start();
}

int32_t VideoCodecImpl::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->Prepare();
}

int32_t VideoCodecImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->Stop();
}

int32_t VideoCodecImpl::Flush()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->Flush();
}

int32_t VideoCodecImpl::Reset()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->Reset();
}

int32_t VideoCodecImpl::GetOutputFormat(Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->GetOutputFormat(format);
}

int32_t VideoCodecImpl::SetParameter(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->SetParameter(format);
}

sptr<Media::AVBufferQueueProducer> VideoCodecImpl::GetInputBufferQueue()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->GetInputBufferQueue();
}

int32_t VideoCodecImpl::SetOutputBufferQueue(sptr<Media::AVBufferQueueProducer> bufferQueue)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->SetOutputBufferQueue(bufferQueue);
}

sptr<Surface> VideoCodecImpl::CreateInputSurface()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->CreateInputSurface();
}

int32_t VideoCodecImpl::SetOutputSurface(sptr<Surface> surface)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->SetOutputSurface(surface);
}

int32_t VideoCodecImpl::NotifyEos()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->NotifyEos();
}

int32_t VideoCodecImpl::SurfaceModeReturnData(std::shared_ptr<Meida::AVBuffer> buffer, bool available)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->SurfaceModeReturnData(buffer, available);
}
} // namespace MediaAVCodec
} // namespace OHOS