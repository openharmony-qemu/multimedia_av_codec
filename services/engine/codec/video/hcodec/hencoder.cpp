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

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::HDI::Codec::V1_0;

int32_t HEncoder::OnConfigure(const Format &format)
{
    configFormat_ = make_shared<Format>(format);
    int32_t ret = SetupPort(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    switch (static_cast<int>(codingType_)) {
        case OMX_VIDEO_CodingAVC:
            ret = SetupAVCEncoderParameters(format);
            break;
        case CODEC_OMX_VIDEO_CodingHEVC:
            ret = SetupHEVCEncoderParameters(format);
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
    return AVCS_ERR_OK;
}

int32_t HEncoder::SetColorAspects(const Format &format)
{
    CodecVideoColorspace param;
    InitOMXParamExt(param);
    param.portIndex = OMX_DirInput;

    param.aspects.range = RANGE_UNSPECIFIED;
    int range = 0;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, range)) {
        HLOGI("user set range flag %{public}d", range);
        param.aspects.range = TypeConverter::RangeFlagToOmxRangeType(static_cast<bool>(range));
    }

    int primary = static_cast<int>(COLOR_PRIMARY_UNSPECIFIED);
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, primary)) {
        HLOGI("user set primary %{public}d", primary);
    }
    param.aspects.primaries = TypeConverter::InnerPrimaryToOmxPrimary(static_cast<ColorPrimary>(primary));

    int transfer = static_cast<int>(TRANSFER_CHARACTERISTIC_UNSPECIFIED);
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, transfer)) {
        HLOGI("user set transfer %{public}d", transfer);
    }
    param.aspects.transfer = TypeConverter::InnerTransferToOmxTransfer(static_cast<TransferCharacteristic>(transfer));

    int matrix = static_cast<int>(MATRIX_COEFFICIENT_UNSPECIFIED);
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, matrix)) {
        HLOGI("user set matrix %{public}d", matrix);
    }
    param.aspects.matrixCoeffs = TypeConverter::InnerMatrixToOmxMatrix(static_cast<MatrixCoefficient>(matrix));

    if (!SetParameter(OMX_IndexColorAspects, param, true)) {
        HLOGE("failed to set CodecVideoColorSpace");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set omx color aspects (full range:%{public}d, primary:%{public}d, "
          "transfer:%{public}d, matrix:%{public}d) succ",
          param.aspects.range, param.aspects.primaries, param.aspects.transfer, param.aspects.matrixCoeffs);
    return AVCS_ERR_OK;
}

void HEncoder::CalcInputBufSize(PortInfo& info, VideoPixelFormat pixelFmt)
{
    uint32_t inSize = AlignTo(info.width, 128u) * AlignTo(info.height, 128u); // 128: block size
    if (pixelFmt == RGBA) {
        inSize = inSize * 4; // 4 byte per pixel
    } else {
        inSize = inSize * 3 / 2; // 3: nom, 2: denom
    }
    info.inputBufSize = inSize;
}

int32_t HEncoder::SetupPort(const Format &format)
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
    HLOGI("user set width %{public}d, height %{public}d", width, height);
    if (!GetPixelFmtFromUser(format)) {
        return AVCS_ERR_INVALID_VAL;
    }

    double frameRate = 30.0;
    if (format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRate)) {
        HLOGI("user set frame rate %{public}.2f", frameRate);
    }

    PortInfo inputPortInfo = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), std::nullopt,
                              OMX_VIDEO_CodingUnused, configuredFmt_, frameRate};
    CalcInputBufSize(inputPortInfo, configuredFmt_.innerFmt);
    int32_t ret = SetVideoPortInfo(OMX_DirInput, inputPortInfo);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    PortInfo outputPortInfo = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), std::nullopt,
                               codingType_, std::nullopt, frameRate};
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

    if (inputFormat_ == nullptr) {
        inputFormat_ = make_shared<Format>();
    }
    inputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, w);
    inputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, h);
    inputFormat_->PutIntValue("stride", def.format.video.nStride);
    sharedBufferFormat_ = { w, h, def.format.video.nStride, OMX_VIDEO_CodingUnused, configuredFmt_ };
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
    uint32_t iFramesInterval = iFramesIntervalInMs * frameRate / 1000;
    uint32_t pFramesSpacing = iFramesInterval / (bFramesSpacing + 1);
    return pFramesSpacing > 0 ? pFramesSpacing - 1 : 0;
}

