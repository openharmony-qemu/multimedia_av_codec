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
#include "hdf_base.h"
#include "OMX_VideoExt.h"
#include "media_description.h"  // foundation/multimedia/av_codec/interfaces/inner_api/native/
#include "type_converter.h"
#include "hcodec_log.h"
#include "hcodec_dfx.h"
#include "hcodec_list.h"
#include "v4_0/codec_ext_types.h"
#include <algorithm>
#include <regex>

namespace OHOS::MediaAVCodec {
using namespace std;

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

    EnableVariableFrameRate(format);
    optional<double> frameRate = GetFrameRateFromUser(format);
    defaultFrameRate_ = frameRate;
    ret = ConfigBEncodeMode(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = SetupPort(format, frameRate);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ConfigureProtocol(format, frameRate);

    (void)ConfigureOutputBitrate(format);
    (void)SetColorAspects(format);
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
    ret = EnableFrameQPMap(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    (void)EnableEncoderParamsFeedback(format);
    return AVCS_ERR_OK;
}

void HEncoder::EnableVariableFrameRate(const Format &format)
{
    int32_t enableVariableFrameRate = 0;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_PTS_BASED_RATECONTROL, enableVariableFrameRate)) {
        HLOGE("fail to get video_encoder_enable_pts_based_ratecontrol");
        return;
    }
    HLOGI("video_encoder_enable_pts_based_ratecontrol flag is %d", enableVariableFrameRate);
    if (enableVariableFrameRate) {
        enableVariableFrameRate_ = true;
    }
    return;
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
    CodecHDI::VideoFeature ltrFeature =
        HCodecList::FindFeature(caps_.port.video.features, CodecHDI::VIDEO_FEATURE_LTR);
    if (!ltrFeature.support || ltrFeature.extendInfo.empty()) {
        HLOGW("platform not support LTR");
        return AVCS_ERR_OK;
    }
    int32_t maxLtrNum = ltrFeature.extendInfo[0];
    if (ltrFrameNum <= 0 || ltrFrameNum > maxLtrNum) {
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

// LCOV_EXCL_START
int32_t HEncoder::EnableFrameQPMap(const Format &format)
{
    int32_t enableQPMap = false;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_QP_MAP, enableQPMap)) {
        return AVCS_ERR_OK;
    }
    CodecHDI::VideoFeature feature =
        HCodecList::FindFeature(caps_.port.video.features, CodecHDI::VIDEO_FEATURE_QP_MAP);
    if (!feature.support) {
        HLOGE("this device dont support qp map");
        return AVCS_ERR_UNSUPPORT;
    }
    OMX_CONFIG_BOOLEANTYPE param {};
    InitOMXParam(param);
    param.bEnabled = enableQPMap ? OMX_TRUE : OMX_FALSE;
    if (!SetParameter(OMX_IndexParamEnableQPMap, param)) {
        HLOGE("enable encoder frame qp map[%d] failed", enableQPMap);
        return AVCS_ERR_INVALID_VAL;
    }
    HLOGI("enable encoder frame qp map[%d] success", enableQPMap);
    enableQPMap_ = true;
    return AVCS_ERR_OK;
}

