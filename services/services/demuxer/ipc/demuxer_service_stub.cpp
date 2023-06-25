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
#include "demuxer_service_stub.h"
#include <unistd.h>
#include "avcodec_server_manager.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avsharedmemory_ipc.h"
#include "avcodec_xcollie.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "DemuxerServiceStub"};

    const std::map<int32_t, std::string> DEMUXER_FUNC_NAME = {
        { OHOS::MediaAVCodec::DemuxerServiceStub::DemuxerServiceMsg::INIT, "DemuxerServiceStub Init" },
        { OHOS::MediaAVCodec::DemuxerServiceStub::DemuxerServiceMsg::SELECT_TRACK_BY_ID,
            "DemuxerServiceStub SelectTrackByID" },
        { OHOS::MediaAVCodec::DemuxerServiceStub::DemuxerServiceMsg::UNSELECT_TRACK_BY_ID,
            "DemuxerServiceStub UnselectTrackByID" },
        { OHOS::MediaAVCodec::DemuxerServiceStub::DemuxerServiceMsg::READ_SAMPLE, "DemuxerServiceStub ReadSample" },
        { OHOS::MediaAVCodec::DemuxerServiceStub::DemuxerServiceMsg::SEEK_TO_TIME, "DemuxerServiceStub SeekToTime" },
        { OHOS::MediaAVCodec::DemuxerServiceStub::DemuxerServiceMsg::DESTROY_STUB, "DemuxerServiceStub DestroyStub" },
    };
}

namespace OHOS {
namespace MediaAVCodec {
sptr<DemuxerServiceStub> DemuxerServiceStub::Create()
{
    sptr<DemuxerServiceStub> demuxerStub = new(std::nothrow) DemuxerServiceStub();
    CHECK_AND_RETURN_RET_LOG(demuxerStub != nullptr, nullptr, "Failed to create demuxer service stub");

    int32_t ret = demuxerStub->InitStub();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Failed to init DemuxerServiceStub");
    return demuxerStub;
}

DemuxerServiceStub::DemuxerServiceStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

DemuxerServiceStub::~DemuxerServiceStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t DemuxerServiceStub::InitStub()
{
    std::lock_guard<std::mutex> lock(mutex_);
    demuxerServer_ = DemuxerServer::Create();
    CHECK_AND_RETURN_RET_LOG(demuxerServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Failed to create demuxer server");

    demuxerFuncs_[INIT] = &DemuxerServiceStub::Init;
    demuxerFuncs_[SELECT_TRACK_BY_ID] = &DemuxerServiceStub::SelectTrackByID;
    demuxerFuncs_[UNSELECT_TRACK_BY_ID] = &DemuxerServiceStub::UnselectTrackByID;
    demuxerFuncs_[READ_SAMPLE] = &DemuxerServiceStub::ReadSample;
    demuxerFuncs_[SEEK_TO_TIME] = &DemuxerServiceStub::SeekToTime;

    demuxerFuncs_[DESTROY_STUB] = &DemuxerServiceStub::DestroyStub;

    return AVCS_ERR_OK;
}

int DemuxerServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    AVCODEC_LOGD("Stub: OnRemoteRequest of code: %{public}u is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    if (DemuxerServiceStub::GetDescriptor() != remoteDescriptor) {
        AVCODEC_LOGE("Invalid descriptor");
        return AVCS_ERR_INVALID_OPERATION;
    }

    auto itFunc = demuxerFuncs_.find(code);
    if (itFunc != demuxerFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = -1;
            auto itFuncName = DEMUXER_FUNC_NAME.find(code);
            std::string funcName =
                itFuncName != DEMUXER_FUNC_NAME.end() ? itFuncName->second : "DemuxerServiceStub OnRemoteRequest";
            ret = (this->*memberFunc)(data, reply);
            CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret,
                "Failed to call member func %{public}s", funcName.c_str());
            return AVCS_ERR_OK;
        }
    }
    AVCODEC_LOGW("Failed to find corresponding function");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t DemuxerServiceStub::DestroyStub()
{
    std::lock_guard<std::mutex> lock(mutex_);
    demuxerServer_ = nullptr;
    AVCodecServerManager::GetInstance().DestroyStubObject(AVCodecServerManager::DEMUXER, AsObject());
    return AVCS_ERR_OK;
}

int32_t DemuxerServiceStub::Init(uintptr_t sourceAddr)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(demuxerServer_ != nullptr, AVCS_ERR_NO_MEMORY, "demuxer service is nullptr");
    int32_t ret = demuxerServer_->Init(sourceAddr);
    if (ret != AVCS_ERR_OK) {
        lock.unlock();
        DestroyStub();
    }
    return ret;
}

