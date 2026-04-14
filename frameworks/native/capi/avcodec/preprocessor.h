/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <memory>
#include "format.h"
#include "surface.h"

namespace OHOS {
namespace MediaAVCodec {

class Preprocessor {
public:
    Preprocessor();
    ~Preprocessor();

    int32_t Configure(const Media::Format& format);
    int32_t SetParameter(const Media::Format& format);

    int32_t Process(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output);
    bool ShouldDropFrame(int64_t pts);

    bool IsCropEnabled() const;
    bool IsDownsamplingEnabled() const;

private:
    enum class ConfigResult {
        NOT_SET,
        DISABLED,
        ENABLED
    };

    struct CropParams {
        ConfigResult result{ConfigResult::NOT_SET};
        int32_t left{0};
        int32_t right{0};
        int32_t top{0};
        int32_t bottom{0};
    };

    struct DownsamplingParams {
        ConfigResult result{ConfigResult::NOT_SET};
        int32_t width{0};
        int32_t height{0};
    };

    struct DropFrameParams {
        ConfigResult result{ConfigResult::NOT_SET};
        int32_t targetFrameRate{0};
    };

    struct CropConfig {
        int32_t left{0};
        int32_t right{0};
        int32_t top{0};
        int32_t bottom{0};
        bool enabled{false};
        CropConfig() = default;
        explicit CropConfig(const CropParams& params)
            : left(params.left), right(params.right), top(params.top), bottom(params.bottom),
              enabled(params.result == ConfigResult::ENABLED) {}
    };

    struct DownsamplingConfig {
        int32_t width{0};
        int32_t height{0};
        bool enabled{false};
        DownsamplingConfig() = default;
        explicit DownsamplingConfig(const DownsamplingParams& params)
            : width(params.width), height(params.height), enabled(params.result == ConfigResult::ENABLED) {}
    };

    struct DropFrameConfig {
        int32_t targetFrameRate{0};
        bool enabled{false};
        DropFrameConfig() = default;
        explicit DropFrameConfig(const DropFrameParams& params)
            : targetFrameRate(params.targetFrameRate), enabled(params.result == ConfigResult::ENABLED) {}
    };

    int32_t ValidateCrop(const Media::Format& format, CropParams& params);
    int32_t ValidateDownsampling(const Media::Format& format, DownsamplingParams& params);
    int32_t ValidateDropFrame(const Media::Format& format, DropFrameParams& params);
    const CropConfig& GetCropConfig() const;
    const DownsamplingConfig& GetDownsamplingConfig() const;
    const DropFrameConfig& GetDropFrameConfig() const;
    void Crop(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output);
    void Downsampling(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output);
    void Copy(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output);

    CropConfig cropConfig_;
    DownsamplingConfig downsamplingConfig_;
    DropFrameConfig dropFrameConfig_;
    int64_t lastFramePts_{0};
    int32_t configuredWidth_{0};
    int32_t configuredHeight_{0};
    double configuredFramerate_{0};
    int32_t dAlgoWidthMin_{0};
    int32_t dAlgoHeightMin_{0};
    int32_t dAlgoWidthMax_{0};
    int32_t dAlgoHeightMax_{0};
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // PREPROCESSOR_H
