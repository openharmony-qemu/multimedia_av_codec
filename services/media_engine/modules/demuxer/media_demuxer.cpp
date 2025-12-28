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
#define HST_LOG_TAG "MediaDemuxer"
#define MEDIA_ATOMIC_ABILITY

#include "media_demuxer.h"

#include <algorithm>
#include <map>
#include <memory>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>

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
#include "plugin/plugin_info.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_list.h"
#include "source/source.h"
#include "stream_demuxer.h"
#include "media_core.h"
#include "osal/utils/dump_buffer.h"
#include "demuxer_plugin_manager.h"
#include "media_demuxer_pts_functions.cpp"
#include "avcodec_log.h"
#include "scoped_timer.h"
#include "param_wrapper.h"
#include "parameters.h"
#include "scope_guard.h"
#include "syspara/parameters.h"

namespace {
const std::string DUMP_PARAM = "a";
const std::string DUMP_DEMUXER_AUDIO_FILE_NAME = "player_demuxer_audio_output.es";
const std::string DUMP_DEMUXER_VIDEO_FILE_NAME = "player_demuxer_video_output.es";
const std::string DEMUXER_PLUGIN_NAME_HEADER = "avdemux_";
const char DEMUXER_PLUGIN_NAME_DELIMITER = ',';
static constexpr char PERFORMANCE_STATS[] = "PERFORMANCE";
static constexpr int32_t INVALID_STREAM_OR_TRACK_ID = -1;
static constexpr int32_t SKIP_NEXT_OPEN_GOP_CNT = 2;
constexpr uint32_t THREAD_PRIORITY_41 = 7;
constexpr uint32_t MAX_VIDEO_LEAD_TIME_ON_MUTE_US = 34000; // Maximum video frame advance time during video mute
constexpr uint32_t SAMPLE_QUEUE_SIZE_ON_MUTE = 50; // After video mute, sampleSize increases to 50
constexpr uint32_t SAMPLE_QUEUE_ADD_SIZE_ON_MUTE = 20; // When samplequeue is full on mute, samplequeue size add 20
std::map<OHOS::Media::TrackType, OHOS::Media::StreamType> TRACK_TO_STREAM_MAP = {
    {OHOS::Media::TrackType::TRACK_VIDEO, OHOS::Media::StreamType::VIDEO},
    {OHOS::Media::TrackType::TRACK_AUDIO, OHOS::Media::StreamType::AUDIO},
    {OHOS::Media::TrackType::TRACK_SUBTITLE, OHOS::Media::StreamType::SUBTITLE},
    {OHOS::Media::TrackType::TRACK_INVALID, OHOS::Media::StreamType::MIXED}
};
} // namespace

namespace OHOS {
namespace Media {
constexpr uint32_t REQUEST_BUFFER_TIMEOUT = 0; // Requesting buffer overtimes 0ms means no retry
constexpr int32_t START = 1;
constexpr int32_t PAUSE = 2;
constexpr bool SEEK_TO_EOS = true;
constexpr uint32_t RETRY_DELAY_TIME_US = 100000; // 100ms, Delay time for RETRY if no buffer in avbufferqueue producer.
constexpr uint32_t NEXT_DELAY_TIME_US = 10; // 10us is ok
constexpr uint32_t SAMPLE_LOOP_RETRY_TIME_US = 20000;
constexpr uint32_t SAMPLE_LOOP_DELAY_TIME_US = 100000;
constexpr uint32_t SAMPLE_FLOW_CONTROL_MIN_SAMPLE_DURATION_US = 200000;
constexpr uint32_t SAMPLE_FLOW_CONTROL_RATE_POW = 6; // 2^6
constexpr int64_t UPDATE_SOURCE_CACHE_MS = 100;

constexpr uint32_t BUFFERING_WAVELINE_FOR_SAMPLE_QUEUE = 1000000;
constexpr double DECODE_RATE_THRESHOLD = 0.05;   // allow actual rate exceeding 5%
constexpr uint32_t REQUEST_FAILED_RETRY_TIMES = 12000; // Max times for RETRY if no buffer in avbufferqueue producer.
constexpr uint32_t SAMPLE_LOOP_ACQUIRE_FAILED_LOG_POW2 = 3;
constexpr uint32_t SAMPLE_LOOP_REQUEST_FAILED_LOG_POW2 = 8;
constexpr int32_t US_TO_S = 1000000;
constexpr int32_t US_TO_MS = 1000;
constexpr int64_t SEEK_ONLINE_WARNING_MS = 600;
constexpr int64_t SEEKCLOSEST_ONLINE_WARNING_MS = 800;
constexpr int64_t SEEK_LOCAL_WARNING_MS = 78;
constexpr int64_t SEEKCLOSEST_LOCAL_WARNING_MS = 309;
constexpr int64_t READSAMPLE_AUIDO_WARNING_MS = 50;
constexpr int64_t READSAMPLE_WARNING_MS = 100;
constexpr int32_t CONVERT_PACKET_ERROR_MAX_COUNT = 30;
const std::unordered_map<PluginDfxEventType, std::pair<std::string, DfxEventType>> DFX_EVENT_MAP = {
    { PluginDfxEventType::PERF_SOURCE, { "SRC", DfxEventType::DFX_INFO_PERF_REPORT } }
};
constexpr uint32_t LIMIT_MEMORY_REPORT_COUNT = 1000;
constexpr int32_t DFX_BUFFER_QUEUE_SIZE_MAX = 50;
constexpr int64_t LOG_INTERVAL_MS = 2000; // 2s
constexpr uint32_t LOG_MAX_COUNT = 10; // 10 times

static const std::map<TrackType, DemuxerTrackType> TRACK_MAP = {
    {TrackType::TRACK_AUDIO, DemuxerTrackType::AUDIO},
    {TrackType::TRACK_VIDEO, DemuxerTrackType::VIDEO},
    {TrackType::TRACK_SUBTITLE, DemuxerTrackType::SUBTITLE},
};
 
static const std::map<DemuxerTrackType, std::string> TRACK_SUFFIX_MAP = {
    {DemuxerTrackType::VIDEO, "V"},
    {DemuxerTrackType::AUDIO, "A"},
    {DemuxerTrackType::SUBTITLE, "S"},
};
enum SceneCode : int32_t {
    /**
     * This option is used to mark parser ref for dragging play scene.
     */
    AV_META_SCENE_PARSE_REF_FOR_DRAGGING_PLAY = 3 // scene code of parser ref for dragging play is 3
};

std::unordered_set<FileType> ptsManagedFileTypes = {
    FileType::AVI,
    FileType::MPEGPS,
    FileType::WMV
};

class MediaDemuxer::AVBufferQueueProducerListener : public IRemoteStub<IProducerListener> {
public:
    explicit AVBufferQueueProducerListener(int32_t trackId, std::shared_ptr<MediaDemuxer> demuxer,
        std::unique_ptr<Task>& notifyTask) : trackId_(trackId), demuxer_(demuxer), notifyTask_(std::move(notifyTask)) {}

    virtual ~AVBufferQueueProducerListener() = default;
    int OnRemoteRequest(uint32_t code, MessageParcel& arguments, MessageParcel& reply, MessageOption& option) override
    {
        return IPCObjectStub::OnRemoteRequest(code, arguments, reply, option);
    }
    void OnBufferAvailable() override
    {
        MEDIA_LOG_DD("Buffer available for track " PUBLIC_LOG_D32, trackId_);
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
    int32_t trackId_{0};
    std::weak_ptr<MediaDemuxer> demuxer_;
    std::unique_ptr<Task> notifyTask_;
};

class MediaDemuxer::TrackWrapper {
public:
    explicit TrackWrapper(int32_t trackId, sptr<IProducerListener> listener, std::shared_ptr<MediaDemuxer> demuxer)
        : trackId_(trackId), listener_(listener), demuxer_(demuxer)
    {
        MEDIA_LOG_D("TrackWrapper TrackId:" PUBLIC_LOG_U32, trackId_);
    }

    sptr<IProducerListener> GetProducerListener()
    {
        return listener_;
    }
    void SetNotifyFlag(bool isNotifyNeeded)
    {
        isNotifyNeeded_ = isNotifyNeeded;
        MEDIA_LOG_DD("TrackId:" PUBLIC_LOG_D32 ", isNotifyNeeded:" PUBLIC_LOG_D32,
            trackId_, isNotifyNeeded);
    }
    bool GetNotifyFlag()
    {
        return isNotifyNeeded_.load();
    }

    void SetNotifySampleConsumerFlag(bool isNotifySampleConsumerNeeded)
    {
        isNotifySampleConsumerNeeded_ = isNotifySampleConsumerNeeded;
        MEDIA_LOG_DD("TrackId:" PUBLIC_LOG_D32 ", isNotifySampleConsumerNeeded:" PUBLIC_LOG_D32,
            trackId_, isNotifySampleConsumerNeeded);
    }

    bool GetNotifySampleConsumerFlag() const
    {
        return isNotifySampleConsumerNeeded_.load();
    }
private:
    std::atomic<bool> isNotifyNeeded_{false};
    std::atomic<bool> isNotifySampleConsumerNeeded_{false};
    int32_t trackId_{0};
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
      sampleQueueMap_(),
      eventReceiver_(),
      streamDemuxer_(),
      demuxerPluginManager_(std::make_shared<DemuxerPluginManager>()),
      sampleQueueController_(std::make_shared<SampleQueueController>())
{
    MEDIA_LOG_D("In");
    InitEnableSampleQueueFlag();
    InitEnableDfxBufferQueue();

    funcBeforeReadSampleMap_.emplace(TrackType::TRACK_SUBTITLE,
        [this](int32_t trackId) { return DoBeforeSubtitleTrackReadLoop(trackId); });
    std::string enableAsyncDemuxer;
    int32_t result = OHOS::system::GetStringParameter("sys.media.enable.async.demuxer", enableAsyncDemuxer, "");
    if (result == 0 && !enableAsyncDemuxer.empty() && enableAsyncDemuxer == "false") {
        enableAsyncDemuxer_ = false;
    }
}

MediaDemuxer::~MediaDemuxer()
{
    MEDIA_LOG_D("In");
    ResetInner();
    if (parserRefInfoTask_ != nullptr) {
        parserRefInfoTask_->Stop();
        parserRefInfoTask_ = nullptr;
    }
    demuxerPluginManager_ = nullptr;
    sampleQueueController_ = nullptr;
    source_ = nullptr;
    eventReceiver_ = nullptr;
    eosMap_.clear();
    segmentEosMap_.clear();
    requestBufferErrorCountMap_.clear();
    streamDemuxer_ = nullptr;
    localDrmInfos_.clear();
}

std::shared_ptr<Plugins::DemuxerPlugin> MediaDemuxer::GetCurFFmpegPlugin()
{
    int32_t tempTrackId = (IsValidTrackId(videoTrackId_) ? videoTrackId_ : audioTrackId_);
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
    FALSE_RETURN_V_MSG_E(IsValidTrackId(videoTrackId_), false, "Video track is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> videoPlugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(videoPlugin != nullptr, false, "Demuxer plugin is nullptr");
    return videoPlugin->IsRefParserSupported();
}

Status MediaDemuxer::StartReferenceParser(int64_t startTimeMs, bool isForward)
{
    FALSE_RETURN_V_MSG_E(startTimeMs >= 0, Status::ERROR_UNKNOWN,
                         "Start failed, startTimeMs: " PUBLIC_LOG_D64, startTimeMs);
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(IsValidTrackId(videoTrackId_), Status::ERROR_UNKNOWN, "Video track is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    Status ret = plugin->ParserRefUpdatePos(startTimeMs, isForward);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ParserRefUpdatePos failed");
    if (isFirstParser_) {
        isFirstParser_ = false;
        FALSE_RETURN_V_MSG(source_->GetSeekable() == Plugins::Seekable::SEEKABLE,
            Status::ERROR_INVALID_OPERATION, "Unsupport online video");
        parserRefInfoTask_ = std::make_unique<Task>("ParserRefInfo", playerId_);
        parserRefInfoTask_->RegisterJob([this] { return ParserRefInfo(); });
        ReportSceneCodeForDemuxer(SceneCode::AV_META_SCENE_PARSE_REF_FOR_DRAGGING_PLAY);
        parserRefInfoTask_->Start();
    }
    TryReclaimParserTask();
    return ret;
}

void MediaDemuxer::TryReclaimParserTask()
{
    std::lock_guard<std::mutex> lock(parserTaskMutex_);
    if (isParserTaskEnd_ && parserRefInfoTask_ != nullptr) {
        parserRefInfoTask_->Stop();
        parserRefInfoTask_ = nullptr;
        MEDIA_LOG_I("Success to reclaim parser task");
    }
}

int64_t MediaDemuxer::ParserRefInfo()
{
    FALSE_RETURN_V_MSG_D(demuxerPluginManager_ != nullptr, 0, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_D(plugin != nullptr, 0, "Demuxer plugin is nullptr");
    Status ret = plugin->ParserRefInfo();
    std::lock_guard<std::mutex> lock(parserTaskMutex_);
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
    FALSE_RETURN_V_MSG_E(videoSample != nullptr, Status::ERROR_NULL_POINTER, "VideoSample is nullptr");
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryReclaimParserTask();
    Status ret = plugin->GetFrameLayerInfo(videoSample, frameLayerInfo);
    if (ret == Status::ERROR_NULL_POINTER && parserRefInfoTask_ != nullptr) {
        return Status::ERROR_AGAIN;
    }
    return ret;
}

Status MediaDemuxer::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryReclaimParserTask();
    Status ret = plugin->GetFrameLayerInfo(frameId, frameLayerInfo);
    if (ret == Status::ERROR_NULL_POINTER && parserRefInfoTask_ != nullptr) {
        return Status::ERROR_AGAIN;
    }
    return ret;
}

Status MediaDemuxer::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryReclaimParserTask();
    Status ret = plugin->GetGopLayerInfo(gopId, gopLayerInfo);
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
    EnterDraggingOpenGopCnt();
}

Status MediaDemuxer::GetIFramePos(std::vector<uint32_t> &IFramePos)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryReclaimParserTask();
    return plugin->GetIFramePos(IFramePos);
}

Status MediaDemuxer::Dts2FrameId(int64_t dts, uint32_t &frameId)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(plugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryReclaimParserTask();
    return plugin->Dts2FrameId(dts, frameId);
}

Status MediaDemuxer::SeekMs2FrameId(int64_t seekMs, uint32_t &frameId)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> videoPlugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(videoPlugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryReclaimParserTask();
    return videoPlugin->SeekMs2FrameId(seekMs, frameId);
}

Status MediaDemuxer::FrameId2SeekMs(uint32_t frameId, int64_t &seekMs)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "Source is nullptr");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> videoPlugin = GetCurFFmpegPlugin();
    FALSE_RETURN_V_MSG_E(videoPlugin != nullptr, Status::ERROR_NULL_POINTER, "Demuxer plugin is nullptr");
    TryReclaimParserTask();
    return videoPlugin->FrameId2SeekMs(frameId, seekMs);
}

void MediaDemuxer::OnBufferAvailable(int32_t trackId)
{
    MEDIA_LOG_DD("Buffer available track " PUBLIC_LOG_D32, trackId);
    if (GetEnableSampleQueueFlag()) {
        UpdateLastVideoBufferAbsPts(trackId);
        // buffer is available trigger SampleConsumer working task to run immediately.
        AccelerateSampleConsumerTask(trackId);
    } else {
        // buffer is available trigger demuxer track working task to run immediately.
        AccelerateTrackTask(trackId);
    }
}

void MediaDemuxer::AccelerateSampleConsumerTask(int32_t trackId)
{
    MEDIA_TRACE_DEBUG("MediaDemuxer::AccelerateSampleConsumerTask");
    {
        std::unique_lock<std::mutex> stopLock(stopMutex_);
        if (isStopped_ || isThreadExit_) {
            return;
        }
    }
    AutoLock lock(mapMutex_);
    auto track = trackMap_.find(trackId);
    if (track == trackMap_.end() || track->second == nullptr || !track->second->GetNotifySampleConsumerFlag()) {
        return;
    }
    track->second->SetNotifySampleConsumerFlag(false);

    auto sampleConsumerTask = sampleConsumerTaskMap_.find(trackId);
    if (sampleConsumerTask == sampleConsumerTaskMap_.end()) {
        return;
    }
    sampleConsumerTask->second->UpdateDelayTime();
}

void MediaDemuxer::AccelerateTrackTask(int32_t trackId)
{
    {
        std::unique_lock<std::mutex> stopLock(stopMutex_);
        if (isStopped_ || isThreadExit_) {
            return;
        }
    }
    AutoLock lock(mapMutex_);

    auto track = trackMap_.find(trackId);
    if (track == trackMap_.end() || track->second == nullptr || !track->second->GetNotifyFlag()) {
        return;
    }
    track->second->SetNotifyFlag(false);

    auto task = taskMap_.find(trackId);
    if (task == taskMap_.end()) {
        return;
    }
    MEDIA_LOG_DD("Accelerate track " PUBLIC_LOG_D32, trackId);
    task->second->UpdateDelayTime();
}

void MediaDemuxer::SetTrackNotifyFlag(int32_t trackId, bool isNotifyNeeded)
{
    // This function is called in demuxer track working thread, and if track info exists it is valid.
    auto track = trackMap_.find(trackId);
    if (track != trackMap_.end() && track->second != nullptr) {
        track->second->SetNotifyFlag(isNotifyNeeded);
    }
}

void MediaDemuxer::SetTrackNotifySampleConsumerFlag(int32_t trackId, bool isNotifySampleConsumerNeeded)
{
    // This function is called in samplequeue consumer working thread, and if track info exists it is valid.
    auto track = trackMap_.find(trackId);
    if (track != trackMap_.end() && track->second != nullptr) {
        track->second->SetNotifySampleConsumerFlag(isNotifySampleConsumerNeeded);
    }
}
 
Status MediaDemuxer::GetBitRates(std::vector<uint32_t> &bitRates)
{
    FALSE_RETURN_V_MSG(source_ != nullptr, Status::ERROR_INVALID_OPERATION, "Source is nullptr");
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
    FALSE_RETURN_V_MSG(source_ != nullptr, Status::ERROR_INVALID_OPERATION, "Source is nullptr");
    Status ret = Status::OK;
    ret = source_->GetDownloadInfo(downloadInfo);
    if (streamDemuxer_ != nullptr) {
        downloadInfo.firstFrameDecapsulationTime =
            streamDemuxer_->GetFirstFrameDecapsulationTime() - downloadInfo.firstFrameDecapsulationTime;
        if (downloadInfo.firstFrameDecapsulationTime < 0) {
            downloadInfo.firstFrameDecapsulationTime = 0;
        }
    }
    return ret;
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

void MediaDemuxer::SetPlayerId(std::string playerId)
{
    playerId_ = playerId;
}

void MediaDemuxer::SetDumpInfo(bool isDump, uint64_t instanceId)
{
    (void)instanceId;
    auto tid = gettid();
    if (isDump && tid <= 0) {
        MEDIA_LOG_W("Cannot dump with tid <= 0.");
        return;
    }
    dumpPrefix_ = std::to_string(tid);
    isDump_ = isDump;
}

bool MediaDemuxer::GetDuration(int64_t& durationMs)
{
    AutoLock lock(mapMutex_);
    if (source_ == nullptr) {
        durationMs = -1;
        return false;
    }
    MEDIA_TRACE_DEBUG("MediaDemuxer::GetDuration");
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
            MEDIA_LOG_D("Get drminfo failed, ret:" PUBLIC_LOG_D32, static_cast<int32_t>(ret));
        }
    }
    return Status::OK;
}

Status MediaDemuxer::AddDemuxerCopyTask(int32_t trackId, TaskType type)
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
            MEDIA_LOG_E("Add demuxer task failed, unknow task type:" PUBLIC_LOG_D32, static_cast<int32_t>(type));
            return Status::ERROR_UNKNOWN;
        }
    }

    std::unique_ptr<Task> task = std::make_unique<Task>(taskName, playerId_, type);
    FALSE_RETURN_V_MSG_W(task != nullptr, Status::OK,
        "Create task failed, track:" PUBLIC_LOG_D32 ", type:" PUBLIC_LOG_D32,
        trackId, static_cast<int32_t>(type));

    taskMap_[trackId] = std::move(task);
    UpdateThreadPriority(trackId);
    taskMap_[trackId]->RegisterJob([this, trackId] { return ReadLoop(trackId); });

    // To wake up DEMUXER TRACK WORKING TASK immediately on input buffer available.
    std::unique_ptr<Task> notifyTask =
        std::make_unique<Task>(taskName + "N", playerId_, type, TaskPriority::NORMAL, false);
    FALSE_RETURN_V_MSG_W(notifyTask != nullptr, Status::OK,
        "Create notify task failed, track:" PUBLIC_LOG_D32 ", type:" PUBLIC_LOG_D32,
        trackId, static_cast<int32_t>(type));

    sptr<IProducerListener> listener =
        OHOS::sptr<AVBufferQueueProducerListener>::MakeSptr(trackId, shared_from_this(), notifyTask);
    FALSE_RETURN_V_MSG_W(listener != nullptr, Status::OK,
        "Create listener failed, track:" PUBLIC_LOG_D32 ", type:" PUBLIC_LOG_D32,
        trackId, static_cast<int32_t>(type));

    trackMap_.emplace(trackId, std::make_shared<TrackWrapper>(trackId, listener, shared_from_this()));
    return Status::OK;
}

void MediaDemuxer::UpdateThreadPriority(int32_t trackId)
{
#ifdef SUPPORT_START_STOP_ON_DEMAND
    taskMap_[trackId]->UpdateThreadPriority(THREAD_PRIORITY_41, "media_service");
    if (GetEnableSampleQueueFlag()) {
        sampleConsumerTaskMap_[trackId]->UpdateThreadPriority(THREAD_PRIORITY_41, "media_service");
    }
#else
    if (!HasVideo() && trackId == audioTrackId_) {
        taskMap_[trackId]->UpdateThreadPriority(THREAD_PRIORITY_41, "media_service");
        if (GetEnableSampleQueueFlag()) {
            sampleConsumerTaskMap_[trackId]->UpdateThreadPriority(THREAD_PRIORITY_41, "media_service");
        }
        MEDIA_LOG_I("Update thread priority for audio-only source");
    }
#endif
}

Status MediaDemuxer::AddDemuxerCopyTaskByTrack(int32_t trackId, DemuxerTrackType type)
{
    int32_t trackType = static_cast<int32_t>(type);
    std::string taskName = "Demux" + TRACK_SUFFIX_MAP.at(type);
    auto task = std::make_unique<Task>(taskName, playerId_, TaskType::DEMUXER);
    FALSE_RETURN_V_MSG_W(task != nullptr, Status::ERROR_NULL_POINTER,
        "Create task failed, track:" PUBLIC_LOG_D32 ",DemuxerTrackType:" PUBLIC_LOG_D32, trackId, trackType);
    taskMap_[trackId] = std::move(task);
    taskMap_[trackId]->RegisterJob([this, trackId] { return ReadLoop(trackId); });

    std::string sampleConsumerTaskName = "SampleConsumer" + TRACK_SUFFIX_MAP.at(type);
    auto sampleConsumerTask = std::make_unique<Task>(sampleConsumerTaskName, playerId_, TaskType::DECODER);
    FALSE_RETURN_V_MSG_W(sampleConsumerTask != nullptr, Status::ERROR_NULL_POINTER,
        "Create sampleConsumerTask failed, track:" PUBLIC_LOG_D32 ",DemuxerTrackType:" PUBLIC_LOG_D32,
        trackId, trackType);
    sampleConsumerTaskMap_[trackId] = std::move(sampleConsumerTask);
    sampleConsumerTaskMap_[trackId]->RegisterJob([this, trackId] { return SampleConsumerLoop(trackId); });
    UpdateThreadPriority(trackId);
    if (notifySampleConsumeTask_ == nullptr) {
        notifySampleConsumeTask_
            = std::make_unique<Task>("SAM_CON", playerId_, TaskType::DECODER, TaskPriority::HIGH, false);
        FALSE_RETURN_V_MSG_W(notifySampleConsumeTask_ != nullptr, Status::ERROR_NULL_POINTER,
            "Create notifyConsume task failed, track:" PUBLIC_LOG_D32 ", TrackType:" PUBLIC_LOG_D32,
            trackId, trackType);
    }

    if (notifySampleProduceTask_ == nullptr) {
        notifySampleProduceTask_
            = std::make_unique<Task>("SAM_PRO", playerId_, TaskType::DEMUXER, TaskPriority::HIGH, false);
        FALSE_RETURN_V_MSG_W(notifySampleProduceTask_ != nullptr, Status::ERROR_NULL_POINTER,
            "Create notifyProduce task failed, track:" PUBLIC_LOG_D32 ", TrackType:" PUBLIC_LOG_D32,
            trackId, trackType);
    }

    // To wake up DEMUXER TRACK WORKING TASK immediately on input buffer available.
    std::unique_ptr<Task> notifyTask =
        std::make_unique<Task>(taskName + "N", playerId_, TaskType::DECODER, TaskPriority::HIGH, false);
    FALSE_RETURN_V_MSG_W(notifyTask != nullptr, Status::ERROR_NULL_POINTER,
        "Create notify task failed, track:" PUBLIC_LOG_D32 ", TrackType:" PUBLIC_LOG_D32, trackId, trackType);

    sptr<IProducerListener> listener =
        OHOS::sptr<AVBufferQueueProducerListener>::MakeSptr(trackId, shared_from_this(), notifyTask);
    FALSE_RETURN_V_MSG_W(listener != nullptr, Status::ERROR_NULL_POINTER,
        "Create listener failed, track:" PUBLIC_LOG_D32 ", type:" PUBLIC_LOG_D32, trackId, trackType);
    trackMap_.emplace(trackId, std::make_shared<TrackWrapper>(trackId, listener, shared_from_this()));
    MEDIA_LOG_I("create tasks done: trackId=" PUBLIC_LOG_D32 ",type=" PUBLIC_LOG_D32, trackId, trackType);
    return Status::OK;
}

void MediaDemuxer::AddDemuxerCopyTaskByTrackIfFilter(int32_t trackId, DemuxerTrackType type)
{
    FALSE_RETURN_NOLOG(isCreatedByFilter_);
    AddDemuxerCopyTaskByTrack(trackId, type);
}

void MediaDemuxer::AddDemuxerCopyTaskIfFilter(int32_t trackId, TaskType type)
{
    FALSE_RETURN_NOLOG(isCreatedByFilter_);
    AddDemuxerCopyTask(trackId, type);
}

void MediaDemuxer::AddHandleFlvSelectBitrateTask()
{
    TaskType type = GetEnableSampleQueueFlag() ? TaskType::DEMUXER : TaskType::VIDEO;
    std::string taskName = GetEnableSampleQueueFlag() ? "DemFlv" : "DemVFlv";
    handleFlvSelectBitrateTask_ = std::make_unique<Task>(taskName, playerId_, type, TaskPriority::NORMAL, false);
    FALSE_RETURN_MSG_W(handleFlvSelectBitrateTask_ != nullptr,
        "Create handleFlvSelectBitrateTask_ failed, type:" PUBLIC_LOG_D32, type);
}

Status MediaDemuxer::InnerPrepare()
{
    Plugins::MediaInfo mediaInfo;
    Status ret = demuxerPluginManager_->LoadCurrentAllPlugin(streamDemuxer_, mediaInfo);
    if (ret == Status::OK) {
        if (isTranscoderMode_ || isPlayerMode_) {
            UpdateMjpegMediaMetaData(mediaInfo);
        }
        InitMediaMetaData(mediaInfo);
        InitDefaultTrack(mediaInfo, videoTrackId_, audioTrackId_, subtitleTrackId_, videoMime_);
        InitIsAudioDemuxDecodeAsync();
        if (isTranscoderMode_) {
            TranscoderInitMediaStartPts();
            MEDIA_LOG_I("Media startTime: " PUBLIC_LOG_D64, transcoderStartPts_);
        }
        InitMediaStartPts();
        InitVideoTrack();
        InitAudioTrack();
        InitSubtitleTrack();
    } else {
        MEDIA_LOG_E("Parse meta failed, ret: " PUBLIC_LOG_D32, (int32_t)(ret));
    }
    return ret;
}

void MediaDemuxer::InitVideoTrack()
{
    FALSE_RETURN_MSG(IsValidTrackId(videoTrackId_), "videoTrackId_ invalid");
    GetEnableSampleQueueFlag() ? AddDemuxerCopyTaskByTrackIfFilter(videoTrackId_, DemuxerTrackType::VIDEO)
                            : AddDemuxerCopyTaskIfFilter(videoTrackId_, TaskType::VIDEO);
    if (isFlvLiveStream_) {
        AddHandleFlvSelectBitrateTask();
    }
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

void MediaDemuxer::InitAudioTrack()
{
    FALSE_RETURN_MSG(IsValidTrackId(audioTrackId_), "audioTrackId_ invalid");
    GetEnableSampleQueueFlag() ? AddDemuxerCopyTaskByTrackIfFilter(audioTrackId_, DemuxerTrackType::AUDIO)
                            : AddDemuxerCopyTaskIfFilter(audioTrackId_, TaskType::AUDIO);
    demuxerPluginManager_->UpdateTempTrackMapInfo(audioTrackId_, audioTrackId_, -1);
    int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(audioTrackId_);
    streamDemuxer_->SetNewAudioStreamID(streamId);
    streamDemuxer_->SetChangeFlag(true);
    streamDemuxer_->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
    int64_t bitRate = 0;
    mediaMetaData_.trackMetas[audioTrackId_]->GetData(Tag::MEDIA_BITRATE, bitRate);
    // ignore the bitrate of audio in flv livestream case
    if (!isFlvLiveStream_) {
        source_->SetCurrentBitRate(bitRate, streamId);
    }
}

void MediaDemuxer::InitSubtitleTrack()
{
    FALSE_RETURN_MSG(IsValidTrackId(subtitleTrackId_), "subtitleTrackId_ invalid");
    GetEnableSampleQueueFlag() ? AddDemuxerCopyTaskByTrackIfFilter(subtitleTrackId_, DemuxerTrackType::SUBTITLE)
                            : AddDemuxerCopyTaskIfFilter(subtitleTrackId_, TaskType::SUBTITLE);
    demuxerPluginManager_->UpdateTempTrackMapInfo(subtitleTrackId_, subtitleTrackId_, -1);
    int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(subtitleTrackId_);
    streamDemuxer_->SetNewSubtitleStreamID(streamId);
    streamDemuxer_->SetChangeFlag(true);
    streamDemuxer_->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
}

int32_t MediaDemuxer::GetTargetVideoTrackId(std::vector<std::shared_ptr<Meta>> trackInfos)
{
    FALSE_RETURN_V(!IsValidTrackId(targetVideoTrackId_), targetVideoTrackId_);
    MEDIA_LOG_I_SHORT("GetTargetVideoTrackId enter");
    int32_t trackInfoSize = static_cast<int32_t>(trackInfos.size());
    for (int32_t index = 0; index < trackInfoSize; index++) {
        std::shared_ptr<Meta> meta = trackInfos[index];
        if (meta == nullptr) {
            MEDIA_LOG_E_SHORT("meta is invalid, index: " PUBLIC_LOG_D32, index);
            continue;
        }
        Plugins::MediaType mediaType = Plugins::MediaType::AUDIO;
        if (!meta->GetData(Tag::MEDIA_TYPE, mediaType)) {
            continue;
        }
        if (mediaType != Plugins::MediaType::VIDEO) {
            continue;
        }
        MEDIA_LOG_I_SHORT("SelectVideoTrack trackId: %{public}d", index);
        targetVideoTrackId_ = index;
        break;
    }
    return targetVideoTrackId_;
}

Status MediaDemuxer::BoostReadThreadPriority()
{
    MEDIA_LOG_I("Boost read thread priority start");
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = GetCurFFmpegPlugin();
    if (plugin == nullptr) {
        MEDIA_LOG_W("Demuxer not available, boost read thread priority failed");
        return Status::ERROR_INVALID_PARAMETER;
    }
    Status ret = plugin->BoostReadThreadPriority();
    if (ret != Status::OK) {
        MEDIA_LOG_W("Boost read thread priority failed, ret: " PUBLIC_LOG_D32, static_cast<int32_t>(ret));
        return ret;
    }
    MEDIA_LOG_I("Read thread priority successfully boosted");
    return Status::OK;
}

bool MediaDemuxer::IsWatchDevice()
{
    std::string deviceType = system::GetParameter("ro.build.device_type", "");
    if (!deviceType.empty()) {
        if (deviceType.find("watch") != std::string::npos || deviceType.find("wearable") != std::string::npos) {
            MEDIA_LOG_I("Detected watch device by device_type: %{public}s", deviceType.c_str());
            return true;
        }
    }

    MEDIA_LOG_D("Device is not detected as watch device");
    return false;
}

bool MediaDemuxer::BoostThreadPriorityIfNeeded()
{
    if (IsWatchDevice()) {
        MEDIA_LOG_I("Watch device, boosting read thread priority");
        Status boostWatchRet = BoostReadThreadPriority();
        if (boostWatchRet != Status::OK) {
            MEDIA_LOG_W("Failed to boost read thread priority, ret: %{public}d",
                static_cast<int32_t>(boostWatchRet));
            return false;
        } else {
            MEDIA_LOG_I("Successfully boosted read thread priority for watch");
            return true;
        }
    } else if (!HasVideo() && HasAudio()) {
        MEDIA_LOG_I("Audio only, boosting read thread priority");
        Status boostAudioRet = BoostReadThreadPriority();
        if (boostAudioRet != Status::OK) {
            MEDIA_LOG_W("Failed to boost read thread priority for audio, ret: %{public}d",
                static_cast<int32_t>(boostAudioRet));
            return false;
        } else {
            MEDIA_LOG_I("Successfully boosted read thread priority for audio");
            return true;
        }
    }
    return false;
}

Status MediaDemuxer::SetDataSource(const std::shared_ptr<MediaSource> &source)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running");
    isPrepared_.store(false);
    source_->SetCallback(this);
    auto res = source_->SetSource(source);
    FALSE_RETURN_V_MSG_E(res == Status::OK, res, "Plugin set source failed");
    isFlvLiveStream_ = source_->IsFlvLiveStream();
    Status ret = source_->GetSize(mediaDataSize_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Get file size failed");

    std::vector<StreamInfo> streams;
    source_->GetStreamInfo(streams);
    isHlsFmp4_ = source_->IsHlsFmp4();
    isHls_ = source_->IsHls();
    MEDIA_LOG_I("ishlsfmp4: " PUBLIC_LOG_D32, static_cast<int32_t>(isHlsFmp4_));
    demuxerPluginManager_->SetIsHlsFmp4(isHls_);
    demuxerPluginManager_->InitDefaultPlay(streams);

    streamDemuxer_ = std::make_shared<StreamDemuxer>();
    streamDemuxer_->SetSourceType(source->GetSourceType());
    streamDemuxer_->SetInterruptState(isInterruptNeeded_);
    streamDemuxer_->SetSource(source_);
    streamDemuxer_->Init(uri_);

    std::string inferPluginName = InferDemuxerPluginNameByContentType();
    res = InnerPrepare();
    std::string snifferPluginName;
    bool isGotPlugin = demuxerPluginManager_->GetPluginName(snifferPluginName);
    source_->NotifyInitSuccess();
    ProcessDrmInfos();
    FALSE_RETURN_V_NOLOG(eventReceiver_ != nullptr, res);
    eventReceiver_->OnMemoryUsageEvent({"SOURCE",
        DfxEventType::DFX_INFO_MEMORY_USAGE, source_->GetMemorySize()});
    if (!inferPluginName.empty() && isGotPlugin) {
        eventReceiver_->OnDfxEvent({"DEMUX", DfxEventType::DFX_INFO_PLAYER_EOS_SEEK,
            static_cast<int32_t>(snifferPluginName == inferPluginName)});
    }
    bool isBoosted = BoostThreadPriorityIfNeeded();
    MEDIA_LOG_I("Thread priority boosted: %{public}d", isBoosted);
    isPrepared_.store(true);
    MEDIA_LOG_I("Out");
    return res;
}

std::string MediaDemuxer::InferDemuxerPluginNameByContentType()
{
    FALSE_RETURN_V(source_ != nullptr, "");
    std::string contentType = source_->GetContentType();
    FALSE_RETURN_V_NOLOG(!contentType.empty(), "");
    FALSE_RETURN_V(demuxerPluginManager_ != nullptr, "");
    std::vector<Plugins::PluginDescription> matchedPluginsDescriptions =
        Plugins::PluginList::GetInstance().GetPluginsByType(Plugins::PluginType::DEMUXER);
    for (auto& iter : matchedPluginsDescriptions) {
        if (IsHitPlugin(iter.pluginName, contentType)) {
            MEDIA_LOG_I("ContentType: " PUBLIC_LOG_S ", pluginName: " PUBLIC_LOG_S,
                contentType.c_str(), iter.pluginName.c_str());
            return iter.pluginName;
        }
    }
    return "";
}

bool MediaDemuxer::IsHitPlugin(std::string& pluginName, std::string& contentType)
{
    std::stringstream ss(pluginName);
    std::string token;
    while (std::getline(ss, token, DEMUXER_PLUGIN_NAME_DELIMITER)) {
        size_t pos = 0;
        if ((pos = token.find(DEMUXER_PLUGIN_NAME_HEADER, pos)) != std::string::npos) {
            token.erase(pos, DEMUXER_PLUGIN_NAME_HEADER.size());
        }
        if (!token.empty() && contentType.find(token) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool MediaDemuxer::IsSubtitleMime(const std::string& mime)
{
    if (mime == "application/x-subrip" || mime == "text/vtt") {
        return true;
    }
#ifdef SUPPORT_DEMUXER_LRC
    if (mime == "text/plain") {
        return true;
    }
#endif
#ifdef SUPPORT_DEMUXER_SAMI
    if (mime == "application/x-sami") {
        return true;
    }
#endif
#ifdef SUPPORT_DEMUXER_ASS
    if (mime == "text/x-ass") {
        return true;
    }
#endif
    return false;
}

Status MediaDemuxer::SetSubtitleSource(const std::shared_ptr<MediaSource> &subSource)
{
    MEDIA_LOG_I("In");
    if (IsValidTrackId(subtitleTrackId_)) {
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

    MediaInfo mediaInfo;
    ret = demuxerPluginManager_->LoadCurrentSubtitlePlugin(subStreamDemuxer_, mediaInfo);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse meta failed, ret:" PUBLIC_LOG_D32, static_cast<int32_t>(ret));
    InitMediaMetaData(mediaInfo);  // update mediaMetaData_
    int32_t trackSize = static_cast<int32_t>(mediaInfo.tracks.size());
    for (int32_t index = 0; index < trackSize; index++) {
        auto trackMeta = mediaInfo.tracks[index];
        std::string mimeType;
        std::string trackType;
        trackMeta.Get<Tag::MIME_TYPE>(mimeType);
        int32_t curStreamID = demuxerPluginManager_->GetStreamIDByTrackID(index);
        if (trackMeta.Get<Tag::MIME_TYPE>(mimeType) && IsSubtitleMime(mimeType) && curStreamID == subtitleStreamID) {
            MEDIA_LOG_I("STrack " PUBLIC_LOG_D32, index);
            subtitleTrackId_ = index;   // dash inner subtitle can be replace by out subtitle
            break;
        }
    }

    if (IsValidTrackId(subtitleTrackId_)) {
        GetEnableSampleQueueFlag() ? AddDemuxerCopyTaskByTrack(subtitleTrackId_, DemuxerTrackType::SUBTITLE)
                                   : AddDemuxerCopyTask(subtitleTrackId_, TaskType::SUBTITLE);
        demuxerPluginManager_->UpdateTempTrackMapInfo(subtitleTrackId_, subtitleTrackId_, -1);
        subStreamDemuxer_->SetNewSubtitleStreamID(subtitleStreamID);
        subStreamDemuxer_->SetDemuxerState(subtitleStreamID, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
    }

    MEDIA_LOG_I("Out, ext sub %{public}d", subtitleTrackId_);
    return ret;
}

void MediaDemuxer::OnInterrupted(bool isInterruptNeeded)
{
    MEDIA_LOG_D("MediaDemuxer OnInterrupted %{public}d", isInterruptNeeded);
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
    if (subStreamDemuxer_ != nullptr) {
        subStreamDemuxer_->SetInterruptState(isInterruptNeeded);
    }
    if (demuxerPluginManager_ != nullptr) {
        demuxerPluginManager_->NotifyInitialBufferingEnd(false);
        demuxerPluginManager_->SetInterruptState(isInterruptNeeded);
    }
}

void MediaDemuxer::SetBundleName(const std::string& bundleName)
{
    FALSE_RETURN(source_ != nullptr && streamDemuxer_ != nullptr);
    MEDIA_LOG_I("BundleName: " PUBLIC_LOG_S, bundleName.c_str());
    bundleName_ = bundleName;
    streamDemuxer_->SetBundleName(bundleName);
    source_->SetBundleName(bundleName);
}

Status MediaDemuxer::SetOutputBufferQueue(int32_t trackId, const sptr<AVBufferQueueProducer>& producer)
{
    AutoLock lock(mapMutex_);
    FALSE_RETURN_V_MSG_E(IsValidTrackId(trackId) &&
        static_cast<size_t>(trackId) < mediaMetaData_.trackMetas.size(),
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
        bufferQueueMap_.insert(std::pair<int32_t, sptr<AVBufferQueueProducer>>(trackId, producer));
        bufferMap_.insert(std::pair<int32_t, std::shared_ptr<AVBuffer>>(trackId, nullptr));
        MEDIA_LOG_I("Set bufferQueue successfully");
        GenerateDfxBufferQueue(trackId);
        if (GetEnableSampleQueueFlag()) {
            ret = AddSampleBufferQueue(trackId);
            FALSE_RETURN_V_MSG_E(
                ret == Status::OK, ret, "AddSampleBufferQueue failed for track " PUBLIC_LOG_D32, trackId);
        }
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

std::map<int32_t, sptr<AVBufferQueueProducer>> MediaDemuxer::GetBufferQueueProducerMap()
{
    return bufferQueueMap_;
}

Status MediaDemuxer::InnerSelectTrack(int32_t trackId)
{
    eosMap_[trackId] = false;
    segmentEosMap_[trackId] = false;
    requestBufferErrorCountMap_[trackId] = 0;

    int32_t innerTrackID = trackId;
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    if (IsNeedMapToInnerTrackID()) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(demuxerPluginManager_->GetStreamIDByTrackID(trackId));
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
    }

    return pluginTemp->SelectTrack(static_cast<uint32_t>(innerTrackID));
}

Status MediaDemuxer::StartTask(int32_t trackId)
{
    if (GetEnableSampleQueueFlag()) {
        return StartTaskWithSampleQueue(trackId);
    }
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
            UpdateBufferQueueListener(trackId);
            taskMap_[trackId]->Start();
        }
    } else {
        if (taskMap_[trackId] != nullptr && !taskMap_[trackId]->IsTaskRunning()) {
            UpdateBufferQueueListener(trackId);
            taskMap_[trackId]->Start();
        }
    }
    return Status::OK;
}

Status MediaDemuxer::StartTaskWithSampleQueue(int32_t trackId)
{
    MEDIA_LOG_I("In, track " PUBLIC_LOG_D32, trackId);
    Status ret = Status::OK;
    TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    auto iterTrack = TRACK_MAP.find(trackType);
    FALSE_RETURN_V_MSG_E(iterTrack != TRACK_MAP.end(), Status::ERROR_INVALID_PARAMETER,
        "invalid trackId: " PUBLIC_LOG_D32, trackId);
    if (taskMap_.find(trackId) == taskMap_.end() ||
        sampleConsumerTaskMap_.find(trackId) == sampleConsumerTaskMap_.end()) {
        ret = AddDemuxerCopyTaskByTrack(trackId, iterTrack->second);
        FALSE_RETURN_V(ret == Status::OK, ret);
    }
    if (taskMap_[trackId] != nullptr && !taskMap_[trackId]->IsTaskRunning()) {
        UpdateBufferQueueListener(trackId);
        taskMap_[trackId]->Start();
    }
    if (sampleConsumerTaskMap_[trackId] != nullptr && !sampleConsumerTaskMap_[trackId]->IsTaskRunning()) {
        FALSE_RETURN_V(trackId != videoTrackId_ || !isVideoMuted_ || needReleaseVideoDecoder_, Status::OK);
        sampleConsumerTaskMap_[trackId]->Start();
    }
    return Status::OK;
}

void MediaDemuxer::UpdateBufferQueueListener(int32_t trackId)
{
    FALSE_RETURN(bufferQueueMap_.find(trackId) != bufferQueueMap_.end() && trackMap_.find(trackId) != trackMap_.end());
    auto producer = bufferQueueMap_[trackId];
    auto listener = trackMap_[trackId]->GetProducerListener();
    producer->SetBufferAvailableListener(listener);
}

Status MediaDemuxer::HandleDashSelectTrack(int32_t trackId)
{
    MEDIA_LOG_I("In, track " PUBLIC_LOG_D32, trackId);
    eosMap_[trackId] = false;
    segmentEosMap_[trackId] = false;
    requestBufferErrorCountMap_[trackId] = 0;

    int32_t targetStreamID = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
    if (targetStreamID == -1) {
        MEDIA_LOG_E("Get stream failed");
        return Status::ERROR_UNKNOWN;
    }

    int32_t curTrackId = TRACK_ID_INVALID;
    TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    if (trackType == TrackType::TRACK_AUDIO) {
        curTrackId = audioTrackId_;
    } else if (trackType == TrackType::TRACK_VIDEO) {
        curTrackId = videoTrackId_;
    } else if (trackType == TrackType::TRACK_SUBTITLE) {
        curTrackId = subtitleTrackId_;
    } else {   // invalid
        MEDIA_LOG_E("Track type invalid");
        return Status::ERROR_UNKNOWN;
    }

    if (trackId == curTrackId || targetStreamID != demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)) {
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
        if (trackType == TrackType::TRACK_AUDIO) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_AUDIO_TRACK_CHANGE, trackId});
            audioTrackId_ = trackId;
        } else if (trackType == TrackType::TRACK_VIDEO) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_VIDEO_TRACK_CHANGE, trackId});
            videoTrackId_ = trackId;
        } else if (trackType == TrackType::TRACK_SUBTITLE) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_SUBTITLE_TRACK_CHANGE, trackId});
            subtitleTrackId_ = trackId;
        } else {}
    }
    return Status::OK;
}

