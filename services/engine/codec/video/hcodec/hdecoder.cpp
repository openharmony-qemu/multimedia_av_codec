/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "hdecoder.h"
#include <cassert>
#include <sys/ioctl.h>
#include <linux/dma-buf.h>
#include "hdf_base.h"
#include "codec_omx_ext.h"
#include "media_description.h"  // foundation/multimedia/av_codec/interfaces/inner_api/native/
#include "sync_fence.h"  // foundation/graphic/graphic_2d/utils/sync_fence/export/
#include "OMX_VideoExt.h"
#include "hcodec_log.h"
#include "hcodec_dfx.h"
#include "type_converter.h"
#include "surface_buffer.h"
#include "buffer_extra_data_impl.h"  // foundation/graphic/graphic_surface/surface/include/
#include "surface_tools.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace CodecHDI;

HDecoder::~HDecoder()
{
    MsgHandleLoop::Stop();
#ifdef USE_VIDEO_PROCESSING_ENGINE
    if (vpeHandle_ != nullptr) {
        if (VrrDestroyFunc_ != nullptr) {
            VrrDestroyFunc_(vrrHandle_);
        }
        dlclose(vpeHandle_);
        vpeHandle_ = nullptr;
    }
#endif
}

int32_t HDecoder::OnConfigure(const Format &format)
{
    configFormat_ = make_shared<Format>(format);

    SupportBufferType type;
    InitOMXParamExt(type);
    type.portIndex = OMX_DirOutput;
    if (GetParameter(OMX_IndexParamSupportBufferType, type) &&
        (type.bufferTypes & CODEC_BUFFER_TYPE_DYNAMIC_HANDLE) &&
        UseHandleOnOutputPort(true)) {
        HLOGI("dynamic mode");
        isDynamic_ = true;
    } else if (UseHandleOnOutputPort(false)) {
        HLOGI("normal mode");
        isDynamic_ = false;
    } else {
        HLOGE("invalid output buffer type");
        return AVCS_ERR_UNKNOWN;
    }
    int32_t ret = SetLowLatency(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    SaveTransform(format);
    SaveScaleMode(format);
    (void)SetProcessName();
    (void)SetFrameRateAdaptiveMode(format);
    (void)SetVrrEnable(format);
    return SetupPort(format);
}

int32_t HDecoder::SetupPort(const Format &format)
{
    int32_t width;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width) || width <= 0) {
        HLOGE("format should contain width");
        return AVCS_ERR_INVALID_VAL;
    }
    int32_t height;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height) || height <= 0) {
        HLOGE("format should contain height");
        return AVCS_ERR_INVALID_VAL;
    }
    HLOGI("user set width %d, height %d", width, height);
    if (!GetPixelFmtFromUser(format)) {
        return AVCS_ERR_INVALID_VAL;
    }

    optional<double> frameRate = GetFrameRateFromUser(format);
    if (frameRate.has_value()) {
        codecRate_ = frameRate.value();
    } else {
        HLOGI("user don't set valid frame rate, use default 60.0");
        frameRate = 60.0;  // default frame rate 60.0
    }

    PortInfo inputPortInfo {static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                            codingType_, std::nullopt, frameRate.value()};
    int32_t maxInputSize = 0;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, maxInputSize)) {
        if (maxInputSize > 0) {
            inputPortInfo.inputBufSize = static_cast<uint32_t>(maxInputSize);
        } else {
            HLOGW("user don't set valid input buffer size");
        }
    }

    int32_t ret = SetVideoPortInfo(OMX_DirInput, inputPortInfo);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    PortInfo outputPortInfo = {static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                               OMX_VIDEO_CodingUnused, configuredFmt_, frameRate.value()};
    ret = SetVideoPortInfo(OMX_DirOutput, outputPortInfo);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    return AVCS_ERR_OK;
}

int32_t HDecoder::UpdateInPortFormat()
{
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParam(def);
    def.nPortIndex = OMX_DirInput;
    if (!GetParameter(OMX_IndexParamPortDefinition, def)) {
        HLOGE("get input port definition failed");
        return AVCS_ERR_UNKNOWN;
    }
    PrintPortDefinition(def);
    if (inputFormat_ == nullptr) {
        inputFormat_ = make_shared<Format>();
    }
    inputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, def.format.video.nFrameWidth);
    inputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, def.format.video.nFrameHeight);
    return AVCS_ERR_OK;
}

bool HDecoder::UpdateConfiguredFmt(OMX_COLOR_FORMATTYPE portFmt)
{
    auto graphicFmt = static_cast<GraphicPixelFormat>(portFmt);
    if (graphicFmt != configuredFmt_.graphicFmt) {
        optional<PixelFmt> fmt = TypeConverter::GraphicFmtToFmt(graphicFmt);
        if (!fmt.has_value()) {
            return false;
        }
        HLOGI("GraphicPixelFormat need update: configured(%s) -> portdefinition(%s)",
            configuredFmt_.strFmt.c_str(), fmt->strFmt.c_str());
        configuredFmt_ = fmt.value();
        outputFormat_->PutStringValue("pixel_format_string", configuredFmt_.strFmt.c_str());
    }
    return true;
}

int32_t HDecoder::UpdateOutPortFormat()
{
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParam(def);
    def.nPortIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexParamPortDefinition, def)) {
        HLOGE("get output port definition failed");
        return AVCS_ERR_UNKNOWN;
    }
    PrintPortDefinition(def);
    if (def.nBufferCountActual == 0) {
        HLOGE("invalid bufferCount");
        return AVCS_ERR_UNKNOWN;
    }
    (void)UpdateConfiguredFmt(def.format.video.eColorFormat);

    uint32_t w = def.format.video.nFrameWidth;
    uint32_t h = def.format.video.nFrameHeight;

    // save into member variable
    OHOS::Rect damage{};
    GetCropFromOmx(w, h, damage);
    outBufferCnt_ = def.nBufferCountActual;
    requestCfg_.timeout = 0; // never wait when request
    requestCfg_.width = damage.w;
    requestCfg_.height = damage.h;
    requestCfg_.strideAlignment = STRIDE_ALIGNMENT;
    requestCfg_.format = configuredFmt_.graphicFmt;
    requestCfg_.usage = GetProducerUsage();

    // save into format
    if (outputFormat_ == nullptr) {
        outputFormat_ = make_shared<Format>();
    }
    if (!outputFormat_->ContainKey(MediaDescriptionKey::MD_KEY_WIDTH)) {
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, w); // deprecated
    }
    if (!outputFormat_->ContainKey(MediaDescriptionKey::MD_KEY_HEIGHT)) {
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, h); // deprecated
    }
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_DISPLAY_WIDTH, damage.w);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_DISPLAY_HEIGHT, damage.h);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, damage.w);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, damage.h);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(configuredFmt_.innerFmt));
    outputFormat_->PutIntValue("IS_VENDOR", 1);
    HLOGI("output format: %s", outputFormat_->Stringify().c_str());
    return AVCS_ERR_OK;
}

