/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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
#define HST_LOG_TAG "HttpSourcePlugin"

#include "http_source_plugin.h"
#include "download/http_curl_client.h"
#include "common/log.h"
#include "http/http_media_downloader.h"
#include "monitor/download_monitor.h"
#undef ERROR_INVALID_OPERATION

namespace OHOS {
namespace Media {
namespace Plugin {
namespace HttpPlugin {
namespace {
constexpr int DEFAULT_BUFFER_SIZE = 200 * 1024;
}

std::shared_ptr<SourcePlugin> HttpSourcePluginCreater(const std::string& name)
{
    return std::make_shared<HttpSourcePlugin>(name);
}

Status HttpSourceRegister(std::shared_ptr<Register> reg)
{
    SourcePluginDef definition;
    definition.name = "HttpSource";
    definition.description = "Http source";
    definition.rank = 100; // 100
    Capability capability;
    capability.AppendFixedKey<std::vector<ProtocolType>>(Tag::MEDIA_PROTOCOL_TYPE,
        {ProtocolType::HTTP, ProtocolType::HTTPS});
    definition.AddInCaps(capability);
    definition.SetCreator(HttpSourcePluginCreater);
    return reg->AddPlugin(definition);
}
PLUGIN_DEFINITION(HttpSource, LicenseType::APACHE_V2, HttpSourceRegister, [] {});

HttpSourcePlugin::HttpSourcePlugin(std::string name) noexcept
    : SourcePlugin(std::move(name)),
      bufferSize_(DEFAULT_BUFFER_SIZE),
      waterline_(0),
      downloader_(nullptr)
{
    MEDIA_LOG_D("HttpSourcePlugin enter.");
}

HttpSourcePlugin::~HttpSourcePlugin()
{
    MEDIA_LOG_D("~HttpSourcePlugin enter.");
    CloseUri();
}

Status HttpSourcePlugin::Init()
{
    MEDIA_LOG_D("Init enter.");
    return Status::OK;
}

Status HttpSourcePlugin::Deinit()
{
    MEDIA_LOG_D("Deinit enter.");
    CloseUri();
    return Status::OK;
}

Status HttpSourcePlugin::Prepare()
{
    MEDIA_LOG_D("Prepare enter.");
    if (delayReady) {
        return Status::ERROR_DELAY_READY;
    }
    return Status::OK;
}

Status HttpSourcePlugin::Reset()
{
    MEDIA_LOG_D("Reset enter.");
    CloseUri();
    return Status::OK;
}

Status HttpSourcePlugin::Start()
{
    MEDIA_LOG_D("Start enter.");
    return Status::OK;
}

Status HttpSourcePlugin::Stop()
{
    MEDIA_LOG_I("Stop enter.");
    CloseUri();
    return Status::OK;
}

#undef ERROR_INVALID_PARAMETER

Status HttpSourcePlugin::GetParameter(std::shared_ptr<Meta> meta)
{
    MEDIA_LOG_I("GetParameter enter.");
    return Status::OK;
}

Status HttpSourcePlugin::SetParameter(std::shared_ptr<Meta> meta)
{
    MEDIA_LOG_I("SetParameter enter.");
    meta->GetData(Tag::BUFFERING_SIZE, bufferSize_);
    meta->GetData(Tag::WATERLINE_HIGH, waterline_);
    return Status::OK;
}

Status HttpSourcePlugin::SetCallback(Callback* cb)
{
    MEDIA_LOG_D("SetCallback enter.");
    callback_ = cb;
    AutoLock lock(mutex_);
    if (downloader_ != nullptr) {
        downloader_->SetCallback(cb);
    }
    return Status::OK;
}

Status HttpSourcePlugin::SetSource(std::shared_ptr<MediaSource> source)
{
    MEDIA_LOG_D("SetSource enter.");
    AutoLock lock(mutex_);
    FALSE_RETURN_V(downloader_ == nullptr, Status::ERROR_INVALID_OPERATION); // not allowed set again
    auto uri = source->GetSourceUri();
    if (uri.find(".m3u8") != std::string::npos) {
        delayReady = false;
    } else if (uri.compare(0, 4, "http") == 0) { // 0 : position, 4: count
        downloader_ = std::make_shared<DownloadMonitor>(std::make_shared<HttpMediaDownloader>());
    }
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);

    if (callback_ != nullptr) {
        downloader_->SetCallback(callback_);
    }

    MEDIA_LOG_I("SetSource: " PUBLIC_LOG_S, uri.c_str());
    FALSE_RETURN_V(downloader_->Open(uri), Status::ERROR_UNKNOWN);
    return Status::OK;
}

Status HttpSourcePlugin::Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
{
    MEDIA_LOG_D("Read enter.");
    AutoLock lock(mutex_);
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);

    if (buffer == nullptr) {
        buffer = std::make_shared<Buffer>();
    }

    std::shared_ptr<Memory>bufData;
    if (buffer->IsEmpty()) {
        MEDIA_LOG_W("buffer is empty.");
        bufData = buffer->AllocMemory(nullptr, expectedLen);
    } else {
        bufData = buffer->GetMemory();
    }

    bool isEos = false;
    unsigned int realReadSize = 0;
    bool result = downloader_->Read(bufData->GetWritableAddr(expectedLen), expectedLen, realReadSize, isEos);
    bufData->UpdateDataSize(realReadSize);
    MEDIA_LOG_D("Read finished, read size = " PUBLIC_LOG_ZU ", isEos " PUBLIC_LOG_D32, bufData->GetSize(), isEos);
    return result ? Status::OK : Status::END_OF_STREAM;
}

Status HttpSourcePlugin::GetSize(uint64_t& size)
{
    MEDIA_LOG_D("GetSize enter.");
    AutoLock lock(mutex_);
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    size = static_cast<uint64_t>(downloader_->GetContentLength());
    return Status::OK;
}

Seekable HttpSourcePlugin::GetSeekable()
{
    MEDIA_LOG_D("GetSeekable enter.");
    AutoLock lock(mutex_);
    FALSE_RETURN_V(downloader_ != nullptr, Seekable::INVALID);
    return downloader_->GetSeekable();
}

Status HttpSourcePlugin::SeekTo(uint64_t offset)
{
    MEDIA_LOG_I("SeekTo enter, offset = " PUBLIC_LOG_U64, offset);
    AutoLock lock(mutex_);
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    FALSE_RETURN_V(downloader_->GetSeekable() == Seekable::SEEKABLE, Status::ERROR_INVALID_OPERATION);
    FALSE_RETURN_V(offset <= downloader_->GetContentLength(), Status::ERROR_INVALID_PARAMETER);
    FALSE_RETURN_V(downloader_->SeekToPos(offset), Status::ERROR_UNKNOWN);
    return Status::OK;
}

Status HttpSourcePlugin::SeekToTime(int64_t offset)
{
    AutoLock lock(mutex_);
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    FALSE_RETURN_V(downloader_->GetSeekable() == Seekable::SEEKABLE, Status::ERROR_INVALID_OPERATION);
    FALSE_RETURN_V(offset <= downloader_->GetDuration(), Status::ERROR_INVALID_PARAMETER);
    FALSE_RETURN_V(downloader_->SeekToTime(offset), Status::ERROR_UNKNOWN);
    return Status::OK;
}


void HttpSourcePlugin::CloseUri()
{
    AutoLock lock(mutex_);
    if (downloader_ != nullptr) {
        MEDIA_LOG_D("Close uri");
        downloader_->Close(false);
        downloader_ = nullptr;
    }
}

Status HttpSourcePlugin::GetDuration(int64_t& duration)
{
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    duration = downloader_->GetDuration();
    return Status::OK;
}

Status HttpSourcePlugin::GetBitRates(std::vector<uint32_t>& bitRates)
{
    MEDIA_LOG_I("HttpSourcePlugin::GetBitRates() enter.\n");
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    bitRates = downloader_->GetBitRates();
    return Status::OK;
}

Status HttpSourcePlugin::SelectBitRate(uint32_t bitRate)
{
    return  static_cast<Status>(downloader_->SelectBitRate(bitRate));
}
}
}
}
}