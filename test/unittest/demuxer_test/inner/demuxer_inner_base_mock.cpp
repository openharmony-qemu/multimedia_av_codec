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
#include "avmemory_inner_mock.h"
#include "native_averrors.h"
#include "demuxer_inner_mock.h"

namespace OHOS {
namespace MediaAVCodec {
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

int32_t DemuxerInnerMock::SeekToTime(int64_t mSeconds, SeekMode mode)
{
    if (demuxer_ != nullptr) {
        SeekMode seekMode = static_cast<SeekMode>(mode);
        return demuxer_->SeekToTime(mSeconds, seekMode);
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerInnerMock::GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
    const uint64_t relativePresentationTimeUs, uint32_t &index)
{
    if (demuxer_ != nullptr) {
        return demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex, relativePresentationTimeUs, index);
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerInnerMock::GetRelativePresentationTimeUsByIndex(uint32_t trackIndex,
    const uint32_t index, uint64_t &relativePresentationTimeUs)

{
    if (demuxer_ != nullptr) {
        return demuxer_->GetRelativePresentationTimeUsByIndex(trackIndex, index, relativePresentationTimeUs);
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerInnerMock::GetCurrentCacheSize(uint32_t trackId, uint32_t& size)
{
    if (demuxer_ != nullptr) {
        return demuxer_->GetCurrentCacheSize(trackId, size);
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerInnerMock::ReadSampleBuffer(
    uint32_t trackIndex, std::shared_ptr<AVBufferMock> sample, bool checkBufferInfo)
{
    if (sample == nullptr) {
        printf("AVBufferMock is nullptr\n");
        return AV_ERR_UNKNOWN;
    }
    if (demuxer_ == nullptr) {
        printf("Demuxer is nullptr\n");
        return AV_ERR_UNKNOWN;
    }

    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(
        sample->GetAddr(), sample->GetCapacity(), sample->GetCapacity());
    if (buffer == nullptr || buffer->memory_ == nullptr) {
        printf("AVBuffer is nullptr");
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = demuxer_->ReadSampleBuffer(trackIndex, buffer);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    OH_AVCodecBufferAttr bufferAttr;
    bufferAttr.pts = buffer->pts_;
    bufferAttr.size = buffer->memory_->GetSize();
    bufferAttr.offset = 0;
    bufferAttr.flags = buffer->flag_;
    ret = sample->SetBufferAttr(bufferAttr);
    if (ret != AV_ERR_OK) {
        return ret;
    }

    if (checkBufferInfo) {
        std::shared_ptr<Meta> bufferMeta = buffer->meta_;
        if (bufferMeta == nullptr) {
            printf("AVBuffer meta is nullptr");
            return AV_ERR_UNKNOWN;
        } else {
            int64_t duration;
            int64_t dts;
            bufferMeta->GetData(Tag::BUFFER_DURATION, duration);
            bufferMeta->GetData(Tag::BUFFER_DECODING_TIMESTAMP, dts);
            printf("[track %d] duration %" PRId64 " dts %" PRId64 "\n", trackIndex, duration, dts);
        }
    }

    return AV_ERR_OK;
}
}
}