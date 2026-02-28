/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "Source"
#define MEDIA_ATOMIC_ABILITY

#include "avcodec_trace.h"
#include <utility>
#include "cpp_ext/type_traits_ext.h"
#include "common/log.h"
#include "osal/utils/util.h"
#include "common/media_source.h"
#include "plugin/plugin_manager_v2.h"
#include "source.h"
#include "scoped_timer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "Source" };
constexpr int64_t SOURCE_INIT_WARNING_MS = 20;
}

namespace OHOS {
namespace Media {
using namespace Plugins;

static std::map<std::string, ProtocolType> g_protocolStringToType = {
    {"http", ProtocolType::HTTP},
    {"https", ProtocolType::HTTPS},
    {"file", ProtocolType::FILE},
    {"stream", ProtocolType::STREAM},
    {"fd", ProtocolType::FD}
};

const int32_t MAX_RETRY = 20;
const int32_t WAIT_TIME = 10;

Source::Source()
    : protocol_(),
      uri_(),
      seekable_(Seekable::INVALID),
      plugin_(nullptr),
      isPluginReady_(false),
      isAboveWaterline_(false),
      mediaDemuxerCallback_(std::make_shared<CallbackImpl>())
{
    MEDIA_LOG_D("Source called");
}

Source::~Source()
{
    MEDIA_LOG_D("~Source called");
    if (plugin_) {
        plugin_->Deinit();
    }
}

void Source::SetCallback(Callback* callback)
{
    MEDIA_LOG_D("SetCallback entered.");
    FALSE_RETURN_MSG(callback != nullptr, "callback is nullptr");
    FALSE_RETURN_MSG(mediaDemuxerCallback_ != nullptr, "mediaDemuxerCallback is nullptr");
    mediaDemuxerCallback_->SetCallbackWrap(callback);
}

void Source::ClearData()
{
    protocol_.clear();
    uri_.clear();
    seekable_ = Seekable::INVALID;
    isPluginReady_ = false;
    isAboveWaterline_ = false;
    seekToTimeFlag_ = false;
}

bool Source::IsFlvLiveStream() const
{
    return isFlvLiveStream_;
}
Status Source::SetSource(const std::shared_ptr<MediaSource>& source)
{
    MediaAVCodec::AVCodecTrace trace("Source::SetSource");
    MEDIA_LOG_D("SetSource enter.");
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER, "SetSource Invalid source");

    ClearData();
    Status ret = FindPlugin(source);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "SetSource FindPlugin failed");

    {
        ScopedTimer timer("Source InitPlugin", SOURCE_INIT_WARNING_MS);
        ret = InitPlugin(source);
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "SetSource InitPlugin failed");

    if (plugin_ != nullptr) {
        seekToTimeFlag_ = plugin_->IsSeekToTimeSupported();
    }
    MEDIA_LOG_D("SetSource seekToTimeFlag_: " PUBLIC_LOG_D32, seekToTimeFlag_);
    MEDIA_LOG_D("SetSource exit.");
    return Status::OK;
}

Status Source::SetStartPts(int64_t startPts)
{
    MEDIA_LOG_D("startPts=" PUBLIC_LOG_D64, startPts);
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("SetStartPts failed, plugin_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return plugin_->SetStartPts(startPts);
}

Status Source::SetExtraCache(uint64_t cacheDuration)
{
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("SetExtraCache failed, plugin_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return plugin_->SetExtraCache(cacheDuration);
}

void Source::SetBundleName(const std::string& bundleName)
{
    if (plugin_ != nullptr) {
        MEDIA_LOG_I("SetBundleName bundleName: " PUBLIC_LOG_S, bundleName.c_str());
        plugin_->SetBundleName(bundleName);
    }
}

void Source::SetDemuxerState(int32_t streamId)
{
    if (plugin_ != nullptr) {
        plugin_->SetDemuxerState(streamId);
    }
}

std::string Source::GetContentType()
{
    FALSE_RETURN_V(plugin_ != nullptr, "");
    MEDIA_LOG_D("In");
    return plugin_->GetContentType();
}

Status Source::InitPlugin(const std::shared_ptr<MediaSource>& source)
{
    MediaAVCodec::AVCodecTrace trace("Source::InitPlugin");
    MEDIA_LOG_D("InitPlugin enter");
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_OPERATION, "InitPlugin, Source plugin is nullptr");

    Status ret = plugin_->Init();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "InitPlugin failed");

    plugin_->SetCallback(shared_from_this());
    plugin_->SetEnableOnlineFdCache(isEnableFdCache_);
    ret = plugin_->SetSource(source);

    MEDIA_LOG_D("InitPlugin exit");
    return ret;
}

