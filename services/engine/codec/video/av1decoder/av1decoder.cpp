/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include <iostream>
#include <set>
#include <thread>
#include <dlfcn.h>
#include <malloc.h>
#include "syspara/parameters.h"
#include "securec.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "utils.h"
#include "avcodec_codec_name.h"
#include "av1decoder.h"
#include <fstream>
#include <cstdarg>
#include <sstream>
#include <sys/ioctl.h>
#include <linux/dma-buf.h>

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "Av1DecoderLoader"};
const char *AV1_DEC_CREATE_DECODER_FUNC_NAME = "dav1d_create_decoder_api";
const char *AV1_DEC_DECODE_FRAME_FUNC_NAME = "dav1d_codec_decode_api";
const char *AV1_DEC_GET_FRAME_FUNC_NAME = "dav1d_codec_get_frame_api";
const char *AV1_DEC_DELETE_FUNC_NAME = "dav1d_destroy_decoder_api";

constexpr struct {
    const std::string_view codecName;
    const std::string_view mimeType;

} SUPPORT_AV1_DECODER[] = {
#ifdef SUPPORT_CODEC_AV1
    {AVCodecCodecName::VIDEO_DECODER_AV1_NAME, CodecMimeType::VIDEO_AV1},
#endif
};
constexpr uint32_t SUPPORT_AV1_DECODER_NUM = sizeof(SUPPORT_AV1_DECODER) / sizeof(SUPPORT_AV1_DECODER[0]);
constexpr int32_t VIDEO_INSTANCE_SIZE = 64;
constexpr int32_t VIDEO_ALIGNMENT_SIZE = 2;
constexpr int32_t VIDEO_MIN_SIZE = 4;
constexpr int32_t VIDEO_MAX_WIDTH_SIZE = 16384;
constexpr int32_t VIDEO_MAX_HEIGHT_SIZE = 8704;
constexpr int32_t VIDEO_BLOCKPERFRAME_MAX_SIZE = 139264; // MaxPicSize / (BlockWidth * blockHeight)
constexpr int32_t VIDEO_BLOCKPERSEC_MAX_SIZE = 16711680; // MaxDisplayRate / (BlockWidth * blockHeight)
constexpr uint32_t DEFAULT_TRY_DECODE_TIME = 1;
constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;
constexpr int32_t VIDEO_MAX_FRAMERATE = 300;
constexpr int32_t BITS_PER_PIXEL_COMPONENT_8 = 8;
constexpr int32_t BITS_PER_PIXEL_COMPONENT_10 = 10;
using namespace OHOS::Media;

Av1Decoder::Av1Decoder(const std::string &name, const std::string &path) : VideoDecoder(name, path)
{
    AVCODEC_SYNC_TRACE;
    std::unique_lock<std::mutex> lock(decoderCountMutex_);
    pid_ = getpid();
    if (!freeIDSet_.empty()) {
        decInstanceID_ = freeIDSet_[0];
        freeIDSet_.erase(freeIDSet_.begin());
        decInstanceIDSet_.push_back(decInstanceID_);
    } else if (freeIDSet_.size() + decInstanceIDSet_.size() < VIDEO_INSTANCE_SIZE) {
        decInstanceID_ = freeIDSet_.size() + decInstanceIDSet_.size();
        decInstanceIDSet_.push_back(decInstanceID_);
    } else {
        decInstanceID_ = VIDEO_INSTANCE_SIZE + 1;
    }
    lock.unlock();

    if (decInstanceID_ < VIDEO_INSTANCE_SIZE) {
        handle_ = dlopen(libPath_.c_str(), RTLD_LAZY);
        if (handle_ == nullptr) {
            AVCODEC_LOGE("Load codec failed: %{public}s", libPath_.c_str());
            isValid_ = false;
            return;
        }
        DecoderFuncMatch();
        AVCODEC_LOGI("Num %{public}u Decoder entered, state: Uninitialized", decInstanceID_);
    } else {
        AVCODEC_LOGE("Decoder already has %{public}d instances, cannot has more instances", VIDEO_INSTANCE_SIZE);
        isValid_ = false;
        return;
    }
}

