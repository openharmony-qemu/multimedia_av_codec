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
#include <malloc.h>
#include "syspara/parameters.h"
#include "securec.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "utils.h"
#include "avcodec_codec_name.h"
#include "avc_encoder_util.h"
#include "avc_encoder_convert.h"
#include "avc_encoder.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AvcEncoder"};
constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;
constexpr uint32_t DEFAULT_IN_BUFFER_CNT = 4;
constexpr uint32_t DEFAULT_OUT_BUFFER_CNT = 4;
constexpr uint32_t DEFAULT_MIN_BUFFER_CNT = 2;
constexpr int32_t VIDEO_MIN_BUFFER_SIZE = 1474560;
constexpr int32_t VIDEO_MIN_SIZE = 64;
constexpr int32_t VIDEO_ALIGNMENT_SIZE = 2;
constexpr int32_t VIDEO_MAX_WIDTH_SIZE = 2560;
constexpr int32_t VIDEO_MAX_HEIGHT_SIZE = 2560;
constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
constexpr uint32_t DEFAULT_TRY_ENCODE_TIME = 100;
constexpr uint32_t DEFAULT_ENCODE_WAIT_TIME = 200;
constexpr int32_t VIDEO_INSTANCE_SIZE = 16;
constexpr int32_t VIDEO_BITRATE_MIN_SIZE = 10000;
constexpr int32_t VIDEO_BITRATE_MAX_SIZE = 30000000;
constexpr int32_t VIDEO_FRAMERATE_MIN_SIZE = 1;
constexpr int32_t VIDEO_FRAMERATE_MAX_SIZE = 60;
constexpr int32_t VIDEO_BLOCKPERFRAME_SIZE = 36864;
constexpr int32_t VIDEO_BLOCKPERSEC_SIZE = 983040;
constexpr int32_t VIDEO_QUALITY_MAX = 100;
constexpr int32_t VIDEO_QUALITY_MIN = 0;
constexpr int32_t TIME_SEC_TO_MS = 1000;
constexpr int32_t VIDEO_QP_MAX = 51;
constexpr int32_t VIDEO_QP_MIN = 4;
constexpr int32_t VIDEO_QP_DEFAULT = 20;
constexpr int32_t VIDEO_IFRAME_INTERVAL_MIN_TIME = 1000;
constexpr int32_t VIDEO_IFRAME_INTERVAL_MAX_TIME = 3600000;
constexpr int32_t DEFAULT_VIDEO_INTERVAL_TIME = 2000;
constexpr int32_t DEFAULT_VIDEO_IFRAME_INTERVAL = 60;
constexpr int32_t DEFAULT_VIDEO_BITRATE = 6000000;
constexpr double DEFAULT_VIDEO_FRAMERATE = 30.0;
constexpr int32_t VIDEO_ALIGN_SIZE = 16;
constexpr uint32_t VIDEO_PIX_DEPTH_RGBA = 4;

constexpr struct {
    const std::string_view codecName;
    const std::string_view mimeType;
    const char *codecStr;
    const bool isEncoder;
} SUPPORT_VCODEC[] = {
    {AVCodecCodecName::VIDEO_ENCODER_AVC_NAME, CodecMimeType::VIDEO_AVC, "h264", true},
};
constexpr uint32_t SUPPORT_VCODEC_NUM = sizeof(SUPPORT_VCODEC) / sizeof(SUPPORT_VCODEC[0]);
} // namespace

const char *AVC_ENC_LIB_PATH = "libavcenc_ohos.z.so";
const char *AVC_ENC_CREATE_FUNC_NAME = "InitEncoder";
const char *AVC_ENC_ENCODE_FRAME_FUNC_NAME = "EncodeProcess";
const char *AVC_ENC_DELETE_FUNC_NAME = "ReleaseEncoder";

using namespace OHOS::Media;

void AvcEncLog(uint32_t channelId, IHW264VIDEO_ALG_LOG_LEVEL eLevel, uint8_t *pMsg, ...)
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

    if (eLevel >= IHW264VIDEO_ALG_LOG_DEBUG) {
        switch (eLevel) {
            case IHW264VIDEO_ALG_LOG_ERROR: {
                AVCODEC_LOGE("%{public}s", msg.c_str());
                break;
            }
            case IHW264VIDEO_ALG_LOG_WARNING: {
                AVCODEC_LOGW("%{public}s", msg.c_str());
                break;
            }
            case IHW264VIDEO_ALG_LOG_INFO: {
                AVCODEC_LOGI("%{public}s", msg.c_str());
                break;
            }
            case IHW264VIDEO_ALG_LOG_DEBUG: {
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

AvcEncoder::AvcEncoder(const std::string &name)
    : codecName_(name),
      state_(State::UNINITIALIZED),
      encWidth_(DEFAULT_VIDEO_WIDTH),
      encHeight_(DEFAULT_VIDEO_HEIGHT),
      encBitrate_(DEFAULT_VIDEO_BITRATE),
      encQp_(VIDEO_QP_DEFAULT),
      encQpMax_(VIDEO_QP_MAX),
      encQpMin_(VIDEO_QP_MIN),
      encIperiod_(DEFAULT_VIDEO_IFRAME_INTERVAL),
      encFrameRate_(DEFAULT_VIDEO_FRAMERATE)
{
    AVCODEC_SYNC_TRACE;
    std::unique_lock<std::mutex> lock(encoderCountMutex_);
    if (!freeIDSet_.empty()) {
        encInstanceID_ = freeIDSet_[0];
        freeIDSet_.erase(freeIDSet_.begin());
        encInstanceIDSet_.push_back(encInstanceID_);
    } else if (freeIDSet_.size() + encInstanceIDSet_.size() < VIDEO_INSTANCE_SIZE) {
        encInstanceID_ = freeIDSet_.size() + encInstanceIDSet_.size();
        encInstanceIDSet_.push_back(encInstanceID_);
    } else {
        encInstanceID_ = VIDEO_INSTANCE_SIZE + 1;
    }
    lock.unlock();

    if (encInstanceID_ < VIDEO_INSTANCE_SIZE) {
        handle_ = dlopen(AVC_ENC_LIB_PATH, RTLD_LAZY);
        if (handle_ == nullptr) {
            AVCODEC_LOGE("Load avc codec failed: %{public}s", AVC_ENC_LIB_PATH);
        }
        AvcFuncMatch();
        AVCODEC_LOGI("Num %{public}u AvcEncoder entered, state: Uninitialized", encInstanceID_);
    } else {
        AVCODEC_LOGE("AvcEncoder already has %{public}d instances, cannot has more instances", VIDEO_INSTANCE_SIZE);
        state_ = State::ERROR;
    }
    InitAvcEncoderParams();
}

AvcEncoder::~AvcEncoder()
{
    ReleaseResource();
    ReleaseHandle();
    callback_ = nullptr;
    if (encInstanceID_ < VIDEO_INSTANCE_SIZE) {
        std::lock_guard<std::mutex> lock(encoderCountMutex_);
        freeIDSet_.push_back(encInstanceID_);
        auto it = std::find(encInstanceIDSet_.begin(), encInstanceIDSet_.end(), encInstanceID_);
        if (it != encInstanceIDSet_.end()) {
            encInstanceIDSet_.erase(it);
        }
    }
}

void AvcEncoder::AvcFuncMatch()
{
    if (handle_ != nullptr) {
        avcEncoderCreateFunc_ = reinterpret_cast<CreateAvcEncoderFuncType>(dlsym(handle_,
            AVC_ENC_CREATE_FUNC_NAME));
        avcEncoderFrameFunc_ = reinterpret_cast<EncodeFuncType>(dlsym(handle_,
            AVC_ENC_ENCODE_FRAME_FUNC_NAME));
        avcEncoderDeleteFunc_ = reinterpret_cast<DeleteFuncType>(dlsym(handle_,
            AVC_ENC_DELETE_FUNC_NAME));
        if (avcEncoderCreateFunc_ == nullptr ||
            avcEncoderFrameFunc_ == nullptr ||
            avcEncoderDeleteFunc_ == nullptr) {
                AVCODEC_LOGE("AvcEncoder avcFuncMatch_ failed!");
                ReleaseHandle();
        }
    }
}

void AvcEncoder::ReleaseHandle()
{
    std::unique_lock<std::mutex> runLock(encRunMutex_);
    avcEncoderCreateFunc_ = nullptr;
    avcEncoderFrameFunc_ = nullptr;
    avcEncoderDeleteFunc_ = nullptr;
    if (handle_ != nullptr) {
        dlclose(handle_);
        handle_ = nullptr;
    }
    runLock.unlock();
}

void AvcEncoder::ReleaseSurfaceBuffer()
{
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    int64_t pts = -1;
    OHOS::Rect damage;
    GSError ret = inputSurface_->AcquireBuffer(buffer, fence, pts, damage);
    if (ret != GSERROR_OK || buffer == nullptr) {
        return;
    }
    AVCODEC_LOGW("release buffer to surface, seq: %{public}u, pts: %" PRId64 "", buffer->GetSeqNum(), pts);
    inputSurface_->ReleaseBuffer(buffer, -1);
}

void AvcEncoder::ClearDirtyList()
{
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    int64_t pts = -1;
    OHOS::Rect damage;
    while (true) {
        GSError ret = inputSurface_->AcquireBuffer(buffer, fence, pts, damage);
        if (ret != GSERROR_OK || buffer == nullptr) {
            return;
        }
        AVCODEC_LOGI("release surface buffer, seq: %{public}u, pts: %{public}" PRId64 "", buffer->GetSeqNum(), pts);
        inputSurface_->ReleaseBuffer(buffer, -1);
    }
}

void AvcEncoder::WaitForInBuffer()
{
    std::unique_lock<std::mutex> listLock(freeListMutex_);
    surfaceRecvCv_.wait(listLock, [this] {
        if (state_ == State::STOPPING) {
            return true;
        }

        if (!freeList_.empty()) {
            return true;
        }

        return false;
    });
}

void AvcEncoder::GetBufferFromSurface()
{
    CHECK_AND_RETURN_LOG(inputSurface_ != nullptr, "inputSurface_ not exists");
    if (freeList_.empty()) {
        WaitForInBuffer();
    }
    if (state_ == State::STOPPING) {
        ReleaseSurfaceBuffer();
        AVCODEC_LOGE("surface exit .");
        return;
    }

    sptr<SurfaceBuffer> buffer = nullptr;
    sptr<SyncFence> fence = nullptr;
    OHOS::Rect damage;
    int64_t pts = -1;
    GSError ret = inputSurface_->AcquireBuffer(buffer, fence, pts, damage);
    if (ret != GSERROR_OK || buffer == nullptr) {
        return;
    }
    if (state_ == State::STOPPING) {
        inputSurface_->ReleaseBuffer(buffer, -1);
        return;
    }

    AVCODEC_LOGD("timestamp: %{public}" PRId64 ", dataSize: %{public}d", pts, buffer->GetSize());
    CHECK_AND_RETURN_LOG(!freeList_.empty(), "freeList_ is empty!");
    std::unique_lock<std::mutex> listLock(freeListMutex_);
    uint32_t index = freeList_.front();
    freeList_.pop_front();
    listLock.unlock();
    std::shared_ptr<FBuffer> &inputBuffer = buffers_[INDEX_INPUT][index];
    CHECK_AND_RETURN_LOG(inputBuffer != nullptr, "input buffer is null");
    inputBuffer->surfaceBuffer_ = buffer;
    inputBuffer->fence_ = fence;
    CHECK_AND_RETURN_LOG(inputBuffer->avBuffer_ != nullptr, "Allocate input buffer failed!");
    inputBuffer->avBuffer_->pts_ = pts;
    if (enableSurfaceModeInputCb_) {
        callback_->OnInputBufferAvailable(index, inputBuffer->avBuffer_);
    } else {
        inputBuffer->owner_ = FBuffer::Owner::OWNED_BY_CODEC;
        inputAvailQue_->Push(index);
    }
}

void AvcEncoder::EncoderBuffersConsumerListener::OnBufferAvailable()
{
    if (codec_) {
        codec_->GetBufferFromSurface();
    }
}

sptr<Surface> AvcEncoder::CreateInputSurface()
{
    if (inputSurface_) {
        AVCODEC_LOGE("inputSurface_ already exists");
        return nullptr;
    }

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("HEncoderSurface");
    if (consumerSurface == nullptr) {
        AVCODEC_LOGE("Create the surface consummer fail");
        return nullptr;
    }
    GSError err = consumerSurface->SetDefaultUsage(SURFACE_MODE_CONSUMER_USAGE);
    if (err == GSERROR_OK) {
        AVCODEC_LOGI("set consumer usage 0x%{public}x succ", SURFACE_MODE_CONSUMER_USAGE);
    } else {
        AVCODEC_LOGW("set consumer usage 0x%{public}x failed", SURFACE_MODE_CONSUMER_USAGE);
    }

    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    if (producer == nullptr) {
        AVCODEC_LOGE("Get the surface producer fail");
        return nullptr;
    }

    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);
    if (producerSurface == nullptr) {
        AVCODEC_LOGE("CreateSurfaceAsProducer fail");
        return nullptr;
    }

    inputSurface_ = consumerSurface;
    if (DEFAULT_IN_BUFFER_CNT > inputSurface_->GetQueueSize()) {
        inputSurface_->SetQueueSize(DEFAULT_IN_BUFFER_CNT);
    }
    AVCODEC_LOGI("succ, surface id = %" PRIu64 ", queue size %u",
          inputSurface_->GetUniqueId(), inputSurface_->GetQueueSize());
    return producerSurface;
}

int32_t AvcEncoder::SetInputSurface(sptr<Surface> surface)
{
    if (inputSurface_) {
        AVCODEC_LOGW("inputSurface_ already exists");
    }

    if (surface == nullptr) {
        AVCODEC_LOGE("surface is null");
        return AVCS_ERR_INVALID_VAL;
    }
    if (!surface->IsConsumer()) {
        AVCODEC_LOGE("expect consumer surface");
        return AVCS_ERR_INVALID_VAL;
    }

    inputSurface_ = surface;
    if (DEFAULT_IN_BUFFER_CNT > inputSurface_->GetQueueSize()) {
        inputSurface_->SetQueueSize(DEFAULT_IN_BUFFER_CNT);
    }
    AVCODEC_LOGI("succ");
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::Initialize()
{
    AVCODEC_SYNC_TRACE;
    std::string codecName;
    std::string_view mime;
    for (uint32_t i = 0; i < SUPPORT_VCODEC_NUM; ++i) {
        if (SUPPORT_VCODEC[i].codecName == codecName_) {
            codecName = SUPPORT_VCODEC[i].codecStr;
            mime = SUPPORT_VCODEC[i].mimeType;
            break;
        }
    }
    CHECK_AND_RETURN_RET_LOG(!codecName.empty(), AVCS_ERR_INVALID_VAL,
                             "Init codec failed: not support name: %{public}s", codecName_.c_str());
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, mime);
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, codecName_);

    sendTask_ = std::make_shared<TaskThread>("SendFrame");
    sendTask_->RegisterHandler([this] { SendFrame(); });
    state_ = State::INITIALIZED;
    isFirstFrame_ = true;
    AVCODEC_LOGI("Init codec successful, state: Uninitialized -> Initialized");
    return AVCS_ERR_OK;
}

void AvcEncoder::ConfigureDefaultVal(const Format &format, const std::string_view &formatKey,
    int32_t minVal, int32_t maxVal)
{
    int32_t val32 = 0;
    if (format.GetIntValue(formatKey, val32) && val32 >= minVal && val32 <= maxVal) {
        format_.PutIntValue(formatKey, val32);
    } else {
        AVCODEC_LOGW("Set parameter failed: %{public}s, which minimum threshold=%{public}d, "
                     "maximum threshold=%{public}d",
                     std::string(formatKey).c_str(), minVal, maxVal);
    }
}

int32_t AvcEncoder::Configure(const Format &format)
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
            ConfigureDefaultVal(format, it.first, DEFAULT_MIN_BUFFER_CNT);
        } else if (it.first == MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT) {
            ConfigureDefaultVal(format, it.first, DEFAULT_MIN_BUFFER_CNT);
        } else if (it.first == MediaDescriptionKey::MD_KEY_WIDTH) {
            ConfigureDefaultVal(format, it.first, VIDEO_MIN_SIZE, VIDEO_MAX_WIDTH_SIZE);
        } else if (it.first == MediaDescriptionKey::MD_KEY_HEIGHT) {
            ConfigureDefaultVal(format, it.first, VIDEO_MIN_SIZE, VIDEO_MAX_HEIGHT_SIZE);
        } else if (it.first == MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL) {
            ConfigureDefaultVal(format, it.first);
        } else if (it.first == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT) {
            ConfigureDefaultVal(format, it.first);
        } else if (it.first == MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE) {
            ConfigureDefaultVal(format, it.first);
        } else if (it.first == OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK) {
            ConfigureDefaultVal(format, it.first);
        } else if (it.first == MediaDescriptionKey::MD_KEY_BITRATE) {
            int64_t val64 = 0;
            format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, val64);
            format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, val64);
        } else if (it.first == MediaDescriptionKey::MD_KEY_FRAME_RATE) {
            double val = 0;
            format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, val);
            format_.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, val);
        } else {
            AVCODEC_LOGW("Set parameter need check: size:%{public}s,", it.first.data());
        }
    }
    int32_t ret = ConfigureContext(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "config error");
    state_ = State::CONFIGURED;
    AVCODEC_LOGI("Configured codec successful, state: Initialized -> Configured");
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::ConfigureContext(const Format &format)
{
    GetBitRateFromUser(format);
    GetFrameRateFromUser(format);
    GetBitRateModeFromUser(format);
    GetIFrameIntervalFromUser(format);
    GetRequestIDRFromUser(format);
    GetQpRangeFromUser(format);
    GetColorAspects(format);
    CheckIfEnableCb(format);
    GetPixelFmtFromUser(format);
    int32_t ret = SetupPort(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_VAL, "config format error");
    return AVCS_ERR_OK;
}

