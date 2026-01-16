/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <vector>
#include <algorithm>
#include <thread>
#include <fcntl.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "codec_omx_ext.h"
#include "hcodec_list.h"
#include "hencoder.h"
#include "hdecoder.h"
#include "hitrace_meter.h"
#include "hcodec_log.h"
#include "hcodec_dfx.h"
#include "hcodec_utils.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace CodecHDI;
using namespace Media;

#define DMA_DEVICE_FILE "/dev/dma_reclaim"
#define DMA_BUF_RECLAIM_IOC_MAGIC 'd'
#define  DMA_BUF_RECLAIM_FD \
    _IOWR(DMA_BUF_RECLAIM_IOC_MAGIC, 0x07, int)
#define  DMA_BUF_RESUME_FD \
    _IOWR(DMA_BUF_RECLAIM_IOC_MAGIC, 0x08, int)

struct DmaBufIoctlSwPara {
    pid_t pid;
    unsigned long ino;
    unsigned int fd;
};

class DmaSwaper {
public:
    int32_t SwapOutDma(pid_t pid, int bufFd)
    {
        if (reclaimDriverFd_ <= 0) {
            return AVCS_ERR_UNKNOWN;
        }
        DmaBufIoctlSwPara param {
            .pid = pid,
            .ino = 0,
            .fd = bufFd
        };
        return ioctl(reclaimDriverFd_, DMA_BUF_RECLAIM_FD, &param);
    }

    int32_t SwapInDma(pid_t pid, int bufFd)
    {
        if (reclaimDriverFd_ <= 0) {
            return AVCS_ERR_UNKNOWN;
        }
        DmaBufIoctlSwPara param {
            .pid = pid,
            .ino = 0,
            .fd = bufFd
        };
        return ioctl(reclaimDriverFd_, DMA_BUF_RESUME_FD, &param);
    }

    static DmaSwaper& GetInstance()
    {
        static DmaSwaper swaper;
        return swaper;
    }
private:
    DmaSwaper()
    {
        if (reclaimDriverFd_ > 0) {
            LOGE("already initialized!");
            return;
        }
        reclaimDriverFd_ = open(DMA_DEVICE_FILE, O_RDWR | O_CLOEXEC | O_NONBLOCK);
        if (reclaimDriverFd_ <= 0) {
            LOGE("fail to open device");
        }
    }

    ~DmaSwaper()
    {
        if (reclaimDriverFd_ <= 0) {
            LOGE("invalid fd!");
            return;
        }
        close(reclaimDriverFd_);
        reclaimDriverFd_ = -1;
    }

    DmaSwaper(const DmaSwaper &dmaSwaper) = delete;
    const DmaSwaper &operator=(const DmaSwaper &dmaSwaper) = delete;
    int reclaimDriverFd_ = -1;
};


int32_t HCodec::NotifyMemoryRecycle()
{
    SCOPED_TRACE();
    FUNC_TRACKER();
    return DoSyncCall(MsgWhat::BUFFER_RECYCLE, nullptr);
}

int32_t HCodec::NotifyMemoryWriteBack()
{
    SCOPED_TRACE();
    FUNC_TRACKER();
    return DoSyncCall(MsgWhat::BUFFER_WRITEBACK, nullptr);
}

int32_t HCodec::NotifySuspend()
{
    SCOPED_TRACE();
    FUNC_TRACKER();
    DoSyncCall(MsgWhat::SUSPEND, nullptr);
    return AVCS_ERR_OK;
}

int32_t HCodec::NotifyResume()
{
    SCOPED_TRACE();
    FUNC_TRACKER();
    DoSyncCall(MsgWhat::RESUME, nullptr);
    return AVCS_ERR_OK;
}

void HCodec::RecordBufferStatus(OMX_DIRTYPE portIndex, uint32_t bufferId, BufferOwner nextOwner)
{
    auto bufferInfo = FindBufferInfoByID(portIndex, bufferId);
    HLOGI("port[%d] buffer[%u] next owner[%s]", portIndex, bufferId, ToString(nextOwner));
    if (bufferInfo != nullptr) {
        bufferInfo->nextStepOwner = nextOwner;
    }
}

