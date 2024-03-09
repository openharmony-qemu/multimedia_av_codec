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
#include "utils/hdf_base.h"
#include "codec_omx_ext.h"
#include "media_description.h"  // foundation/multimedia/av_codec/interfaces/inner_api/native/
#include "sync_fence.h"  // foundation/graphic/graphic_2d/utils/sync_fence/export/
#include "OMX_VideoExt.h"
#include "hcodec_log.h"
#include "type_converter.h"
#include "surface_buffer.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::HDI::Codec::V2_0;

int32_t HDecoder::OnConfigure(const Format &format)
{
    configFormat_ = make_shared<Format>(format);

    UseBufferType useBufferTypes;
    InitOMXParamExt(useBufferTypes);
    useBufferTypes.portIndex = OMX_DirOutput;
    useBufferTypes.bufferType = CODEC_BUFFER_TYPE_HANDLE;
    if (!SetParameter(OMX_IndexParamUseBufferType, useBufferTypes)) {
        HLOGE("component don't support CODEC_BUFFER_TYPE_HANDLE");
        return AVCS_ERR_INVALID_VAL;
    }

    SaveTransform(format);
    SaveScaleMode(format);
    (void)SetProcessName(format);
    (void)SetFrameRateAdaptiveMode(format);
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
    HLOGI("user set width %{public}d, height %{public}d", width, height);
    if (!GetPixelFmtFromUser(format)) {
        return AVCS_ERR_INVALID_VAL;
    }

    optional<double> frameRate = GetFrameRateFromUser(format);
    if (!frameRate.has_value()) {
        HLOGI("user don't set valid frame rate, use default 30.0");
        frameRate = 30.0;  // default frame rate 30.0
    }

    PortInfo inputPortInfo {static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                            codingType_, std::nullopt, frameRate.value()};
    int32_t maxInputSize = 0;
    (void)format.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, maxInputSize);
    if (maxInputSize > 0) {
        inputPortInfo.inputBufSize = static_cast<uint32_t>(maxInputSize);
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
        HLOGI("GraphicPixelFormat need update: configured(%{public}s) -> portdefinition(%{public}s)",
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
    if (def.nBufferCountActual == 0) {
        HLOGE("invalid bufferCount");
        return AVCS_ERR_UNKNOWN;
    }
    (void)UpdateConfiguredFmt(def.format.video.eColorFormat);

    uint32_t w = def.format.video.nFrameWidth;
    uint32_t h = def.format.video.nFrameHeight;

    // save into member variable
    GetCropFromOmx(w, h);
    outBufferCnt_ = def.nBufferCountActual;
    requestCfg_.timeout = 0;
    requestCfg_.width = flushCfg_.damage.w;
    requestCfg_.height = flushCfg_.damage.h;
    requestCfg_.strideAlignment = STRIDE_ALIGNMENT;
    requestCfg_.format = configuredFmt_.graphicFmt;
    requestCfg_.usage = GetProducerUsage();

    // save into format
    if (outputFormat_ == nullptr) {
        outputFormat_ = make_shared<Format>();
    }
    if (!outputFormat_->ContainKey(MediaDescriptionKey::MD_KEY_WIDTH)) {
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, w);
    }
    if (!outputFormat_->ContainKey(MediaDescriptionKey::MD_KEY_HEIGHT)) {
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, h);
    }
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_DISPLAY_WIDTH, flushCfg_.damage.w);
    outputFormat_->PutIntValue(OHOS::Media::Tag::VIDEO_DISPLAY_HEIGHT, flushCfg_.damage.h);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(configuredFmt_.innerFmt));
    HLOGI("output format: %{public}s", outputFormat_->Stringify().c_str());
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
    HLOGI("range:%{public}d, primary:%{public}d, transfer:%{public}d, matrix:%{public}d)",
        param.aspects.range, param.aspects.primaries, param.aspects.transfer, param.aspects.matrixCoeffs);
    if (outputFormat_) {
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, param.aspects.range);
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, param.aspects.primaries);
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, param.aspects.transfer);
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, param.aspects.matrixCoeffs);
        HLOGI("output format changed: %{public}s", outputFormat_->Stringify().c_str());
        callback_->OnOutputFormatChanged(*(outputFormat_.get()));
    }
}

