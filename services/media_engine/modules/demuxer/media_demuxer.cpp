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
#include <map>
#include <memory>

#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "cpp_ext/type_traits_ext.h"
#include "buffer/avallocator.h"
#include "common/event.h"
#include "format.h"
#include "common/log.h"
#include "hisysevent.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_time.h"
#include "source/source.h"
#include "stream_demuxer.h"
#include "media_core.h"
#include "osal/utils/dump_buffer.h"
#include "demuxer_plugin_manager.h"
#include "media_demuxer_pts_founctions.cpp"

namespace {
const std::string DUMP_PARAM = "a";
const std::string DUMP_DEMUXER_AUDIO_FILE_NAME = "player_demuxer_audio_output.es";
const std::string DUMP_DEMUXER_VIDEO_FILE_NAME = "player_demuxer_video_output.es";
static constexpr char PERFORMANCE_STATS[] = "PERFORMANCE";
static constexpr int32_t INVALID_TRACK_ID = -1;
constexpr uint32_t THREAD_PRIORITY_41 = 7;
std::map<OHOS::Media::TrackType, OHOS::Media::StreamType> TRACK_TO_STREAM_MAP = {
    {OHOS::Media::TrackType::TRACK_VIDEO, OHOS::Media::StreamType::VIDEO},
    {OHOS::Media::TrackType::TRACK_AUDIO, OHOS::Media::StreamType::AUDIO},
    {OHOS::Media::TrackType::TRACK_SUBTITLE, OHOS::Media::StreamType::SUBTITLE},
    {OHOS::Media::TrackType::TRACK_INVALID, OHOS::Media::StreamType::MIXED}
};
} // namespace

namespace OHOS {
namespace Media {
namespace {
constexpr uint32_t REQUEST_BUFFER_TIMEOUT = 0; // Requesting buffer overtimes 0ms means no retry
constexpr int32_t START = 1;
constexpr int32_t PAUSE = 2;
constexpr int32_t SEEK_TO_EOS = 1;
constexpr uint32_t RETRY_DELAY_TIME_US = 100000; // 100ms, Delay time for RETRY if no buffer in avbufferqueue producer.
constexpr double DECODE_RATE_THRESHOLD = 0.05;   // allow actual rate exceeding 5%
constexpr uint32_t REQUEST_FAILED_RETRY_TIMES = 12000; // Max times for RETRY if no buffer in avbufferqueue producer.
constexpr int32_t DEFAULT_MULTI_VIDEO_TRACK_NUM = 5;
}

enum SceneCode : int32_t {
    /**
     * This option is used to mark parser ref for dragging play scene.
     */
    AV_META_SCENE_PARSE_REF_FOR_DRAGGING_PLAY = 3 // scene code of parser ref for dragging play is 3
};

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
        MEDIA_LOG_D("Buffer available for track " PUBLIC_LOG_U32, trackId_);
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
        MEDIA_LOG_D("TrackId:" PUBLIC_LOG_U32 ", isNotifyNeeded:" PUBLIC_LOG_D32,
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
      subSeekable_(Plugins::Seekable::INVALID),
      uri_(),
      mediaDataSize_(0),
      subMediaDataSize_(0),
      source_(std::make_shared<Source>()),
      subtitleSource_(std::make_shared<Source>()),
      mediaMetaData_(),
      bufferQueueMap_(),
      bufferMap_(),
      eventReceiver_(),
      streamDemuxer_(),
      demuxerPluginManager_(std::make_shared<DemuxerPluginManager>())
{
    MEDIA_LOG_D("In");
}

MediaDemuxer::~MediaDemuxer()
{
    MEDIA_LOG_D("In");
    ResetInner();
    demuxerPluginManager_ = nullptr;
    mediaSource_ = nullptr;
    source_ = nullptr;
    eventReceiver_ = nullptr;
    eosMap_.clear();
    requestBufferErrorCountMap_.clear();
    streamDemuxer_ = nullptr;
    localDrmInfos_.clear();

    if (parserRefInfoTask_ != nullptr) {
        parserRefInfoTask_->Stop();
        parserRefInfoTask_ = nullptr;
    }
}

std::shared_ptr<Plugins::DemuxerPlugin> MediaDemuxer::GetCurFFmpegPlugin()
{
    int32_t tempTrackId = (videoTrackId_ != TRACK_ID_DUMMY ? static_cast<int32_t>(videoTrackId_) : -1);
    tempTrackId = (tempTrackId == -1 ? static_cast<int32_t>(audioTrackId_) : tempTrackId);
    int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(tempTrackId);
    return demuxerPluginManager_->GetPluginByStreamID(streamID);
}

static void ReportSceneCodeForDemuxer(SceneCode scene)
{
    if (scene != SceneCode::AV_META_SCENE_PARSE_REF_FOR_DRAGGING_PLAY) {
        return;
    }
    MEDIA_LOG_I("Scene %{public}d", scene);
    auto now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    int32_t ret = HiSysEventWrite(
        PERFORMANCE_STATS, "CPU_SCENE_ENTRY", OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR, "PACKAGE_NAME",
        "media_service", "SCENE_ID", std::to_string(scene).c_str(), "HAPPEN_TIME", now.count());
    if (ret != MSERR_OK) {
        MEDIA_LOG_W("Report failed");
    }
}

bool MediaDemuxer::IsRefParserSupported()
{
    FALSE_RETURN_V_MSG_E(videoTrackId_ != TRACK_ID_DUMMY, false, "Video track is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> videoPlugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(videoPlugin != nullptr, false, "Demuxer plugin is nullptr");
    return videoPlugin->IsRefParserSupported();
}

Status MediaDemuxer::StartReferenceParser(int64_t startTimeMs, bool isForward)
{
    FALSE_RETURN_V_MSG_E(startTimeMs >= 0, Status::ERROR_UNKNOWN,
                         "Start failed, startTimeMs: " PUBLIC_LOG_D64, startTimeMs);
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(videoTrackId_ != TRACK_ID_DUMMY, Status::ERROR_UNKNOWN, "Video track is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> videoPlugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(videoPlugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    Status ret = videoPlugin->ParserRefUpdatePos(startTimeMs, isForward);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ParserRefUpdatePos failed");
    if (isFirstParser_) {
        isFirstParser_ = false;
        if (source_->GetSeekable() != Plugins::Seekable::SEEKABLE) {
            MEDIA_LOG_E("Unsupport online video");
            return Status::ERROR_INVALID_OPERATION;
        }
        parserRefInfoTask_ = std::make_unique<Task>("ParserRefInfo", playerId_);
        parserRefInfoTask_->RegisterJob([this] { return ParserRefInfo(); });
        ReportSceneCodeForDemuxer(SceneCode::AV_META_SCENE_PARSE_REF_FOR_DRAGGING_PLAY);
        parserRefInfoTask_->Start();
    }
    TryRecvParserTask();
    return ret;
}

void MediaDemuxer::TryRecvParserTask()
{
    if (isParserTaskEnd_ && parserRefInfoTask_ != nullptr) {
        parserRefInfoTask_->Stop();
        parserRefInfoTask_ = nullptr;
        MEDIA_LOG_I("Success to recovery");
    }
}

int64_t MediaDemuxer::ParserRefInfo()
{
    if (demuxerPluginManager_ == nullptr) {
        MEDIA_LOG_D("Plugin manager is nullptr");
        return 0;
    }
    std::shared_ptr<Plugins::DemuxerPlugin> videoPlugin = GetCurFFmpegPlugin();
    if (videoPlugin == nullptr) {
        MEDIA_LOG_D("Demuxer plugin is nullptr");
        return 0;
    }
    Status ret = videoPlugin->ParserRefInfo();
    if ((ret == Status::OK || ret == Status::ERROR_UNKNOWN) && parserRefInfoTask_ != nullptr) {
        parserRefInfoTask_->Stop();
        isParserTaskEnd_ = true;
        MEDIA_LOG_I("Success to stop");
    } else {
        MEDIA_LOG_I("ret is " PUBLIC_LOG_D32, ret);
    }
    return 0;
}

Status MediaDemuxer::GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_E(videoSample != nullptr, Status::ERROR_NULL_POINTER, "videoSample is nullptr");
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> videoPlugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(videoPlugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryRecvParserTask();
    Status ret = videoPlugin->GetFrameLayerInfo(videoSample, frameLayerInfo);
    if (ret == Status::ERROR_NULL_POINTER && parserRefInfoTask_ != nullptr) {
        return Status::ERROR_AGAIN;
    }
    return ret;
}

Status MediaDemuxer::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> videoPlugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(videoPlugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryRecvParserTask();
    Status ret = videoPlugin->GetFrameLayerInfo(frameId, frameLayerInfo);
    if (ret == Status::ERROR_NULL_POINTER && parserRefInfoTask_ != nullptr) {
        return Status::ERROR_AGAIN;
    }
    return ret;
}

Status MediaDemuxer::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> videoPlugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(videoPlugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryRecvParserTask();
    Status ret = videoPlugin->GetGopLayerInfo(gopId, gopLayerInfo);
    if (ret == Status::ERROR_NULL_POINTER && parserRefInfoTask_ != nullptr) {
        return Status::ERROR_AGAIN;
    }
    return ret;
}

void MediaDemuxer::RegisterVideoStreamReadyCallback(const std::shared_ptr<VideoStreamReadyCallback> &callback)
{
    std::unique_lock<std::mutex> draggingLock(draggingMutex_);
    MEDIA_LOG_I("In");
    VideoStreamReadyCallback_ = callback;
}

void MediaDemuxer::DeregisterVideoStreamReadyCallback()
{
    std::unique_lock<std::mutex> draggingLock(draggingMutex_);
    MEDIA_LOG_I("In");
    VideoStreamReadyCallback_ = nullptr;
}

Status MediaDemuxer::GetIFramePos(std::vector<uint32_t> &IFramePos)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> videoPlugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(videoPlugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryRecvParserTask();
    return videoPlugin->GetIFramePos(IFramePos);
}

Status MediaDemuxer::Dts2FrameId(int64_t dts, uint32_t &frameId, bool offset)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> videoPlugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(videoPlugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryRecvParserTask();
    return videoPlugin->Dts2FrameId(dts, frameId, offset);
}

void MediaDemuxer::OnBufferAvailable(uint32_t trackId)
{
    MEDIA_LOG_D("Buffer available track " PUBLIC_LOG_U32, trackId);
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
    MEDIA_LOG_D("Accelerate track " PUBLIC_LOG_U32, trackId);
    task->second->UpdateDelayTime();
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
        MEDIA_LOG_E("Source is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return source_->GetBitRates(bitRates);
}

Status MediaDemuxer::GetMediaKeySystemInfo(std::multimap<std::string, std::vector<uint8_t>> &infos)
{
    MEDIA_LOG_I("In");
    std::shared_lock<std::shared_mutex> lock(drmMutex);
    infos = localDrmInfos_;
    return Status::OK;
}

Status MediaDemuxer::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    if (source_ == nullptr) {
        MEDIA_LOG_E("Source is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return source_->GetDownloadInfo(downloadInfo);
}

Status MediaDemuxer::GetPlaybackInfo(PlaybackInfo& playbackInfo)
{
    FALSE_RETURN_V_MSG(source_ != nullptr, Status::ERROR_INVALID_OPERATION, "Source is nullptr");
    return source_->GetPlaybackInfo(playbackInfo);
}

void MediaDemuxer::SetDrmCallback(const std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> &callback)
{
    MEDIA_LOG_I("In");
    drmCallback_ = callback;
    bool isExisted = IsLocalDrmInfosExisted();
    if (isExisted) {
        MEDIA_LOG_D("Already received drminfo and report");
        ReportDrmInfos(localDrmInfos_);
    }
}

void MediaDemuxer::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver)
{
    eventReceiver_ = receiver;
}

void MediaDemuxer::SetPlayerId(const std::string &playerId)
{
    playerId_ = playerId;
}

void MediaDemuxer::SetDumpInfo(bool isDump, uint64_t instanceId)
{
    if (isDump && instanceId == 0) {
        MEDIA_LOG_W("Can not dump with instanceId 0");
        return;
    }
    dumpPrefix_ = std::to_string(instanceId);
    isDump_ = isDump;
}

bool MediaDemuxer::GetDuration(int64_t& durationMs)
{
    AutoLock lock(mapMutex_);
    if (source_ == nullptr) {
        durationMs = -1;
        return false;
    }
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::GetDuration");
    seekable_ = source_->GetSeekable();

    FALSE_LOG(seekable_ != Seekable::INVALID);
    if (source_->IsSeekToTimeSupported()) {
        duration_ = source_->GetDuration();
        if (duration_ != Plugins::HST_TIME_NONE) {
            MEDIA_LOG_I("Hls: " PUBLIC_LOG_D64, duration_);
            mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(Plugins::HstTime2Us(duration_));
        }
        MEDIA_LOG_I("SeekToTime");
        return mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs);
    }

    // not hls and seekable
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        duration_ = source_->GetDuration();
        if (duration_ != Plugins::HST_TIME_NONE) {
            MEDIA_LOG_I("Not hls: " PUBLIC_LOG_D64, duration_);
            mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(Plugins::HstTime2Us(duration_));
        }
        MEDIA_LOG_I("Seekble");
        return mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs);
    }
    MEDIA_LOG_I("Other");
    return mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs);
}

bool MediaDemuxer::GetDrmInfosUpdated(const std::multimap<std::string, std::vector<uint8_t>> &newInfos,
    std::multimap<std::string, std::vector<uint8_t>> &result)
{
    MEDIA_LOG_D("In");
    std::unique_lock<std::shared_mutex> lock(drmMutex);
    for (auto &newItem : newInfos) {
        if (localDrmInfos_.find(newItem.first) == localDrmInfos_.end()) {
            MEDIA_LOG_D("Uuid doesn't exist, update");
            result.insert(newItem);
            localDrmInfos_.insert(newItem);
            continue;
        }
        auto pos = localDrmInfos_.equal_range(newItem.first);
        bool isSame = false;
        for (; pos.first != pos.second; ++pos.first) {
            if (newItem.second == pos.first->second) {
                MEDIA_LOG_D("Uuid exists and the pssh is same, not update");
                isSame = true;
                break;
            }
        }
        if (!isSame) {
            MEDIA_LOG_D("Uuid exists but pssh not same, update");
            result.insert(newItem);
            localDrmInfos_.insert(newItem);
        }
    }
    return !result.empty();
}

bool MediaDemuxer::IsLocalDrmInfosExisted()
{
    std::shared_lock<std::shared_mutex> lock(drmMutex);
    return !localDrmInfos_.empty();
}

Status MediaDemuxer::ReportDrmInfos(const std::multimap<std::string, std::vector<uint8_t>> &info)
{
    MEDIA_LOG_I("In");
    FALSE_RETURN_V_MSG_E(drmCallback_ != nullptr, Status::ERROR_INVALID_PARAMETER, "Drm callback is nullptr");
    MEDIA_LOG_D("Filter will update drminfo");
    drmCallback_->OnDrmInfoChanged(info);
    return Status::OK;
}

Status MediaDemuxer::ProcessDrmInfos()
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");

    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    Status ret = pluginTemp->GetDrmInfo(drmInfo);
    if (ret == Status::OK && !drmInfo.empty()) {
        MEDIA_LOG_D("Get drminfo success");
        std::multimap<std::string, std::vector<uint8_t>> infosUpdated;
        bool isUpdated = GetDrmInfosUpdated(drmInfo, infosUpdated);
        if (isUpdated) {
            return ReportDrmInfos(infosUpdated);
        } else {
            MEDIA_LOG_D("Received drminfo but not update");
        }
    } else {
        if (ret != Status::OK) {
            MEDIA_LOG_D("Get drminfo failed, ret:" PUBLIC_LOG_D32, (int32_t)(ret));
        }
    }
    return Status::OK;
}

Status MediaDemuxer::AddDemuxerCopyTask(uint32_t trackId, TaskType type)
{
    std::string taskName = "Demux";
    switch (type) {
        case TaskType::VIDEO: {
            taskName += "V";
            break;
        }
        case TaskType::AUDIO: {
            taskName += "A";
            break;
        }
        case TaskType::SUBTITLE: {
            taskName += "S";
            break;
        }
        default: {
            MEDIA_LOG_E("Add demuxer task failed, unknow task type:" PUBLIC_LOG_D32, type);
            return Status::ERROR_UNKNOWN;
        }
    }

    std::unique_ptr<Task> task = std::make_unique<Task>(taskName, playerId_, type);
<<<<<<< HEAD
    if (task == nullptr) {
        MEDIA_LOG_W("Create task failed, track:" PUBLIC_LOG_U32 ", type:" PUBLIC_LOG_D32,
            trackId, type);
        return Status::OK;
    }
=======
    FALSE_RETURN_V_MSG_W(task != nullptr, Status::OK,
        "Create task failed, track:" PUBLIC_LOG_U32 ", type:" PUBLIC_LOG_D32,
        trackId, type);
#ifdef SUPPORT_START_STOP_ON_DEMAND
    task->UpdateThreadPriority(THREAD_PRIORITY_41, "media_service");
#endif
>>>>>>> 1ce3ac4ca (update thread priority)
    taskMap_[trackId] = std::move(task);
    taskMap_[trackId]->RegisterJob([this, trackId] { return ReadLoop(trackId); });

    // To wake up DEMUXER TRACK WORKING TASK immediately on input buffer available.
    std::unique_ptr<Task> notifyTask =
        std::make_unique<Task>(taskName + "N", playerId_, type, TaskPriority::NORMAL, false);
    if (!notifyTask) {
        MEDIA_LOG_W("Create notify task failed, track:" PUBLIC_LOG_U32 ", type:" PUBLIC_LOG_D32,
            trackId, static_cast<uint32_t>(type));
        return Status::OK;
    }

    sptr<IProducerListener> listener =
        OHOS::sptr<AVBufferQueueProducerListener>::MakeSptr(trackId, shared_from_this(), notifyTask);
    if (listener == nullptr) {
        MEDIA_LOG_W("Create listener failed, track:" PUBLIC_LOG_U32 ", type:" PUBLIC_LOG_D32,
            trackId, static_cast<uint32_t>(type));
        return Status::OK;
    }

    trackMap_.emplace(trackId, std::make_shared<TrackWrapper>(trackId, listener, shared_from_this()));
    return Status::OK;
}

Status MediaDemuxer::InnerPrepare()
{
    Plugins::MediaInfo mediaInfo;
    Status ret = demuxerPluginManager_->LoadCurrentAllPlugin(streamDemuxer_, mediaInfo);
    if (ret == Status::OK) {
        InitMediaMetaData(mediaInfo);
        InitDefaultTrack(mediaInfo, videoTrackId_, audioTrackId_, subtitleTrackId_, videoMime_);
        if (videoTrackId_ != TRACK_ID_DUMMY) {
            if (isEnableReselectVideoTrack_ && IsHasMultiVideoTrack()) {
                videoTrackId_ = GetTargetVideoTrackId(mediaMetaData_.trackMetas);
            }
            AddDemuxerCopyTask(videoTrackId_, TaskType::VIDEO);
            demuxerPluginManager_->UpdateTempTrackMapInfo(videoTrackId_, videoTrackId_, -1);
            int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(videoTrackId_);
            streamDemuxer_->SetNewVideoStreamID(streamId);
            streamDemuxer_->SetChangeFlag(true);
            streamDemuxer_->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
            int64_t bitRate = 0;
            mediaMetaData_.trackMetas[videoTrackId_]->GetData(Tag::MEDIA_BITRATE, bitRate);
            source_->SetCurrentBitRate(bitRate, streamId);
            targetBitRate_ = demuxerPluginManager_->GetCurrentBitRate();
        }
        if (audioTrackId_ != TRACK_ID_DUMMY) {
            AddDemuxerCopyTask(audioTrackId_, TaskType::AUDIO);
            demuxerPluginManager_->UpdateTempTrackMapInfo(audioTrackId_, audioTrackId_, -1);
            int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(audioTrackId_);
            streamDemuxer_->SetNewAudioStreamID(streamId);
            streamDemuxer_->SetChangeFlag(true);
            streamDemuxer_->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
            int64_t bitRate = 0;
            mediaMetaData_.trackMetas[audioTrackId_]->GetData(Tag::MEDIA_BITRATE, bitRate);
            source_->SetCurrentBitRate(bitRate, streamId);
        }
        if (subtitleTrackId_ != TRACK_ID_DUMMY) {
            AddDemuxerCopyTask(subtitleTrackId_, TaskType::SUBTITLE);
            demuxerPluginManager_->UpdateTempTrackMapInfo(subtitleTrackId_, subtitleTrackId_, -1);
            int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(subtitleTrackId_);
            streamDemuxer_->SetNewSubtitleStreamID(streamId);
            streamDemuxer_->SetChangeFlag(true);
            streamDemuxer_->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
        }
    } else {
        MEDIA_LOG_E("Parse meta failed, ret: " PUBLIC_LOG_D32, (int32_t)(ret));
    }
    return ret;
}

uint32_t MediaDemuxer::GetTargetVideoTrackId(std::vector<std::shared_ptr<Meta>> trackInfos)
{
    FALSE_RETURN_V(targetVideoTrackId_ == TRACK_ID_DUMMY, targetVideoTrackId_);
    MEDIA_LOG_I_SHORT("GetTargetVideoTrackId enter");
    int64_t videoRes = 0;
    int32_t videoWidth = 0;
    int32_t videoHeight = 0;
    for (size_t index = 0; index < trackInfos.size(); index++) {
        std::shared_ptr<Meta> meta = trackInfos[index];
        if (meta == nullptr) {
            MEDIA_LOG_E_SHORT("meta is invalid, index: %zu", index);
            continue;
        }
        Plugins::MediaType mediaType = Plugins::MediaType::AUDIO;
        if (!meta->GetData(Tag::MEDIA_TYPE, mediaType)) {
            continue;
        }
        if (mediaType != Plugins::MediaType::VIDEO) {
            continue;
        }
        if (!meta->GetData(Tag::VIDEO_WIDTH, videoWidth)) {
            continue;
        }
        if (!meta->GetData(Tag::VIDEO_HEIGHT, videoHeight)) {
            continue;
        }
        MEDIA_LOG_I_SHORT("SelectVideoTrack trackId: %{public}d width: %{public}d height: %{public}d",
            static_cast<int32_t>(index), videoWidth, videoHeight);
        int64_t resolution = static_cast<int64_t>(videoWidth) * static_cast<int64_t>(videoHeight);
        if (resolution > videoRes) {
            videoRes = resolution;
            targetVideoTrackId_ = static_cast<uint32_t>(index);
        }
    }
    return targetVideoTrackId_;
}

Status MediaDemuxer::SetDataSource(const std::shared_ptr<MediaSource> &source)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_I("In");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running");
    source_->SetCallback(this);
    auto res = source_->SetSource(source);
    FALSE_RETURN_V_MSG_E(res == Status::OK, res, "Plugin set source failed");

    std::vector<StreamInfo> streams;
    source_->GetStreamInfo(streams);
    demuxerPluginManager_->InitDefaultPlay(streams);

    streamDemuxer_ = std::make_shared<StreamDemuxer>();
    streamDemuxer_->SetSourceType(source->GetSourceType());
    streamDemuxer_->SetInterruptState(isInterruptNeeded_);
    streamDemuxer_->SetSource(source_);
    streamDemuxer_->Init(uri_);
    streamDemuxer_->SetIsExtSubtitle(false);

    res = InnerPrepare();

    ProcessDrmInfos();
    MEDIA_LOG_I("Out");
    return res;
}

bool MediaDemuxer::IsSubtitleMime(const std::string& mime)
{
    if (mime == "application/x-subrip" || mime == "text/vtt") {
        return true;
    }
    return false;
}

Status MediaDemuxer::SetSubtitleSource(const std::shared_ptr<MediaSource> &subSource)
{
    MEDIA_LOG_I("In");
    if (subtitleTrackId_ != TRACK_ID_DUMMY) {
        MEDIA_LOG_W("Found subtitle track, not support ext");
        return Status::OK;
    }
    subtitleSource_->SetCallback(this);
    subtitleSource_->SetSource(subSource);
    Status ret = subtitleSource_->GetSize(subMediaDataSize_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Get file size failed");
    subSeekable_ = subtitleSource_->GetSeekable();

    int32_t subtitleStreamID = demuxerPluginManager_->AddExternalSubtitle();
    subStreamDemuxer_ = std::make_shared<StreamDemuxer>();
    subStreamDemuxer_->SetSourceType(subSource->GetSourceType());
    subStreamDemuxer_->SetInterruptState(isInterruptNeeded_);
    subStreamDemuxer_->SetSource(subtitleSource_);
    subStreamDemuxer_->Init(subSource->GetSourceUri());
    subStreamDemuxer_->SetIsExtSubtitle(true);

    MediaInfo mediaInfo;
    ret = demuxerPluginManager_->LoadCurrentSubtitlePlugin(subStreamDemuxer_, mediaInfo);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse meta failed, ret:" PUBLIC_LOG_D32, (int32_t)(ret));
    InitMediaMetaData(mediaInfo);  // update mediaMetaData_
    for (uint32_t index = 0; index < mediaInfo.tracks.size(); index++) {
        auto trackMeta = mediaInfo.tracks[index];
        std::string mimeType;
        std::string trackType;
        trackMeta.Get<Tag::MIME_TYPE>(mimeType);
        int32_t curStreamID = demuxerPluginManager_->GetStreamIDByTrackID(index);
        if (trackMeta.Get<Tag::MIME_TYPE>(mimeType) && IsSubtitleMime(mimeType) && curStreamID == subtitleStreamID) {
            MEDIA_LOG_I("STrack " PUBLIC_LOG_U32, index);
            subtitleTrackId_ = index;   // dash inner subtitle can be replace by out subtitle
            break;
        }
    }

    if (subtitleTrackId_ != TRACK_ID_DUMMY) {
        AddDemuxerCopyTask(subtitleTrackId_, TaskType::SUBTITLE);
        demuxerPluginManager_->UpdateTempTrackMapInfo(subtitleTrackId_, subtitleTrackId_, -1);
        subStreamDemuxer_->SetNewSubtitleStreamID(subtitleStreamID);
        subStreamDemuxer_->SetDemuxerState(subtitleStreamID, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
    }

    MEDIA_LOG_I("Out, ext sub %{public}d", subtitleTrackId_);
    return ret;
}

void MediaDemuxer::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
    if (demuxerPluginManager_ != nullptr && demuxerPluginManager_->IsDash()) {
        std::unique_lock<std::mutex> lock(rebootPluginMutex_);
        rebootPluginCondition_.notify_all();
    }
    if (source_ != nullptr) {
        source_->SetInterruptState(isInterruptNeeded);
    }
    if (streamDemuxer_ != nullptr) {
        streamDemuxer_->SetInterruptState(isInterruptNeeded);
    }
}

void MediaDemuxer::SetBundleName(const std::string& bundleName)
{
    if (source_ != nullptr) {
        MEDIA_LOG_I("BundleName: " PUBLIC_LOG_S, bundleName.c_str());
        bundleName_ = bundleName;
        streamDemuxer_->SetBundleName(bundleName);
        source_->SetBundleName(bundleName);
    }
}

Status MediaDemuxer::SetOutputBufferQueue(int32_t trackId, const sptr<AVBufferQueueProducer>& producer)
{
    AutoLock lock(mapMutex_);
    FALSE_RETURN_V_MSG_E(trackId >= 0 && (uint32_t)trackId < mediaMetaData_.trackMetas.size(),
        Status::ERROR_INVALID_PARAMETER, "Invalid track");
    useBufferQueue_ = true;
    MEDIA_LOG_I("Set bufferQueue for track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running");
    FALSE_RETURN_V_MSG_E(producer != nullptr, Status::ERROR_INVALID_PARAMETER, "BufferQueue is nullptr");
    if (bufferQueueMap_.find(trackId) != bufferQueueMap_.end()) {
        MEDIA_LOG_W("Has been set already");
    }
    Status ret = InnerSelectTrack(trackId);
    if (ret == Status::OK) {
        auto track = trackMap_.find(trackId);
        if (track != trackMap_.end()) {
            auto listener = track->second->GetProducerListener();
            if (listener) {
                MEDIA_LOG_W("Set listener");
                producer->SetBufferAvailableListener(listener);
            }
        }
        bufferQueueMap_.insert(std::pair<uint32_t, sptr<AVBufferQueueProducer>>(trackId, producer));
        bufferMap_.insert(std::pair<uint32_t, std::shared_ptr<AVBuffer>>(trackId, nullptr));
        MEDIA_LOG_I("Set bufferQueue successfully");
    }
    return ret;
}

void MediaDemuxer::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D("In");
    if (fd < 0) {
        MEDIA_LOG_E("Invalid fd");
        return;
    }
    std::string dumpString;
    dumpString += "MediaDemuxer buffer queue map size: " + std::to_string(bufferQueueMap_.size()) + "\n";
    dumpString += "MediaDemuxer buffer map size: " + std::to_string(bufferMap_.size()) + "\n";
    int ret = write(fd, dumpString.c_str(), dumpString.size());
    if (ret < 0) {
        MEDIA_LOG_E("MediaDemuxer::OnDumpInfo write failed");
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
    if (demuxerPluginManager_->IsDash() || demuxerPluginManager_->GetTmpStreamIDByTrackID(subtitleTrackId_) != -1) {
        int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamId);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        int32_t streamId = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamId);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
    }
    return pluginTemp->SelectTrack(innerTrackID);
}

Status MediaDemuxer::StartTask(int32_t trackId)
{
    MEDIA_LOG_I("In, track " PUBLIC_LOG_D32, trackId);
    std::string mimeType;
    auto iter = taskMap_.find(trackId);
    if (iter == taskMap_.end()) {
        TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
        if (trackType == TrackType::TRACK_AUDIO) {
            AddDemuxerCopyTask(trackId, TaskType::AUDIO);
        } else if (trackType == TrackType::TRACK_VIDEO) {
            AddDemuxerCopyTask(trackId, TaskType::VIDEO);
        } else if (trackType == TrackType::TRACK_SUBTITLE) {
            AddDemuxerCopyTask(trackId, TaskType::SUBTITLE);
        }
        if (taskMap_.find(trackId) != taskMap_.end() && taskMap_[trackId] != nullptr) {
            taskMap_[trackId]->Start();
        }
    } else {
        if (taskMap_[trackId] != nullptr && !taskMap_[trackId]->IsTaskRunning()) {
            taskMap_[trackId]->Start();
        }
    }
    return Status::OK;
}

Status MediaDemuxer::HandleDashSelectTrack(int32_t trackId)
{
    MEDIA_LOG_I("In, track " PUBLIC_LOG_D32, trackId);
    eosMap_[trackId] = false;
    requestBufferErrorCountMap_[trackId] = 0;

    int32_t targetStreamID = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
    if (targetStreamID == -1) {
        MEDIA_LOG_E("Get stream failed");
        return Status::ERROR_UNKNOWN;
    }

    int32_t curTrackId = -1;
    TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    if (trackType == TrackType::TRACK_AUDIO) {
        curTrackId = static_cast<int32_t>(audioTrackId_);
    } else if (trackType == TrackType::TRACK_VIDEO) {
        curTrackId = static_cast<int32_t>(videoTrackId_);
    } else if (trackType == TrackType::TRACK_SUBTITLE) {
        curTrackId = static_cast<int32_t>(subtitleTrackId_);
    } else {   // invalid
        MEDIA_LOG_E("Track type invalid");
        return Status::ERROR_UNKNOWN;
    }

    if (curTrackId == trackId || targetStreamID != demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)) {
        MEDIA_LOG_I("Select stream");
        {
            std::lock_guard<std::mutex> lock(isSelectTrackMutex_);
            inSelectTrackType_[static_cast<int32_t>(trackType)] = trackId;
        }
        return source_->SelectStream(targetStreamID);
    }

    // same streamID
    Status ret = DoSelectTrack(trackId, curTrackId);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "DoSelectTrack track failed");
    if (eventReceiver_ != nullptr) {
        MEDIA_LOG_I("HandleDashSelectTrack report track change event");
        if (trackType == TrackType::TRACK_AUDIO) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_AUDIO_TRACK_CHANGE, trackId});
            audioTrackId_ = static_cast<uint32_t>(trackId);
        } else if (trackType == TrackType::TRACK_VIDEO) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_VIDEO_TRACK_CHANGE, trackId});
            videoTrackId_ = static_cast<uint32_t>(trackId);
        } else if (trackType == TrackType::TRACK_SUBTITLE) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_SUBTITLE_TRACK_CHANGE, trackId});
            subtitleTrackId_ = static_cast<uint32_t>(trackId);
        } else {}
    }

    return Status::OK;
}

Status MediaDemuxer::DoSelectTrack(int32_t trackId, int32_t curTrackId)
{
    if (static_cast<uint32_t>(curTrackId) != TRACK_ID_DUMMY) {
        if (taskMap_.find(curTrackId) != taskMap_.end() && taskMap_[curTrackId] != nullptr) {
            taskMap_[curTrackId]->Stop();
        }
        Status ret = UnselectTrack(curTrackId);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Unselect track failed");
        bufferQueueMap_.insert(
            std::pair<uint32_t, sptr<AVBufferQueueProducer>>(trackId, bufferQueueMap_[curTrackId]));
        bufferMap_.insert(std::pair<uint32_t, std::shared_ptr<AVBuffer>>(trackId, bufferMap_[curTrackId]));
        bufferQueueMap_.erase(curTrackId);
        bufferMap_.erase(curTrackId);
        demuxerPluginManager_->UpdateTempTrackMapInfo(trackId, trackId, -1);
        return InnerSelectTrack(trackId);
    }
    return Status::ERROR_UNKNOWN;
}

Status MediaDemuxer::HandleSelectTrack(int32_t trackId)
{
    int32_t curTrackId = -1;
    TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    if (trackType == TrackType::TRACK_AUDIO) {
        MEDIA_LOG_I("Select audio [" PUBLIC_LOG_D32 "->" PUBLIC_LOG_D32 "]", audioTrackId_, trackId);
        curTrackId = static_cast<int32_t>(audioTrackId_);
    } else {    // inner subtitle and video not support
        MEDIA_LOG_W("Unsupport track " PUBLIC_LOG_D32, trackId);
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (curTrackId == trackId) {
        return Status::OK;
    }

    Status ret = DoSelectTrack(trackId, curTrackId);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "DoSelectTrack track failed");
    if (eventReceiver_ != nullptr) {
        MEDIA_LOG_I("HandleSelectTrack report track change event");
        if (trackType == TrackType::TRACK_AUDIO) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_AUDIO_TRACK_CHANGE, trackId});
            audioTrackId_ = static_cast<uint32_t>(trackId);
        } else if (trackType == TrackType::TRACK_VIDEO) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_VIDEO_TRACK_CHANGE, trackId});
            videoTrackId_ = static_cast<uint32_t>(trackId);
        } else {}
    }
    
    return ret;
}

Status MediaDemuxer::SelectTrack(int32_t trackId)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_D("Select " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(trackId >= 0 && (uint32_t)trackId < mediaMetaData_.trackMetas.size(),
        Status::ERROR_INVALID_PARAMETER, "Select track failed");
    if (!useBufferQueue_) {
        return InnerSelectTrack(trackId);
    }
    if (demuxerPluginManager_->IsDash()) {
        if (streamDemuxer_->CanDoChangeStream() == false) {
            MEDIA_LOG_W("Wrong state: selecting");
            return Status::ERROR_WRONG_STATE;
        }
        return HandleDashSelectTrack(trackId);
    }
    return HandleSelectTrack(trackId);
}

Status MediaDemuxer::UnselectTrack(int32_t trackId)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_D("Unselect " PUBLIC_LOG_D32, trackId);

    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    int32_t innerTrackID = trackId;
    if (demuxerPluginManager_->IsDash() || demuxerPluginManager_->GetTmpStreamIDByTrackID(subtitleTrackId_) != -1) {
        int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamId);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        int32_t streamID = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
        if (streamID == -1) {
            MEDIA_LOG_W("Invalid track " PUBLIC_LOG_D32, trackId);
            return Status::OK;
        }
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
    }
    demuxerPluginManager_->DeleteTempTrackMapInfo(trackId);

    return pluginTemp->UnselectTrack(innerTrackID);
}

Status MediaDemuxer::HandleRebootPlugin(int32_t trackId, bool& isRebooted)
{
    FALSE_RETURN_V(!subStreamDemuxer_ || trackId != static_cast<int32_t>(subtitleTrackId_), Status::OK);
    Status ret = Status::OK;
    if (static_cast<uint32_t>(trackId) != TRACK_ID_DUMMY) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
        MEDIA_LOG_D("TrackType " PUBLIC_LOG_D32, static_cast<int32_t>(trackType));
        FALSE_RETURN_V_MSG_E(trackType != TRACK_INVALID, Status::ERROR_INVALID_PARAMETER, "TrackType is invalid");
        StreamType streamType = TRACK_TO_STREAM_MAP[trackType];
        std::pair<int32_t, bool> seekReadyInfo;
        {
            std::unique_lock<std::mutex> lock(rebootPluginMutex_);
            if (!isInterruptNeeded_.load() &&
                seekReadyStreamInfo_.find(static_cast<int32_t>(streamType)) == seekReadyStreamInfo_.end()) {
                rebootPluginCondition_.wait(lock, [this, streamType] {
                    return isInterruptNeeded_.load() ||
                        seekReadyStreamInfo_.find(static_cast<int32_t>(streamType)) != seekReadyStreamInfo_.end();
                });
            }
            FALSE_RETURN_V(!isInterruptNeeded_.load(), Status::OK);
            seekReadyInfo = seekReadyStreamInfo_[static_cast<int32_t>(streamType)];
            seekReadyStreamInfo_.erase(static_cast<int32_t>(streamType));
        }
        if (seekReadyInfo.second == SEEK_TO_EOS || (seekReadyInfo.first >= 0 && seekReadyInfo.first != streamID)) {
            MEDIA_LOG_I("End of stream or streamID changed, isEOS: " PUBLIC_LOG_D32 ", streamId: " PUBLIC_LOG_D32,
                seekReadyInfo.second, seekReadyInfo.first);
            return Status::OK;
        }
        ret = demuxerPluginManager_->RebootPlugin(streamID, trackType, streamDemuxer_, isRebooted);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Reboot demuxer plugin failed");
        ret = InnerSelectTrack(trackId);
    }
    return ret;
}

Status MediaDemuxer::SeekToTimeAfter()
{
    FALSE_RETURN_V_NOLOG(demuxerPluginManager_ != nullptr && demuxerPluginManager_->IsDash(), Status::OK);
    MEDIA_LOG_D("Reboot plugin begin");
    Status ret = Status::OK;
    bool isDemuxerPluginRebooted = true;
    ret = HandleRebootPlugin(subtitleTrackId_, isDemuxerPluginRebooted);
    if (shouldCheckSubtitleFramePts_) {
        shouldCheckSubtitleFramePts_ = false;
    }
    FALSE_LOG_MSG_W(ret == Status::OK, "Reboot subtitle demuxer plugin failed");

    isDemuxerPluginRebooted = true;
    ret = HandleRebootPlugin(audioTrackId_, isDemuxerPluginRebooted);
    if (shouldCheckAudioFramePts_) {
        shouldCheckAudioFramePts_ = false;
    }
    FALSE_LOG_MSG_W(ret == Status::OK, "Reboot audio demuxer plugin failed");

    isDemuxerPluginRebooted = true;
    ret = HandleRebootPlugin(videoTrackId_, isDemuxerPluginRebooted);
    {
        std::unique_lock<std::mutex> lock(rebootPluginMutex_);
        seekReadyStreamInfo_.clear();
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Reboot video demuxer plugin failed");
    MEDIA_LOG_D("Reboot plugin success");
    return Status::OK;
}

Status MediaDemuxer::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    Status ret;
    isSeekError_.store(false);
    if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
        MEDIA_LOG_D("Source seek");
        if (mode == SeekMode::SEEK_CLOSEST_INNER) {
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_PREVIOUS_SYNC);
        } else {
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_CLOSEST_SYNC);
        }
        if (subStreamDemuxer_) {
            demuxerPluginManager_->localSubtitleSeekTo(seekTime);
        }
        SeekToTimeAfter();
        Plugins::Ms2HstTime(seekTime, realSeekTime);
    } else {
        MEDIA_LOG_D("Demuxer seek");
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
    if (ret != Status::OK) {
        isSeekError_.store(true);
    }
    isFirstFrameAfterSeek_.store(true);
    MEDIA_LOG_D("Out");
    return ret;
}

Status MediaDemuxer::SelectBitRate(uint32_t bitRate)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_INVALID_PARAMETER, "Source is nullptr");
    MEDIA_LOG_I("In");
    if (demuxerPluginManager_->IsDash()) {
        if (streamDemuxer_->CanDoChangeStream() == false) {
            MEDIA_LOG_W("Wrong state: selecting");
            return Status::OK;
        }
        if (bitRate == demuxerPluginManager_->GetCurrentBitRate()) {
            MEDIA_LOG_W("Same bitrate");
            return Status::OK;
        }
        isSelectBitRate_.store(true);
    } else {
        streamDemuxer_->ResetAllCache();
    }
 
    Status ret = source_->SelectBitRate(bitRate);
    if (ret != Status::OK) {
        MEDIA_LOG_E("Source select bitrate failed");
        if (demuxerPluginManager_->IsDash()) {
            isSelectBitRate_.store(false);
        }
    }
    targetBitRate_ = bitRate;
    MEDIA_LOG_I("Out");
    return ret;
}

std::vector<std::shared_ptr<Meta>> MediaDemuxer::GetStreamMetaInfo() const
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    return mediaMetaData_.trackMetas;
}

std::shared_ptr<Meta> MediaDemuxer::GetGlobalMetaInfo() const
{
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
    MEDIA_LOG_I("In");
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
        // hls reset ffmpeg eos status
        if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
            demuxerPluginManager_->SetResetEosStatus(true);
        }
        demuxerPluginManager_->Flush();
    }

    InitPtsInfo();
    return Status::OK;
}

Status MediaDemuxer::StopAllTask()
{
    MEDIA_LOG_I("In");
    isDemuxerLoopExecuting_ = false;
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
    MEDIA_LOG_I("Out");
    return Status::OK;
}

Status MediaDemuxer::PauseAllTask()
{
    MEDIA_LOG_I("In");
    isDemuxerLoopExecuting_ = false;
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
    MEDIA_LOG_I("Out");
    return Status::OK;
}

Status MediaDemuxer::ResumeAllTask()
{
    MEDIA_LOG_I("In");
    isDemuxerLoopExecuting_ = true;
    streamDemuxer_->SetIsIgnoreParse(false);

    auto it = bufferQueueMap_.begin();
    while (it != bufferQueueMap_.end()) {
        uint32_t trackId = it->first;
        if (taskMap_.find(trackId) != taskMap_.end() && taskMap_[trackId] != nullptr) {
            taskMap_[trackId]->Start();
        } else {
            MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " task is not exist", trackId);
        }
        it++;
    }
    MEDIA_LOG_I("Out");
    return Status::OK;
}

Status MediaDemuxer::Pause()
{
    MEDIA_LOG_I("In");
    isPaused_ = true;
    if (streamDemuxer_) {
        streamDemuxer_->SetIsIgnoreParse(true);
        streamDemuxer_->Pause();
    }
    if (source_) {
        source_->SetReadBlockingFlag(false); // Disable source read blocking to prevent pause all task blocking
        source_->Pause();
    }
    if (inPreroll_.load()) {
        if (CheckTrackEnabledById(videoTrackId_)) {
            taskMap_[videoTrackId_]->PauseAsync();
            taskMap_[videoTrackId_]->Pause();
        }
    } else {
        PauseAllTask();
    }
    if (source_ != nullptr) {
        source_->SetReadBlockingFlag(true); // Enable source read blocking to ensure get wanted data
    }
    return Status::OK;
}

Status MediaDemuxer::PauseDragging()
{
    MEDIA_LOG_I("PauseDragging");
    isPaused_ = true;
    if (streamDemuxer_) {
        streamDemuxer_->SetIsIgnoreParse(true);
        streamDemuxer_->Pause();
    }
    if (source_) {
        source_->SetReadBlockingFlag(false); // Disable source read blocking to prevent pause all task blocking
        source_->Pause();
    }
    if (taskMap_.find(videoTrackId_) != taskMap_.end() && taskMap_[videoTrackId_] != nullptr) {
        taskMap_[videoTrackId_]->PauseAsync();
        taskMap_[videoTrackId_]->Pause();
    }
 
    if (source_ != nullptr) {
        source_->SetReadBlockingFlag(true); // Enable source read blocking to ensure get wanted data
    }
    return Status::OK;
}

Status MediaDemuxer::PauseAudioAlign()
{
    MEDIA_LOG_I("PauseDragging");
    isPaused_ = true;
    if (streamDemuxer_) {
        streamDemuxer_->SetIsIgnoreParse(true);
        streamDemuxer_->Pause();
    }
    if (source_) {
        source_->SetReadBlockingFlag(false); // Disable source read blocking to prevent pause all task blocking
        source_->Pause();
    }
    if (taskMap_.find(audioTrackId_) != taskMap_.end() && taskMap_[audioTrackId_] != nullptr) {
        taskMap_[audioTrackId_]->PauseAsync();
        taskMap_[audioTrackId_]->Pause();
    }
 
    if (source_ != nullptr) {
        source_->SetReadBlockingFlag(true); // Enable source read blocking to ensure get wanted data
    }
    return Status::OK;
}

Status MediaDemuxer::PauseTaskByTrackId(int32_t trackId)
{
    MEDIA_LOG_I("In, track %{public}d", trackId);
    FALSE_RETURN_V_MSG_E(trackId >= 0, Status::ERROR_INVALID_PARAMETER, "Invalid track");

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
    MEDIA_LOG_I("In");
    if (streamDemuxer_) {
        streamDemuxer_->Resume();
    }
    if (source_) {
        source_->Resume();
    }
    if (inPreroll_.load()) {
        if (CheckTrackEnabledById(videoTrackId_)) {
            if (streamDemuxer_) {
                streamDemuxer_->SetIsIgnoreParse(false);
            }
            taskMap_[videoTrackId_]->Start();
        }
    } else {
        ResumeAllTask();
    }
    isPaused_ = false;
    return Status::OK;
}

Status MediaDemuxer::ResumeDragging()
{
    MEDIA_LOG_I("In");
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    if (streamDemuxer_) {
        streamDemuxer_->Resume();
    }
    if (source_) {
        source_->Resume();
    }
    if (taskMap_.find(videoTrackId_) != taskMap_.end() && taskMap_[videoTrackId_] != nullptr) {
        if (streamDemuxer_) {
            streamDemuxer_->SetIsIgnoreParse(false);
        }
        taskMap_[videoTrackId_]->Start();
    }
    isPaused_ = false;
    return Status::OK;
}

Status MediaDemuxer::ResumeAudioAlign()
{
    MEDIA_LOG_I("Resume");
    if (streamDemuxer_) {
        streamDemuxer_->Resume();
    }
    if (source_) {
        source_->Resume();
    }
    if (taskMap_.find(audioTrackId_) != taskMap_.end() && taskMap_[audioTrackId_] != nullptr) {
        streamDemuxer_->SetIsIgnoreParse(false);
        taskMap_[audioTrackId_]->Start();
    }
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
    }
    Stop();
    {
        AutoLock lock(mapMutex_);
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
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Not buffer queue mode");
    isDemuxerLoopExecuting_ = false;
    ResetInner();
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    for (auto item : requestBufferErrorCountMap_) {
        requestBufferErrorCountMap_[item.first] = 0;
    }
    videoStartTime_ = 0;
    streamDemuxer_->ResetAllCache();
    isSeekError_.store(false);
    return demuxerPluginManager_->Reset();
}

Status MediaDemuxer::Start()
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::Start");
    MEDIA_LOG_I("In");
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Not buffer queue mode");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::OK, "Process has been started already");
    FALSE_RETURN_V_MSG_E(bufferQueueMap_.size() != 0, Status::OK, "No buffer queue");
    for (auto it = eosMap_.begin(); it != eosMap_.end(); it++) {
        it->second = false;
    }
    for (auto it = requestBufferErrorCountMap_.begin(); it != requestBufferErrorCountMap_.end(); it++) {
        it->second = 0;
    }
    InitPtsInfo();
    isThreadExit_ = false;
    isStopped_ = false;
    isDemuxerLoopExecuting_ = true;
    if (inPreroll_.load()) {
        if (CheckTrackEnabledById(videoTrackId_)) {
            taskMap_[videoTrackId_]->Start();
        }
    } else {
        auto it = bufferQueueMap_.begin();
        while (it != bufferQueueMap_.end()) {
            uint32_t trackId = it->first;
            if (taskMap_.find(trackId) != taskMap_.end() && taskMap_[trackId] != nullptr) {
                taskMap_[trackId]->Start();
            } else {
                MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " task is not exist", trackId);
            }
            it++;
        }
    }
    source_->Start();
    return demuxerPluginManager_->Start();
}

Status MediaDemuxer::Preroll()
{
    std::lock_guard<std::mutex> lock(prerollMutex_);
    if (inPreroll_.load()) {
        return Status::OK;
    }
    if (!CheckTrackEnabledById(videoTrackId_)) {
        return Status::OK;
    }
    inPreroll_.store(true);
    MEDIA_LOG_D("Preroll enter.");
    Status ret = Status::OK;
    if (isStopped_.load()) {
        ret = Start();
    } else if (isPaused_.load()) {
        ret = Resume();
    }
    if (ret != Status::OK) {
        inPreroll_.store(false);
        MEDIA_LOG_E("Preroll failed, ret: %{public}d", ret);
    }
    return ret;
}

Status MediaDemuxer::PausePreroll()
{
    std::lock_guard<std::mutex> lock(prerollMutex_);
    if (!inPreroll_.load()) {
        return Status::OK;
    }
    MEDIA_LOG_D("Preroll enter.");
    Status ret = Pause();
    inPreroll_.store(false);
    return ret;
}

Status MediaDemuxer::Stop()
{
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    FALSE_RETURN_V_MSG_E(!isThreadExit_, Status::OK, "Thread exit");
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::Stop");
    MEDIA_LOG_I("In");
    {
        AutoLock lock(mapMutex_);
        FALSE_RETURN_V_MSG_E(!isStopped_, Status::OK, "Process has been stopped already, ignore");
        isStopped_ = true;
        StopAllTask();
    }
    streamDemuxer_->Stop();
    return demuxerPluginManager_->Stop();
}

bool MediaDemuxer::HasVideo()
{
    return videoTrackId_ != TRACK_ID_DUMMY;
}

void MediaDemuxer::InitMediaMetaData(const Plugins::MediaInfo& mediaInfo)
{
    AutoLock lock(mapMutex_);
    mediaMetaData_.globalMeta = std::make_shared<Meta>(mediaInfo.general);
    mediaMetaData_.trackMetas.clear();
    mediaMetaData_.trackMetas.reserve(mediaInfo.tracks.size());
    for (uint32_t index = 0; index < mediaInfo.tracks.size(); index++) {
        auto trackMeta = mediaInfo.tracks[index];
        mediaMetaData_.trackMetas.emplace_back(std::make_shared<Meta>(trackMeta));
    }
}

void MediaDemuxer::InitDefaultTrack(const Plugins::MediaInfo& mediaInfo, uint32_t& videoTrackId,
    uint32_t& audioTrackId, uint32_t& subtitleTrackId, std::string& videoMime)
{
    AutoLock lock(mapMutex_);
    std::string dafaultTrack = "[";
    for (uint32_t index = 0; index < mediaInfo.tracks.size(); index++) {
        if (demuxerPluginManager_->CheckTrackIsActive(index) == false) {
            continue;
        }
        auto trackMeta = mediaInfo.tracks[index];
        std::string mimeType;
        bool ret = trackMeta.Get<Tag::MIME_TYPE>(mimeType);
        if (ret && mimeType.find("video") == 0 &&
            !IsTrackDisabled(Plugins::MediaType::VIDEO)) {
            videoTrackCount_++;
            dafaultTrack += "/V:";
            dafaultTrack += std::to_string(index);
            videoMime = mimeType;
            if (videoTrackId == TRACK_ID_DUMMY) {
                videoTrackId = index;
            }
            if (!trackMeta.GetData(Tag::MEDIA_START_TIME, videoStartTime_)) {
                MEDIA_LOG_W("Get media start time failed");
            }
        } else if (ret && mimeType.find("audio") == 0 &&
            !IsTrackDisabled(Plugins::MediaType::AUDIO)) {
            dafaultTrack += "/A:";
            dafaultTrack += std::to_string(index);
            if (audioTrackId == TRACK_ID_DUMMY) {
                audioTrackId = index;
            }
        } else if (ret && IsSubtitleMime(mimeType) &&
            !IsTrackDisabled(Plugins::MediaType::SUBTITLE)) {
            dafaultTrack += "/S:";
            dafaultTrack += std::to_string(index);
            if (subtitleTrackId == TRACK_ID_DUMMY) {
                subtitleTrackId = index;
            }
        } else {}
    }
    dafaultTrack += "]";
    MEDIA_LOG_I(PUBLIC_LOG_S, dafaultTrack.c_str());
}

bool MediaDemuxer::GetBufferFromUserQueue(uint32_t queueIndex, uint32_t size)
{
    MEDIA_LOG_D("In, queue: " PUBLIC_LOG_U32 ", size: " PUBLIC_LOG_U32, queueIndex, size);
    FALSE_RETURN_V_MSG_E(bufferQueueMap_.count(queueIndex) > 0 && bufferQueueMap_[queueIndex] != nullptr, false,
        "BufferQueue " PUBLIC_LOG_D32 " is nullptr", queueIndex);

    AVBufferConfig avBufferConfig;
    avBufferConfig.capacity = static_cast<int32_t>(size);
    Status ret = bufferQueueMap_[queueIndex]->RequestBuffer(bufferMap_[queueIndex], avBufferConfig,
        REQUEST_BUFFER_TIMEOUT);
    if (ret != Status::OK) {
        requestBufferErrorCountMap_[queueIndex]++;
        if (requestBufferErrorCountMap_[queueIndex] % 5 == 0) { // log per 5 times fail
            MEDIA_LOG_W("Request buffer failed, queue: " PUBLIC_LOG_U32 ", ret:" PUBLIC_LOG_D32
                ", errorCnt:" PUBLIC_LOG_U32, queueIndex, (int32_t)(ret), requestBufferErrorCountMap_[queueIndex]);
        }
        if (requestBufferErrorCountMap_[queueIndex] >= REQUEST_FAILED_RETRY_TIMES) {
            MEDIA_LOG_E("Request failed too many times in 1min");
        }
    } else {
        requestBufferErrorCountMap_[queueIndex] = 0;
    }
    return ret == Status::OK;
}

bool MediaDemuxer::HandleSelectTrackChangeStream(int32_t trackId, int32_t newStreamID, int32_t& newTrackId)
{
    StreamType streamType = demuxerPluginManager_->GetStreamTypeByTrackID(trackId);
    TrackType type = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    int32_t currentStreamID = demuxerPluginManager_->GetStreamIDByTrackType(type);
    int32_t currentTrackId = trackId;
    if (newStreamID == -1 || currentStreamID == -1 || currentStreamID == newStreamID) {
        return false;
    }
    MEDIA_LOG_I("In");
    // stop plugin
    demuxerPluginManager_->StopPlugin(currentStreamID, streamDemuxer_);
 
    // start plugin
    Status ret = demuxerPluginManager_->StartPlugin(newStreamID, streamDemuxer_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Start plugin failed");
 
    // get new mediainfo
    Plugins::MediaInfo mediaInfo;
    demuxerPluginManager_->UpdateDefaultStreamID(mediaInfo, streamType, newStreamID);
    InitMediaMetaData(mediaInfo); // update mediaMetaData_
 
    // get newStreamID
    int32_t newInnerTrackId;
    demuxerPluginManager_->GetTrackInfoByStreamID(newStreamID, newTrackId, newInnerTrackId);
 
    // update track map
    demuxerPluginManager_->DeleteTempTrackMapInfo(currentTrackId);
    int32_t innerTrackID = demuxerPluginManager_->GetInnerTrackIDByTrackID(newTrackId);
    demuxerPluginManager_->UpdateTempTrackMapInfo(newTrackId, newTrackId, innerTrackID);
    MEDIA_LOG_I("Updata info");
 
    InnerSelectTrack(newTrackId);
 
    // update buffer queue
    bufferQueueMap_.insert(std::pair<uint32_t, sptr<AVBufferQueueProducer>>(newTrackId,
        bufferQueueMap_[currentTrackId]));
    bufferMap_.insert(std::pair<uint32_t, std::shared_ptr<AVBuffer>>(newTrackId,
        bufferMap_[currentTrackId]));
    bufferQueueMap_.erase(currentTrackId);
    bufferMap_.erase(currentTrackId);
 
    MEDIA_LOG_I("Out");
    return true;
}

bool MediaDemuxer::SelectTrackChangeStream(uint32_t trackId)
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::SelectTrackChangeStream");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, false, "Invalid param");
    TrackType type = demuxerPluginManager_->GetTrackTypeByTrackID(static_cast<int32_t>(trackId));
    int32_t newStreamID = -1;
    if (type == TRACK_AUDIO) {
        newStreamID = streamDemuxer_->GetNewAudioStreamID();
    } else if (type == TRACK_SUBTITLE) {
        newStreamID = streamDemuxer_->GetNewSubtitleStreamID();
    } else if (type == TRACK_VIDEO) {
        newStreamID = streamDemuxer_->GetNewVideoStreamID();
    } else {
        MEDIA_LOG_W("Invalid track " PUBLIC_LOG_U32, trackId);
        return false;
    }

    int32_t newTrackId;
    bool ret = HandleSelectTrackChangeStream(trackId, newStreamID, newTrackId);
    if (ret && eventReceiver_ != nullptr) {
        MEDIA_LOG_I("TrackType: " PUBLIC_LOG_U32 ", TrackId " PUBLIC_LOG_D32 " >> " PUBLIC_LOG_D32,
            static_cast<uint32_t>(type), trackId, newTrackId);
        if (type == TrackType::TRACK_AUDIO) {
            audioTrackId_ = static_cast<uint32_t>(newTrackId);
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_AUDIO_TRACK_CHANGE, newTrackId});
            shouldCheckAudioFramePts_ = true;
        } else if (type == TrackType::TRACK_VIDEO) {
            videoTrackId_ = static_cast<uint32_t>(newTrackId);
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_VIDEO_TRACK_CHANGE, newTrackId});
        } else if (type == TrackType::TRACK_SUBTITLE) {
            subtitleTrackId_ = static_cast<uint32_t>(newTrackId);
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_SUBTITLE_TRACK_CHANGE, newTrackId});
            shouldCheckSubtitleFramePts_ = true;
        }
 
        {
            std::lock_guard<std::mutex> lock(isSelectTrackMutex_);
            if (inSelectTrackType_.find(static_cast<int32_t>(type)) != inSelectTrackType_.end() &&
                inSelectTrackType_[static_cast<int32_t>(type)] == newTrackId) {
                inSelectTrackType_.erase(static_cast<int32_t>(type));
            }
        }

        if (taskMap_.find(trackId) != taskMap_.end() && taskMap_[trackId] != nullptr) {
            taskMap_[trackId]->StopAsync();   // stop self
        }
        return true;
    }
    return ret;
}

bool MediaDemuxer::SelectBitRateChangeStream(uint32_t trackId)
{
    int32_t currentStreamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
    int32_t newStreamID = streamDemuxer_->GetNewVideoStreamID();
    if (newStreamID >= 0 && currentStreamID != newStreamID) {
        MEDIA_LOG_I("In");
        demuxerPluginManager_->StopPlugin(currentStreamID, streamDemuxer_);

        Status ret = demuxerPluginManager_->StartPlugin(newStreamID, streamDemuxer_);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Start plugin failed");

        Plugins::MediaInfo mediaInfo;
        demuxerPluginManager_->UpdateDefaultStreamID(mediaInfo, VIDEO, newStreamID);
        InitMediaMetaData(mediaInfo); // update mediaMetaData_
        
        int32_t newInnerTrackId = -1;
        int32_t newTrackId = -1;
        demuxerPluginManager_->GetTrackInfoByStreamID(newStreamID, newTrackId, newInnerTrackId);
        demuxerPluginManager_->UpdateTempTrackMapInfo(videoTrackId_, newTrackId, newInnerTrackId);

        MEDIA_LOG_I("Updata info");
        InnerSelectTrack(static_cast<int32_t>(trackId));
        MEDIA_LOG_I("Out");
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

Status MediaDemuxer::HandleReadSample(uint32_t trackId)
{
    Status ret = InnerReadSample(trackId, bufferMap_[trackId]);
    if (trackId == videoTrackId_) {
        std::unique_lock<std::mutex> draggingLock(draggingMutex_);
        if (VideoStreamReadyCallback_ != nullptr) {
            if (ret != Status::OK && ret != Status::END_OF_STREAM) {
                bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], false);
                MEDIA_LOG_E("Read failed, track " PUBLIC_LOG_U32 ", ret:" PUBLIC_LOG_D32, trackId, (int32_t)(ret));
                return ret;
            }
            MEDIA_LOG_D("In");
            std::shared_ptr<VideoStreamReadyCallback> videoStreamReadyCallback = VideoStreamReadyCallback_;
            draggingLock.unlock();
            bool isDiscardable = videoStreamReadyCallback->IsVideoStreamDiscardable(bufferMap_[trackId]);
            bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], !isDiscardable);
            return Status::OK;
        }
    }

    if (source_ != nullptr && source_->IsSeekToTimeSupported() && isSeeked_ && HasVideo()) {
        if (trackId == videoTrackId_ && isFirstFrameAfterSeek_.load()) {
            bool isSyncFrame = (bufferMap_[trackId]->flag_ & (uint32_t)(AVBufferFlag::SYNC_FRAME)) != 0;
            if (!isSyncFrame) {
                MEDIA_LOG_E("The first frame after seeking is not a sync frame");
            }
            isFirstFrameAfterSeek_.store(false);
        }
        MEDIA_LOG_I("Seeking, found idr frame track " PUBLIC_LOG_U32, trackId);
        isSeeked_ = false;
    }
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        if (bufferMap_[trackId]->flag_ & (uint32_t)(AVBufferFlag::EOS)) {
            eosMap_[trackId] = true;
            if (taskMap_.find(trackId) != taskMap_.end() && taskMap_[trackId] != nullptr) {
                taskMap_[trackId]->StopAsync();
            }
            MEDIA_LOG_I("Track eos, track: " PUBLIC_LOG_U32 ", bufferId: " PUBLIC_LOG_U64
                ", pts: " PUBLIC_LOG_D64 ", flag: " PUBLIC_LOG_U32, trackId, bufferMap_[trackId]->GetUniqueId(),
                bufferMap_[trackId]->pts_, bufferMap_[trackId]->flag_);
            ret = bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], true);
            return Status::OK;
        }
        HandleAutoMaintainPts(trackId, bufferMap_[trackId]);
        bool isDroppable = IsBufferDroppable(bufferMap_[trackId], trackId);
        ret = bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], !isDroppable);
    } else {
        bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], false);
        MEDIA_LOG_E("Read failed, track " PUBLIC_LOG_U32 ", ret:" PUBLIC_LOG_D32, trackId, (int32_t)(ret));
    }
    return ret;
}

bool MediaDemuxer::HandleDashChangeStream(uint32_t trackId)
{
    FALSE_RETURN_V_NOLOG(demuxerPluginManager_->IsDash(), false);
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, false, "Plugin manager is nullptr");
    FALSE_RETURN_V_MSG_E(streamDemuxer_ != nullptr, false, "Stream is nullptr");

    MEDIA_LOG_D("IN");
    TrackType type = demuxerPluginManager_->GetTrackTypeByTrackID(static_cast<int32_t>(trackId));
    int32_t currentStreamID = demuxerPluginManager_->GetStreamIDByTrackType(type);
    int32_t newStreamID = demuxerPluginManager_->GetStreamDemuxerNewStreamID(type, streamDemuxer_);
    bool ret = false;
    FALSE_RETURN_V_NOLOG(newStreamID != -1 && currentStreamID != newStreamID, ret);

    MEDIA_LOG_I("Change stream begin, currentStreamID: " PUBLIC_LOG_D32 " newStreamID: " PUBLIC_LOG_D32,
        currentStreamID, newStreamID);
    if (trackId == videoTrackId_ && demuxerPluginManager_->GetCurrentBitRate() != targetBitRate_) {
        ret = SelectBitRateChangeStream(trackId);
        if (ret) {
            streamDemuxer_->SetChangeFlag(true);
            MEDIA_LOG_I("targetBitrate: " PUBLIC_LOG_U32 " currentBitrate: " PUBLIC_LOG_U32, targetBitRate_,
                demuxerPluginManager_->GetCurrentBitRate());
            isSelectBitRate_.store(targetBitRate_ != demuxerPluginManager_->GetCurrentBitRate());
        }
    } else {
        isSelectTrack_.store(true);
        ret = SelectTrackChangeStream(trackId);
        if (ret) {
            MEDIA_LOG_I("targetBitrate: " PUBLIC_LOG_U32 " currentBitrate: " PUBLIC_LOG_U32, targetBitRate_,
                demuxerPluginManager_->GetCurrentBitRate());
            targetBitRate_ = demuxerPluginManager_->GetCurrentBitRate();
            streamDemuxer_->SetChangeFlag(true);
        }
        isSelectTrack_.store(false);
    }
    MEDIA_LOG_I("Change stream success");
    return ret;
}

Status MediaDemuxer::CopyFrameToUserQueue(uint32_t trackId)
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::CopyFrameToUserQueue");
    MEDIA_LOG_D("In, track:" PUBLIC_LOG_U32, trackId);

    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    int32_t innerTrackID = static_cast<int32_t>(trackId);
    int32_t id = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
    if (demuxerPluginManager_->IsDash() || demuxerPluginManager_->GetTmpStreamIDByTrackID(subtitleTrackId_) != -1) {
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(id);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(id);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
    }

    int32_t size = 0;
    Status ret = pluginTemp->GetNextSampleSize(innerTrackID, size);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_UNKNOWN, ret, "Get size failed for track " PUBLIC_LOG_U32, trackId);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_AGAIN, ret,
        "Get size failed for track " PUBLIC_LOG_U32 ", retry", trackId);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_NO_MEMORY, ret, "Get size failed for track " PUBLIC_LOG_U32, trackId);
    if (HandleDashChangeStream(trackId)) {
        MEDIA_LOG_I("HandleDashChangeStream success");
        return Status::OK;
    }
    SetTrackNotifyFlag(trackId, true);
    if (!GetBufferFromUserQueue(trackId, size)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    SetTrackNotifyFlag(trackId, false);
    ret = HandleReadSample(trackId);
    MEDIA_LOG_D("Out, track:" PUBLIC_LOG_U32, trackId);
    return ret;
}

Status MediaDemuxer::InnerReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    MEDIA_LOG_D("In, track " PUBLIC_LOG_U32, trackId);
    int32_t innerTrackID = static_cast<int32_t>(trackId);
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    if (demuxerPluginManager_->IsDash() || demuxerPluginManager_->GetTmpStreamIDByTrackID(subtitleTrackId_) != -1) {
        int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamId);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        int32_t streamId = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamId);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
    }
 
    Status ret = pluginTemp->ReadSample(innerTrackID, sample);
    if (ret == Status::END_OF_STREAM) {
        MEDIA_LOG_I("Read eos for track " PUBLIC_LOG_U32, trackId);
    } else if (ret != Status::OK) {
        MEDIA_LOG_I("Read error for track " PUBLIC_LOG_U32 ", ret: " PUBLIC_LOG_D32, trackId, (int32_t)(ret));
    }
    MEDIA_LOG_D("Out, track " PUBLIC_LOG_U32, trackId);
    ProcessDrmInfos();
    return ret;
}

int64_t MediaDemuxer::ReadLoop(uint32_t trackId)
{
    if (streamDemuxer_->GetIsIgnoreParse() || isStopped_ || isPaused_ || isSeekError_) {
        MEDIA_LOG_D("ReadLoop pausing or error, track " PUBLIC_LOG_U32, trackId);
        return 6 * 1000; // sleep 6ms in pausing to avoid useless reading
    } else {
        Status ret = CopyFrameToUserQueue(trackId);
        // when read failed, or request always failed in 1min, send error event
        bool ignoreError = isStopped_ || isPaused_ || isInterruptNeeded_.load();
        if ((ret == Status::ERROR_UNKNOWN && !ignoreError) ||
             requestBufferErrorCountMap_[trackId] >= REQUEST_FAILED_RETRY_TIMES) {
            MEDIA_LOG_E("Invalid data source, can not get frame");
            if (eventReceiver_ != nullptr) {
                eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_DATA_SOURCE_ERROR_UNKNOWN});
            } else {
                MEDIA_LOG_D("EventReceiver is nullptr");
            }
        }
        bool isNeedRetry = ret == Status::OK || ret == Status::ERROR_AGAIN;
        if (isNeedRetry) {
            return 0; // retry next frame
        } else if (ret == Status::ERROR_NO_MEMORY) {
            MEDIA_LOG_E("Cache data size is out of limit");
            if (eventReceiver_ != nullptr && !isOnEventNoMemory_.load()) {
                isOnEventNoMemory_.store(true);
                eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_DEMUXER_BUFFER_NO_MEMORY});
            }
            return 0;
        } else {
            MEDIA_LOG_D("ReadLoop wait, track:" PUBLIC_LOG_U32 ", ret:" PUBLIC_LOG_D32,
                trackId, static_cast<int32_t>(ret));
            return RETRY_DELAY_TIME_US; // delay to retry if no frame
        }
    }
}

Status MediaDemuxer::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE, "Not buffer queue mode");
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(eosMap_.count(trackId) > 0, Status::ERROR_INVALID_OPERATION, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_!=nullptr, Status::ERROR_INVALID_PARAMETER,
        "AVBuffer is nullptr");
    if (eosMap_[trackId]) {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has reached eos", trackId);
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
    MEDIA_LOG_I("In");
    std::multimap<std::string, std::vector<uint8_t>> infoUpdated;
    bool isUpdated = GetDrmInfosUpdated(info, infoUpdated);
    if (isUpdated) {
        ReportDrmInfos(infoUpdated);
        return;
    }
    MEDIA_LOG_D("Demuxer filter received source drminfos but not update");
}

void MediaDemuxer::OnEvent(const Plugins::PluginEvent &event)
{
    MEDIA_LOG_D("In");
    if (eventReceiver_ == nullptr && event.type != PluginEventType::SOURCE_DRM_INFO_UPDATE) {
        MEDIA_LOG_D("EventReceiver is nullptr");
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
            MEDIA_LOG_E("OnEvent error code " PUBLIC_LOG_D32, AnyCast<int32_t>(event.param));
            eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, event.param});
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
        case PluginEventType::CACHED_DURATION: {
            MEDIA_LOG_D("OnEvent cached duration");
            eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_CACHED_DURATION, event.param});
            break;
        }
        case PluginEventType::SOURCE_BITRATE_START: {
            MEDIA_LOG_D("OnEvent source bitrate start");
            eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_SOURCE_BITRATE_START, event.param});
            break;
        }
        case PluginEventType::EVENT_BUFFER_PROGRESS: {
            MEDIA_LOG_D("OnEvent percent update");
            eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_BUFFER_PROGRESS, event.param});
            break;
        }
        default:
            break;
    }
    OnSeekReadyEvent(event);
}

void MediaDemuxer::OnSeekReadyEvent(const Plugins::PluginEvent &event)
{
    FALSE_RETURN_NOLOG(event.type == PluginEventType::DASH_SEEK_READY);
    MEDIA_LOG_D("Onevent dash seek ready");
    std::unique_lock<std::mutex> lock(rebootPluginMutex_);
    Format param = AnyCast<Format>(event.param);
    int32_t currentStreamType = -1;
    param.GetIntValue("currentStreamType", currentStreamType);
    int32_t isEOS = -1;
    param.GetIntValue("isEOS", isEOS);
    int32_t currentStreamId = -1;
    param.GetIntValue("currentStreamId", currentStreamId);
    MEDIA_LOG_D("HandleDashSeekReady, streamType: " PUBLIC_LOG_D32 " streamId: " PUBLIC_LOG_D32
        " isEos: " PUBLIC_LOG_D32, currentStreamType, currentStreamId, isEOS);
    switch (currentStreamType) {
        case static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_VID):
            seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = std::make_pair(currentStreamId, isEOS);
            break;
        case static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_AUD):
            seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)] = std::make_pair(currentStreamId, isEOS);
            break;
        case static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE):
            seekReadyStreamInfo_[static_cast<int32_t>(StreamType::SUBTITLE)] = std::make_pair(currentStreamId, isEOS);
            break;
        default:
            break;
    }
    rebootPluginCondition_.notify_all();
}

Status MediaDemuxer::OptimizeDecodeSlow(bool isDecodeOptimizationEnabled)
{
    MEDIA_LOG_I("In");
    isDecodeOptimizationEnabled_ = isDecodeOptimizationEnabled;
    return Status::OK;
}

Status MediaDemuxer::SetDecoderFramerateUpperLimit(int32_t decoderFramerateUpperLimit,
    uint32_t trackId)
{
    MEDIA_LOG_I("DecoderFramerateUpperLimit=" PUBLIC_LOG_D32 " trackId=" PUBLIC_LOG_D32,
        decoderFramerateUpperLimit, trackId);
    FALSE_RETURN_V(trackId == videoTrackId_, Status::OK);
    FALSE_RETURN_V_MSG_E(decoderFramerateUpperLimit > 0, Status::ERROR_INVALID_PARAMETER,
        "SetDecoderFramerateUpperLimit failed, decoderFramerateUpperLimit <= 0");
    decoderFramerateUpperLimit_.store(decoderFramerateUpperLimit);
    return Status::OK;
}

Status MediaDemuxer::SetSpeed(float speed)
{
    MEDIA_LOG_I("Speed=" PUBLIC_LOG_F, speed);
    FALSE_RETURN_V_MSG_E(speed > 0, Status::ERROR_INVALID_PARAMETER, "Speed <= 0");
    speed_.store(speed);
    return Status::OK;
}

Status MediaDemuxer::SetFrameRate(double framerate, uint32_t trackId)
{
    MEDIA_LOG_I("Framerate=" PUBLIC_LOG_F " trackId=" PUBLIC_LOG_D32, framerate, trackId);
    FALSE_RETURN_V(trackId == videoTrackId_, Status::OK);
    FALSE_RETURN_V_MSG_E(framerate > 0, Status::ERROR_INVALID_PARAMETER, "Framerate <= 0");
    framerate_.store(framerate);
    return Status::OK;
}

void MediaDemuxer::CheckDropAudioFrame(std::shared_ptr<AVBuffer> sample, uint32_t trackId)
{
    if (trackId == audioTrackId_) {
        if (shouldCheckAudioFramePts_ == false) {
            lastAudioPts_ = sample->pts_;
            MEDIA_LOG_I("Set last audio pts " PUBLIC_LOG_D64, lastAudioPts_);
            return;
        }
        if (sample->pts_ < lastAudioPts_) {
            MEDIA_LOG_I("Drop audio buffer pts " PUBLIC_LOG_D64, sample->pts_);
            return;
        }
        if (shouldCheckAudioFramePts_) {
            shouldCheckAudioFramePts_ = false;
        }
    }
    if (trackId == subtitleTrackId_) {
        if (shouldCheckSubtitleFramePts_ == false) {
            lastSubtitlePts_ = sample->pts_;
            MEDIA_LOG_I("Set last subtitle pts " PUBLIC_LOG_D64, lastSubtitlePts_);
            return;
        }
        if (sample->pts_ < lastSubtitlePts_) {
            MEDIA_LOG_I("Drop subtitle buffer pts " PUBLIC_LOG_D64, sample->pts_);
            return;
        }
        if (shouldCheckSubtitleFramePts_) {
            shouldCheckSubtitleFramePts_ = false;
        }
    }
}

bool MediaDemuxer::IsBufferDroppable(std::shared_ptr<AVBuffer> sample, uint32_t trackId)
{
    DumpBufferToFile(trackId, sample);

    if (demuxerPluginManager_->IsDash()) {
        CheckDropAudioFrame(sample, trackId);
    }

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

    MEDIA_LOG_D("Drop buffer, framerate=" PUBLIC_LOG_F " speed=" PUBLIC_LOG_F " decodeUpLimit="
        PUBLIC_LOG_D32 " pts=" PUBLIC_LOG_D64, framerate_.load(), speed_.load(),
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

bool MediaDemuxer::CheckTrackEnabledById(uint32_t trackId)
{
    bool hasTrack = trackId != TRACK_ID_DUMMY;
    if (!hasTrack) {
        return false;
    }
    bool hasTask = taskMap_.find(trackId) != taskMap_.end() && taskMap_[trackId] != nullptr;
    if (!hasTask) {
        return false;
    }
    bool linkNode = bufferQueueMap_.find(trackId) != bufferQueueMap_.end()
        && bufferQueueMap_[trackId] != nullptr;
    return linkNode;
}

void MediaDemuxer::SetSelectBitRateFlag(bool flag, uint32_t desBitRate)
{
    MEDIA_LOG_I("Flag=" PUBLIC_LOG_D32 " desBitRate=" PUBLIC_LOG_U32,
        static_cast<int32_t>(flag), desBitRate);
    isSelectBitRate_.store(flag);
    if (flag) {
        targetBitRate_ = desBitRate;
    }
}
 
bool MediaDemuxer::CanAutoSelectBitRate()
{
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, false, "Plugin manager is nullptr");
    // calculating auto selectbitrate time
    return !(isSelectBitRate_.load()) && !(isSelectTrack_.load())
        && (targetBitRate_ == demuxerPluginManager_->GetCurrentBitRate());
}

bool MediaDemuxer::IsRenderNextVideoFrameSupported()
{
    bool isDataSrcLiveStream = source_ != nullptr && source_->IsNeedPreDownload() &&
        source_->GetSeekable() == Plugins::Seekable::UNSEEKABLE;
    return videoTrackId_ != TRACK_ID_DUMMY && !IsTrackDisabled(Plugins::MediaType::VIDEO) &&
        !isDataSrcLiveStream;
}

Status MediaDemuxer::GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
    const uint64_t relativePresentationTimeUs, uint32_t &index)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");

    Status ret = pluginTemp->GetIndexByRelativePresentationTimeUs(trackIndex, relativePresentationTimeUs, index);
    if (ret != Status::OK) {
        MEDIA_LOG_E("Get index failed");
    }
    return ret;
}

Status MediaDemuxer::GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
    const uint32_t index, uint64_t &relativePresentationTimeUs)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");

    Status ret = pluginTemp->GetRelativePresentationTimeUsByIndex(trackIndex, index, relativePresentationTimeUs);
    if (ret != Status::OK) {
        MEDIA_LOG_E("Get pts failed");
    }
    return ret;
}

Status MediaDemuxer::ResumeDemuxerReadLoop()
{
    MEDIA_LOG_I("In");
    if (isDemuxerLoopExecuting_) {
        MEDIA_LOG_I("Has already resumed");
        return Status::OK;
    }
    isDemuxerLoopExecuting_ = true;
    return ResumeAllTask();
}

Status MediaDemuxer::PauseDemuxerReadLoop()
{
    MEDIA_LOG_I("In");
    if (!isDemuxerLoopExecuting_) {
        MEDIA_LOG_I("Has already paused");
        return Status::OK;
    }
    isDemuxerLoopExecuting_ = false;
    return PauseAllTask();
}

void MediaDemuxer::SetCacheLimit(uint32_t limitSize)
{
    FALSE_RETURN_MSG(demuxerPluginManager_ != nullptr, "Plugin manager is nullptr");
    (void)demuxerPluginManager_->SetCacheLimit(limitSize);
}

bool MediaDemuxer::IsVideoEos()
{
    if (videoTrackId_ == TRACK_ID_DUMMY) {
        return true;
    }
    return eosMap_[videoTrackId_];
}

bool MediaDemuxer::HasEosTrack()
{
    for (auto it = eosMap_.begin(); it != eosMap_.end(); it++) {
        if (it->second) {
            return true;
        }
    }
    return false;
}

void MediaDemuxer::SetEnableOnlineFdCache(bool isEnableFdCache)
{
    FALSE_RETURN(source_ != nullptr);
    source_->SetEnableOnlineFdCache(isEnableFdCache);
}

void MediaDemuxer::WaitForBufferingEnd()
{
    FALSE_RETURN_MSG(source_ != nullptr, "Source is nullptr");
    source_->WaitForBufferingEnd();
}

int32_t MediaDemuxer::GetCurrentVideoTrackId()
{
    return (videoTrackId_ != TRACK_ID_DUMMY ? static_cast<int32_t>(videoTrackId_) : INVALID_TRACK_ID);
}

void MediaDemuxer::SetIsEnableReselectVideoTrack(bool isEnable)
{
    isEnableReselectVideoTrack_  = isEnable;
}

bool MediaDemuxer::IsHasMultiVideoTrack()
{
    return videoTrackCount_ >= DEFAULT_MULTI_VIDEO_TRACK_NUM;
}
} // namespace Media
} // namespace OHOS
