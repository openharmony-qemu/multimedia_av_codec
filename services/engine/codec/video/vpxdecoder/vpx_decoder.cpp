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
#include <thread>
#include <malloc.h>
#include "syspara/parameters.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "avcodec_codec_name.h"
#include "vpx_decoder.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "VpxDecoderLoader"};
const char *VPX_DEC_CREATE_DECODER_FUNC_NAME = "vpx_create_decoder_api";
const char *VPX_DEC_DECODE_FRAME_FUNC_NAME = "vpx_codec_decode_api";
const char *VPX_DEC_GET_FRAME_FUNC_NAME = "vpx_codec_get_frame_api";
const char *VPX_DEC_DELETE_FUNC_NAME = "vpx_destroy_decoder_api";

const char *VP8_CODEC = "vp8";
const char *VP9_CODEC = "vp9";

constexpr struct {
    const std::string_view codecName;
    const std::string_view mimeType;
} SUPPORT_VPX_DECODER[] = {
#ifdef SUPPORT_CODEC_VP8
    {AVCodecCodecName::VIDEO_DECODER_VP8_NAME, CodecMimeType::VIDEO_VP8},
#endif
#ifdef SUPPORT_CODEC_VP9
    {AVCodecCodecName::VIDEO_DECODER_VP9_NAME, CodecMimeType::VIDEO_VP9},
#endif
};
constexpr uint32_t SUPPORT_VPX_DECODER_NUM = sizeof(SUPPORT_VPX_DECODER) / sizeof(SUPPORT_VPX_DECODER[0]);
constexpr int32_t VIDEO_INSTANCE_SIZE = 64;
constexpr int32_t VIDEO_ALIGNMENT_SIZE = 2;
constexpr int32_t VP8_MAX_WIDTH_SIZE = 1920;
constexpr int32_t VP8_MAX_HEIGHT_SIZE = 1080;
constexpr int32_t VP9_MAX_WIDTH_SIZE = 8192;
constexpr int32_t VP9_MAX_HEIGHT_SIZE = 4352;
constexpr int32_t VIDEO_MIN_WIDTH_SIZE = 4;
constexpr int32_t VIDEO_MIN_HEIGHT_SIZE = 4;
constexpr int32_t VP9_BLOCKPERSEC_SIZE = 18104320; // MaxDisplayRate / (block_width*block_height)
constexpr int32_t VP8_BLOCKPERSEC_SIZE = 489600; // MaxDisplayRate / (block_width*block_height)
constexpr int32_t VP9_FRAMERATE_MAX_SIZE = 130;
constexpr int32_t VIDEO_FRAMERATE_DEFAULT_SIZE = 60;
constexpr int32_t VP8_BLOCKPERFRAME_SIZE = 8160; // MaxPicSize / (block_width*block_height)
constexpr int32_t VP9_BLOCKPERFRAME_SIZE = 139264; // MaxPicSize / (block_width*block_height)
constexpr uint32_t DEFAULT_TRY_DECODE_TIME = 1;
constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;
using namespace OHOS::Media;

VpxDecoder::VpxDecoder(const std::string &name, const std::string &path) : VideoDecoder(name, path)
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
        }
        DecoderFuncMatch();
        AVCODEC_LOGI("Num %{public}u Decoder entered, state: Uninitialized", decInstanceID_);
    } else {
        AVCODEC_LOGE("Decoder already has %{public}d instances, cannot has more instances", VIDEO_INSTANCE_SIZE);
        isValid_ = false;
    }
}

VpxDecoder::~VpxDecoder()
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

void VpxDecoder::InitParams()
{
    vpxDecoderInputArgs_.pStream = nullptr;
    vpxDecoderInputArgs_.uiStreamLen = 0;
    vpxDecoderInputArgs_.uiTimeStamp = 0;
}

void VpxDecoder::ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal,
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

bool VpxDecoder::CheckVideoPixelFormat(VideoPixelFormat vpf)
{
    if (vpf == VideoPixelFormat::NV12 || vpf == VideoPixelFormat::NV21 || vpf == VideoPixelFormat::YUV420P) {
        return true;
    } else {
        AVCODEC_LOGE("Set parameter failed: pixel format value %{public}d invalid", vpf);
        return false;
    }
}

void VpxDecoder::ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth)
{
    if (codecName_ == AVCodecCodecName::VIDEO_DECODER_VP8_NAME) {
        if (isWidth == true) {
            ConfigureDefaultVal(format, formatKey, VIDEO_MIN_WIDTH_SIZE, VP8_MAX_WIDTH_SIZE);
        } else {
            ConfigureDefaultVal(format, formatKey, VIDEO_MIN_HEIGHT_SIZE, VP8_MAX_HEIGHT_SIZE);
        }
    } else {
        if (isWidth == true) {
            ConfigureDefaultVal(format, formatKey, VIDEO_MIN_WIDTH_SIZE, VP9_MAX_WIDTH_SIZE);
        } else {
            ConfigureDefaultVal(format, formatKey, VIDEO_MIN_HEIGHT_SIZE, VP9_MAX_HEIGHT_SIZE);
        }
    }
}

void VpxDecoder::DecoderFuncMatch()
{
    if (handle_ != nullptr) {
        std::unique_lock<std::mutex> runlock(decRunMutex_);
        vpxDecoderCreateFunc_ = reinterpret_cast<VpxCreateDecoderFuncType>(dlsym(handle_,
            VPX_DEC_CREATE_DECODER_FUNC_NAME));
        vpxDecoderFrameFunc_ = reinterpret_cast<VpxDecodeFrameFuncType>(dlsym(handle_,
            VPX_DEC_DECODE_FRAME_FUNC_NAME));
        vpxDecoderGetFrameFunc_ = reinterpret_cast<VpxGetFrameFuncType>(dlsym(handle_,
            VPX_DEC_GET_FRAME_FUNC_NAME));
        vpxDecoderDestroyFunc_ = reinterpret_cast<VpxDestroyDecoderFuncType>(dlsym(handle_,
            VPX_DEC_DELETE_FUNC_NAME));
        if (vpxDecoderCreateFunc_ == nullptr || vpxDecoderFrameFunc_ == nullptr ||
            vpxDecoderGetFrameFunc_ == nullptr || vpxDecoderDestroyFunc_ == nullptr) {
                AVCODEC_LOGE("VpxDecoder vpx DecoderFuncMatch failed!");
                ReleaseHandle();
        }
    }

    InitParams();
    renderSurface_ = std::make_shared<RenderSurface>();
}

void VpxDecoder::ReleaseHandle()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    vpxDecoderCreateFunc_ = nullptr;
    vpxDecoderFrameFunc_ = nullptr;
    vpxDecoderGetFrameFunc_ = nullptr;
    vpxDecoderDestroyFunc_ = nullptr;
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

int32_t VpxDecoder::Initialize()
{
    AVCODEC_SYNC_TRACE;
    decName_ = "VpxDecoder_["+ std::to_string(instanceId_) + "]";
    AVCODEC_LOGI("current codec name: %{public}s", decName_.c_str());
    CHECK_AND_RETURN_RET_LOG(!codecName_.empty(), AVCS_ERR_INVALID_VAL, "Init codec failed:  empty name");
    std::string_view mime;
    for (uint32_t i = 0; i < SUPPORT_VPX_DECODER_NUM; ++i) {
        if (SUPPORT_VPX_DECODER[i].codecName == codecName_) {
            mime = SUPPORT_VPX_DECODER[i].mimeType;
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

int32_t VpxDecoder::CreateDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    int32_t createRet = 0;
    if (vpxDecHandle_ == nullptr && vpxDecoderCreateFunc_ != nullptr) {
        if (codecName_ == AVCodecCodecName::VIDEO_DECODER_VP8_NAME) {
            createRet = vpxDecoderCreateFunc_(&vpxDecHandle_, VP8_CODEC);
        } else {
            createRet = vpxDecoderCreateFunc_(&vpxDecHandle_, VP9_CODEC);
        }
    }
    runLock.unlock();
    CHECK_AND_RETURN_RET_LOG(createRet == 0 && vpxDecHandle_ != nullptr, AVCS_ERR_INVALID_OPERATION,
        "vpx decoder create failed");
    return AVCS_ERR_OK;
}

void VpxDecoder::DeleteDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    if (vpxDecHandle_ != nullptr && vpxDecoderDestroyFunc_ != nullptr) {
        int ret = vpxDecoderDestroyFunc_(&vpxDecHandle_);
        if (ret != 0) {
            AVCODEC_LOGE("Error: VpxDecoder delete error: %{public}d", ret);
            if (callback_ != nullptr) {
                callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
            }
            state_ = State::ERROR;
        }
        vpxDecHandle_ = nullptr;
    }
    runLock.unlock();
}

AVPixelFormat VpxDecoder::ConvertVpxFmtToAVPixFmt(VpxImageFmt fmt)
{
    AVPixelFormat ret = AVPixelFormat::AV_PIX_FMT_NONE;
    switch (fmt) {
        case VPX_IMG_FMT_YV12:
            ret = AVPixelFormat::AV_PIX_FMT_YUV420P;
            break;
        case VPX_IMG_FMT_I420:
            ret = AVPixelFormat::AV_PIX_FMT_YUV420P;
            break;
        case VPX_IMG_FMT_I422:
            ret = AVPixelFormat::AV_PIX_FMT_YUV422P;
            break;
        case VPX_IMG_FMT_I444:
            ret = AVPixelFormat::AV_PIX_FMT_YUV444P;
            break;
        case VPX_IMG_FMT_I440:
            ret = AVPixelFormat::AV_PIX_FMT_YUV440P;
            break;
        case VPX_IMG_FMT_NV12:
            ret = AVPixelFormat::AV_PIX_FMT_NV12;
            break;
        case VPX_IMG_FMT_I42016:
            ret = AVPixelFormat::AV_PIX_FMT_YUV420P10LE;
            break;
        case VPX_IMG_FMT_I42216:
            ret = AVPixelFormat::AV_PIX_FMT_YUV422P10LE;
            break;
        case VPX_IMG_FMT_I44416:
            ret = AVPixelFormat::AV_PIX_FMT_YUV444P10LE;
            break;
        case VPX_IMG_FMT_I44016:
            ret = AVPixelFormat::AV_PIX_FMT_YUV440P10LE;
            break;
        default:
            break;
    }
    return ret;
}

void VpxDecoder::ConvertDecOutToAVFrame()
{
    if (cachedFrame_ == nullptr) {
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *p) { av_frame_free(&p); });
    }

    if (vpxDecOutputImg_ == nullptr) {
        return;
    }
    uint32_t planeY = 0;
    uint32_t planeU = 1;
    uint32_t planeV = 2;
    cachedFrame_->data[planeY] = vpxDecOutputImg_->planes[planeY]; /**< Y (Luminance) plane */
    cachedFrame_->data[planeU] = vpxDecOutputImg_->planes[planeU]; /**< U (Chroma) plane */
    cachedFrame_->data[planeV] = vpxDecOutputImg_->planes[planeV]; /**< V (Chroma) plane */

    cachedFrame_->format = static_cast<int>(ConvertVpxFmtToAVPixFmt(vpxDecOutputImg_->fmt));

    uint32_t channelY = 0;
    uint32_t channelU = 1;
    uint32_t channelV = 2;
    cachedFrame_->linesize[channelY] = static_cast<int32_t>(vpxDecOutputImg_->stride[channelY]); // 0 y channel
    cachedFrame_->linesize[channelU] = static_cast<int32_t>(vpxDecOutputImg_->stride[channelU]); // 1 u channel
    cachedFrame_->linesize[channelV] = static_cast<int32_t>(vpxDecOutputImg_->stride[channelV]); // 2 v channel

    cachedFrame_->width = static_cast<int32_t>(vpxDecOutputImg_->d_w);
    cachedFrame_->height = static_cast<int32_t>(vpxDecOutputImg_->d_h);
    cachedFrame_->pts = static_cast<int64_t>(vpxDecoderInputArgs_.uiTimeStamp);
}

