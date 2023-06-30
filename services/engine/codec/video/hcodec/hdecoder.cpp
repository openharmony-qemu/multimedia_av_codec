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

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::HDI::Codec::V1_0;

int32_t HDecoder::OnConfigure(const Format &format)
{
    configFormat_ = make_shared<Format>(format);

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

    double frameRate = 30.0;
    if (format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRate)) {
        HLOGI("user set frame rate %{public}.2f", frameRate);
    }

    PortInfo inputPortInfo {static_cast<uint32_t>(width), static_cast<uint32_t>(height), std::nullopt,
                            codingType_, std::nullopt, frameRate, };
    int32_t maxInputSize = 0;
    (void)format.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, maxInputSize);
    if (maxInputSize > 0) {
        inputPortInfo.inputBufSize = static_cast<uint32_t>(maxInputSize);
    }
    int32_t ret = SetVideoPortInfo(OMX_DirInput, inputPortInfo);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    PortInfo outputPortInfo = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), std::nullopt,
                               OMX_VIDEO_CodingUnused, configuredFmt_, frameRate, };
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
    inputFormat_->PutIntValue("stride", def.format.video.nStride);
    return AVCS_ERR_OK;
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
    uint32_t w = def.format.video.nFrameWidth;
    uint32_t h = def.format.video.nFrameHeight;

    // save into member variable
    outBufferCnt_ = def.nBufferCountActual;
    requestCfg_.width = w;
    requestCfg_.height = h;
    requestCfg_.strideAlignment = STRIDE_ALIGNMENT;
    requestCfg_.format = configuredFmt_.graphicFmt;
    requestCfg_.usage = GetUsageFromOmx();
    GetCropFromOmx(w, h);

    // save into format
    if (outputFormat_ == nullptr) {
        outputFormat_ = make_shared<Format>();
    }
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, w);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, h);
    outputFormat_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, configuredFmt_.innerFmt);

    sharedBufferFormat_ = { w, h, def.format.video.nStride, OMX_VIDEO_CodingUnused, configuredFmt_ };
    return AVCS_ERR_OK;
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

uint64_t HDecoder::GetUsageFromOmx()
{
    GetBufferHandleUsageParams usageParams;
    InitOMXParamExt(usageParams);
    usageParams.portIndex = static_cast<uint32_t>(OMX_DirOutput);
    if (!GetParameter(OMX_IndexParamGetBufferHandleUsage, usageParams)) {
        HLOGW("get buffer handle usage failed, use default");
        return BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA;
    }
    return usageParams.usage;
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
    GSError err = surface->RegisterReleaseListener([this](sptr<SurfaceBuffer> &buffer) {
        return OnBufferReleasedByConsumer(buffer);
    });
    if (err != GSERROR_OK) {
        HLOGE("RegisterReleaseListener failed, GSError=%{public}d", err);
        return AVCS_ERR_UNKNOWN;
    }
    UseBufferType param;
    InitOMXParamExt(param);
    param.portIndex = OMX_DirOutput;
    param.bufferType = CODEC_BUFFER_TYPE_HANDLE;
    if (!SetParameter(OMX_IndexParamUseBufferType, param)) {
        HLOGE("component don't support CODEC_BUFFER_TYPE_HANDLE");
        return AVCS_ERR_INVALID_OPERATION;
    }
    outputSurface_ = surface;
    HLOGI("succ");
    return AVCS_ERR_OK;
}

