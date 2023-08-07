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

int32_t HCodec::QueueInputBuffer(uint32_t index, const AVCodecBufferInfo &info, AVCodecBufferFlag flag)
{
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue(BUFFER_ID, index);
        msg->SetValue("buffer-info", info);
        msg->SetValue("buffer-flag", flag);
    };
    return DoSyncCall(MsgWhat::QUEUE_INPUT_BUFFER, proc);
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
    InitCreationTime();
    debugMode_ = OHOS::system::GetBoolParameter("hcodec.debug", false);
    dumpMode_ = static_cast<DumpMode>(OHOS::system::GetIntParameter<int>("hcodec.dump", DUMP_NONE));
    LOGI(">> debug mode = %{public}s, dump input = %{public}s, dump output = %{public}s",
        debugMode_ ? "true" : "false",
        (dumpMode_ & DUMP_INPUT) ? "true" : "false",
        (dumpMode_ & DUMP_OUTPUT) ? "true" : "false");

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
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, *(int*)&innerFmt) &&
        innerFmt != SURFACE_FORMAT) {
        fmt = TypeConverter::InnerFmtToFmt(innerFmt);
    } else {
        HLOGI("user don't set VideoPixelFormat, use default");
        for (int32_t f : caps_.port.video.supportPixFmts) {
            fmt = TypeConverter::GraphicFmtToFmt(static_cast<GraphicPixelFormat>(f));
            if (fmt.has_value()) {
                break;
            }
        }
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
        bufInfo.isInput        = (portIndex == OMX_DirInput) ? true : false;
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

const char* HCodec::ToString(BufferOwner owner)
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

void HCodec::ChangeOwner(BufferInfo& info, BufferOwner targetOwner, bool printInfo)
{
    if (!debugMode_) {
        info.owner = targetOwner;
        return;
    }
    auto now = chrono::steady_clock::now();
    if (info.lastOwnerChangeTime) {
        uint64_t costUs = chrono::duration_cast<chrono::microseconds>(now - info.lastOwnerChangeTime.value()).count();
        double costMs = static_cast<double>(costUs) / TIME_RATIO_MS_TO_US;
        const char* id = info.isInput ? "inBufId" : "outBufId";
        const char* oldOwner = ToString(info.owner);
        const char* newOwner = ToString(targetOwner);
        if (printInfo) {
            HLOGD("%{public}s = %{public}u, %{public}s (hold %{public}.1f ms) -> %{public}s, "
                  "len = %{public}u, flags = 0x%{public}x, pts = %{public}" PRId64 "",
                  id, info.bufferId, oldOwner, costMs, newOwner,
                  info.omxBuffer->filledLen, info.omxBuffer->flag, info.omxBuffer->pts);
        } else {
            HLOGD("%{public}s = %{public}u, %{public}s (hold %{public}.1f ms) -> %{public}s",
                  id, info.bufferId, oldOwner, costMs, newOwner);
        }
    }
    info.lastOwnerChangeTime = now;
    info.owner = targetOwner;
}

bool HCodec::BufferInfo::IsValidFrame() const
{
    if (omxBuffer->flag & OMX_BUFFERFLAG_EOS) {
        return false;
    }
    if (omxBuffer->flag & OMX_BUFFERFLAG_CODECCONFIG) {
        return false;
    }
    if (omxBuffer->filledLen == 0) {
        return false;
    }
    return true;
}

void HCodec::BufferInfo::Dump(const string& prefix, DumpMode dumpMode,
    const optional<PortInfo>& bufferFormat) const
{
    if ((dumpMode & DUMP_INPUT) && isInput) {
        Dump(prefix + "_Input", bufferFormat);
    }
    if ((dumpMode & DUMP_OUTPUT) && !isInput) {
        Dump(prefix + "_Output", bufferFormat);
    }
}

void HCodec::BufferInfo::Dump(const string& prefix, const optional<PortInfo>& bufferFormat) const
{
    DumpSurfaceBuffer(prefix);
    DumpAshmemBuffer(prefix, bufferFormat);
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
    HLOGI("------------INPUT-----------");
    for (const BufferInfo& info : inputBufferPool_) {
        HLOGI("inBufId = %{public}u, owner = %{public}s", info.bufferId, ToString(info.owner));
    }
    HLOGI("----------------------------");
    HLOGI("------------OUTPUT----------");
    for (const BufferInfo& info : outputBufferPool_) {
        HLOGI("outBufId = %{public}u, owner = %{public}s", info.bufferId, ToString(info.owner));
    }
    HLOGI("----------------------------");
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

uint32_t HCodec::UserFlagToOmxFlag(AVCodecBufferFlag userFlag)
{
    uint32_t flags = 0;
    if (userFlag & AVCODEC_BUFFER_FLAG_EOS) {
        flags |= OMX_BUFFERFLAG_EOS;
        HLOGI("got input eos");
    }
    if (userFlag & AVCODEC_BUFFER_FLAG_SYNC_FRAME) {
        flags |= OMX_BUFFERFLAG_SYNCFRAME;
        HLOGI("got input sync frame");
    }
    if (userFlag & AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        flags |= OMX_BUFFERFLAG_CODECCONFIG;
        HLOGI("got input codec config data");
    }
    return flags;
}

AVCodecBufferFlag HCodec::OmxFlagToUserFlag(uint32_t omxFlag)
{
    uint32_t flags = 0;
    if (omxFlag & OMX_BUFFERFLAG_EOS) {
        flags |= AVCODEC_BUFFER_FLAG_EOS;
        HLOGI("got output eos");
    }
    if (omxFlag & OMX_BUFFERFLAG_SYNCFRAME) {
        flags |= AVCODEC_BUFFER_FLAG_SYNC_FRAME;
        HLOGI("got output sync frame");
    }
    if (omxFlag & OMX_BUFFERFLAG_CODECCONFIG) {
        flags |= AVCODEC_BUFFER_FLAG_CODEC_DATA;
        HLOGI("got output codec config data");
    }
    return static_cast<AVCodecBufferFlag>(flags);
}

void HCodec::NotifyUserToFillThisInBuffer(BufferInfo &info)
{
    callback_->OnInputBufferAvailable(info.bufferId, info.sharedBuffer);
    ChangeOwner(info, BufferOwner::OWNED_BY_USER);
}

void HCodec::OnQueueInputBuffer(const MsgInfo &msg, BufferOperationMode mode)
{
    uint32_t bufferId;
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    (void)msg.param->GetValue(BUFFER_ID, bufferId);
    (void)msg.param->GetValue("buffer-info", info);
    (void)msg.param->GetValue("buffer-flag", flag);
    BufferInfo* bufferInfo = FindBufferInfoByID(OMX_DirInput, bufferId);
    if (bufferInfo == nullptr) {
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    if (bufferInfo->owner != BufferOwner::OWNED_BY_USER) {
        HLOGE("wrong ownership: buffer id=%{public}d, owner=%{public}s", bufferId, ToString(bufferInfo->owner));
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    bufferInfo->omxBuffer->filledLen = info.size;
    bufferInfo->omxBuffer->offset = info.offset;
    bufferInfo->omxBuffer->pts = info.presentationTimeUs;
    bufferInfo->omxBuffer->flag = UserFlagToOmxFlag(flag);
    ChangeOwner(*bufferInfo, BufferOwner::OWNED_BY_US);
    ReplyErrorCode(msg.id, AVCS_ERR_OK);
    OnQueueInputBuffer(mode, bufferInfo);
}

void HCodec::OnQueueInputBuffer(BufferOperationMode mode, BufferInfo* info)
{
    switch (mode) {
        case KEEP_BUFFER: {
            return;
        }
        case RESUBMIT_BUFFER: {
            if (inputPortEos_) {
                HLOGI("input already eos, keep this buffer");
                return;
            }
            bool eos = (info->omxBuffer->flag & OMX_BUFFERFLAG_EOS);
            if (!eos && info->omxBuffer->filledLen == 0) {
                HLOGI("this is not a eos buffer but not filled, ask user to re-fill it");
                NotifyUserToFillThisInBuffer(*info);
                return;
            }
            if (eos) {
                inputPortEos_ = true;
            }
            int32_t ret = NotifyOmxToEmptyThisInBuffer(*info);
            if (ret != AVCS_ERR_OK) {
                SignalError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_UNKNOWN);
            }
            return;
        }
        default: {
            HLOGE("SHOULD NEVER BE HERE");
            return;
        }
    }
}

void HCodec::OnSignalEndOfInputStream(const MsgInfo &msg)
{
    ReplyErrorCode(msg.id, AVCS_ERR_UNSUPPORT);
}

int32_t HCodec::NotifyOmxToEmptyThisInBuffer(BufferInfo& info)
{
    info.Dump(ctorTime_ + "_" + componentName_, dumpMode_, sharedBufferFormat_);
    int32_t ret = compNode_->EmptyThisBuffer(*(info.omxBuffer));
    if (ret != HDF_SUCCESS) {
        HLOGE("EmptyThisBuffer failed");
        return AVCS_ERR_UNKNOWN;
    }
    if (info.IsValidFrame()) {
        etbCnt_++;
        if (debugMode_) {
            etbMap_[info.omxBuffer->pts] = chrono::steady_clock::now();
        }
    }
    ChangeOwner(info, BufferOwner::OWNED_BY_OMX, true);
    return AVCS_ERR_OK;
}

int32_t HCodec::NotifyOmxToFillThisOutBuffer(BufferInfo& info)
{
    info.omxBuffer->flag = 0;
    int32_t ret = compNode_->FillThisBuffer(*(info.omxBuffer));
    if (ret != HDF_SUCCESS) {
        HLOGE("outBufId = %{public}u failed", info.bufferId);
        return AVCS_ERR_UNKNOWN;
    }
    ChangeOwner(info, BufferOwner::OWNED_BY_OMX);
    return AVCS_ERR_OK;
}

void HCodec::OnOMXFillBufferDone(const OmxCodecBuffer& omxBuffer, BufferOperationMode mode)
{
    optional<size_t> idx = FindBufferIndexByID(OMX_DirOutput, omxBuffer.bufferId);
    if (!idx.has_value()) {
        return;
    }
    BufferInfo& info = outputBufferPool_[idx.value()];
    if (info.owner != BufferOwner::OWNED_BY_OMX) {
        HLOGE("wrong ownership: buffer id=%{public}d, owner=%{public}s", info.bufferId, ToString(info.owner));
        return;
    }
    info.omxBuffer->offset = omxBuffer.offset;
    info.omxBuffer->filledLen = omxBuffer.filledLen;
    info.omxBuffer->pts = omxBuffer.pts;
    info.omxBuffer->flag = omxBuffer.flag;
    ChangeOwner(info, BufferOwner::OWNED_BY_US, true);
    info.Dump(ctorTime_ + "_" + componentName_, dumpMode_, sharedBufferFormat_);
    OnOMXFillBufferDone(mode, info, idx.value());
}

void HCodec::OnOMXFillBufferDone(BufferOperationMode mode, BufferInfo& info, size_t bufferIdx)
{
    switch (mode) {
        case KEEP_BUFFER:
            return;
        case RESUBMIT_BUFFER: {
            if (outputPortEos_) {
                HLOGI("output eos, keep this buffer");
                return;
            }
            bool eos = (info.omxBuffer->flag & OMX_BUFFERFLAG_EOS);
            if (!eos && info.omxBuffer->filledLen == 0) {
                HLOGI("it's not a eos buffer but not filled, ask omx to re-fill it");
                NotifyOmxToFillThisOutBuffer(info);
                return;
            }
            UpdateFbdRecord(info);
            NotifyUserOutBufferAvaliable(info);
            if (eos) {
                outputPortEos_ = true;
            }
            return;
        }
        case FREE_BUFFER:
            EraseBufferFromPool(OMX_DirOutput, bufferIdx);
            return;
        default:
            HLOGE("SHOULD NEVER BE HERE");
            return;
    }
}

void HCodec::UpdateFbdRecord(const BufferInfo& info)
{
    if (!info.IsValidFrame()) {
        return;
    }
    if (fbdCnt_ == 0) {
        firstFbdTime_ = std::chrono::steady_clock::now();
    }
    fbdCnt_++;
    if (!debugMode_) {
        return;
    }
    auto it = etbMap_.find(info.omxBuffer->pts);
    if (it != etbMap_.end()) {
        auto now = chrono::steady_clock::now();
        uint64_t fromEtbToFbd = chrono::duration_cast<chrono::microseconds>(now - it->second).count();
        etbMap_.erase(it);
        double oneFrameCostMs = static_cast<double>(fromEtbToFbd) / TIME_RATIO_MS_TO_US;
        uint64_t fromFbdToFbd = chrono::duration_cast<chrono::microseconds>(now - firstFbdTime_).count();
        totalCost_ += fromEtbToFbd;
        double averageCostMs = static_cast<double>(totalCost_) / TIME_RATIO_MS_TO_US / fbdCnt_;
        if (fromFbdToFbd == 0) {
            HLOGD("pts = %{public}" PRId64 ", cost %{public}.2f ms, average %{public}.2f ms",
            info.omxBuffer->pts, oneFrameCostMs, averageCostMs);
        } else {
            HLOGD("pts = %{public}" PRId64 ", cost %{public}.2f ms, average %{public}.2f ms, fbd fps %{public}.2f",
            info.omxBuffer->pts, oneFrameCostMs, averageCostMs,
            static_cast<double>(fbdCnt_) / fromFbdToFbd * TIME_RATIO_S_TO_US);
        }
    }
}

void HCodec::NotifyUserOutBufferAvaliable(BufferInfo &info)
{
    shared_ptr<OmxCodecBuffer> omxBuffer = info.omxBuffer;
    AVCodecBufferInfo userInfo {
        .presentationTimeUs = omxBuffer->pts,
        .size = omxBuffer->filledLen,
        .offset = omxBuffer->offset,
    };
    AVCodecBufferFlag userFlag = OmxFlagToUserFlag(omxBuffer->flag);
    callback_->OnOutputBufferAvailable(info.bufferId, userInfo, userFlag, info.sharedBuffer);
    ChangeOwner(info, BufferOwner::OWNED_BY_USER);
}

void HCodec::OnReleaseOutputBuffer(const MsgInfo &msg, BufferOperationMode mode)
{
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
    HLOGD("outBufId = %{public}u", bufferId);
    ChangeOwner(info, BufferOwner::OWNED_BY_US);
    ReplyErrorCode(msg.id, AVCS_ERR_OK);

    switch (mode) {
        case KEEP_BUFFER: {
            return;
        }
        case RESUBMIT_BUFFER: {
            if (outputPortEos_) {
                HLOGI("output eos, keep this buffer");
                return;
            }
            int32_t ret = NotifyOmxToFillThisOutBuffer(info);
            if (ret != AVCS_ERR_OK) {
                SignalError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_UNKNOWN);
            }
            return;
        }
        case FREE_BUFFER: {
            EraseBufferFromPool(OMX_DirOutput, idx.value());
            return;
        }
        default: {
            HLOGE("SHOULD NEVER BE HERE");
            return;
        }
    }
}

void HCodec::OnRenderOutputBuffer(const MsgInfo &msg, BufferOperationMode mode)
{
    ReplyErrorCode(msg.id, AVCS_ERR_UNSUPPORT);
}

void HCodec::ReclaimBuffer(OMX_DIRTYPE portIndex, BufferOwner owner)
{
    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    for (BufferInfo& info : pool) {
        if (info.owner == owner) {
            ChangeOwner(info, BufferOwner::OWNED_BY_US);
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

void HCodec::ReplyErrorCode(MsgId id, int32_t err)
{
    if (id == ASYNC_MSG_ID) {
        return;
    }
    ParamSP reply = ParamBundle::Create();
    reply->SetValue("err", err);
    PostReply(id, reply);
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

const char* HCodec::ToString(MsgWhat what)
{
    static const map<MsgWhat, const char*> m = {
        { INIT, "INIT" }, { SET_CALLBACK, "SET_CALLBACK" }, { CONFIGURE, "CONFIGURE" },
        { CREATE_INPUT_SURFACE, "CREATE_INPUT_SURFACE" }, { SET_INPUT_SURFACE, "SET_INPUT_SURFACE" },
        { SET_OUTPUT_SURFACE, "SET_OUTPUT_SURFACE" }, { START, "START" },
        { GET_INPUT_FORMAT, "GET_INPUT_FORMAT" }, { GET_OUTPUT_FORMAT, "GET_OUTPUT_FORMAT" },
        { SET_PARAMETERS, "SET_PARAMETERS" }, { REQUEST_IDR_FRAME, "REQUEST_IDR_FRAME" },
        { FLUSH, "FLUSH" }, { QUEUE_INPUT_BUFFER, "QUEUE_INPUT_BUFFER" },
        { NOTIFY_EOS, "NOTIFY_EOS" }, { RELEASE_OUTPUT_BUFFER, "RELEASE_OUTPUT_BUFFER" },
        { RENDER_OUTPUT_BUFFER, "RENDER_OUTPUT_BUFFER" }, { STOP, "STOP" }, { RELEASE, "RELEASE" },
        { CODEC_EVENT, "CODEC_EVENT" }, { OMX_EMPTY_BUFFER_DONE, "OMX_EMPTY_BUFFER_DONE" },
        { OMX_FILL_BUFFER_DONE, "OMX_FILL_BUFFER_DONE" }, { GET_BUFFER_FROM_SURFACE, "GET_BUFFER_FROM_SURFACE" },
        { CHECK_IF_STUCK, "CHECK_IF_STUCK" }, { FORCE_SHUTDOWN, "FORCE_SHUTDOWN" },
    };
    auto it = m.find(what);
    if (it != m.end()) {
        return it->second;
    }
    return "UNKNOWN";
}
} // namespace OHOS::MediaAVCodec