/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "source/source.h"
#include <algorithm>
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

namespace OHOS {
namespace Media {
class MediaDemuxer::DataSourceImpl : public Plugin::DataSource {
public:
    explicit DataSourceImpl(const MediaDemuxer& demuxer);
    ~DataSourceImpl() override = default;
    Status ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen) override;
    Status GetSize(uint64_t& size) override;
    Plugin::Seekable GetSeekable() override;

private:
    const MediaDemuxer& demuxer;
};

MediaDemuxer::DataSourceImpl::DataSourceImpl(const MediaDemuxer& demuxer) : demuxer(demuxer)
{
}

/**
* ReadAt Plugin::DataSource::ReadAt implementation.
* @param offset offset in media stream.
* @param buffer caller allocate real buffer.
* @param expectedLen buffer size wanted to read.
* @return read result.
*/
Status MediaDemuxer::DataSourceImpl::ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer,
    size_t expectedLen)
{
    if (!buffer || expectedLen == 0 || !demuxer.IsOffsetValid(offset)) {
        MEDIA_LOG_E("ReadAt failed, buffer empty: " PUBLIC_LOG_D32 ", expectedLen: " PUBLIC_LOG_D32
                            ", offset: " PUBLIC_LOG_D64, !buffer, static_cast<int>(expectedLen), offset);
        return Status::ERROR_UNKNOWN;
    }
    Status rtv = Status::OK;
    switch (demuxer.pluginState_.load()) {
        case DemuxerState::DEMUXER_STATE_NULL:
            rtv = Status::ERROR_WRONG_STATE;
            MEDIA_LOG_E("ReadAt error due to DEMUXER_STATE_NULL");
            break;
        case DemuxerState::DEMUXER_STATE_PARSE_HEADER: {
            if (demuxer.getRange_(static_cast<uint64_t>(offset), expectedLen, buffer)) {
                DUMP_BUFFER2FILE(DEMUXER_INPUT_PEEK, buffer);
            } else {
                rtv = Status::ERROR_NOT_ENOUGH_DATA;
            }
            break;
        }
        case DemuxerState::DEMUXER_STATE_PARSE_FRAME: {
            if (demuxer.getRange_(static_cast<uint64_t>(offset), expectedLen, buffer)) {
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
    size = demuxer.mediaDataSize_;
    return (demuxer.mediaDataSize_ > 0) ? Status::OK : Status::ERROR_WRONG_STATE;
}

Plugin::Seekable MediaDemuxer::DataSourceImpl::GetSeekable()
{
    return demuxer.seekable_;
}

class MediaDemuxer::PushDataImpl {
public:
    explicit PushDataImpl(const MediaDemuxer& demuxer);
    ~PushDataImpl() = default;
    Status PushData(std::shared_ptr<Buffer>& buffer, int64_t offset);
private:
    const MediaDemuxer& demuxer;
};

MediaDemuxer::PushDataImpl::PushDataImpl(const MediaDemuxer& demuxer) : demuxer(demuxer)
{
}

Status MediaDemuxer::PushDataImpl::PushData(std::shared_ptr<Buffer>& buffer, int64_t offset)
{
    if (buffer->flag & BUFFER_FLAG_EOS) {
        demuxer.dataPacker_->SetEos();
    } else {
        demuxer.dataPacker_->PushData(buffer, offset);
    }
    return Status::OK;
}

MediaDemuxer::MediaDemuxer()
    : seekable_(Plugin::Seekable::INVALID),
      uri_(),
      mediaDataSize_(0),
      typeFinder_(nullptr),
      dataPacker_(nullptr),
      pluginName_(),
      plugin_(nullptr),
      dataSource_(std::make_shared<DataSourceImpl>(*this)),
      mediaMetaData_(),
      bufferQueueMap_(),
      bufferMap_(),
      readThread_()
{
    dataPacker_ = std::make_shared<DataPacker>();
    source_ = std::make_shared<Source>();
    MEDIA_LOG_I("MediaDemuxer called");
}

MediaDemuxer::~MediaDemuxer()
{
    MEDIA_LOG_I("~MediaDemuxer called");
    bufferQueueMap_.clear();
    bufferMap_.clear();
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
    bufferQueueMap_.clear();
    bufferMap_.clear();
    eosMap_.clear();
}

Status MediaDemuxer::SetDataSource(const std::shared_ptr<MediaSource> &source)
{
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    source_->SetSource(source);
    Status ret = source_->GetSize(mediaDataSize_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set data source failed due to get file size failed.");
    seekable_ = source_->GetSeekable();
    if (seekable_ == Plugin::Seekable::SEEKABLE) {
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
    return ret;
}

Status MediaDemuxer::SetOutputBufferQueue(int32_t trackId, const sptr<AVBufferQueueProducer>& producer)
{
    std::unique_lock<std::mutex> lock(mutex_);
    useBufferQueue_ = true;
    MEDIA_LOG_I("Set bufferQueue for track " PUBLIC_LOG_D32 ".", trackId);
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    FALSE_RETURN_V_MSG_E(producer != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set bufferQueue failed due to input bufferQueue is nullptr.");
    if (bufferQueueMap_.count(trackId) != 0) {
        MEDIA_LOG_W("BufferQueue has been set already, will be overwritten.");
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
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot select track when use buffer queue.");
    return InnerSelectTrack(trackId);
}

Status MediaDemuxer::UnselectTrack(int32_t trackId)
{
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot unselect track when use buffer queue.");
    return plugin_->UnselectTrack(trackId);
}

Status MediaDemuxer::SeekTo(int64_t seekTime, Plugin::SeekMode mode, int64_t& realSeekTime)
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
    return rtv;
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
    bufferQueueMap_.clear();
    bufferMap_.clear();
    return plugin_->Reset();
}

Status MediaDemuxer::Start()
{
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    dataPacker_->Start();
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Start read failed due to has not set data source.");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::OK,
        "Process has been started already, neet to stop it first.");
    FALSE_RETURN_V_MSG_E(readThread_ == nullptr, Status::OK, "Read task is running already.");
    threadReadName_ = "DemuxerReadLoop";
    isThreadExit_ = false;
    readThread_ = std::make_unique<std::thread>(&MediaDemuxer::ReadLoop, this);
    MEDIA_LOG_I("Demuxer read thread started.");
    return plugin_->Start();
}

Status MediaDemuxer::Stop()
{
    FALSE_RETURN_V_MSG_E(useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot reset track when not use buffer queue.");
    dataPacker_->Stop();
    FALSE_RETURN_V_MSG_E(!isThreadExit_, Status::OK, "Process has been stopped already, need to start if first.");
    FALSE_RETURN_V_MSG_E(std::this_thread::get_id() != readThread_->get_id(),
        Status::ERROR_INVALID_PARAMETER, "Stop at the copy task thread, reject.");
    isThreadExit_ = true;
    std::unique_ptr<std::thread> t;
    std::swap(readThread_, t);
    if (t != nullptr && t->joinable()) {
        t->join();
    }
    readThread_ = nullptr;
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
    plugin_ = std::static_pointer_cast<Plugin::DemuxerPlugin>(
            Plugin::PluginManager::Instance().CreatePlugin(pluginName, Plugin::PluginType::DEMUXER));
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
    checkRange_ = [this](uint64_t offset, uint32_t size) {
        (void)this;
        return true;
    };
    peekRange_ = [this](uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> bool {
        auto ret = source_->PullData(offset, size, bufferPtr);
        return ret == Status::OK;
    };
    getRange_ = peekRange_;
    typeFinder_->Init(uri_, mediaDataSize_, checkRange_, peekRange_);
    std::string type = typeFinder_->FindMediaType();
    MEDIA_LOG_I("FindMediaType result : type : " PUBLIC_LOG_S ", uri_ : " PUBLIC_LOG_S ", mediaDataSize_ : "
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
    typeFinder_->FindMediaTypeAsync([this](std::string pluginName) { MediaTypeFound(std::move(pluginName)); });
}

void MediaDemuxer::MediaTypeFound(std::string pluginName)
{
    if (!InitPlugin(std::move(pluginName))) {
        MEDIA_LOG_E("MediaTypeFound init plugin error.");
    }
}

void MediaDemuxer::InitMediaMetaData(const Plugin::MediaInfo& mediaInfo)
{
    mediaMetaData_.globalMeta = std::make_shared<Meta>(mediaInfo.general);
    mediaMetaData_.trackMetas.clear();
    int trackCnt = 0;
    for (auto& trackMeta : mediaInfo.tracks) {
        mediaMetaData_.trackMetas.push_back(std::make_shared<Meta>(trackMeta));
        if (!trackMeta.Empty()) {
            ++trackCnt;
        }
    }
    mediaMetaData_.trackMetas.reserve(trackCnt);
}

bool MediaDemuxer::IsOffsetValid(int64_t offset) const
{
    if (seekable_ == Plugin::Seekable::SEEKABLE) {
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
    
    Status ret = bufferQueueMap_[queueIndex]->RequestBuffer(bufferMap_[queueIndex], avBufferConfig, 10000);
    if (ret != Status::OK) {
        MEDIA_LOG_I("Get buffer failed due to get buffer from bufferQueue failed, ret=" PUBLIC_LOG_D32, (int32_t)(ret));
        return false;
    }
    return true;
}

Status MediaDemuxer::CopyFrameToUserQueue()
{
    MEDIA_LOG_D("Copy one loop");
    Status ret;
    for (auto item : bufferQueueMap_) {
        uint32_t trackId = item.first;
        if (eosMap_[trackId]) {
            continue;
        }
        uint32_t size = plugin_->GetNextSampleSize(trackId);
        if (size == 0) {
            MEDIA_LOG_D("No more cache in track " PUBLIC_LOG_U32, trackId);
            continue;
        }

        if (!GetBufferFromUserQueue(trackId, size)) {
            MEDIA_LOG_D("Get buffer from queue failed ");
            continue;
        }

        ret = InnerReadSample(trackId, bufferMap_[trackId]);
        if (ret == Status::OK || ret == Status::END_OF_STREAM) {
            if (bufferMap_[trackId]->flag_ & (uint32_t)(AVBufferFlag::EOS)) {
                eosMap_[trackId] = true;
            }
            ret = bufferQueueMap_[trackId]->PushBuffer(bufferMap_[trackId], true);
        }
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
    MEDIA_LOG_D("OK copy frame for track " PUBLIC_LOG_U32, trackId);
    return ret;
}

bool MediaDemuxer::AllTrackReachEOS()
{
    bool allEOS = true;
    for (auto item : eosMap_) {
        if (!item.second) {
            allEOS = false;
        }
    }
    return allEOS;
}

void MediaDemuxer::ReadLoop()
{
    MEDIA_LOG_I("Enter [" PUBLIC_LOG_S "] read thread.", threadReadName_.c_str());
    constexpr uint32_t nameSizeMax = 25;
    pthread_setname_np(pthread_self(), threadReadName_.substr(0, nameSizeMax).c_str());
    for (;;) {
        if (isThreadExit_) {
            MEDIA_LOG_I("Exit [" PUBLIC_LOG_S "] read thread.", threadReadName_.c_str());
            break;
        }
        if (AllTrackReachEOS()) {
            MEDIA_LOG_I("Exit [" PUBLIC_LOG_S "] read thread, all track reach eos.", threadReadName_.c_str());
            break;
        }
        (void)CopyFrameToUserQueue();
    }
}

Status MediaDemuxer::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    FALSE_RETURN_V_MSG_E(!useBufferQueue_, Status::ERROR_WRONG_STATE, "Cannot call read frame when use buffer queue.");
    MEDIA_LOG_D("Read next sample");
    FALSE_RETURN_V_MSG_E(eosMap_.count(trackId) > 0, Status::ERROR_INVALID_PARAMETER, 
        "Read sample failed due to track has not been selected");
    FALSE_RETURN_V_MSG_E(!eosMap_[trackId], Status::END_OF_STREAM, 
        "Read sample failed due to track has reached eos");
    Status ret = InnerReadSample(trackId, sample);
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        if (sample->flag_ & (uint32_t)(AVBufferFlag::EOS)) {
            eosMap_[trackId] = true;
        }
    }
    isThreadExit_ = true;
    return ret;
}
} // namespace Media
} // namespace OHOS