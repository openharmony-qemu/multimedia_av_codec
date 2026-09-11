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

#include <cinttypes>

#include <algorithm>
#include <map>
#include <memory>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include <utility>
#include <cmath>

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
#include "avcodec_info.h"
#include "interstitial_controller.h"
#include "interstitial_scheduler.h"

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
constexpr uint32_t MAX_VIDEO_LEAD_TIME_ON_MUTE_US = 10000000; // Maximum video frame advance time during video mute
constexpr int64_t MAX_SUBTITLE_LEAD_TIME = 1000000;
constexpr uint32_t SAMPLE_QUEUE_ADD_SIZE_ON_MUTE = 20; // When samplequeue is full on mute, samplequeue size add 20
constexpr int64_t INTERSTITIAL_CHECK_INTERVAL_MS = 200;
std::map<OHOS::Media::TrackType, OHOS::Media::Plugins::StreamType> TRACK_TO_STREAM_MAP = {
    {OHOS::Media::TrackType::TRACK_VIDEO, OHOS::Media::Plugins::StreamType::VIDEO},
    {OHOS::Media::TrackType::TRACK_AUDIO, OHOS::Media::Plugins::StreamType::AUDIO},
    {OHOS::Media::TrackType::TRACK_SUBTITLE, OHOS::Media::Plugins::StreamType::SUBTITLE},
    {OHOS::Media::TrackType::TRACK_INVALID, OHOS::Media::Plugins::StreamType::MIXED}
};

bool GetLocalLoadedRange(int64_t durationMs, OHOS::Media::Plugins::SeekRange &range)
{
    FALSE_RETURN_V(durationMs > 0, false);
    int64_t durationHst = 0;
    FALSE_RETURN_V(OHOS::Media::Plugins::Us2HstTime(durationMs, durationHst) && durationHst > 0, false);
    range = {0, durationHst};
    return true;
}

} // namespace

template<typename T>
void FillIfEmpty(T& target, const T& source)
{
    if (target == 0 && source > 0) {
        target = source;
    }
}

void FillIfEmpty(std::string& target, const std::string& source)
{
    if (target.empty() && !source.empty()) {
        target = source;
    }
}

namespace OHOS {
namespace Media {
constexpr uint32_t REQUEST_BUFFER_TIMEOUT = 0; // Requesting buffer overtimes 0ms means no retry
constexpr int32_t START = 1;
constexpr int32_t PAUSE = 2;
constexpr bool SEEK_TO_EOS = true;
constexpr uint32_t RETRY_DELAY_TIME_US = 100000; // 100ms, Delay time for RETRY if no buffer in avbufferqueue producer.
constexpr uint32_t NEXT_DELAY_TIME_US = 10; // 10us is ok
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
constexpr int32_t DURATION_CHANGE_AMOUNT_MILLIONSECOND = 500;
constexpr int64_t MAX_PERCENT = 100;
const std::unordered_map<PluginDfxEventType, std::pair<std::string, DfxEventType>> DFX_EVENT_MAP = {
    { PluginDfxEventType::PERF_SOURCE, { "SRC", DfxEventType::DFX_INFO_PERF_REPORT } },
    { PluginDfxEventType::DFX_EVENT_LOADING_ERROR, { "LOADING_ERROR", DfxEventType::DFX_EVENT_LOADING_ERROR } },
};
constexpr uint32_t LIMIT_MEMORY_REPORT_COUNT = 1000;
constexpr int32_t DFX_BUFFER_QUEUE_SIZE_MAX = 50;
constexpr int64_t LOG_INTERVAL_MS = 2000; // 2s
constexpr uint32_t LOG_MAX_COUNT = 10; // 10 times
constexpr int32_t CACHE_PRESSURE_LIMIT = 5 * 1000 * 1000;
constexpr int32_t CACHE_PRESSURE_LIMIT_TIME = 500;
constexpr int32_t BITRATE = 8;
constexpr int32_t CACHE_PRESSURE_TIME = 2;
constexpr float BASE_SPEED = 1.0f;
constexpr uint64_t BAND_WIDTH_LIMIT = 3 * 1024 * 1024;
constexpr int64_t CHECK_AUTO_SELECT_INTERVAL_MS = 2000;
constexpr float AVERAGE_LOAD_RATE_CHANGE_REPORT_LIMIT = 0.1f;
constexpr int32_t PUSH_CODEC_CHECK_BUFFER_RETRY_COUNT = 20;
constexpr uint32_t PUSH_CODEC_CHECK_BUFFER_RETRY_INTERVAL_US = 10000; // 10ms

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

struct MediaDemuxer::FilteredStreamMap {
    std::map<StreamType, std::vector<StreamInfo>> map;
};

using StreamFilterPredicate = std::function<bool(StreamInfo)>;

class MediaDemuxer::AVBufferQueueProducerListener : public IRemoteStub<IProducerListener> {
public:
    explicit AVBufferQueueProducerListener(int32_t trackId, std::shared_ptr<MediaDemuxer> demuxer,
        std::unique_ptr<Task>& notifyTask) : trackId_(trackId), demuxer_(demuxer), notifyTask_(std::move(notifyTask)) {}

    virtual ~AVBufferQueueProducerListener() = default;

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
      sampleQueueController_(std::make_shared<SampleQueueController>()),
      filteredStreamMap_(std::make_unique<FilteredStreamMap>())
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
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, nullptr, "Plugin manager is nullptr");
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

uint32_t MediaDemuxer::GetCurrentBitRate() const
{
    if (demuxerPluginManager_ != nullptr) {
        return demuxerPluginManager_->GetCurrentBitRate();
    }
    return 0;
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
            int64_t sourceDurationMs = Plugins::HstTime2Us(duration_);
            FALSE_RETURN_V_MSG_E(mediaMetaData_.globalMeta != nullptr, false, "globalMeta is nullptr");
            mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(sourceDurationMs);
            if (sourceDurationMs < 0) {
                sourceDurationMs = (sourceDurationMs == INT64_MIN) ? INT64_MAX : -sourceDurationMs;
            }
            (void)source_->SetMediaDuration(sourceDurationMs);
        }
        MEDIA_LOG_I("SeekToTime");
        FALSE_RETURN_V_MSG_E(mediaMetaData_.globalMeta != nullptr, false, "globalMeta is nullptr");
        return mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs);
    }
    
    // not hls and seekable
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        duration_ = source_->GetDuration();
        if (duration_ != Plugins::HST_TIME_NONE) {
            MEDIA_LOG_I("Not hls: " PUBLIC_LOG_D64, duration_);
            int64_t sourceDurationMs = Plugins::HstTime2Us(duration_);
            FALSE_RETURN_V_MSG_E(mediaMetaData_.globalMeta != nullptr, false, "globalMeta is nullptr");
            mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(sourceDurationMs);
            if (sourceDurationMs < 0) {
                sourceDurationMs = (sourceDurationMs == INT64_MIN) ? INT64_MAX : -sourceDurationMs;
            }
            (void)source_->SetMediaDuration(sourceDurationMs);
        }
        MEDIA_LOG_I("Seekble");
        FALSE_RETURN_V_MSG_E(mediaMetaData_.globalMeta != nullptr, false, "globalMeta is nullptr");
        return mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs);
    }
    MEDIA_LOG_I("Other");
    if (mediaMetaData_.globalMeta != nullptr &&
        mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs)) {
        (void)source_->SetMediaDuration(durationMs);
        return true;
    }
    return false;
}

bool MediaDemuxer::GetStartInfo(std::pair<int64_t, bool>& startInfo)
{
    if (source_ == nullptr) {
        return false;
    }
    if (source_->IsSeekToTimeSupported()) {
        startInfo = source_->GetStartInfo();
        return true;
    }
    return false;
}

std::vector<Plugins::SeekRange> MediaDemuxer::GetSeekableRanges()
{
    {
        AutoLock lock(mapMutex_);
        FALSE_RETURN_V_MSG(source_ != nullptr, {}, "source is nullptr");
        int64_t durationMs = 0;
        if (mediaMetaData_.globalMeta != nullptr &&
            mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs) && durationMs > 0) {
            (void)source_->SetMediaDuration(durationMs);
        }
        auto ranges = source_->GetSeekableRanges();
        if (!ranges.empty()) {
            return ranges;
        }
        seekable_ = source_->GetSeekable();
        if (seekable_ == Plugins::Seekable::SEEKABLE) {
            Plugins::SeekRange fullRange;
            if (GetLocalLoadedRange(durationMs, fullRange)) {
                ranges.push_back(fullRange);
                return ranges;
            }
        }
    }
    MEDIA_LOG_W("AVSource provides null seekable range, will use loaded range fallback");
    return GetLoadedRanges();
}

std::vector<Plugins::SeekRange> MediaDemuxer::GetLoadedRanges()
{
    std::vector<Plugins::SeekRange> ranges;
    {
        AutoLock lock(mapMutex_);
        FALSE_RETURN_V_MSG(source_ != nullptr, ranges, "source is nullptr");
        if (!IsLocalFd()) {
            Plugins::SeekRange range;
            if (BuildNetworkLoadedRange(range)) {
                ranges.push_back(range);
            }
            return ranges;
        }
    }

    int64_t durationMs = 0;
    (void)GetDuration(durationMs);
    {
        AutoLock lock(mapMutex_);
        FALSE_RETURN_V_MSG(source_ != nullptr && IsLocalFd(), ranges,
            "source is nullptr or should be local fd but not");
        Plugins::SeekRange range;
        if (GetLocalLoadedRange(durationMs, range)) {
            ranges.push_back(range);
            return ranges;
        }
    }
    return ranges;
}

bool MediaDemuxer::BuildNetworkLoadedRange(Plugins::SeekRange &range)
{
    FALSE_RETURN_V(GetEnableSampleQueueFlag() && !sampleQueueMap_.empty(), false);

    int64_t minLastOutSamplePts = Plugins::HST_TIME_NONE;
    int64_t minLastEnterSamplePts = Plugins::HST_TIME_NONE;
    bool hasValidQueue = false;
    for (const auto &sqIt : sampleQueueMap_) {
        auto &queue = sqIt.second;
        FALSE_CONTINUE_NOLOG(queue != nullptr);
        int64_t lastEnterPts = queue->GetLastEnterSamplePts();
        FALSE_CONTINUE_NOLOG(lastEnterPts > 0);
        int64_t lastOutPts = queue->GetLastOutSamplePts();
        if (!hasValidQueue) {
            minLastOutSamplePts = lastOutPts;
            minLastEnterSamplePts = lastEnterPts;
            hasValidQueue = true;
        } else {
            minLastOutSamplePts = std::min(minLastOutSamplePts, lastOutPts);
            minLastEnterSamplePts = std::min(minLastEnterSamplePts, lastEnterPts);
        }
    }
    FALSE_RETURN_V(hasValidQueue, false);
    int64_t loadedStartUs = (syncCenter_ != nullptr) ? syncCenter_->GetMediaTimeNow() : 0;
    if (loadedStartUs <= 0 && minLastOutSamplePts != Plugins::HST_TIME_NONE) {
        loadedStartUs = minLastOutSamplePts;
    }
    int64_t bufferedEnd = minLastEnterSamplePts;
    if (bufferedEnd > 0 && mediaStartPts_ != Plugins::HST_TIME_NONE) {
        bufferedEnd -= mediaStartPts_;
    }
    if (loadedStartUs < 0) {
        loadedStartUs = 0;
    }
    FALSE_RETURN_V(loadedStartUs < bufferedEnd, false);
    int64_t startHst = 0;
    int64_t endHst = 0;
    FALSE_RETURN_V(Plugins::Us2HstTime(loadedStartUs, startHst) && Plugins::Us2HstTime(bufferedEnd, endHst), false);
    FALSE_RETURN_V(endHst > startHst, false);
    range = {startHst, endHst};

    return true;
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
    demuxerPluginManager_->SetPlayerMode(isPlayerMode_);
    if (isPlayerMode_ || isTranscoderMode_ || isMetadataMode_) {
        demuxerPluginManager_->SetReadTimeoutForInitSeek(seekTimeoutMs_);
    }
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
        InitBuiltInSubtitleTrack();
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
    if (bundleName_ == "bootanimation") {
        MEDIA_LOG_I("boosting read thread priority for animation");
        return BoostReadThreadPriority() == Status::OK;
    }

#ifdef SUPPORT_START_STOP_ON_DEMAND
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
#endif

        if (!HasVideo() && HasAudio()) {
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
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER, "Source is nullptr");

    if (sampleQueueController_) {
        sampleQueueController_->SetBufferingDuration(source->GetPlayStrategy());
    }
    isPrepared_.store(false);
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_INVALID_PARAMETER, "source_ is nullptr");
    source_->SetCallback(shared_from_this());
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
    subtitleSource_->SetCallback(shared_from_this());
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
    MEDIA_LOG_I("Set bufferQueue for track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running");
    FALSE_RETURN_V_MSG_E(producer != nullptr, Status::ERROR_INVALID_PARAMETER, "BufferQueue is nullptr");
    if (bufferQueueMap_.find(trackId) != bufferQueueMap_.end()) {
        MEDIA_LOG_W("Has been set already");
    }
    if (IsSubtitleTrackId(trackId)) {
        subtitleTrackId_ = trackId;
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
        useBufferQueue_ = true;
        MEDIA_LOG_I("Set bufferQueue successfully");
        GenerateDfxBufferQueue(trackId);
        if (GetEnableSampleQueueFlag()) {
            ret = AddSampleBufferQueue(trackId);
            FALSE_RETURN_V_MSG_E(
                ret == Status::OK, ret, "AddSampleBufferQueue failed for track " PUBLIC_LOG_D32, trackId);
        }
    }
    InitBuiltInSubtitleBufferQueue(trackId, producer);
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
    MEDIA_LOG_I("InnerSelectTrack: " PUBLIC_LOG_D32, trackId);
    eosMap_[trackId] = false;
    segmentEosMap_[trackId] = false;
    requestBufferErrorCountMap_[trackId] = 0;

    int32_t innerTrackID = trackId;
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    if (IsNeedMapToInnerTrackID() || IsSubtitleTrackId(trackId)) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
        FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER, "Demuxer plugin is nullptr");
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr,
            Status::ERROR_INVALID_PARAMETER, "demuxerPluginManager_ is nullptr");
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

Status MediaDemuxer::StartTask(Plugins::MediaType mediaType)
{
    int32_t trackId = TRACK_ID_INVALID;
    if (mediaType == Plugins::MediaType::AUDIO) {
        trackId = audioTrackId_;
    } else if (mediaType == Plugins::MediaType::VIDEO) {
        trackId = videoTrackId_;
    } else if (mediaType == Plugins::MediaType::SUBTITLE) {
        trackId = subtitleTrackId_;
    } else {
        MEDIA_LOG_E("Media type invalid");
        return Status::ERROR_UNKNOWN;
    }
    return StartTask(trackId);
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

Status MediaDemuxer::HandleSegmentMediaSelectTrack(int32_t trackId)
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
    } else {
        MEDIA_LOG_E("Track type invalid");
        return Status::ERROR_UNKNOWN;
    }
    
    MediaStreamType streamType = GetDashChangeStreamType(static_cast<DemuxerTrackType>(trackType));
    
    int32_t curTmpStreamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId);
    bool isStreamIdChanged = (targetStreamID != curTmpStreamID);
    
    if (trackId == curTrackId || isStreamIdChanged) {
        MEDIA_LOG_I("Select stream");
        if (isStreamIdChanged) {
            RecordSelectTrackMediaChange(curTrackId, trackId, curTmpStreamID, targetStreamID, streamType);
        }
        {
            std::lock_guard<std::mutex> lock(isSelectTrackMutex_);
            inSelectTrackType_[static_cast<int32_t>(trackType)] = trackId;
        }
        FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_UNKNOWN, "source_ is null");
        auto ret = source_->SelectStream(targetStreamID);
        if (ret == Status::OK && isHls_ && curTrackId == subtitleTrackId_ && syncCenter_ != nullptr) {
            HandleSelectSubtitle(syncCenter_->GetMediaTimeNow(), SeekMode::SEEK_PREVIOUS_SYNC, trackId);
        }
        return ret;
    }
    
    RecordSelectTrackMediaChange(curTrackId, trackId, curTmpStreamID, targetStreamID, streamType);
    
    Status ret = DoSelectTrack(trackId, curTrackId);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "DoSelectTrack track failed");
    ReportTrackChangeEvent(trackId);
    return Status::OK;
}

void MediaDemuxer::RecordSelectTrackMediaChange(int32_t curTrackId, int32_t trackId,
    int32_t currentStreamID, int32_t targetStreamID, MediaStreamType streamType)
{
    MediaChangeParams params;
    params.isLocalFd = IsLocalFd();
    params.trackIdBefore = curTrackId;
    params.trackIdAfter = trackId;
    if (params.isLocalFd) {
        params.streamIdBefore = -1;
        params.streamIdAfter = -1;
    } else {
        params.streamIdBefore = currentStreamID;
        params.streamIdAfter = targetStreamID;
    }
    params.streamType = streamType;
    params.reason = MEDIA_CHANGE_MANUAL;
    RecordMediaChange(params);
}

void MediaDemuxer::ReportTrackChangeEvent(int32_t trackId)
{
    TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
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
}

Status MediaDemuxer::DoSelectTrack(int32_t trackId, int32_t curTrackId)
{
    if (IsValidTrackId(curTrackId)) {
        if (IsSubtitleTrackId(trackId) && (!IsNeedSelectTrack(trackId) || !IsNeedSelectTrack(curTrackId))) {
            return SelectBuiltInSubtitleTrack(trackId, curTrackId);
        }
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
        UpdateSegmentOffset(curTrackId, trackId);
        return InnerSelectTrack(trackId);
    }
    return Status::ERROR_UNKNOWN;
}

Status MediaDemuxer::HandleSelectTrack(int32_t trackId)
{
    int32_t curTrackId = TRACK_ID_INVALID;
    TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    MediaStreamType streamType = MEDIA_STREAM_TYPE_AUDIO;
    if (trackType == TrackType::TRACK_AUDIO) {
        MEDIA_LOG_I("Select audio [" PUBLIC_LOG_D32 "->" PUBLIC_LOG_D32 "]", audioTrackId_, trackId);
        curTrackId = audioTrackId_;
        streamType = MEDIA_STREAM_TYPE_AUDIO;
    } else if (trackType == TrackType::TRACK_SUBTITLE) {
        MEDIA_LOG_I("Select subtitle [" PUBLIC_LOG_D32 "->" PUBLIC_LOG_D32 "]", subtitleTrackId_, trackId);
        curTrackId = subtitleTrackId_;
        streamType = MEDIA_STREAM_TYPE_SUBTITLE;
    } else {
        MEDIA_LOG_W("Unsupport track " PUBLIC_LOG_D32, trackId);
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (curTrackId == trackId) {
        return Status::OK;
    }

    if (IsLocalFd()) {
        RecordLocalFdMediaChange(trackId, curTrackId, trackType, streamType);
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
        } else if (trackType == TrackType::TRACK_SUBTITLE) {
            eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_SUBTITLE_TRACK_CHANGE, trackId});
            subtitleTrackId_ =  trackId;
        } else {
            // invalid branch
        }
    }
    return ret;
}

void MediaDemuxer::RecordLocalFdMediaChange(int32_t trackId, int32_t curTrackId,
    TrackType trackType, MediaStreamType streamType)
{
    MediaChangeParams params;
    params.isLocalFd = true;
    params.trackIdBefore = curTrackId;
    params.trackIdAfter = trackId;
    params.streamIdBefore = -1;
    params.streamIdAfter = -1;
    params.streamType = streamType;
    params.reason = MEDIA_CHANGE_MANUAL;

    if (trackType == TrackType::TRACK_VIDEO) {
        params.videoTrackIdBefore = curTrackId;
        params.videoTrackIdAfter = trackId;
        params.audioTrackIdBefore = audioTrackId_;
        params.audioTrackIdAfter = audioTrackId_;
        params.subtitleTrackIdBefore = subtitleTrackId_;
        params.subtitleTrackIdAfter = subtitleTrackId_;
    } else if (trackType == TrackType::TRACK_AUDIO) {
        params.videoTrackIdBefore = videoTrackId_;
        params.videoTrackIdAfter = videoTrackId_;
        params.audioTrackIdBefore = curTrackId;
        params.audioTrackIdAfter = trackId;
        params.subtitleTrackIdBefore = subtitleTrackId_;
        params.subtitleTrackIdAfter = subtitleTrackId_;
    } else if (trackType == TrackType::TRACK_SUBTITLE) {
        params.videoTrackIdBefore = videoTrackId_;
        params.videoTrackIdAfter = videoTrackId_;
        params.audioTrackIdBefore = audioTrackId_;
        params.audioTrackIdAfter = audioTrackId_;
        params.subtitleTrackIdBefore = curTrackId;
        params.subtitleTrackIdAfter = trackId;
    }
    RecordMediaChange(params);
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
        return HandleSegmentMediaSelectTrack(trackId);
    }
    if (isHls_) {
        return HandleSegmentMediaSelectTrack(trackId);
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
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, Status::ERROR_NULL_POINTER, "Plugin manager is nullptr");
    bool isAVInOneStream = IsAVInOneStream();
    Status ret = Status::OK;
    if (!isAVInOneStream) {
        // not mixed
        ret = HandleSegmentChange(trackId, isAVInOneStream);
        MEDIA_LOG_I("HandleSegmentChange end");
        return ret;
    }
    FALSE_RETURN_V_NOLOG(IsSegmentEos(), Status::ERROR_ONE_TRACK_SEGMENT_EOS);
    if (IsAVInOneStream()) {
        for (auto &[track, isEos]: hlsSegmentEosMap_) {
            isEos = false;
        }
    } else {
        hlsSegmentEosMap_[trackId] = false;
    }
    if (IsValidTrackId(audioTrackId_)) {
        SetTrackIsBuffering(audioTrackId_, true);
    }
    if (IsValidTrackId(videoTrackId_)) {
        SetTrackIsBuffering(videoTrackId_, true);
    }
    MEDIA_LOG_I("HandleSegmentChange mixed start");
    int32_t tmpTrackId = IsValidTrackId(videoTrackId_) ? videoTrackId_ : audioTrackId_;
    ret = HandleSegmentChange(tmpTrackId, isAVInOneStream);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "HandleSegmentChange mixed failed");
    ret = (tmpTrackId == videoTrackId_ && IsValidTrackId(audioTrackId_)) ? InnerSelectTrack(audioTrackId_) : Status::OK;
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "HandleSegmentChange Select audio track failed");
    FALSE_GOON_NOEXEC(tmpTrackId == videoTrackId_ && IsValidTrackId(audioTrackId_),
        PushCodecCheckBuffer(audioTrackId_));
    MEDIA_LOG_I("HandleSegmentChange mixed end");
    return ret;
}

Status MediaDemuxer::HandleSegmentChange(int32_t trackId, bool isAVInOneStream)
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
    UpdateSegmentOffset(trackId);
    Status ret = demuxerPluginManager_->RebootPlugin(streamID, trackType, streamDemuxer_, isRebooted);
    if (ret != Status::OK) {
        FALSE_RETURN_V_MSG_E(streamDemuxer_ != nullptr, Status::ERROR_NULL_POINTER, "Stream is nullptr");
        TrackType type = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
        int32_t currentStreamID = demuxerPluginManager_->GetStreamIDByTrackType(type);
        int32_t newStreamID = demuxerPluginManager_->GetStreamDemuxerNewStreamID(type, streamDemuxer_);
        FALSE_RETURN_V_MSG_E(newStreamID != -1 && currentStreamID != newStreamID, ret, "StreamId not change.");
        FALSE_RETURN_V_MSG_E(HandleDashChangeStream(trackId), ret, "Reboot demuxer plugin failed");
    }
    Plugins::MediaInfo mediaInfo = demuxerPluginManager_->GetMediaInfo();
    InitMediaMetaData(mediaInfo); // update mediaMetaData_
    if (isAVInOneStream) {
        UpdateTrackMap();
    }
    PushCodecCheckBuffer(trackId);
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
        UpdateTrackMap();
    } else {
        ret = HandleHlsRebootPlugin(audioTrackId_);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Reboot audio plugin failed");
        ret = HandleHlsRebootPlugin(videoTrackId_);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Reboot video plugin failed");
        ret = HandleHlsRebootPlugin(subtitleTrackId_);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Reboot subtitle plugin failed");
    }
    MEDIA_LOG_I("Reboot hls plugin success, isAVInOneStream: %{public}d", isAVInOneStream);
    return Status::OK;
}

Status MediaDemuxer::HandleHlsRebootPlugin(int32_t trackId)
{
    MEDIA_LOG_I("HandleHlsRebootPlugin In, trackId:" PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V(!subStreamDemuxer_, Status::OK);
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
        } else if (seekReadyInfo.first >= 0 && seekReadyInfo.first != streamID &&
            trackType != TrackType::TRACK_SUBTITLE) {
            return HandleSeekChangeStream(streamID, seekReadyInfo.first, trackId);
        }
        bool isRebooted = true;
        UpdateSegmentOffset(trackId);
        ret = demuxerPluginManager_->RebootPlugin(streamID, trackType, streamDemuxer_, isRebooted);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Reboot demuxer plugin failed");
        Plugins::MediaInfo mediaInfo = demuxerPluginManager_->GetMediaInfo();
        InitMediaMetaData(mediaInfo); // update mediaMetaData_
        ret = InnerSelectTrack(trackId);
    }
    return ret;
}

Status MediaDemuxer::HandleSeekChangeStream(int32_t currentStreamId, int32_t newStreamId, int32_t trackId)
{
    MEDIA_LOG_I("streamID changed, streamId: " PUBLIC_LOG_D32 ", newStreamId: " PUBLIC_LOG_D32,
        currentStreamId, newStreamId);
    // Fix hls change stream after seek or change stream after completed seek
    if (streamDemuxer_ != nullptr && (isHls_ || HasEosTrack() ||
        (demuxerPluginManager_ != nullptr && demuxerPluginManager_->IsDash()))) {
        if (trackId == videoTrackId_) {
            streamDemuxer_->SetNewVideoStreamID(newStreamId);
        } else if (trackId == audioTrackId_) {
            streamDemuxer_->SetNewAudioStreamID(newStreamId);
        }
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
        if (seekReadyInfo.second == SEEK_TO_EOS) {
            MEDIA_LOG_I("Seek to eos");
            return Status::OK;
        } else if (seekReadyInfo.first >= 0 && seekReadyInfo.first != streamID) {
            return HandleSeekChangeStream(streamID, seekReadyInfo.first, trackId);
        }
        ret = demuxerPluginManager_->RebootPlugin(streamID, trackType, streamDemuxer_, isRebooted);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Reboot demuxer plugin failed");
        Plugins::MediaInfo mediaInfo = demuxerPluginManager_->GetMediaInfo();
        InitMediaMetaData(mediaInfo); // update mediaMetaData_
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
    if (sampleQueueMap_.find(subtitleTrackId_) != sampleQueueMap_.end() && sampleQueueMap_[subtitleTrackId_]) {
        sampleQueueMap_[subtitleTrackId_]->Clear();
        sampleQueueMap_[subtitleTrackId_]->UpdateLastOutSamplePts(seekTime * US_TO_MS);
        sampleQueueMap_[subtitleTrackId_]->UpdateLastEnterSamplePts(seekTime * US_TO_MS);
    }
    if (IsValidTrackId(audioTrackId_)) {
        SetTrackIsBuffering(audioTrackId_, true);
    }
    if (IsValidTrackId(videoTrackId_)) {
        SetTrackIsBuffering(videoTrackId_, true);
    }
}

void MediaDemuxer::HandleSeekToTime(int64_t seekTime)
{
    SeekToTimeAfter();
    ResetSampleQueueStatus(seekTime);

    FALSE_RETURN_NOLOG(demuxerPluginManager_ != nullptr && demuxerPluginManager_->IsDash());
    PushCodecCheckBuffer(audioTrackId_);
    FALSE_RETURN(hiplayerCallback_ != nullptr);
    FALSE_RETURN_NOLOG(IsValidTrackId(videoTrackId_) &&
        static_cast<size_t>(videoTrackId_) < mediaMetaData_.trackMetas.size());
    auto meta = mediaMetaData_.trackMetas[videoTrackId_];
    FALSE_RETURN(meta != nullptr);
    bool isCodecChanged = false;
    hiplayerCallback_->HandleVideoCodecChange(std::make_shared<Meta>(*meta), isCodecChanged, false);
}

Status MediaDemuxer::HandleSelectSubtitle(int64_t seekTime, Plugins::SeekMode mode, int32_t trackId)
{
    int32_t targetStreamID = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
    return HandleSelectSubtitleStream(seekTime, mode, targetStreamID);
}

Status MediaDemuxer::HandleSelectSubtitleStream(int64_t seekTime, Plugins::SeekMode mode, int32_t streamId)
{
    FALSE_RETURN_V_MSG_E(streamId != INVALID_STREAM_OR_TRACK_ID, Status::ERROR_INVALID_PARAMETER,
        "Invalid streamId");
    FALSE_RETURN_V_MSG_E(source_ != nullptr && source_->IsSeekToTimeSupported(), Status::ERROR_INVALID_PARAMETER,
        "Source is null or seek operation not supported");
    auto ret = source_->MediaSeekTimeByStreamId(seekTime, SeekMode::SEEK_PREVIOUS_SYNC, streamId);
    FALSE_RETURN_V_MSG_E(streamDemuxer_ != nullptr && ret == Status::OK, Status::ERROR_UNKNOWN,
        "Source SeekToSubtitle fail");
    streamDemuxer_->SetNewSubtitleStreamID(streamId);
    FALSE_RETURN_V_MSG_E(HandleDashChangeStream(subtitleTrackId_), Status::ERROR_UNKNOWN,
        "HandleDashChangeStream fail");
    return Status::OK;
}

Status MediaDemuxer::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    FALSE_RETURN_V_MSG_E(seekTime <= (INT64_MAX / US_TO_MS),
                         Status::ERROR_INVALID_DATA, "Millisecond is too large");
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    ProcessHistoryOnSeek();
    Status ret;
    isSeekError_.store(false);
    if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
        MEDIA_LOG_I("Source seek time: %{public}" PRId64, seekTime);
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
        if (!IsLocalFd()) {
            ResetSampleQueueStatus(seekTime);
        }
    }
    ResetAfterSeek(ret);
    MEDIA_LOG_D("Out");
    return ret;
}

void MediaDemuxer::ResetAfterSeek(Status ret)
{
    isSeeked_ = true;
    if (isVideoMuted_ || needRestore_) {
        if (sampleQueueMap_[videoTrackId_] != nullptr) {
            sampleQueueMap_[videoTrackId_]->Clear();
        }
        lastVideoPts_ = -1;
    }
    lastSubtitlePtsMap_.clear();
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
}

Status MediaDemuxer::SeekToStart(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    Status ret;
    isSeekError_.store(false);
    if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
        MEDIA_LOG_I("Source seek");
        if (mode == SeekMode::SEEK_CLOSEST_INNER) {
            ScopedTimer timer("seek closest online", SEEKCLOSEST_ONLINE_WARNING_MS);
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_PREVIOUS_SYNC);
        } else {
            ScopedTimer timer("seek online", SEEK_ONLINE_WARNING_MS);
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_CLOSEST_SYNC);
        }
        if (subtitleSource_) {
            demuxerPluginManager_->localSubtitleSeekToStart(seekTime);
        }
        HandleSeekToTime(seekTime);
        Plugins::Ms2HstTime(seekTime, realSeekTime);
    } else {
        MEDIA_LOG_I("Demuxer seek");
        ScopedTimer timer("SeekToStart", SEEK_LOCAL_WARNING_MS);
        ret = demuxerPluginManager_->SeekToStart(seekTime, realSeekTime);
        if (!IsLocalFd()) {
            ResetSampleQueueStatus(seekTime);
        }
    }
    isSeeked_ = true;
    if (isVideoMuted_ || needRestore_) {
        if (sampleQueueMap_[videoTrackId_] != nullptr) {
            sampleQueueMap_[videoTrackId_]->Clear();
        }
        lastVideoPts_ = -1;
    }
    lastSubtitlePtsMap_.clear();
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

Status MediaDemuxer::SeekToKeyFrame(int64_t seekTime, Plugins::SeekMode mode,
    int64_t& realSeekTime, DemuxerCallerType callerType)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    Status ret;
    isSeekError_.store(false);
    if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
        MEDIA_LOG_I("Source seek time: %{public}" PRId64, seekTime);
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
        ret = demuxerPluginManager_->SeekToKeyFrame(seekTime, mode, realSeekTime, callerType);
        if (!IsLocalFd()) {
            ResetSampleQueueStatus(seekTime);
        }
    }
    isSeeked_ = true;
    if (isVideoMuted_ || needRestore_) {
        if (sampleQueueMap_[videoTrackId_] != nullptr) {
            sampleQueueMap_[videoTrackId_]->Clear();
        }
        lastVideoPts_ = -1;
    }
    lastSubtitlePtsMap_.clear();
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    ResetSegmentEosMap();
    for (auto item : requestBufferErrorCountMap_) {
        requestBufferErrorCountMap_[item.first] = 0;
    }
    if (ret != Status::OK && callerType == DemuxerCallerType::PLAYER) {
        isSeekError_.store(true);
    }
    isFirstFrameAfterSeek_.store(true);
    convertErrorTime_.store(0);
    MEDIA_LOG_D("Out");
    return ret;
}

void MediaDemuxer::WaitTrackSelectionFilter(uint32_t bitRate, int32_t streamId, DemuxerTrackType type)
{
    if (type == DemuxerTrackType::VIDEO) {
        waitVideoBitrate_.store(bitRate);
    } else if (type == DemuxerTrackType::AUDIO) {
        waitAudioStreamId_.store(streamId);
    } else if (type == DemuxerTrackType::SUBTITLE) {
        waitSubtitleStreamId_.store(streamId);
    } else {
        MEDIA_LOG_W("Unsupported track type");
    }
}

void MediaDemuxer::ProcessTrackSelectionFilter()
{
    // process pending video bitrate switch
    uint32_t pendingBitrate = waitVideoBitrate_.load();
    if (pendingBitrate != 0 && demuxerPluginManager_ != nullptr) {
        waitVideoBitrate_.store(0);
        if (pendingBitrate != demuxerPluginManager_->GetCurrentBitRate()) {
            SelectBitRate(pendingBitrate, true, true);
            return;
        }
    }

    // process pending audio or subtitle stream switch (mutually exclusive)
    int32_t pendingAudio = waitAudioStreamId_.load();
    if (pendingAudio != -1) {
        waitAudioStreamId_.store(-1);
        if (defaultAudioStreamId_ == pendingAudio) {
            SelectStreamId(pendingAudio);
            return;
        }
    }

    int32_t pendingSubtitle = waitSubtitleStreamId_.load();
    if (pendingSubtitle != -1) {
        waitSubtitleStreamId_.store(-1);
        if (defaultSubtitleStreamId_ == pendingSubtitle) {
            SelectStreamId(pendingSubtitle, false);
            return;
        }
    }

    MEDIA_LOG_W("No track selection filter to process");
}

Status MediaDemuxer::SelectStreamId(int32_t streamId, bool isAudioStream)
{
    MEDIA_LOG_I("In");

    FALSE_RETURN_V(demuxerPluginManager_ != nullptr && streamDemuxer_ != nullptr, Status::ERROR_WRONG_STATE);
    FALSE_RETURN_V(streamId >= 0, Status::ERROR_WRONG_STATE);
    if (demuxerPluginManager_->IsDash()) {
        if (streamDemuxer_->CanDoChangeStream() == false) {
            WaitTrackSelectionFilter(0, streamId, isAudioStream ? DemuxerTrackType::AUDIO : DemuxerTrackType::SUBTITLE);
            MEDIA_LOG_W("Wrong state: selecting");
            return Status::ERROR_WRONG_STATE;
        }
    } else {
        streamDemuxer_->ResetAllCache();
    }
    if (source_ != nullptr) {
        return source_->SelectStream(streamId);
    }
    return Status::OK;
}

Status MediaDemuxer::SelectBitRate(uint32_t bitRate, bool isAutoSelect, bool isForceSelect)
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_INVALID_PARAMETER, "Source is nullptr");
    MEDIA_LOG_I("In");

    if (isFlvLiveStream_ && IsRightMediaTrack(videoTrackId_, DemuxerTrackType::VIDEO)) {
        FALSE_RETURN_V_NOLOG(!isManualBitRateSetting_.load() || !isAutoSelect || isForceSelect, Status::ERROR_UNKNOWN);
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
        if (!streamDemuxer_->CanDoChangeStream()) {
            if (isForceSelect) {
                WaitTrackSelectionFilter(bitRate, -1, DemuxerTrackType::VIDEO);
            }
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

    RecordSelectBitRateMediaChange(bitRate, isAutoSelect);

    Status ret = source_->SelectBitRate(bitRate);
    if (ret != Status::OK) {
        MEDIA_LOG_E("Source select bitrate failed");
        if (demuxerPluginManager_->IsDash()) {
            isSelectBitRate_.store(false);
        }
        ReportMediaChangedFailedEvent();
    }
    targetBitRate_ = bitRate;
    MEDIA_LOG_I("Out");
    return ret;
}

void MediaDemuxer::RecordSelectBitRateMediaChange(uint32_t bitRate, bool isAutoSelect)
{
    int32_t currentStreamId = -1;
    if (demuxerPluginManager_ != nullptr && videoTrackId_ >= 0) {
        currentStreamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(videoTrackId_);
    }
    int32_t targetStreamId = -1;
    std::vector<Plugins::StreamInfo> streams;
    if (source_ != nullptr) {
        source_->GetStreamInfo(streams);
    }
    for (const auto& stream : streams) {
        if (stream.bitRate == bitRate) {
            targetStreamId = stream.streamId;
            break;
        }
    }
    
    bool hasVideo = IsValidTrackId(videoTrackId_);
    bool hasAudio = IsValidTrackId(audioTrackId_);
    bool isMixed = hasVideo && hasAudio && IsAVInOneStream();
    MediaStreamType streamType = isMixed ? MEDIA_STREAM_TYPE_MIXED : MEDIA_STREAM_TYPE_VIDEO;
    
    MediaChangeParams params;
    params.isLocalFd = false;
    params.trackIdBefore = videoTrackId_;
    params.trackIdAfter = videoTrackId_;
    params.streamIdBefore = currentStreamId;
    params.streamIdAfter = targetStreamId;
    params.streamType = streamType;
    params.reason = isAutoSelect ? MEDIA_CHANGE_NETWORK_QUALITY : MEDIA_CHANGE_MANUAL;
    RecordMediaChange(params);
}

void MediaDemuxer::AdjustMediaChangeRecords(int32_t currentStreamID, int32_t newStreamID)
{
    auto targetIt = mediaChangeHistory_.rend();
    for (auto it = mediaChangeHistory_.rbegin(); it != mediaChangeHistory_.rend(); ++it) {
        if (!it->hasReported && it->streamIdAfter == newStreamID) {
            targetIt = it;
            break;
        }
    }
    if (targetIt == mediaChangeHistory_.rend()) {
        MEDIA_LOG_W("No unreported record found for newStreamID=" PUBLIC_LOG_D32, newStreamID);
        return;
    }
    if (targetIt->streamIdBefore == currentStreamID) {
        return;
    }
    MEDIA_LOG_I("Adjusting media change records: currentStreamID=" PUBLIC_LOG_D32
        " record.streamIdBefore=" PUBLIC_LOG_D32 " newStreamID=" PUBLIC_LOG_D32,
        currentStreamID, targetIt->streamIdBefore, newStreamID);
    auto targetBase = targetIt.base();
    for (auto it = mediaChangeHistory_.begin(); it != targetBase; ++it) {
        if (!it->hasReported) {
            MEDIA_LOG_I("Skipping intermediate record: streamIdBefore=" PUBLIC_LOG_D32
                " streamIdAfter=" PUBLIC_LOG_D32, it->streamIdBefore, it->streamIdAfter);
            it->hasReported = true;
        }
    }
    targetIt->streamIdBefore = currentStreamID;
    FillRecordFromStream(*targetIt, currentStreamID, newStreamID);
}

uint64_t MediaDemuxer::FindActiveChangeSeqNum(int32_t newStreamID)
{
    for (auto it = mediaChangeHistory_.rbegin(); it != mediaChangeHistory_.rend(); ++it) {
        if (!it->hasReported && it->streamIdAfter == newStreamID) {
            return it->changeSeqNum;
        }
    }
    return 0;
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
        streamDemuxer_->SetPlayerMode(isPlayerMode_);
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
        if (CheckTrackEnabledById(audioTrackId_)) {
            taskMap_[audioTrackId_]->PauseAsync();
            taskMap_[audioTrackId_]->Pause();
            if (GetEnableSampleQueueFlag()) {
                sampleConsumerTaskMap_[audioTrackId_]->PauseAsync();
                sampleConsumerTaskMap_[audioTrackId_]->Pause();
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
    ClearMediaChangeHistory();
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

Status MediaDemuxer::SetCachePressureCallback()
{
    Status status = demuxerPluginManager_->Start();
    auto weakDownloader = weak_from_this();
    CachePressureCallback cachePressureCallback = [weakDownloader] (int32_t trackId, uint32_t cachedBytes) -> void {
        auto shareDownloader = weakDownloader.lock();
        if (shareDownloader != nullptr) {
            shareDownloader->CachePressuredCallback(trackId, cachedBytes);
        }
    };
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    if (IsNeedMapToInnerTrackID()) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(videoTrackId_);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
    } else {
        int32_t streamID = demuxerPluginManager_->GetStreamIDByTrackID(videoTrackId_);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
    }
    if (pluginTemp == nullptr) {
        return status;
    }
    pluginTemp->SetCachePressureCallback(cachePressureCallback);
    int64_t bitRate = 0;
    mediaMetaData_.trackMetas[videoTrackId_]->GetData(Tag::MEDIA_BITRATE, bitRate);
    auto cachePressureLimit = bitRate * CACHE_PRESSURE_TIME / BITRATE;
    pluginTemp->SetTrackCacheLimit(videoTrackId_, cachePressureLimit > CACHE_PRESSURE_LIMIT ?
        static_cast<int32_t>(cachePressureLimit) : CACHE_PRESSURE_LIMIT, CACHE_PRESSURE_LIMIT_TIME);
    return status;
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

void MediaDemuxer::HandleForSourceSwitch()
{
    PauseAllTasksForSwitch();
    StopSourceAndPlugin();
    ClearSampleQueues();
}

void MediaDemuxer::PauseAllTasksForSwitch()
{
    PauseAllTaskAsync();
    PauseAllTask();
}

void MediaDemuxer::ClearSampleQueues()
{
    MEDIA_LOG_I("ClearSampleQueues in");
    AutoLock lock(mapMutex_);
    for (auto& kv : sampleQueueMap_) {
        if (kv.second != nullptr) {
            kv.second->Clear();
            MEDIA_LOG_I("ClearSampleQueues: cleared sampleQueue track %{public}d", kv.first);
        }
    }
    for (auto& kv : bufferQueueMap_) {
        if (kv.second != nullptr) {
            kv.second->Clear();
            MEDIA_LOG_I("ClearSampleQueues: cleared bufferQueue track %{public}d", kv.first);
        }
    }
    MEDIA_LOG_I("ClearSampleQueues done");
}

void MediaDemuxer::StopSourceAndPlugin()
{
    {
        std::unique_lock<std::mutex> stopLock(stopMutex_);
        isThreadExit_ = true;
        isStopped_ = false;
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
}

Status MediaDemuxer::ReselectTracks()
{
    if (IsValidTrackId(videoTrackId_)) {
        Status ret = InnerSelectTrack(videoTrackId_);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ReselectTracks: video track %{public}d failed", videoTrackId_);
    }
    if (IsValidTrackId(audioTrackId_)) {
        Status ret = InnerSelectTrack(audioTrackId_);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ReselectTracks: audio track %{public}d failed", audioTrackId_);
    }
    if (IsValidTrackId(subtitleTrackId_)) {
        InnerSelectTrack(subtitleTrackId_);
    }
    return Status::OK;
}

void MediaDemuxer::SetInterstitialController(std::shared_ptr<InterstitialController> controller)
{
    interstitialController_ = std::move(controller);
}

std::shared_ptr<InterstitialController> MediaDemuxer::GetInterstitialController() const
{
    return interstitialController_.lock();
}

std::shared_ptr<InterstitialScheduler> MediaDemuxer::GetInterstitialScheduler() const
{
    std::lock_guard<std::mutex> lock(interstitialSchedulerMutex_);
    return interstitialScheduler_;
}

std::shared_ptr<InterstitialScheduler> MediaDemuxer::SetInterstitialScheduler(
    std::shared_ptr<InterstitialScheduler> scheduler)
{
    std::lock_guard<std::mutex> lock(interstitialSchedulerMutex_);
    if (!interstitialScheduler_) {
        interstitialScheduler_ = std::move(scheduler);
    }
    return interstitialScheduler_;
}

void MediaDemuxer::DriveInterstitialTicks()
{
    auto scheduler = GetInterstitialScheduler();
    if (interstitialController_.expired() && !scheduler) {
        return;
    }
    auto controller = interstitialController_.lock();
    if (controller && controller->IsAdsDisabled()) {
        return;
    }

    int64_t nowMs = GetCurrentTimeMs();
    int64_t lastCheck = lastInterstitialCheckMs_.load();
    if (nowMs - lastCheck < INTERSTITIAL_CHECK_INTERVAL_MS) {
        return;
    }
    if (!lastInterstitialCheckMs_.compare_exchange_strong(lastCheck, nowMs)) {
        return;
    }

    if (scheduler) {
        scheduler->OnPlaybackTick();
    }

    if (controller && !controller->IsPlayingInterstitial() && controller->HasPendingEvents()) {
        controller->OnPreloadTick();
    }
}

void MediaDemuxer::HandleTimedMetadataEvent(const Plugins::PluginEvent& event)
{
    MEDIA_LOG_D("HandleTimedMetadataEvent");
    FALSE_RETURN_MSG(Any::IsSameTypeWith<std::shared_ptr<MediaAVCodec::AVTimedMetaData>>(event.param), "is not same");

    auto controller = interstitialController_.lock();
    if (controller && controller->IsPlayingInterstitial()) {
        MEDIA_LOG_I("HandleTimedMetadataEvent: discard ad source event");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(interstitialSchedulerMutex_);
        if (!interstitialScheduler_) {
            interstitialScheduler_ = std::make_shared<InterstitialScheduler>();
            interstitialScheduler_->SetSyncCenter(syncCenter_);
            std::weak_ptr<Pipeline::EventReceiver> weakReceiver = eventReceiver_;
            interstitialScheduler_->SetNotifyCallback([weakReceiver](
                const std::shared_ptr<MediaAVCodec::AVTimedMetaData>& metadata) {
                auto receiver = weakReceiver.lock();
                if (receiver && metadata) {
                    receiver->OnEvent({"demuxer_filter", EventType::EVENT_TIMED_METADATA, Any(metadata)});
                }
            });
            auto ctrl = interstitialController_.lock();
            if (ctrl) {
                auto strategy = ctrl->GetScheduleStrategy();
                if (strategy) {
                    interstitialScheduler_->SetScheduleStrategy(strategy);
                }
            }
        }
    }

    auto scheduler = GetInterstitialScheduler();
    auto metadata = AnyCast<std::shared_ptr<MediaAVCodec::AVTimedMetaData>>(event.param);
    if (scheduler && metadata) {
        scheduler->CollectEvent(metadata);
    }
}

Status MediaDemuxer::Stop()
{
    MEDIA_LOG_D("In");
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::Stop");
    ClearMediaChangeHistory();
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
        Plugins::MediaType mediaType;
        trackMeta.GetData(Tag::MEDIA_TYPE, mediaType);
        if (!isPlayerMode_ && mediaType == Plugins::MediaType::SUBTITLE) {
            trackMeta.Remove(Tag::MEDIA_LANGUAGE);
            trackMeta.Remove(Tag::MEDIA_TITLE);
        }
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
            if (isCover != trackMeta.end()) {
                continue;
            }
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
            if (IsBuiltInSubtitleTrackId(index)) {
                subtitleTrackIdVec_.push_back(index);
            }
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
        hasSetLargeSize_ = false;
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

    if (!subtitleTrackIdVec_.empty() && IsSubtitleTrackId(queueIndex)) {
        FALSE_RETURN_V(IsSubtitleQueueStale(queueIndex), false);
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
    MEDIA_LOG_D("SelectTrackChangeStream: TrackId: " PUBLIC_LOG_D32, trackId);
    StreamType streamType = demuxerPluginManager_->GetStreamTypeByTrackID(trackId);
    TrackType type = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    int32_t currentStreamID = demuxerPluginManager_->GetStreamIDByTrackType(type);
    int32_t currentTrackId = trackId;
    if (newStreamID == -1 || currentStreamID == -1 || currentStreamID == newStreamID) {
        return false;
    }
    MEDIA_LOG_I("In");
    UpdateSegmentOffset(trackId);

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
    UpdateSegmentOffset(currentTrackId, newTrackId);
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
        UpdateSegmentOffset(videoTrackId_);
        demuxerPluginManager_->StopPlugin(currentStreamID, streamDemuxer_);

        Status ret = demuxerPluginManager_->StartPlugin(newStreamID, streamDemuxer_);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Start plugin failed");

        Plugins::MediaInfo mediaInfo;
        demuxerPluginManager_->UpdateDefaultStreamID(mediaInfo, VIDEO, newStreamID);
        InitMediaMetaData(mediaInfo); // update mediaMetaData_
        int32_t newInnerTrackId = -1;
        int32_t newTrackId = -1;
        if (IsAVInOneStream()) {
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
        PushCodecCheckBuffer(videoTrackId_);
        InnerSelectTrack(videoTrackId_);
        if (IsValidTrackId(audioTrackId_)) {
            PushCodecCheckBuffer(audioTrackId_);
            InnerSelectTrack(audioTrackId_);
        }
        MEDIA_LOG_I("Out");
        return true;
    }
    return false;
}

void MediaDemuxer::SetCodecList(const std::shared_ptr<MediaAVCodec::AVCodecList> &codecList)
{
    codecList_ = codecList;
}

bool CheckStreamCodec(const std::string &codecs, StreamType type, std::shared_ptr<MediaAVCodec::AVCodecList> codecList)
{
    bool isSupported = true;
    if (type == Plugins::AUDIO) {
        std::vector<Plugins::PluginDescription> descriptions =
            Plugins::PluginList::GetInstance().GetPluginsByType(Plugins::PluginType::AUDIO_DECODER);
        isSupported = std::any_of(descriptions.begin(), descriptions.end(),
            [&codecs](const Plugins::PluginDescription &description) {
                if (description.cap.size() >= codecs.size()) {
                    return description.cap.find(codecs) != std::string::npos;
                } else {
                    return codecs.find(description.cap) != std::string::npos;
                }
            });
    } else {
        if (codecList == nullptr) {
            MEDIA_LOG_W("codecList is nullptr");
            return true;
        }
        auto caps = codecList->GetCapabilityList(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_DECODER);
        isSupported = std::any_of(caps.begin(), caps.end(),
            [&codecs](const std::shared_ptr<MediaAVCodec::CapabilityData> &cap) {
                if (!cap) {
                    return false;
                }
                const std::string &codecMime = cap->mimeType;
                if (codecMime.size() >= codecs.size()) {
                    return codecMime.find(codecs) != std::string::npos;
                } else {
                    return codecs.find(codecMime) != std::string::npos;
                }
            });
    }
    MEDIA_LOG_I("Codec: " PUBLIC_LOG_S " is supported: " PUBLIC_LOG_S, codecs.c_str(), isSupported ? "true" : "false");
    return isSupported;
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
                hasSetLargeSize_ = true;
            }
        }
        if (IsValidTrackId(audioTrackId_) && sampleQueueMap_[audioTrackId_] != nullptr &&
            bufferMap_[trackId]->pts_ <= sampleQueueMap_[audioTrackId_]->GetLastOutSamplePts()) {
            sampleQueueMap_[trackId]->Clear();
            sampleQueueMap_[trackId]->UpdateLastOutSamplePts(bufferMap_[trackId]->pts_);
        }
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
    if (!SourceDropFrame(trackId)) {
        return ret;
    }
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        if (bufferMap_[trackId]->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)) {
            return HandleTrackEos(trackId);
        }
        if (isAutoMaintainPts_.load() && trackId != subtitleTrackId_) {
            HandleAutoMaintainPts(trackId, bufferMap_[trackId]);
        }
        lastVideoPts_ = trackId == videoTrackId_ ? bufferMap_[trackId]->pts_ : lastVideoPts_;
        lastAudioPtsInMute_ = trackId == audioTrackId_ ? bufferMap_[trackId]->pts_ : lastAudioPtsInMute_;
        UpdateSubtitleLastPts(trackId);
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

bool MediaDemuxer::SourceDropFrame(int32_t trackId)
{
    if (!videoNeedIFrame_ || trackId != videoTrackId_) {
        return true;
    }
    if (bufferMap_[trackId]->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME)) {
        videoNeedIFrame_ = false;
        return true;
    }
    PushBufferToQueue(trackId, bufferMap_[trackId], false);
    return false;
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

bool MediaDemuxer::HandleDashChangeStream(int32_t trackId, bool isNeedAllEos)
{
    FALSE_RETURN_V_MSG_E(streamDemuxer_ != nullptr, false, "StreamDemuxer nullptr");
    MEDIA_LOG_D("In");
    TrackType type = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    int32_t currentStreamID = demuxerPluginManager_->GetStreamIDByTrackType(type);
    int32_t newStreamID = demuxerPluginManager_->GetStreamDemuxerNewStreamID(type, streamDemuxer_);
    bool ret = false;
    FALSE_RETURN_V_NOLOG(newStreamID != -1 && currentStreamID != newStreamID, ret);
    FALSE_RETURN_V_NOLOG(!isNeedAllEos || !isHls_ || !IsAVInOneStream() || IsSegmentEos(), false);
    AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGE, LOG_INTERVAL_MS, LOG_MAX_COUNT,
        "Change stream begin, currentStreamID: " PUBLIC_LOG_D32 " newStreamID: " PUBLIC_LOG_D32,
        currentStreamID, newStreamID);
    if ((trackId == videoTrackId_ || IsAVInOneStream()) &&
        demuxerPluginManager_->GetCurrentBitRate() != targetBitRate_) {
        ret = SelectBitRateChangeStream(trackId);
        if (ret) {
            streamDemuxer_->SetChangeFlag(true);
            MEDIA_LOG_I("targetBitrate: " PUBLIC_LOG_U32 " currentBitrate: " PUBLIC_LOG_U32, targetBitRate_,
                demuxerPluginManager_->GetCurrentBitRate());
            isSelectBitRate_.store(targetBitRate_ != demuxerPluginManager_->GetCurrentBitRate());
            ProcessTrackSelectionFilter();
            AdjustMediaChangeRecords(currentStreamID, newStreamID);
            activeChangeSeqNum_.store(FindActiveChangeSeqNum(newStreamID));
        }
    } else {
        isSelectTrack_.store(true);
        ret = SelectTrackChangeStream(trackId);
        if (ret) {
            MEDIA_LOG_I("targetBitrate: " PUBLIC_LOG_U32 " currentBitrate: " PUBLIC_LOG_U32, targetBitRate_,
                demuxerPluginManager_->GetCurrentBitRate());
            if (type == TrackType::TRACK_VIDEO) {
                targetBitRate_ = demuxerPluginManager_->GetCurrentBitRate();
            }
            streamDemuxer_->SetChangeFlag(true);
            ProcessTrackSelectionFilter();
            AdjustMediaChangeRecords(currentStreamID, newStreamID);
            activeChangeSeqNum_.store(FindActiveChangeSeqNum(newStreamID));
        }
        isSelectTrack_.store(false);
    }
    AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGE, LOG_INTERVAL_MS, LOG_MAX_COUNT, "Change stream success");
    return ret;
}

MediaStreamType MediaDemuxer::GetDashChangeStreamType(DemuxerTrackType trackType)
{
    MediaStreamType streamType = MEDIA_STREAM_TYPE_VIDEO;
    bool hasVideo = IsValidTrackId(videoTrackId_);
    bool hasAudio = IsValidTrackId(audioTrackId_);

    if (trackType == DemuxerTrackType::AUDIO) {
        if (hasVideo && hasAudio && IsAVInOneStream()) {
            streamType = MEDIA_STREAM_TYPE_MIXED;
        } else {
            streamType = MEDIA_STREAM_TYPE_AUDIO;
        }
    } else if (trackType == DemuxerTrackType::VIDEO) {
        if (hasVideo && hasAudio && IsAVInOneStream()) {
            streamType = MEDIA_STREAM_TYPE_MIXED;
        } else {
            streamType = MEDIA_STREAM_TYPE_VIDEO;
        }
    } else if (trackType == DemuxerTrackType::SUBTITLE) {
        int32_t subtitleStreamId = demuxerPluginManager_->GetStreamIDByTrackType(TrackType::TRACK_SUBTITLE);
        bool isInMixedStream = false;
        if (hasAudio && subtitleStreamId == defaultAudioStreamId_) {
            isInMixedStream = true;
        }
        if (hasVideo && subtitleStreamId == defaultVideoStreamId_) {
            isInMixedStream = true;
        }
        streamType = isInMixedStream ? MEDIA_STREAM_TYPE_MIXED : MEDIA_STREAM_TYPE_SUBTITLE;
    }
    return streamType;
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
    FALSE_RETURN_V_MSG_E(pluginTemp != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Demuxer plugin is nullptr: " PUBLIC_LOG_D32, trackId);
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
    int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
    bool isHlsSegmentEos = isHls_ && ret == Status::END_OF_STREAM && !source_->IsHlsEnd(streamId);
    if (isHlsSegmentEos) {
        segmentEosMap_[trackId] = true;
    }
    if (demuxerPluginManager_->IsDash() && HandleDashChangeStream(trackId, true)) {
        MEDIA_LOG_I("HandleDashChangeStream success");
        return Status::OK;
    }
    if (isHlsSegmentEos) {
        return HandleSegmentEos(trackId);
    }
    SetTrackNotifyFlag(trackId, true);
    if (!GetBufferFromUserQueue(trackId, isPlayerMode_ ? SampleQueue::DEFAULT_SAMPLE_BUFFER_CAP : size)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    SetTrackNotifyFlag(trackId, false);
    ret = HandleReadSample(trackId);
    ProduceWaterLoopControl(trackId);
    BufferingStatus();
    MEDIA_LOG_DD("CopyFrameToUserQueue Out, track:" PUBLIC_LOG_D32, trackId);
    return ret;
}

void PrintStreamInfo(const std::string &logTag, StreamInfo *streamInfo, const std::string &currentPlayType,
    Status status, int32_t currentBitRate = 0)
{
    FALSE_RETURN(streamInfo != nullptr);
    if (streamInfo->type == StreamType::VIDEO || streamInfo->type == StreamType::MIXED) {
        MEDIA_LOG_I(PUBLIC_LOG_S "desStreamInfo Video streamId: " PUBLIC_LOG_D32 " bitrate: " PUBLIC_LOG_D32
            " videoHeight: " PUBLIC_LOG_D32 " videoWidth: " PUBLIC_LOG_D32 " framerate: " PUBLIC_LOG_D32 " mimeType: "
            PUBLIC_LOG_S " codec:" PUBLIC_LOG_S " curDownloadBitrate: " PUBLIC_LOG_D32 " change stream state: "
            PUBLIC_LOG_D32 "videoType: " PUBLIC_LOG_D32, logTag.c_str(), streamInfo->streamId, streamInfo->bitRate,
            streamInfo->videoHeight, streamInfo->videoWidth, streamInfo->frameRate,
            currentPlayType == "dash" ? streamInfo->mimeType.c_str() : "", streamInfo->originCodecs.c_str(),
            currentBitRate, status, streamInfo->videoType);
    } else if (streamInfo->type == StreamType::AUDIO) {
        MEDIA_LOG_I(PUBLIC_LOG_S "desStreamInfo Audio streamId: " PUBLIC_LOG_D32 " channels: " PUBLIC_LOG_D32
            " bitRate: " PUBLIC_LOG_D32 " mimeType: " PUBLIC_LOG_S  " codec:" PUBLIC_LOG_S " lang: " PUBLIC_LOG_S
            " change stream state: " PUBLIC_LOG_D32, logTag.c_str(), streamInfo->streamId, streamInfo->channels,
            streamInfo->bitRate, currentPlayType == "dash" ? streamInfo->mimeType.c_str() : "",
            streamInfo->originCodecs.c_str(), streamInfo->lang.c_str(), status);
    } else if (streamInfo->type == StreamType::SUBTITLE) {
        MEDIA_LOG_I(PUBLIC_LOG_S "desStreamInfo Subtitle streamId: " PUBLIC_LOG_D32 " lang: " PUBLIC_LOG_S
            " change stream state: " PUBLIC_LOG_D32, logTag.c_str(), streamInfo->streamId, streamInfo->lang.c_str(),
            status);
    }
}

StreamInfo *GetPreferredVideoStream(std::map<StreamType, std::vector<StreamInfo>> &streamMap, uint32_t desBitrate,
    int32_t streamId = -1, std::string currentPlayType = "")
{
    bool isFirstSelect = true;
    StreamInfo *destStream = nullptr;
    uint32_t maxGap = 0;
    for (StreamInfo &stream : streamMap[VIDEO]) {
        if (stream.type != StreamType::VIDEO && stream.type != StreamType::MIXED) {
            continue;
        }
        if (streamId == stream.streamId && currentPlayType == "dash" && stream.videoType != VIDEO_TYPE_SDR) {
            return nullptr;
        }
        uint32_t tempGap = stream.bitRate > desBitrate ? stream.bitRate - desBitrate : desBitrate - stream.bitRate;
        if (isFirstSelect || tempGap < maxGap) {
            isFirstSelect = false;
            maxGap = tempGap;
            if (currentPlayType == "dash" && stream.videoType != VIDEO_TYPE_SDR) {
                continue;
            }
            destStream = &stream;
        }
    }
    if (destStream != nullptr) {
        MEDIA_LOG_I("GetPreferredVideoBitrate destStreamBitRate: " PUBLIC_LOG_D32 " curDownloadRate: " PUBLIC_LOG_D32,
            destStream->bitRate, desBitrate);
    }
    return destStream;
}

void MediaDemuxer::AutoSelectTrackByBitrate(int32_t trackId, bool isSelectHigherBitrate)
{
    FALSE_RETURN_NOLOG(isAutoSelect_.load() && curDownloadBitRate_.load() != 0 &&
        curDownloadBitRate_.load() != lastSwitchDownloadRate_.load());
    
    if (lastCheckAutoSelectTimeMs_ == 0) {
        lastCheckAutoSelectTimeMs_ = GetCurrentTimeMs();
        return;
    }
    if (GetCurrentTimeMs() - lastCheckAutoSelectTimeMs_ < CHECK_AUTO_SELECT_INTERVAL_MS) {
        return;
    }
    
    std::lock_guard<std::recursive_mutex> lock(streamMapMutex_);
    StreamInfo *preferredStreamInfo = GetPreferredVideoStream(filteredStreamMap_->map,
        curDownloadBitRate_.load(), defaultVideoStreamId_, currentPlayType_);
    FALSE_RETURN(preferredStreamInfo != nullptr);
    
    if (isSelectHigherBitrate) {
        if (preferredStreamInfo->bitRate < lastAutoSelectBitRate_ && lastAutoSelectBitRate_ != 0) {
            MEDIA_LOG_I("AutoSelectTrackByBitrate: Select lower bitrate stream");
            return;
        }
    } else {
        if (preferredStreamInfo->bitRate > lastAutoSelectBitRate_ && lastAutoSelectBitRate_ != 0) {
            MEDIA_LOG_I("AutoSelectTrackByBitrate: Select higher bitrate stream");
            return;
        }
    }
    
    MEDIA_LOG_D("Check can auto select, defaultVideoStreamId_: " PUBLIC_LOG_D32 " preferredStreamInfo->streamId: "
        PUBLIC_LOG_D32 " lastAutoSelectBitRate_: " PUBLIC_LOG_U32
        " trackId: " PUBLIC_LOG_D32 " preferredBitrate: " PUBLIC_LOG_U32 " isSelectBitRate_: " PUBLIC_LOG_D32
        " isSelectTrack_: " PUBLIC_LOG_D32 " targetBitRate_: " PUBLIC_LOG_U32
        " demuxerPluginManager_->GetCurrentBitRate(): " PUBLIC_LOG_U32, defaultVideoStreamId_,
        preferredStreamInfo->streamId, lastAutoSelectBitRate_, trackId, preferredStreamInfo->bitRate,
        isSelectBitRate_.load(), isSelectTrack_.load(), targetBitRate_, demuxerPluginManager_->GetCurrentBitRate());
    
    if (defaultVideoStreamId_ != preferredStreamInfo->streamId &&
        lastAutoSelectBitRate_ != preferredStreamInfo->bitRate && CanAutoSelectBitRate()) {
        Status status = SelectBitRate(preferredStreamInfo->bitRate, true);
        if (status == Status::OK) {
            defaultVideoStreamId_ = preferredStreamInfo->streamId;
            lastAutoSelectBitRate_ = preferredStreamInfo->bitRate;
            lastSwitchDownloadRate_.store(curDownloadBitRate_.load());
        } else {
            ReportMediaChangedFailedEvent();
        }
        PrintStreamInfo(
            "AutoSelectTrackByBitrate ", preferredStreamInfo, currentPlayType_, status, curDownloadBitRate_.load());
    }
    lastCheckAutoSelectTimeMs_ = 0;
}

void MediaDemuxer::StartConsume(int32_t trackId)
{
    bool startConsumeResult = false;
    if (trackId == videoTrackId_ && isVideoMuted_) {
        SetTrackIsBuffering(trackId, false);
        return;
    }
    {
        std::unique_lock<std::mutex> stopLock(stopMutex_);
        FALSE_RETURN(!isStopped_);
        AutoLock lock(mapMutex_);
        startConsumeResult = sampleQueueController_->ShouldStartConsume(trackId, sampleQueueMap_[trackId],
            sampleConsumerTaskMap_[trackId], inPreroll_.load());
    }
    if (!startConsumeResult && !eosMap_[trackId]) {
        // if controllor do not start consume, and not eos, do nothing
        return;
    }

    // set is buffering
    SetTrackIsBuffering(trackId, false);
    if (IsValidTrackId(audioTrackId_)) {
        // if both has video & audio, both check and start consumer task
        if (GetTrackIsBuffering(videoTrackId_) || GetTrackIsBuffering(audioTrackId_)) {
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
    if (GetTrackIsBuffering(videoTrackId_)) {
        return;
    }
    if (sampleConsumerTaskMap_[videoTrackId_] && !sampleConsumerTaskMap_[videoTrackId_]->IsTaskRunning()) {
        MEDIA_LOG_I("Pure Video StartConsume, trackId: %{public}d", videoTrackId_);
        sampleConsumerTaskMap_[videoTrackId_]->Start();
    }
    CheckAndReportBufferingStatus(EventType::BUFFERING_END);
}

int64_t MediaDemuxer::GetCurrentTimeMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void MediaDemuxer::ProduceWaterLoopControl(int32_t trackId)
{
    if (!sampleQueueController_ || trackId == subtitleTrackId_ || IsLocalFd()
        || !GetEnableSampleQueueFlag()) {
        return;
    }
    FALSE_RETURN_NOLOG(isBuffering_.load());
    StartConsume(trackId);
    if (trackId == videoTrackId_ && isVideoMuted_) {
        return;
    }
    {
        std::unique_lock<std::mutex> stopLock(stopMutex_);
        FALSE_RETURN(!isStopped_);
        AutoLock lock(mapMutex_);
        sampleQueueController_->ShouldStopProduce(trackId, sampleQueueMap_[trackId], taskMap_[trackId]);
    }
}

void MediaDemuxer::BufferingStatus()
{
    int32_t mainTrackId = GetMainTrackId();
    if (IsLocalFd() || sampleQueueMap_.find(mainTrackId) == sampleQueueMap_.end()
        || sampleQueueMap_[mainTrackId] == nullptr) {
        return;
    }
    if (GetTrackIsBuffering(mainTrackId)) {
        int64_t percent = 0;
        uint64_t temDuration = sampleQueueController_->GetPlayBufferingDuration();
        if (temDuration > 0) {
            percent = static_cast<int64_t>((sampleQueueMap_[mainTrackId]->NewGetCacheDuration() * MAX_PERCENT) /
                temDuration);
        } else {
            percent = MAX_PERCENT;
        }
            
        AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGI, LOG_INTERVAL_MS, LOG_MAX_COUNT,
            "BUFFERING_PERCENT: %{public}" PRId64, percent);
        if (eventReceiver_) {
            eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_BUFFER_PROGRESS, percent});
        }
    }
    auto cachedDuration = static_cast<int64_t>(sampleQueueMap_[mainTrackId]->NewGetCacheDuration() / US_TO_MS);
    if (std::abs(cachedDuration - lastCacheDuration_) > DURATION_CHANGE_AMOUNT_MILLIONSECOND) {
        MEDIA_LOG_I("CACHED_DURATION: %{public}" PRId64, cachedDuration);
        if (eventReceiver_) {
            eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_CACHED_DURATION, cachedDuration});
        }
        lastCacheDuration_ = cachedDuration;
    }
}

int32_t MediaDemuxer::GetMainTrackId()
{
    if (IsValidTrackId(videoTrackId_)) {
        return videoTrackId_;
    }
    if (IsValidTrackId(audioTrackId_)) {
        return audioTrackId_;
    }
    return INVALID_STREAM_OR_TRACK_ID;
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

    if (sample != nullptr && sample->meta_ != nullptr && mediaChangeSeqNum_.load() > 0) {
        int32_t streamId = IsNeedMapToInnerTrackID() ?
            demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId) :
            demuxerPluginManager_->GetStreamIDByTrackID(trackId);
        sample->meta_->SetData(Tag::REGULAR_TRACK_ID, trackId);
        sample->meta_->SetData(Tag::STREAM_ID_FOR_REPORT, streamId);
        if (!mediaChangeHistory_.empty()) {
            TagSampleWithLatestChange(sample, trackId);
        }
    }
    ProcessDrmInfos();
    RecordDemuxerTimeStamp(*sample, StallingStage::DEMUXER_END);
    return ret;
}

