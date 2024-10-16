/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "hevc_decoder.h"
#include <fstream>
#include <cstdarg>

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "HevcDecoderLoader"};
const char *HEVC_DEC_LIB_PATH = "libhevcdec_ohos.z.so";
const char *HEVC_DEC_CREATE_FUNC_NAME = "HEVC_CreateDecoder";
const char *HEVC_DEC_DECODE_FRAME_FUNC_NAME = "HEVC_DecodeFrame";
const char *HEVC_DEC_FLUSH_FRAME_FUNC_NAME = "HEVC_FlushFrame";
const char *HEVC_DEC_DELETE_FUNC_NAME = "HEVC_DeleteDecoder";

constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;
constexpr int32_t DEFAULT_IN_BUFFER_CNT = 4;
constexpr int32_t DEFAULT_OUT_SURFACE_CNT = 4;
constexpr int32_t DEFAULT_OUT_BUFFER_CNT = 3;
constexpr int32_t DEFAULT_MIN_BUFFER_CNT = 2;
constexpr int32_t DEFAULT_MAX_BUFFER_CNT = 10;
constexpr uint32_t VIDEO_PIX_DEPTH_YUV = 3;
constexpr int32_t VIDEO_MIN_BUFFER_SIZE = 1474560; // 1280*768
constexpr int32_t VIDEO_MAX_BUFFER_SIZE = 3110400; // 1080p
constexpr int32_t VIDEO_MIN_SIZE = 2;
constexpr int32_t VIDEO_ALIGNMENT_SIZE = 2;
constexpr int32_t VIDEO_MAX_WIDTH_SIZE = 1920;
constexpr int32_t VIDEO_MAX_HEIGHT_SIZE = 1920;
constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
constexpr uint32_t DEFAULT_TRY_DECODE_TIME = 1;
constexpr uint32_t DEFAULT_TRY_REQ_TIME = 10;
constexpr int32_t VIDEO_INSTANCE_SIZE = 64;
constexpr int32_t VIDEO_BLOCKPERFRAME_SIZE = 36864;
constexpr int32_t VIDEO_BLOCKPERSEC_SIZE = 983040;
#ifdef BUILD_ENG_VERSION
constexpr uint32_t PATH_MAX_LEN = 128;
constexpr char DUMP_PATH[] = "/data/misc/hevcdecoderdump";
#endif
constexpr struct {
    const std::string_view codecName;
    const std::string_view mimeType;
} SUPPORT_HEVC_DECODER[] = {
    {AVCodecCodecName::VIDEO_DECODER_HEVC_NAME, CodecMimeType::VIDEO_HEVC},
};
constexpr uint32_t SUPPORT_HEVC_DECODER_NUM = sizeof(SUPPORT_HEVC_DECODER) / sizeof(SUPPORT_HEVC_DECODER[0]);
} // namespace
using namespace OHOS::Media;
HevcDecoder::HevcDecoder(const std::string &name) : codecName_(name), state_(State::UNINITIALIZED)
{
    AVCODEC_SYNC_TRACE;
    std::unique_lock<std::mutex> lock(decoderCountMutex_);
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
        handle_ = dlopen(HEVC_DEC_LIB_PATH, RTLD_LAZY);
        if (handle_ == nullptr) {
            AVCODEC_LOGE("Load codec failed: %{public}s", HEVC_DEC_LIB_PATH);
        }
        HevcFuncMatch();
        AVCODEC_LOGI("Num %{public}u HevcDecoder entered, state: Uninitialized", decInstanceID_);
    } else {
        AVCODEC_LOGE("HevcDecoder already has %{public}d instances, cannot has more instances", VIDEO_INSTANCE_SIZE);
        state_ = State::ERROR;
    }

    initParams_.logFxn = nullptr;
    initParams_.uiChannelID = 0;
    InitHevcParams();
}

void HevcDecoder::HevcFuncMatch()
{
    if (handle_ != nullptr) {
        hevcDecoderCreateFunc_ = reinterpret_cast<CreateHevcDecoderFuncType>(dlsym(handle_,
            HEVC_DEC_CREATE_FUNC_NAME));
        hevcDecoderDecodecFrameFunc_ = reinterpret_cast<DecodeFuncType>(dlsym(handle_,
            HEVC_DEC_DECODE_FRAME_FUNC_NAME));
        hevcDecoderFlushFrameFunc_ = reinterpret_cast<FlushFuncType>(dlsym(handle_, HEVC_DEC_FLUSH_FRAME_FUNC_NAME));
        hevcDecoderDeleteFunc_ = reinterpret_cast<DeleteFuncType>(dlsym(handle_, HEVC_DEC_DELETE_FUNC_NAME));
        if (hevcDecoderCreateFunc_ == nullptr || hevcDecoderDecodecFrameFunc_ == nullptr ||
            hevcDecoderDeleteFunc_ == nullptr || hevcDecoderFlushFrameFunc_ == nullptr) {
                AVCODEC_LOGE("HevcDecoder hevcFuncMatch_ failed!");
                ReleaseHandle();
        }
    }
}

void HevcDecoder::ReleaseHandle()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    hevcDecoderCreateFunc_ = nullptr;
    hevcDecoderDecodecFrameFunc_ = nullptr;
    hevcDecoderFlushFrameFunc_ = nullptr;
    hevcDecoderDeleteFunc_ = nullptr;
    if (handle_ != nullptr) {
        dlclose(handle_);
        handle_ = nullptr;
    }
    runLock.unlock();
}

HevcDecoder::~HevcDecoder()
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

int32_t HevcDecoder::Initialize()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(!codecName_.empty(), AVCS_ERR_INVALID_VAL, "Init codec failed:  empty name");
    std::string_view mime;
    for (uint32_t i = 0; i < SUPPORT_HEVC_DECODER_NUM; ++i) {
        if (SUPPORT_HEVC_DECODER[i].codecName == codecName_) {
            mime = SUPPORT_HEVC_DECODER[i].mimeType;
            break;
        }
    }
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, mime);
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, codecName_);
    sendTask_ = std::make_shared<TaskThread>("SendFrame");
    sendTask_->RegisterHandler([this] { SendFrame(); });

#ifdef BUILD_ENG_VERSION
    OpenDumpFile();
#endif
    state_ = State::INITIALIZED;
    AVCODEC_LOGI("Init codec successful,  state: Uninitialized -> Initialized");
    return AVCS_ERR_OK;
}

#ifdef BUILD_ENG_VERSION
void HevcDecoder::OpenDumpFile()
{
    std::string dumpModeStr = OHOS::system::GetParameter("hevcdecoder.dump", "0");
    AVCODEC_LOGI("dumpModeStr %{public}s", dumpModeStr.c_str());

    char fileName[PATH_MAX_LEN] = {0};
    if (dumpModeStr == "10" || dumpModeStr == "11") {
        int ret = sprintf_s(fileName, sizeof(fileName), "%s/input_%p.h265", DUMP_PATH, this);
        if (ret > 0) {
            dumpInFile_ = std::make_shared<std::ofstream>();
            dumpInFile_->open(fileName, std::ios::out | std::ios::binary);
            if (!dumpInFile_->is_open()) {
                AVCODEC_LOGW("fail open file %{public}s", fileName);
                dumpInFile_ = nullptr;
            }
        }
    }
    if (dumpModeStr == "1" || dumpModeStr == "01" || dumpModeStr == "11") {
        int ret = sprintf_s(fileName, sizeof(fileName), "%s/output_%p.yuv", DUMP_PATH, this);
        if (ret > 0) {
            dumpOutFile_ = std::make_shared<std::ofstream>();
            dumpOutFile_->open(fileName, std::ios::out | std::ios::binary);
            if (!dumpOutFile_->is_open()) {
                AVCODEC_LOGW("fail open file %{public}s", fileName);
                dumpOutFile_ = nullptr;
            }
        }
        ret = sprintf_s(fileName, sizeof(fileName), "%s/outConvert_%p.data", DUMP_PATH, this);
        if (ret > 0) {
            dumpConvertFile_ = std::make_shared<std::ofstream>();
            dumpConvertFile_->open(fileName, std::ios::out | std::ios::binary);
            if (!dumpConvertFile_->is_open()) {
                AVCODEC_LOGW("fail open file %{public}s", fileName);
                dumpConvertFile_ = nullptr;
            }
        }
    }
}
#endif

void HevcDecoder::ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal,
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

void HevcDecoder::ConfigureSurface(const Format &format, const std::string_view &formatKey, FormatDataType formatType)
{
    CHECK_AND_RETURN_LOG(formatType == FORMAT_TYPE_INT32, "Set parameter failed: type should be int32");

    int32_t val = 0;
    CHECK_AND_RETURN_LOG(format.GetIntValue(formatKey, val), "Set parameter failed: get value fail");

    if (formatKey == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT) {
        VideoPixelFormat vpf = static_cast<VideoPixelFormat>(val);
        CHECK_AND_RETURN_LOG(vpf == VideoPixelFormat::NV12 || vpf == VideoPixelFormat::NV21,
            "Set parameter failed: pixel format value %{public}d invalid", val);
        outputPixelFmt_ = vpf;
        format_.PutIntValue(formatKey, val);
    } else if (formatKey == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE) {
        VideoRotation sr = static_cast<VideoRotation>(val);
        CHECK_AND_RETURN_LOG(sr == VideoRotation::VIDEO_ROTATION_0 || sr == VideoRotation::VIDEO_ROTATION_90 ||
                                 sr == VideoRotation::VIDEO_ROTATION_180 || sr == VideoRotation::VIDEO_ROTATION_270,
                             "Set parameter failed: rotation angle value %{public}d invalid", val);
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val);
    } else if (formatKey == MediaDescriptionKey::MD_KEY_SCALE_TYPE) {
        ScalingMode scaleMode = static_cast<ScalingMode>(val);
        CHECK_AND_RETURN_LOG(scaleMode == ScalingMode::SCALING_MODE_SCALE_TO_WINDOW ||
                                 scaleMode == ScalingMode::SCALING_MODE_SCALE_CROP,
                             "Set parameter failed: scale type value %{public}d invalid", val);
        format_.PutIntValue(formatKey, val);
    } else {
        AVCODEC_LOGW("Set parameter failed: %{public}s, please check your parameter key", formatKey.data());
        return;
    }
    AVCODEC_LOGI("Set parameter  %{public}s success, val %{public}d", formatKey.data(), val);
}

int32_t HevcDecoder::Configure(const Format &format)
{
    AVCODEC_SYNC_TRACE;
    if (state_ == State::UNINITIALIZED) {
        int32_t ret = Initialize();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Init codec failed");
    }
    CHECK_AND_RETURN_RET_LOG((state_ == State::INITIALIZED), AVCS_ERR_INVALID_STATE,
                             "Configure codec failed:  not in Initialized state");
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, DEFAULT_OUT_BUFFER_CNT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, DEFAULT_IN_BUFFER_CNT);
    for (auto &it : format.GetFormatMap()) {
        if (it.first == MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT) {
            isOutBufSetted_ = true;
            ConfigureDefaultVal(format, it.first, DEFAULT_MIN_BUFFER_CNT, DEFAULT_MAX_BUFFER_CNT);
        } else if (it.first == MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT) {
            ConfigureDefaultVal(format, it.first, DEFAULT_MIN_BUFFER_CNT, DEFAULT_MAX_BUFFER_CNT);
        } else if (it.first == MediaDescriptionKey::MD_KEY_WIDTH) {
            ConfigureDefaultVal(format, it.first, VIDEO_MIN_SIZE, VIDEO_MAX_WIDTH_SIZE);
        } else if (it.first == MediaDescriptionKey::MD_KEY_HEIGHT) {
            ConfigureDefaultVal(format, it.first, VIDEO_MIN_SIZE, VIDEO_MAX_HEIGHT_SIZE);
        } else if (it.first == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT ||
                   it.first == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE ||
                   it.first == MediaDescriptionKey::MD_KEY_SCALE_TYPE) {
            ConfigureSurface(format, it.first, it.second.type);
        } else {
            AVCODEC_LOGW("Set parameter failed: size:%{public}s, unsupport key", it.first.data());
        }
    }
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width_);
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height_);

    initParams_.uiChannelID = decInstanceID_;
    initParams_.logFxn = HevcDecLog;

    state_ = State::CONFIGURED;
    
    return AVCS_ERR_OK;
}

bool HevcDecoder::IsActive() const
{
    return state_ == State::RUNNING || state_ == State::FLUSHED || state_ == State::EOS;
}

int32_t HevcDecoder::Start()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Start codec failed: callback is null");
    CHECK_AND_RETURN_RET_LOG((state_ == State::CONFIGURED || state_ == State::FLUSHED), AVCS_ERR_INVALID_STATE,
                             "Start codec failed: not in Configured or Flushed state");

    std::unique_lock<std::mutex> runLock(decRunMutex_);
    int32_t createRet = 0;
    if (hevcSDecoder_ == nullptr && hevcDecoderCreateFunc_ != nullptr) {
        createRet = hevcDecoderCreateFunc_(&hevcSDecoder_, &initParams_);
    }
    runLock.unlock();
    CHECK_AND_RETURN_RET_LOG(createRet == 0 && hevcSDecoder_ != nullptr, AVCS_ERR_INVALID_OPERATION,
                             "hevc deocder create failed");

    if (!isBufferAllocated_) {
        for (int32_t i = 0; i < AV_NUM_DATA_POINTERS; i++) {
            scaleData_[i] = nullptr;
            scaleLineSize_[i] = 0;
        }
        isConverted_ = false;
        int32_t ret = AllocateBuffers();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Start codec failed: cannot allocate buffers");
        isBufferAllocated_ = true;
    }
    state_ = State::RUNNING;
    InitBuffers();
    isSendEos_ = false;
    sendTask_->Start();
    AVCODEC_LOGI("Start codec successful, state: Running");
    return AVCS_ERR_OK;
}

void HevcDecoder::InitBuffers()
{
    inputAvailQue_->SetActive(true);
    codecAvailQue_->SetActive(true);
    if (sInfo_.surface != nullptr) {
        renderAvailQue_->SetActive(true);
    }
    if (buffers_[INDEX_INPUT].size() > 0) {
        for (uint32_t i = 0; i < buffers_[INDEX_INPUT].size(); i++) {
            buffers_[INDEX_INPUT][i]->owner_ = HBuffer::Owner::OWNED_BY_USER;
            callback_->OnInputBufferAvailable(i, buffers_[INDEX_INPUT][i]->avBuffer);
            AVCODEC_LOGI("OnInputBufferAvailable frame index = %{public}d, owner = %{public}d", i,
                         buffers_[INDEX_INPUT][i]->owner_.load());
        }
    }
    if (buffers_[INDEX_OUTPUT].size() <= 0) {
        return;
    }
    if (sInfo_.surface == nullptr) {
        for (uint32_t i = 0; i < buffers_[INDEX_OUTPUT].size(); i++) {
            buffers_[INDEX_OUTPUT][i]->owner_ = HBuffer::Owner::OWNED_BY_CODEC;
            codecAvailQue_->Push(i);
        }
    } else {
        for (uint32_t i = 0; i < buffers_[INDEX_OUTPUT].size(); i++) {
            std::shared_ptr<FSurfaceMemory> surfaceMemory = buffers_[INDEX_OUTPUT][i]->sMemory;
            if (surfaceMemory->GetSurfaceBuffer() == nullptr) {
                buffers_[INDEX_OUTPUT][i]->owner_ = HBuffer::Owner::OWNED_BY_SURFACE;
                renderAvailQue_->Push(i);
            } else {
                buffers_[INDEX_OUTPUT][i]->owner_ = HBuffer::Owner::OWNED_BY_CODEC;
                codecAvailQue_->Push(i);
            }
        }
    }
    InitHevcParams();
}

void HevcDecoder::InitHevcParams()
{
    hevcDecoderInputArgs_.pStream = nullptr;
    hevcDecoderInputArgs_.uiStreamLen = 0;
    hevcDecoderInputArgs_.uiTimeStamp = 0;
    hevcDecoderOutpusArgs_.pucOutYUV[0] = nullptr;
    hevcDecoderOutpusArgs_.pucOutYUV[1] = nullptr; // 1 u channel
    hevcDecoderOutpusArgs_.pucOutYUV[2] = nullptr; // 2 v channel
    hevcDecoderOutpusArgs_.uiDecBitDepth = 0;
    hevcDecoderOutpusArgs_.uiDecHeight = 0;
    hevcDecoderOutpusArgs_.uiDecStride = 0;
    hevcDecoderOutpusArgs_.uiDecWidth = 0;
    hevcDecoderOutpusArgs_.uiTimeStamp = 0;
}

void HevcDecoder::ResetData()
{
    if (scaleData_[0] != nullptr) {
        if (isConverted_) {
            av_free(scaleData_[0]);
            isConverted_ = false;
            scale_.reset();
        }
        for (int32_t i = 0; i < AV_NUM_DATA_POINTERS; i++) {
            scaleData_[i] = nullptr;
            scaleLineSize_[i] = 0;
        }
    }
}

void HevcDecoder::ResetBuffers()
{
    inputAvailQue_->Clear();
    codecAvailQue_->Clear();
    if (sInfo_.surface != nullptr) {
        renderAvailQue_->Clear();
        renderSurfaceBufferMap_.clear();
    }
    ResetData();
}

void HevcDecoder::StopThread()
{
    if (inputAvailQue_ != nullptr) {
        inputAvailQue_->SetActive(false, false);
    }
    if (codecAvailQue_ != nullptr) {
        codecAvailQue_->SetActive(false, false);
    }
    if (sendTask_ != nullptr) {
        sendTask_->Stop();
    }
    if (sInfo_.surface != nullptr && renderAvailQue_ != nullptr) {
        renderAvailQue_->SetActive(false, false);
    }
}

