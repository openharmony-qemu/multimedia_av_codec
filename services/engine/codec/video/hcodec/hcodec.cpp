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

#include "hcodec.h"
#include <cassert>
#include <vector>
#include <algorithm>
#include <thread>
#include <fstream>
#include "syspara/parameters.h" // base/startup/init/interfaces/innerkits/include/
#include "utils/hdf_base.h"
#include "codec_omx_ext.h"
#include "hcodec_list.h"
#include "hencoder.h"
#include "hdecoder.h"
#include "hcodec_log.h"
#include "utils.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::HDI::Codec::V1_0;

std::shared_ptr<HCodec> HCodec::Create(const std::string &name)
{
    vector<CodecCompCapability> capList = GetCapList();
    shared_ptr<HCodec> codec;
    for (const auto& cap : capList) {
        if (cap.compName != name) {
            continue;
        }
        optional<OMX_VIDEO_CODINGTYPE> type = TypeConverter::HdiRoleToOmxCodingType(cap.role);
        if (!type) {
            LOGE("unsupported role %{public}d", cap.role);
            return nullptr;
        }
        if (cap.type == VIDEO_DECODER) {
            codec = make_shared<HDecoder>(cap, type.value());
        } else if (cap.type == VIDEO_ENCODER) {
            codec = make_shared<HEncoder>(cap, type.value());
        }
        break;
    }
    if (codec == nullptr) {
        LOGE("cannot find %{public}s", name.c_str());
        return nullptr;
    }
    if (codec->InitWithName(name) != AVCS_ERR_OK) {
        return nullptr;
    }
    return codec;
}

int32_t HCodec::SetCallback(const shared_ptr<AVCodecCallback> &callback)
{
    HLOGI(">>");
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue("callback", callback);
    };
    return DoSyncCall(MsgWhat::SET_CALLBACK, proc);
}

int32_t HCodec::Configure(const Format &format)
{
    HLOGI(">>");
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue("format", format);
    };
    return DoSyncCall(MsgWhat::CONFIGURE, proc);
}

int32_t HCodec::SetOutputSurface(sptr<Surface> surface)
{
    HLOGI(">>");
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue("surface", surface);
    };
    return DoSyncCall(MsgWhat::SET_OUTPUT_SURFACE, proc);
}

int32_t HCodec::Start()
{
    HLOGI(">>");
    return DoSyncCall(MsgWhat::START, nullptr);
}

int32_t HCodec::Stop()
{
    HLOGI(">>");
    return DoSyncCall(MsgWhat::STOP, nullptr);
}

int32_t HCodec::Flush()
{
    HLOGI(">>");
    return DoSyncCall(MsgWhat::FLUSH, nullptr);
}

int32_t HCodec::Reset()
{
    HLOGI(">>");
    string previouslyConfiguredName = componentName_;
    int32_t ret = Release();
    if (ret == AVCS_ERR_OK) {
        ret = InitWithName(previouslyConfiguredName);
    }
    return ret;
}

int32_t HCodec::Release()
{
    HLOGI(">>");
    return DoSyncCall(MsgWhat::RELEASE, nullptr);
}

int32_t HCodec::NotifyEos()
{
    HLOGI(">>");
    return DoSyncCall(MsgWhat::NOTIFY_EOS, nullptr);
}

int32_t HCodec::SetParameter(const Format &format)
{
    HLOGI(">>");
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue("params", format);
    };
    return DoSyncCall(MsgWhat::SET_PARAMETERS, proc);
}

int32_t HCodec::GetInputFormat(Format& format)
{
    HLOGI(">>");
    ParamSP reply;
    int32_t ret = DoSyncCallAndGetReply(MsgWhat::GET_INPUT_FORMAT, nullptr, reply);
    if (ret != AVCS_ERR_OK) {
        HLOGE("failed to get input format");
        return ret;
    }
    IF_TRUE_RETURN_VAL_WITH_MSG(!reply->GetValue("format", format),
        AVCS_ERR_UNKNOWN, "input format not replied");
    return AVCS_ERR_OK;
}

int32_t HCodec::GetOutputFormat(Format &format)
{
    HLOGI(">>");
    ParamSP reply;
    int32_t ret = DoSyncCallAndGetReply(MsgWhat::GET_OUTPUT_FORMAT, nullptr, reply);
    if (ret != AVCS_ERR_OK) {
        HLOGE("failed to get output format");
        return ret;
    }
    IF_TRUE_RETURN_VAL_WITH_MSG(!reply->GetValue("format", format),
        AVCS_ERR_UNKNOWN, "output format not replied");
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, componentName_);
    return AVCS_ERR_OK;
}

sptr<Surface> HCodec::CreateInputSurface()
{
    HLOGI(">>");
    ParamSP reply;
    int32_t ret = DoSyncCallAndGetReply(MsgWhat::CREATE_INPUT_SURFACE, nullptr, reply);
    if (ret != AVCS_ERR_OK) {
        HLOGE("failed to create input surface");
        return nullptr;
    }
    sptr<Surface> inputSurface;
    IF_TRUE_RETURN_VAL_WITH_MSG(!reply->GetValue("surface", inputSurface), nullptr, "input surface not replied");
    return inputSurface;
}

int32_t HCodec::SetInputSurface(sptr<Surface> surface)
{
    HLOGI(">>");
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue("surface", surface);
    };
    return DoSyncCall(MsgWhat::SET_INPUT_SURFACE, proc);
}

int32_t HCodec::SignalRequestIDRFrame()
{
    HLOGI(">>");
    return DoSyncCall(MsgWhat::REQUEST_IDR_FRAME, nullptr);
}

std::shared_ptr<AVSharedMemoryBase> HCodec::GetInputBuffer(uint32_t index)
{
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue(BUFFER_ID, index);
    };
    ParamSP reply;
    int32_t ret = DoSyncCallAndGetReply(MsgWhat::GET_INPUT_BUFFER, proc, reply);
    if (ret != AVCS_ERR_OK) {
        return nullptr;
    }
    std::shared_ptr<AVSharedMemoryBase> inputBuffer;
    IF_TRUE_RETURN_VAL_WITH_MSG(!reply->GetValue("input-buffer", inputBuffer), nullptr, "input buffer not replied");
    return inputBuffer;
}

int32_t HCodec::QueueInputBuffer(uint32_t index, const AVCodecBufferInfo &info, AVCodecBufferFlag flag)
{
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue(BUFFER_ID, index);
        msg->SetValue("buffer-info", info);
        msg->SetValue("buffer-flag", flag);
    };
    return DoSyncCall(MsgWhat::QUEUE_INPUT_BUFFER, proc);
}

std::shared_ptr<AVSharedMemoryBase> HCodec::GetOutputBuffer(uint32_t index)
{
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue(BUFFER_ID, index);
    };
    ParamSP reply;
    int32_t ret = DoSyncCallAndGetReply(MsgWhat::GET_OUTPUT_BUFFER, proc, reply);
    if (ret != AVCS_ERR_OK) {
        return nullptr;
    }
    std::shared_ptr<AVSharedMemoryBase> outputBuffer;
    IF_TRUE_RETURN_VAL_WITH_MSG(!reply->GetValue("output-buffer", outputBuffer), nullptr, "output buffer not replied");
    return outputBuffer;
}

int32_t HCodec::RenderOutputBuffer(uint32_t index)
{
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue(BUFFER_ID, index);
    };
    return DoSyncCall(MsgWhat::RENDER_OUTPUT_BUFFER, proc);
}

int32_t HCodec::ReleaseOutputBuffer(uint32_t index)
{
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue(BUFFER_ID, index);
    };
    return DoSyncCall(MsgWhat::RELEASE_OUTPUT_BUFFER, proc);
}
/**************************** public functions end ****************************/


HCodec::HCodec(CodecCompCapability caps, OMX_VIDEO_CODINGTYPE codingType, bool isEncoder)
    : caps_(caps), codingType_(codingType), isEncoder_(isEncoder)
{
    LOGI(">>");
    InitCreationTime();

    uninitializedState_ = make_shared<UninitializedState>(this);
    initializedState_ = make_shared<InitializedState>(this);
    startingState_ = make_shared<StartingState>(this);
    runningState_ = make_shared<RunningState>(this);
    outputPortChangedState_ = make_shared<OutputPortChangedState>(this);
    stoppingState_ = make_shared<StoppingState>(this);
    flushingState_ = make_shared<FlushingState>(this);
    StateMachine::ChangeStateTo(uninitializedState_);
}

void HCodec::InitCreationTime()
{
    chrono::system_clock::time_point now = chrono::system_clock::now();
    time_t nowTimeT = chrono::system_clock::to_time_t(now);
    tm *localTm = localtime(&nowTimeT);
    if (localTm == nullptr) {
        return;
    }
    char buffer[32] = "\0";
    if (strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", localTm) > 0) {
        ctorTime_ = string(buffer);
    }
}

HCodec::~HCodec()
{
    HLOGI(">>");
    MsgHandleLoop::Stop();
    ReleaseComponent();
}

int32_t HCodec::InitWithName(const std::string &name)
{
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue("name", name);
    };
    return DoSyncCall(MsgWhat::INIT, proc);
}

int32_t HCodec::HdiCallback::EventHandler(CodecEventType event, const EventInfo &info)
{
    LOGI("event = %{public}d, data1 = %{public}u, data2 = %{public}u", event, info.data1, info.data2);
    ParamSP msg = ParamBundle::Create();
    msg->SetValue("event", event);
    msg->SetValue("data1", info.data1);
    msg->SetValue("data2", info.data2);
    codec_->SendAsyncMsg(MsgWhat::CODEC_EVENT, msg);
    return HDF_SUCCESS;
}

int32_t HCodec::HdiCallback::EmptyBufferDone(int64_t appData, const OmxCodecBuffer& buffer)
{
    ParamSP msg = ParamBundle::Create();
    msg->SetValue(BUFFER_ID, buffer.bufferId);
    codec_->SendAsyncMsg(MsgWhat::OMX_EMPTY_BUFFER_DONE, msg);
    return HDF_SUCCESS;
}

int32_t HCodec::HdiCallback::FillBufferDone(int64_t appData, const OmxCodecBuffer& buffer)
{
    ParamSP msg = ParamBundle::Create();
    msg->SetValue("omxBuffer", buffer);
    codec_->SendAsyncMsg(MsgWhat::OMX_FILL_BUFFER_DONE, msg);
    return HDF_SUCCESS;
}

bool HCodec::GetPixelFmtFromUser(const Format &format)
{
    optional<PixelFmt> fmt;
    VideoPixelFormat innerFmt;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, *(int*)&innerFmt) ||
        innerFmt == SURFACE_FORMAT) {
        HLOGI("user don't set VideoPixelFormat, use default");
        if (caps_.port.video.supportPixFmts.empty()) {
            HLOGE("this codec dont have default format");
            return false;
        }
        GraphicPixelFormat graphicFmt = static_cast<GraphicPixelFormat>(caps_.port.video.supportPixFmts[0]);
        HLOGI("default GraphicPixelFormat %{public}d", graphicFmt);
        fmt = TypeConverter::GraphicFmtToFmt(graphicFmt);
    } else {
        fmt = TypeConverter::InnerFmtToFmt(innerFmt);
    }
    if (!fmt) {
        HLOGE("pixel format unspecified");
        return false;
    }
    configuredFmt_ = fmt.value();
    HLOGI("GraphicPixelFormat %{public}d, VideoPixelFormat %{public}d",
          configuredFmt_.graphicFmt, configuredFmt_.innerFmt);
    return true;
}

int32_t HCodec::SetVideoPortInfo(OMX_DIRTYPE portIndex, const PortInfo& info)
{
    {
        OMX_PARAM_PORTDEFINITIONTYPE def;
        InitOMXParam(def);
        def.nPortIndex = portIndex;
        if (!GetParameter(OMX_IndexParamPortDefinition, def)) {
            HLOGE("get port definition failed");
            return AVCS_ERR_UNKNOWN;
        }
        def.format.video.nFrameWidth = info.width;
        def.format.video.nFrameHeight = info.height;
        def.format.video.eCompressionFormat = info.codingType;
        // we dont set eColorFormat here because it will be set below
        def.format.video.xFramerate = info.frameRate * FRAME_RATE_COEFFICIENT;
        if (portIndex == OMX_DirInput && info.inputBufSize.has_value()) {
            def.nBufferSize = info.inputBufSize.value();
        }
        if (!SetParameter(OMX_IndexParamPortDefinition, def)) {
            HLOGE("set port definition failed");
            return AVCS_ERR_UNKNOWN;
        }
        if (portIndex == OMX_DirOutput) {
            if (outputFormat_ == nullptr) {
                outputFormat_ = make_shared<Format>();
            }
            outputFormat_->PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, info.frameRate);
        }
    }
    if (info.pixelFmt.has_value()) {
        CodecVideoPortFormatParam param;
        InitOMXParamExt(param);
        param.portIndex = portIndex;
        param.codecCompressFormat = info.codingType;
        param.codecColorFormat = info.pixelFmt->graphicFmt;
        param.framerate = info.frameRate * FRAME_RATE_COEFFICIENT;
        if (!SetParameter(OMX_IndexCodecVideoPortFormat, param)) {
            HLOGE("set port format failed");
            return AVCS_ERR_UNKNOWN;
        }
    }
    return (portIndex == OMX_DirInput) ? UpdateInPortFormat() : UpdateOutPortFormat();
}

void HCodec::PrintPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& def)
{
    const OMX_VIDEO_PORTDEFINITIONTYPE& video = def.format.video;
    HLOGI("----- %{public}s port definition -----", (def.nPortIndex == OMX_DirInput) ? "INPUT" : "OUTPUT");
    HLOGI("bEnabled %{public}d, bPopulated %{public}d", def.bEnabled, def.bPopulated);
    HLOGI("nBufferCountActual %{public}u, nBufferSize %{public}u", def.nBufferCountActual, def.nBufferSize);
    HLOGI("nFrameWidth x nFrameHeight (%{public}u x %{public}u), framerate %{public}u(%{public}.2f)",
        video.nFrameWidth, video.nFrameHeight, video.xFramerate, video.xFramerate / FRAME_RATE_COEFFICIENT);
    HLOGI("    nStride x nSliceHeight (%{public}u x %{public}u)", video.nStride, video.nSliceHeight);
    HLOGI("eCompressionFormat %{public}d(%{public}#x), eColorFormat %{public}d(%{public}#x)",
        video.eCompressionFormat, video.eCompressionFormat, video.eColorFormat, video.eColorFormat);
    HLOGI("----------------------------------");
}

void HCodec::NotifyUserToFillThisInputBuffer(BufferInfo &info)
{
    HLOGD("inBufId = %{public}u", info.bufferId);
    callback_->OnInputBufferAvailable(info.bufferId);
    info.owner = BufferOwner::OWNED_BY_USER;
}

std::shared_ptr<AVSharedMemoryBase> HCodec::OnUserGetInputBuffer(uint32_t bufferId)
{
    HLOGD("inBufId = %{public}u", bufferId);
    BufferInfo *info = FindBufferInfoByID(OMX_DirInput, bufferId);
    if (info == nullptr) {
        return nullptr;
    }
    if (info->owner != BufferOwner::OWNED_BY_USER) {
        HLOGE("wrong ownership: buffer id=%{public}d, owner=%{public}s", bufferId, info->Owner());
        return nullptr;
    }
    return info->sharedBuffer;
}