int32_t HDecoder::SwapOutBufferByPortIndex(OMX_DIRTYPE portIndex)
{
    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    
    for (BufferInfo& info : pool) {
        if (CanSwapOut(portIndex, info) == false) {
            HLOGD("buf[%u] can't freeze owner[%d] swaped out[%d]", info.bufferId, info.owner, info.hasSwapedOut);
            continue;
        }
        int fd = (portIndex == OMX_DirInput) ? info.avBuffer->memory_->GetFileDescriptor() :
                 info.surfaceBuffer->GetFileDescriptor();
        if (DmaSwaper::GetInstance().SwapOutDma(pid_, fd) != AVCS_ERR_OK) {
            HLOGE("prot[%d] bufferId[%d], fd[%d] freeze failed", portIndex, info.bufferId, fd);
            return ActiveBuffers();
        }
        info.hasSwapedOut = true;
    }
    return AVCS_ERR_OK;
}

int32_t HDecoder::SwapInBufferByPortIndex(OMX_DIRTYPE portIndex)
{
    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;

    for (BufferInfo& info : pool) {
        if (info.hasSwapedOut == true) {
            int fd = (portIndex == OMX_DirInput) ? info.avBuffer->memory_->GetFileDescriptor() :
                     info.surfaceBuffer->GetFileDescriptor();
            if (DmaSwaper::GetInstance().SwapInDma(pid_, fd) != AVCS_ERR_OK) {
                HLOGE("buffer fd[%d] swap in error", fd);
                return AVCS_ERR_UNKNOWN;
            }
            info.hasSwapedOut = false;
        }
    }
    return AVCS_ERR_OK;
}

bool HDecoder::CanSwapOut(OMX_DIRTYPE portIndex, BufferInfo& info)
{
    if (portIndex == OMX_DirInput) {
        if (info.owner == BufferOwner::OWNED_BY_USER || info.hasSwapedOut) {
            return false;
        }
    }
    if (portIndex == OMX_DirOutput) {
        if (currSurface_.surface_) {
            return !(info.owner == BufferOwner::OWNED_BY_SURFACE ||
                     info.hasSwapedOut || info.surfaceBuffer == nullptr);
        } else {
            return !(info.owner == BufferOwner::OWNED_BY_SURFACE || info.surfaceBuffer == nullptr ||
                     info.owner == BufferOwner::OWNED_BY_USER || info.hasSwapedOut);
        }
    }
    return true;
}

