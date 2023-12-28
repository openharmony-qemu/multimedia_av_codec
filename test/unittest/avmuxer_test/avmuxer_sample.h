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

#ifndef AVMUXER_SAMPLE
#define AVMUXER_SAMPLE
#include <iostream>
#include "avformat_mock.h"
#include "avmuxer_mock.h"

namespace OHOS {
namespace MediaAVCodec {
class AVMuxerSample : public NoCopyable {
public:
    explicit AVMuxerSample();
    virtual ~AVMuxerSample();
    bool CreateMuxer(int32_t fd, const OH_AVOutputFormat format);
    int32_t Destroy();
    int32_t Start();
    int32_t Stop();
    int32_t AddTrack(int32_t &trackIndex, std::shared_ptr<FormatMock> &trackFormat);
    int32_t WriteSample(uint32_t trackIndex, const uint8_t *sample, const OH_AVCodecBufferAttr &info);
    int32_t WriteSampleBuffer(uint32_t trackIndex, const OH_AVBuffer *sample);
    int32_t SetRotation(int32_t rotation);
private:
    std::shared_ptr<AVMuxerMock> muxer_;
};
}  // namespace MediaAVCodec
}  // namespace OHOS
#endif // AVMUXER_SAMPLE