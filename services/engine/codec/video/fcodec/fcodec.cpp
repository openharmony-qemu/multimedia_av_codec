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

#include "fcodec.h"
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

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "FCodec"};
constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;
constexpr int32_t DEFAULT_IN_BUFFER_CNT = 4;
constexpr int32_t DEFAULT_OUT_SURFACE_CNT = 4;
constexpr int32_t DEFAULT_OUT_BUFFER_CNT = 3;
constexpr int32_t DEFAULT_MIN_BUFFER_CNT = 2;
constexpr uint32_t VIDEO_PIX_DEPTH_YUV = 3;
constexpr int32_t VIDEO_MIN_BUFFER_SIZE = 1474560;
constexpr int32_t VIDEO_MIN_SIZE = 2;
constexpr int32_t VIDEO_ALIGNMENT_SIZE = 2;
constexpr int32_t VIDEO_MAX_WIDTH_SIZE = 4096;
constexpr int32_t VIDEO_MAX_HEIGHT_SIZE = 4096;
constexpr int32_t VIDEO_MAX_WIDTH_H263_SIZE = 2048;
constexpr int32_t VIDEO_MAX_HEIGHT_H263_SIZE = 1152;
constexpr int32_t VIDEO_MIN_WIDTH_H263_SIZE = 20;
constexpr int32_t VIDEO_MIN_HEIGHT_H263_SIZE = 20;
constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
constexpr uint32_t DEFAULT_TRY_DECODE_TIME = 1;
constexpr uint32_t DEFAULT_DECODE_WAIT_TIME = 200;
constexpr int32_t VIDEO_INSTANCE_SIZE = 64;
constexpr int32_t VIDEO_BITRATE_MAX_SIZE = 300000000;
constexpr int32_t VIDEO_FRAMERATE_MAX_SIZE = 120;
constexpr int32_t VIDEO_FRAMERATE_DEFAULT_SIZE = 60;
constexpr int32_t VIDEO_BLOCKPERFRAME_SIZE = 139264;
constexpr int32_t VIDEO_BLOCKPERSEC_SIZE = 983040;
constexpr int32_t DEFAULT_THREAD_COUNT = 2;
#ifdef BUILD_ENG_VERSION
constexpr uint32_t PATH_MAX_LEN = 128;
constexpr char DUMP_PATH[] = "/data/misc/fcodecdump";
#endif // BUILD_ENG_VERSION
constexpr struct {
    const std::string_view codecName;
    const std::string_view mimeType;
    const char *ffmpegCodec;
} SUPPORT_VCODEC[] = {
    {AVCodecCodecName::VIDEO_DECODER_AVC_NAME, CodecMimeType::VIDEO_AVC, "h264"},
    {AVCodecCodecName::VIDEO_DECODER_H263_NAME, CodecMimeType::VIDEO_H263, "h263"},
    {AVCodecCodecName::VIDEO_DECODER_MPEG2_NAME, CodecMimeType::VIDEO_MPEG2, "mpeg2video"},
    {AVCodecCodecName::VIDEO_DECODER_MPEG4_NAME, CodecMimeType::VIDEO_MPEG4, "mpeg4"},
#ifdef SUPPORT_CODEC_RV
    {AVCodecCodecName::VIDEO_DECODER_RV30_NAME, CodecMimeType::VIDEO_RV30, "rv30"},
    {AVCodecCodecName::VIDEO_DECODER_RV40_NAME, CodecMimeType::VIDEO_RV40, "rv40"},
#endif
};
constexpr uint32_t SUPPORT_VCODEC_NUM = sizeof(SUPPORT_VCODEC) / sizeof(SUPPORT_VCODEC[0]);
} // namespace
using namespace OHOS::Media;

FCodec::FCodec(const std::string &name) : codecName_(name), state_(State::UNINITIALIZED) {}

FCodec::~FCodec()
{
    ReleaseResource();
    callback_ = nullptr;
#ifdef BUILD_ENG_VERSION
    if (dumpInFile_ != nullptr) {
        dumpInFile_->close();
    }
    if (dumpOutFile_ != nullptr) {
        dumpOutFile_->close();
    }
#endif // BUILD_ENG_VERSION
    mallopt(M_FLUSH_THREAD_CACHE, 0);
}

int32_t FCodec::Init(Meta &callerInfo)
{
    callerInfo.GetData("av_codec_event_info_instance_id", instanceId_);
    decName_ = "fdecoder_[" + std::to_string(instanceId_) + "]";
    AVCODEC_LOGI("current codec name: %{public}s", decName_.c_str());
    return AVCS_ERR_OK;
}

#ifdef BUILD_ENG_VERSION
void FCodec::OpenDumpFile()
{
    std::string dumpModeStr = OHOS::system::GetParameter("fcodec.dump", "0");
    AVCODEC_LOGI("dumpModeStr %{public}s", dumpModeStr.c_str());
    CHECK_AND_RETURN_LOG(dumpModeStr.length() == 2, "dumpModeStr length should equal 2"); // 2
    char fileName[PATH_MAX_LEN] = {0};
    int ret;
    if (dumpModeStr[0] == '1') {
        ret = sprintf_s(fileName, sizeof(fileName), "%s/input_%p.h264", DUMP_PATH, this);
        CHECK_AND_RETURN_LOG(ret > 0, "Fail to sprintf input fileName");
        dumpInFile_ = std::make_shared<std::ofstream>();
        dumpInFile_->open(fileName, std::ios::out | std::ios::binary);
        if (!dumpInFile_->is_open()) {
            AVCODEC_LOGW("fail open file %{public}s", fileName);
            dumpInFile_ = nullptr;
        }
    }

    if (dumpModeStr[1] == '1') {
        ret = sprintf_s(fileName, sizeof(fileName), "%s/output_%p.yuv", DUMP_PATH, this);
        CHECK_AND_RETURN_LOG(ret > 0, "Fail to sprintf output fileName");
        dumpOutFile_ = std::make_shared<std::ofstream>();
        dumpOutFile_->open(fileName, std::ios::out | std::ios::binary);
        if (!dumpOutFile_->is_open()) {
            AVCODEC_LOGW("fail open file %{public}s", fileName);
            dumpOutFile_ = nullptr;
        }
    }
}
#endif // BUILD_ENG_VERSION

int32_t FCodec::Initialize()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(!codecName_.empty(), AVCS_ERR_INVALID_VAL, "Init codec failed:  empty name");
    pid_ = getpid();
    std::string fcodecName;
    std::string_view mime;
    for (uint32_t i = 0; i < SUPPORT_VCODEC_NUM; ++i) {
        if (SUPPORT_VCODEC[i].codecName == codecName_) {
            fcodecName = SUPPORT_VCODEC[i].ffmpegCodec;
            mime = SUPPORT_VCODEC[i].mimeType;
            break;
        }
    }
    CHECK_AND_RETURN_RET_LOG(!fcodecName.empty(), AVCS_ERR_INVALID_VAL,
                             "Init codec failed: not support name: %{public}s", codecName_.c_str());
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, mime);
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, codecName_);
    avCodec_ = std::shared_ptr<AVCodec>(const_cast<AVCodec *>(avcodec_find_decoder_by_name(fcodecName.c_str())),
                                        [](void *ptr) {});
    CHECK_AND_RETURN_RET_LOG(avCodec_ != nullptr, AVCS_ERR_INVALID_VAL,
                             "Init codec failed:  cannot find codec with name %{public}s", codecName_.c_str());
    sendTask_ = std::make_shared<TaskThread>("SendFrame");
    sendTask_->RegisterHandler([this] { SendFrame(); });
    receiveTask_ = std::make_shared<TaskThread>("ReceiveFrame");
    receiveTask_->RegisterHandler([this] { ReceiveFrame(); });
#ifdef BUILD_ENG_VERSION
    OpenDumpFile();
#endif // BUILD_ENG_VERSION
    state_ = State::INITIALIZED;
    AVCODEC_LOGI("Init codec successful,  state: Uninitialized -> Initialized");
    return AVCS_ERR_OK;
}

void FCodec::ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal,
                                 int32_t maxVal)
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

void FCodec::ConfigureSurface(const Format &format, const std::string_view &formatKey, FormatDataType formatType)
{
    CHECK_AND_RETURN_LOG(formatType == FORMAT_TYPE_INT32, "Set parameter failed: type should be int32");

    int32_t val = 0;
    CHECK_AND_RETURN_LOG(format.GetIntValue(formatKey, val), "Set parameter failed: get value fail");

    if (formatKey == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT) {
        VideoPixelFormat vpf = static_cast<VideoPixelFormat>(val);
        CHECK_AND_RETURN_LOG(vpf == VideoPixelFormat::RGBA || vpf == VideoPixelFormat::YUVI420 ||
                             vpf == VideoPixelFormat::NV12 || vpf == VideoPixelFormat::NV21,
                             "Set parameter failed: pixel format value %{public}d invalid", val);
        outputPixelFmt_ = vpf;
        format_.PutIntValue(formatKey, val);
    } else if (formatKey == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE) {
        VideoRotation sr = static_cast<VideoRotation>(val);
        CHECK_AND_RETURN_LOG(sr == VideoRotation::VIDEO_ROTATION_0 || sr == VideoRotation::VIDEO_ROTATION_90 ||
                             sr == VideoRotation::VIDEO_ROTATION_180 || sr == VideoRotation::VIDEO_ROTATION_270,
                             "Set parameter failed: rotation angle value %{public}d invalid", val);
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val);
    } else {
        ScalingMode scaleMode = static_cast<ScalingMode>(val);
        CHECK_AND_RETURN_LOG(scaleMode == ScalingMode::SCALING_MODE_SCALE_TO_WINDOW ||
                             scaleMode == ScalingMode::SCALING_MODE_SCALE_CROP,
                             "Set parameter failed: scale type value %{public}d invalid", val);
        format_.PutIntValue(formatKey, val);
    }
    AVCODEC_LOGI("Set parameter %{public}s success, val %{public}d", std::string(formatKey).c_str(), val);
}

int32_t FCodec::ConfigureContext(const Format &format)
{
    avCodecContext_ = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(avCodec_.get()), [](AVCodecContext *p) {
        if (p != nullptr) {
            if (p->extradata) {
                av_free(p->extradata);
                p->extradata = nullptr;
            }
            avcodec_free_context(&p);
        }
    });
    CHECK_AND_RETURN_RET_LOG(avCodecContext_ != nullptr, AVCS_ERR_INVALID_OPERATION,
                             "Configure codec failed: Allocate context error");
    avCodecContext_->codec_type = AVMEDIA_TYPE_VIDEO;
    CHECK_AND_RETURN_RET_LOG(format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width_) &&
                             width_ >= VIDEO_MIN_SIZE && width_ <= VIDEO_MAX_WIDTH_SIZE,
                             AVCS_ERR_INVALID_VAL, "Invalid width %{public}d!", width_);
    CHECK_AND_RETURN_RET_LOG(format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height_) &&
                             height_ >= VIDEO_MIN_SIZE && height_ <= VIDEO_MAX_HEIGHT_SIZE,
                             AVCS_ERR_INVALID_VAL, "Invalid height %{public}d!", height_);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE,
                        outputPixelFmt_ == VideoPixelFormat::RGBA ? width_ * VIDEO_PIX_DEPTH_RGBA : width_);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_SLICE_HEIGHT, height_);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, width_);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, height_);

    avCodecContext_->width = width_;
    avCodecContext_->height = height_;
    avCodecContext_->thread_count = DEFAULT_THREAD_COUNT;
