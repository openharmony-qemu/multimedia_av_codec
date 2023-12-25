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

#include "avmuxer_sample.h"
#include "nocopyable.h"
using namespace std;

namespace OHOS {
namespace MediaAVCodec {
AVMuxerSample::AVMuxerSample()
{
}

AVMuxerSample::~AVMuxerSample()
{
}

bool AVMuxerSample::CreateMuxer(int32_t fd, const OH_AVOutputFormat format)
{
    muxer_ = AVMuxerMockFactory::CreateMuxer(fd, format);
    return muxer_ != nullptr;
}

int32_t AVMuxerSample::Destroy()
{
    if (muxer_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return muxer_->Destroy();
}

int32_t AVMuxerSample::Start()
{
    if (muxer_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return muxer_->Start();
}

int32_t AVMuxerSample::Stop()
{
    if (muxer_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return muxer_->Stop();
}

int32_t AVMuxerSample::AddTrack(int32_t &trackIndex, std::shared_ptr<FormatMock> &trackFormat)
{
    if (muxer_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return muxer_->AddTrack(trackIndex, trackFormat);
}

int32_t AVMuxerSample::WriteSample(uint32_t trackIndex, const uint8_t *sample, const OH_AVCodecBufferAttr &info)
{
    if (muxer_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return muxer_->WriteSample(trackIndex, sample, info);
}

int32_t AVMuxerSample::WriteSampleBuffer(uint32_t trackIndex, const OH_AVBuffer *sample)
{
    if (muxer_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return muxer_->WriteSampleBuffer(trackIndex, sample);
}

int32_t AVMuxerSample::SetRotation(int32_t rotation)
{
    if (muxer_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return muxer_->SetRotation(rotation);
}
}  // namespace MediaAVCodec
}  // namespace OHOS
