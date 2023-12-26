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

#ifndef AVMUXER_MOCK_H
#define AVMUXER_MOCK_H

#include <string>
#include "native_averrors.h"
#include "avformat_mock.h"
#include "native_avcodec_base.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class AVMuxerMock : public NoCopyable {
public:
    virtual ~AVMuxerMock() = default;
    virtual int32_t Destroy() = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t AddTrack(int32_t &trackIndex, std::shared_ptr<FormatMock> &trackFormat) = 0;
    virtual int32_t WriteSample(uint32_t trackIndex, const uint8_t *sample,
        const OH_AVCodecBufferAttr &info)  = 0;
    virtual int32_t WriteSampleBuffer(uint32_t trackIndex, const OH_AVBuffer *sample)  = 0;
    virtual int32_t SetRotation(int32_t rotation) = 0;
};

class __attribute__((visibility("default"))) AVMuxerMockFactory {
public:
    static std::shared_ptr<AVMuxerMock> CreateMuxer(int32_t fd, const OH_AVOutputFormat &format);
private:
    AVMuxerMockFactory() = delete;
    ~AVMuxerMockFactory() = delete;
};
}  // namespace MediaAVCodec
}  // namespace OHOS
#endif // AVMUXER_MOCK_H