int32_t HEncoder::ConfigureOutputBitrate(const Format &format)
{
    VideoEncodeBitrateMode mode;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, *reinterpret_cast<int*>(&mode))) {
        return AVCS_ERR_INVALID_VAL;
    }
    switch (mode) {
        case CBR:
        case VBR: {
            int64_t bitRate;
            if (!format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate) ||
                bitRate <= 0 || bitRate > UINT32_MAX) {
                HLOGW("user set CBR/VBR mode but not set valid bitrate");
                return AVCS_ERR_INVALID_VAL;
            }
            OMX_VIDEO_PARAM_BITRATETYPE bitrateType;
            InitOMXParam(bitrateType);
            bitrateType.nPortIndex = OMX_DirOutput;
            bitrateType.eControlRate = (mode == CBR) ? OMX_Video_ControlRateConstant : OMX_Video_ControlRateVariable;
            bitrateType.nTargetBitrate = static_cast<OMX_U32>(bitRate);
            if (!SetParameter(OMX_IndexParamVideoBitrate, bitrateType)) {
                HLOGE("failed to set OMX_IndexParamVideoBitrate");
                return AVCS_ERR_UNKNOWN;
            }
            HLOGI("set %{public}s mode and target bitrate %{public}u bps succ",
                (mode == CBR) ? "CBR" : "VBR", bitrateType.nTargetBitrate);
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
            HLOGI("set CQ mode and target quality %{public}u succ", bitrateType.qualityValue);
            return AVCS_ERR_OK;
        }
        default:
            return AVCS_ERR_INVALID_VAL;
    }
}

int32_t HEncoder::SetupAVCEncoderParameters(const Format &format)
{
    OMX_VIDEO_PARAM_AVCTYPE avcType;
    InitOMXParam(avcType);
    avcType.nPortIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexParamVideoAvc, avcType)) {
        HLOGE("get OMX_IndexParamVideoAvc parameter fail");
        return AVCS_ERR_UNKNOWN;
    }
    avcType.nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
    avcType.eProfile = OMX_VIDEO_AVCProfileBaseline;
    avcType.nBFrames = 0;

    SetAvcFields(avcType, format);
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

void HEncoder::SetAvcFields(OMX_VIDEO_PARAM_AVCTYPE& avcType, const Format &format)
{
    int32_t iFrameInterval = -1;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, iFrameInterval);
    double frameRate = 30.0;
    format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRate);

    AVCProfile profile;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_PROFILE, *reinterpret_cast<int*>(&profile))) {
        optional<OMX_VIDEO_AVCPROFILETYPE> omxAvcProfile = TypeConverter::InnerAvcProfileToOmxProfile(profile);
        if (omxAvcProfile.has_value()) {
            avcType.eProfile = omxAvcProfile.value();
        }
    }
    HLOGI("iFrameInterval:%{public}d, frameRate:%{public}.2f, eProfile:0x%{public}x, eLevel:0x%{public}x",
        iFrameInterval, frameRate, avcType.eProfile, avcType.eLevel);

    if (avcType.eProfile == OMX_VIDEO_AVCProfileBaseline) {
        avcType.nSliceHeaderSpacing = 0;
        avcType.bUseHadamard = OMX_TRUE;
        avcType.nRefFrames = 1;
        avcType.nPFrames = SetPFramesSpacing(iFrameInterval, frameRate, avcType.nBFrames);
        if (avcType.nPFrames == 0) {
            avcType.nAllowedPictureTypes = OMX_VIDEO_PictureTypeI;
        }
        avcType.nRefIdx10ActiveMinus1  = 0;
        avcType.nRefIdx11ActiveMinus1  = 0;
        avcType.bEntropyCodingCABAC    = OMX_FALSE;
        avcType.bWeightedPPrediction   = OMX_FALSE;
        avcType.bconstIpred            = OMX_FALSE;
        avcType.bDirect8x8Inference    = OMX_FALSE;
        avcType.bDirectSpatialTemporal = OMX_FALSE;
        avcType.nCabacInitIdc          = 0;
    } else if (avcType.eProfile == OMX_VIDEO_AVCProfileMain || avcType.eProfile == OMX_VIDEO_AVCProfileHigh) {
        avcType.nSliceHeaderSpacing = 0;
        avcType.bUseHadamard = OMX_TRUE;
        int32_t maxBframes;
        if (format.GetIntValue("max-bframes", maxBframes)) {
            avcType.nBFrames = maxBframes;
        }
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

int32_t HEncoder::SetupHEVCEncoderParameters(const Format &format)
{
    CodecVideoParamHevc hevcType;
    InitOMXParamExt(hevcType);
    hevcType.portIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexParamVideoHevc, hevcType)) {
        HLOGE("get OMX_IndexParamVideoHevc parameter fail");
        return AVCS_ERR_UNKNOWN;
    }

    HEVCProfile profile;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_PROFILE, *reinterpret_cast<int*>(&profile))) {
        optional<CodecHevcProfile> omxHevcProfile = TypeConverter::InnerHevcProfileToOmxProfile(profile);
        if (omxHevcProfile.has_value()) {
            hevcType.profile = omxHevcProfile.value();
            HLOGI("HEVCProfile %{public}d, CodecHevcProfile 0x%{public}x", profile, hevcType.profile);
        }
    }

    int32_t iFrameInterval;
    double frameRate;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, iFrameInterval) &&
        iFrameInterval >= 0 &&
        format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRate) &&
        frameRate > 0) {
        if (iFrameInterval == 0) { // all intra
            hevcType.keyFrameInterval = 1;
        } else {
            hevcType.keyFrameInterval = iFrameInterval * frameRate / 1000; // 1000: one second in milliseconds
        }
        HLOGI("frameRate %{public}.2f, iFrameInterval %{public}d, keyFrameInterval %{public}u",
            frameRate, iFrameInterval, hevcType.keyFrameInterval);
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
    int32_t requestIdr;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_REQUEST_I_FRAME, requestIdr) && requestIdr != 0) {
        int32_t ret = RequestIDRFrame();
        if (ret != AVCS_ERR_OK) {
            return ret;
        }
    }
    return AVCS_ERR_OK;
}

int32_t HEncoder::SubmitOutputBuffersToOmxNode()
{
    for (BufferInfo& info : outputBufferPool_) {
        if (info.owner == BufferOwner::OWNED_BY_US) {
            int32_t ret = NotifyOmxToFillThisOutputBuffer(info);
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
    if (inputSurface_ == nullptr) {
        inputBufferType_ = BufferType::PRESET_ASHM_BUFFER;
        HLOGI("buffer mode");
    } else {
        inputBufferType_ = BufferType::DYNAMIC_SURFACE_BUFFER;
        HLOGI("surface mode");
    }
    return true;
}

int32_t HEncoder::SubmitAllBuffersOwnedByUs()
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

    if (inputBufferType_ == BufferType::PRESET_ASHM_BUFFER) {
        for (BufferInfo& info : inputBufferPool_) {
            if (info.owner == BufferOwner::OWNED_BY_US) {
                NotifyUserToFillThisInputBuffer(info);
            }
        }
    }

    isBufferCirculating_ = true;
    return AVCS_ERR_OK;
}

sptr<Surface> HEncoder::OnCreateInputSurface()
{
    if (inputSurface_ != nullptr) {
        HLOGE("inputSurface_ already exists");
        return nullptr;
    }

    sptr<Surface> consumerSurface  = Surface::CreateSurfaceAsConsumer();
    if (consumerSurface == nullptr) {
        HLOGE("Create the surface consummer fail");
        return nullptr;
    }

    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    if (producer == nullptr) {
        HLOGE("Get the surface producer fail");
        return nullptr;
    }

    sptr<Surface> producerSurface  = Surface::CreateSurfaceAsProducer(producer);
    if (producerSurface == nullptr) {
        HLOGE("CreateSurfaceAsProducer fail");
        return nullptr;
    }

    UseBufferType useBufferTypes;
    InitOMXParamExt(useBufferTypes);
    useBufferTypes.portIndex = OMX_DirInput;
    useBufferTypes.bufferType = CODEC_BUFFER_TYPE_DYNAMIC_HANDLE;
    if (!SetParameter(OMX_IndexParamUseBufferType, useBufferTypes)) {
        HLOGE("set OMX_IndexParamUseBufferType failed");
        return nullptr;
    }

    sptr<IBufferConsumerListener> listener = new EncoderBuffersConsumerListener(this);
    consumerSurface->RegisterConsumerListener(listener);

    inputSurface_ = consumerSurface;
    return producerSurface;
}

int32_t HEncoder::OnSetInputSurface(sptr<Surface>& inputSurface)
{
    if (inputSurface_ != nullptr) {
        HLOGE("inputSurface_ already exists");
        return AVCS_ERR_INVALID_OPERATION;
    }

    if (inputSurface == nullptr) {
        HLOGE("surface is null");
        return AVCS_ERR_INVALID_VAL;
    }
    if (inputSurface->IsConsumer()) {
        HLOGE("expect a producer surface but got a consumer surface");
        return AVCS_ERR_INVALID_VAL;
    }
    inputSurface_ = inputSurface;
    HLOGI("succ");
    return AVCS_ERR_OK;
}

shared_ptr<OmxCodecBuffer> HEncoder::AllocOmxBufferOfDynamicType()
{
    auto omxBuffer = make_shared<OmxCodecBuffer>();
    omxBuffer->size = sizeof(OmxCodecBuffer);
    omxBuffer->version.version.majorVersion = 1;
    omxBuffer->bufferType = CODEC_BUFFER_TYPE_DYNAMIC_HANDLE;
    omxBuffer->allocLen = 0;
    omxBuffer->fenceFd = -1;
    omxBuffer->pts = 0;
    omxBuffer->flag = 0;
    return omxBuffer;
}

int32_t HEncoder::WrapSurfaceBufferIntoOmxBuffer(shared_ptr<OmxCodecBuffer>& omxBuffer,
    const sptr<SurfaceBuffer>& surfaceBuffer, int32_t fenceFd, int64_t pts, uint32_t flag)
{
    BufferHandle* bufferHandle = surfaceBuffer->GetBufferHandle();
    if (bufferHandle == nullptr) {
        HLOGE("null BufferHandle");
        return AVCS_ERR_UNKNOWN;
    }
    omxBuffer->bufferhandle = new NativeBuffer(bufferHandle);
    omxBuffer->filledLen = surfaceBuffer->GetSize();
    omxBuffer->fenceFd = fenceFd;
    omxBuffer->pts = pts;
    omxBuffer->flag = flag;
    return AVCS_ERR_OK;
}

int32_t HEncoder::AllocInBufsForDynamicSurfaceBuf()
{
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParam(def);
    def.nPortIndex = OMX_DirInput;
    if (!GetParameter(OMX_IndexParamPortDefinition, def)) {
        HLOGE("get input port definition failed");
        return AVCS_ERR_INVALID_VAL;
    }

    inputBufferPool_.clear();
    for (uint32_t i = 0; i < def.nBufferCountActual; ++i) {
        shared_ptr<OmxCodecBuffer> omxBuffer = AllocOmxBufferOfDynamicType();
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t ret = compNode_->UseBuffer(OMX_DirInput, *omxBuffer, *outBuffer);
        if (ret != HDF_SUCCESS) {
            HLOGE("Failed to UseBuffer on input port");
            return AVCS_ERR_UNKNOWN;
        }
        BufferInfo info {};
        info.owner          = BufferOwner::OWNED_BY_US;
        info.surfaceBuffer  = nullptr;
        info.sharedBuffer   = nullptr;
        info.omxBuffer      = outBuffer;
        info.bufferId       = outBuffer->bufferId;
        inputBufferPool_.push_back(info);
    }

    return AVCS_ERR_OK;
}

int32_t HEncoder::AllocateBuffersOnPort(OMX_DIRTYPE portIndex)
{
    if ((portIndex == OMX_DirInput) && (inputBufferType_ == BufferType::DYNAMIC_SURFACE_BUFFER)) {
        return AllocInBufsForDynamicSurfaceBuf();
    } else {
        return AllocateSharedBuffers(portIndex, (portIndex == OMX_DirInput));
    }
}

void HEncoder::EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i)
{
    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    if (i >= pool.size()) {
        return;
    }
    const BufferInfo& info = pool[i];
    FreeOmxBuffer(portIndex, info);
    pool.erase(pool.begin() + i);
}

