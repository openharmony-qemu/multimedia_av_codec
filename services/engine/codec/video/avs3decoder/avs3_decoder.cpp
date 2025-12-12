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
#include "avs3_decoder.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "Avs3DecoderLoader"};
const char *AVS3_DEC_CREATE_DECODER_FUNC_NAME = "uavs3d_create";
const char *AVS3_DEC_DECODE_FRAME_FUNC_NAME = "uavs3d_decode";
const char *AVS3_DEC_DELETE_FUNC_NAME = "uavs3d_delete";
const char *AVS3_DEC_FLUSH_FUNC_NAME = "uavs3d_flush";
const char *AVS3_IMG_CPY_FUNC_NAME = "uavs3d_img_cpy_cvt";
// const char *AVS3_DEC_CREATE_TEST_FUNC_NAME = "uavs3d_test";

constexpr struct {
    const std::string_view codecName;
    const std::string_view mimeType;
} SUPPORT_AVS3_DECODER[] = {
    {AVCodecCodecName::VIDEO_DECODER_AVS3_NAME, CodecMimeType::VIDEO_AVS3},
};
constexpr uint32_t SUPPORT_AVS3_DECODER_NUM = sizeof(SUPPORT_AVS3_DECODER) / sizeof(SUPPORT_AVS3_DECODER[0]);
constexpr int32_t VIDEO_INSTANCE_SIZE = 64;
constexpr int32_t VIDEO_ALIGNMENT_SIZE = 2;
constexpr int32_t VIDEO_MAX_WIDTH_SIZE = 1920;
constexpr int32_t VIDEO_MAX_HEIGHT_SIZE = 1920;
constexpr int32_t VIDEO_MIN_WIDTH_SIZE = 4;
constexpr int32_t VIDEO_MIN_HEIGHT_SIZE = 4;
constexpr int32_t VIDEO_FRAMERATE_DEFAULT_SIZE = 60;
constexpr uint32_t DEFAULT_TRY_DECODE_TIME = 1;
constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;
using namespace OHOS::Media;

Avs3Decoder::Avs3Decoder(const std::string &name, const std::string &path) : VideoDecoder(name, path)
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

Avs3Decoder::~Avs3Decoder()
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

void Avs3Decoder::InitParams()
{
    avs3DecoderCtx_.dec_frame.bs_len = 0;
    avs3DecoderCtx_.dec_frame.bs = nullptr;
}

void Avs3Decoder::ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal,
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

bool Avs3Decoder::CheckVideoPixelFormat(VideoPixelFormat vpf)
{
    if (vpf == VideoPixelFormat::NV12 || vpf == VideoPixelFormat::NV21 || vpf == VideoPixelFormat::YUV420P) {
        return true;
    } else {
        AVCODEC_LOGE("Set parameter failed: pixel format value %{public}d invalid", vpf);
        return false;
    }
}

void Avs3Decoder::ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth)
{
    if (isWidth == true) {
        ConfigureDefaultVal(format, formatKey, VIDEO_MIN_WIDTH_SIZE, VIDEO_MAX_WIDTH_SIZE);
    } else {
        ConfigureDefaultVal(format, formatKey, VIDEO_MIN_HEIGHT_SIZE, VIDEO_MAX_HEIGHT_SIZE);
    }
}

void Avs3Decoder::DecoderFuncMatch()
{
    if (handle_ != nullptr) {
        std::unique_lock<std::mutex> runlock(decRunMutex_);
        avs3DecoderCreateFunc_ = reinterpret_cast<Avs3CreateDecoderFuncType>(dlsym(handle_,
            AVS3_DEC_CREATE_DECODER_FUNC_NAME));
        avs3DecoderFrameFunc_ = reinterpret_cast<Avs3DecodeFrameFuncType>(dlsym(handle_,
            AVS3_DEC_DECODE_FRAME_FUNC_NAME));
        avs3DecoderDestroyFunc_ = reinterpret_cast<Avs3DestroyDecoderFuncType>(dlsym(handle_,
            AVS3_DEC_DELETE_FUNC_NAME));
        avs3DecoderFlushFunc_ = reinterpret_cast<Avs3DestroyFlushFuncType>(dlsym(handle_,
            AVS3_DEC_FLUSH_FUNC_NAME));
        avs3DecoderImgCpyFunc_ = reinterpret_cast<Avs3CpyDecoderFuncType>(dlsym(handle_,
            AVS3_IMG_CPY_FUNC_NAME));
        if (avs3DecoderCreateFunc_ == nullptr || avs3DecoderFrameFunc_ == nullptr ||
            avs3DecoderDestroyFunc_ == nullptr || avs3DecoderImgCpyFunc_ == nullptr ||
            avs3DecoderFlushFunc_ == nullptr) {
                AVCODEC_LOGE("Avs3Decoder avs3 DecoderFuncMatch failed!");
                ReleaseHandle();
        }
    }

    InitParams();
    renderSurface_ = std::make_shared<RenderSurface>();
}

