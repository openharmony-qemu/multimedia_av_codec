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

#include <iostream>
#include <set>
#include <thread>
#include <malloc.h>
#include "securec.h"
#include "avcodec_dfx.h"
#include "avcodec_log.h"
#include "utils.h"
#include "avcodec_codec_name.h"
#include "fcodec.h"
namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "FCodec"};
constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;
constexpr int32_t DEFAULT_IN_BUFFER_CNT = 4;
constexpr int32_t DEFAULT_OUT_SURFACE_CNT = 4;
constexpr int32_t DEFAULT_OUT_BUFFER_CNT = 3;
constexpr int32_t DEFAULT_MIN_BUFFER_CNT = 2;
constexpr uint32_t VIDEO_PIX_DEPTH_YUV = 3;
constexpr uint32_t VIDEO_PIX_DEPTH_RGBA = 4;
constexpr int32_t VIDEO_MIN_SIZE = 96;
constexpr int32_t VIDEO_ALIGNMENT_SIZE = 2;
constexpr int32_t VIDEO_MAX_WIDTH_SIZE = 4096;
constexpr int32_t VIDEO_MAX_HEIGHT_SIZE = 2304;
constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
constexpr uint32_t DEFAULT_TRY_DECODE_TIME = 10;
constexpr uint32_t DEFAULT_DECODE_WAIT_TIME = 33;
constexpr int32_t VIDEO_INSTANCE_SIZE = 16;
constexpr int32_t VIDEO_BITRATE_MAX_SIZE = 300000000;
constexpr int32_t VIDEO_FRAMERATE_MAX_SIZE = 120;
constexpr int32_t VIDEO_BLOCKPERFRAME_SIZE = 36864;
constexpr int32_t VIDEO_BLOCKPERSEC_SIZE = 983040;
constexpr int32_t DEFAULT_THREAD_COUNT = 2;
constexpr struct {
    const std::string_view codecName;
    const std::string_view mimeType;
    const char *ffmpegCodec;
    const bool isEncoder;
} SUPPORT_VCODEC[] = {
    {AVCodecCodecName::VIDEO_DECODER_AVC_NAME, CodecMimeType::VIDEO_AVC, "h264", false},
};
constexpr uint32_t SUPPORT_VCODEC_NUM = sizeof(SUPPORT_VCODEC) / sizeof(SUPPORT_VCODEC[0]);
} // namespace

FCodec::FCodec(const std::string &name) : codecName_(name), state_(State::Uninitialized)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("Fcodec entered, state: Uninitialized");
}

FCodec::~FCodec()
{
    if (sendTask_ != nullptr) {
        sendTask_->Stop();
    }
    if (surface_ != nullptr && renderTask_ != nullptr) {
        renderTask_->Stop();
    }
    if (receiveTask_ != nullptr) {
        receiveTask_->Stop();
    }
    if (avCodecContext_ != nullptr) {
        avcodec_close(avCodecContext_.get());
        ResetContext();
    }
    ReleaseBuffers();
    surface_ = nullptr;
    callback_ = nullptr;
    mallopt(M_FLUSH_THREAD_CACHE, 0);
}

int32_t FCodec::Init()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(!codecName_.empty(), AVCS_ERR_INVALID_VAL, "Init codec failed:  empty name");
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
    sendTask_ = std::make_shared<TaskThread>("sendFrame");
    sendTask_->RegisterHandler([this] { SendFrame(); });
    receiveTask_ = std::make_shared<TaskThread>("ReceiveFrame");
    receiveTask_->RegisterHandler([this] { ReceiveFrame(); });
    state_ = State::Initialized;

    mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);

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
                     formatKey.data(), minVal, maxVal);
    }
}

void FCodec::ConfigureSufrace(const Format &format, const std::string_view &formatKey, uint32_t FORMAT_TYPE)
{
    int32_t val = 0;
    if (formatKey == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT && FORMAT_TYPE == FORMAT_TYPE_INT32) {
        if (format.GetIntValue(formatKey, val)) {
            VideoPixelFormat vpf = static_cast<VideoPixelFormat>(val);
            if (vpf == VideoPixelFormat::RGBA || vpf == VideoPixelFormat::YUV420P || vpf == VideoPixelFormat::NV12 ||
                vpf == VideoPixelFormat::NV21) {
                outputPixelFmt_ = vpf;
                format_.PutIntValue(formatKey, val);
            }
        }
    } else if (formatKey == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE && FORMAT_TYPE == FORMAT_TYPE_INT32) {
        if (format.GetIntValue(formatKey, val)) {
            VideoRotation sr = static_cast<VideoRotation>(val);
            if (sr == VideoRotation::VIDEO_ROTATION_0 || sr == VideoRotation::VIDEO_ROTATION_90 ||
                sr == VideoRotation::VIDEO_ROTATION_180 || sr == VideoRotation::VIDEO_ROTATION_270) {
                format_.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val);
            }
        }
    } else if (formatKey == MediaDescriptionKey::MD_KEY_SCALE_TYPE && FORMAT_TYPE == FORMAT_TYPE_INT32) {
        if (format.GetIntValue(formatKey, val)) {
            ScalingMode scaleMode = static_cast<ScalingMode>(val);
            if (scaleMode == ScalingMode::SCALING_MODE_SCALE_TO_WINDOW ||
                scaleMode == ScalingMode::SCALING_MODE_SCALE_CROP) {
                format_.PutIntValue(formatKey, val);
            }
        }
    } else {
        AVCODEC_LOGW("Set parameter failed: size: %{public}zu  %{public}s, please check your value", formatKey.size(),
                     formatKey.data());
    }
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
    format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width_);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height_);
    avCodecContext_->width = width_;
    avCodecContext_->height = height_;
    avCodecContext_->thread_count = DEFAULT_THREAD_COUNT;
    return AVCS_ERR_OK;
}

int32_t FCodec::Configure(const Format &format)
{
    AVCODEC_SYNC_TRACE;
    if (state_ == State::Uninitialized) {
        int32_t ret = Init();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Init codec failed");
    }
    CHECK_AND_RETURN_RET_LOG((state_ == State::Initialized), AVCS_ERR_INVALID_STATE,
                             "Configure codec failed:  not in Initialized state");
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
            int64_t val64 = 0;
            format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, val64);
            format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, val64);
        } else if (it.first == MediaDescriptionKey::MD_KEY_FRAME_RATE) {
            double val = 0;
            format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, val);
            format_.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, val);
        } else if (it.first == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT ||
                   it.first == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE ||
                   it.first == MediaDescriptionKey::MD_KEY_SCALE_TYPE) {
            ConfigureSufrace(format, it.first, it.second.type);
        } else {
            AVCODEC_LOGW("Set parameter failed: size:%{public}s, unsupport key", it.first.data());
        }
    }

    int32_t ret = ConfigureContext(format);

    state_ = State::Configured;
    AVCODEC_LOGI("Configured codec successful: state: Initialized -> Configured");
    return ret;
}

bool FCodec::IsActive() const
{
    return state_ == State::Running || state_ == State::Flushed || state_ == State::EOS;
}

void FCodec::ResetContext(bool isFlush)
{
    if (avCodecContext_ == nullptr) {
        return;
    }
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
    CHECK_AND_RETURN_RET_LOG((state_ == State::Configured || state_ == State::Flushed), AVCS_ERR_INVALID_STATE,
                             "Start codec failed: not in Configured or Flushed state");
    if (state_ == State::Flushed) {
        state_ = State::Running;
        receiveTask_->Start();
        sendTask_->Start();
        if (surface_ != nullptr) {
            renderTask_->Start();
        }
        for (uint32_t i = 0; i < buffers_[INDEX_INPUT].size(); i++) {
            buffers_[INDEX_INPUT][i]->owner_ = AVBuffer::Owner::OWNED_BY_USER;
            callback_->OnInputBufferAvailable(i, buffers_[INDEX_INPUT][i]->memory_);
        }
        AVCODEC_LOGI("Codec starts successful, state: Flushed -> Running");
        return AVCS_ERR_OK;
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
    CHECK_AND_RETURN_RET_LOG(avcodec_open2(avCodecContext_.get(), avCodec_.get(), nullptr) == 0, AVCS_ERR_UNKNOWN,
                             "Start codec failed: cannot open avcodec");
    state_ = State::Running;
    receiveTask_->Start();
    sendTask_->Start();
    if (surface_ != nullptr) {
        renderTask_->Start();
    }
    for (uint32_t i = 0; i < buffers_[INDEX_INPUT].size(); i++) {
        buffers_[INDEX_INPUT][i]->owner_ = AVBuffer::Owner::OWNED_BY_USER;
        callback_->OnInputBufferAvailable(i, buffers_[INDEX_INPUT][i]->memory_);
    }
    AVCODEC_LOGI("Codec starts successful, state: Configured -> Running");
    return AVCS_ERR_OK;
}

int32_t FCodec::ResetBuffers()
{
    std::unique_lock<std::shared_mutex> iLock(inputMutex_);
    inBufQue_.clear();
    synIndex_ = std::nullopt;
    iLock.unlock();
    std::unique_lock<std::mutex> oLock(outputMutex_);
    codecAvailBuffers_.clear();
    renderBuffers_.clear();
    for (uint32_t i = 0; i < buffers_[INDEX_OUTPUT].size(); i++) {
        buffers_[INDEX_OUTPUT][i]->owner_ = AVBuffer::Owner::OWNED_BY_CODEC;
        codecAvailBuffers_.emplace_back(i);
    }
    oLock.unlock();
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
    av_frame_unref(cachedFrame_.get());
    av_packet_unref(avPacket_.get());
    if (surface_ != nullptr) {
        surface_->CleanCache();
    }
    return AVCS_ERR_OK;
}

int32_t FCodec::Stop()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG((IsActive()), AVCS_ERR_INVALID_STATE, "Stop codec failed: not in executing state");
    state_ = State::Stopping;
    outputCv_.notify_one();
    sendCv_.notify_one();
    recvCv_.notify_one();
    sendTask_->Stop();
    if (surface_ != nullptr && renderTask_ != nullptr) {
        renderTask_->Stop();
    }
    receiveTask_->Stop();
    avcodec_close(avCodecContext_.get());
    ResetContext(true);
    ResetBuffers();
    state_ = State::Configured;
    AVCODEC_LOGI("Stop codec successful, state: Configured");
    return AVCS_ERR_OK;
}

int32_t FCodec::Flush()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG((state_ == State::Running || state_ == State::EOS), AVCS_ERR_INVALID_STATE,
                             "Flush codec failed: not in running or Eos state");
    state_ = State::Flushing;
    outputCv_.notify_one();
    sendCv_.notify_one();
    recvCv_.notify_one();
    sendTask_->Pause();
    receiveTask_->Pause();
    if (surface_ != nullptr) {
        renderTask_->Pause();
    }
    avcodec_flush_buffers(avCodecContext_.get());
    ResetContext(true);
    CHECK_AND_RETURN_RET_LOG(ResetBuffers() == AVCS_ERR_OK, AVCS_ERR_UNKNOWN,
                             "Flush codec failed: cannot release buffer");
    state_ = State::Flushed;
    AVCODEC_LOGI("Flush codec successful, state: Flushed");
    return AVCS_ERR_OK;
}

int32_t FCodec::Reset()
{
    AVCODEC_SYNC_TRACE;
    Release();
    int32_t ret = Init();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Reset codec failed: cannot init codec");
    AVCODEC_LOGI("Reset codec successful, state: Initialized");
    return AVCS_ERR_OK;
}

int32_t FCodec::Release()
{
    AVCODEC_SYNC_TRACE;
    state_ = State::Stopping;
    outputCv_.notify_one();
    sendCv_.notify_one();
    recvCv_.notify_one();
    if (sendTask_ != nullptr) {
        sendTask_->Stop();
    }
    if (surface_ != nullptr && renderTask_ != nullptr) {
        renderTask_->Stop();
    }
    if (receiveTask_ != nullptr) {
        receiveTask_->Stop();
    }
    avcodec_close(avCodecContext_.get());
    ResetContext();
    format_ = Format();
    surface_ = nullptr;
    ReleaseBuffers();
    state_ = State::Uninitialized;
    AVCODEC_LOGI("Release codec successful, state: Uninitialized");
    return AVCS_ERR_OK;
}

void FCodec::SetSurfaceParameter(const Format &format, const std::string_view &formatKey, uint32_t FORMAT_TYPE)
{
    int32_t val = 0;
    if (formatKey == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT && format.GetIntValue(formatKey, val)) {
        VideoPixelFormat vpf = static_cast<VideoPixelFormat>(val);
        if (vpf == VideoPixelFormat::RGBA || vpf == VideoPixelFormat::YUV420P || vpf == VideoPixelFormat::NV12 ||
            vpf == VideoPixelFormat::NV21) {
            outputPixelFmt_ = vpf;
            format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, val);
            GraphicPixelFormat surfacePixelFmt = TranslateSurfaceFormat(vpf);
            std::lock_guard<std::mutex> oLock(outputMutex_);
            SurfaceMemory::SetConfig(width_, height_, surfacePixelFmt);
        }
    } else if (formatKey == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE && format.GetIntValue(formatKey, val)) {
        VideoRotation sr = static_cast<VideoRotation>(val);
        if (sr == VideoRotation::VIDEO_ROTATION_0 || sr == VideoRotation::VIDEO_ROTATION_90 ||
            sr == VideoRotation::VIDEO_ROTATION_180 || sr == VideoRotation::VIDEO_ROTATION_270) {
            format_.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val);
            std::lock_guard<std::mutex> oLock(outputMutex_);
            surface_->SetTransform(TranslateSurfaceRotation(sr));
        }
    } else if (formatKey == MediaDescriptionKey::MD_KEY_SCALE_TYPE && format.GetIntValue(formatKey, val)) {
        ScalingMode scaleMode = static_cast<ScalingMode>(val);
        if (scaleMode == ScalingMode::SCALING_MODE_SCALE_TO_WINDOW ||
            scaleMode == ScalingMode::SCALING_MODE_SCALE_CROP) {
            format_.PutIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, val);
            std::lock_guard<std::mutex> oLock(outputMutex_);
            SurfaceMemory::SetScaleType(scaleMode);
        }
    } else {
        AVCODEC_LOGW("Set parameter failed: size: %{public}s", formatKey.data());
    }
}

int32_t FCodec::SetParameter(const Format &format)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(IsActive(), AVCS_ERR_INVALID_STATE,
                             "Set parameter failed: not in Running or Flushed state");
    for (auto &it : format.GetFormatMap()) {
        if (surface_ != nullptr && it.second.type == FORMAT_TYPE_INT32) {
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
            format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, avCodecContext_->bit_rate);
        }
    }
    if (!format_.ContainKey(MediaDescriptionKey::MD_KEY_FRAME_RATE)) {
        if (avCodecContext_ != nullptr && avCodecContext_->framerate.den > 0) {
            double value = static_cast<double>(avCodecContext_->framerate.num) /
                           static_cast<double>(avCodecContext_->framerate.den);
            format_.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, value);
        }
    }
    if (!format_.ContainKey(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE)) {
        int32_t stride = AlignUp(width_, VIDEO_ALIGN_SIZE);
        int32_t maxInputSize = static_cast<int32_t>((stride * height_ * VIDEO_PIX_DEPTH_YUV) >> 1);
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, maxInputSize);
    }
    format = format_;
    AVCODEC_LOGI("Get outputFormat successful");
    return AVCS_ERR_OK;
}

std::tuple<int32_t, int32_t> FCodec::CalculateBufferSize()
{
    int32_t stride = AlignUp(width_, VIDEO_ALIGN_SIZE);
    int32_t inputBufferSize = static_cast<int32_t>((stride * height_ * VIDEO_PIX_DEPTH_YUV) >> 1);
    int32_t outputBufferSize = inputBufferSize;
    if (outputPixelFmt_ == VideoPixelFormat::RGBA) {
        outputBufferSize = static_cast<int32_t>(stride * height_ * VIDEO_PIX_DEPTH_RGBA);
    }
    AVCODEC_LOGI("Input buffer size = %{public}d, output buffer size=%{public}d", inputBufferSize, outputBufferSize);
    return std::make_tuple(inputBufferSize, outputBufferSize);
}

int32_t FCodec::AllocateInputBuffer(int32_t bufferCnt, int32_t inBufferSize)
{
    int32_t valBufferCnt = 0;
    for (int32_t i = 0; i < bufferCnt; i++) {
        std::shared_ptr<AVBuffer> buf = std::make_shared<AVBuffer>();
        buf->memory_ = AVSharedMemoryBase::CreateFromLocal(inBufferSize, AVSharedMemory::FLAGS_READ_WRITE,
                                                           std::string("Buffer") + std::to_string(i));
        if (buf->memory_ == nullptr || buf->memory_->GetBase() == nullptr) {
            AVCODEC_LOGE("Allocate input buffer failed, index=%{public}d", i);
            continue;
        }
        buf->owner_ = AVBuffer::Owner::OWNED_BY_USER;
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

int32_t FCodec::AllocateOutputBuffer(int32_t bufferCnt, int32_t outBufferSize)
{
    int32_t valBufferCnt = 0;
    if (surface_) {
        CHECK_AND_RETURN_RET_LOG(surface_->SetQueueSize(bufferCnt) == OHOS::SurfaceError::SURFACE_ERROR_OK,
                                 AVCS_ERR_NO_MEMORY, "Surface set QueueSize=%{public}d failed", bufferCnt);
        if (outputPixelFmt_ == VideoPixelFormat::UNKNOWN_FORMAT) {
            format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
                                static_cast<int32_t>(VideoPixelFormat::YUV420P));
        }
        int32_t val32 = 0;
        format_.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, val32);
        GraphicPixelFormat surfacePixelFmt = TranslateSurfaceFormat(static_cast<VideoPixelFormat>(val32));
        CHECK_AND_RETURN_RET_LOG(surfacePixelFmt != GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT, AVCS_ERR_UNSUPPORT,
                                 "Failed to allocate output buffer: unsupported surface format");
        SurfaceMemory::SetSurface(surface_);
        SurfaceMemory::SetConfig(static_cast<int32_t>(width_), static_cast<int32_t>(height_), surfacePixelFmt);

        format_.GetIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, val32);
        SurfaceMemory::SetScaleType(static_cast<ScalingMode>(val32));
        format_.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val32);
        surface_->SetTransform(TranslateSurfaceRotation(static_cast<VideoRotation>(val32)));
    }
    for (int i = 0; i < bufferCnt; i++) {
        std::shared_ptr<AVBuffer> buf = std::make_shared<AVBuffer>();
        if (surface_ == nullptr) {
            buf->memory_ = AVSharedMemoryBase::CreateFromLocal(outBufferSize, AVSharedMemory::FLAGS_READ_WRITE,
                                                               std::string("Buffer") + std::to_string(i));
        } else {
            buf->memory_ = SurfaceMemory::Create();
        }
        if (buf->memory_ == nullptr || buf->memory_->GetBase() == nullptr) {
            AVCODEC_LOGE("Allocate output buffer failed, index=%{public}d", i);
            continue;
        }
        buf->owner_ = AVBuffer::Owner::OWNED_BY_CODEC;
        buffers_[INDEX_OUTPUT].emplace_back(buf);
        codecAvailBuffers_.emplace_back(valBufferCnt);
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

int32_t FCodec::AllocateBuffers()
{
    AVCODEC_SYNC_TRACE;
    std::tie(inputBufferSize_, outputBufferSize_) = CalculateBufferSize();
    CHECK_AND_RETURN_RET_LOG(inputBufferSize_ > 0 && outputBufferSize_ > 0, AVCS_ERR_INVALID_VAL,
                             "Allocate buffer with input size=%{public}d, output size=%{public}d failed",
                             inputBufferSize_, outputBufferSize_);
    if (surface_ != nullptr && isOutBufSetted_ == false) {
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, DEFAULT_OUT_SURFACE_CNT);
    }
    int32_t InputBufferCnt = 0;
    int32_t OutputBufferCnt = 0;
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, InputBufferCnt);
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, OutputBufferCnt);
    if (AllocateInputBuffer(InputBufferCnt, inputBufferSize_) == AVCS_ERR_NO_MEMORY ||
        AllocateOutputBuffer(OutputBufferCnt, outputBufferSize_) == AVCS_ERR_NO_MEMORY) {
        return AVCS_ERR_NO_MEMORY;
    }
    AVCODEC_LOGI("Allocate buffers successful");
    return AVCS_ERR_OK;
}

int32_t FCodec::UpdateBuffers(uint32_t index, int32_t buffer_size, uint32_t buffer_type)
{
    AVCODEC_SYNC_TRACE;
    int32_t old_size = buffers_[buffer_type][index]->memory_->GetSize();
    if (buffer_size != old_size) {
        std::shared_ptr<AVBuffer> buf = std::make_shared<AVBuffer>();
        buf->memory_ = AVSharedMemoryBase::CreateFromLocal(buffer_size, AVSharedMemory::FLAGS_READ_WRITE,
                                                           std::string("Buffer") + std::to_string(index));
        if (buf->memory_ == nullptr || buf->memory_->GetBase() == nullptr) {
            AVCODEC_LOGE("Buffer allocate failed, index=%{public}d", index);
            return AVCS_ERR_NO_MEMORY;
        }
        buf->owner_ = AVBuffer::Owner::OWNED_BY_USER;
        buffers_[buffer_type][index] = buf;
    }
    return AVCS_ERR_OK;
}

int32_t FCodec::UpdateSurfaceMemory()
{
    AVCODEC_SYNC_TRACE;
    sptr<SurfaceBuffer> surfaceBuffer = nullptr;
    std::shared_ptr<AVBuffer> outputBuffer = nullptr;
    std::shared_ptr<SurfaceMemory> surfaceMemory = nullptr;
    std::unique_lock<std::mutex> oLock(outputMutex_);
    while (codecAvailBuffers_.size() > 0) {
        uint32_t idx = *codecAvailBuffers_.begin();
        outputBuffer = buffers_[INDEX_OUTPUT][idx];
        surfaceMemory = std::static_pointer_cast<SurfaceMemory>(outputBuffer->memory_);
        surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
        CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_NO_MEMORY,
                                 "getSurfaceBuffer failed, index=%{public}u", idx);
        surface_->CancelBuffer(surfaceBuffer);
        codecAvailBuffers_.erase(codecAvailBuffers_.begin());
        surfaceMemory->ReleaseSurfaceBuffer();
        renderBuffers_.emplace_back(idx);
    }
    outputCv_.wait(oLock, [this]() { return codecAvailBuffers_.size() > 0 || state_ != State::Running; });
    return state_ != State::Running ? AVCS_ERR_INVALID_STATE : AVCS_ERR_OK;
}

int32_t FCodec::CheckFormatChange(uint32_t index, int width, int height)
{
    AVCODEC_SYNC_TRACE;
    if (width_ != width || height_ != height) {
        width_ = width;
        height_ = height;
        formatChange_ = true;
        std::tie(inputBufferSize_, outputBufferSize_) = CalculateBufferSize();
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width_);
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height_);
        if (surface_) {
            int32_t val32 = 0;
            format_.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, val32);
            GraphicPixelFormat surfacePixelFmt = TranslateSurfaceFormat(static_cast<VideoPixelFormat>(val32));
            SurfaceMemory::SetConfig(static_cast<int32_t>(width_), static_cast<int32_t>(height_), surfacePixelFmt);
        }
        callback_->OnOutputFormatChanged(format_);
    } else {
        formatChange_ = false;
    }
    if (surface_ == nullptr) {
        CHECK_AND_RETURN_RET_LOG((UpdateBuffers(index, outputBufferSize_, INDEX_OUTPUT) == AVCS_ERR_OK),
                                 AVCS_ERR_NO_MEMORY, "Update output buffer failed, index=%{public}u", index);
    } else if (formatChange_) {
        int32_t ret = UpdateSurfaceMemory();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Update surface memory failed");
    }
    return AVCS_ERR_OK;
}

int32_t FCodec::ReleaseBuffers(bool isFlush)
{
    std::unique_lock<std::shared_mutex> iLock(inputMutex_);
    inBufQue_.clear();
    buffers_[INDEX_INPUT].clear();
    synIndex_ = std::nullopt;
    iLock.unlock();
    std::unique_lock<std::mutex> oLock(outputMutex_);
    codecAvailBuffers_.clear();
    renderBuffers_.clear();
    buffers_[INDEX_OUTPUT].clear();
    oLock.unlock();
    isBufferAllocated_ = false;
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
    if (surface_ != nullptr) {
        surface_->CleanCache();
    }
    return AVCS_ERR_OK;
}

int32_t FCodec::QueueInputBuffer(uint32_t index, const AVCodecBufferInfo &info, AVCodecBufferFlag flag)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(state_ == State::Running, AVCS_ERR_INVALID_STATE,
                             "Queue input buffer failed: not in Running state");
    std::vector<std::shared_ptr<AVBuffer>> &inBuffers = buffers_[INDEX_INPUT];
    CHECK_AND_RETURN_RET_LOG(index < inBuffers.size(), AVCS_ERR_INVALID_VAL,
                             "Queue input buffer failed with bad index, index=%{public}u, buffer_size=%{public}zu",
                             index, inBuffers.size());
    CHECK_AND_RETURN_RET_LOG(inBuffers[index]->owner_ == AVBuffer::Owner::OWNED_BY_USER, AVCS_ERR_INVALID_OPERATION,
                             "Queue input buffer failed: buffer with index=%{public}u is not available", index);
    std::unique_lock<std::shared_mutex> iLock(inputMutex_);
    inBuffers[index]->owner_ = AVBuffer::Owner::OWNED_BY_CODEC;
    inBuffers[index]->bufferInfo_ = info;
    inBuffers[index]->bufferFlag_ = flag;
    if (synIndex_) {
        std::shared_ptr<AVBuffer> &curInputBuffer = buffers_[INDEX_INPUT][synIndex_.value()];
        if ((curInputBuffer->bufferInfo_.size + info.size <= curInputBuffer->memory_->GetSize()) &&
            memcpy_s(curInputBuffer->memory_->GetBase() + curInputBuffer->bufferInfo_.size, info.size,
                     inBuffers[index]->memory_->GetBase(), info.size) == EOK) {
            curInputBuffer->bufferInfo_.size += info.size;
            curInputBuffer->bufferFlag_ = flag;
            curInputBuffer->bufferInfo_.presentationTimeUs = info.presentationTimeUs;

            if (!(flag & AVCODEC_BUFFER_FLAG_CODEC_DATA) && !(flag & AVCODEC_BUFFER_FLAG_PARTIAL_FRAME)) {
                inBufQue_.emplace_back(synIndex_.value());
                synIndex_ = std::nullopt;
            }
            inBuffers[index]->owner_ = AVBuffer::Owner::OWNED_BY_USER;
            iLock.unlock();
            callback_->OnInputBufferAvailable(index, inBuffers[index]->memory_);
            return AVCS_ERR_OK;
        } else {
            AVCODEC_LOGE("packet size %{public}d over buffer size %{public}d",
                         curInputBuffer->bufferInfo_.size + info.size, curInputBuffer->memory_->GetSize());
            callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY);
            state_ = State::Error;
            return AVCS_ERR_NO_MEMORY;
        }
    } else {
        if ((flag & AVCODEC_BUFFER_FLAG_CODEC_DATA) || (flag & AVCODEC_BUFFER_FLAG_PARTIAL_FRAME)) {
            synIndex_ = index;
        } else {
            inBufQue_.emplace_back(index);
        }
    }

    return AVCS_ERR_OK;
}

void FCodec::SendFrame()
{
    AVCODEC_SYNC_TRACE;
    if (state_ != State::Running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    std::unique_lock<std::shared_mutex> iLock(inputMutex_);
    if (inBufQue_.empty()) {
        iLock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    uint32_t index = inBufQue_.front();
    std::shared_ptr<AVBuffer> &inputBuffer = buffers_[INDEX_INPUT][index];
    iLock.unlock();
    if (inputBuffer->bufferFlag_ & AVCODEC_BUFFER_FLAG_EOS) {
        AVCODEC_LOGI("Send eos start");
        avPacket_->data = nullptr;
        std::unique_lock<std::mutex> sendLock(sendMutex_);
        isSendEos_ = true;
        sendCv_.wait_for(sendLock, std::chrono::milliseconds(DEFAULT_DECODE_WAIT_TIME));
        AVCODEC_LOGI("Send eos end");
    } else {
        avPacket_->data = inputBuffer->memory_->GetBase();
        avPacket_->size = static_cast<int32_t>(inputBuffer->bufferInfo_.size);
        avPacket_->pts = inputBuffer->bufferInfo_.presentationTimeUs;
    }
    std::unique_lock<std::mutex> sLock(syncMutex_);
    int ret = avcodec_send_packet(avCodecContext_.get(), avPacket_.get());
    sLock.unlock();
    if (ret == 0 || ret == AVERROR_INVALIDDATA) {
        if (ret == AVERROR_INVALIDDATA) {
            AVCODEC_LOGW("ffmpeg ret = %{public}s", AVStrError(ret).c_str());
        }
        std::unique_lock<std::shared_mutex> iLock(inputMutex_);
        inBufQue_.pop_front();
        iLock.unlock();
        std::unique_lock<std::mutex> recvLock(recvMutex_);
        recvCv_.notify_one();
        recvLock.unlock();
        inputBuffer->owner_ = AVBuffer::Owner::OWNED_BY_USER;
        callback_->OnInputBufferAvailable(index, inputBuffer->memory_);
    } else if (ret == AVERROR(EAGAIN)) {
        std::unique_lock<std::mutex> sendLock(sendMutex_);
        isSendWait_ = true;
        sendCv_.wait_for(sendLock, std::chrono::milliseconds(DEFAULT_DECODE_WAIT_TIME));
    } else {
        AVCODEC_LOGE("Cannot send frame to codec: ffmpeg ret = %{public}s", AVStrError(ret).c_str());
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
        state_ = State::Error;
    }
}

int32_t FCodec::FillFrameBuffer(const std::shared_ptr<AVBuffer> &frameBuffer)
{
    CHECK_AND_RETURN_RET_LOG((cachedFrame_->flags & AV_FRAME_FLAG_CORRUPT) == 0, AVCS_ERR_INVALID_VAL,
                             "Recevie frame from codec failed: decoded frame is corrupt");
    VideoPixelFormat targetPixelFmt = outputPixelFmt_;
    if (outputPixelFmt_ == VideoPixelFormat::UNKNOWN_FORMAT) {
        targetPixelFmt = surface_ ? VideoPixelFormat::YUV420P : ConvertPixelFormatFromFFmpeg(cachedFrame_->format);
    }
    AVPixelFormat ffmpegFormat = ConvertPixelFormatToFFmpeg(targetPixelFmt);
    int32_t ret;
    if (ffmpegFormat == static_cast<AVPixelFormat>(cachedFrame_->format)) {
        for (int32_t i = 0; cachedFrame_->linesize[i] > 0; i++) {
            scaleData_[i] = cachedFrame_->data[i];
            scaleLineSize_[i] = cachedFrame_->linesize[i];
        }
    } else {
        scale_ = formatChange_ ? nullptr : scale_;
        ret = ConvertVideoFrame(&scale_, cachedFrame_, scaleData_, scaleLineSize_, ffmpegFormat);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Scale video frame failed: %{public}d", ret);
        isConverted_ = true;
    }
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(targetPixelFmt));
    std::shared_ptr<AVSharedMemory> frameMomory = frameBuffer->memory_;
    if (surface_) {
        std::shared_ptr<SurfaceMemory> surfaceMemory = std::static_pointer_cast<SurfaceMemory>(frameMomory);
        CHECK_AND_RETURN_RET_LOG(surfaceMemory != nullptr, AVCS_ERR_INVALID_VAL,
                                 "Failed to dynamic cast AVSharedMemory to SurfaceMemory");
        ret = WriteSurfaceData(surfaceMemory, scaleData_, scaleLineSize_, format_);
        frameBuffer->bufferInfo_.size = surfaceMemory->GetUsedSize();
    } else {
        std::shared_ptr<AVSharedMemoryBase> bufferMemory = std::static_pointer_cast<AVSharedMemoryBase>(frameMomory);
        CHECK_AND_RETURN_RET_LOG(bufferMemory != nullptr, AVCS_ERR_INVALID_VAL,
                                 "Failed to dynamic cast AVSharedMemory to ShareMemory");
        ret = WriteBufferData(bufferMemory, scaleData_, scaleLineSize_, format_);
        frameBuffer->bufferInfo_.size = bufferMemory->GetUsedSize();
    }
    frameBuffer->bufferInfo_.presentationTimeUs = cachedFrame_->pts;
    AVCODEC_LOGD("Fill frame buffer successful");
    return ret;
}

void FCodec::FramePostProcess(std::shared_ptr<AVBuffer> frameBuffer, int32_t status, int ret)
{
    uint32_t index = codecAvailBuffers_.front();
    if (status == AVCS_ERR_OK) {
        std::unique_lock<std::mutex> oLock(outputMutex_);
        codecAvailBuffers_.pop_front();
        oLock.unlock();
        frameBuffer->owner_ = AVBuffer::Owner::OWNED_BY_USER;
        if (ret == AVERROR_EOF) {
            std::unique_lock<std::mutex> sLock(syncMutex_);
            avcodec_flush_buffers(avCodecContext_.get());
            sLock.unlock();
            callback_->OnOutputBufferAvailable(index, frameBuffer->bufferInfo_, AVCODEC_BUFFER_FLAG_EOS, nullptr);
        } else {
            if (isSendWait_) {
                std::lock_guard<std::mutex> sLock(sendMutex_);
                isSendWait_ = false;
                sendCv_.notify_one();
            }
            callback_->OnOutputBufferAvailable(index, frameBuffer->bufferInfo_, AVCODEC_BUFFER_FLAG_NONE,
                                               surface_ ? nullptr : frameBuffer->memory_);
        }
    } else if (status == AVCS_ERR_UNSUPPORT) {
        AVCODEC_LOGE("Recevie frame from codec failed: OnError");
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT);
        state_ = State::Error;
    } else {
        AVCODEC_LOGE("Recevie frame from codec failed");
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
        state_ = State::Error;
    }
}

