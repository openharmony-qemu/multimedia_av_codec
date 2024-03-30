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
#include "live_stream_demuxer.h"
#include "vod_stream_demuxer.h"

namespace OHOS {
namespace Media {
static const uint32_t REQUEST_BUFFER_TIMEOUT = 200; // Retry if the time of requesting buffer overtimes 200ms.
static const int32_t MSERR_EXT_IO = 5400103;
static const int32_t START = 1;
static const int32_t PAUSE = 2;
static const uint32_t RETRY_FRAME_TIME = 10; // Retry if no buffer ready 10ms.

class MediaDemuxer::DataSourceImpl : public Plugins::DataSource {
public:
    explicit DataSourceImpl(MediaDemuxer& demuxer);
    ~DataSourceImpl() override = default;
    Status ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen) override;
    Status GetSize(uint64_t& size) override;
    Plugins::Seekable GetSeekable() override;

private:
    MediaDemuxer& demuxer_;
};

MediaDemuxer::DataSourceImpl::DataSourceImpl(MediaDemuxer& demuxer) : demuxer_(demuxer)
{
}

/**
* ReadAt Plugins::DataSource::ReadAt implementation.
* @param offset offset in media stream.
* @param buffer caller allocate real buffer.
* @param expectedLen buffer size wanted to read.
* @return read result.
*/
Status MediaDemuxer::DataSourceImpl::ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen)
{
    MediaAVCodec::AVCodecTrace trace("DataSourceImpl::ReadAt");
    FALSE_RETURN_V(expectedLen != 0, Status::OK);
    if (!buffer || !demuxer_.IsOffsetValid(offset)) {
        MEDIA_LOG_E("ReadAt failed, buffer empty: " PUBLIC_LOG_D32 ", expectedLen: " PUBLIC_LOG_D32
                            ", offset: " PUBLIC_LOG_D64, !buffer, static_cast<int>(expectedLen), offset);
        return Status::ERROR_UNKNOWN;
    }
    return demuxer_.streamDemuxer_->CallbackReadAt(offset, buffer, expectedLen);
}

Status MediaDemuxer::DataSourceImpl::GetSize(uint64_t& size)
{
    size = demuxer_.mediaDataSize_;
    return (size > 0) ? Status::OK : Status::ERROR_WRONG_STATE;
}

Plugins::Seekable MediaDemuxer::DataSourceImpl::GetSeekable()
{
    return demuxer_.seekable_;
}


MediaDemuxer::MediaDemuxer()
    : seekable_(Plugins::Seekable::INVALID),
      uri_(),
      mediaDataSize_(0),
      pluginName_(),
      plugin_(nullptr),
      dataSource_(std::make_shared<DataSourceImpl>(*this)),
      source_(std::make_shared<Source>()),
      mediaMetaData_(),
      bufferQueueMap_(),
      bufferMap_(),
      eventReceiver_(),
      streamDemuxer_()
{
    MEDIA_LOG_I("MediaDemuxer called");
}

MediaDemuxer::~MediaDemuxer()
{
    MEDIA_LOG_I("~MediaDemuxer called");
    {
        AutoLock lock(mapMetaMutex_);
        bufferQueueMap_.clear();
        bufferMap_.clear();
    }
    if (!isThreadExit_) {
        Stop();
    }
    if (plugin_) {
        plugin_->Deinit();
    }
    plugin_ = nullptr;
    dataSource_ = nullptr;
    mediaSource_ = nullptr;
    source_ = nullptr;
    eventReceiver_ = nullptr;
    eosMap_.clear();
    streamDemuxer_ = nullptr;
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

void MediaDemuxer::SetDrmCallback(const std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> &callback)
{
    MEDIA_LOG_I("SetDrmCallback called");
    drmCallback_ = callback;
    bool isExisted = IsLocalDrmInfosExisted();
    if (isExisted) {
        MEDIA_LOG_D("Already received drminfo and report!");
        ReportDrmInfos(localDrmInfos_);
    }
}

void MediaDemuxer::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver)
{
    eventReceiver_ = receiver;
}

bool MediaDemuxer::GetDuration(int64_t& durationMs)
{
    AutoLock lock(mapMetaMutex_);
    return mediaMetaData_.globalMeta->Get<Tag::MEDIA_DURATION>(durationMs);
}

bool MediaDemuxer::IsDrmInfosUpdate(const std::multimap<std::string, std::vector<uint8_t>> &info)
{
    MEDIA_LOG_I("IsDrmInfosUpdate");
    bool isUpdated = false;
    std::unique_lock<std::shared_mutex> lock(drmMutex);
    for (auto &newItem : info) {
        auto pos = localDrmInfos_.equal_range(newItem.first);
        if (pos.first == pos.second && pos.first == localDrmInfos_.end()) {
            MEDIA_LOG_D("IsDrmInfosUpdate this uuid not exists and update");
            localDrmInfos_.insert(newItem);
            isUpdated = true;
            continue;
        }
        MEDIA_LOG_D("IsDrmInfosUpdate this uuid exists many times");
        bool isSame = false;
        for (; pos.first != pos.second; ++pos.first) {
            if (newItem.second == pos.first->second) {
                MEDIA_LOG_D("IsDrmInfosUpdate this uuid exists and same pssh");
                isSame = true;
                break;
            }
        }
        if (!isSame) {
            MEDIA_LOG_D("IsDrmInfosUpdate this uuid exists but pssh not same");
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
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "ProcessDrmInfos failed due to create demuxer plugin failed.");

    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    Status ret = plugin_->GetDrmInfo(drmInfo);
    if (ret == Status::OK && !drmInfo.empty()) {
        MEDIA_LOG_D("demuxer filter get drminfo success");
        bool isUpdated = IsDrmInfosUpdate(drmInfo);
        if (isUpdated) {
            return ReportDrmInfos(drmInfo);
        } else {
            MEDIA_LOG_D("demuxer filter received drminfo but not update");
        }
    } else {
        if (ret != Status::OK) {
            MEDIA_LOG_D("demuxer filter get drminfo failed, ret=" PUBLIC_LOG_D32, (int32_t)(ret));
        }
    }
    return Status::OK;
}

Status MediaDemuxer::ProcessVideoStartTime(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    MEDIA_LOG_D("ProcessVideoStartTime,  trackId: %{public}u", trackId);
    if (trackId == videoTrackId_ && source_ != nullptr && source_->IsSeekToTimeSupported()
        && seekable_ == Plugins::Seekable::SEEKABLE) {
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

Status MediaDemuxer::SetDataSource(const std::shared_ptr<MediaSource> &source)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_I("SetDataSource enter");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    source_->SetCallback(this);
    source_->SetSource(source);
    Status ret = source_->GetSize(mediaDataSize_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set data source failed due to get file size failed.");
    seekable_ = source_->GetSeekable();
    FALSE_RETURN_V_MSG_E(seekable_ != Plugins::Seekable::INVALID, Status::ERROR_NULL_POINTER,
        "Set data source failed due to get seekable failed.");
    ReportIsLiveStreamEvent();
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        Flush();
        streamDemuxer_ = std::make_shared<VodStreamDemuxer>();
    } else {
        streamDemuxer_ = std::make_shared<LiveStreamDemuxer>();
    }
    streamDemuxer_->SetSource(source_);
    std::string type = streamDemuxer_->Init(uri_, mediaDataSize_);
    MediaTypeFound(std::move(type));

    MediaInfo mediaInfo;
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set data source failed due to create demuxer plugin failed.");
    ret = plugin_->GetMediaInfo(mediaInfo);
    if (ret == Status::OK) {
        InitMediaMetaData(mediaInfo);
        streamDemuxer_->SetDemuxerState(DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
    } else {
        MEDIA_LOG_E("demuxer filter parse meta failed, ret: " PUBLIC_LOG_D32, (int32_t)(ret));
    }

    ProcessDrmInfos();
    MEDIA_LOG_I("SetDataSource exit");
    return ret;
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
    AutoLock lock(mapMetaMutex_);
    FALSE_RETURN_V_MSG_E(trackId >= 0 && (uint32_t)trackId < mediaMetaData_.trackMetas.size(),
        Status::ERROR_INVALID_PARAMETER, "Set bufferQueue trackId error.");
    useBufferQueue_ = true;
    MEDIA_LOG_I("Set bufferQueue for track " PUBLIC_LOG_D32 ".", trackId);
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    FALSE_RETURN_V_MSG_E(producer != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set bufferQueue failed due to input bufferQueue is nullptr.");
    if (bufferQueueMap_.find(trackId) != bufferQueueMap_.end()) {
        MEDIA_LOG_W("BufferQueue has been set already, will be overwritten. trackId: ");
    }
    Status ret = InnerSelectTrack(trackId);
    if (ret == Status::OK) {
        bufferQueueMap_.insert(std::pair<uint32_t, sptr<AVBufferQueueProducer>>(trackId, producer));
        bufferMap_.insert(std::pair<uint32_t, std::shared_ptr<AVBuffer>>(trackId, nullptr));
        MEDIA_LOG_I("Set bufferQueue successfully.");
    }
    return ret;
}

std::map<uint32_t, sptr<AVBufferQueueProducer>> MediaDemuxer::GetBufferQueueProducerMap()
{
    return bufferQueueMap_;
}

Status MediaDemuxer::InnerSelectTrack(int32_t trackId)
{
    eosMap_[trackId] = false;
    return plugin_->SelectTrack(trackId);
}

Status MediaDemuxer::SelectTrack(int32_t trackId)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    FALSE_RETURN_V_MSG_E(trackId >= 0 && (uint32_t)trackId < mediaMetaData_.trackMetas.size(),
        Status::ERROR_INVALID_PARAMETER, "Select trackId error.");
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot select track when use buffer queue.");
    return InnerSelectTrack(trackId);
}

Status MediaDemuxer::UnselectTrack(int32_t trackId)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot unselect track when use buffer queue.");
    return plugin_->UnselectTrack(trackId);
}

Status MediaDemuxer::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_OPERATION, "SeekTo error seekTime: " PUBLIC_LOG_D64
        ", mode: " PUBLIC_LOG_U32, seekTime, (uint32_t)mode);

    Status ret;
    if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
        MEDIA_LOG_I("SeekTo source SeekToTime start");
        if (mode == SeekMode::SEEK_CLOSEST_INNER) {
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_PREVIOUS_SYNC);
        } else {
            ret = source_->SeekToTime(seekTime, SeekMode::SEEK_CLOSEST_SYNC);
        }
        Plugins::Ms2HstTime(seekTime, realSeekTime);
    } else {
        MEDIA_LOG_I("SeekTo start");
        if (mode == SeekMode::SEEK_CLOSEST_INNER) {
            ret = plugin_->SeekTo(-1, seekTime, SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
        } else {
            ret = plugin_->SeekTo(-1, seekTime, mode, realSeekTime);
        }
    }
    isSeeked_ = true;
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    MEDIA_LOG_I("SeekTo done");
    return ret;
}

Status MediaDemuxer::SelectBitRate(uint32_t bitRate)
{
    if (source_ == nullptr) {
        MEDIA_LOG_E("SelectBitRate failed, source_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    streamDemuxer_->Reset();
    return source_->SelectBitRate(bitRate);
}

std::vector<std::shared_ptr<Meta>> MediaDemuxer::GetStreamMetaInfo() const
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    return mediaMetaData_.trackMetas;
}

std::shared_ptr<Meta> MediaDemuxer::GetGlobalMetaInfo()
{
    AutoLock lock(mapMetaMutex_);
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    return mediaMetaData_.globalMeta;
}

std::shared_ptr<Meta> MediaDemuxer::GetUserMeta()
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, nullptr, "Demuxer plugin is not exist.");
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    FALSE_RETURN_V_MSG_E(meta != nullptr, nullptr, "Create meta failed.");
    Status ret = plugin_->GetUserMeta(meta);
    if (ret != Status::OK) {
        MEDIA_LOG_W("No valid user data");
    }
    return meta;
}

Status MediaDemuxer::Flush()
{
    MEDIA_LOG_I("Flush enter.");
    if (streamDemuxer_) {
        streamDemuxer_->Flush();
    }
    
    {
        AutoLock lock(mapMetaMutex_);
        auto it = bufferQueueMap_.begin();
        while (it != bufferQueueMap_.end()) {
            uint32_t trackId = it->first;
            if (trackId != videoTrackId_) {
                bufferQueueMap_[trackId]->Clear();
            }
            it++;
        }
    }

    if (plugin_) {
        plugin_->Flush();
    }

    return Status::OK;
}

Status MediaDemuxer::StopAllTask()
{
    MEDIA_LOG_I("StopAllTask enter.");
    streamDemuxer_->SetIsIgnoreParse(true);

    auto it = taskMap_.begin();
    while (it != taskMap_.end()) {
        if (it->second != nullptr) {
            it->second->Stop();
            it->second = nullptr;
        }
        it = taskMap_.erase(it);
    }
    isThreadExit_ = true;
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

    streamDemuxer_->SetIsIgnoreParse(true);

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

    return Status::OK;
}

Status MediaDemuxer::ResumeAllTask()
{
    MEDIA_LOG_I("ResumeAllTask enter.");
    streamDemuxer_->SetIsIgnoreParse(false);

    auto it = taskMap_.begin();
    while (it != taskMap_.end()) {
        if (it->second != nullptr) {
            it->second->Start();
        }
        it++;
    }
    return Status::OK;
}

Status MediaDemuxer::Pause()
{
    MEDIA_LOG_I("Pause");
    if (streamDemuxer_) {
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

    // To accelerate DemuxerLoop thread to run into PAUSED state
    for (auto &iter : taskMap_) {
        if (iter.first == trackId && iter.second != nullptr) {
            iter.second->PauseAsync();
        }
    }

    for (auto &iter : taskMap_) {
        if (iter.first == trackId && iter.second != nullptr) {
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
    return Status::OK;
}

Status MediaDemuxer::Reset()
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::Reset");
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    {
        AutoLock lock(mapMetaMutex_);
        mediaMetaData_.globalMeta.reset();
        mediaMetaData_.trackMetas.clear();
        if (!isThreadExit_) {
            Stop();
        }
        bufferQueueMap_.clear();
        bufferMap_.clear();
    }
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    videoStartTime_ = 0;
    streamDemuxer_->Reset();
    return plugin_->Reset();
}

Status MediaDemuxer::Start()
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::Start");
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Start read failed due to has not set data source.");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::OK,
        "Process has been started already, neet to stop it first.");
    FALSE_RETURN_V_MSG_E(bufferQueueMap_.size() != 0, Status::OK,
        "Read task is running already.");
    for (auto it = eosMap_.begin(); it != eosMap_.end(); it++) {
        it->second = false;
    }
    isThreadExit_ = false;
    auto it = bufferQueueMap_.begin();
    while (it != bufferQueueMap_.end()) {
        uint32_t trackId = it->first;
        taskMap_[trackId]->Start();
        it++;
    }
    MEDIA_LOG_I("Demuxer thread started.");
    source_->Start();
    return plugin_->Start();
}

Status MediaDemuxer::Stop()
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::Stop");
    MEDIA_LOG_I("MediaDemuxer Stop.");
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    FALSE_RETURN_V_MSG_E(!isThreadExit_, Status::OK, "Process has been stopped already, need to start if first.");
    source_->Stop();
    StopAllTask();
    source->Stop();
    streamDemuxer_->Stop();
    return plugin_->Stop();
}

bool MediaDemuxer::CreatePlugin(std::string pluginName)
{
    if (plugin_) {
        plugin_->Deinit();
    }
    auto plugin = Plugins::PluginManager::Instance().CreatePlugin(pluginName, Plugins::PluginType::DEMUXER);
    plugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(plugin);
    if (!plugin_ || plugin_->Init() != Status::OK) {
        MEDIA_LOG_E("CreatePlugin " PUBLIC_LOG_S " failed.", pluginName.c_str());
        return false;
    }
    plugin_->SetCallback(this);
    pluginName_.swap(pluginName);
    return true;
}

bool MediaDemuxer::InitPlugin(std::string pluginName)
{
    if (pluginName.empty()) {
        return false;
    }
    if (pluginName_ != pluginName) {
        FALSE_RETURN_V(CreatePlugin(std::move(pluginName)), false);
    } else {
        if (plugin_->Reset() != Status::OK) {
            FALSE_RETURN_V(CreatePlugin(std::move(pluginName)), false);
        }
    }
    MEDIA_LOG_I("InitPlugin, " PUBLIC_LOG_S " used.", pluginName_.c_str());
    streamDemuxer_->SetDemuxerState(DemuxerState::DEMUXER_STATE_PARSE_HEADER);
    Status st = plugin_->SetDataSource(dataSource_);
    return st == Status::OK;
}

void MediaDemuxer::MediaTypeFound(std::string pluginName)
{
    MediaAVCodec::AVCodecTrace trace("MediaDemuxer::MediaTypeFound");
    if (!InitPlugin(std::move(pluginName))) {
        MEDIA_LOG_E("MediaTypeFound init plugin error.");
    }
}

void MediaDemuxer::InitMediaMetaData(const Plugins::MediaInfo& mediaInfo)
{
    AutoLock lock(mapMetaMutex_);
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
        if (trackMeta.Get<Tag::MIME_TYPE>(mimeType) && mimeType.find("video") == 0) {
            MEDIA_LOG_I("Found video track, id: " PUBLIC_LOG_U32 ", mimeType: " PUBLIC_LOG_S, index, mimeType.c_str());
            videoMime_ = mimeType;
            videoTrackId_ = index;
            if (!trackMeta.GetData(Tag::MEDIA_START_TIME, videoStartTime_)) {
                MEDIA_LOG_W("Get media start time failed");
            }
            trackType = "V";
        } else if (trackMeta.Get<Tag::MIME_TYPE>(mimeType) && mimeType.find("audio") == 0) {
            MEDIA_LOG_I("Found audio track, id: " PUBLIC_LOG_U32 ", mimeType: " PUBLIC_LOG_S, index, mimeType.c_str());
            audioTrackId_ = index;
            trackType = "A";
        }
        std::string threadReadName = std::string("DemuxerLoop") + trackType.c_str();
        std::unique_ptr<Task> tempTask = std::make_unique<Task>(threadReadName);
        taskMap_[index] = std::move(tempTask);
        taskMap_[index]->RegisterJob([this, index] { ReadLoop(index); });
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
    avBufferConfig.capacity = size;
    Status ret = bufferQueueMap_[queueIndex]->RequestBuffer(bufferMap_[queueIndex], avBufferConfig,
        REQUEST_BUFFER_TIMEOUT);
    FALSE_LOG_MSG_W(ret == Status::OK, "Get buffer failed due to get buffer from bufferQueue failed, user queue: "
        PUBLIC_LOG_U32 ", ret: " PUBLIC_LOG_D32, queueIndex, (int32_t)(ret));
    return ret == Status::OK;
}

