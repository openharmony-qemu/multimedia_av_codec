/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#include "common/log.h"
#include "hls/hls_media_downloader.h"
#include "dash/dash_media_downloader.h"
#include "http/http_media_downloader.h"
#include "monitor/download_monitor.h"
#include "avcodec_sysevent.h"
#include "http_media_utils.h"
#include "storage_usage_util.h"
namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr int DEFAULT_BUFFER_SIZE = 200 * 1024;
constexpr int DEFAULT_EXPECT_DURATION = 19;
constexpr int ERROR_COUNT = 5;
const std::string LOWER_M3U8 = "m3u8";
const std::string DASH_SUFFIX = ".mpd";
const std::string EQUAL_M3U8 = "=" + LOWER_M3U8;
const std::string DASH_LIST[] = {
    std::string(".mpd"),
    std::string("type=mpd"),
};

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

std::string HttpSourcePlugin::GetContentType()
{
    FALSE_RETURN_V(downloader_ != nullptr, "");
    return downloader_->GetContentType();
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
    if (delayReady_) {
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
    if (downloader_ != nullptr && CheckIsM3U8Uri()) {
        downloader_->Pause();
    }
    return Status::OK;
}

Status HttpSourcePlugin::Resume()
{
    MEDIA_LOG_I("Resume enter.");
    if (downloader_ != nullptr && CheckIsM3U8Uri()) {
        downloader_->Resume();
    }
    return Status::OK;
}

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
    InitSourcePlugin(source);
    redirectUrl_ = GetCurUrl();
    FALSE_RETURN_V(!redirectUrl_.empty() && source->GetSourceUri() != redirectUrl_, Status::OK);
    uri_ = redirectUrl_;
    FALSE_RETURN_V(IsSeekToTimeSupported(), Status::OK);
    InitSourcePlugin(source);
    return Status::OK;
}

Status HttpSourcePlugin::InitSourcePlugin(const std::shared_ptr<MediaSource>& source)
{
    SetDownloaderBySource(source);
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    if (callback_ != nullptr) {
        downloader_->SetCallback(callback_);
    }
    FALSE_RETURN_V(downloader_->Open(uri_, httpHeader_), Status::ERROR_UNKNOWN);
    return Status::OK;
}

std::shared_ptr<PlayStrategy> HttpSourcePlugin::PlayStrategyInit(std::shared_ptr<MediaSource> source)
{
    uri_ = redirectUrl_.empty() ? source->GetSourceUri() : redirectUrl_;
    httpHeader_ = source->GetSourceHeader();
    std::shared_ptr<PlayStrategy> playStrategy = source->GetPlayStrategy();
    mimeType_ = source->GetMimeType();
    return playStrategy;
}

void HttpSourcePlugin::MediaStreamDfxTrace(std::shared_ptr<MediaSource> source)
{
    auto uuid = source->GetAppUid();
    std::string bundleName = OHOS::Media::HttpMediaUtils::GetClientBundleName(uuid);
    MediaAVCodec::StreamAppPackageNameEventWrite("AVSource", bundleName,
        "OH_AVSource_CreateWithURI", "{\"result\": \"success\"}");
}

void HttpSourcePlugin::SetDownloaderBySource(std::shared_ptr<MediaSource> source)
{
    FALSE_RETURN_MSG(source != nullptr, "source is null.");
    std::shared_ptr<PlayStrategy> playStrategy;
    if (source != nullptr) {
        playStrategy = PlayStrategyInit(source);
    }
    if (source->GetSourceLoader() != nullptr) {
        loaderCombinations_ = std::make_shared<MediaSourceLoaderCombinations>(source->GetSourceLoader());
        loaderCombinations_->EnableOfflineCache(source->GetenableOfflineCache());
        if (httpHeader_.find("Cookie") != httpHeader_.end() && loaderCombinations_->GetenableOfflineCache()) {
            loaderCombinations_->Close(-1);
        } else {
            std::shared_ptr<StorageUsageUtil> storageUsage = std::make_shared<StorageUsageUtil>();
            if (loaderCombinations_->GetenableOfflineCache() && !storageUsage->HasEnoughStorage()) {
                loaderCombinations_->Close(-1);
            }
        }
    }
    if (IsDash()) {
        downloader_ = std::make_shared<DownloadMonitor>(
                      std::make_shared<DashMediaDownloader>(loaderCombinations_));
        downloader_->Init();
        delayReady_ = false;
    } else if (IsSeekToTimeSupported() && mimeType_ != AVMimeTypes::APPLICATION_M3U8) {
        bool userDefinedDuration = false;  // 允许自动调节缓冲区大小
        uint32_t expectDuration = DEFAULT_EXPECT_DURATION;
        if (playStrategy != nullptr && playStrategy->duration > 0) {
            expectDuration = playStrategy->duration;
            userDefinedDuration = true;  // 以用户配置为准，不许调节
        }
        downloader_ = std::make_shared<DownloadMonitor>(std::make_shared<HlsMediaDownloader>
                      (expectDuration, userDefinedDuration, httpHeader_, loaderCombinations_));
        downloader_->Init();
        delayReady_ = false;
    } else if (uri_.compare(0, 4, "http") == 0) { // 0 : position, 4: count
        InitHttpSource(source);
    }
    if (downloader_ != nullptr && playStrategy != nullptr) {
        downloader_->SetPlayStrategy(playStrategy);
    }
    if (mimeType_== AVMimeTypes::APPLICATION_M3U8) {
        downloader_ = std::make_shared<DownloadMonitor>(std::make_shared<HlsMediaDownloader>(mimeType_));
        downloader_->Init();
    }
    MediaStreamDfxTrace(source);
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded_);
        downloader_->SetAppUid(source->GetAppUid());
    }
}

