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

#ifndef DEMUXER_INNER_MOCK_H
#define DEMUXER_INNER_MOCK_H

#include "demuxer_mock.h"
#include "common_mock.h"
#include "avformat_inner_mock.h"
#include "avdemuxer.h"

namespace OHOS {
namespace MediaAVCodec {
class DemuxerInnerMock : public DemuxerMock {
public:
    explicit DemuxerInnerMock(std::shared_ptr<AVDemuxer> demuxer) : demuxer_(demuxer) {}
    ~DemuxerInnerMock() = default;
    int32_t Destroy() override;
    int32_t SelectTrackByID(uint32_t trackIndex) override;
    int32_t UnselectTrackByID(uint32_t trackIndex) override;
    int32_t ReadSample(uint32_t trackIndex, std::shared_ptr<AVMemoryMock> sample,
        AVCodecBufferInfo *bufferInfo, uint32_t &flag) override;
    int32_t SeekToTime(int64_t mSeconds, SeekMode mode) override;
private:
    std::shared_ptr<AVDemuxer> demuxer_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // DEMUXER_INNER_MOCK_H