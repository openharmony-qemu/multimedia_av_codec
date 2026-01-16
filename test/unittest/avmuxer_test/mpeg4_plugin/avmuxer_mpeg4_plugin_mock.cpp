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

#include "avmuxer_mpeg4_plugin_mock.h"
#include "securec.h"
#include "avformat_inner_mock.h"
#include "common/native_mfmagic.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
AVMuxerMpeg4PluginMock::AVMuxerMpeg4PluginMock(std::shared_ptr<Media::Plugins::DataSink> dataSink,
    std::shared_ptr<Media::Plugins::MuxerPlugin> muxer) : dataSink_(dataSink), muxer_(muxer)
{
    if (muxer_) {
        muxer_->SetDataSink(dataSink_);
    }
}

int32_t AVMuxerMpeg4PluginMock::Destroy()
{
    if (muxer_ != nullptr) {
        muxer_ = nullptr;
        return AV_ERR_OK;
    }
    return AV_ERR_UNKNOWN;
}

int32_t AVMuxerMpeg4PluginMock::Start()
{
    if (muxer_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return static_cast<int32_t>(muxer_->Start());
}

int32_t AVMuxerMpeg4PluginMock::Stop()
{
    if (muxer_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return static_cast<int32_t>(muxer_->Stop());
}

int32_t AVMuxerMpeg4PluginMock::AddTrack(int32_t &trackIndex, std::shared_ptr<FormatMock> &trackFormat)
{
    if (muxer_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    auto formatMock = std::static_pointer_cast<AVFormatInnerMock>(trackFormat);
    return static_cast<int32_t>(muxer_->AddTrack(trackIndex, formatMock->GetFormat().GetMeta()));
}

int32_t AVMuxerMpeg4PluginMock::WriteSample(uint32_t trackIndex,
    const uint8_t *sample, const OH_AVCodecBufferAttr &info)
{
    if (muxer_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> avSample = AVBuffer::CreateAVBuffer(alloc, info.size);
    avSample->memory_->Write(sample + info.offset, info.size);
    avSample->pts_ = info.pts;
    avSample->flag_ = info.flags;
    return static_cast<int32_t>(muxer_->WriteSample(trackIndex, avSample));
}

int32_t AVMuxerMpeg4PluginMock::WriteSampleBuffer(uint32_t trackIndex, const OH_AVBuffer *sample)
{
    if (muxer_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return static_cast<int32_t>(muxer_->WriteSample(trackIndex, sample->buffer_));
}

int32_t AVMuxerMpeg4PluginMock::SetTimedMetadata()
{
    if (muxer_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return 0;
}

int32_t AVMuxerMpeg4PluginMock::SetRotation(int32_t rotation)
{
    if (muxer_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::VIDEO_ROTATION>(static_cast<Plugins::VideoRotation>(rotation));
    return static_cast<int32_t>(muxer_->SetParameter(param));
}

int32_t AVMuxerMpeg4PluginMock::SetFormat(std::shared_ptr<FormatMock> &format)
{
    if (muxer_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    auto formatMock = std::static_pointer_cast<AVFormatInnerMock>(format);
    muxer_->SetUserMeta(formatMock->GetFormat().GetMeta());
    return static_cast<int32_t>(muxer_->SetParameter(formatMock->GetFormat().GetMeta()));
}

int32_t AVMuxerMpeg4PluginMock::SetParameter(const std::shared_ptr<Meta> &param)
{
    if (muxer_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return static_cast<int32_t>(muxer_->SetParameter(param));
}

int32_t AVMuxerMpeg4PluginMock::SetUserMeta(const std::shared_ptr<Meta> &userMate)
{
    if (muxer_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return static_cast<int32_t>(muxer_->SetUserMeta(userMate));
}

int32_t AVMuxerMpeg4PluginMock::AddTrack(int32_t& trackIndex, const std::shared_ptr<Meta> &trackDesc)
{
    if (muxer_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return static_cast<int32_t>(muxer_->AddTrack(trackIndex, trackDesc));
}
} // namespace MediaAVCodec
} // namespace OHOS