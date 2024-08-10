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
    MEDIA_LOG_D_SHORT("BaseStreamDemuxer called");
    seekable_ = Plugins::Seekable::UNSEEKABLE;
}

BaseStreamDemuxer::~BaseStreamDemuxer()
{
    MEDIA_LOG_D_SHORT("~BaseStreamDemuxer called");
}

void BaseStreamDemuxer::SetSource(const std::shared_ptr<Source>& source)
{
    MEDIA_LOG_D_SHORT("BaseStreamDemuxer::SetSource");
    source_ = source;
    source_->GetSize(mediaDataSize_);
    seekable_ = source_->GetSeekable();
}

std::string BaseStreamDemuxer::SnifferMediaType(int32_t streamID)
{
    MediaAVCodec::AVCodecTrace trace("BaseStreamDemuxer::SnifferMediaType");
    MEDIA_LOG_I_SHORT("BaseStreamDemuxer::SnifferMediaType called");
    std::shared_ptr<TypeFinder> typeFinder = std::make_shared<TypeFinder>();
    typeFinder->Init(uri_, mediaDataSize_, checkRange_, peekRange_, streamID);
    std::string type = typeFinder->FindMediaType();
    MEDIA_LOG_D_SHORT("SnifferMediaType result type: " PUBLIC_LOG_S, type.c_str());
    return type;
}

void BaseStreamDemuxer::SetDemuxerState(int32_t streamId, DemuxerState state)
{
    pluginStateMap_[streamId] = state;
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
    isInterruptNeeded_ = isInterruptNeeded;
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
    MEDIA_LOG_I("SetNewAudioStreamID id: " PUBLIC_LOG_D32, streamID);
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
} // namespace Media
} // namespace OHOS