void HttpSourcePlugin::InitHttpSource(const std::shared_ptr<MediaSource>& source)
{
    std::shared_ptr<PlayStrategy> playStrategy = source->GetPlayStrategy();
    auto playMediaStreams = source->GetMediaStreamList();
    if (playMediaStreams.size() > 0) {
        std::sort(playMediaStreams.begin(), playMediaStreams.end(),
            [](const std::shared_ptr<PlayMediaStream>& streamA, const std::shared_ptr<PlayMediaStream>& streamB) {
                return (streamA->bitrate < streamB->bitrate) ||
                       (streamA->bitrate == streamB->bitrate &&
                        streamA->width * streamA->height < streamB->width * streamB->height);
        });
        uri_ = playMediaStreams.front()->url;
    }
    uint32_t expectDuration = DEFAULT_EXPECT_DURATION;
    if (playStrategy != nullptr && playStrategy->duration > 0) {
        expectDuration = playStrategy->duration;
    }
    downloader_ = std::make_shared<DownloadMonitor>(std::make_shared<HttpMediaDownloader>
        (uri_, expectDuration, loaderCombinations_));
    downloader_->Init();
    downloader_->SetMediaStreams(playMediaStreams);
}

bool HttpSourcePlugin::IsSeekToTimeSupported()
{
    if (mimeType_ != AVMimeTypes::APPLICATION_M3U8) {
        return CheckIsM3U8Uri() || uri_.find(DASH_SUFFIX) != std::string::npos;
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
    MediaAVCodec::AVCodecTrace trace("HttpSourcePlugin::Read, offset: "
        + std::to_string(offset) + ", expectedLen: " + std::to_string(expectedLen));
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
    auto writableAddr = bufData->GetWritableAddr(expectedLen);
    FALSE_RETURN_V(writableAddr != nullptr, Status::ERROR_AGAIN);

    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = streamId;
    readDataInfo.nextStreamId_ = streamId;
    readDataInfo.wantReadLength_ = expectedLen;
    readDataInfo.ffmpegOffset = offset;

    auto result = downloader_->Read(writableAddr, readDataInfo);
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
    if (offset > downloader_->GetContentLength() && downloader_->GetContentLength() != 0) {
        MEDIA_LOG_I("SeekTo enter fail, offset = " PUBLIC_LOG_U64, offset);
        MEDIA_LOG_I("SeekTo enter fail, content = " PUBLIC_LOG_ZU, downloader_->GetContentLength());
        seekErrorCount_++;
        if (seekErrorCount_ > ERROR_COUNT) {
            callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "seek error"});
        }
        FALSE_RETURN_V(offset <= downloader_->GetContentLength(), Status::ERROR_INVALID_PARAMETER);
    }
    bool isSeekHit = false;
    FALSE_RETURN_V(downloader_->SeekToPos(offset, isSeekHit), Status::ERROR_UNKNOWN);
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

Status HttpSourcePlugin::GetStartInfo(std::pair<int64_t, bool>& startInfo)
{
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    startInfo = downloader_->GetStartInfo();
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

Status HttpSourcePlugin::AutoSelectBitRate(uint32_t bitRate)
{
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    if (downloader_->AutoSelectBitRate(bitRate)) {
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

size_t HttpSourcePlugin::GetSegmentOffset()
{
    if (downloader_) {
        return downloader_->GetSegmentOffset();
    }
    return 0;
}

bool HttpSourcePlugin::GetHLSDiscontinuity()
{
    if (downloader_) {
        return downloader_->GetHLSDiscontinuity();
    }
    return false;
}

bool HttpSourcePlugin::SetSourceInitialBufferSize(int32_t offset, int32_t size)
{
    FALSE_RETURN_V_MSG_W(downloader_ != nullptr, false, "SetInitialBufferSize downloader is nullptr");
    return downloader_->SetInitialBufferSize(offset, size);
}

Status HttpSourcePlugin::StopBufferring(bool isAppBackground)
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, Status::ERROR_NULL_POINTER, "downloader_ is nullptr");
    return downloader_->StopBufferring(isAppBackground);
}

void HttpSourcePlugin::WaitForBufferingEnd()
{
    FALSE_RETURN_MSG(downloader_ != nullptr, "WaitForBufferingEnd downloader is nullptr");
    downloader_->WaitForBufferingEnd();
}

static std::pair<std::string, std::string> SplitUri(const std::string& uri)
{
    /* RFC3986 Url可以划分成若干个组件，协议、主机、路径等。有一些字符（:/?#[]@）是用作分隔不同组件的，作为保留字符。
        例如:冒号用于分隔协议和主机，/用于分隔主机和路径，?用于分隔路径和查询参数，百分号(%)制定特殊字符，#号指定书签，
        &号分隔参数。当组件中的普通数据包含这些特殊字符时，需要对其进行编码。*/
    std::string::size_type pos = uri.find_first_of('?');
    if (pos != std::string::npos) {
        return std::make_pair(uri.substr(0, pos), uri.substr(pos + 1));
    }

    // #作用是已给纯粹的客户端“锚点”，它不会被发送到客户端。因此#放在?之前的URL不符合规范的。
    // 例如：http://1500005450.vod2.myqcloud.com/playlist_eof.m3u8#maxBufferSize=30000
    pos = uri.find_first_of('#');
    if (pos != std::string::npos) {
        return std::make_pair(uri.substr(0, pos), "");
    }

    return std::make_pair(uri, "");
}

bool HttpSourcePlugin::CheckIsM3U8Uri()
{
    if (uri_.empty()) {
        return false;
    }
    std::string uri = uri_;
    std::transform(uri.begin(), uri.end(), uri.begin(), ::tolower);

    auto pairUri = SplitUri(uri);
    std::string::size_type pos = pairUri.first.rfind('/');
    if (pos == std::string::npos) {
        return false;
    }

    // 现网的很多hls链接是非.m3u8关键字，举例：http://xxx/xxxx?autotype=m3u8; http://xxx/aaa.doplaylist?auto=m3u8
    // 优先判断资源类型(.和?#中间的字符串，姑且称资源类型)；参数携带为次。如果不携带资源类型，还走原来的逻辑
    std::string leafNameSuffix = pairUri.first.substr(pos + 1);
    pos = leafNameSuffix.rfind('.');
    if (pos != std::string::npos) {     // 找到资源格式
        std::string suffix = leafNameSuffix.substr(pos + 1);
        if (suffix == LOWER_M3U8) {
            return true;
        }
        if (!pairUri.second.empty()) {
            if (pairUri.second.find(EQUAL_M3U8) != std::string::npos) {
                return true;
            }
        }
    } else {    // 没有标明资源格式，按原先规则里找；当前未从param后面找=m3u8
        if (uri.find(LOWER_M3U8) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

void HttpSourcePlugin::NotifyInitSuccess()
{
    FALSE_RETURN_MSG(downloader_ != nullptr, "NotifyInitSuccess downloader is nullptr");
    downloader_->NotifyInitSuccess();
}

Status HttpSourcePlugin::SetStartPts(int64_t startPts)
{
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    downloader_->SetStartPts(startPts);
    return Status::OK;
}

Status HttpSourcePlugin::SetExtraCache(uint64_t cacheDuration)
{
    FALSE_RETURN_V(downloader_ != nullptr, Status::ERROR_NULL_POINTER);
    downloader_->SetExtraCache(cacheDuration);
    return Status::OK;
}

uint64_t HttpSourcePlugin::GetCachedDuration()
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, 0, "downloader_ is nullptr");
    return downloader_->GetCachedDuration();
}

void HttpSourcePlugin::RestartAndClearBuffer()
{
    FALSE_RETURN_MSG(downloader_ != nullptr, "downloader_ is nullptr");
    return downloader_->RestartAndClearBuffer();
}

bool HttpSourcePlugin::IsFlvLive()
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, false, "downloader_ is nullptr");
    return downloader_->IsFlvLive();
}

bool HttpSourcePlugin::IsDash()
{
    auto it = std::find_if(std::begin(DASH_LIST), std::end(DASH_LIST),
        [this](const std::string& key) {
            return this->uri_.find(key) != std::string::npos;
    });
    return it != std::end(DASH_LIST);
}

bool HttpSourcePlugin::IsHlsFmp4()
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, false, "downloader_ is nullptr");
    return downloader_->IsHlsFmp4();
}

uint64_t HttpSourcePlugin::GetMemorySize()
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, 0, "downloader_ is nullptr");
    return downloader_->GetMemorySize();
}

std::string HttpSourcePlugin::GetCurUrl()
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, "", "downloader_ is nullptr");
    return downloader_->GetCurUrl();
}

bool HttpSourcePlugin::IsHlsEnd(int32_t streamId)
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, false, "downloader_ is nullptr");
    return downloader_->IsHlsEnd(streamId);
}

bool HttpSourcePlugin::IsHls()
{
    if (mimeType_ != AVMimeTypes::APPLICATION_M3U8) {
        return CheckIsM3U8Uri();
    }
    MEDIA_LOG_I("IsHls return true");
    return true;
}
}
}
}
}