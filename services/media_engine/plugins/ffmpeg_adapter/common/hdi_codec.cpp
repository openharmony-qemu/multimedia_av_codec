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
#include <unistd.h>

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
    CHECK_AND_RETURN_RET_LOG(compMgr_ != nullptr, Status::ERROR_NULL_POINTER, "GetCodecComponentManager failed!");

    componentName_ = name;
    compCb_ = new HdiCodec::HdiCallback(shared_from_this());
    int32_t ret = compMgr_->CreateComponent(compNode_, componentId_, componentName_, 0, compCb_);
    if (ret != HDF_SUCCESS || compNode_ == nullptr) {
        if (compCb_ != nullptr) {
            compCb_ = nullptr;
        }
        compMgr_ = nullptr;
        AVCODEC_LOGE("CreateComponent failed, ret=%{public}d", ret);
        return Status::ERROR_NULL_POINTER;
    }
    return Status::OK;
}

sptr<ICodecComponentManager> HdiCodec::GetComponentManager()
{
    sptr<ICodecComponentManager> compMgr = ICodecComponentManager::Get(false); // false: ipc
    return compMgr;
}

std::vector<CodecCompCapability> HdiCodec::GetCapabilityList()
{
    CHECK_AND_RETURN_RET_LOG(compMgr_ != nullptr, {}, "compMgr is nullptr!");
    int32_t compCount = 0;
    int32_t ret = compMgr_->GetComponentNum(compCount);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, {}, "GetComponentNum failed, ret=%{public}d", ret);
    CHECK_AND_RETURN_RET_LOG(compCount > 0, {}, "GetComponentNum failed, compCount=%{public}d", compCount);

    std::vector<CodecCompCapability> capabilityList(compCount);
    ret = compMgr_->GetComponentCapabilityList(capabilityList, compCount);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, {}, "GetComponentCapabilityList failed, ret=%{public}d", ret);

    return capabilityList;
}

bool HdiCodec::IsSupportCodecType(const std::string &name, MediaAVCodec::CapabilityData *audioCapability)
{
    if (compMgr_ == nullptr) {
        compMgr_ = GetComponentManager();
        CHECK_AND_RETURN_RET_LOG(compMgr_ != nullptr, false, "compMgr_ is null");
    }

    std::vector<CodecCompCapability> capabilityList = GetCapabilityList();
    CHECK_AND_RETURN_RET_LOG(!capabilityList.empty(), false, "Hdi capabilityList is empty!");

    bool checkName = std::any_of(std::begin(capabilityList),
        std::end(capabilityList), [name, audioCapability](CodecCompCapability capability) {
            if (capability.compName == name) {
                audioCapability->bitrate = MediaAVCodec::Range(capability.bitRate.min, capability.bitRate.max);
                audioCapability->sampleRate.push_back(capability.port.audio.sampleRate[0]); //set first number so far
                audioCapability->maxInstance = capability.maxInst;
                audioCapability->profiles.push_back(capability.supportProfiles[0]);
                audioCapability->channels = MediaAVCodec::Range(1, capability.port.audio.channelCount[0]);
            }
            
        return capability.compName == name;
    });
    return checkName;
}

void HdiCodec::InitParameter(AudioCodecOmxParam &param)
{
    (void)memset_s(&param, sizeof(param), 0x0, sizeof(param));
    param.size = sizeof(param);
    param.version.s.nVersionMajor = 1;
}

Status HdiCodec::GetParameter(uint32_t index, AudioCodecOmxParam &param)
{
    CHECK_AND_RETURN_RET_LOG(compNode_ != nullptr, Status::ERROR_NULL_POINTER, "compNode is nullptr!");
    int8_t *p = reinterpret_cast<int8_t *>(&param);
    std::vector<int8_t> inParamVec(p, p + sizeof(param));
    std::vector<int8_t> outParamVec;
    int32_t ret = compNode_->GetParameter(index, inParamVec, outParamVec);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, Status::ERROR_INVALID_PARAMETER, "GetParameter failed!");
    CHECK_AND_RETURN_RET_LOG(outParamVec.size() == sizeof(param), Status::ERROR_INVALID_PARAMETER,
        "param size is invalid!");

    errno_t rc = memcpy_s(&param, sizeof(param), outParamVec.data(), outParamVec.size());
    CHECK_AND_RETURN_RET_LOG(rc == EOK, Status::ERROR_INVALID_DATA, "memory copy failed!");
    return Status::OK;
}

Status HdiCodec::SetParameter(uint32_t index, const std::vector<int8_t> &paramVec)
{
    CHECK_AND_RETURN_RET_LOG(compNode_ != nullptr, Status::ERROR_NULL_POINTER, "compNode is nullptr!");
    int32_t ret = compNode_->SetParameter(index, paramVec);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, Status::ERROR_INVALID_PARAMETER, "SetParameter failed!");
    return Status::OK;
}

Status HdiCodec::InitBuffers(uint32_t bufferSize)
{
    Status ret;

    ret = InitBuffersByPort(PortIndex::INPUT_PORT, bufferSize);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret, "Init Input Buffers failed, ret=%{public}d", ret);
    ret = InitBuffersByPort(PortIndex::OUTPUT_PORT, bufferSize);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret, "Init Output Buffers failed, ret=%{public}d", ret);

    return Status::OK;
}

Status HdiCodec::InitBuffersByPort(PortIndex portIndex, uint32_t bufferSize)
{
    CHECK_AND_RETURN_RET_LOG(compNode_ != nullptr, Status::ERROR_NULL_POINTER, "compNode is nullptr!");
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(avAllocator, bufferSize);
    if (avBuffer == nullptr) {
        AVCODEC_LOGE("Create avBuffer failed!");
        return Status::ERROR_NO_MEMORY;
    }
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
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, Status::ERROR_NO_MEMORY,
        "InitBuffers failed, ret=%{public}d", ret);

    omxBuffer->bufferId = outBuffer.bufferId;
    if (portIndex == PortIndex::INPUT_PORT) {
        std::unique_lock lock(inMutex_);
        omxInBufferInfo_->omxBuffer = omxBuffer;
        omxInBufferInfo_->avBuffer = avBuffer;
    } else if (portIndex == PortIndex::OUTPUT_PORT) {
        std::unique_lock lock(outMutex_);
        omxOutBufferInfo_->omxBuffer = omxBuffer;
        omxOutBufferInfo_->avBuffer = avBuffer;
    }

    return Status::OK;
}

Status HdiCodec::SendCommand(CodecCommandType cmd, uint32_t param)
{
    std::unique_lock lock(inMutex_);
    CHECK_AND_RETURN_RET_LOG(compNode_ != nullptr, Status::ERROR_NULL_POINTER, "compNode is nullptr!");
    event_ = CODEC_EVENT_ERROR;
    int32_t ret = compNode_->SendCommand(cmd, param, {});
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, Status::ERROR_INVALID_DATA, "SendCommand failed");

    condition_.wait_for(lock, std::chrono::milliseconds(TIMEOUT_MS),
                        [this] { return event_ == CODEC_EVENT_CMD_COMPLETE; });
    CHECK_AND_RETURN_RET_LOG(event_ == CODEC_EVENT_CMD_COMPLETE, Status::ERROR_INVALID_PARAMETER,
        "SendCommand timeout!");

    return Status::OK;
}

Status HdiCodec::EmptyThisBuffer(const std::shared_ptr<AVBuffer> &buffer)
{
    std::unique_lock lock(inMutex_);
    CHECK_AND_RETURN_RET_LOG(compNode_ != nullptr, Status::ERROR_NULL_POINTER, "compNode is nullptr!");
    omxInBufferInfo_->omxBuffer->filledLen = static_cast<uint32_t>(buffer->memory_->GetSize());
    omxInBufferInfo_->omxBuffer->offset = static_cast<uint32_t>(buffer->memory_->GetOffset());
    omxInBufferInfo_->omxBuffer->pts = buffer->pts_;
    omxInBufferInfo_->omxBuffer->flag = buffer->flag_;

    errno_t rc = memcpy_s(omxInBufferInfo_->avBuffer->memory_->GetAddr(), omxInBufferInfo_->omxBuffer->filledLen,
                          buffer->memory_->GetAddr(), omxInBufferInfo_->omxBuffer->filledLen);
    CHECK_AND_RETURN_RET_LOG(rc == EOK, Status::ERROR_INVALID_DATA, "memory copy failed!");

    int32_t ret = compNode_->EmptyThisBuffer(*omxInBufferInfo_->omxBuffer.get());
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, Status::ERROR_INVALID_DATA,
        "EmptyThisBuffer failed, ret=%{public}d", ret);

    AVCODEC_LOGD("EmptyThisBuffer OK");
    return Status::OK;
}

Status HdiCodec::FillThisBuffer(std::shared_ptr<AVBuffer> &buffer)
{
    std::unique_lock lock(outMutex_);
    CHECK_AND_RETURN_RET_LOG(compNode_ != nullptr, Status::ERROR_NULL_POINTER, "compNode is nullptr!");
    outputOmxBuffer_ = nullptr;
    int32_t ret = compNode_->FillThisBuffer(*omxOutBufferInfo_->omxBuffer.get());
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, Status::ERROR_INVALID_DATA, "FillThisBuffer failed!");

    condition_.wait_for(lock, std::chrono::milliseconds(TIMEOUT_MS), [this] { return outputOmxBuffer_ != nullptr; });
    CHECK_AND_RETURN_RET_LOG(outputOmxBuffer_ != nullptr, Status::ERROR_INVALID_PARAMETER, "FillThisBuffer timeout!");

    errno_t rc = memcpy_s(buffer->memory_->GetAddr(), outputOmxBuffer_->filledLen,
                          omxOutBufferInfo_->avBuffer->memory_->GetAddr(), outputOmxBuffer_->filledLen);
    CHECK_AND_RETURN_RET_LOG(rc == EOK, Status::ERROR_INVALID_DATA, "memory copy failed!");

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
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, Status::ERROR_INVALID_DATA,
            "Free buffer fail, portIndex=%{public}d, ret=%{public}d", portIndex, ret);
    }

    AVCODEC_LOGD("FreeBuffer OK");
    return Status::OK;
}

Status HdiCodec::OnEventHandler(CodecEventType event, const EventInfo &info)
{
    std::unique_lock lock(inMutex_);
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
    std::unique_lock lock(outMutex_);
    outputOmxBuffer_ = std::make_shared<OmxCodecBuffer>(buffer);
    condition_.notify_all();
    return Status::OK;
}

Status HdiCodec::Reset()
{
    {
        std::unique_lock lock(inMutex_);
        FreeBuffer(PortIndex::INPUT_PORT, omxInBufferInfo_->omxBuffer);
        omxInBufferInfo_->Reset();
    }
    {
        std::unique_lock lock(outMutex_);
        FreeBuffer(PortIndex::OUTPUT_PORT, omxOutBufferInfo_->omxBuffer);
        omxOutBufferInfo_->Reset();
    }
    return Status::OK;
}

void HdiCodec::Release()
{
    {
        std::unique_lock lock(inMutex_);
        FreeBuffer(PortIndex::INPUT_PORT, omxInBufferInfo_->omxBuffer);
        omxInBufferInfo_->Reset();
    }
    {
        std::unique_lock lock(outMutex_);
        FreeBuffer(PortIndex::OUTPUT_PORT, omxOutBufferInfo_->omxBuffer);
        omxOutBufferInfo_->Reset();
    }
    
    if (compMgr_ != nullptr && componentId_ > 0) {
        compMgr_->DestroyComponent(componentId_);
    }
    compNode_ = nullptr;
    if (compCb_ != nullptr) {
        compCb_ = nullptr;
    }
    compMgr_ = nullptr;
}

HdiCodec::HdiCallback::HdiCallback(std::shared_ptr<HdiCodec> hdiCodec) : hdiCodec_(hdiCodec)
{
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
    if (buffer.fd >= 0) {
        close(buffer.fd);
    }
    return HDF_SUCCESS;
}

int32_t HdiCodec::HdiCallback::FillBufferDone(int64_t appData, const OmxCodecBuffer &buffer)
{
    AVCODEC_LOGD("FillBufferDone");
    if (hdiCodec_) {
        hdiCodec_->OnFillBufferDone(buffer);
    }
    if (buffer.fd >= 0) {
        close(buffer.fd);
    }
    return HDF_SUCCESS;
}
} // namespace Hdi
} // namespace Plugins
} // namespace Media
} // namespace OHOS
