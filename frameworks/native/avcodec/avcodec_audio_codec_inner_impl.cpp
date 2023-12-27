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
#include "avcodec_audio_codec_inner_impl.h"
#include "i_avcodec_service.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "avcodec_codec_name.h"
#include "codec_server.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecAudioCodecInnerImpl"};
}

std::shared_ptr<AVCodecAudioCodec> AudioCodecFactory::CreateByMime(const std::string &mime, bool isEncoder)
{
    AVCODEC_SYNC_TRACE;
    std::shared_ptr<AVCodecAudioCodecInnerImpl> impl = std::make_shared<AVCodecAudioCodecInnerImpl>();
    AVCodecType type = AVCODEC_TYPE_AUDIO_DECODER;
    if (isEncoder) {
        type = AVCODEC_TYPE_AUDIO_ENCODER;
    }
    Media::Status ret = impl->Init(type, true, mime);
    CHECK_AND_RETURN_RET_LOG(ret == Media::Status::OK, nullptr, "failed to init AVCodecAudioCodecInnerImpl");
    return impl;
}

std::shared_ptr<AVCodecAudioCodec> AudioCodecFactory::CreateByName(const std::string &name)
{
    AVCODEC_SYNC_TRACE;
    std::shared_ptr<AVCodecAudioCodecInnerImpl> impl = std::make_shared<AVCodecAudioCodecInnerImpl>();
    std::string codecMimeName = name;
    if (name.compare(AVCodecCodecName::AUDIO_DECODER_API9_AAC_NAME) == 0) {
        codecMimeName = AVCodecCodecName::AUDIO_DECODER_AAC_NAME;
    } else if (name.compare(AVCodecCodecName::AUDIO_ENCODER_API9_AAC_NAME) == 0) {
        codecMimeName = AVCodecCodecName::AUDIO_ENCODER_AAC_NAME;
    }
    AVCodecType type = AVCODEC_TYPE_AUDIO_ENCODER;
    if (codecMimeName.find("Encoder") != codecMimeName.npos) {
        type = AVCODEC_TYPE_AUDIO_ENCODER;
    } else {
        type = AVCODEC_TYPE_AUDIO_DECODER;
    }
    Media::Status ret = impl->Init(type, false, name);
    CHECK_AND_RETURN_RET_LOG(ret == Media::Status::OK, nullptr, "failed to init AVCodecAudioCodecInnerImpl");
    return impl;
}

AVCodecAudioCodecInnerImpl::AVCodecAudioCodecInnerImpl()
{
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecAudioCodecInnerImpl::~AVCodecAudioCodecInnerImpl()
{
    codecService_ = nullptr;
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

Media::Status AVCodecAudioCodecInnerImpl::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Init");
    codecService_ = CodecServer::Create();
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, Media::Status::ERROR_NO_MEMORY,
                             "failed to create codec service");
    int32_t ret = codecService_->Init(type, isMimeType, name, API_VERSION::API_VERSION_11);
    return static_cast<Media::Status>(ret);
}

Media::Status AVCodecAudioCodecInnerImpl::Configure(const std::shared_ptr<Media::Meta> &meta)
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Configure");
    int32_t ret = codecService_->Configure(meta);
    return static_cast<Media::Status>(ret);
}

Media::Status AVCodecAudioCodecInnerImpl::SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer)
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl SetOutputBufferQueue");
    int32_t ret = codecService_->SetOutputBufferQueue(bufferQueueProducer);
    return static_cast<Media::Status>(ret);
}

Media::Status AVCodecAudioCodecInnerImpl::Prepare()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Prepare");
    int32_t ret = codecService_->Prepare();
    return static_cast<Media::Status>(ret);
}

sptr<Media::AVBufferQueueProducer> AVCodecAudioCodecInnerImpl::GetInputBufferQueue()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl GetInputBufferQueue");
    return codecService_->GetInputBufferQueue();
}

Media::Status AVCodecAudioCodecInnerImpl::Start()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Start");
    int32_t ret = codecService_->Start();
    return static_cast<Media::Status>(ret);
}

Media::Status AVCodecAudioCodecInnerImpl::Stop()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Stop");
    int32_t ret = codecService_->Stop();
    return static_cast<Media::Status>(ret);
}

Media::Status AVCodecAudioCodecInnerImpl::Flush()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Flush");
    int32_t ret = codecService_->Flush();
    return static_cast<Media::Status>(ret);
}

Media::Status AVCodecAudioCodecInnerImpl::Reset()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Reset");
    int32_t ret = codecService_->Reset();
    return static_cast<Media::Status>(ret);
}

Media::Status AVCodecAudioCodecInnerImpl::Release()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Release");
    int32_t ret = codecService_->Release();
    return static_cast<Media::Status>(ret);
}

Media::Status AVCodecAudioCodecInnerImpl::NotifyEos()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl NotifyEos");
    int32_t ret = codecService_->NotifyEos();
    return static_cast<Media::Status>(ret);
}

Media::Status AVCodecAudioCodecInnerImpl::SetParameter(const std::shared_ptr<Media::Meta> &parameter)
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl SetParameter");
    int32_t ret = codecService_->SetParameter(parameter);
    return static_cast<Media::Status>(ret);
}

Media::Status AVCodecAudioCodecInnerImpl::GetOutputFormat(std::shared_ptr<Media::Meta> &parameter)
{
    AVCODEC_LOGD_LIMIT(10, "AVCodecAudioCodecInnerImpl GetOutputFormat");   // limit10
    int32_t ret = codecService_->GetOutputFormat(parameter);
    return static_cast<Media::Status>(ret);
}

void AVCodecAudioCodecInnerImpl::ProcessInputBuffer()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl ProcessInputBuffer");
    codecService_->ProcessInputBuffer();
}

} // namespace MediaAVCodec
} // namespace OHOS