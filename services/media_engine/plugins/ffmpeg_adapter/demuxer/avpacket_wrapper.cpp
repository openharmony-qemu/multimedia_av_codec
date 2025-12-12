/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "avpacket_wrapper.h"

namespace OHOS {
namespace Media {
namespace Plugins {
AVPacketWrapper::AVPacketWrapper()
{
    pkt_ = av_packet_alloc();
}

AVPacketWrapper::AVPacketWrapper(AVPacket* pkt) noexcept
    : pkt_(pkt)
{
}

AVPacketWrapper::~AVPacketWrapper()
{
    if (pkt_) {
        av_packet_free(&pkt_);
        pkt_ = nullptr;
    }
}

AVPacketWrapper::AVPacketWrapper(AVPacketWrapper&& other) noexcept
    : pkt_(other.pkt_)
{
    other.pkt_ = nullptr;
}

AVPacketWrapper& AVPacketWrapper::operator=(AVPacketWrapper&& other) noexcept
{
    if (this != &other) {
        if (pkt_) {
            av_packet_free(&pkt_);
        }
        pkt_ = other.pkt_;
        other.pkt_ = nullptr;
    }
    return *this;
}

AVPacket* AVPacketWrapper::GetAVPacket() const noexcept
{
    return pkt_;
}

int64_t AVPacketWrapper::GetPts() const noexcept
{
    return pkt_ ? pkt_->pts : AV_NOPTS_VALUE;
}

int64_t AVPacketWrapper::GetDts() const noexcept
{
    return pkt_ ? pkt_->dts : AV_NOPTS_VALUE;
}

int AVPacketWrapper::GetSize() const noexcept
{
    return pkt_ ? pkt_->size : 0;
}

uint8_t* AVPacketWrapper::GetData() const noexcept
{
    return pkt_ ? pkt_->data : nullptr;
}

int AVPacketWrapper::GetStreamIndex() const noexcept
{
    return pkt_ ? pkt_->stream_index : -1;
}

int AVPacketWrapper::GetFlags() const noexcept
{
    return pkt_ ? pkt_->flags : 0;
}

int64_t AVPacketWrapper::GetDuration() const noexcept
{
    return pkt_ ? pkt_->duration : 0;
}

int64_t AVPacketWrapper::GetPos() const noexcept
{
    return pkt_ ? pkt_->pos : -1;
}

AVRational AVPacketWrapper::GetTimeBase() const noexcept
{
    return pkt_ ? pkt_->time_base : AVRational{0, 1};
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS