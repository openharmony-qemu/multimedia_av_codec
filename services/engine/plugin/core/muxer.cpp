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

#include "muxer.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
Muxer::Muxer(uint32_t pkgVer, uint32_t apiVer, std::shared_ptr<MuxerPlugin> plugin)
    : pkgVersion_(pkgVer), apiVersion_(apiVer), muxer_(std::move(plugin)) {}

Status Muxer::SetDataSink(const std::shared_ptr<DataSink>& dataSink)
{
    return muxer_->SetDataSink(dataSink);
}

Status Muxer::SetRotation(int32_t rotation)
{
    return muxer_->SetRotation(rotation);
}

Status Muxer::AddTrack(int32_t &trackIndex, const MediaDescription &trackDesc)
{
    return muxer_->AddTrack(trackIndex, trackDesc);
}

Status Muxer::Start()
{
    return muxer_->Start();
}

Status Muxer::WriteSample(uint32_t trackIndex, const uint8_t *sample, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    return muxer_->WriteSample(trackIndex, sample, info, flag);
}

Status Muxer::Stop()
{
    return muxer_->Stop();
}
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS