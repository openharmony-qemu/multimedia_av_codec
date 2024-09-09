/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef POST_PROCESSING_H
#define POST_PROCESSING_H

#include <array>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include "surface.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "codecbase.h"
#include "meta/format.h"
#include "media_description.h"
#include "controller.h"
#include "dynamic_controller.h"
#include "state_machine.h"
#include "utils.h"
#include "post_processing_callback.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PostProcessing {

template<typename T, typename = IsDerivedController<T>>
class PostProcessing {
public:
    static std::unique_ptr<PostProcessing<T>> Create(const std::shared_ptr<CodecBase> codec,
        const Format& format, int32_t& ret)
    {
        AVCODEC_SYNC_TRACE;
        auto p = std::make_unique<PostProcessing<T>>(codec);
        if (!p) {
            AVCODEC_LOGE("Create post processing failed");
            ret = AVCS_ERR_NO_MEMORY;
            return nullptr;
        }
        ret = p->Init(format);
        if (ret != AVCS_ERR_OK) {
            return nullptr;
        }
        return p;
    }

    explicit PostProcessing(std::shared_ptr<CodecBase> codec) : codec_(codec) {}

    ~PostProcessing()
    {
        callbackUserData_ = nullptr;
    }

    int32_t SetCallback(const Callback& callback, void* userData)
    {
        AVCODEC_SYNC_TRACE;
        callback_ = callback;
        callbackUserData_ = userData;
        return AVCS_ERR_OK;
    }

    int32_t SetOutputSurface(sptr<Surface> surface)
    {
        CHECK_AND_RETURN_RET_LOG(controller_, AVCS_ERR_UNKNOWN, "Post processing controller is null");
        AVCODEC_SYNC_TRACE;
        switch (state_.Get()) {
            case State::CONFIGURED:
                {
                    config_.outputSurface = surface;
                    return AVCS_ERR_OK;
                }
            case State::PREPARED:
                [[fallthrough]];
            case State::RUNNING:
                [[fallthrough]];
            case State::FLUSHED:
                [[fallthrough]];
            case State::STOPPED:
                {
                    int32_t ret = controller_->SetOutputSurface(surface);
                    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Set output surface failed");
                    config_.outputSurface = surface;
                    return ret;
                }
            default:
                {
                    AVCODEC_LOGE("Invalid post processing state: %{public}s", state_.Name());
                    return AVCS_ERR_UNKNOWN;
                }
        }
    }

    int32_t Prepare()
    {
        CHECK_AND_RETURN_RET_LOG(controller_, AVCS_ERR_UNKNOWN, "Post processing controller is null");
        CHECK_AND_RETURN_RET_LOG(state_.Get() == State::CONFIGURED, AVCS_ERR_INVALID_STATE,
            "Invalid post processing state: %{public}s", state_.Name());
        CHECK_AND_RETURN_RET_LOG(config_.outputSurface != nullptr, AVCS_ERR_INVALID_OPERATION,
            "Output surface is not set");

        AVCODEC_SYNC_TRACE;

        int32_t ret = controller_->Create();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Prepare failed");

        SetOutputSurfaceTransform();
        ret = controller_->SetOutputSurface(config_.outputSurface);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Prepare failed");

        ret = SetDecoderInputSurface();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Prepare failed");

        ret = controller_->SetCallback(static_cast<void*>(&callback_), callbackUserData_);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Prepare failed");

        ret = ConfigureController();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Prepare failed");

        ret = controller_->Prepare();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Prepare failed");

        state_.Set(State::PREPARED);
        return AVCS_ERR_OK;
    }

    int32_t Start()
    {
        CHECK_AND_RETURN_RET_LOG(controller_, AVCS_ERR_UNKNOWN, "Post processing controller is null");
        CHECK_AND_RETURN_RET_LOG(state_.Get() == State::PREPARED || state_.Get() == State::STOPPED ||
                                 state_.Get() == State::FLUSHED,
                                 AVCS_ERR_INVALID_OPERATION, "Post processing is not prepared");
        AVCODEC_SYNC_TRACE;
        int32_t ret = controller_->Start();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Start failed");
        state_.Set(State::RUNNING);
        return AVCS_ERR_OK;
    }

    int32_t Stop()
    {
        CHECK_AND_RETURN_RET_LOG(controller_, AVCS_ERR_UNKNOWN, "Post processing controller is null");
        CHECK_AND_RETURN_RET_LOG(state_.Get() == State::RUNNING || state_.Get() == State::FLUSHED,
                                 AVCS_ERR_INVALID_STATE, "Invalid post processing state: %{public}s", state_.Name());
        AVCODEC_SYNC_TRACE;
        int32_t ret = controller_->Stop();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Start failed");
        state_.Set(State::STOPPED);
        return AVCS_ERR_OK;
    }

    int32_t Flush()
    {
        CHECK_AND_RETURN_RET_LOG(controller_, AVCS_ERR_UNKNOWN, "Post processing controller is null");
        CHECK_AND_RETURN_RET_LOG(state_.Get() == State::RUNNING, AVCS_ERR_INVALID_STATE,
            "Invalid post processing state: %{public}s", state_.Name());
        AVCODEC_SYNC_TRACE;
        int32_t ret = controller_->Flush();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Flush failed");
        state_.Set(State::FLUSHED);
        return AVCS_ERR_OK;
    }

    int32_t Reset()
    {
        CHECK_AND_RETURN_RET_LOG(controller_, AVCS_ERR_UNKNOWN, "Post processing controller is null");
        AVCODEC_SYNC_TRACE;
        int32_t ret = controller_->Reset();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Reset failed");
        codec_.reset();
        state_.Set(State::DISABLED);
        return AVCS_ERR_OK;
    }

    int32_t Release()
    {
        CHECK_AND_RETURN_RET_LOG(controller_, AVCS_ERR_UNKNOWN, "Post processing controller is null");
        AVCODEC_SYNC_TRACE;
        config_.inputSurface = nullptr;
        config_.outputSurface = nullptr;
        controller_->Release();
        controller_->Destroy();
        controller_->UnloadInterfaces();
        codec_.reset();
        state_.Set(State::DISABLED);
        return AVCS_ERR_OK;
    }

    int32_t ReleaseOutputBuffer(uint32_t index, bool render)
    {
        CHECK_AND_RETURN_RET_LOG(controller_, AVCS_ERR_UNKNOWN, "Post processing controller is null");
        CHECK_AND_RETURN_RET_LOG(state_.Get() == State::RUNNING, AVCS_ERR_INVALID_STATE,
            "Invalid post processing state: %{public}s", state_.Name());
        AVCODEC_SYNC_TRACE;
        int32_t ret = controller_->ReleaseOutputBuffer(index, render);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "ReleaseOutputBuffer failed");
        return AVCS_ERR_OK;
    }

    void GetOutputFormat(Format& format)
    {
        CHECK_AND_RETURN_LOG(controller_, "Post processing controller is null");
        AVCODEC_SYNC_TRACE;
        int32_t ret = controller_->GetOutputFormat(format);
        CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "GetOutputFormat failed");
        return;
    }
