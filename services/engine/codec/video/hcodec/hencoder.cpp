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
using namespace CodecHDI;

HEncoder::~HEncoder()
{
    MsgHandleLoop::Stop();
}

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
    ConfigureProtocol(format, frameRate);

    ret = ConfigureOutputBitrate(format);
    if (ret != AVCS_ERR_OK) {
        HLOGW("ConfigureOutputBitrate failed");
    }
    ret = SetColorAspects(format);
    if (ret != AVCS_ERR_OK) {
        HLOGW("set color aspect failed");
    }
    (void)SetProcessName();
    (void)SetFrameRateAdaptiveMode(format);
    CheckIfEnableCb(format);
    ret = SetTemperalLayer(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
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
    ret = SetRepeat(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    (void)EnableEncoderParamsFeedback(format);
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

int32_t HEncoder::SetRepeat(const Format &format)
{
    int repeatMs = 0;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatMs)) {
        return AVCS_ERR_OK;
    }
    if (repeatMs <= 0) {
        HLOGW("invalid repeatMs %d", repeatMs);
        return AVCS_ERR_INVALID_VAL;
    }
    repeatUs_ = static_cast<uint64_t>(repeatMs * TIME_RATIO_S_TO_MS);

    int repeatMaxCnt = 0;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatMaxCnt)) {
        return AVCS_ERR_OK;
    }
    if (repeatMaxCnt == 0) {
        HLOGW("invalid repeatMaxCnt %d", repeatMaxCnt);
        return AVCS_ERR_INVALID_VAL;
    }
    repeatMaxCnt_ = repeatMaxCnt;
    return AVCS_ERR_OK;
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
    if (enableTSVC_) {
        HLOGW("user has enabled temporal scale, can not set LTR param");
        return AVCS_ERR_INVALID_VAL;
    }
    CodecLTRParam info;
    InitOMXParamExt(info);
    info.ltrFrameListLen = static_cast<uint32_t>(ltrFrameNum);
    if (!SetParameter(OMX_IndexParamLTR, info)) {
        HLOGE("configure LTR failed");
        return AVCS_ERR_INVALID_VAL;
    }
    enableLTR_ = true;
    return AVCS_ERR_OK;
}

int32_t HEncoder::EnableEncoderParamsFeedback(const Format &format)
{
    int32_t enableParamsFeedback {};
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_PARAMS_FEEDBACK, enableParamsFeedback)) {
        return AVCS_ERR_OK;
    }
    OMX_CONFIG_BOOLEANTYPE param {};
    InitOMXParam(param);
    param.bEnabled = enableParamsFeedback ? OMX_TRUE : OMX_FALSE;
    if (!SetParameter(OMX_IndexParamEncParamsFeedback, param)) {
        HLOGE("configure encoder params feedback[%d] failed", enableParamsFeedback);
        return AVCS_ERR_INVALID_VAL;
    }
    HLOGI("configure encoder params feedback[%d] success", enableParamsFeedback);
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

int32_t HEncoder::SetTemperalLayer(const Format &format)
{
    int32_t enableTemporalScale = 0;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, enableTemporalScale) ||
        (enableTemporalScale == 0)) {
        return AVCS_ERR_OK;
    }
    Media::Plugins::TemporalGopReferenceMode GopReferenceMode{};
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
        *reinterpret_cast<int *>(&GopReferenceMode)) ||
        static_cast<int32_t>(GopReferenceMode) != 2) { // 2: gop mode
        HLOGE("user enable temporal scalability but not set invalid temporal gop mode");
        return AVCS_ERR_INVALID_VAL;
    }
    int32_t temporalGopSize = 0;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, temporalGopSize)) {
        HLOGE("user enable temporal scalability but not set invalid temporal gop size");
        return AVCS_ERR_INVALID_VAL;
    }

    CodecTemperalLayerParam temperalLayerParam;
    InitOMXParamExt(temperalLayerParam);
    switch (temporalGopSize) {
        case 2: // 2: picture size of the temporal group
            temperalLayerParam.layerCnt = 2; // 2: layer of the temporal group
            break;
        case 4: // 4: picture size of the temporal group
            temperalLayerParam.layerCnt = 3; // 3: layer of the temporal group
            break;
        default:
            HLOGE("user set invalid temporal gop size %d", temporalGopSize);
            return AVCS_ERR_INVALID_VAL;
    }
    
    if (!SetParameter(OMX_IndexParamTemperalLayer, temperalLayerParam)) {
        HLOGE("set temporal layer param failed");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set temporal layer param %d succ", temperalLayerParam.layerCnt);
    enableTSVC_ = true;
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
    requestCfg_.width = static_cast<int32_t>(w);
    requestCfg_.height = static_cast<int32_t>(h);
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
            outputFormat_->PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE,
                static_cast<int64_t>(bitrateType.nTargetBitrate));
            outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
                static_cast<int32_t>(mode));
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
            outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, quality);
            outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
                static_cast<int32_t>(mode));
            HLOGI("set CQ mode and target quality %u succ", bitrateType.qualityValue);
            return AVCS_ERR_OK;
        }
        default:
            return AVCS_ERR_INVALID_VAL;
    }
}

void HEncoder::ConfigureProtocol(const Format &format, std::optional<double> frameRate)
{
    int32_t ret = AVCS_ERR_OK;
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

void HEncoder::ClearDirtyList()
{
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    int64_t pts = -1;
    OHOS::Rect damage;
    while (true) {
        GSError ret = inputSurface_->AcquireBuffer(buffer, fence, pts, damage);
        if (ret != GSERROR_OK || buffer == nullptr) {
            return;
        }
        HLOGI("return stale buffer to surface, seq = %u, pts = %" PRId64 "", buffer->GetSeqNum(), pts);
        inputSurface_->ReleaseBuffer(buffer, -1);
    }
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
        ClearDirtyList();
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

void HEncoder::WrapPerFrameParamIntoOmxBuffer(shared_ptr<OmxCodecBuffer> &omxBuffer,
                                              const shared_ptr<Media::Meta> &meta)
{
    omxBuffer->alongParam.clear();
    WrapLTRParamIntoOmxBuffer(omxBuffer, meta);
    WrapRequestIFrameParamIntoOmxBuffer(omxBuffer, meta);
    WrapQPRangeParamIntoOmxBuffer(omxBuffer, meta);
    WrapStartQPIntoOmxBuffer(omxBuffer, meta);
    WrapIsSkipFrameIntoOmxBuffer(omxBuffer, meta);
    meta->Clear();
}

void HEncoder::WrapLTRParamIntoOmxBuffer(shared_ptr<OmxCodecBuffer> &omxBuffer,
                                         const shared_ptr<Media::Meta> &meta)
{
    if (!enableLTR_) {
        return;
    }
    AppendToVector(omxBuffer->alongParam, OMX_IndexParamLTR);
    CodecLTRPerFrameParam param;
    bool markLTR = false;
    meta->GetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_MARK_LTR, markLTR);
    param.markAsLTR = markLTR;
    int32_t useLtrPoc = 0;
    param.useLTR = meta->GetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_USE_LTR, useLtrPoc);
    param.useLTRPoc = static_cast<uint32_t>(useLtrPoc);
    AppendToVector(omxBuffer->alongParam, param);
}

void HEncoder::WrapRequestIFrameParamIntoOmxBuffer(shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
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

void HEncoder::WrapQPRangeParamIntoOmxBuffer(shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
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

void HEncoder::WrapStartQPIntoOmxBuffer(shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                        const shared_ptr<Media::Meta> &meta)
{
    int32_t startQp {};
    if (!meta->GetData(OHOS::Media::Tag::VIDEO_ENCODER_QP_START, startQp)) {
        return;
    }
    AppendToVector(omxBuffer->alongParam, OMX_IndexParamQPStsart);
    AppendToVector(omxBuffer->alongParam, startQp);
}

void HEncoder::WrapIsSkipFrameIntoOmxBuffer(shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                            const shared_ptr<Media::Meta> &meta)
{
    bool isSkip {};
    if (!meta->GetData(OHOS::Media::Tag::VIDEO_PER_FRAME_IS_SKIP, isSkip)) {
        return;
    }
    AppendToVector(omxBuffer->alongParam, OMX_IndexParamSkipFrame);
    AppendToVector(omxBuffer->alongParam, isSkip);
}

void HEncoder::ExtractPerFrameParamFromOmxBuffer(
    const shared_ptr<OmxCodecBuffer> &omxBuffer, shared_ptr<Media::Meta> &meta)
{
    meta->Clear();
    BinaryReader reader(static_cast<uint8_t*>(omxBuffer->alongParam.data()), omxBuffer->alongParam.size());
    int* index = nullptr;
    while ((index = reader.Read<int>()) != nullptr) {
        switch (*index) {
            case OMX_IndexParamEncOutQp: {
                auto *averageQp = reader.Read<OMX_S32>();
                if (averageQp == nullptr) {
                    return;
                }
                HLOGD("pts=%" PRId64 ", averageQp=(%d)", omxBuffer->pts, *averageQp);
                meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_QP_AVERAGE, *averageQp);
                break;
            }
            case OMX_IndexParamEncOutMse: {
                auto *averageMseLcu = reader.Read<double>();
                if (averageMseLcu == nullptr) {
                    return;
                }
                HLOGD("pts=%" PRId64 ", averageMseLcu=(%f)", omxBuffer->pts, *averageMseLcu);
                meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_MSE, *averageMseLcu);
                break;
            }
            case OMX_IndexParamEncOutLTR: {
                auto *encOutLtrParam = reader.Read<CodecEncOutLTRParam>();
                ExtractPerFrameLTRParam(encOutLtrParam, meta);
                break;
            }
            case OMX_IndexParamEncOutFrameLayer: {
                auto *frameLayer = reader.Read<OMX_S32>();
                if (frameLayer == nullptr) {
                    return;
                }
                HLOGD("pts=%" PRId64 ", frameLayer=(%d)", omxBuffer->pts, *frameLayer);
                meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_FRAME_TEMPORAL_ID, *frameLayer);
                break;
            }
            default: {
                break;
            }
        }
    }
    omxBuffer->alongParam.clear();
}

void HEncoder::ExtractPerFrameLTRParam(const CodecEncOutLTRParam* ltrParam, shared_ptr<Media::Meta> &meta)
{
    if (ltrParam == nullptr) {
        return;
    }
    meta->SetData(OHOS::Media::Tag::VIDEO_PER_FRAME_IS_LTR, ltrParam->isLTR);
    meta->SetData(OHOS::Media::Tag::VIDEO_PER_FRAME_POC, static_cast<int32_t>(ltrParam->poc));
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
        info.owner = BufferOwner::OWNED_BY_SURFACE;
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
    ReduceOwner((portIndex == OMX_DirInput), info.owner);
    pool.erase(pool.begin() + i);
}

void HEncoder::OnQueueInputBuffer(const MsgInfo &msg, BufferOperationMode mode)
{
    if (inputSurface_ && !enableSurfaceModeInputCb_) {
        HLOGE("cannot queue input on surface mode");
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_OPERATION);
        return;
    }
    // buffer mode or surface callback mode
    uint32_t bufferId = 0;
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

    bool discard = false;
    if (inputSurface_ && bufferInfo->avBuffer->meta_->GetData(
        OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, discard) && discard) {
        HLOGI("inBufId = %u, discard by user, pts = %" PRId64, bufferId, bufferInfo->avBuffer->pts_);
        bufferInfo->avBuffer->meta_->Clear();
        ResetSlot(*bufferInfo);
        ReplyErrorCode(msg.id, AVCS_ERR_OK);
        return;
    }
    ChangeOwner(*bufferInfo, BufferOwner::OWNED_BY_US);
    WrapSurfaceBufferToSlot(*bufferInfo, bufferInfo->surfaceBuffer, bufferInfo->avBuffer->pts_,
        UserFlagToOmxFlag(static_cast<AVCodecBufferFlag>(bufferInfo->avBuffer->flag_)));
    WrapPerFrameParamIntoOmxBuffer(bufferInfo->omxBuffer, bufferInfo->avBuffer->meta_);
    ReplyErrorCode(msg.id, AVCS_ERR_OK);
    HCodec::OnQueueInputBuffer(mode, bufferInfo);
}

void HEncoder::OnGetBufferFromSurface(const ParamSP& param)
{
    if (GetOneBufferFromSurface()) {
        TraverseAvaliableBuffers();
    }
}

bool HEncoder::GetOneBufferFromSurface()
{
    SCOPED_TRACE();
    InSurfaceBufferEntry entry{};
    entry.item = make_shared<BufferItem>();
    GSError ret = inputSurface_->AcquireBuffer(
        entry.item->buffer, entry.item->fence, entry.pts, entry.item->damage);
    if (ret != GSERROR_OK || entry.item->buffer == nullptr) {
        return false;
    }
    entry.item->generation = ++currGeneration_;
    entry.item->surface = inputSurface_;
    avaliableBuffers_.push_back(entry);
    newestBuffer_ = entry;
    HLOGD("generation = %" PRIu64 ", seq = %u, pts = %" PRId64 ", now list size = %zu",
          entry.item->generation, entry.item->buffer->GetSeqNum(), entry.pts, avaliableBuffers_.size());
    if (repeatUs_ != 0) {
        SendRepeatMsg(entry.item->generation);
    }
    return true;
}

void HEncoder::SendRepeatMsg(uint64_t generation)
{
    ParamSP param = make_shared<ParamBundle>();
    param->SetValue("generation", generation);
    SendAsyncMsg(MsgWhat::CHECK_IF_REPEAT, param, repeatUs_);
}