void Avs3Decoder::ReleaseHandle()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    avs3DecoderCreateFunc_ = nullptr;
    avs3DecoderFrameFunc_ = nullptr;
    avs3DecoderDestroyFunc_ = nullptr;
    avs3DecoderFlushFunc_ = nullptr;
    avs3DecoderImgCpyFunc_ = nullptr;
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

int32_t Avs3Decoder::Initialize()
{
    AVCODEC_SYNC_TRACE;
    decName_ = "Avs3Decoder_["+ std::to_string(instanceId_) + "]";
    AVCODEC_LOGI("current codec name: %{public}s", decName_.c_str());
    CHECK_AND_RETURN_RET_LOG(!codecName_.empty(), AVCS_ERR_INVALID_VAL, "Init codec failed:  empty name");
    std::string_view mime;
    for (uint32_t i = 0; i < SUPPORT_AVS3_DECODER_NUM; ++i) {
        if (SUPPORT_AVS3_DECODER[i].codecName == codecName_) {
            mime = SUPPORT_AVS3_DECODER[i].mimeType;
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

int32_t Avs3Decoder::CreateDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    if (avs3DecoderCreateFunc_ != nullptr) {
        Uavs3dCfg cdsc;
        cdsc.frm_threads = 1;
        cdsc.check_md5 = 0;
        avs3DecoderCtx_.dec_handle = avs3DecoderCreateFunc_(&cdsc, uavs3d_output_callback, nullptr);
        avs3DecoderCtx_.got_seqhdr = 0;
    }
    runLock.unlock();
    return AVCS_ERR_OK;
}

void Avs3Decoder::DeleteDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    if (avs3DecoderDestroyFunc_ != nullptr) {
        avs3DecoderDestroyFunc_(&avs3DecoderCtx_.dec_handle);
    }
    runLock.unlock();
}

void Avs3Decoder::ConvertDecOutToAVFrame()
{
    uint32_t planeY = 0;
    uint32_t planeU = 1;
    uint32_t planeV = 2;
    int32_t nume = 8;
    cachedFrame_->data[planeY] = static_cast<uint8_t*>(avs3DecoderCtx_.dec_frame.buffer[planeY]);
    cachedFrame_->data[planeU] = static_cast<uint8_t*>(avs3DecoderCtx_.dec_frame.buffer[planeU]);
    cachedFrame_->data[planeV] = static_cast<uint8_t*>(avs3DecoderCtx_.dec_frame.buffer[planeV]);

    if (avs3DecoderCtx_.dec_frame.bit_depth == nume) {
        cachedFrame_->format = static_cast<int>(AVPixelFormat::AV_PIX_FMT_YUV420P);
    } else {
        cachedFrame_->format = static_cast<int>(AVPixelFormat::AV_PIX_FMT_YUV420P10LE);
    }

    uint32_t channelY = 0;
    uint32_t channelU = 1;
    uint32_t channelV = 2;
    cachedFrame_->linesize[channelY] = static_cast<int32_t>(avs3DecoderCtx_.dec_frame.stride[channelY]);
    cachedFrame_->linesize[channelU] = static_cast<int32_t>(avs3DecoderCtx_.dec_frame.stride[channelU]);
    cachedFrame_->linesize[channelV] = static_cast<int32_t>(avs3DecoderCtx_.dec_frame.stride[channelV]);

    cachedFrame_->width = static_cast<int32_t>(avs3DecoderCtx_.dec_frame.width[0]);
    cachedFrame_->height = static_cast<int32_t>(avs3DecoderCtx_.dec_frame.height[0]);
}

void Avs3Decoder::SendFrame()
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
        avs3DecoderCtx_.dec_frame.bs_len = 0;
        avs3DecoderCtx_.dec_frame.bs = nullptr;
        isSendEos_ = true;
        AVCODEC_LOGI("Send eos end");
    } else {
        avs3DecoderCtx_.dec_frame.pts = inputAVBuffer->pts_;
        avs3DecoderCtx_.dec_frame.dts = 0;
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

        inputAvailQue_->Pop();
        inputBuffer->owner_ = Owner::OWNED_BY_USER;
        callback_->OnInputBufferAvailable(index, inputAVBuffer);
    }
}

int32_t Avs3Decoder::DecodeFrameOnce()
{
    const int32_t frameCount = 3;
    for (int i = 0; i < frameCount; i++) {
        avs3DecoderCtx_.dec_frame.stride[i] = 0;
        avs3DecoderCtx_.dec_frame.height[i] = 0;
        avs3DecoderCtx_.dec_frame.width[i] = 0;
        avs3DecoderCtx_.dec_frame.buffer[i] = nullptr;
    }
    if (avs3DecoderFrameFunc_ != nullptr && avs3DecoderCtx_.dec_handle != nullptr) {
        int32_t ret = avs3DecoderFrameFunc_(avs3DecoderCtx_.dec_handle, &avs3DecoderCtx_.dec_frame);
    } else {
        ret = -1;
    }
    if (ret == 0) {
        ConvertDecOutToAVFrame();
#ifdef BUILD_ENG_VERSION
        DumpOutputBuffer();
#endif
        auto index = codecAvailQue_->Front();
        CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
        std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
        int32_t status = AVCS_ERR_OK;
        if (CheckFormatChange(index, cachedFrame_->width, cachedFrame_->height) == AVCS_ERR_OK) {
            CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
            frameBuffer = buffers_[INDEX_OUTPUT][index];
            status = FillFrameBuffer(frameBuffer);
        } else {
            CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
            callback_->OnError(AVCODEC_ERROR_EXTEND_START, AVCS_ERR_NO_MEMORY);
        }
        frameBuffer->avBuffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        FramePostProcess(frameBuffer, index, status, AVCS_ERR_OK);
    }
    return ret;
}

int32_t Avs3Decoder::CheckAvs3DecLibStatus()
{
    void* handle = dlopen("libuavs3d.z.so", RTLD_LAZY);
    if (handle != nullptr) {
        auto avs3DecoderCreateFunc = reinterpret_cast<Avs3CreateDecoderFuncType>(
            dlsym(handle, AVS3_DEC_CREATE_DECODER_FUNC_NAME));
        auto avs3DecoderDecodeFrameFunc = reinterpret_cast<Avs3DecodeFrameFuncType>(
            dlsym(handle, AVS3_DEC_DECODE_FRAME_FUNC_NAME));
        auto avs3DecoderDestroyFunc = reinterpret_cast<Avs3DestroyDecoderFuncType>(
            dlsym(handle, AVS3_DEC_DELETE_FUNC_NAME));
        auto avs3DecoderFlushFunc = reinterpret_cast<Avs3DestroyFlushFuncType>(
            dlsym(handle, AVS3_DEC_FLUSH_FUNC_NAME));
        auto avs3DecoderImgCpyFunc = reinterpret_cast<Avs3CpyDecoderFuncType>(
            dlsym(handle, AVS3_IMG_CPY_FUNC_NAME));
        if (avs3DecoderCreateFunc == nullptr || avs3DecoderDecodeFrameFunc == nullptr ||
            avs3DecoderDestroyFunc == nullptr || avs3DecoderImgCpyFunc == nullptr ||
            avs3DecoderFlushFunc == nullptr) {
                AVCODEC_LOGE("Avs3Decoder avs3FuncMatch_ failed!");
                avs3DecoderCreateFunc = nullptr;
                avs3DecoderDecodeFrameFunc = nullptr;
                avs3DecoderDestroyFunc = nullptr;
                avs3DecoderImgCpyFunc = nullptr;
                avs3DecoderFlushFunc = nullptr;
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

int32_t Avs3Decoder::GetCodecCapability(std::vector<CapabilityData> &capaArray)
{
    CHECK_AND_RETURN_RET_LOG(CheckAvs3DecLibStatus() == AVCS_ERR_OK, AVCS_ERR_UNSUPPORT,
                             "avs3 decoder libs not available");
    for (uint32_t i = 0; i < SUPPORT_AVS3_DECODER_NUM; ++i) {
        CapabilityData capsData;
        capsData.codecName = static_cast<std::string>(SUPPORT_AVS3_DECODER[i].codecName);
        capsData.mimeType = static_cast<std::string>(SUPPORT_AVS3_DECODER[i].mimeType);
        capsData.codecType = AVCODEC_TYPE_VIDEO_DECODER;
        capsData.isVendor = false;
        capsData.maxInstance = VIDEO_INSTANCE_SIZE;
        capsData.alignment.width = VIDEO_ALIGNMENT_SIZE;
        capsData.alignment.height = VIDEO_ALIGNMENT_SIZE;
        capsData.width.minVal = VIDEO_MIN_WIDTH_SIZE;
        capsData.height.minVal = VIDEO_MIN_HEIGHT_SIZE;
        capsData.frameRate.minVal = 0;
        capsData.frameRate.maxVal = VIDEO_FRAMERATE_DEFAULT_SIZE;
        capsData.width.maxVal = VIDEO_MAX_WIDTH_SIZE;
        capsData.height.maxVal = VIDEO_MAX_HEIGHT_SIZE;
        capsData.blockPerFrame.minVal = 1;
        capsData.blockPerSecond.minVal = 1;
        capsData.blockSize.width = VIDEO_ALIGN_SIZE;
        capsData.blockSize.height = VIDEO_ALIGN_SIZE;
        capsData.pixFormat = {static_cast<int32_t>(VideoPixelFormat::NV12),
            static_cast<int32_t>(VideoPixelFormat::NV21)};
        capaArray.emplace_back(capsData);
    }
    return AVCS_ERR_OK;
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
