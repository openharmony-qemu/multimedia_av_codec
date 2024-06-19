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

#define HST_LOG_TAG "MediaDemuxer"
#define MEDIA_ATOMIC_ABILITY

#include "media_demuxer.h"

#include <algorithm>
#include <memory>
#include <map>

#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "cpp_ext/type_traits_ext.h"
#include "buffer/avallocator.h"
#include "common/event.h"
#include "common/log.h"
#include "frame_detector.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_manager.h"
#include "plugin/plugin_buffer.h"
#include "source/source.h"
#include "live_datasource_stream_demuxer.h"
#include "vod_stream_demuxer.h"
#include "live_http_stream_demuxer.h"
#include "media_core.h"
#include "osal/utils/dump_buffer.h"
#include "demuxer_plugin_manager.h"

namespace {
const std::string DUMP_PARAM = "a";
const std::string DUMP_DEMUXER_AUDIO_FILE_NAME = "player_demuxer_audio_output.es";
const std::string DUMP_DEMUXER_VIDEO_FILE_NAME = "player_demuxer_video_output.es";
} // namespace

namespace OHOS {
namespace Media {
constexpr uint32_t REQUEST_BUFFER_TIMEOUT = 0; // Requesting buffer overtimes 0ms means no retry
constexpr int32_t START = 1;
constexpr int32_t PAUSE = 2;
constexpr uint32_t RETRY_DELAY_TIME_US = 100000; // 100ms, Delay time for RETRY if no buffer in avbufferqueue producer.
constexpr uint32_t LOCK_WAIT_TIME = 3000; // Lock wait for 3000ms. if network wait long time.
constexpr double DECODE_RATE_THRESHOLD = 0.05;   // allow actual rate exceeding 5%
constexpr uint32_t REQUEST_FAILED_RETRY_TIMES = 12000; // Max times for RETRY if no buffer in avbufferqueue producer.
constexpr uint64_t DEFAULT_PREPARE_FRAME_COUNT = 0; // Default prepare frame count 0.

class MediaDemuxer::AVBufferQueueProducerListener : public IRemoteStub<IProducerListener> {
public:
    explicit AVBufferQueueProducerListener(uint32_t trackId, std::shared_ptr<MediaDemuxer> demuxer,
        std::unique_ptr<Task>& notifyTask) : trackId_(trackId), demuxer_(demuxer), notifyTask_(std::move(notifyTask)) {}

    virtual ~AVBufferQueueProducerListener() = default;
    int OnRemoteRequest(uint32_t code, MessageParcel& arguments, MessageParcel& reply, MessageOption& option) override
    {
        return IPCObjectStub::OnRemoteRequest(code, arguments, reply, option);
    }
    void OnBufferAvailable() override
    {
        MEDIA_LOG_D("AVBufferQueueProducerListener::OnBufferAvailable trackId:" PUBLIC_LOG_U32, trackId_);
        if (notifyTask_ == nullptr) {
            return;
        }
        notifyTask_->SubmitJobOnce([this] {
            auto demuxer = demuxer_.lock();
            if (demuxer) {
                demuxer->OnBufferAvailable(trackId_);
            }
        });
    }
private:
    uint32_t trackId_{0};
    std::weak_ptr<MediaDemuxer> demuxer_;
    std::unique_ptr<Task> notifyTask_;
};

class MediaDemuxer::TrackWrapper {
public:
    explicit TrackWrapper(uint32_t trackId, sptr<IProducerListener> listener, std::shared_ptr<MediaDemuxer> demuxer)
        : trackId_(trackId), listener_(listener), demuxer_(demuxer) {}
    sptr<IProducerListener> GetProducerListener()
    {
        return listener_;
    }
    void SetNotifyFlag(bool isNotifyNeeded)
    {
        isNotifyNeeded_ = isNotifyNeeded;
        MEDIA_LOG_D("SetNotifyFlag trackId:" PUBLIC_LOG_U32 ", isNotifyNeeded:" PUBLIC_LOG_D32,
            trackId_, isNotifyNeeded);
    }
    bool GetNotifyFlag()
    {
        return isNotifyNeeded_.load();
    }
private:
    std::atomic<bool> isNotifyNeeded_{false};
    uint32_t trackId_{0};
    sptr<IProducerListener> listener_ = nullptr;
    std::weak_ptr<MediaDemuxer> demuxer_;
};

MediaDemuxer::MediaDemuxer()
    : seekable_(Plugins::Seekable::INVALID),
      uri_(),
      mediaDataSize_(0),
      source_(std::make_shared<Source>()),
      subtitleSource_(std::make_shared<Source>()),
      mediaMetaData_(),
      bufferQueueMap_(),
      bufferMap_(),
      eventReceiver_(),
      streamDemuxer_(),
      demuxerPluginManager_(std::make_shared<DemuxerPluginManager>())
{
    MEDIA_LOG_I("MediaDemuxer called");
}

MediaDemuxer::~MediaDemuxer()
{
    MEDIA_LOG_I("~MediaDemuxer called");
    ResetInner();
    demuxerPluginManager_ = nullptr;
    mediaSource_ = nullptr;
    source_ = nullptr;
    eventReceiver_ = nullptr;
    eosMap_.clear();
    requestBufferErrorCountMap_.clear();
    streamDemuxer_ = nullptr;
    localDrmInfos_.clear();
}

void MediaDemuxer::OnBufferAvailable(uint32_t trackId)
{
    MEDIA_LOG_D("MediaDemuxer::OnBufferAvailable trackId:" PUBLIC_LOG_U32, trackId);
    AccelerateTrackTask(trackId); // buffer is available trigger demuxer track working task to run immediately.
}

void MediaDemuxer::AccelerateTrackTask(uint32_t trackId)
{
    AutoLock lock(mapMutex_);
    if (isStopped_ || isThreadExit_) {
        return;
    }
    auto track = trackMap_.find(trackId);
    if (track == trackMap_.end() || !track->second->GetNotifyFlag()) {
        return;
    }
    track->second->SetNotifyFlag(false);

    auto task = taskMap_.find(trackId);
    if (task == taskMap_.end()) {
        return;
    }
    MEDIA_LOG_I("AccelerateTrackTask trackId:" PUBLIC_LOG_U32, trackId);
    task->second->UpdataDelayTime();
}

void MediaDemuxer::SetTrackNotifyFlag(uint32_t trackId, bool isNotifyNeeded)
{
    // This function is called in demuxer track working thread, and if track info exists it is valid.
    auto track = trackMap_.find(trackId);
    if (track != trackMap_.end()) {
        track->second->SetNotifyFlag(isNotifyNeeded);
    }
}

Status MediaDemuxer::GetBitRates(std::vector<uint32_t> &bitRates)
{
    if (source_ == nullptr) {
        MEDIA_LOG_E("GetBitRates failed, source_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return source_->GetBitRates(bitRates);
}

Status MediaDemuxer::GetMediaKeySystemInfo(std::multimap<std::string, std::vector<uint8_t>> &infos)
{
    MEDIA_LOG_I("GetMediaKeySystemInfo called");
    std::shared_lock<std::shared_mutex> lock(drmMutex);
    infos = localDrmInfos_;
    return Status::OK;
}

Status MediaDemuxer::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    if (source_ == nullptr) {
        MEDIA_LOG_E("GetDownloadInfo failed, source_ is null");
        return Status::ERROR_INVALID_OPERATION;
    }
    return source_->GetDownloadInfo(downloadInfo);
}

void MediaDemuxer::SetDrmCallback(const std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> &callback)
{
    MEDIA_LOG_I("SetDrmCallback called");
    drmCallback_ = callback;
    bool isExisted = IsLocalDrmInfosExisted();
    if (isExisted) {
        MEDIA_LOG_D("SetDrmCallback Already received drminfo and report!");
        ReportDrmInfos(localDrmInfos_);
    }
}

void MediaDemuxer::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver)
{
    eventReceiver_ = receiver;
}

void MediaDemuxer::SetPlayerId(std::string playerId)
{
    playerId_ = playerId;
}

void MediaDemuxer::SetDumpInfo(bool isDump, uint64_t instanceId)
{
    if (isDump && instanceId == 0) {
        MEDIA_LOG_W("Cannot dump with instanceId 0.");
        return;
    }
    dumpPrefix_ = std::to_string(instanceId);
    isDump_ = isDump;
}

bool MediaDemuxer::GetDuration(int64_t& durationMs)
{
    AutoLock lock(mapMutex_);
    return mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs);
}

bool MediaDemuxer::IsDrmInfosUpdate(const std::multimap<std::string, std::vector<uint8_t>> &info)
{
    MEDIA_LOG_D("IsDrmInfosUpdate");
    bool isUpdated = false;
    std::unique_lock<std::shared_mutex> lock(drmMutex);
    for (auto &newItem : info) {
        if (newItem.second.size() == 0) {
            continue;
        }
        auto pos = localDrmInfos_.equal_range(newItem.first);
        if (pos.first == pos.second && pos.first == localDrmInfos_.end()) {
            MEDIA_LOG_D("this uuid doesn't exist, and update");
            localDrmInfos_.insert(newItem);
            isUpdated = true;
            continue;
        }
        bool isSame = false;
        for (; pos.first != pos.second; ++pos.first) {
            if (newItem.second == pos.first->second) {
                MEDIA_LOG_D("this uuid exists and same pssh, not update");
                isSame = true;
                break;
            }
        }
        if (!isSame) {
            MEDIA_LOG_D("this uuid exists but pssh not same, update");
            localDrmInfos_.insert(newItem);
            isUpdated = true;
        }
    }
    return isUpdated;
}

bool MediaDemuxer::IsLocalDrmInfosExisted()
{
    MEDIA_LOG_I("CheckLocalDrmInfos");
    std::shared_lock<std::shared_mutex> lock(drmMutex);
    return !localDrmInfos_.empty();
}

Status MediaDemuxer::ReportDrmInfos(const std::multimap<std::string, std::vector<uint8_t>> &info)
{
    MEDIA_LOG_I("ReportDrmInfos");
    FALSE_RETURN_V_MSG_E(drmCallback_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "ReportDrmInfos failed due to drmcallback nullptr.");
    MEDIA_LOG_I("demuxer filter will update drminfo OnDrmInfoChanged");
    drmCallback_->OnDrmInfoChanged(info);
    return Status::OK;
}

Status MediaDemuxer::ProcessDrmInfos()
{
    MEDIA_LOG_D("ProcessDrmInfos");
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = demuxerPluginManager_->GetCurVideoPlugin();
    FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
        "ProcessDrmInfos failed due to get demuxer plugin failed.");
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    Status ret = pluginTemp->GetDrmInfo(drmInfo);
    if (ret == Status::OK && !drmInfo.empty()) {
        MEDIA_LOG_D("MediaDemuxer get drminfo success");
        bool isUpdated = IsDrmInfosUpdate(drmInfo);
        if (isUpdated) {
            return ReportDrmInfos(drmInfo);
        } else {
            MEDIA_LOG_D("MediaDemuxer received drminfo but not update");
        }
    } else {
        if (ret != Status::OK) {
            MEDIA_LOG_D("MediaDemuxer get drminfo failed, ret=" PUBLIC_LOG_D32, (int32_t)(ret));
        }
    }
    return Status::OK;
}

Status MediaDemuxer::ProcessVideoStartTime(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    MEDIA_LOG_D("ProcessVideoStartTime,  trackId: %{public}u", trackId);
    if (trackId == videoTrackId_ && source_ != nullptr && source_->IsSeekToTimeSupported()
        && seekable_ == Plugins::Seekable::SEEKABLE && !(demuxerPluginManager_->IsDash())) {
        MEDIA_LOG_D("add start time, videoStartTime_: %{public}" PRId64 ", sample->pts_: %{public}" PRId64,
         videoStartTime_, sample->pts_);
        sample->pts_ += Plugins::HstTime2Us(videoStartTime_);
    }
    return Status::OK;
}

void MediaDemuxer::ReportIsLiveStreamEvent()
{
    if (eventReceiver_ == nullptr) {
        MEDIA_LOG_W("eventReceiver_ is nullptr!");
        return;
    }
    if (seekable_ == Plugins::Seekable::INVALID) {
        MEDIA_LOG_W("Seekable is invalid, do not report is_live_stream.");
        return;
    }
    if (seekable_ == Plugins::Seekable::UNSEEKABLE) {
        MEDIA_LOG_I("Report EventType::EVENT_IS_LIVE_STREAM.");
        eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_IS_LIVE_STREAM, true});
        return;
    }
}

Status MediaDemuxer::AddDemuxerCopyTask(uint32_t trackId, TaskType type)
{
    std::string taskName = "Demux";
    if (type == TaskType::VIDEO) {
        taskName += "V";
    } else if (type == TaskType::AUDIO) {
        taskName += "A";
    } else if (type == TaskType::SUBTITLE) {
        taskName += "S";
    } else {
        MEDIA_LOG_E("AddDemuxerCopyTask failed, unknow task type:" PUBLIC_LOG_D32, type);
        return Status::ERROR_UNKNOWN;
    }

    std::unique_ptr<Task> task = std::make_unique<Task>(taskName, playerId_, type);
    if (task == nullptr) {
        MEDIA_LOG_W("AddDemuxerCopyTask create task failed, trackId:" PUBLIC_LOG_U32 ", type:" PUBLIC_LOG_D32,
            trackId, type);
        return Status::OK;
    }
    taskMap_[trackId] = std::move(task);
    taskMap_[trackId]->RegisterJob([this, trackId] { return ReadLoop(trackId); });

    // To wake up DEMUXER TRACK WORKING TASK immediately on input buffer available.
    std::unique_ptr<Task> notifyTask =
        std::make_unique<Task>(taskName + "N", playerId_, type, TaskPriority::NORMAL, false);
    if (!notifyTask) {
        MEDIA_LOG_W("Add track notify task, make task failed, trackId:" PUBLIC_LOG_U32 ", type:" PUBLIC_LOG_D32,
            trackId, static_cast<uint32_t>(type));
        return Status::OK;
    }

    sptr<IProducerListener> listener =
        OHOS::sptr<AVBufferQueueProducerListener>::MakeSptr(trackId, shared_from_this(), notifyTask);
    if (listener == nullptr) {
        MEDIA_LOG_W("Add track notify task, make listener failed, trackId:" PUBLIC_LOG_U32 ", type:" PUBLIC_LOG_D32,
            trackId, static_cast<uint32_t>(type));
        return Status::OK;
    }

    trackMap_.emplace(trackId, std::make_shared<TrackWrapper>(trackId, listener, shared_from_this()));
    return Status::OK;
}

Status MediaDemuxer::SetDataSource(const std::shared_ptr<MediaSource> &source)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_I("SetDataSource enter");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    source_->SetCallback(this);
    auto res = source_->SetSource(source);
    FALSE_RETURN_V_MSG_E(res == Status::OK, res, "plugin set source failed.");
    Status ret = source_->GetSize(mediaDataSize_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set data source failed due to get file size failed.");
    seekable_ = source_->GetSeekable();
    FALSE_RETURN_V_MSG_E(seekable_ != Plugins::Seekable::INVALID, Status::ERROR_NULL_POINTER,
        "Set data source failed due to get seekable failed.");

    std::vector<StreamInfo> streams;
    source_->GetStreamInfo(streams);
    demuxerPluginManager_->InitDefaultPlay(streams);

    ReportIsLiveStreamEvent();
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        Flush();
        streamDemuxer_ = std::make_shared<VodStreamDemuxer>();
    } else if (source_->IsNeedPreDownload() && source_->GetSeekable() == Plugins::Seekable::UNSEEKABLE) {
        streamDemuxer_ = std::make_shared<LiveDataSourceStreamDemuxer>();
    } else {
        streamDemuxer_ = std::make_shared<LiveHttpStreamDemuxer>();
    }
    streamDemuxer_->SetSource(source_);
    streamDemuxer_->Init(uri_);

    Plugins::MediaInfo mediaInfo;
    ret = demuxerPluginManager_->LoadCurrentAllPlugin(streamDemuxer_, mediaInfo);
    if (ret == Status::OK) {
        InitMediaMetaData(mediaInfo, videoTrackId_, audioTrackId_, videoMime_);
        if (videoTrackId_ != TRACK_ID_DUMMY) {
            AddDemuxerCopyTask(videoTrackId_, TaskType::VIDEO);
            demuxerPluginManager_->UpdateTempTrackMapInfo(videoTrackId_, videoTrackId_);
            int32_t streamId = demuxerPluginManager_->GetStreamID(videoTrackId_);
            streamDemuxer_->SetNewVideoStreamID(streamId);
            streamDemuxer_->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
        }
        if (audioTrackId_ != TRACK_ID_DUMMY) {
            AddDemuxerCopyTask(audioTrackId_, TaskType::AUDIO);
            demuxerPluginManager_->UpdateTempTrackMapInfo(audioTrackId_, audioTrackId_);
            int32_t streamId = demuxerPluginManager_->GetStreamID(audioTrackId_);
            streamDemuxer_->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
        }
    } else {
        MEDIA_LOG_E("demuxer filter parse meta failed, ret: " PUBLIC_LOG_D32, (int32_t)(ret));
    }

    ProcessDrmInfos();
    MEDIA_LOG_I("SetDataSource exit");
    return ret;
}

Status MediaDemuxer::SetSubtitleSource(const std::shared_ptr<MediaSource> &subSource)
{
    subtitleSource_->SetCallback(this);
    subtitleSource_->SetSource(subSource);
    Status ret = subtitleSource_->GetSize(subMediaDataSize_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set subtitle data source failed due to get file size failed.");
    subSeekable_ = subtitleSource_->GetSeekable();

    std::vector<StreamInfo> subtitleStreams;
    subtitleSource_->GetStreamInfo(subtitleStreams);
    subtitleStreams[0].type = StreamType::SUBTITLE;
    subtitleStreams[0].streamId = demuxerPluginManager_->GetStreamCount();
    demuxerPluginManager_->InitDefaultPlay(subtitleStreams);
    subStreamDemuxer_ = std::make_shared<VodStreamDemuxer>();
    subStreamDemuxer_->SetSource(subtitleSource_);
    subStreamDemuxer_->Init(subSource->GetSourceUri());

    MediaInfo mediaInfo;
    ret = demuxerPluginManager_->LoadCurrentSubtitlePlugin(subStreamDemuxer_, mediaInfo);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "demuxer filter parse meta failed, ret: "
        PUBLIC_LOG_D32, (int32_t)(ret));
    InitSubtitleMediaMetaData(mediaInfo);
    if (extSubtitleTrackId_ != TRACK_ID_DUMMY) {
        AddDemuxerCopyTask(extSubtitleTrackId_, TaskType::SUBTITLE);
        demuxerPluginManager_->UpdateTempTrackMapInfo(extSubtitleTrackId_, extSubtitleTrackId_);
        int32_t streamId = demuxerPluginManager_->GetStreamID(extSubtitleTrackId_);
        subStreamDemuxer_->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
    }

    FALSE_RETURN_V_MSG(mediaInfo.tracks.size() != 0, Status::ERROR_WRONG_STATE, "subtitle no data");
    auto trackMeta = mediaInfo.tracks[0];
    mediaMetaData_.trackMetas.emplace_back(std::make_shared<Meta>(trackMeta));

    MEDIA_LOG_I("SetSubtitleSource done, ext sub track id = %{public}d", extSubtitleTrackId_);
    return ret;
}

void MediaDemuxer::SetInterruptState(bool isInterruptNeeded)
{
    if (source_ != nullptr) {
        source_->SetInterruptState(isInterruptNeeded);
    }
    if (streamDemuxer_ != nullptr) {
        streamDemuxer_->SetInterruptState(isInterruptNeeded);
    }
    if (subStreamDemuxer_ != nullptr) {
        subStreamDemuxer_->SetInterruptState(isInterruptNeeded);
    }
}

void MediaDemuxer::SetBundleName(const std::string& bundleName)
{
    if (source_ != nullptr) {
        MEDIA_LOG_I("SetBundleName bundleName: " PUBLIC_LOG_S, bundleName.c_str());
        bundleName_ = bundleName;
        streamDemuxer_->SetBundleName(bundleName);
        source_->SetBundleName(bundleName);
    }
}

Status MediaDemuxer::SetOutputBufferQueue(int32_t trackId, const sptr<AVBufferQueueProducer>& producer)
{
    AutoLock lock(mapMutex_);
    FALSE_RETURN_V_MSG_E(trackId >= 0 && (uint32_t)trackId < mediaMetaData_.trackMetas.size(),
        Status::ERROR_INVALID_PARAMETER, "Set bufferQueue trackId error.");
    useBufferQueue_ = true;
    MEDIA_LOG_I("Set bufferQueue for track " PUBLIC_LOG_D32 ".", trackId);
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    FALSE_RETURN_V_MSG_E(producer != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set bufferQueue failed due to input bufferQueue is nullptr.");
    if (bufferQueueMap_.find(trackId) != bufferQueueMap_.end()) {
        MEDIA_LOG_W("BufferQueue has been set already, will be overwritten. trackId:" PUBLIC_LOG_D32, trackId);
    }
    Status ret = InnerSelectTrack(trackId);
    if (ret == Status::OK) {
        auto track = trackMap_.find(trackId);
        if (track != trackMap_.end()) {
            auto listener = track->second->GetProducerListener();
            if (listener) {
                MEDIA_LOG_W("SetBufferAvailableListener trackId:" PUBLIC_LOG_D32, trackId);
                producer->SetBufferAvailableListener(listener);
            }
        }
        bufferQueueMap_.insert(std::pair<uint32_t, sptr<AVBufferQueueProducer>>(trackId, producer));
        bufferMap_.insert(std::pair<uint32_t, std::shared_ptr<AVBuffer>>(trackId, nullptr));
        MEDIA_LOG_I("Set bufferQueue successfully.");
    }
    return ret;
}

void MediaDemuxer::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D("MediaDemuxer::OnDumpInfo called.");
    if (fd < 0) {
        MEDIA_LOG_E("MediaDemuxer::OnDumpInfo fd is invalid.");
        return;
    }
    std::string dumpString;
    dumpString += "MediaDemuxer buffer queue map size: " + std::to_string(bufferQueueMap_.size()) + "\n";
    dumpString += "MediaDemuxer buffer map size: " + std::to_string(bufferMap_.size()) + "\n";
    int ret = write(fd, dumpString.c_str(), dumpString.size());
    if (ret < 0) {
        MEDIA_LOG_E("MediaDemuxer::OnDumpInfo write failed.");
        return;
    }
}

std::map<uint32_t, sptr<AVBufferQueueProducer>> MediaDemuxer::GetBufferQueueProducerMap()
{
    return bufferQueueMap_;
}

Status MediaDemuxer::InnerSelectTrack(int32_t trackId)
{
    eosMap_[trackId] = false;
    requestBufferErrorCountMap_[trackId] = 0;

    int32_t innerTrackID = trackId;
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    if (demuxerPluginManager_->IsDash() || demuxerPluginManager_->IsSubtitle()) {
        pluginTemp = demuxerPluginManager_->SelectPlugin(trackId);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "InnerSelectTrack failed due to get demuxer plugin failed.");
        innerTrackID = demuxerPluginManager_->GetInnerTrackID(trackId);
    } else {
        pluginTemp = demuxerPluginManager_->GetCurVideoPlugin();
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "InnerSelectTrack failed due to get demuxer plugin failed.");
    }

    return pluginTemp->SelectTrack(innerTrackID);
}

Status MediaDemuxer::StartAudioTask()
{
    MEDIA_LOG_I("StartAudioTask trackId: " PUBLIC_LOG_D32, audioTrackId_);
    if (!taskMap_[audioTrackId_]->IsTaskRunning()) {
        taskMap_[audioTrackId_]->Start();
    }
    return Status::OK;
}

Status MediaDemuxer::SelectTrack(int32_t trackId)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    FALSE_RETURN_V_MSG_E(trackId >= 0 && (uint32_t)trackId < mediaMetaData_.trackMetas.size(),
        Status::ERROR_INVALID_PARAMETER, "Select trackId error.");
    if (!useBufferQueue_) {
        return InnerSelectTrack(trackId);
    }
    std::string mimeType;
    Status ret = Status::OK;
    if (mediaMetaData_.trackMetas[trackId]->Get<Tag::MIME_TYPE>(mimeType) && mimeType.find("audio") == 0) {
        MEDIA_LOG_I("SelectTrack now: " PUBLIC_LOG_D32 ", to: " PUBLIC_LOG_D32, audioTrackId_, trackId);
        if (audioTrackId_ != static_cast<uint32_t>(trackId)) {
            if (audioTrackId_ != TRACK_ID_DUMMY) {
                taskMap_[audioTrackId_]->Stop();
                ret = UnselectTrack(audioTrackId_);
                FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Unselect track error.");
                bufferQueueMap_.insert(
                    std::pair<uint32_t, sptr<AVBufferQueueProducer>>(trackId, bufferQueueMap_[audioTrackId_]));
                bufferMap_.insert(std::pair<uint32_t, std::shared_ptr<AVBuffer>>(trackId, bufferMap_[audioTrackId_]));
                bufferQueueMap_.erase(audioTrackId_);
                bufferMap_.erase(audioTrackId_);
            }
            ret = InnerSelectTrack(trackId);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Select track error.");
            audioTrackId_ = trackId;
        }
    } else {
        FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE,
            "Cannot select track when use buffer queue.");
        return InnerSelectTrack(trackId);
    }
    return Status::OK;
}

Status MediaDemuxer::UnselectTrack(int32_t trackId)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_I("UnselectTrack trackId: " PUBLIC_LOG_D32, trackId);

    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    int32_t innerTrackID = trackId;
    if (demuxerPluginManager_->IsDash() || demuxerPluginManager_->IsSubtitle()) {
        pluginTemp = demuxerPluginManager_->SelectPlugin(trackId);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "UnselectTrack failed due to get demuxer plugin failed.");
        innerTrackID = demuxerPluginManager_->GetInnerTrackID(trackId);
    } else {
        pluginTemp = demuxerPluginManager_->GetCurVideoPlugin();
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "UnselectTrack failed due to get demuxer plugin failed.");
    }

    return pluginTemp->UnselectTrack(innerTrackID);
}

Status MediaDemuxer::SeekToTimePre(bool jumperRestartPlugin)
{
    if (demuxerPluginManager_->IsDash()) {
        if (jumperRestartPlugin == true) {
            int32_t streamID = demuxerPluginManager_->GetStreamID(audioTrackId_);
            MEDIA_LOG_I("SeekTo source SeekToTime stop audio plugin, id = " PUBLIC_LOG_D32, streamID);
            demuxerPluginManager_->StopPlugin(streamID);
            streamDemuxer_->ResetCache(streamID);
        } else {
            MEDIA_LOG_I("SeekTo source SeekToTime stop all plugin");
            demuxerPluginManager_->StopAllPlugin();
            streamDemuxer_->ResetAllCache();
        }
    }
    return Status::OK;
}

Status MediaDemuxer::SeekToTimeAfter(bool jumperRestartPlugin)
{
    if (demuxerPluginManager_->IsDash()) {
        if (jumperRestartPlugin == true) {
            if (audioTrackId_ != TRACK_ID_DUMMY) {
                int32_t streamID = demuxerPluginManager_->GetStreamID(audioTrackId_);
                streamDemuxer_->SetDemuxerState(streamID, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
                demuxerPluginManager_->StartPlugin(streamID, streamDemuxer_);
                InnerSelectTrack(audioTrackId_);
                streamDemuxer_->SetDemuxerState(streamID, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
            }
        } else {
            if (videoTrackId_ != TRACK_ID_DUMMY) {
                int32_t streamID = demuxerPluginManager_->GetStreamID(videoTrackId_);
                streamDemuxer_->SetDemuxerState(streamID, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
            }
            if (audioTrackId_ != TRACK_ID_DUMMY) {
                int32_t streamID = demuxerPluginManager_->GetStreamID(audioTrackId_);
                streamDemuxer_->SetDemuxerState(streamID, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
            }
            demuxerPluginManager_->StartAllPlugin(streamDemuxer_);
            if (videoTrackId_ != TRACK_ID_DUMMY) {
                InnerSelectTrack(videoTrackId_);
                int32_t streamID = demuxerPluginManager_->GetStreamID(videoTrackId_);
                streamDemuxer_->SetDemuxerState(streamID, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
            }
            if (audioTrackId_ != TRACK_ID_DUMMY) {
                InnerSelectTrack(audioTrackId_);
                int32_t streamID = demuxerPluginManager_->GetStreamID(audioTrackId_);
                streamDemuxer_->SetDemuxerState(streamID, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
            }
        }
    }
    return Status::OK;
}

Status MediaDemuxer::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    Status ret;
    if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
        bool jumperRestartPlugin = (isSelectBitRate_.load() == true) ? true : false;
        MEDIA_LOG_I("SeekTo source SeekToTime start, jumperRestartPlugin = " PUBLIC_LOG_D32, jumperRestartPlugin);
        SeekToTimePre(jumperRestartPlugin);
        if (mode == SeekMode::SEEK_CLOSEST_INNER) {
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_PREVIOUS_SYNC);
        } else {
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_CLOSEST_SYNC);
        }
        SeekToTimeAfter(jumperRestartPlugin);
        Plugins::Ms2HstTime(seekTime, realSeekTime);
    } else {
        MEDIA_LOG_I("SeekTo start");
        if (mode == SeekMode::SEEK_CLOSEST_INNER) {
            ret = demuxerPluginManager_->SeekTo(seekTime, SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
        } else {
            ret = demuxerPluginManager_->SeekTo(seekTime, mode, realSeekTime);
        }
    }
    isSeeked_ = true;
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    for (auto item : requestBufferErrorCountMap_) {
        requestBufferErrorCountMap_[item.first] = 0;
    }
    MEDIA_LOG_I("SeekTo done");
    return ret;
}

Status MediaDemuxer::SelectBitRate(uint32_t bitRate)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "SelectBitRate failed, source_ is nullptr.");
    MEDIA_LOG_I("SelectBitRate begin");
    if (demuxerPluginManager_->IsDash()) {
        if (isSelectBitRate_.load() == true) {
            MEDIA_LOG_W("SelectBitRate is running, can not select");
            return Status::OK;
        }
        isSelectBitRate_.store(true);
    }
    streamDemuxer_->Reset();
    Status ret = source_->SelectBitRate(bitRate);
    if (ret != Status::OK) {
        MEDIA_LOG_E("MediaDemuxer SelectBitRate failed");
        if (demuxerPluginManager_->IsDash()) {
            isSelectBitRate_.store(false);
        }
    } else {
        if (demuxerPluginManager_->IsDash() && bitRate == demuxerPluginManager_->GetCurrentBitRate()) {
            isSelectBitRate_.store(false);
        }
    }
    return ret;
}

std::vector<std::shared_ptr<Meta>> MediaDemuxer::GetStreamMetaInfo() const
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    return mediaMetaData_.trackMetas;
}

std::shared_ptr<Meta> MediaDemuxer::GetGlobalMetaInfo()
{
    AutoLock lock(mapMutex_);
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    return mediaMetaData_.globalMeta;
}

std::shared_ptr<Meta> MediaDemuxer::GetUserMeta()
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    return demuxerPluginManager_->GetUserMeta();
}

Status MediaDemuxer::Flush()
{
    MEDIA_LOG_I("Flush");
    if (streamDemuxer_) {
        streamDemuxer_->Flush();
    }
    
    {
        AutoLock lock(mapMutex_);
        auto it = bufferQueueMap_.begin();
        while (it != bufferQueueMap_.end()) {
            uint32_t trackId = it->first;
            if (trackId != videoTrackId_) {
                bufferQueueMap_[trackId]->Clear();
            }
            it++;
        }
    }

    if (demuxerPluginManager_) {
        if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
            demuxerPluginManager_->SetResetEosStatus(true);
        }
        demuxerPluginManager_->Flush();
    }

    return Status::OK;
}

Status MediaDemuxer::StopAllTask()
{
    MEDIA_LOG_I("StopAllTask enter.");
    if (streamDemuxer_ != nullptr) {
        streamDemuxer_->SetIsIgnoreParse(true);
    }
    if (source_ != nullptr) {
        source_->Stop();
    }

    auto it = taskMap_.begin();
    while (it != taskMap_.end()) {
        if (it->second != nullptr) {
            it->second->Stop();
            it->second = nullptr;
        }
        it = taskMap_.erase(it);
    }
    isThreadExit_ = true;
    MEDIA_LOG_I("StopAllTask done.");
    return Status::OK;
}

Status MediaDemuxer::StopTask(uint32_t trackId)
{
    MEDIA_LOG_I("StopTask trackId: %{public}u", trackId);
    auto it = taskMap_.begin();
    while (it != taskMap_.end()) {
        if (it->first != trackId) {
            continue;
        }
        if (it->second != nullptr) {
            it->second->Stop();
            it->second = nullptr;
        }
        it = taskMap_.erase(it);
        break;
    }
    return Status::OK;
}

Status MediaDemuxer::PauseAllTask()
{
    MEDIA_LOG_I("PauseAllTask enter.");
    // To accelerate DemuxerLoop thread to run into PAUSED state
    for (auto &iter : taskMap_) {
        if (iter.second != nullptr) {
            iter.second->PauseAsync();
        }
    }

    for (auto &iter : taskMap_) {
        if (iter.second != nullptr) {
            iter.second->Pause();
        }
    }
    MEDIA_LOG_I("PauseAllTask done.");
    return Status::OK;
}

Status MediaDemuxer::ResumeAllTask()
{
    MEDIA_LOG_I("ResumeAllTask enter.");
    streamDemuxer_->SetIsIgnoreParse(false);

    auto it = bufferQueueMap_.begin();
    while (it != bufferQueueMap_.end()) {
        uint32_t trackId = it->first;
        taskMap_[trackId]->Start();
        it++;
    }
    MEDIA_LOG_I("ResumeAllTask done.");
    return Status::OK;
}

Status MediaDemuxer::Pause()
{
    MEDIA_LOG_I("Pause");
    isPaused_ = true;
    if (streamDemuxer_) {
        streamDemuxer_->SetIsIgnoreParse(true);
        streamDemuxer_->Pause();
    }
    if (source_) {
        source_->SetReadBlockingFlag(false); // Disable source read blocking to prevent pause all task blocking
        source_->Pause();
    }
    PauseAllTask();
    if (source_ != nullptr) {
        source_->SetReadBlockingFlag(true); // Enable source read blocking to ensure get wanted data
    }
    return Status::OK;
}

Status MediaDemuxer::PauseTaskByTrackId(int32_t trackId)
{
    MEDIA_LOG_I("Pause trackId: %{public}d", trackId);
    FALSE_RETURN_V_MSG_E(trackId >= 0, Status::ERROR_INVALID_PARAMETER, "Track id is invalid.");

    // To accelerate DemuxerLoop thread to run into PAUSED state
    for (auto &iter : taskMap_) {
        if (iter.first == static_cast<uint32_t>(trackId) && iter.second != nullptr) {
            iter.second->PauseAsync();
        }
    }

    for (auto &iter : taskMap_) {
        if (iter.first == static_cast<uint32_t>(trackId) && iter.second != nullptr) {
            iter.second->Pause();
        }
    }
    return Status::OK;
}

Status MediaDemuxer::Resume()
{
    MEDIA_LOG_I("Resume");
    if (streamDemuxer_) {
        streamDemuxer_->Resume();
    }
    if (source_) {
        source_->Resume();
    }
    ResumeAllTask();
    isPaused_ = false;
    return Status::OK;
}

void MediaDemuxer::ResetInner()
{
    std::map<uint32_t, std::shared_ptr<TrackWrapper>> trackMap;
    {
        AutoLock lock(mapMutex_);
        mediaMetaData_.globalMeta.reset();
        mediaMetaData_.trackMetas.clear();
        if (!isThreadExit_) {
            Stop();
        }
        std::swap(trackMap, trackMap_);
        bufferQueueMap_.clear();
        bufferMap_.clear();
        localDrmInfos_.clear();
    }
    // Should perform trackMap_ clear without holding mapMutex_ to avoid dead lock:
    // 1. TrackWrapper indirectly holds notifyTask which holds its jobMutex_ firstly when run, then requires mapMutex_.
    // 2. Release notifyTask also needs hold its jobMutex_ firstly.
    trackMap.clear();
}

Status MediaDemuxer::Reset()
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::Reset");
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    ResetInner();
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    for (auto item : requestBufferErrorCountMap_) {
        requestBufferErrorCountMap_[item.first] = 0;
    }
    videoStartTime_ = 0;
    streamDemuxer_->Reset();
    return demuxerPluginManager_->Reset();
}

Status MediaDemuxer::Start()
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::Start");
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::OK,
        "Process has been started already, neet to stop it first.");
    FALSE_RETURN_V_MSG_E(bufferQueueMap_.size() != 0, Status::OK,
        "Read task is running already.");
    for (auto it = eosMap_.begin(); it != eosMap_.end(); it++) {
        it->second = false;
    }
    for (auto it = requestBufferErrorCountMap_.begin(); it != requestBufferErrorCountMap_.end(); it++) {
        it->second = 0;
    }
    isThreadExit_ = false;
    isStopped_ = false;
    auto it = bufferQueueMap_.begin();
    while (it != bufferQueueMap_.end()) {
        uint32_t trackId = it->first;
        taskMap_[trackId]->Start();
        it++;
    }
    MEDIA_LOG_I("Demuxer thread started.");
    source_->Start();
    return demuxerPluginManager_->Start();
}

Status MediaDemuxer::Stop()
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::Stop");
    MEDIA_LOG_I("MediaDemuxer Stop.");
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    FALSE_RETURN_V_MSG_E(!isThreadExit_, Status::OK, "Process has been stopped already, need to start if first.");
    isStopped_ = true;
    StopAllTask();
    streamDemuxer_->Stop();
    return demuxerPluginManager_->Stop();
}

bool MediaDemuxer::HasVideo()
{
    return videoTrackId_ != TRACK_ID_DUMMY;
}

Status MediaDemuxer::PrepareFrame(bool renderFirstFrame)
{
    MEDIA_LOG_I("PrepareFrame enter.");
    doPrepareFrame_ = true;
    Status ret = Status::OK;
    if ((firstFrameCount_ != DEFAULT_PREPARE_FRAME_COUNT) || waitForDataFail_) {
        MEDIA_LOG_I("Current is Seeking and resume demuxer");
        ret = Resume();
    } else {
        ret = Start();
    }
    if (ret != Status::OK) {
        MEDIA_LOG_E("PrepareFrame and start demuxer failed.");
        return ret;
    }
    AutoLock lock(firstFrameMutex_);
    bool res = firstFrameCond_.WaitFor(lock, LOCK_WAIT_TIME, [this] {
         return firstFrameCount_ == taskMap_.size();
    });
    doPrepareFrame_ = false;
    if (!res) {
        MEDIA_LOG_E("PrepareFrame wait data failed res= %{public}d.", res);
        waitForDataFail_ = true;
    }
    return Pause();
}

void MediaDemuxer::InitMediaMetaData(const Plugins::MediaInfo& mediaInfo, uint32_t& videoTrackId,
    uint32_t& audioTrackId, std::string& videoMime)
{
    AutoLock lock(mapMutex_);
    mediaMetaData_.globalMeta = std::make_shared<Meta>(mediaInfo.general);
    if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
        int64_t duration = source_->GetDuration();
        if (duration != Plugins::HST_TIME_NONE) {
            MEDIA_LOG_I("InitMediaMetaData for hls, duration: " PUBLIC_LOG_D64, duration);
            mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(Plugins::HstTime2Us(duration));
        }
    }
    mediaMetaData_.trackMetas.clear();
    mediaMetaData_.trackMetas.reserve(mediaInfo.tracks.size());
    for (uint32_t index = 0; index < mediaInfo.tracks.size(); index++) {
        auto trackMeta = mediaInfo.tracks[index];
        mediaMetaData_.trackMetas.emplace_back(std::make_shared<Meta>(trackMeta));
        std::string mimeType;
        std::string trackType;
        if (trackMeta.Get<Tag::MIME_TYPE>(mimeType) && mimeType.find("video") == 0 &&
            !IsTrackDisabled(Plugins::MediaType::VIDEO)) {
            MEDIA_LOG_I("Found video track, id: " PUBLIC_LOG_U32 ", mimeType: " PUBLIC_LOG_S, index, mimeType.c_str());
            videoMime = mimeType;
            videoTrackId = index;
            if (!trackMeta.GetData(Tag::MEDIA_START_TIME, videoStartTime_)) {
                MEDIA_LOG_W("Get media start time failed");
            }
        } else if (trackMeta.Get<Tag::MIME_TYPE>(mimeType) && mimeType.find("audio") == 0 &&
            !IsTrackDisabled(Plugins::MediaType::AUDIO)) {
            MEDIA_LOG_I("Found audio track, id: " PUBLIC_LOG_U32 ", mimeType: " PUBLIC_LOG_S, index, mimeType.c_str());
            if (audioTrackId_ == TRACK_ID_DUMMY) {
                audioTrackId = index;
            }
        }
    }
}

void MediaDemuxer::InitSubtitleMediaMetaData(const Plugins::MediaInfo& mediaInfo)
{
    AutoLock lock(mapMutex_);
    subMediaMetaData_.globalMeta = std::make_shared<Meta>(mediaInfo.general);
    if (subtitleSource_ != nullptr && subtitleSource_->IsSeekToTimeSupported()) {
        int64_t duration = subtitleSource_->GetDuration();
        if (duration != Plugins::HST_TIME_NONE) {
            MEDIA_LOG_I("InitMediaMetaData for hls, duration: " PUBLIC_LOG_D64, duration);
            subMediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(Plugins::HstTime2Us(duration));
        }
    }
    subMediaMetaData_.trackMetas.clear();
    subMediaMetaData_.trackMetas.reserve(mediaInfo.tracks.size());
    for (uint32_t index = 0; index < mediaInfo.tracks.size(); index++) {
        auto trackMeta = mediaInfo.tracks[index];
        subMediaMetaData_.trackMetas.emplace_back(std::make_shared<Meta>(trackMeta));
        std::string mimeType;
        std::string trackType;
        trackMeta.Get<Tag::MIME_TYPE>(mimeType);

        if (trackMeta.Get<Tag::MIME_TYPE>(mimeType) && mimeType.find("application/x-subrip") == 0) {
            MEDIA_LOG_I("Found subtitle track, id: " PUBLIC_LOG_U32 ", mimeType: " PUBLIC_LOG_S,
                index, mimeType.c_str());
            extSubtitleTrackId_ = mediaMetaData_.trackMetas.size() + index;
            break;
        }
    }
}

bool MediaDemuxer::IsOffsetValid(int64_t offset) const
{
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        return mediaDataSize_ == 0 || offset <= static_cast<int64_t>(mediaDataSize_);
    }
    return true;
}

bool MediaDemuxer::GetBufferFromUserQueue(uint32_t queueIndex, uint32_t size)
{
    MEDIA_LOG_D("Get buffer from user queue: " PUBLIC_LOG_U32 ", size: " PUBLIC_LOG_U32, queueIndex, size);
    FALSE_RETURN_V_MSG_E(bufferQueueMap_.count(queueIndex) > 0 && bufferQueueMap_[queueIndex] != nullptr, false,
        "bufferQueue " PUBLIC_LOG_D32 " is nullptr", queueIndex);

    AVBufferConfig avBufferConfig;
    avBufferConfig.capacity = static_cast<int32_t>(size);
    Status ret = bufferQueueMap_[queueIndex]->RequestBuffer(bufferMap_[queueIndex], avBufferConfig,
        REQUEST_BUFFER_TIMEOUT);
    if (ret != Status::OK) {
        requestBufferErrorCountMap_[queueIndex]++;
        MEDIA_LOG_D("Request buffer failed, try again later, user queue: " PUBLIC_LOG_U32 ", ret: " PUBLIC_LOG_D32,
            queueIndex, (int32_t)(ret));
        if (requestBufferErrorCountMap_[queueIndex] >= REQUEST_FAILED_RETRY_TIMES) {
            MEDIA_LOG_E("Request buffer failed from buffer queue too many times in one minute.");
        }
    } else {
        requestBufferErrorCountMap_[queueIndex] = 0;
    }
    return ret == Status::OK;
}

bool MediaDemuxer::ChangeStream(uint32_t trackId)
{
    int32_t videoStreamID = demuxerPluginManager_->GetStreamID(videoTrackId_);
    int32_t newStreamID = streamDemuxer_->GetNewVideoStreamID();
    if (newStreamID >= 0 && videoStreamID != newStreamID) {
        MEDIA_LOG_I("ChangeStream dash, END_OF_STREAM begin");
        streamDemuxer_->ResetCache(videoStreamID);
        Plugins::MediaInfo mediaInfo;
        streamDemuxer_->SetDemuxerState(videoStreamID, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
        demuxerPluginManager_->UpdateDefaultVideoStreamID(streamDemuxer_, mediaInfo);

        uint32_t tempVideoTrack = TRACK_ID_DUMMY;
        uint32_t tempAudioTrack = TRACK_ID_DUMMY;
        InitMediaMetaData(mediaInfo, tempVideoTrack, tempAudioTrack, videoMime_);
        int32_t localVideoTrackId_ = static_cast<int32_t>(videoTrackId_);
        int32_t localTempVideoTrack = static_cast<int32_t>(tempVideoTrack);
        int32_t localAudioTrackId_ = static_cast<int32_t>(audioTrackId_);
        int32_t localTempAudioTrack = static_cast<int32_t>(tempAudioTrack);
        if (tempVideoTrack != TRACK_ID_DUMMY) {
            demuxerPluginManager_->UpdateTempTrackMapInfo(localVideoTrackId_, localTempVideoTrack);
        }
        if (tempAudioTrack != TRACK_ID_DUMMY) {
            demuxerPluginManager_->UpdateTempTrackMapInfo(localAudioTrackId_, localTempAudioTrack);
        }
        MEDIA_LOG_I("ChangeStream dash, UpdateTempTrackMapInfo done");
        InnerSelectTrack(videoTrackId_);
        MEDIA_LOG_I("ChangeStream dash, InnerSelectTrack video done");
        streamDemuxer_->SetDemuxerState(videoStreamID, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
        isSelectBitRate_.store(false);
        MEDIA_LOG_I("ChangeStream dash, END_OF_STREAM end");
        return true;
    }
    return false;
}

void MediaDemuxer::DumpBufferToFile(uint32_t trackId, std::shared_ptr<AVBuffer> buffer)
{
    std::string mimeType;
    if (isDump_) {
        if (mediaMetaData_.trackMetas[trackId]->Get<Tag::MIME_TYPE>(mimeType) && mimeType.find("audio") == 0) {
                DumpAVBufferToFile(DUMP_PARAM, dumpPrefix_ + DUMP_DEMUXER_AUDIO_FILE_NAME, buffer);
        }
        if (mediaMetaData_.trackMetas[trackId]->Get<Tag::MIME_TYPE>(mimeType) && mimeType.find("video") == 0) {
                DumpAVBufferToFile(DUMP_PARAM, dumpPrefix_ + DUMP_DEMUXER_VIDEO_FILE_NAME, buffer);
        }
    }
}

Status MediaDemuxer::HandleRead(uint32_t trackId)
{
    Status ret = InnerReadSample(trackId, bufferMap_[trackId]);
    if (source_ != nullptr && source_->IsSeekToTimeSupported() && isSeeked_ && HasVideo()) {
        if (trackId != videoTrackId_ || ret != Status::OK ||
            !IsContainIdrFrame(bufferMap_[trackId]->memory_->GetAddr(), bufferMap_[trackId]->memory_->GetSize())) {
            if (firstAudio_ && trackId == audioTrackId_) {
                bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], true);
                firstAudio_ = false;
                return Status::ERROR_INVALID_PARAMETER;
            }
            bool isEos = (bufferMap_[trackId]->flag_ & (uint32_t)(AVBufferFlag::EOS)) != 0;
            bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], isEos);
            eosMap_[trackId] = isEos;
            MEDIA_LOG_I("CopyFrameToUserQueue is seeking, not found idr frame. trackId: " PUBLIC_LOG_U32
                ", isEos: %{public}i", trackId, isEos);
            return Status::ERROR_INVALID_PARAMETER;
        }
        MEDIA_LOG_I("CopyFrameToUserQueue is seeking, found idr frame. trackId: " PUBLIC_LOG_U32, trackId);
        isSeeked_ = false;
    }
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        if (bufferMap_[trackId]->flag_ & (uint32_t)(AVBufferFlag::EOS)) {
            eosMap_[trackId] = true;
            taskMap_[trackId]->StopAsync();
            MEDIA_LOG_I("CopyFrameToUserQueue track eos, trackId: " PUBLIC_LOG_U32 ", bufferId: " PUBLIC_LOG_U64
                ", pts: " PUBLIC_LOG_U64 ", flag: " PUBLIC_LOG_U32, trackId, bufferMap_[trackId]->GetUniqueId(),
                bufferMap_[trackId]->pts_, bufferMap_[trackId]->flag_);
            ret = bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], true);
            return Status::OK;
        }
        bool isDroppable = IsBufferDroppable(bufferMap_[trackId], trackId);
        bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], !isDroppable);
    } else {
        bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], false);
        MEDIA_LOG_E("ReadSample failed, track " PUBLIC_LOG_U32 ", ret: " PUBLIC_LOG_D32, trackId, (int32_t)(ret));
    }
    return ret;
}

Status MediaDemuxer::CopyFrameToUserQueue(uint32_t trackId)
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::CopyFrameToUserQueue");
    MEDIA_LOG_D("CopyFrameToUserQueue enter, track:" PUBLIC_LOG_U32, trackId);
    int32_t size = 0;
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = demuxerPluginManager_->SelectPlugin(trackId);
    FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
        "CopyFrameToUserQueue failed, pluginTemp is nullptr.");
    int32_t innerTrackId = demuxerPluginManager_->GetInnerTrackID(trackId);
    Status ret = pluginTemp->GetNextSampleSize(innerTrackId, size);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_UNKNOWN, Status::ERROR_UNKNOWN,
        "CopyFrameToUserQueue error for track " PUBLIC_LOG_U32, trackId);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_AGAIN, Status::ERROR_AGAIN,
        "CopyFrameToUserQueue error for track " PUBLIC_LOG_U32 ", try again", trackId);
    if (trackId == videoTrackId_ && demuxerPluginManager_->IsDash()) {
        auto result = ChangeStream(trackId);
        if (result) {
            return Status::OK;
        }
    }
    SetTrackNotifyFlag(trackId, true);
    if (!GetBufferFromUserQueue(trackId, size)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    SetTrackNotifyFlag(trackId, false);
    ret = HandleRead(trackId);
    MEDIA_LOG_D("CopyFrameToUserQueue exit, track:" PUBLIC_LOG_U32, trackId);
    return ret;
}

Status MediaDemuxer::InnerReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    MEDIA_LOG_D("copy frame for track " PUBLIC_LOG_U32, trackId);

    int32_t innerTrackID = static_cast<int32_t>(trackId);
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    if (demuxerPluginManager_->IsDash() || demuxerPluginManager_->IsSubtitle()) {
        pluginTemp = demuxerPluginManager_->SelectPlugin(trackId);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "InnerReadSample failed due to get demuxer plugin failed.");
        innerTrackID = demuxerPluginManager_->GetInnerTrackID(trackId);
    } else {
        pluginTemp = demuxerPluginManager_->GetCurVideoPlugin();
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "InnerReadSample failed due to get demuxer plugin failed.");
    }

    Status ret = pluginTemp->ReadSample(innerTrackID, sample);
    if (ret == Status::END_OF_STREAM) {
        MEDIA_LOG_I("Read buffer eos for track " PUBLIC_LOG_U32, trackId);
    } else if (ret != Status::OK) {
        MEDIA_LOG_I("Read buffer error for track " PUBLIC_LOG_U32 ", ret: " PUBLIC_LOG_D32, trackId, (int32_t)(ret));
    }
    MEDIA_LOG_D("finish copy frame for track " PUBLIC_LOG_U32, trackId);

    // to get DrmInfo
    ProcessDrmInfos();
    ProcessVideoStartTime(trackId, sample);
    return ret;
}

int64_t MediaDemuxer::ReadLoop(uint32_t trackId)
{
    if (streamDemuxer_->GetIsIgnoreParse() || isStopped_ || isPaused_) {
        MEDIA_LOG_D("ReadLoop pausing, copy frame for track " PUBLIC_LOG_U32, trackId);
        return 6 * 1000; // sleep 6ms in pausing to avoid useless reading
    } else {
        Status ret = CopyFrameToUserQueue(trackId);
        // when read failed, or request always failed in 1min, send error event
        if ((ret == Status::ERROR_UNKNOWN && !isStopped_ && !isPaused_) ||
             requestBufferErrorCountMap_[trackId] >= REQUEST_FAILED_RETRY_TIMES) {
            MEDIA_LOG_E("Data source is invalid, can not get frame");
            if (eventReceiver_ != nullptr) {
                eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_DATA_SOURCE_ERROR_UNKNOWN});
            } else {
                MEDIA_LOG_D("OnEvent eventReceiver_ null.");
            }
        }
        if (ret == Status::OK && doPrepareFrame_) {
            AutoLock lock(firstFrameMutex_);
            firstFrameCount_++;
            firstFrameCond_.NotifyAll();
            taskMap_[trackId]->Pause();
        }
        if (ret == Status::OK || ret == Status::ERROR_AGAIN) {
            return 0; // retry next frame
        } else {
            MEDIA_LOG_D("ReadLoop wait, track:" PUBLIC_LOG_U32 ", ret:" PUBLIC_LOG_D32,
                trackId, static_cast<int32_t>(ret));
            return RETRY_DELAY_TIME_US; // delay to retry if no frame
        }
    }
}

bool MediaDemuxer::IsContainIdrFrame(const uint8_t* buff, size_t bufSize)
{
    if (buff == nullptr) {
        return false;
    }
    std::shared_ptr<FrameDetector> frameDetector;
    if (videoMime_ == std::string(MimeType::VIDEO_AVC)) {
        frameDetector = FrameDetector::GetFrameDetector(CodeType::H264);
    } else if (videoMime_ == std::string(MimeType::VIDEO_HEVC)) {
        frameDetector = FrameDetector::GetFrameDetector(CodeType::H265);
    } else {
        return true;
    }
    if (frameDetector == nullptr) {
        return true;
    }
    return frameDetector->IsContainIdrFrame(buff, bufSize);
}

Status MediaDemuxer::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE,
        "Cannot call read frame when use buffer queue.");
    MEDIA_LOG_D("Read next sample");
    FALSE_RETURN_V_MSG_E(eosMap_.count(trackId) > 0, Status::ERROR_INVALID_OPERATION,
        "Read sample failed due to track has not been selected");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_!=nullptr, Status::ERROR_INVALID_PARAMETER,
        "Read Sample failed due to input sample is nullptr");
    if (eosMap_[trackId]) {
        MEDIA_LOG_W("Read sample failed due to track" PUBLIC_LOG_U32 "has reached eos", trackId);
        sample->flag_ = (uint32_t)(AVBufferFlag::EOS);
        sample->memory_->SetSize(0);
        return Status::END_OF_STREAM;
    }
    Status ret = InnerReadSample(trackId, sample);
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        if (sample->flag_ & (uint32_t)(AVBufferFlag::EOS)) {
            eosMap_[trackId] = true;
            sample->memory_->SetSize(0);
        }
        if (sample->flag_ & (uint32_t)(AVBufferFlag::PARTIAL_FRAME)) {
            ret = Status::ERROR_NO_MEMORY;
        }
    }
    return ret;
}

void MediaDemuxer::HandleSourceDrmInfoEvent(const std::multimap<std::string, std::vector<uint8_t>> &info)
{
    MEDIA_LOG_I("HandleSourceDrmInfoEvent");
    bool isUpdated = IsDrmInfosUpdate(info);
    if (isUpdated) {
        ReportDrmInfos(info);
    }
    MEDIA_LOG_D("demuxer filter received source drminfos but not update");
}

void MediaDemuxer::OnEvent(const Plugins::PluginEvent &event)
{
    MEDIA_LOG_D("OnEvent");
    if (eventReceiver_ == nullptr && event.type != PluginEventType::SOURCE_DRM_INFO_UPDATE) {
        MEDIA_LOG_D("OnEvent source eventReceiver_ null.");
        return;
    }
    switch (event.type) {
        case PluginEventType::SOURCE_DRM_INFO_UPDATE: {
            MEDIA_LOG_D("OnEvent source drmInfo update");
            HandleSourceDrmInfoEvent(AnyCast<std::multimap<std::string, std::vector<uint8_t>>>(event.param));
            break;
        }
        case PluginEventType::CLIENT_ERROR:
        case PluginEventType::SERVER_ERROR: {
            MEDIA_LOG_E("error code " PUBLIC_LOG_D32, MSERR_EXT_IO);
            eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_DATA_SOURCE_IO_ERROR});
            break;
        }
        case PluginEventType::BUFFERING_END: {
            MEDIA_LOG_D("OnEvent pause");
            eventReceiver_->OnEvent({"demuxer_filter", EventType::BUFFERING_END, PAUSE});
            break;
        }
        case PluginEventType::BUFFERING_START: {
            MEDIA_LOG_D("OnEvent start");
            eventReceiver_->OnEvent({"demuxer_filter", EventType::BUFFERING_START, START});
            break;
        }
        case PluginEventType::SOURCE_BITRATE_START: {
            MEDIA_LOG_D("OnEvent source bitrate start");
            eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_SOURCE_BITRATE_START, event.param});
            break;
        }
        default:
            break;
    }
}

Status MediaDemuxer::OptimizeDecodeSlow(bool isDecodeOptimizationEnabled)
{
    MEDIA_LOG_I("OptimizeDecodeSlow entered.");
    isDecodeOptimizationEnabled_ = isDecodeOptimizationEnabled;
    return Status::OK;
}

Status MediaDemuxer::SetDecoderFramerateUpperLimit(int32_t decoderFramerateUpperLimit,
    uint32_t trackId)
{
    MEDIA_LOG_I("decoderFramerateUpperLimit = " PUBLIC_LOG_D32 " trackId = " PUBLIC_LOG_D32,
        decoderFramerateUpperLimit, trackId);
    FALSE_RETURN_V(trackId == videoTrackId_, Status::OK);
    FALSE_RETURN_V_MSG_E(decoderFramerateUpperLimit > 0, Status::ERROR_INVALID_PARAMETER,
        "SetDecoderFramerateUpperLimit failed, decoderFramerateUpperLimit <= 0");
    decoderFramerateUpperLimit_.store(decoderFramerateUpperLimit);
    return Status::OK;
}

Status MediaDemuxer::SetSpeed(float speed)
{
    MEDIA_LOG_I("speed = " PUBLIC_LOG_F, speed);
    FALSE_RETURN_V_MSG_E(speed > 0, Status::ERROR_INVALID_PARAMETER,
        "SetSpeed failed, speed <= 0");
    speed_.store(speed);
    return Status::OK;
}

Status MediaDemuxer::SetFrameRate(double framerate, uint32_t trackId)
{
    MEDIA_LOG_I("framerate = " PUBLIC_LOG_F " trackId = " PUBLIC_LOG_D32,
        framerate, trackId);
    FALSE_RETURN_V(trackId == videoTrackId_, Status::OK);
    FALSE_RETURN_V_MSG_E(framerate > 0, Status::ERROR_INVALID_PARAMETER,
        "SetFrameRate failed, framerate <= 0");
    framerate_.store(framerate);
    return Status::OK;
}

bool MediaDemuxer::IsBufferDroppable(std::shared_ptr<AVBuffer> sample, uint32_t trackId)
{
    DumpBufferToFile(trackId, sample);

    if (trackId != videoTrackId_) {
        return false;
    }

    if (!isDecodeOptimizationEnabled_.load()) {
        return false;
    }

    double targetRate = framerate_.load() * speed_.load();
    double actualRate = decoderFramerateUpperLimit_.load() * (1 + DECODE_RATE_THRESHOLD);
    if (targetRate <= actualRate) {
        return false;
    }

    bool canDrop = false;
    bool ret = sample->meta_->GetData(Media::Tag::VIDEO_BUFFER_CAN_DROP, canDrop);
    if (!ret || !canDrop) {
        return false;
    }

    MEDIA_LOG_D("drop buffer, framerate = " PUBLIC_LOG_F " speed = " PUBLIC_LOG_F " decodeUpLimit = "
        PUBLIC_LOG_D32 " pts = " PUBLIC_LOG_U64, framerate_.load(), speed_.load(),
        decoderFramerateUpperLimit_.load(), sample->pts_);
    return true;
}

Status MediaDemuxer::DisableMediaTrack(Plugins::MediaType mediaType)
{
    disabledMediaTracks_.emplace(mediaType);
    return Status::OK;
}
 
bool MediaDemuxer::IsTrackDisabled(Plugins::MediaType mediaType)
{
    return !disabledMediaTracks_.empty() && disabledMediaTracks_.find(mediaType) != disabledMediaTracks_.end();
}

void MediaDemuxer::SetSelectBitRateFlag(bool flag)
{
    MEDIA_LOG_I("SetSelectBitRateFlag = " PUBLIC_LOG_D32, static_cast<int32_t>(flag));
    isSelectBitRate_.store(flag);
}

bool MediaDemuxer::CanDoSelectBitRate()
{
    // calculating auto selectbitrate time
    return !(isSelectBitRate_.load());
}
} // namespace Media
} // namespace OHOS
