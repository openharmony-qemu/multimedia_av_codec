/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SUPER_RESOLUTION_POST_PROCESSOR_H
#define SUPER_RESOLUTION_POST_PROCESSOR_H

#include "base_video_post_processor.h"
#include "video_post_processor_factory.h"

#include "algorithm_video.h"
#include "algorithm_video_common.h"

namespace OHOS {
namespace Media {

class SuperResolutionPostProcessor : public BaseVideoPostProcessor,
                                     public std::enable_shared_from_this<SuperResolutionPostProcessor> {
public:
    SuperResolutionPostProcessor();
    ~SuperResolutionPostProcessor() override;

    bool IsValid();

    Status Init() override;
    Status Flush() override;
    Status Stop() override;
    Status Start() override;
    Status Release() override;
    Status NotifyEos(int64_t eosPts = 0) override;

    sptr<Surface> GetInputSurface() override;
    Status SetOutputSurface(sptr<Surface> surface) override;

    Status SetCallback(const std::shared_ptr<PostProcessorCallback> callback) override;
    Status SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver) override;

    Status SetParameter(const Format &format) override;
    Status SetPostProcessorOn(bool isPostProcessorOn) override;
    Status SetVideoWindowSize(int32_t width, int32_t height) override;

    Status ReleaseOutputBuffer(uint32_t index, bool render) override;
    Status RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs) override;

    void OnOutputFormatChanged(const Format &format);
    void OnOutputBufferAvailable(uint32_t index, VideoProcessingEngine::VpeBufferFlag flag);
    void OnOutputBufferAvailable(uint32_t index, const VideoProcessingEngine::VpeBufferInfo& info);
    void OnSuperResolutionChanged(bool enable);
    void OnError(VideoProcessingEngine::VPEAlgoErrCode errorCode);
private:
    Status SetQualityLevel(VideoProcessingEngine::DetailEnhancerQualityLevel level);

    static constexpr VideoProcessingEngine::DetailEnhancerQualityLevel DEFAULT_QUALITY_LEVEL =
        VideoProcessingEngine::DETAIL_ENHANCER_LEVEL_HIGH;
    std::shared_ptr<VideoProcessingEngine::VpeVideo> postProcessor_ {nullptr};
    bool isPostProcessorOn_ {false};

    std::shared_ptr<PostProcessorCallback> filterCallback_;
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver_ {nullptr};

    std::shared_mutex mutex_ {};
};

} // namespace Media
} // namespace OHOS
#endif // SUPER_RESOLUTION_POST_PROCESSOR_H