int32_t HCodec::OnUserQueueInputBuffer(uint32_t bufferId, const AVCodecBufferInfo &info,
    AVCodecBufferFlag flag, BufferOperationMode mode)
{
    HLOGD("inBufId = %{public}u, size = %{public}d, flags = %{public}u, pts = %{public}" PRId64 "",
        bufferId, info.size, flag, info.presentationTimeUs);
    BufferInfo* bufferInfo = FindBufferInfoByID(OMX_DirInput, bufferId);
    if (bufferInfo == nullptr) {
        return AVCS_ERR_INVALID_VAL;
    }
    if (bufferInfo->owner != BufferOwner::OWNED_BY_USER) {
        HLOGE("wrong ownership: buffer id=%{public}d, owner=%{public}s", bufferId, bufferInfo->Owner());
        return AVCS_ERR_INVALID_VAL;
    }
    if (bufferInfo->omxBuffer == nullptr) {
        HLOGE("null omx buffer");
        return AVCS_ERR_UNKNOWN;
    }
    bufferInfo->owner = BufferOwner::OWNED_BY_US;
    bool eos = (flag & AVCODEC_BUFFER_FLAG_EOS);

    switch (mode) {
        case KEEP_BUFFER: {
            return AVCS_ERR_OK;
        }
        case RESUBMIT_BUFFER: {
            if (inputPortEos_) {
                HLOGI("input already eos, keep this buffer");
                return AVCS_ERR_OK;
            }
            if (!eos && info.size == 0) {
                HLOGI("this is not a eos buffer but not filled, ask user to re-fill it");
                NotifyUserToFillThisInputBuffer(*bufferInfo);
                return AVCS_ERR_OK;
            }
            SetBufferInfoFromUser(*bufferInfo, info, flag);
            return NotifyOmxToEmptyThisInputBuffer(*bufferInfo);
        }
        default: {
            HLOGE("SHOULD NEVER BE HERE");
            return AVCS_ERR_OK;
        }
    }
}

int32_t HCodec::AllocateSharedBuffers(OMX_DIRTYPE portIndex, bool isImageData)
{
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParam(def);
    def.nPortIndex = portIndex;
    if (!GetParameter(OMX_IndexParamPortDefinition, def)) {
        HLOGE("get %{public}s port definition failed", (portIndex == OMX_DirInput ? "input" : "output"));
        return AVCS_ERR_INVALID_VAL;
    }
    if (def.nBufferSize == 0 || def.nBufferSize > MAX_HCODEC_BUFFER_SIZE) {
        HLOGE("invalid nBufferSize %{public}u", def.nBufferSize);
        return AVCS_ERR_INVALID_VAL;
    }
    HLOGI("%{public}s port definition: nBufferCountActual %{public}u, nBufferSize %{public}u",
        (portIndex == OMX_DirInput ? "input" : "output"), def.nBufferCountActual, def.nBufferSize);

    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    pool.clear();
    for (uint32_t i = 0; i < def.nBufferCountActual; ++i) {
        shared_ptr<AVSharedMemoryBase> ashm = std::static_pointer_cast<AVSharedMemoryBase>(
            AVSharedMemoryBase::CreateFromLocal(static_cast<int32_t>(def.nBufferSize),
            static_cast<int32_t>(AVSharedMemory::FLAGS_READ_WRITE), "HCodecAshmem"));
        if (ashm == nullptr || ashm->GetSize() != (int)def.nBufferSize) {
            HLOGE("allocate AVSharedMemory failed");
            return AVCS_ERR_NO_MEMORY;
        }
        shared_ptr<OmxCodecBuffer> omxBuffer = AshmemToOmxBuffer(portIndex, ashm->GetFd(), ashm->GetSize());
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t ret = compNode_->UseBuffer(portIndex, *omxBuffer, *outBuffer);
        if (ret != HDF_SUCCESS) {
            LOGE("Failed to UseBuffer on %{public}s port", (portIndex == OMX_DirInput ? "input" : "output"));
            return AVCS_ERR_INVALID_VAL;
        }
        BufferInfo bufInfo;
        bufInfo.isImageDataInSharedBuffer = isImageData;
        bufInfo.owner          = BufferOwner::OWNED_BY_US;
        bufInfo.surfaceBuffer  = nullptr;
        bufInfo.sharedBuffer   = ashm;
        bufInfo.omxBuffer      = outBuffer;
        bufInfo.bufferId       = outBuffer->bufferId;
        pool.push_back(bufInfo);
    }
    return AVCS_ERR_OK;
}

shared_ptr<OmxCodecBuffer> HCodec::AshmemToOmxBuffer(OMX_DIRTYPE portIndex, int32_t fd, uint32_t size)
{
    std::shared_ptr<OmxCodecBuffer> omxBuffer = std::make_shared<OmxCodecBuffer>();
    omxBuffer->size = sizeof(OmxCodecBuffer);
    omxBuffer->version.version.majorVersion = 1;
    omxBuffer->bufferType = CODEC_BUFFER_TYPE_AVSHARE_MEM_FD;
    omxBuffer->fd = fd;
    omxBuffer->allocLen = size;
    omxBuffer->fenceFd = -1;
    omxBuffer->pts = 0;
    omxBuffer->flag = 0;
    omxBuffer->type = (portIndex == OMX_DirInput) ? READ_ONLY_TYPE : READ_WRITE_TYPE;
    return omxBuffer;
}

