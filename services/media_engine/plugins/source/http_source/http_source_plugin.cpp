/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#include "meta/meta_key.h"
#include <array>

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
constexpr auto UNALLOWED_PROTOCOLS = std::array{
    std::string_view{"file://"},
};
constexpr std::string_view HTTP_PREFIX = "http";

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
    FALSE_RETURN_MSG(reportInfo_ != nullptr, "reportInfo_ is nullptr");
    OHOS::MediaAVCodec::SourceStatisticsReportInfo reportInfoCopy;
    std::thread([reportInfoCopy = std::move(this->reportInfo_)]() {
        OHOS::MediaAVCodec::SourceStatisticsEventWrite(*reportInfoCopy);
    }).detach();
}

std::string HttpSourcePlugin::GetContentType()
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, "");
    return downloader->GetContentType();
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
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_UNKNOWN);
    downloader->SetReadBlockingFlag(isReadBlockingAllowed);
    return Status::OK;
}

Status HttpSourcePlugin::GetStreamInfo(std::vector<StreamInfo>& streams, bool isUpdate)
{
    MEDIA_LOG_D("GetStreamInfo entered");
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::OK);
    return downloader->GetStreamInfo(streams, isUpdate);
}

Status HttpSourcePlugin::SelectStream(int32_t streamID)
{
    MEDIA_LOG_D("SelectStream entered");
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::OK);
    downloader->SelectStream(streamID);
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
    auto downloader = GetDownloader();
    if (downloader != nullptr && CheckIsM3U8Uri()) {
        downloader->Pause();
    }
    return Status::OK;
}

Status HttpSourcePlugin::Resume()
{
    MEDIA_LOG_I("Resume enter.");
    auto downloader = GetDownloader();
    if (downloader != nullptr && CheckIsM3U8Uri()) {
        downloader->Resume();
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
    FALSE_RETURN_V(meta != nullptr, Status::ERROR_NULL_POINTER);
    meta->GetData(Tag::BUFFERING_SIZE, bufferSize_);
    meta->GetData(Tag::WATERLINE_HIGH, waterline_);
    return Status::OK;
}

Status HttpSourcePlugin::SetMediaDuration(int64_t mediaDuration)
{
    MEDIA_LOG_I("SetMediaDuration enter.");
    auto downloader = GetDownloader();
    if (downloader != nullptr && mediaDuration > 0) {
        downloader->SetMediaDuration(mediaDuration);
    }
    return Status::OK;
}

Status HttpSourcePlugin::SetCallback(const std::shared_ptr<Callback>& cb)
{
    MEDIA_LOG_D("SetCallback enter.");
    callback_ = cb;
    auto downloader = GetDownloader();
    if (downloader != nullptr) {
        downloader->SetCallback(cb);
    }
    return Status::OK;
}

Status HttpSourcePlugin::SetSource(std::shared_ptr<MediaSource> source)
{
    MediaAVCodec::AVCodecTrace trace("HttpSourcePlugin::SetSource");
    MEDIA_LOG_D("SetSource enter.");
    FALSE_RETURN_V(GetDownloader() == nullptr, Status::ERROR_INVALID_OPERATION); // not allowed set again
    FALSE_RETURN_V(source != nullptr, Status::ERROR_INVALID_OPERATION);
    FALSE_RETURN_V(IsAllowedProtocol(source->GetSourceUri()), Status::ERROR_INVALID_OPERATION);
    InitSourcePlugin(source);
    redirectUrl_ = GetCurUrl();
    FALSE_RETURN_V(!redirectUrl_.empty() && source->GetSourceUri() != redirectUrl_, Status::OK);
    FALSE_RETURN_V(IsAllowedProtocol(redirectUrl_), Status::ERROR_INVALID_OPERATION);
    uri_ = redirectUrl_;
    FALSE_RETURN_V(IsSeekToTimeSupported(), Status::OK);
    InitSourcePlugin(source);
    return Status::OK;
}

Status HttpSourcePlugin::InitSourcePlugin(const std::shared_ptr<MediaSource>& source)
{
    SetDownloaderBySource(source);
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    auto cb = callback_.lock();
    if (cb) {
        downloader->SetCallback(cb);
    }
    FALSE_RETURN_V(downloader->Open(uri_, httpHeader_), Status::ERROR_UNKNOWN);
    auto uuid = source->GetAppUid();
    std::string bundleName = OHOS::Media::HttpMediaUtils::GetClientBundleName(uuid);
    if (reportInfo_ != nullptr) {
        reportInfo_->appName_ = bundleName;
        reportInfo_->sourceUri_ = uri_;
    }
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

static void UserDefinedDuration(std::shared_ptr<PlayStrategy> strategy, bool& uDefinedDuration, uint32_t& expectDur)
{
    if (strategy != nullptr && strategy->duration > 0) {
        expectDur = strategy->duration;
        uDefinedDuration = true;  // 以用户配置为准，不许调节
    }
}

std::shared_ptr<MediaDownloader> HttpSourcePlugin::CreateDashDownloader()
{
    auto downloader = std::make_shared<DownloadMonitor>(std::make_shared<DashMediaDownloader>(loaderCombinations_));
    FALSE_RETURN_V_MSG(downloader != nullptr, nullptr, "CreateDashDownloader downloader is nullptr");
    downloader->Init();
    downloader->SetSourceStatisticsDfx(reportInfo_);
    return downloader;
}

std::shared_ptr<MediaDownloader> HttpSourcePlugin::CreateHlsDownloader()
{
    bool userDefinedDuration = false;
    uint32_t expectDuration = DEFAULT_EXPECT_DURATION;
    UserDefinedDuration(playStrategy_, userDefinedDuration, expectDuration);
    auto downloader = std::make_shared<DownloadMonitor>(std::make_shared<HlsMediaDownloader>
        (expectDuration, userDefinedDuration, httpHeader_, loaderCombinations_));
    FALSE_RETURN_V_MSG(downloader != nullptr, nullptr, "CreateHlsDownloader downloader is nullptr");
    downloader->Init();
    downloader->SetSourceStatisticsDfx(reportInfo_);
    return downloader;
}

void HttpSourcePlugin::InitSourceLoader(std::shared_ptr<MediaSource> source)
{
    if (source->GetSourceLoader() == nullptr) {
        return;
    }
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

void HttpSourcePlugin::SetDownloaderBySource(std::shared_ptr<MediaSource> source)
{
    FALSE_RETURN_MSG(source != nullptr, "source is null.");
    playStrategy_ = PlayStrategyInit(source);
    InitSourceLoader(source);
    if (IsDash()) {
        auto downloader = CreateDashDownloader();
        SetDownloader(downloader);
        delayReady_ = false;
    } else if (IsSeekToTimeSupported() && mimeType_ != AVMimeTypes::APPLICATION_M3U8) {
        auto downloader = CreateHlsDownloader();
        SetDownloader(downloader);
        delayReady_ = false;
    } else if (uri_.compare(0, HTTP_PREFIX.length(), HTTP_PREFIX) == 0) {
        InitHttpSource(source);
    }
    auto downloader = GetDownloader();
    if (downloader != nullptr && playStrategy_ != nullptr) {
        downloader->SetPlayStrategy(playStrategy_);
    }
    if (mimeType_ == AVMimeTypes::APPLICATION_M3U8) {
        auto hlsDownloader = std::make_shared<DownloadMonitor>(std::make_shared<HlsMediaDownloader>(mimeType_));
        hlsDownloader->Init();
        SetDownloader(hlsDownloader);
    }
    MediaStreamDfxTrace(source);
    downloader = GetDownloader();
    if (downloader != nullptr) {
        downloader->SetInterruptState(isInterruptNeeded_);
        downloader->SetAppUid(source->GetAppUid());
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
                        static_cast<uint64_t>(streamA->width) * streamA->height <
                        static_cast<uint64_t>(streamB->width) * streamB->height);
        });
        uri_ = playMediaStreams.front()->url;
    }
    uint32_t expectDuration = DEFAULT_EXPECT_DURATION;
    if (playStrategy != nullptr && playStrategy->duration > 0) {
        expectDuration = playStrategy->duration;
    }
    auto downloader = std::make_shared<DownloadMonitor>(std::make_shared<HttpMediaDownloader>
        (uri_, expectDuration, loaderCombinations_));
    downloader->Init();
    downloader->SetSourceStatisticsDfx(reportInfo_);
    auto cb = callback_.lock();
    if (cb) {
        downloader->SetCallback(cb);
    }
    SetDownloader(downloader);
    downloader->SetMediaStreams(playMediaStreams);
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
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);

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

    auto result = downloader->Read(writableAddr, readDataInfo);
    buffer->streamID = readDataInfo.nextStreamId_;

    bufData->UpdateDataSize(readDataInfo.realReadLength_);
    MEDIA_LOG_D("Read finished, read size = " PUBLIC_LOG_ZU ", nextStreamId = " PUBLIC_LOG_D32 ", isDownloadDone "
        PUBLIC_LOG_D32, bufData->GetSize(), readDataInfo.nextStreamId_, readDataInfo.isEos_);
    return result;
}

Status HttpSourcePlugin::GetSize(uint64_t& size)
{
    MEDIA_LOG_D("GetSize enter.");
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    size = static_cast<uint64_t>(downloader->GetContentLength());
    return Status::OK;
}

Seekable HttpSourcePlugin::GetSeekable()
{
    MEDIA_LOG_D("GetSeekable enter.");
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Seekable::INVALID);
    return downloader->GetSeekable();
}

void HttpSourcePlugin::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("SetInterruptState %{public}d.", isInterruptNeeded);
    isInterruptNeeded_ = isInterruptNeeded;
    auto downloader = GetDownloader();
    if (downloader != nullptr) {
        downloader->SetInterruptState(isInterruptNeeded);
    }
}

Status HttpSourcePlugin::SeekTo(uint64_t offset)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    MediaAVCodec::AVCodecTrace trace("HttpSourcePlugin::SeekTo");
    MEDIA_LOG_I("SeekTo enter, offset = " PUBLIC_LOG_U64, offset);
    MEDIA_LOG_I("SeekTo enter, content length = " PUBLIC_LOG_ZU, downloader->GetContentLength());
    FALSE_RETURN_V(downloader->GetSeekable() == Seekable::SEEKABLE, Status::ERROR_INVALID_OPERATION);
    if (offset > downloader->GetContentLength() && downloader->GetContentLength() != 0) {
        MEDIA_LOG_I("SeekTo enter fail, offset = " PUBLIC_LOG_U64, offset);
        MEDIA_LOG_I("SeekTo enter fail, content = " PUBLIC_LOG_ZU, downloader->GetContentLength());
        seekErrorCount_++;
        if (seekErrorCount_ > ERROR_COUNT) {
            auto cb = callback_.lock();
            if (cb != nullptr) {
                cb->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT},
                    "errorMessage: seek time exceeds the maximum duration of the resource"});
            }
        }
        FALSE_RETURN_V(offset <= downloader->GetContentLength(), Status::ERROR_INVALID_PARAMETER);
    }
    bool isSeekHit = false;
    FALSE_RETURN_V(downloader->SeekToPos(offset, isSeekHit), Status::ERROR_UNKNOWN);
    return Status::OK;
}

Status HttpSourcePlugin::SeekToTime(int64_t seekTime, SeekMode mode)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    FALSE_RETURN_V(downloader->GetSeekable() == Seekable::SEEKABLE, Status::ERROR_INVALID_OPERATION);
    auto duration = downloader->GetDuration();
    if (duration < -1) {
        duration *= -1;
    }
    FALSE_RETURN_V(seekTime <= duration, Status::ERROR_INVALID_PARAMETER);
    FALSE_RETURN_V(downloader->SeekToTime(seekTime, mode), Status::ERROR_UNKNOWN);
    return Status::OK;
}

Status HttpSourcePlugin::MediaSeekTimeByStreamId(int64_t seekTime, SeekMode mode, int32_t streamId)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    FALSE_RETURN_V(downloader->GetSeekable() == Seekable::SEEKABLE, Status::ERROR_INVALID_OPERATION);
    auto duration = downloader->GetDuration();
    if (duration < -1) {
        duration *= -1;
    }
    FALSE_RETURN_V(seekTime <= duration, Status::ERROR_INVALID_PARAMETER);
    FALSE_RETURN_V(downloader->MediaSeekTimeByStreamId(seekTime, mode, streamId), Status::ERROR_UNKNOWN);
    return Status::OK;
}

void HttpSourcePlugin::CloseUri(bool isAsync)
{
    auto downloader = GetDownloader();
    if (downloader != nullptr) {
        MEDIA_LOG_D("Close uri");
        downloader->Close(true);
    }
    if (!isAsync) {
        SetDownloader(nullptr);
    }
    uri_.clear();
}

Status HttpSourcePlugin::GetDuration(int64_t& duration)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    duration = downloader->GetDuration();
    return Status::OK;
}

Status HttpSourcePlugin::GetStartInfo(std::pair<int64_t, bool>& startInfo)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    startInfo = downloader->GetStartInfo();
    return Status::OK;
}

Status HttpSourcePlugin::GetBitRates(std::vector<uint32_t>& bitRates)
{
    MEDIA_LOG_I("HttpSourcePlugin::GetBitRates() enter.\n");
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    bitRates = downloader->GetBitRates();
    return Status::OK;
}

Status HttpSourcePlugin::SelectBitRate(uint32_t bitRate)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    downloader->SetIsTriggerAutoMode(false);
    if (downloader->SelectBitRate(bitRate)) {
        return Status::OK;
    }
    return Status::ERROR_UNKNOWN;
}

Status HttpSourcePlugin::AutoSelectBitRate(uint32_t bitRate)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    if (downloader->AutoSelectBitRate(bitRate)) {
        return Status::OK;
    }
    return Status::ERROR_UNKNOWN;
}

Status HttpSourcePlugin::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    MEDIA_LOG_I("HttpSourcePlugin::GetDownloadInfo");
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    downloader->GetDownloadInfo(downloadInfo);
    return Status::OK;
}

Status HttpSourcePlugin::GetPlaybackInfo(PlaybackInfo& playbackInfo)
{
    MEDIA_LOG_I("HttpSourcePlugin::GetPlaybackInfo");
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    downloader->GetPlaybackInfo(playbackInfo);
    return Status::OK;
}

std::vector<SeekRange> HttpSourcePlugin::GetSeekableRanges() const
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, {});
    return downloader->GetSeekableRanges();
}

void HttpSourcePlugin::SetDemuxerState(int32_t streamId)
{
    auto downloader = GetDownloader();
    if (downloader != nullptr) {
        downloader->SetDemuxerState(streamId);
    }
}

void HttpSourcePlugin::SetDownloadErrorState()
{
}

Status HttpSourcePlugin::SetCurrentBitRate(int32_t bitRate, int32_t streamID)
{
    MEDIA_LOG_I("SetCurrentBitRate");
    auto downloader = GetDownloader();
    if (downloader == nullptr) {
        MEDIA_LOG_E("SetCurrentBitRate failed, downloader is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    if (reportInfo_ != nullptr) {
        reportInfo_->bitRate_ = bitRate;
    }
    return downloader->SetCurrentBitRate(bitRate, streamID);
}

size_t HttpSourcePlugin::GetSegmentOffset()
{
    auto downloader = GetDownloader();
    if (downloader) {
        return downloader->GetSegmentOffset();
    }
    return 0;
}

bool HttpSourcePlugin::GetHLSDiscontinuity()
{
    auto downloader = GetDownloader();
    if (downloader) {
        return downloader->GetHLSDiscontinuity();
    }
    return false;
}

bool HttpSourcePlugin::SetSourceInitialBufferSize(int32_t offset, int32_t size)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V_MSG_W(downloader != nullptr, false, "SetInitialBufferSize downloader is nullptr");
    return downloader->SetInitialBufferSize(offset, size);
}