int32_t DemuxerServiceStub::SelectTrackByID(uint32_t trackIndex)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(demuxerServer_ != nullptr, AVCS_ERR_NO_MEMORY, "demuxer service is nullptr");
    return demuxerServer_->SelectTrackByID(trackIndex);
}

int32_t DemuxerServiceStub::UnselectTrackByID(uint32_t trackIndex)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(demuxerServer_ != nullptr, AVCS_ERR_NO_MEMORY, "demuxer service is nullptr");
    return demuxerServer_->UnselectTrackByID(trackIndex);
}

int32_t DemuxerServiceStub::ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
    AVCodecBufferInfo &info, AVCodecBufferFlag &flag)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(demuxerServer_ != nullptr, AVCS_ERR_NO_MEMORY, "demuxer service is nullptr");
    return demuxerServer_->ReadSample(trackIndex, sample, info, flag);
}

int32_t DemuxerServiceStub::SeekToTime(int64_t millisecond, const AVSeekMode mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(demuxerServer_ != nullptr, AVCS_ERR_NO_MEMORY, "demuxer service is nullptr");
    return demuxerServer_->SeekToTime(millisecond, mode);
}

int32_t DemuxerServiceStub::Init(MessageParcel &data, MessageParcel &reply)
{
    uint64_t sourceAddr = data.ReadPointer();
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(Init(sourceAddr)), AVCS_ERR_UNKNOWN, "Reply Init failed!");
    return AVCS_ERR_OK;
}

int32_t DemuxerServiceStub::SelectTrackByID(MessageParcel &data, MessageParcel &reply)
{
    uint32_t trackIndex = data.ReadUint32();

    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(SelectTrackByID(trackIndex)), AVCS_ERR_UNKNOWN,
        "Reply SelectTrackByID failed!");
    return AVCS_ERR_OK;
}

int32_t DemuxerServiceStub::UnselectTrackByID(MessageParcel &data, MessageParcel &reply)
{
    uint32_t trackIndex = data.ReadUint32();

    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(UnselectTrackByID(trackIndex)), AVCS_ERR_UNKNOWN,
        "Reply UnselectTrackByID failed!");
    return AVCS_ERR_OK;
}

int32_t DemuxerServiceStub::ReadSample(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    uint32_t trackIndex = data.ReadUint32();
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    std::shared_ptr<AVSharedMemory> sample = ReadAVSharedMemoryFromParcel(data);
    
    int32_t ret = ReadSample(trackIndex, sample, info, flag);

    WriteAVSharedMemoryToParcel(sample, reply);
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt64(info.presentationTimeUs), AVCS_ERR_UNKNOWN,
        "Write info presentationTimeUs failed when call ReadSample!");
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(info.size), AVCS_ERR_UNKNOWN,
        "Write info size failed when call ReadSample!");
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(info.offset), AVCS_ERR_UNKNOWN,
        "Write info offset failed when call ReadSample!");
    CHECK_AND_RETURN_RET_LOG(reply.WriteUint32(static_cast<uint32_t>(flag)), AVCS_ERR_UNKNOWN,
        "Write info flag failed when call ReadSample!");
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(ret), AVCS_ERR_UNKNOWN, "Reply ReadSample failed!");
    return AVCS_ERR_OK;
}

int32_t DemuxerServiceStub::SeekToTime(MessageParcel &data, MessageParcel &reply)
{
    int64_t millisecond = data.ReadInt64();
    AVSeekMode seekMode = static_cast<AVSeekMode>(data.ReadInt32());

    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(SeekToTime(millisecond, seekMode)),
        AVCS_ERR_UNKNOWN, "Reply SeekToTime failed!");
    return AVCS_ERR_OK;
}

int32_t DemuxerServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(DestroyStub()), AVCS_ERR_UNKNOWN, "Reply DestroyStub failed!");
    return AVCS_ERR_OK;
}
}  // namespace MediaAVCodec
}  // namespace OHOS