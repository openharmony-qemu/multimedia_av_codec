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

#ifndef AVCODEC_SAMPLE_VIDEO_DECODER_SAMPLE_H
#define AVCODEC_SAMPLE_VIDEO_DECODER_SAMPLE_H

#include "video_sample_base.h"
#include "iconsumer_surface.h"

#include "../../window/window_manager/interfaces/innerkits/wm/window.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class VideoDecoderSample : public VideoSampleBase, public OHOS::IBufferConsumerListener {
public:
    VideoDecoderSample() {};
    ~VideoDecoderSample() override;

private:
    int32_t Init() override;
    int32_t StartThread() override;
    void InputThread();
    void OutputThread();
    int32_t CreateWindow(std::shared_ptr<NativeWindow> &window);
    void OnBufferAvailable() override;

    OHOS::sptr<OHOS::Surface> surfaceConsumer_ = nullptr;
    sptr<Rosen::Window> rosenWindow_;
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_VIDEO_DECODER_SAMPLE_H