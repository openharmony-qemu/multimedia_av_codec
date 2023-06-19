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

#include "source_service_stub.h"
#include <map>
#include "avcodec_server_manager.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_dfx.h"
#include "avsharedmemory_ipc.h"
#include "avcodec_parcel.h"
#include "avcodec_xcollie.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SourceAVServiceStub"};

    const std::map<int32_t, std::string> SOURCE_FUNC_NAME = {
        { OHOS::MediaAVCodec::SourceServiceStub::SourceServiceMsg::INIT_WITH_URI, "SourceServiceStub InitWithURI" },
        { OHOS::MediaAVCodec::SourceServiceStub::SourceServiceMsg::INIT_WITH_FD, "SourceServiceStub InitWithFD" },
        { OHOS::MediaAVCodec::SourceServiceStub::SourceServiceMsg::GET_TRACK_COUNT,
            "SourceServiceStub GetTrackCount" },
        { OHOS::MediaAVCodec::SourceServiceStub::SourceServiceMsg::DESTROY, "SourceServiceStub Destroy" },
        { OHOS::MediaAVCodec::SourceServiceStub::SourceServiceMsg::GET_TRACK_FORMAT,
            "SourceServiceStub GetTrackFormat" },
        { OHOS::MediaAVCodec::SourceServiceStub::SourceServiceMsg::GET_SOURCE_FORMAT,
            "SourceServiceStub GetSourceFormat" },
        { OHOS::MediaAVCodec::SourceServiceStub::SourceServiceMsg::GET_SOURCE_ADDR,
            "SourceServiceStub GetSourceAddr" },
        { OHOS::MediaAVCodec::SourceServiceStub::SourceServiceMsg::DESTROY_STUB, "SourceServiceStub DestroyStub" },
    };
}

namespace OHOS {
namespace MediaAVCodec {
sptr<SourceServiceStub> SourceServiceStub::Create()
{
    sptr<SourceServiceStub> sourceStub = new(std::nothrow) SourceServiceStub();
    CHECK_AND_RETURN_RET_LOG(sourceStub != nullptr, nullptr, "Failed to create source service stub");

    int32_t ret = sourceStub->InitStub();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Failed to init SourceServiceStub");
    return sourceStub;
}

SourceServiceStub::SourceServiceStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

SourceServiceStub::~SourceServiceStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t SourceServiceStub::InitStub()
{
    std::lock_guard<std::mutex> lock(mutex_);
    sourceServer_ = SourceServer::Create();
    CHECK_AND_RETURN_RET_LOG(sourceServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Failed to create source server");

    sourceFuncs_[INIT_WITH_URI] = &SourceServiceStub::InitWithURI;
    sourceFuncs_[INIT_WITH_FD] = &SourceServiceStub::InitWithFD;
    sourceFuncs_[GET_TRACK_COUNT] = &SourceServiceStub::GetTrackCount;
    sourceFuncs_[GET_TRACK_FORMAT] = &SourceServiceStub::GetTrackFormat;
    sourceFuncs_[GET_SOURCE_FORMAT] = &SourceServiceStub::GetSourceFormat;
    sourceFuncs_[GET_SOURCE_ADDR] = &SourceServiceStub::GetSourceAddr;
    sourceFuncs_[DESTROY_STUB] = &SourceServiceStub::DestroyStub;
    return AVCS_ERR_OK;
}

int SourceServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    AVCODEC_LOGD("Stub: OnRemoteRequest of code: %{public}u is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    if (SourceServiceStub::GetDescriptor() != remoteDescriptor) {
        AVCODEC_LOGE("Invalid descriptor");
        return AVCS_ERR_INVALID_OPERATION;
    }

    auto itFunc = sourceFuncs_.find(code);
    if (itFunc != sourceFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = -1;
            auto itFuncName = SOURCE_FUNC_NAME.find(code);
            std::string funcName =
                itFuncName != SOURCE_FUNC_NAME.end() ? itFuncName->second : "SourceServiceStub OnRemoteRequest";
            ret = (this->*memberFunc)(data, reply);
            CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret,
                "Failed to call member func %{public}s", funcName.c_str());
            return AVCS_ERR_OK;
        }
    }
    AVCODEC_LOGW("Failed to find corresponding function");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t SourceServiceStub::DestroyStub()
{
    std::lock_guard<std::mutex> lock(mutex_);
    sourceServer_ = nullptr;
    AVCodecServerManager::GetInstance().DestroyStubObject(AVCodecServerManager::SOURCE, AsObject());
    return AVCS_ERR_OK;
}

int32_t SourceServiceStub::InitWithURI(const std::string &uri)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(sourceServer_ != nullptr, AVCS_ERR_NO_MEMORY, "source server is nullptr");
    int32_t ret = sourceServer_->InitWithURI(uri);
    if (ret != AVCS_ERR_OK) {
        lock.unlock();
        DestroyStub();
    }
    return ret;
}

int32_t SourceServiceStub::InitWithFD(int32_t fd, int64_t offset, int64_t size)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(sourceServer_ != nullptr, AVCS_ERR_NO_MEMORY, "source server is nullptr");
    int32_t ret = sourceServer_->InitWithFD(fd, offset, size);
    if (ret != AVCS_ERR_OK) {
        lock.unlock();
        DestroyStub();
    }
    return ret;
}

int32_t SourceServiceStub::GetTrackCount(uint32_t &trackCount)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(sourceServer_ != nullptr, AVCS_ERR_NO_MEMORY, "source server is nullptr");
    return sourceServer_->GetTrackCount(trackCount);
}

int32_t SourceServiceStub::GetTrackFormat(Format &format, uint32_t trackIndex)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(sourceServer_ != nullptr, AVCS_ERR_NO_MEMORY, "source server is nullptr");
    return sourceServer_->GetTrackFormat(format, trackIndex);
}

int32_t SourceServiceStub::GetSourceFormat(Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(sourceServer_ != nullptr, AVCS_ERR_NO_MEMORY, "source server is nullptr");
    return sourceServer_->GetSourceFormat(format);
}

int32_t SourceServiceStub::GetSourceAddr(uintptr_t &addr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(sourceServer_ != nullptr, AVCS_ERR_NO_MEMORY, "source server is nullptr");
    return sourceServer_->GetSourceAddr(addr);
}

int32_t SourceServiceStub::DumpInfo(int32_t fd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(sourceServer_ != nullptr, AVCS_ERR_NO_MEMORY, "source server is nullptr");
    return std::static_pointer_cast<SourceServer>(sourceServer_)->DumpInfo(fd);
}

int32_t SourceServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(DestroyStub()), AVCS_ERR_UNKNOWN, "Reply DestroyStub failed!");
    return AVCS_ERR_OK;
}

int32_t SourceServiceStub::InitWithURI(MessageParcel &data, MessageParcel &reply)
{
    std::string uri = data.ReadString();
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(InitWithURI(uri)), AVCS_ERR_UNKNOWN, "Reply InitWithURI failed");
    return AVCS_ERR_OK;
}

int32_t SourceServiceStub::InitWithFD(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    int64_t offset = data.ReadInt64();
    int64_t size = data.ReadInt64();
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(InitWithFD(fd, offset, size)), AVCS_ERR_UNKNOWN,
        "Reply InitWithFD failed");
    return AVCS_ERR_OK;
}

int32_t SourceServiceStub::GetTrackCount(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    uint32_t trackCount = 0;
    int32_t ret = GetTrackCount(trackCount);
    CHECK_AND_RETURN_RET_LOG(reply.WriteUint32(trackCount), AVCS_ERR_UNKNOWN,
        "Write trackCount failed when call GetTrackCount");
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(ret), AVCS_ERR_UNKNOWN, "Reply GetTrackCount failed");
    return AVCS_ERR_OK;
}

int32_t SourceServiceStub::GetTrackFormat(MessageParcel &data, MessageParcel &reply)
{
    uint32_t trackIndex = data.ReadUint32();
    Format format;
    int32_t ret = GetTrackFormat(format, trackIndex);
    AVCodecParcel::Marshalling(reply, format);
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(ret), AVCS_ERR_UNKNOWN, "Reply GetTrackFormat failed");
    return AVCS_ERR_OK;
}

int32_t SourceServiceStub::GetSourceFormat(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    Format format;
    int32_t ret = GetSourceFormat(format);
    AVCodecParcel::Marshalling(reply, format);
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(ret), AVCS_ERR_UNKNOWN, "Reply GetSourceAddr failed");
    return AVCS_ERR_OK;
}

int32_t SourceServiceStub::GetSourceAddr(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    uintptr_t addr;
    auto ret = GetSourceAddr(addr);
    CHECK_AND_RETURN_RET_LOG(reply.WritePointer(addr), AVCS_ERR_UNKNOWN, "Write addr faile when call GetSourceAddr!");
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(ret), AVCS_ERR_UNKNOWN, "Reply GetSourceAddr failed!");
    return AVCS_ERR_OK;
}
}  // namespace MediaAVCodec
}  // namespace OHOS