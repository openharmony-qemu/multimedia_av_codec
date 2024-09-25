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
#include "hisysevent.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_buffer.h"
#include "source/source.h"
#include "stream_demuxer.h"
#include "media_core.h"
#include "osal/utils/dump_buffer.h"
#include "demuxer_plugin_manager.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MediaDemuxer" };
const std::string DUMP_PARAM = "a";
const std::string DUMP_DEMUXER_AUDIO_FILE_NAME = "player_demuxer_audio_output.es";
const std::string DUMP_DEMUXER_VIDEO_FILE_NAME = "player_demuxer_video_output.es";
static constexpr char PERFORMANCE_STATS[] = "PERFORMANCE";
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
constexpr uint32_t DEFAULT_PREPARE_FRAME_COUNT = 1; // Default prepare frame count 1.

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
    MEDIA_LOG_I("Report scene code %{public}d", scene);
    auto now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    int32_t ret = HiSysEventWrite(
        PERFORMANCE_STATS, "CPU_SCENE_ENTRY", OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR, "PACKAGE_NAME",
        "media_service", "SCENE_ID", std::to_string(scene).c_str(), "HAPPEN_TIME", now.count());
    if (ret != MSERR_OK) {
        MEDIA_LOG_W("report error");
    }
}

Status MediaDemuxer::StartReferenceParser(int64_t startTimeMs, bool isForward)
{
    FALSE_RETURN_V_MSG_E(startTimeMs >= 0, Status::ERROR_UNKNOWN,
                         "StartReferenceParser failed, startTimeMs: " PUBLIC_LOG_D64, startTimeMs);
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER,
                         "StartReferenceParser failed due to source is nullptr");
    FALSE_RETURN_V_MSG_E(videoTrackId_ != TRACK_ID_DUMMY, Status::ERROR_UNKNOWN,
                         "StartReferenceParser failed due to video track is null");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER,
                         "StartReferenceParser failed due to demuxerPluginManager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER,
                         "StartReferenceParser failed due to plugin is nullptr");
    Status ret = plugin->ParserRefUpdatePos(startTimeMs, isForward);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ParserRefUpdatePos fail.");
    if (isFirstParser_) {
        isFirstParser_ = false;
        FALSE_RETURN_V_MSG(source_->GetSeekable() == Plugins::Seekable::SEEKABLE,
            Status::ERROR_INVALID_OPERATION, "Do not support online video");
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
        MEDIA_LOG_I("Success to recovery parserRefInfo task.");
    }
}

int64_t MediaDemuxer::ParserRefInfo()
{
    FALSE_RETURN_V_MSG_D(demuxerPluginManager_ != nullptr, 0,
        "ParserRefInfo failed due to demuxerPluginManager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_D(plugin != nullptr, 0, "ParserRefInfo failed due to plugin is nullptr");
    Status ret = plugin->ParserRefInfo();
    if ((ret == Status::OK || ret == Status::ERROR_UNKNOWN) && parserRefInfoTask_ != nullptr) {
        parserRefInfoTask_->Stop();
        isParserTaskEnd_ = true;
        MEDIA_LOG_I("Success to stop parserRefInfo task.");
    }
    return 0;
}

Status MediaDemuxer::GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_E(videoSample != nullptr, Status::ERROR_NULL_POINTER, "videoSample is nullptr");
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER,
                         "GetFrameLayerInfo failed due to source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER,
                         "GetFrameLayerInfo failed due to demuxerPluginManager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER,
                         "GetFrameLayerInfo failed due to plugin is nullptr");
    TryRecvParserTask();
    Status ret = plugin->GetFrameLayerInfo(videoSample, frameLayerInfo);
    if (ret == Status::ERROR_NULL_POINTER && parserRefInfoTask_ != nullptr) {
        return Status::ERROR_AGAIN;
    }
    return ret;
}

Status MediaDemuxer::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER,
                         "GetFrameLayerInfo failed due to source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER,
                         "GetFrameLayerInfo failed due to demuxerPluginManager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER,
                         "GetFrameLayerInfo failed due to plugin is nullptr");
    TryRecvParserTask();
    Status ret = plugin->GetFrameLayerInfo(frameId, frameLayerInfo);
    if (ret == Status::ERROR_NULL_POINTER && parserRefInfoTask_ != nullptr) {
        return Status::ERROR_AGAIN;
    }
    return ret;
}

Status MediaDemuxer::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER,
                         "GetGopLayerInfo failed due to source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER,
                         "GetGopLayerInfo failed due to demuxerPluginManager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER,
                         "GetGopLayerInfo failed due to plugin is nullptr");
    TryRecvParserTask();
    Status ret = plugin->GetGopLayerInfo(gopId, gopLayerInfo);
    if (ret == Status::ERROR_NULL_POINTER && parserRefInfoTask_ != nullptr) {
        return Status::ERROR_AGAIN;
    }
    return ret;
}

void MediaDemuxer::RegisterVideoStreamReadyCallback(const std::shared_ptr<VideoStreamReadyCallback> &callback)
{
    MEDIA_LOG_I("RegisterVideoStreamReadyCallback step into");
    VideoStreamReadyCallback_ = callback;
}

void MediaDemuxer::DeregisterVideoStreamReadyCallback()
{
    MEDIA_LOG_I("DeregisterVideoStreamReadyCallback step into");
    VideoStreamReadyCallback_ = nullptr;
}

Status MediaDemuxer::GetIFramePos(std::vector<uint32_t> &IFramePos)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER,
                         "GetIFramePos failed due to source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER,
                         "GetIFramePos failed due to demuxerPluginManager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER,
                         "GetIFramePos failed due to plugin is nullptr");
    TryRecvParserTask();
    return plugin->GetIFramePos(IFramePos);
}

Status MediaDemuxer::Dts2FrameId(int64_t dts, uint32_t &frameId, bool offset)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Dts2FrameId failed due to source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER,
                         "Dts2FrameId failed due to demuxerPluginManager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER,
                         "Dts2FrameId failed due to plugin is nullptr");
    TryRecvParserTask();
    return plugin->Dts2FrameId(dts, frameId, offset);
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
    MEDIA_LOG_D("AccelerateTrackTask trackId:" PUBLIC_LOG_U32, trackId);
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
    FALSE_RETURN_V_MSG(source_ != nullptr, Status::ERROR_INVALID_OPERATION,
        "GetBitRates failed, source_ is nullptr");
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
    FALSE_RETURN_V_MSG(source_ != nullptr, Status::ERROR_INVALID_OPERATION,
        "GetDownloadInfo failed, source_ is null");
    return source_->GetDownloadInfo(downloadInfo);
}

Status MediaDemuxer::GetPlaybackInfo(PlaybackInfo& playbackInfo)
{
    if (source_ == nullptr) {
        MEDIA_LOG_E("GetPlaybackInfo  failed, source_ is null");
        return Status::ERROR_INVALID_OPERATION;
    }
    return source_->GetPlaybackInfo(playbackInfo);
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
    FALSE_RETURN_MSG(!isDump || instanceId != 0, "Cannot dump with instanceId 0.");
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
            MEDIA_LOG_I("GetDuration for hls, duration: " PUBLIC_LOG_D64, duration_);
            mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(Plugins::HstTime2Us(duration_));
        }
        MEDIA_LOG_I("GetDuration for seek to time");
        return mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs);
    }
    
    // not hls and seekable
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        duration_ = source_->GetDuration();
        if (duration_ != Plugins::HST_TIME_NONE) {
            MEDIA_LOG_I("GetDuration for not hls, duration: " PUBLIC_LOG_D64, duration_);
            mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(Plugins::HstTime2Us(duration_));
        }
        MEDIA_LOG_I("GetDuration for seekble");
        return mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs);
    }
    MEDIA_LOG_I("GetDuration for other");
    return mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs);
}

bool MediaDemuxer::GetDrmInfosUpdated(const std::multimap<std::string, std::vector<uint8_t>> &newInfos,
    std::multimap<std::string, std::vector<uint8_t>> &result)
{
    MEDIA_LOG_D("GetDrmInfosUpdated");
    std::unique_lock<std::shared_mutex> lock(drmMutex);
    for (auto &newItem : newInfos) {
        if (localDrmInfos_.find(newItem.first) == localDrmInfos_.end()) {
            MEDIA_LOG_D("this uuid doesn't exist, update.");
            result.insert(newItem);
            localDrmInfos_.insert(newItem);
            continue;
        }
        auto pos = localDrmInfos_.equal_range(newItem.first);
        bool isSame = false;
        for (; pos.first != pos.second; ++pos.first) {
            if (newItem.second == pos.first->second) {
                MEDIA_LOG_D("this uuid exists and the pssh is same, not update.");
                isSame = true;
                break;
            }
        }
        if (!isSame) {
            MEDIA_LOG_D("this uuid exists but pssh not same, update.");
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
    MEDIA_LOG_I("ReportDrmInfos");
    FALSE_RETURN_V_MSG_E(drmCallback_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "ReportDrmInfos failed due to drmcallback nullptr.");
    MEDIA_LOG_I("demuxer filter will update drminfo OnDrmInfoChanged");
    drmCallback_->OnDrmInfoChanged(info);
    return Status::OK;
}

Status MediaDemuxer::ProcessDrmInfos()
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER,
                         "source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER,
                         "demuxerPluginManager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
        "get demuxer plugin failed.");
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    Status ret = pluginTemp->GetDrmInfo(drmInfo);
    if (ret == Status::OK && !drmInfo.empty()) {
        MEDIA_LOG_D("MediaDemuxer get drminfo success");
        std::multimap<std::string, std::vector<uint8_t>> infosUpdated;
        bool isUpdated = GetDrmInfosUpdated(drmInfo, infosUpdated);
        if (isUpdated) {
            return ReportDrmInfos(infosUpdated);
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
            MEDIA_LOG_E("AddDemuxerCopyTask failed, unknow task type:" PUBLIC_LOG_D32, type);
            return Status::ERROR_UNKNOWN;
        }
    }

    std::unique_ptr<Task> task = std::make_unique<Task>(taskName, playerId_, type);
    FALSE_RETURN_V_MSG_W(task != nullptr, Status::OK,
        "AddDemuxerCopyTask create task failed, trackId:" PUBLIC_LOG_U32 ", type:" PUBLIC_LOG_D32,
        trackId, type);
    taskMap_[trackId] = std::move(task);
    taskMap_[trackId]->RegisterJob([this, trackId] { return ReadLoop(trackId); });

    // To wake up DEMUXER TRACK WORKING TASK immediately on input buffer available.
    std::unique_ptr<Task> notifyTask =
        std::make_unique<Task>(taskName + "N", playerId_, type, TaskPriority::NORMAL, false);
    FALSE_RETURN_V_MSG_W(notifyTask != nullptr, Status::OK,
        "Add track notify task, make task failed, trackId:" PUBLIC_LOG_U32 ", type:" PUBLIC_LOG_D32,
        trackId, static_cast<uint32_t>(type));

    sptr<IProducerListener> listener =
        OHOS::sptr<AVBufferQueueProducerListener>::MakeSptr(trackId, shared_from_this(), notifyTask);
    FALSE_RETURN_V_MSG_W(listener != nullptr, Status::OK,
        "Add track notify task, make listener failed, trackId:" PUBLIC_LOG_U32 ", type:" PUBLIC_LOG_D32,
        trackId, static_cast<uint32_t>(type));

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
            AddDemuxerCopyTask(videoTrackId_, TaskType::VIDEO);
            demuxerPluginManager_->UpdateTempTrackMapInfo(videoTrackId_, videoTrackId_, -1);
            int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(videoTrackId_);
            streamDemuxer_->SetNewVideoStreamID(streamId);
            streamDemuxer_->SetChangeFlag(true);
            streamDemuxer_->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
            int64_t bitRate = 0;
            mediaMetaData_.trackMetas[videoTrackId_]->GetData(Tag::MEDIA_BITRATE, bitRate);
            source_->SetCurrentBitRate(bitRate, streamId);
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
        MEDIA_LOG_E("demuxer filter parse meta failed, ret: " PUBLIC_LOG_D32, (int32_t)(ret));
    }
    return ret;
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

    std::vector<StreamInfo> streams;
    source_->GetStreamInfo(streams);
    demuxerPluginManager_->InitDefaultPlay(streams);

    streamDemuxer_ = std::make_shared<StreamDemuxer>();
    streamDemuxer_->SetSource(source_);
    streamDemuxer_->Init(uri_);

    ret = InnerPrepare();

    ProcessDrmInfos();
    MEDIA_LOG_I("SetDataSource exit");
    return ret;
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
    MEDIA_LOG_I("SetSubtitleSource begin");
    if (subtitleTrackId_ != TRACK_ID_DUMMY) {
        MEDIA_LOG_W("SetSubtitleSource found subtitle track, not support add ext subtitle");
        return Status::OK;
    }
    subtitleSource_->SetCallback(this);
    subtitleSource_->SetSource(subSource);
    Status ret = subtitleSource_->GetSize(subMediaDataSize_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set subtitle data source failed due to get file size failed.");
    subSeekable_ = subtitleSource_->GetSeekable();

    int32_t subtitleStreamID = demuxerPluginManager_->AddExternalSubtitle();
    subStreamDemuxer_ = std::make_shared<StreamDemuxer>();
    subStreamDemuxer_->SetSource(subtitleSource_);
    subStreamDemuxer_->Init(subSource->GetSourceUri());

    MediaInfo mediaInfo;
    ret = demuxerPluginManager_->LoadCurrentSubtitlePlugin(subStreamDemuxer_, mediaInfo);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "demuxer filter parse meta failed, ret: "
        PUBLIC_LOG_D32, (int32_t)(ret));
    InitMediaMetaData(mediaInfo);  // update mediaMetaData_
    for (uint32_t index = 0; index < mediaInfo.tracks.size(); index++) {
        auto trackMeta = mediaInfo.tracks[index];
        std::string mimeType;
        std::string trackType;
        trackMeta.Get<Tag::MIME_TYPE>(mimeType);
        int32_t curStreamID = demuxerPluginManager_->GetStreamIDByTrackID(index);
        if (trackMeta.Get<Tag::MIME_TYPE>(mimeType) && IsSubtitleMime(mimeType) && curStreamID == subtitleStreamID) {
            MEDIA_LOG_I("Found subtitle track, id: " PUBLIC_LOG_U32 ", mimeType: " PUBLIC_LOG_S,
                index, mimeType.c_str());
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

    MEDIA_LOG_I("SetSubtitleSource done, ext sub track id = %{public}d", subtitleTrackId_);
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
    if (demuxerPluginManager_->IsDash() || demuxerPluginManager_->GetTmpStreamIDByTrackID(subtitleTrackId_) != -1) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "InnerSelectTrack failed due to get demuxer plugin failed.");
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(demuxerPluginManager_->GetStreamIDByTrackID(trackId));
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "InnerSelectTrack failed due to get demuxer plugin failed.");
    }

    return pluginTemp->SelectTrack(innerTrackID);
}

Status MediaDemuxer::StartTask(int32_t trackId)
{
    MEDIA_LOG_I("StartTask trackId: " PUBLIC_LOG_D32, trackId);
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
        taskMap_[trackId]->Start();
    } else {
        if (!taskMap_[trackId]->IsTaskRunning()) {
            taskMap_[trackId]->Start();
        }
    }
    return Status::OK;
}

Status MediaDemuxer::HandleDashSelectTrack(int32_t trackId)
{
    MEDIA_LOG_I("HandleDashSelectTrack begin trackId: " PUBLIC_LOG_D32, trackId);
    eosMap_[trackId] = false;
    requestBufferErrorCountMap_[trackId] = 0;

    int32_t targetStreamID = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
    if (targetStreamID == -1) {
        MEDIA_LOG_E("HandleDashSelectTrack GetStreamIDByTrackID failed");
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
        MEDIA_LOG_E("HandleDashSelectTrack trackType invalid");
        return Status::ERROR_UNKNOWN;
    }

    if (curTrackId == trackId) {
        MEDIA_LOG_E("HandleDashSelectTrack same trackID");
        return Status::OK;
    }

    if (targetStreamID != demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)) {
        MEDIA_LOG_I("HandleDashSelectTrack SelectStream");
        selectTrackTrackID_ = static_cast<uint32_t>(trackId);
        isSelectTrack_.store(true);
        return source_->SelectStream(targetStreamID);
    }

    // same streamID
    Status ret = DoSelectTrack(trackId, curTrackId);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "DoSelectTrack track error.");
    if (eventReceiver_ != nullptr) {
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
        taskMap_[curTrackId]->Stop();
        Status ret = UnselectTrack(curTrackId);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "DoSelectTrack track error.");
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
        MEDIA_LOG_I("SelectTrack audio now: " PUBLIC_LOG_D32 ", to: " PUBLIC_LOG_D32, audioTrackId_, trackId);
        curTrackId = static_cast<int32_t>(audioTrackId_);
    } else {    // inner subtitle and video not support
        MEDIA_LOG_W("SelectTrack : " PUBLIC_LOG_D32 " failed, not support", trackId);
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (curTrackId == trackId) {
        return Status::OK;
    }

    Status ret = DoSelectTrack(trackId, curTrackId);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "DoSelectTrack track error.");
    if (eventReceiver_ != nullptr) {
        if (trackType == TrackType::TRACK_AUDIO) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_AUDIO_TRACK_CHANGE, trackId});
            audioTrackId_ =  static_cast<uint32_t>(trackId);
        } else if (trackType == TrackType::TRACK_VIDEO) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_VIDEO_TRACK_CHANGE, trackId});
            videoTrackId_ =  static_cast<uint32_t>(trackId);
        } else {}
    }
    
    return ret;
}

Status MediaDemuxer::SelectTrack(int32_t trackId)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_I("SelectTrack begin trackId: " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(trackId >= 0 && (uint32_t)trackId < mediaMetaData_.trackMetas.size(),
        Status::ERROR_INVALID_PARAMETER, "Select trackId error.");
    if (!useBufferQueue_) {
        return InnerSelectTrack(trackId);
    }
    if (demuxerPluginManager_->IsDash()) {
        if (streamDemuxer_->CanDoChangeStream() == false) {
            MEDIA_LOG_W("SelectBitRate or SelectTrack is running, can not SelectTrack");
            return Status::ERROR_WRONG_STATE;
        }
        return HandleDashSelectTrack(trackId);
    }
    return HandleSelectTrack(trackId);
}

Status MediaDemuxer::UnselectTrack(int32_t trackId)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_I("UnselectTrack trackId: " PUBLIC_LOG_D32, trackId);

    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    int32_t innerTrackID = trackId;
    if (demuxerPluginManager_->IsDash() || demuxerPluginManager_->GetTmpStreamIDByTrackID(subtitleTrackId_) != -1) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "UnselectTrack failed due to get demuxer plugin failed.");
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        int32_t streamID = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
        if (streamID == -1) {
            MEDIA_LOG_W("UnselectTrack invalid trackId: " PUBLIC_LOG_D32, trackId);
            return Status::OK;
        }
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "UnselectTrack failed due to get demuxer plugin failed.");
    }
    demuxerPluginManager_->DeleteTempTrackMapInfo(trackId);

    return pluginTemp->UnselectTrack(innerTrackID);
}

void MediaDemuxer::HandleStopPlugin(int32_t trackId)
{
    if (static_cast<uint32_t>(trackId) != TRACK_ID_DUMMY) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        MEDIA_LOG_I("HandleStopPlugin, id = " PUBLIC_LOG_D32, streamID);
        demuxerPluginManager_->StopPlugin(streamID, streamDemuxer_);
    }
}

void MediaDemuxer::HandleStartPlugin(int32_t trackId)
{
    if (static_cast<uint32_t>(trackId) != TRACK_ID_DUMMY) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        demuxerPluginManager_->StartPlugin(streamID, streamDemuxer_);
        InnerSelectTrack(trackId);
    }
}

Status MediaDemuxer::SeekToTimePre()
{
    if (demuxerPluginManager_->IsDash() == false) {
        return Status::OK;
    }

    if (isSelectBitRate_ == true) {
        HandleStopPlugin(audioTrackId_);
    } else if (isSelectTrack_ == true) {
        TrackType type = demuxerPluginManager_->GetTrackTypeByTrackID(selectTrackTrackID_);
        if (type == TrackType::TRACK_AUDIO) {
            HandleStopPlugin(videoTrackId_);
            HandleStopPlugin(subtitleTrackId_);
        } else if (type == TrackType::TRACK_VIDEO) {
            HandleStopPlugin(audioTrackId_);
            HandleStopPlugin(subtitleTrackId_);
        } else if (type == TrackType::TRACK_SUBTITLE) {
            HandleStopPlugin(videoTrackId_);
            HandleStopPlugin(audioTrackId_);
        } else {
            // invalid
        }
    } else {
        MEDIA_LOG_I("SeekTo source SeekToTime stop all plugin");
        HandleStopPlugin(videoTrackId_);
        HandleStopPlugin(audioTrackId_);
        HandleStopPlugin(subtitleTrackId_);
        streamDemuxer_->ResetAllCache();
    }
    return Status::OK;
}

Status MediaDemuxer::SeekToTimeAfter()
{
    if (demuxerPluginManager_->IsDash() == false) {
        return Status::OK;
    }

    if (isSelectBitRate_ == true) {
        HandleStartPlugin(audioTrackId_);
    } else if (isSelectTrack_ == true) {
        TrackType type = demuxerPluginManager_->GetTrackTypeByTrackID(selectTrackTrackID_);
        if (type == TrackType::TRACK_AUDIO) {
            HandleStartPlugin(videoTrackId_);
            HandleStartPlugin(subtitleTrackId_);
            if (shouldCheckAudioFramePts_) {
                shouldCheckAudioFramePts_ = false;
            }
        } else if (type == TrackType::TRACK_VIDEO) {
            HandleStartPlugin(audioTrackId_);
            HandleStartPlugin(subtitleTrackId_);
        } else if (type == TrackType::TRACK_SUBTITLE) {
            HandleStartPlugin(audioTrackId_);
            HandleStartPlugin(videoTrackId_);
            if (shouldCheckSubtitleFramePts_) {
                shouldCheckSubtitleFramePts_ = false;
            }
        } else {
            // invalid
        }
    } else {
        HandleStartPlugin(audioTrackId_);
        HandleStartPlugin(videoTrackId_);
        HandleStartPlugin(subtitleTrackId_);
    }

    return Status::OK;
}

Status MediaDemuxer::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    Status ret;
    isSeekError_.store(false);
    if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
        MEDIA_LOG_D("SeekTo source SeekToTime start");
        SeekToTimePre();
        if (mode == SeekMode::SEEK_CLOSEST_INNER) {
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_PREVIOUS_SYNC);
        } else {
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_CLOSEST_SYNC);
        }
        if (subtitleSource_) {
            demuxerPluginManager_->localSubtitleSeekTo(seekTime);
        }
        SeekToTimeAfter();
        Plugins::Ms2HstTime(seekTime, realSeekTime);
    } else {
        MEDIA_LOG_D("SeekTo start");
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
    MEDIA_LOG_D("SeekTo done");
    isFirstFrameAfterSeek_.store(true);
    return ret;
}

Status MediaDemuxer::SelectBitRate(uint32_t bitRate)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "SelectBitRate failed, source_ is nullptr.");
    MEDIA_LOG_I("SelectBitRate begin");
    if (demuxerPluginManager_->IsDash()) {
        if (streamDemuxer_->CanDoChangeStream() == false) {
            MEDIA_LOG_W("SelectBitRate or SelectTrack is running, can not SelectBitRate");
            return Status::OK;
        }
        if (bitRate == demuxerPluginManager_->GetCurrentBitRate()) {
            MEDIA_LOG_W("same bitrate, can not SelectBitRate");
            return Status::OK;
        }
        isSelectBitRate_.store(true);
    }
    streamDemuxer_->ResetAllCache();
    Status ret = source_->SelectBitRate(bitRate);
    if (ret != Status::OK) {
        MEDIA_LOG_E("MediaDemuxer SelectBitRate failed");
        if (demuxerPluginManager_->IsDash()) {
            isSelectBitRate_.store(false);
        }
    }
    MEDIA_LOG_I("SelectBitRate success");
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
    MEDIA_LOG_I("StopAllTask done.");
    return Status::OK;
}

Status MediaDemuxer::PauseAllTask()
{
    MEDIA_LOG_I("PauseAllTask enter.");
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
    MEDIA_LOG_I("PauseAllTask done.");
    return Status::OK;
}

Status MediaDemuxer::ResumeAllTask()
{
    MEDIA_LOG_I("ResumeAllTask enter.");
    isDemuxerLoopExecuting_ = true;
    streamDemuxer_->SetIsIgnoreParse(false);

    auto it = bufferQueueMap_.begin();
    while (it != bufferQueueMap_.end()) {
        uint32_t trackId = it->first;
        if (taskMap_[trackId] == nullptr) {
            MEDIA_LOG_W("track " PUBLIC_LOG_U32 " task is not exist, start failed.", trackId);
        } else {
            taskMap_[trackId]->Start();
        }
        it++;
    }
    MEDIA_LOG_I("ResumeAllTask done.");
    return Status::OK;
}

Status MediaDemuxer::PauseForPrepareFrame()
{
    MEDIA_LOG_I("PauseForPrepareFrame");
    isPaused_ = true;
    if (streamDemuxer_ != nullptr) {
        streamDemuxer_->SetIsIgnoreParse(true);
        streamDemuxer_->Pause();
    }
    if (source_ != nullptr) {
        source_->SetReadBlockingFlag(false); // Disable source read blocking to prevent pause all task blocking
        source_->Pause();
    }
    if (taskMap_[videoTrackId_] != nullptr) {
        taskMap_[videoTrackId_]->PauseAsync();
        taskMap_[videoTrackId_]->Pause();
    }
    if (source_ != nullptr) {
        source_->SetReadBlockingFlag(true); // Enable source read blocking to ensure get wanted data
    }
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
    if (!doPrepareFrame_) {
        ResumeAllTask();
    } else {
        if (taskMap_[videoTrackId_] != nullptr) {
            streamDemuxer_->SetIsIgnoreParse(false);
            taskMap_[videoTrackId_]->Start();
        }
    }
    isPaused_ = false;
    return Status::OK;
}