#if (defined SUPPORT_CODEC_RV) || (defined SUPPORT_CODEC_MP4V_ES)
    return SetCodecExtradata(format);
#else
    return AVCS_ERR_OK;
#endif
}

#if (defined SUPPORT_CODEC_RV) || (defined SUPPORT_CODEC_MP4V_ES)
int32_t FCodec::SetCodecExtradata(const Format &format)
{
    size_t extraSize = 0;
    uint8_t *extraData = nullptr;
    if (format.GetBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, &extraData, extraSize)) {
        if (extraData == nullptr || extraSize == 0) {
            AVCODEC_LOGE("extradata getBufer failed!");
            return AVCS_ERR_INVALID_VAL;
        }
        avCodecContext_->extradata = static_cast<uint8_t *>(av_mallocz(extraSize + AV_INPUT_BUFFER_PADDING_SIZE));
        if (avCodecContext_->extradata == nullptr) {
            AVCODEC_LOGE("extradata malloc failed!");
            return AVCS_ERR_INVALID_VAL;
        }
        avCodecContext_->extradata_size = static_cast<int>(extraSize);
        errno_t rc = memcpy_s(avCodecContext_->extradata, extraSize, extraData, extraSize);
        if (rc != EOK) {
            AVCODEC_LOGE("extradata memcpy_s failed.");
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
        }
        rc = memset_s(avCodecContext_->extradata + extraSize, AV_INPUT_BUFFER_PADDING_SIZE, 0,
                      AV_INPUT_BUFFER_PADDING_SIZE);
        if (rc != EOK) {
            AVCODEC_LOGE("extradata memset_s failed.");
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
        }
    }
    return AVCS_ERR_OK;
}
#endif

int32_t FCodec::Configure(const Format &format)
{
    AVCODEC_SYNC_TRACE;
    if (state_ == State::UNINITIALIZED) {
        int32_t ret = Initialize();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Init codec failed");
    }
    CHECK_AND_RETURN_RET_LOG((state_ == State::INITIALIZED), AVCS_ERR_INVALID_STATE,
                             "Configure codec failed: not in Initialized state");
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, DEFAULT_OUT_BUFFER_CNT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, DEFAULT_IN_BUFFER_CNT);
    for (auto &it : format.GetFormatMap()) {
        if (it.first == MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT) {
            isOutBufSetted_ = true;
            ConfigureDefaultVal(format, it.first, DEFAULT_MIN_BUFFER_CNT);
        } else if (it.first == MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT) {
            ConfigureDefaultVal(format, it.first, DEFAULT_MIN_BUFFER_CNT);
        } else if (it.first == MediaDescriptionKey::MD_KEY_WIDTH) {
            ConfigureDefaultVal(format, it.first, VIDEO_MIN_SIZE, VIDEO_MAX_WIDTH_SIZE);
        } else if (it.first == MediaDescriptionKey::MD_KEY_HEIGHT) {
            ConfigureDefaultVal(format, it.first, VIDEO_MIN_SIZE, VIDEO_MAX_HEIGHT_SIZE);
        } else if (it.first == MediaDescriptionKey::MD_KEY_BITRATE) {
            int64_t val64 = 0L;
            CHECK_AND_RETURN_RET_LOG(format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, val64) && val64 > 0L,
                                     AVCS_ERR_INVALID_VAL, "Cannot get bit rate!");
            format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, val64);
        } else if (it.first == MediaDescriptionKey::MD_KEY_FRAME_RATE) {
            double val = 0;
            CHECK_AND_RETURN_RET_LOG(format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, val) && val > 0.0f,
                                     AVCS_ERR_INVALID_VAL, "Cannot get frame rate!");
            format_.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, val);
        } else if (it.first == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT ||
                   it.first == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE ||
                   it.first == MediaDescriptionKey::MD_KEY_SCALE_TYPE) {
            ConfigureSurface(format, it.first, it.second.type);
        } else {
            AVCODEC_LOGW("Set parameter failed: %{public}s, unsupport key", it.first.data());
        }
    }
    AVCODEC_LOGI("current pixel format %{public}d", static_cast<int32_t>(outputPixelFmt_));
    int32_t ret = ConfigureContext(format);

    state_ = State::CONFIGURED;
    AVCODEC_LOGI("Configured codec successful: state: Initialized -> Configured");
    return ret;
}

bool FCodec::IsActive() const
{
    return state_ == State::RUNNING || state_ == State::FLUSHED || state_ == State::EOS;
}

void FCodec::ResetContext(bool isFlush)
{
    CHECK_AND_RETURN_LOG(avCodecContext_ != nullptr, "Avcodec context is nullptr");
    if (avCodecContext_->extradata) {
        av_free(avCodecContext_->extradata);
        avCodecContext_->extradata = nullptr;
    }
    avCodecContext_->coded_width = 0;
    avCodecContext_->coded_height = 0;
    avCodecContext_->extradata_size = 0;
    if (!isFlush) {
        avCodecContext_->width = 0;
        avCodecContext_->height = 0;
        avCodecContext_->get_buffer2 = nullptr;
    }
}

int32_t FCodec::Start()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Start codec failed: callback is null");
    CHECK_AND_RETURN_RET_LOG((state_ == State::CONFIGURED || state_ == State::FLUSHED), AVCS_ERR_INVALID_STATE,
                             "Start codec failed: not in Configured or Flushed state");
    if (state_ != State::FLUSHED) {
        CHECK_AND_RETURN_RET_LOG(avcodec_open2(avCodecContext_.get(), avCodec_.get(), nullptr) == 0, AVCS_ERR_UNKNOWN,
                                 "Start codec failed: cannot open avcodec");
    }
    if (!isBufferAllocated_) {
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *p) { av_frame_free(&p); });
        avPacket_ = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *p) { av_packet_free(&p); });
        CHECK_AND_RETURN_RET_LOG((cachedFrame_ != nullptr && avPacket_ != nullptr), AVCS_ERR_UNKNOWN,
                                 "Start codec failed: cannot allocate frame or packet");
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
    receiveTask_->Start();
    AVCODEC_LOGI("Start codec successful, state: Running");
    return AVCS_ERR_OK;
}

void FCodec::InitBuffers()
{
    inputAvailQue_->SetActive(true);
    codecAvailQue_->SetActive(true);
    if (sInfo_.surface != nullptr) {
        renderAvailQue_->SetActive(true);
        requestSurfaceBufferQue_->SetActive(true);
    }
    CHECK_AND_RETURN_LOG(buffers_[INDEX_INPUT].size() > 0, "Input buffer is null!");
    for (uint32_t i = 0u; i < buffers_[INDEX_INPUT].size(); i++) {
        buffers_[INDEX_INPUT][i]->owner_ = Owner::OWNED_BY_USER;
        callback_->OnInputBufferAvailable(i, buffers_[INDEX_INPUT][i]->avBuffer_);
        AVCODEC_LOGI("OnInputBufferAvailable frame index = %{public}u, owner = %{public}d", i,
                     buffers_[INDEX_INPUT][i]->owner_.load());
    }
    CHECK_AND_RETURN_LOG(buffers_[INDEX_OUTPUT].size() > 0, "Output buffer is null!");
    for (uint32_t i = 0u; i < buffers_[INDEX_OUTPUT].size(); i++) {
        buffers_[INDEX_OUTPUT][i]->owner_ = Owner::OWNED_BY_CODEC;
        codecAvailQue_->Push(i);
    }
}

void FCodec::ResetData()
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

void FCodec::ResetBuffers()
{
    inputAvailQue_->Clear();
    std::unique_lock<std::mutex> iLock(inputMutex_);
    synIndex_ = std::nullopt;
    iLock.unlock();
    codecAvailQue_->Clear();
    if (sInfo_.surface != nullptr) {
        renderAvailQue_->Clear();
        requestSurfaceBufferQue_->Clear();
        renderSurfaceBufferMap_.clear();
    }
    ResetData();
    av_frame_unref(cachedFrame_.get());
    av_packet_unref(avPacket_.get());
}

void FCodec::StopThread()
{
    if (sendTask_ != nullptr && inputAvailQue_ != nullptr) {
        std::unique_lock<std::mutex> sLock(sendMutex_);
        sendCv_.notify_one();
        sLock.unlock();
        inputAvailQue_->SetActive(false, false);
        sendTask_->Stop();
    }
    if (receiveTask_ != nullptr && codecAvailQue_ != nullptr) {
        std::unique_lock<std::mutex> rLock(recvMutex_);
        recvCv_.notify_one();
        rLock.unlock();
        codecAvailQue_->SetActive(false, false);
        receiveTask_->Stop();
    }
    if (sInfo_.surface != nullptr) {
        if (renderAvailQue_ != nullptr) {
            renderAvailQue_->SetActive(false, false);
        }
        if (requestSurfaceBufferQue_ != nullptr) {
            requestSurfaceBufferQue_->SetActive(false, false);
        }
    }
}

int32_t FCodec::Stop()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG((IsActive()), AVCS_ERR_INVALID_STATE, "Stop codec failed: not in executing state");
    state_ = State::STOPPING;
    AVCODEC_LOGI("step into STOPPING status");
    StopThread();
    avcodec_close(avCodecContext_.get());
    ResetContext(true);
    ReleaseBuffers();
    state_ = State::CONFIGURED;
    AVCODEC_LOGI("Stop codec successful, state: Configured");
    return AVCS_ERR_OK;
}

int32_t FCodec::Flush()
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
    sendTask_->Pause();

    if (sInfo_.surface != nullptr) {
        renderAvailQue_->SetActive(false, false);
        requestSurfaceBufferQue_->SetActive(false, false);
    }
    std::unique_lock<std::mutex> rLock(recvMutex_);
    recvCv_.notify_one();
    rLock.unlock();
    codecAvailQue_->SetActive(false, false);
    receiveTask_->Pause();

    avcodec_flush_buffers(avCodecContext_.get());
    ResetContext(true);
    ResetBuffers();
    state_ = State::FLUSHED;
    AVCODEC_LOGI("Flush codec successful, state: Flushed");
    return AVCS_ERR_OK;
}

int32_t FCodec::Reset()
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

void FCodec::ReleaseResource()
{
    StopThread();
    if (avCodecContext_ != nullptr) {
        avcodec_close(avCodecContext_.get());
        ResetContext();
    }
    ReleaseBuffers();
    format_ = Format();
    CHECK_AND_RETURN_LOG(sInfo_.surface != nullptr, "Surface is nullptr!");
    UnRegisterListenerToSurface(sInfo_.surface);
    sInfo_.surface = nullptr;
    StopRequestSurfaceBufferThread();
}

int32_t FCodec::Release()
{
    AVCODEC_SYNC_TRACE;
    state_ = State::STOPPING;
    AVCODEC_LOGI("step into STOPPING status");
    ReleaseResource();
    state_ = State::UNINITIALIZED;
    AVCODEC_LOGI("Release codec successful, state: Uninitialized");
    return AVCS_ERR_OK;
}