const char* HCodec::BufferInfo::Owner() const
{
    switch (owner) {
        case BufferOwner::OWNED_BY_US:
            return "us";
        case BufferOwner::OWNED_BY_USER:
            return "user";
        case BufferOwner::OWNED_BY_OMX:
            return "omx";
        case BufferOwner::OWNED_BY_SURFACE:
            return "surface";
        default:
            return "";
    }
}

void HCodec::BufferInfo::Dump(const string& prefix, const std::optional<PortInfo>& bufferFormat) const
{
    if (OHOS::system::GetBoolParameter("hcodec.dump", false)) {
        DumpSurfaceBuffer(prefix);
        DumpAshmemBuffer(prefix, bufferFormat);
    }
}

void HCodec::BufferInfo::DumpSurfaceBuffer(const std::string& prefix) const
{
    if (surfaceBuffer == nullptr) {
        return;
    }
    bool eos = (omxBuffer->flag & OMX_BUFFERFLAG_EOS);
    if (eos || omxBuffer->filledLen == 0) {
        return;
    }
    int w = surfaceBuffer->GetWidth();
    int h = surfaceBuffer->GetHeight();
    int alignedW = surfaceBuffer->GetStride();
    void* va = surfaceBuffer->GetVirAddr();
    uint32_t totalSize = surfaceBuffer->GetSize();
    if (w <= 0 || h <= 0 || alignedW <= 0 || w > alignedW || va == nullptr) {
        LOGW("invalid buffer");
        return;
    }
    GraphicPixelFormat fmt = static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat());
    optional<uint32_t> assumeAlignedH;
    string suffix;
    bool dumpAsVideo = true;  // we could only save it as individual image if we don't know aligned height
    DecideDumpInfo(assumeAlignedH, suffix, dumpAsVideo);

    static char name[128];
    int ret = 0;
    if (dumpAsVideo) {
        ret = sprintf_s(name, sizeof(name), "%s/%s_%dx%d(%dx%d)_fmt%d.%s",
                        DUMP_PATH, prefix.c_str(), w, h, alignedW, assumeAlignedH.value_or(h), fmt, suffix.c_str());
    } else {
        ret = sprintf_s(name, sizeof(name), "%s/%s_%dx%d(%d)_fmt%d_pts%" PRId64 ".%s",
                        DUMP_PATH, prefix.c_str(), w, h, alignedW, fmt, omxBuffer->pts, suffix.c_str());
    }
    if (ret > 0) {
        ofstream ofs(name, ios::binary | ios::app);
        if (ofs.is_open()) {
            ofs.write(reinterpret_cast<const char*>(va), totalSize);
        } else {
            LOGW("cannot open %{public}s", name);
        }
    }
    surfaceBuffer->Unmap();
}

void HCodec::BufferInfo::DecideDumpInfo(optional<uint32_t>& assumeAlignedH, string& suffix, bool& dumpAsVideo) const
{
    int h = surfaceBuffer->GetHeight();
    int alignedW = surfaceBuffer->GetStride();
    if (alignedW <= 0) {
        return;
    }
    uint32_t totalSize = surfaceBuffer->GetSize();
    GraphicPixelFormat fmt = static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat());
    switch (fmt) {
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP: {
            suffix = "yuv";
            if (GetYuv420Size(alignedW, h) == totalSize) {
                break;
            }
            uint32_t alignedH = totalSize * 2 / 3 / alignedW; // 2 bytes per pixel for UV, 3 bytes per pixel for YUV
            if (GetYuv420Size(alignedW, alignedH) == totalSize) {
                dumpAsVideo = true;
                assumeAlignedH = alignedH;
            } else {
                dumpAsVideo = false;
            }
            break;
        }
        case GRAPHIC_PIXEL_FMT_RGBA_8888: {
            suffix = "rgba";
            if (static_cast<uint32_t>(alignedW * h) != totalSize) {
                dumpAsVideo = false;
            }
            break;
        }
        default: {
            suffix = "bin";
            dumpAsVideo = false;
            break;
        }
    }
}

void HCodec::BufferInfo::DumpAshmemBuffer(const string& prefix, const std::optional<PortInfo>& bufferFormat) const
{
    if (sharedBuffer == nullptr) {
        return;
    }
    bool eos = (omxBuffer->flag & OMX_BUFFERFLAG_EOS);
    if (eos || omxBuffer->filledLen == 0) {
        return;
    }

    static char name[128];
    int ret;
    if (isImageDataInSharedBuffer && bufferFormat.has_value()) {
        ret = sprintf_s(name, sizeof(name), "%s/%s_%dx%d(%dx%d)_fmt%d.bin",
                        DUMP_PATH, prefix.c_str(), bufferFormat->width, bufferFormat->height, bufferFormat->stride,
                        bufferFormat->height, bufferFormat->pixelFmt->graphicFmt);
    } else {
        ret = sprintf_s(name, sizeof(name), "%s/%s.bin", DUMP_PATH, prefix.c_str());
    }
    if (ret <= 0) {
        LOGW("sprintf_s failed");
        return;
    }
    ofstream ofs(name, ios::binary | ios::app);
    if (ofs.is_open()) {
        ofs.write(reinterpret_cast<const char*>(sharedBuffer->GetBase()), omxBuffer->filledLen);
    } else {
        LOGW("cannot open %{public}s", name);
    }
}

void HCodec::PrintAllBufferInfo()
{
    HLOGD("------------INPUT-----------");
    for (const BufferInfo& info : inputBufferPool_) {
        HLOGD("inBufId = %{public}u, owner = %{public}s", info.bufferId, info.Owner());
    }
    HLOGD("----------------------------");
    HLOGD("------------OUTPUT----------");
    for (const BufferInfo& info : outputBufferPool_) {
        HLOGD("outBufId = %{public}u, owner = %{public}s", info.bufferId, info.Owner());
    }
    HLOGD("----------------------------");
}

HCodec::BufferInfo* HCodec::FindBufferInfoByID(OMX_DIRTYPE portIndex, uint32_t bufferId)
{
    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    for (BufferInfo &info : pool) {
        if (info.bufferId == bufferId) {
            return &info;
        }
    }
    HLOGE("unknown buffer id %{public}u", bufferId);
    return nullptr;
}

optional<size_t> HCodec::FindBufferIndexByID(OMX_DIRTYPE portIndex, uint32_t bufferId)
{
    const vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    for (size_t i = 0; i < pool.size(); i++) {
        if (pool[i].bufferId == bufferId) {
            return i;
        }
    }
    HLOGE("unknown buffer id %{public}u", bufferId);
    return nullopt;
}

void HCodec::SetBufferInfoFromUser(BufferInfo& bufferInfo, const AVCodecBufferInfo &info, AVCodecBufferFlag flag)
{
    bufferInfo.omxBuffer->filledLen = info.size;
    bufferInfo.omxBuffer->offset = info.offset;
    bufferInfo.omxBuffer->pts    = info.presentationTimeUs;
    bufferInfo.omxBuffer->flag = 0;
    if (flag & AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        bufferInfo.omxBuffer->flag |= OMX_BUFFERFLAG_CODECCONFIG;
    }
    if (flag & AVCODEC_BUFFER_FLAG_EOS) {
        bufferInfo.omxBuffer->flag |= OMX_BUFFERFLAG_EOS;
    }
}

int32_t HCodec::NotifyOmxToEmptyThisInputBuffer(BufferInfo& bufferInfo)
{
    HLOGD("inBufId = %{public}u, filledLen = %{public}d, flags = %{public}u, pts = %{public}" PRId64 "",
        bufferInfo.bufferId, bufferInfo.omxBuffer->filledLen, bufferInfo.omxBuffer->flag, bufferInfo.omxBuffer->pts);

    uint32_t flags = bufferInfo.omxBuffer->flag;
    if (flags & OMX_BUFFERFLAG_CODECCONFIG) {
        HLOGI("this is codec specific data");
    } else if (flags & OMX_BUFFERFLAG_EOS) {
        HLOGI("this is eos data");
        inputPortEos_ = true;
    }
    bufferInfo.Dump(ctorTime_ + "_" + componentName_ + "_Input", sharedBufferFormat_);

    int32_t ret = compNode_->EmptyThisBuffer(*(bufferInfo.omxBuffer));
    if (ret != HDF_SUCCESS) {
        HLOGE("EmptyThisBuffer failed");
        return AVCS_ERR_UNKNOWN;
    }
    etbCnt_++;
    bufferInfo.owner = BufferOwner::OWNED_BY_OMX;
    return AVCS_ERR_OK;
}

int32_t HCodec::NotifyOmxToFillThisOutputBuffer(BufferInfo& info)
{
    HLOGD("outBufId = %{public}u", info.bufferId);
    info.omxBuffer->flag = 0;
    int32_t ret = compNode_->FillThisBuffer(*(info.omxBuffer));
    if (ret != HDF_SUCCESS) {
        HLOGE("outBufId = %{public}u failed", info.bufferId);
        return AVCS_ERR_UNKNOWN;
    }
    info.owner = BufferOwner::OWNED_BY_OMX;
    return AVCS_ERR_OK;
}

void HCodec::OnOMXFillBufferDone(const OmxCodecBuffer& omxBuffer, BufferOperationMode mode)
{
    bool eos = (omxBuffer.flag & OMX_BUFFERFLAG_EOS);
    HLOGD("outBufId = %{public}u, pts = %{public}" PRId64 ", eos = %{public}d, filledLen = %{public}u",
        omxBuffer.bufferId, omxBuffer.pts, eos, omxBuffer.filledLen);
    optional<size_t> idx = FindBufferIndexByID(OMX_DirOutput, omxBuffer.bufferId);
    if (!idx.has_value()) {
        return;
    }
    BufferInfo& info = outputBufferPool_[idx.value()];
    if (info.owner != BufferOwner::OWNED_BY_OMX) {
        HLOGE("wrong ownership: buffer id=%{public}d, owner=%{public}s", info.bufferId, info.Owner());
        return;
    }
    if (!eos && omxBuffer.filledLen != 0) {
        if (fbdCnt_ == 0) {
            firstFbdTime_ = std::chrono::steady_clock::now();
        }
        fbdCnt_++;
    }

    info.owner = BufferOwner::OWNED_BY_US;
    info.omxBuffer->offset = omxBuffer.offset;
    info.omxBuffer->filledLen = omxBuffer.filledLen;
    info.omxBuffer->pts = omxBuffer.pts;
    info.omxBuffer->flag = omxBuffer.flag;
    info.Dump(ctorTime_ + "_" + componentName_ + "_Output", sharedBufferFormat_);

    switch (mode) {
        case KEEP_BUFFER:
            return;
        case RESUBMIT_BUFFER: {
            if (outputPortEos_) {
                HLOGI("output eos, keep this buffer");
                return;
            }
            if (!eos && omxBuffer.filledLen == 0) {
                HLOGI("it's not a eos buffer but not filled, ask omx to re-fill it");
                NotifyOmxToFillThisOutputBuffer(info);
                return;
            }
            NotifyUserOutputBufferAvaliable(info);
            if (eos) {
                outputPortEos_ = true;
            }
            return;
        }
        case FREE_BUFFER:
            EraseBufferFromPool(OMX_DirOutput, idx.value());
            return;
        default:
            HLOGE("SHOULD NEVER BE HERE");
            return;
    }
}

void HCodec::NotifyUserOutputBufferAvaliable(BufferInfo &bufferInfo)
{
    HLOGD("outBufId = %{public}u", bufferInfo.bufferId);
    shared_ptr<OmxCodecBuffer> omxBuffer = bufferInfo.omxBuffer;
    if (omxBuffer == nullptr) {
        HLOGE("SHOULD NEVER BE HERE");
        return;
    }
    uint32_t flags = 0;
    if (omxBuffer->flag & OMX_BUFFERFLAG_SYNCFRAME) {
        flags |= AVCODEC_BUFFER_FLAG_SYNC_FRAME;
    }
    if (omxBuffer->flag & OMX_BUFFERFLAG_CODECCONFIG) {
        flags |= AVCODEC_BUFFER_FLAG_CODEC_DATA;
    }
    if (omxBuffer->flag & OMX_BUFFERFLAG_EOS) {
        flags |= AVCODEC_BUFFER_FLAG_EOS;
    }
    AVCodecBufferInfo info {
        .presentationTimeUs = omxBuffer->pts,
        .size = omxBuffer->filledLen,
        .offset = omxBuffer->offset,
    };
    callback_->OnOutputBufferAvailable(bufferInfo.bufferId, info, (AVCodecBufferFlag)flags);
    bufferInfo.owner = BufferOwner::OWNED_BY_USER;
}

std::shared_ptr<AVSharedMemoryBase> HCodec::OnUserGetOutputBuffer(uint32_t bufferId)
{
    BufferInfo *info = FindBufferInfoByID(OMX_DirOutput, bufferId);
    if (info == nullptr) {
        HLOGE("unknown buffer id %{public}u", bufferId);
        return nullptr;
    }
    if (info->owner != BufferOwner::OWNED_BY_USER) {
        HLOGE("wrong ownership: buffer id=%{public}d, owner=%{public}s", bufferId, info->Owner());
        return nullptr;
    }
    return info->sharedBuffer;
}