Status Source::Prepare()
{
    MEDIA_LOG_I("Prepare entered.");
    if (plugin_ == nullptr) {
        return Status::OK;
    }
    Status ret = plugin_->Prepare();
    if (ret == Status::OK) {
        MEDIA_LOG_D("media source send EVENT_READY");
    } else if (ret == Status::ERROR_DELAY_READY) {
        if (isAboveWaterline_) {
            MEDIA_LOG_D("media source send EVENT_READY");
            isPluginReady_ = false;
            isAboveWaterline_ = false;
        }
    }
    return ret;
}

bool Source::IsSeekToTimeSupported()
{
    return seekToTimeFlag_;
}

Status Source::Start()
{
    MEDIA_LOG_I("Start entered.");
    return plugin_ ? plugin_->Start() : Status::ERROR_INVALID_OPERATION;
}

Status Source::GetBitRates(std::vector<uint32_t>& bitRates)
{
    MEDIA_LOG_I("GetBitRates");
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("GetBitRates failed, plugin_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return plugin_->GetBitRates(bitRates);
}

Status Source::SelectBitRate(uint32_t bitRate)
{
    MEDIA_LOG_I("SelectBitRate" PUBLIC_LOG_U32, bitRate);
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("SelectBitRate failed, plugin_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return plugin_->SelectBitRate(bitRate);
}

Status Source::AutoSelectBitRate(uint32_t bitRate)
{
    MEDIA_LOG_I("AutoSelectBitRate" PUBLIC_LOG_U32, bitRate);
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("AutoSelectBitRate failed, plugin_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return plugin_->AutoSelectBitRate(bitRate);
}

Status Source::SetCurrentBitRate(int32_t bitRate, int32_t streamID)
{
    MEDIA_LOG_D("SetCurrentBitRate");
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("SetCurrentBitRate failed, plugin_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return plugin_->SetCurrentBitRate(bitRate, streamID);
}

Status Source::SeekToTime(int64_t seekTime, SeekMode mode)
{
    if (seekable_ != Seekable::SEEKABLE) {
        GetSeekable();
    }
    int64_t timeNs;
    if (Plugins::Ms2HstTime(seekTime, timeNs)) {
        return plugin_->SeekToTime(timeNs, mode);
    } else {
        return Status::ERROR_INVALID_PARAMETER;
    }
}

Status Source::MediaSeekTimeByStreamId(int64_t seekTime, SeekMode mode, int32_t streamId)
{
    if (seekable_ != Seekable::SEEKABLE) {
        GetSeekable();
    }
    int64_t timeUs;
    return (plugin_ != nullptr && Plugins::Us2HstTime(seekTime, timeUs)) ?
        plugin_->MediaSeekTimeByStreamId(timeUs, mode, streamId) :
        Status::ERROR_INVALID_PARAMETER;
}

Status Source::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    MEDIA_LOG_D("GetDownloadInfo");
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("GetDownloadInfo failed, plugin_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return plugin_->GetDownloadInfo(downloadInfo);
}

Status Source::GetPlaybackInfo(PlaybackInfo& playbackInfo)
{
    MEDIA_LOG_D("GetPlaybackInfo");
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("GetPlaybackInfo  failed, plugin_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return plugin_->GetPlaybackInfo(playbackInfo);
}

bool Source::IsNeedPreDownload()
{
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("IsNeedPreDownload failed, plugin_ is nullptr");
        return false;
    }
    return plugin_->IsNeedPreDownload();
}

Status Source::Stop()
{
    MEDIA_LOG_D("Stop entered.");
    seekable_ = Seekable::INVALID;
    protocol_.clear();
    uri_.clear();
    return plugin_->Stop();
}

Status Source::Pause()
{
    MEDIA_LOG_D("Pause entered.");
    if (plugin_ != nullptr) {
        plugin_->Pause();
    }
    return Status::OK;
}

Status Source::Resume()
{
    MEDIA_LOG_I("Resume entered.");
    if (plugin_ != nullptr) {
        plugin_->Resume();
    }
    return Status::OK;
}

Status Source::SetReadBlockingFlag(bool isReadBlockingAllowed)
{
    MEDIA_LOG_D("SetReadBlockingFlag entered, IsReadBlockingAllowed %{public}d", isReadBlockingAllowed);
    FALSE_RETURN_V(plugin_ != nullptr, Status::OK);
    return plugin_->SetReadBlockingFlag(isReadBlockingAllowed);
}

void Source::OnEvent(const Plugins::PluginEvent& event)
{
    MEDIA_LOG_D("OnEvent");
    if (protocol_ == "http" && isInterruptNeeded_) {
        MEDIA_LOG_I("http OnEvent isInterruptNeeded, return");
        return;
    }
    if (event.type == PluginEventType::ABOVE_LOW_WATERLINE) {
        if (isPluginReady_ && isAboveWaterline_) {
            isPluginReady_ = false;
            isAboveWaterline_ = false;
        }
        return;
    }
    FALSE_RETURN_MSG(mediaDemuxerCallback_ != nullptr, "mediaDemuxerCallback is nullptr");
    if (event.type == PluginEventType::CLIENT_ERROR || event.type == PluginEventType::SERVER_ERROR) {
        MEDIA_LOG_I("Error happened, need notify client by OnEvent");
        mediaDemuxerCallback_->OnEvent(event);
    } else if (event.type == PluginEventType::SOURCE_DRM_INFO_UPDATE) {
        MEDIA_LOG_I("Drminfo updates from source");
        mediaDemuxerCallback_->OnEvent(event);
    } else if (event.type == PluginEventType::BUFFERING_END || event.type == PluginEventType::BUFFERING_START) {
        MEDIA_LOG_I("Buffering start or end.");
        mediaDemuxerCallback_->OnEvent(event);
    } else if (event.type == PluginEventType::SOURCE_BITRATE_START) {
        MEDIA_LOG_I("source bitrate start from source.");
        mediaDemuxerCallback_->OnEvent(event);
    } else if (event.type == PluginEventType::CACHED_DURATION) {
        MEDIA_LOG_D("Onevent cached duration.");
        mediaDemuxerCallback_->OnEvent(event);
    } else if (event.type == PluginEventType::EVENT_BUFFER_PROGRESS) {
        MEDIA_LOG_D("buffer percent update.");
        mediaDemuxerCallback_->OnEvent(event);
    } else if (event.type == PluginEventType::INITIAL_BUFFER_SUCCESS) {
        MEDIA_LOG_I("initial buffer success.");
        mediaDemuxerCallback_->OnEvent(event);
    } else if (event.type == PluginEventType::DASH_SEEK_READY) {
        MEDIA_LOG_D("Onevent dash seek ready.");
        mediaDemuxerCallback_->OnEvent(event);
    } else if (event.type == PluginEventType::FLV_AUTO_SELECT_BITRATE) {
        MEDIA_LOG_D("Onevent flv select bitrate.");
        mediaDemuxerCallback_->OnEvent(event);
    } else if (event.type == PluginEventType::HLS_SEEK_READY) {
        MEDIA_LOG_D("Onevent hls seek ready.");
        mediaDemuxerCallback_->OnEvent(event);
    } else {
        MEDIA_LOG_I("on event type undefined.");
    }
}

void Source::SetSelectBitRateFlag(bool flag, uint32_t desBitRate)
{
    if (mediaDemuxerCallback_) {
        mediaDemuxerCallback_->SetSelectBitRateFlag(flag, desBitRate);
    }
}

bool Source::CanAutoSelectBitRate()
{
    FALSE_RETURN_V_MSG_E(mediaDemuxerCallback_ != nullptr, false, "mediaDemuxerCallback_ is nullptr.");
    return mediaDemuxerCallback_->CanAutoSelectBitRate();
}

void Source::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_D("Source OnInterrupted %{public}d", isInterruptNeeded);
    std::unique_lock<std::mutex> lock(mutex_);
    isInterruptNeeded_ = isInterruptNeeded;
    if (plugin_) {
        plugin_->SetInterruptState(isInterruptNeeded_);
    }
    seekCond_.notify_all();
}

Plugins::Seekable Source::GetSeekable()
{
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Plugins::Seekable::INVALID, "GetSeekable, Source plugin is nullptr");
    int32_t retry {0};
    seekable_ = Seekable::INVALID;
    do {
        seekable_ = plugin_->GetSeekable();
        retry++;
        if (seekable_ == Seekable::INVALID) {
            if (retry >= MAX_RETRY) {
                break;
            }
            std::unique_lock<std::mutex> lock(mutex_);
            seekCond_.wait_for(lock, std::chrono::milliseconds(WAIT_TIME), [&] { return isInterruptNeeded_.load(); });
        }
    } while (seekable_ == Seekable::INVALID && !isInterruptNeeded_.load());
    return seekable_;
}

std::string Source::GetUriSuffix(const std::string& uri)
{
    MEDIA_LOG_D("IN");
    std::string suffix;
    auto const pos = uri.find_last_of('.');
    if (pos != std::string::npos) {
        suffix = uri.substr(pos + 1);
    }
    return suffix;
}

Status Source::Read(int32_t streamID, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
{
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_OPERATION, "ReadData, Source plugin is nullptr");
    FALSE_RETURN_V_NOLOG(!perfRecEnabled_, ReadWithPerfRecord(streamID, buffer, offset, expectedLen));
    if (seekToTimeFlag_) {
        return plugin_->Read(streamID, buffer, offset, expectedLen);
    }
    return plugin_->Read(buffer, offset, expectedLen);
}

Status Source::SetPerfRecEnabled(bool perfRecEnabled)
{
    perfRecEnabled_ = perfRecEnabled;
    return Status::OK;
}

Status Source::ReadWithPerfRecord(
    int32_t streamID, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
{
    int64_t readDuration = 0;
    Status readRes = Status::OK;
    if (seekToTimeFlag_) {
        readDuration = CALC_EXPR_TIME_MS(readRes = plugin_->Read(streamID, buffer, offset, expectedLen));
    } else {
        readDuration = CALC_EXPR_TIME_MS(readRes = plugin_->Read(buffer, offset, expectedLen));
    }
    int64_t readDurationUs = 0;
    FALSE_RETURN_V_MSG(
        Plugins::Ms2Us(readDuration, readDurationUs), readRes, "Invalid readDuration %{public}" PRId64, readDuration);
    int64_t readSpeed = static_cast<int64_t>(expectedLen) / readDurationUs;
    FALSE_RETURN_V_NOLOG(perfRecorder_.Record(readSpeed) == PerfRecorder::FULL, readRes);
    FALSE_RETURN_V_MSG(mediaDemuxerCallback_ != nullptr, readRes, "Report perf failed, callback is nullptr");
    mediaDemuxerCallback_->OnDfxEvent(
        { .type = Plugins::PluginDfxEventType::PERF_SOURCE, .param = perfRecorder_.GetMainPerfData() });
    perfRecorder_.Reset();
    return readRes;
}

Status Source::SeekTo(uint64_t offset)
{
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_OPERATION, "SeekTo, Source plugin is nullptr");
    return plugin_->SeekTo(offset);
}

Status Source::GetStreamInfo(std::vector<StreamInfo>& streams)
{
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_OPERATION,
        "GetStreamInfo, Source plugin is nullptr");
    Status ret = plugin_->GetStreamInfo(streams);
    if (ret == Status::OK && streams.size() == 0) {
        MEDIA_LOG_D("GetStreamInfo empty, MIX Stream");
        Plugins::StreamInfo info;
        info.streamId = 0;
        info.bitRate = 0;
        info.type = Plugins::MIXED;
        streams.push_back(info);
    }
    for (auto& iter : streams) {
        MEDIA_LOG_D("Source GetStreamInfo id = " PUBLIC_LOG_D32 " type = " PUBLIC_LOG_D32,
            iter.streamId, iter.type);
    }
    return ret;
}

bool Source::GetProtocolByUri()
{
    auto ret = true;
    auto const pos = uri_.find("://");
    if (pos != std::string::npos) {
        auto prefix = uri_.substr(0, pos);
        protocol_.append(prefix);
    } else {
        protocol_.append("file");
        std::string fullPath;
        ret = OSAL::ConvertFullPath(uri_, fullPath); // convert path to full path
        if (ret && !fullPath.empty()) {
            uri_ = fullPath;
        }
    }
    return ret;
}

bool Source::ParseProtocol(const std::shared_ptr<MediaSource>& source)
{
    bool ret = true;
    SourceType srcType = source->GetSourceType();
    MEDIA_LOG_D("sourceType = " PUBLIC_LOG_D32, CppExt::to_underlying(srcType));
    if (srcType == SourceType::SOURCE_TYPE_URI) {
        uri_ = source->GetSourceUri();
        isFlvLiveStream_ = source->GetMediaStreamList().size() > 0;
        std::string mimeType = source->GetMimeType();
        if (mimeType == AVMimeTypes::APPLICATION_M3U8) {
            protocol_ = "http";
        } else {
            ret = GetProtocolByUri();
        }
    } else if (srcType == SourceType::SOURCE_TYPE_FD) {
        protocol_.append("fd");
        uri_ = source->GetSourceUri();
    } else if (srcType == SourceType::SOURCE_TYPE_STREAM) {
        protocol_.append("stream");
        uri_.append("stream://");
    }
    return ret;
}

Status Source::FindPlugin(const std::shared_ptr<MediaSource>& source)
{
    MediaAVCodec::AVCodecTrace trace("Source::FindPlugin");
    if (!ParseProtocol(source)) {
        MEDIA_LOG_E("Invalid source!");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (protocol_.empty()) {
        MEDIA_LOG_E("protocol_ is empty");
        return Status::ERROR_INVALID_PARAMETER;
    }
    auto plugin = Plugins::PluginManagerV2::Instance().CreatePluginByMime(Plugins::PluginType::SOURCE, protocol_);
    if (plugin != nullptr) {
        plugin_ = std::static_pointer_cast<SourcePlugin>(plugin);
        plugin_->SetInterruptState(isInterruptNeeded_);
        return Status::OK;
    }
    MEDIA_LOG_E("Cannot find any plugin");
    return Status::ERROR_UNSUPPORTED_FORMAT;
}

bool Source::IsLocalFd()
{
    FALSE_RETURN_V_MSG_W(plugin_ != nullptr, false, "IsLocalFd source plugin is nullptr");
    return plugin_->IsLocalFd();
}

int64_t Source::GetDuration()
{
    FALSE_RETURN_V_MSG_W(seekToTimeFlag_, Plugins::HST_TIME_NONE, "Source GetDuration return -1 for isHls false");
    FALSE_RETURN_V_MSG_W(plugin_ != nullptr, Plugins::HST_TIME_NONE, "Source GetDuration error, plugin_ is nullptr");
    int64_t duration;
    Status ret = plugin_->GetDuration(duration);
    FALSE_RETURN_V_MSG_W(ret == Status::OK, Plugins::HST_TIME_NONE, "Source GetDuration from source plugin failed");
    return duration;
}

std::pair<int64_t, bool> Source::GetStartInfo()
{
    FALSE_RETURN_V_MSG_W(plugin_ != nullptr, std::make_pair(0, false), "GetStartInfo Source plugin is nullptr!");
    std::pair<int64_t, bool> startInfo;
    plugin_->GetStartInfo(startInfo);
    return startInfo;
}

Status Source::GetSize(uint64_t &fileSize)
{
    FALSE_RETURN_V_MSG_W(plugin_ != nullptr, Status::ERROR_INVALID_OPERATION, "GetSize Source plugin is nullptr!");
    return plugin_->GetSize(fileSize);
}

Status Source::SelectStream(int32_t streamID)
{
    FALSE_RETURN_V_MSG_W(plugin_ != nullptr, Status::ERROR_INVALID_OPERATION, "SelectStream Source plugin is nullptr!");
    return plugin_->SelectStream(streamID);
}

void Source::SetEnableOnlineFdCache(bool isEnableFdCache)
{
    isEnableFdCache_ = isEnableFdCache;
}

size_t Source::GetSegmentOffset()
{
    FALSE_RETURN_V_MSG_W(plugin_ != nullptr, 0, "GetSegmentOffset source plugin is nullptr!");
    return plugin_->GetSegmentOffset();
}

bool Source::GetHLSDiscontinuity()
{
    FALSE_RETURN_V_MSG_W(plugin_ != nullptr, false, "GetHLSDiscontinuity source plugin is nullptr!");
    return plugin_->GetHLSDiscontinuity();
}

bool Source::SetInitialBufferSize(int32_t offset, int32_t size)
{
    FALSE_RETURN_V_MSG_W(plugin_ != nullptr, false, "SetInitialBufferSize source plugin is nullptr");
    return plugin_->SetSourceInitialBufferSize(offset, size);
}

void Source::WaitForBufferingEnd()
{
    FALSE_RETURN_MSG(plugin_ != nullptr, "WaitForBufferingEnd source plugin is nullptr");
    return plugin_->WaitForBufferingEnd();
}

void Source::NotifyInitSuccess()
{
    FALSE_RETURN_MSG(plugin_ != nullptr, "NotifyInitSuccess source plugin is nullptr");
    plugin_->NotifyInitSuccess();
}

uint64_t Source::GetCachedDuration()
{
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, 0, "plugin_ is nullptr");
    return plugin_->GetCachedDuration();
}

void Source::RestartAndClearBuffer()
{
    FALSE_RETURN_MSG(plugin_ != nullptr, "plugin_ is nullptr");
    return plugin_->RestartAndClearBuffer();
}

bool Source::IsFlvLive()
{
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, false, "plugin_ is nullptr");
    return plugin_->IsFlvLive();
}

bool Source::IsHlsFmp4()
{
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, false, "plugin_ is nullptr");
    return plugin_->IsHlsFmp4();
}

uint64_t Source::GetMemorySize()
{
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, 0, "plugin_ is nullptr");
    return plugin_->GetMemorySize();
}

Status Source::StopBufferring(bool isAppBackground)
{
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_NULL_POINTER, "plugin_ is nullptr");
    return plugin_->StopBufferring(isAppBackground);
}

bool Source::IsHlsEnd(int32_t streamId)
{
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, false, "plugin_ is nullptr");
    return plugin_->IsHlsEnd(streamId);
}

bool Source::IsHls()
{
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, false, "plugin_ is nullptr");
    return plugin_->IsHls();
}
} // namespace Media
} // namespace OHOS