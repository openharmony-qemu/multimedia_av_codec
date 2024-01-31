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

#include "hcodec.h"
#include "utils/hdf_base.h"
#include "hcodec_list.h"
#include "hcodec_log.h"
#include "hcodec_utils.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::HDI::Codec::V2_0;

/**************************** BaseState Start ****************************/
void HCodec::BaseState::OnMsgReceived(const MsgInfo &info)
{
    switch (info.type) {
        case MsgWhat::CODEC_EVENT: {
            OnCodecEvent(info);
            return;
        }
        case MsgWhat::OMX_EMPTY_BUFFER_DONE: {
            uint32_t bufferId;
            (void)info.param->GetValue(BUFFER_ID, bufferId);
            codec_->OnOMXEmptyBufferDone(bufferId, inputMode_);
            return;
        }
        case MsgWhat::OMX_FILL_BUFFER_DONE: {
            OmxCodecBuffer omxBuffer;
            (void)info.param->GetValue("omxBuffer", omxBuffer);
            codec_->OnOMXFillBufferDone(omxBuffer, outputMode_);
            return;
        }
        case MsgWhat::GET_INPUT_FORMAT:
        case MsgWhat::GET_OUTPUT_FORMAT: {
            OnGetFormat(info);
            return;
        }
        case MsgWhat::STOP:
        case MsgWhat::RELEASE: {
            OnShutDown(info);
            return;
        }
        default: {
            const char* msgWhat = HCodec::ToString(static_cast<MsgWhat>(info.type));
            if (info.id == ASYNC_MSG_ID) {
                SLOGI("ignore msg %{public}s in current state", msgWhat);
            } else { // Make sure that all sync message are replied
                SLOGE("%{public}s cannot be called at this state", msgWhat);
                ReplyErrorCode(info.id, AVCS_ERR_INVALID_STATE);
            }
            return;
        }
    }
}

void HCodec::BaseState::ReplyErrorCode(MsgId id, int32_t err)
{
    if (id == ASYNC_MSG_ID) {
        return;
    }
    ParamSP reply = ParamBundle::Create();
    reply->SetValue("err", err);
    codec_->PostReply(id, reply);
}

void HCodec::BaseState::OnCodecEvent(const MsgInfo &info)
{
    CodecEventType event;
    uint32_t data1;
    uint32_t data2;
    (void)info.param->GetValue("event", event);
    (void)info.param->GetValue("data1", data1);
    (void)info.param->GetValue("data2", data2);
    if (event == CODEC_EVENT_CMD_COMPLETE &&
        data1 == static_cast<uint32_t>(CODEC_COMMAND_FLUSH) &&
        data2 == static_cast<uint32_t>(OMX_ALL)) {
        SLOGD("ignore flush all complete event");
    } else {
        OnCodecEvent(event, data1, data2);
    }
}

void HCodec::BaseState::OnCodecEvent(CodecEventType event, uint32_t data1, uint32_t data2)
{
    if (event == CODEC_EVENT_ERROR) {
        SLOGE("omx report error event, data1 = %{public}u, data2 = %{public}u", data1, data2);
        codec_->SignalError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_SERVICE_DIED);
    } else {
        SLOGW("ignore event %{public}d, data1 = %{public}u, data2 = %{public}u", event, data1, data2);
    }
}

void HCodec::BaseState::OnGetFormat(const MsgInfo &info)
{
    shared_ptr<Format> fmt = (info.type == MsgWhat::GET_INPUT_FORMAT) ?
        codec_->inputFormat_ : codec_->outputFormat_;
    ParamSP reply = ParamBundle::Create();
    if (fmt) {
        reply->SetValue<int32_t>("err", AVCS_ERR_OK);
        reply->SetValue("format", *fmt);
        codec_->PostReply(info.id, reply);
    } else {
        ReplyErrorCode(info.id, AVCS_ERR_UNKNOWN);
    }
}

void HCodec::BaseState::OnCheckIfStuck(const MsgInfo &info)
{
    int32_t generation;
    (void)info.param->GetValue("generation", generation);
    if (generation == codec_->stateGeneration_) {
        SLOGE("stucked");
        codec_->PrintAllBufferInfo();
        codec_->SignalError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_UNKNOWN);
    }
}

void HCodec::BaseState::OnForceShutDown(const MsgInfo &info)
{
    int32_t generation;
    (void)info.param->GetValue("generation", generation);
    codec_->ForceShutdown(generation);
}
/**************************** BaseState End ******************************/


/**************************** UninitializedState start ****************************/
void HCodec::UninitializedState::OnStateEntered()
{
    codec_->OnEnterUninitializedState();
    codec_->ReleaseComponent();
}

void HCodec::UninitializedState::OnMsgReceived(const MsgInfo &info)
{
    switch (info.type) {
        case MsgWhat::INIT: {
            string name;
            (void)info.param->GetValue("name", name);
            int32_t err = OnAllocateComponent(name);
            ReplyErrorCode(info.id, err);
            if (err == AVCS_ERR_OK) {
                codec_->ChangeStateTo(codec_->initializedState_);
            }
            break;
        }
        default: {
            BaseState::OnMsgReceived(info);
        }
    }
}

static bool IsSecureMode(const string &name)
{
    string prefix = ".secure";
    if (name.length() <= prefix.length()) {
        return false;
    }
    return (name.rfind(prefix) == (name.length() - prefix.length()));
}

int32_t HCodec::UninitializedState::OnAllocateComponent(const std::string &name)
{
    codec_->isSecure_ = IsSecureMode(name);
    codec_->compMgr_ = codec_->isSecure_ ? GetIpcManager() : GetManager();
    if (codec_->compMgr_ == nullptr) {
        SLOGE("GetCodecComponentManager failed");
        return AVCS_ERR_UNKNOWN;
    }
    codec_->compCb_ = new HdiCallback(codec_);
    int32_t ret = codec_->compMgr_->CreateComponent(codec_->compNode_, codec_->componentId_, name,
                                                    0, codec_->compCb_);
    if (ret != HDF_SUCCESS || codec_->compNode_ == nullptr) {
        codec_->compCb_ = nullptr;
        codec_->compMgr_ = nullptr;
        SLOGE("CreateComponent failed, ret=%{public}d", ret);
        return AVCS_ERR_UNKNOWN;
    }
    codec_->componentName_ = name;
    codec_->compUniqueStr_ = "[" + to_string(codec_->componentId_) + "][" + name + "]";
    SLOGI("create omx node succ");
    return AVCS_ERR_OK;
}

void HCodec::UninitializedState::OnShutDown(const MsgInfo &info)
{
    ReplyErrorCode(info.id, AVCS_ERR_OK);
}

/**************************** UninitializedState End ******************************/

/**************************** InitializedState Start **********************************/
void HCodec::InitializedState::OnStateEntered()
{
    codec_->inputPortEos_ = false;
    codec_->outputPortEos_ = false;
    codec_->inputFormat_.reset();
    codec_->outputFormat_.reset();

    ProcessShutDownFromRunning();
    codec_->notifyCallerAfterShutdownComplete_ = false;
    codec_->ProcessDeferredMessages();
}

void HCodec::InitializedState::ProcessShutDownFromRunning()
{
    if (!codec_->isShutDownFromRunning_) {
        return;
    }
    SLOGI("we are doing shutdown from running/portchange/flush -> stopping -> initialized");
    bool keepComponentAllocated = codec_->keepComponentAllocated_;
    if (keepComponentAllocated) {
        if (codec_->configFormat_ == nullptr) {
            SLOGW("stored configuration is null");
        } else {
            Format copyOfCurConfig(*codec_->configFormat_);
            codec_->OnConfigure(copyOfCurConfig);
        }
    } else {
        codec_->ChangeStateTo(codec_->uninitializedState_);
    }
    if (codec_->notifyCallerAfterShutdownComplete_) {
        SLOGI("reply to %{public}s msg", keepComponentAllocated ? "stop" : "release");
        MsgInfo msg { keepComponentAllocated ? MsgWhat::STOP : MsgWhat::RELEASE, 0, nullptr };
        if (codec_->GetFirstSyncMsgToReply(msg)) {
            ReplyErrorCode(msg.id, AVCS_ERR_OK);
        }
        codec_->notifyCallerAfterShutdownComplete_ = false;
    }
    codec_->isShutDownFromRunning_ = false;
    codec_->keepComponentAllocated_ = false;
}

void HCodec::InitializedState::OnMsgReceived(const MsgInfo &info)
{
    switch (info.type) {
        case MsgWhat::SET_CALLBACK: {
            OnSetCallBack(info);
            return;
        }
        case MsgWhat::CONFIGURE: {
            OnConfigure(info);
            return;
        }
        case MsgWhat::CREATE_INPUT_SURFACE: {
            sptr<Surface> surface = codec_->OnCreateInputSurface();
            ParamSP reply = ParamBundle::Create();
            reply->SetValue<int32_t>("err", surface != nullptr ? AVCS_ERR_OK : AVCS_ERR_UNKNOWN);
            reply->SetValue("surface", surface);
            codec_->PostReply(info.id, reply);
            return;
        }
        case MsgWhat::SET_INPUT_SURFACE: {
            OnSetSurface(info, true);
            return;
        }
        case MsgWhat::SET_OUTPUT_SURFACE: {
            OnSetSurface(info, false);
            return;
        }
        case MsgWhat::START: {
            OnStart(info);
            return;
        }
        default: {
            BaseState::OnMsgReceived(info);
        }
    }
}

void HCodec::InitializedState::OnSetCallBack(const MsgInfo &info)
{
    int32_t err;
    shared_ptr<MediaCodecCallback> cb;
    (void)info.param->GetValue("callback", cb);
    if (cb == nullptr) {
        err = AVCS_ERR_INVALID_VAL;
        SLOGE("invalid param");
    } else {
        codec_->callback_ = cb;
        err = AVCS_ERR_OK;
    }
    ReplyErrorCode(info.id, err);
}

void HCodec::InitializedState::OnConfigure(const MsgInfo &info)
{
    Format fmt;
    (void)info.param->GetValue("format", fmt);
    ReplyErrorCode(info.id, codec_->OnConfigure(fmt));
}

void HCodec::InitializedState::OnSetSurface(const MsgInfo &info, bool isInput)
{
    sptr<Surface> surface;
    (void)info.param->GetValue("surface", surface);
    int32_t err = isInput ? codec_->OnSetInputSurface(surface) : codec_->OnSetOutputSurface(surface);
    ReplyErrorCode(info.id, err);
}

void HCodec::InitializedState::OnStart(const MsgInfo &info)
{
    if (!codec_->ReadyToStart()) {
        SLOGE("callback not set or format is not configured, can't start");
        ReplyErrorCode(info.id, AVCS_ERR_INVALID_OPERATION);
        return;
    }
    SLOGI("begin to set omx to idle");
    int32_t ret = codec_->compNode_->SendCommand(CODEC_COMMAND_STATE_SET, CODEC_STATE_IDLE, {});
    if (ret == HDF_SUCCESS) {
        codec_->ReplyToSyncMsgLater(info);
        codec_->ChangeStateTo(codec_->startingState_);
    } else {
        SLOGE("set omx to idle failed, ret=%{public}d", ret);
        ReplyErrorCode(info.id, AVCS_ERR_UNKNOWN);
    }
}

void HCodec::InitializedState::OnShutDown(const MsgInfo &info)
{
    if (info.type == MsgWhat::STOP) {
        SLOGI("receive STOP");
    } else {
        SLOGI("receive RELEASE");
        codec_->ChangeStateTo(codec_->uninitializedState_);
    }
    codec_->notifyCallerAfterShutdownComplete_ = false;
    ReplyErrorCode(info.id, AVCS_ERR_OK);
}
/**************************** InitializedState End ******************************/


/**************************** StartingState Start ******************************/
void HCodec::StartingState::OnStateEntered()
{
    hasError_ = false;

    ParamSP msg = ParamBundle::Create();
    msg->SetValue("generation", codec_->stateGeneration_);
    codec_->SendAsyncMsg(MsgWhat::CHECK_IF_STUCK, msg, THREE_SECONDS_IN_US);

    int32_t ret = AllocateBuffers();
    if (ret != AVCS_ERR_OK) {
        SLOGE("AllocateBuffers failed, back to init state");
        hasError_ = true;
        ReplyStartMsg(ret);
        codec_->ChangeStateTo(codec_->initializedState_);
    }
}

int32_t HCodec::StartingState::AllocateBuffers()
{
    int32_t ret = codec_->AllocateBuffersOnPort(OMX_DirInput);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = codec_->AllocateBuffersOnPort(OMX_DirOutput);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    return AVCS_ERR_OK;
}

void HCodec::StartingState::OnMsgReceived(const MsgInfo &info)
{
    switch (info.type) {
        case MsgWhat::GET_BUFFER_FROM_SURFACE:
        case MsgWhat::SET_PARAMETERS:
        case MsgWhat::GET_INPUT_FORMAT:
        case MsgWhat::GET_OUTPUT_FORMAT: {
            codec_->DeferMessage(info);
            return;
        }
        case MsgWhat::START:
        case MsgWhat::FLUSH: {
            ReplyErrorCode(info.id, AVCS_ERR_OK);
            return;
        }
        case MsgWhat::CHECK_IF_STUCK: {
            int32_t generation;
            if (info.param->GetValue("generation", generation) &&
                generation == codec_->stateGeneration_) {
                SLOGE("stucked, force state transition");
                hasError_ = true;
                ReplyStartMsg(AVCS_ERR_UNKNOWN);
                codec_->ChangeStateTo(codec_->initializedState_);
            }
            return;
        }
        default: {
            BaseState::OnMsgReceived(info);
        }
    }
}

void HCodec::StartingState::OnCodecEvent(CodecEventType event, uint32_t data1, uint32_t data2)
{
    if (event != CODEC_EVENT_CMD_COMPLETE) {
        return BaseState::OnCodecEvent(event, data1, data2);
    }
    if (data1 != (uint32_t)CODEC_COMMAND_STATE_SET) {
        SLOGW("ignore event: data1=%{public}u, data2=%{public}u", data1, data2);
        return;
    }
    if (data2 == (uint32_t)CODEC_STATE_IDLE) {
        SLOGI("omx now idle, begin to set omx to executing");
        int32_t ret = codec_->compNode_->SendCommand(CODEC_COMMAND_STATE_SET, CODEC_STATE_EXECUTING, {});
        if (ret != HDF_SUCCESS) {
            SLOGE("set omx to executing failed, ret=%{public}d", ret);
            hasError_ = true;
            ReplyStartMsg(AVCS_ERR_UNKNOWN);
            codec_->ChangeStateTo(codec_->initializedState_);
        }
    } else if (data2 == (uint32_t)CODEC_STATE_EXECUTING) {
        SLOGI("omx now executing");
        ReplyStartMsg(AVCS_ERR_OK);
        codec_->SubmitAllBuffersOwnedByUs();
        codec_->ChangeStateTo(codec_->runningState_);
    }
}

void HCodec::StartingState::OnShutDown(const MsgInfo &info)
{
    codec_->DeferMessage(info);
}

void HCodec::StartingState::ReplyStartMsg(int32_t errCode)
{
    MsgInfo msg {MsgWhat::START, 0, nullptr};
    if (codec_->GetFirstSyncMsgToReply(msg)) {
        SLOGI("start %{public}s", (errCode == 0) ? "succ" : "failed");
        ReplyErrorCode(msg.id, errCode);
    } else {
        SLOGE("there should be a start msg to reply");
    }
}

void HCodec::StartingState::OnStateExited()
{
    if (hasError_) {
        SLOGW("error occured, roll omx back to loaded and free allocated buffers");
        if (codec_->RollOmxBackToLoaded()) {
            codec_->ClearBufferPool(OMX_DirInput);
            codec_->ClearBufferPool(OMX_DirOutput);
        }
    }
    BaseState::OnStateExited();
}

/**************************** StartingState End ******************************/

/**************************** RunningState Start ********************************/
void HCodec::RunningState::OnStateEntered()
{
    codec_->ProcessDeferredMessages();
}

void HCodec::RunningState::OnMsgReceived(const MsgInfo &info)
{
    switch (info.type) {
        case MsgWhat::START:
            ReplyErrorCode(info.id, codec_->SubmitAllBuffersOwnedByUs());
            break;
        case MsgWhat::SET_PARAMETERS:
            OnSetParameters(info);
            break;
        case MsgWhat::REQUEST_IDR_FRAME:
            ReplyErrorCode(info.id, codec_->RequestIDRFrame());
            break;
        case MsgWhat::FLUSH:
            OnFlush(info);
            break;
        case MsgWhat::GET_BUFFER_FROM_SURFACE:
            codec_->OnGetBufferFromSurface();
            break;
        case MsgWhat::QUEUE_INPUT_BUFFER:
            codec_->OnQueueInputBuffer(info, inputMode_);
            break;
        case MsgWhat::NOTIFY_EOS:
            codec_->OnSignalEndOfInputStream(info);
            break;
        case MsgWhat::RENDER_OUTPUT_BUFFER:
            codec_->OnRenderOutputBuffer(info, outputMode_);
            break;
        case MsgWhat::RELEASE_OUTPUT_BUFFER:
            codec_->OnReleaseOutputBuffer(info, outputMode_);
            break;
        default:
            BaseState::OnMsgReceived(info);
            break;
    }
}

void HCodec::RunningState::OnCodecEvent(CodecEventType event, uint32_t data1, uint32_t data2)
{
    switch (event) {
        case CODEC_EVENT_PORT_SETTINGS_CHANGED: {
            if (data1 != OMX_DirOutput) {
                SLOGI("ignore input port changed");
                return;
            }
            if (data2 == 0 || data2 == OMX_IndexParamPortDefinition) {
                SLOGI("output port settings changed, begin to ask omx to disable out port");
                codec_->UpdateOutPortFormat();
                int32_t ret = codec_->compNode_->SendCommand(
                    CODEC_COMMAND_PORT_DISABLE, OMX_DirOutput, {});
                if (ret == HDF_SUCCESS) {
                    codec_->EraseOutBuffersOwnedByUsOrSurface();
                    codec_->ChangeStateTo(codec_->outputPortChangedState_);
                } else {
                    SLOGE("ask omx to disable out port failed");
                    codec_->SignalError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_UNKNOWN);
                }
            } else if (data2 == OMX_IndexColorAspects) {
                codec_->UpdateColorAspects();
            } else {
                SLOGI("unknown data2 0x%{public}x for CODEC_EVENT_PORT_SETTINGS_CHANGED", data2);
            }
            return;
        }
        default: {
            BaseState::OnCodecEvent(event, data1, data2);
        }
    }
}

void HCodec::RunningState::OnShutDown(const MsgInfo &info)
{
    codec_->isShutDownFromRunning_ = true;
    codec_->notifyCallerAfterShutdownComplete_ = true;
    codec_->keepComponentAllocated_ = (info.type == MsgWhat::STOP);
    codec_->isBufferCirculating_ = false;

    SLOGI("receive %{public}s msg, begin to set omx to idle", info.type == MsgWhat::RELEASE ? "release" : "stop");
    int32_t ret = codec_->compNode_->SendCommand(CODEC_COMMAND_STATE_SET, CODEC_STATE_IDLE, {});
    if (ret == HDF_SUCCESS) {
        codec_->ReplyToSyncMsgLater(info);
        codec_->ChangeStateTo(codec_->stoppingState_);
    } else {
        SLOGE("set omx to idle failed, ret=%{public}d", ret);
        ReplyErrorCode(info.id, AVCS_ERR_UNKNOWN);
    }
}

void HCodec::RunningState::OnFlush(const MsgInfo &info)
{
    codec_->isBufferCirculating_ = false;
    SLOGI("begin to ask omx to flush");
    int32_t ret = codec_->compNode_->SendCommand(CODEC_COMMAND_FLUSH, OMX_ALL, {});
    if (ret == HDF_SUCCESS) {
        codec_->ReplyToSyncMsgLater(info);
        codec_->ChangeStateTo(codec_->flushingState_);
    } else {
        SLOGI("ask omx to flush failed, ret=%{public}d", ret);
        ReplyErrorCode(info.id, AVCS_ERR_UNKNOWN);
    }
}

void HCodec::RunningState::OnSetParameters(const MsgInfo &info)
{
    Format params;
    (void)info.param->GetValue("params", params);
    ReplyErrorCode(info.id, codec_->OnSetParameters(params));
}
/**************************** RunningState End ********************************/


/**************************** OutputPortChangedState Start ********************************/
void HCodec::OutputPortChangedState::OnStateEntered()
{
    ParamSP msg = ParamBundle::Create();
    msg->SetValue("generation", codec_->stateGeneration_);
    codec_->SendAsyncMsg(MsgWhat::CHECK_IF_STUCK, msg, THREE_SECONDS_IN_US);
}

void HCodec::OutputPortChangedState::OnMsgReceived(const MsgInfo &info)
{
    switch (info.type) {
        case MsgWhat::FLUSH: {
            OnFlush(info);
            return;
        }
        case MsgWhat::START:
        case MsgWhat::SET_PARAMETERS:
        case MsgWhat::GET_INPUT_FORMAT:
        case MsgWhat::GET_OUTPUT_FORMAT: {
            codec_->DeferMessage(info);
            return;
        }
        case MsgWhat::QUEUE_INPUT_BUFFER: {
            codec_->OnQueueInputBuffer(info, inputMode_);
            return;
        }
        case MsgWhat::NOTIFY_EOS: {
            codec_->OnSignalEndOfInputStream(info);
            return;
        }
        case MsgWhat::RENDER_OUTPUT_BUFFER: {
            codec_->OnRenderOutputBuffer(info, outputMode_);
            return;
        }
        case MsgWhat::RELEASE_OUTPUT_BUFFER: {
            codec_->OnReleaseOutputBuffer(info, outputMode_);
            return;
        }
        case MsgWhat::FORCE_SHUTDOWN: {
            OnForceShutDown(info);
            return;
        }
        case MsgWhat::CHECK_IF_STUCK: {
            OnCheckIfStuck(info);
            return;
        }
        default: {
            BaseState::OnMsgReceived(info);
        }
    }
}

void HCodec::OutputPortChangedState::OnShutDown(const MsgInfo &info)
{
    if (codec_->hasFatalError_) {
        ParamSP stopMsg = ParamBundle::Create();
        stopMsg->SetValue("generation", codec_->stateGeneration_);
        codec_->SendAsyncMsg(MsgWhat::FORCE_SHUTDOWN, stopMsg, THREE_SECONDS_IN_US);
    }
    codec_->ReclaimBuffer(OMX_DirOutput, BufferOwner::OWNED_BY_USER, true);
    codec_->DeferMessage(info);
}

void HCodec::OutputPortChangedState::OnCodecEvent(CodecEventType event, uint32_t data1, uint32_t data2)
{
    switch (event) {
        case CODEC_EVENT_CMD_COMPLETE: {
            if (data1 == CODEC_COMMAND_PORT_DISABLE) {
                if (data2 != OMX_DirOutput) {
                    SLOGW("ignore input port disable complete");
                    return;
                }
                SLOGI("output port is disabled");
                HandleOutputPortDisabled();
            } else if (data1 == CODEC_COMMAND_PORT_ENABLE) {
                if (data2 != OMX_DirOutput) {
                    SLOGW("ignore input port enable complete");
                    return;
                }
                SLOGI("output port is enabled");
                HandleOutputPortEnabled();
            }
            return;
        }
        default: {
            BaseState::OnCodecEvent(event, data1, data2);
        }
    }
}

void HCodec::OutputPortChangedState::HandleOutputPortDisabled()
{
    int32_t ret = AVCS_ERR_OK;
    if (!codec_->outputBufferPool_.empty()) {
        SLOGE("output port is disabled but not empty: %{public}zu", codec_->outputBufferPool_.size());
        ret = AVCS_ERR_UNKNOWN;
    }

    if (ret == AVCS_ERR_OK) {
        SLOGI("begin to ask omx to enable out port");
        int32_t err = codec_->compNode_->SendCommand(CODEC_COMMAND_PORT_ENABLE, OMX_DirOutput, {});
        if (err == HDF_SUCCESS) {
            ret = codec_->AllocateBuffersOnPort(OMX_DirOutput);
        } else {
            SLOGE("ask omx to enable out port failed, ret=%{public}d", ret);
            ret = AVCS_ERR_UNKNOWN;
        }
    }
    if (ret != AVCS_ERR_OK) {
        codec_->SignalError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_INVALID_VAL);
    }
}

void HCodec::OutputPortChangedState::HandleOutputPortEnabled()
{
    if (codec_->isBufferCirculating_) {
        codec_->SubmitOutputBuffersToOmxNode();
    }
    SLOGI("output format changed: %{public}s", codec_->outputFormat_->Stringify().c_str());
    codec_->callback_->OnOutputFormatChanged(*(codec_->outputFormat_.get()));
    codec_->ChangeStateTo(codec_->runningState_);
}

void HCodec::OutputPortChangedState::OnFlush(const MsgInfo &info)
{
    if (codec_->hasFatalError_) {
        ParamSP stopMsg = ParamBundle::Create();
        stopMsg->SetValue("generation", codec_->stateGeneration_);
        codec_->SendAsyncMsg(MsgWhat::FORCE_SHUTDOWN, stopMsg, THREE_SECONDS_IN_US);
    }
    codec_->ReclaimBuffer(OMX_DirOutput, BufferOwner::OWNED_BY_USER, true);
    codec_->DeferMessage(info);
}
/**************************** OutputPortChangedState End ********************************/


/**************************** FlushingState Start ********************************/
void HCodec::FlushingState::OnStateEntered()
{
    flushCompleteFlag_[OMX_DirInput] = false;
    flushCompleteFlag_[OMX_DirOutput] = false;
    codec_->ReclaimBuffer(OMX_DirInput, BufferOwner::OWNED_BY_USER);
    codec_->ReclaimBuffer(OMX_DirOutput, BufferOwner::OWNED_BY_USER);
    SLOGI("all buffer owned by user are now owned by us");

    ParamSP msg = ParamBundle::Create();
    msg->SetValue("generation", codec_->stateGeneration_);
    codec_->SendAsyncMsg(MsgWhat::CHECK_IF_STUCK, msg, THREE_SECONDS_IN_US);
}

void HCodec::FlushingState::OnMsgReceived(const MsgInfo &info)
{
    switch (info.type) {
        case MsgWhat::GET_BUFFER_FROM_SURFACE: {
            codec_->DeferMessage(info);
            return;
        }
        case MsgWhat::FLUSH: {
            ReplyErrorCode(info.id, AVCS_ERR_OK);
            return;
        }
        case MsgWhat::FORCE_SHUTDOWN: {
            OnForceShutDown(info);
            return;
        }
        case MsgWhat::CHECK_IF_STUCK: {
            OnCheckIfStuck(info);
            return;
        }
        default: {
            BaseState::OnMsgReceived(info);
        }
    }
}

void HCodec::FlushingState::OnCodecEvent(CodecEventType event, uint32_t data1, uint32_t data2)
{
    switch (event) {
        case CODEC_EVENT_CMD_COMPLETE: {
            auto ret = UpdateFlushStatusOnPorts(data1, data2);
            if (ret == AVCS_ERR_OK && IsFlushCompleteOnAllPorts()) {
                ChangeStateIfWeOwnAllBuffers();
            }
            return;
        }
        case CODEC_EVENT_PORT_SETTINGS_CHANGED: {
            ParamSP portSettingChangedMsg = ParamBundle::Create();
            portSettingChangedMsg->SetValue("generation", codec_->stateGeneration_);
            portSettingChangedMsg->SetValue("event", event);
            portSettingChangedMsg->SetValue("data1", data1);
            portSettingChangedMsg->SetValue("data2", data2);
            codec_->DeferMessage(MsgInfo {MsgWhat::CODEC_EVENT, 0, portSettingChangedMsg});
            SLOGI("deferring CODEC_EVENT_PORT_SETTINGS_CHANGED");
            return;
        }
        default: {
            BaseState::OnCodecEvent(event, data1, data2);
        }
    }
}

int32_t HCodec::FlushingState::UpdateFlushStatusOnPorts(uint32_t data1, uint32_t data2)
{
    if (data2 == OMX_DirInput || data2 == OMX_DirOutput) {
        if (flushCompleteFlag_[data2]) {
            SLOGE("flush already completed for port (%{public}u)", data2);
            return AVCS_ERR_OK;
        }
        flushCompleteFlag_[data2] = true;
    } else if (data2 == OMX_ALL) {
        if (!IsFlushCompleteOnAllPorts()) {
            SLOGW("received flush complete event for OMX_ALL, portFlushStatue=(%{public}d/%{public}d)",
                flushCompleteFlag_[OMX_DirInput], flushCompleteFlag_[OMX_DirOutput]);
            return AVCS_ERR_INVALID_VAL;
        }
    } else {
        SLOGW("unexpected data2(%{public}d) for CODEC_COMMAND_FLUSH complete", data2);
    }
    return AVCS_ERR_OK;
}

bool HCodec::FlushingState::IsFlushCompleteOnAllPorts()
{
    return flushCompleteFlag_[OMX_DirInput] && flushCompleteFlag_[OMX_DirOutput];
}

void HCodec::FlushingState::ChangeStateIfWeOwnAllBuffers()
{
    if (!IsFlushCompleteOnAllPorts() || !codec_->IsAllBufferOwnedByUsOrSurface()) {
        return;
    }
    MsgInfo msg {MsgWhat::FLUSH, 0, nullptr};
    if (codec_->GetFirstSyncMsgToReply(msg)) {
        ReplyErrorCode(msg.id, AVCS_ERR_OK);
    }
    codec_->inputPortEos_ = false;
    codec_->outputPortEos_ = false;
    codec_->ChangeStateTo(codec_->runningState_);
}

