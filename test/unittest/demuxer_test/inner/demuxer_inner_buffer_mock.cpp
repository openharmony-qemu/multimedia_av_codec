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

#include <memory>
#include "avmemory_inner_mock.h"
#include "native_averrors.h"
#include "demuxer_inner_mock.h"
#include "buffer/avbuffer.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
int32_t DemuxerInnerMock::Destroy()
{
    if (demuxer_ != nullptr) {
        demuxer_ = nullptr;
        return AV_ERR_OK;
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerInnerMock::SelectTrackByID(uint32_t trackIndex)
{
    if (demuxer_ != nullptr) {
        return demuxer_->SelectTrackByID(trackIndex);
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerInnerMock::UnselectTrackByID(uint32_t trackIndex)
{
    if (demuxer_ != nullptr) {
        return demuxer_->UnselectTrackByID(trackIndex);
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerInnerMock::ReadSample(uint32_t trackIndex, std::shared_ptr<AVMemoryMock> sample,
    AVCodecBufferInfo *bufferInfo, uint32_t &flag)
{
    if (bufferInfo == nullptr) {
        printf("AVCodecBufferInfo is nullptr");
        return AV_ERR_UNKNOWN;
    }
    auto mem = std::static_pointer_cast<AVMemoryInnerMock>(sample);
    std::shared_ptr<AVSharedMemory> sharedMem = (mem != nullptr) ? mem->GetAVMemory() : nullptr;
    if (sharedMem == nullptr) {
        printf("AVSharedMemory is nullptr");
        return AV_ERR_UNKNOWN;
    }
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(
        sharedMem->GetBase(), sharedMem->GetSize(), sharedMem->GetSize());
    if (buffer == nullptr || buffer->memory_ == nullptr) {
        printf("AVBuffer is nullptr");
        return AV_ERR_UNKNOWN;
    }
    if (demuxer_ != nullptr) {
        int32_t ret = demuxer_->ReadSampleBuffer(trackIndex, buffer);
        bufferInfo->presentationTimeUs = buffer->pts_;
        bufferInfo->size = buffer->memory_->GetSize();
        bufferInfo->offset = 0;
        flag = buffer->flag_;
        return ret;
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerInnerMock::SeekToTime(int64_t mSeconds, SeekMode mode)
{
    if (demuxer_ != nullptr) {
        SeekMode seekMode = static_cast<SeekMode>(mode);
        return demuxer_->SeekToTime(mSeconds, seekMode);
    }
    return AV_ERR_UNKNOWN;
}
}
}