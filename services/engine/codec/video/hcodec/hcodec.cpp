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
#include "syspara/parameters.h" // base/startup/init/interfaces/innerkits/include/
#include "qos.h"
#include "utils/hdf_base.h"
#include "codec_omx_ext.h"
#include "hcodec_list.h"
#include "hencoder.h"
#include "hdecoder.h"
#include "hcodec_log.h"
#include "hcodec_dfx.h"
#include "hcodec_utils.h"
#include "av_hardware_memory.h"
#include "av_hardware_allocator.h"
#include "av_shared_memory_ext.h"
#include "av_shared_allocator.h"
#include "av_surface_memory.h"
#include "av_surface_allocator.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::HDI::Codec::V3_0;
using namespace Media;

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
            LOGE("unsupported role %d", cap.role);
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
        LOGE("cannot find %s", name.c_str());
        return nullptr;
    }
    if (codec->InitWithName(name) != AVCS_ERR_OK) {
        return nullptr;
    }
    return codec;
}

int32_t HCodec::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    HLOGI(">>");
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue("callback", callback);
    };
    return DoSyncCall(MsgWhat::SET_CALLBACK, proc);
}

int32_t HCodec::Configure(const Format &format)
{
    SCOPED_TRACE();
    HLOGI("%s", format.Stringify().c_str());
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
    SCOPED_TRACE();
    FUNC_TRACKER();
    return DoSyncCall(MsgWhat::START, nullptr);
}

int32_t HCodec::Stop()
{
    SCOPED_TRACE();
    FUNC_TRACKER();
    return DoSyncCall(MsgWhat::STOP, nullptr);
}

int32_t HCodec::Flush()
{
    SCOPED_TRACE();
    FUNC_TRACKER();
    return DoSyncCall(MsgWhat::FLUSH, nullptr);
}

int32_t HCodec::Reset()
{
    SCOPED_TRACE();
    FUNC_TRACKER();
    string previouslyConfiguredName = componentName_;
    int32_t ret = Release();
    if (ret == AVCS_ERR_OK) {
        ret = InitWithName(previouslyConfiguredName);
    }
    return ret;
}

int32_t HCodec::Release()
{
    SCOPED_TRACE();
    FUNC_TRACKER();
    return DoSyncCall(MsgWhat::RELEASE, nullptr);
}

int32_t HCodec::NotifyEos()
{
    HLOGI(">>");
    return DoSyncCall(MsgWhat::NOTIFY_EOS, nullptr);
}

int32_t HCodec::SetParameter(const Format &format)
{
    HLOGI("%s", format.Stringify().c_str());
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
    ParamSP reply;
    int32_t ret = DoSyncCallAndGetReply(MsgWhat::GET_OUTPUT_FORMAT, nullptr, reply);
    if (ret != AVCS_ERR_OK) {
        HLOGE("failed to get output format");
        return ret;
    }
    IF_TRUE_RETURN_VAL_WITH_MSG(!reply->GetValue("format", format),
        AVCS_ERR_UNKNOWN, "output format not replied");
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, componentName_);
    format.PutIntValue("IS_VENDOR", 1);
    return AVCS_ERR_OK;
}

std::string HCodec::GetHidumperInfo()
{
    HLOGI(">>");
    ParamSP reply;
    int32_t ret = DoSyncCallAndGetReply(MsgWhat::GET_HIDUMPER_INFO, nullptr, reply);
    if (ret != AVCS_ERR_OK) {
        HLOGW("failed to get hidumper info");
        return "";
    }
    string info;
    IF_TRUE_RETURN_VAL_WITH_MSG(!reply->GetValue("hidumper-info", info),
        "", "hidumper info not replied");
    return info;
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

int32_t HCodec::QueueInputBuffer(uint32_t index)
{
    std::function<void(ParamSP)> proc = [&](ParamSP msg) {
        msg->SetValue(BUFFER_ID, index);
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
    debugMode_ = HiLogIsLoggable(0xD002B32, "HCODEC", LOG_DEBUG);
    string dumpModeStr = OHOS::system::GetParameter("hcodec.dump", "0");
    dumpMode_ = static_cast<DumpMode>(strtoul(dumpModeStr.c_str(), nullptr, 2)); // 2 is binary
    LOGI(">> debug mode = %d, dump mode = %s(%lu)",
        debugMode_, dumpModeStr.c_str(), dumpMode_);

    string isEncoderStr = isEncoder ? "enc." : "dec.";
    switch (static_cast<int>(codingType_)) {
        case OMX_VIDEO_CodingAVC:
            shortName_ = isEncoderStr + "avc";
            break;
        case CODEC_OMX_VIDEO_CodingHEVC:
            shortName_ = isEncoderStr + "hevc";
            break;
        default:
            shortName_ = isEncoderStr;
            break;
    };

    uninitializedState_ = make_shared<UninitializedState>(this);
    initializedState_ = make_shared<InitializedState>(this);
    startingState_ = make_shared<StartingState>(this);
    runningState_ = make_shared<RunningState>(this);
    outputPortChangedState_ = make_shared<OutputPortChangedState>(this);
    stoppingState_ = make_shared<StoppingState>(this);
    flushingState_ = make_shared<FlushingState>(this);
    StateMachine::ChangeStateTo(uninitializedState_);
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
    LOGI("event = %d, data1 = %u, data2 = %u", event, info.data1, info.data2);
    ParamSP msg = make_shared<ParamBundle>();
    msg->SetValue("event", event);
    msg->SetValue("data1", info.data1);
    msg->SetValue("data2", info.data2);
    codec_->SendAsyncMsg(MsgWhat::CODEC_EVENT, msg);
    return HDF_SUCCESS;
}

int32_t HCodec::HdiCallback::EmptyBufferDone(int64_t appData, const OmxCodecBuffer& buffer)
{
    ParamSP msg = make_shared<ParamBundle>();
    msg->SetValue(BUFFER_ID, buffer.bufferId);
    codec_->SendAsyncMsg(MsgWhat::OMX_EMPTY_BUFFER_DONE, msg);
    return HDF_SUCCESS;
}

int32_t HCodec::HdiCallback::FillBufferDone(int64_t appData, const OmxCodecBuffer& buffer)
{
    ParamSP msg = make_shared<ParamBundle>();
    msg->SetValue("omxBuffer", buffer);
    codec_->SendAsyncMsg(MsgWhat::OMX_FILL_BUFFER_DONE, msg);
    return HDF_SUCCESS;
}

int32_t HCodec::SetFrameRateAdaptiveMode(const Format &format)
{
    if (!format.ContainKey(OHOS::Media::Tag::VIDEO_FRAME_RATE_ADAPTIVE_MODE)) {
        return AVCS_ERR_UNKNOWN;
    }

    WorkingFrequencyParam param {};
    InitOMXParamExt(param);
    if (!GetParameter(OMX_IndexParamWorkingFrequency, param)) {
        HLOGW("get working freq param failed");
        return AVCS_ERR_UNKNOWN;
    }
    if (param.level == 0) {
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("level cnt is %d, set level to %d", param.level, param.level - 1);
    param.level = param.level - 1;

    if (!SetParameter(OMX_IndexParamWorkingFrequency, param)) {
        HLOGW("set working freq param failed");
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

int32_t HCodec::SetProcessName(const Format &format)
{
    std::string processName;
    if (!format.GetStringValue(OHOS::Media::Tag::PROCESS_NAME, processName)) {
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("processName name is %s", processName.c_str());

    ProcessNameParam param {};
    InitOMXParamExt(param);
    if (strcpy_s(param.processName, sizeof(param.processName), processName.c_str()) != EOK) {
        HLOGW("strcpy failed");
        return AVCS_ERR_UNKNOWN;
    }
    if (!SetParameter(OMX_IndexParamProcessName, param)) {
        HLOGW("set process name failed");
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

int32_t HCodec::SetLowLatency(const Format &format)
{
    int32_t enableLowLatency;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENABLE_LOW_LATENCY, enableLowLatency)) {
        return AVCS_ERR_OK;
    }

    OMX_CONFIG_BOOLEANTYPE param {};
    InitOMXParam(param);
    param.bEnabled = enableLowLatency ? OMX_TRUE : OMX_FALSE;
    if (!SetParameter(OMX_IndexParamLowLatency, param)) {
        HLOGW("set low latency failed");
        return AVCS_ERR_UNKNOWN;
    }
    HLOGI("set low latency succ %d", enableLowLatency);
    return AVCS_ERR_OK;
}

bool HCodec::GetPixelFmtFromUser(const Format &format)
{
    optional<PixelFmt> fmt;
    VideoPixelFormat innerFmt;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, *(int *)&innerFmt) &&
        innerFmt != VideoPixelFormat::SURFACE_FORMAT) {
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
    HLOGI("configured pixel format is %s", configuredFmt_.strFmt.c_str());
    return true;
}

std::optional<double> HCodec::GetFrameRateFromUser(const Format &format)
{
    double frameRateDouble;
    if (format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRateDouble) && frameRateDouble > 0) {
        LOGI("user set frame rate %.2f", frameRateDouble);
        return frameRateDouble;
    }
    int frameRateInt;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRateInt) && frameRateInt > 0) {
        LOGI("user set frame rate %d", frameRateInt);
        return static_cast<double>(frameRateInt);
    }
    return nullopt;
}

int32_t HCodec::SetVideoPortInfo(OMX_DIRTYPE portIndex, const PortInfo& info)
{
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
        // we dont set eColorFormat here because it has been set by CodecVideoPortFormatParam
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
    
    return (portIndex == OMX_DirInput) ? UpdateInPortFormat() : UpdateOutPortFormat();
}

void HCodec::PrintPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& def)
{
    const OMX_VIDEO_PORTDEFINITIONTYPE& video = def.format.video;
    HLOGI("----- %s port definition -----", (def.nPortIndex == OMX_DirInput) ? "INPUT" : "OUTPUT");
    HLOGI("bEnabled %d, bPopulated %d", def.bEnabled, def.bPopulated);
    HLOGI("nBufferCountActual %u, nBufferSize %u", def.nBufferCountActual, def.nBufferSize);
    HLOGI("nFrameWidth x nFrameHeight (%u x %u), framerate %u(%.2f)",
        video.nFrameWidth, video.nFrameHeight, video.xFramerate, video.xFramerate / FRAME_RATE_COEFFICIENT);
    HLOGI("    nStride x nSliceHeight (%u x %u)", video.nStride, video.nSliceHeight);
    HLOGI("eCompressionFormat %d(%#x), eColorFormat %d(%#x)",
        video.eCompressionFormat, video.eCompressionFormat, video.eColorFormat, video.eColorFormat);
    HLOGI("----------------------------------");
}

int32_t HCodec::GetPortDefinition(OMX_DIRTYPE portIndex, OMX_PARAM_PORTDEFINITIONTYPE& def)
{
    InitOMXParam(def);
    def.nPortIndex = portIndex;
    if (!GetParameter(OMX_IndexParamPortDefinition, def)) {
        HLOGE("get %s port definition failed", (portIndex == OMX_DirInput ? "input" : "output"));
        return AVCS_ERR_INVALID_VAL;
    }
    if (def.nBufferSize == 0 || def.nBufferSize > MAX_HCODEC_BUFFER_SIZE) {
        HLOGE("invalid nBufferSize %u", def.nBufferSize);
        return AVCS_ERR_INVALID_VAL;
    }
    PrintPortDefinition(def);
    return AVCS_ERR_OK;
}

int32_t HCodec::AllocateAvLinearBuffers(OMX_DIRTYPE portIndex)
{
    SCOPED_TRACE();
    OMX_PARAM_PORTDEFINITIONTYPE def;
    int32_t ret = GetPortDefinition(portIndex, def);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    SupportBufferType type;
    InitOMXParamExt(type);
    type.portIndex = portIndex;
    if (GetParameter(OMX_IndexParamSupportBufferType, type) && (type.bufferTypes & CODEC_BUFFER_TYPE_DMA_MEM_FD)) {
        HLOGI("allocate hardware buffer");
        return AllocateAvHardwareBuffers(portIndex, def);
    } else {
        HLOGI("allocate shared buffer");
        return AllocateAvSharedBuffers(portIndex, def);
    }
}

int32_t HCodec::AllocateAvHardwareBuffers(OMX_DIRTYPE portIndex, const OMX_PARAM_PORTDEFINITIONTYPE& def)
{
    SCOPED_TRACE();
    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    pool.clear();
    for (uint32_t i = 0; i < def.nBufferCountActual; ++i) {
        std::shared_ptr<OmxCodecBuffer> omxBuffer = std::make_shared<OmxCodecBuffer>();
        omxBuffer->size = sizeof(OmxCodecBuffer);
        omxBuffer->version.version.majorVersion = 1;
        omxBuffer->bufferType = CODEC_BUFFER_TYPE_DMA_MEM_FD;
        omxBuffer->fd = -1;
        omxBuffer->allocLen = def.nBufferSize;
        omxBuffer->fenceFd = -1;
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t ret = compNode_->AllocateBuffer(portIndex, *omxBuffer, *outBuffer);
        if (ret != HDF_SUCCESS) {
            HLOGE("Failed to AllocateBuffer on %s port", (portIndex == OMX_DirInput ? "input" : "output"));
            return AVCS_ERR_INVALID_VAL;
        }
        MemoryFlag memFlag = MEMORY_READ_WRITE;
        std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateHardwareAllocator(
            outBuffer->fd, static_cast<int32_t>(def.nBufferSize), memFlag, isSecure_);
        IF_TRUE_RETURN_VAL_WITH_MSG(avAllocator == nullptr, AVCS_ERR_INVALID_VAL, "CreateHardwareAllocator failed");

        std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(
            avAllocator, static_cast<int32_t>(def.nBufferSize));
        if (avBuffer == nullptr || avBuffer->memory_ == nullptr ||
            avBuffer->memory_->GetCapacity() != static_cast<int32_t>(def.nBufferSize)) {
            HLOGE("CreateAVBuffer failed");
            return AVCS_ERR_NO_MEMORY;
        }
        BufferInfo bufInfo;
        bufInfo.isInput        = (portIndex == OMX_DirInput) ? true : false;
        bufInfo.owner          = BufferOwner::OWNED_BY_US;
        bufInfo.surfaceBuffer  = nullptr;
        bufInfo.avBuffer       = avBuffer;
        bufInfo.omxBuffer      = outBuffer;
        bufInfo.bufferId       = outBuffer->bufferId;
        bufInfo.CleanUpUnusedInfo();
        pool.push_back(bufInfo);
    }
    return AVCS_ERR_OK;
}

int32_t HCodec::AllocateAvSharedBuffers(OMX_DIRTYPE portIndex, const OMX_PARAM_PORTDEFINITIONTYPE& def)
{
    SCOPED_TRACE();
    MemoryFlag memFlag = MEMORY_READ_WRITE;
    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(memFlag);
    IF_TRUE_RETURN_VAL_WITH_MSG(avAllocator == nullptr, AVCS_ERR_INVALID_VAL, "CreateSharedAllocator failed");

    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    pool.clear();
    for (uint32_t i = 0; i < def.nBufferCountActual; ++i) {
        std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(avAllocator,
                                                                      static_cast<int32_t>(def.nBufferSize));
        if (avBuffer == nullptr || avBuffer->memory_ == nullptr ||
            avBuffer->memory_->GetCapacity() != static_cast<int32_t>(def.nBufferSize)) {
            HLOGE("CreateAVBuffer failed");
            return AVCS_ERR_NO_MEMORY;
        }
        std::shared_ptr<OmxCodecBuffer> omxBuffer = std::make_shared<OmxCodecBuffer>();
        omxBuffer->size = sizeof(OmxCodecBuffer);
        omxBuffer->version.version.majorVersion = 1;
        omxBuffer->bufferType = CODEC_BUFFER_TYPE_AVSHARE_MEM_FD;
        omxBuffer->fd = avBuffer->memory_->GetFileDescriptor();
        omxBuffer->allocLen = def.nBufferSize;
        omxBuffer->fenceFd = -1;
        omxBuffer->type = (portIndex == OMX_DirInput) ? READ_ONLY_TYPE : READ_WRITE_TYPE;
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t ret = compNode_->UseBuffer(portIndex, *omxBuffer, *outBuffer);
        if (ret != HDF_SUCCESS) {
            HLOGE("Failed to UseBuffer on %s port", (portIndex == OMX_DirInput ? "input" : "output"));
            return AVCS_ERR_INVALID_VAL;
        }
        BufferInfo bufInfo;
        bufInfo.isInput        = (portIndex == OMX_DirInput) ? true : false;
        bufInfo.owner          = BufferOwner::OWNED_BY_US;
        bufInfo.surfaceBuffer  = nullptr;
        bufInfo.avBuffer       = avBuffer;
        bufInfo.omxBuffer      = outBuffer;
        bufInfo.bufferId       = outBuffer->bufferId;
        pool.push_back(bufInfo);
    }
    return AVCS_ERR_OK;
}

int32_t HCodec::AllocateAvSurfaceBuffers(OMX_DIRTYPE portIndex)
{
    SCOPED_TRACE();
    OMX_PARAM_PORTDEFINITIONTYPE def;
    int32_t ret = GetPortDefinition(portIndex, def);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSurfaceAllocator(requestCfg_);
    IF_TRUE_RETURN_VAL_WITH_MSG(avAllocator == nullptr, AVCS_ERR_INVALID_VAL, "CreateSurfaceAllocator failed");
    bool needDealWithCache = (requestCfg_.usage & BUFFER_USAGE_MEM_MMZ_CACHE);

    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    pool.clear();
    for (uint32_t i = 0; i < def.nBufferCountActual; ++i) {
        std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(avAllocator,
                                                                      static_cast<int32_t>(def.nBufferSize));
        if (avBuffer == nullptr || avBuffer->memory_ == nullptr) {
            HLOGE("CreateAVBuffer failed");
            return AVCS_ERR_NO_MEMORY;
        }
        sptr<SurfaceBuffer> surfaceBuffer = avBuffer->memory_->GetSurfaceBuffer();
        IF_TRUE_RETURN_VAL_WITH_MSG(surfaceBuffer == nullptr, AVCS_ERR_INVALID_VAL, "avbuffer has null surfacebuffer");
        shared_ptr<OmxCodecBuffer> omxBuffer = isEncoder_ ?
            DynamicSurfaceBufferToOmxBuffer() : SurfaceBufferToOmxBuffer(surfaceBuffer);
        IF_TRUE_RETURN_VAL(omxBuffer == nullptr, AVCS_ERR_INVALID_VAL);
        shared_ptr<OmxCodecBuffer> outBuffer = make_shared<OmxCodecBuffer>();
        int32_t hdiRet = compNode_->UseBuffer(portIndex, *omxBuffer, *outBuffer);
        if (hdiRet != HDF_SUCCESS) {
            HLOGE("Failed to UseBuffer on %s port", (portIndex == OMX_DirInput ? "input" : "output"));
            return AVCS_ERR_INVALID_VAL;
        }
        BufferInfo bufInfo;
        bufInfo.isInput        = (portIndex == OMX_DirInput) ? true : false;
        bufInfo.owner          = BufferOwner::OWNED_BY_US;
        bufInfo.surfaceBuffer  = surfaceBuffer;
        bufInfo.avBuffer       = avBuffer;
        bufInfo.omxBuffer      = outBuffer;
        bufInfo.bufferId       = outBuffer->bufferId;
        bufInfo.needDealWithCache = needDealWithCache;
        pool.push_back(bufInfo);
    }

    return AVCS_ERR_OK;
}

shared_ptr<OmxCodecBuffer> HCodec::SurfaceBufferToOmxBuffer(const sptr<SurfaceBuffer>& surfaceBuffer)
{
    BufferHandle* bufferHandle = surfaceBuffer->GetBufferHandle();
    IF_TRUE_RETURN_VAL_WITH_MSG(bufferHandle == nullptr, nullptr, "surfacebuffer has null bufferhandle");
    auto omxBuffer = std::make_shared<OmxCodecBuffer>();
    omxBuffer->size = sizeof(OmxCodecBuffer);
    omxBuffer->version.version.majorVersion = 1;
    omxBuffer->bufferType = CODEC_BUFFER_TYPE_HANDLE;
    omxBuffer->bufferhandle = new NativeBuffer(bufferHandle);
    omxBuffer->fd = -1;
    omxBuffer->allocLen = surfaceBuffer->GetSize();
    omxBuffer->fenceFd = -1;
    return omxBuffer;
}

shared_ptr<OmxCodecBuffer> HCodec::DynamicSurfaceBufferToOmxBuffer()
{
    auto omxBuffer = make_shared<OmxCodecBuffer>();
    omxBuffer->size = sizeof(OmxCodecBuffer);
    omxBuffer->version.version.majorVersion = 1;
    omxBuffer->bufferType = CODEC_BUFFER_TYPE_DYNAMIC_HANDLE;
    omxBuffer->fd = -1;
    omxBuffer->allocLen = 0;
    omxBuffer->fenceFd = -1;
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

void HCodec::BufferInfo::CleanUpUnusedInfo()
{
    if (omxBuffer == nullptr || omxBuffer->fd < 0) {
        return;
    }
    if (omxBuffer->fd == 0) {
        LOGW("fd of omxbuffer should never be 0");
    }
    close(omxBuffer->fd);
    omxBuffer->fd = -1;
}

void HCodec::BufferInfo::BeginCpuAccess()
{
    if (surfaceBuffer && needDealWithCache) {
        GSError err = surfaceBuffer->InvalidateCache();
        if (err != GSERROR_OK) {
            LOGW("InvalidateCache failed, GSError=%d", err);
        }
    }
}

void HCodec::BufferInfo::EndCpuAccess()
{
    if (surfaceBuffer && needDealWithCache) {
        GSError err = surfaceBuffer->Map();
        if (err != GSERROR_OK) {
            LOGW("Map failed, GSError=%d", err);
            return;
        }
        err = surfaceBuffer->FlushCache();
        if (err != GSERROR_OK) {
            LOGW("FlushCache failed, GSError=%d", err);
        }
    }
}

HCodec::BufferInfo* HCodec::FindBufferInfoByID(OMX_DIRTYPE portIndex, uint32_t bufferId)
{
    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    for (BufferInfo &info : pool) {
        if (info.bufferId == bufferId) {
            return &info;
        }
    }
    HLOGE("unknown buffer id %u", bufferId);
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
    HLOGE("unknown buffer id %u", bufferId);
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
    }
    if (omxFlag & OMX_BUFFERFLAG_CODECCONFIG) {
        flags |= AVCODEC_BUFFER_FLAG_CODEC_DATA;
    }
    return static_cast<AVCodecBufferFlag>(flags);
}

bool HCodec::WaitFence(const sptr<SyncFence>& fence)
{
    if (fence == nullptr || !fence->IsValid()) {
        return true;
    }
    SCOPED_TRACE();
    auto before = chrono::steady_clock::now();
    int waitRes = fence->Wait(WAIT_FENCE_MS);
    if (waitRes == 0) {
        int64_t costMs = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - before).count();
        if (costMs >= WARN_FENCE_MS) {
            HLOGW("wait fence succ but cost %" PRId64 " ms", costMs);
        }
        return true;
    } else {
        HLOGE("wait fence time out, cost more than %" PRId64 " ms", WAIT_FENCE_MS);
        return false;
    }
}

void HCodec::NotifyUserToFillThisInBuffer(BufferInfo &info)
{
    SCOPED_TRACE_WITH_ID(info.bufferId);
    callback_->OnInputBufferAvailable(info.bufferId, info.avBuffer);
    ChangeOwner(info, BufferOwner::OWNED_BY_USER);
}

void HCodec::OnQueueInputBuffer(const MsgInfo &msg, BufferOperationMode mode)
{
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
    bufferInfo->omxBuffer->filledLen = static_cast<uint32_t>
        (bufferInfo->avBuffer->memory_->GetSize());
    bufferInfo->omxBuffer->offset = static_cast<uint32_t>(bufferInfo->avBuffer->memory_->GetOffset());
    bufferInfo->omxBuffer->pts = bufferInfo->avBuffer->pts_;
    bufferInfo->omxBuffer->flag = UserFlagToOmxFlag(static_cast<AVCodecBufferFlag>(bufferInfo->avBuffer->flag_));
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
    SCOPED_TRACE_WITH_ID(info.bufferId);
    info.Dump(compUniqueStr_, dumpMode_, isEncoder_);
    info.EndCpuAccess();
    int32_t ret = compNode_->EmptyThisBuffer(*(info.omxBuffer));
    if (ret != HDF_SUCCESS) {
        HLOGE("EmptyThisBuffer failed");
        return AVCS_ERR_UNKNOWN;
    }
    ChangeOwner(info, BufferOwner::OWNED_BY_OMX);
    return AVCS_ERR_OK;
}

int32_t HCodec::NotifyOmxToFillThisOutBuffer(BufferInfo& info)
{
    SCOPED_TRACE_WITH_ID(info.bufferId);
    info.omxBuffer->flag = 0;
    int32_t ret = compNode_->FillThisBuffer(*(info.omxBuffer));
    if (ret != HDF_SUCCESS) {
        HLOGE("outBufId = %u failed", info.bufferId);
        return AVCS_ERR_UNKNOWN;
    }
    ChangeOwner(info, BufferOwner::OWNED_BY_OMX);
    return AVCS_ERR_OK;
}

void HCodec::OnOMXFillBufferDone(const OmxCodecBuffer& omxBuffer, BufferOperationMode mode)
{
    SCOPED_TRACE_WITH_ID(omxBuffer.bufferId);
    optional<size_t> idx = FindBufferIndexByID(OMX_DirOutput, omxBuffer.bufferId);
    if (!idx.has_value()) {
        return;
    }
    BufferInfo& info = outputBufferPool_[idx.value()];
    if (info.owner != BufferOwner::OWNED_BY_OMX) {
        HLOGE("wrong ownership: buffer id=%d, owner=%s", info.bufferId, ToString(info.owner));
        return;
    }
    info.omxBuffer->offset = omxBuffer.offset;
    info.omxBuffer->filledLen = omxBuffer.filledLen;
    info.omxBuffer->pts = omxBuffer.pts;
    info.omxBuffer->flag = omxBuffer.flag;
    ChangeOwner(info, BufferOwner::OWNED_BY_US);
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
            NotifyUserOutBufferAvaliable(info);
            if (eos) {
                outputPortEos_ = true;
            }
            return;
        }
        case FREE_BUFFER:
            EraseBufferFromPool(OMX_DirOutput, bufferIdx);
            EraseOutBuffersOwnedByOmx(info.bufferId);
            return;
        default:
            HLOGE("SHOULD NEVER BE HERE");
            return;
    }
}

void HCodec::NotifyUserOutBufferAvaliable(BufferInfo &info)
{
    SCOPED_TRACE_WITH_ID(info.bufferId);
    if (!gotFirstOutput_) {
        LOGI("decrease thread priority");
        OHOS::QOS::ResetThreadQos();
        gotFirstOutput_ = true;
    }
    info.BeginCpuAccess();
    info.Dump(compUniqueStr_, dumpMode_, isEncoder_);
    shared_ptr<OmxCodecBuffer> omxBuffer = info.omxBuffer;
    info.avBuffer->pts_ = omxBuffer->pts;
    info.avBuffer->flag_ = OmxFlagToUserFlag(omxBuffer->flag);
    if (info.avBuffer->memory_) {
        info.avBuffer->memory_->SetSize(static_cast<int32_t>(omxBuffer->filledLen));
        info.avBuffer->memory_->SetOffset(static_cast<int32_t>(omxBuffer->offset));
    }
    callback_->OnOutputBufferAvailable(info.bufferId, info.avBuffer);
    ChangeOwner(info, BufferOwner::OWNED_BY_USER);
}

void HCodec::OnReleaseOutputBuffer(const MsgInfo &msg, BufferOperationMode mode)
{
    uint32_t bufferId;
    (void)msg.param->GetValue(BUFFER_ID, bufferId);
    SCOPED_TRACE_WITH_ID(bufferId);
    optional<size_t> idx = FindBufferIndexByID(OMX_DirOutput, bufferId);
    if (!idx.has_value()) {
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    BufferInfo& info = outputBufferPool_[idx.value()];
    if (info.owner != BufferOwner::OWNED_BY_USER) {
        HLOGE("wrong ownership: buffer id=%d, owner=%s", bufferId, ToString(info.owner));
        ReplyErrorCode(msg.id, AVCS_ERR_INVALID_VAL);
        return;
    }
    OnReleaseOutputBuffer(info);
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

void HCodec::ReclaimBuffer(OMX_DIRTYPE portIndex, BufferOwner owner, bool erase)
{
    vector<BufferInfo>& pool = (portIndex == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;
    for (size_t i = pool.size(); i > 0;) {
        i--;
        BufferInfo& info = pool[i];
        if (info.owner == owner) {
            ChangeOwner(info, BufferOwner::OWNED_BY_US);
            if (erase) {
                EraseBufferFromPool(portIndex, i);
            }
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
    OnClearBufferPool(portIndex);
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

void HCodec::RecordOutBuffersOwnedByOmx()
{
    outBuffersOwnedByOmx_.clear();
    for (const BufferInfo& info : outputBufferPool_) {
        if (info.owner == BufferOwner::OWNED_BY_OMX) {
            outBuffersOwnedByOmx_.insert(info.bufferId);
        }
    }
}

void HCodec::EraseOutBuffersOwnedByOmx(uint32_t bufferId)
{
    outBuffersOwnedByOmx_.erase(bufferId);
}

int32_t HCodec::ForceShutdown(int32_t generation, bool isNeedNotifyCaller)
{
    if (generation != stateGeneration_) {
        HLOGE("ignoring stale force shutdown message: #%d (now #%d)",
            generation, stateGeneration_);
        return AVCS_ERR_OK;
    }
    HLOGI("force to shutdown");
    isShutDownFromRunning_ = true;
    notifyCallerAfterShutdownComplete_ = isNeedNotifyCaller;
    keepComponentAllocated_ = false;
    auto err = compNode_->SendCommand(CODEC_COMMAND_STATE_SET, CODEC_STATE_IDLE, {});
    if (err == HDF_SUCCESS) {
        ChangeStateTo(stoppingState_);
    }
    return AVCS_ERR_OK;
}

void HCodec::SignalError(AVCodecErrorType errorType, int32_t errorCode)
{
    HLOGE("fatal error happened: errType=%d, errCode=%d", errorType, errorCode);
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
    ParamSP msg = make_shared<ParamBundle>();
    IF_TRUE_RETURN_VAL_WITH_MSG(msg == nullptr, AVCS_ERR_NO_MEMORY, "out of memory");
    if (oper) {
        oper(msg);
    }
    bool ret = MsgHandleLoop::SendSyncMsg(msgType, msg, reply, FIVE_SECONDS_IN_MS);
    IF_TRUE_RETURN_VAL_WITH_MSG(!ret, AVCS_ERR_UNKNOWN, "wait msg %d time out", msgType);
    int32_t err;
    IF_TRUE_RETURN_VAL_WITH_MSG(reply == nullptr || !reply->GetValue("err", err),
        AVCS_ERR_UNKNOWN, "error code of msg %d not replied", msgType);
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
    std::tie(msg.id, msg.param) = iter->second.front();
    iter->second.pop();
    return true;
}

void HCodec::ReplyErrorCode(MsgId id, int32_t err)
{
    if (id == ASYNC_MSG_ID) {
        return;
    }
    ParamSP reply = make_shared<ParamBundle>();
    reply->SetValue("err", err);
    PostReply(id, reply);
}

void HCodec::ChangeOmxToTargetState(CodecStateType &state, CodecStateType targetState)
{
    int32_t ret = compNode_->SendCommand(CODEC_COMMAND_STATE_SET, targetState, {});
    if (ret != HDF_SUCCESS) {
        HLOGE("failed to change omx state, ret=%d", ret);
        return;
    }

    int tryCnt = 0;
    do {
        if (tryCnt++ > 10) { // try up to 10 times
            HLOGE("failed to change to state(%d), abort", targetState);
            state = CODEC_STATE_INVALID;
            break;
        }
        this_thread::sleep_for(10ms); // wait 10ms
        ret = compNode_->GetState(state);
        if (ret != HDF_SUCCESS) {
            HLOGE("failed to get omx state, ret=%d", ret);
        }
    } while (ret == HDF_SUCCESS && state != targetState && state != CODEC_STATE_INVALID);
}

bool HCodec::RollOmxBackToLoaded()
{
    CodecStateType state;
    int32_t ret = compNode_->GetState(state);
    if (ret != HDF_SUCCESS) {
        HLOGE("failed to get omx node status(ret=%d), can not perform state rollback", ret);
        return false;
    }
    HLOGI("current omx state (%d)", state);
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
            HLOGE("invalid omx state: %d", state);
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