void HDecoder::GetCropFromOmx(uint32_t w, uint32_t h)
{
    flushCfg_.damage.x = 0;
    flushCfg_.damage.y = 0;
    flushCfg_.damage.w = w;
    flushCfg_.damage.h = h;

    OMX_CONFIG_RECTTYPE rect;
    InitOMXParam(rect);
    rect.nPortIndex = OMX_DirOutput;
    if (!GetParameter(OMX_IndexConfigCommonOutputCrop, rect, true)) {
        HLOGW("get crop failed, use default");
        return;
    }
    if (rect.nLeft < 0 || rect.nTop < 0 ||
        rect.nWidth == 0 || rect.nHeight == 0 ||
        rect.nLeft + rect.nWidth > w ||
        rect.nTop + rect.nHeight > h) {
        HLOGW("wrong crop rect (%{public}d, %{public}d, %{public}u, %{public}u) vs. frame (%{public}u," \
              "%{public}u), use default", rect.nLeft, rect.nTop, rect.nWidth, rect.nHeight, w, h);
        return;
    }
    HLOGI("crop rect (%{public}d, %{public}d, %{public}u, %{public}u)",
          rect.nLeft, rect.nTop, rect.nWidth, rect.nHeight);
    flushCfg_.damage.x = rect.nLeft;
    flushCfg_.damage.y = rect.nTop;
    flushCfg_.damage.w = rect.nWidth;
    flushCfg_.damage.h = rect.nHeight;
}

int32_t HDecoder::OnSetOutputSurface(const sptr<Surface> &surface)
{
    if (surface == nullptr) {
        HLOGE("surface is null");
        return AVCS_ERR_INVALID_VAL;
    }
    if (surface->IsConsumer()) {
        HLOGE("expect a producer surface but got a consumer surface");
        return AVCS_ERR_INVALID_VAL;
    }
    std::weak_ptr<HDecoder> weakThis = weak_from_this();
    GSError err = surface->RegisterReleaseListener([weakThis](sptr<SurfaceBuffer> &buffer) {
        std::shared_ptr<HDecoder> decoder = weakThis.lock();
        if (decoder == nullptr) {
            LOGI("decoder is gone");
            return GSERROR_OK;
        }
        return decoder->OnBufferReleasedByConsumer(buffer);
    });
    if (err != GSERROR_OK) {
        HLOGE("RegisterReleaseListener failed, GSError=%{public}d", err);
        return AVCS_ERR_UNKNOWN;
    }
    outputSurface_ = surface;
    originalTransform_ = surface->GetTransform();
    HLOGI("set surface (%{public}s) succ", surface->GetName().c_str());
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
    return AVCS_ERR_OK;
}

int32_t HDecoder::SaveTransform(const Format &format, bool set)
{
    int32_t rotate;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, rotate)) {
        return AVCS_ERR_OK;
    }
    optional<GraphicTransformType> transform = TypeConverter::InnerRotateToDisplayRotate(
        static_cast<VideoRotation>(rotate));
    if (!transform.has_value()) {
        return AVCS_ERR_INVALID_VAL;
    }
    HLOGI("VideoRotation = %{public}d, GraphicTransformType = %{public}d", rotate, transform.value());
    transform_ = transform.value();
    if (set) {
        return SetTransform();
    }
    return AVCS_ERR_OK;
}

int32_t HDecoder::SetTransform()
{
    if (outputSurface_ == nullptr) {
        return AVCS_ERR_INVALID_VAL;
    }
    GSError err = outputSurface_->SetTransform(transform_);
    if (err != GSERROR_OK) {
        HLOGW("set GraphicTransformType %{public}d to surface failed", transform_);
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set GraphicTransformType %{public}d to surface succ", transform_);
    return AVCS_ERR_OK;
}

int32_t HDecoder::SaveScaleMode(const Format &format, bool set)
{
    int scaleType;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, scaleType)) {
        return AVCS_ERR_OK;
    }
    optional<ScalingMode> scaleMode = TypeConverter::InnerScaleToSurfaceScale(
        static_cast<OHOS::Media::Plugins::VideoScaleType>(scaleType));
    if (!scaleMode.has_value()) {
        return AVCS_ERR_INVALID_VAL;
    }
    HLOGI("VideoScaleType = %{public}d, ScalingMode = %{public}d", scaleType, scaleMode.value());
    scaleMode_ = scaleMode.value();
    if (set) {
        return SetScaleMode();
    }
    return AVCS_ERR_OK;
}

int32_t HDecoder::SetScaleMode()
{
    if (outputSurface_ == nullptr || !scaleMode_.has_value()) {
        return AVCS_ERR_INVALID_VAL;
    }
    for (const BufferInfo& info : outputBufferPool_) {
        if (info.surfaceBuffer == nullptr) {
            continue;
        }
        GSError err = outputSurface_->SetScalingMode(info.surfaceBuffer->GetSeqNum(), scaleMode_.value());
        if (err != GSERROR_OK) {
            HLOGW("set ScalingMode %{public}d to surface failed", scaleMode_.value());
            return AVCS_ERR_UNKNOWN;
        }
    }
    return AVCS_ERR_OK;
}

GSError HDecoder::OnBufferReleasedByConsumer(sptr<SurfaceBuffer> &buffer)
{
    SendAsyncMsg(MsgWhat::GET_BUFFER_FROM_SURFACE, nullptr);
    return GSERROR_OK;
}

int32_t HDecoder::SubmitOutputBuffersToOmxNode()
{
    for (BufferInfo& info : outputBufferPool_) {
        switch (info.owner) {
            case BufferOwner::OWNED_BY_US: {
                int32_t ret = NotifyOmxToFillThisOutBuffer(info);
                if (ret != AVCS_ERR_OK) {
                    return ret;
                }
                continue;
            }
            case BufferOwner::OWNED_BY_SURFACE: {
                continue;
            }
            case BufferOwner::OWNED_BY_OMX: {
                continue;
            }
            default: {
                HLOGE("buffer id %{public}u has invalid owner %{public}d", info.bufferId, info.owner);
                return AVCS_ERR_UNKNOWN;
            }
        }
    }
    return AVCS_ERR_OK;
}

bool HDecoder::ReadyToStart()
{
    if (callback_ == nullptr || outputFormat_ == nullptr || inputFormat_ == nullptr) {
        return false;
    }
    if (outputSurface_) {
        HLOGI("surface mode");
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
    int32_t ret = outputSurface_ ? AllocateOutputBuffersFromSurface() : AllocateAvSurfaceBuffers(portIndex);
    if (ret == AVCS_ERR_OK) {
        UpdateFormatFromSurfaceBuffer();
    }
    return ret;
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
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, surfaceBuffer->GetStride());

    OMX_PARAM_PORTDEFINITIONTYPE def;
    int32_t ret = GetPortDefinition(OMX_DirOutput, def);
    int32_t sliceHeight = static_cast<int32_t>(def.format.video.nSliceHeight);
    if (ret == AVCS_ERR_OK && sliceHeight >= surfaceBuffer->GetHeight()) {
        outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, sliceHeight);
    }
}

int32_t HDecoder::SubmitAllBuffersOwnedByUs()
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
    if (portIndex == OMX_DirOutput && outputSurface_ &&
        info.owner != BufferOwner::OWNED_BY_SURFACE) {
        CancelBufferToSurface(info);
    }
    FreeOmxBuffer(portIndex, info);
    pool.erase(pool.begin() + i);
}

uint64_t HDecoder::GetProducerUsage()
{
    uint64_t producerUsage = outputSurface_ ? SURFACE_MODE_PRODUCER_USAGE : BUFFER_MODE_REQUEST_USAGE;

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
    uint32_t consumerUsage = outputSurface_->GetDefaultUsage();
    uint64_t finalUsage = requestCfg_.usage | consumerUsage;
    HLOGI("producer usage 0x%{public}" PRIx64 " | consumer usage 0x%{public}x -> 0x%{public}" PRIx64 "",
        requestCfg_.usage, consumerUsage, finalUsage);
    requestCfg_.usage = finalUsage;
}

int32_t HDecoder::AllocateOutputBuffersFromSurface()
{
    GSError err = outputSurface_->CleanCache();
    if (err != GSERROR_OK) {
        HLOGW("clean cache failed");
    }
    err = outputSurface_->SetQueueSize(outBufferCnt_);
    if (err != GSERROR_OK) {
        HLOGE("set surface queue size failed");
        return AVCS_ERR_INVALID_VAL;
    }
    if (!outputBufferPool_.empty()) {
        HLOGW("output buffer pool should be empty");
    }
    outputBufferPool_.clear();
    CombineConsumerUsage();
    for (uint32_t i = 0; i < outBufferCnt_; ++i) {
        sptr<SurfaceBuffer> surfaceBuffer;
        sptr<SyncFence> fence;
        err = outputSurface_->RequestBuffer(surfaceBuffer, fence, requestCfg_);
        if (err != GSERROR_OK || surfaceBuffer == nullptr) {
            HLOGE("RequestBuffer %{public}u failed, GSError=%{public}d", i, err);
            return AVCS_ERR_UNKNOWN;
        }
        shared_ptr<OmxCodecBuffer> omxBuffer = SurfaceBufferToOmxBuffer(surfaceBuffer);
        if (omxBuffer == nullptr) {
            outputSurface_->CancelBuffer(surfaceBuffer);
            return AVCS_ERR_UNKNOWN;
        }
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t ret = compNode_->UseBuffer(OMX_DirOutput, *omxBuffer, *outBuffer);
        if (ret != HDF_SUCCESS) {
            outputSurface_->CancelBuffer(surfaceBuffer);
            HLOGE("Failed to UseBuffer with output port");
            return AVCS_ERR_NO_MEMORY;
        }
        outBuffer->fenceFd = -1;
        BufferInfo info {};
        info.isInput = false;
        info.owner = BufferOwner::OWNED_BY_US;
        info.surfaceBuffer = surfaceBuffer;
        info.avBuffer = AVBuffer::CreateAVBuffer();
        info.omxBuffer = outBuffer;
        info.bufferId = outBuffer->bufferId;
        outputBufferPool_.push_back(info);
    }
    SetTransform();
    SetScaleMode();
    return AVCS_ERR_OK;
}

void HDecoder::CancelBufferToSurface(BufferInfo& info)
{
    HLOGD("outBufId = %{public}u", info.bufferId);
    GSError ret = outputSurface_->CancelBuffer(info.surfaceBuffer);
    if (ret != OHOS::GSERROR_OK) {
        HLOGW("bufferId=%{public}u cancel failed, GSError=%{public}d", info.bufferId, ret);
    }
    ChangeOwner(info, BufferOwner::OWNED_BY_SURFACE); // change owner even if cancel failed
}

void HDecoder::OnGetBufferFromSurface()
{
    while (true) {
        if (!GetOneBufferFromSurface()) {
            break;
        }
    }
}

bool HDecoder::GetOneBufferFromSurface()
{
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    GSError ret = outputSurface_->RequestBuffer(buffer, fence, requestCfg_);
    if (ret != GSERROR_OK || buffer == nullptr) {
        return false;
    }
    if (fence != nullptr && fence->IsValid()) {
        int waitRes = fence->Wait(WAIT_FENCE_MS);
        if (waitRes != 0) {
            HLOGW("wait fence time out, cancel it");
            outputSurface_->CancelBuffer(buffer);
            return false;
        }
    }
    for (BufferInfo& info : outputBufferPool_) {
        if (info.owner == BufferOwner::OWNED_BY_SURFACE &&
            info.surfaceBuffer->GetBufferHandle() == buffer->GetBufferHandle()) {
            int32_t err = NotifyOmxToFillThisOutBuffer(info);
            if (err == AVCS_ERR_OK) {
                return true;
            }
            break;
        }
    }
    HLOGW("cannot find slot or submit to omx failed, cancel it");
    outputSurface_->CancelBuffer(buffer);
    return false;
}

int32_t HDecoder::NotifySurfaceToRenderOutputBuffer(BufferInfo &info)
{
    flushCfg_.timestamp = info.omxBuffer->pts;
    GSError ret = outputSurface_->FlushBuffer(info.surfaceBuffer, -1, flushCfg_);
    if (ret != GSERROR_OK) {
        HLOGE("FlushBuffer failed, GSError=%{public}d", ret);
        return AVCS_ERR_UNKNOWN;
    }
    HLOGD("outBufId = %{public}u, render succ, pts = %{public}" PRId64 ", "
        "[%{public}d %{public}d %{public}d %{public}d]", info.bufferId, flushCfg_.timestamp,
        flushCfg_.damage.x, flushCfg_.damage.y, flushCfg_.damage.w, flushCfg_.damage.h);
    ChangeOwner(info, BufferOwner::OWNED_BY_SURFACE);
    return AVCS_ERR_OK;
}

void HDecoder::OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode)
{
    BufferInfo *info = FindBufferInfoByID(OMX_DirInput, bufferId);
    if (info == nullptr) {
        HLOGE("unknown buffer id %{public}u", bufferId);
        return;
    }
    if (info->owner != BufferOwner::OWNED_BY_OMX) {
        HLOGE("wrong ownership: buffer id=%{public}d, owner=%{public}s", bufferId, ToString(info->owner));
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

void HDecoder::OnRenderOutputBuffer(const MsgInfo &msg, BufferOperationMode mode)
{
    if (outputSurface_ == nullptr) {
        HLOGE("can only render in surface mode");
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_OPERATION);
        return;
    }
    uint32_t bufferId;
    (void)msg.param->GetValue(BUFFER_ID, bufferId);
    optional<size_t> idx = FindBufferIndexByID(OMX_DirOutput, bufferId);
    if (!idx.has_value()) {
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    BufferInfo& info = outputBufferPool_[idx.value()];
    if (info.owner != BufferOwner::OWNED_BY_USER) {
        HLOGE("wrong ownership: buffer id=%{public}d, owner=%{public}s", bufferId, ToString(info.owner));
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    HLOGD("outBufId = %{public}u", info.bufferId);
    ChangeOwner(info, BufferOwner::OWNED_BY_US);
    ReplyErrorCode(msg.id, AVCS_ERR_OK);

    NotifySurfaceToRenderOutputBuffer(info);
    if (mode == FREE_BUFFER) {
        EraseBufferFromPool(OMX_DirOutput, idx.value());
    }
}

void HDecoder::OnEnterUninitializedState()
{
    if (outputSurface_) {
        outputSurface_->SetTransform(originalTransform_);
        outputSurface_->UnRegisterReleaseListener();
    }
}
} // namespace OHOS::MediaAVCodec