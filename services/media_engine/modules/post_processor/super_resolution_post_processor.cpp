/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "super_resolution_post_processor.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "SuperResolutionPostProcessor" };
}

namespace OHOS {
namespace Media {

namespace {
constexpr int32_t MAX_WIDTH = 1920;
constexpr int32_t MAX_HEIGHT = 1080;
}

using namespace VideoProcessingEngine;

static bool isSuperResolutionSupported(const std::shared_ptr<Meta>& meta)
{
    FALSE_RETURN_V(meta != nullptr, false);

    int32_t width = 0;
    int32_t height = 0;
    bool isDrmProtected = false;
    bool isHdrVivid = false;
    meta->GetData(Tag::VIDEO_WIDTH, width);
    meta->GetData(Tag::VIDEO_HEIGHT, height);
    meta->GetData(Tag::VIDEO_IS_HDR_VIVID, isHdrVivid);
    meta->GetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, isDrmProtected);
    bool isVideoSizeValid = (width > 0 && width <= MAX_WIDTH) &&
                            (height > 0 && height <= MAX_HEIGHT);
    bool canCreatePostProcessor = !isDrmProtected && !isHdrVivid && isVideoSizeValid;
    if (!canCreatePostProcessor) {
        MEDIA_LOG_E("invalid input stream for super resolution! width: " PUBLIC_LOG_D32
            ", height: " PUBLIC_LOG_D32 ", isHdrVivid: " PUBLIC_LOG_D32 ", drm: " PUBLIC_LOG_D32,
            width, height, isHdrVivid, isDrmProtected);
    }
    return canCreatePostProcessor;
}

static AutoRegisterPostProcessor<SuperResolutionPostProcessor> g_registerSuperResolutionPostProcessor(
    VideoPostProcessorType::SUPER_RESOLUTION, []() -> std::shared_ptr<BaseVideoPostProcessor> {
        auto postProcessor = std::make_shared<SuperResolutionPostProcessor>();
        if (postProcessor == nullptr || !postProcessor->IsValid()) {
            return nullptr;
        } else {
            return postProcessor;
        }
    }, &isSuperResolutionSupported);

class VPECallback : public VpeVideoCallback {
public:
    explicit VPECallback(std::shared_ptr<SuperResolutionPostProcessor> postProcessor)
        : postProcessor_(postProcessor) {}
    ~VPECallback() = default;

    void OnError(VPEAlgoErrCode errorCode)
    {
        if (auto postProcessor = postProcessor_.lock()) {
            postProcessor->OnError(errorCode);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
    }
    void OnState(VPEAlgoState state)
    {
    }
    void OnEffectChange(uint32_t type)
    {
        if (auto postProcessor = postProcessor_.lock()) {
            postProcessor->OnSuperResolutionChanged(type == VIDEO_TYPE_DETAIL_ENHANCER);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
    }
    void OnOutputFormatChanged(const Format& format)
    {
        size_t addrSize;
        uint8_t* addr = nullptr;
        if (!format.GetBuffer(ParameterKey::DETAIL_ENHANCER_TARGET_SIZE, &addr, addrSize)) {
            MEDIA_LOG_I("can not get target size");
            return;
        }
        FALSE_RETURN_MSG(addr != nullptr && addrSize == sizeof(VpeBufferSize),
            "Invalid input: addr is null or addrSize=%{public}zu(Expected:%{public}zu)!",
            addrSize, sizeof(VpeBufferSize));
        auto size = reinterpret_cast<VpeBufferSize*>(addr);

        MEDIA_LOG_D("OnOutputFormatChanged nextW=" PUBLIC_LOG_D32 " nextH=" PUBLIC_LOG_D32, size->width, size->height);
        if (size->width <= 0 || size->height <= 0) {
            MEDIA_LOG_W("invaild video size");
            return;
        }
    }
    void OnOutputBufferAvailable(uint32_t index, VpeBufferFlag flag)
    {
    }
    void OnOutputBufferAvailable(uint32_t index, const VpeBufferInfo& info)
    {
        if (auto postProcessor = postProcessor_.lock()) {
            postProcessor->OnOutputBufferAvailable(index, info);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
    }

private:
    std::weak_ptr<SuperResolutionPostProcessor> postProcessor_;
};

SuperResolutionPostProcessor::SuperResolutionPostProcessor()
{
    postProcessor_ = VpeVideo::Create(VIDEO_TYPE_DETAIL_ENHANCER);
    isPostProcessorOn_ = true;
}

SuperResolutionPostProcessor::~SuperResolutionPostProcessor()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    postProcessor_ = nullptr;
}

bool SuperResolutionPostProcessor::IsValid()
{
    return postProcessor_ != nullptr;
}

Status SuperResolutionPostProcessor::Init()
{
    MEDIA_LOG_D("Init in");
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);

    auto ret = SetQualityLevel(DEFAULT_QUALITY_LEVEL);
    FALSE_RETURN_V(ret == Status::OK, ret);
    auto callback = std::make_shared<VPECallback>(shared_from_this());
    return postProcessor_->RegisterCallback(callback) == VPEAlgoErrCode::VPE_ALGO_ERR_OK ?
        Status::OK : Status::ERROR_INVALID_STATE;
}

Status SuperResolutionPostProcessor::Flush()
{
    MEDIA_LOG_D("Flush in");
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);
    auto ret = postProcessor_->Flush();
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_STATE);
    return Status::OK;
}

Status SuperResolutionPostProcessor::Stop()
{
    MEDIA_LOG_D("Stop in");
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);
    auto ret = postProcessor_->Stop();
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_STATE);
    return Release();
}

Status SuperResolutionPostProcessor::Start()
{
    MEDIA_LOG_D("Start in");
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);
    auto ret = postProcessor_->Start();
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_STATE);
    return Status::OK;
}

Status SuperResolutionPostProcessor::Release()
{
    MEDIA_LOG_D("Release in");
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);
    auto ret = postProcessor_->Release();
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_STATE);
    return Status::OK;
}

Status SuperResolutionPostProcessor::NotifyEos(int64_t eosPts)
{
    MEDIA_LOG_D("Notify eos");
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);
    auto ret = postProcessor_->NotifyEos();
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}

sptr<Surface> SuperResolutionPostProcessor::GetInputSurface()
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, nullptr);
    return postProcessor_->GetInputSurface();
}

Status SuperResolutionPostProcessor::SetOutputSurface(sptr<Surface> surface)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);
    auto ret = postProcessor_->SetOutputSurface(surface);
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}

void SuperResolutionPostProcessor::OnOutputBufferAvailable(uint32_t index, VpeBufferFlag flag)
{
    auto buffer = AVBuffer::CreateAVBuffer();
    if (buffer == nullptr) {
        MEDIA_LOG_E("Create buffer failed");
        ReleaseOutputBuffer(index, false);
        return;
    }
    if (flag & static_cast<uint32_t>(VPE_BUFFER_FLAG_EOS)) {
        buffer->flag_ |= static_cast<uint32_t>(Plugins::AVBufferFlag::EOS);
    }
    FALSE_RETURN_MSG(filterCallback_ != nullptr, "filterCallback_ is nullptr");
    filterCallback_->OnOutputBufferAvailable(index, buffer);
}

void SuperResolutionPostProcessor::OnOutputBufferAvailable(uint32_t index, const VpeBufferInfo& info)
{
    auto buffer = AVBuffer::CreateAVBuffer();
    if (buffer == nullptr) {
        MEDIA_LOG_E("Create buffer failed");
        ReleaseOutputBuffer(index, false);
        return;
    }
    if (info.flag & static_cast<uint32_t>(VPE_BUFFER_FLAG_EOS)) {
        buffer->flag_ |= static_cast<uint32_t>(Plugins::AVBufferFlag::EOS);
    }
    buffer->pts_ = info.presentationTimestamp;
    FALSE_RETURN_MSG(filterCallback_ != nullptr, "filterCallback_ is nullptr");
    filterCallback_->OnOutputBufferAvailable(index, buffer);
}

