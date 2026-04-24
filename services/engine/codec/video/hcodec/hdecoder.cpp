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
#include <numeric>
#include <limits>
#include <sstream>
#include <cassert>
#include <sstream>
#include "syspara/parameters.h" // base/startup/init/interfaces/innerkits/include/
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
#include "hcodec_utils.h"
#ifdef USE_VIDEO_PROCESSING_ENGINE
#include "algorithm_video.h"
#endif

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace CodecHDI;

std::shared_mutex g_xperfMtx;
sptr<HDecoder::XperfConnector> g_xperfConnector = nullptr;
std::unordered_map<HDecoder*, HDecoder::DecoderInst> g_insts;

#ifdef USE_VIDEO_PROCESSING_ENGINE
std::mutex g_vpeLock;
std::vector<std::string> g_vpeSupportList{};
bool g_isVpeSupportListInit{false};
#endif

HDecoder::HDecoder(CodecHDI::CodecCompCapability caps, OMX_VIDEO_CODINGTYPE codingType)
    : HCodec(caps, codingType, false)
{
    isReleaseWithSeq_ = OHOS::system::GetBoolParameter("hcodec.surfacemode.release_with_seq", true);
}

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

bool HDecoder::DecideOutputBufMode()
{
    isAlloc_ = false;
    isDynamic_ = false;
 
    SupportBufferTypeV2 typeV2;
    InitOMXParamExt(typeV2);
    typeV2.portIndex = OMX_DirOutput;
    if (GetParameter(OMX_IndexParamSupportBufferTypeV2, typeV2)) {
        if (typeV2.supportedAllocBufTypes & CODEC_BUFFER_TYPE_HANDLE) {
            isAlloc_ = true;
            isDynamic_ = false;
        } else if (typeV2.supportedUseBufTypes & CODEC_BUFFER_TYPE_DYNAMIC_HANDLE) {
            isAlloc_ = false;
            isDynamic_ = true;
        }
    } else {
        SupportBufferType type;
        InitOMXParamExt(type);
        type.portIndex = OMX_DirOutput;
        if (GetParameter(OMX_IndexParamSupportBufferType, type)) {
            if (type.bufferTypes & CODEC_BUFFER_TYPE_DYNAMIC_HANDLE) {
                isAlloc_ = false;
                isDynamic_ = true;
            }
        }
    }
    HLOGI("%s %s", isAlloc_ ? "alloc" : "use", isDynamic_ ? "dynamic" : "preset");
    if (isAlloc_) {
        AllocBufferType param;
        InitOMXParamExt(param);
        param.portIndex = OMX_DirOutput;
        param.bufferType = CODEC_BUFFER_TYPE_HANDLE;
        return SetParameter(OMX_IndexParamAllocBufferType, param);
    }
    return UseHandleOnOutputPort(isDynamic_);
}

int32_t HDecoder::OnConfigure(const Format &format)
{
    configFormat_ = make_shared<Format>(format);

   (void)SetProcessName();
   (void)SetVrrEnable(format);
 
    if (!DecideOutputBufMode()) {
        HLOGE("set output buffer type failed");
        return AVCS_ERR_UNKNOWN;
    }
    int32_t ret = SetLowLatency(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    SetColorAspects(format);
    SetMasteringDisplayColourVolumeFromContainer(format);
    SetContentLightLevelFromContainer(format);

    SaveTransform(format);
    (void)SetProcessName();
    (void)SetFrameRateAdaptiveMode(format);
    (void)SetIsMvUpload(format);
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
    if (def.nBufferCountActual == 0 || def.nBufferCountActual > MAX_BUFFER_COUNT) {
        HLOGE("output buffer count %u is invalid", def.nBufferCountActual);
        return AVCS_ERR_UNKNOWN;
    }
    (void)UpdateConfiguredFmt(def.format.video.eColorFormat);

    uint32_t w = def.format.video.nFrameWidth;
    uint32_t h = def.format.video.nFrameHeight;
    bool isNeedUseDecResolution = static_cast<bool>(reinterpret_cast<uintptr_t>(def.format.video.pNativeRender));

    // save into member variable
    OHOS::Rect damage{};
    GetCropFromOmx(w, h, damage);
    outBufferCnt_ = def.nBufferCountActual;
    requestCfg_.timeout = 0; // never wait when request
    requestCfg_.width = isNeedUseDecResolution ?  static_cast<int32_t>(def.format.video.nFrameWidth) : damage.w;
    requestCfg_.height = isNeedUseDecResolution ? static_cast<int32_t>(def.format.video.nFrameHeight) : damage.h;
    requestCfg_.strideAlignment = STRIDE_ALIGNMENT;
    requestCfg_.format = configuredFmt_.graphicFmt;

    // save into format
    if (outputFormat_ == nullptr) {
        outputFormat_ = make_shared<Format>();
    }
    if (currGeneration_ != 0) {
        return AVCS_ERR_OK;
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
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT, configuredFmt_.graphicFmt);
    outputFormat_->PutIntValue("IS_VENDOR", 1);
    HLOGI("needUseDecReso %d, Request Reso %dx%d, output format: %s",
          isNeedUseDecResolution, requestCfg_.width, requestCfg_.height, outputFormat_->Stringify().c_str());
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
    currSurface_ = SurfaceItem(surface, compUniqueStr_, instanceId_);
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
        codecRate_ = frameRate.value();
    }
    optional<double> operatingRate = GetOperatingRateFromUser(format);
    if (isLpp_) {
        operatingRate = nullopt;
    }
    if (operatingRate.has_value() || frameRate.has_value()) {
        double maxFramerate = std::max(operatingRate.value_or(0.0), frameRate.value_or(0.0));
        OMX_PARAM_U32TYPE maxFramerateCfgType;
        InitOMXParam(maxFramerateCfgType);
        maxFramerateCfgType.nPortIndex = OMX_DirInput;
        maxFramerateCfgType.nU32 = static_cast<uint32_t>(maxFramerate * FRAME_RATE_COEFFICIENT);
        if (!SetParameter(OMX_IndexCodecExtConfigOperatingRate, maxFramerateCfgType, true)) {
            HLOGW("failed to set maxFramerate %.f", maxFramerate);
        }
    }
    (void)SetVrrEnable(format);
    (void)SetIsMvUpload(format);
    (void)SetLppTargetPts(format);
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

void HDecoder::SetIsMvUpload(const Format &format)
{
    int32_t isMvUpload = 0;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, isMvUpload) || isMvUpload != 1) {
        HLOGD("isMvUpload disabled");
        return;
    }

    OMX_CONFIG_BOOLEANTYPE param {};
    InitOMXParam(param);
    param.bEnabled = OMX_TRUE;
    if (!SetParameter(OMX_IndexParamIsMvUpload, param)) {
        HLOGE("SetIsMvUpload SetParameter failed");
        return;
    }
    HLOGI("SetIsMvUpload SetParameter success");
}

// LCOV_EXCL_START
int32_t HDecoder::SetVrrEnable(const Format &format)
{
    int32_t vrrEnable = 0;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_DECODER_OUTPUT_ENABLE_VRR, vrrEnable) || vrrEnable != 1) {
#ifdef USE_VIDEO_PROCESSING_ENGINE
        vrrDynamicSwitch_ = false;
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
        HLOGI("dlclose libvideoprocessingengine.z.so success");
        return AVCS_ERR_UNSUPPORT;
    }
    vrrHandle_ = VrrCreateFunc_();
    int32_t ret = VrrCheckSupportFunc_(vrrHandle_, caller_.app.processName.c_str());
    if (ret != AVCS_ERR_OK) {
        HLOGE("VRR check ltpo support failed");
        VrrDestroyFunc_(vrrHandle_);
        dlclose(vpeHandle_);
        vpeHandle_ = nullptr;
        HLOGI("dlclose libvideoprocessingengine.z.so success");
        if (ret == Media::VideoProcessingEngine::VPE_ALGO_ERR_INVALID_OPERATION) {
            return AVCS_ERR_INVALID_OPERATION;
        }
        return AVCS_ERR_UNSUPPORT;
    }
    return AVCS_ERR_OK;
}
#endif

int32_t HDecoder::SetLppTargetPts(const Format &format)
{
    int64_t targetPts = 0;
    if (!format.GetLongValue("video_seek_pts", targetPts)) {
        return AVCS_ERR_OK;
    }
    HLOGI("SetLppTargetPts targePts = %lld", targetPts);
    LppTargetPtsParam param;
    InitOMXParamExt(param);
    param.targetPts = targetPts;
    if (!SetParameter(OMX_IndexParamLppTargetPts, param)) {
        HLOGI("SetLppTargetPts failed");
        return AVCS_ERR_INVALID_OPERATION;
    }
    return AVCS_ERR_OK;
}
// LCOV_EXCL_STOP

int32_t HDecoder::SubmitOutBufToOmx()
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
        std::min<uint32_t>(def.nBufferCountMin, record_[OMX_DirInput].frameCntTotal_ + 1);
    HLOGI("submit buffer count[%u], inTotalCnt_[%u]", outputBufferCnt, record_[OMX_DirInput].frameCntTotal_);
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
        cfgedConsumerUsage_ = currSurface_.surface_->GetDefaultUsage();
        SetTransform();
        SetScaleMode();
    } else {
        HLOGI("buffer mode");
    }
    uint64_t mode = GetModeUsage();
    uint64_t consumer = GetSurfaceConsumerUsage();
    if (isAlloc_) {
        uint64_t usageToVendor = mode | cfgedConsumerUsage_ | consumer;
        HLOGI("usage to vendor(0x%" PRIx64 ") = mode(0x%" PRIx64 ") | "
            "cfged(0x%" PRIx64 ") | consumer(0x%" PRIx64 ")",
            usageToVendor, mode, cfgedConsumerUsage_, consumer);
        (void)SetUsageToVendor(usageToVendor);
    }
    return true;
}

int32_t HDecoder::AllocateBuffersOnPort(OMX_DIRTYPE portIndex)
{
    if (portIndex == OMX_DirInput) {
        return AllocateAvLinearBuffers(portIndex);
    }
    currGeneration_++;
    currGenerationLayoutHasParsed_ = false;
    outputBufferPool_.clear();
    OMX_PARAM_PORTDEFINITIONTYPE def;
    int32_t ret = GetPortDefinition(portIndex, def);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    outBufferCnt_ = def.nBufferCountActual;
    if (currSurface_.surface_) {
        ret = ClearSurfaceAndSetQueueSize(currSurface_.surface_, outBufferCnt_);
        if (ret != AVCS_ERR_OK) {
            return ret;
        }
    }
    uint64_t mode = GetModeUsage();
    uint64_t consumer = GetSurfaceConsumerUsage();
 
    if (isAlloc_) { // alloc by omx
        uint64_t usageToVendor = mode | cfgedConsumerUsage_ | consumer;
        HLOGI("usage to vendor(0x%" PRIx64 ") = mode(0x%" PRIx64 ") | "
            "cfged(0x%" PRIx64 ") | consumer(0x%" PRIx64 ")",
            usageToVendor, mode, cfgedConsumerUsage_, consumer);
        if (!SetUsageToVendor(usageToVendor)) {
            return AVCS_ERR_INVALID_VAL;
        }
        ret = AllocateOutputBuffersFromOmx();
    } else { // alloc by us and use to omx
        uint64_t vendor = GetVendorProducerUsage();
        requestCfg_.usage = vendor | mode | cfgedConsumerUsage_ | consumer;
        HLOGI("final usage(0x%" PRIx64 ") = vendor(0x%" PRIx64 ") | mode(0x%" PRIx64 ") | "
            "cfged(0x%" PRIx64 ") | consumer(0x%" PRIx64 ")",
            requestCfg_.usage, vendor, mode, cfgedConsumerUsage_, consumer);
 
        if (isDynamic_) {  // use dynamic
            ret = AllocOutDynamicSurfaceBuf();
        } else {  // use preset
            ret = (currSurface_.surface_ ? AllocateOutputBuffersFromSurface() : AllocateAvSurfaceBuffers(portIndex));
        }
    }
    return ret;
}

void HDecoder::ProcSurfaceBufferToUser(const sptr<SurfaceBuffer>& buffer)
{
    if (buffer == nullptr) {
        return;
    }
    SurfaceBufferLayout layout{};
    if (!currGenerationLayoutHasParsed_) {
        std::optional<PixelFmt> fmt = TypeConverter::GraphicFmtToFmt(
            static_cast<GraphicPixelFormat>(buffer->GetFormat()));
        if (!fmt.has_value()) {
            return;
        }
        layout.fmt = fmt.value();
        layout.handleW = buffer->GetWidth();
        layout.handleH = buffer->GetHeight();
        layout.byteStride = buffer->GetStride();
        OH_NativeBuffer_Planes *planes = nullptr;
        GSError err = buffer->GetPlanesInfo(reinterpret_cast<void**>(&planes));
        if (err != GSERROR_OK || planes == nullptr) {
            HLOGI("can not get plane info, ignore");
            layout.alignedHeight = layout.handleH;
        } else {
            for (uint32_t i = 0; i < planes->planeCount; i++) {
                HLOGI("plane[%u]: offset=%" PRIu64 ", rowStride=%u, columnStride=%u",
                    i, planes->planes[i].offset, planes->planes[i].rowStride, planes->planes[i].columnStride);
            }
            layout.alignedHeight = static_cast<int32_t>(static_cast<int64_t>(planes->planes[1].offset) / layout.byteStride);
        }
        currGenerationLayoutHasParsed_ = true;
    } else {
        layout.fmt = currLayout_->fmt;
        layout.handleW = currLayout_->handleW;
        layout.handleH = currLayout_->handleH;
        layout.byteStride = currLayout_->byteStride;
        layout.alignedHeight = currLayout_->alignedHeight;
    }
    using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
    vector<uint8_t> vec;
    GSError err = buffer->GetMetadata(ATTRKEY_CROP_REGION, vec);
    if (err == GSERROR_OK && vec.size() == sizeof(BufferHandleMetaRegion)) {
        auto* crop = reinterpret_cast<BufferHandleMetaRegion*>(vec.data());
        layout.cropLeft = static_cast<int32_t>(crop->left);
        layout.cropTop = static_cast<int32_t>(crop->top);
        layout.cropW = static_cast<int32_t>(crop->width);
        layout.cropH = static_cast<int32_t>(crop->height);
    } else {
        layout.cropLeft = 0;
        layout.cropTop = 0;
        layout.cropW = layout.handleW;
        layout.cropH = layout.handleH;
    }
    if (currLayout_ == nullptr) {
        currLayout_ = std::make_shared<SurfaceBufferLayout>(layout);
        HLOGI("buffer layout update: %s", layout.ToString().c_str());
    } else {
        if (*currLayout_ == layout) {
            return;
        }
        HLOGI("buffer layout update: %s -> %s", currLayout_->ToString().c_str(), layout.ToString().c_str());
        *currLayout_ = layout;
    }
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(layout.fmt.innerFmt));
    outputFormat_->PutStringValue("pixel_format_string", layout.fmt.strFmt);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, layout.byteStride);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, layout.byteStride);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_SLICE_HEIGHT, layout.alignedHeight);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, layout.alignedHeight);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_DISPLAY_WIDTH, layout.cropW);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_DISPLAY_HEIGHT, layout.cropH);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, layout.cropW);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, layout.cropH);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_LEFT, layout.cropLeft);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_TOP, layout.cropTop);

    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_RIGHT,
        static_cast<int32_t>(layout.cropLeft + layout.cropW) - 1);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_CROP_BOTTOM,
        static_cast<int32_t>(layout.cropTop + layout.cropH) - 1);
    HLOGI("output format changed: %s", outputFormat_->Stringify().c_str());
    callback_->OnOutputFormatChanged(*(outputFormat_.get()));
}

void HDecoder::ProcAVBufferToUser(shared_ptr<AVBuffer> avBuffer, shared_ptr<CodecHDI::OmxCodecBuffer> omxBuffer)
{
    if (avBuffer == nullptr || avBuffer->meta_ == nullptr || omxBuffer == nullptr) {
        return;
    }
    shared_ptr<Media::Meta> meta = avBuffer->meta_;
    meta->Clear();
    BinaryReader reader(static_cast<uint8_t*>(omxBuffer->alongParam.data()), omxBuffer->alongParam.size());
    uint32_t* index = nullptr;
    while ((index = reader.Read<uint32_t>()) != nullptr) {
        switch (*index) {
            case static_cast<uint32_t>(OMX_IndexInputStreamError): {
                auto *inputStreamError = reader.Read<int32_t>();
                IF_TRUE_RETURN_VOID(inputStreamError == nullptr);
                meta->SetData(OHOS::Media::Tag::VIDEO_DECODER_INPUT_STREAM_ERROR, *inputStreamError);
                HLOGI("inputStreamError: %d, pts: %" PRId64" ", *inputStreamError, omxBuffer->pts);
                std::stringstream sysEventMsg;
                sysEventMsg << "[" << caller_.app.processName << "]" << compUniqueStr_;
                sysEventMsg << "PTS: " << omxBuffer->pts;
                FaultEventWrite("INPUT_STREAM_ERROR", sysEventMsg.str());
                break;
            }
            default:
                break;
        }
    }
}

void HDecoder::BeforeCbOutToUser(BufferInfo &info)
{
    ProcSurfaceBufferToUser(info.surfaceBuffer);
    ProcAVBufferToUser(info.avBuffer, info.omxBuffer);
}

int32_t HDecoder::SubmitAllBuffersOwnedByUs()
{
    HLOGD(">>");
    if (isBufferCirculating_) {
        HLOGI("buffer is already circulating, no need to do again");
        return AVCS_ERR_OK;
    }
    int32_t ret = SubmitOutBufToOmx();
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
    ReduceOwner(portIndex, info.owner);
    pool.erase(pool.begin() + i);
}

void HDecoder::OnClearBufferPool(OMX_DIRTYPE portIndex)
{
    if ((portIndex == OMX_DirOutput) && currSurface_.surface_) {
        SurfaceTools::GetInstance().CleanCache(instanceId_, currSurface_.surface_, false);
        freeList_.clear();
    }
}

uint64_t HDecoder::GetModeUsage()
{
    return currSurface_.surface_ ? SURFACE_MODE_PRODUCER_USAGE : BUFFER_MODE_REQUEST_USAGE;
    
}
 uint64_t HDecoder::GetVendorProducerUsage()
{
    GetBufferHandleUsageParams vendorUsage;
    InitOMXParamExt(vendorUsage);
    vendorUsage.portIndex = static_cast<uint32_t>(OMX_DirOutput);
    if (GetParameter(OMX_IndexParamGetBufferHandleUsage, vendorUsage)) {
        HLOGI("vendor producer usage = 0x%" PRIx64 "", vendorUsage.usage);
        return vendorUsage.usage;
    }
    return 0;
}

bool HDecoder::SetUsageToVendor(uint64_t usage)
{
    GetBufferHandleUsageParams param;
    InitOMXParamExt(param);
    param.portIndex = OMX_DirOutput;
    param.usage = usage;
    if (!SetParameter(OMX_IndexParamConsumerUsage, param)) {
        HLOGE("set usage to vendor failed");
        return false;
    }
    return true;
}

#ifdef USE_VIDEO_PROCESSING_ENGINE
void HDecoder::CombineConsumerUsageByVpe(uint64_t& consumerUsage)
{
    SCOPED_TRACE();
    std::lock_guard<std::mutex> lock(g_vpeLock);
    if (caller_.app.processName.empty()) {
        return;
    }
    HLOGD("VPE CombineUsage app=%s", caller_.app.processName.c_str());
    if (!g_isVpeSupportListInit) {
        auto vpeHandle = dlopen("libvideoprocessingengine.z.so", RTLD_NOW);
        if (vpeHandle == nullptr) {
            HLOGE("dlopen libvideoprocessingengine.z.so failed, dlerror: %s", dlerror());
            return;
        }
        VpeVideoGetSupportedListByType getSupportedListByTypeFunc =
            reinterpret_cast<VpeVideoGetSupportedListByType>(dlsym(vpeHandle, "VpeVideoGetSupportedListByTypeOnce"));
        if (getSupportedListByTypeFunc == nullptr) {
            HLOGE("dlsym failed");
            dlclose(vpeHandle);
            return;
        }
        if (!getSupportedListByTypeFunc(Media::VideoProcessingEngine::VIDEO_TYPE_AIHDR_ENHANCER, g_vpeSupportList)) {
            HLOGE("get VPE list failed");
            dlclose(vpeHandle);
            return;
        }
        g_isVpeSupportListInit = true;
        HLOGD("VPE init support list ok");
        dlclose(vpeHandle);
    }
    if (std::find(g_vpeSupportList.cbegin(), g_vpeSupportList.cend(), caller_.app.processName) !=
        g_vpeSupportList.cend()) {
        consumerUsage |= (BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_VENDOR_PRI10);
        HLOGI("VPE Aihdr update usage = 0x%" PRIx64 "", consumerUsage);
    }
}
#endif

uint64_t HDecoder::GetSurfaceConsumerUsage()
{
    if (currSurface_.surface_ == nullptr) {
        return 0;
    }
    uint64_t consumerUsage = currSurface_.surface_->GetDefaultUsage();
    if (currSurface_.surface_->GetName().find("SurfaceImage") != string::npos) {
        consumerUsage |= BUFFER_USAGE_HW_COMPOSER;
#ifdef USE_VIDEO_PROCESSING_ENGINE
        CombineConsumerUsageByVpe(consumerUsage);
#endif
    }
    return consumerUsage;
}

int32_t HDecoder::ClearSurfaceAndSetQueueSize(const sptr<Surface> &surface, uint32_t targetSize)
{
    surface->Connect(); // cleancache will work only if the surface is connected by us
    SurfaceTools::GetInstance().CleanCache(instanceId_, surface, false);
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
    for (uint32_t i = 0; i < outBufferCnt_; ++i) {
        shared_ptr<OmxCodecBuffer> omxBuffer = DynamicSurfaceBufferToOmxBuffer();
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t ret = compNode_->UseBuffer(OMX_DirOutput, *omxBuffer, *outBuffer);
        if (ret != HDF_SUCCESS) {
            HLOGE("Failed to UseBuffer on input port");
            return AVCS_ERR_UNKNOWN;
        }
        BufferInfo info(false, BufferOwner::OWNED_BY_US, record_);
        info.surfaceBuffer = nullptr;
        info.avBuffer = nullptr;
        info.omxBuffer = outBuffer;
        info.bufferId = outBuffer->bufferId;
        outputBufferPool_.push_back(info);
    }
    HLOGI("succ");
    return AVCS_ERR_OK;
}

// LCOV_EXCL_START
int32_t HDecoder::AllocateOutputBuffersFromSurface()
{
    SCOPED_TRACE();
    std::map<uint32_t, sptr<SurfaceBuffer>> bufferMap;
    for (uint32_t i = 0; i < outBufferCnt_; ++i) {
        sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
        IF_TRUE_RETURN_VAL(surfaceBuffer == nullptr, AVCS_ERR_UNKNOWN);

        GSError err = surfaceBuffer->Alloc(requestCfg_);
        if (err != GSERROR_OK) {
            HLOGE("Alloc surfacebuffer %u failed, GSError=%d", i, err);
            return err == GSERROR_NO_MEM ? AVCS_ERR_NO_MEMORY : AVCS_ERR_UNKNOWN;
        }
        shared_ptr<OmxCodecBuffer> omxBuffer = SurfaceBufferToOmxBuffer(surfaceBuffer);
        IF_TRUE_RETURN_VAL(omxBuffer == nullptr, AVCS_ERR_UNKNOWN);
        if (isLpp_) {
            err = currSurface_.surface_->AttachBufferToQueue(surfaceBuffer);
            IF_TRUE_RETURN_VAL_WITH_MSG(err != GSERROR_OK, AVCS_ERR_UNKNOWN,
                                        "AttachBufferToQueue %u failed, GSError=%d", i, err);
        }
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t hdfRet = compNode_->UseBuffer(OMX_DirOutput, *omxBuffer, *outBuffer);
        IF_TRUE_RETURN_VAL_WITH_MSG(hdfRet != HDF_SUCCESS, AVCS_ERR_NO_MEMORY, "Failed to UseBuffer with output port");

        SetCallerToBuffer(surfaceBuffer->GetFileDescriptor(),
                          static_cast<uint32_t>(surfaceBuffer->GetWidth()),
                          static_cast<uint32_t>(surfaceBuffer->GetHeight()));
        outBuffer->fenceFd = -1;
        BufferInfo info(false, BufferOwner::OWNED_BY_US, record_);
        info.surfaceBuffer = surfaceBuffer;
        info.avBuffer = AVBuffer::CreateAVBuffer(surfaceBuffer);
        info.omxBuffer = outBuffer;
        info.bufferId = outBuffer->bufferId;
        info.attached = isLpp_ ? true : false;
        outputBufferPool_.push_back(info);
        HLOGI("generation=%d, bufferId=%u, seq=%u", currGeneration_, info.bufferId, surfaceBuffer->GetSeqNum());
        bufferMap.emplace(surfaceBuffer->GetSeqNum(), surfaceBuffer);
    }
    if (callback_ != nullptr && isLpp_) {
        callback_->OnOutputBufferBinded(bufferMap);
    }
    return AVCS_ERR_OK;
}

int32_t HDecoder::AllocateOutputBuffersFromOmx()
{
    for (uint32_t i = 0; i < outBufferCnt_; ++i) {
        std::shared_ptr<OmxCodecBuffer> omxBuffer = std::make_shared<OmxCodecBuffer>();
        omxBuffer->size = sizeof(OmxCodecBuffer);
        omxBuffer->version.version.majorVersion = 1;
        omxBuffer->bufferType = CODEC_BUFFER_TYPE_HANDLE;
        omxBuffer->fd = -1;
        omxBuffer->allocLen = 0;
        omxBuffer->fenceFd = -1;
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t ret = compNode_->AllocateBuffer(OMX_DirOutput, *omxBuffer, *outBuffer);
        if (ret != HDF_SUCCESS || outBuffer->bufferhandle == nullptr) {
            HLOGE("Failed to AllocateBuffer on output port");
            return AVCS_ERR_INVALID_VAL;
        }
        BufferHandle* handle = outBuffer->bufferhandle->Move();
        if (handle == nullptr) {
            HLOGE("null BufferHandle");
            return AVCS_ERR_INVALID_VAL;
        }
        sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
        surfaceBuffer->SetBufferHandle(handle);
        requestCfg_.width = surfaceBuffer->GetWidth();
        requestCfg_.height = surfaceBuffer->GetHeight();
        requestCfg_.format = surfaceBuffer->GetFormat();
        requestCfg_.usage = surfaceBuffer->GetUsage();
        surfaceBuffer->SetBufferRequestConfig(requestCfg_);
        if (currSurface_.surface_) {
            GSError err = currSurface_.surface_->AttachBufferToQueue(surfaceBuffer);
            IF_TRUE_RETURN_VAL_WITH_MSG(err != GSERROR_OK, AVCS_ERR_UNKNOWN,
                                        "AttachBufferToQueue %u failed, GSError=%d", i, err);
        }
        SetCallerToBuffer(surfaceBuffer->GetFileDescriptor(),
                          static_cast<uint32_t>(surfaceBuffer->GetWidth()),
                          static_cast<uint32_t>(surfaceBuffer->GetHeight()));
        outBuffer->bufferhandle = nullptr;
        outBuffer->fd = -1;
        outBuffer->fenceFd = -1;
        BufferInfo info(false, BufferOwner::OWNED_BY_US, record_);
        info.surfaceBuffer = surfaceBuffer;
        info.avBuffer = (currSurface_.surface_ ? AVBuffer::CreateAVBuffer() : AVBuffer::CreateAVBuffer(surfaceBuffer));
        info.omxBuffer = outBuffer;
        info.bufferId = outBuffer->bufferId;
        info.attached = true;
        outputBufferPool_.push_back(info);
        HLOGI("generation=%d, bufferId=%u, seq=%u", currGeneration_, info.bufferId, surfaceBuffer->GetSeqNum());
    }
    return AVCS_ERR_OK;
}
// LCOV_EXCL_STOP

int32_t HDecoder::RegisterListenerToSurface(const sptr<Surface> &surface)
{
    uint64_t surfaceId = surface->GetUniqueId();
    std::weak_ptr<MsgToken> weakThis = m_token;
    OHSurfaceSource sourceType = isLpp_ ? OH_SURFACE_SOURCE_LOWPOWERVIDEO : OH_SURFACE_SOURCE_VIDEO;
    bool ret = false;
    if (isReleaseWithSeq_) {
        HLOGI("surface(%" PRIu64 "), register OnReleaseFuncWithSequenceAndFence", surfaceId);
        ret = SurfaceTools::GetInstance().RegisterReleaseListener(instanceId_, surface,
        [weakThis, surfaceId](uint32_t seq, const sptr<SyncFence>& fence) {
            std::shared_ptr<MsgToken> codec = weakThis.lock();
            if (codec == nullptr) {
                LOGD("decoder is gone");
                return GSERROR_OK;
            }
            ParamSP param = make_shared<ParamBundle>();
            param->SetValue("surfaceId", surfaceId);
            param->SetValue("seqnum", seq);
            param->SetValue("fence", fence);
            codec->SendAsyncMsg(MsgWhat::GET_BUFFER_FROM_SURFACE, param);
            return GSERROR_OK;
        }, sourceType);
    } else {
        HLOGI("surface(%" PRIu64 "), register OnReleaseFunc", surfaceId);
        ret = SurfaceTools::GetInstance().RegisterReleaseListener(instanceId_, surface,
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
        }, sourceType);
    }
    if (!ret) {
        HLOGE("surface(%" PRIu64 "), RegisterReleaseListener failed", surfaceId);
        return AVCS_ERR_UNKNOWN;
    }
    std::unique_lock<std::shared_mutex> lk(g_xperfMtx);
    if (g_xperfConnector != nullptr) {
        auto iter = g_insts.find(this);
        if (iter != g_insts.end()) {
            iter->second.surfaceId = surfaceId;
        }
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
        return {};
    }
    item.seqnum = item.buffer->GetSeqNum();
    return item;
}

std::vector<HCodec::BufferInfo>::iterator HDecoder::FindBelongTo(uint32_t seqnum)
{
    return std::find_if(outputBufferPool_.begin(), outputBufferPool_.end(), [seqnum](const BufferInfo& info) {
        return (info.owner == BufferOwner::OWNED_BY_SURFACE) &&
               info.surfaceBuffer && (info.surfaceBuffer->GetSeqNum() == seqnum);
    });
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

    SurfaceBufferItem item{};
    if (isReleaseWithSeq_) {
        param->GetValue("seqnum", item.seqnum);
        param->GetValue("fence", item.fence);
    } else {
        item = RequestBuffer();
        if (item.buffer == nullptr) {
            return;
        }
    }

    HitraceMeterFmtScoped tracePoint(HITRACE_TAG_ZMEDIA, "requested: %u", item.seqnum);
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
    SCOPED_TRACE_FMT("seq: %u", item.seqnum);
    auto iter = outputBufferPool_.end();
    if (isReleaseWithSeq_) {
        iter = FindBelongTo(item.seqnum);
        if (iter == outputBufferPool_.end()) {
            HLOGI("seq=%u dont belong to output set, ignore", item.seqnum);
            return false;
        }
    } else {
        if (item.generation != currGeneration_) {
            HLOGD("buffer generation %d != current generation %d, ignore", item.generation, currGeneration_);
            return false;
        }
        iter = FindBelongTo(item.buffer);
        if (iter == outputBufferPool_.end()) {
            auto nullSlot = FindNullSlotIfDynamicMode();
            if (nullSlot != outputBufferPool_.end()) {
                HLOGI("seq=%u dont belong to output set, bind as dynamic", item.seqnum);
                WaitFence(item.fence);
                DynamicModeSubmitBufferToSlot(item.buffer, nullSlot);
                nullSlot->attached = true;
                return true;
            }
            HLOGI("seq=%u dont belong to output set, ignore", item.seqnum);
            return false;
        }
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
    HLOGI("generation=%d, bufferId=%u, seq=%u", currGeneration_, nullSlot->bufferId, buffer->GetSeqNum());
    std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(buffer);
    IF_TRUE_RETURN_VOID_WITH_MSG(avBuffer == nullptr || avBuffer->memory_ == nullptr, "CreateAVBuffer failed");
    nullSlot->avBuffer = avBuffer;
    if (currSurface_.surface_ == nullptr) {
        nullSlot->needDealWithCache = (requestCfg_.usage & BUFFER_USAGE_MEM_MMZ_CACHE);
    }
    SetCallerToBuffer(buffer->GetFileDescriptor(),
                      static_cast<uint32_t>(buffer->GetWidth()),
                      static_cast<uint32_t>(buffer->GetHeight()));
    WrapSurfaceBufferToSlot(*nullSlot, buffer, 0, 0);
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
    if (!isVrrInitialized_) {
        sptr<BufferExtraData> extraData = new BufferExtraDataImpl();
        extraData->ExtraSet("VIDEO_RATE", codecRate_);
        info.surfaceBuffer->SetExtraData(extraData);
    }

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

    PerfMeasure();

    ChangeOwner(info, BufferOwner::OWNED_BY_SURFACE);
    return AVCS_ERR_OK;
}
 
void HDecoder::OnStartJankDetect()
{
    if (detect_) {
        return;
    }
    detect_ = true;
    currSurface_.beginTime_ = GetCurrUs();
}
 
void HDecoder::OnStopJankDetect()
{
    if (!detect_) {
        return;
    }
    detect_ = false;
    int64_t endTime = GetCurrUs();
    int64_t duration = endTime - currSurface_.lastFlushTime_;
    if (duration > 1000000) { //1000000: 大于1s
        std::stringstream ss;
        ss << "#UNIQUEID:" << currSurface_.surface_->GetUniqueId() <<
           "#SURFACE_NAME:" << currSurface_.surface_->GetName() <<
           "#LAST_FLUSH_TIME:" << currSurface_.lastFlushTime_ <<
           "#DURATION:" << duration;
 
        callback_->OnXperfEvent(OHOS::HiviewDFX::AvcodecEventCode::AVCODEC_JANK, ss.str()); //上报卡顿事件
    }

    std::stringstream ss;
    ss << "#UNIQUEID:" << currSurface_.surface_->GetUniqueId() <<
      "#PID:" << caller_.app.pid <<
      "#BUNDLE_NAME:" << caller_.app.processName <<
      "#SURFACE_NAME:" << currSurface_.surface_->GetName() <<
      "#BEGIN_TIME:" << currSurface_.beginTime_ <<
      "#END_TIME:" << endTime <<
      "#TIMES:" << currSurface_.jankCnt_ <<
      "#TOTAL_DUR:" << currSurface_.totalJankDur_;
 
    callback_->OnXperfEvent(OHOS::HiviewDFX::AvcodecEventCode::AVCODEC_FRAME_STATS, ss.str());
 
    currSurface_.jankCnt_ = 0;
    currSurface_.totalJankDur_ = 0;
    currSurface_.beginTime_ = 0;
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
            record_[OMX_DirOutput].discardCntInterval_++;
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
    currGenerationLayoutHasParsed_ = false;
    currLayout_.reset();
    cfgedConsumerUsage_ = 0;
    currGeneration_ = 0;
    scaleMode_ = std::nullopt;
    transform_ = GRAPHIC_ROTATE_NONE;
}

HDecoder::SurfaceItem::SurfaceItem(const sptr<Surface> &surface, std::string codecId, int32_t instanceId)
    : surface_(surface), originalTransform_(surface->GetTransform()), compUniqueStr_(codecId),
      instanceId_(instanceId) {}

void HDecoder::SurfaceItem::Release(bool cleanAll)
{
    if (surface_) {
        LOGI("release surface(%" PRIu64 ")", surface_->GetUniqueId());
        if (originalTransform_.has_value()) {
            surface_->SetTransform(originalTransform_.value());
            originalTransform_ = std::nullopt;
        }
        SurfaceTools::GetInstance().ReleaseSurface(instanceId_, surface_, cleanAll);
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
        auto iter = isReleaseWithSeq_ ? FindBelongTo(item.seqnum) : FindBelongTo(item.buffer);
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
    currSurface_ = SurfaceItem(newSurface, compUniqueStr_, instanceId_);
    SetTransform();
    SetScaleMode();
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
    HLOGI("set surface(%" PRIu64 ")(%s) succ", newId, newSurface->GetName().c_str());
}

// LCOV_EXCL_START
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

void HDecoder::PerfMeasure()
{
    int64_t currTime = GetCurrUs();
    currSurface_.flushCnt_++;
    if (currSurface_.flushCnt_ == 1) { //1: 首帧
        std::stringstream ss;
        ss << "#UNIQUEID:" << currSurface_.surface_->GetUniqueId() <<
          "#PID:" << caller_.app.pid <<
          "#BUNDLE_NAME:" << caller_.app.processName <<
          "#SURFACE_NAME:" << currSurface_.surface_->GetName() <<
          "#FPS:" << codecRate_ <<
          "#REPORT_INTERVAL:" << 300;  // 300ms
        callback_->OnXperfEvent(OHOS::HiviewDFX::AvcodecEventCode::AVCODEC_FIRST_FRAME_START, ss.str()); //上报首帧事件
    } else if (currSurface_.flushCnt_ == 2) { //2: 第二帧
        detect_ = true;
        currSurface_.beginTime_ = currTime;

        std::stringstream ss;
        ss << "#UNIQUEID:" << currSurface_.surface_->GetUniqueId() <<
          "#PID:" << caller_.app.pid <<
          "#BUNDLE_NAME:" << caller_.app.processName <<
          "#SURFACE_NAME:" << currSurface_.surface_->GetName() <<
          "#FPS:" << codecRate_ <<
          "#REPORT_INTERVAL:" << 300;  // 300ms
        callback_->OnXperfEvent(OHOS::HiviewDFX::AvcodecEventCode::AVCODEC_SECOND_FRAME, ss.str()); //上报第二帧事件
    } else if (detect_) {
        int64_t span = currTime - currSurface_.lastFlushTime_;
        if (span > 100000) { //100000: 大于100ms
            currSurface_.jankCnt_++;
            currSurface_.totalJankDur_ += span;
        }
        if (span > 300000) { //300000: 大于300ms
            std::stringstream ss;
            ss << "#UNIQUEID:" << currSurface_.surface_->GetUniqueId() <<
               "#SURFACE_NAME:" << currSurface_.surface_->GetName() <<
               "#LAST_FLUSH_TIME:" << currSurface_.lastFlushTime_ <<
               "#DURATION:" << span;
            callback_->OnXperfEvent(OHOS::HiviewDFX::AvcodecEventCode::AVCODEC_JANK, ss.str()); //上报卡顿事件
        }
    }
    currSurface_.lastFlushTime_ = currTime;
}

void HDecoder::OnEnterRunningState()
{
    std::unique_lock<std::shared_mutex> lk(g_xperfMtx);
    if (g_xperfConnector == nullptr) {
        g_xperfConnector = sptr<XperfConnector>::MakeSptr();
        int regret = OHOS::HiviewDFX::XperfServiceClient::GetInstance().RegisterVideoState("avcodec", g_xperfConnector);
        if (regret != 0) {
            g_xperfConnector = nullptr;
        }
    }
    if (g_xperfConnector != nullptr) {
        DecoderInst inst {
            .token = m_token,
            .surfaceId = std::nullopt,
            .processName = caller_.app.processName,
        };
        if (currSurface_.surface_) {
            inst.surfaceId = currSurface_.surface_->GetUniqueId();
        }
        g_insts[this] = inst;
    }
}

void HDecoder::OnExitRunningState()
{
    std::unique_lock<std::shared_mutex> lk(g_xperfMtx);
    if (g_xperfConnector != nullptr) {
        g_insts.erase(this);
    }
}

std::shared_ptr<HDecoder::MsgToken> HDecoder::XperfConnector::FindSuitableDecoder(
    uint64_t surfaceId, const std::string& bundleName)
{
    std::shared_lock<std::shared_mutex> lk(g_xperfMtx);
    if (g_xperfConnector == nullptr) {
        return nullptr;
    }
 
    auto it = std::find_if(g_insts.begin(), g_insts.end(), [surfaceId](const pair<HDecoder*, DecoderInst>& pair) {
        return pair.second.surfaceId == surfaceId;
    });
    if (it != g_insts.end()) {
        return it->second.token.lock();
    }
    LOGI("no decoder is using this surfaceId=%" PRIu64, surfaceId);
 
    it = std::find_if(g_insts.begin(), g_insts.end(), [&bundleName](const pair<HDecoder*, DecoderInst>& pair) {
        return pair.second.processName == bundleName;
    });
    if (it != g_insts.end()) {
        return it->second.token.lock();
    }
    LOGI("no decoder is created by this app=%s", bundleName.c_str());
    return nullptr;
}

ErrCode HDecoder::XperfConnector::OnVideoJankEvent(const std::string& msg)
{
    LOGD("msg=%s", msg.c_str());
    std::string sub = "#UNIQUEID:";
    auto pos = msg.find(sub);
    if (pos == std::string::npos) {
        LOGW("cannot find #UNIQUEID:");
        return 0;
    }
    uint64_t surfaceId = strtoull(msg.substr(pos + sub.length()).c_str(), nullptr, 10);
 
    sub = "#BUNDLE_NAME:";
    pos = msg.find(sub);
    if (pos == std::string::npos) {
        LOGW("cannot find #BUNDLE_NAME:");
        return 0;
    }
    string bundleName = msg.substr(pos + sub.length());
 
    std::shared_ptr<MsgToken> codec = FindSuitableDecoder(surfaceId, bundleName);
    if (codec != nullptr) {
        codec->SendAsyncMsg(MsgWhat::QUERY_JANK_REASON, nullptr);
        return 0;
    }
 
    std::stringstream s;
    s << "#UNIQUEID:" << surfaceId <<
         "#PID:" << 0 <<
         "#BUNDLE_NAME:" << bundleName <<
         "#SURFACE_NAME:" << "" <<
         "#FAULT_ID:" << OHOS::HiviewDFX::DomainId::AVCODEC <<
         "#FAULT_CODE:" << OHOS::HiviewDFX::AvcodecFaultCode::NO_MATCHING_DECODER <<
         "#JANK_REASON:" << "no matching decoder";
    string str = s.str();
    OHOS::HiviewDFX::XperfServiceClient::GetInstance().NotifyToXperf(
        OHOS::HiviewDFX::DomainId::AVCODEC, OHOS::HiviewDFX::AvcodecEventCode::AVCODEC_JANK_REPORT, str);
    return 0;
}

std::shared_ptr<HDecoder::MsgToken> HDecoder::XperfConnector::FindSuitableDecoder(const std::string& msg)
{
    std::string sub = "#UNIQUEID:";
    auto pos = msg.find(sub);
    if (pos == std::string::npos) {
        LOGD("cannot find #UNIQUEID:");
        return nullptr;
    }
    uint64_t surfaceId = strtoull(msg.substr(pos + sub.length()).c_str(), nullptr, 10);
    sub = "#BUNDLE_NAME:";
    pos = msg.find(sub);
    if (pos == std::string::npos) {
        LOGD("cannot find #BUNDLE_NAME:");
        return nullptr;
    }
    string bundleName = msg.substr(pos + sub.length());
    return FindSuitableDecoder(surfaceId, bundleName);
}
 
ErrCode HDecoder::XperfConnector::OnVideoResumed(const std::string& msg)
{
    std::shared_ptr<MsgToken> codec = FindSuitableDecoder(msg);
    if (codec == nullptr) {
        LOGD("cannot find decoder");
        return 0;
    }
    codec->SendAsyncMsg(MsgWhat::XPERF_RESUME_EVENT, nullptr);
    return 0;
}
 
ErrCode HDecoder::XperfConnector::OnVideoPaused(const std::string& msg)
{
    std::shared_ptr<MsgToken> codec = FindSuitableDecoder(msg);
    if (codec == nullptr) {
        LOGD("cannot find decoder");
        return 0;
    }
    codec->SendAsyncMsg(MsgWhat::XPERF_PAUSE_EVENT, nullptr);
    return 0;
}

void HDecoder::OnQueryJankReason()
{
    HLOGI(">>");
    if (currSurface_.surface_ == nullptr) {
        return;
    }
    auto now = chrono::steady_clock::now();
    double maxPercent = 0;
    using namespace OHOS::HiviewDFX;
    AvcodecFaultCode fault = AvcodecFaultCode::AVCODEC_NONE;
    string reason = "unknown";
    GetJankReason(now, OMX_DirInput, maxPercent, fault, reason);
    GetJankReason(now, OMX_DirOutput, maxPercent, fault, reason);
    HLOGI("reason %s", reason.c_str());
    if (fault == AvcodecFaultCode::AVCODEC_NONE) {
        return;
    }
    std::stringstream s;
    s << "#UNIQUEID:" << currSurface_.surface_->GetUniqueId() <<
         "#PID:" << caller_.app.pid <<
         "#BUNDLE_NAME:" << caller_.app.processName <<
         "#SURFACE_NAME:" << currSurface_.surface_->GetName() <<
         "#FAULT_ID:" << OHOS::HiviewDFX::DomainId::AVCODEC <<
         "#FAULT_CODE:" << fault <<
         "#JANK_REASON:" << reason;
    string str = s.str();
    OHOS::HiviewDFX::XperfServiceClient::GetInstance().NotifyToXperf(
        OHOS::HiviewDFX::DomainId::AVCODEC, OHOS::HiviewDFX::AvcodecEventCode::AVCODEC_JANK_REPORT, str);
}
 
void HDecoder::GetJankReason(const TimePoint& now, OMX_DIRTYPE port,
    double& maxPercent, OHOS::HiviewDFX::AvcodecFaultCode& fault, std::string& reason)
{
    bool eos = (port == OMX_DirInput) ? inputPortEos_ : outputPortEos_;
    if (eos) {
        return;
    }
    IntervalAverage ave;
    if (!CalculateInterval(now, port, ave)) {
        return;
    }
    PrintAllBufferInfo(now, port);
    string portStr = (port == OMX_DirInput) ? "input" : "output";
    using namespace OHOS::HiviewDFX;
    static std::array<AvcodecFaultCode, OWNER_CNT> inCode = { HCODEC_HOLD_INPUT_TOO_MORE, USER_HOLD_INPUT_TOO_MORE,
        HAL_HOLD_INPUT_TOO_MORE, AVCODEC_NONE };
    static std::array<AvcodecFaultCode, OWNER_CNT> outCode = { HCODEC_HOLD_OUTPUT_TOO_MORE, USER_HOLD_OUTPUT_TOO_MORE,
        HAL_HOLD_OUTPUT_TOO_MORE, CONSUMER_HOLD_OUTPUT_TOO_MORE };
    const std::array<AvcodecFaultCode, OWNER_CNT>& faultArr = (port == OMX_DirInput) ? inCode : outCode;
    const vector<BufferInfo>& pool = (port == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    std::array<uint64_t, OWNER_CNT> holdTotalTime;
    holdTotalTime.fill(0);
    for (const BufferInfo& info : pool) {
        int64_t holdMs = chrono::duration_cast<chrono::milliseconds>(now - info.lastOwnerChangeTime).count();
        if (holdMs < 0) {
            continue;
        }
        holdTotalTime[info.owner] += static_cast<uint64_t>(holdMs);
    }
    for (uint32_t owner = 0; owner < static_cast<uint32_t>(OWNER_CNT); owner++) {
        const char* ownerStr = ToString(static_cast<BufferOwner>(owner));
        double holdTotalTimeNow = static_cast<double>(holdTotalTime[owner]);
        double holdTotalTimeAve = ave.holdCnt[owner] * ave.holdMs[owner];
        if (holdTotalTimeNow <= holdTotalTimeAve) {
            continue;
        }
        HLOGD("port=%d, owner %s, now %" PRId64 ", average %f",
            port, ownerStr, holdTotalTime[owner], holdTotalTimeAve);
        double increasePercent = (holdTotalTimeNow - holdTotalTimeAve) / holdTotalTimeAve;
        if (increasePercent <= 0.2 || increasePercent <= maxPercent) { // 0.2: threshold
            continue;
        }
        HLOGD("port=%d, owner %s, increasePercent %f", port, ownerStr, increasePercent);
        maxPercent = increasePercent;
        fault = faultArr[owner];
        std::stringstream s;
        s << ownerStr << " hold " << portStr << " too more";
        reason = s.str();
    }
}

void HDecoder::RecordProcessTimeOfUpstream(const std::shared_ptr<AVBuffer>& avBuffer)
{
    size_t mapMaxSize = 50;
    //To prevent memory leaks in extreme cases, the maximum size of the map is limited to 50
    if (ptsToProcessTimesMap_.size() > mapMaxSize) {
        HLOGD("the ptsToProcessTimesMap_ size is over 50. remove the key-value pair corresponding pts: %d",
            ptsToProcessTimesMap_.begin()->first);
        ptsToProcessTimesMap_.erase(ptsToProcessTimesMap_.begin());
    }
    std::vector<int64_t> stallStegeTimeList;
    if (!avBuffer->meta_->GetData(OHOS::Media::Tag::STALLING_TIMESTAMP, stallStegeTimeList)) {
        return;
    }
    ptsToProcessTimesMap_[avBuffer->pts_] = stallStegeTimeList;
    HLOGD("save pts and corresponding stall stage vector from inputBuffer, curr pts is:%d, map size is:%d",
        avBuffer->pts_, ptsToProcessTimesMap_.size());
}

void HDecoder::AppendProcessTimeOfUs(const shared_ptr<AVBuffer>& avBuffer, int64_t pts, const TimePoint& now)
{
    auto it = ptsToProcessTimesMap_.find(pts);
    if (it == ptsToProcessTimesMap_.end()) {
        HLOGD("missing pts and corresponding stall stage vector");
        return;
    }
    auto stallStageTimeList = it->second;
    int64_t stallStage = static_cast<int64_t>(Media::StallingStage::DECODER_END);
    int64_t decodeCompTime = static_cast<int64_t>(
        chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count());
    stallStageTimeList.insert(stallStageTimeList.end(), {stallStage, decodeCompTime});
    avBuffer->meta_->SetData(OHOS::Media::Tag::STALLING_TIMESTAMP, stallStageTimeList);
    ptsToProcessTimesMap_.erase(it);
    HLOGD("append decoding end stage: pts is %ld, stalling stage is %d, end time is %ld",
        pts, stallStage, decodeCompTime);
}

int32_t HDecoder::SetMasteringDisplayColourVolumeFromContainer(const Format &format)
{
    uint8_t* buffer;
    size_t bufferSize;
    if (!format.GetBuffer(OHOS::Media::Tag::VIDEO_STATIC_METADATA_SMPT2086, &buffer, bufferSize)) {
        return AVCS_ERR_OK;
    }
    if (buffer == nullptr) {
        HLOGE("null colourVolume buffer pointer");
        return AVCS_ERR_INVALID_VAL;
    }
    if (bufferSize != sizeof(OmxSmpte2086)) {
        HLOGE("invalid buffer size: bufferSize=%zu (expected %zu)", bufferSize, sizeof(OmxSmpte2086));
        return AVCS_ERR_INVALID_VAL;
    }
    auto *colourVolumeInfo = reinterpret_cast<OmxSmpte2086 *>(buffer);

    HdrColourVolumeFromContainer param;
    InitOMXParamExt(param);
    param.portIndex = OMX_DirOutput;
    param.colourVolume = *colourVolumeInfo;
    if (!SetParameter(OMX_IndexParamMasteringDisplayColourVolume, param, false)) {
        HLOGE("failed to set HdrColourVolumeFromContainer to OMX");
        return AVCS_ERR_UNKNOWN;
    }

    return AVCS_ERR_OK;
}

int32_t HDecoder::SetContentLightLevelFromContainer(const Format &format)
{
    uint8_t* buffer;
    size_t bufferSize;
    if (!format.GetBuffer(OHOS::Media::Tag::VIDEO_STATIC_METADATA_CTA861, &buffer, bufferSize)) {
        return AVCS_ERR_OK;
    }
    if (buffer == nullptr) {
        HLOGE("null contentLightLevel buffer pointer");
        return AVCS_ERR_INVALID_VAL;
    }
    if (bufferSize != sizeof(OmxCta861)) {
        HLOGE("invalid buffer size: bufferSize=%zu (expected %zu)", bufferSize, sizeof(OmxCta861));
        return AVCS_ERR_INVALID_VAL;
    }
    auto *contentLightLevelInfo = reinterpret_cast<OmxCta861 *>(buffer);

    HdrContentLightLevelFromContainer param;
    InitOMXParamExt(param);
    param.portIndex = OMX_DirOutput;
    param.contentLightLevel = *contentLightLevelInfo;
    if (!SetParameter(OMX_IndexParamContentLightLevel, param, false)) {
        HLOGE("failed to set HdrContentLightLevelFromContainer to OMX");
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}
// LCOV_EXCL_STOP
} // namespace OHOS::MediaAVCodec