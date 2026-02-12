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

#define HST_LOG_TAG "BaseStreamDemuxer"

#include "base_stream_demuxer.h"

#include <algorithm>
#include <map>
#include <memory>

#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "cpp_ext/type_traits_ext.h"
#include "buffer/avallocator.h"
#include "common/event.h"
#include "common/log.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_time.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "BaseStreamDemuxer" };
}

namespace OHOS {
namespace Media {

BaseStreamDemuxer::BaseStreamDemuxer()
{
    MEDIA_LOG_D("BaseStreamDemuxer called");
    seekable_ = Plugins::Seekable::UNSEEKABLE;
}

BaseStreamDemuxer::~BaseStreamDemuxer()
{
    MEDIA_LOG_D("~BaseStreamDemuxer called");
}

void BaseStreamDemuxer::SetSource(const std::shared_ptr<Source>& source)
{
    MEDIA_LOG_D("BaseStreamDemuxer::SetSource");
    FALSE_RETURN_MSG(source != nullptr, "source is nullptr");
    source_ = source;
    source_->GetSize(mediaDataSize_);
    seekable_ = source_->GetSeekable();
    isDataSrcNoSeek_ = (seekable_ == Plugins::Seekable::UNSEEKABLE && sourceType_ == SourceType::SOURCE_TYPE_STREAM);
    isDataSrc_ = sourceType_ == SourceType::SOURCE_TYPE_STREAM;
    MEDIA_LOG_I("mediaDataSize_: " PUBLIC_LOG_U64 ", seekable_: " PUBLIC_LOG_D32 ", source_->GetSourceType(): "
        PUBLIC_LOG_D32 ", isDataSrcNoSeek_: " PUBLIC_LOG_D32, mediaDataSize_, seekable_, sourceType_, isDataSrcNoSeek_);
}

bool BaseStreamDemuxer::GetIsDataSrcNoSeek()
{
    return isDataSrcNoSeek_;
}

void BaseStreamDemuxer::SetSourceType(SourceType type)
{
    sourceType_ = type;
}

std::string BaseStreamDemuxer::SnifferMediaType(const StreamInfo& streamInfo)
{
    MediaAVCodec::AVCodecTrace trace("BaseStreamDemuxer::SnifferMediaType");
    MEDIA_LOG_D("BaseStreamDemuxer::SnifferMediaType called");
    typeFinder_ = std::make_shared<TypeFinder>();
    typeFinder_->Init(uri_, mediaDataSize_, checkRange_, peekRange_, streamInfo.streamId);
    typeFinder_->SetSniffSize(streamInfo.sniffSize);
    std::string type = typeFinder_->FindMediaType();
    std::unique_lock<std::mutex> lock(typeFinderMutex_);
    typeFinder_ = nullptr;
    MEDIA_LOG_D("SnifferMediaType result type: " PUBLIC_LOG_S, type.c_str());
    return type;
}

void BaseStreamDemuxer::SetDemuxerState(int32_t streamId, DemuxerState state)
{
    {
        std::lock_guard lock(pluginStateMutex_);
        pluginStateMap_[streamId] = state;
    }
    if ((IsDash() || streamId == 0) && state == DemuxerState::DEMUXER_STATE_PARSE_FRAME) {
        source_->SetDemuxerState(streamId);
    }
}

void BaseStreamDemuxer::SetBundleName(const std::string& bundleName)
{
    MEDIA_LOG_I_SHORT("SetBundleName bundleName: " PUBLIC_LOG_S, bundleName.c_str());
    bundleName_ = bundleName;
}

void BaseStreamDemuxer::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_D("BaseStreamDemuxer onInterrupted %{public}d", isInterruptNeeded);
    isInterruptNeeded_ = isInterruptNeeded;
    TypeFinderInterrupt(isInterruptNeeded);
}

void BaseStreamDemuxer::TypeFinderInterrupt(bool isInterruptNeeded)
{
    std::unique_lock<std::mutex> lock(typeFinderMutex_);
    if (typeFinder_ != nullptr) {
        typeFinder_->SetInterruptState(isInterruptNeeded);
    }
}

void BaseStreamDemuxer::SetIsIgnoreParse(bool state)
{
    return isIgnoreParse_.store(state);
}

bool BaseStreamDemuxer::GetIsIgnoreParse()
{
    return isIgnoreParse_.load();
}

Plugins::Seekable BaseStreamDemuxer::GetSeekable()
{
    return source_->GetSeekable();
}

bool BaseStreamDemuxer::IsDash() const
{
    return isDash_;
}
void BaseStreamDemuxer::SetIsDash(bool flag)
{
    isDash_ = flag;
}

Status BaseStreamDemuxer::SetNewVideoStreamID(int32_t streamID)
{
    MEDIA_LOG_I_SHORT("SetNewVideoStreamID id: " PUBLIC_LOG_D32, streamID);
    SetChangeFlag(false);
    newVideoStreamID_.store(streamID);
    return Status::OK;
}

Status BaseStreamDemuxer::SetNewAudioStreamID(int32_t streamID)
{
    MEDIA_LOG_D_SHORT("SetNewAudioStreamID id: " PUBLIC_LOG_D32, streamID);
    SetChangeFlag(false);
    newAudioStreamID_.store(streamID);
    return Status::OK;
}

Status BaseStreamDemuxer::SetNewSubtitleStreamID(int32_t streamID)
{
    MEDIA_LOG_I("SetNewSubtitleStreamID id: " PUBLIC_LOG_D32, streamID);
    SetChangeFlag(false);
    newSubtitleStreamID_.store(streamID);
    return Status::OK;
}

int32_t BaseStreamDemuxer::GetNewVideoStreamID()
{
    return newVideoStreamID_.load();
}

int32_t BaseStreamDemuxer::GetNewAudioStreamID()
{
    return newAudioStreamID_.load();
}

int32_t BaseStreamDemuxer::GetNewSubtitleStreamID()
{
    return newSubtitleStreamID_.load();
}

bool BaseStreamDemuxer::CanDoChangeStream()
{
    return changeStreamFlag_.load();
}

void BaseStreamDemuxer::SetChangeFlag(bool flag)
{
    return changeStreamFlag_.store(flag);
}

bool BaseStreamDemuxer::SetSourceInitialBufferSize(int32_t offset, int32_t size)
{
    return source_->SetInitialBufferSize(offset, size);
}
} // namespace Media
} // namespace OHOS