void FCodec::ReceiveFrame()
{
    AVCODEC_SYNC_TRACE;
    if (state_ != State::Running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    std::unique_lock<std::mutex> oLock(outputMutex_);
    outputCv_.wait(oLock, [this]() { return codecAvailBuffers_.size() > 0 || state_ != State::Running; });
    if (state_ != State::Running) {
        return;
    }
    uint32_t index = codecAvailBuffers_.front();
    std::shared_ptr<AVBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
    oLock.unlock();
    std::unique_lock<std::mutex> sLock(syncMutex_);
    av_frame_unref(cachedFrame_.get());
    int ret = avcodec_receive_frame(avCodecContext_.get(), cachedFrame_.get());
    sLock.unlock();
    int32_t status = AVCS_ERR_OK;
    if (ret >= 0) {
        if (CheckFormatChange(index, cachedFrame_->width, cachedFrame_->height) == AVCS_ERR_OK) {
            frameBuffer = buffers_[INDEX_OUTPUT][codecAvailBuffers_.front()];
            status = FillFrameBuffer(frameBuffer);
        } else {
            CHECK_AND_RETURN_LOG(state_ == State::Running, "Not in running state");
            callback_->OnError(AVCODEC_ERROR_EXTEND_START, AVCS_ERR_NO_MEMORY);
            return;
        }
    } else if (ret == AVERROR_EOF) {
        AVCODEC_LOGI("Receive eos");
        frameBuffer->bufferFlag_ = AVCODEC_BUFFER_FLAG_EOS;
        frameBuffer->bufferInfo_.size = 0;
        state_ = State::EOS;
    } else if (ret == AVERROR(EAGAIN)) {
        if (isSendWait_ || isSendEos_) {
            std::lock_guard<std::mutex> sendLock(sendMutex_);
            isSendWait_ = false;
            sendCv_.notify_one();
        }
        std::unique_lock<std::mutex> recvLock(recvMutex_);
        recvCv_.wait_for(recvLock, std::chrono::milliseconds(DEFAULT_DECODE_WAIT_TIME));
        return;
    } else if (ret == AVERROR_INVALIDDATA) {
        AVCODEC_LOGW("ffmpeg ret = %{public}s", AVStrError(ret).c_str());
        return;
    } else {
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
        state_ = State::Error;
        return;
    }
    FramePostProcess(frameBuffer, status, ret);
}

void FCodec::RenderFrame()
{
    AVCODEC_SYNC_TRACE;
    if (state_ != State::Running && state_ != State::EOS) {
        AVCODEC_LOGD("Failed to render frame to codec: not in Running or Flushed state");
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    std::unique_lock<std::mutex> oLock(outputMutex_);
    if (renderBuffers_.empty()) {
        oLock.unlock();
        AVCODEC_LOGD("Failed to render frame to codec: empty render buffer");
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    uint32_t index = renderBuffers_.front();
    std::shared_ptr<AVBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
    oLock.unlock();
    std::shared_ptr<SurfaceMemory> surfaceMemory = std::static_pointer_cast<SurfaceMemory>(outputBuffer->memory_);
    while (state_ == State::Running || state_ == State::EOS) {
        if (surfaceMemory->GetSurfaceBuffer() == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        } else {
            AVCODEC_LOGD("render frame success, index=%{public}u", index);
            oLock.lock();
            codecAvailBuffers_.emplace_back(index);
            renderBuffers_.pop_front();
            oLock.unlock();
            outputBuffer->owner_ = AVBuffer::Owner::OWNED_BY_CODEC;
            if (codecAvailBuffers_.size() == 1) {
                outputCv_.notify_one();
            }
            break;
        }
    }
}

int32_t FCodec::ReleaseOutputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_OUTPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Failed to release output buffer: invalid index");
    std::lock_guard<std::mutex> oLock(outputMutex_);
    if (std::find(codecAvailBuffers_.begin(), codecAvailBuffers_.end(), index) == codecAvailBuffers_.end() &&
        buffers_[INDEX_OUTPUT][index]->owner_ == AVBuffer::Owner::OWNED_BY_USER) {
        if (surface_ != nullptr) {
            std::shared_ptr<AVBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
            std::shared_ptr<SurfaceMemory> surfaceMemory =
                std::static_pointer_cast<SurfaceMemory>(outputBuffer->memory_);
            surfaceMemory->ReleaseSurfaceBuffer();
            renderBuffers_.emplace_back(index);
        } else {
            buffers_[INDEX_OUTPUT][index]->owner_ = AVBuffer::Owner::OWNED_BY_CODEC;
            codecAvailBuffers_.emplace_back(index);
            if (codecAvailBuffers_.size() == 1) {
                outputCv_.notify_one();
            }
        }
        return AVCS_ERR_OK;
    } else {
        AVCODEC_LOGE("Release output buffer failed: check your index=%{public}u", index);
        return AVCS_ERR_INVALID_VAL;
    }
}

int32_t FCodec::UpdateSurfaceMemory(std::shared_ptr<SurfaceMemory> &surfaceMemory, int64_t pts)
{
    sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_INVALID_VAL,
                             "Failed to update surface memory: surface buffer is NULL");
    OHOS::BufferFlushConfig flushConfig = {{0, 0, surfaceBuffer->GetWidth(), surfaceBuffer->GetHeight()}, pts};
    surfaceMemory->SetNeedRender(true);
    surfaceMemory->UpdateSurfaceBufferScaleMode();
    auto res = surface_->FlushBuffer(surfaceBuffer, -1, flushConfig);
    if (res != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        AVCODEC_LOGW("Failed to update surface memory: %{public}d", res);
        surfaceMemory->SetNeedRender(false);
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

int32_t FCodec::RenderOutputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(index < buffers_[INDEX_OUTPUT].size(), AVCS_ERR_INVALID_VAL,
                             "Failed to render output buffer: invalid index");
    if (std::find(codecAvailBuffers_.begin(), codecAvailBuffers_.end(), index) == codecAvailBuffers_.end() &&
        buffers_[INDEX_OUTPUT][index]->owner_ == AVBuffer::Owner::OWNED_BY_USER) {
        std::shared_ptr<AVBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
        std::shared_ptr<SurfaceMemory> surfaceMemory = std::static_pointer_cast<SurfaceMemory>(outputBuffer->memory_);

        int32_t ret = UpdateSurfaceMemory(surfaceMemory, outputBuffer->bufferInfo_.presentationTimeUs);
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGW("Update surface memory failed: %{public}d", static_cast<int32_t>(ret));
        } else {
            AVCODEC_LOGD("Update surface memory successful");
        }
        std::lock_guard<std::mutex> oLock(outputMutex_);
        surfaceMemory->ReleaseSurfaceBuffer();
        outputBuffer->owner_ = AVBuffer::Owner::OWNED_BY_SURFACE;
        renderBuffers_.emplace_back(index);
        return AVCS_ERR_OK;
    } else {
        AVCODEC_LOGE("Failed to render output buffer with bad index, index=%{public}u", index);
        return AVCS_ERR_INVALID_VAL;
    }
}

int32_t FCodec::SetOutputSurface(sptr<Surface> surface)
{
    AVCODEC_SYNC_TRACE;
    surface_ = surface;
    if (!format_.ContainKey(MediaDescriptionKey::MD_KEY_SCALE_TYPE)) {
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE,
                            static_cast<int32_t>(ScalingMode::SCALING_MODE_SCALE_TO_WINDOW));
    }
    if (!format_.ContainKey(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE)) {
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE,
                            static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_0));
    }

    if (renderTask_ == nullptr) {
        renderTask_ = std::make_shared<TaskThread>("RenderFrame");
        renderTask_->RegisterHandler([this] { (void)RenderFrame(); });
    }
    AVCODEC_LOGI("Set surface success");
    return AVCS_ERR_OK;
}

int32_t FCodec::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_INVALID_VAL, "Set callback failed: callback is NULL");
    callback_ = callback;
    return AVCS_ERR_OK;
}

int32_t FCodec::GetCodecCapability(std::vector<CapabilityData> &capaArray)
{
    for (uint32_t i = 0; i < SUPPORT_VCODEC_NUM; ++i) {
        CapabilityData capsData;
        capsData.codecName = static_cast<std::string>(SUPPORT_VCODEC[i].codecName);
        capsData.mimeType = static_cast<std::string>(SUPPORT_VCODEC[i].mimeType);
        capsData.codecType = SUPPORT_VCODEC[i].isEncoder ? AVCODEC_TYPE_VIDEO_ENCODER : AVCODEC_TYPE_VIDEO_DECODER;
        capsData.isVendor = false;
        capsData.maxInstance = VIDEO_INSTANCE_SIZE;
        capsData.alignment.width = VIDEO_ALIGNMENT_SIZE;
        capsData.alignment.height = VIDEO_ALIGNMENT_SIZE;
        capsData.width.minVal = VIDEO_MIN_SIZE;
        capsData.width.maxVal = VIDEO_MAX_WIDTH_SIZE;
        capsData.height.minVal = VIDEO_MIN_SIZE;
        capsData.height.maxVal = VIDEO_MAX_HEIGHT_SIZE;
        capsData.frameRate.minVal = 0;
        capsData.frameRate.maxVal = VIDEO_FRAMERATE_MAX_SIZE;
        capsData.bitrate.minVal = 1;
        capsData.bitrate.maxVal = VIDEO_BITRATE_MAX_SIZE;
        capsData.blockPerFrame.minVal = 1;
        capsData.blockPerFrame.maxVal = VIDEO_BLOCKPERFRAME_SIZE;
        capsData.blockPerSecond.minVal = 1;
        capsData.blockPerSecond.maxVal = VIDEO_BLOCKPERSEC_SIZE;
        capsData.blockSize.width = VIDEO_ALIGN_SIZE;
        capsData.blockSize.height = VIDEO_ALIGN_SIZE;
        if (SUPPORT_VCODEC[i].isEncoder) {
            capsData.complexity.minVal = 0;
            capsData.complexity.maxVal = 0;
            capsData.encodeQuality.minVal = 0;
            capsData.encodeQuality.maxVal = 0;
        }
        capsData.pixFormat = {
            static_cast<int32_t>(VideoPixelFormat::YUV420P), static_cast<int32_t>(VideoPixelFormat::NV12),
            static_cast<int32_t>(VideoPixelFormat::NV21), static_cast<int32_t>(VideoPixelFormat::RGBA)};
        capsData.profiles = {static_cast<int32_t>(AVC_PROFILE_BASELINE), static_cast<int32_t>(AVC_PROFILE_MAIN),
                             static_cast<int32_t>(AVC_PROFILE_HIGH)};
        std::vector<int32_t> levels;
        for (int32_t i = 0; i <= static_cast<int32_t>(AVCLevel::AVC_LEVEL_51); ++i) {
            levels.emplace_back(i);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AVC_PROFILE_MAIN), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AVC_PROFILE_HIGH), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AVC_PROFILE_BASELINE), levels));
        capaArray.emplace_back(capsData);
    }
    return AVCS_ERR_OK;
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS