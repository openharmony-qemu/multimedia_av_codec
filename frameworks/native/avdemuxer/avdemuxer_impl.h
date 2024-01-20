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

#ifndef AVDEMUXER_IMPL_H
#define AVDEMUXER_IMPL_H

#include <memory>
#include "avdemuxer.h"
#include "avsource.h"
#include "nocopyable.h"
#include "media_demuxer.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
class AVDemuxerImpl : public AVDemuxer, public NoCopyable {
public:
    AVDemuxerImpl();
    ~AVDemuxerImpl();

    int32_t SelectTrackByID(uint32_t trackIndex) override;
    int32_t UnselectTrackByID(uint32_t trackIndex) override;
    int32_t ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
        AVCodecBufferInfo &info, uint32_t &flag) override;
    int32_t ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
        AVCodecBufferInfo &info, AVCodecBufferFlag &flag) override;
    int32_t ReadSampleBuffer(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample) override;
    int32_t SeekToTime(int64_t millisecond, const SeekMode mode) override;
    int32_t SetCallback(const std::shared_ptr<AVDemuxerCallback> &callback) override;
    int32_t GetMediaKeySystemInfo(std::multimap<std::string, std::vector<uint8_t>> &infos) override;
    int32_t Init(std::shared_ptr<AVSource> source);

private:
    std::shared_ptr<MediaDemuxer> demuxerEngine_ = nullptr;
    std::string sourceUri_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVDEMUXER_IMPL_H