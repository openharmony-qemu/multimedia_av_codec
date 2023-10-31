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

#include "avcodec_media_codec_impl.h"
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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecMediaCodecImpl"};
}

std::shared_ptr<AVCodecMediaCodec> MediaCodecFactory::CreateByMime(bool isEncoder, const std::string &mime)
{
    AVCODEC_SYNC_TRACE;

    std::shared_ptr<AVCodecMediaCodecImpl> impl = std::make_shared<AVCodecMediaCodecImpl>();

    int32_t ret = impl->Init(isEncoder, true, mime);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "AVCodec video decoder impl init failed");

    return impl;
}

std::shared_ptr<AVCodecMediaCodec> MediaCodecFactory::CreateByName(const std::string &name)
{
    AVCODEC_SYNC_TRACE;

    std::shared_ptr<AVCodecMediaCodecImpl> impl = std::make_shared<AVCodecMediaCodecImpl>();

    int32_t ret = impl->Init(false, false, name);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "AVCodec video decoder impl init failed");

    return impl;
}

// Keep mind here! Audio codec can not run in not pass through mode, and video codec is opposite.
bool IsPassThroughMode(bool isMimeType, const std::string &name)
{
    // std::shared_ptr<AVCodecList> avcodecList = AVCodecListFactory::CreateAVCodecList();
    // std::shared_ptr<CapabilityData> capabilityData(avcodecList->GetCapability());
    // if () {

    // }
    bool ret = false;

    AVCODEC_LOGD("Codec framework run in %{public}s mode", ret ? "pass through" : "ipc");
    return ret;
}

int32_t AVCodecMediaCodecImpl::Init(bool isEncoder, bool isMimeType, const std::string &name)
{
    AVCODEC_SYNC_TRACE;

    if (IsPassThroughMode()) {
        codecService_ = MediaCodecServer::Create();
    } else {
        codecService_ = AVCodecServiceFactory::GetInstance().CreateMediaCodecService();
    }
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr,
        AVCS_ERR_INVALID_OPERATION, "Codec service create failed, name: %{public}s", name.c_str());

    return codecService_->Init(isEncoder, isMimeType, name);
}

int32_t AVCodecMediaCodecImpl::Release()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec service is nullptr");

    AVCODEC_SYNC_TRACE;
    return codecService_->Release();
}

int32_t AVCodecMediaCodecImpl::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    return 0;
}

int32_t AVCodecMediaCodecImpl::Configure(const Format &format)
{
    return 0;
}

int32_t AVCodecMediaCodecImpl::Start()
{
return 0;
}

int32_t AVCodecMediaCodecImpl::Prepare()
{
return 0;
}

int32_t AVCodecMediaCodecImpl::Stop()
{
return 0;
}

int32_t AVCodecMediaCodecImpl::Flush()
{
    return 0;
}

int32_t AVCodecMediaCodecImpl::Reset()
{
    return 0;
}

int32_t AVCodecMediaCodecImpl::GetOutputFormat(Format & format)
{
return 0;
}

int32_t AVCodecMediaCodecImpl::SetParameter(const Format &format)
{
    return 0;
}

int32_t AVCodecMediaCodecImpl::VideoEncoderGetSurface(sptr<Surface> surface)
{
    return 0;
}

int32_t AVCodecMediaCodecImpl::VideoDecoderSetSurface(sptr<Surface> surface)
{
    return 0;
}

int32_t AVCodecMediaCodecImpl::VideoEncoderNotifyEndOfStream()
{
    return 0;
}

int32_t AVCodecMediaCodecImpl::VideoReturnSurfacemodeData()
{
    return 0;
}

} // namespace MediaAVCodec
} // namespace OHOS