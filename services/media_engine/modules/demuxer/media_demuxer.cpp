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
    auto demuxer = demuxer_.lock();
    FALSE_RETURN_V(demuxer != nullptr, Status::ERROR_NULL_POINTER);
    if (buffer->flag & BUFFER_FLAG_EOS) {
        demuxer->SetEos();
    } else {
        demuxer->PushData(buffer, offset);
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
      eventReceiver_(),
      cacheData_()
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
    cacheData_.data = nullptr;
    cacheData_.offset = 0;
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

bool MediaDemuxer::IsDrmInfosUpdate(const std::multimap<std::string, std::vector<uint8_t>> &info)
{
    MEDIA_LOG_I("IsDrmInfosUpdate");
    bool isUpdated = false;
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
    MEDIA_LOG_I("ProcessDrmInfos");
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "ProcessDrmInfos failed due to create demuxer plugin failed.");

    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    Status ret = plugin_->GetDrmInfo(drmInfo);
    if (ret == Status::OK && !drmInfo.empty()) {
        MEDIA_LOG_I("demuxer filter get drminfo success");
        bool isUpdated = IsDrmInfosUpdate(drmInfo);
        if (isUpdated) {
            return ReportDrmInfos(drmInfo);
        } else {
            MEDIA_LOG_D("demuxer filter received drminfo but not update");
        }
    } else {
        MEDIA_LOG_D("demuxer filter get drminfo failed or no drm info, ret=" PUBLIC_LOG_D32, (int32_t)(ret));
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
    eventReceiver_->OnEvent(
        {"media_demuxer", EventType::EVENT_IS_LIVE_STREAM, seekable_ != Plugins::Seekable::SEEKABLE});
}

Status MediaDemuxer::SetDataSource(const std::shared_ptr<MediaSource> &source)
{
    MEDIA_LOG_I("SetDataSource enter");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    source_->SetCallback(this);
    source_->SetSource(source);
    std::shared_ptr<PushDataImpl> pushData_ = std::make_shared<PushDataImpl>(shared_from_this());
    source_->SetPushData(pushData_);
    Status ret = source_->GetSize(mediaDataSize_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set data source failed due to get file size failed.");
    seekable_ = source_->GetSeekable();
    ReportIsLiveStreamEvent();
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
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
        MEDIA_LOG_E("demuxer filter parse meta failed, ret: " PUBLIC_LOG_D32, (int32_t)(ret));
    }

    ProcessDrmInfos();
    MEDIA_LOG_I("SetDataSource exit");
    return ret;
}

Status MediaDemuxer::SetOutputBufferQueue(int32_t trackId, const sptr<AVBufferQueueProducer>& producer)
{
    std::unique_lock<std::mutex> lock(mutex_);
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

Status MediaDemuxer::InnerSelectTrack(int32_t trackId)
{
    eosMap_[trackId] = false;
    return plugin_->SelectTrack(trackId);
}

Status MediaDemuxer::SelectTrack(int32_t trackId)
{
    FALSE_RETURN_V_MSG_E(trackId >= 0 && (uint32_t)trackId < mediaMetaData_.trackMetas.size(),
        Status::ERROR_INVALID_PARAMETER, "Select trackId error.");
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
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_OPERATION, "SeekTo error seekTime: " PUBLIC_LOG_D64
        ", mode: " PUBLIC_LOG_U32, seekTime, (uint32_t)mode);
    lastSeekTime_ = seekTime;
    Status ret = Status::ERROR_INVALID_OPERATION;
    if (source_ != nullptr && source_->IsSeekToTimeSupported()) {
        MEDIA_LOG_I("SeekTo source SeekToTime start");
        ret = source_->SeekToTime(seekTime);
        Plugins::Ms2HstTime(seekTime, realSeekTime);
    } else {
        MEDIA_LOG_I("SeekTo start");
        ret = plugin_->SeekTo(-1, seekTime, mode, realSeekTime);
    }
    isSeeked_ = true;
    lastSeekTime_ = Plugins::HST_TIME_NONE;
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
    cacheData_.data = nullptr;
    cacheData_.offset = 0;
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

bool MediaDemuxer::PullDataWithCache(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr)
{
    if (cacheData_.data == nullptr || cacheData_.data->GetMemory() == nullptr ||
        offset < cacheData_.offset || offset >= (cacheData_.offset + cacheData_.data->GetMemory()->GetSize())) {
        return false;
    }
    auto memory = cacheData_.data->GetMemory();
    if (memory == nullptr || memory->GetSize() <= 0) {
        return false;
    }

    MEDIA_LOG_I("PullMode, Read data from cache data.");
    uint64_t offsetInCache = offset - cacheData_.offset;
    if (size <= memory->GetSize() - offsetInCache) {
        bufferPtr->GetMemory()->Write(memory->GetReadOnlyData() + offsetInCache, size, 0);
        return true;
    }
    bufferPtr->GetMemory()->Write(memory->GetReadOnlyData() + offsetInCache, memory->GetSize() - offsetInCache, 0);
    uint64_t remainOffset = cacheData_.offset + memory->GetSize();
    uint64_t remainSize = size - (memory->GetSize() - offsetInCache);
    std::shared_ptr<Buffer> tempBuffer = Buffer::CreateDefaultBuffer(remainSize);
    if (tempBuffer == nullptr || tempBuffer->GetMemory() == nullptr) {
        MEDIA_LOG_W("PullMode, Read data from cache data. only get partial data.");
        return true;
    }
    Status ret = source_->PullData(remainOffset, lastSeekTime_, remainSize, tempBuffer);
    if (ret == Status::OK) {
        bufferPtr->GetMemory()->Write(tempBuffer->GetMemory()->GetReadOnlyData(),
            tempBuffer->GetMemory()->GetSize(), memory->GetSize() - offsetInCache);
        std::shared_ptr<Buffer> mergedBuffer = Buffer::CreateDefaultBuffer(
            tempBuffer->GetMemory()->GetSize() + memory->GetSize());
        if (mergedBuffer == nullptr || mergedBuffer->GetMemory() == nullptr) {
            MEDIA_LOG_W("PullMode, Read data from cache data success. update cache data fail.");
            return true;
        }
        mergedBuffer->GetMemory()->Write(memory->GetReadOnlyData(), memory->GetSize(), 0);
        mergedBuffer->GetMemory()->Write(tempBuffer->GetMemory()->GetReadOnlyData(),
            tempBuffer->GetMemory()->GetSize(), memory->GetSize());
        cacheData_.data = mergedBuffer;
        MEDIA_LOG_I("PullMode, offset: " PUBLIC_LOG_U64 ", cache offset: " PUBLIC_LOG_U64
            ", cache size: " PUBLIC_LOG_ZU, offset, cacheData_.offset,
            cacheData_.data->GetMemory()->GetSize());
    }
    return ret == Status::OK;
}

bool MediaDemuxer::PullDataWithoutCache(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr)
{
    Status ret = source_->PullData(offset, lastSeekTime_, size, bufferPtr);
    if ((cacheData_.data == nullptr || cacheData_.data->GetMemory() == nullptr) && ret == Status::OK) {
        MEDIA_LOG_I("PullMode, write cache data.");
        if (bufferPtr->GetMemory() == nullptr) {
            MEDIA_LOG_W("PullMode, write cache data error. memory is nullptr!");
        } else {
            auto buffer = Buffer::CreateDefaultBuffer(bufferPtr->GetMemory()->GetSize());
            if (buffer != nullptr && buffer->GetMemory() != nullptr) {
                buffer->GetMemory()->Write(bufferPtr->GetMemory()->GetReadOnlyData(),
                    bufferPtr->GetMemory()->GetSize(), 0);
                cacheData_.data = buffer;
                cacheData_.offset = offset;
                MEDIA_LOG_I("PullMode, write cache data success.");
            } else {
                MEDIA_LOG_W("PullMode, write cache data failed. memory is nullptr!");
            }
        }
    }
    return ret == Status::OK;
}

void MediaDemuxer::ActivatePullMode()
{
    MEDIA_LOG_I("ActivatePullMode called");
    InitTypeFinder();
    checkRange_ = [](uint64_t offset, uint32_t size) {
        return true;
    };
    peekRange_ = [this](uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> bool {
        if (pluginState_.load() == DemuxerState::DEMUXER_STATE_PARSE_FRAME) {
            MEDIA_LOG_D("PullMode, DemuxerState::DEMUXER_STATE_PARSE_FRAME");
            return Status::OK == source_->PullData(offset, lastSeekTime_, size, bufferPtr);
        }
        MEDIA_LOG_D("PullMode, offset: " PUBLIC_LOG_U64 ", cache offset: " PUBLIC_LOG_U64
            ", cache data: " PUBLIC_LOG_D32, offset, cacheData_.offset, (int32_t)(cacheData_.data != nullptr));
        if (cacheData_.data != nullptr && cacheData_.data->GetMemory() != nullptr &&
            offset >= cacheData_.offset && offset < (cacheData_.offset + cacheData_.data->GetMemory()->GetSize())) {
            auto memory = cacheData_.data->GetMemory();
            if (memory != nullptr && memory->GetSize() > 0) {
                MEDIA_LOG_I("PullMode, Read data from cache data.");
                return PullDataWithCache(offset, size, bufferPtr);
            }
        }
        return PullDataWithoutCache(offset, size, bufferPtr);
    };
    getRange_ = peekRange_;
    typeFinder_->Init(uri_, mediaDataSize_, checkRange_, peekRange_);
    std::string type = typeFinder_->FindMediaType();
    FALSE_RETURN_MSG(!type.empty(), "Find media type failed");
    MEDIA_LOG_I("PullMode FindMediaType result type: " PUBLIC_LOG_S ", uri: %{private}s, mediaDataSize: "
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
        if (trackMeta.Get<Tag::MIME_TYPE>(mimeType) && mimeType.find("video") == 0) {
            MEDIA_LOG_I("Found video track, id: " PUBLIC_LOG_U32 ", mimeType: " PUBLIC_LOG_S, index, mimeType.c_str());
            videoMime_ = mimeType;
            videoTrackId_ = index;
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
    Status ret;
    uint32_t size = plugin_->GetNextSampleSize(trackId);
    FALSE_RETURN_V_MSG_E(size != 0, Status::ERROR_INVALID_PARAMETER, "No more cache in track " PUBLIC_LOG_U32, trackId);
    FALSE_RETURN_V_MSG_E(GetBufferFromUserQueue(trackId, size), Status::ERROR_INVALID_PARAMETER,
        "Get buffer from queue failed, trackId: " PUBLIC_LOG_U32, trackId);

    ret = InnerReadSample(trackId, bufferMap_[trackId]);
    if (source_ != nullptr && source_->IsSeekToTimeSupported() && isSeeked_) {
        if (trackId != videoTrackId_ || ret != Status::OK ||
            !IsContainIdrFrame(bufferMap_[trackId]->memory_->GetAddr(), bufferMap_[trackId]->memory_->GetSize())) {
            MEDIA_LOG_I("CopyFrameToUserQueue is seeking, not found idr frame. trackId: " PUBLIC_LOG_U32, trackId);
            bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], false);
            return Status::ERROR_INVALID_PARAMETER;
        }
        MEDIA_LOG_I("CopyFrameToUserQueue is seeking, found idr frame. trackId: " PUBLIC_LOG_U32, trackId);
        isSeeked_ = false;
    }
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        if (bufferMap_[trackId]->flag_ & (uint32_t)(AVBufferFlag::EOS)) {
            eosMap_[trackId] = true;
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
            MEDIA_LOG_I("Exit [" PUBLIC_LOG_S "] read thread, track reach eos, trackId: " PUBLIC_LOG_U32,
                threadReadName.c_str(), trackId);
            break;
        }
        (void)CopyFrameToUserQueue(trackId);
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
        default:
            break;
    }
}
} // namespace Media
} // namespace OHOS