int32_t HDecoder::FreezeBuffers()
{
    if (isSecure_) {
        return AVCS_ERR_OK;
    }
    OMX_CONFIG_BOOLEANTYPE param {};
    InitOMXParam(param);
    param.bEnabled = OMX_TRUE;
    if (!SetParameter(OMX_IndexParamBufferRecycle, param)) {
        HLOGE("failed to set decoder to background to freeze buffers");
        return AVCS_ERR_UNKNOWN;
    }
    if (SwapOutBufferByPortIndex(OMX_DirInput) != AVCS_ERR_OK) {
        return AVCS_ERR_UNKNOWN;
    }
    if (SwapOutBufferByPortIndex(OMX_DirOutput) != AVCS_ERR_OK) {
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("freeze buffers success");
    return AVCS_ERR_OK;
}

int32_t HDecoder::DecreaseFreq()
{
    OMX_CONFIG_BOOLEANTYPE param {};
    InitOMXParam(param);
    param.bEnabled = OMX_TRUE;
    if (!SetParameter(OMX_IndexParamFreqUpdate, param)) {
        HLOGE("failed to set decoder to background to decrease freq");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("Decrease Freq success");
    return AVCS_ERR_OK;
}

int32_t HDecoder::ActiveBuffers()
{
    if (isSecure_) {
        return AVCS_ERR_OK;
    }
    if (SwapInBufferByPortIndex(OMX_DirInput) != AVCS_ERR_OK) {
        return AVCS_ERR_UNKNOWN;
    }
    if (SwapInBufferByPortIndex(OMX_DirOutput) != AVCS_ERR_OK) {
        return AVCS_ERR_UNKNOWN;
    }
    OMX_CONFIG_BOOLEANTYPE param {};
    InitOMXParam(param);
    param.bEnabled = OMX_FALSE;
    if (!SetParameter(OMX_IndexParamBufferRecycle, param)) {
        HLOGE("failed to set OMX_IndexParamBufferRecycle");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("buffers active success");
    return AVCS_ERR_OK;
}

int32_t HDecoder::RecoverFreq()
{
    OMX_CONFIG_BOOLEANTYPE param {};
    InitOMXParam(param);
    param.bEnabled = OMX_FALSE;
    if (!SetParameter(OMX_IndexParamFreqUpdate, param)) {
        HLOGE("failed to set OMX_IndexParamFreqUpdate");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("Recover Freq success");
    return AVCS_ERR_OK;
}

void HDecoder::SubmitBuffersToNextOwner()
{
    while (!inputBufIdQueueToOmx_.empty()) {
        uint32_t bufferId = inputBufIdQueueToOmx_.front();
        inputBufIdQueueToOmx_.pop();
        auto bufInfo = FindBufferInfoByID(OMX_DirInput, bufferId);
        if (bufInfo != nullptr) {
            OnQueueInputBuffer(RESUBMIT_BUFFER, bufInfo);
        }
    }
    for (BufferInfo& info : inputBufferPool_) {
        if (info.nextStepOwner == BufferOwner::OWNED_BY_OMX) {
            HLOGI("bufferId = %d, input buffer next owner is omx", info.bufferId);
        } else if (info.nextStepOwner == BufferOwner::OWNED_BY_USER) {
            HLOGI("bufferId = %d, input buffer next owner is user", info.bufferId);
            if (!inputPortEos_) {
                NotifyUserToFillThisInBuffer(info);
            }
        }
        info.nextStepOwner = BufferOwner::OWNER_CNT;
    }
 
    for (BufferInfo& info : outputBufferPool_) {
        if (info.nextStepOwner == BufferOwner::OWNED_BY_OMX) {
            NotifyOmxToFillThisOutBuffer(info);
            HLOGI("bufferId = %d, output buffer next owner is omx", info.bufferId);
        } else if (info.nextStepOwner == BufferOwner::OWNED_BY_USER) {
            optional<size_t> idx = FindBufferIndexByID(OMX_DirOutput, info.bufferId);
            if (!idx.has_value()) {
                return;
            }
            HLOGI("bufferId = %d, output buffer next owner is user", info.bufferId);
            OnOMXFillBufferDone(RESUBMIT_BUFFER, info, idx.value());
        } else if (info.nextStepOwner == BufferOwner::OWNED_BY_SURFACE) {
            if (info.omxBuffer->filledLen != 0) {
                NotifySurfaceToRenderOutputBuffer(info);
            }
            SurfaceModeSubmitBuffer();
        }
        info.nextStepOwner = BufferOwner::OWNER_CNT;
    }
}

void HCodec::RunningState::OnBufferRecycle(const MsgInfo &info)
{
    if (codec_->disableDmaSwap_) {
        SLOGI("hcodec dma swap has been disabled!");
        ReplyErrorCode(info.id, AVCS_ERR_OK);
        return;
    }
    SLOGI("begin to buffer recycle");
    int32_t errCode = codec_->FreezeBuffers();
    if (errCode == AVCS_ERR_OK) {
        codec_->ChangeStateTo(codec_->frozenState_);
    }
    ReplyErrorCode(info.id, errCode);
}

/**************************** FrozenState Start ********************************/
void HCodec::FrozenState::OnMsgReceived(const MsgInfo &info)
{
    switch (info.type) {
        case MsgWhat::FORCE_SHUTDOWN: {
            codec_->ChangeStateTo(codec_->stoppingState_);
            return;
        }
        case MsgWhat::SET_PARAMETERS:
            OnSetParameters(info);
            return;
        case MsgWhat::QUEUE_INPUT_BUFFER: {
            uint32_t bufferId = 0;
            (void)info.param->GetValue(BUFFER_ID, bufferId);
            codec_->OnQueueInputBuffer(info, inputMode_);
            codec_->RecordBufferStatus(OMX_DirInput, bufferId, OWNED_BY_OMX);
            codec_->inputBufIdQueueToOmx_.push(bufferId);
            return;
        }
        case MsgWhat::RENDER_OUTPUT_BUFFER: {
            uint32_t bufferId = 0;
            (void)info.param->GetValue(BUFFER_ID, bufferId);
            codec_->OnRenderOutputBuffer(info, outputMode_);
            codec_->RecordBufferStatus(OMX_DirOutput, bufferId, OWNED_BY_OMX);
            return;
        }
        case MsgWhat::RELEASE_OUTPUT_BUFFER: {
            uint32_t bufferId = 0;
            (void)info.param->GetValue(BUFFER_ID, bufferId);
            codec_->OnReleaseOutputBuffer(info, outputMode_);
            codec_->RecordBufferStatus(OMX_DirOutput, bufferId, OWNED_BY_OMX);
            return;
        }
        case MsgWhat::OMX_EMPTY_BUFFER_DONE: {
            uint32_t bufferId = 0;
            (void)info.param->GetValue(BUFFER_ID, bufferId);
            codec_->OnOMXEmptyBufferDone(bufferId, inputMode_);
            codec_->RecordBufferStatus(OMX_DirInput, bufferId, OWNED_BY_USER);
            return;
        }
        case MsgWhat::OMX_FILL_BUFFER_DONE: {
            OmxCodecBuffer omxBuffer;
            (void)info.param->GetValue("omxBuffer", omxBuffer);
            codec_->OnOMXFillBufferDone(omxBuffer, outputMode_);
            codec_->RecordBufferStatus(OMX_DirOutput, omxBuffer.bufferId, OWNED_BY_OMX);
            return;
        }
        case MsgWhat::BUFFER_WRITEBACK: {
            OnBufferWriteback(info);
            return;
        }
        case MsgWhat::SUSPEND:{
            OnSuspend(info);
            break;
        }
        case MsgWhat::RESUME:{
            OnResume(info);
            return;
        }
        case MsgWhat::GET_BUFFER_FROM_SURFACE: {
            SLOGD("defer GET_BUFFER_FROM_SURFACE");
            codec_->DeferMessage(info);
            return;
        }
        case MsgWhat::CODEC_EVENT: {
            codec_->DeferMessage(info);
            SLOGI("deferring codec event");
            return;
        }
        default: {
            BaseState::OnMsgReceived(info);
        }
    }
}

void HCodec::FrozenState::OnBufferWriteback(const MsgInfo &info)
{
    SLOGI("begin to write back buffers");
    int32_t errCode = codec_->ActiveBuffers();
    if (errCode == AVCS_ERR_OK) {
        codec_->SubmitBuffersToNextOwner();
        codec_->ChangeStateTo(codec_->runningState_);
    }
    ReplyErrorCode(info.id, errCode);
}

void HCodec::FrozenState::OnShutDown(const MsgInfo &info)
{
    (void)codec_->ActiveBuffers();
    codec_->isShutDownFromRunning_ = true;
    codec_->notifyCallerAfterShutdownComplete_ = true;
    codec_->keepComponentAllocated_ = (info.type == MsgWhat::STOP);
    codec_->isBufferCirculating_ = false;
    codec_->PrintAllBufferInfo();
    SLOGI("receive %s msg, begin to set omx to idle", info.type == MsgWhat::RELEASE ? "release" : "stop");
    int32_t ret = codec_->compNode_->SendCommand(CODEC_COMMAND_STATE_SET, CODEC_STATE_IDLE, {});
    if (ret == HDF_SUCCESS) {
        codec_->ReplyToSyncMsgLater(info);
        codec_->ChangeStateTo(codec_->stoppingState_);
    } else {
        SLOGE("set omx to idle failed, ret=%d", ret);
        ReplyErrorCode(info.id, AVCS_ERR_UNKNOWN);
    }
}

/**************************** FrozenState End ********************************/
}