private:
    int32_t Init(const Format& format)
    {
        controller_ = std::make_unique<T>();
        CHECK_AND_RETURN_RET_LOG(controller_, AVCS_ERR_NO_MEMORY, "Create post processing controller failed");
        CHECK_AND_RETURN_RET_LOG(controller_->LoadInterfaces(), AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION,
                                 "Initialize interfaces failed.");

        CreateConfiguration(format);

        CapabilityInfo output{
            .colorSpaceType = config_.outputColorSpaceType,
            .metadataType = config_.outputMetadataType,
            .pixelFormat = config_.outputPixelFormat
        };

        constexpr int32_t hdrVividVideoColorSpaceTypeList[]{
            0x440504, // BT2020 HLG Limit
            0x440404 // BT2020 PQ Limit
        };
        constexpr int32_t hdrVividVideoMetadataType{3}; // HDR Vivid Video
        constexpr int32_t hdrVividVideoPixelFormatList[]{35, 36};

        CapabilityInfo input;
        input.metadataType = hdrVividVideoMetadataType;
        bool supported{false};
        for (auto colorSpaceType : hdrVividVideoColorSpaceTypeList) {
            for (auto pixelFormat : hdrVividVideoPixelFormatList) {
                input.colorSpaceType = colorSpaceType,
                input.pixelFormat = pixelFormat,
                supported |= controller_->IsColorSpaceConversionSupported(input, output);
            }
        }
        CHECK_AND_RETURN_RET_LOG(supported, AVCS_ERR_UNSUPPORT, "No capability found");

        state_.Set(State::CONFIGURED);
        return AVCS_ERR_OK;
    }

    void CreateConfiguration(const Format& format)
    {
        format_ = format;
        constexpr int32_t colorSpaceTypeBt709Limited{0x410101}; // OH_COLORSPACE_BT709_LIMIT
        constexpr int32_t pixelFormatNV12{24}; // NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP
        constexpr int32_t pixelFormatNV21{25}; // NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP

        // the field is checked before
        int32_t width;
        (void)format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
        int32_t height;
        (void)format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);

        int32_t pixelFormat;
        if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, pixelFormat)) {
            pixelFormat = static_cast<int32_t>(VideoPixelFormat::NV12);
            (void)format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, pixelFormat);
        }
        int32_t rotation;
        if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, rotation)) {
            rotation = 0; // rotation 0
        }
        int32_t scalingMode;
        if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, scalingMode)) {
            scalingMode = 0; // No scaling
        }
        switch (pixelFormat) {
            case static_cast<int32_t>(VideoPixelFormat::NV12):
                pixelFormat = pixelFormatNV12;
                break;
            case static_cast<int32_t>(VideoPixelFormat::NV21):
                pixelFormat = pixelFormatNV21;
                break;
            default:
                AVCODEC_LOGE("Unsupported pixel format %{public}d", pixelFormat);
        }

        config_.width = width;
        config_.height = height;
        config_.outputColorSpaceType = colorSpaceTypeBt709Limited;
        config_.outputMetadataType = 0; // see OH_COLORSPACE_NONE
        config_.outputPixelFormat = pixelFormat;
        config_.rotation = rotation;
        config_.scalingMode = scalingMode;
    }

    int32_t SetDecoderInputSurface()
    {
        CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, AVCS_ERR_UNKNOWN, "Decoder is not found");
        sptr<Surface> surface = nullptr;
        int32_t ret = controller_->CreateInputSurface(surface);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK && surface != nullptr, ret, "Create input surface failed");
        GSError gsRet = surface->SetSurfaceSourceType(OHSurfaceSource::OH_SURFACE_SOURCE_VIDEO);
        EXPECT_AND_LOGW(gsRet != GSERROR_OK, "Set surface source type failed, %{public}s", GSErrorStr(gsRet).c_str());
        ret = codec_->SetOutputSurface(surface);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Set output surface of decoder failed");
        config_.inputSurface = surface;
        return ret;
    }

    void SetOutputSurfaceTransform()
    {
        CHECK_AND_RETURN_LOG(config_.outputSurface != nullptr, "Output surface is null");

        GSError err{GSERROR_OK};
        switch (config_.rotation) {
            case VIDEO_ROTATION_0:
                err = config_.outputSurface->SetTransform(GRAPHIC_ROTATE_NONE);
                break;
            case VIDEO_ROTATION_90:
                err = config_.outputSurface->SetTransform(GRAPHIC_ROTATE_270);
                break;
            case VIDEO_ROTATION_180:
                err = config_.outputSurface->SetTransform(GRAPHIC_ROTATE_180);
                break;
            case VIDEO_ROTATION_270:
                err = config_.outputSurface->SetTransform(GRAPHIC_ROTATE_90);
                break;
            default:
                break;
        }
        if (err != GSERROR_OK) {
            AVCODEC_LOGE("Set transform failed");
        }

        switch (config_.scalingMode) {
            case SCALING_MODE_SCALE_TO_WINDOW:
                [[fallthrough]];
            case SCALING_MODE_SCALE_CROP:
                err = config_.outputSurface->SetScalingMode(static_cast<ScalingMode>(config_.scalingMode));
                if (err != GSERROR_OK) {
                    AVCODEC_LOGE("Set transform failed");
                }
                break;
            default:
                break;
        }
    }

    int32_t ConfigureController()
    {
        constexpr std::string_view keyPrimaries{"colorspace_primaries"};
        constexpr std::string_view keyTransFunc{"colorspace_trans_func"};
        constexpr std::string_view keyMatrix{"colorspace_matrix"};
        constexpr std::string_view keyRange{"colorspace_range"};
        constexpr std::string_view keyMetadataType{"hdr_metadata_type"};
        constexpr std::string_view keyRenderIntent{"render_intent"};
        constexpr std::string_view keyPixelFormat{"pixel_format"};
        constexpr int32_t primaries{1};
        constexpr int32_t transFunc{1};
        constexpr int32_t matrix{1};
        constexpr int32_t range{2};
        constexpr int32_t metadataType{0};
        constexpr int32_t renderIntent{2};
        Format format(format_);
        format.PutIntValue(keyPrimaries, primaries);
        format.PutIntValue(keyTransFunc, transFunc);
        format.PutIntValue(keyMatrix, matrix);
        format.PutIntValue(keyRange, range);
        format.PutIntValue(keyMetadataType, metadataType);
        format.PutIntValue(keyRenderIntent, renderIntent);
        format.PutIntValue(keyPixelFormat, config_.outputPixelFormat);
        return controller_->Configure(format);
    }

    struct Configuration {
        int32_t width{0};
        int32_t height{0};
        int32_t inputColorSpaceType{0};
        int32_t inputMetadataType{0};
        int32_t inputPixelFormat{0};
        sptr<Surface> inputSurface{nullptr};
        int32_t outputColorSpaceType{0};
        int32_t outputMetadataType{0};
        int32_t outputPixelFormat{0};
        sptr<Surface> outputSurface{nullptr};
        int32_t rotation{0};
        int32_t scalingMode{0};
    };

    StateMachine state_;
    Configuration config_;
    Format format_;
    std::unique_ptr<Controller<T>> controller_{nullptr};
    std::shared_ptr<CodecBase> codec_{nullptr};
    static constexpr HiviewDFX::HiLogLabel LABEL{LogLabel("PostProcessing")};
    Callback callback_;
    void* callbackUserData_{nullptr};
};

using DynamicPostProcessing = PostProcessing<DynamicController>;

} // namespace PostProcessing
} // namespace OHOS
} // namespace MediaAVCodec

#endif // POST_PROCESSING_H