Status MediaDemuxer::DoSelectTrack(int32_t trackId, int32_t curTrackId)
{
    if (IsValidTrackId(curTrackId)) {
        if (taskMap_.find(curTrackId) != taskMap_.end() && taskMap_[curTrackId] != nullptr) {
            taskMap_[curTrackId]->Stop();
        }
        if (sampleConsumerTaskMap_.find(curTrackId) != sampleConsumerTaskMap_.end() &&
            sampleConsumerTaskMap_[curTrackId] != nullptr) {
            sampleConsumerTaskMap_[curTrackId]->Stop();
        }
        if (curTrackId == audioTrackId_ && sampleQueueMap_[curTrackId] != nullptr) {
            sampleQueueMap_[curTrackId]->Clear();
        }
        Status ret = UnselectTrack(curTrackId);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Unselect track failed");
        bufferQueueMap_.insert(
            std::pair<int32_t, sptr<AVBufferQueueProducer>>(trackId, bufferQueueMap_[curTrackId]));
        bufferMap_.insert(std::pair<int32_t, std::shared_ptr<AVBuffer>>(trackId, bufferMap_[curTrackId]));
        bufferQueueMap_.erase(curTrackId);
        bufferMap_.erase(curTrackId);
        demuxerPluginManager_->UpdateTempTrackMapInfo(trackId, trackId, -1);

        if (GetEnableSampleQueueFlag()) {
            AutoLock lock(mapMutex_);
            sampleQueueMap_.insert(
                std::pair<int32_t, std::shared_ptr<SampleQueue>>(trackId, sampleQueueMap_[curTrackId]));
            sampleQueueMap_.erase(curTrackId);
            if (sampleQueueMap_[trackId] != nullptr) {
                sampleQueueMap_[trackId]->UpdateQueueId(trackId);
            }
        }
        return InnerSelectTrack(trackId);
    }
    return Status::ERROR_UNKNOWN;
}

Status MediaDemuxer::HandleSelectTrack(int32_t trackId)
{
    int32_t curTrackId = TRACK_ID_INVALID;
    TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    if (trackType == TrackType::TRACK_AUDIO) {
        MEDIA_LOG_I("Select audio [" PUBLIC_LOG_D32 "->" PUBLIC_LOG_D32 "]", audioTrackId_, trackId);
        curTrackId = audioTrackId_;
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
        if (trackType == TrackType::TRACK_AUDIO) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_AUDIO_TRACK_CHANGE, trackId});
            audioTrackId_ =  trackId;
        } else if (trackType == TrackType::TRACK_VIDEO) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_VIDEO_TRACK_CHANGE, trackId});
            videoTrackId_ =  trackId;
        } else {}
    }
    
    return ret;
}

Status MediaDemuxer::SelectTrack(uint32_t trackIndex)
{
    int32_t trackId = static_cast<int32_t>(trackIndex);
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_D("Select " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(IsValidTrackId(trackId) &&
        static_cast<size_t>(trackIndex) < mediaMetaData_.trackMetas.size(),
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

Status MediaDemuxer::UnselectTrack(uint32_t trackIndex)
{
    int32_t trackId = static_cast<int32_t>(trackIndex);
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_D("Unselect " PUBLIC_LOG_D32, trackId);

    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    int32_t innerTrackID = trackId;
    if (IsNeedMapToInnerTrackID()) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
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

    return pluginTemp->UnselectTrack(static_cast<uint32_t>(innerTrackID));
}

Status MediaDemuxer::HandleSegmentEos(int32_t trackId)
{
    segmentEosMap_[trackId] = true;
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    bool isAVInOneStream = IsAVInOneStream();
    Status ret = Status::OK;
    if (!isAVInOneStream) {
        // not mixed
        ret = HandleSegmentChange(trackId);
        MEDIA_LOG_I("HandleSegmentChange end");
        return ret;
    }
    FALSE_RETURN_V_NOLOG(IsSegmentEos(), Status::ERROR_ONE_TRACK_SEGMENT_EOS);
    MEDIA_LOG_I("HandleSegmentChange mixed start");
    int32_t tmpTrackId = IsValidTrackId(videoTrackId_) ? videoTrackId_ : audioTrackId_;
    ret = HandleSegmentChange(tmpTrackId);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "HandleSegmentChange mixed failed");
    ret = (tmpTrackId == videoTrackId_ && IsValidTrackId(audioTrackId_)) ? InnerSelectTrack(audioTrackId_) : Status::OK;
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "HandleSegmentChange Select audio track failed");
    if (IsAVInOneStream()) {
        for (auto &[track, isEos]: hlsSegmentEosMap_) {
            isEos = false;
        }
    } else {
        hlsSegmentEosMap_[trackId] = false;
    }
    if (IsValidTrackId(audioTrackId_)) {
        isBufferingMap_[audioTrackId_].store(true);
    }
    if (IsValidTrackId(videoTrackId_)) {
        isBufferingMap_[videoTrackId_].store(true);
    }
    MEDIA_LOG_I("HandleSegmentChange mixed end");
    return ret;
}

Status MediaDemuxer::HandleSegmentChange(int32_t trackId)
{
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    FALSE_RETURN_V(!subStreamDemuxer_ || trackId != subtitleTrackId_, Status::OK);
    FALSE_RETURN_V(IsValidTrackId(trackId), Status::OK);

    int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
    FALSE_RETURN_V_MSG_E(streamID != INVALID_STREAM_OR_TRACK_ID, Status::ERROR_INVALID_PARAMETER,
        "Invalid streamId");
    TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    MEDIA_LOG_D("TrackType " PUBLIC_LOG_D32 " TrackId " PUBLIC_LOG_D32, static_cast<int32_t>(trackType), trackId);
    FALSE_RETURN_V_MSG_E(trackType != TRACK_INVALID, Status::ERROR_INVALID_PARAMETER, "TrackType is invalid");
    bool isRebooted = true;
    Status ret = demuxerPluginManager_->RebootPlugin(streamID, trackType, streamDemuxer_, isRebooted);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Reboot demuxer plugin failed");
    ret = InnerSelectTrack(trackId);
    return ret;
}

Status MediaDemuxer::HandleHlsSeek()
{
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    bool isAVInOneStream = IsAVInOneStream();
    Status ret = Status::OK;
    if (isAVInOneStream) {
        // mixed
        int32_t trackId = IsValidTrackId(videoTrackId_) ? videoTrackId_ : audioTrackId_;
        ret = HandleHlsRebootPlugin(trackId);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Hls reboot mixed plugin failed");
        ret = (trackId == videoTrackId_ && IsValidTrackId(audioTrackId_)) ? InnerSelectTrack(audioTrackId_) :
            Status::OK;
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Select audio track failed");
    } else {
        ret = HandleHlsRebootPlugin(audioTrackId_);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Reboot audio plugin failed");
        ret = HandleHlsRebootPlugin(videoTrackId_);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Reboot video plugin failed");
    }
    MEDIA_LOG_I("Reboot hls plugin success, isAVInOneStream: %{public}d", isAVInOneStream);
    return Status::OK;
}

Status MediaDemuxer::HandleHlsRebootPlugin(int32_t trackId)
{
    MEDIA_LOG_I("HandleHlsRebootPlugin In");
    FALSE_RETURN_V(!subStreamDemuxer_ || trackId != subtitleTrackId_, Status::OK);
    Status ret = Status::OK;
    if (IsValidTrackId(trackId)) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        FALSE_RETURN_V_MSG_E(streamID != INVALID_STREAM_OR_TRACK_ID, Status::ERROR_INVALID_PARAMETER,
            "Invalid streamId");
        TrackType trackType = IsAVInOneStream() ? TrackType::TRACK_VIDEO :
            demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
        MEDIA_LOG_D("TrackType " PUBLIC_LOG_D32 " TrackId " PUBLIC_LOG_D32, static_cast<int32_t>(trackType), trackId);
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
        if (seekReadyInfo.second == SEEK_TO_EOS) {
            MEDIA_LOG_I("Seek to eos");
            return Status::OK;
        } else if (seekReadyInfo.first >= 0 && seekReadyInfo.first != streamID) {
            return HandleSeekChangeStream(streamID, seekReadyInfo.first, trackId);
        }
        bool isRebooted = true;
        ret = demuxerPluginManager_->RebootPlugin(streamID, trackType, streamDemuxer_, isRebooted);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Reboot demuxer plugin failed");
        ret = InnerSelectTrack(trackId);
    }
    return ret;
}

Status MediaDemuxer::HandleSeekChangeStream(int32_t currentStreamId, int32_t newStreamId, int32_t trackId)
{
    MEDIA_LOG_I("streamID changed, streamId: " PUBLIC_LOG_D32 ", newStreamId: " PUBLIC_LOG_D32,
        currentStreamId, newStreamId);
    // only fix completed seek currently
    if (streamDemuxer_ != nullptr && HasEosTrack()) {
        streamDemuxer_->SetNewVideoStreamID(newStreamId);
        FALSE_GOON_NOEXEC(demuxerPluginManager_ && demuxerPluginManager_->IsDash(), HandleDashChangeStream(trackId));
    }
    return Status::OK;
}

Status MediaDemuxer::HandleRebootPlugin(int32_t trackId, bool& isRebooted)
{
    FALSE_RETURN_V(!subStreamDemuxer_ || trackId != static_cast<int32_t>(subtitleTrackId_), Status::OK);
    Status ret = Status::OK;
    if (IsValidTrackId(trackId)) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        FALSE_RETURN_V_MSG_E(streamID != INVALID_STREAM_OR_TRACK_ID, Status::ERROR_INVALID_PARAMETER,
            "Invalid streamId");
        TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
        MEDIA_LOG_D("TrackType " PUBLIC_LOG_D32 " TrackId " PUBLIC_LOG_D32, static_cast<int32_t>(trackType), trackId);
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
    MEDIA_LOG_I("Reboot plugin begin");
    Status ret = Status::OK;
    if (isHls_) {
        ret = HandleHlsSeek();
        {
            std::unique_lock<std::mutex> lock(rebootPluginMutex_);
            seekReadyStreamInfo_.clear();
        }
        return ret;
    }
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
    MEDIA_LOG_I("Reboot plugin success");
    return Status::OK;
}

void MediaDemuxer::ResetSampleQueueStatus(int64_t seekTime)
{
    if (sampleQueueMap_.find(videoTrackId_) != sampleQueueMap_.end() && sampleQueueMap_[videoTrackId_]) {
        auto &sampleQueue = sampleQueueMap_[videoTrackId_];
        sampleQueue->Clear();
        sampleQueue->UpdateLastOutSamplePts(seekTime * US_TO_MS);
        sampleQueue->UpdateLastEnterSamplePts(seekTime * US_TO_MS);
    }
    if (sampleQueueMap_.find(audioTrackId_) != sampleQueueMap_.end() && sampleQueueMap_[audioTrackId_]) {
        auto &sampleQueue = sampleQueueMap_[audioTrackId_];
        sampleQueue->Clear();
        sampleQueue->UpdateLastOutSamplePts(seekTime * US_TO_MS);
        sampleQueue->UpdateLastEnterSamplePts(seekTime * US_TO_MS);
    }
    if (IsValidTrackId(audioTrackId_)) {
        isBufferingMap_[audioTrackId_].store(true);
    }
    if (IsValidTrackId(videoTrackId_)) {
        isBufferingMap_[videoTrackId_].store(true);
    }
}

void MediaDemuxer::HandleSeekToTime(int64_t seekTime)
{
    SeekToTimeAfter();
    ResetSampleQueueStatus(seekTime);
}

Status MediaDemuxer::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    Status ret;
    isSeekError_.store(false);
    if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
        MEDIA_LOG_I("Source seek time: %{public}lld", seekTime);
        if (mode == SeekMode::SEEK_CLOSEST_INNER) {
            ScopedTimer timer("seek closest online", SEEKCLOSEST_ONLINE_WARNING_MS);
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_PREVIOUS_SYNC);
        } else {
            ScopedTimer timer("seek online", SEEK_ONLINE_WARNING_MS);
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_CLOSEST_SYNC);
        }
        if (subtitleSource_) {
            demuxerPluginManager_->localSubtitleSeekTo(seekTime);
        }
        HandleSeekToTime(seekTime);
        Plugins::Ms2HstTime(seekTime, realSeekTime);
    } else {
        MEDIA_LOG_I("Demuxer seek");
        if (mode == SeekMode::SEEK_CLOSEST_INNER) {
            ScopedTimer timer("seek closest local", SEEKCLOSEST_LOCAL_WARNING_MS);
            ret = demuxerPluginManager_->SeekTo(seekTime, SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
        } else {
            ScopedTimer timer("seek closest", SEEK_LOCAL_WARNING_MS);
            ret = demuxerPluginManager_->SeekTo(seekTime, mode, realSeekTime);
        }
    }
    isSeeked_ = true;
    if (isVideoMuted_ || needRestore_) {
        if (sampleQueueMap_[videoTrackId_] != nullptr) {
            sampleQueueMap_[videoTrackId_]->Clear();
        }
        lastVideoPts_ = -1;
    }
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    ResetSegmentEosMap();
    for (auto item : requestBufferErrorCountMap_) {
        requestBufferErrorCountMap_[item.first] = 0;
    }
    if (ret != Status::OK) {
        isSeekError_.store(true);
    }
    isFirstFrameAfterSeek_.store(true);
    convertErrorTime_.store(0);
    MEDIA_LOG_D("Out");
    return ret;
}

Status MediaDemuxer::SeekToKeyFrame(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    Status ret;
    isSeekError_.store(false);
    if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
        MEDIA_LOG_I("Source seek time: %{public}lld", seekTime);
        if (mode == SeekMode::SEEK_CLOSEST_INNER) {
            ScopedTimer timer("seek closest online", SEEKCLOSEST_ONLINE_WARNING_MS);
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_PREVIOUS_SYNC);
        } else {
            ScopedTimer timer("seek online", SEEK_ONLINE_WARNING_MS);
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_CLOSEST_SYNC);
        }
        if (subtitleSource_) {
            demuxerPluginManager_->localSubtitleSeekTo(seekTime);
        }
        HandleSeekToTime(seekTime);
        Plugins::Ms2HstTime(seekTime, realSeekTime);
    } else {
        MEDIA_LOG_I("Demuxer seek");
        ScopedTimer timer("seek closest", SEEK_LOCAL_WARNING_MS);
        ret = demuxerPluginManager_->SeekToKeyFrame(seekTime, mode, realSeekTime);
    }
    isSeeked_ = true;
    if (isVideoMuted_ || needRestore_) {
        if (sampleQueueMap_[videoTrackId_] != nullptr) {
            sampleQueueMap_[videoTrackId_]->Clear();
        }
        lastVideoPts_ = -1;
    }
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    ResetSegmentEosMap();
    for (auto item : requestBufferErrorCountMap_) {
        requestBufferErrorCountMap_[item.first] = 0;
    }
    if (ret != Status::OK) {
        isSeekError_.store(true);
    }
    isFirstFrameAfterSeek_.store(true);
    convertErrorTime_.store(0);
    MEDIA_LOG_D("Out");
    return ret;
}

Status MediaDemuxer::SelectBitRate(uint32_t bitRate, bool isAutoSelect)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_INVALID_PARAMETER, "Source is nullptr");
    MEDIA_LOG_I("In");

    if (isFlvLiveStream_ && IsRightMediaTrack(videoTrackId_, DemuxerTrackType::VIDEO)) {
        FALSE_RETURN_V_NOLOG(!isManualBitRateSetting_.load() || !isAutoSelect, Status::ERROR_UNKNOWN);
        if (!isManualBitRateSetting_.load() && !isAutoSelect) {
            isManualBitRateSetting_.store(true);
        }
        FALSE_RETURN_V_NOLOG(GetEnableSampleQueueFlag(), SelectBitrateForNonSQ(0, bitRate));
        auto sqIt = sampleQueueMap_.find(videoTrackId_);
        FALSE_RETURN_V_MSG_E(sqIt != sampleQueueMap_.end() && sqIt->second, Status::ERROR_WRONG_STATE,
            "sampleQueue is nullptr");
        return sqIt->second->ReadySwitchBitrate(bitRate);
    }
    FALSE_RETURN_V(demuxerPluginManager_ != nullptr && streamDemuxer_ != nullptr, Status::ERROR_WRONG_STATE);
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
    MEDIA_LOG_I("In");
    ResetDraggingOpenGopCnt();
    if (streamDemuxer_) {
        streamDemuxer_->Flush();
    }
    
    {
        AutoLock lock(mapMutex_);
        auto it = bufferQueueMap_.begin();
        while (it != bufferQueueMap_.end()) {
            int32_t trackId = it->first;
            if (trackId != videoTrackId_) {
                bufferQueueMap_[trackId]->Clear();
            }
            it++;
        }

        auto sqIt = sampleQueueMap_.begin();
        while (sqIt != sampleQueueMap_.end()) {
            int32_t trackId = sqIt->first;
            if (sampleQueueMap_[trackId] != nullptr) {
                sampleQueueMap_[trackId]->Clear();
            }
            sqIt++;
        }
    }

    if (demuxerPluginManager_) {
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
    AutoLock lock(mapMutex_);
    isDemuxerLoopExecuting_ = false;
    if (streamDemuxer_ != nullptr) {
        streamDemuxer_->SetIsIgnoreParse(true);
    }
    auto it = taskMap_.begin();
    while (it != taskMap_.end()) {
        if (it->second != nullptr) {
            it->second->Stop();
            it->second = nullptr;
        }
        it = taskMap_.erase(it);
    }
    for (auto stIt = sampleConsumerTaskMap_.begin(); stIt != sampleConsumerTaskMap_.end();) {
        if (stIt->second != nullptr) {
            stIt->second->Stop();
        }
        stIt = sampleConsumerTaskMap_.erase(stIt);
    }
    isThreadExit_ = true;
    MEDIA_LOG_I("Out");
    return Status::OK;
}

