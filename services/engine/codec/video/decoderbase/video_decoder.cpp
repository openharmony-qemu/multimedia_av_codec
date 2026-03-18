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

#include "syspara/parameters.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "avcodec_codec_name.h"
#include "video_decoder.h"
#include <sstream>
#include <sys/ioctl.h>
#include <linux/dma-buf.h>

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
namespace {
#define DMA_BUF_SET_LEAK_TYPE _IOW(DMA_BUF_BASE, 5, const char *)
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "VideoDecoder"};
constexpr int32_t DEFAULT_OUT_SURFACE_CNT = 4;
constexpr int32_t VIDEO_PLANE_COUNT_YUV = 3;
constexpr int32_t VIDEO_PLANE_SIZE_YUVP10 = 10;
constexpr int32_t VIDEO_MIN_BUFFER_SIZE = 1474560; // 1280*768*1.5
constexpr int32_t DEFAULT_MIN_BUFFER_CNT = 2;
constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;
constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
constexpr int32_t DEFAULT_OUT_BUFFER_CNT = 3;
constexpr int32_t DEFAULT_IN_BUFFER_CNT = 4;
constexpr int32_t DEFAULT_MAX_BUFFER_CNT = 10;
constexpr uint32_t LOG_LOW_FREQUENCY = 100;

#ifdef BUILD_ENG_VERSION
constexpr uint32_t PATH_MAX_LEN = 128;
constexpr char DUMP_PATH[] = "/data/misc/VideoDecoderdump";
#endif
} // namespace
using namespace OHOS::Media;
VideoDecoder::VideoDecoder(const std::string &name) : codecName_(name) {}

int32_t VideoDecoder::Init(Meta &callerInfo)
{
    if (callerInfo.GetData(Tag::AV_CODEC_FORWARD_CALLER_PID, decInfo_.pid) &&
        callerInfo.GetData(Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, decInfo_.processName)) {
        decInfo_.calledByAvcodec = false;
    } else if (callerInfo.GetData(Tag::AV_CODEC_CALLER_PID, decInfo_.pid) &&
               callerInfo.GetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, decInfo_.processName)) {
        decInfo_.calledByAvcodec = true;
    }
    callerInfo.GetData("av_codec_event_info_instance_id", instanceId_);
    int32_t ret = Initialize();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Failed to initialize");
    decInfo_.instanceId = std::to_string(instanceId_);
    AVCODEC_LOGI("Num %{public}u Decoder codec name: %{public}s", decInstanceID_, decName_.c_str());
    return AVCS_ERR_OK;
}

#ifdef BUILD_ENG_VERSION
void VideoDecoder::OpenDumpFile()
{
    std::string dumpModeStr = OHOS::system::GetParameter("decoder.dump", "0");
    AVCODEC_LOGI("dumpModeStr %{public}s", dumpModeStr.c_str());

    char fileName[PATH_MAX_LEN] = {0};
    if (dumpModeStr[0] == '1') {
        int ret = sprintf_s(fileName, sizeof(fileName), "%s/input_%p.dat", DUMP_PATH, this);
        if (ret > 0) {
            dumpInFile_ = std::make_shared<std::ofstream>();
            dumpInFile_->open(fileName, std::ios::out | std::ios::binary);
            if (!dumpInFile_->is_open()) {
                AVCODEC_LOGW("fail open file %{public}s", fileName);
                dumpInFile_ = nullptr;
            }
        }
    }
    if (dumpModeStr[1] == '1') {
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

void VideoDecoder::ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal,
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

void VideoDecoder::GetSurfaceCfgFromFmt(const Format &format)
{
    int32_t val = -1;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, val)) {
        if (val == static_cast<int32_t>(VideoPixelFormat::NV12) ||
            val == static_cast<int32_t>(VideoPixelFormat::NV21)) {
            std::lock_guard<std::mutex> runlock(decRunMutex_);
            outputPixelFmt_ = static_cast<VideoPixelFormat>(val);
            format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, val);
        } else {
            AVCODEC_LOGI("Invalid pixel_format: %{public}d.", val);
        }
    }
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, val)) {
        if (IsValidScaleType(val)) {
            format_.PutIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, val);
            AVCODEC_LOGI("Set parameter scale_type: %{public}d success.", val);
        } else {
            AVCODEC_LOGE("Invalid scale_type: %{public}d.", val);
        }
    }
    std::optional<int32_t> orientation = std::nullopt;
    if (format.GetIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, val)) {
        orientation = val;
        AVCODEC_LOGI("Set parameter video_orientation_type: %{public}d success.", orientation.value());
    }
    if (!orientation.has_value() && format.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val)) {
        if (IsValidRotation(val)) {
            orientation = static_cast<int32_t>(TranslateSurfaceRotation(static_cast<VideoRotation>(val)));
            AVCODEC_LOGI("Set parameter rotation_angle: %{public}d success.", orientation.value());
        } else {
            AVCODEC_LOGE("Invalid rotation_angle: %{public}d.", val);
        }
    }
    if (orientation) {
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, orientation.value());
    }
}

bool VideoDecoder::IsActive() const
{
    return state_ == State::RUNNING || state_ == State::FLUSHED || state_ == State::EOS;
}

int32_t VideoDecoder::Start()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Start codec failed: callback is null");
    CHECK_AND_RETURN_RET_LOG((state_ == State::CONFIGURED || state_ == State::FLUSHED), AVCS_ERR_INVALID_STATE,
                             "Start codec failed: not in Configured or Flushed state");

    int32_t ret = CreateDecoder();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "decoder create failed");
    if (!isBufferAllocated_) {
        for (int32_t i = 0; i < AV_NUM_DATA_POINTERS; i++) {
            std::lock_guard<std::mutex> convertLock(convertDataMutex_);
            scaleData_[i] = nullptr;
            scaleLineSize_[i] = 0;
        }
        isConverted_ = false;
        int32_t allocateResult = AllocateBuffers();
        CHECK_AND_RETURN_RET_LOG(allocateResult == AVCS_ERR_OK, allocateResult,
            "Start codec failed: cannot allocate buffers");
        isBufferAllocated_ = true;
    }
    InitBuffers();
    isSendEos_ = false;
    sendTask_->Start();
    state_ = State::RUNNING;
    AVCODEC_LOGI("Num %{public}u %{public}s Start codec successful, state: Running", decInstanceID_, decName_.c_str());
    return AVCS_ERR_OK;
}

void VideoDecoder::ConfigureHdrMetadata(const Format &format)
{
    (void)format;
    AVCODEC_LOGI("Decoder does not support HDR metadata configuration.");
}

int32_t VideoDecoder::Configure(const Format &format)
{
    AVCODEC_SYNC_TRACE;
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
            ConfigurelWidthAndHeight(format, it.first, true);
        } else if (it.first == MediaDescriptionKey::MD_KEY_HEIGHT) {
            ConfigurelWidthAndHeight(format, it.first, false);
        } else if (it.first == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT ||
                   it.first == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE ||
                   it.first == MediaDescriptionKey::MD_KEY_SCALE_TYPE ||
                   it.first == OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE) {
            continue;
        } else if (it.first == MediaDescriptionKey::MD_KEY_VIDEO_HDR_METADATA) {
            ConfigureHdrMetadata(format);
        } else {
            AVCODEC_LOGW("Set parameter failed: size:%{public}s, unsupport key", it.first.data());
        }
    }
    GetSurfaceCfgFromFmt(format);
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width_);
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height_);

    state_ = State::CONFIGURED;

    return AVCS_ERR_OK;
}

