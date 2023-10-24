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
#include "avsharedmemorybase.h"

namespace OHOS {
namespace MediaAVCodec {
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
        return muxer_->AddTrack(trackIndex, static_cast<MediaDescription>(formatMock->GetFormat()));
    }
    return AV_ERR_UNKNOWN;
}

int32_t AVMuxerInnerMock::WriteSample(uint32_t trackIndex,
    const uint8_t *sample, const OH_AVCodecBufferAttr &info)
{
    if (muxer_ != nullptr) {
        std::shared_ptr<AVSharedMemoryBase> avSample =
            std::make_shared<AVSharedMemoryBase>(info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
        (void)avSample->Init();
        (void)memcpy_s(avSample->GetBase(), avSample->GetSize(), sample, info.size);
        AVCodecBufferInfo sampleInfo;
        sampleInfo.presentationTimeUs = info.pts;
        sampleInfo.size = info.size;
        sampleInfo.offset = info.offset;
        AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(info.flags);
        return muxer_->WriteSample(trackIndex, avSample, sampleInfo, flag);
    }
    return AV_ERR_UNKNOWN;
}

int32_t AVMuxerInnerMock::SetRotation(int32_t rotation)
{
    if (muxer_ != nullptr) {
        return muxer_->SetRotation(rotation);
    }
    return AV_ERR_UNKNOWN;
}
} // namespace MediaAVCodec
} // namespace OHOS