Status MediaDemuxer::PauseAllTask()
{
    MEDIA_LOG_I("In");
    isDemuxerLoopExecuting_ = false;
    for (auto &iter : taskMap_) {
        if (iter.second != nullptr) {
            iter.second->Pause();
        }
    }
 
    for (auto &iter : sampleConsumerTaskMap_) {
        if (iter.second != nullptr) {
            iter.second->Pause();
        }
    }
    if (demuxerPluginManager_) {
        demuxerPluginManager_->Pause();
    }
    MEDIA_LOG_I("Out");
    return Status::OK;
}

Status MediaDemuxer::PauseAllTaskAsync()
{
    MEDIA_LOG_I("In");
    // To accelerate DemuxerLoop thread to run into PAUSED state
    for (auto &iter : taskMap_) {
        if (iter.second != nullptr) {
            iter.second->PauseAsync();
        }
    }
    for (auto &iter : sampleConsumerTaskMap_) {
        if (iter.second != nullptr) {
            iter.second->PauseAsync();
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
        StartTaskInner(it->first);
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
    }
    if (inPreroll_.load()) {
        if (CheckTrackEnabledById(videoTrackId_)) {
            taskMap_[videoTrackId_]->PauseAsync();
            taskMap_[videoTrackId_]->Pause();
            if (GetEnableSampleQueueFlag()) {
                sampleConsumerTaskMap_[videoTrackId_]->PauseAsync();
                sampleConsumerTaskMap_[videoTrackId_]->Pause();
            }
        }
    } else {
        PauseAllTaskAsync();
        PauseAllTask();
    }
    if (source_ != nullptr) {
        source_->SetReadBlockingFlag(true); // Enable source read blocking to ensure get wanted data
    }
    if (demuxerPluginManager_) {
        demuxerPluginManager_->Pause();
    }
    return Status::OK;
}

Status MediaDemuxer::PauseDragging()
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
    if (CheckTrackEnabledById(videoTrackId_)) {
            taskMap_[videoTrackId_]->PauseAsync();
            taskMap_[videoTrackId_]->Pause();
            if (GetEnableSampleQueueFlag()) {
                sampleConsumerTaskMap_[videoTrackId_]->PauseAsync();
                sampleConsumerTaskMap_[videoTrackId_]->Pause();
            }
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
    if (CheckTrackEnabledById(audioTrackId_)) {
            taskMap_[audioTrackId_]->PauseAsync();
            taskMap_[audioTrackId_]->Pause();
            if (GetEnableSampleQueueFlag()) {
                sampleConsumerTaskMap_[audioTrackId_]->PauseAsync();
                sampleConsumerTaskMap_[audioTrackId_]->Pause();
            }
    }
 
    if (source_ != nullptr) {
        source_->SetReadBlockingFlag(true); // Enable source read blocking to ensure get wanted data
    }
    return Status::OK;
}

Status MediaDemuxer::PauseTaskByTrackId(int32_t trackId)
{
    MEDIA_LOG_I("In, track %{public}d", trackId);
    FALSE_RETURN_V_MSG_E(IsValidTrackId(trackId), Status::ERROR_INVALID_PARAMETER, "Invalid track");

    // To accelerate DemuxerLoop thread to run into PAUSED state
    if (CheckTrackEnabledById(trackId)) {
            taskMap_[trackId]->PauseAsync();
            taskMap_[trackId]->Pause();
            if (GetEnableSampleQueueFlag()) {
                sampleConsumerTaskMap_[trackId]->PauseAsync();
                sampleConsumerTaskMap_[trackId]->Pause();
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
            StartTaskInner(videoTrackId_);
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
    ResetDraggingOpenGopCnt();
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    if (streamDemuxer_) {
        streamDemuxer_->Resume();
    }
    if (source_) {
        source_->Resume();
    }
    if (CheckTrackEnabledById(videoTrackId_)) {
        if (streamDemuxer_) {
            streamDemuxer_->SetIsIgnoreParse(false);
        }
        StartTaskInner(videoTrackId_);
    }
    isPaused_ = false;
    return Status::OK;
}

Status MediaDemuxer::ResumeAudioAlign()
{
    MEDIA_LOG_I("ResumeAudioAlign");
    {
        AutoLock lock(mapMutex_);
        auto it = bufferQueueMap_.begin();
        while (it != bufferQueueMap_.end()) {
            int32_t trackId = it->first;
            if (trackId == audioTrackId_) {
                bufferQueueMap_[trackId]->Clear();
                auto itSample = sampleQueueMap_.find(trackId);
                if (itSample != sampleQueueMap_.end() && itSample->second != nullptr) {
                    itSample->second->Clear();
                }
            }
            it++;
        }
    }
    if (streamDemuxer_) {
        streamDemuxer_->Resume();
    }
    if (source_) {
        source_->Resume();
    }
    if (CheckTrackEnabledById(audioTrackId_)) {
        if (streamDemuxer_) {
            streamDemuxer_->SetIsIgnoreParse(false);
        }
        StartTaskInner(audioTrackId_);
    }
    isPaused_ = false;
    return Status::OK;
}

void MediaDemuxer::ResetInner()
{
    std::map<int32_t, std::shared_ptr<TrackWrapper>> trackMap;
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
        sampleQueueMap_.clear();
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
    ResetSegmentEosMap();
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
    ResetSegmentEosMap();
    for (auto it = requestBufferErrorCountMap_.begin(); it != requestBufferErrorCountMap_.end(); it++) {
        it->second = 0;
    }
    InitPtsInfo();
    {
        std::unique_lock<std::mutex> stopLock(stopMutex_);
        isThreadExit_ = false;
        isStopped_ = false;
    }
    isDemuxerLoopExecuting_ = true;
    if (inPreroll_.load()) {
        if (CheckTrackEnabledById(videoTrackId_)) {
            StartTaskInner(videoTrackId_);
        }
        if (CheckTrackEnabledById(audioTrackId_)) {
            StartTaskInner(audioTrackId_);
        }
    } else {
        auto it = bufferQueueMap_.begin();
        while (it != bufferQueueMap_.end()) {
            int32_t trackId = it->first;
            if (CheckTrackEnabledById(trackId)) {
                StartTaskInner(trackId);
            } else {
                MEDIA_LOG_W("Track " PUBLIC_LOG_D32 " task is not exist", trackId);
            }
            it++;
        }
    }
    source_->Start();
    return demuxerPluginManager_->Start();
}

Status MediaDemuxer::Preroll()
{
    MEDIA_LOG_I("Preroll in");
    std::lock_guard<std::mutex> lock(prerollMutex_);
    if (inPreroll_.load()) {
        MEDIA_LOG_I("Preroll inPreroll_ return");
        return Status::OK;
    }
    if (!CheckTrackEnabledById(videoTrackId_)) {
        MEDIA_LOG_I("Preroll track not enabled return");
        return Status::OK;
    }
    inPreroll_.store(true);
    Status ret = Status::OK;
    if (isStopped_.load()) {
        MEDIA_LOG_D("Preroll Start");
        ret = Start();
    } else if (isPaused_.load()) {
        MEDIA_LOG_D("Preroll Resume");
        ret = Resume();
    }
    if (ret != Status::OK) {
        inPreroll_.store(false);
        MEDIA_LOG_E("Preroll failed, ret: %{public}d", ret);
    }
    MEDIA_LOG_I("Preroll done, ret: %{public}d", ret);
    return ret;
}

Status MediaDemuxer::PausePreroll()
{
    MEDIA_LOG_I("PausePreroll in");
    std::lock_guard<std::mutex> lock(prerollMutex_);
    if (!inPreroll_.load()) {
        MEDIA_LOG_I("PausePreroll inPreroll_ return");
        return Status::OK;
    }
    Status ret = Pause();
    inPreroll_.store(false);
    MEDIA_LOG_I("PausePreroll done");
    return ret;
}

Status MediaDemuxer::Stop()
{
    MEDIA_LOG_D("In");
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::Stop");
    FALSE_RETURN_V_MSG_E(!isThreadExit_, Status::OK, "Thread exit");
    if (useBufferQueue_) {
        FALSE_RETURN_V_MSG_E(!isStopped_, Status::OK, "Process has been stopped already, ignore");
        {
            std::unique_lock<std::mutex> stopLock(stopMutex_);
            isStopped_ = true;
        }
        StopAllTask();
    }
    if (source_ != nullptr) {
        source_->Stop();
    }
    if (streamDemuxer_) {
        streamDemuxer_->Stop();
    }
    if (demuxerPluginManager_) {
        demuxerPluginManager_->Stop();
    }
    return Status::OK;
}

bool MediaDemuxer::HasVideo()
{
    return IsValidTrackId(videoTrackId_);
}

bool MediaDemuxer::HasAudio()
{
    return IsValidTrackId(audioTrackId_);
}

void MediaDemuxer::SetIsCreatedByFilter(bool isCreatedByFilter)
{
    isCreatedByFilter_ = isCreatedByFilter;
}

void MediaDemuxer::InitMediaMetaData(const Plugins::MediaInfo& mediaInfo)
{
    AutoLock lock(mapMutex_);
    mediaMetaData_.globalMeta = std::make_shared<Meta>(mediaInfo.general);
    if (mediaMetaData_.globalMeta != nullptr && mediaMetaData_.globalMeta->GetData(Tag::MEDIA_FILE_TYPE, fileType_)) {
        MEDIA_LOG_D("FileType " PUBLIC_LOG_D32, static_cast<int32_t>(fileType_));
    }
    mediaMetaData_.trackMetas.clear();
    mediaMetaData_.trackMetas.reserve(mediaInfo.tracks.size());
    int32_t trackSize = static_cast<int32_t>(mediaInfo.tracks.size());
    for (int32_t index = 0; index < trackSize; index++) {
        auto trackMeta = mediaInfo.tracks[index];
        mediaMetaData_.trackMetas.emplace_back(std::make_shared<Meta>(trackMeta));
    }
}

void MediaDemuxer::UpdateMjpegMediaMetaData(Plugins::MediaInfo& mediaInfo)
{
    AutoLock lock(mapMutex_);
    int32_t trackSize = static_cast<int32_t>(mediaInfo.tracks.size());
    for (int32_t index = 0; index < trackSize; index++) {
        auto& trackMeta = mediaInfo.tracks[index];
        std::string mimeType;
        bool ret = trackMeta.Get<Tag::MIME_TYPE>(mimeType);
        if (ret && mimeType.find("image/jpeg") == 0) {
            auto isCover = trackMeta.Find(Tag::MEDIA_COVER);
            if (isCover != trackMeta.end())
                continue;
            mimeType = "video/mjpeg";
            MEDIA_LOG_I("MediaMetaData update to: " PUBLIC_LOG_S, mimeType.c_str());
            trackMeta.Set<Tag::MIME_TYPE>(mimeType);
        }
    }
}

void MediaDemuxer::InitDefaultTrack(const Plugins::MediaInfo& mediaInfo, int32_t& videoTrackId,
    int32_t& audioTrackId, int32_t& subtitleTrackId, std::string& videoMime)
{
    AutoLock lock(mapMutex_);
    std::string dafaultTrack = "[";
    int32_t trackSize = static_cast<int32_t>(mediaInfo.tracks.size());
    for (int32_t index = 0; index < trackSize; index++) {
        if (demuxerPluginManager_->CheckTrackIsActive(index) == false) {
            continue;
        }
        auto trackMeta = mediaInfo.tracks[index];
        std::string mimeType;
        bool ret = trackMeta.Get<Tag::MIME_TYPE>(mimeType);
        (void)trackMeta.Get<Tag::ORIGINAL_CODEC_NAME>(originalCodecName_);
        if (ret) {
            MEDIA_LOG_D("mimeType: " PUBLIC_LOG_S ", index: " PUBLIC_LOG_D32, mimeType.c_str(), index);
        }
        if (ret && mimeType.find("video") == 0 &&
            !IsTrackDisabled(Plugins::MediaType::VIDEO)) {
            isVideoTrackDisabled_ = false;
            dafaultTrack += "/V:";
            dafaultTrack += std::to_string(index);
            videoMime = mimeType;
            if (!IsValidTrackId(videoTrackId)) {
                videoTrackId = index;
            }
            if (!trackMeta.GetData(Tag::MEDIA_START_TIME, videoStartTime_)) {
                MEDIA_LOG_W("Get media start time failed");
            }
        } else if (ret && mimeType.find("audio") == 0 &&
            !IsTrackDisabled(Plugins::MediaType::AUDIO)) {
            dafaultTrack += "/A:";
            dafaultTrack += std::to_string(index);
            this->audioMime_ = mimeType;
            if (!IsValidTrackId(audioTrackId)) {
                audioTrackId = index;
            }
        } else if (ret && IsSubtitleMime(mimeType) &&
            !IsTrackDisabled(Plugins::MediaType::SUBTITLE)) {
            dafaultTrack += "/S:";
            dafaultTrack += std::to_string(index);
            if (!IsValidTrackId(subtitleTrackId)) {
                subtitleTrackId = index;
            }
        } else {}
    }
    dafaultTrack += "]";
    MEDIA_LOG_I(PUBLIC_LOG_S, dafaultTrack.c_str());
}

const std::string& MediaDemuxer::GetOriginalCodecName() const
{
    return originalCodecName_;
}

bool MediaDemuxer::IsOffsetValid(int64_t offset) const
{
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        return mediaDataSize_ == 0 || offset <= static_cast<int64_t>(mediaDataSize_);
    }
    return true;
}

bool MediaDemuxer::GetBufferFromUserQueue(int32_t queueIndex, int32_t size)
{
    MEDIA_LOG_DD("In, queue: " PUBLIC_LOG_D32 ", size: " PUBLIC_LOG_D32, queueIndex, size);
    if (GetEnableSampleQueueFlag()) {
        FALSE_RETURN_V_MSG_E(sampleQueueMap_.count(queueIndex) > 0 && sampleQueueMap_[queueIndex] != nullptr,
            false, "UserQueue " PUBLIC_LOG_D32 " is nullptr", queueIndex);
    } else {
        FALSE_RETURN_V_MSG_E(bufferQueueMap_.count(queueIndex) > 0 && bufferQueueMap_[queueIndex] != nullptr,
            false, "UserQueue " PUBLIC_LOG_D32 " is nullptr", queueIndex);
    }
    bool needSetSmallerSize = queueIndex == videoTrackId_ && hasSetLargeSize_ && !isVideoMuted_ && !needRestore_;
    if (needSetSmallerSize && sampleQueueMap_[queueIndex]->IsEmpty()) {
        sampleQueueMap_[queueIndex]->SetLargerQueueSize(SampleQueue::MAX_SAMPLE_QUEUE_SIZE);
        hasSetLargeSize_ = false;
    } else if (needSetSmallerSize) {
        return false;
    }
    bool needControlRead = !HasEosTrack() && queueIndex == videoTrackId_ && (isVideoMuted_ || needRestore_);
    if (needControlRead) {
        int64_t duration = 0;
        mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(duration);
        int64_t mediaTime = (duration > 0 && syncCenter_ != nullptr) ?
            syncCenter_->GetMediaTimeNow() : lastAudioPtsInMute_;
        if (lastVideoPts_ - mediaTime >= MAX_VIDEO_LEAD_TIME_ON_MUTE_US) {
            return false;
        }
    }

    AVBufferConfig avBufferConfig;
    if (isTranscoderMode_ && isSkippingAudioDecAndEnc_ && queueIndex == audioTrackId_) {
        avBufferConfig.memoryType = MemoryType::SHARED_MEMORY;
    }
    avBufferConfig.capacity = size;
    avBufferConfig.size = size;
    Status ret = Status::OK;
    if (GetEnableSampleQueueFlag()) {
        ret = sampleQueueMap_[queueIndex]->RequestBuffer(bufferMap_[queueIndex], avBufferConfig,
            REQUEST_BUFFER_TIMEOUT);
        bool needHandleSampleQueue = ret != Status::OK && isVideoMuted_ &&
            queueIndex == videoTrackId_ && !needReleaseVideoDecoder_;
        if (needHandleSampleQueue) {
            HandleVideoSampleQueue();
            ret = sampleQueueMap_[queueIndex]->RequestBuffer(bufferMap_[queueIndex], avBufferConfig,
                REQUEST_BUFFER_TIMEOUT);
        }
    } else {
        ret = bufferQueueMap_[queueIndex]->RequestBuffer(bufferMap_[queueIndex], avBufferConfig,
            REQUEST_BUFFER_TIMEOUT);
    }

    RecordErrorCount(queueIndex, ret);

    return ret == Status::OK;
}

void MediaDemuxer::RecordErrorCount(int32_t queueIndex, Status ret)
{
    if (ret != Status::OK) {
        requestBufferErrorCountMap_[queueIndex]++;
        if ((requestBufferErrorCountMap_[queueIndex] & 0x00000007) == 0) { // log per 8 times fail
            MEDIA_LOG_W("Request buffer failed, queue: " PUBLIC_LOG_D32 ", ret:" PUBLIC_LOG_D32
                ", errorCnt:" PUBLIC_LOG_U32, queueIndex,
                static_cast<int32_t>(ret), requestBufferErrorCountMap_[queueIndex]);
        }
        if (requestBufferErrorCountMap_[queueIndex] >= REQUEST_FAILED_RETRY_TIMES) {
            MEDIA_LOG_E("Request failed too many times in 1min");
        }
    } else {
        requestBufferErrorCountMap_[queueIndex] = 0;
        MEDIA_LOG_DD("RequestBuffer from UserQueue trackId=" PUBLIC_LOG_D32 ",size=" PUBLIC_LOG_U32, queueIndex, size);
    }
}

void MediaDemuxer::HandleSelectTrackStreamSeek(int32_t streamID, int32_t& trackId)
{
    int64_t startTime = 0;
    int64_t realSeekTime = 0;
    std::string mimeType;
    FALSE_RETURN(mediaMetaData_.trackMetas[trackId]->Get<Tag::MIME_TYPE>(mimeType) &&
        mediaMetaData_.trackMetas[trackId]->Get<Tag::MEDIA_START_TIME>(startTime));
    if (mimeType.find("audio") == 0) {
        Status retSeek = demuxerPluginManager_->SingleStreamSeekTo((lastAudioPts_ - startTime) / US_TO_S,
            SeekMode::SEEK_CLOSEST_SYNC, streamID, realSeekTime);
        MEDIA_LOG_I("Audio lastAudioPts_ " PUBLIC_LOG_D64 " relativePts " PUBLIC_LOG_D64
            " realSeekTime " PUBLIC_LOG_D64" ret " PUBLIC_LOG_D32, lastAudioPts_,
            lastAudioPts_ - startTime, realSeekTime, static_cast<int32_t>(retSeek));
    }
    if (mimeType == "application/x-subrip" || mimeType == "text/vtt") {
        Status retSeek = demuxerPluginManager_->SingleStreamSeekTo((lastSubtitlePts_ - startTime) / US_TO_S,
            SeekMode::SEEK_CLOSEST_SYNC, streamID, realSeekTime);
        MEDIA_LOG_I("Subtitle lastSubtitlePts_ " PUBLIC_LOG_D64 " relativePts " PUBLIC_LOG_D64
            " realSeekTime " PUBLIC_LOG_D64 " ret " PUBLIC_LOG_D32, lastSubtitlePts_,
            lastSubtitlePts_ - startTime, realSeekTime, static_cast<int32_t>(retSeek));
    }
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

    HandleSelectTrackStreamSeek(newStreamID, newTrackId);

    // update buffer queue
    bufferQueueMap_.insert(std::pair<int32_t, sptr<AVBufferQueueProducer>>(newTrackId,
        bufferQueueMap_[currentTrackId]));
    bufferMap_.insert(std::pair<int32_t, std::shared_ptr<AVBuffer>>(newTrackId,
        bufferMap_[currentTrackId]));
    bufferQueueMap_.erase(currentTrackId);
    bufferMap_.erase(currentTrackId);

    if (GetEnableSampleQueueFlag()) {
        AutoLock lock(mapMutex_);
        MEDIA_LOG_I("change TrackType: " PUBLIC_LOG_D32 ", TrackId " PUBLIC_LOG_D32 " >> " PUBLIC_LOG_D32,
            static_cast<int32_t>(type), currentTrackId, newTrackId);
        FALSE_RETURN_V_MSG_E(newTrackId != currentTrackId, true, "newTrackId equals currentTrackId");
        sampleQueueMap_.insert(
            std::pair<int32_t, std::shared_ptr<SampleQueue>>(newTrackId, sampleQueueMap_[currentTrackId]));
        sampleQueueMap_.erase(currentTrackId);
        bool hasSampleQueue = sampleQueueMap_.find(newTrackId) != sampleQueueMap_.end()
            && sampleQueueMap_[newTrackId] != nullptr;
        FALSE_RETURN_V_MSG_E(hasSampleQueue == true, false, "sampleQueueMap_ in newTrackId is null");
        sampleQueueMap_[newTrackId]->UpdateQueueId(newTrackId);
    }
    MEDIA_LOG_I("Out");
    return true;
}

bool MediaDemuxer::SelectTrackChangeStream(int32_t trackId)
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::SelectTrackChangeStream");
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, false, "Invalid param");
    TrackType type = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    int32_t newStreamID = -1;
    if (type == TRACK_AUDIO) {
        newStreamID = streamDemuxer_->GetNewAudioStreamID();
    } else if (type == TRACK_SUBTITLE) {
        newStreamID = streamDemuxer_->GetNewSubtitleStreamID();
    } else if (type == TRACK_VIDEO) {
        newStreamID = streamDemuxer_->GetNewVideoStreamID();
    } else {
        MEDIA_LOG_W("Invalid track " PUBLIC_LOG_D32, trackId);
        return false;
    }

    int32_t newTrackId;
    bool ret = HandleSelectTrackChangeStream(trackId, newStreamID, newTrackId);
    if (ret && eventReceiver_ != nullptr) {
        MEDIA_LOG_I("TrackType: " PUBLIC_LOG_D32 ", TrackId " PUBLIC_LOG_D32 " >> " PUBLIC_LOG_D32,
            static_cast<int32_t>(type), trackId, newTrackId);
        if (type == TrackType::TRACK_AUDIO) {
            audioTrackId_ = newTrackId;
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_AUDIO_TRACK_CHANGE, newTrackId});
            shouldCheckAudioFramePts_ = true;
        } else if (type == TrackType::TRACK_VIDEO) {
            videoTrackId_ = newTrackId;
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_VIDEO_TRACK_CHANGE, newTrackId});
        } else if (type == TrackType::TRACK_SUBTITLE) {
            subtitleTrackId_ = newTrackId;
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
        if (CheckTrackEnabledById(trackId)) {
            taskMap_[trackId]->StopAsync(); // stop self
            if (GetEnableSampleQueueFlag()) {
                sampleConsumerTaskMap_[trackId]->StopAsync();
            }
        }
    }
    return ret;
}

bool MediaDemuxer::SelectBitRateChangeStream(int32_t trackId)
{
    (void) trackId;
    FALSE_RETURN_V(IsValidTrackId(videoTrackId_), false);
    int32_t currentStreamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(videoTrackId_);
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
        if (isHlsFmp4_ && IsAVInOneStream()) {
            demuxerPluginManager_->GetTrackInfoByStreamID(newStreamID, newTrackId, newInnerTrackId, TRACK_VIDEO);
            demuxerPluginManager_->UpdateTempTrackMapInfo(videoTrackId_, newTrackId, newInnerTrackId);
            newInnerTrackId = -1;
            newTrackId = -1;
            if (IsValidTrackId(audioTrackId_)) {
                demuxerPluginManager_->GetTrackInfoByStreamID(newStreamID, newTrackId, newInnerTrackId, TRACK_AUDIO);
                demuxerPluginManager_->UpdateTempTrackMapInfo(audioTrackId_, newTrackId, newInnerTrackId);
            }
        } else {
            demuxerPluginManager_->GetTrackInfoByStreamID(newStreamID, newTrackId, newInnerTrackId);
            demuxerPluginManager_->UpdateTempTrackMapInfo(videoTrackId_, newTrackId, newInnerTrackId);
        }

        MEDIA_LOG_I("Updata info");
        InnerSelectTrack(videoTrackId_);
        if (isHlsFmp4_ && IsValidTrackId(audioTrackId_)) {
            InnerSelectTrack(audioTrackId_);
        }
        MEDIA_LOG_I("Out");
        return true;
    }
    return false;
}

void MediaDemuxer::DumpBufferToFile(int32_t trackId, std::shared_ptr<AVBuffer> buffer)
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

void MediaDemuxer::StartTaskInner(int32_t trackId)
{
    auto taskIt = taskMap_.find(trackId);
    if (taskIt != taskMap_.end() && taskIt->second != nullptr) {
        taskIt->second->Start();
    } else {
        MEDIA_LOG_W("Track " PUBLIC_LOG_D32 " task is not exist", trackId);
    }
    if (GetEnableSampleQueueFlag()) {
        auto sampleConsumerTaskIt = sampleConsumerTaskMap_.find(trackId);
        if (sampleConsumerTaskIt != sampleConsumerTaskMap_.end() && sampleConsumerTaskIt->second != nullptr) {
            FALSE_RETURN_MSG(trackId != videoTrackId_ || !isVideoMuted_ || needReleaseVideoDecoder_,
                "sampleConsumerV is pause on mute, do not need to start");
            sampleConsumerTaskIt->second->Start();
        } else {
            MEDIA_LOG_W("Track " PUBLIC_LOG_D32 " sampleConsumerTask is not exist", trackId);
        }
    }
}

Status MediaDemuxer::PushBufferToQueue(int32_t trackId, std::shared_ptr<AVBuffer>& buffer, bool available)
{
    return GetEnableSampleQueueFlag() ? sampleQueueMap_[trackId]->PushBuffer(buffer, available) :
                    bufferQueueMap_[trackId]->PushBuffer(buffer, available);
}

void MediaDemuxer::HandleVideoTrack(int32_t trackId)
{
    if (isVideoMuted_ && (bufferMap_[trackId]->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME))) {
        // callback release decoder
        if (needReleaseVideoDecoder_) {
            needReleaseVideoDecoder_ = false;
            MEDIA_LOG_I("MediaDemuxer::HandleReadSample read key frame, ReleaseVideoDecoder");
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_RELEASE_VIDEO_DECODER, trackId});
            bool needPauseSampleConsumer = sampleConsumerTaskMap_.find(videoTrackId_) !=
                sampleConsumerTaskMap_.end() && sampleConsumerTaskMap_[videoTrackId_] != nullptr &&
                sampleConsumerTaskMap_[videoTrackId_]->IsTaskRunning();
            if (needPauseSampleConsumer) {
                sampleConsumerTaskMap_[videoTrackId_]->PauseAsync();
                sampleConsumerTaskMap_[videoTrackId_]->Pause();
            }
            if (!hasSetLargeSize_) {
                sampleQueueMap_[videoTrackId_]->SetLargerQueueSize(SAMPLE_QUEUE_SIZE_ON_MUTE);
                hasSetLargeSize_ = true;
            }
        }
        sampleQueueMap_[trackId]->Clear();
    }
    lastVideoPts_ = bufferMap_[trackId]->pts_;
}

Status MediaDemuxer::HandleReadSample(int32_t trackId)
{
    Status ret = InnerReadSample(trackId, bufferMap_[trackId], false);
    bool isBufferSizeValid = bufferMap_[trackId] != nullptr ? bufferMap_[trackId]->GetConfig().size > 0 : true;
    if (IsRightMediaTrack(trackId, DemuxerTrackType::VIDEO)) {
        std::unique_lock<std::mutex> draggingLock(draggingMutex_);
        HandleVideoTrack(trackId);
        if (VideoStreamReadyCallback_ != nullptr) {
            if (ret != Status::OK && ret != Status::END_OF_STREAM) {
                PushBufferToQueue(trackId, bufferMap_[trackId], false);
                MEDIA_LOG_E("Read failed, track " PUBLIC_LOG_D32 ", ret:" PUBLIC_LOG_D32,
                    trackId, static_cast<int32_t>(ret));
                return ret;
            }
            std::shared_ptr<VideoStreamReadyCallback> videoStreamReadyCallback = VideoStreamReadyCallback_;
            draggingLock.unlock();
            bool isDiscardable = videoStreamReadyCallback->IsVideoStreamDiscardable(bufferMap_[trackId]);
            HandleEosDrag(trackId, isDiscardable);
            UpdateSyncFrameInfo(bufferMap_[trackId], trackId, isDiscardable);
            CopyBufferToDfxBufferQueue(bufferMap_[trackId], !isDiscardable && isBufferSizeValid);
            PushBufferToQueue(trackId, bufferMap_[trackId], !isDiscardable && isBufferSizeValid);
            return Status::OK;
        }
    }

    HandleSeek(trackId);
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        if (bufferMap_[trackId]->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)) {
            return HandleTrackEos(trackId);
        }
        FALSE_GOON_NOEXEC(isAutoMaintainPts_, HandleAutoMaintainPts(trackId, bufferMap_[trackId]));
        lastVideoPts_ = trackId == videoTrackId_ ? bufferMap_[trackId]->pts_ : lastVideoPts_;
        lastAudioPtsInMute_ = trackId == audioTrackId_ ? bufferMap_[trackId]->pts_ : lastAudioPtsInMute_;
        bool isDroppable = IsBufferDroppable(bufferMap_[trackId], trackId);
        if (ptsManagedFileTypes.find(fileType_) != ptsManagedFileTypes.end() && trackId == videoTrackId_) {
            SetOutputBufferPts(bufferMap_[trackId]);
        }
        FALSE_GOON_NOEXEC(isTranscoderMode_, TranscoderUpdateOutputBufferPts(trackId, bufferMap_[trackId]));
        CopyBufferToDfxBufferQueue(bufferMap_[trackId], !isDroppable && isBufferSizeValid);
        PushBufferToQueue(trackId, bufferMap_[trackId], !isDroppable && isBufferSizeValid);
    } else {
        PushBufferToQueue(trackId, bufferMap_[trackId], false);
        MEDIA_LOG_E("Read failed, track " PUBLIC_LOG_D32 ", ret:" PUBLIC_LOG_D32, trackId, static_cast<int32_t>(ret));
    }
    return ret;
}

void MediaDemuxer::HandleEosDrag(int32_t trackId, bool isDiscardable)
{
    if (bufferMap_[trackId]->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS) && !isDiscardable) {
        eosMap_[trackId] = true;
    }
}

void MediaDemuxer::CopyBufferToDfxBufferQueue(std::shared_ptr<AVBuffer> buffer, bool dropable)
{
    FALSE_RETURN_NOLOG(dfxBufferQueueProducer_ != nullptr && dfxBufferQueueConsumer_ != nullptr);
    FALSE_RETURN_NOLOG(!dropable);
    std::shared_ptr<AVBuffer> dfxBuffer = nullptr;
    auto config = buffer->GetConfig();
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    auto res = dfxBufferQueueProducer_->RequestBuffer(dfxBuffer, config, REQUEST_BUFFER_TIMEOUT);
    if (res != Status::OK) {
        std::shared_ptr<AVBuffer> tmpBuffer = nullptr;
        dfxBufferQueueConsumer_->AcquireBuffer(tmpBuffer);
        dfxBufferQueueConsumer_->ReleaseBuffer(tmpBuffer);
        res = dfxBufferQueueProducer_->RequestBuffer(dfxBuffer, config, REQUEST_BUFFER_TIMEOUT);
    }
    FALSE_RETURN(res == Status::OK && dfxBuffer != nullptr);
    res = AVBuffer::Clone(buffer, dfxBuffer);
    TRUE_LOG(res != Status::OK, MEDIA_LOG_E, "Clone AVBuffer failed, errCode %{public}d", static_cast<int32_t>(res));
    dfxBufferQueueProducer_->PushBuffer(dfxBuffer, res == Status::OK);
}

std::string Sha256HashMemory(const void* data, size_t size)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, size);
    SHA256_Final(hash, &sha256);

    static const int32_t setWNum = 2;
    std::stringstream ss;
    for (int32_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(setWNum) << std::setfill('0') << static_cast<int32_t>(hash[i]);
    }
    return ss.str();
}

void MediaDemuxer::HandleDecoderErrorFrame(int64_t pts)
{
    FALSE_RETURN_NOLOG(dfxBufferQueueConsumer_ != nullptr && dfxBufferQueueProducer_ != nullptr);
    for (std::shared_ptr<AVBuffer> buffer = nullptr; dfxBufferQueueConsumer_->AcquireBuffer(buffer) == Status::OK;) {
        ON_SCOPE_EXIT(0) {
            dfxBufferQueueConsumer_->ReleaseBuffer(buffer);
        };
        if (buffer->pts_ == pts) {
            FALSE_RETURN_MSG_W(
                buffer->memory_->GetAddr() != nullptr && buffer->memory_->GetSize() > 0, "invalid buffer");
            auto hashStr = Sha256HashMemory(buffer->memory_->GetAddr(), buffer->memory_->GetSize());
            MEDIA_LOG_E("InputBuffer hash res %{public}s", hashStr.c_str());
            return;
        }
        continue;
    }
    MEDIA_LOG_E("Cant find buffer with same pts " PUBLIC_LOG_D64 ", maybe Gop is too long", pts);
}

void MediaDemuxer::HandleSeek(int32_t trackId)
{
    if (source_ != nullptr && source_->IsSeekToTimeSupported() && isSeeked_ && HasVideo()) {
        if (trackId == videoTrackId_ && isFirstFrameAfterSeek_.load()) {
            bool isSyncFrame = (bufferMap_[trackId]->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) != 0;
            if (!isSyncFrame) {
                MEDIA_LOG_E("The first frame after seeking is not a sync frame");
            }
            isFirstFrameAfterSeek_.store(false);
        }
        MEDIA_LOG_I("Seeking, found idr frame track " PUBLIC_LOG_D32, trackId);
        isSeeked_ = false;
    }
}

Status MediaDemuxer::HandleTrackEos(int32_t trackId)
{
    eosMap_[trackId] = true;
    if (taskMap_.find(trackId) != taskMap_.end() && taskMap_[trackId] != nullptr) {
        taskMap_[trackId]->StopAsync();
    }
    MEDIA_LOG_I("Track eos, track: " PUBLIC_LOG_D32 ", bufferId: " PUBLIC_LOG_U64
        ", pts: " PUBLIC_LOG_D64 ", flag: " PUBLIC_LOG_U32, trackId, bufferMap_[trackId]->GetUniqueId(),
        bufferMap_[trackId]->pts_, bufferMap_[trackId]->flag_);
    PushBufferToQueue(trackId, bufferMap_[trackId], true);
    if (trackId == videoTrackId_ && isVideoMuted_) {
        ReportEosEvent();
    }
    return Status::OK;
}

Status MediaDemuxer::GenerateDfxBufferQueue(int32_t trackId)
{
    FALSE_RETURN_V_NOLOG(enableDfxBufferQueue_ && trackId == videoTrackId_, Status::OK);
    dfxBufferQueue_ = AVBufferQueue::Create(DFX_BUFFER_QUEUE_SIZE_MAX, MemoryType::VIRTUAL_MEMORY, "DfxBufferQueue");
    dfxBufferQueueProducer_ = dfxBufferQueue_->GetProducer();
    dfxBufferQueueConsumer_ = dfxBufferQueue_->GetConsumer();

    static const int32_t normalBufferSize = 256 * 1024;
    for (uint32_t i = 0; i < DFX_BUFFER_QUEUE_SIZE_MAX; i++) {
        auto avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
        std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(avAllocator, normalBufferSize);
        FALSE_RETURN_V_MSG_E(buffer != nullptr, Status::ERROR_NO_MEMORY, "CreateAVBuffer failed");
        Status status = dfxBufferQueueProducer_->AttachBuffer(buffer, false);
        FALSE_RETURN_V_MSG_E(
            status == Status::OK, status, "AttachBuffer failed status=" PUBLIC_LOG_D32, static_cast<int32_t>(status));
    }
    MEDIA_LOG_I("Generate DFX buffer queue success");
    return Status::OK;
}

void MediaDemuxer::ReportEosEvent()
{
    MEDIA_LOG_I("MediaDemuxer ReportEOSEvent");
    FALSE_RETURN_MSG(eventReceiver_ != nullptr, "MediaDemuxer ReportEOSEvent without eventReceiver_");
    Event event {
        .srcFilter = "VideoSink",
        .type = EventType::EVENT_COMPLETE,
    };
    eventReceiver_->OnEvent(event);
}

void MediaDemuxer::SetOutputBufferPts(std::shared_ptr<AVBuffer> &outputBuffer)
{
    FALSE_RETURN_MSG(outputBuffer != nullptr, "outputBuffer is nullptr.");

    MEDIA_LOG_DD("OutputBuffer PTS: " PUBLIC_LOG_D64 " DTS: " PUBLIC_LOG_D64, outputBuffer->pts_, outputBuffer->dts_);
    outputBuffer->pts_ = outputBuffer->dts_;
}

void MediaDemuxer::TranscoderUpdateOutputBufferPts(int32_t trackId, std::shared_ptr<AVBuffer> &outputBuffer)
{
    FALSE_RETURN_NOLOG(isTranscoderMode_);
    if (transcoderStartPts_ > 0 && outputBuffer != nullptr) {
        outputBuffer->pts_ -= transcoderStartPts_;
    }
}

bool MediaDemuxer::HandleDashChangeStream(int32_t trackId)
{
    // the caller should insure demuxerPluginManager_ not nullptr and isDash_ true
    FALSE_RETURN_V_MSG_E(streamDemuxer_ != nullptr, false, "Stream is nullptr");

    MEDIA_LOG_D("IN");
    TrackType type = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    int32_t currentStreamID = demuxerPluginManager_->GetStreamIDByTrackType(type);
    int32_t newStreamID = demuxerPluginManager_->GetStreamDemuxerNewStreamID(type, streamDemuxer_);
    bool ret = false;
    FALSE_RETURN_V_NOLOG(newStreamID != -1 && currentStreamID != newStreamID, ret);

    AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGE, LOG_INTERVAL_MS, LOG_MAX_COUNT,
        "Change stream begin, currentStreamID: " PUBLIC_LOG_D32 " newStreamID: " PUBLIC_LOG_D32,
        currentStreamID, newStreamID);
    if ((trackId == videoTrackId_ || (isHlsFmp4_ && IsAVInOneStream())) &&
        demuxerPluginManager_->GetCurrentBitRate() != targetBitRate_) {
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
    AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGE, LOG_INTERVAL_MS, LOG_MAX_COUNT, "Change stream success");
    return ret;
}

void MediaDemuxer::RecordDemuxerTimeStamp(AVBuffer &buffer, StallingStage stage)
{
    int64_t nowTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    std::vector<int64_t> timeStampList;
    buffer.meta_->GetData(Tag::STALLING_TIMESTAMP, timeStampList);

    if (stage == StallingStage::DEMUXER_START) {
        timeStampList.clear();
    }
    timeStampList.push_back(static_cast<int64_t>(stage));
    timeStampList.push_back(nowTime);
    buffer.meta_->SetData(Tag::STALLING_TIMESTAMP, timeStampList);
    MEDIA_LOG_D("demuxer set stalling stage:" PUBLIC_LOG_D64 ", nowTimeMs:" PUBLIC_LOG_D64, stage, nowTime);
}

Status MediaDemuxer::CopyFrameToUserQueue(int32_t trackId)
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::CopyFrameToUserQueue");
    MEDIA_LOG_D("CopyFrameToUserQueue IN, track:" PUBLIC_LOG_D32, trackId);

    int32_t innerTrackID = trackId;
    int32_t id = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = demuxerPluginManager_->GetPluginByStreamID(id);
    FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
    if (IsNeedMapToInnerTrackID()) {
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    }
    GetMemoryUsage(trackId, pluginTemp);
    int32_t size = 0;
    Status ret = enableAsyncDemuxer_ ?
        pluginTemp->GetNextSampleSize(static_cast<uint32_t>(innerTrackID), size, timeout_) :
        pluginTemp->GetNextSampleSize(static_cast<uint32_t>(innerTrackID), size);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_WAIT_TIMEOUT, ret, "Get size timeout " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_UNKNOWN, ret, "Get size failed for track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_AGAIN, ret,
        "Get size failed for track " PUBLIC_LOG_D32 ", retry", trackId);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_NO_MEMORY, ret, "Get size failed for track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_WRONG_STATE, ret, " Get size interrupt");
    if (demuxerPluginManager_->IsDash() && HandleDashChangeStream(trackId)) {
        MEDIA_LOG_I("HandleDashChangeStream success");
        return Status::OK;
    }
    int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
    if (isHls_ && ret == Status::END_OF_STREAM && !source_->IsHlsEnd(streamId)) {
        return HandleSegmentEos(trackId);
    }
    SetTrackNotifyFlag(trackId, true);
    if (!GetBufferFromUserQueue(trackId, size)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    SetTrackNotifyFlag(trackId, false);
    ret = HandleReadSample(trackId);
    ProduceWaterLoopControl(trackId);
    BufferingStatus(trackId);
    MEDIA_LOG_DD("CopyFrameToUserQueue Out, track:" PUBLIC_LOG_D32, trackId);
    return ret;
}

void MediaDemuxer::StartConsume(int32_t trackId)
{
    bool startConsumeResult = false;
    {
        AutoLock lock(mapMutex_);
        startConsumeResult = sampleQueueController_->ShouldStartConsume(trackId, sampleQueueMap_[trackId],
            sampleConsumerTaskMap_[trackId]);
    }
    if (!startConsumeResult && !eosMap_[trackId]) {
        // if controllor do not start consume, and not eos, do nothing
        return;
    }

    // set is buffering
    isBufferingMap_[trackId].store(false);
    if (IsValidTrackId(audioTrackId_)) {
        // if both has video & audio, both check and start consumer task
        if (isBufferingMap_[videoTrackId_] || isBufferingMap_[audioTrackId_]) {
            return;
        }
        if (sampleConsumerTaskMap_[audioTrackId_] && !sampleConsumerTaskMap_[audioTrackId_]->IsTaskRunning()) {
            MEDIA_LOG_I("Audio StartConsume, trackId: %{public}d", audioTrackId_);
            sampleConsumerTaskMap_[audioTrackId_]->Start();
        }
        if (sampleConsumerTaskMap_[videoTrackId_] && !sampleConsumerTaskMap_[videoTrackId_]->IsTaskRunning()) {
            MEDIA_LOG_I("Video StartConsume, trackId: %{public}d", videoTrackId_);
            sampleConsumerTaskMap_[videoTrackId_]->Start();
        }
        CheckAndReportBufferingStatus(EventType::BUFFERING_END);
        return;
    }
    if (isBufferingMap_[videoTrackId_]) {
        return;
    }
    if (sampleConsumerTaskMap_[videoTrackId_] && !sampleConsumerTaskMap_[videoTrackId_]->IsTaskRunning()) {
        MEDIA_LOG_I("Pure Video StartConsume, trackId: %{public}d", videoTrackId_);
        sampleConsumerTaskMap_[videoTrackId_]->Start();
    }
    CheckAndReportBufferingStatus(EventType::BUFFERING_END);
}

void MediaDemuxer::ProduceWaterLoopControl(int32_t trackId)
{
    if (!sampleQueueController_ || trackId == subtitleTrackId_ || isVideoMuted_ || IsLocalFd()) {
        return;
    }
    StartConsume(trackId);
    {
        AutoLock lock(mapMutex_);
        sampleQueueController_->ShouldStopProduce(trackId, sampleQueueMap_[trackId], taskMap_[trackId]);
    }
}

void MediaDemuxer::BufferingStatus(int32_t trackId)
{
    if (IsLocalFd() || sampleQueueMap_.find(trackId) == sampleQueueMap_.end() || sampleQueueMap_[trackId] == nullptr) {
        return;
    }
    if (isBufferingMap_[videoTrackId_].load()) {
        int64_t percent = static_cast<int64_t>((sampleQueueMap_[trackId]->NewGetCacheDuration() * 100) /
            SampleQueueController::START_CONSUME_WATER_LOOP);
        MEDIA_LOG_I("BUFFERING_PERCENT: %{public}lld", percent);
        auto eventReceiver = eventReceiver_;
        if (eventReceiver) {
            eventReceiver->OnEvent({"demuxer_filter", EventType::EVENT_BUFFER_PROGRESS, percent});
        }
    }
    auto cachedDuration = static_cast<int64_t>(sampleQueueMap_[trackId]->NewGetCacheDuration() / US_TO_MS);
    MEDIA_LOG_I("CACHED_DURATION: %{public}lld", cachedDuration);
    auto eventReceiver = eventReceiver_;
    if (eventReceiver) {
        eventReceiver->OnEvent({"demuxer_filter", EventType::EVENT_CACHED_DURATION, cachedDuration});
    }
}

Status MediaDemuxer::InnerReadSample(int32_t trackId, std::shared_ptr<AVBuffer> sample, bool isAVDemuxer)
{
    MEDIA_LOG_DD("InnerReadSample In, track " PUBLIC_LOG_D32, trackId);
    RecordDemuxerTimeStamp(*sample, StallingStage::DEMUXER_START);
    int32_t innerTrackID = trackId;
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    if (IsNeedMapToInnerTrackID()) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(demuxerPluginManager_->GetStreamIDByTrackID(trackId));
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
    }

    int64_t threshold = trackId == audioTrackId_ ? READSAMPLE_AUIDO_WARNING_MS : READSAMPLE_WARNING_MS;
    Status ret = Status::OK;
    {
        ScopedTimer timer("ReadSample", threshold);
        ret = ReadSampleWithPerfRecord(pluginTemp, innerTrackID, sample, isAVDemuxer);
    }
    if (ret == Status::END_OF_STREAM) {
        MEDIA_LOG_I("Read eos for track " PUBLIC_LOG_D32, trackId);
    } else if (ret != Status::OK) {
        MEDIA_LOG_I("Read error for track " PUBLIC_LOG_D32 ", ret: " PUBLIC_LOG_D32,
            trackId, (int32_t)(ret));
    }
    MEDIA_LOG_D("InnerReadSample Out, track " PUBLIC_LOG_D32, trackId);

    // to get DrmInfo
    ProcessDrmInfos();
    RecordDemuxerTimeStamp(*sample, StallingStage::DEMUXER_END);
    return ret;
}

Status MediaDemuxer::ReadSampleWithPerfRecord(const std::shared_ptr<Plugins::DemuxerPlugin> &pluginTemp,
    const int32_t &innerTrackID, const std::shared_ptr<AVBuffer> &sample, bool isAVDemuxer)
{
    Status ret = Status::OK;
    int64_t demuxDuration = 0;
    if (isAVDemuxer || !enableAsyncDemuxer_) {
        FALSE_RETURN_V_NOLOG(perfRecEnabled_, pluginTemp->ReadSample(static_cast<uint32_t>(innerTrackID), sample));
        demuxDuration =
            CALC_EXPR_TIME_MS(ret = pluginTemp->ReadSample(static_cast<uint32_t>(innerTrackID), sample));
    } else {
        FALSE_RETURN_V_NOLOG(perfRecEnabled_,
            pluginTemp->ReadSample(static_cast<uint32_t>(innerTrackID), sample, timeout_));
        demuxDuration =
            CALC_EXPR_TIME_MS(ret = pluginTemp->ReadSample(static_cast<uint32_t>(innerTrackID), sample, timeout_));
    }
    FALSE_RETURN_V_MSG(eventReceiver_ != nullptr, Status::OK, "Report perf failed, callback is nullptr");
    FALSE_RETURN_V_NOLOG(perfRecorder_.Record(demuxDuration) == PerfRecorder::FULL, ret);
    eventReceiver_->OnDfxEvent({ "DEMUX", DfxEventType::DFX_INFO_PERF_REPORT, perfRecorder_.GetMainPerfData() });
    perfRecorder_.Reset();
    return ret;
}

Status MediaDemuxer::SetPerfRecEnabled(bool isPerfRecEnabled)
{
    MEDIA_LOG_I("widdraw DoSetPerfRecEnabled %{public}d", isPerfRecEnabled);
    perfRecEnabled_ = isPerfRecEnabled;
    FALSE_RETURN_V_MSG(source_ != nullptr, Status::ERROR_NO_MEMORY, "Source not exist, no memory");
    source_->SetPerfRecEnabled(isPerfRecEnabled);
    return Status::OK;
}

int64_t MediaDemuxer::GetReadLoopRetryUs(int32_t trackId)
{
    FALSE_RETURN_V_NOLOG(GetEnableSampleQueueFlag(), 0);
    FALSE_RETURN_V_NOLOG(isFlvLiveStream_, NEXT_DELAY_TIME_US);
    FALSE_RETURN_V_MSG_E(sampleQueueMap_.count(trackId) > 0 && sampleQueueMap_[trackId] != nullptr, NEXT_DELAY_TIME_US,
        "sampleQueue " PUBLIC_LOG_D32 " is nullptr", trackId);
    uint64_t sampleDuration = sampleQueueMap_[trackId]->GetCacheDuration();
    if (sampleDuration <= SAMPLE_FLOW_CONTROL_MIN_SAMPLE_DURATION_US  ||
        ((isVideoMuted_ || needRestore_ || hasSetLargeSize_) && trackId == videoTrackId_)) {
        return NEXT_DELAY_TIME_US;
    }
    return static_cast<int64_t>(sampleDuration >> SAMPLE_FLOW_CONTROL_RATE_POW);
}

int64_t MediaDemuxer::DoBeforeEachLoop(int32_t trackId)
{
    FALSE_RETURN_V_NOLOG(demuxerPluginManager_ != nullptr, 0);
    auto trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    auto hasRegisteredFunc = funcBeforeReadSampleMap_.find(trackType) != funcBeforeReadSampleMap_.end();
    FALSE_RETURN_V_NOLOG(hasRegisteredFunc, 0);
    return funcBeforeReadSampleMap_[trackType](trackId);
}

int64_t MediaDemuxer::DoBeforeSubtitleTrackReadLoop(int32_t trackId)
{
    FALSE_RETURN_V_NOLOG(!demuxerPluginManager_->IsDash() && subStreamDemuxer_ == nullptr, static_cast<int64_t>(0));
    auto subtitleStreamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
    auto subtitleDemuxerPlugin = demuxerPluginManager_->GetPluginByStreamID(subtitleStreamId);
    FALSE_RETURN_V_MSG_E(subtitleDemuxerPlugin != nullptr, RETRY_DELAY_TIME_US,
        "Invalid demuxer plugin, unabled to read subtitle sample");
    uint32_t cacheSize = 0;
    auto res = subtitleDemuxerPlugin->GetCurrentCacheSize(trackId, cacheSize);
    // Only if demuxer plugin has subtitle cache can read subtitle sample
    if (res == Status::OK && cacheSize > 0) {
        MEDIA_LOG_DD("Demuxer plugin has cached subtitle data size " PUBLIC_LOG_U32, cacheSize);
        return static_cast<int64_t>(0);
    }
    MEDIA_LOG_DD("Invalid cache size for subtitle track GetCurrentCacheSize res " PUBLIC_LOG_D32
                " size " PUBLIC_LOG_U32, static_cast<int32_t>(res), cacheSize);
    return RETRY_DELAY_TIME_US;
}

std::string MediaDemuxer::GetMime()
{
    std::string mime;
    if (!videoMime_.empty()) {
        mime = videoMime_;
    }
    if (mime == "" && !audioMime_.empty()) {
        mime = audioMime_;
    }
    return mime;
}

void MediaDemuxer::HandleNotAllTrackEos(int32_t trackId)
{
    hlsSegmentEosMap_[trackId] = true;
    if (isBufferingMap_[trackId].load() && (!taskMap_[audioTrackId_]->IsTaskRunning() ||
        !taskMap_[videoTrackId_]->IsTaskRunning())) {
        isBufferingMap_[trackId].store(false);
        CheckAndReportBufferingStatus(EventType::BUFFERING_END);
    }
}

void MediaDemuxer::CheckAndReportBufferingStatus(EventType type)
{
    if (type == EventType::BUFFERING_END && isBuffering_.load()) {
        MEDIA_LOG_I("BUFFERING_END");
        isBuffering_.store(false);
        auto eventReceiver = eventReceiver_;
        if (eventReceiver) {
            eventReceiver->OnEvent({"demuxer_filter", type, PAUSE});
        }
        return;
    }

    if (type == EventType::BUFFERING_START && !isBuffering_.load()) {
        MEDIA_LOG_I("BUFFERING_START");
        auto eventReceiver = eventReceiver_;
        if (eventReceiver) {
            eventReceiver->OnEvent({"demuxer_filter", type, START});
        }
        isBuffering_.store(true);
    }
}

int64_t MediaDemuxer::ReadLoop(int32_t trackId)
{
    if (streamDemuxer_->GetIsIgnoreParse() || isStopped_ || isPaused_ || isSeekError_ || isFlvLiveSelectingBitRate_) {
        MEDIA_LOG_D("ReadLoop pausing or error, track " PUBLIC_LOG_D32, trackId);
        perfRecorder_.Reset();
        return 6 * 1000; // sleep 6ms in pausing to avoid useless reading
    }
    auto resPreReadSample = DoBeforeEachLoop(trackId);
    FALSE_RETURN_V_NOLOG(resPreReadSample == 0, resPreReadSample);
    Status ret = CopyFrameToUserQueue(trackId);
    if (ret == Status::ERROR_ONE_TRACK_SEGMENT_EOS) {
        HandleNotAllTrackEos(trackId);
    }
    // when read failed, or request always failed in 1min, send error event
    bool ignoreError = isStopped_ || isPaused_ || isInterruptNeeded_.load();
    if ((ret == Status::ERROR_UNKNOWN && !ignoreError) ||
            requestBufferErrorCountMap_[trackId] >= REQUEST_FAILED_RETRY_TIMES) {
        MEDIA_LOG_E("Invalid data source, can not get frame");
        if (eventReceiver_ != nullptr) {
            eventReceiver_->OnEvent(
                {"demuxer_filter", EventType::EVENT_ERROR, MSERR_DATA_SOURCE_ERROR_UNKNOWN, GetMime()});
        } else {
            MEDIA_LOG_D("EventReceiver is nullptr");
        }
    }
    FALSE_GOON_NOEXEC(ret == Status::ERROR_PACKET_CONVERT_FAILED, HandlePacketConvertError());
    FALSE_GOON_NOEXEC(ret == Status::OK, convertErrorTime_.store(0));
    bool isNeedRetry = ret == Status::OK || ret == Status::ERROR_AGAIN || ret == Status::ERROR_WAIT_TIMEOUT;
    if (isNeedRetry) {
        return GetReadLoopRetryUs(trackId);
    }
    if (ret == Status::ERROR_NO_MEMORY) {
        MEDIA_LOG_E("Cache data size is out of limit");
        if (eventReceiver_ != nullptr && !isOnEventNoMemory_.load()) {
            isOnEventNoMemory_.store(true);
            eventReceiver_->OnEvent(
                {"demuxer_filter", EventType::EVENT_ERROR, MSERR_DEMUXER_BUFFER_NO_MEMORY, GetMime()});
        }
        return GetEnableSampleQueueFlag() ? NEXT_DELAY_TIME_US : 0;
    }
    MEDIA_LOG_DD("ReadLoop wait, track:" PUBLIC_LOG_D32 ", ret:" PUBLIC_LOG_D32,
        trackId, static_cast<int32_t>(ret));
    return RETRY_DELAY_TIME_US; // delay to retry if no frame
}

void MediaDemuxer::HandlePacketConvertError()
{
    ++convertErrorTime_;
    TRUE_LOG(convertErrorTime_.load() == 1, MEDIA_LOG_W, "PacketConvert error once");
    FALSE_RETURN_NOLOG(convertErrorTime_ >= CONVERT_PACKET_ERROR_MAX_COUNT);
    MEDIA_LOG_E("PacketConvertError happened %{public}d times, stream is unsupported!", convertErrorTime_.load());
    FALSE_RETURN_MSG(eventReceiver_ != nullptr, "eventReceiver_ is nullptr");
    eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_DATA_SOURCE_ERROR_UNKNOWN, GetMime()});
}

Status MediaDemuxer::ReadSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE, "Not buffer queue mode");
    MEDIA_LOG_DD("ReadSample In");
    FALSE_RETURN_V_MSG_E(eosMap_.count(trackIndex) > 0, Status::ERROR_INVALID_OPERATION, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_!=nullptr, Status::ERROR_INVALID_PARAMETER,
        "AVBuffer is nullptr");
    if (eosMap_[trackIndex]) {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has reached eos", trackIndex);
        sample->flag_ = static_cast<uint32_t>(AVBufferFlag::EOS);
        sample->memory_->SetSize(0);
        return Status::END_OF_STREAM;
    }
    Status ret = InnerReadSample(static_cast<int32_t>(trackIndex), sample, true);
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        if (sample->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)) {
            eosMap_[trackIndex] = true;
            sample->memory_->SetSize(0);
        }
        if (sample->flag_ & static_cast<uint32_t>(AVBufferFlag::PARTIAL_FRAME)) {
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

void MediaDemuxer::HandleEvent(const Plugins::PluginEvent &event)
{
    switch (event.type) {
        case PluginEventType::CLIENT_ERROR:
        case PluginEventType::SERVER_ERROR: {
            MEDIA_LOG_E("HandleEvent CLIENT/SERVER_ERROR");
            FALSE_RETURN(demuxerPluginManager_ != nullptr);
            demuxerPluginManager_->NotifyInitialBufferingEnd(false);
            break;
        }
        case PluginEventType::INITIAL_BUFFER_SUCCESS: {
            MEDIA_LOG_I("HandleEvent initial buffer success");
            FALSE_RETURN(demuxerPluginManager_ != nullptr);
            demuxerPluginManager_->NotifyInitialBufferingEnd(true);
            break;
        }
        default:
            break;
    }
}

void MediaDemuxer::OnEvent(const Plugins::PluginEvent &event)
{
    MEDIA_LOG_D("In");
    HandleEvent(event);
    std::weak_ptr<Pipeline::EventReceiver> weakEventReceiver = eventReceiver_;
    auto eventReceiver = weakEventReceiver.lock();
    if (eventReceiver == nullptr && event.type != PluginEventType::SOURCE_DRM_INFO_UPDATE) {
        MEDIA_LOG_D("EventReceiver is nullptr");
        return;
    }
    switch (event.type) {
        case PluginEventType::SOURCE_DRM_INFO_UPDATE: {
            MEDIA_LOG_D("OnEvent source drmInfo update");
            if (Any::IsSameTypeWith<std::multimap<std::string, std::vector<uint8_t>>>(event.param)) {
                HandleSourceDrmInfoEvent(AnyCast<std::multimap<std::string, std::vector<uint8_t>>>(event.param));
            }
            break;
        }
        case PluginEventType::CLIENT_ERROR: {
            Event evt {"demuxer_filter", EventType::EVENT_ERROR, event.param, "client"};
            eventReceiver->OnEvent(evt);
            break;
        }
        case PluginEventType::SERVER_ERROR: {
            Event evt {"demuxer_filter", EventType::EVENT_ERROR, event.param, "server"};
            eventReceiver->OnEvent(evt);
            break;
        }
        case PluginEventType::CACHED_DURATION: {
            MEDIA_LOG_D("OnEvent cached duration, but ignore");
            break;
        }
        case PluginEventType::SOURCE_BITRATE_START: {
            MEDIA_LOG_D("OnEvent source bitrate start");
            eventReceiver->OnEvent({"demuxer_filter", EventType::EVENT_SOURCE_BITRATE_START, event.param});
            break;
        }
        case PluginEventType::FLV_AUTO_SELECT_BITRATE: {
            MEDIA_LOG_D("OnEvent flv auto select bitrate.");
            eventReceiver->OnEvent({"demuxer_filter", EventType::EVENT_FLV_AUTO_SELECT_BITRATE, event.param});
            break;
        }
        default:
            break;
    }
    OnEventBuffer(event, eventReceiver);
}

void MediaDemuxer::OnEventBuffer(const Plugins::PluginEvent &event,
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver)
{
    switch (event.type) {
        case PluginEventType::BUFFERING_END: {
            MEDIA_LOG_D("OnEvent pause, but ignore");
            break;
        }
        case PluginEventType::BUFFERING_START: {
            MEDIA_LOG_D("OnEvent start, but ignore");
            break;
        }
        case PluginEventType::EVENT_BUFFER_PROGRESS: {
            MEDIA_LOG_D("OnEvent percent update, but ignore");
            break;
        }
        default:
            break;
    }
    OnSeekReadyEvent(event);
}

void MediaDemuxer::OnSeekReadyEvent(const Plugins::PluginEvent &event)
{
    switch (event.type) {
        case PluginEventType::DASH_SEEK_READY: {
            MEDIA_LOG_D("OnEvent dash seek ready");
            OnDashSeekReadyEvent(event);
            break;
        }
        case PluginEventType::HLS_SEEK_READY: {
            MEDIA_LOG_D("OnEvent hls seek ready");
            OnHlsSeekReadyEvent(event);
            break;
        }
        default:
            break;
    }
}

void MediaDemuxer::OnDashSeekReadyEvent(const Plugins::PluginEvent &event)
{
    MEDIA_LOG_D("Onevent dash seek ready");
    std::unique_lock<std::mutex> lock(rebootPluginMutex_);
    FALSE_RETURN(Any::IsSameTypeWith<Format>(event.param));
    Format param = AnyCast<Format>(event.param);
    int32_t currentStreamType = -1;
    param.GetIntValue("currentStreamType", currentStreamType);
    int32_t isEOS = -1;
    param.GetIntValue("isEOS", isEOS);
    int32_t currentStreamId = -1;
    param.GetIntValue("currentStreamId", currentStreamId);
    int64_t seekTimeMs = -1;
    param.GetLongValue("seekTime", seekTimeMs);

    bool isValidVideoSeek = seekTimeMs >= 0 && HasVideo();
    if (isValidVideoSeek) {
        Plugins::Ms2Us(seekTimeMs, videoSeekTime_);
        bool isValidVideoSeekTime = videoStartTime_ <= 0 || INT64_MAX - videoStartTime_ >= videoSeekTime_;
        if (isValidVideoSeekTime) {
            videoSeekTime_ += videoStartTime_;
            isInSeekDropAudio_ = true;
        }
    }

    MEDIA_LOG_D("HandleDashSeekReady, streamType: " PUBLIC_LOG_D32 " streamId: " PUBLIC_LOG_D32 " isEos: "
        PUBLIC_LOG_D32 " seekTimeMs: " PUBLIC_LOG_D64, currentStreamType, currentStreamId, isEOS, seekTimeMs);
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

void MediaDemuxer::OnHlsSeekReadyEvent(const Plugins::PluginEvent &event)
{
    MEDIA_LOG_D("Onevent hls seek ready");
    std::unique_lock<std::mutex> lock(rebootPluginMutex_);
    FALSE_RETURN(Any::IsSameTypeWith<Format>(event.param));
    Format param = AnyCast<Format>(event.param);
    int32_t currentStreamType = -1;
    param.GetIntValue("currentStreamType", currentStreamType);
    int32_t isEOS = -1;
    param.GetIntValue("isEOS", isEOS);
    int32_t currentStreamId = -1;
    param.GetIntValue("currentStreamId", currentStreamId);
    MEDIA_LOG_D("HandleHlsSeekReady, streamType: " PUBLIC_LOG_D32 " streamId: " PUBLIC_LOG_D32
        " isEos: " PUBLIC_LOG_D32, currentStreamType, currentStreamId, isEOS);
    switch (currentStreamType) {
        case static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_VID):
            seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = std::make_pair(currentStreamId, isEOS);
            break;
        case static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_AUD):
            seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)] = std::make_pair(currentStreamId, isEOS);
            break;
        default:
            break;
    }
    rebootPluginCondition_.notify_all();
}

void MediaDemuxer::OnDfxEvent(const Plugins::PluginDfxEvent &event)
{
    FALSE_RETURN_MSG(eventReceiver_ != nullptr, "Dfx event report error, receiver is nullptr");
    auto it = DFX_EVENT_MAP.find(event.type);
    FALSE_RETURN_MSG(it != DFX_EVENT_MAP.end(), "No mapped dfx event type, src type %{public}d", event.type);
    eventReceiver_->OnDfxEvent({ it->second.first, it->second.second, event.param });
}

Status MediaDemuxer::OptimizeDecodeSlow(bool isDecodeOptimizationEnabled)
{
    MEDIA_LOG_I("In");
    isDecodeOptimizationEnabled_ = isDecodeOptimizationEnabled;
    return Status::OK;
}

Status MediaDemuxer::SetDecoderFramerateUpperLimit(int32_t decoderFramerateUpperLimit,
    int32_t trackId)
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
    if (sampleQueueController_) {
        sampleQueueController_->SetSpeed(speed);
    }
    return Status::OK;
}

Status MediaDemuxer::SetFrameRate(double framerate, int32_t trackId)
{
    MEDIA_LOG_I("Framerate=" PUBLIC_LOG_F " trackId=" PUBLIC_LOG_D32, framerate, trackId);
    FALSE_RETURN_V(trackId == videoTrackId_, Status::OK);
    FALSE_RETURN_V_MSG_E(framerate > 0, Status::ERROR_INVALID_PARAMETER, "Framerate <= 0");
    framerate_.store(framerate);
    return Status::OK;
}

bool MediaDemuxer::CheckDropAudioFrame(std::shared_ptr<AVBuffer> sample, int32_t trackId)
{
    if (trackId == audioTrackId_) {
        if (isInSeekDropAudio_) {
            if (sample->pts_ < videoSeekTime_) {
                MEDIA_LOG_I("isInSeekDropAudio_ Drop audio buffer pts " PUBLIC_LOG_D64, sample->pts_);
                return true;
            } else {
                isInSeekDropAudio_ = false;
            }
        }
        if (shouldCheckAudioFramePts_ == false) {
            lastAudioPts_ = sample->pts_;
            MEDIA_LOG_I("Set last audio pts " PUBLIC_LOG_D64, lastAudioPts_);
            return false;
        }
        if (sample->pts_ <= lastAudioPts_) {
            MEDIA_LOG_I("Drop audio buffer pts " PUBLIC_LOG_D64, sample->pts_);
            return true;
        }
        if (shouldCheckAudioFramePts_) {
            shouldCheckAudioFramePts_ = false;
            lastAudioPts_ = sample->pts_;
        }
    }
    if (trackId == subtitleTrackId_) {
        if (shouldCheckSubtitleFramePts_ == false) {
            lastSubtitlePts_ = sample->pts_;
            MEDIA_LOG_I("Set last subtitle pts " PUBLIC_LOG_D64, lastSubtitlePts_);
            return false;
        }
        if (sample->pts_ <= lastSubtitlePts_) {
            MEDIA_LOG_I("Drop subtitle buffer pts " PUBLIC_LOG_D64, sample->pts_);
            return true;
        }
        if (shouldCheckSubtitleFramePts_) {
            shouldCheckSubtitleFramePts_ = false;
            lastSubtitlePts_ = sample->pts_;
        }
    }
    return false;
}

bool MediaDemuxer::IsBufferDroppable(std::shared_ptr<AVBuffer> sample, int32_t trackId)
{
    FALSE_GOON_NOEXEC(isDump_, DumpBufferToFile(trackId, sample));

    if (demuxerPluginManager_->IsDash() && (trackId == audioTrackId_ || trackId == subtitleTrackId_)) {
        return CheckDropAudioFrame(sample, trackId);
    }

    if (trackId != videoTrackId_) {
        return false;
    }

    FALSE_RETURN_V_NOLOG(!IsOpenGopBufferDroppable(sample, trackId), true);

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

    MEDIA_LOG_DD("Drop buffer, framerate=" PUBLIC_LOG_F " speed=" PUBLIC_LOG_F " decodeUpLimit="
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

bool MediaDemuxer::CheckTrackEnabledById(int32_t trackId)
{
    bool hasTrack = IsValidTrackId(trackId);
    if (!hasTrack) {
        return false;
    }
    bool hasTask = taskMap_.find(trackId) != taskMap_.end() && taskMap_[trackId] != nullptr;
    bool hasSampleQueueTask = sampleConsumerTaskMap_.find(trackId) != sampleConsumerTaskMap_.end() &&
                            sampleConsumerTaskMap_[trackId] != nullptr;
    if (!hasTask || (GetEnableSampleQueueFlag() && !hasSampleQueueTask)) {
        return false;
    }

    bool hasBufferQueue = bufferQueueMap_.find(trackId) != bufferQueueMap_.end()
        && bufferQueueMap_[trackId] != nullptr;
    bool hasSampleQueue = sampleQueueMap_.find(trackId) != sampleQueueMap_.end()
        && sampleQueueMap_[trackId] != nullptr;
    if (!hasBufferQueue || (GetEnableSampleQueueFlag() && !hasSampleQueue)) {
        MEDIA_LOG_I(" not hasBufferQueue or hasSampleQueueTask: TrackId=" PUBLIC_LOG_D32, trackId);
        return false;
    }
    return true;
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
    FALSE_RETURN_V_MSG_E(isPrepared_.load(), false, "is not prepared, cannot auto select bitrate");
    // calculating auto selectbitrate time
    return !(isSelectBitRate_.load()) && !(isSelectTrack_.load())
        && (targetBitRate_ == demuxerPluginManager_->GetCurrentBitRate());
}

bool MediaDemuxer::IsRenderNextVideoFrameSupported()
{
    bool isDataSrcLiveStream = source_ != nullptr && source_->IsNeedPreDownload() &&
        source_->GetSeekable() == Plugins::Seekable::UNSEEKABLE;
    return IsValidTrackId(videoTrackId_) && !IsTrackDisabled(Plugins::MediaType::VIDEO) &&
        !isDataSrcLiveStream && !isFlvLiveStream_;
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
    PauseAllTaskAsync();
    if (!GetEnableSampleQueueFlag()) {
        PauseAllTask();
    }
    if (demuxerPluginManager_) {
        demuxerPluginManager_->Pause();
    }
    return Status::OK;
}

Status MediaDemuxer::SetTranscoderMode()
{
    isTranscoderMode_ = true;
    return Status::OK;
}

Status MediaDemuxer::SetPlayerMode()
{
    isPlayerMode_ = true;
    return Status::OK;
}

Status MediaDemuxer::SetSkippingAudioDecAndEnc()
{
    isSkippingAudioDecAndEnc_ = true;
    return Status::OK;
}

void MediaDemuxer::SetCacheLimit(uint32_t limitSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_MSG(demuxerPluginManager_ != nullptr, "Plugin manager is nullptr");
    int32_t tempTrackId = (IsValidTrackId(videoTrackId_) ? videoTrackId_ : audioTrackId_);
    int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(tempTrackId);
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
    FALSE_RETURN_MSG(pluginTemp != nullptr, "Demuxer plugin is nullptr");

    pluginTemp->SetCacheLimit(limitSize);
}

bool MediaDemuxer::IsVideoEos()
{
    if (!IsValidTrackId(videoTrackId_)) {
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
    return videoTrackId_;
}

void MediaDemuxer::SetIsEnableReselectVideoTrack(bool isEnable)
{
    isEnableReselectVideoTrack_  = isEnable;
}

bool MediaDemuxer::IsOpenGopBufferDroppable(std::shared_ptr<AVBuffer> sample, int32_t trackId)
{
    FALSE_RETURN_V_NOLOG(trackId == videoTrackId_ && sample != nullptr, false);
    std::lock_guard<std::mutex> lock(syncFrameInfoMutex_);
    if ((sample->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) > 0) {
        syncFrameInfo_.pts = sample->pts_;
        if (syncFrameInfo_.skipOpenGopUnrefFrameCnt > 0) {
            syncFrameInfo_.skipOpenGopUnrefFrameCnt--;
        }
        return false;
    }
    if (syncFrameInfo_.skipOpenGopUnrefFrameCnt <= 0 || sample->pts_ >= syncFrameInfo_.pts) {
        return false;
    }
    MEDIA_LOG_DD("drop opengop-buffer after dragging, pts: " PUBLIC_LOG_D64 ", i frame pts: "
        PUBLIC_LOG_D64, sample->pts_, syncFrameInfo_.pts);
    return true;
}

void MediaDemuxer::UpdateSyncFrameInfo(std::shared_ptr<AVBuffer> sample, int32_t trackId, bool isDiscardable)
{
    FALSE_RETURN_NOLOG(trackId == videoTrackId_ && sample != nullptr && !isDiscardable);
    std::lock_guard<std::mutex> lock(syncFrameInfoMutex_);
    if ((sample->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) > 0) {
        syncFrameInfo_.pts = sample->pts_;
    }
}

void MediaDemuxer::EnterDraggingOpenGopCnt()
{
    std::lock_guard<std::mutex> lock(syncFrameInfoMutex_);
    syncFrameInfo_.skipOpenGopUnrefFrameCnt = SKIP_NEXT_OPEN_GOP_CNT;
}

void MediaDemuxer::ResetDraggingOpenGopCnt()
{
    std::lock_guard<std::mutex> lock(syncFrameInfoMutex_);
    syncFrameInfo_.skipOpenGopUnrefFrameCnt = 0;
}

void MediaDemuxer::SetApiVersion(int32_t apiVersion)
{
    apiVersion_ = apiVersion;
    demuxerPluginManager_->SetApiVersion(apiVersion);
}

bool MediaDemuxer::IsLocalFd()
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, false, "source_ is nullptr");
    return source_->IsLocalFd();
}

Status MediaDemuxer::RebootPlugin()
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::RebootPlugin");
    FALSE_RETURN_V(source_ != nullptr && demuxerPluginManager_ != nullptr && streamDemuxer_ != nullptr,
        Status::ERROR_NULL_POINTER);
    RestartAndClearBuffer();
    Status ret = Status::OK;
    int32_t videoStreamID = streamDemuxer_->GetNewVideoStreamID();
    demuxerPluginManager_->StopPlugin(videoStreamID, streamDemuxer_);
    ret = demuxerPluginManager_->StartPlugin(videoStreamID, streamDemuxer_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Start plugin failed" PUBLIC_LOG_D32, videoStreamID);
    ret = InnerSelectTrack(static_cast<int32_t>(videoTrackId_));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "inner select video track failed");
    ret = InnerSelectTrack(static_cast<int32_t>(audioTrackId_));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "inner select audio track failed");
    return Status::OK;
}

Status MediaDemuxer::AddSampleBufferQueue(int32_t trackId)
{
    std::shared_ptr<SampleQueue> sampleQueue = std::make_shared<SampleQueue>();
    FALSE_RETURN_V_MSG_E(sampleQueue != nullptr, Status::ERROR_NO_MEMORY, "SampleQueue create failed");
    bool isVideo = IsRightMediaTrack(trackId, DemuxerTrackType::VIDEO);
    SampleQueue::Config sampleQueueConfig{};
    sampleQueueConfig.isFlvLiveStream_ = isFlvLiveStream_;
    sampleQueueConfig.isSupportBitrateSwitch_ = sampleQueueConfig.isFlvLiveStream_ && isVideo;
    sampleQueueConfig.queueId_ = trackId;
    sampleQueueConfig.bufferCap_ =
        isVideo ? SampleQueue::DEFAULT_VIDEO_SAMPLE_BUFFER_CAP : SampleQueue::DEFAULT_SAMPLE_BUFFER_CAP;
    sampleQueueConfig.queueSize_ = IsLocalFd() ? SampleQueue::MAX_SAMPLE_QUEUE_SIZE :
        SampleQueue::DEFAULT_SAMPLE_QUEUE_SIZE;
    produceSteadyClock_.Reset();
    Status status = sampleQueue->Init(sampleQueueConfig);
    FALSE_RETURN_V_MSG_E(status == Status::OK, status, "SampleQueue Init failed");
    sampleQueue->SetSampleQueueCallback(shared_from_this());

    sampleQueueMap_.insert(std::pair<int32_t, std::shared_ptr<SampleQueue>>(trackId, sampleQueue));
    MEDIA_LOG_I("AddSampleBufferQueue successfully trackId " PUBLIC_LOG_D32, trackId);
    return Status::OK;
}

int64_t MediaDemuxer::SampleConsumerLoop(int32_t trackId)
{
    MEDIA_LOG_DD("In, SampleConsumerLoop trackId: " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(bufferQueueMap_.count(trackId) > 0 && bufferQueueMap_[trackId] != nullptr, RETRY_DELAY_TIME_US,
        "BufferQueue " PUBLIC_LOG_D32 " is nullptr", trackId);
  
    FALSE_RETURN_V_MSG_E(sampleQueueMap_.count(trackId) > 0 && sampleQueueMap_[trackId] != nullptr,
        RETRY_DELAY_TIME_US, "SampleQueueMap " PUBLIC_LOG_D32 " is nullptr", trackId);

    auto& sampleQueue = sampleQueueMap_[trackId];
    auto& bufferQueue = bufferQueueMap_[trackId];
    Status status = Status::OK;

    do {
        ConsumeWaterLoopControl(trackId, sampleQueue);
        size_t size = 0;
        status = sampleQueue->QuerySizeForNextAcquireBuffer(size);
        CHECK_AND_BREAK_LOG_LIMIT_POW2(status == Status::OK, SAMPLE_LOOP_ACQUIRE_FAILED_LOG_POW2,
            "QuerySizeForNextAcquireBuffer failed, trackId: " PUBLIC_LOG_D32, trackId);
        UpdateSampleQueueCache();

        SetTrackNotifySampleConsumerFlag(trackId, true);
        std::shared_ptr<AVBuffer> dstBuffer;
        status = RequestDstBuffer(trackId, static_cast<int32_t>(size), dstBuffer);
        CHECK_AND_BREAK_LOG_LIMIT_POW2(status == Status::OK, SAMPLE_LOOP_REQUEST_FAILED_LOG_POW2,
            "RequestBuffer from bufferQueue failed " PUBLIC_LOG_D32, trackId);
        SetTrackNotifySampleConsumerFlag(trackId, false);

        if (static_cast<int32_t>(size) <= dstBuffer->memory_->GetCapacity()) {
            status = sampleQueue->AcquireCopyToDstBuffer(dstBuffer);
            status = HandlePushBuffer(trackId, dstBuffer, bufferQueue, status);
            CHECK_AND_BREAK_LOG(status == Status::OK,
                "HandlePushBuffer failed, trackId: %{public}d, status: %{public}d", trackId, status);
            if (status == Status::OK) {
                sampleQueueController_->ConsumeSpeed(trackId);
            }
        } else {
            std::shared_ptr<AVBuffer> srcBuffer;
            status = sampleQueue->AcquireBuffer(srcBuffer);
            CHECK_AND_BREAK_LOG_LIMIT_POW2(
                status == Status::OK && srcBuffer && srcBuffer->memory_, SAMPLE_LOOP_ACQUIRE_FAILED_LOG_POW2,
                "AcquireSrcBuffer failed, trackId: %{public}d, status: %{public}d", trackId, status);

            status = CopyAndPushBufferBySlices(trackId, srcBuffer, dstBuffer);
            CHECK_AND_BREAK_LOG(status == Status::OK,
                "CopySrcBufferByMinSize failed, trackId: %{public}d, status: %{public}d", trackId, status);
        }
    } while (0);
    uint32_t retryTime = hasSetLargeSize_ && !isVideoMuted_ && trackId == videoTrackId_ ?
        NEXT_DELAY_TIME_US : SAMPLE_LOOP_RETRY_TIME_US;
    return status == Status::OK ?
        static_cast<int64_t>(retryTime / speed_) : static_cast<int64_t>(SAMPLE_LOOP_DELAY_TIME_US / speed_);
}

Status MediaDemuxer::RequestDstBuffer(int32_t trackId, int32_t size, std::shared_ptr<AVBuffer> &dstBuffer)
{
    auto requestSize = trackId == videoTrackId_ ? SampleQueue::DEFAULT_SAMPLE_BUFFER_CAP : size;
    auto &bufferQueue = bufferQueueMap_[trackId];
    AVBufferConfig config;
    config.capacity = requestSize;
    config.size = requestSize;
    return bufferQueue->RequestBuffer(dstBuffer, config, REQUEST_BUFFER_TIMEOUT);
}

Status MediaDemuxer::CopyAndPushBufferBySlices(int32_t trackId, std::shared_ptr<AVBuffer> &srcBuffer,
    std::shared_ptr<AVBuffer> &dstBuffer, int32_t sliceSize)
{
    auto srcBufferSize = sliceSize == 0 ? srcBuffer->memory_->GetSize() : sliceSize;
    int32_t copySize = std::min(dstBuffer->memory_->GetCapacity(), srcBufferSize);
    MEDIA_LOG_D("prepare to copy: %{public}d, dest cap: %{public}d, src buf id: %{public}llu",
        copySize, dstBuffer->memory_->GetCapacity(), srcBuffer->GetUniqueId());
    auto status = sampleQueueMap_[trackId]->CopyBufferSlice(srcBuffer, dstBuffer, copySize);
    FALSE_RETURN_V_MSG_E(status == Status::OK, status, "CopyPartBuffer failed, errCode: %{public}d", status);
    status = HandlePushBuffer(trackId, dstBuffer, bufferQueueMap_[trackId], Status::OK);
    FALSE_RETURN_V_MSG_E(status == Status::OK, status, "HandlePushBuffer failed, errCode: %{public}d", status);
    status = CheckAndReleaseRemainBuffer(srcBuffer, trackId);
    return status;
}

Status MediaDemuxer::ReleaseSrcBuffer(std::shared_ptr<AVBuffer> &srcBuffer, int32_t trackId)
{
    MEDIA_LOG_D("release src buff id: %{public}lld", srcBuffer->GetUniqueId());
    auto status = sampleQueueMap_[trackId]->ReleaseBuffer(srcBuffer);
    return status;
}

Status MediaDemuxer::CheckAndReleaseRemainBuffer(std::shared_ptr<AVBuffer> &srcBuffer, int32_t trackId)
{
    Status status = Status::OK;
    
    // requestBuffer may fail. So we use AVMemory offset to compute remainSize.
    auto remainSize = srcBuffer->memory_->GetSize() - srcBuffer->memory_->GetOffset();
    if (remainSize <= 0) {
        return ReleaseSrcBuffer(srcBuffer, trackId);
    }
    MEDIA_LOG_D("CheckRemainSrcBufferAndCopy, SrcSize: %{public}d, remainSize: %{public}d",
        srcBuffer->memory_->GetSize(), remainSize);
    std::shared_ptr<AVBuffer> dstBuffer;
    status = RequestDstBuffer(trackId, 0, dstBuffer);
    if (status != Status::OK) {
        sampleQueueMap_[trackId]->RollbackBuffer(srcBuffer);
        MEDIA_LOG_E("RequestDstBuffer failed: %{public}d", status);
        return status;
    }
    status = CopyAndPushBufferBySlices(trackId, srcBuffer, dstBuffer, remainSize);
    return status;
}

void MediaDemuxer::ConsumeWaterLoopControl(int32_t trackId, std::shared_ptr<SampleQueue> sampleQueue)
{
    if (!sampleQueueController_ || trackId == subtitleTrackId_ || IsLocalFd() || eosMap_[trackId]) {
        return;
    }
    bool stopConsumeResult = false;
    {
        AutoLock lock(mapMutex_);
        sampleQueueController_->ShouldStartProduce(trackId, sampleQueue, taskMap_[trackId]);
        stopConsumeResult =
            sampleQueueController_->ShouldStopConsume(trackId, sampleQueue, sampleConsumerTaskMap_[trackId]);
    }
    if (stopConsumeResult && !hlsSegmentEosMap_[trackId]) {
        if (trackId == videoTrackId_ && isVideoMuted_) {
            isBufferingMap_[trackId].store(false);
            return;
        }
        isBufferingMap_[trackId].store(true);
        CheckAndReportBufferingStatus(EventType::BUFFERING_START);
    }
}

Status MediaDemuxer::HandlePushBuffer(int32_t trackId, std::shared_ptr<AVBuffer>& dstBuffer,
                                      sptr<AVBufferQueueProducer>& bufferQueue, Status status)
{
    if (trackId == videoTrackId_ && needReleaseVideoDecoder_) {
        Status ret = bufferQueue->PushBuffer(dstBuffer, status == Status::OK);
        int64_t duration = 0;
        mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(duration);
        int64_t mediaTime = (duration > 0 && syncCenter_ != nullptr) ?
            syncCenter_->GetMediaTimeNow() : lastAudioPtsInMute_;
        if (dstBuffer->pts_ > mediaTime) {
            return Status::ERROR_UNKNOWN;
        }
        return ret;
    }
    if (!(needRestore_ && trackId == videoTrackId_ && !isVideoMuted_)) {
        return bufferQueue->PushBuffer(dstBuffer, status == Status::OK);
    }

    if (!(dstBuffer->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME))) {
        MEDIA_LOG_I("MediaDemuxer::SampleConsumerLoop throw away buffer trackId: " PUBLIC_LOG_U32 " pts: "
            PUBLIC_LOG_U64 " flag is: " PUBLIC_LOG_U32, trackId, (uint64_t)dstBuffer->pts_,
            dstBuffer->flag_);
        return bufferQueue->PushBuffer(dstBuffer, false);
    }

    std::vector<uint8_t> config;
    mediaMetaData_.trackMetas[videoTrackId_]->GetData(Tag::MEDIA_CODEC_CONFIG, config);
    if (config.size() > 0) {
        int32_t size = dstBuffer->memory_->GetSize();
        std::vector<uint8_t> memory;
        memory.resize(static_cast<size_t>(size) + config.size());
        dstBuffer->memory_->Read(memory.data(), size, 0);
        bool hasXps = false;
        if (size >= static_cast<int32_t>(config.size())) {
            hasXps = memcmp(config.data(), memory.data(), config.size()) == 0;
        } else {
            hasXps = false;
        }
        if (!hasXps) {
            memory.insert(memory.begin(), config.begin(), config.end());
            dstBuffer->memory_->Write(memory.data(), memory.size(), 0);
            MEDIA_LOG_I("MediaDemuxer::HandlePushBuffer write xps to buffer");
        }
    }
    needRestore_ = false;
    return bufferQueue->PushBuffer(dstBuffer, status == Status::OK);
}