void HEncoder::RepeatIfNecessary(const ParamSP& param)
{
    uint64_t generation = 0;
    param->GetValue("generation", generation);
    if (inputPortEos_ || (repeatUs_ == 0) || newestBuffer_.item == nullptr ||
        newestBuffer_.item->generation != generation) {
        return;
    }
    if (repeatMaxCnt_ > 0 && newestBuffer_.repeatTimes >= repeatMaxCnt_) {
        HLOGD("stop repeat generation = %" PRIu64 ", seq = %u, pts = %" PRId64 ", which has been repeated %d times",
              generation, newestBuffer_.item->buffer->GetSeqNum(), newestBuffer_.pts, newestBuffer_.repeatTimes);
        return;
    }
    if (avaliableBuffers_.size() >= MAX_LIST_SIZE) {
        HLOGW("stop repeat, list size to big: %zu", avaliableBuffers_.size());
        return;
    }
    int64_t newPts = newestBuffer_.pts + static_cast<int64_t>(repeatUs_);
    HLOGD("generation = %" PRIu64 ", seq = %u, pts %" PRId64 " -> %" PRId64,
          generation, newestBuffer_.item->buffer->GetSeqNum(), newestBuffer_.pts, newPts);
    newestBuffer_.pts = newPts;
    newestBuffer_.repeatTimes++;
    avaliableBuffers_.push_back(newestBuffer_);
    SendRepeatMsg(generation);
    TraverseAvaliableBuffers();
}

void HEncoder::TraverseAvaliableBuffers()
{
    while (!avaliableBuffers_.empty()) {
        auto it = find_if(inputBufferPool_.begin(), inputBufferPool_.end(),
                          [](const BufferInfo &info) { return info.owner == BufferOwner::OWNED_BY_SURFACE; });
        if (it == inputBufferPool_.end()) {
            HLOGD("buffer cnt = %zu, but no avaliable slot", avaliableBuffers_.size());
            return;
        }
        InSurfaceBufferEntry entry = avaliableBuffers_.front();
        avaliableBuffers_.pop_front();
        SubmitOneBuffer(entry, *it);
    }
}

void HEncoder::TraverseAvaliableSlots()
{
    for (BufferInfo& info : inputBufferPool_) {
        if (info.owner != BufferOwner::OWNED_BY_SURFACE) {
            continue;
        }
        if (avaliableBuffers_.empty() && !GetOneBufferFromSurface()) {
            HLOGD("slot %u is avaliable, but no buffer", info.bufferId);
            return;
        }
        InSurfaceBufferEntry entry = avaliableBuffers_.front();
        avaliableBuffers_.pop_front();
        SubmitOneBuffer(entry, info);
    }
}

void HEncoder::SubmitOneBuffer(InSurfaceBufferEntry& entry, BufferInfo &info)
{
    if (entry.item == nullptr) {
        ChangeOwner(info, BufferOwner::OWNED_BY_US);
        HLOGI("got input eos");
        inputPortEos_ = true;
        info.omxBuffer->flag = OMX_BUFFERFLAG_EOS;
        info.surfaceBuffer = nullptr;
        NotifyOmxToEmptyThisInBuffer(info);
        return;
    }
    if (!WaitFence(entry.item->fence)) {
        return;
    }
    ChangeOwner(info, BufferOwner::OWNED_BY_US);
    WrapSurfaceBufferToSlot(info, entry.item->buffer, entry.pts, 0);
    encodingBuffers_[info.bufferId] = entry;
    if (enableSurfaceModeInputCb_) {
        info.avBuffer->pts_ = entry.pts;
        NotifyUserToFillThisInBuffer(info);
    } else {
        int32_t err = NotifyOmxToEmptyThisInBuffer(info);
        if (err != AVCS_ERR_OK) {
            ResetSlot(info);
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
    if (inputSurface_) {
        ResetSlot(*info);
        if (mode == RESUBMIT_BUFFER && !inputPortEos_) {
            TraverseAvaliableSlots();
        }
    } else {
        ChangeOwner(*info, BufferOwner::OWNED_BY_US);
        if (mode == RESUBMIT_BUFFER && !inputPortEos_) {
            NotifyUserToFillThisInBuffer(*info);
        }
    }
}

void HEncoder::ResetSlot(BufferInfo& info)
{
    ChangeOwner(info, BufferOwner::OWNED_BY_SURFACE);
    encodingBuffers_.erase(info.bufferId);
    info.surfaceBuffer = nullptr;
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
    TraverseAvaliableBuffers();
}

void HEncoder::OnEnterUninitializedState()
{
    if (inputSurface_) {
        inputSurface_->UnregisterConsumerListener();
    }
    avaliableBuffers_.clear();
    newestBuffer_.item.reset();
    encodingBuffers_.clear();
}

HEncoder::BufferItem::~BufferItem()
{
    if (surface && buffer) {
        LOGD("release seq = %u", buffer->GetSeqNum());
        surface->ReleaseBuffer(buffer, -1);
    }
}

} // namespace OHOS::MediaAVCodec