int32_t HEncoder::ConfigBEncodeMode(const Format &format)
{
    Media::Plugins::VideoEncodeBFrameGopMode gopMode;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE, *reinterpret_cast<int *>(&gopMode))) {
        return AVCS_ERR_OK;
    }

    CodecHDI::VideoFeature feature =
        HCodecList::FindFeature(caps_.port.video.features, CodecHDI::VIDEO_FEATURE_ENCODE_B_FRAME);
    if (!feature.support || feature.extendInfo.empty() || feature.extendInfo[0] <= 0) {
        HLOGE("this device or protocol not support b frame");
        return AVCS_ERR_UNSUPPORT;
    }

    CodecEncGopMode param {};
    InitOMXParamExt(param);
    if (gopMode == Media::Plugins::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE) {
        HLOGI("encoder use adaptive-b");
        param.gopMode = OMX_ENCODE_GOP_ADAPTIVE_B_MODE;
    } else if (gopMode == Media::Plugins::VIDEO_ENCODE_GOP_H3B_MODE) {
        HLOGI("encoder use h3b");
        param.gopMode = OMX_ENCODE_GOP_H3B_MODE;
    } else {
        HLOGE("invalid gop mode");
        return AVCS_ERR_UNSUPPORT;
    }

    if (!SetParameter(OMX_IndexParamEncBFrameMode, param)) {
        HLOGE("config b gop mode [%d] failed", gopMode);
        return AVCS_ERR_INVALID_VAL;
    }
    return AVCS_ERR_OK;
}
// LCOV_EXCL_STOP

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
    if (!HCodecList::FindFeature(caps_.port.video.features, CodecHDI::VIDEO_FEATURE_TSVC).support) {
        HLOGW("platform not support temporal scale");
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

int32_t HEncoder::GetWaterMarkInfo(std::shared_ptr<AVBuffer> buffer, WaterMarkInfo &info)
{
    if (!buffer->meta_->GetData(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_WATERMARK, info.enableWaterMark) ||
        !buffer->meta_->GetData(OHOS::Media::Tag::VIDEO_COORDINATE_X, info.x) ||
        !buffer->meta_->GetData(OHOS::Media::Tag::VIDEO_COORDINATE_Y, info.y) ||
        !buffer->meta_->GetData(OHOS::Media::Tag::VIDEO_COORDINATE_W, info.w) ||
        !buffer->meta_->GetData(OHOS::Media::Tag::VIDEO_COORDINATE_H, info.h)) {
        LOGE("invalid value");
        return AVCS_ERR_INVALID_VAL;
    }
    if (info.x < 0 || info.y < 0 || info.w <= 0 || info.h <= 0) {
        LOGE("invalid coordinate, x %d, y %d, w %d, h %d", info.x, info.y, info.w, info.h);
        return AVCS_ERR_INVALID_VAL;
    }
    return AVCS_ERR_OK;
}

int32_t HEncoder::OnConfigureBuffer(std::shared_ptr<AVBuffer> buffer)
{
    if (!HCodecList::FindFeature(caps_.port.video.features, CodecHDI::VIDEO_FEATURE_WATERMARK).support) {
        HLOGE("this device dont support water mark");
        return AVCS_ERR_UNSUPPORT;
    }
    if (buffer == nullptr || buffer->memory_ == nullptr || buffer->meta_ == nullptr) {
        HLOGE("invalid buffer");
        return AVCS_ERR_INVALID_VAL;
    }
    sptr<SurfaceBuffer> waterMarkBuffer = buffer->memory_->GetSurfaceBuffer();
    if (waterMarkBuffer == nullptr) {
        HLOGE("null surfacebuffer");
        return AVCS_ERR_INVALID_VAL;
    }
    if (waterMarkBuffer->GetFormat() != GRAPHIC_PIXEL_FMT_RGBA_8888) {
        HLOGE("pixel fmt should be RGBA8888");
        return AVCS_ERR_INVALID_VAL;
    }
    WaterMarkInfo info;
    int32_t ret = GetWaterMarkInfo(buffer, info);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    CodecHDI::CodecParamOverlay param {
        .size = sizeof(param), .enable = info.enableWaterMark,
        .dstX = static_cast<uint32_t>(info.x), .dstY = static_cast<uint32_t>(info.y),
        .dstW = static_cast<uint32_t>(info.w), .dstH = static_cast<uint32_t>(info.h),
    };
    int8_t* p = reinterpret_cast<int8_t*>(&param);
    std::vector<int8_t> inVec(p, p + sizeof(param));
    CodecHDI::OmxCodecBuffer omxbuffer {};
    omxbuffer.bufferhandle = new HDI::Base::NativeBuffer(waterMarkBuffer->GetBufferHandle());
    ret = compNode_->SetParameterWithBuffer(CodecHDI::Codec_IndexParamOverlayBuffer, inVec, omxbuffer);
    if (ret != HDF_SUCCESS) {
        HLOGE("SetParameterWithBuffer failed");
        return AVCS_ERR_INVALID_VAL;
    }
    HLOGI("SetParameterWithBuffer succ");
    return AVCS_ERR_OK;
}

int32_t HEncoder::SetColorAspects(const Format &format)
{
    int range = 0;
    int primary = static_cast<int>(COLOR_PRIMARY_UNSPECIFIED);
    int transfer = static_cast<int>(TRANSFER_CHARACTERISTIC_UNSPECIFIED);
    int matrix = static_cast<int>(MATRIX_COEFFICIENT_UNSPECIFIED);

    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, range)) {
        HLOGI("user set range flag %d", range);
    }
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, primary)) {
        HLOGI("user set primary %d", primary);
    }
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, transfer)) {
        HLOGI("user set transfer %d", transfer);
    }
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, matrix)) {
        HLOGI("user set matrix %d", matrix);
    }
    if (primary < 0 || primary > UINT8_MAX ||
        transfer < 0 || transfer > UINT8_MAX ||
        matrix < 0 || matrix > UINT8_MAX) {
        HLOGW("invalid color");
        return AVCS_ERR_INVALID_VAL;
    }

    CodecVideoColorspace param;
    InitOMXParamExt(param);
    param.portIndex = OMX_DirInput;
    param.aspects.range = static_cast<bool>(range);
    param.aspects.primaries = static_cast<uint8_t>(primary);
    param.aspects.transfer = static_cast<uint8_t>(transfer);
    param.aspects.matrixCoeffs = static_cast<uint8_t>(matrix);

    if (!SetParameter(OMX_IndexColorAspects, param, true)) {
        HLOGE("failed to set CodecVideoColorSpace");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set color aspects (isFullRange %d, primary %u, transfer %u, matrix %u) succ",
          param.aspects.range, param.aspects.primaries,
          param.aspects.transfer, param.aspects.matrixCoeffs);
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

int32_t HEncoder::SetupPort(const Format &format, std::optional<double> &frameRate)
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
    width_ = static_cast<uint32_t>(width);
    height_ = static_cast<uint32_t>(height);
    HLOGI("user set width %d, height %d", width, height);
    if (!GetPixelFmtFromUser(format)) {
        return AVCS_ERR_INVALID_VAL;
    }

    if (!frameRate.has_value()) {
        HLOGI("user don't set valid frame rate, use default 60.0");
        frameRate = 60.0; // default frame rate 60.0
    }

    codecRate_ = frameRate.value();
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
    inputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT, configuredFmt_.graphicFmt);
    inputFormat_->PutIntValue("IS_VENDOR", 1);
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
    outputFormat_->PutIntValue("IS_VENDOR", 1);
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

std::optional<uint32_t> HEncoder::GetSQRFactorFromUser(const Format &format)
{
    int32_t sqrFactor;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactor) && sqrFactor >= 0) {
        LOGI("user set SQR factor is  %d", sqrFactor);
        return static_cast<uint32_t>(sqrFactor);
    }
    return nullopt;
}

std::optional<uint32_t> HEncoder::GetSQRMaxBitrateFromUser(const Format &format)
{
    int64_t maxBitRateLong;
    if (format.GetLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitRateLong) &&
        maxBitRateLong > 0 && maxBitRateLong <= UINT32_MAX) {
        LOGI("user set max bit rate %" PRId64 "", maxBitRateLong);
        return static_cast<uint32_t>(maxBitRateLong);
    }
    int32_t maxBitRateInt;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitRateInt) && maxBitRateInt > 0) {
        LOGI("user set max bit rate %d", maxBitRateInt);
        return static_cast<uint32_t>(maxBitRateInt);
    }
    return nullopt;
}

std::optional<uint32_t> HEncoder::GetCRFtagetQpFromUser(const Format &format)
{
    int32_t targetQp;
    if (format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TARGET_QP, targetQp) && targetQp > 0) {
        LOGI("user set CRF target_qp %d", targetQp);
        return static_cast<uint32_t>(targetQp);
    }
    return nullopt;
}

std::optional<VideoEncodeBitrateMode> HEncoder::GetBitRateModeFromUser(const Format &format)
{
    VideoEncodeBitrateMode mode;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, *reinterpret_cast<int *>(&mode))) {
        return mode;
    }
    return nullopt;
}

// LCOV_EXCL_START
int32_t HEncoder::SetCRFMode(int32_t targetQp)
{
    ControlQualityTargetQp bitrateType;
    InitOMXParamExt(bitrateType);
    bitrateType.portIndex = OMX_DirOutput;
    bitrateType.targetQp = static_cast<uint32_t>(targetQp);
    if (!SetParameter(OMX_IndexParamControlRateCRF, bitrateType)) {
        HLOGE("failed to set OMX_IndexParamControlRateCRF");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set CRF mode and target quality %u succ", bitrateType.targetQp);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CRF);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TARGET_QP, targetQp);
    return AVCS_ERR_OK;
}
// LCOV_EXCL_STOP

int32_t HEncoder::SetConstantQualityMode(int32_t quality)
{
    ControlRateConstantQuality bitrateType;
    InitOMXParamExt(bitrateType);
    bitrateType.portIndex = OMX_DirOutput;
    bitrateType.qualityValue = static_cast<uint32_t>(quality);
    if (!SetParameter(OMX_IndexParamControlRateConstantQuality, bitrateType)) {
        HLOGE("failed to set OMX_IndexParamControlRateConstantQuality");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set CQ mode and target quality %u succ", bitrateType.qualityValue);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, quality);
    return AVCS_ERR_OK;
}

int32_t HEncoder::SetSQRMode(const Format &format)
{
    StableControlRate bitrateType;
    InitOMXParamExt(bitrateType);
    bitrateType.portIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexParamControlRateSQR, bitrateType)) {
        HLOGE("get OMX_IndexParamControlRateSQR failed");
        return AVCS_ERR_UNKNOWN;
    }
    optional<uint32_t> sqrFactor = GetSQRFactorFromUser(format);
    optional<uint32_t> maxBitrate = GetSQRMaxBitrateFromUser(format);
    optional<uint32_t> bitRate = GetBitRateFromUser(format);
    if (sqrFactor.has_value()) {
        bitrateType.sqrFactor = sqrFactor.value();
        LOGI("set sqr factor %u", bitrateType.sqrFactor);
    }
    if (maxBitrate.has_value()) {
        bitrateType.sMaxBitrate = maxBitrate.value();
        LOGI("set max bit rate %u bps", bitrateType.sMaxBitrate);
    }
    if (bitRate.has_value() && !maxBitrate.has_value()) {
        bitrateType.sTargetBitrate = bitRate.value();
        bitrateType.bitrateEnabled = true;
        LOGI("set target bitrate %u bps", bitrateType.sTargetBitrate);
    }
    if (!SetParameter(OMX_IndexParamControlRateSQR, bitrateType)) {
        HLOGE("failed to set OMX_IndexParamControlRateSQR");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set SQR mode succ");
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, bitrateType.sqrFactor);
    if (bitrateType.bitrateEnabled) {
        outputFormat_->PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE,
            static_cast<int64_t>(bitrateType.sTargetBitrate));
    } else {
        outputFormat_->PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            static_cast<int64_t>(bitrateType.sMaxBitrate));
    }
    return AVCS_ERR_OK;
}

int32_t HEncoder::ConfigureOutputBitrate(const Format &format)
{
    OMX_VIDEO_PARAM_BITRATETYPE bitrateType;
    InitOMXParam(bitrateType);
    bitrateType.nPortIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexParamVideoBitrate, bitrateType)) {
        HLOGE("get OMX_IndexParamVideoBitrate failed");
        return AVCS_ERR_UNKNOWN;
    }
    optional<VideoEncodeBitrateMode> bitRateMode = GetBitRateModeFromUser(format);
    if (bitRateMode.has_value()) {
        int32_t metaValue;
        if (bitRateMode.value() == CRF &&
            format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TARGET_QP, metaValue) && metaValue >= 0) {
            return SetCRFMode(metaValue);
        }
        if (format.GetIntValue(MediaDescriptionKey::MD_KEY_QUALITY, metaValue)) {
            if (bitRateMode.value() == CQ && metaValue >= 0) {
                return SetConstantQualityMode(metaValue);
            }
        } else {
            if (bitRateMode.value() == SQR) {
                return SetSQRMode(format);
            }
        }
        if (bitRateMode.value() != SQR && bitRateMode.value() != CQ) {
            auto omxBitrateMode = TypeConverter::InnerModeToOmxBitrateMode(bitRateMode.value());
            if (omxBitrateMode.has_value()) {
                bitrateType.eControlRate = omxBitrateMode.value();
            }
        }
    }
    optional<uint32_t> bitRate = GetBitRateFromUser(format);
    if (bitRate.has_value()) {
        bitrateType.nTargetBitrate = bitRate.value();
    }
    if (!SetParameter(OMX_IndexParamVideoBitrate, bitrateType)) {
        HLOGE("failed to set OMX_IndexParamVideoBitrate");
        return AVCS_ERR_UNKNOWN;
    }
    outputFormat_->PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE,
        static_cast<int64_t>(bitrateType.nTargetBitrate));
    auto innerMode = TypeConverter::OmxBitrateModeToInnerMode(bitrateType.eControlRate);
    if (innerMode.has_value()) {
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
            static_cast<int32_t>(innerMode.value()));
        HLOGI("set %d mode and target bitrate %u bps succ", innerMode.value(), bitrateType.nTargetBitrate);
    } else {
        HLOGI("set default bitratemode and target bitrate %u bps succ", bitrateType.nTargetBitrate);
    }
    return AVCS_ERR_OK;
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
        inputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, profile);
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

void HEncoder::SetSqrParam(const Format &format)
{
    StableControlRate bitrateType;
    InitOMXParamExt(bitrateType);
    bitrateType.portIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexParamControlRateSQR, bitrateType)) {
        HLOGE("get OMX_IndexParamControlRateSQR failed");
        return;
    }
    optional<uint32_t> sqrFactor = GetSQRFactorFromUser(format);
    optional<uint32_t> maxBitrate = GetSQRMaxBitrateFromUser(format);
    optional<uint32_t> bitRate = GetBitRateFromUser(format);
    if (sqrFactor.has_value()) {
        bitrateType.sqrFactor = sqrFactor.value();
        LOGI("set dynamic sqr factor %u", bitrateType.sqrFactor);
    }
    if (maxBitrate.has_value() && !bitrateType.bitrateEnabled) {
        bitrateType.sMaxBitrate = maxBitrate.value();
        LOGI("set dynamic max bitrate %u bps", bitrateType.sMaxBitrate);
    }
    if (bitRate.has_value() && bitrateType.bitrateEnabled) {
        bitrateType.sTargetBitrate = bitRate.value();
        LOGI("set dynamic target bitrate %u bps", bitrateType.sTargetBitrate);
    }
    if (!SetParameter(OMX_IndexParamControlRateSQR, bitrateType, true)) {
        HLOGW("failed to set OMX_IndexParamControlRateSQR");
    }
}

int32_t HEncoder::OnSetParameters(const Format &format)
{
    optional<VideoEncodeBitrateMode> bitRateMode = GetBitRateModeFromUser(*outputFormat_);
    if (bitRateMode.has_value() && bitRateMode.value() == SQR) {
        SetSqrParam(format);
    } else {
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
    }
// LCOV_EXCL_START
    optional<uint32_t> targetQp = GetCRFtagetQpFromUser(format);
    if (targetQp.has_value() && bitRateMode.has_value() && bitRateMode.value() == CRF) {
        ControlQualityTargetQp bitrateType;
        InitOMXParamExt(bitrateType);
        bitrateType.portIndex = OMX_DirOutput;
        bitrateType.targetQp = targetQp.value();
        if (!SetParameter(OMX_IndexParamControlRateCRF, bitrateType, true)) {
            HLOGW("failed to config OMX_IndexConfigVideoBitrate");
        }
    }
// LCOV_EXCL_STOP
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

    optional<double> operatingRate = GetOperatingRateFromUser(format);
    if (operatingRate.has_value()) {
        OMX_PARAM_U32TYPE config;
        InitOMXParam(config);
        config.nU32 = static_cast<uint32_t>(operatingRate.value());
        if (!SetParameter(OMX_IndexParamOperatingRate, config, true)) {
            HLOGW("failed to set OMX_IndexParamOperatingRate");
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

int32_t HEncoder::SubmitOutBufToOmx()
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
        HLOGI("surface mode, surface id = %" PRIu64, inputSurface_->GetUniqueId());
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
            UpdateFmtFromSurfaceBuffer();
        }
        return ret;
    }
}

void HEncoder::UpdateFmtFromSurfaceBuffer()
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
    int32_t ret = SubmitOutBufToOmx();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    if (inputSurface_) {
        ClearDirtyList();
        sptr<IBufferConsumerListener> listener = new EncoderBuffersConsumerListener(m_token);
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
    HLOGI("succ, surface id = %" PRIu64 ", queue size %u",
          inputSurface_->GetUniqueId(), inputSurface_->GetQueueSize());
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

void HEncoder::WrapPerFrameParamIntoOmxBuffer(shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                              const shared_ptr<Media::Meta> &meta)
{
    omxBuffer->alongParam.clear();
    WrapLTRParamIntoOmxBuffer(omxBuffer, meta);
    WrapRequestIFrameParamIntoOmxBuffer(omxBuffer, meta);
    WrapQPRangeParamIntoOmxBuffer(omxBuffer, meta);
    WrapStartQPIntoOmxBuffer(omxBuffer, meta);
    WrapIsSkipFrameIntoOmxBuffer(omxBuffer, meta);
    WrapRoiParamIntoOmxBuffer(omxBuffer, meta);
    WrapQPMapParamIntoOmxBuffer(omxBuffer, meta);
    meta->Clear();
}

void HEncoder::WrapLTRParamIntoOmxBuffer(shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
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

// LCOV_EXCL_START
void HEncoder::WrapQPMapParamIntoOmxBuffer(shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                           const shared_ptr<Media::Meta> &meta)
{
    if (!enableQPMap_) {
        return;
    }
    bool isAbsQp;
    if (!meta->GetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_ABS_QP_MAP, isAbsQp)) {
        isAbsQp = false;
    }
    vector<uint8_t> qpMap;
    if (!meta->GetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_QP_MAP, qpMap) || qpMap.empty()) {
        return;
    }
    AppendToVector(omxBuffer->alongParam, OMX_IndexParamBlockQP);
    CodecBlockQpParam param;
    InitOMXParamExt(param);
    param.blockQpAddr = nullptr;
    param.blockQpSize = static_cast<uint32_t>(qpMap.size());
    param.blockQpSetByUser = true;
    param.absQp = isAbsQp;
    AppendToVector(omxBuffer->alongParam, param);
    AppendArrayToVector(omxBuffer->alongParam, qpMap);
}
// LCOV_EXCL_STOP

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

double HEncoder::CalculateSmoothFactorBasedPts(int64_t curPts, int64_t curDuration)
{
    double smoothFactor = 0.0;
    int32_t instantFrameRate = round((1.0 / curDuration) * TIME_RATIO_US_TO_S);
    if (instantFrameRate > SMOOTH_FACTOR_CLIP_RANGE_MIN && instantFrameRate < SMOOTH_FACTOR_CLIP_RANGE_MAX) {
        smoothFactor = 1 - DURATION_SCALE_FACTOR / instantFrameRate;
    } else if (instantFrameRate <= SMOOTH_FACTOR_CLIP_RANGE_MIN) {
        smoothFactor = SMOOTH_FACTOR_CLIP_MIN;
    } else {
        smoothFactor = SMOOTH_FACTOR_CLIP_MAX;
    }
    return smoothFactor;
}

int32_t HEncoder::CalculateSmoothFpsBasedPts(int64_t curPts, int64_t curDuration)
{
    int64_t previousDuration1st = previousPtsWindow_.back() - previousPtsWindow_[2];
    int64_t previousDuration2nd = previousPtsWindow_[2] - previousPtsWindow_[1];
    double smoothFactor = CalculateSmoothFactorBasedPts(curPts, curDuration);
    double previousSmoothFactor = CalculateSmoothFactorBasedPts(previousPtsWindow_.back(), previousDuration1st);
    double smoothDuration = curDuration * (1 - smoothFactor) + (previousDuration1st * (1 - previousSmoothFactor) +
        previousDuration2nd * previousSmoothFactor) * smoothFactor;
    int32_t smoothFrameRate = round((1.0 / smoothDuration) * TIME_RATIO_US_TO_S);
    HLOGD("pts: %ld, smoothFactor: %f, previousSmoothFactor: %f, smoothDuration: %f", curPts, smoothFactor,
        previousSmoothFactor, smoothDuration);
    return smoothFrameRate;
}