void FCodec::SetSurfaceParameter(const Format &format, const std::string_view &formatKey, FormatDataType formatType)
{
    CHECK_AND_RETURN_LOG(formatType == FORMAT_TYPE_INT32, "Set parameter failed: type should be int32");
    int32_t val = 0;
    CHECK_AND_RETURN_LOG(format.GetIntValue(formatKey, val), "Set parameter failed: get value fail");
    if (formatKey == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT) {
        VideoPixelFormat vpf = static_cast<VideoPixelFormat>(val);
        CHECK_AND_RETURN_LOG(vpf == VideoPixelFormat::RGBA || vpf == VideoPixelFormat::YUVI420 ||
                             vpf == VideoPixelFormat::NV12 || vpf == VideoPixelFormat::NV21,
                             "Set parameter failed: pixel format value %{public}d invalid", val);
        outputPixelFmt_ = vpf;
        {
            std::lock_guard<std::mutex> lock(formatMutex_);
            format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, val);
        }
        GraphicPixelFormat surfacePixelFmt = TranslateSurfaceFormat(vpf);
        std::lock_guard<std::mutex> sLock(surfaceMutex_);
        sInfo_.requestConfig.format = surfacePixelFmt;
    } else if (formatKey == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE) {
        VideoRotation sr = static_cast<VideoRotation>(val);
        CHECK_AND_RETURN_LOG(sr == VideoRotation::VIDEO_ROTATION_0 || sr == VideoRotation::VIDEO_ROTATION_90 ||
                             sr == VideoRotation::VIDEO_ROTATION_180 || sr == VideoRotation::VIDEO_ROTATION_270,
                             "Set parameter failed: rotation angle value %{public}d invalid", val);
        {
            std::lock_guard<std::mutex> lock(formatMutex_);
            format_.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val);
        }
        std::lock_guard<std::mutex> sLock(surfaceMutex_);
        sInfo_.surface->SetTransform(TranslateSurfaceRotation(sr));
    } else {
        ScalingMode scaleMode = static_cast<ScalingMode>(val);
        CHECK_AND_RETURN_LOG(scaleMode == ScalingMode::SCALING_MODE_SCALE_TO_WINDOW ||
                             scaleMode == ScalingMode::SCALING_MODE_SCALE_CROP,
                             "Set parameter failed: scale type value %{public}d invalid", val);
        {
            std::lock_guard<std::mutex> lock(formatMutex_);
            format_.PutIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, val);
        }
        std::lock_guard<std::mutex> sLock(surfaceMutex_);
        sInfo_.scalingMode = val;
        sInfo_.surface->SetScalingMode(scaleMode);
    }
    AVCODEC_LOGI("Set parameter %{public}s success, val %{public}d", std::string(formatKey).c_str(), val);
}

int32_t FCodec::SetParameter(const Format &format)
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

int32_t FCodec::GetOutputFormat(Format &format)
{
    AVCODEC_SYNC_TRACE;
    if (!format_.ContainKey(MediaDescriptionKey::MD_KEY_BITRATE)) {
        if (avCodecContext_ != nullptr) {
            std::lock_guard<std::mutex> lock(formatMutex_);
            format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, avCodecContext_->bit_rate);
        }
    }
    if (!format_.ContainKey(MediaDescriptionKey::MD_KEY_FRAME_RATE)) {
        if (avCodecContext_ != nullptr && avCodecContext_->framerate.den > 0) {
            double value = static_cast<double>(avCodecContext_->framerate.num) /
                           static_cast<double>(avCodecContext_->framerate.den);
            std::lock_guard<std::mutex> lock(formatMutex_);
            format_.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, value);
        }
    }
    if (!format_.ContainKey(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE)) {
        int32_t stride = AlignUp(width_, VIDEO_ALIGN_SIZE);
        int32_t maxInputSize = static_cast<int32_t>((stride * height_ * VIDEO_PIX_DEPTH_YUV) / UV_SCALE_FACTOR);
        std::lock_guard<std::mutex> lock(formatMutex_);
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, maxInputSize);
    }
    {
        std::lock_guard<std::mutex> lock(formatMutex_);
        format = format_;
    }
    AVCODEC_LOGI("Get outputFormat successful");
    return AVCS_ERR_OK;
}

void FCodec::CalculateBufferSize()
{
    int32_t stride = AlignUp(width_, VIDEO_ALIGN_SIZE);
    outputBufferSize_ = static_cast<int32_t>((stride * height_ * VIDEO_PIX_DEPTH_YUV) / UV_SCALE_FACTOR);
    inputBufferSize_ = std::max(VIDEO_MIN_BUFFER_SIZE, outputBufferSize_);
    if (outputPixelFmt_ == VideoPixelFormat::RGBA) {
        outputBufferSize_ = static_cast<int32_t>(stride * height_ * VIDEO_PIX_DEPTH_RGBA);
    }
    AVCODEC_LOGI("width = %{public}d, height = %{public}d, stride = %{public}d, Input buffer size = %{public}d, output "
                 "buffer size=%{public}d",
                 width_, height_, stride, inputBufferSize_, outputBufferSize_);
}

int32_t FCodec::AllocateInputBuffer(int32_t bufferCnt, int32_t inBufferSize)
{
    int32_t valBufferCnt = 0;
    for (int32_t i = 0; i < bufferCnt; i++) {
        std::shared_ptr<FBuffer> buf = std::make_shared<FBuffer>();
        std::shared_ptr<AVAllocator> allocator =
            AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        CHECK_AND_CONTINUE_LOG(allocator != nullptr, "input buffer %{public}d allocator is nullptr", i);
        buf->avBuffer_ = AVBuffer::CreateAVBuffer(allocator, inBufferSize);
        CHECK_AND_CONTINUE_LOG(buf->avBuffer_ != nullptr && buf->avBuffer_->memory_ != nullptr,
                               "Allocate input buffer failed, index=%{public}d", i);
        AVCODEC_LOGI("Allocate input buffer success: index=%{public}d, size=%{public}d",
                     i, buf->avBuffer_->memory_->GetCapacity());
        buffers_[INDEX_INPUT].emplace_back(buf);
        valBufferCnt++;
    }
    CHECK_AND_RETURN_RET_LOGD(valBufferCnt < DEFAULT_MIN_BUFFER_CNT, AVCS_ERR_OK, "Allocate input buffers successful");
    AVCODEC_LOGE("Allocate input buffer failed: only %{public}d buffer is allocated, no memory", valBufferCnt);
    buffers_[INDEX_INPUT].clear();
    return AVCS_ERR_NO_MEMORY;
}

int32_t FCodec::SetSurfaceCfg()
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
    if (format_.ContainKey(MediaDescriptionKey::MD_KEY_SCALE_TYPE)) {
        CHECK_AND_RETURN_RET_LOG(format_.GetIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, val32) && val32 >= 0 &&
                                 val32 <= static_cast<int32_t>(ScalingMode::SCALING_MODE_SCALE_FIT),
                                 AVCS_ERR_INVALID_VAL, "Invalid scaling mode %{public}d", val32);
        sInfo_.scalingMode = val32;
        sInfo_.surface->SetScalingMode(static_cast<ScalingMode>(val32));
    }
    if (format_.ContainKey(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE)) {
        CHECK_AND_RETURN_RET_LOG(format_.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val32) && val32 >= 0 &&
                                 val32 <= static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_270),
                                 AVCS_ERR_INVALID_VAL, "Invalid rotation angle %{public}d", val32);
        sInfo_.surface->SetTransform(TranslateSurfaceRotation(static_cast<VideoRotation>(val32)));
    }
    return AVCS_ERR_OK;
}

void FCodec::RequestSurfaceBufferThread()
{
    while (!requestBufferThreadExit_.load()) {
        std::unique_lock<std::mutex> lck(requestBufferMutex_);
        requestBufferCV_.wait(lck, [this]() {
            return requestBufferThreadExit_.load() || !requestBufferFinished_.load();
        });
        if (requestBufferThreadExit_.load()) {
            requestBufferFinished_ = true;
            requestBufferOnceDoneCV_.notify_one();
            break;
        }
        auto index = requestSurfaceBufferQue_->Front();
        requestSurfaceBufferQue_->Pop();
        std::shared_ptr<FBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
        std::shared_ptr<FSurfaceMemory> surfaceMemory = outputBuffer->sMemory_;
        sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
        if (surfaceBuffer == nullptr) {
            AVCODEC_LOGE("Get surface buffer failed, index=%{public}u", index);
        } else {
            requestSucceed_ = true;
        }
        requestBufferFinished_ = true;
        requestBufferOnceDoneCV_.notify_one();
    }
    AVCODEC_LOGI("RequestSurfaceBufferThread exit.");
}

void FCodec::StartRequestSurfaceBufferThread()
{
    if (!mRequestSurfaceBufferThread_.joinable()) {
        requestBufferThreadExit_ = false;
        requestBufferFinished_ = true;
        mRequestSurfaceBufferThread_ = std::thread(&FCodec::RequestSurfaceBufferThread, this);
    }
}

void FCodec::StopRequestSurfaceBufferThread()
{
    if (mRequestSurfaceBufferThread_.joinable()) {
        requestBufferThreadExit_ = true;
        requestBufferFinished_ = false;
        requestBufferCV_.notify_all();
        requestBufferFinished_ = true;
        requestBufferOnceDoneCV_.notify_all();
        mRequestSurfaceBufferThread_.join();
    }
}

bool FCodec::RequestSurfaceBufferOnce(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(!requestBufferThreadExit_.load(), false, "request surfacebuffer thread exited!");
    std::unique_lock<std::mutex> lck(requestBufferMutex_);
    requestSucceed_ = false;
    requestBufferFinished_ = false;
    requestSurfaceBufferQue_->Push(index);
    requestBufferCV_.notify_one();
    requestBufferOnceDoneCV_.wait(lck, [this]() { return requestBufferFinished_.load(); });
    CHECK_AND_RETURN_RET_LOG(requestSucceed_.load(), false, "Output surface memory %{public}u allocate failed", index);
    return true;
}

int32_t FCodec::AllocateOutputBuffer(int32_t bufferCnt, int32_t outBufferSize)
{
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface == nullptr, AVCS_ERR_UNKNOWN, "Not in buffer mode!");
    int32_t valBufferCnt = 0;
    for (int i = 0; i < bufferCnt; i++) {
        std::shared_ptr<FBuffer> buf = std::make_shared<FBuffer>();
        buf->width_ = width_;
        buf->height_ = height_;
        std::shared_ptr<AVAllocator> allocator =
            AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        CHECK_AND_CONTINUE_LOG(allocator != nullptr, "Output buffer %{public}d allocator is nullptr!", i);
        buf->avBuffer_ = AVBuffer::CreateAVBuffer(allocator, outBufferSize);
        CHECK_AND_CONTINUE_LOG(buf->avBuffer_ != nullptr && buf->avBuffer_->memory_ != nullptr,
                               "Allocate output buffer failed, index=%{public}d", i);
        AVCODEC_LOGI("Allocate output share buffer success: index=%{public}d, size=%{public}d", i,
                     buf->avBuffer_->memory_->GetCapacity());
        CHECK_AND_CONTINUE_LOG(buf->avBuffer_ != nullptr, "Allocate output buffer failed, index=%{public}d", i);
        buffers_[INDEX_OUTPUT].emplace_back(buf);
        valBufferCnt++;
    }
    CHECK_AND_RETURN_RET_LOGD(valBufferCnt < DEFAULT_MIN_BUFFER_CNT, AVCS_ERR_OK, "Allocate output buffers successful");
    AVCODEC_LOGE("Allocate output buffer failed: only %{public}d buffer is allocated, no memory", valBufferCnt);
    buffers_[INDEX_INPUT].clear();
    buffers_[INDEX_OUTPUT].clear();
    return AVCS_ERR_NO_MEMORY;
}

int32_t FCodec::ClearSurfaceAndSetQueueSize(const sptr<Surface> &surface, int32_t bufferCnt)
{
    surface->Connect();
    surface->CleanCache(); // clean cache will work only if the surface is connected by us.
    int32_t ret = SetQueueSize(surface, bufferCnt);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Set surface queue size failed!");
    ret = SetSurfaceCfg();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Set surface cfg failed!");
    CHECK_AND_RETURN_RET_LOGD(buffers_[INDEX_OUTPUT].size() > 0u, AVCS_ERR_OK, "Set surface cfg & queue size success.");
    int32_t valBufferCnt = 0;
    for (auto &it : buffers_[INDEX_OUTPUT]) {
        std::shared_ptr<FSurfaceMemory> surfaceMemory = it->sMemory_;
        surfaceMemory->isAttached = false;
        valBufferCnt++;
    }
    CHECK_AND_RETURN_RET_LOG(valBufferCnt == bufferCnt, AVCS_ERR_UNKNOWN, "Outbuf cnt(%{public}d) != %{public}d",
                             valBufferCnt, bufferCnt);
    return AVCS_ERR_OK;
}

int32_t FCodec::AllocateOutputBuffersFromSurface(int32_t bufferCnt)
{
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AVCS_ERR_UNKNOWN, "Not in surface mode!");
    int32_t ret = ClearSurfaceAndSetQueueSize(sInfo_.surface, bufferCnt);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Clean surface and set queue size failed!");
    requestSurfaceBufferQue_->Clear();
    requestSurfaceBufferQue_->SetActive(true);
    StartRequestSurfaceBufferThread();
    for (int32_t i = 0; i < bufferCnt; i++) {
        std::shared_ptr<FSurfaceMemory> surfaceMemory = std::make_shared<FSurfaceMemory>(&sInfo_);
        CHECK_AND_RETURN_RET_LOG(surfaceMemory != nullptr, AVCS_ERR_UNKNOWN, "Creata surface memory failed!");
        ret = surfaceMemory->AllocSurfaceBuffer();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Alloc surface buffer failed!");
        std::shared_ptr<FBuffer> buf = std::make_shared<FBuffer>();
        CHECK_AND_RETURN_RET_LOG(buf != nullptr, AVCS_ERR_UNKNOWN, "Creata output buffer failed!");
        buf->sMemory_ = surfaceMemory;
        buf->height_ = height_;
        buf->width_ = width_;
        outAVBuffer4Surface_.emplace_back(AVBuffer::CreateAVBuffer());
        buf->avBuffer_ = AVBuffer::CreateAVBuffer(buf->sMemory_->GetBase(), buf->sMemory_->GetSize());
        AVCODEC_LOGI("Allocate output surface buffer success, index=%{public}d, size=%{public}d, stride=%{public}d", i,
                     buf->sMemory_->GetSize(), buf->sMemory_->GetSurfaceBufferStride());
        buffers_[INDEX_OUTPUT].emplace_back(buf);
    }
    int32_t outputBufferNum = static_cast<int32_t>(buffers_[INDEX_OUTPUT].size());
    CHECK_AND_RETURN_RET_LOG(outputBufferNum == bufferCnt, AVCS_ERR_UNKNOWN,
                             "Only alloc %{public}d buffers, less %{public}d", outputBufferNum, bufferCnt);
    return AVCS_ERR_OK;
}

int32_t FCodec::AllocateBuffers()
{
    AVCODEC_SYNC_TRACE;
    CalculateBufferSize();
    CHECK_AND_RETURN_RET_LOG(inputBufferSize_ > 0 && outputBufferSize_ > 0, AVCS_ERR_INVALID_VAL,
                             "Allocate buffer with input size=%{public}d, output size=%{public}d failed",
                             inputBufferSize_, outputBufferSize_);
    if (sInfo_.surface != nullptr && isOutBufSetted_ == false) {
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, DEFAULT_OUT_SURFACE_CNT);
    }
    CHECK_AND_RETURN_RET_LOG(
        format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, inputBufferCnt_) &&
        inputBufferCnt_ >= DEFAULT_MIN_BUFFER_CNT, AVCS_ERR_INVALID_VAL,
        "Invalid input buffer cnt: %{public}d", inputBufferCnt_);
    CHECK_AND_RETURN_RET_LOG(
        format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, outputBufferCnt_) &&
        outputBufferCnt_ >= DEFAULT_MIN_BUFFER_CNT, AVCS_ERR_INVALID_VAL,
        "Invalid output buffer cnt: %{public}d", outputBufferCnt_);
    inputAvailQue_ = std::make_shared<BlockQueue<uint32_t>>("inputAvailQue", inputBufferCnt_);
    codecAvailQue_ = std::make_shared<BlockQueue<uint32_t>>("codecAvailQue", outputBufferCnt_);
    if (sInfo_.surface != nullptr) {
        renderAvailQue_ = std::make_shared<BlockQueue<uint32_t>>("renderAvailQue", outputBufferCnt_);
        requestSurfaceBufferQue_ = std::make_shared<BlockQueue<uint32_t>>("requestSurfaceBufferQue", outputBufferCnt_);
    }
    int32_t ret = AllocateInputBuffer(inputBufferCnt_, inputBufferSize_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Allocate input buffers failed!");
    ret = sInfo_.surface ? AllocateOutputBuffersFromSurface(outputBufferCnt_)
                         : AllocateOutputBuffer(outputBufferCnt_, outputBufferSize_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Allocate output buffers failed!");
    return AVCS_ERR_OK;
}

int32_t FCodec::UpdateBuffers(uint32_t index, int32_t bufferSize, uint32_t bufferType)
{
    int32_t curBufSize = buffers_[bufferType][index]->avBuffer_->memory_->GetCapacity();
    if (bufferSize != curBufSize) {
        std::shared_ptr<FBuffer> buf = std::make_shared<FBuffer>();
        std::shared_ptr<AVAllocator> allocator =
            AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        CHECK_AND_RETURN_RET_LOG(allocator != nullptr, AVCS_ERR_NO_MEMORY, "buffer %{public}d allocator is nullptr",
                                 index);
        buf->avBuffer_ = AVBuffer::CreateAVBuffer(allocator, bufferSize);
        CHECK_AND_RETURN_RET_LOG(buf->avBuffer_ != nullptr, AVCS_ERR_NO_MEMORY,
                                 "Buffer allocate failed, index=%{public}d", index);
        AVCODEC_LOGI("update share buffer success: bufferType=%{public}u, index=%{public}d, size=%{public}d",
                     bufferType, index, buf->avBuffer_->memory_->GetCapacity());

        if (bufferType == INDEX_INPUT) {
            buf->owner_ = Owner::OWNED_BY_USER;
        } else {
            buf->owner_ = Owner::OWNED_BY_CODEC;
        }
        buffers_[bufferType][index] = buf;
    }
    return AVCS_ERR_OK;
}

int32_t FCodec::UpdateSurfaceMemory(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    std::unique_lock<std::mutex> oLock(outputMutex_);
    std::shared_ptr<FBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
    oLock.unlock();
    if (width_ != outputBuffer->width_ || height_ != outputBuffer->height_) {
        std::shared_ptr<FSurfaceMemory> surfaceMemory = outputBuffer->sMemory_;
        CHECK_AND_RETURN_RET_LOG(surfaceMemory != nullptr, AVCS_ERR_UNKNOWN, "Surface memory is nullptr!");
        AVCODEC_LOGI("Update surface memory, width=%{public}d, height=%{public}d", width_, height_);
        std::lock_guard<std::mutex> sLock(surfaceMutex_);
        if (surfaceMemory->isAttached) {
            sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
            CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_UNKNOWN, "Get surface buffer failed!");
            int32_t ret = Detach(surfaceBuffer);
            CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Surface buffer detach failed!");
            surfaceMemory->isAttached = false;
        }
        surfaceMemory->ReleaseSurfaceBuffer();
        int32_t ret = surfaceMemory->AllocSurfaceBuffer();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Alloc surface buffer failed!");
        sptr<SurfaceBuffer> newSurfaceBuffer = surfaceMemory->GetSurfaceBuffer();
        CHECK_AND_RETURN_RET_LOG(newSurfaceBuffer != nullptr, AVCS_ERR_UNKNOWN, "Alloc surface buffer failed!");
        ret = Attach(newSurfaceBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Surface buffer attach failed!");
        surfaceMemory->isAttached = true;
        outputBuffer->avBuffer_ =
            AVBuffer::CreateAVBuffer(outputBuffer->sMemory_->GetBase(), outputBuffer->sMemory_->GetSize());
        outputBuffer->width_ = width_;
        outputBuffer->height_ = height_;
    }
    return AVCS_ERR_OK;
}

int32_t FCodec::CheckFormatChange(uint32_t index, int width, int height)
{
    if (width_ != width || height_ != height) {
        AVCODEC_LOGI("format change, width: %{public}d->%{public}d, height: %{public}d->%{public}d", width_, width,
                     height_, height);
        width_ = width;
        height_ = height;
        ResetData();
        scale_ = nullptr;
        CalculateBufferSize();
        {
            std::lock_guard<std::mutex> lock(formatMutex_);
            format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width_);
            format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height_);
            format_.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE,
                                outputPixelFmt_ == VideoPixelFormat::RGBA ? width_ * VIDEO_PIX_DEPTH_RGBA : width_);
            format_.PutIntValue(OHOS::Media::Tag::VIDEO_SLICE_HEIGHT, height_);
            format_.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_WIDTH, width_);
            format_.PutIntValue(OHOS::Media::Tag::VIDEO_PIC_HEIGHT, height_);
        }
        if (sInfo_.surface) {
            std::lock_guard<std::mutex> sLock(surfaceMutex_);
            sInfo_.requestConfig.width = width_;
            sInfo_.requestConfig.height = height_;
        }
        callback_->OnOutputFormatChanged(format_);
    }
    if (sInfo_.surface == nullptr) {
        std::lock_guard<std::mutex> oLock(outputMutex_);
        CHECK_AND_RETURN_RET_LOG((UpdateBuffers(index, outputBufferSize_, INDEX_OUTPUT) == AVCS_ERR_OK),
                                 AVCS_ERR_NO_MEMORY, "Update  output buffer failed, index=%{public}u", index);
    } else {
        CHECK_AND_RETURN_RET_LOG((UpdateSurfaceMemory(index) == AVCS_ERR_OK), AVCS_ERR_NO_MEMORY,
                                 "Update buffer failed");
    }
    return AVCS_ERR_OK;
}

void FCodec::ReleaseBuffers()
{
    ResetData();
    if (!isBufferAllocated_) {
        return;
    }

    inputAvailQue_->Clear();
    std::unique_lock<std::mutex> iLock(inputMutex_);
    buffers_[INDEX_INPUT].clear();
    inputBufferCnt_ = 0;
    synIndex_ = std::nullopt;
    iLock.unlock();

    std::unique_lock<std::mutex> oLock(outputMutex_);
    codecAvailQue_->Clear();
    if (sInfo_.surface != nullptr) {
        StopRequestSurfaceBufferThread();
        renderAvailQue_->Clear();
        requestSurfaceBufferQue_->Clear();
        renderSurfaceBufferMap_.clear();
        for (uint32_t i = 0; i < buffers_[INDEX_OUTPUT].size(); i++) {
            std::shared_ptr<FBuffer> outputBuffer = buffers_[INDEX_OUTPUT][i];
            if (outputBuffer->owner_ == Owner::OWNED_BY_CODEC) {
                std::shared_ptr<FSurfaceMemory> surfaceMemory = outputBuffer->sMemory_;
                surfaceMemory->ReleaseSurfaceBuffer();
                outputBuffer->owner_ = Owner::OWNED_BY_SURFACE;
            }
        }
        sInfo_.surface->CleanCache();
        AVCODEC_LOGI("surface cleancache success");
    }
    buffers_[INDEX_OUTPUT].clear();
    outAVBuffer4Surface_.clear();
    outputBufferCnt_ = 0;
    oLock.unlock();
    isBufferAllocated_ = false;
}

int32_t FCodec::QueueInputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, AVCS_ERR_INVALID_STATE,
                             "Queue input buffer failed: not in Running state");
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_INPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Queue input buffer failed with bad index, index=%{public}u, buffer_size=%{public}zu",
                             index, buffers_[INDEX_INPUT].size());
    std::shared_ptr<FBuffer> inputBuffer = buffers_[INDEX_INPUT][index];
    CHECK_AND_RETURN_RET_LOG(inputBuffer->owner_ == Owner::OWNED_BY_USER, AVCS_ERR_INVALID_OPERATION,
                             "Queue input buffer failed: buffer with index=%{public}u is not available", index);

    inputBuffer->owner_ = Owner::OWNED_BY_CODEC;
    std::shared_ptr<AVBuffer> &inputAVBuffer = inputBuffer->avBuffer_;
    if (synIndex_) {
        const std::shared_ptr<AVBuffer> &curAVBuffer = buffers_[INDEX_INPUT][synIndex_.value()]->avBuffer_;
        int32_t curAVBufferSize = curAVBuffer->memory_->GetSize();
        int32_t inputAVBufferSize = inputAVBuffer->memory_->GetSize();
        if ((curAVBufferSize + inputAVBufferSize <= curAVBuffer->memory_->GetCapacity()) &&
            memcpy_s(curAVBuffer->memory_->GetAddr() + curAVBufferSize, inputAVBufferSize,
                     inputAVBuffer->memory_->GetAddr(), inputAVBufferSize) == EOK) {
            curAVBuffer->memory_->SetSize(curAVBufferSize + inputAVBufferSize);
            curAVBuffer->flag_ = inputAVBuffer->flag_;
            curAVBuffer->pts_ = inputAVBuffer->pts_;

            if (inputAVBuffer->flag_ != AVCODEC_BUFFER_FLAG_CODEC_DATA &&
                inputAVBuffer->flag_ != AVCODEC_BUFFER_FLAG_PARTIAL_FRAME) {
                inputAvailQue_->Push(synIndex_.value());
                synIndex_ = std::nullopt;
            }
            inputBuffer->owner_ = Owner::OWNED_BY_USER;
            callback_->OnInputBufferAvailable(index, inputAVBuffer);
            return AVCS_ERR_OK;
        } else {
            AVCODEC_LOGE("packet size %{public}d over buffer size %{public}d", curAVBufferSize + inputAVBufferSize,
                         curAVBuffer->memory_->GetCapacity());
            callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY);
            state_ = State::ERROR;
            return AVCS_ERR_NO_MEMORY;
        }
    } else {
        if ((inputAVBuffer->flag_ == AVCODEC_BUFFER_FLAG_CODEC_DATA) ||
            (inputAVBuffer->flag_ == AVCODEC_BUFFER_FLAG_PARTIAL_FRAME)) {
            synIndex_ = index;
        } else {
            inputAvailQue_->Push(index);
        }
    }

    return AVCS_ERR_OK;
}

void FCodec::SendFrame()
{
    CHECK_AND_RETURN_LOG(state_ != State::STOPPING && state_ != State::FLUSHING, "Invalid state");
    if (state_ != State::RUNNING || isSendEos_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    uint32_t index = inputAvailQue_->Front();
    CHECK_AND_RETURN_LOG(state_ == State::RUNNING, "Not in running state");
    std::shared_ptr<FBuffer> &inputBuffer = buffers_[INDEX_INPUT][index];
    std::shared_ptr<AVBuffer> &inputAVBuffer = inputBuffer->avBuffer_;
    if (inputAVBuffer->flag_ & AVCODEC_BUFFER_FLAG_EOS) {
        avPacket_->data = nullptr;
        avPacket_->size = 0;
        avPacket_->pts = 0;
        std::unique_lock<std::mutex> sendLock(sendMutex_);
        isSendEos_ = true;
        sendCv_.wait_for(sendLock, std::chrono::milliseconds(DEFAULT_DECODE_WAIT_TIME));
        AVCODEC_LOGI("Send eos end");
    } else {
        avPacket_->data = inputAVBuffer->memory_->GetAddr();
        avPacket_->size = static_cast<int32_t>(inputAVBuffer->memory_->GetSize());
        avPacket_->pts = inputAVBuffer->pts_;
    }
    std::unique_lock<std::mutex> sLock(syncMutex_);
    int ret = avcodec_send_packet(avCodecContext_.get(), avPacket_.get());
    sLock.unlock();
    if (ret == 0 || ret == AVERROR_INVALIDDATA) {
        EXPECT_AND_LOGW(ret == AVERROR_INVALIDDATA, "ffmpeg ret = %{public}s", AVStrError(ret).c_str());
        std::unique_lock<std::mutex> recvLock(recvMutex_);
        recvCv_.notify_one();
        recvLock.unlock();
        inputAvailQue_->Pop();
        inputBuffer->owner_ = Owner::OWNED_BY_USER;
#ifdef BUILD_ENG_VERSION
        if (dumpInFile_ && dumpInFile_->is_open()) {
            dumpInFile_->write(reinterpret_cast<char *>(inputAVBuffer->memory_->GetAddr()),
                               static_cast<int32_t>(inputAVBuffer->memory_->GetSize()));
        }
#endif // BUILD_ENG_VERSION
        callback_->OnInputBufferAvailable(index, inputAVBuffer);
    } else if (ret == AVERROR(EAGAIN)) {
        std::unique_lock<std::mutex> sendLock(sendMutex_);
        isSendWait_ = true;
        sendCv_.wait_for(sendLock, std::chrono::milliseconds(DEFAULT_DECODE_WAIT_TIME));
    } else {
        AVCODEC_LOGE("Cannot send frame to codec: ffmpeg ret = %{public}s", AVStrError(ret).c_str());
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
        state_ = State::ERROR;
    }
}

int32_t FCodec::FillFrameBuffer(const std::shared_ptr<FBuffer> &frameBuffer)
{
    VideoPixelFormat targetPixelFmt = outputPixelFmt_;
    if (outputPixelFmt_ == VideoPixelFormat::UNKNOWN) {
        targetPixelFmt = sInfo_.surface ? VideoPixelFormat::NV12 : ConvertPixelFormatFromFFmpeg(cachedFrame_->format);
    }
    AVPixelFormat ffmpegFormat = ConvertPixelFormatToFFmpeg(targetPixelFmt);
    int32_t ret;
    if (ffmpegFormat == static_cast<AVPixelFormat>(cachedFrame_->format)) {
        for (int32_t i = 0; i < AV_NUM_DATA_POINTERS && cachedFrame_->linesize[i] > 0; i++) {
            scaleData_[i] = cachedFrame_->data[i];
            scaleLineSize_[i] = cachedFrame_->linesize[i];
        }
    } else {
        ret = ConvertVideoFrame(&scale_, cachedFrame_, scaleData_, scaleLineSize_, ffmpegFormat);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Scale video frame failed: %{public}d", ret);
        isConverted_ = true;
    }
    {
        std::lock_guard<std::mutex> lock(formatMutex_);
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(targetPixelFmt));
    }
    std::shared_ptr<AVMemory> &bufferMemory = frameBuffer->avBuffer_->memory_;
    CHECK_AND_RETURN_RET_LOG(bufferMemory != nullptr, AVCS_ERR_INVALID_VAL, "bufferMemory is nullptr");
    bufferMemory->SetSize(0);
    if (sInfo_.surface) {
        struct SurfaceInfo surfaceInfo;
        surfaceInfo.surfaceStride = static_cast<uint32_t>(frameBuffer->sMemory_->GetSurfaceBufferStride());
        surfaceInfo.surfaceFence = frameBuffer->sMemory_->GetFence();
        surfaceInfo.scaleData = scaleData_;
        surfaceInfo.scaleLineSize = scaleLineSize_;
        ret = WriteSurfaceData(bufferMemory, surfaceInfo, format_);
    } else {
        ret = WriteBufferData(bufferMemory, scaleData_, scaleLineSize_, format_);
    }
    frameBuffer->avBuffer_->pts_ = cachedFrame_->pts;
    AVCODEC_LOGD("Fill frame buffer successful");
    return ret;
}

void FCodec::FramePostProcess(std::shared_ptr<FBuffer> &frameBuffer, uint32_t index, int32_t status, int ret)
{
    if (status == AVCS_ERR_OK) {
        codecAvailQue_->Pop();
        frameBuffer->owner_ = Owner::OWNED_BY_USER;
        if (sInfo_.surface) {
            outAVBuffer4Surface_[index]->pts_ = frameBuffer->avBuffer_->pts_;
            outAVBuffer4Surface_[index]->flag_ = frameBuffer->avBuffer_->flag_;
        }
        if (ret == AVERROR_EOF) {
            std::unique_lock<std::mutex> sLock(syncMutex_);
            avcodec_flush_buffers(avCodecContext_.get());
            sLock.unlock();
        } else {
            if (isSendWait_) {
                std::lock_guard<std::mutex> sLock(sendMutex_);
                isSendWait_ = false;
                sendCv_.notify_one();
            }
        }
        callback_->OnOutputBufferAvailable(index,
                                           sInfo_.surface ? outAVBuffer4Surface_[index] : frameBuffer->avBuffer_);
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

void FCodec::DumpOutputBuffer()
{
    AVCODEC_LOGD("cur decNum: %{public}u", decNum_);
    if (decNum_ == 0) {
        callback_->OnOutputFormatChanged(format_);
    }
    decNum_++;
#ifdef BUILD_ENG_VERSION
    if (!dumpOutFile_ || !dumpOutFile_->is_open()) {
        return;
    }
    for (int32_t i = 0; i < cachedFrame_->height; i++) {
        dumpOutFile_->write(reinterpret_cast<char *>(cachedFrame_->data[0] + i * cachedFrame_->linesize[0]),
                            static_cast<int32_t>(cachedFrame_->width));
    }
    for (int32_t i = 0; i < cachedFrame_->height / 2; i++) { // 2
        dumpOutFile_->write(reinterpret_cast<char *>(cachedFrame_->data[1] + i * cachedFrame_->linesize[1]),
                            static_cast<int32_t>(cachedFrame_->width / 2)); // 2
    }
    for (int32_t i = 0; i < cachedFrame_->height / 2; i++) { // 2
        dumpOutFile_->write(reinterpret_cast<char *>(cachedFrame_->data[2] + i * cachedFrame_->linesize[2]),
                            static_cast<int32_t>(cachedFrame_->width / 2)); // 2
    }
#endif // BUILD_ENG_VERSION
}

void FCodec::ReceiveFrame()
{
    CHECK_AND_RETURN_LOG(state_ != State::STOPPING && state_ != State::FLUSHING, "Invalid state");
    if (state_ != State::RUNNING) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    auto index = codecAvailQue_->Front();
    CHECK_AND_RETURN_LOG(state_ == State::RUNNING, "Not in running state");
    std::shared_ptr<FBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
    std::unique_lock<std::mutex> sLock(syncMutex_);
    int ret = avcodec_receive_frame(avCodecContext_.get(), cachedFrame_.get());
    sLock.unlock();
    int32_t status = AVCS_ERR_OK;
    CHECK_AND_RETURN_LOG(ret != AVERROR_INVALIDDATA, "ffmpeg ret = %{public}s", AVStrError(ret).c_str());
    if (ret >= 0) {
        DumpOutputBuffer();
        if (CheckFormatChange(index, cachedFrame_->width, cachedFrame_->height) == AVCS_ERR_OK) {
            CHECK_AND_RETURN_LOG(state_ == State::RUNNING, "Not in running state");
            frameBuffer = buffers_[INDEX_OUTPUT][index];
            status = FillFrameBuffer(frameBuffer);
        } else {
            CHECK_AND_RETURN_LOG(state_ == State::RUNNING, "Not in running state");
            callback_->OnError(AVCODEC_ERROR_EXTEND_START, AVCS_ERR_NO_MEMORY);
            return;
        }
        frameBuffer->avBuffer_->flag_ = AVCODEC_BUFFER_FLAG_NONE;
    } else if (ret == AVERROR_EOF) {
        AVCODEC_LOGI("Receive eos");
        frameBuffer->avBuffer_->flag_ = AVCODEC_BUFFER_FLAG_EOS;
        frameBuffer->avBuffer_->memory_->SetSize(0);
        state_ = State::EOS;
    } else if (ret == AVERROR(EAGAIN)) {
        std::unique_lock<std::mutex> sendLock(sendMutex_);
        if (isSendWait_ || isSendEos_) {
            isSendWait_ = false;
            sendCv_.notify_one();
        }
        sendLock.unlock();
        std::unique_lock<std::mutex> recvLock(recvMutex_);
        recvCv_.wait_for(recvLock, std::chrono::milliseconds(DEFAULT_DECODE_WAIT_TIME));
        return;
    } else {
        AVCODEC_LOGE("Cannot recv frame from codec: ffmpeg ret = %{public}s", AVStrError(ret).c_str());
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
        state_ = State::ERROR;
        return;
    }
    FramePostProcess(frameBuffer, index, status, ret);
}

void FCodec::FindAvailIndex(uint32_t index)
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

int32_t FCodec::ReleaseOutputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    std::unique_lock<std::mutex> oLock(outputMutex_);
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_OUTPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Failed to release output buffer: invalid index");
    std::shared_ptr<FBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
    oLock.unlock();
    if (frameBuffer->owner_ == Owner::OWNED_BY_USER) {
        frameBuffer->owner_ = Owner::OWNED_BY_CODEC;
        codecAvailQue_->Push(index);
        return AVCS_ERR_OK;
    } else {
        AVCODEC_LOGE("Release output buffer failed: check your index=%{public}u", index);
        return AVCS_ERR_INVALID_VAL;
    }
}

int32_t FCodec::Attach(sptr<SurfaceBuffer> surfaceBuffer)
{
    int32_t err = sInfo_.surface->AttachBufferToQueue(surfaceBuffer);
    CHECK_AND_RETURN_RET_LOG(
        err == 0, err, "Surface(%{public}" PRIu64 "), attach buffer(%{public}u) to queue failed, GSError=%{public}d",
        sInfo_.surface->GetUniqueId(), surfaceBuffer->GetSeqNum(), err);
    return AVCS_ERR_OK;
}

int32_t FCodec::Detach(sptr<SurfaceBuffer> surfaceBuffer)
{
    int32_t err = sInfo_.surface->DetachBufferFromQueue(surfaceBuffer);
    CHECK_AND_RETURN_RET_LOG(
        err == 0, err, "Surface(%{public}" PRIu64 "), detach buffer(%{public}u) to queue failed, GSError=%{public}d",
        sInfo_.surface->GetUniqueId(), surfaceBuffer->GetSeqNum(), err);
    return AVCS_ERR_OK;
}

int32_t FCodec::FlushSurfaceMemory(std::shared_ptr<FSurfaceMemory> &surfaceMemory, uint32_t index)
{
    RequestSurfaceBufferOnce(index);
    sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_UNKNOWN, "Get surface buffer failed!");
    if (!surfaceMemory->isAttached) {
        int32_t ret = Attach(surfaceBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Surface buffer attach failed!");
        surfaceMemory->isAttached = true;
    }
    OHOS::BufferFlushConfig flushConfig = {{0, 0, surfaceBuffer->GetWidth(), surfaceBuffer->GetHeight()},
        outAVBuffer4Surface_[index]->pts_, -1};
    if (outAVBuffer4Surface_[index]->meta_->Find(OHOS::Media::Tag::VIDEO_DECODER_DESIRED_PRESENT_TIMESTAMP) !=
        outAVBuffer4Surface_[index]->meta_->end()) {
        outAVBuffer4Surface_[index]->meta_->Get<OHOS::Media::Tag::VIDEO_DECODER_DESIRED_PRESENT_TIMESTAMP>(
            flushConfig.desiredPresentTimestamp);
        outAVBuffer4Surface_[index]->meta_->Remove(OHOS::Media::Tag::VIDEO_DECODER_DESIRED_PRESENT_TIMESTAMP);
    }
    auto res = sInfo_.surface->FlushBuffer(surfaceBuffer, -1, flushConfig);
    if (res == GSERROR_BUFFER_NOT_INCACHE) {
        AVCODEC_LOGW("Surface(%{public}" PRIu64 "), flush buffer(seq=%{public}u) failed, try to recover",
                     sInfo_.surface->GetUniqueId(), surfaceBuffer->GetSeqNum());
        int32_t ret = ClearSurfaceAndSetQueueSize(sInfo_.surface, outputBufferCnt_);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Clean surface and set queue size failed!");
        ret = Attach(surfaceBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Surface buffer attach failed!");
        surfaceMemory->isAttached = true;
        res = sInfo_.surface->FlushBuffer(surfaceBuffer, -1, flushConfig);
    }
    surfaceMemory->owner = Owner::OWNED_BY_SURFACE;
    if (res != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        AVCODEC_LOGW("Failed to update surface memory: %{public}d", res);
        return AVCS_ERR_UNKNOWN;
    }
    renderSurfaceBufferMap_[index] = std::make_pair(surfaceBuffer, flushConfig);
    return AVCS_ERR_OK;
}

int32_t FCodec::RenderOutputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AVCS_ERR_UNSUPPORT,
                             "Render output buffer failed, surface is nullptr!");
    std::unique_lock<std::mutex> oLock(outputMutex_);
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_OUTPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Failed to render output buffer: invalid index");
    std::shared_ptr<FBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
    oLock.unlock();
    std::lock_guard<std::mutex> sLock(surfaceMutex_);
    std::shared_ptr<AVMemory> &bufferMemory = frameBuffer->avBuffer_->memory_;
    CHECK_AND_RETURN_RET_LOG(bufferMemory != nullptr, AVCS_ERR_INVALID_VAL, "bufferMemory is nullptr");
    int32_t size = bufferMemory->GetSize();
    CHECK_AND_RETURN_RET_LOGW(size > 0, AVCS_ERR_OK, "buf(%{public}u) size=%{public}d", index, size);
    if (frameBuffer->owner_ == Owner::OWNED_BY_USER) {
        std::shared_ptr<FSurfaceMemory> surfaceMemory = frameBuffer->sMemory_;
        int32_t ret = FlushSurfaceMemory(surfaceMemory, index);
        EXPECT_AND_LOGW(ret != AVCS_ERR_OK, "Flush surface memory(index=%{public}u) failed: %{public}d", index, ret);
        frameBuffer->owner_ = Owner::OWNED_BY_SURFACE;
        renderAvailQue_->Push(index);
        AVCODEC_LOGD("Render output buffer with index, index=%{public}u", index);
        return AVCS_ERR_OK;
    } else {
        AVCODEC_LOGE("Failed to render output buffer with bad index, index=%{public}u", index);
        return AVCS_ERR_INVALID_VAL;
    }
}

int32_t FCodec::ReplaceOutputSurfaceWhenRunning(sptr<Surface> newSurface)
{
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AV_ERR_OPERATE_NOT_PERMIT,
                             "Not support convert from AVBuffer Mode to Surface Mode");
    sptr<Surface> oldSurface = sInfo_.surface;
    uint64_t oldId = oldSurface->GetUniqueId();
    uint64_t newId = newSurface->GetUniqueId();
    AVCODEC_LOGI("Begin to switch surface %{public}" PRIu64 " -> %{public}" PRIu64 ".", oldId, newId);
    if (oldId == newId) {
        return AVCS_ERR_OK;
    }
    int32_t ret = RegisterListenerToSurface(newSurface);
    CHECK_AND_RETURN_RET_LOG(ret == GSERROR_OK, ret,
        "surface %{public}" PRIu64 ", RegisterListenerToSurface failed, GSError=%{public}d", newId, ret);
    int32_t outputBufferCnt = 0;
    CHECK_AND_RETURN_RET_LOG(
        format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, outputBufferCnt) &&
        outputBufferCnt >= DEFAULT_MIN_BUFFER_CNT, AVCS_ERR_INVALID_VAL,
        "Invalid output buffer cnt: %{public}d", outputBufferCnt);
    ret = SetQueueSize(newSurface, outputBufferCnt);
    if (ret != AVCS_ERR_OK) {
        UnRegisterListenerToSurface(newSurface);
        return ret;
    }
    std::unique_lock<std::mutex> sLock(surfaceMutex_);
    ret = SwitchBetweenSurface(newSurface);
    if (ret != AVCS_ERR_OK) {
        UnRegisterListenerToSurface(newSurface);
        sInfo_.surface = oldSurface;
        CombineConsumerUsage();
        return ret;
    }
    sLock.unlock();
    AVCODEC_LOGI("Switch surface %{public}" PRIu64 " -> %{public}" PRIu64 ".", oldId, newId);
    return AVCS_ERR_OK;
}

int32_t FCodec::SetQueueSize(const sptr<Surface> &surface, uint32_t targetSize)
{
    uint64_t surfaceId = surface->GetUniqueId();
    int32_t err = surface->SetQueueSize(targetSize);
    CHECK_AND_RETURN_RET_LOG(err == 0, err,
                             "Surface(%{public}" PRIu64 ") set queue size %{public}u failed, GSError=%{public}d",
                             surfaceId, targetSize, err);
    AVCODEC_LOGI("Surface(%{public}" PRIu64 ") set queue size %{public}u succss.", surfaceId, targetSize);
    return AVCS_ERR_OK;
}

int32_t FCodec::SwitchBetweenSurface(const sptr<Surface> &newSurface)
{
    sptr<Surface> curSurface = sInfo_.surface;
    newSurface->Connect(); // cleancache will work only if the surface is connected by us
    newSurface->CleanCache(); // make sure new surface is empty
    std::vector<uint32_t> ownedBySurfaceBufferIndex;
    uint64_t newId = newSurface->GetUniqueId();
    for (uint32_t index = 0; index < buffers_[INDEX_OUTPUT].size(); index++) {
        if (buffers_[INDEX_OUTPUT][index]->sMemory_ == nullptr) {
            continue;
        }
        sptr<SurfaceBuffer> surfaceBuffer = nullptr;
        if (buffers_[INDEX_OUTPUT][index]->owner_ == Owner::OWNED_BY_SURFACE) {
            if (renderSurfaceBufferMap_.count(index)) {
                surfaceBuffer = renderSurfaceBufferMap_[index].first;
                ownedBySurfaceBufferIndex.push_back(index);
            }
        } else {
            RequestSurfaceBufferOnce(index);
            surfaceBuffer = buffers_[INDEX_OUTPUT][index]->sMemory_->GetSurfaceBuffer();
        }
        CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_UNKNOWN, "Get old surface buffer error!");
        int32_t err = newSurface->AttachBufferToQueue(surfaceBuffer);
        CHECK_AND_RETURN_RET_LOG(
            err == 0, err, "surface(%{public}" PRIu64 ")attach buffer(seq=%{public}u) to queue failed,"
            "GSError=%{public}d", newId, surfaceBuffer->GetSeqNum(), err);
        buffers_[INDEX_OUTPUT][index]->sMemory_->isAttached = true;
    }
    int32_t val32 = 0;
    if (format_.ContainKey(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE)) {
        CHECK_AND_RETURN_RET_LOG(format_.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val32) && val32 >= 0 &&
                                 val32 <= static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_270),
                                 AVCS_ERR_INVALID_VAL, "Invalid rotation angle %{public}d", val32);
        sInfo_.surface->SetTransform(TranslateSurfaceRotation(static_cast<VideoRotation>(val32)));
    }
    sInfo_.surface = newSurface;
    CombineConsumerUsage();
    for (uint32_t index: ownedBySurfaceBufferIndex) {
        int32_t ret = RenderNewSurfaceWithOldBuffer(newSurface, index);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Old surface buffer render failed!");
    }
    UnRegisterListenerToSurface(curSurface);
    curSurface->CleanCache(true); // make sure old surface is empty and go black
    return AVCS_ERR_OK;
}

int32_t FCodec::RenderNewSurfaceWithOldBuffer(const sptr<Surface> &newSurface, uint32_t index)
{
    std::shared_ptr<FSurfaceMemory> surfaceMemory = buffers_[INDEX_OUTPUT][index]->sMemory_;
    sptr<SurfaceBuffer> surfaceBuffer = renderSurfaceBufferMap_[index].first;
    OHOS::BufferFlushConfig flushConfig = renderSurfaceBufferMap_[index].second;
    if (sInfo_.scalingMode >= 0) {
        newSurface->SetScalingMode(surfaceBuffer->GetSeqNum(), static_cast<ScalingMode>(sInfo_.scalingMode));
    }
    auto res = newSurface->FlushBuffer(surfaceBuffer, -1, flushConfig);
    if (res != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        AVCODEC_LOGE("Failed to update surface memory: %{public}d", res);
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

void FCodec::RequestBufferFromConsumer()
{
    auto index = renderAvailQue_->Front();
    RequestSurfaceBufferOnce(index);
    std::shared_ptr<FBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
    std::shared_ptr<FSurfaceMemory> surfaceMemory = outputBuffer->sMemory_;
    auto queSize = renderAvailQue_->Size();
    uint32_t curIndex = 0;
    uint32_t i = 0;
    for (i = 0; i < queSize; i++) {
        curIndex = renderAvailQue_->Pop();
        if (surfaceMemory->GetBase() == buffers_[INDEX_OUTPUT][curIndex]->avBuffer_->memory_->GetAddr() &&
            surfaceMemory->GetSize() == buffers_[INDEX_OUTPUT][curIndex]->avBuffer_->memory_->GetCapacity()) {
            buffers_[INDEX_OUTPUT][index]->sMemory_ = buffers_[INDEX_OUTPUT][curIndex]->sMemory_;
            buffers_[INDEX_OUTPUT][curIndex]->sMemory_ = surfaceMemory;
            break;
        } else {
            renderAvailQue_->Push(curIndex);
        }
    }
    if (i == queSize) {
        curIndex = index;
        outputBuffer->avBuffer_ = AVBuffer::CreateAVBuffer(surfaceMemory->GetBase(), surfaceMemory->GetSize());
        outputBuffer->width_ = width_;
        outputBuffer->height_ = height_;
        FindAvailIndex(curIndex);
    }
    buffers_[INDEX_OUTPUT][curIndex]->owner_ = Owner::OWNED_BY_CODEC;
    codecAvailQue_->Push(curIndex);
    if (renderSurfaceBufferMap_.count(curIndex)) {
        renderSurfaceBufferMap_.erase(curIndex);
    }
    AVCODEC_LOGD("Request output buffer success, index = %{public}u, queSize=%{public}zu, i=%{public}d", curIndex,
                 queSize, i);
}

GSError FCodec::BufferReleasedByConsumer(uint64_t surfaceId)
{
    CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING || state_ == State::EOS, GSERROR_NO_PERMISSION, "In valid state");
    std::lock_guard<std::mutex> sLock(surfaceMutex_);
    CHECK_AND_RETURN_RET_LOG(renderAvailQue_->Size() > 0, GSERROR_NO_BUFFER, "No available buffer");
    CHECK_AND_RETURN_RET_LOG(surfaceId == sInfo_.surface->GetUniqueId(), GSERROR_INVALID_ARGUMENTS,
                             "Ignore callback from old surface");
    RequestBufferFromConsumer();
    return GSERROR_OK;
}

void FCodec::UnRegisterListenerToSurface(const sptr<Surface> &surface)
{
    CHECK_AND_RETURN_LOG(surface != nullptr, "Surface is null, not need to unregister listener.");
    SurfaceTools::GetInstance().ReleaseSurface(decName_, surface, false);
}

int32_t FCodec::RegisterListenerToSurface(const sptr<Surface> &surface)
{
    uint64_t surfaceId = surface->GetUniqueId();
    wptr<FCodec> wp = this;
    bool ret =
        SurfaceTools::GetInstance().RegisterReleaseListener(decName_, surface, [wp, surfaceId](sptr<SurfaceBuffer> &) {
            sptr<FCodec> codec = wp.promote();
            if (!codec) {
                AVCODEC_LOGD("decoder is gone");
                return GSERROR_OK;
            }
            return codec->BufferReleasedByConsumer(surfaceId);
        });
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_UNKNOWN, "surface(%" PRIu64 ") register listener failed", surfaceId);
    StartRequestSurfaceBufferThread();
    return AVCS_ERR_OK;
}

void FCodec::CombineConsumerUsage()
{
    uint64_t defaultUsage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA;
    uint64_t consumerUsage = sInfo_.surface->GetDefaultUsage();
    uint64_t cfgedUsage = sInfo_.requestConfig.usage;
    uint64_t finalUsage = defaultUsage | consumerUsage | cfgedUsage;
    sInfo_.requestConfig.usage = finalUsage;
    AVCODEC_LOGI("Usage: default(0x%{public}" PRIu64 ") | consumer(0x%{public}" PRIu64 ") | cfged(0x%{public}" PRIu64
                 ") -> final(0x%{public}" PRIu64 ").",
                 defaultUsage, consumerUsage, cfgedUsage, finalUsage);
}

int32_t FCodec::SetOutputSurface(sptr<Surface> surface)
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
    UnRegisterListenerToSurface(sInfo_.surface);
    uint64_t surfaceId = surface->GetUniqueId();
    sInfo_.surface = surface;
    CombineConsumerUsage();
    int32_t ret = RegisterListenerToSurface(sInfo_.surface);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret,
                             "surface(%{public}" PRIu64 ") register listener to surface failed, GSError=%{public}d",
                             sInfo_.surface->GetUniqueId(), ret);
    AVCODEC_LOGI("Set surface(%{public}" PRIu64 ") success.", surfaceId);
    return AVCS_ERR_OK;
}

int32_t FCodec::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_INVALID_VAL, "Set callback failed: callback is NULL");
    callback_ = callback;
    return AVCS_ERR_OK;
}

// for memory recycle
bool FCodec::CanSwapOut(bool isOutputBuffer, std::shared_ptr<FBuffer> &fBuffer)
{
    if (!isOutputBuffer) {
        AVCODEC_LOGD("Current buffers unsupport.");
        return false;
    }
    std::shared_ptr<FSurfaceMemory> surfaceMemory = fBuffer->sMemory_;
    CHECK_AND_RETURN_RET_LOGD(surfaceMemory != nullptr, false, "Current buffer->sMemory error!");
    Owner ownerValue = surfaceMemory->owner;
    AVCODEC_LOGD("Buffer type: [%{public}u], fBuffer->owner_: [%{public}d], fBuffer->hasSwapedOut_: [%{public}d].",
                 isOutputBuffer, ownerValue, fBuffer->hasSwapedOut_);
    return !(ownerValue == Owner::OWNED_BY_SURFACE || fBuffer->hasSwapedOut_);
}

int32_t FCodec::SwapOutBuffers(bool isOutputBuffer, State curState)
{
    uint32_t bufferType = isOutputBuffer ? INDEX_OUTPUT : INDEX_INPUT;
    CHECK_AND_RETURN_RET_LOGD(bufferType == INDEX_OUTPUT, AVCS_ERR_OK, "Input buffers can't be swapped out!");
    for (uint32_t i = 0u; i < buffers_[bufferType].size(); i++) {
        std::shared_ptr<FBuffer> fBuffer = buffers_[bufferType][i];
        if (!CanSwapOut(isOutputBuffer, fBuffer)) {
            AVCODEC_LOGW("Buf: [%{public}u] can't freeze, owner: [%{public}d] swaped out: [%{public}d]!", i,
                         fBuffer->owner_.load(), fBuffer->hasSwapedOut_);
            continue;
        }
        std::shared_ptr<FSurfaceMemory> surfaceMemory = fBuffer->sMemory_;
        CHECK_AND_RETURN_RET_LOG(surfaceMemory != nullptr, AVCS_ERR_UNKNOWN, "Buf[%{public}u]->sMemory error!", i);
        sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
        CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_UNKNOWN, "Buf[%{public}u]->surfaceBuf error!", i);
        int32_t fd = surfaceBuffer->GetFileDescriptor();
        int32_t ret = DmaSwaper::GetInstance().SwapOutDma(pid_, fd);
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGE("Buffer type[%{public}u] bufferId[%{public}u], fd[%{public}d], pid[%{public}d] freeze failed!",
                         bufferType, i, fd, pid_);
            int32_t errCode = ActiveBuffers();
            state_ = curState;
            CHECK_AND_RETURN_RET_LOG(errCode == AVCS_ERR_OK, errCode, "Active buffers failed!");
            return ret;
        }
        AVCODEC_LOGI("Buf[%{public}u] fd[%{public}u] swap out success!", i, fd);
        fBuffer->hasSwapedOut_ = true;
    }
    return AVCS_ERR_OK;
}

int32_t FCodec::SwapInBuffers(bool isOutputBuffer)
{
    uint32_t bufferType = isOutputBuffer ? INDEX_OUTPUT : INDEX_INPUT;
    CHECK_AND_RETURN_RET_LOGD(bufferType == INDEX_OUTPUT, AVCS_ERR_OK, "Input buffers can't be swapped in!");
    for (uint32_t i = 0u; i < buffers_[bufferType].size(); i++) {
        std::shared_ptr<FBuffer> fBuffer = buffers_[bufferType][i];
        if (!fBuffer->hasSwapedOut_) {
            continue;
        }
        std::shared_ptr<FSurfaceMemory> surfaceMemory = fBuffer->sMemory_;
        CHECK_AND_CONTINUE_LOG(surfaceMemory != nullptr, "Buf[%{public}u]->sMemory error!", i);
        sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
        CHECK_AND_CONTINUE_LOG(surfaceBuffer != nullptr, "Buf[%{public}u]->surfaceBuf error!", i);
        int32_t fd = surfaceBuffer->GetFileDescriptor();
        int32_t ret = DmaSwaper::GetInstance().SwapInDma(pid_, fd);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Buf[%{public}u] fd[%{public}u] swap in error!", i, fd);
        AVCODEC_LOGI("Buf[%{public}u] fd[%{public}u] swap in success!", i, fd);
        fBuffer->hasSwapedOut_ = false;
    }
    return AVCS_ERR_OK;
}

int32_t FCodec::FreezeBuffers(State curState)
{
    CHECK_AND_RETURN_RET_LOGD(state_ != State::FROZEN, AVCS_ERR_OK, "FCodec had been frozen!");
    std::lock_guard<std::mutex> sLock(surfaceMutex_);
    int32_t ret = SwapOutBuffers(INDEX_INPUT, curState);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Input buffers swap out failed!");
    ret = SwapOutBuffers(INDEX_OUTPUT, curState);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Output buffers swap out failed!");
    return AVCS_ERR_OK;
}

int32_t FCodec::ActiveBuffers()
{
    CHECK_AND_RETURN_RET_LOGD(state_ == State::FREEZING || state_ == State::FROZEN, AVCS_ERR_INVALID_STATE,
                              "Only freezing or frozen state can swap in dma buffer!");
    int32_t ret = SwapInBuffers(INDEX_INPUT);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Input buffers swap in failed!");
    ret = SwapInBuffers(INDEX_OUTPUT);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Output buffers swap in failed!");
    return AVCS_ERR_OK;
}

int32_t FCodec::NotifyMemoryRecycle()
{
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AVCS_ERR_UNKNOWN, "Only surface mode support!");
    CHECK_AND_RETURN_RET_LOG(!disableDmaSwap_, 0, "FCodec dma swap has been diabled!");
    CHECK_AND_RETURN_RET_LOGD(state_ == State::RUNNING || state_ == State::FLUSHED || state_ == State::EOS,
                              AVCS_ERR_INVALID_STATE, "Current state can't recycle memory!");
    AVCODEC_LOGI("Begin to freeze this codec!");
    State currentState = state_;
    state_ = State::FREEZING;
    int32_t errCode = FreezeBuffers(currentState);
    CHECK_AND_RETURN_RET_LOG(errCode == AVCS_ERR_OK, errCode, "Fcodec freeze buffers failed!");
    state_ = State::FROZEN;
    return AVCS_ERR_OK;
}

int32_t FCodec::NotifyMemoryWriteBack()
{
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AVCS_ERR_UNKNOWN, "Only surface mode support!");
    AVCODEC_LOGI("Begin to active this codec!");
    int32_t errCode = ActiveBuffers();
    CHECK_AND_RETURN_RET_LOG(errCode == AVCS_ERR_OK, errCode, "Fcodec active buffers failed!");
    state_ = State::RUNNING;
    return AVCS_ERR_OK;
}

void FCodec::GetMpeg2CapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.profiles = {static_cast<int32_t>(MPEG2_PROFILE_422), static_cast<int32_t>(MPEG2_PROFILE_HIGH),
                            static_cast<int32_t>(MPEG2_PROFILE_MAIN), static_cast<int32_t>(MPEG2_PROFILE_SNR),
                            static_cast<int32_t>(MPEG2_PROFILE_SIMPLE), static_cast<int32_t>(MPEG2_PROFILE_SPATIAL)};
        std::vector<int32_t> levels_sp;
        std::vector<int32_t> levels_mp;
        std::vector<int32_t> levels_snr;
        std::vector<int32_t> levels_422p;
        for (int32_t j = 0; j <= static_cast<int32_t>(MPEG2Level::MPEG2_LEVEL_ML); ++j) {
            levels_sp.emplace_back(j);
        }
        for (int32_t j = 0; j <= static_cast<int32_t>(MPEG2Level::MPEG2_LEVEL_HL); ++j) {
            levels_mp.emplace_back(j);
        }
        for (int32_t j = 0; j <= static_cast<int32_t>(MPEG2Level::MPEG2_LEVEL_H14); ++j) {
            levels_snr.emplace_back(j);
        }
        for (int32_t j = static_cast<int32_t>(MPEG2Level::MPEG2_LEVEL_ML);
                j <= static_cast<int32_t>(MPEG2Level::MPEG2_LEVEL_HL); ++j) {
            levels_422p.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_SIMPLE), levels_sp));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_MAIN), levels_mp));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_SNR), levels_snr));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_SPATIAL), levels_mp));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_HIGH), levels_mp));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_422), levels_422p));
    }
}

void FCodec::SetMpeg4Profiles(CapabilityData& capsData)
{
    capsData.profiles = {
        static_cast<int32_t>(MPEG4_PROFILE_SIMPLE),
        static_cast<int32_t>(MPEG4_PROFILE_SIMPLE_SCALABLE),
        static_cast<int32_t>(MPEG4_PROFILE_CORE),
        static_cast<int32_t>(MPEG4_PROFILE_MAIN),
        static_cast<int32_t>(MPEG4_PROFILE_NBIT),
        static_cast<int32_t>(MPEG4_PROFILE_HYBRID),
        static_cast<int32_t>(MPEG4_PROFILE_BASIC_ANIMATED_TEXTURE),
        static_cast<int32_t>(MPEG4_PROFILE_SCALABLE_TEXTURE),
        static_cast<int32_t>(MPEG4_PROFILE_SIMPLE_FA),
        static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_REAL_TIME_SIMPLE),
        static_cast<int32_t>(MPEG4_PROFILE_CORE_SCALABLE),
        static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY),
        static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_CORE),
        static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_SCALABLE_TEXTURE),
        static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_SIMPLE),
    };
}

void FCodec::SetMpeg4LevelsProfileGroup1(CapabilityData& capsData)
{
    capsData.profileLevelsMap = {
        {static_cast<int32_t>(MPEG4_PROFILE_SIMPLE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_0), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_0B),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_4A),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_5), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_6)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_SIMPLE_SCALABLE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_0), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_CORE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_MAIN), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_4)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_NBIT), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_HYBRID), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_BASIC_ANIMATED_TEXTURE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_SCALABLE_TEXTURE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1)
        }}
    };
}

void FCodec::SetMpeg4LevelsProfileGroup2(CapabilityData& capsData)
{
    capsData.profileLevelsMap.insert({
        {static_cast<int32_t>(MPEG4_PROFILE_SIMPLE_FA), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_REAL_TIME_SIMPLE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_4)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_CORE_SCALABLE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_4)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_CORE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_SCALABLE_TEXTURE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_SIMPLE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_0), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3B), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_4),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_5)
        }}
    });
}

void FCodec::GetMpeg4esCapProf(std::vector<CapabilityData>& capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        SetMpeg4Profiles(capsData);
        SetMpeg4LevelsProfileGroup1(capsData);
        SetMpeg4LevelsProfileGroup2(capsData);
    }
}

void FCodec::GetH263CapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.profiles = {static_cast<int32_t>(H263_PROFILE_BASELINE),
                             static_cast<int32_t>(H263_PROFILE_VERSION_1_BACKWARD_COMPATIBILITY)};
        std::vector<int32_t> levels;
        for (int32_t j = 0; j <= static_cast<int32_t>(H263Level::H263_LEVEL_70); ++j) {
            levels.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(H263_PROFILE_BASELINE), levels));
        capsData.profileLevelsMap.insert(
            std::make_pair(static_cast<int32_t>(H263_PROFILE_VERSION_1_BACKWARD_COMPATIBILITY), levels));
    }
    return;
}

void FCodec::GetAvcCapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.profiles = {static_cast<int32_t>(AVC_PROFILE_BASELINE), static_cast<int32_t>(AVC_PROFILE_MAIN),
                                    static_cast<int32_t>(AVC_PROFILE_HIGH)};
        std::vector<int32_t> levels;
        for (int32_t j = 0; j <= static_cast<int32_t>(AVCLevel::AVC_LEVEL_62); ++j) {
            levels.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AVC_PROFILE_MAIN), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AVC_PROFILE_HIGH), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AVC_PROFILE_BASELINE), levels));
    }
}

int32_t FCodec::GetCodecCapability(std::vector<CapabilityData> &capaArray)
{
    for (uint32_t i = 0; i < SUPPORT_VCODEC_NUM; ++i) {
        CapabilityData capsData;
        capsData.codecName = static_cast<std::string>(SUPPORT_VCODEC[i].codecName);
        capsData.mimeType = static_cast<std::string>(SUPPORT_VCODEC[i].mimeType);
        capsData.codecType = AVCODEC_TYPE_VIDEO_DECODER;
        capsData.isVendor = false;
        capsData.maxInstance = VIDEO_INSTANCE_SIZE;
        capsData.alignment.width = VIDEO_ALIGNMENT_SIZE;
        capsData.alignment.height = VIDEO_ALIGNMENT_SIZE;
        capsData.width.minVal = capsData.mimeType == "video/h263" ? VIDEO_MIN_WIDTH_H263_SIZE : VIDEO_MIN_SIZE;
        capsData.height.minVal = capsData.mimeType == "video/h263" ? VIDEO_MIN_HEIGHT_H263_SIZE : VIDEO_MIN_SIZE;
        capsData.width.maxVal = capsData.mimeType == "video/h263" ? VIDEO_MAX_WIDTH_H263_SIZE : VIDEO_MAX_WIDTH_SIZE;
        capsData.height.maxVal = capsData.mimeType == "video/h263" ? VIDEO_MAX_HEIGHT_H263_SIZE : VIDEO_MAX_HEIGHT_SIZE;
        capsData.frameRate.minVal = 0;
        capsData.frameRate.maxVal = VIDEO_FRAMERATE_DEFAULT_SIZE;
        capsData.bitrate.minVal = 1;
        capsData.bitrate.maxVal = VIDEO_BITRATE_MAX_SIZE;
        capsData.blockPerFrame.minVal = 1;
        capsData.blockPerFrame.maxVal = VIDEO_BLOCKPERFRAME_SIZE;
        capsData.blockPerSecond.minVal = 1;
        capsData.blockPerSecond.maxVal = VIDEO_BLOCKPERSEC_SIZE;
        capsData.blockSize.width = VIDEO_ALIGN_SIZE;
        capsData.blockSize.height = VIDEO_ALIGN_SIZE;
        capsData.pixFormat = {
            static_cast<int32_t>(VideoPixelFormat::YUVI420), static_cast<int32_t>(VideoPixelFormat::NV12),
            static_cast<int32_t>(VideoPixelFormat::NV21), static_cast<int32_t>(VideoPixelFormat::RGBA)};
        if (capsData.mimeType == "video/mpeg2") {
            capaArray.emplace_back(capsData);
            GetMpeg2CapProf(capaArray);
        } else if (capsData.mimeType == "video/mp4v-es") {
            capaArray.emplace_back(capsData);
            GetMpeg4esCapProf(capaArray);
        } else if (capsData.mimeType == "video/h263") {
            capaArray.emplace_back(capsData);
            GetH263CapProf(capaArray);
        } else {
            capsData.frameRate.maxVal = VIDEO_FRAMERATE_MAX_SIZE;
            capaArray.emplace_back(capsData);
            GetAvcCapProf(capaArray);
        }
    }
    return AVCS_ERR_OK;
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