Status HttpSourcePlugin::StopBufferring(bool isAppBackground)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V_MSG_E(downloader != nullptr, Status::ERROR_NULL_POINTER, "downloader is nullptr");
    return downloader->StopBufferring(isAppBackground);
}

void HttpSourcePlugin::WaitForBufferingEnd()
{
    auto downloader = GetDownloader();
    FALSE_RETURN_MSG(downloader != nullptr, "WaitForBufferingEnd downloader is nullptr");
    downloader->WaitForBufferingEnd();
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
    auto downloader = GetDownloader();
    FALSE_RETURN_MSG(downloader != nullptr, "NotifyInitSuccess downloader is nullptr");
    downloader->NotifyInitSuccess();
}

Status HttpSourcePlugin::SetStartPts(int64_t startPts)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    downloader->SetStartPts(startPts);
    return Status::OK;
}

Status HttpSourcePlugin::SetExtraCache(uint64_t cacheDuration)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V(downloader != nullptr, Status::ERROR_NULL_POINTER);
    downloader->SetExtraCache(cacheDuration);
    return Status::OK;
}

uint64_t HttpSourcePlugin::GetCachedDuration()
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V_MSG_E(downloader != nullptr, 0, "downloader is nullptr");
    return downloader->GetCachedDuration();
}

void HttpSourcePlugin::RestartAndClearBuffer()
{
    auto downloader = GetDownloader();
    FALSE_RETURN_MSG(downloader != nullptr, "downloader is nullptr");
    return downloader->RestartAndClearBuffer();
}

bool HttpSourcePlugin::IsFlvLive()
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V_MSG_E(downloader != nullptr, false, "downloader is nullptr");
    return downloader->IsFlvLive();
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
    auto downloader = GetDownloader();
    FALSE_RETURN_V_MSG_E(downloader != nullptr, false, "downloader is nullptr");
    return downloader->IsHlsFmp4();
}

uint64_t HttpSourcePlugin::GetMemorySize()
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V_MSG_E(downloader != nullptr, 0, "downloader is nullptr");
    return downloader->GetMemorySize();
}

std::string HttpSourcePlugin::GetCurUrl()
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V_MSG_E(downloader != nullptr, "", "downloader is nullptr");
    return downloader->GetCurUrl();
}

bool HttpSourcePlugin::IsHlsEnd(int32_t streamId)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V_MSG_E(downloader != nullptr, false, "downloader is nullptr");
    return downloader->IsHlsEnd(streamId);
}

void HttpSourcePlugin::SetDefaultStreamId(int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_MSG(downloader != nullptr, "downloader is nullptr");
    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
}

bool HttpSourcePlugin::IsHls()
{
    if (mimeType_ != AVMimeTypes::APPLICATION_M3U8) {
        return CheckIsM3U8Uri();
    }
    MEDIA_LOG_I("IsHls return true");
    return true;
}

bool HttpSourcePlugin::IsAllowedProtocol(std::string_view url)
{
    for (auto& protocol : UNALLOWED_PROTOCOLS) {
        size_t protocolLen = protocol.size();
        if (url.size() < protocolLen) {
            continue;
        }

        bool isMatch = true;
        for (size_t i = 0; i < protocolLen; ++i) {
            char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(url[i])));
            if (ch != protocol[i]) {
                isMatch = false;
                break;
            }
        }

        if (isMatch) {
            return false;
        }
    }
    return true;
}

std::shared_ptr<MediaDownloader> HttpSourcePlugin::GetDownloader() const
{
    std::shared_lock<std::shared_mutex> lock(downloaderMutex_);
    return downloader_;
}

void HttpSourcePlugin::SetDownloader(const std::shared_ptr<MediaDownloader>& downloader)
{
    std::unique_lock<std::shared_mutex> lock(downloaderMutex_);
    downloader_ = downloader;
}
}
}
}
}