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

#define HST_LOG_TAG "LiveDataSourceStreamDemuxer"

#include "live_datasource_stream_demuxer.h"

#include <algorithm>
#include <map>
#include <memory>

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
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_manager.h"
#include "plugin/plugin_time.h"
#include "source/source.h"

namespace OHOS {
namespace Media {
constexpr size_t DEFAULT_READ_SIZE = 4096;
const int32_t READ_LOOP_RETRY_TIMES = 15;
LiveDataSourceStreamDemuxer::LiveDataSourceStreamDemuxer()
    : dataPacker_(std::make_shared<DataPacker>()),
    taskPtr_(nullptr),
    mediaOffset_(0)

{
    MEDIA_LOG_I("LiveDataSourceStreamDemuxer called");
    dataPacker_->Start();
}

LiveDataSourceStreamDemuxer::~LiveDataSourceStreamDemuxer()
{
    MEDIA_LOG_I("~LiveDataSourceStreamDemuxer called");
    Stop();
    dataPacker_ = nullptr;
    taskPtr_ = nullptr;
}

Status LiveDataSourceStreamDemuxer::Init(std::string uri)
{
    dataPacker_->IsSupportPreDownload(source_->IsNeedPreDownload());
    if (taskPtr_ == nullptr) {
        taskPtr_ = std::make_shared<Task>("DataReader", "", TaskType::SINGLETON);
        taskPtr_->RegisterJob([this] {
            ReadLoop();
            return 0;
        });
    }
    MEDIA_LOG_I("Init task start");
    taskPtr_->Start();

    MediaAVCodec::AVCodecTrace trace("LiveDataSourceStreamDemuxer::Init");
    MEDIA_LOG_I("LiveDataSourceStreamDemuxer::Init called");
    checkRange_ = [this](int32_t streamID, uint64_t offset, uint32_t size) {
        (void)streamID;
        return !dataPacker_->IsEmpty(); // True if there is some data
    };
    peekRange_ = [this](int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> bool {
        (void)streamID;
        return dataPacker_->PeekRange(offset, size, bufferPtr);
    };
    getRange_ = [this](int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> bool {
        // In push mode, ignore offset, always get data from the start of the data packer.
        bool isRemove = (source_->IsNeedPreDownload() && source_->GetSeekable() == Plugins::Seekable::UNSEEKABLE
            && pluginStateMap_[streamID] == DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
        return dataPacker_->GetRange(size, bufferPtr, offset, isRemove);
    };
    uri_ = uri;
    return Status::OK;
}

Status LiveDataSourceStreamDemuxer::PushData(std::shared_ptr<Plugins::Buffer>& buffer, uint64_t offset)
{
    if (buffer->flag & BUFFER_FLAG_EOS) {
        dataPacker_->SetEos();
    } else {
        dataPacker_->PushData(buffer, offset);
    }
    return Status::OK;
}

void LiveDataSourceStreamDemuxer::ReadLoop()
{
    std::shared_ptr<Plugins::Buffer> data = std::make_shared<Plugins::Buffer>();
    Status err = source_->Read(-1, data, -1, DEFAULT_READ_SIZE);
    if (err == Status::END_OF_STREAM) {
        MEDIA_LOG_I("ReadLoop current is end of stream");
        if (taskPtr_) {
            taskPtr_->StopAsync();
        }
        data->flag |= BUFFER_FLAG_EOS;
        PushData(data, (uint64_t)mediaOffset_);
        return;
    }
    if (err != Status::OK) {
        MEDIA_LOG_E("Read data failed (" PUBLIC_LOG_D32 ")", err);
        return;
    }
    auto memory = data->GetMemory();
    auto size = memory->GetSize();
    if (size <= 0) {
        if (retryTimes_ >= READ_LOOP_RETRY_TIMES) {
            MEDIA_LOG_E("ReadLoop retry time reach to max times");
            if (taskPtr_) {
                taskPtr_->StopAsync();
            }
            retryTimes_ = 0;
            data->flag |= BUFFER_FLAG_EOS;
            PushData(data, (uint64_t)mediaOffset_);
        } else {
            retryTimes_++;
            OSAL::SleepFor(1); // Read data failure, sleep 1ms then retry
            return;
        }
    } else {
        retryTimes_ = 0; // Read data success, reset retry times
    }
    if (data->flag & BUFFER_FLAG_EOS) {
        if (taskPtr_) {
            MEDIA_LOG_I("ReadLoop eos buffer, stop task");
            taskPtr_->StopAsync();
        }
    }

    MEDIA_LOG_D("Read data mediaOffset_: " PUBLIC_LOG_D64, mediaOffset_ + size);
    PushData(data, (uint64_t)mediaOffset_);
    mediaOffset_ += static_cast<int64_t>(size);
}

Status LiveDataSourceStreamDemuxer::Reset()
{
    return Status::OK;
}

Status LiveDataSourceStreamDemuxer::Start()
{
    if (dataPacker_) {
        dataPacker_->Start();
    }
    return Status::OK;
}

Status LiveDataSourceStreamDemuxer::Pause()
{
    MEDIA_LOG_I("Pause entered.");
    if (dataPacker_) {
        dataPacker_->Stop();
    }

    mediaOffset_ = 0;
    if (taskPtr_) {
        taskPtr_->Stop();
    }
    return Status::OK;
}

Status LiveDataSourceStreamDemuxer::Resume()
{
    MEDIA_LOG_I("Resume entered.");
    if (dataPacker_) {
        dataPacker_->Start();
    }

    if (taskPtr_) {
        taskPtr_->Start();
    }
    return Status::OK;
}

Status LiveDataSourceStreamDemuxer::Stop()
{
    MEDIA_LOG_I("Stop entered.");
    if (dataPacker_) {
        dataPacker_->Stop();
    }
    mediaOffset_ = 0;
    if (taskPtr_) {
        taskPtr_->Stop();
    }
    return Status::OK;
}

Status LiveDataSourceStreamDemuxer::Flush()
{
    MEDIA_LOG_I("Flush entered.");
    if (dataPacker_) {
        dataPacker_->Flush();
    }
    return Status::OK;
}

Status LiveDataSourceStreamDemuxer::ResetCache(int32_t streamID)
{
    return Status::OK;
}

Status LiveDataSourceStreamDemuxer::ResetAllCache()
{
    return Status::OK;
}

Status LiveDataSourceStreamDemuxer::CallbackReadAt(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
    size_t expectedLen)
{
    switch (pluginStateMap_[streamID]) {
        case DemuxerState::DEMUXER_STATE_NULL:
            return Status::ERROR_WRONG_STATE;
        case DemuxerState::DEMUXER_STATE_PARSE_HEADER: {
            MEDIA_LOG_D("Demuxer parse DEMUXER_STATE_PARSE_HEADER, offset: " PUBLIC_LOG_D64
                ", expectedLen: " PUBLIC_LOG_ZU, offset, expectedLen);

            if (source_->IsNeedPreDownload() && source_->GetSeekable() == Plugins::Seekable::UNSEEKABLE) {
                dataPacker_->GetOrWaitDataAvailable(offset, expectedLen);
                auto ret = peekRange_(streamID, static_cast<uint64_t>(offset), expectedLen, buffer);
                MEDIA_LOG_D("Demuxer parse header, peekRange finish, ret: " PUBLIC_LOG_D32, ret);
                FALSE_RETURN_V(ret, Status::END_OF_STREAM);
            } else if (getRange_(streamID, static_cast<uint64_t>(offset), expectedLen, buffer)) {
                DUMP_BUFFER2FILE(DEMUXER_INPUT_PEEK, buffer);
            } else {
                return Status::ERROR_NOT_ENOUGH_DATA;
            }
            break;
        }
        case DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME:
        case DemuxerState::DEMUXER_STATE_PARSE_FRAME: {
            MEDIA_LOG_D("Demuxer parse DEMUXER_STATE_PARSE_FRAME");
            if (getRange_(streamID, static_cast<uint64_t>(offset), expectedLen, buffer)) {
                DUMP_BUFFER2LOG("Demuxer GetRange", buffer, offset);
                DUMP_BUFFER2FILE(DEMUXER_INPUT_GET, buffer);
                if (isIgnoreParse_.load() && buffer->GetMemory()->GetSize() == 0) {
                    MEDIA_LOG_I("Demuxer parse DEMUXER_STATE_PARSE_FRAME in pausing(isIgnoreParse),"
                                " Read fail and try again");
                    return Status::ERROR_WRONG_STATE;
                } else if (isIgnoreRead_.load() && buffer->GetMemory()->GetSize() == 0) {
                    return Status::ERROR_AGAIN;
                }
            } else {
                MEDIA_LOG_I("Demuxer parse DEMUXER_STATE_PARSE_FRAME, Status::END_OF_STREAM");
                if (pluginStateMap_[streamID] == DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME) {
                    SetDemuxerState(streamID, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
                }
                return Status::END_OF_STREAM;
            }
            if (pluginStateMap_[streamID] == DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME) {
                SetDemuxerState(streamID, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
            }
            break;
        }
        default:
            break;
    }
    return Status::OK;
}

} // namespace Media
} // namespace OHOS