Status MediaDemuxer::CopyFrameToUserQueue(uint32_t trackId)
{
    MEDIA_LOG_D("CopyFrameToUserQueue enter, copy frame for track: " PUBLIC_LOG_U32, trackId);
    int32_t size = 0;
    Status ret = plugin_->GetNextSampleSize(trackId, size);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_UNKNOWN, Status::ERROR_UNKNOWN,
        "CopyFrameToUserQueue error for track " PUBLIC_LOG_U32, trackId);
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_AGAIN, Status::ERROR_AGAIN,
        "CopyFrameToUserQueue error for track " PUBLIC_LOG_U32 ", try again", trackId);
    FALSE_RETURN_V_MSG_E(GetBufferFromUserQueue(trackId, size), Status::ERROR_INVALID_PARAMETER,
        "Get buffer from queue failed, trackId: " PUBLIC_LOG_U32, trackId);

    ret = InnerReadSample(trackId, bufferMap_[trackId]);
    if (source_ != nullptr && source_->IsSeekToTimeSupported() && isSeeked_) {
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
        }
        ret = bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], true);
    }
    MEDIA_LOG_D("CopyFrameToUserQueue exit, copy frame for track: " PUBLIC_LOG_U32, trackId);
    return Status::OK;
}

Status MediaDemuxer::InnerReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    MEDIA_LOG_D("copy frame for track " PUBLIC_LOG_U32, trackId);
    Status ret = plugin_->ReadSample(trackId, sample);
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

void MediaDemuxer::ReadLoop(uint32_t trackId)
{
    if (streamDemuxer_->GetIsIgnoreParse()) {
        MEDIA_LOG_D("ReadLoop pausing, copy frame for track " PUBLIC_LOG_U32, trackId);
        OSAL::SleepFor(RETRY_FRAME_TIME); // sleep 10ms to avoid useless reading
    } else {
        Status ret = CopyFrameToUserQueue(trackId);
        if (ret == Status::ERROR_UNKNOWN) {
            MEDIA_LOG_E("Data source is invalid, can not get frame");
            if (eventReceiver_ != nullptr) {
                eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_EXT_IO});
            } else {
                MEDIA_LOG_D("OnEvent eventReceiver_ null.");
            }
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
        MEDIA_LOG_W("Read sample failed due to track has reached eos");
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
    switch (event.type) {
        case PluginEventType::SOURCE_DRM_INFO_UPDATE: {
            MEDIA_LOG_D("OnEvent source drmInfo update");
            HandleSourceDrmInfoEvent(AnyCast<std::multimap<std::string, std::vector<uint8_t>>>(event.param));
            break;
        }
        case PluginEventType::CLIENT_ERROR:
        case PluginEventType::SERVER_ERROR: {
            MEDIA_LOG_D("OnEvent source http error");
            if (eventReceiver_ != nullptr) {
                eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_EXT_IO});
            } else {
                MEDIA_LOG_D("OnEvent source eventReceiver_ null.");
            }
            break;
        }
        case PluginEventType::BUFFERING_END: {
            MEDIA_LOG_D("OnEvent pause");
            if (eventReceiver_ != nullptr) {
                eventReceiver_->OnEvent({"demuxer_filter", EventType::BUFFERING_END, PAUSE});
            } else {
                MEDIA_LOG_D("OnEvent source eventReceiver_ null.");
            }
            break;
        }
        case PluginEventType::BUFFERING_START: {
            MEDIA_LOG_D("OnEvent start");
            if (eventReceiver_ != nullptr) {
                eventReceiver_->OnEvent({"demuxer_filter", EventType::BUFFERING_START, START});
            } else {
                MEDIA_LOG_D("OnEvent source eventReceiver_ null.");
            }
            break;
        }
        case PluginEventType::VIDEO_SIZE_CHANGE: {
            MEDIA_LOG_D("OnEvent video size change");
            if (eventReceiver_ != nullptr) {
                eventReceiver_->OnEvent({"demuxer_filter", EventType::EVENT_RESOLUTION_CHANGE,
                    AnyCast<std::pair<int32_t, int32_t>>(event.param)});
            } else {
                MEDIA_LOG_D("OnEvent source eventReceiver_ null.");
            }
            break;
        }
        default:
            break;
    }
}
} // namespace Media
} // namespace OHOS