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

#include <algorithm>
#include <memory>
#include <map>
#include "avcodec_common.h"
#include "source/source.h"
#include "cpp_ext/type_traits_ext.h"
#include "buffer/avallocator.h"
#include "common/log.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_info.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "plugin/plugin_manager.h"
#include "common/event.h"
#include "plugin/plugin_buffer.h"
#include "media_demuxer.h"

namespace OHOS {
namespace Media {
static const uint32_t REQUEST_BUFFER_TIMEOUT = 1000; // Retry if the time of requesting buffer overtimes 1 second.

class MediaDemuxer::DataSourceImpl : public Plugins::DataSource {
public:
    explicit DataSourceImpl(const MediaDemuxer& demuxer);
    ~DataSourceImpl() override = default;
    Status ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen) override;
    Status GetSize(uint64_t& size) override;
    Plugins::Seekable GetSeekable() override;

private:
    const MediaDemuxer& demuxer_;
};

MediaDemuxer::DataSourceImpl::DataSourceImpl(const MediaDemuxer& demuxer) : demuxer_(demuxer)
{
}

/**
* ReadAt Plugins::DataSource::ReadAt implementation.
* @param offset offset in media stream.
* @param buffer caller allocate real buffer.
* @param expectedLen buffer size wanted to read.
* @return read result.
*/
Status MediaDemuxer::DataSourceImpl::ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer,
    size_t expectedLen)
{
    if (expectedLen == 0) {
        return Status::OK;
    }
    if (!buffer || expectedLen == 0 || !demuxer_.IsOffsetValid(offset)) {
        MEDIA_LOG_E("ReadAt failed, buffer empty: " PUBLIC_LOG_D32 ", expectedLen: " PUBLIC_LOG_D32
                            ", offset: " PUBLIC_LOG_D64, !buffer, static_cast<int>(expectedLen), offset);
        return Status::ERROR_UNKNOWN;
    }
    Status rtv = Status::OK;
    switch (demuxer_.pluginState_.load()) {
        case DemuxerState::DEMUXER_STATE_NULL:
            rtv = Status::ERROR_WRONG_STATE;
            MEDIA_LOG_E("ReadAt error due to DEMUXER_STATE_NULL");
            break;
        case DemuxerState::DEMUXER_STATE_PARSE_HEADER: {
            if (demuxer_.getRange_(static_cast<uint64_t>(offset), expectedLen, buffer)) {
                DUMP_BUFFER2FILE(DEMUXER_INPUT_PEEK, buffer);
            } else {
                rtv = Status::ERROR_NOT_ENOUGH_DATA;
            }
            break;
        }
        case DemuxerState::DEMUXER_STATE_PARSE_FRAME: {
            if (demuxer_.getRange_(static_cast<uint64_t>(offset), expectedLen, buffer)) {
                DUMP_BUFFER2LOG("Demuxer GetRange", buffer, offset);
                DUMP_BUFFER2FILE(DEMUXER_INPUT_GET, buffer);
            } else {
                rtv = Status::END_OF_STREAM;
            }
            break;
        }
        default:
            break;
    }
    return rtv;
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


PushDataImpl::PushDataImpl(std::shared_ptr<MediaDemuxer> demuxer)
{
    demuxer_ = demuxer;
}

Status PushDataImpl::PushData(std::shared_ptr<Buffer>& buffer, int64_t offset)
{
    if (buffer->flag & BUFFER_FLAG_EOS) {
        demuxer_->SetEos();
    } else {
        demuxer_->PushData(buffer, offset);
    }
    return Status::OK;
}

MediaDemuxer::MediaDemuxer()
    : seekable_(Plugins::Seekable::INVALID),
      uri_(),
      mediaDataSize_(0),
      typeFinder_(nullptr),
      dataPacker_(std::make_shared<DataPacker>()),
      pluginName_(),
      plugin_(nullptr),
      dataSource_(std::make_shared<DataSourceImpl>(*this)),
      source_(std::make_shared<Source>()),
      mediaMetaData_(),
      bufferQueueMap_(),
      bufferMap_(),
      eventReceiver_()
{
    MEDIA_LOG_I("MediaDemuxer called");
}

MediaDemuxer::~MediaDemuxer()
{
    MEDIA_LOG_I("~MediaDemuxer called");
    {
        std::unique_lock<std::mutex> lock(mutex_);
        bufferQueueMap_.clear();
        bufferMap_.clear();
    }
    if (!isThreadExit_) {
        Stop();
    }
    if (plugin_) {
        plugin_->Deinit();
    }
    typeFinder_ = nullptr;
    dataPacker_ = nullptr;
    plugin_ = nullptr;
    dataSource_ = nullptr;
    mediaSource_ = nullptr;
    source_ = nullptr;
    eventReceiver_ = nullptr;
    eosMap_.clear();
}

void MediaDemuxer::PushData(std::shared_ptr<Buffer>& bufferPtr, uint64_t offset)
{
    dataPacker_->PushData(bufferPtr, offset);
}

void MediaDemuxer::SetEos()
{
    dataPacker_->SetEos();
}

Status MediaDemuxer::GetBitRates(std::vector<uint32_t> &bitRates)
{
    if (source_ == nullptr) {
        MEDIA_LOG_E("GetBitRates failed, source_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return source_->GetBitRates(bitRates);
}

void MediaDemuxer::SetDrmCallback(const std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> &callback)
{
    MEDIA_LOG_I("SetDrmCallback called");
    drmCallback_ = callback;
}

void MediaDemuxer::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver)
{
    eventReceiver_ = receiver;
}

Status MediaDemuxer::SetDataSource(const std::shared_ptr<MediaSource> &source)
{
    MEDIA_LOG_I("SetDataSource");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    source_->SetSource(source);
    std::shared_ptr<PushDataImpl> pushData_ = std::make_shared<PushDataImpl>(shared_from_this());
    source_->SetPushData(pushData_);
    Status ret = source_->GetSize(mediaDataSize_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set data source failed due to get file size failed.");
    seekable_ = source_->GetSeekable();
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        eventReceiver_->OnEvent({"IS_LIVE_STREAM_EVENT", EventType::EVENT_IS_LIVE_STREAM, true});
        Flush();
        ActivatePullMode();
    } else {
        ActivatePushMode();
    }
    MediaInfo mediaInfo;
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set data source failed due to create demuxer plugin failed.");
    ret = plugin_->GetMediaInfo(mediaInfo);
    if (ret == Status::OK) {
        InitMediaMetaData(mediaInfo);
        pluginState_ = DemuxerState::DEMUXER_STATE_PARSE_FRAME;
    } else {
        MEDIA_LOG_E("demuxer filter parse meta failed, ret=" PUBLIC_LOG_D32, (int32_t)(ret));
    }

    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    ret = plugin_->GetDrmInfo(drmInfo);
    if (ret == Status::OK && !drmInfo.empty()) {
        MEDIA_LOG_I("demuxer filter get drminfo success");
        if (drmCallback_ != nullptr) {
            MEDIA_LOG_I("demuxer filter OnDrmInfoChanged");
            drmCallback_->OnDrmInfoChanged(drmInfo);
        } else {
            MEDIA_LOG_E("demuxer filter get drminfo failed callback is nullptr");
        }
    } else {
        MEDIA_LOG_E("demuxer filter get drminfo failed or no drm info, ret=" PUBLIC_LOG_D32, (int32_t)(ret));
    }
    return ret;
}

Status MediaDemuxer::SetOutputBufferQueue(int32_t trackId, const sptr<AVBufferQueueProducer>& producer)
{
    std::unique_lock<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG_E(trackId >= 0 && trackId < mediaMetaData_.trackMetas.size(), Status::ERROR_INVALID_PARAMETER,
        "Set bufferQueue trackId error.");
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

Status MediaDemuxer::InnerSelectTrack(int32_t trackId)
{
    eosMap_[trackId] = false;
    return plugin_->SelectTrack(trackId);
}

Status MediaDemuxer::SelectTrack(int32_t trackId)
{
    FALSE_RETURN_V_MSG_E(trackId >= 0 && trackId < mediaMetaData_.trackMetas.size(), Status::ERROR_INVALID_PARAMETER,
        "Select trackId error.");
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot select track when use buffer queue.");
    return InnerSelectTrack(trackId);
}

Status MediaDemuxer::UnselectTrack(int32_t trackId)
{
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot unselect track when use buffer queue.");
    return plugin_->UnselectTrack(trackId);
}

Status MediaDemuxer::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    if (!plugin_) {
        MEDIA_LOG_E("SeekTo failed due to no valid plugin");
        return Status::ERROR_INVALID_OPERATION;
    }
    auto rtv = plugin_->SeekTo(-1, seekTime, mode, realSeekTime);
    if (rtv != Status::OK) {
        MEDIA_LOG_E("SeekTo failed with return value: " PUBLIC_LOG_D32, static_cast<int>(rtv));
    }
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    return rtv;
}

Status MediaDemuxer::SelectBitRate(uint32_t bitRate)
{
    if (source_ == nullptr) {
        MEDIA_LOG_E("SelectBitRate failed, source_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return source_->SelectBitRate(bitRate);
}

std::vector<std::shared_ptr<Meta>> MediaDemuxer::GetStreamMetaInfo() const
{
    return mediaMetaData_.trackMetas;
}

std::shared_ptr<Meta> MediaDemuxer::GetGlobalMetaInfo() const
{
    return mediaMetaData_.globalMeta;
}

Status MediaDemuxer::Flush()
{
    dataPacker_->Flush();
    return Status::OK;
}

Status MediaDemuxer::Reset()
{
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    mediaMetaData_.globalMeta.reset();
    mediaMetaData_.trackMetas.clear();
    if (!isThreadExit_) {
        Stop();
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        bufferQueueMap_.clear();
        bufferMap_.clear();
    }
    for (auto item : eosMap_) {
        eosMap_[item.first] = false;
    }
    return plugin_->Reset();
}

Status MediaDemuxer::Start()
{
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
    dataPacker_->Start();
    isThreadExit_ = false;

    auto it = bufferQueueMap_.begin();
    while (it != bufferQueueMap_.end()) {
        uint32_t trackId = it->first;
        std::unique_ptr<std::thread> tempThread = std::make_unique<std::thread>(&MediaDemuxer::ReadLoop, this, trackId);
        threadMap_[trackId] = std::move(tempThread);
        it++;
    }
    MEDIA_LOG_I("Demuxer thread started.");
    return plugin_->Start();
}

Status MediaDemuxer::Stop()
{
    MEDIA_LOG_I("MediaDemuxer Stop.");
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    FALSE_RETURN_V_MSG_E(!isThreadExit_, Status::OK, "Process has been stopped already, need to start if first.");
    isThreadExit_ = true;
    auto it = threadMap_.begin();
    while (it != threadMap_.end()) {
        std::unique_ptr<std::thread> tempThread = std::move(it->second);
        if (tempThread != nullptr && tempThread->joinable()) {
            tempThread->join();
            tempThread = nullptr;
        }
        it = threadMap_.erase(it);
    }
    dataPacker_->Stop();
    return plugin_->Stop();
}

void MediaDemuxer::InitTypeFinder()
{
    if (!typeFinder_) {
        typeFinder_ = std::make_shared<TypeFinder>();
    }
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
    pluginState_ = DemuxerState::DEMUXER_STATE_PARSE_HEADER;
    Status st = plugin_->SetDataSource(dataSource_);
    return st == Status::OK;
}

void MediaDemuxer::ActivatePullMode()
{
    MEDIA_LOG_I("ActivatePullMode called");
    InitTypeFinder();
    checkRange_ = [](uint64_t offset, uint32_t size) {
        return true;
    };
    peekRange_ = [this](uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> bool {
        auto ret = source_->PullData(offset, size, bufferPtr);
        return ret == Status::OK;
    };
    getRange_ = peekRange_;
    typeFinder_->Init(uri_, mediaDataSize_, checkRange_, peekRange_);
    std::string type = typeFinder_->FindMediaType();
    FALSE_RETURN_MSG(!type.empty(), "Find media type failed");
    MEDIA_LOG_I("PullMode FindMediaType result : type : " PUBLIC_LOG_S ", uri_ : %{private}s, mediaDataSize_ : "
        PUBLIC_LOG_U64, type.c_str(), uri_.c_str(), mediaDataSize_);
    MediaTypeFound(std::move(type));
}

void MediaDemuxer::ActivatePushMode()
{
    MEDIA_LOG_I("ActivatePushMode called");
    InitTypeFinder();
    checkRange_ = [this](uint64_t offset, uint32_t size) {
        return !dataPacker_->IsEmpty(); // True if there is some data
    };
    peekRange_ = [this](uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> bool {
        return dataPacker_->PeekRange(offset, size, bufferPtr);
    };
    getRange_ = [this](uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> bool {
        // In push mode, ignore offset, always get data from the start of the data packer.
        return dataPacker_->GetRange(size, bufferPtr);
    };
    typeFinder_->Init(uri_, mediaDataSize_, checkRange_, peekRange_);
    std::string type = typeFinder_->FindMediaType();
    MEDIA_LOG_I("PushMode FindMediaType result : type : " PUBLIC_LOG_S ", uri_ : %{private}s, mediaDataSize_ : "
                        PUBLIC_LOG_U64, type.c_str(), uri_.c_str(), mediaDataSize_);
    MediaTypeFound(std::move(type));
}

void MediaDemuxer::MediaTypeFound(std::string pluginName)
{
    if (!InitPlugin(std::move(pluginName))) {
        MEDIA_LOG_E("MediaTypeFound init plugin error.");
    }
}

void MediaDemuxer::InitMediaMetaData(const Plugins::MediaInfo& mediaInfo)
{
    mediaMetaData_.globalMeta = std::make_shared<Meta>(mediaInfo.general);
    mediaMetaData_.trackMetas.clear();
    mediaMetaData_.trackMetas.reserve(mediaInfo.tracks.size());
    for (auto& trackMeta : mediaInfo.tracks) {
        mediaMetaData_.trackMetas.emplace_back(std::make_shared<Meta>(trackMeta));
    }
}

bool MediaDemuxer::IsOffsetValid(int64_t offset) const
{
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        return mediaDataSize_ == 0 || offset <= static_cast<int64_t>(mediaDataSize_);
    }
    return true;
}

bool MediaDemuxer::GetBufferFromUserQueue(uint32_t queueIndex, int32_t size)
{
    MEDIA_LOG_I("Get buffer from user queue " PUBLIC_LOG_D32 ".", queueIndex);
    FALSE_RETURN_V_MSG_E(bufferQueueMap_.count(queueIndex) > 0 && bufferQueueMap_[queueIndex] != nullptr, false,
        "bufferQueue " PUBLIC_LOG_D32 " is nullptr", queueIndex);

    AVBufferConfig avBufferConfig;
    avBufferConfig.capacity = size;
    Status ret = bufferQueueMap_[queueIndex]->RequestBuffer(bufferMap_[queueIndex],
        avBufferConfig, REQUEST_BUFFER_TIMEOUT);
    if (ret != Status::OK) {
        MEDIA_LOG_I("Get buffer failed due to get buffer from bufferQueue failed, ret=" PUBLIC_LOG_D32,
            (int32_t)(ret));
        return false;
    }
    return true;
}

Status MediaDemuxer::CopyFrameToUserQueue(uint32_t trackId)
{
    MEDIA_LOG_D("Copy one loop, trackId=" PUBLIC_LOG_U32, trackId);
    Status ret;
    uint32_t size = plugin_->GetNextSampleSize(trackId);
    if (size == 0) {
        MEDIA_LOG_D("No more cache in track " PUBLIC_LOG_U32, trackId);
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (!GetBufferFromUserQueue(trackId, size)) {
        MEDIA_LOG_D("Get buffer from queue failed ");
        return Status::ERROR_INVALID_PARAMETER;
    }

    ret = InnerReadSample(trackId, bufferMap_[trackId]);
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        if (bufferMap_[trackId]->flag_ & (uint32_t)(AVBufferFlag::EOS)) {
            eosMap_[trackId] = true;
        }
        ret = bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], true);
    }
    return Status::OK;
}

Status MediaDemuxer::InnerReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    MEDIA_LOG_D("copy frame for track " PUBLIC_LOG_U32, trackId);
    Status ret = plugin_->ReadSample(trackId, sample);
    if (ret == Status::END_OF_STREAM) {
        MEDIA_LOG_I("Read eos buffer for track " PUBLIC_LOG_U32, trackId);
    } else if (ret != Status::OK) {
        MEDIA_LOG_I("Read buffer for track " PUBLIC_LOG_U32 " error, ret=" PUBLIC_LOG_D32, trackId, (uint32_t)(ret));
    }
    MEDIA_LOG_D("finish copy frame for track " PUBLIC_LOG_U32, trackId);
    // to get DrmInfo
    return ret;
}

void MediaDemuxer::ReadLoop(uint32_t trackId)
{
    std::string threadReadName = "DemuxerRLoop" + std::to_string(trackId);
    MEDIA_LOG_I("Enter [" PUBLIC_LOG_S "] read thread.", threadReadName.c_str());
    pthread_setname_np(pthread_self(), threadReadName.c_str());
    for (;;) {
        if (isThreadExit_) {
            MEDIA_LOG_I("Exit [" PUBLIC_LOG_S "] read thread.", threadReadName.c_str());
            break;
        }
        if (eosMap_[trackId]) {
            MEDIA_LOG_I("Exit [" PUBLIC_LOG_S "] read thread, all track reach eos.", threadReadName.c_str());
            break;
        }
        (void)CopyFrameToUserQueue(trackId);
    }
}

Status MediaDemuxer::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE,
        "Cannot call read frame when use buffer queue.");
    MEDIA_LOG_D("Read next sample");
    FALSE_RETURN_V_MSG_E(eosMap_.count(trackId) > 0, Status::ERROR_INVALID_OPERATION,
        "Read sample failed due to track has not been selected");
    if (eosMap_[trackId]) {
        MEDIA_LOG_W("Read sample failed due to track has reached eos");
        sample->flag_ = (uint32_t)(AVBufferFlag::EOS);
        return Status::END_OF_STREAM;
    }
    Status ret = InnerReadSample(trackId, sample);
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        if (sample->flag_ & (uint32_t)(AVBufferFlag::EOS)) {
            eosMap_[trackId] = true;
        }
        if (sample->flag_ & (uint32_t)(AVBufferFlag::PARTIAL_FRAME)) {
            ret = Status::ERROR_NO_MEMORY;
        }
    }
    isThreadExit_ = true;
    return ret;
}

void MediaDemuxer::OnEvent(const Plugins::PluginEvent &event)
{
    MEDIA_LOG_D("OnEvent");
}
} // namespace Media
} // namespace OHOS