void HDecoder::UpdateColorAspects()
{
    CodecVideoColorspace param;
    InitOMXParamExt(param);
    param.portIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexColorAspects, param, true)) {
        return;
    }
    HLOGI("isFullRange %d, primary %u, transfer %u, matrix %u",
        param.aspects.range, param.aspects.primaries, param.aspects.transfer, param.aspects.matrixCoeffs);
    if (outputFormat_) {
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, param.aspects.range);
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, param.aspects.primaries);
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, param.aspects.transfer);
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, param.aspects.matrixCoeffs);
        HLOGI("output format changed: %s", outputFormat_->Stringify().c_str());
        callback_->OnOutputFormatChanged(*(outputFormat_.get()));
    }
}

void HDecoder::GetCropFromOmx(uint32_t w, uint32_t h, OHOS::Rect& damage)
{
    damage.x = 0;
    damage.y = 0;
    damage.w = static_cast<int32_t>(w);
    damage.h = static_cast<int32_t>(h);

    OMX_CONFIG_RECTTYPE rect;
    InitOMXParam(rect);
    rect.nPortIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexConfigCommonOutputCrop, rect, true)) {
        HLOGW("get crop failed, use default");
        return;
    }
    if (rect.nLeft < 0 || rect.nTop < 0 ||
        rect.nWidth == 0 || rect.nHeight == 0 ||
        rect.nLeft + static_cast<int32_t>(rect.nWidth) > static_cast<int32_t>(w) ||
        rect.nTop + static_cast<int32_t>(rect.nHeight) > static_cast<int32_t>(h)) {
        HLOGW("wrong crop rect (%d, %d, %u, %u) vs. frame (%u," \
              "%u), use default", rect.nLeft, rect.nTop, rect.nWidth, rect.nHeight, w, h);
        return;
    }
    HLOGI("crop rect (%d, %d, %u, %u)",
          rect.nLeft, rect.nTop, rect.nWidth, rect.nHeight);
    damage.x = rect.nLeft;
    damage.y = rect.nTop;
    damage.w = static_cast<int32_t>(rect.nWidth);
    damage.h = static_cast<int32_t>(rect.nHeight);
    if (outputFormat_) {
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_LEFT, rect.nLeft);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_TOP, rect.nTop);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_RIGHT,
            static_cast<int32_t>(rect.nLeft + rect.nWidth) - 1);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_BOTTOM,
            static_cast<int32_t>(rect.nTop + rect.nHeight) - 1);
        crop_.left = static_cast<uint32_t>(rect.nLeft);
        crop_.top = static_cast<uint32_t>(rect.nTop);
        crop_.width = rect.nWidth;
        crop_.height = rect.nHeight;
    }
}

void HDecoder::OnSetOutputSurface(const MsgInfo &msg, BufferOperationMode mode)
{
    sptr<Surface> surface;
    (void)msg.param->GetValue("surface", surface);
    if (mode == KEEP_BUFFER) {
        ReplyErrorCode(msg.id, OnSetOutputSurfaceWhenCfg(surface));
        return;
    }
    OnSetOutputSurfaceWhenRunning(surface, msg, mode);
}

int32_t HDecoder::OnSetOutputSurfaceWhenCfg(const sptr<Surface> &surface)
{
    SCOPED_TRACE();
    HLOGI(">>");
    if (surface == nullptr) {
        HLOGE("surface is null");
        return AVCS_ERR_INVALID_VAL;
    }
    if (surface->IsConsumer()) {
        HLOGE("expect a producer surface but got a consumer surface");
        return AVCS_ERR_INVALID_VAL;
    }
    int32_t ret = RegisterListenerToSurface(surface);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    currSurface_ = SurfaceItem(surface, compUniqueStr_);
    HLOGI("set surface(%" PRIu64 ")(%s) succ", surface->GetUniqueId(), surface->GetName().c_str());
    return AVCS_ERR_OK;
}

int32_t HDecoder::OnSetParameters(const Format &format)
{
    int32_t ret = SaveTransform(format, true);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = SaveScaleMode(format, true);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    optional<double> frameRate = GetFrameRateFromUser(format);
    if (frameRate.has_value()) {
        OMX_PARAM_U32TYPE framerateCfgType;
        InitOMXParam(framerateCfgType);
        framerateCfgType.nPortIndex = OMX_DirInput;
        framerateCfgType.nU32 = frameRate.value() * FRAME_RATE_COEFFICIENT;
        if (SetParameter(OMX_IndexCodecExtConfigOperatingRate, framerateCfgType, true)) {
            HLOGI("succ to set frameRate %.f", frameRate.value());
        } else {
            HLOGW("succ to set frameRate %.f", frameRate.value());
        }
        codecRate_ = frameRate.value();
    }
    (void)SetVrrEnable(format);
    return AVCS_ERR_OK;
}

int32_t HDecoder::SaveTransform(const Format &format, bool set)
{
    int32_t orientation = 0;
    if (format.GetIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, orientation)) {
        HLOGI("GraphicTransformType = %d", orientation);
        transform_ = static_cast<GraphicTransformType>(orientation);
        if (set) {
            return SetTransform();
        }
        return AVCS_ERR_OK;
    }
    int32_t rotate;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, rotate)) {
        return AVCS_ERR_OK;
    }
    optional<GraphicTransformType> transform = TypeConverter::InnerRotateToDisplayRotate(
        static_cast<VideoRotation>(rotate));
    if (!transform.has_value()) {
        return AVCS_ERR_INVALID_VAL;
    }
    HLOGI("VideoRotation = %d, GraphicTransformType = %d", rotate, transform.value());
    transform_ = transform.value();
    if (set) {
        return SetTransform();
    }
    return AVCS_ERR_OK;
}

int32_t HDecoder::SetTransform()
{
    if (currSurface_.surface_ == nullptr) {
        return AVCS_ERR_INVALID_VAL;
    }
    GSError err = currSurface_.surface_->SetTransform(transform_);
    if (err != GSERROR_OK) {
        HLOGW("set GraphicTransformType %d to surface failed", transform_);
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set GraphicTransformType %d to surface succ", transform_);
    return AVCS_ERR_OK;
}

int32_t HDecoder::SaveScaleMode(const Format &format, bool set)
{
    int scaleType;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, scaleType)) {
        return AVCS_ERR_OK;
    }
    auto scaleMode = static_cast<ScalingMode>(scaleType);
    if (scaleMode != SCALING_MODE_SCALE_TO_WINDOW && scaleMode != SCALING_MODE_SCALE_CROP) {
        HLOGW("user set invalid scale mode %d", scaleType);
        return AVCS_ERR_INVALID_VAL;
    }
    HLOGD("user set ScalingType = %d", scaleType);
    scaleMode_ = scaleMode;
    if (set) {
        return SetScaleMode();
    }
    return AVCS_ERR_OK;
}

int32_t HDecoder::SetScaleMode()
{
    if (currSurface_.surface_ == nullptr || !scaleMode_.has_value()) {
        return AVCS_ERR_INVALID_VAL;
    }
    GSError err = currSurface_.surface_->SetScalingMode(scaleMode_.value());
    if (err != GSERROR_OK) {
        HLOGW("set ScalingMode %d to surface failed", scaleMode_.value());
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set ScalingMode %d to surface succ", scaleMode_.value());
    return AVCS_ERR_OK;
}

int32_t HDecoder::SetVrrEnable(const Format &format)
{
    int32_t vrrEnable = 0;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_DECODER_OUTPUT_ENABLE_VRR, vrrEnable) || vrrEnable != 1) {
#ifdef USE_VIDEO_PROCESSING_ENGINE
        vrrDynamicSwitch_ = false;
        HLOGI("VRR vrrDynamicSwitch_ false");
#endif
        HLOGD("VRR disabled");
        return AVCS_ERR_OK;
    }
#ifdef USE_VIDEO_PROCESSING_ENGINE
    if (isVrrInitialized_) {
        vrrDynamicSwitch_ = true;
        HLOGI("VRR vrrDynamicSwitch_ true");
        return AVCS_ERR_OK;
    }
    optional<double> frameRate = GetFrameRateFromUser(format);
    if (!frameRate.has_value()) {
        HLOGE("VRR without frameRate");
        return AVCS_ERR_UNSUPPORT;
    }

    OMX_CONFIG_BOOLEANTYPE param {};
    InitOMXParam(param);
    param.bEnabled = OMX_TRUE;
    if (!SetParameter(OMX_IndexParamIsMvUpload, param)) {
        HLOGE("VRR SetIsMvUploadParam SetParameter failed");
        return AVCS_ERR_UNSUPPORT;
    }
    int32_t ret = InitVrr();
    if (ret != AVCS_ERR_OK) {
        HLOGE("VRR Init failed");
        return ret;
    }
    isVrrInitialized_ = true;
    vrrDynamicSwitch_ = true;
    HLOGI("VRR enabled");
    return AVCS_ERR_OK;
#else
    HLOGE("VRR unsupport");
    return AVCS_ERR_UNSUPPORT;
#endif
}

#ifdef USE_VIDEO_PROCESSING_ENGINE
int32_t HDecoder::InitVrr()
{
    if (vpeHandle_ != nullptr) {
        return AVCS_ERR_OK;
    }
    if (vpeHandle_ == nullptr) {
        vpeHandle_ = dlopen("libvideoprocessingengine.z.so", RTLD_NOW);
        if (vpeHandle_ == nullptr) {
            HLOGE("dlopen libvideoprocessingengine.z.so failed, dlerror: %{public}s", dlerror());
            return AVCS_ERR_UNSUPPORT;
        }
        HLOGI("dlopen libvideoprocessingengine.z.so success");
    }
    VrrCreateFunc_ = reinterpret_cast<VrrCreate>(dlsym(vpeHandle_, "VideoRefreshRatePredictionCreate"));
    VrrCheckSupportFunc_ = reinterpret_cast<VrrCheckSupport>(dlsym(vpeHandle_,
        "VideoRefreshRatePredictionCheckSupport"));
    VrrProcessFunc_ = reinterpret_cast<VrrProcess>(dlsym(vpeHandle_, "VideoRefreshRatePredictionProcess"));
    VrrDestroyFunc_ = reinterpret_cast<VrrDestroy>(dlsym(vpeHandle_, "VideoRefreshRatePredictionDestroy"));
    if (VrrCreateFunc_ == nullptr || VrrCheckSupportFunc_ == nullptr || VrrProcessFunc_ == nullptr ||
        VrrDestroyFunc_ == nullptr) {
        dlclose(vpeHandle_);
        vpeHandle_ = nullptr;
        return AVCS_ERR_UNSUPPORT;
    }
    vrrHandle_ = VrrCreateFunc_();
    int32_t ret = VrrCheckSupportFunc_(vrrHandle_, caller_.app.processName.c_str());
    if (ret != AVCS_ERR_OK) {
        HLOGE("VRR check ltpo support failed");
        VrrDestroyFunc_(vrrHandle_);
        dlclose(vpeHandle_);
        vpeHandle_ = nullptr;
        if (ret == Media::VideoProcessingEngine::VPE_ALGO_ERR_INVALID_OPERATION) {
            return AVCS_ERR_INVALID_OPERATION;
        }
        return AVCS_ERR_UNSUPPORT;
    }
    return AVCS_ERR_OK;
}
#endif

int32_t HDecoder::SubmitOutputBuffersToOmxNode()
{
    for (BufferInfo& info : outputBufferPool_) {
        if (info.owner != BufferOwner::OWNED_BY_US) {
            continue;
        }
        if (info.surfaceBuffer != nullptr) {
            int32_t ret = NotifyOmxToFillThisOutBuffer(info);
            if (ret != AVCS_ERR_OK) {
                return ret;
            }
        }
    }
    if (!isDynamic_) {
        return AVCS_ERR_OK;
    }
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParam(def);
    def.nPortIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexParamPortDefinition, def)) {
        HLOGE("get input port definition failed");
        return AVCS_ERR_UNKNOWN;
    }
    uint32_t outputBufferCnt = outPortHasChanged_ ? def.nBufferCountMin :
                                                    std::min<uint32_t>(def.nBufferCountMin, inTotalCnt_ + 1);
    HLOGI("submit buffer count[%u], inTotalCnt_[%u]", outputBufferCnt, inTotalCnt_);
    for (uint32_t i = 0; i < outputBufferCnt; i++) {
        DynamicModeSubmitBuffer();
    }
    DynamicModeSubmitIfEos();
    return AVCS_ERR_OK;
}

void HDecoder::DynamicModeSubmitIfEos()
{
    auto nullSlot = FindNullSlotIfDynamicMode();
    if (nullSlot != outputBufferPool_.end() && inputPortEos_ && !outputPortEos_) {
        SendAsyncMsg(MsgWhat::SUBMIT_DYNAMIC_IF_EOS, nullptr);
    }
}

bool HDecoder::UseHandleOnOutputPort(bool isDynamic)
{
    UseBufferType useBufferTypes;
    InitOMXParamExt(useBufferTypes);
    useBufferTypes.portIndex = OMX_DirOutput;
    useBufferTypes.bufferType = (isDynamic ? CODEC_BUFFER_TYPE_DYNAMIC_HANDLE : CODEC_BUFFER_TYPE_HANDLE);
    return SetParameter(OMX_IndexParamUseBufferType, useBufferTypes);
}

bool HDecoder::ReadyToStart()
{
    if (callback_ == nullptr || outputFormat_ == nullptr || inputFormat_ == nullptr) {
        return false;
    }
    if (currSurface_.surface_) {
        HLOGI("surface mode");
        cfgedConsumerUsage = currSurface_.surface_->GetDefaultUsage();
        SetTransform();
        SetScaleMode();
    } else {
        HLOGI("buffer mode");
    }
    return true;
}

int32_t HDecoder::AllocateBuffersOnPort(OMX_DIRTYPE portIndex)
{
    if (portIndex == OMX_DirInput) {
        return AllocateAvLinearBuffers(portIndex);
    }
    int32_t ret;
    if (isDynamic_) {
        ret = AllocOutDynamicSurfaceBuf();
    } else {
        ret = (currSurface_.surface_ ? AllocateOutputBuffersFromSurface() : AllocateAvSurfaceBuffers(portIndex));
    }
    if (ret == AVCS_ERR_OK) {
        UpdateFormatFromSurfaceBuffer();
    }
    return ret;
}

void HDecoder::SetCallerToBuffer(int fd)
{
    if (currSurface_.surface_ == nullptr) {
        return; // only set on surface mode
    }
    string pid = std::to_string(caller_.app.pid);
    int ret = ioctl(fd, DMA_BUF_SET_NAME_A, pid.c_str());
    if (ret != 0) {
        HLOGD("set pid %s to fd %d failed", pid.c_str(), fd);
        return;
    }
    HLOGD("set pid %s to fd %d succ", pid.c_str(), fd);
}

void HDecoder::UpdateFormatFromSurfaceBuffer()
{
    if (outputBufferPool_.empty()) {
        return;
    }
    sptr<SurfaceBuffer> surfaceBuffer = outputBufferPool_.front().surfaceBuffer;
    if (surfaceBuffer == nullptr) {
        return;
    }
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_DISPLAY_WIDTH, surfaceBuffer->GetWidth());
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_DISPLAY_HEIGHT, surfaceBuffer->GetHeight());
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, surfaceBuffer->GetWidth());
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, surfaceBuffer->GetHeight());
    int32_t stride = surfaceBuffer->GetStride();
    if (stride <= 0) {
        HLOGW("invalid stride %d", stride);
        return;
    }
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, stride);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, stride); // deprecated

    OH_NativeBuffer_Planes *planes = nullptr;
    GSError err = surfaceBuffer->GetPlanesInfo(reinterpret_cast<void**>(&planes));
    if (err != GSERROR_OK || planes == nullptr) {
        HLOGI("can not get plane info, ignore");
        return;
    }
    for (uint32_t i = 0; i < planes->planeCount; i++) {
        HLOGI("plane[%u]: offset=%" PRIu64 ", rowStride=%u, columnStride=%u",
              i, planes->planes[i].offset, planes->planes[i].rowStride, planes->planes[i].columnStride);
    }
    int32_t sliceHeight = static_cast<int32_t>(static_cast<int64_t>(planes->planes[1].offset) / stride);
    HLOGI("[%dx%d][%dx%d]", surfaceBuffer->GetWidth(), surfaceBuffer->GetHeight(), stride, sliceHeight);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_SLICE_HEIGHT, sliceHeight);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, sliceHeight);
}

static bool IsNotSame(const OHOS::HDI::Display::Graphic::Common::V1_0::BufferHandleMetaRegion& crop1,
                      const OHOS::HDI::Display::Graphic::Common::V1_0::BufferHandleMetaRegion& crop2)
{
    return crop1.left != crop2.left ||
           crop1.top != crop2.top ||
           crop1.width != crop2.width ||
           crop1.height != crop2.height;
}

void HDecoder::BeforeCbOutToUser(BufferInfo &info)
{
    using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
    std::vector<uint8_t> vec;
    GSError err = info.surfaceBuffer->GetMetadata(ATTRKEY_CROP_REGION, vec);
    if (err != GSERROR_OK || vec.size() != sizeof(BufferHandleMetaRegion)) {
        return;
    }
    auto* newCrop = reinterpret_cast<BufferHandleMetaRegion*>(vec.data());
    if (IsNotSame(crop_, *newCrop)) {
        HLOGI("crop update: left/top/width/height, %u/%u/%u/%u -> %u/%u/%u/%u",
            crop_.left, crop_.top, crop_.width, crop_.height,
            newCrop->left, newCrop->top, newCrop->width, newCrop->height);
        crop_ = *newCrop;
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_DISPLAY_WIDTH, newCrop->width);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_DISPLAY_HEIGHT, newCrop->height);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, newCrop->width);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, newCrop->height);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_LEFT, newCrop->left);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_TOP, newCrop->top);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_RIGHT,
            static_cast<int32_t>(newCrop->left + newCrop->width) - 1);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_BOTTOM,
            static_cast<int32_t>(newCrop->top + newCrop->height) - 1);
        HLOGI("output format changed: %s", outputFormat_->Stringify().c_str());
        callback_->OnOutputFormatChanged(*(outputFormat_.get()));
    }
}

int32_t HDecoder::SubmitAllBuffersOwnedByUs()
{
    HLOGD(">>");
    if (isBufferCirculating_) {
        HLOGI("buffer is already circulating, no need to do again");
        return AVCS_ERR_OK;
    }
    int32_t ret = SubmitOutputBuffersToOmxNode();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    for (BufferInfo& info : inputBufferPool_) {
        if (info.owner == BufferOwner::OWNED_BY_US) {
            NotifyUserToFillThisInBuffer(info);
        }
    }
    isBufferCirculating_ = true;
    return AVCS_ERR_OK;
}

void HDecoder::EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i)
{
    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    if (i >= pool.size()) {
        return;
    }
    BufferInfo& info = pool[i];
    FreeOmxBuffer(portIndex, info);
    ReduceOwner((portIndex == OMX_DirInput), info.owner);
    pool.erase(pool.begin() + i);
}

void HDecoder::OnClearBufferPool(OMX_DIRTYPE portIndex)
{
    if ((portIndex == OMX_DirOutput) && currSurface_.surface_) {
        SurfaceTools::GetInstance().CleanCache(compUniqueStr_, currSurface_.surface_, false);
        freeList_.clear();
    }
}

uint64_t HDecoder::GetProducerUsage()
{
    uint64_t producerUsage = currSurface_.surface_ ? SURFACE_MODE_PRODUCER_USAGE : BUFFER_MODE_REQUEST_USAGE;

    GetBufferHandleUsageParams vendorUsage;
    InitOMXParamExt(vendorUsage);
    vendorUsage.portIndex = static_cast<uint32_t>(OMX_DirOutput);
    if (GetParameter(OMX_IndexParamGetBufferHandleUsage, vendorUsage)) {
        HLOGI("vendor producer usage = 0x%" PRIx64 "", vendorUsage.usage);
        producerUsage |= vendorUsage.usage;
    } else {
        HLOGW("get vendor producer usage failed, add CPU_READ");
        producerUsage |= BUFFER_USAGE_CPU_READ;
    }
    HLOGI("decoder producer usage = 0x%" PRIx64 "", producerUsage);
    return producerUsage;
}

void HDecoder::CombineConsumerUsage()
{
    uint32_t consumerUsage = currSurface_.surface_->GetDefaultUsage();
    if (currSurface_.surface_->GetName().find("SurfaceImage") != string::npos) {
        consumerUsage |= BUFFER_USAGE_HW_COMPOSER;
    }
    uint64_t finalUsage = requestCfg_.usage | consumerUsage | cfgedConsumerUsage;
    HLOGI("producer 0x%" PRIx64 " | consumer 0x%" PRIx64 " | cfg 0x%" PRIx64 " -> 0x%" PRIx64 "",
        requestCfg_.usage, consumerUsage, cfgedConsumerUsage, finalUsage);
    requestCfg_.usage = finalUsage;
}

int32_t HDecoder::ClearSurfaceAndSetQueueSize(const sptr<Surface> &surface, uint32_t targetSize)
{
    surface->Connect(); // cleancache will work only if the surface is connected by us
    SurfaceTools::GetInstance().CleanCache(compUniqueStr_, surface, false);
    GSError err = surface->SetQueueSize(targetSize);
    if (err != GSERROR_OK) {
        HLOGE("surface(%" PRIu64 "), SetQueueSize to %u failed, GSError=%d",
              surface->GetUniqueId(), targetSize, err);
        return AVCS_ERR_UNKNOWN;
    }
    for (BufferInfo& info : outputBufferPool_) {
        info.attached = false;
    }
    HLOGI("surface(%" PRIu64 "), SetQueueSize to %u succ", surface->GetUniqueId(), targetSize);
    return AVCS_ERR_OK;
}

int32_t HDecoder::AllocOutDynamicSurfaceBuf()
{
    SCOPED_TRACE();
    if (currSurface_.surface_) {
        int32_t ret = ClearSurfaceAndSetQueueSize(currSurface_.surface_, outBufferCnt_);
        if (ret != AVCS_ERR_OK) {
            return ret;
        }
        currGeneration_++;
        CombineConsumerUsage();
    }
    outputBufferPool_.clear();

    for (uint32_t i = 0; i < outBufferCnt_; ++i) {
        shared_ptr<OmxCodecBuffer> omxBuffer = DynamicSurfaceBufferToOmxBuffer();
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t ret = compNode_->UseBuffer(OMX_DirOutput, *omxBuffer, *outBuffer);
        if (ret != HDF_SUCCESS) {
            HLOGE("Failed to UseBuffer on input port");
            return AVCS_ERR_UNKNOWN;
        }
        BufferInfo info {};
        info.isInput = false;
        info.owner = BufferOwner::OWNED_BY_US;
        info.surfaceBuffer = nullptr;
        info.avBuffer = (currSurface_.surface_ ? AVBuffer::CreateAVBuffer() : nullptr);
        info.omxBuffer = outBuffer;
        info.bufferId = outBuffer->bufferId;
        outputBufferPool_.push_back(info);
    }
    HLOGI("succ");
    return AVCS_ERR_OK;
}

int32_t HDecoder::AllocateOutputBuffersFromSurface()
{
    SCOPED_TRACE();
    int32_t ret = ClearSurfaceAndSetQueueSize(currSurface_.surface_, outBufferCnt_);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    currGeneration_++;
    outputBufferPool_.clear();
    CombineConsumerUsage();
    for (uint32_t i = 0; i < outBufferCnt_; ++i) {
        sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
        if (surfaceBuffer == nullptr) {
            return AVCS_ERR_UNKNOWN;
        }
        GSError err = surfaceBuffer->Alloc(requestCfg_);
        if (err != GSERROR_OK) {
            HLOGE("Alloc surfacebuffer %u failed, GSError=%d", i, err);
            return err == GSERROR_NO_MEM ? AVCS_ERR_NO_MEMORY : AVCS_ERR_UNKNOWN;
        }
        shared_ptr<OmxCodecBuffer> omxBuffer = SurfaceBufferToOmxBuffer(surfaceBuffer);
        if (omxBuffer == nullptr) {
            return AVCS_ERR_UNKNOWN;
        }
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t hdfRet = compNode_->UseBuffer(OMX_DirOutput, *omxBuffer, *outBuffer);
        if (hdfRet != HDF_SUCCESS) {
            HLOGE("Failed to UseBuffer with output port");
            return AVCS_ERR_NO_MEMORY;
        }
        SetCallerToBuffer(surfaceBuffer->GetFileDescriptor());
        outBuffer->fenceFd = -1;
        BufferInfo info {};
        info.isInput = false;
        info.owner = BufferOwner::OWNED_BY_US;
        info.surfaceBuffer = surfaceBuffer;
        info.avBuffer = AVBuffer::CreateAVBuffer();
        info.omxBuffer = outBuffer;
        info.bufferId = outBuffer->bufferId;
        info.attached = false;
        outputBufferPool_.push_back(info);
        HLOGI("generation=%d, bufferId=%u, seq=%u", currGeneration_, info.bufferId, surfaceBuffer->GetSeqNum());
    }
    return AVCS_ERR_OK;
}

int32_t HDecoder::RegisterListenerToSurface(const sptr<Surface> &surface)
{
    uint64_t surfaceId = surface->GetUniqueId();
    std::weak_ptr<MsgToken> weakThis = m_token;
    bool ret = SurfaceTools::GetInstance().RegisterReleaseListener(compUniqueStr_, surface,
        [weakThis, surfaceId](sptr<SurfaceBuffer>&) {
        std::shared_ptr<MsgToken> codec = weakThis.lock();
        if (codec == nullptr) {
            LOGD("decoder is gone");
            return GSERROR_OK;
        }
        ParamSP param = make_shared<ParamBundle>();
        param->SetValue("surfaceId", surfaceId);
        codec->SendAsyncMsg(MsgWhat::GET_BUFFER_FROM_SURFACE, param);
        return GSERROR_OK;
    });
    if (!ret) {
        HLOGE("surface(%" PRIu64 "), RegisterReleaseListener failed", surfaceId);
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

HDecoder::SurfaceBufferItem HDecoder::RequestBuffer()
{
    SurfaceBufferItem item{};
    item.generation = currGeneration_;
    GSError err = currSurface_.surface_->RequestBuffer(item.buffer, item.fence, requestCfg_);
    if (err != GSERROR_OK || item.buffer == nullptr || item.buffer->GetBufferHandle() == nullptr) {
        HLOGW("RequestBuffer failed, GSError=%d", err);
        return { nullptr, nullptr };
    }
    return item;
}

std::vector<HCodec::BufferInfo>::iterator HDecoder::FindBelongTo(sptr<SurfaceBuffer>& buffer)
{
    BufferHandle* handle = buffer->GetBufferHandle();
    return std::find_if(outputBufferPool_.begin(), outputBufferPool_.end(), [handle](const BufferInfo& info) {
        return (info.owner == BufferOwner::OWNED_BY_SURFACE) &&
               info.surfaceBuffer && (info.surfaceBuffer->GetBufferHandle() == handle);
    });
}

std::vector<HCodec::BufferInfo>::iterator HDecoder::FindNullSlotIfDynamicMode()
{
    if (!isDynamic_) {
        return outputBufferPool_.end();
    }
    return std::find_if(outputBufferPool_.begin(), outputBufferPool_.end(), [](const BufferInfo& info) {
        return info.surfaceBuffer == nullptr;
    });
}

void HDecoder::OnGetBufferFromSurface(const ParamSP& param)
{
    SCOPED_TRACE();
    uint64_t surfaceId = 0;
    param->GetValue("surfaceId", surfaceId);
    if (!currSurface_.surface_ || currSurface_.surface_->GetUniqueId() != surfaceId) {
        return;
    }
    SurfaceBufferItem item = RequestBuffer();
    if (item.buffer == nullptr) {
        return;
    }
    HitraceMeterFmtScoped tracePoint(HITRACE_TAG_ZMEDIA, "requested: %u", item.buffer->GetSeqNum());
    freeList_.push_back(item); // push to list, retrive it later, to avoid wait fence too early
    static constexpr size_t MAX_CACHE_CNT = 2;
    if (item.fence == nullptr || !item.fence->IsValid() || freeList_.size() > MAX_CACHE_CNT) {
        SurfaceModeSubmitBufferFromFreeList();
    }
}

void HDecoder::DynamicModeSubmitBuffer()
{
    auto nullSlot = FindNullSlotIfDynamicMode();
    if (nullSlot != outputBufferPool_.end()) {
        DynamicModeSubmitBufferToSlot(nullSlot);
    }
}

void HDecoder::SurfaceModeSubmitBuffer()
{
    auto nullSlot = FindNullSlotIfDynamicMode();
    if (nullSlot != outputBufferPool_.end()) {
        DynamicModeSubmitBufferToSlot(nullSlot);
    } else {
        SurfaceModeSubmitBufferFromFreeList();
    }
}

void HDecoder::SurfaceModeSubmitBufferFromFreeList()
{
    while (!freeList_.empty()) {
        SurfaceBufferItem item = freeList_.front();
        freeList_.pop_front();
        if (SurfaceModeSubmitOneItem(item)) {
            return;
        }
    }
}

bool HDecoder::SurfaceModeSubmitOneItem(SurfaceBufferItem& item)
{
    SCOPED_TRACE_FMT("seq: %u", item.buffer->GetSeqNum());
    if (item.generation != currGeneration_) {
        HLOGD("buffer generation %d != current generation %d, ignore", item.generation, currGeneration_);
        return false;
    }
    auto iter = FindBelongTo(item.buffer);
    if (iter == outputBufferPool_.end()) {
        auto nullSlot = FindNullSlotIfDynamicMode();
        if (nullSlot != outputBufferPool_.end()) {
            HLOGI("seq=%u dont belong to output set, bind as dynamic", item.buffer->GetSeqNum());
            WaitFence(item.fence);
            DynamicModeSubmitBufferToSlot(item.buffer, nullSlot);
            nullSlot->attached = true;
            return true;
        }
        HLOGI("seq=%u dont belong to output set, ignore", item.buffer->GetSeqNum());
        return false;
    }
    WaitFence(item.fence);
    ChangeOwner(*iter, BufferOwner::OWNED_BY_US);
    NotifyOmxToFillThisOutBuffer(*iter);
    return true;
}

void HDecoder::DynamicModeSubmitBufferToSlot(std::vector<BufferInfo>::iterator nullSlot)
{
    SCOPED_TRACE();
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    IF_TRUE_RETURN_VOID_WITH_MSG(buffer == nullptr, "CreateSurfaceBuffer failed");
    GSError err = buffer->Alloc(requestCfg_);
    IF_TRUE_RETURN_VOID_WITH_MSG(err != GSERROR_OK, "AllocSurfaceBuffer failed");
    DynamicModeSubmitBufferToSlot(buffer, nullSlot);
}

void HDecoder::DynamicModeSubmitBufferToSlot(sptr<SurfaceBuffer>& buffer, std::vector<BufferInfo>::iterator nullSlot)
{
    if (currSurface_.surface_) {
        HLOGI("generation=%d, bufferId=%u, seq=%u", currGeneration_, nullSlot->bufferId, buffer->GetSeqNum());
    } else {
        std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(buffer);
        IF_TRUE_RETURN_VOID_WITH_MSG(avBuffer == nullptr || avBuffer->memory_ == nullptr, "CreateAVBuffer failed");
        nullSlot->avBuffer = avBuffer;
        nullSlot->needDealWithCache = (requestCfg_.usage & BUFFER_USAGE_MEM_MMZ_CACHE);
        HLOGI("bufferId=%u, seq=%u", nullSlot->bufferId, buffer->GetSeqNum());
    }
    SetCallerToBuffer(buffer->GetFileDescriptor());
    WrapSurfaceBufferToSlot(*nullSlot, buffer, 0, 0);
    if (nullSlot == outputBufferPool_.begin()) {
        UpdateFormatFromSurfaceBuffer();
    }
    NotifyOmxToFillThisOutBuffer(*nullSlot);
    nullSlot->omxBuffer->bufferhandle = nullptr;
}

int32_t HDecoder::Attach(BufferInfo &info)
{
    if (info.attached) {
        return AVCS_ERR_OK;
    }
    GSError err = currSurface_.surface_->AttachBufferToQueue(info.surfaceBuffer);
    if (err != GSERROR_OK) {
        HLOGW("surface(%" PRIu64 "), AttachBufferToQueue(seq=%u) failed, GSError=%d",
            currSurface_.surface_->GetUniqueId(), info.surfaceBuffer->GetSeqNum(), err);
        return AVCS_ERR_UNKNOWN;
    }
    info.attached = true;
    return AVCS_ERR_OK;
}

int32_t HDecoder::NotifySurfaceToRenderOutputBuffer(BufferInfo &info)
{
    info.lastFlushTime = GetNowUs();
    sptr<BufferExtraData> extraData = new BufferExtraDataImpl();
    extraData->ExtraSet("VIDEO_RATE", codecRate_);
    info.surfaceBuffer->SetExtraData(extraData);
    BufferFlushConfig cfg {
        .damage = {.x = 0, .y = 0, .w = info.surfaceBuffer->GetWidth(), .h = info.surfaceBuffer->GetHeight() },
        .timestamp = info.omxBuffer->pts,
        .desiredPresentTimestamp = -1,
    };
    if (info.avBuffer->meta_->Find(OHOS::Media::Tag::VIDEO_DECODER_DESIRED_PRESENT_TIMESTAMP) !=
        info.avBuffer->meta_->end()) {
        info.avBuffer->meta_->Get<OHOS::Media::Tag::VIDEO_DECODER_DESIRED_PRESENT_TIMESTAMP>(
            cfg.desiredPresentTimestamp);
        info.avBuffer->meta_->Remove(OHOS::Media::Tag::VIDEO_DECODER_DESIRED_PRESENT_TIMESTAMP);
    }
    SCOPED_TRACE_FMT("id: %u, pts: %" PRId64 ", desiredPts: %" PRId64,
        info.bufferId, cfg.timestamp, cfg.desiredPresentTimestamp);

    int32_t ret = Attach(info);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    GSError err = currSurface_.surface_->FlushBuffer(info.surfaceBuffer, -1, cfg);
    if (err == GSERROR_BUFFER_NOT_INCACHE) {
        HLOGW("surface(%" PRIu64 "), FlushBuffer(seq=%u) failed, BUFFER_NOT_INCACHE, try to recover",
              currSurface_.surface_->GetUniqueId(), info.surfaceBuffer->GetSeqNum(), err);
        ret = ClearSurfaceAndSetQueueSize(currSurface_.surface_, outputBufferPool_.size());
        if (ret != AVCS_ERR_OK) {
            return ret;
        }
        ret = Attach(info);
        if (ret != AVCS_ERR_OK) {
            return ret;
        }
        err = currSurface_.surface_->FlushBuffer(info.surfaceBuffer, -1, cfg);
    }
    if (err != GSERROR_OK) {
        HLOGW("surface(%" PRIu64 "), FlushBuffer(seq=%u) failed, GSError=%d",
              currSurface_.surface_->GetUniqueId(), info.surfaceBuffer->GetSeqNum(), err);
        return AVCS_ERR_UNKNOWN;
    }
    ChangeOwner(info, BufferOwner::OWNED_BY_SURFACE);
    return AVCS_ERR_OK;
}

void HDecoder::OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode)
{
    SCOPED_TRACE_FMT("id: %u", bufferId);
    BufferInfo *info = FindBufferInfoByID(OMX_DirInput, bufferId);
    if (info == nullptr) {
        HLOGE("unknown buffer id %u", bufferId);
        return;
    }
    if (info->owner != BufferOwner::OWNED_BY_OMX) {
        HLOGE("wrong ownership: buffer id=%d, owner=%s", bufferId, ToString(info->owner));
        return;
    }
    ChangeOwner(*info, BufferOwner::OWNED_BY_US);
    switch (mode) {
        case KEEP_BUFFER:
            return;
        case RESUBMIT_BUFFER: {
            if (!inputPortEos_) {
                NotifyUserToFillThisInBuffer(*info);
            }
            return;
        }
        default: {
            HLOGE("SHOULD NEVER BE HERE");
            return;
        }
    }
}

void HDecoder::OnReleaseOutputBuffer(const BufferInfo &info)
{
    if (currSurface_.surface_) {
        if (debugMode_) {
            HLOGI("outBufId = %u, discard by user, pts = %" PRId64, info.bufferId, info.omxBuffer->pts);
        } else {
            outputDiscardCnt_++;
        }
    }
}

void HDecoder::OnRenderOutputBuffer(const MsgInfo &msg, BufferOperationMode mode)
{
    if (currSurface_.surface_ == nullptr) {
        HLOGE("can only render in surface mode");
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_OPERATION);
        return;
    }
    uint32_t bufferId = 0;
    (void)msg.param->GetValue(BUFFER_ID, bufferId);
    SCOPED_TRACE_FMT("id: %u", bufferId);
    optional<size_t> idx = FindBufferIndexByID(OMX_DirOutput, bufferId);
    if (!idx.has_value()) {
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    BufferInfo& info = outputBufferPool_[idx.value()];
    if (info.owner != BufferOwner::OWNED_BY_USER) {
        HLOGE("wrong ownership: buffer id=%d, owner=%s", bufferId, ToString(info.owner));
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    info.omxBuffer->pts = info.avBuffer->pts_;
    ChangeOwner(info, BufferOwner::OWNED_BY_US);
    ReplyErrorCode(msg.id, AVCS_ERR_OK);
    if (mode == KEEP_BUFFER) {
        return;
    }
    if (info.omxBuffer->filledLen != 0) {
        NotifySurfaceToRenderOutputBuffer(info);
    }
    if (mode == FREE_BUFFER) {
        EraseBufferFromPool(OMX_DirOutput, idx.value());
    } else {
        SurfaceModeSubmitBuffer();
    }
}

void HDecoder::OnEnterUninitializedState()
{
    freeList_.clear();
    currSurface_.Release();
    crop_ = {0};
    cfgedConsumerUsage = 0;
}

HDecoder::SurfaceItem::SurfaceItem(const sptr<Surface> &surface, std::string codecId)
    : surface_(surface), originalTransform_(surface->GetTransform()), compUniqueStr_(codecId) {}

void HDecoder::SurfaceItem::Release(bool cleanAll)
{
    if (surface_) {
        LOGI("release surface(%" PRIu64 ")", surface_->GetUniqueId());
        if (originalTransform_.has_value()) {
            surface_->SetTransform(originalTransform_.value());
            originalTransform_ = std::nullopt;
        }
        SurfaceTools::GetInstance().ReleaseSurface(compUniqueStr_, surface_, cleanAll);
        surface_ = nullptr;
    }
}

void HDecoder::OnSetOutputSurfaceWhenRunning(const sptr<Surface> &newSurface,
    const MsgInfo &msg, BufferOperationMode mode)
{
    SCOPED_TRACE();
    if (currSurface_.surface_ == nullptr) {
        HLOGE("can only switch surface on surface mode");
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_OPERATION);
        return;
    }
    if (newSurface == nullptr) {
        HLOGE("surface is null");
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    if (newSurface->IsConsumer()) {
        HLOGE("expect a producer surface but got a consumer surface");
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    uint64_t oldId = currSurface_.surface_->GetUniqueId();
    uint64_t newId = newSurface->GetUniqueId();
    HLOGI("surface %" PRIu64 " -> %" PRIu64, oldId, newId);
    if (oldId == newId) {
        HLOGI("same surface, no need to set again");
        ReplyErrorCode(msg.id, AVCS_ERR_OK);
        return;
    }
    int32_t ret = RegisterListenerToSurface(newSurface);
    if (ret != AVCS_ERR_OK) {
        ReplyErrorCode(msg.id, ret);
        return;
    }
    ret = ClearSurfaceAndSetQueueSize(newSurface, outBufferCnt_);
    if (ret != AVCS_ERR_OK) {
        ReplyErrorCode(msg.id, ret);
        return;
    }
    SwitchBetweenSurface(newSurface, msg, mode);
}

void HDecoder::ConsumeFreeList(BufferOperationMode mode)
{
    SCOPED_TRACE();
    while (!freeList_.empty()) {
        SurfaceBufferItem item = freeList_.front();
        freeList_.pop_front();
        auto iter = FindBelongTo(item.buffer);
        if (iter == outputBufferPool_.end()) {
            continue;
        }
        ChangeOwner(*iter, BufferOwner::OWNED_BY_US);
        if (mode == RESUBMIT_BUFFER) {
            WaitFence(item.fence);
            NotifyOmxToFillThisOutBuffer(*iter);
        }
    }
}

void HDecoder::ClassifyOutputBufferOwners(vector<size_t>& ownedByUs,
                                          map<int64_t, size_t>& ownedBySurfaceFlushTime2BufferIndex)
{
    for (size_t i = 0; i < outputBufferPool_.size(); i++) {
        BufferInfo& info = outputBufferPool_[i];
        if (info.surfaceBuffer == nullptr) {
            continue;
        }
        if (info.owner == OWNED_BY_SURFACE) {
            ownedBySurfaceFlushTime2BufferIndex[info.lastFlushTime] = i;
        } else if (info.owner == OWNED_BY_US) {
            ownedByUs.push_back(i);
        }
    }
}

void HDecoder::SwitchBetweenSurface(const sptr<Surface> &newSurface,
    const MsgInfo &msg, BufferOperationMode mode)
{
    SCOPED_TRACE();
    BufferRequestConfig cfg = requestCfg_;
    cfg.usage |= newSurface->GetDefaultUsage();
    uint64_t newId = newSurface->GetUniqueId();
    for (size_t i = 0; i < outputBufferPool_.size(); i++) {
        BufferInfo& info = outputBufferPool_[i];
        if (info.surfaceBuffer == nullptr) {
            continue;
        }
        // since bufferqueue use BufferRequestConfig to decide to do reuse/alloc,
        // we need to update consumer usage of new surface to surfacebuffer to avoid alloc
        info.surfaceBuffer->SetBufferRequestConfig(cfg);
        GSError err = newSurface->AttachBufferToQueue(info.surfaceBuffer);
        if (err != GSERROR_OK) {
            HLOGE("surface(%" PRIu64 "), AttachBufferToQueue(seq=%u) failed, GSError=%d",
                  newId, info.surfaceBuffer->GetSeqNum(), err);
            ReplyErrorCode(msg.id, AVCS_ERR_UNKNOWN);
            return;
        }
        info.attached = true;
    }
    ReplyErrorCode(msg.id, AVCS_ERR_OK);

    ConsumeFreeList(mode);
    map<int64_t, size_t> ownedBySurfaceFlushTime2BufferIndex;
    vector<size_t> ownedByUs;
    ClassifyOutputBufferOwners(ownedByUs, ownedBySurfaceFlushTime2BufferIndex);

    SurfaceItem oldSurface = currSurface_;
    currSurface_ = SurfaceItem(newSurface, compUniqueStr_);
    CombineConsumerUsage();
    // if owned by old surface, we need to transfer them to new surface
    for (auto [flushTime, i] : ownedBySurfaceFlushTime2BufferIndex) {
        ChangeOwner(outputBufferPool_[i], BufferOwner::OWNED_BY_US);
        NotifySurfaceToRenderOutputBuffer(outputBufferPool_[i]);
    }
    // the consumer of old surface may be destroyed, so flushbuffer will fail, and they are owned by us
    for (size_t i : ownedByUs) {
        if (mode == RESUBMIT_BUFFER) {
            NotifyOmxToFillThisOutBuffer(outputBufferPool_[i]);
        }
    }

    oldSurface.Release(true); // make sure old surface is empty and go black
    SetTransform();
    SetScaleMode();
    HLOGI("set surface(%" PRIu64 ")(%s) succ", newId, newSurface->GetName().c_str());
}

#ifdef USE_VIDEO_PROCESSING_ENGINE
int32_t HDecoder::VrrPrediction(BufferInfo &info)
{
    SCOPED_TRACE();
    if (vrrDynamicSwitch_ == false) {
        info.surfaceBuffer->GetExtraData()->ExtraSet("VIDEO_RATE", codecRate_);
        HLOGD("VRR flush video rate %{public}d", static_cast<int32_t>(codecRate_));
        return AVCS_ERR_OK;
    }
    if (VrrProcessFunc_ == nullptr) {
        HLOGE("VrrProcessFunc_ is nullptr");
        return AVCS_ERR_INVALID_OPERATION;
    }
    int vrrMvType = Media::VideoProcessingEngine::MOTIONVECTOR_TYPE_NONE;
    if (static_cast<int>(codingType_) == CODEC_OMX_VIDEO_CodingHEVC) {
        vrrMvType = Media::VideoProcessingEngine::MOTIONVECTOR_TYPE_HEVC;
    } else if (static_cast<int>(codingType_) == OMX_VIDEO_CodingAVC) {
        vrrMvType = Media::VideoProcessingEngine::MOTIONVECTOR_TYPE_AVC;
    } else {
        HLOGE("VRR only support for HEVC or AVC");
        return AVCS_ERR_UNSUPPORT;
    }
    VrrProcessFunc_(vrrHandle_, info.surfaceBuffer->SurfaceBufferToNativeBuffer(),
        static_cast<int32_t>(codecRate_), vrrMvType);
    return AVCS_ERR_OK;
}
#endif
} // namespace OHOS::MediaAVCodec