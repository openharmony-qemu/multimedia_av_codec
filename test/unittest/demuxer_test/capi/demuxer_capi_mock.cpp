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

#include "native_averrors.h"
#include "avmemory_capi_mock.h"
#include "demuxer_capi_mock.h"

namespace OHOS {
namespace MediaAVCodec {
int32_t DemuxerCapiMock::Destroy()
{
    if (demuxer_ != nullptr) {
        int32_t ret = OH_AVDemuxer_Destroy(demuxer_);
        demuxer_ = nullptr;
        return ret;
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerCapiMock::SelectTrackByID(uint32_t trackIndex)
{
    if (demuxer_ != nullptr) {
        return OH_AVDemuxer_SelectTrackByID(demuxer_, trackIndex);
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerCapiMock::UnselectTrackByID(uint32_t trackIndex)
{
    if (demuxer_ != nullptr) {
        return OH_AVDemuxer_UnselectTrackByID(demuxer_, trackIndex);
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerCapiMock::ReadSample(uint32_t trackIndex, std::shared_ptr<AVMemoryMock> sample,
    AVCodecBufferInfo *bufferInfo, uint32_t &flag)
{
    auto mem = std::static_pointer_cast<AVMemoryCapiMock>(sample);
    OH_AVMemory *avMemory = (mem != nullptr) ? mem->GetAVMemory() : nullptr;
    if (demuxer_ != nullptr) {
        OH_AVCodecBufferAttr bufferAttr;
        int32_t ret = OH_AVDemuxer_ReadSample(demuxer_, trackIndex, avMemory, &bufferAttr);
        bufferInfo->presentationTimeUs = bufferAttr.pts;
        bufferInfo->size = bufferAttr.size;
        bufferInfo->offset = bufferAttr.offset;
        flag = bufferAttr.flags;
        return ret;
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerCapiMock::SeekToTime(int64_t mSeconds, Media::SeekMode mode)
{
    if (demuxer_ != nullptr) {
        OH_AVSeekMode seekMode = static_cast<OH_AVSeekMode>(mode);
        return OH_AVDemuxer_SeekToTime(demuxer_, mSeconds, seekMode);
    }
    return AV_ERR_UNKNOWN;
}
}
}