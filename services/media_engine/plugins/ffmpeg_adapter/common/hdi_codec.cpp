/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "hdi_codec.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-HdiCodec"};
constexpr int TIMEOUT_MS = 1000;
using namespace OHOS::HDI::Codec::V3_0;
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Hdi {
HdiCodec::HdiCodec()
    : componentId_(0),
      componentName_(""),
      compCb_(nullptr),
      compNode_(nullptr),
      compMgr_(nullptr),
      outputOmxBuffer_(nullptr),
      omxInBufferInfo_(std::make_shared<OmxBufferInfo>()),
      omxOutBufferInfo_(std::make_shared<OmxBufferInfo>()),
      event_(CODEC_EVENT_ERROR)
{
}

Status HdiCodec::InitComponent(const std::string &name)
{
    compMgr_ = GetComponentManager();
    if (compMgr_ == nullptr) {
        AVCODEC_LOGE("GetCodecComponentManager failed!");
        return Status::ERROR_NULL_POINTER;
    }

    componentName_ = name;
    compCb_ = new HdiCodec::HdiCallback(shared_from_this());
    int32_t ret = compMgr_->CreateComponent(compNode_, componentId_, componentName_, 0, compCb_);
    if (ret != HDF_SUCCESS || compNode_ == nullptr) {
        compCb_ = nullptr;
        compMgr_ = nullptr;
        AVCODEC_LOGE("CreateComponent failed, ret=%{public}d", ret);
        return Status::ERROR_NULL_POINTER;
    }
    return Status::OK;
}

sptr<ICodecComponentManager> HdiCodec::GetComponentManager()
{
    sptr<ICodecComponentManager> compMgr = ICodecComponentManager::Get(true);
    return compMgr;
}

std::vector<CodecCompCapability> HdiCodec::GetCapabilityList()
{
    int32_t compCount = 0;
    int32_t ret = compMgr_->GetComponentNum(compCount);
    if (ret != HDF_SUCCESS || compCount <= 0) {
        AVCODEC_LOGE("GetComponentNum failed, ret=%{public}d", ret);
        return {};
    }

    std::vector<CodecCompCapability> capabilityList(compCount);
    ret = compMgr_->GetComponentCapabilityList(capabilityList, compCount);
    if (ret != HDF_SUCCESS) {
        AVCODEC_LOGE("GetComponentCapabilityList failed, ret=%{public}d", ret);
        return {};
    }
    return capabilityList;
}

bool HdiCodec::IsSupportCodecType(const std::string &name)
{
    if (compMgr_ == nullptr) {
        compMgr_ = GetComponentManager();
        if (compMgr_ == nullptr) {
            AVCODEC_LOGE("compMgr_ is null");
            return false;
        }
    }

    std::vector<CodecCompCapability> capabilityList = GetCapabilityList();
    if (capabilityList.empty()) {
        AVCODEC_LOGE("Hdi capabilityList is empty!");
        return false;
    }

    for (const CodecCompCapability &capability : capabilityList) {
        if (capability.compName == name) {
            return true;
        }
    }
    
    return false;
}

void HdiCodec::InitParameter(AudioCodecOmxParam &param)
{
    (void)memset_s(&param, sizeof(param), 0x0, sizeof(param));
    param.size = sizeof(param);
    param.version.s.nVersionMajor = 1;
}

Status HdiCodec::GetParameter(uint32_t index, AudioCodecOmxParam &param)
{
    int8_t *p = reinterpret_cast<int8_t *>(&param);
    std::vector<int8_t> inParamVec(p, p + sizeof(param));
    std::vector<int8_t> outParamVec;
    int32_t ret = compNode_->GetParameter(index, inParamVec, outParamVec);
    if (ret != HDF_SUCCESS) {
        AVCODEC_LOGE("GetParameter failed!");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (outParamVec.size() != sizeof(param)) {
        AVCODEC_LOGE("param size is invalid!");
        return Status::ERROR_INVALID_PARAMETER;
    }
    errno_t rc = memcpy_s(&param, sizeof(param), outParamVec.data(), outParamVec.size());
    if (rc != EOK) {
        AVCODEC_LOGE("memory copy failed!");
        return Status::ERROR_INVALID_DATA;
    }
    return Status::OK;
}

Status HdiCodec::SetParameter(uint32_t index, const std::vector<int8_t> &paramVec)
{
    int32_t ret = compNode_->SetParameter(index, paramVec);
    if (ret != HDF_SUCCESS) {
        AVCODEC_LOGE("SetParameter failed!");
        return Status::ERROR_INVALID_PARAMETER;
    }
    return Status::OK;
}

Status HdiCodec::InitBuffers(uint32_t bufferSize)
{
    Status ret = InitBuffersByPort(PortIndex::INPUT_PORT, bufferSize);
    if (ret != Status::OK) {
        AVCODEC_LOGE("Init Input Buffers failed, ret=%{public}d", ret);
        return ret;
    }

    ret = InitBuffersByPort(PortIndex::OUTPUT_PORT, bufferSize);
    if (ret != Status::OK) {
        AVCODEC_LOGE("Init Output Buffers failed, ret=%{public}d", ret);
        return ret;
    }

    return Status::OK;
}

Status HdiCodec::InitBuffersByPort(PortIndex portIndex, uint32_t bufferSize)
{
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(avAllocator, bufferSize);
    std::shared_ptr<OmxCodecBuffer> omxBuffer = std::make_shared<OmxCodecBuffer>();
    omxBuffer->size = sizeof(OmxCodecBuffer);
    omxBuffer->version.version.majorVersion = 1;
    omxBuffer->bufferType = CODEC_BUFFER_TYPE_AVSHARE_MEM_FD;
    omxBuffer->fd = avBuffer->memory_->GetFileDescriptor();
    omxBuffer->bufferhandle = nullptr;
    omxBuffer->allocLen = bufferSize;
    omxBuffer->fenceFd = -1;
    omxBuffer->pts = 0;
    omxBuffer->flag = 0;

    if (portIndex == PortIndex::INPUT_PORT) {
        omxBuffer->type = READ_ONLY_TYPE;
    } else {
        omxBuffer->type = READ_WRITE_TYPE;
    }

    OmxCodecBuffer outBuffer;
    int32_t ret = compNode_->UseBuffer(static_cast<uint32_t>(portIndex), *omxBuffer.get(), outBuffer);
    if (ret != HDF_SUCCESS) {
        AVCODEC_LOGE("InitBuffers failed, ret=%{public}d", ret);
        return Status::ERROR_NO_MEMORY;
    }

    omxBuffer->bufferId = outBuffer.bufferId;
    if (portIndex == PortIndex::INPUT_PORT) {
        omxInBufferInfo_->omxBuffer = omxBuffer;
        omxInBufferInfo_->avBuffer = avBuffer;
    } else if (portIndex == PortIndex::OUTPUT_PORT) {
        omxOutBufferInfo_->omxBuffer = omxBuffer;
        omxOutBufferInfo_->avBuffer = avBuffer;
    }

    return Status::OK;
}

Status HdiCodec::SendCommand(CodecCommandType cmd, uint32_t param)
{
    std::unique_lock lock(mutex_);
    event_ = CODEC_EVENT_ERROR;
    int32_t ret = compNode_->SendCommand(cmd, param, {});
    if (ret != HDF_SUCCESS) {
        return Status::ERROR_INVALID_DATA;
    }
    condition_.wait_for(lock, std::chrono::milliseconds(TIMEOUT_MS),
                        [this] { return event_ == CODEC_EVENT_CMD_COMPLETE; });
    
    if (event_ != CODEC_EVENT_CMD_COMPLETE) {
        AVCODEC_LOGE("SendCommand timeout!");
        return Status::ERROR_INVALID_PARAMETER;
    }

    return Status::OK;
}

Status HdiCodec::EmptyThisBuffer(const std::shared_ptr<AVBuffer> &buffer)
{
    omxInBufferInfo_->omxBuffer->filledLen = buffer->memory_->GetSize();
    omxInBufferInfo_->omxBuffer->offset = buffer->memory_->GetOffset();
    omxInBufferInfo_->omxBuffer->pts = buffer->pts_;
    omxInBufferInfo_->omxBuffer->flag = buffer->flag_;

    errno_t rc = memcpy_s(omxInBufferInfo_->avBuffer->memory_->GetAddr(), omxInBufferInfo_->omxBuffer->filledLen,
                          buffer->memory_->GetAddr(), omxInBufferInfo_->omxBuffer->filledLen);
    if (rc != EOK) {
        AVCODEC_LOGE("memory copy failed!");
        return Status::ERROR_INVALID_DATA;
    }

    int32_t ret = compNode_->EmptyThisBuffer(*omxInBufferInfo_->omxBuffer.get());
    if (ret != HDF_SUCCESS) {
        AVCODEC_LOGE("EmptyThisBuffer failed, ret=%{public}d", ret);
        return Status::ERROR_INVALID_DATA;
    }
    AVCODEC_LOGD("EmptyThisBuffer OK");
    return Status::OK;
}

Status HdiCodec::FillThisBuffer(std::shared_ptr<AVBuffer> &buffer)
{
    std::unique_lock lock(mutex_);
    outputOmxBuffer_ = nullptr;
    int32_t ret = compNode_->FillThisBuffer(*omxOutBufferInfo_->omxBuffer.get());
    if (ret != HDF_SUCCESS) {
        return Status::ERROR_INVALID_DATA;
    }
    condition_.wait_for(lock, std::chrono::milliseconds(TIMEOUT_MS), [this] { return outputOmxBuffer_ != nullptr; });
    
    if (outputOmxBuffer_ == nullptr) {
        AVCODEC_LOGE("FillThisBuffer timeout!");
        return Status::ERROR_INVALID_PARAMETER;
    }

    errno_t rc = memcpy_s(buffer->memory_->GetAddr(), outputOmxBuffer_->filledLen,
                          omxOutBufferInfo_->avBuffer->memory_->GetAddr(), outputOmxBuffer_->filledLen);
    if (rc != EOK) {
        AVCODEC_LOGE("memory copy failed!");
        return Status::ERROR_INVALID_DATA;
    }

    buffer->memory_->SetSize(outputOmxBuffer_->filledLen);
    buffer->memory_->SetOffset(outputOmxBuffer_->offset);
    buffer->pts_ = outputOmxBuffer_->pts;
    buffer->flag_ = outputOmxBuffer_->flag;

    AVCODEC_LOGD("FillThisBuffer OK");
    return Status::OK;
}

Status HdiCodec::FreeBuffer(PortIndex portIndex, const std::shared_ptr<OmxCodecBuffer> &omxBuffer)
{
    if (omxBuffer != nullptr) {
        int32_t ret = compNode_->FreeBuffer(static_cast<uint32_t>(portIndex), *omxBuffer.get());
        if (ret != HDF_SUCCESS) {
            AVCODEC_LOGE("Free buffer fail, portIndex=%{public}d, ret=%{public}d", portIndex, ret);
            return Status::ERROR_INVALID_DATA;
        }
    }

    AVCODEC_LOGD("FreeBuffer OK");
    return Status::OK;
}

Status HdiCodec::OnEventHandler(CodecEventType event, const EventInfo &info)
{
    std::unique_lock lock(mutex_);
    event_ = event;
    condition_.notify_all();
    return Status::OK;
}

Status HdiCodec::OnEmptyBufferDone(const OmxCodecBuffer &buffer)
{
    return Status::OK;
}

Status HdiCodec::OnFillBufferDone(const OmxCodecBuffer &buffer)
{
    std::unique_lock lock(mutex_);
    outputOmxBuffer_ = std::make_shared<OmxCodecBuffer>(buffer);
    condition_.notify_all();
    return Status::OK;
}

Status HdiCodec::Reset()
{
    FreeBuffer(PortIndex::INPUT_PORT, omxInBufferInfo_->omxBuffer);
    FreeBuffer(PortIndex::OUTPUT_PORT, omxOutBufferInfo_->omxBuffer);
    omxInBufferInfo_->Reset();
    omxOutBufferInfo_->Reset();
    return Status::OK;
}

void HdiCodec::Release()
{
    FreeBuffer(PortIndex::INPUT_PORT, omxInBufferInfo_->omxBuffer);
    FreeBuffer(PortIndex::OUTPUT_PORT, omxOutBufferInfo_->omxBuffer);
    omxInBufferInfo_->Reset();
    omxOutBufferInfo_->Reset();
    
    if (compMgr_ != nullptr && componentId_ > 0) {
        compMgr_->DestroyComponent(componentId_);
    }
    compNode_ = nullptr;
    compCb_ = nullptr;
    compMgr_ = nullptr;
}

HdiCodec::HdiCallback::HdiCallback(std::shared_ptr<HdiCodec> hdiCodec)
{
    hdiCodec_ = hdiCodec;
}

int32_t HdiCodec::HdiCallback::EventHandler(CodecEventType event, const EventInfo &info)
{
    AVCODEC_LOGD("OnEventHandler");
    if (hdiCodec_) {
        hdiCodec_->OnEventHandler(event, info);
    }
    return HDF_SUCCESS;
}

int32_t HdiCodec::HdiCallback::EmptyBufferDone(int64_t appData, const OmxCodecBuffer &buffer)
{
    if (hdiCodec_) {
        hdiCodec_->OnEmptyBufferDone(buffer);
    }
    return HDF_SUCCESS;
}

int32_t HdiCodec::HdiCallback::FillBufferDone(int64_t appData, const OmxCodecBuffer &buffer)
{
    AVCODEC_LOGD("FillBufferDone");
    if (hdiCodec_) {
        hdiCodec_->OnFillBufferDone(buffer);
    }
    return HDF_SUCCESS;
}
} // namespace Hdi
} // namespace Plugins
} // namespace Media
} // namespace OHOS