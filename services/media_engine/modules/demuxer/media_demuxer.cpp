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

#include "native/media_demuxer.h"
#include "src/modules/source/source.h"
#include <algorithm>
#include "cpp_ext/type_traits_ext.h"
#include "buffer/avallocator.h"
#include "common/log.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_info.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "plugin/plugin_manager.h"
#include "plugin/demuxer_plugin.h"
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
      mediaMetaData_()
{
    dataPacker_ = std::make_shared<DataPacker>();
    source_ = std::make_shared<Source>();
    MEDIA_LOG_I("MediaDemuxer called");
}

MediaDemuxer::~MediaDemuxer()
{
    MEDIA_LOG_I("~MediaDemuxer called");
    if (plugin_) {
        plugin_->Deinit();
    }
}

Status MediaDemuxer::SetDataSource(const std::shared_ptr<MediaSource> &source)
{
    source_->SetSource(source);

    seekable_ = source_->GetSeekable();
    if (seekable_ == Plugin::Seekable::SEEKABLE) {
        Flush();
        ActivatePullMode();
    } else {
        ActivatePushMode();
    }
    MediaInfo mediaInfo;
    Status ret = plugin_->GetMediaInfo(mediaInfo);
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
    return plugin_->SetOutputBufferQueue(trackId, producer);
}

Status MediaDemuxer::SelectTrack(int32_t trackId)
{
    return plugin_->SelectTrack(trackId);
}

Status MediaDemuxer::UnselectTrack(int32_t trackId)
{
    return plugin_->UnselectTrack(trackId);
}

Status MediaDemuxer::SeekTo(int64_t seekTime, Plugin::SeekMode mode, int64_t& realSeekTime)
{
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
    mediaMetaData_.globalMeta.reset();
    mediaMetaData_.trackMetas.clear();
    return plugin_->Reset();
}

Status MediaDemuxer::Start()
{
    dataPacker_->Start();
    return plugin_->Start();
}

Status MediaDemuxer::Stop()
{
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

void MediaDemuxer::OnEvent(const Plugin::PluginEvent &event)
{
    MEDIA_LOG_D("OnEvent");
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
    MEDIA_LOG_D("ActivatePullMode called");
    InitTypeFinder();
    checkRange_ = [this](uint64_t offset, uint32_t size) {
        uint64_t curOffset = offset;
        if (dataPacker_->IsDataAvailable(offset, size, curOffset)) {
            return true;
        }
        MEDIA_LOG_DD("IsDataAvailable false, require offset " PUBLIC_LOG_D64 ", DataPacker data offset end - curOffset "
                             PUBLIC_LOG_D64, offset, curOffset);
        std::shared_ptr<Buffer> bufferPtr = std::make_shared<Buffer>();
        auto ret = source_->PullData(curOffset, size, bufferPtr);
        if (ret == Status::OK) {
            dataPacker_->PushData(bufferPtr, curOffset);
            return true;
        } else if (ret == Status::END_OF_STREAM) {
            // hasDataToRead is ture if there is some data in data packer can be read.
            bool hasDataToRead = offset < curOffset && (!dataPacker_->IsEmpty());
            if (hasDataToRead) {
                dataPacker_->SetEos();
            } else {
                dataPacker_->Flush();
            }
            return hasDataToRead;
        } else {
            MEDIA_LOG_E("PullData from source filter failed " PUBLIC_LOG_D32, ret);
        }
        return false;
    };
    peekRange_ = [this](uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> bool {
        if (checkRange_(offset, size)) {
            return dataPacker_->PeekRange(offset, size, bufferPtr);
        }
        return false;
    };
    getRange_ = [this](uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> bool {
        if (checkRange_(offset, size)) {
            auto ret = dataPacker_->GetRange(offset, size, bufferPtr);
            return ret;
        }
        return false;
    };
    typeFinder_->Init(uri_, mediaDataSize_, checkRange_, peekRange_);
    std::string type = typeFinder_->FindMediaType();
    MEDIA_LOG_I("FindMediaType result : type : " PUBLIC_LOG_S ", uri_ : " PUBLIC_LOG_S ", mediaDataSize_ : "
                        PUBLIC_LOG_U64, type.c_str(), uri_.c_str(), mediaDataSize_);
    MediaTypeFound(std::move(type));
}

void MediaDemuxer::ActivatePushMode()
{
    MEDIA_LOG_D("ActivatePushMode called");
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
} // namespace Media
} // namespace OHOS