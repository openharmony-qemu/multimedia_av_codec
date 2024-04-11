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

#define HST_LOG_TAG "DataStreamSourcePlugin"

#ifndef OHOS_LITE
#include "data_stream_source_plugin.h"
#include "common/log.h"
#include "osal/utils/util.h"

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

Status DataStreamSourcePlugin::Read(std::shared_ptr<Plugins::Buffer>& buffer, uint64_t offset, size_t expectedLen)
{
    MEDIA_LOG_D("Read, offset: " PUBLIC_LOG_D64 ", expectedLen: " PUBLIC_LOG_ZU ", seekable: " PUBLIC_LOG_D32,
        offset, expectedLen, seekable_);
    std::shared_ptr<AVSharedMemory> memory = GetMemory();
    FALSE_RETURN_V_MSG(memory != nullptr, Status::ERROR_NO_MEMORY, "allocate memory failed!");
    int32_t realLen;
    do {
        if (seekable_ == Plugins::Seekable::SEEKABLE) {
            FALSE_RETURN_V(static_cast<int64_t>(offset_) <= size_, Status::END_OF_STREAM);
            expectedLen = std::min(static_cast<size_t>(size_ - offset_), expectedLen);
            expectedLen = std::min(static_cast<size_t>(memory->GetSize()), expectedLen);
            realLen = dataSrc_->ReadAt(static_cast<int64_t>(offset_), expectedLen, memory);
        } else {
            expectedLen = std::min(static_cast<size_t>(memory->GetSize()), expectedLen);
            realLen = dataSrc_->ReadAt(expectedLen, memory);
        }
        if (realLen <= 0) {
            OSAL::SleepFor(50); // 50ms sleep time for connect
            retryTimes_++;
        } else {
            retryTimes_ = 0;
        }
    } while (realLen <= 0 && retryTimes_ < DEFAULT_RETRY_TIMES);
    FALSE_RETURN_V_MSG(realLen != MediaDataSourceError::SOURCE_ERROR_IO, Status::ERROR_UNKNOWN, "read data error!");
    FALSE_RETURN_V_MSG(realLen != MediaDataSourceError::SOURCE_ERROR_EOF, Status::END_OF_STREAM, "eos reached!");
    offset_ += realLen;
    if (buffer && buffer->GetMemory()) {
        buffer->GetMemory()->Write(memory->GetBase(), realLen, 0);
    } else {
        buffer = WrapAVSharedMemory(memory, realLen);
    }
    MEDIA_LOG_D("DataStreamSourcePlugin Read, size: " PUBLIC_LOG_ZU ", realLen: " PUBLIC_LOG_D32
        ", retryTimes: " PUBLIC_LOG_U32, (buffer && buffer->GetMemory()) ?
        buffer->GetMemory()->GetSize() : -100, realLen, retryTimes_); // -100 invalid size
    return Status::OK;
}

Status DataStreamSourcePlugin::GetSize(uint64_t& size)
{
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        size = size_;
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
    MEDIA_LOG_D("seek to offset_ " PUBLIC_LOG_U64 " success", offset_);
    return Status::OK;
}

Status DataStreamSourcePlugin::Reset()
{
    MEDIA_LOG_D("IN");
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