int32_t HEncoder::OnUserQueueInputBuffer(uint32_t bufferId, const AVCodecBufferInfo &info,
    AVCodecBufferFlag flag, BufferOperationMode mode)
{
    if (inputBufferType_ == BufferType::DYNAMIC_SURFACE_BUFFER) {
        HLOGE("The current input buffer is surface buffer");
        return AVCS_ERR_INVALID_OPERATION;
    }
    return HCodec::OnUserQueueInputBuffer(bufferId, info, flag, mode);
}

void HEncoder::OnGetBufferFromSurface()
{
    for (BufferInfo& info : inputBufferPool_) {
        if (info.owner != BufferOwner::OWNED_BY_US) {
            continue;
        }
        sptr<SurfaceBuffer> surfaceBuffer;
        int32_t fenceFd;
        int64_t timestamp;
        OHOS::Rect damage;
        GSError ret = inputSurface_->AcquireBuffer(surfaceBuffer, fenceFd, timestamp, damage);
        if ((ret != GSERROR_OK) || (surfaceBuffer == nullptr)) {
            HLOGW("AcquireBuffer failed");
            return;
        }
        WrapSurfaceBufferIntoOmxBuffer(info.omxBuffer, surfaceBuffer, fenceFd, timestamp, 0);
        info.surfaceBuffer = surfaceBuffer;
        NotifyOmxToEmptyThisInputBuffer(info);
        return;
    }
    HLOGD("no input slot");
}

void HEncoder::OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode)
{
    HLOGD("inBufId = %{public}u", bufferId);
    BufferInfo *info = FindBufferInfoByID(OMX_DirInput, bufferId);
    if (info == nullptr) {
        HLOGE("unknown buffer id %{public}u", bufferId);
        return;
    }
    if (info->owner != BufferOwner::OWNED_BY_OMX) {
        HLOGE("wrong ownership: buffer id=%{public}d, owner=%{public}s", bufferId, info->Owner());
        return;
    }
    info->owner = BufferOwner::OWNED_BY_US;
    if (inputBufferType_ == BufferType::DYNAMIC_SURFACE_BUFFER) {
        GSError ret = inputSurface_->ReleaseBuffer(info->surfaceBuffer, -1);
        if (ret != GSERROR_OK) {
            HLOGW("ReleaseBuffer failed");
        }
    } else {
        switch (mode) {
            case KEEP_BUFFER:
                return;
            case RESUBMIT_BUFFER: {
                if (!inputPortEos_) {
                    NotifyUserToFillThisInputBuffer(*info);
                }
                return;
            }
            default: {
                HLOGE("SHOULD NEVER BE HERE");
                return;
            }
        }
    }
}

void HEncoder::EncoderBuffersConsumerListener::OnBufferAvailable()
{
    codec_->SendAsyncMsg(MsgWhat::GET_BUFFER_FROM_SURFACE, nullptr);
}

int32_t HEncoder::OnSignalEndOfInputStream()
{
    if (inputBufferType_ == BufferType::PRESET_ASHM_BUFFER) {
        HLOGE("can only be called in surface mode");
        return AVCS_ERR_INVALID_OPERATION;
    }

    inputPortEos_ = true;
    for (auto &item : inputBufferPool_) {
        if (item.owner == BufferOwner::OWNED_BY_US) {
            item.omxBuffer->flag = OMX_BUFFERFLAG_EOS;
            return NotifyOmxToEmptyThisInputBuffer(item);
        }
    }
    HLOGE("can not find any input buffer currently owned by us");
    return AVCS_ERR_UNKNOWN;
}
} // namespace OHOS::MediaAVCodec
