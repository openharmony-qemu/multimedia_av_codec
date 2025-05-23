/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "DataStreamSourcePlugin"

#ifndef OHOS_LITE
#include "data_stream_source_plugin.h"
#include "common/log.h"
#include "common/media_core.h"
#include "osal/utils/util.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "HiStreamer" };
}

namespace OHOS {
namespace Media {
namespace Plugin {
namespace DataStreamSource {
namespace {
    constexpr uint32_t INIT_MEM_CNT = 10;
    constexpr int32_t MEM_SIZE = 10240;
    constexpr uint32_t MAX_MEM_CNT = 10 * 1024;
    constexpr size_t DEFAULT_PREDOWNLOAD_SIZE_BYTE = 10 * 1024 * 1024;
    constexpr uint32_t DEFAULT_RETRY_TIMES = 20;
    constexpr uint32_t READ_AGAIN_RETRY_TIME_ONE = 100;
    constexpr uint32_t READ_AGAIN_RETRY_TIME_TWO = 200;
    constexpr uint32_t READ_AGAIN_RETRY_TIME_THREE = 500;
    constexpr uint32_t RETRY_TIMES_ONE = 5; // The number of attempts determines the sleep duration.
    constexpr uint32_t RETRY_TIMES_TWO = 15;
}
std::shared_ptr<Plugins::SourcePlugin> DataStreamSourcePluginCreator(const std::string& name)
{
    return std::make_shared<DataStreamSourcePlugin>(name);
}

Status DataStreamSourceRegister(const std::shared_ptr<Plugins::Register>& reg)
{
    Plugins::SourcePluginDef definition;
    definition.name = "DataStreamSource";
    definition.description = "Data stream source";
    definition.rank = 100; // 100: max rank
    Plugins::Capability capability;
    capability.AppendFixedKey<std::vector<Plugins::ProtocolType>>(
        Tag::MEDIA_PROTOCOL_TYPE, {Plugins::ProtocolType::STREAM});
    definition.AddInCaps(capability);
    definition.SetCreator(DataStreamSourcePluginCreator);
    return reg->AddPlugin(definition);
}

PLUGIN_DEFINITION(DataStreamSource, Plugins::LicenseType::APACHE_V2, DataStreamSourceRegister, [] {});

DataStreamSourcePlugin::DataStreamSourcePlugin(std::string name)
    : SourcePlugin(std::move(name))
{
    MEDIA_LOG_D("ctor called");
    pool_ = std::make_shared<AVSharedMemoryPool>("pool");
    InitPool();
}

DataStreamSourcePlugin::~DataStreamSourcePlugin()
{
    MEDIA_LOG_D("dtor called");
    ResetPool();
}

Status DataStreamSourcePlugin::SetSource(std::shared_ptr<Plugins::MediaSource> source)
{
    dataSrc_ = source->GetDataSrc();
    FALSE_RETURN_V(dataSrc_ != nullptr, Status::ERROR_INVALID_PARAMETER);
    int64_t size = 0;
    if (dataSrc_->GetSize(size) != 0) {
        MEDIA_LOG_E("Get size failed");
    }
    size_ = size;
    seekable_ = size_ == -1 ? Plugins::Seekable::UNSEEKABLE : Plugins::Seekable::SEEKABLE;
    MEDIA_LOG_I("SetSource, size_: " PUBLIC_LOG_D64 ", seekable_: " PUBLIC_LOG_D32, size_, seekable_);
    return Status::OK;
}

Status DataStreamSourcePlugin::SetCallback(Plugins::Callback* cb)
{
    MEDIA_LOG_D("IN");
    callback_ = cb;
    MEDIA_LOG_D("OUT");
    return Status::OK;
}

std::shared_ptr<Plugins::Buffer> DataStreamSourcePlugin::WrapAVSharedMemory(
    const std::shared_ptr<AVSharedMemory>& avSharedMemory, int32_t realLen)
{
    std::shared_ptr<Plugins::Buffer> buffer = std::make_shared<Plugins::Buffer>();
    std::shared_ptr<uint8_t> address = std::shared_ptr<uint8_t>(avSharedMemory->GetBase(),
                                                                [avSharedMemory](uint8_t* ptr) { ptr = nullptr; });
    buffer->WrapMemoryPtr(address, avSharedMemory->GetSize(), realLen);
    return buffer;
}

void DataStreamSourcePlugin::InitPool()
{
    AVSharedMemoryPool::InitializeOption InitOption {
        INIT_MEM_CNT,
        MEM_SIZE,
        MAX_MEM_CNT,
        AVSharedMemory::Flags::FLAGS_READ_WRITE,
        true,
        nullptr,
    };
    pool_->Init(InitOption);
    pool_->GetName();
    pool_->Reset();
}

std::shared_ptr<AVSharedMemory> DataStreamSourcePlugin::GetMemory()
{
    return pool_->AcquireMemory(MEM_SIZE); // 10240
}

void DataStreamSourcePlugin::ResetPool()
{
    pool_->Reset();
}

void DataStreamSourcePlugin::WaitForRetry(uint32_t time)
{
    std::unique_lock<std::mutex> lock(mutex_);
    readCond_.wait_for(lock, std::chrono::milliseconds(time), [&] {
        return isInterrupted_.load() || isExitRead_.load();
    });
}

Status DataStreamSourcePlugin::Read(std::shared_ptr<Plugins::Buffer>& buffer, uint64_t offset, size_t expectedLen)
{
    MEDIA_LOG_D("Read, offset: " PUBLIC_LOG_D64 ", expectedLen: " PUBLIC_LOG_ZU ", seekable: " PUBLIC_LOG_D32,
        offset, expectedLen, seekable_);
    std::shared_ptr<AVSharedMemory> memory = GetMemory();
    FALSE_RETURN_V_MSG(memory != nullptr, Status::ERROR_NO_MEMORY, "allocate memory failed!");
    int32_t realLen = 0;
    do {
        if (isInterrupted_ || isExitRead_) {
            retryTimes_ = 0;
            isBufferingStart = false;
            return Status::OK;
        }
        FALSE_RETURN_V_MSG(dataSrc_ != nullptr, Status::ERROR_WRONG_STATE, "dataSrc_ is nullptr!");
        if (seekable_ == Plugins::Seekable::SEEKABLE) {
            FALSE_RETURN_V(static_cast<int64_t>(offset_) <= size_, Status::END_OF_STREAM);
            expectedLen = std::min(static_cast<size_t>(size_ - offset_), expectedLen);
            expectedLen = std::min(static_cast<size_t>(memory->GetSize()), expectedLen);
            realLen = dataSrc_->ReadAt(static_cast<int64_t>(offset_), expectedLen, memory);
        } else {
            expectedLen = std::min(static_cast<size_t>(memory->GetSize()), expectedLen);
            realLen = dataSrc_->ReadAt(expectedLen, memory);
        }
        FALSE_RETURN_V_MSG(realLen > MediaDataSourceError::SOURCE_ERROR_IO, Status::ERROR_UNKNOWN,
            "read data error! realLen:" PUBLIC_LOG_D32, realLen);
        FALSE_RETURN_V_MSG_W(realLen != MediaDataSourceError::SOURCE_ERROR_EOF, Status::END_OF_STREAM, "eos reached!");
        if (realLen > 0) {
            FALSE_LOG_MSG_W(realLen == static_cast<int32_t>(expectedLen), "realLen != expectedLen, realLen:"
                PUBLIC_LOG_D32 ", expectedLen: " PUBLIC_LOG_ZU, realLen, expectedLen);
            retryTimes_ = 0;
            HandleBufferingEnd();
            break;
        }
        if (realLen == 0) {
            HandleBufferingStart();
        }
        WaitForRetry(GetRetryTime());
        retryTimes_++;
    } while (retryTimes_ < DEFAULT_RETRY_TIMES);
    offset_ += static_cast<uint64_t>(realLen);
    if (buffer && buffer->GetMemory()) {
        buffer->GetMemory()->Write(memory->GetBase(), realLen, 0);
    } else {
        buffer = WrapAVSharedMemory(memory, realLen);
    }
    FALSE_RETURN_V(buffer != nullptr, Status::ERROR_AGAIN);
    MEDIA_LOG_D("DataStreamSourcePlugin Read, size: " PUBLIC_LOG_ZU ", realLen: " PUBLIC_LOG_D32
        ", retryTimes: " PUBLIC_LOG_U32, (buffer && buffer->GetMemory()) ?
        buffer->GetMemory()->GetSize() : -100, realLen, retryTimes_); // -100 invalid size
    FALSE_RETURN_V(realLen != 0, Status::ERROR_AGAIN);
    return Status::OK;
}

uint32_t DataStreamSourcePlugin::GetRetryTime()
{
    MEDIA_LOG_I("read again. retryTimes:" PUBLIC_LOG_U32, retryTimes_);
    FALSE_RETURN_V(retryTimes_ > RETRY_TIMES_ONE, READ_AGAIN_RETRY_TIME_ONE);
    FALSE_RETURN_V(retryTimes_ > RETRY_TIMES_TWO, READ_AGAIN_RETRY_TIME_TWO);
    return READ_AGAIN_RETRY_TIME_THREE;
}

void DataStreamSourcePlugin::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("OnInterrupted %{public}d", isInterruptNeeded);
    std::unique_lock<std::mutex> lock(mutex_);
    isInterrupted_ = isInterruptNeeded;
    isExitRead_ = isInterruptNeeded;
    readCond_.notify_all();
}

void DataStreamSourcePlugin::HandleBufferingStart()
{
    if (!isBufferingStart) {
        isBufferingStart = true;
        if (callback_ != nullptr) {
            MEDIA_LOG_I("OnEvent BUFFERING_START.");
            callback_->OnEvent({Plugins::PluginEventType::BUFFERING_START,
                {BufferingInfoType::BUFFERING_START}, "pause"});
        }
    }
}

void DataStreamSourcePlugin::HandleBufferingEnd()
{
    if (isBufferingStart) {
        isBufferingStart = false;
        if (callback_ != nullptr) {
            MEDIA_LOG_I("OnEvent BUFFERING_END.");
            callback_->OnEvent({Plugins::PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
        }
    }
}

Status DataStreamSourcePlugin::GetSize(uint64_t& size)
{
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        size = static_cast<uint64_t>(size_);
    } else {
        size = std::max(static_cast<size_t>(offset_), DEFAULT_PREDOWNLOAD_SIZE_BYTE);
    }
    return Status::OK;
}

Plugins::Seekable DataStreamSourcePlugin::GetSeekable()
{
    return seekable_;
}

Status DataStreamSourcePlugin::SeekTo(uint64_t offset)
{
    if (seekable_ == Plugins::Seekable::UNSEEKABLE) {
        MEDIA_LOG_E("source is unseekable!");
        return Status::ERROR_INVALID_OPERATION;
    }
    if (offset >= static_cast<uint64_t>(size_)) {
        MEDIA_LOG_E("Invalid parameter");
        return Status::ERROR_INVALID_PARAMETER;
    }
    offset_ = offset;
    isExitRead_ = false;
    MEDIA_LOG_D("seek to offset_ " PUBLIC_LOG_U64 " success", offset_);
    return Status::OK;
}

Status DataStreamSourcePlugin::Pause()
{
    MEDIA_LOG_I("Pause enter.");
    isExitRead_ = true;
    return Status::OK;
}
 
Status DataStreamSourcePlugin::Resume()
{
    MEDIA_LOG_I("Resume enter.");
    isExitRead_ = false;
    return Status::OK;
}

Status DataStreamSourcePlugin::Reset()
{
    MEDIA_LOG_I("Reset enter.");
    isInterrupted_ = true;
    return Status::OK;
}

bool DataStreamSourcePlugin::IsNeedPreDownload()
{
    return true;
}
} // namespace DataStreamSourcePlugin
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif