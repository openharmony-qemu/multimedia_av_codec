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

#include "avmuxer_capi_mock.h"
#include "securec.h"

namespace OHOS {
namespace MediaAVCodec {
int32_t AVMuxerCapiMock::Destroy()
{
    int ret = OH_AVMuxer_Destroy(muxer_);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    muxer_ = nullptr;
    return AV_ERR_OK;
}

int32_t AVMuxerCapiMock::Start()
{
    return OH_AVMuxer_Start(muxer_);
}

int32_t AVMuxerCapiMock::Stop()
{
    return OH_AVMuxer_Stop(muxer_);
}

int32_t AVMuxerCapiMock::AddTrack(int32_t &trackIndex, std::shared_ptr<FormatMock> &trackFormat)
{
    auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(trackFormat);
    return OH_AVMuxer_AddTrack(muxer_, &trackIndex, formatMock->GetFormat());
}

int32_t AVMuxerCapiMock::WriteSample(uint32_t trackIndex,
    const uint8_t *sample, const OH_AVCodecBufferAttr &info)
{
    int32_t ret = AV_ERR_NO_MEMORY;
    OH_AVMemory *avSample = OH_AVMemory_Create(info.size);
    uint8_t *data = OH_AVMemory_GetAddr(avSample);
    int32_t size = OH_AVMemory_GetSize(avSample);
    if (memcpy_s(data, size, sample, info.size) == EOK) {
        OH_AVCodecBufferAttr bufferAttr;
        bufferAttr.pts = info.pts;
        bufferAttr.size = info.size;
        bufferAttr.offset = info.offset;
        bufferAttr.flags = info.flags;
        ret = OH_AVMuxer_WriteSample(muxer_, trackIndex, avSample, bufferAttr);
    }
    (void)OH_AVMemory_Destroy(avSample);
    return ret;
}

int32_t AVMuxerCapiMock::WriteSampleBuffer(uint32_t trackIndex, const OH_AVBuffer *sample)
{
    if (muxer_ != nullptr) {
        return OH_AVMuxer_WriteSampleBuffer(muxer_, trackIndex, sample);
    }
    return AV_ERR_UNKNOWN;
}

int32_t AVMuxerCapiMock::SetRotation(int32_t rotation)
{
    return OH_AVMuxer_SetRotation(muxer_, rotation);
}
} // namespace MediaAVCodec
} // namespace OHOS