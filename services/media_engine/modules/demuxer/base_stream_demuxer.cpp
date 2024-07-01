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
#include "frame_detector.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_time.h"

namespace OHOS {
namespace Media {

BaseStreamDemuxer::BaseStreamDemuxer()
{
    MEDIA_LOG_I("BaseStreamDemuxer called");
    seekable_ = Plugins::Seekable::UNSEEKABLE;
}

BaseStreamDemuxer::~BaseStreamDemuxer()
{
    MEDIA_LOG_D("~BaseStreamDemuxer called");
}

void BaseStreamDemuxer::SetSource(const std::shared_ptr<Source>& source)
{
    MEDIA_LOG_I("BaseStreamDemuxer::SetSource");
    source_ = source;
    source_->GetSize(mediaDataSize_);
    seekable_ = source_->GetSeekable();
}

std::string BaseStreamDemuxer::SnifferMediaType(int32_t streamID)
{
    MediaAVCodec::AVCodecTrace trace("BaseStreamDemuxer::SnifferMediaType");
    MEDIA_LOG_I("BaseStreamDemuxer::SnifferMediaType called");
    std::shared_ptr<TypeFinder> typeFinder = std::make_shared<TypeFinder>();
    typeFinder->Init(uri_, mediaDataSize_, checkRange_, peekRange_, streamID);
    std::string type = typeFinder->FindMediaType();
    MEDIA_LOG_D("SnifferMediaType result type: " PUBLIC_LOG_S, type.c_str());
    return type;
}

void BaseStreamDemuxer::SetDemuxerState(int32_t streamId, DemuxerState state)
{
    pluginStateMap_[streamId] = state;
    if (streamId == 0 && state == DemuxerState::DEMUXER_STATE_PARSE_FRAME) {
        source_->SetDemuxerState();
    }
}

void BaseStreamDemuxer::SetBundleName(const std::string& bundleName)
{
    MEDIA_LOG_I("SetBundleName bundleName: " PUBLIC_LOG_S, bundleName.c_str());
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
    MEDIA_LOG_I("SetNewVideoStreamID id: " PUBLIC_LOG_D32, streamID);
    newVideoStreamID_.store(streamID);
    return Status::OK;
}

int32_t BaseStreamDemuxer::GetNewVideoStreamID()
{
    return newVideoStreamID_.load();
}

} // namespace Media
} // namespace OHOS