void VpxDecoder::SendFrame()
{
    if (state_ == State::STOPPING || state_ == State::FLUSHING) {
        return;
    } else if (state_ != State::RUNNING || isSendEos_ ||
        codecAvailQue_->Size() == 0u || inputAvailQue_->Size() == 0u) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    uint32_t index = inputAvailQue_->Front();
    CHECK_AND_RETURN_LOG(state_ == State::RUNNING, "Not in running state");
    std::shared_ptr<CodecBuffer> &inputBuffer = buffers_[INDEX_INPUT][index];
    std::shared_ptr<AVBuffer> &inputAVBuffer = inputBuffer->avBuffer;
    if (inputAVBuffer->flag_ & AVCODEC_BUFFER_FLAG_EOS) {
        vpxDecoderInputArgs_.pStream = nullptr;
        vpxDecoderInputArgs_.uiStreamLen = 0;
        isSendEos_ = true;
        AVCODEC_LOGI("Send eos end");
    } else {
        vpxDecoderInputArgs_.pStream = inputAVBuffer->memory_->GetAddr();
        vpxDecoderInputArgs_.uiStreamLen = static_cast<UINT32>(inputAVBuffer->memory_->GetSize());
        vpxDecoderInputArgs_.uiTimeStamp = static_cast<UINT64>(inputAVBuffer->pts_);
    }

#ifdef BUILD_ENG_VERSION
    if (dumpInFile_ && dumpInFile_->is_open() && !isSendEos_) {
        dumpInFile_->write(reinterpret_cast<char*>(inputAVBuffer->memory_->GetAddr()),
                           static_cast<int32_t>(inputAVBuffer->memory_->GetSize()));
    }
#endif

    int32_t ret = 0;
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    do {
        ret = DecodeFrameOnce();
    } while (isSendEos_ && ret == 0);
    runLock.unlock();

    if (isSendEos_) {
        auto outIndex = codecAvailQue_->Front();
        std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][outIndex];
        frameBuffer->avBuffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
        FramePostProcess(buffers_[INDEX_OUTPUT][outIndex], outIndex, AVCS_ERR_OK);
        state_ = State::EOS;
    } else if (ret < 0) {
        AVCODEC_LOGE("decode frame error: ret = %{public}d", ret);
    }

    inputAvailQue_->Pop();
    inputBuffer->owner_ = Owner::OWNED_BY_USER;
    callback_->OnInputBufferAvailable(index, inputAVBuffer);
}

int32_t VpxDecoder::DecodeFrameOnce()
{
    int32_t ret = 0;
    if (vpxDecHandle_ != nullptr && vpxDecoderFrameFunc_ != nullptr && vpxDecoderGetFrameFunc_ != nullptr) {
        if (!isSendEos_) {
            ret = vpxDecoderFrameFunc_(vpxDecHandle_, vpxDecoderInputArgs_.pStream, vpxDecoderInputArgs_.uiStreamLen);
        }
    } else {
        AVCODEC_LOGW("vpxDecoderFrameFunc_ = nullptr || vpxDecHandle_ = nullptr");
        ret = -1;
    }
    if (vpxDecHandle_ != nullptr && vpxDecoderGetFrameFunc_ != nullptr) {
        vpxDecoderGetFrameFunc_(vpxDecHandle_, &vpxDecOutputImg_);
        ret = vpxDecOutputImg_ == nullptr ? -1 : 0;
    } else {
        AVCODEC_LOGW("vpxDecoderGetFrameFunc_ = nullptr || vpxDecHandle_ = nullptr");
        ret = -1;
    }
    if (ret == 0 && vpxDecOutputImg_ != nullptr) {
        ConvertDecOutToAVFrame();
#ifdef BUILD_ENG_VERSION
        DumpOutputBuffer();
#endif
        auto index = codecAvailQue_->Front();
        CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
        std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
        int32_t status = AVCS_ERR_OK;
        if (CheckFormatChange(index, cachedFrame_->width, cachedFrame_->height, bitDepth_) == AVCS_ERR_OK) {
            CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
            frameBuffer = buffers_[INDEX_OUTPUT][index];
            status = FillFrameBuffer(frameBuffer);
        } else {
            CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
            callback_->OnError(AVCODEC_ERROR_EXTEND_START, AVCS_ERR_NO_MEMORY);
            state_ = State::ERROR;
            return -1;
        }
        frameBuffer->avBuffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        FramePostProcess(frameBuffer, index, status);
    }
    return ret;
}

void VpxDecoder::FlushAllFrames()
{
    std::unique_lock<std::mutex> runlock(decRunMutex_);
    if (vpxDecoderGetFrameFunc_ != nullptr) {
        do {
            vpxDecoderGetFrameFunc_(vpxDecHandle_, &vpxDecOutputImg_);
        } while (vpxDecOutputImg_ != nullptr);
    }
    runlock.unlock();
}

