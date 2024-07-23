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

#ifndef AVCODEC_SAMPLE_VIDEO_ENCODER_SAMPLE_H
#define AVCODEC_SAMPLE_VIDEO_ENCODER_SAMPLE_H

#include "video_sample_base.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class VideoEncoderSample : public VideoSampleBase {
public:
    VideoEncoderSample() {};

private:
    int32_t Init() override;
    int32_t StartThread() override;
    void BufferInputThread();
    void SurfaceInputThread();
    void OutputThread();
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_VIDEO_ENCODER_SAMPLE_H