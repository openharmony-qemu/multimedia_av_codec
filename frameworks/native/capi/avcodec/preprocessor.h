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
#include <string>
#include "avcodec_info.h"
#include "cpu_image_processor.h"
#include "fast_kits_interface.h"
#include "format.h"
#include "frame_drop_filter.h"
#include "surface.h"

namespace OHOS {
namespace MediaAVCodec {

class Preprocessor {
public:
    explicit Preprocessor(const std::string &mime);
    ~Preprocessor();

    int32_t Configure(const Media::Format &format);
    int32_t SetParameter(const Media::Format &format);

    int32_t Process(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output, uint64_t pts);
    bool ShouldDropFrame(uint64_t pts);
    void FlushTimeStamp();
    void SetOnBufferReallocCallback(std::function<void(sptr<SurfaceBuffer>, sptr<SurfaceBuffer>)> callback);

    bool IsCropEnabled() const;
    bool IsDownsamplingEnabled() const;

    int32_t GetConfiguredWidth() const;
    int32_t GetConfiguredHeight() const;
    GraphicPixelFormat GetConfiguredPixelFormat() const;
    int32_t GetOutputWidth() const;
    int32_t GetOutputHeight() const;

    // Field-level config getters for metadata query
    int32_t GetCropLeft() const;
    int32_t GetCropRight() const;
    int32_t GetCropTop() const;
    int32_t GetCropBottom() const;
    int32_t GetDownsamplingWidth() const;
    int32_t GetDownsamplingHeight() const;

private:
    static constexpr int32_t CROP_RECT_INCLUSIVE_OFFSET = 1;

    enum class ConfigResult { NOT_SET, DISABLED, ENABLED };

    struct EncoderConfig {
        int32_t width{0};
        int32_t height{0};
        GraphicPixelFormat pixelFormat{GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT};
        double frameRate{0.0};
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
        double targetFrameRate{0};
    };

    struct CropConfig {
        int32_t left{0};
        int32_t right{0};
        int32_t top{0};
        int32_t bottom{0};
        int32_t width{0};
        int32_t height{0};
        bool enabled{false};
        CropConfig() = default;
        explicit CropConfig(const CropParams &params)
            : left(params.left),
              right(params.right),
              top(params.top),
              bottom(params.bottom),
              width(params.right - params.left + CROP_RECT_INCLUSIVE_OFFSET),
              height(params.bottom - params.top + CROP_RECT_INCLUSIVE_OFFSET),
              enabled(params.result == ConfigResult::ENABLED)
        {
        }
    };

    struct DownsamplingConfig {
        int32_t width{0};
        int32_t height{0};
        GraphicPixelFormat pixelFormat{GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT};
        bool enabled{false};
        DownsamplingConfig() = default;
        explicit DownsamplingConfig(const DownsamplingParams &params)
            : width(params.width), height(params.height), enabled(params.result == ConfigResult::ENABLED)
        {
        }
    };

    struct DropFrameConfig {
        double targetFrameRate{0};
        bool enabled{false};
        DropFrameConfig() = default;
        explicit DropFrameConfig(const DropFrameParams &params)
            : targetFrameRate(params.targetFrameRate), enabled(params.result == ConfigResult::ENABLED)
        {
        }
    };

    int32_t ValidateCrop(const Media::Format &format, CropParams &params);
    int32_t ValidateDownsampling(const Media::Format &format, DownsamplingParams &params);
    int32_t ValidateDropFrame(const Media::Format &format, DropFrameParams &params);
    const CropConfig &GetCropConfig() const;
    const DownsamplingConfig &GetDownsamplingConfig() const;
    const DropFrameConfig &GetDropFrameConfig() const;
    int32_t Crop(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output);
    int32_t Downsampling(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output);
    int32_t Copy(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output);
    void UpdateOutputMetadata(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                              double scaleX, double scaleY, int32_t offsetTop, int32_t offsetLeft);
    void CopyInputMetadata(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output);
    void UpdateRoiMetadata(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                           double scaleX, double scaleY, int32_t offsetTop, int32_t offsetLeft);

    bool IsHevc10BitData(HEVCProfile profile);
    GraphicPixelFormat ConvertVideoPixelFormat2GraphicPixelFormat(VideoPixelFormat pixelFormat, int32_t profile);
    void UpdateConfiguredValues(const Media::Format &format, EncoderConfig &config);
    bool GetEncoderRange(const Media::Format &format, Range &encWidth, Range &encHeight);
    bool CheckOrReallocOutSurfaceBuffer(int32_t targetWidth, int32_t targetHeight,
                                        int32_t targetFormat, sptr<SurfaceBuffer> &output);

    CropConfig cropConfig_;
    DownsamplingConfig downsamplingConfig_;
    DropFrameConfig dropFrameConfig_;
    EncoderConfig encoderConfig_;
    int32_t dAlgoWidthMin_{0};
    int32_t dAlgoHeightMin_{0};
    int32_t dAlgoWidthMax_{0};
    int32_t dAlgoHeightMax_{0};
    std::string mime_;
    PreProcessing::FastKitsInterface &fastKitsInterface_ = PreProcessing::FastKitsInterface::GetInstance();
    PreProcessing::FrameDropFilter dropFilter_;
    std::function<void(sptr<SurfaceBuffer>, sptr<SurfaceBuffer>)> onOutSurfaceBufferReallocCb_{};
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // PREPROCESSOR_H
