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
#include "avcodec_dfx_component.h"
#include "http_source_plugin.h"
#include "common/log.h"
#include "hls/hls_media_downloader.h"
#include "dash/dash_media_downloader.h"
#include "http/http_media_downloader.h"
#include "monitor/download_monitor.h"
#include "sniffer/sniffer_coordinator.h"
#include <climits>
#include "utils/initial_stream_selection.h"
#include "utils/string_utils.h"
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
constexpr int DEFAULT_EXPECT_DURATION = 19;
constexpr int ERROR_COUNT = 5;
constexpr std::string_view FD_URI_PREFIX = "fd://";
constexpr auto UNALLOWED_PROTOCOLS = std::array{
    std::string_view{"file://"},
};

std::string LogResource(const std::string& url)
{
    constexpr std::string_view HTTP_PREFIX = "http://";
    constexpr std::string_view HTTPS_PREFIX = "https://";
    if (url.compare(0, HTTP_PREFIX.size(), HTTP_PREFIX) != 0 &&
        url.compare(0, HTTPS_PREFIX.size(), HTTPS_PREFIX) != 0) {
        return "local_or_custom";
    }
    // Strip query/fragment even on the mask helper's pathless-URL branch.
    return StringUtil::MaskUrl(url.substr(0, url.find_first_of("?#")));
}
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
    : SourcePlugin(std::move(name)), downloader_(nullptr)
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
    if (sourceState_.load() == SourceState::PROBE_FAILED) {
        return FinishProbeFailure();
    }
    return delayReady_ ? Status::ERROR_DELAY_READY : Status::OK;
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
    if (downloader != nullptr && IsHls()) {
        downloader->Pause();
    }
    return Status::OK;
}

Status HttpSourcePlugin::Resume()
{
    MEDIA_LOG_I("Resume enter.");
    auto downloader = GetDownloader();
    if (downloader != nullptr && IsHls()) {
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

std::shared_ptr<Callback> HttpSourcePlugin::GetCallback() const
{
    std::lock_guard<std::mutex> lock(sourceStateMutex_);
    return callback_.lock();
}

std::shared_ptr<ProbeFailureNotifier> HttpSourcePlugin::GetProbeFailure() const
{
    std::lock_guard<std::mutex> lock(sourceStateMutex_);
    return probeFailure_;
}

void HttpSourcePlugin::BeginProbeRoute()
{
    std::shared_ptr<ProbeFailureNotifier> previous;
    {
        std::lock_guard<std::mutex> lock(sourceStateMutex_);
        previous.swap(probeFailure_);
        routeId_.store(MediaAVCodec::AllocateUniqueId());
        sourceState_.store(SourceState::PROBING);
        routeStatus_.store(Status::ERROR_UNKNOWN);
    }
    if (previous != nullptr) {
        previous->Cancel();
    }
}

void HttpSourcePlugin::SaveProbeFailure(const ProbeFailure& failure, const std::string& url, bool localManifest)
{
    bool network = !localManifest &&
        (failure.kind == ProbeFailureKind::NETWORK || failure.kind == ProbeFailureKind::TIMEOUT);
    auto notification = network ? std::make_shared<ProbeFailureNotifier>(failure, url,
        std::weak_ptr<Callback> {}) : nullptr;
    {
        std::lock_guard<std::mutex> lock(sourceStateMutex_);
        if (sourceClosed_.load() || isInterruptNeeded_.load() || failure.routeId != routeId_.load()) {
            return;
        }
        sourceState_.store(network ? SourceState::PROBE_FAILED : SourceState::IDLE);
        if (notification != nullptr) {
            notification->SetCallback(callback_);
            probeFailure_ = std::move(notification);
            routeStatus_.store(Status::OK);
        }
    }
    LogRoute("probe_failure_saved", network ? "callback_pending" : "synchronous_failure", "probe");
}

void HttpSourcePlugin::CancelProbeFailure()
{
    std::shared_ptr<ProbeFailureNotifier> notification;
    {
        std::lock_guard<std::mutex> lock(sourceStateMutex_);
        sourceState_.store(SourceState::CLOSED);
        notification.swap(probeFailure_);
    }
    if (notification != nullptr) {
        // No source/pointer lock is held across a callback drain; Cancel handles self-reentry.
        notification->Cancel();
    }
}

Status HttpSourcePlugin::FinishProbeFailure(const std::shared_ptr<Buffer>& buffer)
{
    if (buffer != nullptr && buffer->GetMemory() != nullptr) {
        buffer->GetMemory()->UpdateDataSize(0);
    }
    auto notification = GetProbeFailure();
    if (notification != nullptr) {
        notification->WaitForDelivery();
    }
    // ReadRetry terminates on ERROR_UNKNOWN; never report a successful EOS for failed input.
    return Status::ERROR_UNKNOWN;
}

Status HttpSourcePlugin::SetCallback(const std::shared_ptr<Callback>& cb)
{
    MEDIA_LOG_D("SetCallback enter.");
    std::shared_ptr<ProbeFailureNotifier> failure;
    {
        std::lock_guard<std::mutex> lock(sourceStateMutex_);
        callback_ = cb;
        failure = probeFailure_;
        if (failure != nullptr) {
            failure->SetCallback(cb);
        }
    }
    auto downloader = GetDownloader();
    if (downloader != nullptr) {
        downloader->SetCallback(cb);
    }
    if (failure != nullptr) {
        failure->Notify();
    }
    return Status::OK;
}

Status HttpSourcePlugin::SetSource(std::shared_ptr<MediaSource> source)
{
    MediaAVCodec::AVCodecTrace trace("HttpSourcePlugin::SetSource");
    MEDIA_LOG_D("SetSource enter.");
    FALSE_RETURN_V(GetDownloader() == nullptr, Status::ERROR_INVALID_OPERATION); // not allowed set again
    FALSE_RETURN_V(sourceState_.load() != SourceState::PROBING, Status::ERROR_INVALID_OPERATION);
    FALSE_RETURN_V(source != nullptr, Status::ERROR_INVALID_OPERATION);
    FALSE_RETURN_V(IsAllowedProtocol(source->GetSourceUri()), Status::ERROR_INVALID_OPERATION);
    sourceClosed_.store(false);
    auto status = InitSourcePlugin(source);
    FALSE_RETURN_V(!sourceClosed_.load() && !isInterruptNeeded_.load(), Status::ERROR_UNKNOWN);
    if (sourceState_.load() == SourceState::PROBE_FAILED) {
        auto failure = GetProbeFailure();
        if (failure != nullptr) {
            // Last action: an external callback may release or reset this plugin.
            failure->Notify();
        }
        return Status::OK;
    }
    // A published formal downloader retains its existing acceptance and error-reporting behavior.
    FALSE_RETURN_V(status == Status::OK || GetDownloader() != nullptr, status);
    redirectUrl_ = GetCurUrl();
    FALSE_RETURN_V(!redirectUrl_.empty() && source->GetSourceUri() != redirectUrl_, Status::OK);
    FALSE_RETURN_V(IsAllowedProtocol(redirectUrl_), Status::ERROR_INVALID_OPERATION);
    return Status::OK;
}

Status HttpSourcePlugin::InitSourcePlugin(const std::shared_ptr<MediaSource>& source)
{
    SetDownloaderBySource(source);
    auto downloader = GetDownloader();
    LogRoute("formal_open_check", downloader != nullptr ? "downloader_ready" : "no_downloader", "formal");
    FALSE_RETURN_V(downloader != nullptr, routeStatus_.load());
    if (isInterruptNeeded_.load() || sourceClosed_.load()) {
        LogRoute("formal_open_vetoed", "closed_or_interrupted", "formal");
        return Status::ERROR_UNKNOWN;
    }
    LogRoute("formal_open_begin", "submit", "formal");
    bool opened = downloader->Open(uri_, httpHeader_);
    LogRoute("formal_open_end", opened ? "accepted" : "rejected", "formal");
    FALSE_RETURN_V(opened, Status::ERROR_UNKNOWN);
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
    bool local = IsLocalManifest();
    std::shared_ptr<MediaDownloader> impl;
    if (local) {
        impl = std::make_shared<HlsMediaDownloader>(mimeType_, httpHeader_);
    } else {
        impl = std::make_shared<HlsMediaDownloader>(expectDuration, userDefinedDuration, httpHeader_,
            loaderCombinations_);
    }
    auto downloader = std::make_shared<DownloadMonitor>(impl);
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

void HttpSourcePlugin::SelectInitialStream(MediaStreamList& streams)
{
    streams.erase(std::remove(streams.begin(), streams.end(), nullptr), streams.end());
    if (!streams.empty()) {
        std::sort(streams.begin(), streams.end(), [](const auto& a, const auto& b) {
            return a->bitrate < b->bitrate || (a->bitrate == b->bitrate &&
                static_cast<uint64_t>(a->width) * a->height < static_cast<uint64_t>(b->width) * b->height);
        });
        uint64_t resolution = 0;
        if (playStrategy_ != nullptr && playStrategy_->width > 0 && playStrategy_->width < USHRT_MAX &&
            playStrategy_->height > 0 && playStrategy_->height < USHRT_MAX) {
            resolution = static_cast<uint64_t>(playStrategy_->width) * playStrategy_->height;
        }
        uri_ = ChooseInitialStream(streams, resolution)->url;
    }
    auto selected = std::find_if(streams.begin(), streams.end(), [this](const auto& stream) {
        return stream != nullptr && stream->url == uri_;
    });
    auto resource = LogResource(uri_);
    MEDIA_LOG_I("route=%{public}" PRIu64 " event=resource_selected candidates=%{public}zu local=%{public}d "
        "resource=%{public}s width=%{public}u height=%{public}u bitrate=%{public}u",
        routeId_.load(), streams.size(), IsLocalManifest(), resource.c_str(),
        selected != streams.end() ? (*selected)->width : 0, selected != streams.end() ? (*selected)->height : 0,
        selected != streams.end() ? (*selected)->bitrate : 0);
}

bool HttpSourcePlugin::IsLocalManifest() const
{
    return mimeType_ == AVMimeTypes::APPLICATION_M3U8 && uri_.compare(0, FD_URI_PREFIX.size(), FD_URI_PREFIX) == 0;
}

std::shared_ptr<SniffResult> HttpSourcePlugin::ProbeSource(int32_t appUid)
{
    const auto probeUrl = uri_;
    const bool localManifest = IsLocalManifest();
    auto snifferLoader = loaderCombinations_ != nullptr && loaderCombinations_->loader_ != nullptr ?
        loaderCombinations_ : nullptr;
    auto sniffer = std::make_shared<SnifferCoordinator>(snifferLoader,
        SnifferCoordinator::DownloaderFactory {}, routeId_.load());
    if (!SetActiveSniffer(sniffer)) {
        LogRoute("probe_rejected", "closed_or_interrupted", "probe");
        return nullptr;
    }
    LogRoute("probe_begin", localManifest ? "local_fd" : "network", "probe");
    sniffer->Start(probeUrl, httpHeader_, localManifest, appUid);
    std::shared_ptr<SniffResult> result;
    bool ok = sniffer->WaitResult(result);
    auto failure = sniffer->GetFailure();
    LogRoute("probe_result", ok ? "classified" : "not_accepted", "probe");
    SetActiveSniffer(nullptr);
    // WaitResult quiesced the transport; release its prefix storage before creating a media parser.
    sniffer.reset();
    if (!ok) {
        SaveProbeFailure(failure, probeUrl, localManifest);
    }
    if (!ok || isInterruptNeeded_.load() || sourceClosed_.load()) {
        LogRoute("probe_vetoed", !ok ? "probe_unsuccessful" : "closed_or_interrupted", "probe");
        return nullptr;
    }
    return result;
}

std::shared_ptr<MediaDownloader> HttpSourcePlugin::CreateDownloader(
    const std::shared_ptr<MediaSource>& source, const SniffResult& result)
{
    std::shared_ptr<MediaDownloader> downloader;
    switch (result.type) {
        case StreamProtocolType::HLS:
            downloader = CreateHlsDownloader();
            break;
        case StreamProtocolType::DASH:
            downloader = CreateDashDownloader();
            break;
        default:
            downloader = CreateHttpDownloader(source, result.isFlv);
            break;
    }
    return downloader;
}

void HttpSourcePlugin::SetDownloaderBySource(std::shared_ptr<MediaSource> source)
{
    BeginProbeRoute();
    LogRoute("route_begin", source != nullptr ? "source_present" : "null_source");
    FALSE_RETURN_MSG(source != nullptr, "source is null.");
    playStrategy_ = PlayStrategyInit(source);
    InitSourceLoader(source);
    auto streams = source->GetMediaStreamList();
    SelectInitialStream(streams);
    if (!IsAllowedProtocol(uri_)) {
        routeStatus_.store(Status::ERROR_INVALID_OPERATION);
        sourceState_.store(SourceState::IDLE);
        LogRoute("route_rejected", "protocol_not_allowed");
        return;
    }
    auto result = ProbeSource(source->GetAppUid());
    FALSE_RETURN_MSG(result != nullptr, "probe failed or cancelled");
    auto downloader = CreateDownloader(source, *result);
    MEDIA_LOG_I("route=%{public}" PRIu64 " event=downloader_created protocol=%{public}d flv=%{public}d "
        "created=%{public}d media=%{public}" PRIu64, routeId_.load(), static_cast<int32_t>(result->type), result->isFlv,
        downloader != nullptr, downloader != nullptr ? downloader->GetLogId() : 0);
    FALSE_RETURN_MSG(downloader != nullptr, "create downloader failed");
    protocolType_.store(result->type);
    if (!SetDownloader(downloader)) {
        protocolType_.store(StreamProtocolType::HTTP);
        return;
    }
    ConfigureDownloader(downloader, source, *result, streams);
    MediaStreamDfxTrace(source);
    LogRoute("route_configured");
}

void HttpSourcePlugin::ConfigureDownloader(const std::shared_ptr<MediaDownloader>& downloader,
    const std::shared_ptr<MediaSource>& source, const SniffResult& result, const MediaStreamList& streams)
{
    auto callback = GetCallback();
    if (callback != nullptr) {
        downloader->SetCallback(callback);
    }
    if (playStrategy_ != nullptr) {
        downloader->SetPlayStrategy(playStrategy_);
    }
    downloader->SetAppUid(source->GetAppUid());
    downloader->SetInterruptState(isInterruptNeeded_.load());
    if (result.type == StreamProtocolType::HTTP) {
        downloader->SetMediaStreams(streams);
    }
    delayReady_ = false;
    std::lock_guard<std::mutex> lock(sourceStateMutex_);
    if (!sourceClosed_.load() && !isInterruptNeeded_.load()) {
        sourceState_.store(SourceState::READY);
    }
}

void HttpSourcePlugin::LogRoute(const char* event, const char* reason, const char* phase) const
{
    MEDIA_LOG_I("route=%{public}" PRIu64 " phase=%{public}s event=%{public}s reason=%{public}s "
        "closed=%{public}d interrupted=%{public}d protocol=%{public}d", routeId_.load(), phase, event, reason,
        sourceClosed_.load(), isInterruptNeeded_.load(), static_cast<int32_t>(protocolType_.load()));
}

std::shared_ptr<MediaDownloader> HttpSourcePlugin::CreateHttpDownloader(
    const std::shared_ptr<MediaSource>& source, bool isFlv)
{
    auto playStrategy = source->GetPlayStrategy();
    uint32_t expectDuration = DEFAULT_EXPECT_DURATION;
    if (playStrategy != nullptr && playStrategy->duration > 0) {
        expectDuration = playStrategy->duration;
    }
    auto downloader = std::make_shared<DownloadMonitor>(std::make_shared<HttpMediaDownloader>
        (uri_, expectDuration, loaderCombinations_, isFlv));
    FALSE_RETURN_V_MSG(downloader != nullptr, nullptr, "CreateHttpDownloader downloader is nullptr");
    downloader->Init();
    downloader->SetSourceStatisticsDfx(reportInfo_);
    return downloader;
}

bool HttpSourcePlugin::IsSeekToTimeSupported()
{
    auto type = protocolType_.load();
    return type == StreamProtocolType::HLS || type == StreamProtocolType::DASH;
}

Status HttpSourcePlugin::Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
{
    return Read(0, buffer, offset, expectedLen);
}

Status HttpSourcePlugin::Read(int32_t streamId, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
{
    if (sourceState_.load() == SourceState::PROBE_FAILED) {
        return FinishProbeFailure(buffer);
    }
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
    if (sourceState_.load() == SourceState::PROBE_FAILED) {
        size = 0; // Accepted network source with unknown length; no playable content is implied.
        return Status::OK;
    }
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
    LogRoute(isInterruptNeeded ? "interrupt_requested" : "interrupt_cleared");
    if (isInterruptNeeded) {
        CancelProbeFailure();
        auto sniffer = GetActiveSniffer();
        if (sniffer != nullptr) {
            sniffer->Stop();
        }
    }
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
            auto cb = GetCallback();
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
    sourceClosed_.store(true);
    CancelProbeFailure();
    LogRoute("close_requested", isAsync ? "async" : "sync");
    {
        auto sniffer = GetActiveSniffer();
        if (sniffer != nullptr) {
            sniffer->Stop();
        }
    }
    auto downloader = GetDownloader();
    if (downloader != nullptr) {
        MEDIA_LOG_D("Close uri");
        downloader->Close(true);
    }
    if (!isAsync) {
        SetDownloader(nullptr);
    }
    uri_.clear();
    protocolType_.store(StreamProtocolType::HTTP);
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

size_t HttpSourcePlugin::GetSegmentOffset(StreamType streamType)
{
    auto downloader = GetDownloader();
    if (downloader) {
        return downloader->GetSegmentOffset(streamType);
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
    return protocolType_.load() == StreamProtocolType::HLS;
}

std::string HttpSourcePlugin::GetMaskUrlByReadIndex(int32_t streamId)
{
    auto downloader = GetDownloader();
    FALSE_RETURN_V_MSG_E(downloader != nullptr, "", "GetMaskUrlByReadIndex downloader is nullptr");
    return downloader->GetMaskUrlByReadIndex(streamId);
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

bool HttpSourcePlugin::SetDownloader(std::shared_ptr<MediaDownloader> downloader)
{
    bool accepted = true;
    bool clearing = downloader == nullptr;
    {
        std::unique_lock<std::shared_mutex> lock(downloaderMutex_);
        if (downloader != nullptr && (isInterruptNeeded_.load() || sourceClosed_.load())) {
            accepted = false;
        } else {
            downloader_.swap(downloader);
        }
    }
    LogRoute(accepted ? (clearing ? "downloader_cleared" : "downloader_published") : "publication_rejected",
        accepted ? "accepted" : "closed_or_interrupted", "formal");
    return accepted;
}

std::shared_ptr<SnifferCoordinator> HttpSourcePlugin::GetActiveSniffer() const
{
    std::shared_lock<std::shared_mutex> lock(activeSnifferMutex_);
    return activeSniffer_;
}

bool HttpSourcePlugin::SetActiveSniffer(std::shared_ptr<SnifferCoordinator> sniffer)
{
    {
        std::unique_lock<std::shared_mutex> lock(activeSnifferMutex_);
        if (sniffer != nullptr && (isInterruptNeeded_.load() || sourceClosed_.load())) {
            return false;
        }
        activeSniffer_.swap(sniffer);
    }
    return true;
}
}
}
}
}