Status SuperResolutionPostProcessor::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);
    auto ret = postProcessor_->ReleaseOutputBuffer(index, render);
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_STATE);
    return Status::OK;
}

Status SuperResolutionPostProcessor::RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs)
{
    MEDIA_LOG_D("RenderOutputBufferAtTime timestamp: %{public}" PRId64, renderTimestampNs);
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);
    auto ret = postProcessor_->RenderOutputBufferAtTime(index, renderTimestampNs);
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_STATE);
    return Status::OK;
}

Status SuperResolutionPostProcessor::SetCallback(const std::shared_ptr<PostProcessorCallback> callback)
{
    filterCallback_ = callback;
    return Status::OK;
}

Status SuperResolutionPostProcessor::SetPostProcessorOn(bool isPostProcessorOn)
{
    MEDIA_LOG_D("SetPostProcessorOn: %{public}d", isPostProcessorOn);
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);
    VPEAlgoErrCode ret = VPEAlgoErrCode::VPE_ALGO_ERR_OK;
    if (isPostProcessorOn) {
        ret = postProcessor_->Enable();
    } else {
        ret = postProcessor_->Disable();
    }
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_STATE);
    return Status::OK;
}

Status SuperResolutionPostProcessor::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver)
{
    eventReceiver_ = receiver;
    return Status::OK;
}

Status SuperResolutionPostProcessor::SetParameter(const Format &format)
{
    MEDIA_LOG_D("Setparameter in");
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);
    auto ret = postProcessor_->SetParameter(format);
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}

Status SuperResolutionPostProcessor::SetQualityLevel(DetailEnhancerQualityLevel level)
{
    MEDIA_LOG_D("SetQualityLevel in");
    Format parameter;
    parameter.PutIntValue(ParameterKey::DETAIL_ENHANCER_QUALITY_LEVEL, level);
    parameter.PutIntValue(ParameterKey::DETAIL_ENHANCER_AUTO_DOWNSHIFT, 0);
    auto ret = postProcessor_->SetParameter(parameter);
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}

Status SuperResolutionPostProcessor::SetVideoWindowSize(int32_t width, int32_t height)
{
    MEDIA_LOG_D("SetVideoWindowSize in");
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::ERROR_INVALID_STATE);
    Format parameter;
    VpeBufferSize outputSize = { width, height };
    parameter.PutBuffer(ParameterKey::DETAIL_ENHANCER_TARGET_SIZE,
        reinterpret_cast<const uint8_t*>(&outputSize), sizeof(VpeBufferSize));
    auto ret = postProcessor_->SetParameter(parameter);
    FALSE_RETURN_V(ret == VPEAlgoErrCode::VPE_ALGO_ERR_OK, Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}

void SuperResolutionPostProcessor::OnError(VPEAlgoErrCode errorCode)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_MSG(filterCallback_ != nullptr, "filterCallback_ is nullptr");
    MEDIA_LOG_E("SuperResolutionPostProcessor error happened. ErrorCode: %{public}d", errorCode);
}

void SuperResolutionPostProcessor::OnOutputFormatChanged(const Format &format)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FALSE_RETURN_MSG(filterCallback_ != nullptr, "filterCallback_ is nullptr");
    filterCallback_->OnOutputFormatChanged(format);
}

void SuperResolutionPostProcessor::OnSuperResolutionChanged(bool enable)
{
    MEDIA_LOG_D("OnSuperResolutionChanged: %{public}d", enable);
    std::shared_lock<std::shared_mutex> lock(mutex_);
    isPostProcessorOn_ = enable;
    if (eventReceiver_ != nullptr) {
        eventReceiver_->OnEvent({"SuperResolutionPostProcessor", EventType::EVENT_SUPER_RESOLUTION_CHANGED, enable});
    }
}

} // namespace Media
} // namespace OHOS