void VideoDecoder::ConfigureSurface(const Format &format, const std::string_view &formatKey, FormatDataType formatType)
{
    CHECK_AND_RETURN_LOG(formatType == FORMAT_TYPE_INT32, "Set parameter failed: type should be int32");

    int32_t val = 0;
    CHECK_AND_RETURN_LOG(format.GetIntValue(formatKey, val), "Set parameter failed: get value fail");

    if (formatKey == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT) {
        VideoPixelFormat vpf = static_cast<VideoPixelFormat>(val);
        if (!CheckVideoPixelFormat(vpf)) {
            return;
        }
        std::lock_guard<std::mutex> runlock(decRunMutex_);
        outputPixelFmt_ = vpf;
        format_.PutIntValue(formatKey, val);
        GraphicPixelFormat surfacePixelFmt = TranslateSurfaceFormat(outputPixelFmt_);
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT, static_cast<int32_t>(surfacePixelFmt));
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

void VideoDecoder::InitBuffers()
{
    inputAvailQue_->SetActive(true);
    codecAvailQue_->SetActive(true);
    if (sInfo_.surface != nullptr) {
        renderAvailQue_->SetActive(true);
        requestSurfaceBufferQue_->SetActive(true);
    }
    CHECK_AND_RETURN_LOG(buffers_[INDEX_INPUT].size() > 0, "Input buffers is null!");
    if (buffers_[INDEX_INPUT].size() > 0) {
        for (uint32_t i = 0; i < buffers_[INDEX_INPUT].size(); i++) {
            buffers_[INDEX_INPUT][i]->owner_ = Owner::OWNED_BY_USER;
            callback_->OnInputBufferAvailable(i, buffers_[INDEX_INPUT][i]->avBuffer);
            AVCODEC_LOGI("%{public}s OnInputBufferAvailable frame index = %{public}u, owner = %{public}d",
                         decName_.c_str(), i, buffers_[INDEX_INPUT][i]->owner_.load());
        }
    }
    CHECK_AND_RETURN_LOG(buffers_[INDEX_OUTPUT].size() > 0, "Output buffers is null!");
    InitParams();
    if (sInfo_.surface == nullptr || state_ == State::CONFIGURED) {
        for (uint32_t i = 0u; i < buffers_[INDEX_OUTPUT].size(); i++) {
            buffers_[INDEX_OUTPUT][i]->owner_ = Owner::OWNED_BY_CODEC;
            codecAvailQue_->Push(i);
        }
        return;
    }
    std::lock_guard<std::mutex> sLock(surfaceMutex_);
    for (uint32_t i = 0u; i < buffers_[INDEX_OUTPUT].size(); i++) {
        if (buffers_[INDEX_OUTPUT][i]->owner_ != Owner::OWNED_BY_SURFACE) {
            buffers_[INDEX_OUTPUT][i]->owner_ = Owner::OWNED_BY_CODEC;
            codecAvailQue_->Push(i);
        }
    }
}

void VideoDecoder::ResetData()
{
    std::lock_guard<std::mutex> convertLock(convertDataMutex_);
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

void VideoDecoder::ResetBuffers()
{
    inputAvailQue_->Clear();
    codecAvailQue_->Clear();
    ResetData();
}

void VideoDecoder::StopThread()
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
    if (sInfo_.surface != nullptr) {
        if (renderAvailQue_ != nullptr) {
            renderAvailQue_->SetActive(false, false);
        }
        if (requestSurfaceBufferQue_ != nullptr) {
            requestSurfaceBufferQue_->SetActive(false, false);
        }
    }
}

int32_t VideoDecoder::Stop()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG((IsActive()), AVCS_ERR_INVALID_STATE, "Stop codec failed: not in executing state");
    state_ = State::STOPPING;
    inputAvailQue_->SetActive(false, false);
    codecAvailQue_->SetActive(false, false);
    sendTask_->Stop();

    if (sInfo_.surface != nullptr) {
        renderAvailQue_->SetActive(false, false);
        requestSurfaceBufferQue_->SetActive(false, false);
    }
    DeleteDecoder();
    ReleaseBuffers();
    state_ = State::CONFIGURED;
    AVCODEC_LOGI("Num %{public}u Stop codec successful, state: Configured", decInstanceID_);
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::Flush()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG((state_ == State::RUNNING || state_ == State::EOS), AVCS_ERR_INVALID_STATE,
                             "%{public}s Flush codec failed: not in running or Eos state", decName_.c_str());
    state_ = State::FLUSHING;
    AVCODEC_LOGI("%{public}s step into FLUSHING status", decName_.c_str());
    inputAvailQue_->SetActive(false, false);
    codecAvailQue_->SetActive(false, false);
    sendTask_->Pause();

    ResetBuffers();
    FlushAllFrames();
    state_ = State::FLUSHED;
    AVCODEC_LOGI("Num %{public}u %{public}s Flush codec successful, state: Flushed", decInstanceID_, decName_.c_str());
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::Reset()
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGI("Num %{public}u Reset codec called", decInstanceID_);
    int32_t ret = Release();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Reset codec failed: cannot release codec");
    ret = Initialize();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Reset codec failed: cannot init codec");
    AVCODEC_LOGI("Reset codec successful, state: Initialized");
    return AVCS_ERR_OK;
}

void VideoDecoder::ReleaseResource()
{
    StopThread();
    ReleaseBuffers();
    format_ = Format();
    if (sInfo_.surface != nullptr) {
        transform_.store(GraphicTransformType::GRAPHIC_ROTATE_NONE);
        sInfo_.surface->SetTransform(transform_.load());
        UnRegisterListenerToSurface(sInfo_.surface);
        StopRequestSurfaceBufferThread();
    }
    sInfo_.surface = nullptr;
    DeleteDecoder();
}

