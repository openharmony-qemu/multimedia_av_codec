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

#ifndef DEMUXER_MOCK_H
#define DEMUXER_MOCK_H

#include <string>
#include <memory>
#include "nocopyable.h"
#include "avformat_mock.h"
#include "avsource_mock.h"
#include "common_mock.h"
#include "av_common.h"
#include "avcodec_common.h"

namespace OHOS {
namespace MediaAVCodec {
class DemuxerMock : public NoCopyable {
public:
    virtual ~DemuxerMock() = default;

    virtual int32_t Destroy() = 0;
    virtual int32_t SelectTrackByID(uint32_t trackIndex) = 0;
    virtual int32_t UnselectTrackByID(uint32_t trackIndex) = 0;
    virtual int32_t ReadSample(uint32_t trackIndex, std::shared_ptr<AVMemoryMock> sample,
        AVCodecBufferInfo *bufferInfo, uint32_t &flag) = 0;
    virtual int32_t SeekToTime(int64_t mSeconds, Media::SeekMode mode) = 0;
};

class __attribute__((visibility("default"))) AVDemuxerMockFactory {
public:
    static std::shared_ptr<DemuxerMock> CreateDemuxer(std::shared_ptr<AVSourceMock> source);
private:
    AVDemuxerMockFactory() = delete;
    ~AVDemuxerMockFactory() = delete;
};
}  // namespace MediaAVCodec
}  // namespace OHOS
#endif // DEMUXER_MOCK_H