int32_t MediaDemuxer::GetInnerTrackId(int32_t trackId)
{
    if (IsNeedMapToInnerTrackID()) {
        return demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    }
    return trackId;
}

void MediaDemuxer::TagSampleWithLatestChange(std::shared_ptr<AVBuffer> sample, int32_t trackId)
{
    if (mediaChangeHistory_.empty()) {
        return;
    }
    uint64_t seqNum = activeChangeSeqNum_.load();
    if (seqNum == 0) {
        auto it = std::find_if(mediaChangeHistory_.rbegin(), mediaChangeHistory_.rend(),
            [](const MediaChangeRecord& record) { return !record.hasReported; });
        if (it == mediaChangeHistory_.rend()) {
            return;
        }
        seqNum = it->changeSeqNum;
    }
    sample->meta_->SetData(Tag::MEDIA_CHANGE_SEQ, seqNum);
    auto it = std::find_if(mediaChangeHistory_.rbegin(), mediaChangeHistory_.rend(),
        [seqNum](const MediaChangeRecord& record) { return !record.hasReported && record.changeSeqNum == seqNum; });
    if (it == mediaChangeHistory_.rend()) {
        return;
    }
    auto& targetRecord = *it;
    TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    bool shouldTag = false;
    if (trackType == TrackType::TRACK_AUDIO) {
        shouldTag = (targetRecord.streamType == Plugins::StreamType::AUDIO ||
            targetRecord.streamType == Plugins::StreamType::MIXED);
    } else if (trackType == TrackType::TRACK_VIDEO) {
        shouldTag = (targetRecord.streamType == Plugins::StreamType::VIDEO ||
            targetRecord.streamType == Plugins::StreamType::MIXED);
    } else if (trackType == TrackType::TRACK_SUBTITLE) {
        if (targetRecord.streamType == Plugins::StreamType::SUBTITLE ||
            targetRecord.streamType == Plugins::StreamType::MIXED) {
            shouldTag = (targetRecord.trackIdBefore == subtitleTrackId_ ||
                targetRecord.trackIdAfter == subtitleTrackId_);
        }
    }
    if (shouldTag) {
        sample->meta_->SetData(Tag::MEDIA_CHANGE_STREAM_TYPE,
            static_cast<int32_t>(targetRecord.streamType));
    }
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
        if (isPlayerMode_) {
            FALSE_RETURN_V_NOLOG(perfRecEnabled_,
                pluginTemp->ReadSampleZeroCopy(static_cast<uint32_t>(innerTrackID), sample, timeout_));
            demuxDuration = CALC_EXPR_TIME_MS(
                ret = pluginTemp->ReadSampleZeroCopy(static_cast<uint32_t>(innerTrackID), sample, timeout_));
        } else {
            FALSE_RETURN_V_NOLOG(perfRecEnabled_,
                pluginTemp->ReadSample(static_cast<uint32_t>(innerTrackID), sample, timeout_));
            demuxDuration =
                CALC_EXPR_TIME_MS(ret = pluginTemp->ReadSample(static_cast<uint32_t>(innerTrackID), sample, timeout_));
        }
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
    return std::min(sampleDuration >> SAMPLE_FLOW_CONTROL_RATE_POW, static_cast<uint64_t>(SAMPLE_LOOP_DELAY_TIME_US));
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
    std::unique_lock<std::mutex> stopLock(stopMutex_);
    FALSE_RETURN(!isStopped_);
    AutoLock lock(mapMutex_);
    hlsSegmentEosMap_[trackId] = true;
    // check sampleQueue video and audio exit
    bool audioQueueExists = sampleQueueMap_.find(audioTrackId_) != sampleQueueMap_.end()
        && sampleQueueMap_[audioTrackId_] != nullptr;
    bool videoQueueExists = sampleQueueMap_.find(videoTrackId_) != sampleQueueMap_.end()
        && sampleQueueMap_[videoTrackId_] != nullptr;
    // if sampleQueue reached the limit.
    bool audioTaskExists = taskMap_.find(audioTrackId_) != taskMap_.end() && taskMap_[audioTrackId_] != nullptr;
    bool videoTaskExists = taskMap_.find(videoTrackId_) != taskMap_.end() && taskMap_[videoTrackId_] != nullptr;
    if (GetTrackIsBuffering(trackId) && ((audioTaskExists && !taskMap_[audioTrackId_]->IsTaskRunning()) ||
        (videoTaskExists && !taskMap_[videoTrackId_]->IsTaskRunning()) ||
        (audioQueueExists &&
            sampleQueueMap_[audioTrackId_]->GetFilledBufferSize() >= SampleQueue::DEFAULT_SAMPLE_QUEUE_SIZE - 1) ||
        (videoQueueExists &&
            sampleQueueMap_[videoTrackId_]->GetFilledBufferSize() >= SampleQueue::DEFAULT_SAMPLE_QUEUE_SIZE - 1))) {
        SetTrackIsBuffering(trackId, false);
        // check sampleConsumerTaskMap video and audio exit
        bool audioConsumerExists = sampleConsumerTaskMap_.find(audioTrackId_) != sampleConsumerTaskMap_.end()
            && sampleConsumerTaskMap_[audioTrackId_] != nullptr;
        bool videoConsumerExists = sampleConsumerTaskMap_.find(videoTrackId_) != sampleConsumerTaskMap_.end()
            && sampleConsumerTaskMap_[videoTrackId_] != nullptr;
        if (audioConsumerExists && !sampleConsumerTaskMap_[audioTrackId_]->IsTaskRunning()) {
            MEDIA_LOG_I("Audio StartConsume, trackId: %{public}d", audioTrackId_);
            sampleConsumerTaskMap_[audioTrackId_]->Start();
        }
        if (videoConsumerExists && !sampleConsumerTaskMap_[videoTrackId_]->IsTaskRunning()) {
            MEDIA_LOG_I("Video StartConsume, trackId: %{public}d", videoTrackId_);
            sampleConsumerTaskMap_[videoTrackId_]->Start();
        }
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
    if (isStopped_ || isPaused_ || isSeekError_ || isFlvLiveSelectingBitRate_ ||
        (streamDemuxer_ != nullptr && streamDemuxer_->GetIsIgnoreParse())) {
        MEDIA_LOG_D("ReadLoop pausing or error, track " PUBLIC_LOG_D32, trackId);
        perfRecorder_.Reset();
        return 6 * 1000; // sleep 6ms in pausing to avoid useless reading
    }
    auto resPreReadSample = DoBeforeEachLoop(trackId);
    FALSE_RETURN_V_NOLOG(resPreReadSample == 0, resPreReadSample);
    AfterDrop(trackId);
    AfterSeekNeedDrop(trackId);
    if (GetTrackSeekNeedDrop(trackId)) {
        return NEXT_DELAY_TIME_US;
    }
    if (HandleFrameDropForTrack(trackId) > 0) {
        return NEXT_DELAY_TIME_US;
    }
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
            eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR,
                static_cast<int32_t>(MSERR_DATA_SOURCE_ERROR_UNKNOWN), GetMime() + GetMarkedUrl(trackId)});
        } else {
            MEDIA_LOG_D("EventReceiver is nullptr");
        }
    }
    FALSE_GOON_NOEXEC(ret == Status::ERROR_PACKET_CONVERT_FAILED, HandlePacketConvertError(trackId));
    FALSE_GOON_NOEXEC(ret == Status::OK, convertErrorTime_.store(0));
    bool isNeedRetry = ret == Status::OK || ret == Status::ERROR_AGAIN || ret == Status::ERROR_WAIT_TIMEOUT;
    if (isNeedRetry) {
        return GetReadLoopRetryUs(trackId);
    }
    if (ret == Status::ERROR_NO_MEMORY) {
        MEDIA_LOG_E("Cache data size is out of limit");
        if (eventReceiver_ != nullptr && !isOnEventNoMemory_.load()) {
            isOnEventNoMemory_.store(true);
            eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR,
                static_cast<int32_t>(MSERR_DEMUXER_BUFFER_NO_MEMORY), GetMime()});
        }
        return GetEnableSampleQueueFlag() ? NEXT_DELAY_TIME_US : 0;
    }
    MEDIA_LOG_DD("ReadLoop wait, track:" PUBLIC_LOG_D32 ", ret:" PUBLIC_LOG_D32, trackId, static_cast<int32_t>(ret));
    return RETRY_DELAY_TIME_US; // delay to retry if no frame
}

std::string MediaDemuxer::GetMarkedUrl(int32_t trackId)
{
    std::string curUrl = "-";
    if (isHls_ && demuxerPluginManager_!= nullptr && source_ != nullptr) {
        int32_t streamId = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
        curUrl += source_->GetMaskUrlByReadIndex(streamId);
    }
    return curUrl;
}

int64_t MediaDemuxer::HandleFrameDropForTrack(int32_t trackId)
{
    uint32_t frameCount = GetTrackNeedDropFrame(trackId);
    if (frameCount <= 0) {
        return 0;
    }
    if (IsLocalFd()) {
        return 0;
    }
    std::shared_ptr<AVBuffer> sample = AVBuffer::CreateAVBuffer();
    if (!sample) {
        SetTrackNeedDropFrame(trackId, 0);
        return 0;
    }
    Status dropStatus = ReadSampleToDrop(trackId, sample);
    if (dropStatus == Status::OK) {
        SetTrackNeedDropFrame(trackId, frameCount - 1);
    } else {
        SetTrackNeedDropFrame(trackId, 0);
        return 0;
    }
    return NEXT_DELAY_TIME_US;
}

void MediaDemuxer::AfterSeekNeedDrop(int32_t trackId)
{
    if (!GetTrackSeekNeedDrop(trackId)) {
        return;
    }
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    int32_t innerTrackID = trackId;
    if (IsNeedMapToInnerTrackID()) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        int32_t streamId = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamId);
    }
    if (pluginTemp == nullptr) {
        return;
    }
    std::shared_ptr<AVBuffer> sample = AVBuffer::CreateAVBuffer();
    pluginTemp->ReadSampleZeroCopy(static_cast<int32_t>(innerTrackID), sample, timeout_);
    if (sample->dts_ >= afterDropDts_[trackId]) {
        SetTrackSeekNeedDrop(trackId, false);
    }
}

void MediaDemuxer::HandlePacketConvertError(int32_t trackId)
{
    ++convertErrorTime_;
    TRUE_LOG(convertErrorTime_.load() == 1, MEDIA_LOG_W, "PacketConvert error once");
    FALSE_RETURN_NOLOG(convertErrorTime_ >= CONVERT_PACKET_ERROR_MAX_COUNT);
    MEDIA_LOG_E("PacketConvertError happened %{public}d times, stream is unsupported!", convertErrorTime_.load());
    FALSE_RETURN_MSG(eventReceiver_ != nullptr, "eventReceiver_ is nullptr");
    eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR,
        static_cast<int32_t>(MSERR_DATA_SOURCE_ERROR_UNKNOWN), GetMime() + GetMarkedUrl(trackId)});
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
    Status ret = InnerReadSample(static_cast<int32_t>(trackIndex), sample,
        readSampleMode_ == ReadSampleMode::READ_SAMPLE_SYNC);
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

void MediaDemuxer::DispatchPluginEvent(const Plugins::PluginEvent &event,
    const std::shared_ptr<Pipeline::EventReceiver> &eventReceiver)
{
    switch (event.type) {
        case PluginEventType::SOURCE_DRM_INFO_UPDATE: {
            HandleDrmInfoUpdate(event);
            break;
        }
        case PluginEventType::CLIENT_ERROR: {
            eventReceiver->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, event.param, "client"});
            break;
        }
        case PluginEventType::SERVER_ERROR: {
            eventReceiver->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, event.param, "server"});
            break;
        }
        case PluginEventType::CACHED_DURATION: {
            FALSE_BREAK_NOLOG(!GetEnableSampleQueueFlag() && IsNeedPreDownload());
            MEDIA_LOG_D("OnEvent cached duration");
            eventReceiver->OnEvent({"demuxer_filter", EventType::EVENT_CACHED_DURATION, event.param});
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
        case PluginEventType::SOURCE_TIMED_METADATA: {
            HandleTimedMetadataEvent(event);
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
    OnEventStream(event);
    std::weak_ptr<Pipeline::EventReceiver> weakEventReceiver = eventReceiver_;
    auto eventReceiver = weakEventReceiver.lock();
    if (eventReceiver == nullptr && event.type != PluginEventType::SOURCE_DRM_INFO_UPDATE) {
        MEDIA_LOG_E("EventReceiver is nullptr");
        return;
    }
    DispatchPluginEvent(event, eventReceiver);
    OnEventBuffer(event, eventReceiver);
}

void MediaDemuxer::HandleDrmInfoUpdate(const Plugins::PluginEvent &event)
{
    MEDIA_LOG_D("OnEvent source drmInfo update");
    if (Any::IsSameTypeWith<std::multimap<std::string, std::vector<uint8_t>>>(event.param)) {
        HandleSourceDrmInfoEvent(AnyCast<std::multimap<std::string, std::vector<uint8_t>>>(event.param));
    }
}

void MediaDemuxer::OnEventStream(const Plugins::PluginEvent &event)
{
    switch (event.type) {
        case PluginEventType::STREAM_UPDATE: {
            MEDIA_LOG_D("OnEvent stream update.");
            FALSE_RETURN(Any::IsSameTypeWith<std::string>(event.param));
            currentPlayType_ = AnyCast<std::string>(event.param);
            MEDIA_LOG_I("current play type: %{public}s", currentPlayType_.c_str());
            ChooseDefaultStreams();
            FALSE_RETURN(source_ != nullptr);
            source_->SetDefaultStreamId(defaultVideoStreamId_, defaultAudioStreamId_, defaultSubtitleStreamId_);
            MEDIA_LOG_I("select default streams: video: " PUBLIC_LOG_D32 ", audio: " PUBLIC_LOG_D32 ", subtitle: "
                PUBLIC_LOG_D32, defaultVideoStreamId_, defaultAudioStreamId_, defaultSubtitleStreamId_);
            break;
        }
        case PluginEventType::NETWORK_BITRATE_CHANGED: {
            MEDIA_LOG_D("OnEvent network bitrate change.");
            FALSE_RETURN(Any::IsSameTypeWith<uint32_t>(event.param));
            curDownloadBitRate_.store(AnyCast<uint32_t>(event.param));
            break;
        }
        case PluginEventType::LOADING_RATE_CHANGED: {
            MEDIA_LOG_D("OnEvent loading rate change.");
            FALSE_RETURN(Any::IsSameTypeWith<uint32_t>(event.param));
            uint32_t newBitRate = AnyCast<uint32_t>(event.param);
            uint32_t prevBitRate = prevBitRate_.load();
            prevBitRate_.store(newBitRate);
            if (prevBitRate <= 0) {
                break;
            }
            double changeRatio = std::abs(static_cast<double>(newBitRate) - static_cast<double>(prevBitRate)) /
                static_cast<double>(prevBitRate);
            if (changeRatio <= AVERAGE_LOAD_RATE_CHANGE_REPORT_LIMIT) {
                break;
            }
            MEDIA_LOG_I("Loading rate changed significantly: prev=" PUBLIC_LOG_U32
                ", new=" PUBLIC_LOG_U32 ", ratio=" PUBLIC_LOG_F, prevBitRate, newBitRate, changeRatio);
            if (eventReceiver_ != nullptr) {
                std::pair<uint32_t, uint32_t> ratePair = {prevBitRate, newBitRate};
                eventReceiver_->OnDfxEvent({"demuxer", DfxEventType::DFX_EVENT_LOADING_BITRATE, ratePair});
            }
            break;
        }
        default:
            break;
    }
}

void MediaDemuxer::OnEventBuffer(const Plugins::PluginEvent &event,
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver)
{
    switch (event.type) {
        case PluginEventType::BUFFERING_END: {
            FALSE_BREAK_NOLOG(!GetEnableSampleQueueFlag() && IsNeedPreDownload()); // no sampleQueue and dataSrc
            MEDIA_LOG_D("OnEvent pause");
            eventReceiver->OnEvent({"demuxer_filter", EventType::BUFFERING_END, PAUSE});
            break;
        }
        case PluginEventType::BUFFERING_START: {
            FALSE_BREAK_NOLOG(!GetEnableSampleQueueFlag() && IsNeedPreDownload());
            MEDIA_LOG_D("OnEvent start");
            eventReceiver->OnEvent({"demuxer_filter", EventType::BUFFERING_START, START});
            break;
        }
        case PluginEventType::EVENT_BUFFER_PROGRESS: {
            FALSE_BREAK_NOLOG(!GetEnableSampleQueueFlag() && IsNeedPreDownload());
            MEDIA_LOG_D("OnEvent percent update");
            eventReceiver->OnEvent({"demuxer_filter", EventType::EVENT_BUFFER_PROGRESS, event.param});
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

void MediaDemuxer::SetReadSampleMode(ReadSampleMode readSampleMode)
{
    readSampleMode_ = readSampleMode;
}

void MediaDemuxer::SetReadSampleTimeout(uint32_t timeoutMs)
{
    timeout_ = timeoutMs;
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
            MEDIA_LOG_D("Set last audio pts " PUBLIC_LOG_D64, lastAudioPts_);
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

Status MediaDemuxer::SetMetadataMode()
{
    isMetadataMode_ = true;
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
    FALSE_RETURN_MSG(source_ != nullptr, "Source is nullptr!");
    source_->WaitForBufferingEnd();
}

int32_t MediaDemuxer::GetCurrentVideoTrackId()
{
    return videoTrackId_;
}

int32_t MediaDemuxer::GetCurrentAudioTrackId()
{
    return audioTrackId_;
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
    sampleQueueConfig.queueSize_ = IsLocalFd() ? SampleQueue::FD_SAMPLE_QUEUE_SIZE :
        SampleQueue::DEFAULT_SAMPLE_QUEUE_SIZE;
    if (trackId == subtitleTrackId_) {
        sampleQueueConfig.queueSize_ = 1;
    }
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

        bool needRelease = false;
        CheckCodecChange(sampleQueue, trackId, needRelease);
        if (needRelease) {
            status = HandlePushBuffer(trackId, dstBuffer, bufferQueue, Status::ERROR_NULL_POINT_BUFFER);
            break;
        }

        if (static_cast<int32_t>(size) <= dstBuffer->memory_->GetCapacity()) {
            status = PushDirectBuffer(trackId, sampleQueue, dstBuffer, bufferQueue);
        } else {
            status = PushBufferBySlices(trackId, sampleQueue, dstBuffer, bufferQueue);
        }
    } while (0);
    DriveInterstitialTicks();
    float speed = speed_.load() < BASE_SPEED ? BASE_SPEED : speed_.load();
    return status == Status::OK ?
        static_cast<int64_t>(NEXT_DELAY_TIME_US / speed) : static_cast<int64_t>(SAMPLE_LOOP_DELAY_TIME_US / speed);
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
    auto srcBufferSize = sliceSize == 0 ? srcBuffer->memory_->GetSize() - srcBuffer->memory_->GetOffset() : sliceSize;
    int32_t copySize = std::min(dstBuffer->memory_->GetCapacity(), srcBufferSize);
    MEDIA_LOG_D("prepare to copy: %{public}d, dest cap: %{public}d, src buf id: %{public}" PRIu64,
        copySize, dstBuffer->memory_->GetCapacity(), srcBuffer->GetUniqueId());
    auto status = sampleQueueMap_[trackId]->CopyBufferSlice(srcBuffer, dstBuffer, copySize);
    FALSE_RETURN_V_MSG_E(status == Status::OK, status, "CopyPartBuffer failed, errCode: %{public}d", status);
    if (dstBuffer->meta_ != nullptr && IsSubtitleTrackId(trackId)) {
        dstBuffer->meta_->SetData(Tag::REGULAR_TRACK_ID, trackId);
    }
    status = HandlePushBuffer(trackId, dstBuffer, bufferQueueMap_[trackId], Status::OK);
    FALSE_RETURN_V_MSG_E(status == Status::OK, status, "HandlePushBuffer failed, errCode: %{public}d", status);
    status = CheckAndReleaseRemainBuffer(srcBuffer, trackId);
    return status;
}

Status MediaDemuxer::ReleaseSrcBuffer(std::shared_ptr<AVBuffer> &srcBuffer, int32_t trackId)
{
    MEDIA_LOG_D("release src buff id: %{public}" PRId64, srcBuffer->GetUniqueId());
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

bool MediaDemuxer::HandleInterstitialEos(int32_t trackId, const std::shared_ptr<AVBuffer>& dstBuffer)
{
    if (!(dstBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS))) {
        return false;
    }
    if (trackId != videoTrackId_ && trackId != audioTrackId_) {
        return false;
    }
    auto controller = GetInterstitialController();
    if (!controller) {
        return false;
    }
    if (controller->IsPlayingInterstitial()) {
        MEDIA_LOG_I("SampleConsumerLoop: ad EOS detected, notifying controller, trackId=%{public}d", trackId);
        controller->OnAdEos();
        return true;
    }
    if (controller->HasPendingEvents()) {
        MEDIA_LOG_I("SampleConsumerLoop: main EOS detected with pending ads, dropping, trackId=%{public}d", trackId);
        return true;
    }
    return false;
}

Status MediaDemuxer::PushDirectBuffer(int32_t trackId, std::shared_ptr<SampleQueue>& sampleQueue,
    std::shared_ptr<AVBuffer>& dstBuffer, sptr<AVBufferQueueProducer>& bufferQueue)
{
    auto status = sampleQueue->AcquireCopyToDstBuffer(dstBuffer);
    if (status == Status::OK) {
        bool isNeedDrop = HandleInterstitialEos(trackId, dstBuffer);
        if (isNeedDrop) {
            bufferQueue->PushBuffer(dstBuffer, false);
            return Status::OK;
        }
    }
    if (dstBuffer->meta_ != nullptr && IsSubtitleTrackId(trackId)) {
        dstBuffer->meta_->SetData(Tag::REGULAR_TRACK_ID, trackId);
    }
    status = HandlePushBuffer(trackId, dstBuffer, bufferQueue, status);
    CHECK_AND_RETURN_RET_LOG(status == Status::OK, status, "HandlePushBuffer failed");
    sampleQueueController_->ConsumeSpeed(trackId);
    return status;
}

Status MediaDemuxer::PushBufferBySlices(int32_t trackId, std::shared_ptr<SampleQueue>& sampleQueue,
    std::shared_ptr<AVBuffer>& dstBuffer, sptr<AVBufferQueueProducer>& bufferQueue)
{
    std::shared_ptr<AVBuffer> srcBuffer;
    auto status = sampleQueue->AcquireBuffer(srcBuffer);
    CHECK_AND_RETURN_RET_LOG(status == Status::OK && srcBuffer && srcBuffer->memory_,
        status, "AcquireSrcBuffer failed, trackId: %{public}d, status: %{public}d", trackId, status);

    status = CopyAndPushBufferBySlices(trackId, srcBuffer, dstBuffer);
    CHECK_AND_RETURN_RET_LOG(status == Status::OK, status,
        "CopySrcBufferByMinSize failed, trackId: %{public}d, status: %{public}d", trackId, status);
    return status;
}

void MediaDemuxer::ConsumeWaterLoopControl(int32_t trackId, std::shared_ptr<SampleQueue> sampleQueue)
{
    if (!sampleQueueController_ || trackId == subtitleTrackId_ || IsLocalFd() || eosMap_[trackId]) {
        return;
    }
    StreamType streamType = demuxerPluginManager_->GetStreamTypeByTrackID(trackId);
    if (!IsCloudFd() && !IsNeedPreDownload() &&
        (streamType == StreamType::VIDEO || streamType == StreamType::MIXED)) {
        if (sampleQueueController_->CheckWaterLineStopProduce(trackId, sampleQueueMap_[trackId])) {
            AutoSelectTrackByBitrate(trackId, true);
        } else if (sampleQueueController_->CheckWaterLineStartConsume(trackId, sampleQueueMap_[trackId])) {
            AutoSelectTrackByBitrate(trackId, false);
        } else {
            lastCheckAutoSelectTimeMs_ = 0;
        }
    }

    bool stopConsumeResult = false;
    {
        std::unique_lock<std::mutex> stopLock(stopMutex_);
        FALSE_RETURN(!isStopped_);
        AutoLock lock(mapMutex_);
        sampleQueueController_->ShouldStartProduce(trackId, sampleQueue, taskMap_[trackId]);
        stopConsumeResult =
            sampleQueueController_->ShouldStopConsume(trackId, sampleQueue, sampleConsumerTaskMap_[trackId]);
    }
    if (stopConsumeResult && !hlsSegmentEosMap_[trackId]) {
        if (trackId == videoTrackId_ && isVideoMuted_) {
            SetTrackIsBuffering(trackId, false);
            return;
        }
        SetTrackIsBuffering(trackId, true);
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
    FALSE_RETURN_V(sampleConsumerTask->second != nullptr, Status::ERROR_NULL_POINTER);
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

void MediaDemuxer::CachePressuredCallback(int32_t trackId, uint32_t cachedBytes)
{
    if (!GetEnableSampleQueueFlag()) {
        return;
    }
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    if (IsNeedMapToInnerTrackID()) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
    } else {
        int32_t streamID = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
    }
    if (pluginTemp == nullptr) {
        MEDIA_LOG_E("CachePressuredCallback pluginTemp nullptr");
        return;
    }
    uint32_t frameCount = 0;
    if (pluginTemp->GetCurrentCacheFrameCount(trackId, frameCount) != Status::OK) {
        MEDIA_LOG_E("CachePressuredCallback frameCount error");
        return;
    }
    if (NeedDroped(trackId) || hasDropedMap_[trackId].load()) {
        hasDropedMap_[trackId].store(true);
        if (!IsLocalFd()) {
            std::unique_lock<std::mutex> stopLock(stopMutex_);
            if (isStopped_) {
                return;
            }
            AutoLock lock(mapMutex_);
            if (!taskMap_[trackId]->IsTaskRunning()) {
                taskMap_[trackId]->Start();
            }
            SetTrackNeedDropFrame(trackId, frameCount);
            MEDIA_LOG_I("source need drop count: " PUBLIC_LOG_U32, frameCount);
        }
    } else {
        OnSampleQueueBufferAvailable(trackId);
    }
}

bool MediaDemuxer::NeedDroped(int32_t trackId)
{
    if (sampleQueueMap_[trackId] == nullptr) {
        return false;
    }
    if (IsLocalFd()) {
        if (sampleQueueMap_[trackId]->GetFilledBufferSize() >= SampleQueue::FD_SAMPLE_QUEUE_SIZE - 1) {
            hasDropedMap_[trackId].store(true);
            return true;
        }
    } else {
        if (sampleQueueMap_[trackId]->NewGetCacheDuration() >= SampleQueueController::STOP_PRODUCE_WATER_LOOP
            || sampleQueueMap_[trackId]->GetFilledBufferSize() >= SampleQueue::DEFAULT_SAMPLE_QUEUE_SIZE - 1) {
            hasDropedMap_[trackId].store(true);
            return true;
        }
    }
    return false;
}

void MediaDemuxer::AfterDrop(int32_t trackId)
{
    if (!GetEnableSampleQueueFlag()) {
        return;
    }
    if (!hasDropedMap_[trackId].load()) {
        return;
    }
    if (IsLocalFd()) {
        if (sampleQueueMap_[trackId] == nullptr) {
            return;
        }
        std::shared_ptr<AVBuffer> videoSample = AVBuffer::CreateAVBuffer();
        ReadSampleToDrop(videoTrackId_, videoSample);
        std::shared_ptr<AVBuffer> audioSample = AVBuffer::CreateAVBuffer();
        ReadSampleToDrop(audioTrackId_, audioSample);
        int64_t realSeekTime = 0;
        if (IsNeedMapToInnerTrackID()) {
            int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
            demuxerPluginManager_->SeekToFrameByDts(streamID, trackId, videoSample->dts_ / US_TO_MS,
                SeekMode::SEEK_CLOSEST, realSeekTime);
        } else {
            int32_t streamID = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
            demuxerPluginManager_->SeekToFrameByDts(streamID, trackId, videoSample->dts_ / US_TO_MS,
                SeekMode::SEEK_CLOSEST, realSeekTime);
        }
        afterDropDts_[videoTrackId_] = videoSample->dts_;
        afterDropDts_[audioTrackId_] = audioSample->dts_;
        MEDIA_LOG_I("afterDrop seekTo dts: " PUBLIC_LOG_D64 " realTime: "
            PUBLIC_LOG_D64 " videoDts: " PUBLIC_LOG_D64 "audioDts: " PUBLIC_LOG_D64,
            videoSample->dts_, realSeekTime, videoSample->dts_, audioSample->dts_);
        if (trackId == videoTrackId_) {
            SetTrackSeekNeedDrop(audioTrackId_, true);
        } else if (trackId == audioTrackId_) {
            SetTrackSeekNeedDrop(videoTrackId_, true);
        }
    } else {
        videoNeedIFrame_ = true;
    }
    hasDropedMap_[trackId].store(false);
}

Status MediaDemuxer::ReadSampleToDrop(int32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    std::shared_ptr<Plugins::DemuxerPlugin> pluginTemp = nullptr;
    int32_t innerTrackID = trackId;
    if (IsNeedMapToInnerTrackID()) {
        int32_t streamID = demuxerPluginManager_->GetTmpStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
        innerTrackID = demuxerPluginManager_->GetTmpInnerTrackIDByTrackID(trackId);
    } else {
        int32_t streamID = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
        pluginTemp = demuxerPluginManager_->GetPluginByStreamID(streamID);
    }
    if (pluginTemp == nullptr) {
        return Status::ERROR_UNKNOWN;
    }
    Status status = pluginTemp->ReadSampleZeroCopy(static_cast<uint32_t>(innerTrackID), sample, timeout_);
    return status;
}

bool MediaDemuxer::GetTrackIsBuffering(int32_t trackId)
{
    std::lock_guard<std::mutex> lock(bufferingMapMutex_);
    return isBufferingMap_[trackId];
}

void MediaDemuxer::SetTrackIsBuffering(int32_t trackId, bool isBuffering)
{
    std::lock_guard<std::mutex> lock(bufferingMapMutex_);
    isBufferingMap_[trackId] = isBuffering;
}

bool MediaDemuxer::IsBuffering()
{
    return isBuffering_.load();
}

void MediaDemuxer::SetIsTriggerAutoMode(bool isAutoSelect)
{
    MEDIA_LOG_I("SetIsTriggerAutoMode:" PUBLIC_LOG_S, isAutoSelect ? "true" : "false");
    isAutoSelect_.store(isAutoSelect);
}

Status MediaDemuxer::SetPlayStrategy(const std::shared_ptr<PlayStrategy> playStrategy)
{
    if (playStrategy == nullptr) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    defaultAudioLang_ = playStrategy->audioLanguage;
    defaultSubtitleLang_ = playStrategy->subtitleLanguage;
    isHdrStart_ = playStrategy->preferHDR;
    if (playStrategy->width > 0 && playStrategy->height > 0) {
        uint64_t resolution = static_cast<uint64_t>(playStrategy->width) * playStrategy->height;
        if (resolution > UINT32_MAX) {
            return Status::ERROR_INVALID_PARAMETER;
        } else {
            initResolution_ = static_cast<uint32_t>(resolution);
        }
    }
    return Status::OK;
}

static bool MatchPreferredMimeTypes(const std::vector<std::string> &preferredMimeTypes,
    const StreamInfo &streamInfo)
{
    // Match by explicit mimeType first; if empty, fall back to comparing codec prefix
    // (e.g. "avc1.64001f" should match preferred "avc1" or "avc1.64001f").
    if (!streamInfo.mimeType.empty() &&
        std::find(preferredMimeTypes.begin(), preferredMimeTypes.end(), streamInfo.mimeType) !=
        preferredMimeTypes.end()) {
        return true;
    }
    if (streamInfo.originCodecs.empty()) {
        return false;
    }

    const auto &codecs = streamInfo.originCodecs;
    for (const auto &mimeType : preferredMimeTypes) {
        if (mimeType.empty() || codecs.size() < mimeType.size()) {
            continue;
        }

        const size_t mimeTypeDotPos = mimeType.find('.');
        if (mimeTypeDotPos == std::string::npos) {
            if (codecs.compare(0, mimeType.size(), mimeType, 0, mimeType.size()) == 0 &&
                (codecs.size() == mimeType.size() || codecs[mimeType.size()] == '.')) {
                return true;
            }
            continue;
        }

        size_t codecsDotPos = codecs.find('.');
        if (codecsDotPos == std::string::npos) {
            codecsDotPos = codecs.size();
        }
        if (codecsDotPos != mimeTypeDotPos) {
            continue;
        }

        if (codecs.compare(0, mimeType.size(), mimeType, 0, mimeType.size()) == 0) {
            return true;
        }
    }
    return false;
}

void GetVideoFilterPredicatesExt(const TrackSelectionFilter &filter, std::vector<StreamFilterPredicate> &predicates)
{
    if (filter.minVideoFrameRate > 0) {
        predicates.emplace_back(
            [minFrameRate = filter.minVideoFrameRate](StreamInfo streamInfo) {
                return streamInfo.frameRate != 0 && streamInfo.frameRate >= static_cast<uint32_t>(minFrameRate);
            }
        );
    }
    if (filter.maxVideoResolution.first > 0 && filter.maxVideoResolution.second > 0) {
        predicates.emplace_back(
            [filter = filter](StreamInfo streamInfo) {
                return streamInfo.GetResolution() != 0 && filter.GetMaxVideoResolution() >= streamInfo.GetResolution();
            }
        );
    }
    if (filter.minVideoResolution.first > 0 && filter.minVideoResolution.second > 0) {
        predicates.emplace_back(
            [filter = filter](StreamInfo streamInfo) {
                return streamInfo.GetResolution() != 0 && filter.GetMinVideoResolution() <= streamInfo.GetResolution();
            }
        );
    }
    if (!filter.preferredVideoMimeTypes.empty()) {
        predicates.emplace_back(
            [preferredMimeTypes = filter.preferredVideoMimeTypes](StreamInfo streamInfo) {
                return MatchPreferredMimeTypes(preferredMimeTypes, streamInfo);
            }
        );
    }
}

std::vector<StreamFilterPredicate> GetVideoFilterPredicates(const TrackSelectionFilter &filter)
{
    std::vector<StreamFilterPredicate> predicates;
    if (filter.maxVideoBitrate > 0) {
        predicates.emplace_back(
            [maxBitrate = filter.maxVideoBitrate](const StreamInfo &streamInfo) {
                return streamInfo.bitRate != 0 && streamInfo.bitRate <= static_cast<uint32_t>(maxBitrate);
            }
        );
    }
    if (filter.minVideoBitrate > 0) {
        predicates.emplace_back(
            [minBitrate = filter.minVideoBitrate](const StreamInfo &streamInfo) {
                return streamInfo.bitRate != 0 && streamInfo.bitRate >= static_cast<uint32_t>(minBitrate);
            }
        );
    }
    if (filter.maxVideoFrameRate > 0) {
        predicates.emplace_back(
            [maxFrameRate = filter.maxVideoFrameRate](const StreamInfo &streamInfo) {
                return streamInfo.frameRate != 0 && streamInfo.frameRate <= static_cast<uint32_t>(maxFrameRate);
            }
        );
    }
    GetVideoFilterPredicatesExt(filter, predicates);
    MEDIA_LOG_I("GetVideoFilterPredicates predicates size: " PUBLIC_LOG_ZU, predicates.size());
    return predicates;
}

void GetAudioFilterPredicatesExt(const TrackSelectionFilter &filter, std::vector<StreamFilterPredicate> &predicates)
{
    if (!filter.preferredAudioMimeTypes.empty()) {
        predicates.emplace_back(
            [preferredMimeTypes = filter.preferredAudioMimeTypes](const StreamInfo &streamInfo) {
                return MatchPreferredMimeTypes(preferredMimeTypes, streamInfo);
            }
        );
    }
    if (!filter.preferredAudioLanguages.empty()) {
        predicates.emplace_back(
            [preferredLanguages = filter.preferredAudioLanguages](const StreamInfo &streamInfo) {
                return !streamInfo.lang.empty() && std::find(preferredLanguages.begin(), preferredLanguages.end(),
                    streamInfo.lang) != preferredLanguages.end();
            }
        );
    }
}

std::vector<StreamFilterPredicate> GetAudioFilterPredicates(const TrackSelectionFilter &filter)
{
    std::vector<StreamFilterPredicate> predicates;
    if (filter.maxAudioBitrate > 0) {
        predicates.emplace_back(
            [maxBitrate = filter.maxAudioBitrate](const StreamInfo &streamInfo) {
                return streamInfo.bitRate != 0 && streamInfo.bitRate <= static_cast<uint32_t>(maxBitrate);
            }
        );
    }
    if (filter.minAudioBitrate > 0) {
        predicates.emplace_back(
            [minBitrate = filter.minAudioBitrate](const StreamInfo &streamInfo) {
                return streamInfo.bitRate != 0 && streamInfo.bitRate >= static_cast<uint32_t>(minBitrate);
            }
        );
    }
    if (filter.maxAudioChannels > 0) {
        predicates.emplace_back(
            [maxChannels = filter.maxAudioChannels](const StreamInfo &streamInfo) {
                return streamInfo.channels != 0 && streamInfo.channels <= static_cast<uint32_t>(maxChannels);
            }
        );
    }
    GetAudioFilterPredicatesExt(filter, predicates);
    MEDIA_LOG_I("GetAudioFilterPredicates predicates size: " PUBLIC_LOG_ZU, predicates.size());
    return predicates;
}

std::vector<StreamFilterPredicate> GetSubtitleFilterPredicates(const TrackSelectionFilter &filter)
{
    std::vector<StreamFilterPredicate> predicates;
    if (!filter.preferredSubtitleLanguages.empty()) {
        predicates.emplace_back(
            [preferredLanguages = filter.preferredSubtitleLanguages](const StreamInfo &streamInfo) {
                return !streamInfo.lang.empty() && std::find(preferredLanguages.begin(), preferredLanguages.end(),
                    streamInfo.lang) != preferredLanguages.end();
            }
        );
    }
    MEDIA_LOG_I("GetSubtitleFilterPredicates predicates size: " PUBLIC_LOG_ZU, predicates.size());
    return predicates;
}

StreamInfo *FindStreamForFiledStream(std::vector<StreamInfo> &streams, int32_t lastStreamId,
    int32_t defaultStreamId, bool &needChange)
{
    if (streams.empty()) {
        return nullptr;
    }
    StreamInfo *stream = nullptr;
    for (auto &item : streams) {
        if (item.streamId == lastStreamId) {
            needChange = false;
            break;
        }

        if (item.streamId == defaultStreamId) {
            stream = &item;
        }
    }
    return stream;
}

void MediaDemuxer::SetTrackSelectionFilter(const TrackSelectionFilter &filter)
{
    trackSelectionFilter_ = filter;
    FALSE_RETURN_MSG_W(isPrepared_.load(), "not prepared, cannot auto select bitrate");
    FALSE_RETURN_MSG_W(!IsCloudFd() && !IsNeedPreDownload(), "cloud fd or pre-download, cannot auto select bitrate");
    auto lastVideoStreamId = defaultVideoStreamId_;
    auto lastAudioStreamId = defaultAudioStreamId_;
    auto lastSubtitleStreamId = defaultSubtitleStreamId_;
    ChooseDefaultStreams();
    StreamInfo *stream = nullptr;
    bool needChange = true;
    stream = FindStreamForFiledStream(
        filteredStreamMap_->map[VIDEO], lastVideoStreamId, defaultVideoStreamId_, needChange);
    if (needChange && stream != nullptr) {
        Status states = SelectBitRate(stream->bitRate, true, true);
        if (states == Status::OK) {
            lastAutoSelectBitRate_ = stream->bitRate;
        }
        PrintStreamInfo("SetTrackSelectionFilter ", stream, currentPlayType_, states, curDownloadBitRate_.load());
    } else {
        defaultVideoStreamId_ = lastVideoStreamId;
        MEDIA_LOG_I("video stream does not need to be switched");
    }
    FALSE_RETURN(currentPlayType_ != "http");
    stream = nullptr;
    needChange = true;
    stream = FindStreamForFiledStream(
        filteredStreamMap_->map[AUDIO], lastAudioStreamId, defaultAudioStreamId_, needChange);
    if (needChange && stream != nullptr) {
        Status states = SelectStreamId(defaultAudioStreamId_);
        PrintStreamInfo("SetTrackSelectionFilter ", stream, currentPlayType_, states);
    } else {
        defaultAudioStreamId_ = lastAudioStreamId;
        MEDIA_LOG_I("audio stream does not need to be switched");
    }
    stream = nullptr;
    needChange = true;
    stream = FindStreamForFiledStream(filteredStreamMap_->map[SUBTITLE], lastSubtitleStreamId,
        defaultSubtitleStreamId_, needChange);
    if (needChange && stream != nullptr) {
        Status states = SelectStreamId(defaultSubtitleStreamId_, false);
        PrintStreamInfo("SetTrackSelectionFilter ", stream, currentPlayType_, states);
        if (states == Status::OK && isHls_ && syncCenter_ != nullptr) {
            HandleSelectSubtitleStream(
                syncCenter_->GetMediaTimeNow(), SeekMode::SEEK_PREVIOUS_SYNC, defaultSubtitleStreamId_);
        }
    } else {
        defaultSubtitleStreamId_ = lastSubtitleStreamId;
        MEDIA_LOG_I("subtitle stream does not need to be switched");
    }
}

bool IsLangMatch(const std::string& defaultLang, const std::string& lang)
{
    if (defaultLang.length() > 0) {
        return defaultLang.compare(lang) == 0;
    }
    return true;
}

uint32_t CalcResolutionGap(uint32_t baseResolution, uint32_t resolution)
{
    return (baseResolution > resolution) ? (baseResolution - resolution) : (resolution - baseResolution);
}

bool IsVideoStreamSelectable(const StreamInfo &stream, const std::string &currentPlayType, bool isHdrStart)
{
    if (currentPlayType == "dash" && (stream.videoType != VIDEO_TYPE_SDR) != isHdrStart) {
        return false;
    }
    if (currentPlayType == "hls" && stream.isSimple) {
        return true;
    }
    if (currentPlayType == "hls" && stream.bitRate > BAND_WIDTH_LIMIT) {
        return false;
    }
    return true;
}

bool IsStreamMatched(const StreamInfo &stream,
    std::map<StreamType, std::vector<StreamFilterPredicate>> &filteredStreamPredicates)
{
    auto type = stream.type;
    if (type == MIXED) {
        type = VIDEO;
    }
    if (filteredStreamPredicates[type].empty()) {
        return false;
    }
    for (const auto &predicate : filteredStreamPredicates[type]) {
        if (!predicate(stream)) {
            return false;
        }
    }
    return true;
}

void ScanStreamsAndBuildMap(const std::vector<StreamInfo> &streams,
    std::map<StreamType, std::vector<StreamFilterPredicate>> &filteredStreamPredicates,
    std::map<StreamType, bool> &needFilterMap,
    std::map<StreamType, std::vector<StreamInfo>> &filteredStreamMap,
    std::map<StreamType, std::vector<StreamInfo>> &allStreamMap)
{
    for (const auto &stream : streams) {
        auto type = stream.type;
        if (type == MIXED) {
            type = VIDEO;
        }
        allStreamMap[type].push_back(stream);

        if (needFilterMap[type] && IsStreamMatched(stream, filteredStreamPredicates)) {
            MEDIA_LOG_D("ScanStreamsAndBuildMap stream matched, type: " PUBLIC_LOG_D32 ", streamId: " PUBLIC_LOG_D32,
                static_cast<int32_t>(type), stream.streamId);
            filteredStreamMap[type].push_back(stream);
        } else {
            MEDIA_LOG_D("ScanStreamsAndBuildMap stream not matched, type: " PUBLIC_LOG_D32 ", streamId: "
                PUBLIC_LOG_D32, static_cast<int32_t>(type), stream.streamId);
        }
    }
}

int32_t SelectDefaultVideoId(const std::vector<StreamInfo> &videoStreams, bool isAllStreamSelected,
    TrackSelectionFilter &trackSelectionFilter, const std::string &currentPlayType, bool isHdrStart,
    uint32_t initResolution)
{
    if ((currentPlayType == "http" || currentPlayType == "dash") && isAllStreamSelected) {
        return -1;
    }
    if (!isAllStreamSelected && !trackSelectionFilter.preferredVideoMimeTypes.empty()) {
        const auto &preferredMimeTypes = trackSelectionFilter.preferredVideoMimeTypes;
        for (const auto &mime : preferredMimeTypes) {
            auto it = std::find_if(videoStreams.begin(), videoStreams.end(),
                [&mime](const StreamInfo &stream) {
                    return stream.mimeType == mime;
                });
            if (it != videoStreams.end()) {
                return it->streamId;
            }
        }
    }

    int32_t videoStreamId = -1;
    uint32_t maxResolutionGap = 0;
    bool isFirstSelect = true;
    for (auto stream : videoStreams) {
        if (!IsVideoStreamSelectable(stream, currentPlayType, isHdrStart)) {
            continue;
        }

        if (initResolution == 0) {
            videoStreamId = stream.streamId;
            continue;
        }
        uint32_t resolutionGap = CalcResolutionGap(initResolution, stream.GetResolution());
        if (isFirstSelect || resolutionGap < maxResolutionGap) {
            maxResolutionGap = resolutionGap;
            videoStreamId = stream.streamId;
            isFirstSelect = false;
        }
    }

    if (videoStreamId == -1 && !videoStreams.empty()) {
        videoStreamId = videoStreams[0].streamId;
    }
    return videoStreamId;
}

int32_t SelectAudioDefaultId(std::map<StreamType, std::vector<StreamInfo>> &filteredStreamMap,
    bool isAllStreamSelected, TrackSelectionFilter &trackSelectionFilter, const std::string &defaultLang,
    const std::string &currentPlayType)
{
    if ((currentPlayType == "hls" || currentPlayType == "dash") && isAllStreamSelected) {
        return -1;
    }
    auto &audioStreams = filteredStreamMap[AUDIO];
    if (audioStreams.empty()) {
        return -1;
    }

    if (!isAllStreamSelected && !trackSelectionFilter.preferredAudioLanguages.empty()) {
        const auto &preferredLangs = trackSelectionFilter.preferredAudioLanguages;
        for (const auto &lang : preferredLangs) {
            auto it = std::find_if(audioStreams.begin(), audioStreams.end(),
                [&lang](const StreamInfo &stream) {
                    return stream.lang == lang;
                });
            if (it != audioStreams.end()) {
                return it->streamId;
            }
        }
    }

    if (!isAllStreamSelected && !trackSelectionFilter.preferredAudioMimeTypes.empty()) {
        const auto &preferredMimeTypes = trackSelectionFilter.preferredAudioMimeTypes;
        for (const auto &mime : preferredMimeTypes) {
            auto it = std::find_if(audioStreams.begin(), audioStreams.end(),
                [&mime](const StreamInfo &stream) {
                    return stream.mimeType == mime;
                });
            if (it != audioStreams.end()) {
                return it->streamId;
            }
        }
    }

    int32_t streamId = -1;
    for (const auto &stream : audioStreams) {
        if (IsLangMatch(defaultLang, stream.lang)) {
            streamId = stream.streamId;
        }
    }
    if (streamId == -1) {
        return audioStreams.front().streamId;
    }
    return streamId;
}

int32_t SelectSubtitleDefaultId(std::map<StreamType, std::vector<StreamInfo>> &filteredStreamMap,
    bool isAllStreamSelected, TrackSelectionFilter &trackSelectionFilter, const std::string &defaultLang,
    const std::string &currentPlayType)
{
    if ((currentPlayType == "hls" || currentPlayType == "dash") && isAllStreamSelected) {
        return -1;
    }
    auto &subtitleStreams = filteredStreamMap[SUBTITLE];
    if (subtitleStreams.empty()) {
        return -1;
    }

    if (!isAllStreamSelected && !trackSelectionFilter.preferredSubtitleLanguages.empty()) {
        const auto &preferredLangs = trackSelectionFilter.preferredSubtitleLanguages;
        auto it = std::find_if(subtitleStreams.begin(), subtitleStreams.end(),
            [&preferredLangs](const StreamInfo &stream) {
                return std::find(preferredLangs.begin(), preferredLangs.end(), stream.lang) != preferredLangs.end();
            });
        if (it != subtitleStreams.end()) {
            return it->streamId;
        }
    }

    int32_t streamId = -1;
    for (const auto &stream : subtitleStreams) {
        if (IsLangMatch(defaultLang, stream.lang)) {
            streamId = stream.streamId;
        }
    }
    if (streamId == -1) {
        return subtitleStreams.front().streamId;
    }
    return streamId;
}

std::vector<StreamInfo> CollectValidStreams(std::vector<StreamInfo> &allStreams, const std::string &currentPlayType,
    const TrackSelectionFilter &trackSelectionFilter, std::shared_ptr<MediaAVCodec::AVCodecList> codecList)
{
    std::vector<StreamInfo> filteredStreams;
    bool hasVideoValid = false;
    bool hasAudioValid = false;
    std::vector<StreamInfo> videoStreams;
    std::vector<StreamInfo> audioStreams;

    for (auto &streamInfo : allStreams) {
        if (streamInfo.type == VIDEO || streamInfo.type == AUDIO) {
            streamInfo.type == VIDEO ? videoStreams.push_back(streamInfo) : audioStreams.push_back(streamInfo);

            std::vector<std::string> preferredMimeTypes = streamInfo.type == VIDEO ?
                trackSelectionFilter.preferredVideoMimeTypes : trackSelectionFilter.preferredAudioMimeTypes;
            if (!preferredMimeTypes.empty() && MatchPreferredMimeTypes(preferredMimeTypes, streamInfo)) {
                streamInfo.type == VIDEO ? hasVideoValid = true : hasAudioValid = true;
                filteredStreams.push_back(streamInfo);
                continue;
            }

            if (streamInfo.type == VIDEO && streamInfo.mimeType.empty() && currentPlayType == "hls") {
                continue;
            }

            if (CheckStreamCodec(streamInfo.codecs, streamInfo.type, codecList)) {
                streamInfo.type == VIDEO ? hasVideoValid = true : hasAudioValid = true;
                filteredStreams.push_back(streamInfo);
            }
        } else {
            filteredStreams.push_back(streamInfo);
        }
    }
    if (!hasVideoValid) {
        filteredStreams.insert(filteredStreams.end(), videoStreams.begin(), videoStreams.end());
    }
    if (!hasAudioValid) {
        filteredStreams.insert(filteredStreams.end(), audioStreams.begin(), audioStreams.end());
    }
    return filteredStreams;
}

void GetNeedFilterMap(std::map<StreamType, std::vector<StreamFilterPredicate>> &filteredStreamPredicates,
    const TrackSelectionFilter &trackSelectionFilter, const std::string &currentPlayType,
    std::map<StreamType, bool> &needFilterMap)
{
    if (currentPlayType == "http" && (!filteredStreamPredicates[AUDIO].empty() ||
        !filteredStreamPredicates[SUBTITLE].empty() || !trackSelectionFilter.preferredVideoMimeTypes.empty() ||
        trackSelectionFilter.maxVideoFrameRate > 0 || trackSelectionFilter.minVideoFrameRate > 0)) {
        needFilterMap[VIDEO] = false;
    } else {
        needFilterMap[VIDEO] = true;
    }

    if (currentPlayType == "http") {
        needFilterMap[AUDIO] = false;
    } else {
        needFilterMap[AUDIO] = true;
    }

    if (currentPlayType == "http") {
        needFilterMap[SUBTITLE] = false;
    } else {
        needFilterMap[SUBTITLE] = true;
    }
    MEDIA_LOG_I("GetNeedFilterMap currentPlayType: " PUBLIC_LOG_S " needFilterVideo: " PUBLIC_LOG_D32
        " needFilterAudio: " PUBLIC_LOG_D32 " needFilterSubtitle: " PUBLIC_LOG_D32, currentPlayType.c_str(),
        needFilterMap[VIDEO], needFilterMap[AUDIO], needFilterMap[SUBTITLE]);
}

void MediaDemuxer::ChooseDefaultStreams()
{
    MEDIA_LOG_I("ChooseDefaultStreams In");
    std::map<StreamType, std::vector<StreamFilterPredicate>> filteredStreamPredicates;
    filteredStreamPredicates[VIDEO] = GetVideoFilterPredicates(trackSelectionFilter_);
    filteredStreamPredicates[AUDIO] = GetAudioFilterPredicates(trackSelectionFilter_);
    filteredStreamPredicates[SUBTITLE] = GetSubtitleFilterPredicates(trackSelectionFilter_);

    std::vector<StreamInfo> streams;
    source_->GetStreamInfo(streams, true);
    streams = CollectValidStreams(streams, currentPlayType_, trackSelectionFilter_, codecList_);

    std::map<StreamType, std::vector<StreamInfo>> filteredStreamMap;
    std::map<StreamType, std::vector<StreamInfo>> allStreamMap;
    std::map<StreamType, bool> needFilterMap;
    GetNeedFilterMap(filteredStreamPredicates, trackSelectionFilter_, currentPlayType_, needFilterMap);
    ScanStreamsAndBuildMap(streams, filteredStreamPredicates, needFilterMap, filteredStreamMap, allStreamMap);
    bool isVideoNoFiltered =
        filteredStreamMap[VIDEO].empty() || filteredStreamMap[VIDEO].size() == allStreamMap[VIDEO].size();
    bool isAudioNoFiltered =
        filteredStreamMap[AUDIO].empty() || filteredStreamMap[AUDIO].size() == allStreamMap[AUDIO].size();
    bool isSubtitleNoFiltered =
        filteredStreamMap[SUBTITLE].empty() || filteredStreamMap[SUBTITLE].size() == allStreamMap[SUBTITLE].size();
    MEDIA_LOG_I("ChooseDefaultStreams hasFilteredVideo: " PUBLIC_LOG_D32 " hasFilteredAudio: " PUBLIC_LOG_D32
        " hasFilteredSubtitle: " PUBLIC_LOG_D32, !isVideoNoFiltered, !isAudioNoFiltered, !isSubtitleNoFiltered);
    {
        std::lock_guard<std::recursive_mutex> lock(streamMapMutex_);
        filteredStreamMap_->map.clear();
        filteredStreamMap_->map[VIDEO] =
            isVideoNoFiltered ? std::move(allStreamMap[VIDEO]) : std::move(filteredStreamMap[VIDEO]);
        filteredStreamMap_->map[AUDIO] =
            isAudioNoFiltered ? std::move(allStreamMap[AUDIO]) : std::move(filteredStreamMap[AUDIO]);
        filteredStreamMap_->map[SUBTITLE] =
            isSubtitleNoFiltered ? std::move(allStreamMap[SUBTITLE]) : std::move(filteredStreamMap[SUBTITLE]);
        MEDIA_LOG_I("ChooseDefaultStreams filtered video stream count: " PUBLIC_LOG_ZU
            " audio stream count: " PUBLIC_LOG_ZU " subtitle stream count: " PUBLIC_LOG_ZU,
            filteredStreamMap_->map[VIDEO].size(), filteredStreamMap_->map[AUDIO].size(),
            filteredStreamMap_->map[SUBTITLE].size());
        UpdateDefaultStreamIds(isVideoNoFiltered, isAudioNoFiltered, isSubtitleNoFiltered);
    }
}

void MediaDemuxer::UpdateDefaultStreamIds(bool isVideoNoFiltered, bool isAudioNoFiltered, bool isSubtitleNoFiltered)
{
    auto id = SelectDefaultVideoId(filteredStreamMap_->map[VIDEO], isVideoNoFiltered, trackSelectionFilter_,
        currentPlayType_, isHdrStart_, initResolution_);
    if (id != -1) {
        defaultVideoStreamId_ = id;
    }
    if (currentPlayType_ == "http") {
        return;
    }
    id = SelectAudioDefaultId(
        filteredStreamMap_->map, isAudioNoFiltered, trackSelectionFilter_, defaultAudioLang_, currentPlayType_);
    if (id != -1) {
        defaultAudioStreamId_ = id;
    }

    id = SelectSubtitleDefaultId(
        filteredStreamMap_->map, isSubtitleNoFiltered, trackSelectionFilter_, defaultSubtitleLang_, currentPlayType_);
    if (id != -1) {
        defaultSubtitleStreamId_ = id;
    }

    MEDIA_LOG_I("ChooseDefaultStreams defaultVideoStreamId: " PUBLIC_LOG_D32 " defaultAudioStreamId: " PUBLIC_LOG_D32
        " defaultSubtitleStreamId: " PUBLIC_LOG_D32, defaultVideoStreamId_, defaultAudioStreamId_,
        defaultSubtitleStreamId_);
}

uint32_t MediaDemuxer::GetTrackNeedDropFrame(int32_t trackId)
{
    std::lock_guard<std::mutex> lock(frameCountNeedDropMutex_);
    return frameCountNeedDrop_[trackId];
}

void MediaDemuxer::SetTrackNeedDropFrame(int32_t trackId, uint32_t frameCount)
{
    std::lock_guard<std::mutex> lock(frameCountNeedDropMutex_);
    frameCountNeedDrop_[trackId] = frameCount;
}

bool MediaDemuxer::GetTrackSeekNeedDrop(int32_t trackId)
{
    std::lock_guard<std::mutex> lock(afterSeekNeedDropMutex_);
    return afterSeekNeedDrop_[trackId];
}

void MediaDemuxer::SetTrackSeekNeedDrop(int32_t trackId, bool needDrop)
{
    std::lock_guard<std::mutex> lock(afterSeekNeedDropMutex_);
    afterSeekNeedDrop_[trackId] = needDrop;
}

void MediaDemuxer::UpdateTrackMap()
{
    FALSE_RETURN_NOLOG(demuxerPluginManager_ != nullptr);
    FALSE_RETURN_NOLOG(IsValidTrackId(videoTrackId_) && IsValidTrackId(audioTrackId_));

    // Update track map in track type change case
    int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(videoTrackId_);
    if (demuxerPluginManager_->GetTmpTrackTypeByTrackID(videoTrackId_) != TRACK_VIDEO) {
        demuxerPluginManager_->UpdateTempTrackMapByStreamId(videoTrackId_, streamId, TRACK_VIDEO);
        InnerSelectTrack(videoTrackId_);
    }
    if (demuxerPluginManager_->GetTmpTrackTypeByTrackID(audioTrackId_) != TRACK_AUDIO) {
        demuxerPluginManager_->UpdateTempTrackMapByStreamId(audioTrackId_, streamId, TRACK_AUDIO);
        InnerSelectTrack(audioTrackId_);
    }
}

void MediaDemuxer::SetHiplayerCallback(const std::shared_ptr<Pipeline::FilterCallback> &callback)
{
    hiplayerCallback_ = callback;
}

bool MediaDemuxer::IsCloudFd()
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, false, "source_ is nullptr");
    return source_->IsCloudFd();
}

bool MediaDemuxer::IsNeedPreDownload() const
{
    FALSE_RETURN_V_MSG_E(source_ != nullptr, false, "source_ is nullptr");
    return source_->IsNeedPreDownload();
}
 
bool MediaDemuxer::IsSubtitleTrackId(int32_t trackId)
{
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, false, "demuxerPluginManger_ is nullptr");
    TrackType trackType = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    return trackType == TrackType::TRACK_SUBTITLE;
}

std::deque<MediaDemuxer::MediaChangeRecord>::iterator MediaDemuxer::GetLatestUnreportedRecord()
{
    for (auto it = mediaChangeHistory_.begin(); it != mediaChangeHistory_.end(); ++it) {
        if (!it->hasReported) {
            return it;
        }
    }
    return mediaChangeHistory_.end();
}

void MediaDemuxer::FillChangeInfoFromRecord(MediaChangeInfo& changeInfo,
    const MediaChangeRecord& record, bool isFailed)
{
    changeInfo.isLocalFd = record.isLocalFd;
    changeInfo.streamType = record.streamType;
    changeInfo.changeReason = static_cast<int32_t>(record.changeReason);
    changeInfo.changeResult = isFailed ? MEDIA_CHANGE_FAILED : MEDIA_CHANGE_SUCCESS;
    FillChangeInfo(changeInfo, record, isFailed, record.isLocalFd);
    FillChangeInfoCommon(changeInfo, record, isFailed);
}

void MediaDemuxer::FillChangeInfo(MediaChangeInfo& changeInfo,
    const MediaChangeRecord& record, bool isFailed, bool isLocalFd)
{
    if (isLocalFd) {
        changeInfo.streamIdBefore = -1;
        changeInfo.streamIdAfter = -1;
        changeInfo.bitrateBefore = 0;
        changeInfo.bitrateAfter = 0;
        changeInfo.originCodecsBefore = "";
        changeInfo.originCodecsAfter = "";
        return;
    }
    changeInfo.streamIdBefore = record.streamIdBefore;
    changeInfo.streamIdAfter = record.streamIdAfter;
    changeInfo.bitrateBefore = record.bitrateBefore;
    changeInfo.bitrateAfter = isFailed ? 0 : record.bitrateAfter;
    changeInfo.originCodecsBefore = record.originCodecsBefore;
    changeInfo.originCodecsAfter = isFailed ? "" : record.originCodecsAfter;
}

void MediaDemuxer::FillChangeInfoCommon(MediaChangeInfo& changeInfo,
    const MediaChangeRecord& record, bool isFailed)
{
    changeInfo.videoWidthBefore = record.videoWidthBefore;
    changeInfo.videoHeightBefore = record.videoHeightBefore;
    changeInfo.videoFrameRateBefore = record.videoFrameRateBefore;
    changeInfo.audioChannelsBefore = record.audioChannelsBefore;
    changeInfo.audioSampleRateBefore = record.audioSampleRateBefore;
    changeInfo.audioLangBefore = record.audioLangBefore;
    changeInfo.subtitleLangBefore = record.subtitleLangBefore;
    changeInfo.audioMimeTypeBefore = record.audioMimeTypeBefore;
    changeInfo.videoMimeTypeBefore = record.videoMimeTypeBefore;
    changeInfo.videoTypeBefore = record.videoTypeBefore;
    changeInfo.codecsBefore = record.codecsBefore;
    
    changeInfo.videoWidthAfter = isFailed ? 0 : record.videoWidthAfter;
    changeInfo.videoHeightAfter = isFailed ? 0 : record.videoHeightAfter;
    changeInfo.videoFrameRateAfter = isFailed ? 0 : record.videoFrameRateAfter;
    changeInfo.audioChannelsAfter = isFailed ? 0 : record.audioChannelsAfter;
    changeInfo.audioSampleRateAfter = isFailed ? 0 : record.audioSampleRateAfter;
    changeInfo.audioLangAfter = isFailed ? "" : record.audioLangAfter;
    changeInfo.subtitleLangAfter = isFailed ? "" : record.subtitleLangAfter;
    changeInfo.audioMimeTypeAfter = isFailed ? "" : record.audioMimeTypeAfter;
    changeInfo.videoMimeTypeAfter = isFailed ? "" : record.videoMimeTypeAfter;
    changeInfo.videoTypeAfter = isFailed ? 0 : record.videoTypeAfter;
    changeInfo.codecsAfter = isFailed ? "" : record.codecsAfter;
}

void MediaDemuxer::FillVideoParamsFromVideoTrack(MediaChangeRecord& record,
    int32_t videoTrackIdBefore, int32_t videoTrackIdAfter)
{
    if (videoTrackIdBefore >= 0 && static_cast<size_t>(videoTrackIdBefore) < mediaMetaData_.trackMetas.size()) {
        auto videoTrackMeta = mediaMetaData_.trackMetas[videoTrackIdBefore];
        if (videoTrackMeta != nullptr) {
            int32_t width = 0;
            int32_t height = 0;
            double frameRate = 0.0;
            int32_t videoType = 0;
            std::string mimeType;
            videoTrackMeta->Get<Tag::VIDEO_WIDTH>(width);
            videoTrackMeta->Get<Tag::VIDEO_HEIGHT>(height);
            videoTrackMeta->Get<Tag::VIDEO_FRAME_RATE>(frameRate);
            videoTrackMeta->Get<Tag::VIDEO_TYPE>(videoType);
            videoTrackMeta->Get<Tag::MIME_TYPE>(mimeType);
            record.videoWidthBefore = width;
            record.videoHeightBefore = height;
            record.videoFrameRateBefore = static_cast<uint32_t>(frameRate);
            record.videoTypeBefore = videoType;
            record.videoMimeTypeBefore = mimeType;
        }
    }

    if (videoTrackIdAfter >= 0 && static_cast<size_t>(videoTrackIdAfter) < mediaMetaData_.trackMetas.size()) {
        auto videoTrackMeta = mediaMetaData_.trackMetas[videoTrackIdAfter];
        if (videoTrackMeta != nullptr) {
            int32_t width = 0;
            int32_t height = 0;
            double frameRate = 0.0;
            int32_t videoType = 0;
            std::string mimeType;
            videoTrackMeta->Get<Tag::VIDEO_WIDTH>(width);
            videoTrackMeta->Get<Tag::VIDEO_HEIGHT>(height);
            videoTrackMeta->Get<Tag::VIDEO_FRAME_RATE>(frameRate);
            videoTrackMeta->Get<Tag::VIDEO_TYPE>(videoType);
            videoTrackMeta->Get<Tag::MIME_TYPE>(mimeType);
            
            record.videoWidthAfter = width;
            record.videoHeightAfter = height;
            record.videoFrameRateAfter = static_cast<uint32_t>(frameRate);
            record.videoTypeAfter = videoType;
            record.videoMimeTypeAfter = mimeType;
        }
    }
}

void MediaDemuxer::FillSubtitleParamsFromSubtitleTrack(MediaChangeRecord& record,
    int32_t subtitleTrackIdBefore, int32_t subtitleTrackIdAfter)
{
    if (subtitleTrackIdBefore >= 0 && static_cast<size_t>(subtitleTrackIdBefore) < mediaMetaData_.trackMetas.size()) {
        auto subtitleTrackMeta = mediaMetaData_.trackMetas[subtitleTrackIdBefore];
        if (subtitleTrackMeta != nullptr) {
            std::string lang;
            subtitleTrackMeta->Get<Tag::MEDIA_LANGUAGE>(lang);
            record.subtitleLangBefore = lang;
        }
    }

    if (subtitleTrackIdAfter >= 0 && static_cast<size_t>(subtitleTrackIdAfter) < mediaMetaData_.trackMetas.size()) {
        auto subtitleTrackMeta = mediaMetaData_.trackMetas[subtitleTrackIdAfter];
        if (subtitleTrackMeta != nullptr) {
            std::string lang;
            subtitleTrackMeta->Get<Tag::MEDIA_LANGUAGE>(lang);
            record.subtitleLangAfter = lang;
        }
    }
}

void MediaDemuxer::FillAudioParamsFromAudioTrack(MediaChangeRecord& record,
    int32_t audioTrackIdBefore, int32_t audioTrackIdAfter)
{
    if (audioTrackIdBefore >= 0 && static_cast<size_t>(audioTrackIdBefore) < mediaMetaData_.trackMetas.size()) {
        auto audioTrackMeta = mediaMetaData_.trackMetas[audioTrackIdBefore];
        if (audioTrackMeta != nullptr) {
            int32_t channels = 0;
            int32_t sampleRate = 0;
            Plugins::AudioSampleFormat sampleFormat = Plugins::AudioSampleFormat::SAMPLE_F32LE;
            std::string mimeType;
            std::string lang;
            audioTrackMeta->Get<Tag::AUDIO_CHANNEL_COUNT>(channels);
            audioTrackMeta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
            audioTrackMeta->Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
            audioTrackMeta->Get<Tag::MIME_TYPE>(mimeType);
            audioTrackMeta->Get<Tag::MEDIA_LANGUAGE>(lang);
            record.audioChannelsBefore = static_cast<uint32_t>(channels);
            record.audioSampleRateBefore = static_cast<uint32_t>(sampleRate);
            record.audioMimeTypeBefore = mimeType;
            record.audioLangBefore = lang;
        }
    }
    if (audioTrackIdAfter >= 0 && static_cast<size_t>(audioTrackIdAfter) < mediaMetaData_.trackMetas.size()) {
        auto audioTrackMeta = mediaMetaData_.trackMetas[audioTrackIdAfter];
        if (audioTrackMeta != nullptr) {
            int32_t channels = 0;
            int32_t sampleRate = 0;
            Plugins::AudioSampleFormat sampleFormat = Plugins::AudioSampleFormat::SAMPLE_F32LE;
            std::string mimeType;
            std::string lang;
            audioTrackMeta->Get<Tag::AUDIO_CHANNEL_COUNT>(channels);
            audioTrackMeta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
            audioTrackMeta->Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
            audioTrackMeta->Get<Tag::MIME_TYPE>(mimeType);
            audioTrackMeta->Get<Tag::MEDIA_LANGUAGE>(lang);
            record.audioChannelsAfter = static_cast<uint32_t>(channels);
            record.audioSampleRateAfter = static_cast<uint32_t>(sampleRate);
            record.audioMimeTypeAfter = mimeType;
            record.audioLangAfter = lang;
        }
    }
}

void MediaDemuxer::FillVideoParamsFromVideoStream(MediaChangeRecord& record)
{
    if (videoTrackId_ < 0 || static_cast<size_t>(videoTrackId_) >= mediaMetaData_.trackMetas.size()) {
        return;
    }
    auto meta = mediaMetaData_.trackMetas[videoTrackId_];
    if (meta == nullptr) {
        return;
    }
    int32_t width = 0;
    int32_t height = 0;
    int32_t videoType = 0;
    double frameRate = 0.0;
    std::string mimeType;
    meta->Get<Tag::VIDEO_WIDTH>(width);
    meta->Get<Tag::VIDEO_HEIGHT>(height);
    meta->Get<Tag::VIDEO_FRAME_RATE>(frameRate);
    meta->Get<Tag::VIDEO_TYPE>(videoType);
    meta->Get<Tag::MIME_TYPE>(mimeType);
    FillIfEmpty(record.videoWidthAfter, width);
    FillIfEmpty(record.videoHeightAfter, height);
    FillIfEmpty(record.videoFrameRateAfter, static_cast<uint32_t>(frameRate));
    FillIfEmpty(record.videoTypeAfter, videoType);
    FillIfEmpty(record.videoMimeTypeAfter, mimeType);
    FillIfEmpty(record.videoWidthBefore, width);
    FillIfEmpty(record.videoHeightBefore, height);
    FillIfEmpty(record.videoFrameRateBefore, static_cast<uint32_t>(frameRate));
    FillIfEmpty(record.videoTypeBefore, videoType);
    FillIfEmpty(record.videoMimeTypeBefore, mimeType);
}

void MediaDemuxer::FillAudioParamsFromAudioStream(MediaChangeRecord& record)
{
    if (audioTrackId_ < 0 || static_cast<size_t>(audioTrackId_) >= mediaMetaData_.trackMetas.size()) {
        return;
    }
    auto meta = mediaMetaData_.trackMetas[audioTrackId_];
    if (meta == nullptr) {
        return;
    }
    int32_t sampleRate = 0;
    int32_t channels = 0;
    std::string mimeType;
    std::string lang;
    meta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    meta->Get<Tag::AUDIO_CHANNEL_COUNT>(channels);
    meta->Get<Tag::MIME_TYPE>(mimeType);
    meta->Get<Tag::MEDIA_LANGUAGE>(lang);
    FillIfEmpty(record.audioSampleRateAfter, static_cast<uint32_t>(sampleRate));
    FillIfEmpty(record.audioChannelsAfter, static_cast<uint32_t>(channels));
    FillIfEmpty(record.audioMimeTypeAfter, mimeType);
    FillIfEmpty(record.audioLangAfter, lang);
    FillIfEmpty(record.audioSampleRateBefore, static_cast<uint32_t>(sampleRate));
    FillIfEmpty(record.audioChannelsBefore, static_cast<uint32_t>(channels));
    FillIfEmpty(record.audioMimeTypeBefore, mimeType);
    FillIfEmpty(record.audioLangBefore, lang);
}

void MediaDemuxer::FillSubtitleParamsFromSubtitleStream(MediaChangeRecord& record)
{
    if (subtitleTrackId_ < 0 || static_cast<size_t>(subtitleTrackId_) >= mediaMetaData_.trackMetas.size()) {
        return;
    }
    auto meta = mediaMetaData_.trackMetas[subtitleTrackId_];
    if (meta == nullptr) {
        return;
    }
    std::string lang;
    meta->Get<Tag::MEDIA_LANGUAGE>(lang);
    FillIfEmpty(record.subtitleLangAfter, lang);
    FillIfEmpty(record.subtitleLangBefore, lang);
}

void MediaDemuxer::FillRecordFromStream(MediaChangeRecord& record, int32_t streamIdBefore, int32_t streamIdAfter)
{
    FALSE_RETURN_MSG_W(source_ != nullptr, "source_ is nullptr");
    std::vector<Plugins::StreamInfo> streams;
    source_->GetStreamInfo(streams);
    std::optional<Plugins::StreamInfo> streamInfoBefore;
    std::optional<Plugins::StreamInfo> streamInfoAfter;
    for (const auto& stream : streams) {
        if (stream.streamId == streamIdBefore) {
            streamInfoBefore = stream;
        } else if (stream.streamId == streamIdAfter) {
            streamInfoAfter = stream;
        } else {
            // other stream
        }
    }
    FALSE_RETURN_MSG_W(streamInfoBefore.has_value() && streamInfoAfter.has_value(), "no streamInfo");
    record.streamType = static_cast<int32_t>(streamInfoBefore.value().type);
    record.videoWidthBefore = streamInfoBefore.value().videoWidth;
    record.videoHeightBefore = streamInfoBefore.value().videoHeight;
    record.videoFrameRateBefore = streamInfoBefore.value().frameRate;
    record.audioChannelsBefore = streamInfoBefore.value().channels;
    record.bitrateBefore = streamInfoBefore.value().bitRate;
    record.videoTypeBefore = static_cast<int32_t>(streamInfoBefore.value().videoType);
    record.codecsBefore = streamInfoBefore.value().codecs;
    record.originCodecsBefore = streamInfoBefore.value().originCodecs;
    if (streamInfoBefore.value().mimeType.find("audio/") == 0) {
        record.audioMimeTypeBefore = streamInfoBefore.value().mimeType;
        record.audioLangBefore = streamInfoBefore.value().lang;
    } else if (streamInfoBefore.value().mimeType.find("video/") == 0) {
        record.videoMimeTypeBefore = streamInfoBefore.value().mimeType;
    } else if (streamInfoBefore.value().type == Plugins::StreamType::SUBTITLE) {
        record.subtitleLangBefore = streamInfoBefore.value().lang;
    }
    record.videoWidthAfter = streamInfoAfter.value().videoWidth;
    record.videoHeightAfter = streamInfoAfter.value().videoHeight;
    record.videoFrameRateAfter = streamInfoAfter.value().frameRate;
    record.audioChannelsAfter = streamInfoAfter.value().channels;
    record.bitrateAfter = streamInfoAfter.value().bitRate;
    record.videoTypeAfter = static_cast<int32_t>(streamInfoAfter.value().videoType);
    record.codecsAfter = streamInfoAfter.value().codecs;
    record.originCodecsAfter = streamInfoAfter.value().originCodecs;
    if (streamInfoAfter.value().mimeType.find("audio/") == 0) {
        record.audioMimeTypeAfter = streamInfoAfter.value().mimeType;
        record.audioLangAfter = streamInfoAfter.value().lang;
    } else if (streamInfoAfter.value().mimeType.find("video/") == 0) {
        record.videoMimeTypeAfter = streamInfoAfter.value().mimeType;
    } else if (streamInfoAfter.value().type == Plugins::StreamType::SUBTITLE) {
        record.subtitleLangAfter = streamInfoAfter.value().lang;
    }
}

void MediaDemuxer::FillRecordBeforeFromLastReported(MediaChangeRecord& record)
{
    record.bitrateBefore = lastReportedBitrate_;
    record.videoWidthBefore = lastReportedVideoWidth_;
    record.videoHeightBefore = lastReportedVideoHeight_;
    record.videoFrameRateBefore = lastReportedVideoFrameRate_;
    record.audioChannelsBefore = lastReportedAudioChannels_;
    record.audioSampleRateBefore = lastReportedAudioSampleRate_;
    record.codecsBefore = lastReportedCodecs_;
    record.originCodecsBefore = lastReportedOriginCodecs_;
    record.videoTypeBefore = lastReportedVideoType_;
}

void MediaDemuxer::FillRecordAfterFromRecord(MediaChangeRecord& record, const MediaChangeRecord& source)
{
    record.bitrateAfter = source.bitrateAfter;
    record.videoWidthAfter = source.videoWidthAfter;
    record.videoHeightAfter = source.videoHeightAfter;
    record.videoFrameRateAfter = source.videoFrameRateAfter;
    record.audioChannelsAfter = source.audioChannelsAfter;
    record.audioSampleRateAfter = source.audioSampleRateAfter;
    record.codecsAfter = source.codecsAfter;
    record.originCodecsAfter = source.originCodecsAfter;
    record.videoTypeAfter = source.videoTypeAfter;
}

void MediaDemuxer::TrimHistorySize()
{
    const size_t maxHistorySize = 10;
    while (mediaChangeHistory_.size() > maxHistorySize) {
        auto& oldest = mediaChangeHistory_.front();
        if (oldest.hasReported) {
            mediaChangeHistory_.pop_front();
        } else {
            MEDIA_LOG_W("Force remove unreported change record");
            mediaChangeHistory_.pop_front();
        }
    }
}

void MediaDemuxer::ReportMediaChangedFailedEvent()
{
    FALSE_RETURN(eventReceiver_ != nullptr);
    
    auto latestUnreported = GetLatestUnreportedRecord();
    if (latestUnreported == mediaChangeHistory_.end()) {
        MEDIA_LOG_W("No unreported media change record for failed event");
        return;
    }
    
    MediaChangeInfo changeInfo;
    FillChangeInfoFromRecord(changeInfo, *latestUnreported, true);
    
    eventReceiver_->OnDfxEvent({"DEMUX", DfxEventType::DFX_EVENT_MEDIA_CHANGED, changeInfo});
    
    latestUnreported->hasReported = true;
    CleanUpReportedRecords();
}

void MediaDemuxer::RecordMediaChange(const MediaChangeParams& params)
{
    if (!params.isLocalFd) {
        for (const auto& record : mediaChangeHistory_) {
            if (!record.hasReported && record.streamIdBefore == params.streamIdBefore &&
                record.streamIdAfter == params.streamIdAfter &&
                record.streamType == params.streamType) {
                MEDIA_LOG_I("Duplicate media change detected, skip recording");
                return;
            }
        }
    }
    MediaChangeRecord record;
    record.changeSeqNum = ++mediaChangeSeqNum_;
    record.isLocalFd = params.isLocalFd;
    record.trackIdBefore = params.trackIdBefore;
    record.trackIdAfter = params.trackIdAfter;
    record.streamIdBefore = params.streamIdBefore;
    record.streamIdAfter = params.streamIdAfter;
    record.isStreamIdChange = (params.streamIdBefore != params.streamIdAfter);
    record.streamType = params.streamType;
    record.changeReason = params.reason;
    record.hasReported = false;

    if (params.isLocalFd) {
        FillVideoParamsFromVideoTrack(record, params.videoTrackIdBefore, params.videoTrackIdAfter);
        FillAudioParamsFromAudioTrack(record, params.audioTrackIdBefore, params.audioTrackIdAfter);
        FillSubtitleParamsFromSubtitleTrack(record, params.subtitleTrackIdBefore, params.subtitleTrackIdAfter);
    } else {
        FillRecordFromStream(record, params.streamIdBefore, params.streamIdAfter);
        if (record.streamType == static_cast<int32_t>(Plugins::StreamType::VIDEO)) {
            FillVideoParamsFromVideoStream(record);
            FillAudioParamsFromAudioStream(record);
            FillSubtitleParamsFromSubtitleStream(record);
        } else if (record.streamType == static_cast<int32_t>(Plugins::StreamType::AUDIO)) {
            FillVideoParamsFromVideoStream(record);
            FillAudioParamsFromAudioStream(record);
            FillSubtitleParamsFromSubtitleStream(record);
        } else if (record.streamType == static_cast<int32_t>(Plugins::StreamType::MIXED)) {
            FillVideoParamsFromVideoStream(record);
            FillAudioParamsFromAudioStream(record);
            FillSubtitleParamsFromSubtitleStream(record);
        } else if (record.streamType == static_cast<int32_t>(Plugins::StreamType::SUBTITLE)) {
            FillVideoParamsFromVideoStream(record);
            FillAudioParamsFromAudioStream(record);
        } else {
            // do nothing
        }
    }
    mediaChangeHistory_.push_back(record);
    TrimHistorySize();
}

bool MediaDemuxer::IsBackToOriginalStream(const MediaChangeRecord& firstRecord,
    const MediaChangeRecord& lastRecord)
{
    if (firstRecord.isLocalFd) {
        return (firstRecord.trackIdBefore == lastRecord.trackIdAfter);
    }
    
    bool isSameStream = (firstRecord.streamIdBefore == lastRecord.streamIdAfter);
    bool isSameTrack = (firstRecord.trackIdBefore == lastRecord.trackIdAfter);
    
    return isSameStream && isSameTrack;
}

bool MediaDemuxer::CheckParameterChange(const MediaChangeRecord& firstRecord,
    const MediaChangeRecord& lastRecord)
{
    if (firstRecord.videoWidthBefore != lastRecord.videoWidthAfter ||
        firstRecord.videoHeightBefore != lastRecord.videoHeightAfter ||
        firstRecord.videoFrameRateBefore != lastRecord.videoFrameRateAfter ||
        firstRecord.bitrateBefore != lastRecord.bitrateAfter ||
        firstRecord.videoMimeTypeBefore != lastRecord.videoMimeTypeAfter ||
        firstRecord.codecsBefore != lastRecord.codecsAfter ||
        firstRecord.videoTypeBefore != lastRecord.videoTypeAfter) {
        return true;
    }
    
    if (firstRecord.audioChannelsBefore != lastRecord.audioChannelsAfter ||
        firstRecord.audioSampleRateBefore != lastRecord.audioSampleRateAfter ||
        firstRecord.audioMimeTypeBefore != lastRecord.audioMimeTypeAfter ||
        firstRecord.audioLangBefore != lastRecord.audioLangAfter ||
        firstRecord.subtitleLangBefore != lastRecord.subtitleLangAfter ||
        firstRecord.codecsBefore != lastRecord.codecsAfter) {
        return true;
    }
    
    return false;
}

void MediaDemuxer::ProcessHistoryOnSeek()
{
    std::vector<size_t> unreportedIndices;
    for (size_t i = 0; i < mediaChangeHistory_.size(); i++) {
        if (!mediaChangeHistory_[i].hasReported) {
            unreportedIndices.push_back(i);
        }
    }
    if (unreportedIndices.empty()) {
        return;
    }
    size_t firstIdx = unreportedIndices.front();
    size_t lastIdx = unreportedIndices.back();
    auto& firstRecord = mediaChangeHistory_[firstIdx];
    auto& lastRecord = mediaChangeHistory_[lastIdx];
    if (IsBackToOriginalStream(firstRecord, lastRecord) &&
        !CheckParameterChange(firstRecord, lastRecord)) {
        RemoveUnreportedRecords();
        return;
    }
    MediaChangeRecord newRecord;
    newRecord.changeSeqNum = ++mediaChangeSeqNum_;
    newRecord.isLocalFd = firstRecord.isLocalFd;
    newRecord.trackIdBefore = firstRecord.trackIdBefore;
    newRecord.trackIdAfter = lastRecord.trackIdAfter;
    newRecord.streamIdBefore = firstRecord.streamIdBefore;
    newRecord.streamIdAfter = lastRecord.streamIdAfter;
    newRecord.isStreamIdChange = lastRecord.isStreamIdChange;
    newRecord.streamType = lastRecord.streamType;
    newRecord.changeReason = lastRecord.changeReason;
    newRecord.hasReported = false;
    FillRecordBeforeFromLastReported(newRecord);
    FillRecordAfterFromRecord(newRecord, lastRecord);
    RemoveUnreportedRecords();
    mediaChangeHistory_.push_back(newRecord);
}

void MediaDemuxer::RemoveUnreportedRecords()
{
    auto it = mediaChangeHistory_.begin();
    while (it != mediaChangeHistory_.end()) {
        if (!it->hasReported) {
            it = mediaChangeHistory_.erase(it);
        } else {
            ++it;
        }
    }
}

void MediaDemuxer::CleanUpReportedRecords()
{
    auto it = mediaChangeHistory_.begin();
    while (it != mediaChangeHistory_.end()) {
        if (it->hasReported) {
            it = mediaChangeHistory_.erase(it);
        } else {
            ++it;
        }
    }
}

void MediaDemuxer::ClearMediaChangeHistory()
{
    mediaChangeHistory_.clear();
    mediaChangeSeqNum_ = 0;
    lastReportedSeqNum_ = 0;
    lastReportedStreamId_ = -1;
    lastReportedTrackId_ = -1;
    lastReportedBitrate_ = 0;
    lastReportedVideoWidth_ = 0;
    lastReportedVideoHeight_ = 0;
    lastReportedVideoFrameRate_ = 0;
    lastReportedAudioChannels_ = 0;
    lastReportedAudioSampleRate_ = 0;
    lastReportedCodecs_.clear();
    lastReportedOriginCodecs_.clear();
    lastReportedVideoType_ = 0;
}

void MediaDemuxer::UpdateLastReportedState(const MediaChangeRecord& record)
{
    lastReportedSeqNum_ = record.changeSeqNum;
    lastReportedStreamId_ = record.streamIdAfter;
    lastReportedTrackId_ = record.trackIdAfter;
    lastReportedBitrate_ = record.bitrateAfter;
    lastReportedVideoWidth_ = record.videoWidthAfter;
    lastReportedVideoHeight_ = record.videoHeightAfter;
    lastReportedVideoFrameRate_ = record.videoFrameRateAfter;
    lastReportedAudioChannels_ = record.audioChannelsAfter;
    lastReportedAudioSampleRate_ = record.audioSampleRateAfter;
    lastReportedCodecs_ = record.codecsAfter;
    lastReportedOriginCodecs_ = record.originCodecsAfter;
    lastReportedVideoType_ = record.videoTypeAfter;
}

void MediaDemuxer::OnNewTrackRender(uint64_t renderSeqNum)
{
    for (auto& record : mediaChangeHistory_) {
        if (record.changeSeqNum != renderSeqNum || record.hasReported) {
            continue;
        }
        
        MediaChangeInfo changeInfo;
        FillChangeInfoFromRecord(changeInfo, record, false);
        
        FALSE_RETURN(eventReceiver_ != nullptr);
        eventReceiver_->OnDfxEvent({"DEMUX", DfxEventType::DFX_EVENT_MEDIA_CHANGED, changeInfo});
        
        record.hasReported = true;
        UpdateLastReportedState(record);
        CleanUpReportedRecords();
        break;
    }
}

void MediaDemuxer::PushCodecCheckBuffer(int32_t trackId)
{
    FALSE_RETURN_NOLOG(demuxerPluginManager_ != nullptr && demuxerPluginManager_->IsDash());
    FALSE_RETURN_NOLOG(IsValidTrackId(trackId) && (trackId == videoTrackId_ || trackId == audioTrackId_));

    int32_t retryCount = 0;
    while (!GetBufferFromUserQueue(trackId, SampleQueue::DEFAULT_SAMPLE_BUFFER_CAP) &&
        retryCount < PUSH_CODEC_CHECK_BUFFER_RETRY_COUNT) {
        retryCount++;
        usleep(PUSH_CODEC_CHECK_BUFFER_RETRY_INTERVAL_US);
    }

    auto buffer = bufferMap_[trackId];
    FALSE_RETURN(buffer != nullptr);
    buffer->flag_ = static_cast<uint32_t>(AVBufferFlag::EOS);
    buffer->pts_ = 0;
    int32_t tmpTrackId = demuxerPluginManager_->GetTmpTrackIdByTrackId(trackId);
    FALSE_RETURN(IsValidTrackId(tmpTrackId) && static_cast<size_t>(tmpTrackId) < mediaMetaData_.trackMetas.size());
    auto meta = mediaMetaData_.trackMetas[tmpTrackId];
    FALSE_RETURN(meta != nullptr);
    buffer->meta_ = std::make_shared<Meta>(*meta);
    buffer->meta_->SetData(Tag::CHECK_CODEC_CHANGE, true);
    if (buffer->memory_ == nullptr) {
        auto tmpBuffer = AVBuffer::CreateAVBuffer(buffer->GetConfig());
        buffer->memory_ = tmpBuffer->memory_;
        tmpBuffer->memory_ = nullptr;
    }
    PushBufferToQueue(trackId, buffer, true);
}

void MediaDemuxer::CheckCodecChange(const std::shared_ptr<SampleQueue> &sampleQueue, int32_t trackId, bool& needRelease)
{
    needRelease = false;
    FALSE_RETURN_NOLOG(demuxerPluginManager_ != nullptr && demuxerPluginManager_->IsDash());

    std::shared_ptr<AVBuffer> buffer;
    Status ret = sampleQueue->AcquireBuffer(buffer);
    FALSE_RETURN_NOLOG(ret == Status::OK);

    CheckCodecChange(buffer, trackId, needRelease);
    needRelease ? sampleQueue->ReleaseBuffer(buffer) : sampleQueue->RollbackBuffer(buffer);
}

void MediaDemuxer::CheckCodecChange(const std::shared_ptr<AVBuffer>& buffer, int32_t trackId, bool& needRelease)
{
    needRelease = false;
    FALSE_RETURN(buffer != nullptr && buffer->meta_ != nullptr);

    bool needCheckCodecChange = false;
    buffer->meta_->GetData(Tag::CHECK_CODEC_CHANGE, needCheckCodecChange);
    FALSE_RETURN_NOLOG((buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)) && needCheckCodecChange);

    if (trackId == audioTrackId_) {
        PauseConsumerTaskByTrackId(trackId);
        needRelease = true;
        FALSE_RETURN(eventReceiver_ != nullptr);
        eventReceiver_->OnEvent({"media_demuxer", EventType::EVENT_AUDIO_DECODER_CHANGE, buffer->meta_});
        return;
    }

    FALSE_RETURN(hiplayerCallback_ != nullptr);
    bool isCodecChanged = false;
    hiplayerCallback_->HandleVideoCodecChange(buffer->meta_, isCodecChanged, true);
    if (isCodecChanged) {
        PauseConsumerTaskByTrackId(trackId);
    } else {
        needRelease = true;
    }
}

Status MediaDemuxer::PauseConsumerTaskByTrackId(int32_t trackId)
{
    auto iter = sampleConsumerTaskMap_.find(trackId);
    FALSE_RETURN_V_MSG_E(iter != sampleConsumerTaskMap_.end(), Status::ERROR_INVALID_PARAMETER, "Invalid track");
    iter->second->PauseAsync();
    iter->second->Pause();
    return Status::OK;
}

bool MediaDemuxer::IsNeedSelectTrack(int32_t trackId)
{
    if (subtitleTrackIdVec_.empty()) {
        return true;
    }
    for (size_t i = 0; i < subtitleTrackIdVec_.size(); ++i) {
        if (trackId == subtitleTrackIdVec_[i]) {
            return false;
        }
    }
    return true;
}

int32_t MediaDemuxer::GetBuiltInSubtitleNum()
{
    return subtitleTrackIdVec_.size();
}

bool MediaDemuxer::IsBuiltInSubtitleTrackId(int32_t trackId)
{
    FALSE_RETURN_V(IsSubtitleTrackId(trackId), false);
    FALSE_RETURN_V_MSG_E(demuxerPluginManager_ != nullptr, true, "plugin manager is nullptr");
    int32_t audioStreamId = demuxerPluginManager_->GetStreamIDByTrackID(audioTrackId_);
    int32_t videoStreamId = demuxerPluginManager_->GetStreamIDByTrackID(videoTrackId_);
    int32_t subtitleStreamId = demuxerPluginManager_->GetStreamIDByTrackID(trackId);
    MEDIA_LOG_I("IsBuiltInSubtitleTrackId audioStreamId:" PUBLIC_LOG_D32 ",videoStreamId:" PUBLIC_LOG_D32
        ",subtitleStreamId:" PUBLIC_LOG_D32, audioStreamId, videoStreamId, subtitleStreamId);
    return audioStreamId == subtitleStreamId || videoStreamId == subtitleStreamId;
}

void MediaDemuxer::InitBuiltInSubtitleTrack()
{
    FALSE_RETURN(!subtitleTrackIdVec_.empty());
    for (size_t i = 0; i < subtitleTrackIdVec_.size(); ++i) {
        int32_t tempTrackId = subtitleTrackIdVec_[i];
        if (tempTrackId == subtitleTrackId_) {
            continue;
        }
        GetEnableSampleQueueFlag() ? AddDemuxerCopyTaskByTrackIfFilter(tempTrackId, DemuxerTrackType::SUBTITLE)
            : AddDemuxerCopyTaskIfFilter(tempTrackId, TaskType::SUBTITLE);
        demuxerPluginManager_->UpdateTempTrackMapInfo(tempTrackId, tempTrackId, -1);
        int32_t streamId = demuxerPluginManager_->GetTmpStreamIDByTrackID(tempTrackId);
        MEDIA_LOG_I("InnerPrepare streamID:" PUBLIC_LOG_D32, streamId);
    }
}

Status MediaDemuxer::InitBuiltInSubtitleBufferQueue(int32_t trackId, const sptr<AVBufferQueueProducer>& producer)
{
    Status ret = Status::OK;
    if (!IsSubtitleTrackId(trackId) || subtitleTrackIdVec_.empty()) {
        return ret;
    }
    for (size_t i = 0; i < subtitleTrackIdVec_.size(); ++i) {
        int32_t tempTrackId = subtitleTrackIdVec_[i];
        MEDIA_LOG_I("Set bufferQueue for track" PUBLIC_LOG_D32, tempTrackId);
        if (bufferQueueMap_.find(tempTrackId) != bufferQueueMap_.end()) {
            MEDIA_LOG_W("Has been set already");
        }
        ret = InnerSelectTrack(tempTrackId);
        FALSE_CONTINUE_NOLOG(ret == Status::OK);
        auto track = trackMap_.find(tempTrackId);
        if (track != trackMap_.end()) {
            auto listener = track->second->GetProducerListener();
            if (listener) {
                MEDIA_LOG_W("Set listener");
                producer->SetBufferAvailableListener(listener);
            }
        }
        bufferQueueMap_.insert(std::pair<int32_t, sptr<AVBufferQueueProducer>>(tempTrackId, producer));
        bufferMap_.insert(std::pair<int32_t, std::shared_ptr<AVBuffer>>(tempTrackId, nullptr));
        MEDIA_LOG_I("Set bufferQueue successfully");
        if (GetEnableSampleQueueFlag()) {
            ret = AddSampleBufferQueue(tempTrackId);
            FALSE_RETURN_V_MSG_E(
                ret == Status::OK, ret, "AddSampleBufferQueue failed for track " PUBLIC_LOG_D32, tempTrackId);
        }
    }
    return ret;
}

Status MediaDemuxer::SelectBuiltInSubtitleTrack(int32_t trackId, int32_t curTrackId)
{
    if (!IsNeedSelectTrack(trackId) && !IsNeedSelectTrack(curTrackId)) {
        return Status::OK;
    }
    if (IsNeedSelectTrack(curTrackId)) {
        // 当前字幕不是内置字幕，目标字幕是内置字幕
        if (taskMap_.find(curTrackId) != taskMap_.end() && taskMap_[curTrackId] != nullptr) {
            taskMap_[curTrackId]->Stop();
        }
        if (sampleConsumerTaskMap_.find(curTrackId) != sampleConsumerTaskMap_.end() &&
            sampleConsumerTaskMap_[curTrackId] != nullptr) {
            sampleConsumerTaskMap_[curTrackId]->Stop();
        }
        UnselectTrack(curTrackId);

        bufferQueueMap_.erase(curTrackId);
        bufferMap_.erase(curTrackId);
        if (GetEnableSampleQueueFlag()) {
            AutoLock lock(mapMutex_);
            sampleQueueMap_.erase(curTrackId);
        }
        return Status::OK;
    } else if (IsNeedSelectTrack(trackId)) {
        // 当前字幕是内置字幕，目标字幕不是内置字幕
        bufferQueueMap_.insert(
            std::pair<int32_t, sptr<AVBufferQueueProducer>>(trackId, bufferQueueMap_[curTrackId]));
        bufferMap_.insert(std::pair<int32_t, std::shared_ptr<AVBuffer>>(trackId, nullptr));
        demuxerPluginManager_->UpdateTempTrackMapInfo(trackId, trackId, -1);
        if (GetEnableSampleQueueFlag()) {
            AutoLock lock(mapMutex_);
            AddSampleBufferQueue(trackId);
            if (sampleQueueMap_[trackId] != nullptr) {
                sampleQueueMap_[trackId]->UpdateQueueId(trackId);
            }
        }
        return InnerSelectTrack(trackId);
    }
    return Status::ERROR_UNKNOWN;
}

Status MediaDemuxer::UpdateSubtitleLastPts(int32_t trackId)
{
    if (!subtitleTrackIdVec_.empty() && IsSubtitleTrackId(trackId) && bufferMap_[trackId] != nullptr) {
        lastSubtitlePtsMap_[trackId] = bufferMap_[trackId]->pts_;
    }
    return Status::OK;
}

bool MediaDemuxer::IsSubtitleQueueStale(int32_t queueIndex)
{
    auto iter = lastSubtitlePtsMap_.find(queueIndex);
    if (iter != lastSubtitlePtsMap_.end() && syncCenter_ != nullptr
        && iter->second - syncCenter_->GetMediaTimeNow() > MAX_SUBTITLE_LEAD_TIME) {
        return false;
    }
    return true;
}
} // namespace Media
} // namespace OHOS
