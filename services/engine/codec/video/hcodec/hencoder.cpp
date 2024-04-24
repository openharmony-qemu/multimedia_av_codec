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

#include "hencoder.h"
#include <map>
#include <utility>
#include "utils/hdf_base.h"
#include "OMX_VideoExt.h"
#include "media_description.h"  // foundation/multimedia/av_codec/interfaces/inner_api/native/
#include "type_converter.h"
#include "hcodec_log.h"
#include "hcodec_dfx.h"
#include "hcodec_utils.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::HDI::Codec::V3_0;

int32_t HEncoder::OnConfigure(const Format &format)
{
    configFormat_ = make_shared<Format>(format);
    int32_t ret = ConfigureBufferType();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    optional<double> frameRate = GetFrameRateFromUser(format);
    ret = SetupPort(format, frameRate);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    switch (static_cast<int>(codingType_)) {
        case OMX_VIDEO_CodingAVC:
            ret = SetupAVCEncoderParameters(format, frameRate);
            break;
        case CODEC_OMX_VIDEO_CodingHEVC:
            ret = SetupHEVCEncoderParameters(format, frameRate);
            break;
        default:
            break;
    }
    if (ret != AVCS_ERR_OK) {
        HLOGW("set protocol param failed");
    }
    ret = ConfigureOutputBitrate(format);
    if (ret != AVCS_ERR_OK) {
        HLOGW("ConfigureOutputBitrate failed");
    }
    ret = SetColorAspects(format);
    if (ret != AVCS_ERR_OK) {
        HLOGW("set color aspect failed");
    }
    (void)SetProcessName(format);
    (void)SetFrameRateAdaptiveMode(format);
    CheckIfEnableCb(format);
    ret = SetLTRParam(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = SetQpRange(format, false);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = SetLowLatency(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    return AVCS_ERR_OK;
}

int32_t HEncoder::ConfigureBufferType()
{
    UseBufferType useBufferTypes;
    InitOMXParamExt(useBufferTypes);
    useBufferTypes.portIndex = OMX_DirInput;
    useBufferTypes.bufferType = CODEC_BUFFER_TYPE_DYNAMIC_HANDLE;
    if (!SetParameter(OMX_IndexParamUseBufferType, useBufferTypes)) {
        HLOGE("component don't support CODEC_BUFFER_TYPE_DYNAMIC_HANDLE");
        return AVCS_ERR_INVALID_VAL;
    }
    return AVCS_ERR_OK;
}

void HEncoder::CheckIfEnableCb(const Format &format)
{
    int32_t enableCb = 0;
    if (format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, enableCb)) {
        HLOGI("enable surface mode callback flag %d", enableCb);
        enableSurfaceModeInputCb_ = static_cast<bool>(enableCb);
    }
}

int32_t HEncoder::SetLTRParam(const Format &format)
{
    int32_t ltrFrameNum = -1;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, ltrFrameNum)) {
        return AVCS_ERR_OK;
    }
    if (ltrFrameNum <= 0) {
        HLOGE("invalid ltrFrameNum %d", ltrFrameNum);
        return AVCS_ERR_INVALID_VAL;
    }
    CodecLTRParam info;
    InitOMXParamExt(info);
    info.ltrFrameListLen = static_cast<uint32_t>(ltrFrameNum);
    if (!SetParameter(OMX_IndexParamLTR, info)) {
        HLOGE("configure LTR failed");
        return AVCS_ERR_INVALID_VAL;
    }
    enableLTR = true;
    return AVCS_ERR_OK;
}

int32_t HEncoder::SetQpRange(const Format &format, bool isCfg)
{
    int32_t minQp;
    int32_t maxQp;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MIN, minQp) ||
        !format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MAX, maxQp)) {
        return AVCS_ERR_OK;
    }

    CodecQPRangeParam QPRangeParam;
    InitOMXParamExt(QPRangeParam);
    QPRangeParam.minQp = static_cast<uint32_t>(minQp);
    QPRangeParam.maxQp = static_cast<uint32_t>(maxQp);
    if (!SetParameter(OMX_IndexParamQPRange, QPRangeParam, isCfg)) {
        HLOGE("set qp range (%d~%d) failed", minQp, maxQp);
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set qp range (%d~%d) succ", minQp, maxQp);
    return AVCS_ERR_OK;
}

int32_t HEncoder::SetColorAspects(const Format &format)
{
    int range = -1;
    ColorPrimary primary = COLOR_PRIMARY_UNSPECIFIED;
    TransferCharacteristic transfer = TRANSFER_CHARACTERISTIC_UNSPECIFIED;
    MatrixCoefficient matrix = MATRIX_COEFFICIENT_UNSPECIFIED;

    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, range)) {
        HLOGI("user set range flag %d", range);
    }
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, *(int *)&primary)) {
        HLOGI("user set primary %d", primary);
    }
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, *(int *)&transfer)) {
        HLOGI("user set transfer %d", transfer);
    }
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, *(int *)&matrix)) {
        HLOGI("user set matrix %d", matrix);
    }
    if (range == -1 && primary == COLOR_PRIMARY_UNSPECIFIED && transfer == TRANSFER_CHARACTERISTIC_UNSPECIFIED &&
        matrix == MATRIX_COEFFICIENT_UNSPECIFIED) {
        return AVCS_ERR_OK;
    }

    CodecVideoColorspace param;
    InitOMXParamExt(param);
    param.portIndex = OMX_DirInput;

    param.aspects.range = RANGE_UNSPECIFIED;
    if (range != -1) {
        param.aspects.range = TypeConverter::RangeFlagToOmxRangeType(static_cast<bool>(range));
    }
    param.aspects.primaries = TypeConverter::InnerPrimaryToOmxPrimary(primary);
    param.aspects.transfer = TypeConverter::InnerTransferToOmxTransfer(transfer);
    param.aspects.matrixCoeffs = TypeConverter::InnerMatrixToOmxMatrix(matrix);

    if (!SetParameter(OMX_IndexColorAspects, param, true)) {
        HLOGE("failed to set CodecVideoColorSpace");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set omx color aspects (full range:%d, primary:%d, "
          "transfer:%d, matrix:%d) succ",
          param.aspects.range, param.aspects.primaries, param.aspects.transfer, param.aspects.matrixCoeffs);
    return AVCS_ERR_OK;
}

void HEncoder::CalcInputBufSize(PortInfo &info, VideoPixelFormat pixelFmt)
{
    uint32_t inSize = AlignTo(info.width, 128u) * AlignTo(info.height, 128u); // 128: block size
    if (pixelFmt == VideoPixelFormat::RGBA) {
        inSize = inSize * 4; // 4 byte per pixel
    } else {
        inSize = inSize * 3 / 2; // 3: nom, 2: denom
    }
    info.inputBufSize = inSize;
}

int32_t HEncoder::SetupPort(const Format &format, std::optional<double> frameRate)
{
    constexpr int32_t MAX_ENCODE_WIDTH = 10000;
    constexpr int32_t MAX_ENCODE_HEIGHT = 10000;
    int32_t width;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width) || width <= 0 || width > MAX_ENCODE_WIDTH) {
        HLOGE("format should contain width");
        return AVCS_ERR_INVALID_VAL;
    }
    int32_t height;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height) || height <= 0 || height > MAX_ENCODE_HEIGHT) {
        HLOGE("format should contain height");
        return AVCS_ERR_INVALID_VAL;
    }
    HLOGI("user set width %d, height %d", width, height);
    if (!GetPixelFmtFromUser(format)) {
        return AVCS_ERR_INVALID_VAL;
    }

    if (!frameRate.has_value()) {
        HLOGI("user don't set valid frame rate, use default 60.0");
        frameRate = 60.0; // default frame rate 60.0
    }

    PortInfo inputPortInfo = {static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                              OMX_VIDEO_CodingUnused, configuredFmt_, frameRate.value()};
    CalcInputBufSize(inputPortInfo, configuredFmt_.innerFmt);
    int32_t ret = SetVideoPortInfo(OMX_DirInput, inputPortInfo);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    PortInfo outputPortInfo = {static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                               codingType_, std::nullopt, frameRate.value()};
    ret = SetVideoPortInfo(OMX_DirOutput, outputPortInfo);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    return AVCS_ERR_OK;
}

int32_t HEncoder::UpdateInPortFormat()
{
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParam(def);
    def.nPortIndex = OMX_DirInput;
    if (!GetParameter(OMX_IndexParamPortDefinition, def)) {
        HLOGE("get input port definition failed");
        return AVCS_ERR_UNKNOWN;
    }
    PrintPortDefinition(def);
    uint32_t w = def.format.video.nFrameWidth;
    uint32_t h = def.format.video.nFrameHeight;
    inBufferCnt_ = def.nBufferCountActual;

    // save into member variable
    requestCfg_.timeout = 0;
    requestCfg_.width = w;
    requestCfg_.height = h;
    requestCfg_.strideAlignment = STRIDE_ALIGNMENT;
    requestCfg_.format = configuredFmt_.graphicFmt;
    requestCfg_.usage = BUFFER_MODE_REQUEST_USAGE;

    if (inputFormat_ == nullptr) {
        inputFormat_ = make_shared<Format>();
    }
    inputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, w);
    inputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, h);
    inputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(configuredFmt_.innerFmt));
    return AVCS_ERR_OK;
}

int32_t HEncoder::UpdateOutPortFormat()
{
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParam(def);
    def.nPortIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexParamPortDefinition, def)) {
        HLOGE("get output port definition failed");
        return AVCS_ERR_UNKNOWN;
    }
    PrintPortDefinition(def);
    if (outputFormat_ == nullptr) {
        outputFormat_ = make_shared<Format>();
    }
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, def.format.video.nFrameWidth);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, def.format.video.nFrameHeight);
    return AVCS_ERR_OK;
}

static uint32_t SetPFramesSpacing(int32_t iFramesIntervalInMs, double frameRate, uint32_t bFramesSpacing = 0)
{
    if (iFramesIntervalInMs < 0) { // IPPPP...
        return UINT32_MAX - 1;
    }
    if (iFramesIntervalInMs == 0) { // IIIII...
        return 0;
    }
    uint32_t iFramesInterval = iFramesIntervalInMs * frameRate / TIME_RATIO_S_TO_MS;
    uint32_t pFramesSpacing = iFramesInterval / (bFramesSpacing + 1);
    return pFramesSpacing > 0 ? pFramesSpacing - 1 : 0;
}

std::optional<uint32_t> HEncoder::GetBitRateFromUser(const Format &format)
{
    int64_t bitRateLong;
    if (format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRateLong) && bitRateLong > 0 &&
        bitRateLong <= UINT32_MAX) {
        LOGI("user set bit rate %" PRId64 "", bitRateLong);
        return static_cast<uint32_t>(bitRateLong);
    }
    int32_t bitRateInt;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRateInt) && bitRateInt > 0) {
        LOGI("user set bit rate %d", bitRateInt);
        return static_cast<uint32_t>(bitRateInt);
    }
    return nullopt;
}

int32_t HEncoder::ConfigureOutputBitrate(const Format &format)
{
    VideoEncodeBitrateMode mode;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, *reinterpret_cast<int *>(&mode))) {
        return AVCS_ERR_OK;
    }
    switch (mode) {
        case CBR:
        case VBR: {
            optional<uint32_t> bitRate = GetBitRateFromUser(format);
            if (!bitRate.has_value()) {
                HLOGW("user set CBR/VBR mode but not set valid bitrate");
                return AVCS_ERR_INVALID_VAL;
            }
            OMX_VIDEO_PARAM_BITRATETYPE bitrateType;
            InitOMXParam(bitrateType);
            bitrateType.nPortIndex = OMX_DirOutput;
            bitrateType.eControlRate = (mode == CBR) ? OMX_Video_ControlRateConstant : OMX_Video_ControlRateVariable;
            bitrateType.nTargetBitrate = bitRate.value();
            if (!SetParameter(OMX_IndexParamVideoBitrate, bitrateType)) {
                HLOGE("failed to set OMX_IndexParamVideoBitrate");
                return AVCS_ERR_UNKNOWN;
            }
            HLOGI("set %s mode and target bitrate %u bps succ", (mode == CBR) ? "CBR" : "VBR",
                bitrateType.nTargetBitrate);
            return AVCS_ERR_OK;
        }
        case CQ: {
            int32_t quality;
            if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_QUALITY, quality) || quality < 0) {
                HLOGW("user set CQ mode but not set valid quality");
                return AVCS_ERR_INVALID_VAL;
            }
            ControlRateConstantQuality bitrateType;
            InitOMXParamExt(bitrateType);
            bitrateType.portIndex = OMX_DirOutput;
            bitrateType.qualityValue = static_cast<uint32_t>(quality);
            if (!SetParameter(OMX_IndexParamControlRateConstantQuality, bitrateType)) {
                HLOGE("failed to set OMX_IndexParamControlRateConstantQuality");
                return AVCS_ERR_UNKNOWN;
            }
            HLOGI("set CQ mode and target quality %u succ", bitrateType.qualityValue);
            return AVCS_ERR_OK;
        }
        default:
            return AVCS_ERR_INVALID_VAL;
    }
}

int32_t HEncoder::SetupAVCEncoderParameters(const Format &format, std::optional<double> frameRate)
{
    OMX_VIDEO_PARAM_AVCTYPE avcType;
    InitOMXParam(avcType);
    avcType.nPortIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexParamVideoAvc, avcType)) {
        HLOGE("get OMX_IndexParamVideoAvc parameter fail");
        return AVCS_ERR_UNKNOWN;
    }
    avcType.nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
    avcType.nBFrames = 0;

    AVCProfile profile;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_PROFILE, *reinterpret_cast<int *>(&profile))) {
        optional<OMX_VIDEO_AVCPROFILETYPE> omxAvcProfile = TypeConverter::InnerAvcProfileToOmxProfile(profile);
        if (omxAvcProfile.has_value()) {
            avcType.eProfile = omxAvcProfile.value();
        }
    }
    int32_t iFrameInterval;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, iFrameInterval) && frameRate.has_value()) {
        SetAvcFields(avcType, iFrameInterval, frameRate.value());
    }

    if (avcType.nBFrames != 0) {
        avcType.nAllowedPictureTypes |= OMX_VIDEO_PictureTypeB;
    }
    avcType.bEnableUEP = OMX_FALSE;
    avcType.bEnableFMO = OMX_FALSE;
    avcType.bEnableASO = OMX_FALSE;
    avcType.bEnableRS = OMX_FALSE;
    avcType.bFrameMBsOnly = OMX_TRUE;
    avcType.bMBAFF = OMX_FALSE;
    avcType.eLoopFilterMode = OMX_VIDEO_AVCLoopFilterEnable;

    if (!SetParameter(OMX_IndexParamVideoAvc, avcType)) {
        HLOGE("failed to set OMX_IndexParamVideoAvc");
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

void HEncoder::SetAvcFields(OMX_VIDEO_PARAM_AVCTYPE &avcType, int32_t iFrameInterval, double frameRate)
{
    HLOGI("iFrameInterval:%d, frameRate:%.2f, eProfile:0x%x, eLevel:0x%x",
          iFrameInterval, frameRate, avcType.eProfile, avcType.eLevel);

    if (avcType.eProfile == OMX_VIDEO_AVCProfileBaseline) {
        avcType.nSliceHeaderSpacing = 0;
        avcType.bUseHadamard = OMX_TRUE;
        avcType.nRefFrames = 1;
        avcType.nPFrames = SetPFramesSpacing(iFrameInterval, frameRate, avcType.nBFrames);
        if (avcType.nPFrames == 0) {
            avcType.nAllowedPictureTypes = OMX_VIDEO_PictureTypeI;
        }
        avcType.nRefIdx10ActiveMinus1 = 0;
        avcType.nRefIdx11ActiveMinus1 = 0;
        avcType.bEntropyCodingCABAC = OMX_FALSE;
        avcType.bWeightedPPrediction = OMX_FALSE;
        avcType.bconstIpred = OMX_FALSE;
        avcType.bDirect8x8Inference = OMX_FALSE;
        avcType.bDirectSpatialTemporal = OMX_FALSE;
        avcType.nCabacInitIdc = 0;
    } else if (avcType.eProfile == OMX_VIDEO_AVCProfileMain || avcType.eProfile == OMX_VIDEO_AVCProfileHigh) {
        avcType.nSliceHeaderSpacing = 0;
        avcType.bUseHadamard = OMX_TRUE;
        avcType.nRefFrames = avcType.nBFrames == 0 ? 1 : 2; // 2 is number of reference frames
        avcType.nPFrames = SetPFramesSpacing(iFrameInterval, frameRate, avcType.nBFrames);
        avcType.nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
        avcType.nRefIdx10ActiveMinus1 = 0;
        avcType.nRefIdx11ActiveMinus1 = 0;
        avcType.bEntropyCodingCABAC = OMX_TRUE;
        avcType.bWeightedPPrediction = OMX_TRUE;
        avcType.bconstIpred = OMX_TRUE;
        avcType.bDirect8x8Inference = OMX_TRUE;
        avcType.bDirectSpatialTemporal = OMX_TRUE;
        avcType.nCabacInitIdc = 1;
    }
}

int32_t HEncoder::SetupHEVCEncoderParameters(const Format &format, std::optional<double> frameRate)
{
    CodecVideoParamHevc hevcType;
    InitOMXParamExt(hevcType);
    hevcType.portIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexParamVideoHevc, hevcType)) {
        HLOGE("get OMX_IndexParamVideoHevc parameter fail");
        return AVCS_ERR_UNKNOWN;
    }

    HEVCProfile profile;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_PROFILE, *reinterpret_cast<int *>(&profile))) {
        optional<CodecHevcProfile> omxHevcProfile = TypeConverter::InnerHevcProfileToOmxProfile(profile);
        if (omxHevcProfile.has_value()) {
            hevcType.profile = omxHevcProfile.value();
            HLOGI("HEVCProfile %d, CodecHevcProfile 0x%x", profile, hevcType.profile);
        }
    }

    int32_t iFrameInterval;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, iFrameInterval) && frameRate.has_value()) {
        if (iFrameInterval < 0) { // IPPPP...
            hevcType.keyFrameInterval = UINT32_MAX - 1;
        } else if (iFrameInterval == 0) { // all intra
            hevcType.keyFrameInterval = 1;
        } else {
            hevcType.keyFrameInterval = iFrameInterval * frameRate.value() / TIME_RATIO_S_TO_MS;
        }
        HLOGI("frameRate %.2f, iFrameInterval %d, keyFrameInterval %u", frameRate.value(),
              iFrameInterval, hevcType.keyFrameInterval);
    }

    if (!SetParameter(OMX_IndexParamVideoHevc, hevcType)) {
        HLOGE("failed to set OMX_IndexParamVideoHevc");
        return AVCS_ERR_INVALID_VAL;
    }
    return AVCS_ERR_OK;
}

int32_t HEncoder::RequestIDRFrame()
{
    OMX_CONFIG_INTRAREFRESHVOPTYPE params;
    InitOMXParam(params);
    params.nPortIndex = OMX_DirOutput;
    params.IntraRefreshVOP = OMX_TRUE;
    if (!SetParameter(OMX_IndexConfigVideoIntraVOPRefresh, params, true)) {
        HLOGE("failed to request IDR frame");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("Set IDR Frame success");
    return AVCS_ERR_OK;
}

int32_t HEncoder::OnSetParameters(const Format &format)
{
    optional<uint32_t> bitRate = GetBitRateFromUser(format);
    if (bitRate.has_value()) {
        OMX_VIDEO_CONFIG_BITRATETYPE bitrateCfgType;
        InitOMXParam(bitrateCfgType);
        bitrateCfgType.nPortIndex = OMX_DirOutput;
        bitrateCfgType.nEncodeBitrate = bitRate.value();
        if (!SetParameter(OMX_IndexConfigVideoBitrate, bitrateCfgType, true)) {
            HLOGW("failed to config OMX_IndexConfigVideoBitrate");
        }
    }

    optional<double> frameRate = GetFrameRateFromUser(format);
    if (frameRate.has_value()) {
        OMX_CONFIG_FRAMERATETYPE framerateCfgType;
        InitOMXParam(framerateCfgType);
        framerateCfgType.nPortIndex = OMX_DirInput;
        framerateCfgType.xEncodeFramerate = frameRate.value() * FRAME_RATE_COEFFICIENT;
        if (!SetParameter(OMX_IndexConfigVideoFramerate, framerateCfgType, true)) {
            HLOGW("failed to config OMX_IndexConfigVideoFramerate");
        }
    }

    int32_t requestIdr;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_REQUEST_I_FRAME, requestIdr) && requestIdr != 0) {
        int32_t ret = RequestIDRFrame();
        if (ret != AVCS_ERR_OK) {
            return ret;
        }
    }

    int32_t ret = SetQpRange(format, true);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    return AVCS_ERR_OK;
}

int32_t HEncoder::SubmitOutputBuffersToOmxNode()
{
    for (BufferInfo &info : outputBufferPool_) {
        if (info.owner == BufferOwner::OWNED_BY_US) {
            int32_t ret = NotifyOmxToFillThisOutBuffer(info);
            if (ret != AVCS_ERR_OK) {
                return ret;
            }
        } else {
            HLOGE("buffer should be owned by us");
            return AVCS_ERR_UNKNOWN;
        }
    }
    return AVCS_ERR_OK;
}

bool HEncoder::ReadyToStart()
{
    if (callback_ == nullptr || outputFormat_ == nullptr || inputFormat_ == nullptr) {
        return false;
    }
    if (inputSurface_) {
        HLOGI("surface mode");
    } else {
        HLOGI("buffer mode");
    }
    return true;
}

int32_t HEncoder::AllocateBuffersOnPort(OMX_DIRTYPE portIndex)
{
    if (portIndex == OMX_DirOutput) {
        return AllocateAvLinearBuffers(portIndex);
    }
    if (inputSurface_) {
        return AllocInBufsForDynamicSurfaceBuf();
    } else {
        int32_t ret = AllocateAvSurfaceBuffers(portIndex);
        if (ret == AVCS_ERR_OK) {
            UpdateFormatFromSurfaceBuffer();
        }
        return ret;
    }
}

void HEncoder::UpdateFormatFromSurfaceBuffer()
{
    if (inputBufferPool_.empty()) {
        return;
    }
    sptr<SurfaceBuffer> surfaceBuffer = inputBufferPool_.front().surfaceBuffer;
    if (surfaceBuffer == nullptr) {
        return;
    }
    int32_t stride = surfaceBuffer->GetStride();
    HLOGI("input stride = %d", stride);
    inputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, stride);
}

int32_t HEncoder::SubmitAllBuffersOwnedByUs()
{
    HLOGI(">>");
    if (isBufferCirculating_) {
        HLOGI("buffer is already circulating, no need to do again");
        return AVCS_ERR_OK;
    }
    int32_t ret = SubmitOutputBuffersToOmxNode();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    if (inputSurface_) {
        sptr<IBufferConsumerListener> listener = new EncoderBuffersConsumerListener(this);
        inputSurface_->RegisterConsumerListener(listener);
        SendAsyncMsg(MsgWhat::GET_BUFFER_FROM_SURFACE, nullptr);
    } else {
        for (BufferInfo &info : inputBufferPool_) {
            if (info.owner == BufferOwner::OWNED_BY_US) {
                NotifyUserToFillThisInBuffer(info);
            }
        }
    }

    isBufferCirculating_ = true;
    return AVCS_ERR_OK;
}

sptr<Surface> HEncoder::OnCreateInputSurface()
{
    if (inputSurface_) {
        HLOGE("inputSurface_ already exists");
        return nullptr;
    }

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("HEncoderSurface");
    if (consumerSurface == nullptr) {
        HLOGE("Create the surface consummer fail");
        return nullptr;
    }
    GSError err = consumerSurface->SetDefaultUsage(SURFACE_MODE_CONSUMER_USAGE);
    if (err == GSERROR_OK) {
        HLOGI("set consumer usage 0x%x succ", SURFACE_MODE_CONSUMER_USAGE);
    } else {
        HLOGW("set consumer usage 0x%x failed", SURFACE_MODE_CONSUMER_USAGE);
    }

    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    if (producer == nullptr) {
        HLOGE("Get the surface producer fail");
        return nullptr;
    }

    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);
    if (producerSurface == nullptr) {
        HLOGE("CreateSurfaceAsProducer fail");
        return nullptr;
    }

    inputSurface_ = consumerSurface;
    if (inBufferCnt_ > inputSurface_->GetQueueSize()) {
        inputSurface_->SetQueueSize(inBufferCnt_);
    }
    HLOGI("queue size %u", inputSurface_->GetQueueSize());
    return producerSurface;
}

int32_t HEncoder::OnSetInputSurface(sptr<Surface> &inputSurface)
{
    if (inputSurface_) {
        HLOGW("inputSurface_ already exists");
    }

    if (inputSurface == nullptr) {
        HLOGE("surface is null");
        return AVCS_ERR_INVALID_VAL;
    }
    if (!inputSurface->IsConsumer()) {
        HLOGE("expect consumer surface");
        return AVCS_ERR_INVALID_VAL;
    }

    inputSurface_ = inputSurface;
    if (inBufferCnt_ > inputSurface_->GetQueueSize()) {
        inputSurface_->SetQueueSize(inBufferCnt_);
    }
    HLOGI("succ");
    return AVCS_ERR_OK;
}

int32_t HEncoder::WrapSurfaceBufferIntoOmxBuffer(shared_ptr<OmxCodecBuffer> &omxBuffer,
                                                 const sptr<SurfaceBuffer> &surfaceBuffer, int64_t pts, uint32_t flag)
{
    BufferHandle *bufferHandle = surfaceBuffer->GetBufferHandle();
    if (bufferHandle == nullptr) {
        HLOGE("null BufferHandle");
        return AVCS_ERR_UNKNOWN;
    }
    omxBuffer->bufferhandle = new NativeBuffer(bufferHandle);
    omxBuffer->filledLen = surfaceBuffer->GetSize();
    omxBuffer->fd = -1;
    omxBuffer->fenceFd = -1;
    omxBuffer->pts = pts;
    omxBuffer->flag = flag;
    return AVCS_ERR_OK;
}

void HEncoder::WrapPerFrameParamIntoOmxBuffer(shared_ptr<OmxCodecBuffer> &omxBuffer,
                                              const shared_ptr<Media::Meta> &meta)
{
    omxBuffer->alongParam.clear();
    WrapLTRParamIntoOmxBuffer(omxBuffer, meta);
    WrapRequestIFrameParamIntoOmxBuffer(omxBuffer, meta);
    WrapQPRangeParamIntoOmxBuffer(omxBuffer, meta);
    meta->Clear();
}

void HEncoder::WrapLTRParamIntoOmxBuffer(shared_ptr<OmxCodecBuffer> &omxBuffer,
                                         const shared_ptr<Media::Meta> &meta)
{
    if (!enableLTR) {
        return;
    }
    AppendToVector(omxBuffer->alongParam, OMX_IndexParamLTR);
    CodecLTRPerFrameParam param;
    int32_t markLTR = 0;
    meta->GetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_MARK_LTR, markLTR);
    param.markAsLTR = (markLTR == 0) ? false : true;
    int32_t useLtrPoc = 0;
    param.useLTR = meta->GetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_USE_LTR, useLtrPoc);
    param.useLTRPoc = static_cast<uint32_t>(useLtrPoc);
    AppendToVector(omxBuffer->alongParam, param);
}

void HEncoder::WrapRequestIFrameParamIntoOmxBuffer(shared_ptr<OHOS::HDI::Codec::V3_0::OmxCodecBuffer> &omxBuffer,
                                                   const shared_ptr<Media::Meta> &meta)
{
    bool requestIFrame = false;
    meta->GetData(OHOS::Media::Tag::VIDEO_REQUEST_I_FRAME, requestIFrame);
    if (!requestIFrame) {
        return;
    }
    AppendToVector(omxBuffer->alongParam, OMX_IndexConfigVideoIntraVOPRefresh);
    OMX_CONFIG_INTRAREFRESHVOPTYPE params;
    InitOMXParam(params);
    params.nPortIndex = OMX_DirOutput;
    params.IntraRefreshVOP = OMX_TRUE;
    AppendToVector(omxBuffer->alongParam, params);
    HLOGI("pts=%" PRId64 ", requestIFrame", omxBuffer->pts);
}

void HEncoder::WrapQPRangeParamIntoOmxBuffer(shared_ptr<OHOS::HDI::Codec::V3_0::OmxCodecBuffer> &omxBuffer,
                                             const shared_ptr<Media::Meta> &meta)
{
    int32_t minQp;
    int32_t maxQp;
    if (!meta->GetData(OHOS::Media::Tag::VIDEO_ENCODER_QP_MIN, minQp) ||
        !meta->GetData(OHOS::Media::Tag::VIDEO_ENCODER_QP_MAX, maxQp)) {
        return;
    }
    AppendToVector(omxBuffer->alongParam, OMX_IndexParamQPRange);
    CodecQPRangeParam param;
    InitOMXParamExt(param);
    param.minQp = static_cast<uint32_t>(minQp);
    param.maxQp = static_cast<uint32_t>(maxQp);
    AppendToVector(omxBuffer->alongParam, param);
    HLOGI("pts=%" PRId64 ", qp=(%d~%d)", omxBuffer->pts, minQp, maxQp);
}

int32_t HEncoder::AllocInBufsForDynamicSurfaceBuf()
{
    inputBufferPool_.clear();
    for (uint32_t i = 0; i < inBufferCnt_; ++i) {
        shared_ptr<OmxCodecBuffer> omxBuffer = DynamicSurfaceBufferToOmxBuffer();
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t ret = compNode_->UseBuffer(OMX_DirInput, *omxBuffer, *outBuffer);
        if (ret != HDF_SUCCESS) {
            HLOGE("Failed to UseBuffer on input port");
            return AVCS_ERR_UNKNOWN;
        }
        BufferInfo info {};
        info.isInput = true;
        info.owner = BufferOwner::OWNED_BY_US;
        info.surfaceBuffer = nullptr;
        info.avBuffer = AVBuffer::CreateAVBuffer();
        info.omxBuffer = outBuffer;
        info.bufferId = outBuffer->bufferId;
        inputBufferPool_.push_back(info);
    }

    return AVCS_ERR_OK;
}

void HEncoder::EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i)
{
    vector<BufferInfo> &pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    if (i >= pool.size()) {
        return;
    }
    const BufferInfo &info = pool[i];
    FreeOmxBuffer(portIndex, info);
    pool.erase(pool.begin() + i);
}

void HEncoder::OnQueueInputBuffer(const MsgInfo &msg, BufferOperationMode mode)
{
    if (inputSurface_ && !enableSurfaceModeInputCb_) {
        HLOGE("cannot queue input on surface mode");
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_OPERATION);
        return;
    }
    // buffer mode
    uint32_t bufferId;
    (void)msg.param->GetValue(BUFFER_ID, bufferId);
    SCOPED_TRACE_WITH_ID(bufferId);
    BufferInfo* bufferInfo = FindBufferInfoByID(OMX_DirInput, bufferId);
    if (bufferInfo == nullptr) {
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    if (bufferInfo->owner != BufferOwner::OWNED_BY_USER) {
        HLOGE("wrong ownership: buffer id=%d, owner=%s", bufferId, ToString(bufferInfo->owner));
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    int err = WrapSurfaceBufferIntoOmxBuffer(bufferInfo->omxBuffer, bufferInfo->surfaceBuffer,
        bufferInfo->avBuffer->pts_,
        UserFlagToOmxFlag(static_cast<AVCodecBufferFlag>(bufferInfo->avBuffer->flag_)));
    if (err != AVCS_ERR_OK) {
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    WrapPerFrameParamIntoOmxBuffer(bufferInfo->omxBuffer, bufferInfo->avBuffer->meta_);

    ChangeOwner(*bufferInfo, BufferOwner::OWNED_BY_US);
    ReplyErrorCode(msg.id, AVCS_ERR_OK);
    HCodec::OnQueueInputBuffer(mode, bufferInfo);
}

void HEncoder::OnGetBufferFromSurface(const ParamSP& param)
{
    while (true) {
        if (!GetOneBufferFromSurface()) {
            break;
        }
    }
}

bool HEncoder::GetOneBufferFromSurface()
{
    SCOPED_TRACE();
    InSurfaceBufferEntry entry;
    GSError ret = inputSurface_->AcquireBuffer(entry.buffer, entry.fence, entry.timestamp, entry.damage);
    if (ret != GSERROR_OK || entry.buffer == nullptr) {
        return false;
    }
    avaliableBuffers_.push_back(entry);
    if (debugMode_) {
        HLOGI("acquire buffer succ, pts = %" PRId64 ", now we have %zu buffer wait to be encode",
              entry.timestamp, avaliableBuffers_.size());
    }
    FindAllIdleSlotAndSubmit();
    return true;
}

void HEncoder::FindAllIdleSlotAndSubmit()
{
    while (true) {
        if (avaliableBuffers_.empty()) {
            return;
        }
        auto it = find_if(inputBufferPool_.begin(), inputBufferPool_.end(),
                          [](const BufferInfo &info) { return info.owner == BufferOwner::OWNED_BY_US; });
        if (it == inputBufferPool_.end()) {
            return;
        }
        SubmitOneBuffer(*it);
    }
}

void HEncoder::SubmitOneBuffer(BufferInfo &info)
{
    InSurfaceBufferEntry entry = avaliableBuffers_.front();
    avaliableBuffers_.pop_front();
    if (entry.buffer == nullptr) {
        HLOGI("got input eos");
        inputPortEos_ = true;
        info.omxBuffer->flag = OMX_BUFFERFLAG_EOS;
        info.surfaceBuffer = nullptr;
        NotifyOmxToEmptyThisInBuffer(info);
        return;
    }
    if (!WaitFence(entry.fence)) {
        inputSurface_->ReleaseBuffer(entry.buffer, -1);
        return;
    }
    int32_t err = WrapSurfaceBufferIntoOmxBuffer(info.omxBuffer, entry.buffer, entry.timestamp, 0);
    if (err != AVCS_ERR_OK) {
        inputSurface_->ReleaseBuffer(entry.buffer, -1);
        return;
    }
    info.surfaceBuffer = entry.buffer;

    if (enableSurfaceModeInputCb_) {
        NotifyUserToFillThisInBuffer(info);
    } else {
        err = NotifyOmxToEmptyThisInBuffer(info);
        if (err != AVCS_ERR_OK) {
            inputSurface_->ReleaseBuffer(entry.buffer, -1);
        }
    }
}

void HEncoder::OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode)
{
    SCOPED_TRACE_WITH_ID(bufferId);
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
    if (inputSurface_) {
        if (info->surfaceBuffer != nullptr) {
            inputSurface_->ReleaseBuffer(info->surfaceBuffer, -1);
        }
        if (mode == RESUBMIT_BUFFER && !inputPortEos_) {
            FindAllIdleSlotAndSubmit();
        }
    } else {
        if (mode == RESUBMIT_BUFFER && !inputPortEos_) {
            NotifyUserToFillThisInBuffer(*info);
        }
    }
}

void HEncoder::EncoderBuffersConsumerListener::OnBufferAvailable()
{
    codec_->SendAsyncMsg(MsgWhat::GET_BUFFER_FROM_SURFACE, nullptr);
}

void HEncoder::OnSignalEndOfInputStream(const MsgInfo &msg)
{
    if (inputSurface_ == nullptr) {
        HLOGE("can only be called in surface mode");
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_OPERATION);
        return;
    }
    ReplyErrorCode(msg.id, AVCS_ERR_OK);
    avaliableBuffers_.push_back(InSurfaceBufferEntry {});
    FindAllIdleSlotAndSubmit();
}

void HEncoder::OnEnterUninitializedState()
{
    if (inputSurface_) {
        inputSurface_->UnregisterConsumerListener();
        HLOGI("return %zu stale buffer to surface", avaliableBuffers_.size());
        for (auto& entry : avaliableBuffers_) {
            inputSurface_->ReleaseBuffer(entry.buffer, -1);
        }
    }
    avaliableBuffers_.clear();
}
} // namespace OHOS::MediaAVCodec