void HCodec::FlushingState::OnShutDown(const MsgInfo &info)
{
    codec_->DeferMessage(info);
    if (codec_->hasFatalError_) {
        ParamSP stopMsg = ParamBundle::Create();
        stopMsg->SetValue("generation", codec_->stateGeneration_);
        codec_->SendAsyncMsg(MsgWhat::FORCE_SHUTDOWN, stopMsg, THREE_SECONDS_IN_US);
    }
}
/**************************** FlushingState End ********************************/


/**************************** StoppingState Start ********************************/
void HCodec::StoppingState::OnStateEntered()
{
    omxNodeInIdleState_ = false;
    omxNodeIsChangingToLoadedState_ = false;
    codec_->ReclaimBuffer(OMX_DirInput, BufferOwner::OWNED_BY_USER);
    codec_->ReclaimBuffer(OMX_DirOutput, BufferOwner::OWNED_BY_USER);
    SLOGI("all buffer owned by user are now owned by us");

    ParamSP msg = ParamBundle::Create();
    msg->SetValue("generation", codec_->stateGeneration_);
    codec_->SendAsyncMsg(MsgWhat::CHECK_IF_STUCK, msg, THREE_SECONDS_IN_US);
}

void HCodec::StoppingState::OnMsgReceived(const MsgInfo &info)
{
    switch (info.type) {
        case MsgWhat::CHECK_IF_STUCK: {
            int32_t generation;
            (void)info.param->GetValue("generation", generation);
            if (generation == codec_->stateGeneration_) {
                SLOGE("stucked, force state transition");
                codec_->ReclaimBuffer(OMX_DirInput, BufferOwner::OWNED_BY_OMX);
                codec_->ReclaimBuffer(OMX_DirOutput, BufferOwner::OWNED_BY_OMX);
                SLOGI("all buffer owned by omx are now owned by us");
                ChangeOmxNodeToLoadedState(true);
                codec_->ChangeStateTo(codec_->initializedState_);
            }
            return;
        }
        default: {
            BaseState::OnMsgReceived(info);
        }
    }
}

void HCodec::StoppingState::OnCodecEvent(CodecEventType event, uint32_t data1, uint32_t data2)
{
    switch (event) {
        case CODEC_EVENT_CMD_COMPLETE: {
            if (data1 != (uint32_t)CODEC_COMMAND_STATE_SET) {
                SLOGW("unexpected CODEC_EVENT_CMD_COMPLETE: %{public}u %{public}u", data1, data2);
                return;
            }
            if (data2 == (uint32_t)CODEC_STATE_IDLE) {
                SLOGI("omx now idle");
                omxNodeInIdleState_ = true;
                ChangeStateIfWeOwnAllBuffers();
            } else if (data2 == (uint32_t)CODEC_STATE_LOADED) {
                SLOGI("omx now loaded");
                codec_->ChangeStateTo(codec_->initializedState_);
            }
            return;
        }
        default: {
            BaseState::OnCodecEvent(event, data1, data2);
        }
    }
}

void HCodec::StoppingState::ChangeStateIfWeOwnAllBuffers()
{
    if (omxNodeInIdleState_ && codec_->IsAllBufferOwnedByUsOrSurface()) {
        ChangeOmxNodeToLoadedState(false);
    } else {
        SLOGD("cannot change state yet");
    }
}

void HCodec::StoppingState::ChangeOmxNodeToLoadedState(bool forceToFreeBuffer)
{
    if (!omxNodeIsChangingToLoadedState_) {
        SLOGI("begin to set omx to loaded");
        int32_t ret = codec_->compNode_->SendCommand(CODEC_COMMAND_STATE_SET, CODEC_STATE_LOADED, {});
        if (ret == HDF_SUCCESS) {
            omxNodeIsChangingToLoadedState_ = true;
        } else {
            SLOGE("set omx to loaded failed, ret=%{public}d", ret);
        }
    }
    if (forceToFreeBuffer || omxNodeIsChangingToLoadedState_) {
        codec_->ClearBufferPool(OMX_DirInput);
        codec_->ClearBufferPool(OMX_DirOutput);
        return;
    }
    codec_->SignalError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_UNKNOWN);
}

void HCodec::StoppingState::OnShutDown(const MsgInfo &info)
{
    codec_->DeferMessage(info);
}

/**************************** StoppingState End ********************************/
}