int32_t HevcDecoder::Stop()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG((IsActive()), AVCS_ERR_INVALID_STATE, "Stop codec failed: not in executing state");
    state_ = State::STOPPING;
    inputAvailQue_->SetActive(false, false);
    codecAvailQue_->SetActive(false, false);
    sendTask_->Stop();

    if (sInfo_.surface != nullptr) {
        renderAvailQue_->SetActive(false, false);
    }

    std::unique_lock<std::mutex> runLock(decRunMutex_);
    if (hevcSDecoder_ != nullptr && hevcDecoderDeleteFunc_ != nullptr) {
        int ret = hevcDecoderDeleteFunc_(hevcSDecoder_);
        if (ret != 0) {
            AVCODEC_LOGE("Error: hevcDecoder delete error: %{public}d", ret);
            callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
            state_ = State::ERROR;
        }
        hevcSDecoder_ = nullptr;
    }
    runLock.unlock();

    ReleaseBuffers();
    state_ = State::CONFIGURED;
    AVCODEC_LOGI("Stop codec successful, state: Configured");
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::Flush()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG((state_ == State::RUNNING || state_ == State::EOS), AVCS_ERR_INVALID_STATE,
                             "Flush codec failed: not in running or Eos state");
    state_ = State::FLUSHING;
    inputAvailQue_->SetActive(false, false);
    codecAvailQue_->SetActive(false, false);
    sendTask_->Pause();

    if (sInfo_.surface != nullptr) {
        renderAvailQue_->SetActive(false, false);
    }

    ResetBuffers();
    state_ = State::FLUSHED;
    AVCODEC_LOGI("Flush codec successful, state: Flushed");
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::Reset()
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGI("Reset codec called");
    int32_t ret = Release();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Reset codec failed: cannot release codec");
    ret = Initialize();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Reset codec failed: cannot init codec");
    AVCODEC_LOGI("Reset codec successful, state: Initialized");
    return AVCS_ERR_OK;
}

void HevcDecoder::ReleaseResource()
{
    StopThread();
    ReleaseBuffers();
    format_ = Format();
    if (sInfo_.surface != nullptr) {
        sInfo_.surface->CleanCache();
        AVCODEC_LOGI("surface cleancache success");
        int ret = UnRegisterListenerToSurface(sInfo_.surface);
        if (ret != 0) {
            callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
            state_ = State::ERROR;
        }
    }
    sInfo_.surface = nullptr;
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    if (hevcSDecoder_ != nullptr && hevcDecoderDeleteFunc_ != nullptr) {
        int ret = hevcDecoderDeleteFunc_(hevcSDecoder_);
        if (ret != 0) {
            AVCODEC_LOGE("Error: hevcDecoder delete error: %{public}d", ret);
            callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
            state_ = State::ERROR;
        }
        hevcSDecoder_ = nullptr;
    }
    runLock.unlock();
}

int32_t HevcDecoder::Release()
{
    AVCODEC_SYNC_TRACE;
    state_ = State::STOPPING;
    ReleaseResource();
    state_ = State::UNINITIALIZED;
    AVCODEC_LOGI("Release codec successful, state: Uninitialized");
    return AVCS_ERR_OK;
}

void HevcDecoder::SetSurfaceParameter(const Format &format, const std::string_view &formatKey,
                                      FormatDataType formatType)
{
    CHECK_AND_RETURN_LOG(formatType == FORMAT_TYPE_INT32, "Set parameter failed: type should be int32");
    int32_t val = 0;
    CHECK_AND_RETURN_LOG(format.GetIntValue(formatKey, val), "Set parameter failed: get value fail");
    if (formatKey == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT) {
        VideoPixelFormat vpf = static_cast<VideoPixelFormat>(val);
        CHECK_AND_RETURN_LOG(vpf == VideoPixelFormat::NV12 || vpf == VideoPixelFormat::NV21,
            "Set parameter failed: pixel format value %{public}d invalid", val);
        outputPixelFmt_ = vpf;
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, val);
        GraphicPixelFormat surfacePixelFmt;
        if (bitDepth_ == BIT_DEPTH10BIT) {
            if (vpf == VideoPixelFormat::NV12) {
                surfacePixelFmt = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010;
            } else {
                surfacePixelFmt = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010;
            }
        } else {
            surfacePixelFmt = TranslateSurfaceFormat(vpf);
        }
        std::lock_guard<std::mutex> sLock(surfaceMutex_);
        sInfo_.requestConfig.format = surfacePixelFmt;
    } else if (formatKey == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE) {
        VideoRotation sr = static_cast<VideoRotation>(val);
        CHECK_AND_RETURN_LOG(sr == VideoRotation::VIDEO_ROTATION_0 || sr == VideoRotation::VIDEO_ROTATION_90 ||
                                 sr == VideoRotation::VIDEO_ROTATION_180 || sr == VideoRotation::VIDEO_ROTATION_270,
                             "Set parameter failed: rotation angle value %{public}d invalid", val);
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val);
        std::lock_guard<std::mutex> sLock(surfaceMutex_);
        sInfo_.surface->SetTransform(TranslateSurfaceRotation(sr));
    } else if (formatKey == MediaDescriptionKey::MD_KEY_SCALE_TYPE) {
        ScalingMode scaleMode = static_cast<ScalingMode>(val);
        CHECK_AND_RETURN_LOG(scaleMode == ScalingMode::SCALING_MODE_SCALE_TO_WINDOW ||
                                 scaleMode == ScalingMode::SCALING_MODE_SCALE_CROP,
                             "Set parameter failed: scale type value %{public}d invalid", val);
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, val);
        std::lock_guard<std::mutex> sLock(surfaceMutex_);
        sInfo_.scalingMode = scaleMode;
    } else {
        AVCODEC_LOGW("Set parameter failed: %{public}s", formatKey.data());
        return;
    }
    AVCODEC_LOGI("Set parameter %{public}s success, val %{publid}d", formatKey.data(), val);
}

int32_t HevcDecoder::SetParameter(const Format &format)
{
    AVCODEC_SYNC_TRACE;
    for (auto &it : format.GetFormatMap()) {
        if (sInfo_.surface != nullptr && it.second.type == FORMAT_TYPE_INT32) {
            if (it.first == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT ||
                it.first == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE ||
                it.first == MediaDescriptionKey::MD_KEY_SCALE_TYPE) {
                SetSurfaceParameter(format, it.first, it.second.type);
            }
        } else {
            AVCODEC_LOGW("Current Version, %{public}s is not supported", it.first.data());
        }
    }
    AVCODEC_LOGI("Set parameter successful");
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::GetOutputFormat(Format &format)
{
    AVCODEC_SYNC_TRACE;
    if ((!format_.ContainKey(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE)) &&
        hevcDecoderOutpusArgs_.uiDecStride != 0) {
        int32_t stride = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride);
        int32_t maxInputSize = static_cast<int32_t>(static_cast<UINT32>(stride * height_ * VIDEO_PIX_DEPTH_YUV) >> 1);
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, maxInputSize);
    }

    if (!format_.ContainKey(OHOS::Media::Tag::VIDEO_SLICE_HEIGHT)) {
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_SLICE_HEIGHT, height_);
    }
    if (!format_.ContainKey(OHOS::Media::Tag::VIDEO_PIC_WIDTH) ||
        !format_.ContainKey(OHOS::Media::Tag::VIDEO_PIC_HEIGHT)) {
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, width_);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, height_);
    }

    if (!format_.ContainKey(OHOS::Media::Tag::VIDEO_CROP_RIGHT) ||
        !format_.ContainKey(OHOS::Media::Tag::VIDEO_CROP_BOTTOM)) {
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_RIGHT, width_-1);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_BOTTOM, height_-1);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_LEFT, 0);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_TOP, 0);
    }

    format = format_;
    AVCODEC_LOGI("Get outputFormat successful");
    return AVCS_ERR_OK;
}

void HevcDecoder::CalculateBufferSize()
{
    if ((static_cast<UINT32>(width_ * height_ * VIDEO_PIX_DEPTH_YUV) >> 1) <= VIDEO_MIN_BUFFER_SIZE) {
        inputBufferSize_ = VIDEO_MIN_BUFFER_SIZE;
    } else {
        inputBufferSize_ = VIDEO_MAX_BUFFER_SIZE;
    }
    AVCODEC_LOGI("width = %{public}d, height = %{public}d, Input buffer size = %{public}d",
                 width_, height_, inputBufferSize_);
}

int32_t HevcDecoder::AllocateInputBuffer(int32_t bufferCnt, int32_t inBufferSize)
{
    int32_t valBufferCnt = 0;
    for (int32_t i = 0; i < bufferCnt; i++) {
        std::shared_ptr<HBuffer> buf = std::make_shared<HBuffer>();
        std::shared_ptr<AVAllocator> allocator =
            AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        CHECK_AND_CONTINUE_LOG(allocator != nullptr, "input buffer %{public}d allocator is nullptr", i);
        buf->avBuffer = AVBuffer::CreateAVBuffer(allocator, inBufferSize);
        CHECK_AND_CONTINUE_LOG(buf->avBuffer != nullptr, "Allocate input buffer failed, index=%{public}d", i);
        AVCODEC_LOGI("Allocate input buffer success: index=%{public}d, size=%{public}d", i,
                     buf->avBuffer->memory_->GetCapacity());

        buffers_[INDEX_INPUT].emplace_back(buf);
        valBufferCnt++;
    }
    if (valBufferCnt < DEFAULT_MIN_BUFFER_CNT) {
        AVCODEC_LOGE("Allocate input buffer failed: only %{public}d buffer is allocated, no memory", valBufferCnt);
        buffers_[INDEX_INPUT].clear();
        return AVCS_ERR_NO_MEMORY;
    }
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::SetSurfaceCfg(int32_t bufferCnt)
{
    if (outputPixelFmt_ == VideoPixelFormat::UNKNOWN) {
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    }
    int32_t val32 = 0;
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, val32);
    GraphicPixelFormat surfacePixelFmt = TranslateSurfaceFormat(static_cast<VideoPixelFormat>(val32));
    CHECK_AND_RETURN_RET_LOG(surfacePixelFmt != GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT, AVCS_ERR_UNSUPPORT,
                             "Failed to allocate output buffer: unsupported surface format");
    sInfo_.requestConfig.width = width_;
    sInfo_.requestConfig.height = height_;
    sInfo_.requestConfig.format = surfacePixelFmt;
    if (sInfo_.surface != nullptr) {
        CHECK_AND_RETURN_RET_LOG(sInfo_.surface->SetQueueSize(bufferCnt) == OHOS::SurfaceError::SURFACE_ERROR_OK,
                                 AVCS_ERR_NO_MEMORY, "Surface set QueueSize=%{public}d failed", bufferCnt);

        format_.GetIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, val32);
        sInfo_.scalingMode = static_cast<ScalingMode>(val32);
        format_.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val32);
        sInfo_.surface->SetTransform(TranslateSurfaceRotation(static_cast<VideoRotation>(val32)));
    }
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::AllocateOutputBuffer(int32_t bufferCnt)
{
    int32_t valBufferCnt = 0;
    CHECK_AND_RETURN_RET_LOG(SetSurfaceCfg(bufferCnt) == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "SetSurfaceCfg failed");

    for (int i = 0; i < bufferCnt; i++) {
        std::shared_ptr<HBuffer> buf = std::make_shared<HBuffer>();
        if (sInfo_.surface == nullptr) {
            std::shared_ptr<AVAllocator> allocator =
                AVAllocatorFactory::CreateSurfaceAllocator(sInfo_.requestConfig);
            CHECK_AND_CONTINUE_LOG(allocator != nullptr, "output buffer %{public}d allocator is nullptr", i);
            buf->avBuffer = AVBuffer::CreateAVBuffer(allocator, 0);
            if (buf->avBuffer != nullptr) {
                AVCODEC_LOGI("Allocate output share buffer success: index=%{public}d, size=%{public}d", i,
                             buf->avBuffer->memory_->GetCapacity());
            }
        } else {
            buf->sMemory = std::make_shared<FSurfaceMemory>(&sInfo_);
            CHECK_AND_CONTINUE_LOG(buf->sMemory->GetSurfaceBuffer() != nullptr,
                                   "output surface memory %{public}d create fail", i);
            outAVBuffer4Surface_.emplace_back(AVBuffer::CreateAVBuffer());
            buf->avBuffer = AVBuffer::CreateAVBuffer(buf->sMemory->GetBase(), buf->sMemory->GetSize());
            AVCODEC_LOGI("Allocate output surface buffer success: index=%{public}d, addr=%{public}p, size=%{public}d, "
                         "stride=%{public}d",
                         i, buf->sMemory->GetBase(), buf->sMemory->GetSize(),
                         buf->sMemory->GetSurfaceBufferStride());
        }
        CHECK_AND_CONTINUE_LOG(buf->avBuffer != nullptr, "Allocate output buffer failed, index=%{public}d", i);

        buf->width = width_;
        buf->height = height_;
        buffers_[INDEX_OUTPUT].emplace_back(buf);
        valBufferCnt++;
    }
    if (valBufferCnt < DEFAULT_MIN_BUFFER_CNT) {
        AVCODEC_LOGE("Allocate output buffer failed: only %{public}d buffer is allocated, no memory", valBufferCnt);
        buffers_[INDEX_INPUT].clear();
        buffers_[INDEX_OUTPUT].clear();
        return AVCS_ERR_NO_MEMORY;
    }
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::AllocateBuffers()
{
    AVCODEC_SYNC_TRACE;
    CalculateBufferSize();
    CHECK_AND_RETURN_RET_LOG(inputBufferSize_ > 0, AVCS_ERR_INVALID_VAL,
                             "Allocate buffer with input size=%{public}d failed", inputBufferSize_);
    if (sInfo_.surface != nullptr && isOutBufSetted_ == false) {
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, DEFAULT_OUT_SURFACE_CNT);
    }
    int32_t inputBufferCnt = 0;
    int32_t outputBufferCnt = 0;
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, inputBufferCnt);
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, outputBufferCnt);
    inputAvailQue_ = std::make_shared<BlockQueue<uint32_t>>("inputAvailQue", inputBufferCnt);
    codecAvailQue_ = std::make_shared<BlockQueue<uint32_t>>("codecAvailQue", outputBufferCnt);
    if (sInfo_.surface != nullptr) {
        renderAvailQue_ = std::make_shared<BlockQueue<uint32_t>>("renderAvailQue", outputBufferCnt);
    }
    if (AllocateInputBuffer(inputBufferCnt, inputBufferSize_) == AVCS_ERR_NO_MEMORY ||
        AllocateOutputBuffer(outputBufferCnt) == AVCS_ERR_NO_MEMORY) {
        return AVCS_ERR_NO_MEMORY;
    }
    AVCODEC_LOGI("Allocate buffers successful");
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::UpdateOutputBuffer(uint32_t index)
{
    std::shared_ptr<HBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
    if (width_ != outputBuffer->width || height_ != outputBuffer->height || bitDepth_ != outputBuffer->bitDepth) {
        std::shared_ptr<AVAllocator> allocator =
            AVAllocatorFactory::CreateSurfaceAllocator(sInfo_.requestConfig);
        CHECK_AND_RETURN_RET_LOG(allocator != nullptr, AVCS_ERR_NO_MEMORY, "buffer %{public}d allocator is nullptr",
                                 index);
        outputBuffer->avBuffer = AVBuffer::CreateAVBuffer(allocator, 0);
        CHECK_AND_RETURN_RET_LOG(outputBuffer->avBuffer != nullptr, AVCS_ERR_NO_MEMORY,
                                 "Buffer allocate failed, index=%{public}d", index);
        AVCODEC_LOGI("update output buffer success: index=%{public}d, size=%{public}d", index,
                     outputBuffer->avBuffer->memory_->GetCapacity());

        outputBuffer->owner_ = HBuffer::Owner::OWNED_BY_CODEC;
        outputBuffer->width = width_;
        outputBuffer->height = height_;
        outputBuffer->bitDepth = bitDepth_;
    }
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::UpdateSurfaceMemory(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    std::unique_lock<std::mutex> oLock(outputMutex_);
    std::shared_ptr<HBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
    oLock.unlock();
    if (width_ != outputBuffer->width || height_ != outputBuffer->height || bitDepth_ != outputBuffer->bitDepth) {
        std::shared_ptr<FSurfaceMemory> surfaceMemory = outputBuffer->sMemory;
        surfaceMemory->SetNeedRender(false);
        surfaceMemory->ReleaseSurfaceBuffer();
        while (state_ == State::RUNNING) {
            std::unique_lock<std::mutex> sLock(surfaceMutex_);
            sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
            sLock.unlock();
            if (surfaceBuffer != nullptr) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_REQ_TIME));
        }
        outputBuffer->avBuffer =
            AVBuffer::CreateAVBuffer(outputBuffer->sMemory->GetBase(), outputBuffer->sMemory->GetSize());
        outputBuffer->width = width_;
        outputBuffer->height = height_;
        outputBuffer->bitDepth = bitDepth_;
    }

    return AVCS_ERR_OK;
}

int32_t HevcDecoder::CheckFormatChange(uint32_t index, int width, int height, int bitDepth)
{
    bool formatChanged = false;
    if (width_ != width || height_ != height || bitDepth_ != bitDepth) {
        AVCODEC_LOGI("format change, width: %{public}d->%{public}d, height: %{public}d->%{public}d, "
                     "bitDepth: %{public}d->%{public}d", width_, width, height_, height, bitDepth_, bitDepth);
        width_ = width;
        height_ = height;
        bitDepth_ = bitDepth;
        ResetData();
        scale_ = nullptr;
        std::unique_lock<std::mutex> sLock(surfaceMutex_);
        sInfo_.requestConfig.width = width_;
        sInfo_.requestConfig.height = height_;
        if (bitDepth_ == BIT_DEPTH10BIT) {
            if (outputPixelFmt_ == VideoPixelFormat::NV12 || outputPixelFmt_ == VideoPixelFormat::UNKNOWN) {
                sInfo_.requestConfig.format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010;
            } else {
                sInfo_.requestConfig.format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010;
            }
        }
        sLock.unlock();
        formatChanged = true;
    }
    if (sInfo_.surface == nullptr) {
        std::lock_guard<std::mutex> oLock(outputMutex_);
        CHECK_AND_RETURN_RET_LOG((UpdateOutputBuffer(index) == AVCS_ERR_OK), AVCS_ERR_NO_MEMORY,
                                 "Update output buffer failed, index=%{public}u", index);
    } else {
        CHECK_AND_RETURN_RET_LOG((UpdateSurfaceMemory(index) == AVCS_ERR_OK), AVCS_ERR_NO_MEMORY,
                                 "Update buffer failed");
    }
    if (!format_.ContainKey(OHOS::Media::Tag::VIDEO_STRIDE) || formatChanged) {
        int32_t stride = GetSurfaceBufferStride(buffers_[INDEX_OUTPUT][index]);
        CHECK_AND_RETURN_RET_LOG(stride > 0, AVCS_ERR_NO_MEMORY, "get GetSurfaceBufferStride failed");
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, stride);
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width_);
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height_);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_SLICE_HEIGHT, height_);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, width_);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, height_);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_RIGHT, width_-1);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_BOTTOM, height_-1);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_LEFT, 0);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_CROP_TOP, 0);
        callback_->OnOutputFormatChanged(format_);
    }
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::GetSurfaceBufferStride(const std::shared_ptr<HBuffer> &frameBuffer)
{
    int32_t surfaceBufferStride = 0;
    if (sInfo_.surface == nullptr) {
        auto surfaceBuffer = frameBuffer->avBuffer->memory_->GetSurfaceBuffer();
        CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, -1, "surfaceBuffer is nullptr");
        auto bufferHandle = surfaceBuffer->GetBufferHandle();
        CHECK_AND_RETURN_RET_LOG(bufferHandle != nullptr, -1, "fail to get bufferHandle");
        surfaceBufferStride = bufferHandle->stride;
    } else {
        surfaceBufferStride = frameBuffer->sMemory->GetSurfaceBufferStride();
    }
    return surfaceBufferStride;
}

void HevcDecoder::ReleaseBuffers()
{
    ResetData();
    if (!isBufferAllocated_) {
        return;
    }

    inputAvailQue_->Clear();
    buffers_[INDEX_INPUT].clear();

    std::unique_lock<std::mutex> oLock(outputMutex_);
    codecAvailQue_->Clear();
    if (sInfo_.surface != nullptr) {
        renderAvailQue_->Clear();
        renderSurfaceBufferMap_.clear();
        for (uint32_t i = 0; i < buffers_[INDEX_OUTPUT].size(); i++) {
            std::shared_ptr<HBuffer> outputBuffer = buffers_[INDEX_OUTPUT][i];
            if (outputBuffer->owner_ == HBuffer::Owner::OWNED_BY_CODEC) {
                std::shared_ptr<FSurfaceMemory> surfaceMemory = outputBuffer->sMemory;
                surfaceMemory->SetNeedRender(false);
                surfaceMemory->ReleaseSurfaceBuffer();
                outputBuffer->owner_ = HBuffer::Owner::OWNED_BY_SURFACE;
            }
        }
    }
    buffers_[INDEX_OUTPUT].clear();
    oLock.unlock();
    isBufferAllocated_ = false;
}

int32_t HevcDecoder::QueueInputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, AVCS_ERR_INVALID_STATE,
                             "Queue input buffer failed: not in Running state");
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_INPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Queue input buffer failed with bad index, index=%{public}u, buffer_size=%{public}zu",
                             index, buffers_[INDEX_INPUT].size());
    std::shared_ptr<HBuffer> inputBuffer = buffers_[INDEX_INPUT][index];
    CHECK_AND_RETURN_RET_LOG(inputBuffer->owner_ == HBuffer::Owner::OWNED_BY_USER, AVCS_ERR_INVALID_OPERATION,
                             "Queue input buffer failed: buffer with index=%{public}u is not available", index);

    inputBuffer->owner_ = HBuffer::Owner::OWNED_BY_CODEC;
    inputAvailQue_->Push(index);
    return AVCS_ERR_OK;
}

void HevcDecoder::SendFrame()
{
    if (state_ == State::STOPPING || state_ == State::FLUSHING) {
        return;
    } else if (state_ != State::RUNNING || isSendEos_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    uint32_t index = inputAvailQue_->Front();
    CHECK_AND_RETURN_LOG(state_ == State::RUNNING, "Not in running state");
    std::shared_ptr<HBuffer> &inputBuffer = buffers_[INDEX_INPUT][index];
    std::shared_ptr<AVBuffer> &inputAVBuffer = inputBuffer->avBuffer;
    if (inputAVBuffer->flag_ & AVCODEC_BUFFER_FLAG_EOS) {
        hevcDecoderInputArgs_.pStream = nullptr;
        isSendEos_ = true;
        AVCODEC_LOGI("Send eos end");
    } else {
        hevcDecoderInputArgs_.pStream = inputAVBuffer->memory_->GetAddr();
        hevcDecoderInputArgs_.uiStreamLen = static_cast<UINT32>(inputAVBuffer->memory_->GetSize());
        hevcDecoderInputArgs_.uiTimeStamp = static_cast<UINT64>(inputAVBuffer->pts_);
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
    } while (ret == 0 && isSendEos_);
    runLock.unlock();

    if (isSendEos_) {
        auto outIndex = codecAvailQue_->Front();
        std::shared_ptr<HBuffer> frameBuffer = buffers_[INDEX_OUTPUT][outIndex];
        frameBuffer->avBuffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
        FramePostProcess(buffers_[INDEX_OUTPUT][outIndex], outIndex, AVCS_ERR_OK, AVCS_ERR_OK);
        state_ = State::EOS;
    } else if (ret < 0) {
        AVCODEC_LOGE("decode frame error: ret = %{public}d", ret);
    }

    inputAvailQue_->Pop();
    inputBuffer->owner_ = HBuffer::Owner::OWNED_BY_USER;
    callback_->OnInputBufferAvailable(index, inputAVBuffer);
}

int32_t HevcDecoder::DecodeFrameOnce()
{
    int32_t ret = 0;
    if (hevcSDecoder_ != nullptr && hevcDecoderFlushFrameFunc_ != nullptr &&
        hevcDecoderDecodecFrameFunc_ != nullptr) {
        if (isSendEos_) {
            ret = hevcDecoderFlushFrameFunc_(hevcSDecoder_, &hevcDecoderOutpusArgs_);
        } else {
            ret = hevcDecoderDecodecFrameFunc_(hevcSDecoder_, &hevcDecoderInputArgs_, &hevcDecoderOutpusArgs_);
        }
    } else {
        AVCODEC_LOGW("hevcDecoderDecodecFrameFunc_ = nullptr || hevcSDecoder_ = nullptr || "
                        "hevcDecoderFlushFrameFunc_ = nullptr, cannot call decoder");
        ret = -1;
    }
    int32_t bitDepth = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecBitDepth);
    if (ret == 0) {
        CHECK_AND_RETURN_RET_LOG(bitDepth == BIT_DEPTH8BIT || bitDepth == BIT_DEPTH10BIT, -1,
                                 "Unsupported bitDepth %{public}d", bitDepth);
        ConvertDecOutToAVFrame(bitDepth);
#ifdef BUILD_ENG_VERSION
        DumpOutputBuffer(bitDepth);
#endif
        auto index = codecAvailQue_->Front();
        CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
        std::shared_ptr<HBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
        int32_t status = AVCS_ERR_OK;
        if (CheckFormatChange(index, cachedFrame_->width, cachedFrame_->height, bitDepth) == AVCS_ERR_OK) {
            CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
            frameBuffer = buffers_[INDEX_OUTPUT][index];
            status = FillFrameBuffer(frameBuffer);
        } else {
            CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
            callback_->OnError(AVCODEC_ERROR_EXTEND_START, AVCS_ERR_NO_MEMORY);
            return -1;
        }
        frameBuffer->avBuffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        FramePostProcess(frameBuffer, index, status, AVCS_ERR_OK);
    }
    return ret;
}