int32_t VideoDecoder::Release()
{
    AVCODEC_SYNC_TRACE;
    state_ = State::STOPPING;
    ReleaseResource();
    state_ = State::UNINITIALIZED;
    AVCODEC_LOGI("Num %{public}u Release codec successful, state: Uninitialized", decInstanceID_);
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::SetParameter(const Format &format)
{
    AVCODEC_SYNC_TRACE;
    if (sInfo_.surface != nullptr) {
        GetSurfaceCfgFromFmt(format);
        SetSurfaceParameter();
    }
    AVCODEC_LOGI("Set parameter successful");
    return AVCS_ERR_OK;
}

void VideoDecoder::CalculateBufferSize()
{
    if (codecName_ == AVCodecCodecName::VIDEO_DECODER_VP8_NAME) {
        inputBufferSize_ = static_cast<UINT32>(width_ * height_ * VIDEO_PLANE_COUNT_YUV) >> 1;
    } else if (codecName_ == AVCodecCodecName::VIDEO_DECODER_VP9_NAME ||
               codecName_ == AVCodecCodecName::VIDEO_DECODER_AV1_NAME) {
        inputBufferSize_ =
            static_cast<UINT32>(width_ * height_ * VIDEO_PLANE_COUNT_YUV * VIDEO_PLANE_SIZE_YUVP10) >> 3; // 3: 8bit
    }
    if (inputBufferSize_ <= VIDEO_MIN_BUFFER_SIZE) {
        inputBufferSize_ = VIDEO_MIN_BUFFER_SIZE;
    }
    AVCODEC_LOGI("width = %{public}d, height = %{public}d, Input buffer size = %{public}d",
                 width_, height_, inputBufferSize_);
}

int32_t VideoDecoder::AllocateInputBuffer(int32_t bufferCnt, int32_t inBufferSize)
{
    int32_t valBufferCnt = 0;
    for (int32_t i = 0; i < bufferCnt; i++) {
        std::shared_ptr<CodecBuffer> buf = std::make_shared<CodecBuffer>();
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

int32_t VideoDecoder::AllocateOutputBuffer(int32_t bufferCnt)
{
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface == nullptr, AVCS_ERR_UNKNOWN, "Not in buffer mode!");
    int32_t ret = SetSurfaceCfg();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Set surface cfg failed!");
    int32_t valBufferCnt = 0;
    for (int i = 0; i < bufferCnt; i++) {
        std::shared_ptr<CodecBuffer> buf = std::make_shared<CodecBuffer>();
        buf->width = width_;
        buf->height = height_;
        std::shared_ptr<AVAllocator> allocator =
            AVAllocatorFactory::CreateSurfaceAllocator(sInfo_.requestConfig);
        CHECK_AND_CONTINUE_LOG(allocator != nullptr, "output buffer %{public}d allocator is nullptr", i);
        buf->avBuffer = AVBuffer::CreateAVBuffer(allocator, 0);
        if (buf->avBuffer != nullptr) {
            AVCODEC_LOGI("Allocate output share buffer success: index=%{public}d, size=%{public}d", i,
                         buf->avBuffer->memory_->GetCapacity());
        }
        CHECK_AND_CONTINUE_LOG(buf->avBuffer != nullptr, "Allocate output buffer failed, index=%{public}d", i);
        SetCallerToBuffer(buf->avBuffer->memory_->GetSurfaceBuffer());
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

int32_t VideoDecoder::AllocateOutputBuffersFromSurface(int32_t bufferCnt)
{
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AVCS_ERR_UNKNOWN, "Not in surface mode!");
    int32_t ret = ClearSurfaceAndSetQueueSize(sInfo_.surface, bufferCnt);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Clean surface and set queue size failed!");
    requestSurfaceBufferQue_->Clear();
    requestSurfaceBufferQue_->SetActive(true);
    StartRequestSurfaceBufferThread();
    for (int32_t i = 0; i < bufferCnt; i++) {
        std::shared_ptr<FSurfaceMemory> surfaceMemory =
            std::make_shared<FSurfaceMemory>(&sInfo_, decInfo_);
        CHECK_AND_RETURN_RET_LOG(surfaceMemory != nullptr, AVCS_ERR_UNKNOWN, "Creata surface memory failed!");
        ret = surfaceMemory->AllocSurfaceBuffer(width_, height_);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Alloc surface buffer failed!");
        sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
        CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_UNKNOWN, "surface buf(%{public}u) is null.", i);
        ret = Attach(surfaceBuffer);
        CHECK_AND_CONTINUE_LOG(ret == AVCS_ERR_OK, "surface buf(%{public}u) attach to surface failed.", i);
        surfaceMemory->isAttached = true;
        std::shared_ptr<CodecBuffer> buf = std::make_shared<CodecBuffer>();
        CHECK_AND_RETURN_RET_LOG(buf != nullptr, AVCS_ERR_UNKNOWN, "Creata output buffer failed!");
        buf->sMemory = surfaceMemory;
        buf->height = height_;
        buf->width = width_;
        outAVBuffer4Surface_.emplace_back(AVBuffer::CreateAVBuffer());
        buf->avBuffer = AVBuffer::CreateAVBuffer(buf->sMemory->GetBase(), buf->sMemory->GetSize());
        AVCODEC_LOGI("Allocate output surface buffer success, index=%{public}d, size=%{public}d, stride=%{public}d", i,
                     buf->sMemory->GetSize(), buf->sMemory->GetSurfaceBufferStride());
        buffers_[INDEX_OUTPUT].emplace_back(buf);
    }
    int32_t outputBufferNum = static_cast<int32_t>(buffers_[INDEX_OUTPUT].size());
    CHECK_AND_RETURN_RET_LOG(outputBufferNum == bufferCnt, AVCS_ERR_UNKNOWN,
                             "Only alloc %{public}d buffers, less %{public}d", outputBufferNum, bufferCnt);
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::SetOutputSurface(sptr<Surface> surface)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(state_ != State::UNINITIALIZED, AV_ERR_INVALID_VAL,
                             "set output surface fail: not initialized or configured");
    CHECK_AND_RETURN_RET_LOG((state_ == State::CONFIGURED || state_ == State::FLUSHED ||
        state_ == State::RUNNING || state_ == State::EOS), AVCS_ERR_INVALID_STATE,
        "set output surface fail: state_ %{public}d not support set output surface",
        static_cast<int32_t>(state_.load()));
    if (surface == nullptr || surface->IsConsumer()) {
        AVCODEC_LOGE("Set surface fail");
        return AVCS_ERR_INVALID_VAL;
    }
    if (state_ == State::FLUSHED || state_ == State::RUNNING ||
        state_ == State::EOS) {
        return ReplaceOutputSurfaceWhenRunning(surface);
    }
    UnRegisterListenerToSurface(sInfo_.surface);
    uint64_t surfaceId = surface->GetUniqueId();
    sInfo_.surface = surface;
    CombineConsumerUsage();
    int32_t ret = RegisterListenerToSurface(sInfo_.surface);
    CHECK_AND_RETURN_RET_LOG(ret == GSERROR_OK, ret,
                             "surface(%{public}" PRIu64 ") register listener to surface failed, GSError=%{public}d",
                             sInfo_.surface->GetUniqueId(), ret);
    AVCODEC_LOGI("Set surface(%{public}" PRIu64 ") success.", surfaceId);
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::RenderOutputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AVCS_ERR_UNSUPPORT,
                             "Render output buffer failed, surface is nullptr!");
    AVCODEC_LOGI_LIMIT(LOG_LOW_FREQUENCY, "Num %{public}u render output buffer with index=%{public}u",
        decInstanceID_, index);
    std::unique_lock<std::mutex> oLock(outputMutex_);
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_OUTPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Failed to render output buffer: invalid index");
    std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
    if (frameBuffer->owner_ == Owner::OWNED_BY_USER) {
        frameBuffer->owner_ = Owner::OWNED_BY_SURFACE;
        oLock.unlock();
        std::shared_ptr<FSurfaceMemory> surfaceMemory = frameBuffer->sMemory;
        int32_t ret = FlushSurfaceMemory(surfaceMemory, index);
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGW("Flush surface memory(index=%{public}u) failed: %{public}d", index, ret);
        } else {
            AVCODEC_LOGD("Flush surface memory(index=%{public}u) successful.", index);
        }
        AVCODEC_LOGD("render output buffer with index, index=%{public}u", index);
        return AVCS_ERR_OK;
    } else {
        AVCODEC_LOGE("Failed to render output buffer with bad index, index=%{public}u, owner=%{public}d",
            index, frameBuffer->owner_.load());
        return AVCS_ERR_INVALID_VAL;
    }
}

int32_t VideoDecoder::GetOutputFormat(Format &format)
{
    AVCODEC_SYNC_TRACE;
    if (!format_.ContainKey(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE)) {
        int32_t maxInputSize = static_cast<int32_t>(static_cast<UINT32>(
            GetDecoderWidthStride() * height_ * VIDEO_PLANE_COUNT_YUV) >> 1);
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

void VideoDecoder::SetSurfaceParameter()
{
    int32_t val = -1;
    bool setSurfacePixelFormat = false;
    std::optional<ScalingMode> scaling = std::nullopt;
    std::optional<GraphicTransformType> orientation = std::nullopt;
    if (format_.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, val)) {
        setSurfacePixelFormat = true;
    }
    if (format_.GetIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, val)) {
        scaling = static_cast<ScalingMode>(val);
    }
    if (format_.GetIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, val)) {
        orientation = static_cast<GraphicTransformType>(val);
    }
    std::lock_guard<std::mutex> sLock(surfaceMutex_);
    if (setSurfacePixelFormat) {
        CHECK_AND_RETURN_LOG(SetSurfaceFormat() == AVCS_ERR_OK,
                             "set surface format failed: unsupported surface format");
    }
    if (scaling) {
        sInfo_.scalingMode = scaling.value();
        sInfo_.surface->SetScalingMode(sInfo_.scalingMode.value());
    }
    if (orientation) {
        transform_.store(orientation.value());
        AVCODEC_LOGI("Set surface video_orientation_type: %{public}d success.",
                     static_cast<int32_t>(transform_.load()));
        sInfo_.surface->SetTransform(transform_.load());
    }
}

int32_t VideoDecoder::SetSurfaceFormat()
{
    if (bitDepth_ == BITS_PER_PIXEL_COMPONENT_10) {
        if (outputPixelFmt_ == VideoPixelFormat::NV12 || outputPixelFmt_ == VideoPixelFormat::UNKNOWN) {
            sInfo_.requestConfig.format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010;
        } else {
            sInfo_.requestConfig.format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010;
        }
        format_.PutIntValue("av_codec_event_info_bit_depth", 1);
    } else {
        VideoPixelFormat targetPixelFmt = outputPixelFmt_;
        if (outputPixelFmt_ == VideoPixelFormat::UNKNOWN) {
            targetPixelFmt = VideoPixelFormat::NV12;
        }
        GraphicPixelFormat surfacePixelFmt = TranslateSurfaceFormat(targetPixelFmt);
        CHECK_AND_RETURN_RET_LOG(surfacePixelFmt != GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT, AVCS_ERR_UNSUPPORT,
                                 "Failed to allocate output buffer: unsupported surface format");
        format_.PutIntValue(OHOS::Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT, static_cast<int32_t>(surfacePixelFmt));
        sInfo_.requestConfig.format = surfacePixelFmt;
    }
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::CheckFormatChange(uint32_t index, int width, int height, int bitDepth)
{
    bool formatChanged = false;
    if (width_ != width || height_ != height || bitDepth_ != bitDepth) {
        AVCODEC_LOGI("format change, width: %{public}d->%{public}d, height: %{public}d->%{public}d, "
                     "bitDepth: %{public}d->%{public}d", width_, width, height_, height, bitDepth_, bitDepth);
        width_ = width;
        height_ = height;
        bitDepth_ = bitDepth;
        ResetData();
        std::unique_lock<std::mutex> convertLock(convertDataMutex_);
        scale_ = nullptr;
        convertLock.unlock();
        std::unique_lock<std::mutex> sLock(surfaceMutex_);
        sInfo_.requestConfig.width = width_;
        sInfo_.requestConfig.height = height_;
        CHECK_AND_RETURN_RET_LOG(SetSurfaceFormat() == AVCS_ERR_OK, AVCS_ERR_UNSUPPORT,
                                 "set surface format failed: unsupported surface format");
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

int32_t VideoDecoder::AllocateBuffers()
{
    AVCODEC_SYNC_TRACE;
    CalculateBufferSize();
    CHECK_AND_RETURN_RET_LOG(inputBufferSize_ > 0, AVCS_ERR_INVALID_VAL,
                             "Allocate buffer with input size=%{public}d failed", inputBufferSize_);
    if (sInfo_.surface != nullptr && isOutBufSetted_ == false) {
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, DEFAULT_OUT_SURFACE_CNT);
    }
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, inputBufferCnt_);
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, outputBufferCnt_);
    inputAvailQue_ = std::make_shared<BlockQueue<uint32_t>>("inputAvailQue", inputBufferCnt_);
    codecAvailQue_ = std::make_shared<BlockQueue<uint32_t>>("codecAvailQue", outputBufferCnt_);
    if (sInfo_.surface != nullptr) {
        renderAvailQue_ = std::make_shared<BlockQueue<uint32_t>>("renderAvailQue", outputBufferCnt_);
        requestSurfaceBufferQue_ = std::make_shared<BlockQueue<uint32_t>>("requestSurfaceBufferQue", outputBufferCnt_);
    }
    int32_t ret = AllocateInputBuffer(inputBufferCnt_, inputBufferSize_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Allocate input buffers failed!");
    ret = sInfo_.surface ? AllocateOutputBuffersFromSurface(outputBufferCnt_) :
        AllocateOutputBuffer(outputBufferCnt_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Allocate output buffers failed!");
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::UpdateOutputBuffer(uint32_t index)
{
    std::shared_ptr<CodecBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
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
        SetCallerToBuffer(outputBuffer->avBuffer->memory_->GetSurfaceBuffer());

        outputBuffer->owner_ = Owner::OWNED_BY_CODEC;
        outputBuffer->width = width_;
        outputBuffer->height = height_;
        outputBuffer->bitDepth = bitDepth_;
    }
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::UpdateSurfaceMemory(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    std::unique_lock<std::mutex> oLock(outputMutex_);
    std::shared_ptr<CodecBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
    oLock.unlock();
    if (width_ != outputBuffer->width || height_ != outputBuffer->height || bitDepth_ != outputBuffer->bitDepth) {
        std::shared_ptr<FSurfaceMemory> surfaceMemory = outputBuffer->sMemory;
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
        int32_t ret = surfaceMemory->AllocSurfaceBuffer(width_, height_);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Alloc surface buffer failed!");
        sptr<SurfaceBuffer> newSurfaceBuffer = surfaceMemory->GetSurfaceBuffer();
        CHECK_AND_RETURN_RET_LOG(newSurfaceBuffer != nullptr, AVCS_ERR_UNKNOWN, "Alloc surface buffer failed!");
        ret = Attach(newSurfaceBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Surface buffer attach failed!");
        surfaceMemory->isAttached = true;
        outputBuffer->avBuffer =
            AVBuffer::CreateAVBuffer(outputBuffer->sMemory->GetBase(), outputBuffer->sMemory->GetSize());
        outputBuffer->width = width_;
        outputBuffer->height = height_;
        outputBuffer->bitDepth = bitDepth_;
    }
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::GetSurfaceBufferStride(const std::shared_ptr<CodecBuffer> &frameBuffer)
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

void VideoDecoder::ReleaseBuffers()
{
    ResetData();
    if (!isBufferAllocated_) {
        return;
    }

    inputAvailQue_->Clear();
    buffers_[INDEX_INPUT].clear();
    inputBufferCnt_ = 0;

    std::unique_lock<std::mutex> oLock(outputMutex_);
    codecAvailQue_->Clear();
    if (sInfo_.surface != nullptr) {
        {
            std::lock_guard<std::mutex> sLock(surfaceMutex_);
            StopRequestSurfaceBufferThread();
            sInfo_.surface->CleanCache();
        }
        renderAvailQue_->Clear();
        requestSurfaceBufferQue_->Clear();
        {
            std::lock_guard<std::mutex> mLock(renderBufferMapMutex_);
            renderSurfaceBufferMap_.clear();
        }
        for (uint32_t i = 0; i < buffers_[INDEX_OUTPUT].size(); i++) {
            std::shared_ptr<CodecBuffer> outputBuffer = buffers_[INDEX_OUTPUT][i];
            if (outputBuffer->owner_ == Owner::OWNED_BY_CODEC) {
                std::shared_ptr<FSurfaceMemory> surfaceMemory = outputBuffer->sMemory;
                surfaceMemory->ReleaseSurfaceBuffer();
                outputBuffer->owner_ = Owner::OWNED_BY_SURFACE;
            }
        }
        AVCODEC_LOGI("Num %{public}u surface cleancache success", decInstanceID_);
    }
    buffers_[INDEX_OUTPUT].clear();
    outputBufferCnt_ = 0;
    outAVBuffer4Surface_.clear();
    oLock.unlock();
    isBufferAllocated_ = false;
}

int32_t VideoDecoder::QueueInputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, AVCS_ERR_INVALID_STATE,
                             "Queue input buffer failed: not in Running state");
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_INPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Queue input buffer failed with bad index, index=%{public}u, buffer_size=%{public}zu",
                             index, buffers_[INDEX_INPUT].size());
    std::shared_ptr<CodecBuffer> inputBuffer = buffers_[INDEX_INPUT][index];
    CHECK_AND_RETURN_RET_LOG(inputBuffer->owner_ == Owner::OWNED_BY_USER, AVCS_ERR_INVALID_OPERATION,
                             "Queue input buffer failed: buffer with index=%{public}u is not available", index);

    inputBuffer->owner_ = Owner::OWNED_BY_CODEC;
    inputAvailQue_->Push(index);
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::FillFrameBuffer(const std::shared_ptr<CodecBuffer> &frameBuffer)
{
    VideoPixelFormat targetPixelFmt = outputPixelFmt_;
    AVPixelFormat ffmpegFormat;
    if (bitDepth_ == BITS_PER_PIXEL_COMPONENT_10) {
        ffmpegFormat = AVPixelFormat::AV_PIX_FMT_P010LE;
    } else {
        ffmpegFormat = ConvertPixelFormatToFFmpeg(targetPixelFmt);
    }
    // yuv420 -> nv12 or nv21
    std::lock_guard<std::mutex> convertLock(convertDataMutex_);
    int32_t ret = ConvertVideoFrame(&scale_, cachedFrame_, scaleData_, scaleLineSize_, ffmpegFormat);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Scale video frame failed: %{public}d", ret);
    isConverted_ = true;

    std::shared_ptr<AVMemory> &bufferMemory = frameBuffer->avBuffer->memory_;
    CHECK_AND_RETURN_RET_LOG(bufferMemory != nullptr, AVCS_ERR_INVALID_VAL, "bufferMemory is nullptr");
    sptr<SurfaceBuffer> surfaceBuffer = sInfo_.surface ? frameBuffer->sMemory->GetSurfaceBuffer() :
        frameBuffer->avBuffer->memory_->GetSurfaceBuffer();
    bufferMemory->SetSize(0);
    struct SurfaceInfo surfaceInfo;
    surfaceInfo.scaleData = scaleData_;
    surfaceInfo.scaleLineSize = scaleLineSize_;
    int32_t surfaceStride = GetSurfaceBufferStride(frameBuffer);
    CHECK_AND_RETURN_RET_LOG(surfaceStride > 0, AVCS_ERR_INVALID_VAL, "get GetSurfaceBufferStride failed");
    surfaceInfo.surfaceStride = static_cast<uint32_t>(surfaceStride);
    Format bufferFormat;
    bufferFormat.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, cachedFrame_->height);
    bufferFormat.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, surfaceStride);
    bufferFormat.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(targetPixelFmt));
    if (sInfo_.surface) {
        surfaceInfo.surfaceFence = frameBuffer->sMemory->GetFence();
        ret = WriteSurfaceData(bufferMemory, surfaceInfo, bufferFormat);
    } else {
        ret = WriteBufferData(bufferMemory, scaleData_, scaleLineSize_, bufferFormat);
    }
    FillHdrInfo(surfaceBuffer);
#ifdef BUILD_ENG_VERSION
    DumpConvertOut(surfaceInfo);
#endif
    frameBuffer->avBuffer->pts_ = cachedFrame_->pts;
    AVCODEC_LOGD("Fill frame buffer successful");
    return ret;
}

void VideoDecoder::FramePostProcess(const std::shared_ptr<CodecBuffer> &frameBuffer, uint32_t index, int32_t status)
{
    if (status == AVCS_ERR_OK) {
        codecAvailQue_->Pop();
        frameBuffer->owner_ = Owner::OWNED_BY_USER;
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

#ifdef BUILD_ENG_VERSION
void VideoDecoder::DumpOutputBuffer(int32_t bitDepth)
{
    std::lock_guard<std::mutex> convertLock(convertDataMutex_);
    if (!dumpOutFile_ || !dumpOutFile_->is_open()) {
        return;
    }
    int32_t pixelBytes = 1;
    if (bitDepth == BITS_PER_PIXEL_COMPONENT_10) {
        pixelBytes = 2; // 2 bytes for 10bit
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

void VideoDecoder::DumpConvertOut(struct SurfaceInfo &surfaceInfo)
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

int32_t VideoDecoder::ReleaseOutputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGI_LIMIT(LOG_LOW_FREQUENCY, "Num %{public}u release output buffer with index=%{public}u",
        decInstanceID_, index);
    std::unique_lock<std::mutex> oLock(outputMutex_);
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_OUTPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Failed to release output buffer: invalid index");
    std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
    if (frameBuffer->owner_ == Owner::OWNED_BY_USER) {
        frameBuffer->owner_ = Owner::OWNED_BY_CODEC;
        oLock.unlock();
        codecAvailQue_->Push(index);
        return AVCS_ERR_OK;
    } else {
        AVCODEC_LOGE("Release output buffer failed: check your index=%{public}u, owner=%{public}d",
            index, frameBuffer->owner_.load());
        return AVCS_ERR_INVALID_VAL;
    }
}

int32_t VideoDecoder::Detach(sptr<SurfaceBuffer> surfaceBuffer)
{
    int32_t err = sInfo_.surface->DetachBufferFromQueue(surfaceBuffer);
    CHECK_AND_RETURN_RET_LOG(
        err == 0, err, "Surface(%{public}" PRIu64 "), detach buffer(%{public}u) to queue failed, GSError=%{public}d",
        sInfo_.surface->GetUniqueId(), surfaceBuffer->GetSeqNum(), err);
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_INVALID_VAL, "Set callback failed: callback is NULL");
    callback_ = callback;
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::NotifyMemoryRecycle()
{
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AVCS_ERR_UNKNOWN, "Only surface mode support!");
    CHECK_AND_RETURN_RET_LOGD(state_ == State::RUNNING || state_ == State::FLUSHED || state_ == State::EOS,
                              AVCS_ERR_INVALID_STATE, "Current state can't recycle memory!");
    AVCODEC_LOGI("Begin to freeze this codec");
    State currentState = state_;
    state_ = State::FREEZING;
    int32_t errCode = FreezeBuffers(currentState);
    CHECK_AND_RETURN_RET_LOG(errCode == AVCS_ERR_OK, errCode, "Codec freeze buffers failed!");
    state_ = State::FROZEN;
    return AVCS_ERR_OK;
}

int32_t VideoDecoder::NotifyMemoryWriteBack()
{
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AVCS_ERR_UNKNOWN, "Only surface mode support!");
    AVCODEC_LOGI("Begin to active this codec");
    int32_t errCode = ActiveBuffers();
    CHECK_AND_RETURN_RET_LOG(errCode == AVCS_ERR_OK, errCode, "Codec active buffers failed!");
    state_ = State::RUNNING;
    return AVCS_ERR_OK;
}

void VideoDecoder::SetCallerToBuffer(sptr<SurfaceBuffer> surfaceBuffer)
{
    CHECK_AND_RETURN_LOG(surfaceBuffer != nullptr, "Surface buffer is nullptr!");
    int32_t fd = surfaceBuffer->GetFileDescriptor();
    CHECK_AND_RETURN_LOG(fd > 0, "Invalid fd %{public}d, surfacebuf(%{public}u)", fd, surfaceBuffer->GetSeqNum());
    std::string type = "sw-video-decoder";
    std::string mime(decInfo_.mimeType);
    std::vector<std::string> splitMime;
    std::string token;
    std::istringstream iss(mime);
    while (std::getline(iss, token, '/')) {
        splitMime.push_back(token);
    }
    if (!splitMime.empty()) {
        mime = splitMime.back();
    }
    std::string name =
        std::to_string(width_) + "x" + std::to_string(height_) + "-" + mime + "-" + decInfo_.instanceId;
    ioctl(fd, DMA_BUF_SET_LEAK_TYPE, type.c_str());
    std::string pid = std::to_string(decInfo_.pid);
    ioctl(fd, DMA_BUF_SET_NAME_A, pid.c_str());
    ioctl(fd, DMA_BUF_SET_NAME_A, name.c_str());
}

std::mutex VideoDecoder::decoderCountMutex_;
std::vector<uint32_t> VideoDecoder::decInstanceIDSet_;
std::vector<uint32_t> VideoDecoder::freeIDSet_;

} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