void MediaDemuxer::SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter)
{
    syncCenter_ = syncCenter;
}

bool MediaDemuxer::IsRightMediaTrack(int32_t trackId, DemuxerTrackType type) const
{
    FALSE_RETURN_V(IsValidTrackId(trackId), false);
    switch (type) {
        case DemuxerTrackType::VIDEO:
            return trackId == videoTrackId_;
        case DemuxerTrackType::AUDIO:
            return trackId == audioTrackId_;
        case DemuxerTrackType::SUBTITLE:
            return trackId == subtitleTrackId_;
        default:
            return false;
    }
}

int64_t MediaDemuxer::GetLastVideoBufferAbsPts(int32_t trackId) const
{
    if (syncCenter_ == nullptr) {
        return HST_TIME_NONE;
    }
    FALSE_RETURN_V_MSG_E(syncCenter_ != nullptr, HST_TIME_NONE, "syncCenter_ is nullptr");
    return syncCenter_->GetLastVideoBufferAbsPts();
}

void MediaDemuxer::UpdateLastVideoBufferAbsPts(int32_t trackId)
{
    if (!isFlvLiveStream_ || !IsRightMediaTrack(trackId, DemuxerTrackType::VIDEO)) {
        return;
    }
    int64_t lastVideoBufferAbsPts = GetLastVideoBufferAbsPts(trackId);
    if (lastVideoBufferAbsPts == HST_TIME_NONE) {
        return;
    }
    AutoLock lock(mapMutex_);
    auto sqIt = sampleQueueMap_.find(trackId);
    if (sqIt != sampleQueueMap_.end() && sqIt->second != nullptr) {
        sqIt->second->UpdateLastEndSamplePts(lastVideoBufferAbsPts);
    }
}

Status MediaDemuxer::SelectBitrateForNonSQ(int64_t startPts, uint32_t bitRate)
{
    MEDIA_LOG_I("SelectBitrateForNonSQ startPts=" PUBLIC_LOG_D64 " bitRate=" PUBLIC_LOG_U32, startPts, bitRate);
    FALSE_RETURN_V_MSG_E(handleFlvSelectBitrateTask_ != nullptr, Status::ERROR_NULL_POINTER,
        "handleFlvSelectBitrateTask_ is nullptr");
    FALSE_RETURN_V_MSG_I(!isFlvLiveSelectingBitRate_.load(), Status::OK,
        "isFlvLiveSelectingBitRate, ignore this request");
    isFlvLiveSelectingBitRate_.store(true);
    PauseAllTask();
    handleFlvSelectBitrateTask_->SubmitJobOnce([this, startPts, bitRate] {
        HandleSelectBitrateForFlvLive(startPts, bitRate);
        isFlvLiveSelectingBitRate_.store(false);
    });
    ResumeAllTask();
    return Status::OK;
}

// now only for the flv living streaming case, callback by SampleQueue
Status MediaDemuxer::OnSelectBitrateOk(int64_t startPts, uint32_t bitRate)
{
    MEDIA_LOG_I("OnSelectBitrateOk startPts=" PUBLIC_LOG_D64 " bitRate=" PUBLIC_LOG_U32, startPts, bitRate);
    FALSE_RETURN_V_MSG_E(handleFlvSelectBitrateTask_ != nullptr, Status::ERROR_NULL_POINTER,
        "handleFlvSelectBitrateTask_ is nullptr");
    handleFlvSelectBitrateTask_->SubmitJobOnce([this, startPts, bitRate] {
        HandleSelectBitrateForFlvLive(startPts, bitRate);
    });
    return Status::OK;
}

Status MediaDemuxer::OnSampleQueueBufferAvailable(int32_t queueId)
{
    MEDIA_LOG_DD("OnSampleQueueBufferAvailable queueId=" PUBLIC_LOG_D32, queueId);
    FALSE_RETURN_V_MSG_E(notifySampleProduceTask_ != nullptr, Status::ERROR_NULL_POINTER,
        "notifySampleProduceTask_ is nullptr");
    notifySampleProduceTask_->SubmitJobOnce([demuxerWptr = weak_from_this(), queueId] {
        std::shared_ptr<MediaDemuxer> demuxer = demuxerWptr.lock();
        if (demuxer != nullptr) {
            demuxer->AccelerateTrackTask(queueId);
        }
    });
    return Status::OK;
}

Status MediaDemuxer::OnSampleQueueBufferConsume(int32_t queueId)
{
    FALSE_RETURN_V_MSG_E(notifySampleConsumeTask_ != nullptr, Status::ERROR_NULL_POINTER,
        "notifySampleConsumeTask_ is nullptr");
    notifySampleConsumeTask_->SubmitJobOnce([demuxerWptr = weak_from_this(), queueId] {
        std::shared_ptr<MediaDemuxer> demuxer = demuxerWptr.lock();
        if (demuxer != nullptr) {
            demuxer->NotifySampleQueueBufferConsume(queueId);
        }
    });
    return Status::OK;
}

Status MediaDemuxer::NotifySampleQueueBufferConsume(int32_t queueId)
{
    MEDIA_LOG_DD("NotifySampleQueueBufferConsume queueId=" PUBLIC_LOG_D32, queueId);
    int32_t trackId = queueId;
    {
        std::unique_lock<std::mutex> stopLock(stopMutex_);
        if (isStopped_ || isThreadExit_) {
            return Status::OK;
        }
    }
    AutoLock lock(mapMutex_);

    auto track = trackMap_.find(trackId);
    if (track == trackMap_.end() || track->second == nullptr) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    track->second->SetNotifySampleConsumerFlag(false);

    // accelerate SampleQueue toConsumer
    auto sampleConsumerTask = sampleConsumerTaskMap_.find(trackId);
    if (sampleConsumerTask == sampleConsumerTaskMap_.end()) {
        return Status::OK;
    }
    sampleConsumerTask->second->UpdateDelayTime();

    return Status::OK;
}

Status MediaDemuxer::HandleSelectBitrateForFlvLive(int64_t startPts, uint32_t bitrate)
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::HandleSelectBitrateForFlvLive");
    MEDIA_LOG_I("In bitrate=" PUBLIC_LOG_U32 " startPts=" PUBLIC_LOG_D64, bitrate, startPts);
    FALSE_RETURN_V(source_ != nullptr && demuxerPluginManager_ != nullptr && streamDemuxer_ != nullptr,
        Status::ERROR_NULL_POINTER);

    Status ret = Status::OK;
    source_->SetStartPts(startPts / US_TO_MS);
    if (isManualBitRateSetting_.load()) {
        source_->SelectBitRate(bitrate);
    } else {
        source_->AutoSelectBitRate(bitrate);
    }
    MEDIA_LOG_I("source SelectBitrate bitrate=" PUBLIC_LOG_U32 " startPts=" PUBLIC_LOG_D64, bitrate, startPts);

    if (GetEnableSampleQueueFlag()) {
        AutoLock lock(mapMutex_);
        for (auto &sqIt : sampleQueueMap_) {
            FALSE_RETURN_V_MSG_E(
                sqIt.second != nullptr, Status::ERROR_INVALID_STATE, "invalid trackId" PUBLIC_LOG_D32, sqIt.first);
            ret = sqIt.second->ResponseForSwitchDone(startPts);
            FALSE_RETURN_V_MSG_E(
                ret == Status::OK, ret, "ResponseForSwitchDone failed trackId" PUBLIC_LOG_D64, startPts);
        }
    }

    int32_t videoStreamID = streamDemuxer_->GetNewVideoStreamID();
    demuxerPluginManager_->StopPlugin(videoStreamID, streamDemuxer_);
    ret = demuxerPluginManager_->StartPlugin(videoStreamID, streamDemuxer_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Start plugin failed" PUBLIC_LOG_D32, videoStreamID);

    // update track map in track id change case
    if (demuxerPluginManager_->GetTrackTypeByTrackID(audioTrackId_) == TRACK_VIDEO) {
        demuxerPluginManager_->UpdateTempTrackMapInfo(videoTrackId_, videoTrackId_, audioTrackId_);
    }
    if (demuxerPluginManager_->GetTrackTypeByTrackID(videoTrackId_) == TRACK_AUDIO) {
        demuxerPluginManager_->UpdateTempTrackMapInfo(audioTrackId_, audioTrackId_, videoTrackId_);
    }

    InnerSelectTrack(static_cast<int32_t>(videoTrackId_));
    InnerSelectTrack(static_cast<int32_t>(audioTrackId_));
    return ret;
}

uint64_t MediaDemuxer::GetCachedDuration()
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, 0, "source_ is nullptr");
    demuxerCacheDuration_ = GetEnableSampleQueueFlag() ? GetSampleQueueDuration() : 0;
    sourceCacheDuration_ = source_->GetCachedDuration();
    MEDIA_LOG_I("samplequeue cacheDuration=" PUBLIC_LOG_U64 ", sourceCache=" PUBLIC_LOG_U64, demuxerCacheDuration_,
        sourceCacheDuration_);
    return sourceCacheDuration_ + demuxerCacheDuration_;
}

uint64_t MediaDemuxer::GetSampleQueueDuration()
{
    uint64_t sampleQueueDration = std::numeric_limits<uint64_t>::max();
    {
        AutoLock lock(mapMutex_);
        FALSE_RETURN_V_MSG_E(sampleQueueMap_.size() > 0, 0, "sampleQueueMap_ empty");
        for (auto sqIt = sampleQueueMap_.begin(); sqIt != sampleQueueMap_.end(); sqIt++) {
            FALSE_RETURN_V_MSG_E(sqIt->second != nullptr, 0, "sampleQueue empty");
            sampleQueueDration = std::min(sqIt->second->GetCacheDuration() / US_TO_MS, sampleQueueDration);
        }
    }
    return sampleQueueDration;
}

void MediaDemuxer::UpdateSampleQueueCache()
{
    FALSE_RETURN_NOLOG(isFlvLiveStream_);
    int64_t currentClockTimeMs = SteadyClock::GetCurrentTimeMs();
    if (lastClockTimeMs_ != 0 && currentClockTimeMs - lastClockTimeMs_ < UPDATE_SOURCE_CACHE_MS) {
        return;
    }
    lastClockTimeMs_ = currentClockTimeMs;
    demuxerCacheDuration_ = GetSampleQueueDuration();
    if (source_) {
        source_->SetExtraCache(demuxerCacheDuration_);
        MEDIA_LOG_I("samplequeue cacheDuration=" PUBLIC_LOG_U64 ", sourceCache=" PUBLIC_LOG_U64, demuxerCacheDuration_,
            source_->GetCachedDuration());
    }
}

void MediaDemuxer::RestartAndClearBuffer()
{
    FALSE_RETURN_MSG(source_ != nullptr, "source_ is nullptr");
    return source_->RestartAndClearBuffer();
}

bool MediaDemuxer::IsFlvLive()
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, false, "source_ is nullptr");
    return source_->IsFlvLive();
}

bool MediaDemuxer::IsIgonreBuffering()
{
    MEDIA_LOG_I("IsIgonreBuffering in");
    if (!IsRightMediaTrack(videoTrackId_, DemuxerTrackType::VIDEO)) {
        return false;
    }
    AutoLock lock(mapMutex_);
    auto sqIt = sampleQueueMap_.find(videoTrackId_);
    FALSE_RETURN_V_MSG_E(sqIt != sampleQueueMap_.end() && sqIt->second, false,
        "sampleQueue is nullptr");
    uint64_t cacheDuration = sqIt->second->GetCacheDuration();
    MEDIA_LOG_I("samplequeue cacheDuration=" PUBLIC_LOG_U64, cacheDuration);
    return cacheDuration > BUFFERING_WAVELINE_FOR_SAMPLE_QUEUE;
}

void MediaDemuxer::InitEnableSampleQueueFlag()
{
    const std::string sampleQueueTag = "debug.media_service.enable_samplequeue";
    std::string enableSampleQueue;
    int32_t enableSampleQueueRes = OHOS::system::GetStringParameter(sampleQueueTag, enableSampleQueue, "true");
    enableSampleQueue_ = (enableSampleQueue == "true");
    MEDIA_LOG_I("InitEnableSampleQueueFlag, enableSampleQueueRes: " PUBLIC_LOG_D32
        ", enableSampleQueue_: " PUBLIC_LOG_D32, enableSampleQueueRes, enableSampleQueue_);
}

void MediaDemuxer::InitIsAudioDemuxDecodeAsync()
{
    // To optimize the performance for audio only MediaSource, perform audio DEMUX and DECODE in the same thread.
    // 1. If audiodecoder_async is false, then DEMUX and DECODE run in the same thread.
    // 2. or audiodecoder_async is true, but both audiodemux_audiodecode_merged is true and isVideoTrackDisabled_ true,
    //    then DEMUX and DECODE run in the same thread.
    bool isAudioDeocderAsync =
        OHOS::system::GetParameter("debug.media_service.audio.audiodecoder_async", "1") == "1";
    bool isAudioDemuxDecodeMergedEnabled =
        OHOS::system::GetParameter("debug.media_service.audio.audiodemux_audiodecode_merged", "1") == "1";
    isAudioDemuxDecodeAsync_ = isAudioDeocderAsync && !(isAudioDemuxDecodeMergedEnabled && isVideoTrackDisabled_);

    MEDIA_LOG_I_SHORT("isAudioDeocderAsync: " PUBLIC_LOG_D32 ", isAudioDemuxDecodeMergedEnabled: " PUBLIC_LOG_D32
        ", isVideoTrackDisabled_: " PUBLIC_LOG_D32 ", isAudioDemuxDecodeAsync_: " PUBLIC_LOG_D32,
        isAudioDeocderAsync, isAudioDemuxDecodeMergedEnabled, isVideoTrackDisabled_, isAudioDemuxDecodeAsync_);
}

bool MediaDemuxer::IsNeedMapToInnerTrackID()
{
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, false, "demuxerPluginManager_ is nullptr");
    return (isFlvLiveStream_ || demuxerPluginManager_->IsDash() ||
        demuxerPluginManager_->GetTmpStreamIDByTrackID(subtitleTrackId_) != -1);
}

void MediaDemuxer::GetMemoryUsage(int32_t trackId, std::shared_ptr<Plugins::DemuxerPlugin> &pluginTemp)
{
    std::lock_guard<std::mutex> lock(memoryReportLimitMutex_);
    FALSE_RETURN_NOLOG(eventReceiver_ != nullptr);
    if (memoryReportLimitCount_.find(trackId) == memoryReportLimitCount_.end()) {
        memoryReportLimitCount_[trackId] = 1;
    } else {
        memoryReportLimitCount_[trackId]++;
        FALSE_RETURN_NOLOG(memoryReportLimitCount_[trackId] % LIMIT_MEMORY_REPORT_COUNT == 0);
        ReportMemoryUsage(trackId, pluginTemp);
    }
}

void MediaDemuxer::ReportMemoryUsage(int32_t trackId, std::shared_ptr<Plugins::DemuxerPlugin> &pluginTemp)
{
    uint32_t memoryUsage = 0;
    Status ret = pluginTemp->GetCurrentCacheSize(static_cast<uint32_t>(trackId), memoryUsage);
    FALSE_RETURN_NOLOG(ret == Status::OK);
    trackMemoryUsages_[trackId] = memoryUsage;
    eventReceiver_->OnMemoryUsageEvent({"DEMUXER_PLUGIN", DfxEventType::DFX_INFO_MEMORY_USAGE, trackMemoryUsages_});

    auto sampleIter = sampleQueueMap_.find(trackId);
    FALSE_RETURN_NOLOG(sampleIter != sampleQueueMap_.end() && sampleIter->second);
    memoryUsage = sampleIter->second->GetMemoryUsage();
    eventReceiver_->OnMemoryUsageEvent({"SAMPLE_QUEUE", DfxEventType::DFX_INFO_MEMORY_USAGE, memoryUsage});
}

bool MediaDemuxer::IsSeekToTimeSupported()
{
    return source_ != nullptr && source_->IsSeekToTimeSupported();
}

Status MediaDemuxer::GetCurrentCacheSize(uint32_t trackIndex, uint32_t& size)
{
    int32_t trackId = static_cast<int32_t>(trackIndex);
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    if (IsNeedMapToInnerTrackID()) {
        int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamId);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Plugin is nullptr");
        int32_t innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
        FALSE_RETURN_V_MSG_E(innerTrackID != INVALID_STREAM_OR_TRACK_ID,
            Status::ERROR_INVALID_PARAMETER, "Plugin is nullptr");
        trackIndex = static_cast<uint32_t>(innerTrackID);
    } else {
        int32_t streamId = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamId);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Plugin is nullptr");
    }
    return pluginTemp->GetCurrentCacheSize(trackIndex, size);
}

Status MediaDemuxer::StopBufferring(bool isAppBackground)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "source_ is nullptr");
    return source_->StopBufferring(isAppBackground);
}

void MediaDemuxer::SetMediaMuted(OHOS::Media::MediaType mediaType, bool isMuted)
{
    if (mediaType == OHOS::Media::MediaType::MEDIA_TYPE_VID) {
        needRestore_ = !needReleaseVideoDecoder_ && isVideoMuted_ && !isMuted;
        needReleaseVideoDecoder_ = isMuted ? !isVideoMuted_ || needReleaseVideoDecoder_ : false;
        isVideoMuted_ = isMuted;
        MEDIA_LOG_I("MediaDemuxer::SetMediaMuted " PUBLIC_LOG_U32, isMuted);
    }
}

void MediaDemuxer::InitEnableDfxBufferQueue()
{
    const std::string dfxBufferQueueTag = "debug.media_service.enable_dfx_buffer_queue";
    enableDfxBufferQueue_ = OHOS::system::GetBoolParameter(dfxBufferQueueTag, false);
    MEDIA_LOG_I("enableDfxBufferQueue_ " PUBLIC_LOG_D32, enableDfxBufferQueue_);
}

void MediaDemuxer::NotifyResumeUnMute()
{
    if (sampleConsumerTaskMap_.find(videoTrackId_) != sampleConsumerTaskMap_.end() &&
            sampleConsumerTaskMap_[videoTrackId_] != nullptr) {
        if (!isVideoMuted_ && !sampleConsumerTaskMap_[videoTrackId_]->IsTaskRunning()) {
            sampleConsumerTaskMap_[videoTrackId_]->Start();
        }
    }
}

void MediaDemuxer::HandleVideoSampleQueue()
{
    Status ret = sampleQueueMap_[videoTrackId_]->AddQueueSize(SAMPLE_QUEUE_ADD_SIZE_ON_MUTE);
    sampleQueueController_->AddQueueSize(videoTrackId_, SAMPLE_QUEUE_ADD_SIZE_ON_MUTE);
    FALSE_RETURN_NOLOG(ret != Status::OK);
    std::shared_ptr<AVBuffer> dstBuffer;
    ret = sampleQueueMap_[videoTrackId_]->AcquireBuffer(dstBuffer);
    FALSE_RETURN_NOLOG(ret == Status::OK);
    sampleQueueMap_[videoTrackId_]->ReleaseBuffer(dstBuffer);
}

bool MediaDemuxer::IsSegmentEos()
{
    if (IsValidTrackId(videoTrackId_)) {
        FALSE_RETURN_V_NOLOG(segmentEosMap_[videoTrackId_], false);
    }
    if (IsValidTrackId(audioTrackId_)) {
        FALSE_RETURN_V_NOLOG(segmentEosMap_[audioTrackId_], false);
    }
    return true;
}

void MediaDemuxer::ResetSegmentEosMap()
{
    for (auto& item : segmentEosMap_) {
        item.second = false;
    }
}

bool MediaDemuxer::IsAVInOneStream()
{
    // If IsAVInOneStream return true means mixed stream
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, true, "Plugin manager is nullptr");
    int32_t audioStreamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(audioTrackId_);
    FALSE_RETURN_V_NOLOG(audioStreamId != INVALID_STREAM_OR_TRACK_ID, true);
    int32_t videoStreamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(videoTrackId_);
    FALSE_RETURN_V_NOLOG(videoStreamId != INVALID_STREAM_OR_TRACK_ID, true);
    return audioStreamId == videoStreamId;
}
} // namespace Media
} // namespace OHOS