int32_t HCodec::OnUserReleaseOutputBuffer(uint32_t bufferId, BufferOperationMode mode)
{
    HLOGD("bufferId = %{public}u", bufferId);
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
            return NotifyOmxToFillThisOutputBuffer(info);
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

void HCodec::ReclaimBuffer(OMX_DIRTYPE portIndex, BufferOwner owner)
{
    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    for (BufferInfo& info : pool) {
        if (info.owner == owner) {
            info.owner = BufferOwner::OWNED_BY_US;
        }
    }
}

bool HCodec::IsAllBufferOwnedByUsOrSurface(OMX_DIRTYPE portIndex)
{
    const vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    for (const BufferInfo& info : pool) {
        if (info.owner != BufferOwner::OWNED_BY_US &&
            info.owner != BufferOwner::OWNED_BY_SURFACE) {
            return false;
        }
    }
    return true;
}

bool HCodec::IsAllBufferOwnedByUsOrSurface()
{
    return IsAllBufferOwnedByUsOrSurface(OMX_DirInput) &&
           IsAllBufferOwnedByUsOrSurface(OMX_DirOutput);
}

void HCodec::ClearBufferPool(OMX_DIRTYPE portIndex)
{
    const vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    for (size_t i = pool.size(); i > 0;) {
        i--;
        EraseBufferFromPool(portIndex, i);
    }
}

void HCodec::FreeOmxBuffer(OMX_DIRTYPE portIndex, const BufferInfo& info)
{
    if (compNode_ && info.omxBuffer) {
        int32_t omxRet = compNode_->FreeBuffer(portIndex, *(info.omxBuffer));
        if (omxRet != HDF_SUCCESS) {
            HLOGW("notify omx to free buffer failed");
        }
    }
}

void HCodec::EraseOutBuffersOwnedByUsOrSurface()
{
    // traverse index in reverse order because we need to erase index from vector
    for (size_t i = outputBufferPool_.size(); i > 0;) {
        i--;
        const BufferInfo& info = outputBufferPool_[i];
        if (info.owner == BufferOwner::OWNED_BY_US || info.owner == BufferOwner::OWNED_BY_SURFACE) {
            EraseBufferFromPool(OMX_DirOutput, i);
        }
    }
}

int32_t HCodec::ForceShutdown(int32_t generation)
{
    if (generation != stateGeneration_) {
        HLOGE("ignoring stale force shutdown message: #%{public}d (now #%{public}d)",
            generation, stateGeneration_);
        return AVCS_ERR_OK;
    }
    HLOGI("force to shutdown");
    isShutDownFromRunning_ = true;
    notifyCallerAfterShutdownComplete_ = false;
    keepComponentAllocated_ = false;
    auto err = compNode_->SendCommand(CODEC_COMMAND_STATE_SET, CODEC_STATE_IDLE, {});
    if (err == HDF_SUCCESS) {
        ChangeStateTo(stoppingState_);
    }
    return AVCS_ERR_OK;
}

void HCodec::SignalError(AVCodecErrorType errorType, int32_t errorCode)
{
    HLOGE("fatal error happened: errType=%{public}d, errCode=%{public}d", errorType, errorCode);
    hasFatalError_ = true;
    callback_->OnError(errorType, errorCode);
}

int32_t HCodec::DoSyncCall(MsgWhat msgType, std::function<void(ParamSP)> oper)
{
    ParamSP reply;
    return DoSyncCallAndGetReply(msgType, oper, reply);
}

int32_t HCodec::DoSyncCallAndGetReply(MsgWhat msgType, std::function<void(ParamSP)> oper, ParamSP &reply)
{
    ParamSP msg = ParamBundle::Create();
    IF_TRUE_RETURN_VAL_WITH_MSG(msg == nullptr, AVCS_ERR_NO_MEMORY, "out of memory");
    if (oper) {
        oper(msg);
    }
    bool ret = MsgHandleLoop::SendSyncMsg(msgType, msg, reply);
    IF_TRUE_RETURN_VAL_WITH_MSG(!ret, AVCS_ERR_UNKNOWN, "wait msg %{public}d time out", msgType);
    int32_t err;
    IF_TRUE_RETURN_VAL_WITH_MSG(reply == nullptr || !reply->GetValue("err", err),
        AVCS_ERR_UNKNOWN, "error code of msg %{public}d not replied", msgType);
    return err;
}

void HCodec::DeferMessage(const MsgInfo &info)
{
    deferredQueue_.push_back(info);
}

void HCodec::ProcessDeferredMessages()
{
    for (const MsgInfo &info : deferredQueue_) {
        StateMachine::OnMsgReceived(info);
    }
    deferredQueue_.clear();
}

void HCodec::ReplyToSyncMsgLater(const MsgInfo& msg)
{
    syncMsgToReply_[msg.type].push(std::make_pair(msg.id, msg.param));
}

bool HCodec::GetFirstSyncMsgToReply(MsgInfo& msg)
{
    auto iter = syncMsgToReply_.find(msg.type);
    if (iter == syncMsgToReply_.end()) {
        return false;
    }
    msg.id = iter->second.front().first;
    msg.param = iter->second.front().second;
    iter->second.pop();
    return true;
}

void HCodec::ChangeOmxToTargetState(CodecStateType &state, CodecStateType targetState)
{
    int32_t ret = compNode_->SendCommand(CODEC_COMMAND_STATE_SET, targetState, {});
    if (ret != HDF_SUCCESS) {
        HLOGE("failed to change omx state, ret=%{public}d", ret);
        return;
    }

    int tryCnt = 0;
    do {
        if (tryCnt++ > 10) { // try up to 10 times
            HLOGE("failed to change to state(%{public}d), abort", targetState);
            state = CODEC_STATE_INVALID;
            break;
        }
        this_thread::sleep_for(10ms); // wait 10ms
        ret = compNode_->GetState(state);
        if (ret != HDF_SUCCESS) {
            HLOGE("failed to get omx state, ret=%{public}d", ret);
        }
    } while (ret == HDF_SUCCESS && state != targetState && state != CODEC_STATE_INVALID);
}

bool HCodec::RollOmxBackToLoaded()
{
    CodecStateType state;
    int32_t ret = compNode_->GetState(state);
    if (ret != HDF_SUCCESS) {
        HLOGE("failed to get omx node status(ret=%{public}d), can not perform state rollback", ret);
        return false;
    }
    HLOGI("current omx state (%{public}d)", state);
    switch (state) {
        case CODEC_STATE_EXECUTING: {
            ChangeOmxToTargetState(state, CODEC_STATE_IDLE);
            [[fallthrough]];
        }
        case CODEC_STATE_IDLE: {
            ChangeOmxToTargetState(state, CODEC_STATE_LOADED);
            [[fallthrough]];
        }
        case CODEC_STATE_LOADED:
        case CODEC_STATE_INVALID: {
            return true;
        }
        default: {
            HLOGE("invalid omx state: %{public}d", state);
            return false;
        }
    }
}

void HCodec::CleanUpOmxNode()
{
    if (compNode_ == nullptr) {
        return;
    }

    if (RollOmxBackToLoaded()) {
        for (const BufferInfo& info : inputBufferPool_) {
            FreeOmxBuffer(OMX_DirInput, info);
        }
        for (const BufferInfo& info : outputBufferPool_) {
            FreeOmxBuffer(OMX_DirOutput, info);
        }
    }
}

void HCodec::ReleaseComponent()
{
    CleanUpOmxNode();
    if (compMgr_ != nullptr) {
        compMgr_->DestroyComponent(componentId_);
    }
    compNode_ = nullptr;
    compCb_ = nullptr;
    compMgr_ = nullptr;
    componentId_ = 0;
    componentName_.clear();
}
} // namespace OHOS::MediaAVCodec