/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "avmuxer_inner_mock.h"
#include "securec.h"
#include "common/native_mfmagic.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace OHOS::Media;
int32_t AVMuxerInnerMock::Destroy()
{
    if (muxer_ != nullptr) {
        muxer_ = nullptr;
        return AV_ERR_OK;
    }
    return AV_ERR_UNKNOWN;
}

int32_t AVMuxerInnerMock::Start()
{
    if (muxer_ != nullptr) {
        return muxer_->Start();
    }
    return AV_ERR_UNKNOWN;
}

int32_t AVMuxerInnerMock::Stop()
{
    if (muxer_ != nullptr) {
        return muxer_->Stop();
    }
    return AV_ERR_UNKNOWN;
}

int32_t AVMuxerInnerMock::AddTrack(int32_t &trackIndex, std::shared_ptr<FormatMock> &trackFormat)
{
    if (muxer_ != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatInnerMock>(trackFormat);
        return muxer_->AddTrack(trackIndex, formatMock->GetFormat().GetMeta());
    }
    return AV_ERR_UNKNOWN;
}

int32_t AVMuxerInnerMock::WriteSample(uint32_t trackIndex,
    const uint8_t *sample, const OH_AVCodecBufferAttr &info)
{
    if (muxer_ != nullptr) {
        auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        std::shared_ptr<AVBuffer> avSample = AVBuffer::CreateAVBuffer(alloc, info.size);
        avSample->memory_->Write(sample + info.offset, info.size);
        avSample->pts_ = info.pts;
        avSample->flag_ = info.flags;
        return muxer_->WriteSample(trackIndex, avSample);
    }
    return AV_ERR_UNKNOWN;
}

int32_t AVMuxerInnerMock::WriteSampleBuffer(uint32_t trackIndex, const OH_AVBuffer *sample)
{
    if (muxer_ != nullptr && sample != nullptr) {
        return muxer_->WriteSample(trackIndex, sample->buffer_);
    }
    return AV_ERR_UNKNOWN;
}

int32_t AVMuxerInnerMock::SetRotation(int32_t rotation)
{
    if (muxer_ != nullptr) {
        std::shared_ptr<Meta> param = std::make_shared<Meta>();
        param->Set<Tag::VIDEO_ROTATION>(static_cast<Plugins::VideoRotation>(rotation));
        return muxer_->SetParameter(param);
    }
    return AV_ERR_UNKNOWN;
}
} // namespace MediaAVCodec
} // namespace OHOS