int32_t HDecoder::OnSetParameters(const Format &format)
{
    int32_t rotate;
    if (outputSurface_ && format.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, rotate)) {
        optional<GraphicTransformType> transform = TypeConverter::InnerRotateToDisplayRotate((VideoRotation)rotate);
        if (!transform.has_value()) {
            return AVCS_ERR_INVALID_VAL;
        }
        GSError err = outputSurface_->SetTransform(transform.value());
        if (err != GSERROR_OK) {
            HLOGE("set rotate angle %{public}d to surface failed", transform.value());
            return AVCS_ERR_UNKNOWN;
        }
        HLOGI("set rotate angle %{public}d to surface succ", rotate);
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
                int32_t ret = NotifyOmxToFillThisOutputBuffer(info);
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
    if (outputSurface_ == nullptr) {
        outputBufferType_ = BufferType::PRESET_ASHM_BUFFER;
        HLOGI("buffer mode");
        return true;
    }
    HLOGI("surface mode");
    outputBufferType_ = BufferType::PRESET_SURFACE_BUFFER;
    if (configFormat_) {
        OnSetParameters(*configFormat_);
    }
    return true;
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
            NotifyUserToFillThisInputBuffer(info);
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

shared_ptr<OmxCodecBuffer> HDecoder::SurfaceBufferToOmxBuffer(const sptr<SurfaceBuffer>& surfaceBuffer)
{
    BufferHandle* bufferHandle = surfaceBuffer->GetBufferHandle();
    if (bufferHandle == nullptr) {
        HLOGE("null BufferHandle");
        return nullptr;
    }
    std::shared_ptr<OmxCodecBuffer> omxBuffer = std::make_shared<OmxCodecBuffer>();
    omxBuffer->size = sizeof(OmxCodecBuffer);
    omxBuffer->version.version.majorVersion = 1;
    omxBuffer->bufferType = CODEC_BUFFER_TYPE_HANDLE;
    omxBuffer->bufferhandle = new NativeBuffer(bufferHandle);
    omxBuffer->allocLen = surfaceBuffer->GetSize();
    omxBuffer->fenceFd = -1;
    omxBuffer->pts = 0;
    omxBuffer->flag = 0;
    return omxBuffer;
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
    for (uint32_t i = 0; i < outBufferCnt_; ++i) {
        sptr<SurfaceBuffer> surfaceBuffer;
        {
            sptr<SyncFence> fence;
            err = outputSurface_->RequestBuffer(surfaceBuffer, fence, requestCfg_);
            if (err != GSERROR_OK || surfaceBuffer == nullptr) {
                HLOGE("RequestBuffer %{public}u failed, GSError=%{public}d", i, err);
                return AVCS_ERR_UNKNOWN;
            }
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
        info.owner = BufferOwner::OWNED_BY_US;
        info.surfaceBuffer = surfaceBuffer;
        info.sharedBuffer = nullptr;
        info.omxBuffer = outBuffer;
        info.bufferId = outBuffer->bufferId;
        outputBufferPool_.push_back(info);
    }
    return AVCS_ERR_OK;
}

int32_t HDecoder::AllocateBuffersOnPort(OMX_DIRTYPE portIndex)
{
    if ((portIndex == OMX_DirOutput) && (outputBufferType_ == BufferType::PRESET_SURFACE_BUFFER)) {
        return AllocateOutputBuffersFromSurface();
    } else {
        return AllocateSharedBuffers(portIndex, (portIndex == OMX_DirOutput));
    }
}

void HDecoder::CancelBufferToSurface(BufferInfo& info)
{
    HLOGD("outBufId = %{public}u", info.bufferId);
    GSError ret = outputSurface_->CancelBuffer(info.surfaceBuffer);
    if (ret != OHOS::GSERROR_OK) {
        HLOGW("bufferId=%{public}u cancel failed, GSError=%{public}d", info.bufferId, ret);
    }
    info.owner = BufferOwner::OWNED_BY_SURFACE; // change owner even if cancel failed
}

void HDecoder::FindSurfaceBufferSlotAndSubmit(sptr<SurfaceBuffer>& buffer)
{
    for (BufferInfo& info : outputBufferPool_) {
        if (info.owner == BufferOwner::OWNED_BY_SURFACE &&
            info.surfaceBuffer->GetBufferHandle() == buffer->GetBufferHandle()) {
            HLOGD("outBufId = %{public}u", info.bufferId);
            int32_t err = NotifyOmxToFillThisOutputBuffer(info);
            if (err == AVCS_ERR_OK) {
                return;
            }
            break;
        }
    }
    HLOGW("cannot find slot, cancel it");
    outputSurface_->CancelBuffer(buffer);
}

void HDecoder::OnGetBufferFromSurface()
{
    sptr<SurfaceBuffer> buffer;
    bool needCancel = false;
    {
        sptr<SyncFence> fence;  // it will be closed automatically when sptr destructed
        GSError ret = outputSurface_->RequestBuffer(buffer, fence, requestCfg_);
        if (ret != GSERROR_OK || buffer == nullptr) {
            HLOGW("RequestBuffer failed");
            return;
        }
        if (fence != nullptr && fence->IsValid()) {
            int waitRes = fence->Wait(5);  // 5ms
            if (waitRes != 0) {
                HLOGW("wait fence time out, cancel buffer");
                needCancel = true;
            }
        }
    }
    if (needCancel) {
        outputSurface_->CancelBuffer(buffer);
    } else {
        FindSurfaceBufferSlotAndSubmit(buffer);
    }
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
    info.owner = BufferOwner::OWNED_BY_SURFACE;
    return AVCS_ERR_OK;
}

void HDecoder::OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode)
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

int32_t HDecoder::OnUserRenderOutputBuffer(uint32_t bufferId, BufferOperationMode mode)
{
    if (outputBufferType_ == BufferType::PRESET_ASHM_BUFFER) {
        HLOGE("can only render in surface mode");
        return AVCS_ERR_INVALID_OPERATION;
    }

    HLOGD("outBufId = %{public}u", bufferId);
    optional<size_t> idx = FindBufferIndexByID(OMX_DirOutput, bufferId);
    if (!idx.has_value()) {
        return AVCS_ERR_INVALID_VAL;
    }
    BufferInfo& info = outputBufferPool_[idx.value()];
    if (info.owner != BufferOwner::OWNED_BY_USER) {
        HLOGE("wrong ownership: buffer id=%{public}d, owner=%{public}s", bufferId, info.Owner());
        return AVCS_ERR_INVALID_VAL;
    }
    info.owner = BufferOwner::OWNED_BY_US;

    switch (mode) {
        case KEEP_BUFFER: {
            return AVCS_ERR_OK;
        }
        case RESUBMIT_BUFFER: {
            if (outputPortEos_) {
                HLOGI("output eos, keep this buffer");
                return AVCS_ERR_OK;
            }
            return NotifySurfaceToRenderOutputBuffer(info);
        }
        case FREE_BUFFER: {
            EraseBufferFromPool(OMX_DirOutput, idx.value());
            return AVCS_ERR_OK;
        }
        default: {
            HLOGE("SHOULD NEVER BE HERE");
            return AVCS_ERR_UNKNOWN;
        }
    }
}
} // namespace OHOS::MediaAVCodec