int32_t HevcDecoder::FillFrameBuffer(const std::shared_ptr<HBuffer> &frameBuffer)
{
    VideoPixelFormat targetPixelFmt = outputPixelFmt_;
    if (outputPixelFmt_ == VideoPixelFormat::UNKNOWN) {
        targetPixelFmt = VideoPixelFormat::NV12;
    }
    AVPixelFormat ffmpegFormat;
    if (bitDepth_ == BIT_DEPTH10BIT) {
        ffmpegFormat = AVPixelFormat::AV_PIX_FMT_P010LE;
    } else {
        ffmpegFormat = ConvertPixelFormatToFFmpeg(targetPixelFmt);
    }
    // yuv420 -> nv12 or nv21
    int32_t ret = ConvertVideoFrame(&scale_, cachedFrame_, scaleData_, scaleLineSize_, ffmpegFormat);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Scale video frame failed: %{public}d", ret);
    isConverted_ = true;

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(targetPixelFmt));
    std::shared_ptr<AVMemory> &bufferMemory = frameBuffer->avBuffer->memory_;
    CHECK_AND_RETURN_RET_LOG(bufferMemory != nullptr, AVCS_ERR_INVALID_VAL, "bufferMemory is nullptr");
    bufferMemory->SetSize(0);
    struct SurfaceInfo surfaceInfo;
    surfaceInfo.scaleData = scaleData_;
    surfaceInfo.scaleLineSize = scaleLineSize_;
    int32_t surfaceStride = GetSurfaceBufferStride(frameBuffer);
    CHECK_AND_RETURN_RET_LOG(surfaceStride > 0, AVCS_ERR_INVALID_VAL, "get GetSurfaceBufferStride failed");
    surfaceInfo.surfaceStride = static_cast<uint32_t>(surfaceStride);
    if (sInfo_.surface) {
        surfaceInfo.surfaceFence = frameBuffer->sMemory->GetFence();
        ret = WriteSurfaceData(bufferMemory, surfaceInfo, format_);
    } else {
        Format bufferFormat;
        bufferFormat.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height_);
        bufferFormat.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, surfaceStride);
        bufferFormat.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(targetPixelFmt));
        ret = WriteBufferData(bufferMemory, scaleData_, scaleLineSize_, bufferFormat);
    }
#ifdef BUILD_ENG_VERSION
    DumpConvertOut(surfaceInfo);
#endif
    frameBuffer->avBuffer->pts_ = cachedFrame_->pts;
    AVCODEC_LOGD("Fill frame buffer successful");
    return ret;
}

void HevcDecoder::FramePostProcess(std::shared_ptr<HBuffer> &frameBuffer, uint32_t index, int32_t status, int ret)
{
    if (status == AVCS_ERR_OK) {
        codecAvailQue_->Pop();
        frameBuffer->owner_ = HBuffer::Owner::OWNED_BY_USER;
        if (sInfo_.surface) {
            outAVBuffer4Surface_[index]->pts_ = frameBuffer->avBuffer->pts_;
            outAVBuffer4Surface_[index]->flag_ = frameBuffer->avBuffer->flag_;
        }
        callback_->OnOutputBufferAvailable(index, sInfo_.surface ?
            outAVBuffer4Surface_[index] : frameBuffer->avBuffer);
    } else if (status == AVCS_ERR_UNSUPPORT) {
        AVCODEC_LOGE("Recevie frame from codec failed: OnError");
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT);
        state_ = State::ERROR;
    } else {
        AVCODEC_LOGE("Recevie frame from codec failed");
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
        state_ = State::ERROR;
    }
}

void HevcDecoder::ConvertDecOutToAVFrame(int32_t bitDepth)
{
    if (cachedFrame_ == nullptr) {
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *p) { av_frame_free(&p); });
    }
    cachedFrame_->data[0] = hevcDecoderOutpusArgs_.pucOutYUV[0];
    cachedFrame_->data[1] = hevcDecoderOutpusArgs_.pucOutYUV[1]; // 1 u channel
    cachedFrame_->data[2] = hevcDecoderOutpusArgs_.pucOutYUV[2]; // 2 v channel
    if (bitDepth == BIT_DEPTH8BIT) {
        cachedFrame_->format = static_cast<int>(AVPixelFormat::AV_PIX_FMT_YUV420P);
        cachedFrame_->linesize[0] = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride);
        cachedFrame_->linesize[1] = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride >> 1); // 1 u channel
        cachedFrame_->linesize[2] = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride >> 1); // 2 v channel
    } else {
        cachedFrame_->format = static_cast<int>(AVPixelFormat::AV_PIX_FMT_YUV420P10LE);
        cachedFrame_->linesize[0] =
            static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride * 2); // 2 10bit per pixel 2bytes
        cachedFrame_->linesize[1] = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride); // 1 u channel
        cachedFrame_->linesize[2] = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride); // 2 v channel
        if (outputPixelFmt_ == VideoPixelFormat::NV21) { // exchange uv channel
            cachedFrame_->data[1] = hevcDecoderOutpusArgs_.pucOutYUV[2]; // 2 u -> v
            cachedFrame_->data[2] = hevcDecoderOutpusArgs_.pucOutYUV[1]; // 2 v -> u
        }
    }
    cachedFrame_->width = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecWidth);
    cachedFrame_->height = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecHeight);
    cachedFrame_->pts = static_cast<int64_t>(hevcDecoderOutpusArgs_.uiTimeStamp);
}

#ifdef BUILD_ENG_VERSION
void HevcDecoder::DumpOutputBuffer(int32_t bitDepth)
{
    if (!dumpOutFile_ || !dumpOutFile_->is_open()) {
        return;
    }
    int32_t pixelBytes = 1;
    if (bitDepth == BIT_DEPTH10BIT) {
        pixelBytes = 2; // 2
    }
    for (int32_t i = 0; i < cachedFrame_->height; i++) {
        dumpOutFile_->write(reinterpret_cast<char *>(cachedFrame_->data[0] + i * cachedFrame_->linesize[0]),
                            static_cast<int32_t>(cachedFrame_->width * pixelBytes));
    }
    for (int32_t i = 0; i < cachedFrame_->height / 2; i++) {  // 2
        dumpOutFile_->write(reinterpret_cast<char *>(cachedFrame_->data[1] + i * cachedFrame_->linesize[1]),
                            static_cast<int32_t>(cachedFrame_->width * pixelBytes / 2));  // 2
    }
    for (int32_t i = 0; i < cachedFrame_->height / 2; i++) {  // 2
        dumpOutFile_->write(reinterpret_cast<char *>(cachedFrame_->data[2] + i * cachedFrame_->linesize[2]),
                            static_cast<int32_t>(cachedFrame_->width * pixelBytes / 2)); // 2
    }
}

void HevcDecoder::DumpConvertOut(struct SurfaceInfo &surfaceInfo)
{
    if (!dumpConvertFile_ || !dumpConvertFile_->is_open()) {
        return;
    }
    if (surfaceInfo.scaleData[0] != nullptr) {
        int32_t srcPos = 0;
        int32_t dataSize = surfaceInfo.scaleLineSize[0];
        int32_t writeSize = dataSize > static_cast<int32_t>(surfaceInfo.surfaceStride) ?
            static_cast<int32_t>(surfaceInfo.surfaceStride) : dataSize;
        for (int32_t i = 0; i < height_; i++) {
            dumpConvertFile_->write(reinterpret_cast<char *>(surfaceInfo.scaleData[0] + srcPos), writeSize);
            srcPos += dataSize;
        }
        srcPos = 0;
        dataSize = surfaceInfo.scaleLineSize[1];
        writeSize = dataSize > static_cast<int32_t>(surfaceInfo.surfaceStride) ?
            static_cast<int32_t>(surfaceInfo.surfaceStride) : dataSize;
        for (int32_t i = 0; i < height_ / 2; i++) {  // 2
            dumpConvertFile_->write(reinterpret_cast<char *>(surfaceInfo.scaleData[1] + srcPos), writeSize);
            srcPos += dataSize;
        }
    }
}
#endif

void HevcDecoder::FindAvailIndex(uint32_t index)
{
    uint32_t curQueSize = renderAvailQue_->Size();
    for (uint32_t i = 0u; i < curQueSize; i++) {
        uint32_t num = renderAvailQue_->Pop();
        if (num == index) {
            break;
        } else {
            renderAvailQue_->Push(num);
        }
    }
}

void HevcDecoder::RequestBufferFromConsumer()
{
    auto index = renderAvailQue_->Front();
    std::shared_ptr<HBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
    std::shared_ptr<FSurfaceMemory> surfaceMemory = outputBuffer->sMemory;
    sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
    if (surfaceBuffer == nullptr) {
        AVCODEC_LOGE("get buffer failed.");
        return;
    }
    auto queSize = renderAvailQue_->Size();
    uint32_t curIndex = 0;
    uint32_t i = 0;
    for (i = 0; i < queSize; i++) {
        curIndex = renderAvailQue_->Pop();
        if (surfaceMemory->GetBase() == buffers_[INDEX_OUTPUT][curIndex]->avBuffer->memory_->GetAddr() &&
            surfaceMemory->GetSize() == buffers_[INDEX_OUTPUT][curIndex]->avBuffer->memory_->GetCapacity()) {
            buffers_[INDEX_OUTPUT][index]->sMemory = buffers_[INDEX_OUTPUT][curIndex]->sMemory;
            buffers_[INDEX_OUTPUT][curIndex]->sMemory = surfaceMemory;
            break;
        } else {
            renderAvailQue_->Push(curIndex);
        }
    }
    if (i == queSize) {
        curIndex = index;
        outputBuffer->avBuffer = AVBuffer::CreateAVBuffer(surfaceMemory->GetBase(), surfaceMemory->GetSize());
        outputBuffer->width = width_;
        outputBuffer->height = height_;
        FindAvailIndex(curIndex);
    }
    buffers_[INDEX_OUTPUT][curIndex]->owner_ = HBuffer::Owner::OWNED_BY_CODEC;
    codecAvailQue_->Push(curIndex);
    if (renderSurfaceBufferMap_.count(curIndex)) {
        renderSurfaceBufferMap_.erase(curIndex);
    }
    AVCODEC_LOGD("Request output buffer success, index = %{public}u, queSize=%{public}zu, i=%{public}d", curIndex,
                 queSize, i);
}

GSError HevcDecoder::BufferReleasedByConsumer(uint64_t surfaceId)
{
    CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING || state_ == State::EOS, GSERROR_NO_PERMISSION,
                             "In valid state");
    std::lock_guard<std::mutex> sLock(surfaceMutex_);
    CHECK_AND_RETURN_RET_LOG(renderAvailQue_->Size() > 0, GSERROR_NO_BUFFER, "No available buffer");
    CHECK_AND_RETURN_RET_LOG(surfaceId == sInfo_.surface->GetUniqueId(), GSERROR_INVALID_ARGUMENTS,
                             "Ignore callback from old surface");
    RequestBufferFromConsumer();
    return GSERROR_OK;
}

int32_t HevcDecoder::UnRegisterListenerToSurface(const sptr<Surface> &surface)
{
    GSError err = surface->UnRegisterReleaseListener();
    CHECK_AND_RETURN_RET_LOG(err == GSERROR_OK, AVCS_ERR_UNKNOWN,
                             "surface %{public}" PRIu64 ", UnRegisterReleaseListener failed, GSError=%{public}d",
                             surface->GetUniqueId(), err);
    return AVCS_ERR_OK;
}

GSError HevcDecoder::RegisterListenerToSurface(const sptr<Surface> &surface)
{
    uint64_t surfaceId = surface->GetUniqueId();
    wptr<HevcDecoder> wp = this;
    GSError err = surface->RegisterReleaseListener([wp, surfaceId](sptr<SurfaceBuffer> &) {
        sptr<HevcDecoder> codec = wp.promote();
        if (!codec) {
            AVCODEC_LOGD("decoder is gone");
            return GSERROR_OK;
        }
        return codec->BufferReleasedByConsumer(surfaceId);
    });
    return err;
}

int32_t HevcDecoder::ReleaseOutputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    std::unique_lock<std::mutex> oLock(outputMutex_);
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_OUTPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Failed to release output buffer: invalid index");
    std::shared_ptr<HBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
    oLock.unlock();
    if (frameBuffer->owner_ == HBuffer::Owner::OWNED_BY_USER) {
        frameBuffer->owner_ = HBuffer::Owner::OWNED_BY_CODEC;
        codecAvailQue_->Push(index);
        return AVCS_ERR_OK;
    } else {
        AVCODEC_LOGE("Release output buffer failed: check your index=%{public}u", index);
        return AVCS_ERR_INVALID_VAL;
    }
}

int32_t HevcDecoder::FlushSurfaceMemory(std::shared_ptr<FSurfaceMemory> &surfaceMemory, uint32_t index)
{
    sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_INVALID_VAL,
                             "Failed to update surface memory: surface buffer is NULL");
    OHOS::BufferFlushConfig flushConfig = {{0, 0, surfaceBuffer->GetWidth(), surfaceBuffer->GetHeight()},
        outAVBuffer4Surface_[index]->pts_};
    surfaceMemory->SetNeedRender(true);
    surfaceMemory->UpdateSurfaceBufferScaleMode();
    auto res = sInfo_.surface->FlushBuffer(surfaceBuffer, -1, flushConfig);
    if (res != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        AVCODEC_LOGW("Failed to update surface memory: %{public}d", res);
        surfaceMemory->SetNeedRender(false);
        surfaceMemory->ReleaseSurfaceBuffer();
        return AVCS_ERR_UNKNOWN;
    }
    renderSurfaceBufferMap_[index] = surfaceBuffer;
    surfaceMemory->ReleaseSurfaceBuffer();
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::RenderOutputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AVCS_ERR_UNSUPPORT,
                             "RenderOutputBuffer fail, surface is nullptr");
    std::unique_lock<std::mutex> oLock(outputMutex_);
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_OUTPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Failed to render output buffer: invalid index");
    std::shared_ptr<HBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
    oLock.unlock();
    std::lock_guard<std::mutex> sLock(surfaceMutex_);
    if (frameBuffer->owner_ == HBuffer::Owner::OWNED_BY_USER) {
        std::shared_ptr<FSurfaceMemory> surfaceMemory = frameBuffer->sMemory;
        int32_t ret = FlushSurfaceMemory(surfaceMemory, index);
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGW("Update surface memory failed: %{public}d", static_cast<int32_t>(ret));
        } else {
            AVCODEC_LOGD("Update surface memory successful");
        }
        frameBuffer->owner_ = HBuffer::Owner::OWNED_BY_SURFACE;
        renderAvailQue_->Push(index);
        AVCODEC_LOGD("render output buffer with index, index=%{public}u", index);
        return AVCS_ERR_OK;
    } else {
        AVCODEC_LOGE("Failed to render output buffer with bad index, index=%{public}u", index);
        return AVCS_ERR_INVALID_VAL;
    }
}

int32_t HevcDecoder::ReplaceOutputSurfaceWhenRunning(sptr<Surface> newSurface)
{
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AV_ERR_OPERATE_NOT_PERMIT,
                             "Not support convert from AVBuffer Mode to Surface Mode");
    sptr<Surface> curSurface = sInfo_.surface;
    uint64_t oldId = curSurface->GetUniqueId();
    uint64_t newId = newSurface->GetUniqueId();
    AVCODEC_LOGI("surface %{public}" PRIu64 " -> %{public}" PRIu64 "", oldId, newId);
    if (oldId == newId) {
        return AVCS_ERR_OK;
    }
    GSError err = RegisterListenerToSurface(newSurface);
    CHECK_AND_RETURN_RET_LOG(err == GSERROR_OK, AVCS_ERR_UNKNOWN,
                             "surface %{public}" PRIu64 ", RegisterListenerToSurface failed, GSError=%{public}d",
                             newSurface->GetUniqueId(), err);
    int32_t outputBufferCnt = 0;
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, outputBufferCnt);
    int32_t ret = SetQueueSize(newSurface, outputBufferCnt);
    if (ret != AVCS_ERR_OK) {
        UnRegisterListenerToSurface(newSurface);
        return ret;
    }
    std::unique_lock<std::mutex> sLock(surfaceMutex_);
    ret = SwitchBetweenSurface(newSurface);
    if (ret != AVCS_ERR_OK) {
        UnRegisterListenerToSurface(newSurface);
        sInfo_.surface = curSurface;
        return ret;
    }
    sLock.unlock();
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::SetQueueSize(const sptr<Surface> &surface, uint32_t targetSize)
{
    int32_t err = surface->SetQueueSize(targetSize);
    if (err != 0) {
        AVCODEC_LOGE("surface %{public}" PRIu64 ", SetQueueSize to %{public}u failed, GSError=%{public}d",
            surface->GetUniqueId(), targetSize, err);
        return AVCS_ERR_UNKNOWN;
    }
    AVCODEC_LOGI("surface %{public}" PRIu64 ", SetQueueSize to %{public}u succ", surface->GetUniqueId(), targetSize);
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::SwitchBetweenSurface(const sptr<Surface> &newSurface)
{
    sptr<Surface> curSurface = sInfo_.surface;
    newSurface->Connect(); // cleancache will work only if the surface is connected by us
    newSurface->CleanCache(); // make sure new surface is empty
    std::vector<uint32_t> ownedBySurfaceBufferIndex;
    uint64_t newId = newSurface->GetUniqueId();
    for (uint32_t index = 0; index < buffers_[INDEX_OUTPUT].size(); index++) {
        if (buffers_[INDEX_OUTPUT][index]->sMemory == nullptr) {
            continue;
        }
        sptr<SurfaceBuffer> surfaceBuffer = nullptr;
        if (buffers_[INDEX_OUTPUT][index]->owner_ == HBuffer::Owner::OWNED_BY_SURFACE) {
            if (renderSurfaceBufferMap_.count(index)) {
                surfaceBuffer = renderSurfaceBufferMap_[index];
                ownedBySurfaceBufferIndex.push_back(index);
            }
        } else {
            surfaceBuffer = buffers_[INDEX_OUTPUT][index]->sMemory->GetSurfaceBuffer();
        }
        if (surfaceBuffer == nullptr) {
            AVCODEC_LOGE("Get old surface buffer error!");
            return AVCS_ERR_UNKNOWN;
        }
        int32_t err = newSurface->AttachBufferToQueue(surfaceBuffer);
        if (err != 0) {
            AVCODEC_LOGE("surface %{public}" PRIu64 ", AttachBufferToQueue(seq=%{public}u) failed, GSError=%{public}d",
                newId, surfaceBuffer->GetSeqNum(), err);
            return AVCS_ERR_UNKNOWN;
        }
    }
    int32_t videoRotation = 0;
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, videoRotation);
    newSurface->SetTransform(TranslateSurfaceRotation(static_cast<VideoRotation>(videoRotation)));
    sInfo_.surface = newSurface;

    for (uint32_t index: ownedBySurfaceBufferIndex) {
        int32_t ret = RenderNewSurfaceWithOldBuffer(newSurface, index);
        if (ret != AVCS_ERR_OK) {
            return ret;
        }
    }

    int32_t ret = UnRegisterListenerToSurface(curSurface);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    curSurface->CleanCache(true); // make sure old surface is empty and go black
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::RenderNewSurfaceWithOldBuffer(const sptr<Surface> &newSurface, uint32_t index)
{
    std::shared_ptr<FSurfaceMemory> surfaceMemory = buffers_[INDEX_OUTPUT][index]->sMemory;
    sptr<SurfaceBuffer> surfaceBuffer = renderSurfaceBufferMap_[index];
    OHOS::BufferFlushConfig flushConfig = {{0, 0, surfaceBuffer->GetWidth(), surfaceBuffer->GetHeight()},
        outAVBuffer4Surface_[index]->pts_};
    surfaceMemory->SetNeedRender(true);
    newSurface->SetScalingMode(surfaceBuffer->GetSeqNum(), sInfo_.scalingMode);
    auto res = newSurface->FlushBuffer(surfaceBuffer, -1, flushConfig);
    if (res != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        AVCODEC_LOGE("Failed to update surface memory: %{public}d", res);
        surfaceMemory->SetNeedRender(false);
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::SetOutputSurface(sptr<Surface> surface)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(state_ != State::UNINITIALIZED, AV_ERR_INVALID_VAL,
                             "set output surface fail: not initialized or configured");
    CHECK_AND_RETURN_RET_LOG((state_ == State::CONFIGURED || state_ == State::FLUSHED ||
        state_ == State::RUNNING || state_ == State::EOS), AVCS_ERR_INVALID_STATE,
        "set output surface fail: state %{public}d not support set output surface",
        static_cast<int32_t>(state_.load()));
    if (surface == nullptr || surface->IsConsumer()) {
        AVCODEC_LOGE("Set surface fail");
        return AVCS_ERR_INVALID_VAL;
    }
    if (state_ == State::FLUSHED || state_ == State::RUNNING || state_ == State::EOS) {
        return ReplaceOutputSurfaceWhenRunning(surface);
    }
    sInfo_.surface = surface;
    GSError err = RegisterListenerToSurface(sInfo_.surface);
    CHECK_AND_RETURN_RET_LOG(err == GSERROR_OK, AVCS_ERR_UNKNOWN,
                             "surface %{public}" PRIu64 ", RegisterListenerToSurface failed, GSError=%{public}d",
                             sInfo_.surface->GetUniqueId(), err);
    if (!format_.ContainKey(MediaDescriptionKey::MD_KEY_SCALE_TYPE)) {
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE,
                            static_cast<int32_t>(ScalingMode::SCALING_MODE_SCALE_TO_WINDOW));
    }
    if (!format_.ContainKey(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE)) {
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE,
                            static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_0));
    }
    AVCODEC_LOGI("Set surface success");
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_INVALID_VAL, "Set callback failed: callback is NULL");
    callback_ = callback;
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::CheckHevcDecLibStatus()
{
    void* handle = dlopen(HEVC_DEC_LIB_PATH, RTLD_LAZY);
    if (handle != nullptr) {
        auto hevcDecoderCreateFunc = reinterpret_cast<CreateHevcDecoderFuncType>(
            dlsym(handle, HEVC_DEC_CREATE_FUNC_NAME));
        auto hevcDecoderDecodecFrameFunc = reinterpret_cast<DecodeFuncType>(
            dlsym(handle, HEVC_DEC_DECODE_FRAME_FUNC_NAME));
        auto hevcDecoderFlushFrameFunc = reinterpret_cast<FlushFuncType>(dlsym(handle, HEVC_DEC_FLUSH_FRAME_FUNC_NAME));
        auto hevcDecoderDeleteFunc = reinterpret_cast<DeleteFuncType>(dlsym(handle, HEVC_DEC_DELETE_FUNC_NAME));
        if (hevcDecoderCreateFunc == nullptr || hevcDecoderDecodecFrameFunc == nullptr ||
            hevcDecoderDeleteFunc == nullptr || hevcDecoderFlushFrameFunc == nullptr) {
                AVCODEC_LOGE("HevcDecoder hevcFuncMatch_ failed!");
                hevcDecoderCreateFunc = nullptr;
                hevcDecoderDecodecFrameFunc = nullptr;
                hevcDecoderFlushFrameFunc = nullptr;
                hevcDecoderDeleteFunc = nullptr;
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

int32_t HevcDecoder::GetCodecCapability(std::vector<CapabilityData> &capaArray)
{
    CHECK_AND_RETURN_RET_LOG(CheckHevcDecLibStatus() == AVCS_ERR_OK, AVCS_ERR_UNSUPPORT,
                             "hevc decoder libs not available");

    for (uint32_t i = 0; i < SUPPORT_HEVC_DECODER_NUM; ++i) {
        CapabilityData capsData;
        capsData.codecName = static_cast<std::string>(SUPPORT_HEVC_DECODER[i].codecName);
        capsData.mimeType = static_cast<std::string>(SUPPORT_HEVC_DECODER[i].mimeType);
        capsData.codecType = AVCODEC_TYPE_VIDEO_DECODER;
        capsData.isVendor = false;
        capsData.maxInstance = VIDEO_INSTANCE_SIZE;
        capsData.alignment.width = VIDEO_ALIGNMENT_SIZE;
        capsData.alignment.height = VIDEO_ALIGNMENT_SIZE;
        capsData.width.minVal = VIDEO_MIN_SIZE;
        capsData.width.maxVal = VIDEO_MAX_WIDTH_SIZE;
        capsData.height.minVal = VIDEO_MIN_SIZE;
        capsData.height.maxVal = VIDEO_MAX_HEIGHT_SIZE;
        capsData.blockPerFrame.minVal = 1;
        capsData.blockPerFrame.maxVal = VIDEO_BLOCKPERFRAME_SIZE;
        capsData.blockPerSecond.minVal = 1;
        capsData.blockPerSecond.maxVal = VIDEO_BLOCKPERSEC_SIZE;
        capsData.blockSize.width = VIDEO_ALIGN_SIZE;
        capsData.blockSize.height = VIDEO_ALIGN_SIZE;
        capsData.pixFormat = {static_cast<int32_t>(VideoPixelFormat::NV12),
            static_cast<int32_t>(VideoPixelFormat::NV21)};
        capsData.profiles = {static_cast<int32_t>(HEVC_PROFILE_MAIN), static_cast<int32_t>(HEVC_PROFILE_MAIN_10)};

        std::vector<int32_t> levels;
        for (int32_t j = 0; j <= static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_62); ++j) {
            levels.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(HEVC_PROFILE_MAIN), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(HEVC_PROFILE_MAIN_10), levels));
        capaArray.emplace_back(capsData);
    }
    return AVCS_ERR_OK;
}

void HevcDecLog(UINT32 channelId, IHW265VIDEO_ALG_LOG_LEVEL eLevel, INT8 *pMsg, ...)
{
    va_list args;
    int32_t maxSize = 1024; // 1024 max size of one log
    std::vector<char> buf(maxSize);
    va_start(args, reinterpret_cast<const char*>(pMsg));
    int32_t size = vsnprintf_s(buf.data(), buf.size(), buf.size()-1, reinterpret_cast<const char*>(pMsg), args);
    va_end(args);
    if (size >= maxSize) {
        size = maxSize - 1;
    }

    auto msg = std::string(buf.data(), size);
    
    if (eLevel <= IHW265VIDEO_ALG_LOG_ERROR) {
        switch (eLevel) {
            case IHW265VIDEO_ALG_LOG_ERROR: {
                AVCODEC_LOGE("%{public}s", msg.c_str());
                break;
            }
            case IHW265VIDEO_ALG_LOG_WARNING: {
                AVCODEC_LOGW("%{public}s", msg.c_str());
                break;
            }
            case IHW265VIDEO_ALG_LOG_INFO: {
                AVCODEC_LOGI("%{public}s", msg.c_str());
                break;
            }
            case IHW265VIDEO_ALG_LOG_DEBUG: {
                AVCODEC_LOGD("%{public}s", msg.c_str());
                break;
            }
            default: {
                AVCODEC_LOGI("%{public}s", msg.c_str());
                break;
            }
        }
    }

    return;
}

std::mutex HevcDecoder::decoderCountMutex_;
std::vector<uint32_t> HevcDecoder::decInstanceIDSet_;
std::vector<uint32_t> HevcDecoder::freeIDSet_;

} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