int32_t VpxDecoder::CheckVpxDecLibStatus()
{
    void* handle = dlopen("libvpxdec_ohos.z.so", RTLD_LAZY);
    if (handle != nullptr) {
        auto vpxDecoderCreateFunc = reinterpret_cast<VpxCreateDecoderFuncType>(
            dlsym(handle, VPX_DEC_CREATE_DECODER_FUNC_NAME));
        auto vpxDecoderDecodeFrameFunc = reinterpret_cast<VpxDecodeFrameFuncType>(
            dlsym(handle, VPX_DEC_DECODE_FRAME_FUNC_NAME));
        auto vpxDecoderGetFrameFunc = reinterpret_cast<VpxGetFrameFuncType>(
            dlsym(handle, VPX_DEC_GET_FRAME_FUNC_NAME));
        auto vpxDecoderDestroyFunc = reinterpret_cast<VpxDestroyDecoderFuncType>(
            dlsym(handle, VPX_DEC_DELETE_FUNC_NAME));
        if (vpxDecoderCreateFunc == nullptr || vpxDecoderDecodeFrameFunc == nullptr ||
            vpxDecoderGetFrameFunc == nullptr || vpxDecoderDestroyFunc == nullptr) {
                AVCODEC_LOGE("VpxDecoder vpxFuncMatch_ failed!");
                vpxDecoderCreateFunc = nullptr;
                vpxDecoderDecodeFrameFunc = nullptr;
                vpxDecoderGetFrameFunc = nullptr;
                vpxDecoderDestroyFunc = nullptr;
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

void VpxDecoder::GetVp9CapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.width.maxVal = VP9_MAX_WIDTH_SIZE;
        capsData.height.maxVal = VP9_MAX_HEIGHT_SIZE;
        capsData.blockPerSecond.maxVal = VP9_BLOCKPERSEC_SIZE;
        capsData.blockPerFrame.maxVal = VP9_BLOCKPERFRAME_SIZE;
        capsData.frameRate.maxVal = VP9_FRAMERATE_MAX_SIZE;
        capsData.supportSwapWidthHeight = true;
        capsData.profiles = {static_cast<int32_t>(VP9_PROFILE_0), static_cast<int32_t>(VP9_PROFILE_1),
                             static_cast<int32_t>(VP9_PROFILE_2), static_cast<int32_t>(VP9_PROFILE_3)};
        std::vector<int32_t> levels;
        for (int32_t j = 0; j <= static_cast<int32_t>(VP9Level::VP9_LEVEL_6_2); ++j) {
            levels.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(VP9_PROFILE_0), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(VP9_PROFILE_1), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(VP9_PROFILE_2), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(VP9_PROFILE_3), levels));
    }
}

void VpxDecoder::GetVp8CapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.blockPerSecond.maxVal = VP8_BLOCKPERSEC_SIZE;
        capsData.blockPerFrame.maxVal = VP8_BLOCKPERFRAME_SIZE;
        capsData.width.maxVal = VP8_MAX_WIDTH_SIZE;
        capsData.height.maxVal = VP8_MAX_HEIGHT_SIZE;
        capsData.supportSwapWidthHeight = true;
    }
}

int32_t VpxDecoder::GetCodecCapability(std::vector<CapabilityData> &capaArray)
{
    CHECK_AND_RETURN_RET_LOG(CheckVpxDecLibStatus() == AVCS_ERR_OK, AVCS_ERR_UNSUPPORT,
                             "vpx decoder libs not available");
    for (uint32_t i = 0; i < SUPPORT_VPX_DECODER_NUM; ++i) {
        CapabilityData capsData;
        capsData.codecName = static_cast<std::string>(SUPPORT_VPX_DECODER[i].codecName);
        capsData.mimeType = static_cast<std::string>(SUPPORT_VPX_DECODER[i].mimeType);
        capsData.codecType = AVCODEC_TYPE_VIDEO_DECODER;
        capsData.isVendor = false;
        capsData.maxInstance = VIDEO_INSTANCE_SIZE;
        capsData.alignment.width = VIDEO_ALIGNMENT_SIZE;
        capsData.alignment.height = VIDEO_ALIGNMENT_SIZE;
        capsData.width.minVal = VIDEO_MIN_WIDTH_SIZE;
        capsData.height.minVal = VIDEO_MIN_HEIGHT_SIZE;
        capsData.frameRate.minVal = 0;
        capsData.frameRate.maxVal = VIDEO_FRAMERATE_DEFAULT_SIZE;
        capsData.blockPerFrame.minVal = 1;
        capsData.blockPerSecond.minVal = 1;
        capsData.blockSize.width = VIDEO_ALIGN_SIZE;
        capsData.blockSize.height = VIDEO_ALIGN_SIZE;
        capsData.pixFormat = {static_cast<int32_t>(VideoPixelFormat::NV12),
                              static_cast<int32_t>(VideoPixelFormat::NV21)};
        capsData.graphicPixFormat = {
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010)
        };
        if (capsData.mimeType == "video/vp9") {
            capaArray.emplace_back(capsData);
            GetVp9CapProf(capaArray);
        } else {
            capaArray.emplace_back(capsData);
            GetVp8CapProf(capaArray);
        }
    }
    return AVCS_ERR_OK;
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