void AvcEncoder::GetQpRangeFromUser(const Format &format)
{
    int32_t minQp;
    int32_t maxQp;
    if (!format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MIN, minQp) ||
        !format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MAX, maxQp)) {
        AVCODEC_LOGE("user set qp range default");
        return;
    }

    encQpMin_ = minQp;
    encQpMax_ = maxQp;
    AVCODEC_LOGI("user set qp min %{public}d, max %{public}d", encQpMin_, encQpMax_);
    return;
}

void AvcEncoder::GetBitRateFromUser(const Format &format)
{
    int64_t bitRateLong;
    if (format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRateLong) && bitRateLong > 0 &&
        bitRateLong <= UINT32_MAX) {
        AVCODEC_LOGI("user set bitrate %{public}" PRId64 "", bitRateLong);
        encBitrate_ = static_cast<int32_t>(bitRateLong);
        CheckBitRateSupport(encBitrate_);
        return;
    }
    int32_t bitRateInt;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRateInt) && bitRateInt > 0) {
        AVCODEC_LOGI("user set bitrate %{public}d", bitRateInt);
        encBitrate_ = bitRateInt;
        CheckBitRateSupport(encBitrate_);
        return;
    }
    return;
}

void AvcEncoder::GetPixelFmtFromUser(const Format &format)
{
    VideoPixelFormat innerFmt;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, *(int *)&innerFmt) &&
        innerFmt != VideoPixelFormat::SURFACE_FORMAT) {
        srcPixelFmt_ = innerFmt;
        AVCODEC_LOGI("configuread pixel fmt %{public}d", static_cast<int32_t>(innerFmt));
    } else {
        AVCODEC_LOGI("user don't set pixel fmt, use default yuv420");
    }
    return;
}

void AvcEncoder::GetFrameRateFromUser(const Format &format)
{
    double frameRateDouble;
    if (format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRateDouble) && frameRateDouble > 0) {
        AVCODEC_LOGI("user set frame rate %{public}.2f", frameRateDouble);
        encFrameRate_ = frameRateDouble;
        CheckFrameRateSupport(encFrameRate_);
        return;
    }
    int frameRateInt;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRateInt) && frameRateInt > 0) {
        AVCODEC_LOGI("user set frame rate %{public}d", frameRateInt);
        encFrameRate_ = static_cast<double>(frameRateInt);
        CheckFrameRateSupport(encFrameRate_);
        return;
    }
    encFrameRate_ = DEFAULT_VIDEO_FRAMERATE; // default frame rate 30.0
    return;
}

void AvcEncoder::GetBitRateModeFromUser(const Format &format)
{
    VideoEncodeBitrateMode mode;
    AVCProfile profile;
    AVCLevel level;
    int32_t quality;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, *reinterpret_cast<int *>(&mode))) {
        AVCODEC_LOGI("user set bitrate mod %{public}d", static_cast<int>(mode));
        bitrateMode_ = mode;
        if (mode == VideoEncodeBitrateMode::CQ &&
            format.GetIntValue(MediaDescriptionKey::MD_KEY_QUALITY, quality) && quality >= 0) {
            avcQuality_ = quality;
            encQp_ = VIDEO_QP_MIN +
                (VIDEO_QUALITY_MAX - quality) * ((VIDEO_QP_MAX - VIDEO_QP_MIN) / VIDEO_QUALITY_MAX);
            AVCODEC_LOGI("user set avc quality %{public}d  qp %{public}d", quality, encQp_);
        }
    }

    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_PROFILE, *reinterpret_cast<int *>(&profile))) {
        AVCODEC_LOGI("user set avc profile %{public}d", static_cast<int>(profile));
        avcProfile_ = profile;
    }

    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_LEVEL, *reinterpret_cast<int *>(&level))) {
        AVCODEC_LOGI("user set avc level %{public}d", static_cast<int>(level));
        avcLevel_ = level;
    }

    return;
}

void AvcEncoder::GetIFrameIntervalFromUser(const Format &format)
{
    int32_t interval;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, interval) && interval > 0) {
        CheckIFrameIntervalTimeSupport(interval);
        encIperiod_ = interval * encFrameRate_ / TIME_SEC_TO_MS;
        AVCODEC_LOGI("user set iframe interval %{public}ds, transfer to %{public}dframes",
            interval / TIME_SEC_TO_MS, encIperiod_);
    }
    return;
}

void AvcEncoder::GetRequestIDRFromUser(const Format &format)
{
    int32_t requestIdr;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_REQUEST_I_FRAME, requestIdr) && requestIdr != 0) {
        SignalRequestIDRFrame();
    }
    return;
}

void AvcEncoder::GetColorAspects(const Format &format)
{
    int range = 0;
    int primary = static_cast<int>(COLOR_PRIMARY_UNSPECIFIED);
    int transfer = static_cast<int>(TRANSFER_CHARACTERISTIC_UNSPECIFIED);
    int matrix = static_cast<int>(MATRIX_COEFFICIENT_UNSPECIFIED);

    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, range)) {
        AVCODEC_LOGI("user set range flag %{public}d", range);
    }
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, primary)) {
        AVCODEC_LOGI("user set primary %{public}d", primary);
    }
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, transfer)) {
        AVCODEC_LOGI("user set transfer %{public}d", transfer);
    }
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, matrix)) {
        AVCODEC_LOGI("user set matrix %{public}d", matrix);
    }
    if (primary < 0 || primary > ColorPrimary::COLOR_PRIMARY_P3D65 ||
        transfer < 0 || transfer > TransferCharacteristic::TRANSFER_CHARACTERISTIC_HLG ||
        matrix < 0 || matrix > MatrixCoefficient::MATRIX_COEFFICIENT_ICTCP) {
        AVCODEC_LOGW("user set invalid color");
        return;
    }

    srcRange_ = static_cast<uint8_t>(range);
    srcPrimary_ = static_cast<ColorPrimary>(primary);
    srcTransfer_ = static_cast<TransferCharacteristic>(transfer);
    srcMatrix_ = static_cast<MatrixCoefficient>(matrix);
    return;
}

void AvcEncoder::CheckIfEnableCb(const Format &format)
{
    int32_t enableCb = 0;
    if (format.GetIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, enableCb)) {
        AVCODEC_LOGI("enable surface mode callback flag %{public}d", enableCb);
        enableSurfaceModeInputCb_ = static_cast<bool>(enableCb);
    }
    return;
}

void AvcEncoder::CheckBitRateSupport(int32_t &bitrate)
{
    if (bitrate > VIDEO_BITRATE_MAX_SIZE || bitrate < VIDEO_BITRATE_MIN_SIZE) {
        AVCODEC_LOGW("bitrate %{public}d not in [%{public}d, %{public}d], set default %{public}d",
            bitrate, VIDEO_BITRATE_MIN_SIZE, VIDEO_BITRATE_MAX_SIZE, DEFAULT_VIDEO_BITRATE);
        bitrate = DEFAULT_VIDEO_BITRATE;
    }
    return;
}

void AvcEncoder::CheckFrameRateSupport(double &framerate)
{
    if (framerate > VIDEO_FRAMERATE_MAX_SIZE || framerate < VIDEO_FRAMERATE_MIN_SIZE) {
        AVCODEC_LOGW("framerate %{public}f not in [%{public}d, %{public}d], set default %{public}f",
            framerate, VIDEO_FRAMERATE_MIN_SIZE, VIDEO_FRAMERATE_MAX_SIZE, DEFAULT_VIDEO_FRAMERATE);
        framerate = DEFAULT_VIDEO_FRAMERATE;
    }
    return;
}

void AvcEncoder::CheckIFrameIntervalTimeSupport(int32_t &interval)
{
    if (interval > VIDEO_IFRAME_INTERVAL_MAX_TIME || interval < VIDEO_IFRAME_INTERVAL_MIN_TIME) {
        AVCODEC_LOGW("interval %{public}d not in [%{public}d, %{public}d], set default %{public}d",
            interval, VIDEO_IFRAME_INTERVAL_MIN_TIME, VIDEO_IFRAME_INTERVAL_MAX_TIME,
            DEFAULT_VIDEO_INTERVAL_TIME);
        interval = DEFAULT_VIDEO_INTERVAL_TIME;
    }
    return;
}

int32_t AvcEncoder::SetupPort(const Format &format)
{
    int32_t width;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width) ||
        width <= 0 || width > VIDEO_MAX_WIDTH_SIZE) {
        AVCODEC_LOGE("format should contain width");
        return AVCS_ERR_INVALID_VAL;
    }
    int32_t height;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height) ||
        height <= 0 || height > VIDEO_MAX_HEIGHT_SIZE) {
        AVCODEC_LOGE("format should contain height");
        return AVCS_ERR_INVALID_VAL;
    }
    encWidth_ = width;
    encHeight_ = height;
    AVCODEC_LOGI("user set width %{public}d, height %{public}d", width, height);
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::SignalRequestIDRFrame()
{
    AVCODEC_LOGI("request idr frame success");
    encIdrRequest_ = true;
    return AVCS_ERR_OK;
}

void AvcEncoder::InitBuffers()
{
    inputAvailQue_->SetActive(true);
    codecAvailQue_->SetActive(true);
    freeList_.clear();
    for (uint32_t i = 0; i < buffers_[INDEX_INPUT].size(); i++) {
        buffers_[INDEX_INPUT][i]->owner_ = FBuffer::Owner::OWNED_BY_USER;
        if (inputSurface_ == nullptr) {
            callback_->OnInputBufferAvailable(i, buffers_[INDEX_INPUT][i]->avBuffer_);
        } else {
            freeList_.emplace_back(i);
        }
        AVCODEC_LOGI("OnInputBufferAvailable frame index = %{public}d, owner = %{public}d",
            i, buffers_[INDEX_INPUT][i]->owner_.load());
    }

    for (uint32_t i = 0; i < buffers_[INDEX_OUTPUT].size(); i++) {
        buffers_[INDEX_OUTPUT][i]->owner_ = FBuffer::Owner::OWNED_BY_CODEC;
        codecAvailQue_->Push(i);
    }
}

void AvcEncoder::CalculateBufferSize()
{
    int32_t stride = AlignUp(encWidth_, VIDEO_ALIGN_SIZE);
    inputBufferSize_ = static_cast<int32_t>(stride * encHeight_ * VIDEO_PIX_DEPTH_RGBA);
    outputBufferSize_ = std::max(VIDEO_MIN_BUFFER_SIZE, outputBufferSize_);
    AVCODEC_LOGI("width = %{public}d, height = %{public}d, stride = %{public}d, Input buffer size = %{public}d, output "
                 "buffer size=%{public}d",
                 encWidth_, encHeight_, stride, inputBufferSize_, outputBufferSize_);
}

int32_t AvcEncoder::AllocateInputBuffer(int32_t bufferCnt, int32_t inBufferSize)
{
    uint32_t valBufferCnt = 0;

    convertBuffer_ = nullptr;
    std::shared_ptr<AVAllocator> alloc =
        AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    CHECK_AND_RETURN_RET_LOG(alloc != nullptr, AVCS_ERR_NO_MEMORY, "convert buffer allocator is nullptr");
    convertBuffer_ = AVBuffer::CreateAVBuffer(alloc, inBufferSize);
    CHECK_AND_RETURN_RET_LOG(convertBuffer_ != nullptr, AVCS_ERR_NO_MEMORY, "Allocate convert buffer failed");

    for (int32_t i = 0; i < bufferCnt; i++) {
        std::shared_ptr<FBuffer> buf = std::make_shared<FBuffer>();
        if (inputSurface_ == nullptr) {
            std::shared_ptr<AVAllocator> allocator =
                AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            CHECK_AND_CONTINUE_LOG(allocator != nullptr, "input buffer %{public}d allocator is nullptr", i);
            buf->avBuffer_ = AVBuffer::CreateAVBuffer(allocator, inBufferSize);
            CHECK_AND_CONTINUE_LOG(buf->avBuffer_ != nullptr && buf->avBuffer_->memory_ != nullptr,
                "Allocate input buffer failed, index=%{public}d", i);
            AVCODEC_LOGI("Allocate input buffer success: index=%{public}d, size=%{public}d",
                i, buf->avBuffer_->memory_->GetCapacity());
        } else {
            buf->avBuffer_ = AVBuffer::CreateAVBuffer();
            CHECK_AND_CONTINUE_LOG(buf->avBuffer_ != nullptr,
                "Allocate input buffer failed (surface mode), index=%{public}d", i);
        }
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

int32_t AvcEncoder::AllocateOutputBuffer(int32_t bufferCnt, int32_t outBufferSize)
{
    uint32_t valBufferCnt = 0;
    for (int i = 0; i < bufferCnt; i++) {
        std::shared_ptr<FBuffer> buf = std::make_shared<FBuffer>();
        std::shared_ptr<AVAllocator> allocator =
            AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        CHECK_AND_CONTINUE_LOG(allocator != nullptr, "output buffer %{public}d allocator is nullptr", i);

        buf->avBuffer_ = AVBuffer::CreateAVBuffer(allocator, outBufferSize);
        CHECK_AND_CONTINUE_LOG(buf->avBuffer_ != nullptr && buf->avBuffer_->memory_ != nullptr,
            "Allocate output buffer failed, index=%{public}d", i);
        AVCODEC_LOGI("Allocate output share buffer success: index=%{public}d, size=%{public}d",
            i, buf->avBuffer_->memory_->GetCapacity());

        buf->width_ = encWidth_;
        buf->height_ = encHeight_;
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

int32_t AvcEncoder::AllocateBuffers()
{
    AVCODEC_SYNC_TRACE;
    CalculateBufferSize();
    CHECK_AND_RETURN_RET_LOG(inputBufferSize_ > 0 && outputBufferSize_ > 0, AVCS_ERR_INVALID_VAL,
                             "Allocate buffer with input size=%{public}d, output size=%{public}d failed",
                             inputBufferSize_, outputBufferSize_);

    int32_t inputBufferCnt = DEFAULT_IN_BUFFER_CNT;
    int32_t outputBufferCnt = DEFAULT_OUT_BUFFER_CNT;
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, inputBufferCnt);
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, outputBufferCnt);
    inputAvailQue_ = std::make_shared<BlockQueue<uint32_t>>("inputAvailQue", inputBufferCnt);
    codecAvailQue_ = std::make_shared<BlockQueue<uint32_t>>("codecAvailQue", outputBufferCnt);

    if (AllocateInputBuffer(inputBufferCnt, inputBufferSize_) == AVCS_ERR_NO_MEMORY ||
        AllocateOutputBuffer(outputBufferCnt, outputBufferSize_) == AVCS_ERR_NO_MEMORY) {
        return AVCS_ERR_NO_MEMORY;
    }
    AVCODEC_LOGI("Allocate buffers successful");
    return AVCS_ERR_OK;
}

void AvcEncoder::InitAvcEncoderParams()
{
    initParams_.logFxn = nullptr;
    initParams_.uiChannelID = 0;
    initParams_.width = 0;
    initParams_.height = 0;
    initParams_.frameRate = 0;
    initParams_.bitrate = 0;
    initParams_.qp = 0;
    initParams_.qpMax = 0;
    initParams_.qpMin = 0;
    initParams_.iperiod = 0;
    initParams_.range = 0;
    initParams_.primaries = 0;
    initParams_.transfer = 0;
    initParams_.matrix = 0;
    initParams_.level = 0;
    initParams_.encMode = ENC_MODE::MODE_CBR;
    initParams_.profile = ENC_PROFILE::PROFILE_BASE;
    initParams_.colorFmt = COLOR_FORMAT::YUV_420P;

    for (int i = 0; i < IV_MAX_RAW_COMPONENTS; i++) {
        avcEncInputArgs_.inputBufs[i] = nullptr;
        avcEncInputArgs_.width[i] = 0;
        avcEncInputArgs_.height[i] = 0;
        avcEncInputArgs_.stride[i] = 0;
    }
    avcEncInputArgs_.timestamp = 0;
    avcEncOutputArgs_.streamBuf = nullptr;
    avcEncOutputArgs_.encodedFrameType = 0;
    avcEncOutputArgs_.isLast = 0;
    avcEncOutputArgs_.size = 0;
    avcEncOutputArgs_.bytes = 0;
    avcEncOutputArgs_.timestamp = 0;
}

void AvcEncoder::FillAvcInitParams(AVC_ENC_INIT_PARAM &param)
{
    param.logFxn = AvcEncLog;
    param.uiChannelID = encInstanceID_;
    param.width = static_cast<uint32_t>(encWidth_);
    param.height = static_cast<uint32_t>(encHeight_);
    param.encMode = TranslateEncMode(bitrateMode_);
    param.frameRate = static_cast<uint32_t>(encFrameRate_);
    param.bitrate = static_cast<uint32_t>(encBitrate_);
    param.qp = static_cast<uint32_t>(encQp_);
    param.qpMax = static_cast<uint32_t>(encQpMax_);
    param.qpMin = static_cast<uint32_t>(encQpMin_);
    param.iperiod = static_cast<uint32_t>(encIperiod_);
    param.colorFmt = COLOR_FORMAT::YUV_420P;
    param.range = srcRange_;
    param.primaries = static_cast<uint8_t>(srcPrimary_);
    param.transfer = static_cast<uint8_t>(srcTransfer_);
    param.matrix = static_cast<uint8_t>(srcMatrix_);
    param.level = static_cast<uint32_t>(TranslateEncLevel(avcLevel_));
    param.profile = TranslateEncProfile(avcProfile_);
}

int32_t AvcEncoder::Start()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Start codec failed: callback is null");
    CHECK_AND_RETURN_RET_LOG((state_ == State::CONFIGURED || state_ == State::FLUSHED), AVCS_ERR_INVALID_STATE,
                             "Start codec failed: not in Configured or Flushed state");

    if (!isBufferAllocated_) {
        int32_t ret = AllocateBuffers();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Start codec failed: cannot allocate buffers");
        isBufferAllocated_ = true;
    }

    uint32_t createRet = 0;
    FillAvcInitParams(initParams_);
    std::unique_lock<std::mutex> runLock(encRunMutex_);
    if (avcEncoder_ == nullptr && avcEncoderCreateFunc_ != nullptr) {
        createRet = avcEncoderCreateFunc_(&avcEncoder_, &initParams_);
    }
    CHECK_AND_RETURN_RET_LOG(createRet == 0 && avcEncoder_ != nullptr, AVCS_ERR_INVALID_OPERATION,
                             "avc encoder create failed");
    runLock.unlock();

    InitBuffers();
    isSendEos_ = false;
    sendTask_->Start();
    if (inputSurface_ != nullptr) {
        ClearDirtyList();
        sptr<IBufferConsumerListener> listener = new EncoderBuffersConsumerListener(this);
        inputSurface_->RegisterConsumerListener(listener);
    }

    state_ = State::RUNNING;
    AVCODEC_LOGI("Start codec successful, state: Running");
    return AVCS_ERR_OK;
}

bool AvcEncoder::IsActive() const
{
    return state_ == State::RUNNING || state_ == State::FLUSHED || state_ == State::EOS;
}

void AvcEncoder::ResetBuffers()
{
    inputAvailQue_->Clear();
    codecAvailQue_->Clear();
    freeList_.clear();
}

int32_t AvcEncoder::Stop()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG((IsActive()), AVCS_ERR_INVALID_STATE, "Stop codec failed: not in executing state");
    state_ = State::STOPPING;
    AVCODEC_LOGI("step into STOPPING status");

    if (inputSurface_ != nullptr) {
        surfaceRecvCv_.notify_all();
        ClearDirtyList();
        inputSurface_->UnregisterConsumerListener();
    }

    std::unique_lock<std::mutex> sLock(sendMutex_);
    sendCv_.notify_one();
    sLock.unlock();
    inputAvailQue_->SetActive(false, false);
    codecAvailQue_->SetActive(false, false);
    sendTask_->Stop();
    ResetBuffers();
    std::unique_lock<std::mutex> runLock(encRunMutex_);
    if (avcEncoder_ != nullptr && avcEncoderDeleteFunc_ != nullptr) {
        uint32_t ret = avcEncoderDeleteFunc_(avcEncoder_);
        if (ret != 0) {
            AVCODEC_LOGE("Error: avc encoder delete error: %{public}d", ret);
            callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
            state_ = State::ERROR;
        }
        avcEncoder_ = nullptr;
    }
    runLock.unlock();
    state_ = State::CONFIGURED;
    AVCODEC_LOGI("Stop codec successful, state: Configured");
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::Flush()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG((state_ == State::RUNNING || state_ == State::EOS), AVCS_ERR_INVALID_STATE,
                             "Flush codec failed: not in running or Eos state");
    state_ = State::FLUSHING;
    AVCODEC_LOGI("step into FLUSHING status");
    std::unique_lock<std::mutex> sLock(sendMutex_);
    sendCv_.notify_one();
    sLock.unlock();

    inputAvailQue_->SetActive(false, false);
    codecAvailQue_->SetActive(false, false);
    sendTask_->Pause();

    ResetBuffers();
    state_ = State::FLUSHED;
    AVCODEC_LOGI("Flush codec successful, state: Flushed");
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::Reset()
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

void AvcEncoder::StopThread()
{
    if (sendTask_ != nullptr && inputAvailQue_ != nullptr) {
        std::unique_lock<std::mutex> sLock(sendMutex_);
        sendCv_.notify_one();
        sLock.unlock();
        inputAvailQue_->SetActive(false, false);
        codecAvailQue_->SetActive(false, false);
        sendTask_->Stop();
    }
}

void AvcEncoder::ReleaseBuffers()
{
    if (!isBufferAllocated_) {
        return;
    }

    inputAvailQue_->Clear();
    std::unique_lock<std::mutex> iLock(inputMutex_);
    buffers_[INDEX_INPUT].clear();
    iLock.unlock();

    std::unique_lock<std::mutex> oLock(outputMutex_);
    codecAvailQue_->Clear();
    buffers_[INDEX_OUTPUT].clear();
    oLock.unlock();

    isBufferAllocated_ = false;
}

void AvcEncoder::ReleaseResource()
{
    StopThread();
    ReleaseBuffers();
    format_ = Format();

    if (inputSurface_ != nullptr) {
        surfaceRecvCv_.notify_all();
        ClearDirtyList();
        inputSurface_->UnregisterConsumerListener();
    }

    std::unique_lock<std::mutex> runLock(encRunMutex_);
    if (avcEncoder_ != nullptr && avcEncoderDeleteFunc_ != nullptr) {
        uint32_t ret = avcEncoderDeleteFunc_(avcEncoder_);
        if (ret != 0) {
            AVCODEC_LOGE("Error: avc encoder delete error: %{public}d", ret);
            callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
            state_ = State::ERROR;
        }
        avcEncoder_ = nullptr;
    }
    runLock.unlock();
}

int32_t AvcEncoder::Release()
{
    AVCODEC_SYNC_TRACE;
    state_ = State::STOPPING;
    AVCODEC_LOGI("step into STOPPING status");
    ReleaseResource();
    state_ = State::UNINITIALIZED;
    AVCODEC_LOGI("Release codec successful, state: Uninitialized");
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::SetParameter(const Format &format)
{
    return ConfigureContext(format);
}

int32_t AvcEncoder::GetInputFormat(Format &format)
{
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, AVCodecCodecName::VIDEO_ENCODER_AVC_NAME);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, encWidth_);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, encHeight_);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
        static_cast<int>(bitrateMode_));
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(srcPixelFmt_));
    GraphicPixelFormat srcGraphicPixelFmt = TranslateSurfacePixFormat(srcPixelFmt_);
    format.PutIntValue(OHOS::Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT,
        static_cast<int32_t>(srcGraphicPixelFmt));
    if (srcPixelFmt_ == VideoPixelFormat::RGBA) {
        format.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, encWidth_ * RGBA_COLINC);
    } else {
        format.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, encWidth_);
    }
    AVCODEC_LOGI("GetInputFormat !");
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::GetOutputFormat(Format &format)
{
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, AVCodecCodecName::VIDEO_ENCODER_AVC_NAME);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, encWidth_);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, encHeight_);
    format.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, encWidth_);
    format.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, encHeight_);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, avcQuality_);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
        static_cast<int>(bitrateMode_));
    AVCODEC_LOGI("GetOutputFormat !");
    return AVCS_ERR_OK;
}

bool AvcEncoder::GetDiscardFlagFromAVBuffer(const std::shared_ptr<AVBuffer> &buffer)
{
    bool ret = false;
    if (buffer->meta_->GetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, ret) && ret) {
        AVCODEC_LOGD("discard by user, pts = %{public}" PRId64, buffer->pts_);
        return ret;
    }
    return ret;
}

int64_t AvcEncoder::GetBufferPts(const std::shared_ptr<AVBuffer> &buffer)
{
    int64_t pts = 0;
    AVCODEC_LOGD("avBuffer pts before setted, absolute pts=%{public}" PRId64 "", buffer->pts_);
    bool bret = buffer->meta_->GetData(
        OHOS::Media::Tag::VIDEO_ENCODE_SET_FRAME_PTS, pts);
    AVCODEC_LOGD("avBuffer pts after setted, relative pts=%{public}" PRId64 ", bret=%{public}d", pts, bret);
    if (!bret) {
        pts = buffer->pts_;
    }
    return pts;
}

int32_t AvcEncoder::QueueInputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    if (inputSurface_ != nullptr && !enableSurfaceModeInputCb_) {
        return AVCS_ERR_INVALID_STATE;
    }

    CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, AVCS_ERR_INVALID_STATE,
                             "Queue input buffer failed: not in Running state");
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_INPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Queue input buffer failed with bad index, index=%{public}u, buffer_size=%{public}zu",
                             index, buffers_[INDEX_INPUT].size());
    std::shared_ptr<FBuffer> inputBuffer = buffers_[INDEX_INPUT][index];
    CHECK_AND_RETURN_RET_LOG(inputBuffer->owner_ == FBuffer::Owner::OWNED_BY_USER, AVCS_ERR_INVALID_OPERATION,
                             "Queue input buffer failed: buffer with index=%{public}u is not available", index);

    if (inputSurface_ != nullptr) {
        CHECK_AND_RETURN_RET_LOG(inputBuffer->avBuffer_ != nullptr,
            AVCS_ERR_INVALID_VAL, "Allocate input buffer failed, index=%{public}u", index);
        int64_t pts = GetBufferPts(inputBuffer->avBuffer_);
        bool discard = GetDiscardFlagFromAVBuffer(inputBuffer->avBuffer_);
        inputBuffer->avBuffer_->pts_ = pts;
        if (discard) {
            inputBuffer->owner_ = FBuffer::Owner::OWNED_BY_USER;
            inputSurface_->ReleaseBuffer(inputBuffer->surfaceBuffer_, -1);
            NotifyUserToFillBuffer(index, inputBuffer->avBuffer_);
            return AVCS_ERR_OK;
        }
    }

    inputBuffer->owner_ = FBuffer::Owner::OWNED_BY_CODEC;
    inputAvailQue_->Push(index);
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::ReleaseOutputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    std::unique_lock<std::mutex> oLock(outputMutex_);
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_OUTPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Failed to release output buffer: invalid index");
    std::shared_ptr<FBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
    oLock.unlock();
    if (frameBuffer->owner_ == FBuffer::Owner::OWNED_BY_USER) {
        frameBuffer->owner_ = FBuffer::Owner::OWNED_BY_CODEC;
        codecAvailQue_->Push(index);
        return AVCS_ERR_OK;
    } else {
        AVCODEC_LOGE("Release output buffer failed: check your index=%{public}u", index);
        return AVCS_ERR_INVALID_VAL;
    }
}

int32_t AvcEncoder::NotifyEos()
{
    AVCODEC_LOGI(" start ");
    WaitForInBuffer();
    if (freeList_.empty()) {
        AVCODEC_LOGE("no empty buffer");
        return AVCS_ERR_OK;
    }

    std::unique_lock<std::mutex> listLock(freeListMutex_);
    uint32_t index = freeList_.front();
    freeList_.pop_front();
    listLock.unlock();

    std::shared_ptr<FBuffer> &inputBuffer = buffers_[INDEX_INPUT][index];
    if (inputSurface_ != nullptr) {
        inputBuffer->avBuffer_ = AVBuffer::CreateAVBuffer();
        CHECK_AND_RETURN_RET_LOG(inputBuffer->avBuffer_ != nullptr, AVCS_ERR_INVALID_VAL,
            "Allocate input buffer failed, index=%{public}u", index);
    }
    inputBuffer->avBuffer_->flag_ = AVCODEC_BUFFER_FLAG_EOS;
    inputBuffer->owner_ = FBuffer::Owner::OWNED_BY_CODEC;
    inputAvailQue_->Push(index);

    AVCODEC_LOGI("push eos buffer %{public}d", index);
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_INVALID_VAL, "Set callback failed: callback is NULL");
    callback_ = callback;
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::SetOutputSurface(sptr<Surface> surface)
{
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::RenderOutputBuffer(uint32_t index)
{
    return AVCS_ERR_OK;
}

void AvcEncoder::ReleaseSurfaceBufferByAVBuffer(std::shared_ptr<AVBuffer> &buffer)
{
    CHECK_AND_RETURN_LOG(buffer != nullptr, "input buffer nullptr");
    CHECK_AND_RETURN_LOG(buffer->memory_ != nullptr, "input buffer memory nullptr");
    auto surfaceBuffer = buffer->memory_->GetSurfaceBuffer();
    CHECK_AND_RETURN_LOG(surfaceBuffer != nullptr, "surface buffer nullptr");
    inputSurface_->ReleaseBuffer(surfaceBuffer, -1);
}

void AvcEncoder::NotifyUserToProcessBuffer(uint32_t index, std::shared_ptr<AVBuffer> &buffer)
{
    callback_->OnOutputBufferAvailable(index, buffer);
}

void AvcEncoder::NotifyUserToFillBuffer(uint32_t index, std::shared_ptr<AVBuffer> &buffer)
{
    if (inputSurface_ == nullptr) {
        callback_->OnInputBufferAvailable(index, buffer);
    } else {
        ReleaseSurfaceBufferByAVBuffer(buffer);
        std::unique_lock<std::mutex> listLock(freeListMutex_);
        freeList_.emplace_back(index);
        listLock.unlock();
        surfaceRecvCv_.notify_one();
    }
}

int32_t AvcEncoder::CheckBufferSize(int32_t bufferSize, int32_t stride, int32_t height, VideoPixelFormat pixelFormat)
{
    int32_t expect = 0;
    switch (pixelFormat) {
        case VideoPixelFormat::RGBA: {
            expect = stride * height;                //  rgba origin stride already * 4
            break;
        }
        case VideoPixelFormat::YUVI420:
        case VideoPixelFormat::NV12:
        case VideoPixelFormat::NV21: {
            expect = stride * height * 3 / 2;        //  * 3 / 2: yuv size
            break;
        }
        default:
            AVCODEC_LOGE("Invalid buffer size %{public}d %{public}dX%{public}d for format:%{public}d",
                bufferSize, stride, height, static_cast<int32_t>(pixelFormat));
            return AVCS_ERR_UNSUPPORT_SOURCE;
    }
    if (bufferSize >= expect && expect > 0) {
        return AVCS_ERR_OK;
    }
    return AVCS_ERR_UNSUPPORT_SOURCE;
}

void AvcEncoder::FillYuv420ToAvcEncInArgs(AVC_ENC_INARGS &inArgs, NVFrame &nvFrame, int64_t pts)
{
    inArgs.inputBufs[0] = nvFrame.srcY; // 0 : y buffer
    inArgs.inputBufs[1] = nvFrame.srcU; // 1 : u buffer
    inArgs.inputBufs[2] = nvFrame.srcV; // 2 : v buffer
    inArgs.width[0] = static_cast<uint32_t>(nvFrame.width);        // 0 : y width
    inArgs.width[1] = static_cast<uint32_t>(nvFrame.width) >> 1;   // 1 : u width
    inArgs.width[2] = static_cast<uint32_t>(nvFrame.width) >> 1;   // 2 : v width
    inArgs.height[0] = static_cast<uint32_t>(nvFrame.height);      // 0 : y height
    inArgs.height[1] = static_cast<uint32_t>(nvFrame.height) >> 1; // 1 : u height
    inArgs.height[2] = static_cast<uint32_t>(nvFrame.height) >> 1; // 2 : v height
    inArgs.stride[0] = static_cast<uint32_t>(nvFrame.yStride);     // 0 : y stride
    inArgs.stride[1] = static_cast<uint32_t>(nvFrame.uvStride);    // 1 : u stride
    inArgs.stride[2] = static_cast<uint32_t>(nvFrame.uvStride);    // 2 : v stride

    inArgs.colorFmt = COLOR_FORMAT::YUV_420P;
    inArgs.timestamp = static_cast<uint64_t>(pts);
    inArgs.configArgs.bitrate = static_cast<uint32_t>(encBitrate_);
    inArgs.configArgs.idrRequest = encIdrRequest_ ? 1 : 0;
    encIdrRequest_ = false;
}

int32_t AvcEncoder::FillAvcEncoderInDefaultArgs(AVC_ENC_INARGS &inArgs)
{
    inArgs.colorFmt = COLOR_FORMAT::YUV_420P;
    for (int i = 0; i < IV_MAX_RAW_COMPONENTS; i++) {
        inArgs.inputBufs[i] = nullptr;
        inArgs.width[i] = 0;
        inArgs.height[i] = 0;
        inArgs.stride[i] = 0;
    }
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::Nv21ToAvcEncoderInArgs(InputFrame &inFrame, AVC_ENC_INARGS &inArgs)
{
    int32_t width = inFrame.width;
    int32_t height = inFrame.height;
    uint8_t *dstData = convertBuffer_->memory_->GetAddr();
    int32_t dstSize = convertBuffer_->memory_->GetCapacity();

    YuvImageData yuvData = {
        .data     = inFrame.buffer,
        .stride   = inFrame.stride,
        .uvOffset = inFrame.uvOffset,
    };

    int32_t ret = ConvertNv21ToYuv420(dstData, width, height, dstSize, yuvData);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Scale video frame failed: %{public}d", ret);

    NVFrame nvFrame = {
        .srcY    = dstData,
        .srcU    = dstData + width * height,
        .srcV    = dstData + width * height +
            (static_cast<uint32_t>(width) >> 1) * (static_cast<uint32_t>(height) >> 1),
        .yStride = width,
        .uvStride = static_cast<uint32_t>(width) >> 1,
        .width   = width,
        .height  = height,
    };

    FillYuv420ToAvcEncInArgs(inArgs, nvFrame, inFrame.pts);
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::Nv12ToAvcEncoderInArgs(InputFrame &inFrame, AVC_ENC_INARGS &inArgs)
{
    int32_t width = inFrame.width;
    int32_t height = inFrame.height;
    uint8_t *dstData = convertBuffer_->memory_->GetAddr();
    int32_t dstSize = convertBuffer_->memory_->GetCapacity();

    YuvImageData yuvData = {
        .data     = inFrame.buffer,
        .stride   = inFrame.stride,
        .uvOffset = inFrame.uvOffset,
    };

    int32_t ret = ConvertNv12ToYuv420(dstData, width, height, dstSize, yuvData);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Scale video frame failed: %{public}d", ret);

    NVFrame nvFrame = {
        .srcY    = dstData,
        .srcU    = dstData + width * height,
        .srcV    = dstData + width * height +
            (static_cast<uint32_t>(width) >> 1) * (static_cast<uint32_t>(height) >> 1),
        .yStride = width,
        .uvStride = static_cast<uint32_t>(width) >> 1,
        .width   = width,
        .height  = height,
    };

    FillYuv420ToAvcEncInArgs(inArgs, nvFrame, inFrame.pts);
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::Yuv420ToAvcEncoderInArgs(InputFrame &inFrame, AVC_ENC_INARGS &inArgs)
{
    NVFrame nvFrame = {
        .srcY    = inFrame.buffer,
        .srcU    = inFrame.buffer + inFrame.uvOffset,
        .srcV    = inFrame.buffer + inFrame.uvOffset +
            (static_cast<uint32_t>(inFrame.height) >> 1) * (static_cast<uint32_t>(inFrame.stride) >> 1),
        .yStride = inFrame.stride,
        .uvStride = static_cast<uint32_t>(inFrame.stride) >> 1,
        .width   = inFrame.width,
        .height  = inFrame.height,
    };

    FillYuv420ToAvcEncInArgs(inArgs, nvFrame, inFrame.pts);
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::RgbaToAvcEncoderInArgs(InputFrame &inFrame, AVC_ENC_INARGS &inArgs)
{
    int32_t width = inFrame.width;
    int32_t height = inFrame.height;
    int32_t stride = inFrame.stride;

    if (inputSurface_ != nullptr) {
        stride = inFrame.stride / RGBA_COLINC;   // surface buffer rgba stride has * 4
    }

    uint8_t *dstData = convertBuffer_->memory_->GetAddr();
    int32_t dstSize = convertBuffer_->memory_->GetCapacity();

    RgbImageData rgbData = {
        .data   = inFrame.buffer,
        .stride = stride,
        .matrix = TranslateMatrix(srcMatrix_),
        .range  = TranslateRange(srcRange_),
        .bytesPerPixel = RGBA_COLINC,
    };

    int32_t ret = AVCS_ERR_OK;
#if defined(ARMV8)
    ret = ConvertRgbToYuv420Neon(dstData, width, height, dstSize, rgbData);
#else
    ret = ConvertRgbToYuv420(dstData, width, height, dstSize, rgbData);
#endif
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Scale video frame failed: %{public}d", ret);

    NVFrame nvFrame = {
        .srcY    = dstData,
        .srcU    = dstData + width * height,
        .srcV    = dstData + width * height +
            (static_cast<uint32_t>(width) >> 1) * (static_cast<uint32_t>(height) >> 1),
        .yStride = width,
        .uvStride = static_cast<uint32_t>(width) >> 1,
        .width   = width,
        .height  = height,
    };
    FillYuv420ToAvcEncInArgs(inArgs, nvFrame, inFrame.pts);
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::GetSurfaceBufferUvOffset(sptr<SurfaceBuffer> &surfaceBuffer, VideoPixelFormat format)
{
    int32_t uvOffset = 0;
    if (!((format == VideoPixelFormat::YUVI420) ||
        (format == VideoPixelFormat::NV12) ||
        (format == VideoPixelFormat::NV21))) {
        return uvOffset;
    }

    OH_NativeBuffer_Planes *planes = nullptr;
    GSError retVal = surfaceBuffer->GetPlanesInfo(reinterpret_cast<void**>(&planes));
    if (retVal != GSERROR_OK || planes == nullptr) {
        AVCODEC_LOGE(" GetPlanesInfo failed, retVal:%{public}d", retVal);
        return uvOffset;
    }
    CHECK_AND_RETURN_RET_LOG(planes->planeCount > 1, uvOffset, "Get surface uvinfo failed");
    if (format == VideoPixelFormat::NV21) {
        uvOffset = static_cast<int32_t>(planes->planes[2].offset);     // 2: nv21 v offset
    } else {
        uvOffset = static_cast<int32_t>(planes->planes[1].offset);     // 1: nv21 yuv420 u offset
    }
    return uvOffset;
}

int32_t AvcEncoder::GetInputFrameFromAVBuffer(std::shared_ptr<AVBuffer> &buffer, InputFrame &inFrame)
{
    inFrame.format = srcPixelFmt_;
    inFrame.width = encWidth_;
    inFrame.height = encHeight_;
    inFrame.stride = encWidth_;
    inFrame.size = buffer->memory_->GetSize();
    inFrame.buffer = buffer->memory_->GetAddr();
    if (inputSurface_ != nullptr) {
        auto surfaceBuffer = buffer->memory_->GetSurfaceBuffer();
        CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_INVALID_DATA, "Get surface data failed!");
        inFrame.format =
            TranslateVideoPixelFormat(static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat()));
        inFrame.width = surfaceBuffer->GetWidth();
        CHECK_AND_RETURN_RET_LOG((inFrame.width >= VIDEO_MIN_SIZE) && (inFrame.width <= VIDEO_MAX_WIDTH_SIZE),
            AVCS_ERR_INVALID_DATA, "Get surface width failed!");
        inFrame.height = surfaceBuffer->GetHeight();
        CHECK_AND_RETURN_RET_LOG((inFrame.height >= VIDEO_MIN_SIZE) && (inFrame.height <= VIDEO_MAX_HEIGHT_SIZE),
            AVCS_ERR_INVALID_DATA, "Get surface height failed!");
        inFrame.stride = surfaceBuffer->GetStride();
        CHECK_AND_RETURN_RET_LOG(inFrame.stride >= inFrame.width, AVCS_ERR_INVALID_DATA,
            "Get surface stride failed!");
        inFrame.size = static_cast<int32_t>(surfaceBuffer->GetSize());
        inFrame.buffer = reinterpret_cast<uint8_t *>(surfaceBuffer->GetVirAddr());
        CHECK_AND_RETURN_RET_LOG(inFrame.buffer != nullptr, AVCS_ERR_INVALID_DATA,
            "Get surface buffer failed!");
        inFrame.uvOffset = GetSurfaceBufferUvOffset(surfaceBuffer, inFrame.format);
    }
    if (inFrame.uvOffset == 0) {
        inFrame.uvOffset = inFrame.stride * inFrame.height;
    }
    inFrame.pts = buffer->pts_;
    return AVCS_ERR_OK;
}

int32_t AvcEncoder::FillAvcEncoderInArgs(std::shared_ptr<AVBuffer> &buffer, AVC_ENC_INARGS &inArgs)
{
    SCOPED_TRACE_AVC("FillAvcEncoderInArgs");
    InputFrame inFrame;
    int32_t ret = GetInputFrameFromAVBuffer(buffer, inFrame);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_DATA, "Get input frame failed!");

    ret = CheckBufferSize(inFrame.size, inFrame.stride, inFrame.height, inFrame.format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Check buffer size failed!");

    switch (inFrame.format) {
        case VideoPixelFormat::YUVI420: {
            return Yuv420ToAvcEncoderInArgs(inFrame, inArgs);
        }
        case VideoPixelFormat::RGBA: {
            return RgbaToAvcEncoderInArgs(inFrame, inArgs);
        }
        case VideoPixelFormat::NV12: {
            return Nv12ToAvcEncoderInArgs(inFrame, inArgs);
        }
        case VideoPixelFormat::NV21: {
            return Nv21ToAvcEncoderInArgs(inFrame, inArgs);
        }
        default:
            AVCODEC_LOGE("Invalid video pixel format:%{public}d",
                static_cast<int32_t>(inFrame.format));
    }
    return AVCS_ERR_UNSUPPORT;
}

int32_t AvcEncoder::EncoderAvcFrame(AVC_ENC_INARGS &inArgs, AVC_ENC_OUTARGS &outArgs)
{
    SCOPED_TRACE_AVC("EncoderAvcFrame");
    uint32_t ret = 0;
    std::unique_lock<std::mutex> sLock(encRunMutex_);
    if (avcEncoder_ == nullptr || avcEncoderFrameFunc_ == nullptr) {
        AVCODEC_LOGE("Avc encoder load error !");
        return AVCS_ERR_VID_ENC_FAILED;
    }

    uint32_t codecIndex = codecAvailQue_->Front();
    std::shared_ptr<FBuffer> outputBuffer = buffers_[INDEX_OUTPUT][codecIndex];
    std::shared_ptr<AVBuffer> &outputAVBuffer = outputBuffer->avBuffer_;

    outArgs.streamBuf = outputAVBuffer->memory_->GetAddr();
    outArgs.size = static_cast<uint32_t>(outputAVBuffer->memory_->GetCapacity());

    ret = avcEncoderFrameFunc_(avcEncoder_, &inArgs, &outArgs);
    sLock.unlock();

    if (isSendEos_) {
        outputAVBuffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
    } else if (ret == 0) {
        outputAVBuffer->memory_->SetSize(outArgs.bytes);
        outputAVBuffer->pts_ = static_cast<int64_t>(outArgs.timestamp);
        outputAVBuffer->flag_ = AvcFrameTypeToBufferFlag(outArgs.encodedFrameType);
    } else {
        outputAVBuffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        AVCODEC_LOGE("Cannot send frame to encodec: %{public}d", ret);
    }

    outputBuffer->owner_ = FBuffer::Owner::OWNED_BY_USER;
    codecAvailQue_->Pop();
    NotifyUserToProcessBuffer(codecIndex, outputAVBuffer);
    return AVCS_ERR_OK;
}

void AvcEncoder::EncoderAvcHeader()
{
    AVC_ENC_INARGS headerInArgs;
    FillAvcEncoderInDefaultArgs(headerInArgs);
    int32_t ret = EncoderAvcFrame(headerInArgs, avcEncOutputArgs_);
    CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "Avc Encoder header error: %{public}d", ret);
}

void AvcEncoder::EncoderAvcTailer()
{
    AVC_ENC_INARGS eosInArgs;
    FillAvcEncoderInDefaultArgs(eosInArgs);
    int32_t ret = EncoderAvcFrame(eosInArgs, avcEncOutputArgs_);
    CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "Avc Encoder tailer error: %{public}d", ret);
}

std::shared_ptr<AVBuffer> AvcEncoder::GetAvBuffer(const std::shared_ptr<FBuffer> &inputBuffer)
{
    CHECK_AND_RETURN_RET_LOG(inputBuffer != nullptr, nullptr, "input buffer nullptr");
    std::shared_ptr<AVBuffer> inputAVBuffer = nullptr;
    if (inputBuffer->surfaceBuffer_ == nullptr) {
        inputAVBuffer = inputBuffer->avBuffer_;
        return inputAVBuffer;
    }
    inputAVBuffer = AVBuffer::CreateAVBuffer(inputBuffer->surfaceBuffer_);
    CHECK_AND_RETURN_RET_LOG(inputAVBuffer != nullptr, nullptr, "create buffer fail");
    inputAVBuffer->pts_ = inputBuffer->avBuffer_->pts_;
    inputAVBuffer->flag_ = inputBuffer->avBuffer_->flag_;
    return inputAVBuffer;
}

void AvcEncoder::SendFrame()
{
    SCOPED_TRACE_AVC("SendFrame");
    CHECK_AND_RETURN_LOG(state_ != State::STOPPING && state_ != State::FLUSHING, "Invalid state");
    if (state_ != State::RUNNING || isSendEos_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_ENCODE_TIME));
        return;
    }

    int32_t ret = AVCS_ERR_OK;
    uint32_t index = inputAvailQue_->Front();
    CHECK_AND_RETURN_LOG(state_ == State::RUNNING, "Not in running state");
    std::shared_ptr<FBuffer> &inputBuffer = buffers_[INDEX_INPUT][index];
    std::shared_ptr<AVBuffer> inputAVBuffer = GetAvBuffer(inputBuffer);
    CHECK_AND_RETURN_LOG(inputAVBuffer != nullptr, "input buffer nullptr");
    if (inputAVBuffer->flag_ & AVCODEC_BUFFER_FLAG_EOS) {
        std::unique_lock<std::mutex> sendLock(sendMutex_);
        isSendEos_ = true;
        EncoderAvcTailer();
        state_ = State::EOS;
        sendCv_.wait_for(sendLock, std::chrono::milliseconds(DEFAULT_ENCODE_WAIT_TIME));
        AVCODEC_LOGI("Send eos end");
        return;
    }

    sptr<SyncFence> fence = inputBuffer->fence_;
    if (fence != nullptr) {
        constexpr uint32_t waitForEver = -1;
        fence->Wait(waitForEver);
    }
    CHECK_AND_RETURN_LOG(inputAVBuffer->memory_ != nullptr, "input buffer memory nullptr");
    ret = FillAvcEncoderInArgs(inputAVBuffer, avcEncInputArgs_);
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("convert buffer to encodec error: %{public}d", ret);
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
        state_ = State::ERROR;
        return;
    }

    if (isFirstFrame_) {
        EncoderAvcHeader();
        isFirstFrame_ = false;
    }

    ret = EncoderAvcFrame(avcEncInputArgs_, avcEncOutputArgs_);
    if (ret == AVCS_ERR_OK) {
        inputBuffer->owner_ = FBuffer::Owner::OWNED_BY_USER;
        inputAvailQue_->Pop();
        NotifyUserToFillBuffer(index, inputAVBuffer);
    } else {
        AVCODEC_LOGE("Cannot send frame to encodec: %{public}d", ret);
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
        state_ = State::ERROR;
    }
}

int32_t AvcEncoder::CheckAvcEncLibStatus()
{
    void* handle = dlopen(AVC_ENC_LIB_PATH, RTLD_LAZY);
    if (handle != nullptr) {
        auto avcEncoderCreateFunc = reinterpret_cast<CreateAvcEncoderFuncType>(
            dlsym(handle, AVC_ENC_CREATE_FUNC_NAME));
        auto avcEncoderFrameFunc = reinterpret_cast<EncodeFuncType>(
            dlsym(handle, AVC_ENC_ENCODE_FRAME_FUNC_NAME));
        auto avcEncoderDeleteFunc = reinterpret_cast<DeleteFuncType>(
            dlsym(handle, AVC_ENC_DELETE_FUNC_NAME));
        if (avcEncoderCreateFunc == nullptr || avcEncoderFrameFunc == nullptr ||
            avcEncoderDeleteFunc == nullptr) {
            AVCODEC_LOGE("AvcEncoder avcFuncMatch_ failed!");
            avcEncoderCreateFunc = nullptr;
            avcEncoderFrameFunc = nullptr;
            avcEncoderDeleteFunc = nullptr;
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

void AvcEncoder::GetBaseCapabilityData(CapabilityData &capsData)
{
    capsData.alignment.width = VIDEO_ALIGNMENT_SIZE;
    capsData.alignment.height = VIDEO_ALIGNMENT_SIZE;
    capsData.width.minVal = VIDEO_MIN_SIZE;
    capsData.width.maxVal = VIDEO_MAX_WIDTH_SIZE;
    capsData.height.minVal = VIDEO_MIN_SIZE;
    capsData.height.maxVal = VIDEO_MAX_HEIGHT_SIZE;
    capsData.frameRate.minVal = VIDEO_FRAMERATE_MIN_SIZE;
    capsData.frameRate.maxVal = VIDEO_FRAMERATE_MAX_SIZE;
    capsData.bitrate.minVal = VIDEO_BITRATE_MIN_SIZE;
    capsData.bitrate.maxVal = VIDEO_BITRATE_MAX_SIZE;
    capsData.blockPerFrame.minVal = 1;
    capsData.blockPerFrame.maxVal = VIDEO_BLOCKPERFRAME_SIZE;
    capsData.blockPerSecond.minVal = 1;
    capsData.blockPerSecond.maxVal = VIDEO_BLOCKPERSEC_SIZE;
    capsData.blockSize.width = VIDEO_ALIGN_SIZE;
    capsData.blockSize.height = VIDEO_ALIGN_SIZE;
}

void AvcEncoder::GetCapabilityData(CapabilityData &capsData, uint32_t index)
{
    capsData.codecName = static_cast<std::string>(SUPPORT_VCODEC[index].codecName);
    capsData.mimeType = static_cast<std::string>(SUPPORT_VCODEC[index].mimeType);
    capsData.codecType = SUPPORT_VCODEC[index].isEncoder ? AVCODEC_TYPE_VIDEO_ENCODER : AVCODEC_TYPE_VIDEO_DECODER;
    capsData.isVendor = false;
    capsData.maxInstance = VIDEO_INSTANCE_SIZE;

    GetBaseCapabilityData(capsData);

    if (SUPPORT_VCODEC[index].isEncoder) {
        capsData.complexity.minVal = 1;
        capsData.complexity.maxVal = 1;
        capsData.encodeQuality.minVal = VIDEO_QUALITY_MIN;
        capsData.encodeQuality.maxVal = VIDEO_QUALITY_MAX;
    }
    capsData.pixFormat = {
        static_cast<int32_t>(VideoPixelFormat::YUVI420), static_cast<int32_t>(VideoPixelFormat::NV12),
        static_cast<int32_t>(VideoPixelFormat::NV21), static_cast<int32_t>(VideoPixelFormat::RGBA)
    };
    capsData.graphicPixFormat = {
        static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P),
        static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP),
        static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP),
        static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888)
    };
    capsData.bitrateMode = {
        static_cast<int32_t>(VideoEncodeBitrateMode::CBR), static_cast<int32_t>(VideoEncodeBitrateMode::VBR),
        static_cast<int32_t>(VideoEncodeBitrateMode::CQ)
    };
    capsData.profiles = {static_cast<int32_t>(AVC_PROFILE_BASELINE), static_cast<int32_t>(AVC_PROFILE_MAIN)};
    std::vector<int32_t> levels;
    for (int32_t j = 0; j <= static_cast<int32_t>(AVCLevel::AVC_LEVEL_51); ++j) {
        levels.emplace_back(j);
    }
    capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AVC_PROFILE_MAIN), levels));
    capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AVC_PROFILE_BASELINE), levels));
    return;
}

int32_t AvcEncoder::GetCodecCapability(std::vector<CapabilityData> &capaArray)
{
    CHECK_AND_RETURN_RET_LOG(CheckAvcEncLibStatus() == AVCS_ERR_OK,
        AVCS_ERR_UNSUPPORT, "avc encoder libs not available");

    for (uint32_t i = 0; i < SUPPORT_VCODEC_NUM; ++i) {
        CapabilityData capsData;
        GetCapabilityData(capsData, i);
        capaArray.emplace_back(capsData);
    }
    return AVCS_ERR_OK;
}

std::mutex AvcEncoder::encoderCountMutex_;
std::vector<uint32_t> AvcEncoder::encInstanceIDSet_;
std::vector<uint32_t> AvcEncoder::freeIDSet_;

} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS