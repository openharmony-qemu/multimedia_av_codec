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

#include "avcodec_trace.h"
#include "http_source_plugin.h"
#include "download/http_curl_client.h"
#include "common/log.h"
#include "hls/hls_media_downloader.h"
#include "dash/dash_media_downloader.h"
#include "http/http_media_downloader.h"
#include "monitor/download_monitor.h"
#undef ERROR_INVALID_OPERATION

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr int DEFAULT_BUFFER_SIZE = 200 * 1024;
constexpr int ERROR_COUNT = 5;
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

HttpSourcePlugin::HttpSourcePlugin(const std::string &name) noexcept
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

Status HttpSourcePlugin::SetReadBlockingFlag(bool isReadBlockingAllowed)
{
    MEDIA_LOG_D("SetReadBlockingFlag entered, IsReadBlockingAllowed %{public}d", isReadBlockingAllowed);
    FALSE_RETURN_V(downloader_ != nullptr, Status::OK);
    downloader_->SetReadBlockingFlag(isReadBlockingAllowed);
    return Status::OK;
}

Status HttpSourcePlugin::GetStreamInfo(std::vector<StreamInfo>& streams)
{
    MEDIA_LOG_D("GetStreamInfo entered");
    FALSE_RETURN_V(downloader_ != nullptr, Status::OK);
    downloader_->GetStreamInfo(streams);
    return Status::OK;
}

Status HttpSourcePlugin::SelectStream(int32_t streamID)
{
    MEDIA_LOG_D("SelectStream entered");
    FALSE_RETURN_V(downloader_ != nullptr, Status::OK);
    downloader_->SelectStream(streamID);
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
    CloseUri(true);
    return Status::OK;
}

Status HttpSourcePlugin::Pause()
{
    MEDIA_LOG_I("Pause enter.");
    if (downloader_ != nullptr && uri_.find(".m3u8") != std::string::npos) {
        downloader_->Pause();
    }
    return Status::OK;
}

Status HttpSourcePlugin::Resume()
{
    MEDIA_LOG_I("Resume enter.");
    if (downloader_ != nullptr && uri_.find(".m3u8") != std::string::npos) {
        downloader_->Resume();
    }
    return Status::OK;
}

#undef ERROR_INVALID_PARAMETER

Status HttpSourcePlugin::GetParameter(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("GetParameter enter.");
    return Status::OK;
}

Status HttpSourcePlugin::SetParameter(const std::shared_ptr<Meta> &meta)
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
    MediaAVCodec::AVCodecTrace trace("HttpSourcePlugin::SetSource");
    MEDIA_LOG_D("SetSource enter.");
    AutoLock lock(mutex_);
    FALSE_RETURN_V(downloader_ == nullptr, Status::ERROR_INVALID_OPERATION); // not allowed set again
    FALSE_RETURN_V(source != nullptr, Status::ERROR_INVALID_OPERATION);
    SetDownloaderBySource(source);
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    if (callback_ != nullptr) {
        downloader_->SetCallback(callback_);
    }
    FALSE_RETURN_V(downloader_->Open(uri_, httpHeader_), Status::ERROR_UNKNOWN);
    return Status::OK;
}

void HttpSourcePlugin::SetDownloaderBySource(std::shared_ptr<MediaSource> source)
{
    std::shared_ptr<PlayStrategy> playStrategy;
    if (source != nullptr) {
        uri_ = source->GetSourceUri();
        httpHeader_ = source->GetSourceHeader();
        playStrategy = source->GetPlayStrategy();
        mimeType_ = source->GetMimeType();
    }

    if (uri_.find(".mpd") != std::string::npos) {
        downloader_ = std::make_shared<DownloadMonitor>(std::make_shared<DashMediaDownloader>());
        if (playStrategy != nullptr) {
            downloader_->SetPlayStrategy(playStrategy);
        }
        delayReady = false;
    } else if (IsSeekToTimeSupported() && mimeType_ != AVMimeTypes::APPLICATION_M3U8) {
        if (playStrategy != nullptr && playStrategy->duration > 0) {
            uint32_t expectDuration = playStrategy->duration;
            downloader_ = std::make_shared<DownloadMonitor>(std::make_shared<HlsMediaDownloader>(expectDuration));
        } else {
            downloader_ = std::make_shared<DownloadMonitor>(std::make_shared<HlsMediaDownloader>());
        }
        delayReady = false;
    } else if (uri_.compare(0, 4, "http") == 0) { // 0 : position, 4: count
        if (playStrategy != nullptr && playStrategy->duration > 0) {
            uint32_t expectDuration = playStrategy->duration;
            downloader_ = std::make_shared<DownloadMonitor>(std::make_shared<HttpMediaDownloader>
                                                            (source->GetSourceUri(), expectDuration));
        } else {
            downloader_ = std::make_shared<DownloadMonitor>(std::make_shared<HttpMediaDownloader>
                                                            (source->GetSourceUri()));
        }
    }
    if (mimeType_== AVMimeTypes::APPLICATION_M3U8) {
        downloader_ = std::make_shared<DownloadMonitor>(std::make_shared<HlsMediaDownloader>(mimeType_));
    }
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded_);
    }
}

bool HttpSourcePlugin::IsSeekToTimeSupported()
{
    if (mimeType_ != AVMimeTypes::APPLICATION_M3U8) {
        return uri_.find("m3u8") != std::string::npos || uri_.find(".mpd") != std::string::npos;
    }
    MEDIA_LOG_I("IsSeekToTimeSupported return true");
    return true;
}

Status HttpSourcePlugin::Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
{
    return Read(0, buffer, offset, expectedLen);
}

Status HttpSourcePlugin::Read(int32_t streamId, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
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

    if (bufData == nullptr) {
        return Status::ERROR_AGAIN;
    }

    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = streamId;
    readDataInfo.nextStreamId_ = streamId;
    readDataInfo.wantReadLength_ = expectedLen;

    auto result = downloader_->Read(bufData->GetWritableAddr(expectedLen), readDataInfo);
    buffer->streamID = readDataInfo.nextStreamId_;

    bufData->UpdateDataSize(readDataInfo.realReadLength_);
    MEDIA_LOG_D("Read finished, read size = "
    PUBLIC_LOG_ZU
    ", nextStreamId = "
    PUBLIC_LOG_D32
    ", isDownloadDone "
    PUBLIC_LOG_D32, bufData->GetSize(), readDataInfo.nextStreamId_, readDataInfo.isEos_);
    return result;
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

void HttpSourcePlugin::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("SetInterruptState %{public}d.", isInterruptNeeded);
    isInterruptNeeded_ = isInterruptNeeded;
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
    }
}

Status HttpSourcePlugin::SeekTo(uint64_t offset)
{
    AutoLock lock(mutex_);
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    MediaAVCodec::AVCodecTrace trace("HttpSourcePlugin::SeekTo");
    MEDIA_LOG_I("SeekTo enter, offset = " PUBLIC_LOG_U64, offset);
    MEDIA_LOG_I("SeekTo enter, content length = " PUBLIC_LOG_ZU, downloader_->GetContentLength());
    FALSE_RETURN_V(downloader_->GetSeekable() == Seekable::SEEKABLE, Status::ERROR_INVALID_OPERATION);
    if (offset > downloader_->GetContentLength()) {
        MEDIA_LOG_I("SeekTo enter fail, offset = " PUBLIC_LOG_U64, offset);
        MEDIA_LOG_I("SeekTo enter fail, content = " PUBLIC_LOG_ZU, downloader_->GetContentLength());
        seekErrorCount_++;
        if (seekErrorCount_ > ERROR_COUNT) {
            callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "seek error"});
        }
        FALSE_RETURN_V(offset <= downloader_->GetContentLength(), Status::ERROR_INVALID_PARAMETER);
    }
    FALSE_RETURN_V(downloader_->SeekToPos(offset), Status::ERROR_UNKNOWN);
    return Status::OK;
}

Status HttpSourcePlugin::SeekToTime(int64_t seekTime, SeekMode mode)
{
    // Not use mutex to avoid deadlock in continuously multi times in seeking
    std::shared_ptr<MediaDownloader> downloader = downloader_;
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    FALSE_RETURN_V(downloader->GetSeekable() == Seekable::SEEKABLE, Status::ERROR_INVALID_OPERATION);
    FALSE_RETURN_V(seekTime <= downloader->GetDuration(), Status::ERROR_INVALID_PARAMETER);
    FALSE_RETURN_V(downloader->SeekToTime(seekTime, mode), Status::ERROR_UNKNOWN);
    return Status::OK;
}


void HttpSourcePlugin::CloseUri(bool isAsync)
{
    // As Read function require lock firstly, if the Read function is block, we can not get the lock
    std::shared_ptr<MediaDownloader> downloader = downloader_;
    if (downloader != nullptr) {
        MEDIA_LOG_D("Close uri");
        downloader->Close(true);
    }
    if (!isAsync) {
        AutoLock lock(mutex_);
        downloader_ = nullptr;
    }
    uri_.clear();
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
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    downloader_->SetIsTriggerAutoMode(false);
    if (downloader_->SelectBitRate(bitRate)) {
        return Status::OK;
    }
    return Status::ERROR_UNKNOWN;
}

Status HttpSourcePlugin::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    MEDIA_LOG_I("HttpSourcePlugin::GetDownloadInfo");
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    downloader_->GetDownloadInfo(downloadInfo);
    return Status::OK;
}

Status HttpSourcePlugin::GetPlaybackInfo(PlaybackInfo& playbackInfo)
{
    MEDIA_LOG_I("HttpSourcePlugin::GetPlaybackInfo");
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    downloader_->GetPlaybackInfo(playbackInfo);
    return Status::OK;
}

void HttpSourcePlugin::SetDemuxerState(int32_t streamId)
{
    if (downloader_ != nullptr) {
        downloader_->SetDemuxerState(streamId);
    }
}

void HttpSourcePlugin::SetDownloadErrorState()
{
}

Status HttpSourcePlugin::SetCurrentBitRate(int32_t bitRate, int32_t streamID)
{
    MEDIA_LOG_I("SetCurrentBitRate");
    if (downloader_ == nullptr) {
        MEDIA_LOG_E("SetCurrentBitRate failed, downloader_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return downloader_->SetCurrentBitRate(bitRate, streamID);
}
}
}
}
}