Status MediaDemuxer::ResumeDragging()
{
    MEDIA_LOG_I("Resume");
    if (streamDemuxer_) {
        streamDemuxer_->Resume();
    }
    if (source_) {
        source_->Resume();
    }
    if (taskMap_[videoTrackId_] != nullptr) {
        streamDemuxer_->SetIsIgnoreParse(false);
        taskMap_[videoTrackId_]->Start();
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
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
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
    isDemuxerLoopExecuting_ = true;
    if (!doPrepareFrame_) {
        auto it = bufferQueueMap_.begin();
        while (it != bufferQueueMap_.end()) {
            uint32_t trackId = it->first;
            if (taskMap_[trackId] == nullptr) {
                MEDIA_LOG_W("track " PUBLIC_LOG_U32 " task is not exist, start failed.", trackId);
            } else {
                taskMap_[trackId]->Start();
            }
            it++;
        }
    } else {
        if (taskMap_[videoTrackId_] != nullptr) {
            taskMap_[videoTrackId_]->Start();
        }
    }
    MEDIA_LOG_I("Demuxer thread started.");
    source_->Start();
    return demuxerPluginManager_->Start();
}

Status MediaDemuxer::Stop()
{
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    FALSE_RETURN_V_MSG_E(!isThreadExit_, Status::OK, "Process has been stopped already, need to start if first.");
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::Stop");
    MEDIA_LOG_I("MediaDemuxer Stop.");
    {
        AutoLock lock(mapMutex_);
        FALSE_RETURN_V_MSG_E(!isStopped_, Status::OK, "Process has been stopped already, ignore.");
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

Status MediaDemuxer::PrepareFrame(bool renderFirstFrame)
{
    MEDIA_LOG_D("PrepareFrame enter.");
    doPrepareFrame_ = true;
    Status ret = Status::OK;
    if (isStopped_) {
        MEDIA_LOG_D("Stop was executed before and PrepareFrame.");
        firstFrameCount_ = 0;
        ret = Start();
    } else if (!isThreadExit_ || (firstFrameCount_ >= DEFAULT_PREPARE_FRAME_COUNT) || waitForDataFail_) {
        waitForDataFail_ = false;
        firstFrameCount_ = 0;
        ret = Resume();
    } else {
        ret = Start();
    }
    if (ret != Status::OK) {
        MEDIA_LOG_E("PrepareFrame and start demuxer failed.");
        doPrepareFrame_ = false;
        return ret;
    }
    AutoLock lock(firstFrameMutex_);
    bool res = firstFrameCond_.WaitFor(lock, LOCK_WAIT_TIME, [this] {
         return firstFrameCount_ >= DEFAULT_PREPARE_FRAME_COUNT;
    });
    MEDIA_LOG_I("PrepareFrame res= %{public}d", res);
    doPrepareFrame_ = false;
    if (!res) {
        waitForDataFail_ = true;
        MEDIA_LOG_I("demuxer is timeout and not enough data");
    }
    return PauseForPrepareFrame();
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
    for (uint32_t index = 0; index < mediaInfo.tracks.size(); index++) {
        if (demuxerPluginManager_->CheckTrackIsActive(index) == false) {
            continue;
        }
        auto trackMeta = mediaInfo.tracks[index];
        std::string mimeType;
        bool ret = trackMeta.Get<Tag::MIME_TYPE>(mimeType);
        if (ret && mimeType.find("video") == 0 &&
            !IsTrackDisabled(Plugins::MediaType::VIDEO)) {
            MEDIA_LOG_I("Found video track, id: " PUBLIC_LOG_U32 ", mimeType: " PUBLIC_LOG_S, index, mimeType.c_str());
            videoMime = mimeType;
            if (videoTrackId == TRACK_ID_DUMMY) {
                videoTrackId = index;
            }
            if (!trackMeta.GetData(Tag::MEDIA_START_TIME, videoStartTime_)) {
                MEDIA_LOG_W("Get media start time failed");
            }
        } else if (ret && mimeType.find("audio") == 0 &&
            !IsTrackDisabled(Plugins::MediaType::AUDIO)) {
            MEDIA_LOG_I("Found audio track, id: " PUBLIC_LOG_U32 ", mimeType: " PUBLIC_LOG_S, index, mimeType.c_str());
            if (audioTrackId == TRACK_ID_DUMMY) {
                audioTrackId = index;
            }
        } else if (ret && IsSubtitleMime(mimeType) &&
            !IsTrackDisabled(Plugins::MediaType::SUBTITLE)) {
            MEDIA_LOG_I("Found subtitle track, id: " PUBLIC_LOG_U32 ", mimeType: " PUBLIC_LOG_S, index,
                mimeType.c_str());
            if (subtitleTrackId == TRACK_ID_DUMMY) {
                subtitleTrackId = index;
            }
        } else {}
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
        if (requestBufferErrorCountMap_[queueIndex] % 5 == 0) { // log per 5 times fail
            MEDIA_LOG_W("Request buffer failed, try again later, user queue: " PUBLIC_LOG_U32 ", ret: " PUBLIC_LOG_D32
                ", errorCnt:" PUBLIC_LOG_D32, queueIndex, (int32_t)(ret), requestBufferErrorCountMap_[queueIndex]);
        }
        if (requestBufferErrorCountMap_[queueIndex] >= REQUEST_FAILED_RETRY_TIMES) {
            MEDIA_LOG_E("Request buffer failed from buffer queue too many times in one minute.");
        }
    } else {
        requestBufferErrorCountMap_[queueIndex] = 0;
    }
    return ret == Status::OK;
}

bool MediaDemuxer::HandleSelectTrackChangeStream(int32_t trackId, int32_t newStreamID)
{
    StreamType streamType = demuxerPluginManager_->GetStreamTypeByTrackID(selectTrackTrackID_);
    int32_t currentStreamID = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
    int32_t currentTrackId = trackId;
    if (newStreamID == -1 || currentStreamID == newStreamID) {
        return false;
    }
    MEDIA_LOG_I("HandleSelectTrackChangeStream dash, begin");
    // stop plugin
    demuxerPluginManager_->StopPlugin(currentStreamID, streamDemuxer_);

    // start plugin
    Status ret = demuxerPluginManager_->StartPlugin(newStreamID, streamDemuxer_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "HandleSelectTrackChangeStream StartPlugin failed.");

    // get new mediainfo
    Plugins::MediaInfo mediaInfo;
    demuxerPluginManager_->UpdateDefaultStreamID(mediaInfo, streamType, newStreamID);
    InitMediaMetaData(mediaInfo); // update mediaMetaData_

    // update track map
    demuxerPluginManager_->DeleteTempTrackMapInfo(currentTrackId);
    int32_t innerTrackID = demuxerPluginManager_->GetInnerTrackIDByTrackID(selectTrackTrackID_);
    demuxerPluginManager_->UpdateTempTrackMapInfo(selectTrackTrackID_, selectTrackTrackID_, innerTrackID);
    MEDIA_LOG_I("HandleSelectTrackChangeStream dash, UpdateTempTrackMapInfo done");

    InnerSelectTrack(selectTrackTrackID_);

    // update buffer queue
    bufferQueueMap_.insert(std::pair<uint32_t, sptr<AVBufferQueueProducer>>(selectTrackTrackID_,
        bufferQueueMap_[currentTrackId]));
    bufferMap_.insert(std::pair<uint32_t, std::shared_ptr<AVBuffer>>(selectTrackTrackID_,
        bufferMap_[currentTrackId]));
    bufferQueueMap_.erase(currentTrackId);
    bufferMap_.erase(currentTrackId);

    MEDIA_LOG_I("HandleSelectTrackChangeStream dash, end");
    return true;
}

bool MediaDemuxer::SelectTrackChangeStream(uint32_t trackId)
{
    TrackType type = demuxerPluginManager_->GetTrackTypeByTrackID(selectTrackTrackID_);
    int32_t newStreamID = -1;
    uint32_t oldTrackId = -1;
    if (type == TRACK_AUDIO) {
        newStreamID = streamDemuxer_->GetNewAudioStreamID();
        oldTrackId = audioTrackId_;
    } else if (type == TRACK_SUBTITLE) {
        newStreamID = streamDemuxer_->GetNewSubtitleStreamID();
        oldTrackId = subtitleTrackId_;
    } else if (type == TRACK_VIDEO) {
        newStreamID = streamDemuxer_->GetNewVideoStreamID();
        oldTrackId = videoTrackId_;
    } else {
        MEDIA_LOG_W("SelectTrackChangeStream invalid trackId = " PUBLIC_LOG_U32, trackId);
        return false;
    }

    if (trackId != oldTrackId) {
        return false;
    }
    bool ret = HandleSelectTrackChangeStream(trackId, newStreamID);
    if (ret) {
        if (type == TrackType::TRACK_AUDIO) {
            audioTrackId_ = selectTrackTrackID_;
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_AUDIO_TRACK_CHANGE, selectTrackTrackID_});
            shouldCheckAudioFramePts_ = true;
        } else if (type == TrackType::TRACK_VIDEO) {
            videoTrackId_ = selectTrackTrackID_;
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_VIDEO_TRACK_CHANGE, selectTrackTrackID_});
        } else if (type == TrackType::TRACK_SUBTITLE) {
            subtitleTrackId_ = selectTrackTrackID_;
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_SUBTITLE_TRACK_CHANGE, selectTrackTrackID_});
            shouldCheckSubtitleFramePts_ = true;
        }
        taskMap_[trackId]->StopAsync();   // stop self
    }
    return ret;
}

bool MediaDemuxer::SelectBitRateChangeStream(uint32_t trackId)
{
    int32_t currentStreamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
    int32_t newStreamID = streamDemuxer_->GetNewVideoStreamID();
    if (newStreamID >= 0 && currentStreamID != newStreamID) {
        MEDIA_LOG_I("SelectBitRateChangeStream dash, begin");
        demuxerPluginManager_->StopPlugin(currentStreamID, streamDemuxer_);

        Status ret = demuxerPluginManager_->StartPlugin(newStreamID, streamDemuxer_);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "SelectTrackChangeStream StartPlugin failed.");

        Plugins::MediaInfo mediaInfo;
        demuxerPluginManager_->UpdateDefaultStreamID(mediaInfo, VIDEO, newStreamID);
        InitMediaMetaData(mediaInfo); // update mediaMetaData_
        
        int32_t newInnerTrackId = -1;
        int32_t newTrackId = -1;
        demuxerPluginManager_->GetTrackInfoByStreamID(newStreamID, newTrackId, newInnerTrackId);
        demuxerPluginManager_->UpdateTempTrackMapInfo(videoTrackId_, newTrackId, newInnerTrackId);

        MEDIA_LOG_I("SelectBitRateChangeStream dash, UpdateTempTrackMapInfo done");
        InnerSelectTrack(static_cast<int32_t>(trackId));
        MEDIA_LOG_I("SelectBitRateChangeStream dash, end");
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
if (trackId == videoTrackId_ && VideoStreamReadyCallback_ != nullptr) {
        MEDIA_LOG_D("step into HandleRead");
        bool isDiscardable = VideoStreamReadyCallback_->IsVideoStreamDiscardable(bufferMap_[trackId]);
        bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], !isDiscardable);
        return Status::OK;
    }

    if (source_ != nullptr && source_->IsSeekToTimeSupported() && isSeeked_ && HasVideo()) {
        if (trackId == videoTrackId_ && isFirstFrameAfterSeek_.load()) {
            bool isSyncFrame = (bufferMap_[trackId]->flag_ & (uint32_t)(AVBufferFlag::SYNC_FRAME)) != 0;
            if (!isSyncFrame) {
                MEDIA_LOG_E("The first frame after seeking is not a sync frame.");
            }
            isFirstFrameAfterSeek_.store(false);
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

    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    int32_t innerTrackID = static_cast<int32_t>(trackId);
    int32_t id = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        if (demuxerPluginManager_->IsDash() || demuxerPluginManager_->GetTmpStreamIDByTrackID(subtitleTrackId_) != -1) {
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(id);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "CopyFrameToUserQueue failed due to get demuxer plugin failed.");
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(id);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "CopyFrameToUserQueue failed due to get demuxer plugin failed.");
    }
    
    int32_t size = 0;
    Status ret = pluginTemp->GetNextSampleSize(innerTrackID, size);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_UNKNOWN, Status::ERROR_UNKNOWN,
        "CopyFrameToUserQueue error for track " PUBLIC_LOG_U32, trackId);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_AGAIN, Status::ERROR_AGAIN,
        "CopyFrameToUserQueue error for track " PUBLIC_LOG_U32 ", try again", trackId);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_NO_MEMORY, Status::ERROR_NO_MEMORY,
        "CopyFrameToUserQueue error for track " PUBLIC_LOG_U32, trackId);

    if (demuxerPluginManager_->IsDash()) {
        if (isSelectBitRate_ && (trackId == videoTrackId_)) {
            auto result = SelectBitRateChangeStream(trackId);
            if (result) {
                streamDemuxer_->SetChangeFlag(true);
                isSelectBitRate_.store(false);
                return Status::OK;
            }
        } else if (isSelectTrack_) {
            auto result = SelectTrackChangeStream(trackId);
            if (result) {
                streamDemuxer_->SetChangeFlag(true);
                isSelectTrack_.store(false);
                return Status::OK;
            }
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
    if (demuxerPluginManager_->IsDash() || demuxerPluginManager_->GetTmpStreamIDByTrackID(subtitleTrackId_) != -1) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "InnerReadSample failed due to get demuxer plugin failed.");
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(demuxerPluginManager_->GetStreamIDByTrackID(trackId));
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
            "InnerReadSample failed due to get demuxer plugin failed.");
    }

    Status ret = pluginTemp->ReadSample(innerTrackID, sample);
    if (ret == Status::END_OF_STREAM) {
        MEDIA_LOG_I("Read buffer eos for track " PUBLIC_LOG_U32, trackId);
    } else if (ret == Status::ERROR_NO_MEMORY) {
        MEDIA_LOG_I("Read buffer error for track " PUBLIC_LOG_U32 ", ret: " PUBLIC_LOG_D32,
            trackId, static_cast<int32_t>(ret));
        if (eventReceiver_ != nullptr) {
            eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_NO_MEMORY});
        }
    } else if (ret != Status::OK) {
        MEDIA_LOG_I("Read buffer error for track " PUBLIC_LOG_U32 ", ret: " PUBLIC_LOG_D32, trackId, (int32_t)(ret));
    }
    MEDIA_LOG_D("finish copy frame for track " PUBLIC_LOG_U32, trackId);

    // to get DrmInfo
    ProcessDrmInfos();
    return ret;
}

int64_t MediaDemuxer::ReadLoop(uint32_t trackId)
{
    if (streamDemuxer_->GetIsIgnoreParse() || isStopped_ || isPaused_ || isSeekError_) {
        MEDIA_LOG_D("ReadLoop pausing or error, copy frame for track " PUBLIC_LOG_U32, trackId);
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
        } else if (ret == Status::ERROR_NO_MEMORY) {
            MEDIA_LOG_E("cache data size is greater than cache limit size");
            taskMap_[trackId]->Pause();
            if (eventReceiver_ != nullptr) {
                eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_NO_MEMORY});
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
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE,
        "Cannot call read frame when use buffer queue.");
    MEDIA_LOG_D("Read next sample");
    FALSE_RETURN_V_MSG_E(eosMap_.count(trackId) > 0, Status::ERROR_INVALID_OPERATION,
        "Read sample failed due to track has not been selected");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_!=nullptr, Status::ERROR_INVALID_PARAMETER,
        "Read Sample failed due to input sample is nullptr");
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
    MEDIA_LOG_I("HandleSourceDrmInfoEvent");
    std::multimap<std::string, std::vector<uint8_t>> infoUpdated;
    bool isUpdated = GetDrmInfosUpdated(info, infoUpdated);
    if (isUpdated) {
        ReportDrmInfos(infoUpdated);
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
        case PluginEventType::CACHED_DURATION: {
            MEDIA_LOG_D("OnEvent cached duration.");
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

void MediaDemuxer::CheckDropAudioFrame(std::shared_ptr<AVBuffer> sample, uint32_t trackId)
{
    if (trackId == audioTrackId_) {
        if (shouldCheckAudioFramePts_ == false) {
            lastAudioPts_ = sample->pts_;
            MEDIA_LOG_I("set lastAudioPts_ = " PUBLIC_LOG_U64, lastAudioPts_);
            return;
        }
        if (sample->pts_ < lastAudioPts_) {
            MEDIA_LOG_I("drop audio buffer, pts = " PUBLIC_LOG_U64, sample->pts_);
            return;
        }
        if (shouldCheckAudioFramePts_) {
            shouldCheckAudioFramePts_ = false;
        }
    }
    if (trackId == subtitleTrackId_) {
        if (shouldCheckSubtitleFramePts_ == false) {
            lastSubtitlePts_ = sample->pts_;
            MEDIA_LOG_I("set lastSubtitlePts_ = " PUBLIC_LOG_U64, lastSubtitlePts_);
            return;
        }
        if (sample->pts_ < lastSubtitlePts_) {
            MEDIA_LOG_I("drop subtitle buffer, pts = " PUBLIC_LOG_U64, sample->pts_);
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

bool MediaDemuxer::CanAutoSelectBitRate()
{
    // calculating auto selectbitrate time
    return !(isSelectBitRate_.load()) && !(isSelectTrack_.load());
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
    MEDIA_LOG_D("GetIndexByRelativePresentationTimeUs");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER,
        "GetIndexByRelativePresentationTimeUs failed due to demuxerPluginManager_ is nullptr.");
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_NULL_POINTER,
        "GetIndexByRelativePresentationTimeUs failed due to get demuxer plugin failed.");

    Status ret = pluginTemp->GetIndexByRelativePresentationTimeUs(trackIndex, relativePresentationTimeUs, index);
    if (ret != Status::OK) {
        MEDIA_LOG_E("MediaDemuxer GetIndexByRelativePresentationTimeUs failed");
    }
    return ret;
}

Status MediaDemuxer::GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
    const uint32_t index, uint64_t &relativePresentationTimeUs)
{
    MEDIA_LOG_D("GetRelativePresentationTimeUsByIndex");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER,
        "GetRelativePresentationTimeUsByIndex failed due to demuxerPluginManager_ is nullptr.");
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_NULL_POINTER,
        "GetRelativePresentationTimeUsByIndex failed due to get demuxer plugin failed.");

    Status ret = pluginTemp->GetRelativePresentationTimeUsByIndex(trackIndex, index, relativePresentationTimeUs);
    if (ret != Status::OK) {
        MEDIA_LOG_E("MediaDemuxer GetRelativePresentationTimeUsByIndex failed");
    }
    return ret;
}

Status MediaDemuxer::ResumeDemuxerReadLoop()
{
    MEDIA_LOG_I("ResumeDemuxerReadLoop in.");
    if (isDemuxerLoopExecuting_) {
        MEDIA_LOG_I("Has already resumed");
        return Status::OK;
    }
    isDemuxerLoopExecuting_ = true;
    return ResumeAllTask();
}

Status MediaDemuxer::PauseDemuxerReadLoop()
{
    MEDIA_LOG_I("PauseDemuxerReadLoop in.");
    if (!isDemuxerLoopExecuting_) {
        MEDIA_LOG_I("Has already pause");
        return Status::OK;
    }
    isDemuxerLoopExecuting_ = false;
    return PauseAllTask();
}

void MediaDemuxer::SetCacheLimit(uint32_t limitSize)
{
    MEDIA_LOG_D("SetCacheLimit");
    FALSE_RETURN_MSG(demuxerPluginManager_ != nullptr, "SetCacheLimit failed due to demuxerPluginManager_ is nullptr.");
    int32_t tempTrackId = (videoTrackId_ != TRACK_ID_DUMMY ? static_cast<int32_t>(videoTrackId_) : -1);
    tempTrackId = (tempTrackId == -1 ? static_cast<int32_t>(audioTrackId_) : tempTrackId);
    int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(tempTrackId);
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
    FALSE_RETURN_MSG(pluginTemp != nullptr, "SetCacheLimit failed due to get demuxer plugin failed.");

    pluginTemp->SetCacheLimit(limitSize);
}

bool MediaDemuxer::IsVideoEos()
{
    if (videoTrackId_ == TRACK_ID_DUMMY) {
        return true;
    }
    return eosMap_[videoTrackId_];
}
} // namespace Media
} // namespace OHOS