int32_t HEncoder::UpdateTimeStampWindow(int64_t curPts, int32_t &frameRate)
{
    int32_t previousPtsWindowSize = static_cast<int32_t>(previousPtsWindow_.size());
    if (previousPtsWindow_.empty()) {
        previousPtsWindow_.push_back(curPts);
        frameRate = defaultFrameRate_.value_or(DEFAULT_FRAME_RATE);
        HLOGD("pts: %ld, smoothFrameRate: %d, windowSize: %d", curPts, frameRate, previousPtsWindowSize);
        return 0;
    }
    if (curPts <= previousPtsWindow_.back()) {
        HLOGE("curPts less than last pts !");
        return -1;
    }
    int64_t curDuration = curPts - previousPtsWindow_.back();
    if (previousPtsWindowSize < PREVIOUS_PTS_RECORDED_COUNT) {
        frameRate = round((1.0 / curDuration) * TIME_RATIO_US_TO_S);
        previousPtsWindow_.push_back(curPts);
        previousSmoothFrameRate_ = frameRate;
    } else if (previousPtsWindowSize == PREVIOUS_PTS_RECORDED_COUNT) {
        int32_t instantFrameRate = round((1.0 / curDuration) * TIME_RATIO_US_TO_S);
        if (instantFrameRate == 0) {
            HLOGE("instantFrameRate is 0 !");
            return -1;
        }
        int32_t smoothFrameRate = CalculateSmoothFpsBasedPts(curPts, curDuration);
        double averageThreeFrameDurationError = (curDuration - (previousPtsWindow_[1] - previousPtsWindow_[0])) /
            AVERAGE_DURATION_ERROR_COUNT;
        double frameDurationErrorMin = (1.0 / instantFrameRate - 1.0 / (instantFrameRate -
            AVERAGE_DURATION_ERROR_FRAMERATE_RANGE)) * TIME_RATIO_US_TO_S;
        double frameDurationErrorMax = (1.0 / instantFrameRate - 1.0 / (instantFrameRate +
            AVERAGE_DURATION_ERROR_FRAMERATE_RANGE)) * TIME_RATIO_US_TO_S;
        if (averageThreeFrameDurationError > frameDurationErrorMin &&
            averageThreeFrameDurationError < frameDurationErrorMax) {
            frameRate = smoothFrameRate;
        } else {
            frameRate = previousSmoothFrameRate_;
        }
        previousPtsWindow_.push_back(curPts);
        previousPtsWindow_.pop_front();
        previousSmoothFrameRate_ = frameRate;
    } else {
        HLOGE("previousPtsWindow_ size:%d error !", previousPtsWindowSize);
        return -1;
    }
    HLOGD("pts: %ld, smoothFrameRate: %d, windowSize: %d", curPts, frameRate, previousPtsWindowSize);
    return 0;
}
int32_t HEncoder::CalculateFrameRateParamIntoOmxBuffer(int64_t curPts)
{
    if (curPts <= 0) {
        HLOGE("curPts less than 0 !");
        return -1;
    }
    int32_t frameRate = 0;
    if (inputSurface_) {
        curPts /= TIME_RATIO_NS_TO_US;
    }
    if (UpdateTimeStampWindow(curPts, frameRate) != 0) {
        return -1;
    }
    if (frameRate < caps_.port.video.frameRate.min || frameRate > caps_.port.video.frameRate.max) {
        HLOGE("frameRate: %d over range, min: %d, max: %d", frameRate, caps_.port.video.frameRate.min,
            caps_.port.video.frameRate.max);
        return -1;
    }
    OMX_CONFIG_FRAMERATETYPE framerateCfgType;
    InitOMXParam(framerateCfgType);
    framerateCfgType.nPortIndex = OMX_DirInput;
    framerateCfgType.xEncodeFramerate = frameRate * FRAME_RATE_COEFFICIENT;
    if (!SetParameter(OMX_IndexConfigVideoFramerate, framerateCfgType, true)) {
        HLOGE("failed to config OMX_IndexConfigVideoFramerate");
        return -1;
    }
    return 0;
}

void HEncoder::ParseRoiStringValid(const std::string &roiValue, shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer)
{
    AppendToVector(omxBuffer->alongParam, OMX_IndexParamRoi);
    CodecRoiParam param;
    InitOMXParamExt(param);
    if (roiValue.empty()) { // roiValue string is empty, canceling historical roi configurations!
        AppendToVector(omxBuffer->alongParam, param);
        return;
    }
    // step1 parse the string
    // method regex roi: Top1,Left1-Bottom1,Right1=Offset1(option);Top2,Left2-Bottom2,Right2=Offset2;...
    std::regex pattern(R"((-?\d+),(-?\d+)-(-?\d+),(-?\d+)(?:=(-?\d+))?(?:;|$))");
    std::smatch match;
    std::string temp = roiValue;
    size_t vaildCount = 0;
    constexpr int TOP_INDEX = 1;
    constexpr int LEFT_INDEX = 2;
    constexpr int BOTTOM_INDEX = 3;
    constexpr int RIGHT_INDEX = 4;
    constexpr int QPOFFSET_INDEX = 5;
    constexpr int defaultOffset = -3; // default roi qp
    while (std::regex_search(temp, match, pattern) && vaildCount < roiNum) {
        int32_t qpOffset, left, top, right, bottom;
        top = std::clamp(static_cast<int>(std::strtol(match[TOP_INDEX].str().c_str(), nullptr, 10)), //10:Decimal
            0, static_cast<int>(height_));
        left = std::clamp(static_cast<int>(std::strtol(match[LEFT_INDEX].str().c_str(), nullptr, 10)), //10:Decimal
            0, static_cast<int>(width_));
        bottom = std::clamp(static_cast<int>(std::strtol(match[BOTTOM_INDEX].str().c_str(), nullptr, 10)), //10:Decimal
            0, static_cast<int>(height_));
        right = std::clamp(static_cast<int>(std::strtol(match[RIGHT_INDEX].str().c_str(), nullptr, 10)), //10:Decimal
            0, static_cast<int>(width_));
        if (match[QPOFFSET_INDEX].matched) {
            qpOffset = static_cast<int>(std::strtol(match[QPOFFSET_INDEX].str().c_str(), nullptr, 10)); //10:Decimal
        } else {
            qpOffset = defaultOffset;
        }
        temp = match.suffix().str();
        int32_t roiWidth = right - left;
        int32_t roiHeight = bottom - top;
        param.roiInfo[vaildCount].regionEnable = true;
        param.roiInfo[vaildCount].absQp = 0;
        param.roiInfo[vaildCount].roiQp = static_cast<int32_t>(qpOffset);
        param.roiInfo[vaildCount].roiStartX = static_cast<uint32_t>(left);
        param.roiInfo[vaildCount].roiStartY = static_cast<uint32_t>(top);
        param.roiInfo[vaildCount].roiWidth = static_cast<int32_t>(roiWidth);
        param.roiInfo[vaildCount].roiHeight = static_cast<int32_t>(roiHeight);
        vaildCount++;
    }
    AppendToVector(omxBuffer->alongParam, param);
}

void HEncoder::WrapRoiParamIntoOmxBuffer(shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                         const shared_ptr<Media::Meta> &meta)
{
    std::string roiValue;
    if (!meta->GetData(OHOS::Media::Tag::VIDEO_ENCODER_ROI_PARAMS, roiValue)) { // historical configuration
        return;
    }
    ParseRoiStringValid(roiValue, omxBuffer);
}

void HEncoder::DealWithResolutionChange(uint32_t newWidth, uint32_t newHeight)
{
    if (width_ != newWidth || height_ != newHeight) {
        HLOGI("resolution changed, %ux%u -> %ux%u", width_, height_, newWidth, newHeight);
        width_ = newWidth;
        height_ = newHeight;
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width_);
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height_);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, width_);
        outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, height_);
        HLOGI("output format changed: %s", outputFormat_->Stringify().c_str());
        callback_->OnOutputFormatChanged(*(outputFormat_.get()));
    }
}

void HEncoder::BeforeCbOutToUser(BufferInfo &info)
{
    std::shared_ptr<Media::Meta> meta = info.avBuffer->meta_;
    meta->Clear();
    BinaryReader reader(static_cast<uint8_t*>(info.omxBuffer->alongParam.data()), info.omxBuffer->alongParam.size());
    int* index = nullptr;
    while ((index = reader.Read<int>()) != nullptr) {
        switch (*index) {
            case OMX_IndexConfigCommonOutputSize: {
                auto *param = reader.Read<OMX_FRAMESIZETYPE>();
                IF_TRUE_RETURN_VOID(param == nullptr);
                DealWithResolutionChange(param->nWidth, param->nHeight);
                break;
            }
            case OMX_IndexParamEncOutQp:
                ExtractPerFrameAveQpParam(reader, meta);
                break;
            case OMX_IndexParamEncOutMse:
                ExtractPerFrameMSEParam(reader, meta);
                break;
            case OMX_IndexParamEncOutLTR:
                ExtractPerFrameLTRParam(reader, meta);
                break;
            case OMX_IndexParamEncOutFrameLayer:
                ExtractPerFrameLayerParam(reader, meta);
                break;
            case OMX_IndexParamEncOutRealBitrate:
                ExtractPerFrameRealBitrateParam(reader, meta);
                break;
            case OMX_IndexParamEncOutFrameQp:
                ExtractPerFrameFrameQpParam(reader, meta);
                break;
            case OMX_IndexParamEncOutMad:
                ExtractPerFrameMadParam(reader, meta);
                break;
            case OMX_IndexParamEncOutIRatio:
                ExtractPerFrameIRitioParam(reader, meta);
                break;
            default:
                break;
        }
    }
    info.omxBuffer->alongParam.clear();
    if (debugMode_) {
        auto metaStr = StringifyMeta(meta);
        HLOGI("%s", metaStr.c_str());
    }
}

void HEncoder::ExtractPerFrameLTRParam(BinaryReader &reader, shared_ptr<Media::Meta> &meta)
{
    auto *ltrParam = reader.Read<CodecEncOutLTRParam>();
    IF_TRUE_RETURN_VOID(ltrParam == nullptr);
    meta->SetData(OHOS::Media::Tag::VIDEO_PER_FRAME_IS_LTR, ltrParam->isLTR);
    meta->SetData(OHOS::Media::Tag::VIDEO_PER_FRAME_POC, static_cast<int32_t>(ltrParam->poc));
}

void HEncoder::ExtractPerFrameMadParam(BinaryReader &reader, shared_ptr<Media::Meta> &meta)
{
    auto *madParam = reader.Read<CodecEncOutMadParam>();
    IF_TRUE_RETURN_VOID(madParam == nullptr);
    meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_FRAME_MADI, madParam->frameMadi);
    meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_FRAME_MADP, madParam->frameMadp);
    meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_SUM_MADI, madParam->sumMadi);
}

void HEncoder::ExtractPerFrameRealBitrateParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta)
{
    auto *realBitrate = reader.Read<OMX_S32>();
    IF_TRUE_RETURN_VOID(realBitrate == nullptr);
    meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_REAL_BITRATE, *realBitrate);
}

void HEncoder::ExtractPerFrameFrameQpParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta)
{
    auto *frameQp = reader.Read<OMX_S32>();
    IF_TRUE_RETURN_VOID(frameQp == nullptr);
    meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_FRAME_QP, *frameQp);
}

void HEncoder::ExtractPerFrameIRitioParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta)
{
    auto *iRatio = reader.Read<OMX_S32>();
    IF_TRUE_RETURN_VOID(iRatio == nullptr);
    meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_FRAME_I_RATIO, *iRatio);
}

void HEncoder::ExtractPerFrameAveQpParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta)
{
    auto *averageQp = reader.Read<OMX_S32>();
    IF_TRUE_RETURN_VOID(averageQp == nullptr);
    meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_QP_AVERAGE, *averageQp);
}

void HEncoder::ExtractPerFrameMSEParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta)
{
    auto *averageMseLcu = reader.Read<double>();
    IF_TRUE_RETURN_VOID(averageMseLcu == nullptr);
    meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_MSE, *averageMseLcu);
}

void HEncoder::ExtractPerFrameLayerParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta)
{
    auto *frameLayer = reader.Read<OMX_S32>();
    IF_TRUE_RETURN_VOID(frameLayer == nullptr);
    meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_FRAME_TEMPORAL_ID, *frameLayer);
}

int32_t HEncoder::AllocInBufsForDynamicSurfaceBuf()
{
    inputBufferPool_.clear();
    for (uint32_t i = 0; i < inBufferCnt_; ++i) {
        shared_ptr<CodecHDI::OmxCodecBuffer> omxBuffer = DynamicSurfaceBufferToOmxBuffer();
        shared_ptr<CodecHDI::OmxCodecBuffer> outBuffer = make_shared<CodecHDI::OmxCodecBuffer>();
        int32_t ret = compNode_->UseBuffer(OMX_DirInput, *omxBuffer, *outBuffer);
        if (ret != HDF_SUCCESS) {
            HLOGE("Failed to UseBuffer on input port");
            return AVCS_ERR_UNKNOWN;
        }
        BufferInfo info(true, BufferOwner::OWNED_BY_SURFACE, record_);
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
    ReduceOwner(portIndex, info.owner);
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
    SCOPED_TRACE_FMT("id: %u", bufferId);
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
        record_[OMX_DirInput].discardCntInterval_++;
        bufferInfo->avBuffer->meta_->Clear();
        ResetSlot(*bufferInfo);
        ReplyErrorCode(msg.id, AVCS_ERR_OK);
        return;
    }
    SetBufferPts(bufferInfo);
    ChangeOwner(*bufferInfo, BufferOwner::OWNED_BY_US);
    WrapSurfaceBufferToSlot(*bufferInfo, bufferInfo->surfaceBuffer, bufferInfo->avBuffer->pts_,
        UserFlagToOmxFlag(static_cast<AVCodecBufferFlag>(bufferInfo->avBuffer->flag_)));
    if (enableVariableFrameRate_ && CalculateFrameRateParamIntoOmxBuffer(bufferInfo->omxBuffer->pts) != 0) {
        ReplyErrorCode(msg.id, AVCS_ERR_INPUT_DATA_ERROR);
    }
    WrapPerFrameParamIntoOmxBuffer(bufferInfo->omxBuffer, bufferInfo->avBuffer->meta_);
    ReplyErrorCode(msg.id, AVCS_ERR_OK);
    int32_t err = HCodec::OnQueueInputBuffer(mode, bufferInfo);
    if (err != AVCS_ERR_OK) {
        ResetSlot(*bufferInfo);
        callback_->OnError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_INPUT_DATA_ERROR);
    }
}

void HEncoder::SetBufferPts(BufferInfo* info)
{
    HLOGD("avBuffer->pts_ before setted, absolute pts=%ld", info->avBuffer->pts_);
    bool bret = info->avBuffer->meta_->GetData(
        OHOS::Media::Tag::VIDEO_ENCODE_SET_FRAME_PTS, info->avBuffer->pts_);
    HLOGD("avBuffer->pts_ after setted, relative pts=%ld, bret=%d", info->avBuffer->pts_, bret);
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
    int32_t dispFmt = entry.item->buffer->GetFormat();
    if (!CheckBufPixFmt(dispFmt)) {
        return false;
    }
    inputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT, dispFmt);
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
            return;
        }
        InSurfaceBufferEntry entry = avaliableBuffers_.front();
        avaliableBuffers_.pop_front();
        SubmitOneBuffer(entry, *it);
    }
}

void HEncoder::SubmitOneBuffer(InSurfaceBufferEntry& entry, BufferInfo &info)
{
    if (entry.item == nullptr) {
        ChangeOwner(info, BufferOwner::OWNED_BY_US);
        HLOGI("got input eos");
        inputPortEos_ = true;
        info.omxBuffer->flag = OMX_BUFFERFLAG_EOS;
        info.omxBuffer->bufferhandle = nullptr;
        info.omxBuffer->filledLen = 0;
        info.surfaceBuffer = nullptr;
        InBufUsToOmx(info);
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
        CheckPts(info.omxBuffer->pts);
        int32_t err = InBufUsToOmx(info);
        if (err != AVCS_ERR_OK) {
            ResetSlot(info);
            callback_->OnError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_INPUT_DATA_ERROR);
        }
    }
}

void HEncoder::CheckPts(int64_t currentPts)
{
    if (!pts_.has_value()) {
        pts_ = currentPts;
    } else {
        int64_t lastPts = pts_.value();
        if (lastPts != 0 && currentPts != 0 && lastPts >= currentPts) {
            HLOGW("pts not incremental: last %" PRId64 ", current %" PRId64, lastPts, currentPts);
        }
        pts_ = currentPts;
    }
}

void HEncoder::OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode)
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
    if (inputSurface_) {
        ResetSlot(*info);
        if (mode == RESUBMIT_BUFFER && !inputPortEos_) {
            TraverseAvaliableBuffers();
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
    std::shared_ptr<MsgToken> codec = codec_.lock();
    if (codec != nullptr) {
        codec->SendAsyncMsg(MsgWhat::GET_BUFFER_FROM_SURFACE, nullptr);
    }
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
    pts_ = std::nullopt;
}

HEncoder::BufferItem::~BufferItem()
{
    if (surface && buffer) {
        LOGD("release seq = %u", buffer->GetSeqNum());
        surface->ReleaseBuffer(buffer, -1);
    }
}

} // namespace OHOS::MediaAVCodec