Av1Decoder::~Av1Decoder()
{
    ReleaseResource();
    callback_ = nullptr;
    ReleaseHandle();
    if (decInstanceID_ < VIDEO_INSTANCE_SIZE) {
        std::lock_guard<std::mutex> lock(decoderCountMutex_);
        freeIDSet_.push_back(decInstanceID_);
        auto it = std::find(decInstanceIDSet_.begin(), decInstanceIDSet_.end(), decInstanceID_);
        if (it != decInstanceIDSet_.end()) {
            decInstanceIDSet_.erase(it);
        }
    }
#ifdef BUILD_ENG_VERSION
    if (dumpInFile_ != nullptr) {
        dumpInFile_->close();
    }
    if (dumpOutFile_ != nullptr) {
        dumpOutFile_->close();
    }
    if (dumpConvertFile_ != nullptr) {
        dumpConvertFile_->close();
    }
#endif
    mallopt(M_FLUSH_THREAD_CACHE, 0);
}

void Av1Decoder::InitParams()
{
    av1DecoderInputArgs_.pStream = nullptr;
    av1DecoderInputArgs_.uiStreamLen = 0;
    av1DecoderInputArgs_.uiTimeStamp = 0;
}

void Av1Decoder::ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal,
    int32_t maxVal)
{
    int32_t val32 = 0;
    if (format.GetIntValue(formatKey, val32) && val32 >= minVal && val32 <= maxVal) {
        format_.PutIntValue(formatKey, val32);
    } else {
        AVCODEC_LOGW("Set parameter failed: %{public}s, which minimum threshold=%{public}d, "
                     "maximum threshold=%{public}d",
                     formatKey.data(), minVal, maxVal);
    }
}

bool Av1Decoder::CheckVideoPixelFormat(VideoPixelFormat vpf)
{
    if (vpf == VideoPixelFormat::NV12 || vpf == VideoPixelFormat::NV21) {
        return true;
    } else {
        AVCODEC_LOGE("Set parameter failed: pixel format value %{public}d invalid", vpf);
        return false;
    }
}

void Av1Decoder::ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth)
{
    if (isWidth == true) {
        ConfigureDefaultVal(format, formatKey, VIDEO_MIN_SIZE, VIDEO_MAX_WIDTH_SIZE);
    } else {
        ConfigureDefaultVal(format, formatKey, VIDEO_MIN_SIZE, VIDEO_MAX_HEIGHT_SIZE);
    }
}

void Av1Decoder::DecoderFuncMatch()
{
    if (handle_ != nullptr) {
        std::unique_lock<std::mutex> runlock(decRunMutex_);
        av1DecoderCreateFunc_ = reinterpret_cast<Av1CreateDecoderFuncType>(dlsym(handle_,
            AV1_DEC_CREATE_DECODER_FUNC_NAME));
        av1DecoderFrameFunc_ = reinterpret_cast<Av1DecodeFrameFuncType>(dlsym(handle_,
            AV1_DEC_DECODE_FRAME_FUNC_NAME));
        av1DecoderGetFrameFunc_ = reinterpret_cast<Av1GetFrameFuncType>(dlsym(handle_,
            AV1_DEC_GET_FRAME_FUNC_NAME));
        av1DecoderDestroyFunc_ = reinterpret_cast<Av1DestroyDecoderFuncType>(dlsym(handle_,
            AV1_DEC_DELETE_FUNC_NAME));
        if (av1DecoderCreateFunc_ == nullptr || av1DecoderFrameFunc_ == nullptr ||
            av1DecoderGetFrameFunc_ == nullptr || av1DecoderDestroyFunc_ == nullptr) {
                AVCODEC_LOGE("Av1Decoder av1 DecoderFuncMatch failed!");
                ReleaseHandle();
        }
    }

    InitParams();
    renderSurface_ = std::make_shared<RenderSurface>();
}

void Av1Decoder::ReleaseHandle()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    av1DecoderCreateFunc_ = nullptr;
    av1DecoderFrameFunc_ = nullptr;
    av1DecoderGetFrameFunc_ = nullptr;
    av1DecoderDestroyFunc_ = nullptr;
    if (handle_ != nullptr) {
        dlclose(handle_);
        handle_ = nullptr;
    }
    runLock.unlock();

    if (sInfo_.surface != nullptr) {
        UnRegisterListenerToSurface(sInfo_.surface);
        StopRequestSurfaceBufferThread();
    }
}

int32_t Av1Decoder::Initialize()
{
    AVCODEC_SYNC_TRACE;
    decName_ = "Av1Decoder_["+ std::to_string(instanceId_) + "]";
    AVCODEC_LOGI("current codec name: %{public}s", decName_.c_str());
    CHECK_AND_RETURN_RET_LOG(!codecName_.empty(), AVCS_ERR_INVALID_VAL, "Init codec failed:  empty name");
    std::string_view mime;
    for (uint32_t i = 0; i < SUPPORT_AV1_DECODER_NUM; ++i) {
        if (SUPPORT_AV1_DECODER[i].codecName == codecName_) {
            mime = SUPPORT_AV1_DECODER[i].mimeType;
            break;
        }
    }
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, mime);
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, codecName_);
    decInfo_.mimeType = mime;
    sendTask_ = std::make_shared<TaskThread>("SendFrame");
    sendTask_->RegisterHandler([this] { SendFrame(); });
#ifdef BUILD_ENG_VERSION
    OpenDumpFile();
#endif
    state_ = State::INITIALIZED;
    AVCODEC_LOGI("Init codec successful,  state: Uninitialized -> Initialized");
    return AVCS_ERR_OK;
}

int32_t Av1Decoder::CreateDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    int32_t createRet = 0;
    if (av1DecHandle_ == nullptr && av1DecoderCreateFunc_ != nullptr) {
        createRet = av1DecoderCreateFunc_(&av1DecHandle_);
    }
    runLock.unlock();
    CHECK_AND_RETURN_RET_LOG(createRet == 0 && av1DecHandle_ != nullptr, AVCS_ERR_INVALID_OPERATION,
                             "av1 decoder create failed");
    return AVCS_ERR_OK;
}

void Av1Decoder::DeleteDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    if (av1DecHandle_ != nullptr && av1DecoderDestroyFunc_ != nullptr) {
        av1DecoderDestroyFunc_(&av1DecHandle_);
        if (callback_ != nullptr) {
            callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
            state_ = State::ERROR;
        }
    }
    av1DecHandle_ = nullptr;
    runLock.unlock();
}

AVPixelFormat Av1Decoder::ConvertAv1FmtToAVPixFmt(Dav1dPixelLayout fmt, int32_t bpc)
{
    AVPixelFormat ret = AVPixelFormat::AV_PIX_FMT_NONE;
    if (bpc == BITS_PER_PIXEL_COMPONENT_8) {
        switch (fmt) {
            case DAV1D_PIXEL_LAYOUT_I400:
                ret = AVPixelFormat::AV_PIX_FMT_GRAY8;
                break;
            case DAV1D_PIXEL_LAYOUT_I420:
                ret = AVPixelFormat::AV_PIX_FMT_YUV420P;
                break;
            case DAV1D_PIXEL_LAYOUT_I422:
                ret = AVPixelFormat::AV_PIX_FMT_YUV422P;
                break;
            case DAV1D_PIXEL_LAYOUT_I444:
                ret = AVPixelFormat::AV_PIX_FMT_YUV444P;
                break;
            default:
                break;
        }
    } else if (bpc == BITS_PER_PIXEL_COMPONENT_10) {
        switch (fmt) {
            case DAV1D_PIXEL_LAYOUT_I400:
                ret = AVPixelFormat::AV_PIX_FMT_GRAY10LE;
                break;
            case DAV1D_PIXEL_LAYOUT_I420:
                ret = AVPixelFormat::AV_PIX_FMT_YUV420P10LE;
                break;
            case DAV1D_PIXEL_LAYOUT_I422:
                ret = AVPixelFormat::AV_PIX_FMT_YUV422P10LE;
                break;
            case DAV1D_PIXEL_LAYOUT_I444:
                ret = AVPixelFormat::AV_PIX_FMT_YUV444P10LE;
                break;
            default:
                break;
        }
    }
    return ret;
}

void Av1Decoder::ConvertDecOutToAVFrame()
{
    if (cachedFrame_ == nullptr) {
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *p) { av_frame_free(&p); });
    }

    if (av1DecOutputImg_ == nullptr) {
        return;
    }
    uint32_t planeY = 0;
    uint32_t planeU = 1;
    uint32_t planeV = 2;
    cachedFrame_->data[planeY] = static_cast<uint8_t*>(av1DecOutputImg_->data[planeY]); // Y
    cachedFrame_->data[planeU] = static_cast<uint8_t*>(av1DecOutputImg_->data[planeU]); // U
    cachedFrame_->data[planeV] = static_cast<uint8_t*>(av1DecOutputImg_->data[planeV]); // V

    cachedFrame_->format =
        static_cast<int>(ConvertAv1FmtToAVPixFmt(av1DecOutputImg_->p.layout, av1DecOutputImg_->p.bpc));
    uint32_t channelY = 0;
    uint32_t channelUV = 1;

    cachedFrame_->linesize[0] = static_cast<int32_t>(av1DecOutputImg_->stride[channelY]); // 0 y channel
    cachedFrame_->linesize[1] = static_cast<int32_t>(av1DecOutputImg_->stride[channelUV]); // 1 u channel
    cachedFrame_->linesize[2] = static_cast<int32_t>(av1DecOutputImg_->stride[channelUV]); // 2 v channel

    cachedFrame_->width = static_cast<int32_t>(av1DecOutputImg_->p.w);
    cachedFrame_->height = static_cast<int32_t>(av1DecOutputImg_->p.h);
}

void Av1Decoder::SendFrame()
{
    if (state_ == State::STOPPING || state_ == State::FLUSHING) {
        return;
    } else if (state_ != State::RUNNING || isSendEos_ || codecAvailQue_->Size() == 0u) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    uint32_t index = inputAvailQue_->Front();
    CHECK_AND_RETURN_LOG(state_ == State::RUNNING, "Not in running state");
    std::shared_ptr<CodecBuffer> &inputBuffer = buffers_[INDEX_INPUT][index];
    std::shared_ptr<AVBuffer> &inputAVBuffer = inputBuffer->avBuffer;
    if (inputAVBuffer->flag_ & AVCODEC_BUFFER_FLAG_EOS) {
        av1DecoderInputArgs_.pStream = nullptr;
        av1DecoderInputArgs_.uiStreamLen = 0;
        isSendEos_ = true;
        AVCODEC_LOGI("Send eos end");
    } else {
        av1DecoderInputArgs_.pStream = inputAVBuffer->memory_->GetAddr();
        av1DecoderInputArgs_.uiStreamLen = static_cast<UINT32>(inputAVBuffer->memory_->GetSize());
        av1DecoderInputArgs_.uiTimeStamp = static_cast<UINT64>(inputAVBuffer->pts_);
    }

#ifdef BUILD_ENG_VERSION
    if (dumpInFile_ && dumpInFile_->is_open() && !isSendEos_) {
        dumpInFile_->write(reinterpret_cast<char*>(inputAVBuffer->memory_->GetAddr()),
                           static_cast<int32_t>(inputAVBuffer->memory_->GetSize()));
    }
#endif

    int32_t ret = 0;
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    ret = DecodeFrameOnce();
    runLock.unlock();

    if (isSendEos_) {
        auto outIndex = codecAvailQue_->Front();
        std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][outIndex];
        frameBuffer->avBuffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
        FramePostProcess(buffers_[INDEX_OUTPUT][outIndex], outIndex, AVCS_ERR_OK, AVCS_ERR_OK);
        state_ = State::EOS;
    } else if (ret < 0) {
        AVCODEC_LOGE("decode frame error: ret = %{public}d", ret);
    }

    inputAvailQue_->Pop();
    inputBuffer->owner_ = Owner::OWNED_BY_USER;
    callback_->OnInputBufferAvailable(index, inputAVBuffer);
}

int32_t Av1Decoder::DecodeFrameOnce()
{
    int32_t ret = 0;
    Dav1dPicture pic = { 0 };
    av1DecOutputImg_ = &pic;
    if (av1DecHandle_ != nullptr && av1DecoderFrameFunc_ != nullptr) {
        if (!isSendEos_) {
            ret = av1DecoderFrameFunc_(av1DecHandle_, av1DecoderInputArgs_.pStream, av1DecoderInputArgs_.uiStreamLen);
        }
    } else {
        AVCODEC_LOGW("av1DecoderFrameFunc_ = nullptr || av1DecHandle_ = nullptr");
        ret = -1;
    }
    if (av1DecHandle_ != nullptr && av1DecoderGetFrameFunc_ != nullptr) {
        if (!isSendEos_) {
            ret = av1DecoderGetFrameFunc_(av1DecHandle_, &av1DecOutputImg_);
        }
    } else {
        ret = -1;
    }
    if (ret == 0 && av1DecOutputImg_ != nullptr) {
        ConvertDecOutToAVFrame();
#ifdef BUILD_ENG_VERSION
        DumpOutputBuffer();
#endif
        auto index = codecAvailQue_->Front();
        CHECK_AND_RETURN_RET_LOG(state_ != State::STOPPING && state_ != State::ERROR, -1, "Not in running state");
        std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
        int32_t status = AVCS_ERR_OK;
        if (CheckFormatChange(index, cachedFrame_->width, cachedFrame_->height) == AVCS_ERR_OK) {
            CHECK_AND_RETURN_RET_LOG(state_ != State::STOPPING && state_ != State::ERROR, -1, "Not in running state");
            frameBuffer = buffers_[INDEX_OUTPUT][index];
            status = FillFrameBuffer(frameBuffer);
        } else {
            CHECK_AND_RETURN_RET_LOG(state_ != State::STOPPING && state_ != State::ERROR, -1, "Not in running state");
            callback_->OnError(AVCODEC_ERROR_EXTEND_START, AVCS_ERR_NO_MEMORY);
            return -1;
        }
        frameBuffer->avBuffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        FramePostProcess(frameBuffer, index, status, AVCS_ERR_OK);
    }
    return ret;
}

int32_t Av1Decoder::CheckAv1DecLibStatus()
{
    void* handle = dlopen("libdav1d.z.so", RTLD_LAZY);
    if (handle != nullptr) {
        auto av1DecoderCreateFunc = reinterpret_cast<Av1CreateDecoderFuncType>(
            dlsym(handle, AV1_DEC_CREATE_DECODER_FUNC_NAME));
        auto av1DecoderDecodeFrameFunc = reinterpret_cast<Av1DecodeFrameFuncType>(
            dlsym(handle, AV1_DEC_DECODE_FRAME_FUNC_NAME));
        auto av1DecoderGetFrameFunc = reinterpret_cast<Av1GetFrameFuncType>(
            dlsym(handle, AV1_DEC_GET_FRAME_FUNC_NAME));
        auto av1DecoderDestroyFunc = reinterpret_cast<Av1DestroyDecoderFuncType>(
            dlsym(handle, AV1_DEC_DELETE_FUNC_NAME));
        if (av1DecoderCreateFunc == nullptr || av1DecoderDecodeFrameFunc == nullptr ||
            av1DecoderGetFrameFunc == nullptr || av1DecoderDestroyFunc == nullptr) {
                AVCODEC_LOGE("Av1Decoder av1FuncMatch_ failed!");
                av1DecoderCreateFunc = nullptr;
                av1DecoderDecodeFrameFunc = nullptr;
                av1DecoderGetFrameFunc = nullptr;
                av1DecoderDestroyFunc = nullptr;
                dlclose(handle);
                handle = nullptr;
        }
    }
    if (handle == nullptr) {
        return AVCS_ERR_UNSUPPORT;
    }
    dlclose(handle);
    handle = nullptr;

    return AVCS_ERR_OK;
}

int32_t Av1Decoder::GetCodecCapability(std::vector<CapabilityData> &capaArray)
{
    CHECK_AND_RETURN_RET_LOG(CheckAv1DecLibStatus() == AVCS_ERR_OK, AVCS_ERR_UNSUPPORT,
                             "av1 decoder libs not available");
    //todo
    for (uint32_t i = 0; i < SUPPORT_AV1_DECODER_NUM; ++i) {
        CapabilityData capsData;
        capsData.codecName = static_cast<std::string>(SUPPORT_AV1_DECODER[i].codecName);
        capsData.mimeType = static_cast<std::string>(SUPPORT_AV1_DECODER[i].mimeType);
        capsData.codecType = AVCODEC_TYPE_VIDEO_DECODER;
        capsData.isVendor = false;
        capsData.maxInstance = VIDEO_INSTANCE_SIZE;
        capsData.alignment.width = VIDEO_ALIGNMENT_SIZE;
        capsData.alignment.height = VIDEO_ALIGNMENT_SIZE;
        capsData.width.minVal = VIDEO_MIN_SIZE;
        capsData.width.maxVal = VIDEO_MAX_WIDTH_SIZE;
        capsData.height.minVal = VIDEO_MIN_SIZE;
        capsData.height.maxVal = VIDEO_MAX_HEIGHT_SIZE;
        capsData.frameRate.minVal = 0;
        capsData.frameRate.maxVal = VIDEO_MAX_FRAMERATE;
        capsData.blockPerFrame.minVal = 1;
        capsData.blockPerFrame.maxVal = VIDEO_BLOCKPERFRAME_MAX_SIZE;
        capsData.blockPerSecond.minVal = 1;
        capsData.blockPerSecond.maxVal = VIDEO_BLOCKPERSEC_MAX_SIZE;
        capsData.blockSize.width = VIDEO_ALIGN_SIZE;
        capsData.blockSize.height = VIDEO_ALIGN_SIZE;
        capsData.pixFormat = {
            static_cast<int32_t>(VideoPixelFormat::NV12),
            static_cast<int32_t>(VideoPixelFormat::NV21)
        };
        capsData.graphicPixFormat = {
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010)
        };
        capsData.profiles = {
            static_cast<int32_t>(AV1_PROFILE_MAIN),
            static_cast<int32_t>(AV1_PROFILE_HIGH),
            static_cast<int32_t>(AV1_PROFILE_PROFESSIONAL)
        };
        std::vector<int32_t> levels;
        for (int32_t j = 0; j <= static_cast<int32_t>(AV1Level::AV1_LEVEL_73); ++j) {
            levels.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AV1_PROFILE_MAIN), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AV1_PROFILE_HIGH), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AV1_PROFILE_PROFESSIONAL), levels));
        capaArray.emplace_back(capsData);
    }
    return AVCS_ERR_OK;
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
