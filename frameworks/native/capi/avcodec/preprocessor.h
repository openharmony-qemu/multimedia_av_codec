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

#include <functional>
#include <memory>
#include <mutex>
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
    using BufferReallocCallback = std::function<void(sptr<SurfaceBuffer>, sptr<SurfaceBuffer>)>;

    struct ConfigSnapshot {
        int32_t configuredWidth{0};
        int32_t configuredHeight{0};
        GraphicPixelFormat configuredPixelFormat{GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT};
        bool cropEnabled{false};
        int32_t cropLeft{0};
        int32_t cropRight{0};
        int32_t cropTop{0};
        int32_t cropBottom{0};
        bool downsamplingEnabled{false};
        int32_t downsamplingWidth{0};
        int32_t downsamplingHeight{0};
    };

    explicit Preprocessor(const std::string &mime);
    ~Preprocessor();

    int32_t Configure(const Media::Format &format);
    int32_t SetParameter(const Media::Format &format);

    int32_t Process(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output);
    bool ShouldDropFrame(uint64_t pts);
    void FlushTimeStamp();
    void SetOnBufferReallocCallback(BufferReallocCallback callback);
    ConfigSnapshot GetConfigSnapshot() const;

private:
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
              width(params.right - params.left + 1),
              height(params.bottom - params.top + 1),
              enabled(params.result == ConfigResult::ENABLED)
        {
        }
    };

    struct DownsamplingConfig {
        int32_t width{0};
        int32_t height{0};
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

    struct RoiTransform {
        double scaleX;
        double scaleY;
        int32_t offsetTop;
        int32_t offsetLeft;
    };

    struct RuntimeConfig {
        CropConfig crop;
        DownsamplingConfig downsampling;
        BufferReallocCallback reallocCallback;
    };

    static bool IsAvailable();
    bool UsingAsyncMode(const Media::Format &format) const;
    int32_t ValidateAndApplyConfiguration(const Media::Format &format, bool configuring);
    int32_t ValidateConfiguration(const Media::Format &format, bool configuring);
    int32_t ApplyConfiguration();
    int32_t ValidateCrop(const Media::Format &format);
    int32_t ValidateDownsampling(const Media::Format &format);
    int32_t ValidateDropFrame(const Media::Format &format);
    GraphicPixelFormat GetPixelFormatFromConfiguration(const Media::Format &format) const;
    GraphicPixelFormat GetPixelFormatFromParameter(const Media::Format &format) const;
    RuntimeConfig GetRuntimeConfigSnapshot() const;
    int32_t Crop(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output, const CropConfig &config,
                 const BufferReallocCallback &callback);
    int32_t Downsampling(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output,
                         const DownsamplingConfig &config, const BufferReallocCallback &callback);
    int32_t Copy(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output, const BufferReallocCallback &callback);
    void UpdateOutputMetadata(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                              const RoiTransform &roiTransform);
    void CopyInputMetadata(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output);
    void UpdateRoiMetadata(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                           const RoiTransform &roiTransform);

    bool IsHevc10BitData(HEVCProfile profile) const;
    GraphicPixelFormat ConvertVideoPixelFormat2GraphicPixelFormat(VideoPixelFormat pixelFormat, int32_t profile) const;
    void UpdateConfiguredValues(const Media::Format &format, bool configuring);
    bool GetEncoderRange(const Media::Format &format, Range &encWidth, Range &encHeight);
    bool CheckOrReallocOutSurfaceBuffer(int32_t targetWidth, int32_t targetHeight,
                                        int32_t targetFormat, sptr<SurfaceBuffer> &output,
                                        const BufferReallocCallback &callback);

    EncoderConfig encoderParams_;               // for validation before configure
    CropParams cropParams_;                     // for validation before configure
    DownsamplingParams downsamplingParams_;     // for validation before configure
    DropFrameParams dropParams_;                // for validation before configure
    CropConfig cropConfig_;
    DownsamplingConfig downsamplingConfig_;
    DropFrameConfig dropFrameConfig_;
    EncoderConfig encoderConfig_;
    std::string mime_;
    PreProcessing::FastKitsInterface &fastKitsInterface_ = PreProcessing::FastKitsInterface::GetInstance();
    PreProcessing::FrameDropFilter dropFilter_;
    BufferReallocCallback onOutSurfaceBufferReallocCb_{};
    mutable std::mutex configMutex_;
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // PREPROCESSOR_H
