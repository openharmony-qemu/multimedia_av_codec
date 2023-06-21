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

#include <list>
#include <mutex>
#include "native_avcodec_base.h"
#include "native_avcodec_videoencoder.h"
#include "native_avmagic.h"
#include "native_window.h"
#include "avcodec_video_encoder.h"
#include "avsharedmemory.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avcodec_dfx.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeVideoEncoder"};
}

using namespace OHOS::MediaAVCodec;
class NativeVideoEncoderCallback;

struct VideoEncoderObject : public OH_AVCodec {
    explicit VideoEncoderObject(const std::shared_ptr<AVCodecVideoEncoder> &encoder)
        : OH_AVCodec(AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER), videoEncoder_(encoder) {}
    ~VideoEncoderObject() = default;

    const std::shared_ptr<AVCodecVideoEncoder> videoEncoder_;
    std::list<OHOS::sptr<OH_AVMemory>> memoryObjList_;
    OHOS::sptr<OH_AVFormat> outputFormat_ = nullptr;
    OHOS::sptr<OH_AVFormat> inputputFormat_ = nullptr;
    std::shared_ptr<NativeVideoEncoderCallback> callback_ = nullptr;
    std::atomic<bool> isFlushing_ = false;
    std::atomic<bool> isStop_ = false;
    std::atomic<bool> isEOS_ = false;
    bool isInputSurfaceMode_ = false;
    std::atomic<bool> isFirstFrameIn_ = true;
    std::atomic<bool> isFirstFrameOut_ = true;
};

class NativeVideoEncoderCallback : public AVCodecCallback {
public:
    NativeVideoEncoderCallback(OH_AVCodec *codec, struct OH_AVCodecAsyncCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData) {}
    virtual ~NativeVideoEncoderCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        (void)errorType;

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(callback_.onError != nullptr, "Callback is nullptr");
        int32_t extErr = AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(errorCode));
        callback_.onError(codec_, extErr, userData_);
    }

    void OnOutputFormatChanged(const Format &format) override
    {
        std::unique_lock<std::mutex> lock(mutex_);

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(callback_.onStreamChanged != nullptr, "Callback is nullptr");
        OHOS::sptr<OH_AVFormat> object = new(std::nothrow) OH_AVFormat(format);
        // The object lifecycle is controlled by the current function stack
        callback_.onStreamChanged(codec_, reinterpret_cast<OH_AVFormat *>(object.GetRefPtr()), userData_);
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override
    {
        std::unique_lock<std::mutex> lock(mutex_);

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(callback_.onNeedInputData != nullptr, "Callback is nullptr");

        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoEncObj->videoEncoder_ != nullptr, "Context video decoder is nullptr!");

        if (videoEncObj->isFlushing_.load() || videoEncObj->isStop_.load() || videoEncObj->isEOS_.load()) {
            AVCODEC_LOGD("At flush, eos or stop, no buffer available");
            return;
        }
        OH_AVMemory *data = nullptr;
        if (!videoEncObj->isInputSurfaceMode_) {
            data = GetInputData(codec_, index, buffer);
            CHECK_AND_RETURN_LOG(data != nullptr, "Data is nullptr, get input data failed");
        }
        callback_.onNeedInputData(codec_, index, data, userData_);
    }

    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override
    {
        std::unique_lock<std::mutex> lock(mutex_);

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(callback_.onNeedOutputData != nullptr, "Callback is nullptr");

        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoEncObj->videoEncoder_ != nullptr, "Context video decoder is nullptr!");

        if (videoEncObj->isFlushing_.load() || videoEncObj->isStop_.load()) {
            AVCODEC_LOGD("At flush or stop, ignore");
            return;
        }

        struct OH_AVCodecBufferAttr bufferAttr;
        bufferAttr.pts = info.presentationTimeUs;
        bufferAttr.size = info.size;
        bufferAttr.offset = info.offset;
        bufferAttr.flags = flag;
        // The bufferInfo lifecycle is controlled by the current function stack
        OH_AVMemory *data = GetOutputData(codec_, index, buffer);
        callback_.onNeedOutputData(codec_, index, data, &bufferAttr, userData_);

        if (flag != AVCODEC_BUFFER_FLAG_CODEC_DATA) {
            if (videoEncObj->isFirstFrameOut_) {
                AVCodecTrace::TraceEnd("OH::FirstFrame", info.presentationTimeUs);
                videoEncObj->isFirstFrameOut_ = false;
            } else {
                AVCodecTrace::TraceEnd("OH::Frame", info.presentationTimeUs);
            }
        }
        if (flag == AVCODEC_BUFFER_FLAG_EOS) {
            videoEncObj->isFirstFrameIn_ = true;
            videoEncObj->isFirstFrameOut_ = true;
        }
    }

    void StopCallback()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        codec_ = nullptr;
    }

private:
    OH_AVMemory *GetInputData(struct OH_AVCodec *codec, uint32_t index, std::shared_ptr<AVSharedMemory> memory)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, nullptr, "Codec magic error!");

        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video decoder is nullptr!");
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "Memory is nullptr, get input buffer failed!");

        for (auto &memoryObj : videoEncObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<OH_AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<OH_AVMemory> object = new (std::nothrow) OH_AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "AV memory create failed");

        videoEncObj->memoryObjList_.push_back(object);
        return reinterpret_cast<OH_AVMemory *>(object.GetRefPtr());
    }

    OH_AVMemory *GetOutputData(struct OH_AVCodec *codec, uint32_t index, std::shared_ptr<AVSharedMemory> memory)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, nullptr, "Codec magic error!");

        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video encoder is nullptr!");
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "Memory is nullptr, get output buffer failed!");

        for (auto &memoryObj : videoEncObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<OH_AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<OH_AVMemory> object = new(std::nothrow) OH_AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "AV memory create failed");

        videoEncObj->memoryObjList_.push_back(object);
        return reinterpret_cast<OH_AVMemory *>(object.GetRefPtr());
    }

    struct OH_AVCodec *codec_;
    struct OH_AVCodecAsyncCallback callback_;
    void *userData_;
    std::mutex mutex_;
};

struct OH_AVCodec *OH_VideoEncoder_CreateByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "Mime is nullptr!");

    std::shared_ptr<AVCodecVideoEncoder> videoEncoder = VideoEncoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, nullptr, "Video encoder create by mime failed");

    struct VideoEncoderObject *object = new(std::nothrow) VideoEncoderObject(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Video encoder create by mime failed");

    return object;
}

struct OH_AVCodec *OH_VideoEncoder_CreateByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "Name is nullptr!");

    std::shared_ptr<AVCodecVideoEncoder> videoEncoder = VideoEncoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, nullptr, "Video encoder create by name failed");

    struct VideoEncoderObject *object = new(std::nothrow) VideoEncoderObject(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Video encoder create by name failed");

    return object;
}

OH_AVErrCode OH_VideoEncoder_Destroy(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER,
        AV_ERR_INVALID_VAL, "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);

    if (videoEncObj != nullptr && videoEncObj->videoEncoder_ != nullptr) {
        if (videoEncObj->callback_ != nullptr) {
            videoEncObj->callback_->StopCallback();
        }
        videoEncObj->memoryObjList_.clear();
        videoEncObj->isStop_.store(true);
        int32_t ret = videoEncObj->videoEncoder_->Release();
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGE("Video encoder destroy failed!");
            delete codec;
            return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
        }
    } else {
        AVCODEC_LOGD("Video encoder is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Configure(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "Format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::AVCODEC_MAGIC_FORMAT, AV_ERR_INVALID_VAL,
        "Format magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder configure failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Prepare(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder prepare failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Start(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    videoEncObj->isStop_.store(false);
    videoEncObj->isEOS_.store(false);
    int32_t ret = videoEncObj->videoEncoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder start failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Stop(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    videoEncObj->isStop_.store(true);
    int32_t ret = videoEncObj->videoEncoder_->Stop();
    if (ret != AVCS_ERR_OK) {
        videoEncObj->isStop_.store(false);
        AVCODEC_LOGE("Video encoder stop failed");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    videoEncObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Flush(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    videoEncObj->isFlushing_.store(true);
    int32_t ret = videoEncObj->videoEncoder_->Flush();
    videoEncObj->isFlushing_.store(false);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder flush failed!");
    videoEncObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Reset(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");
    
    videoEncObj->isStop_.store(true);
    int32_t ret = videoEncObj->videoEncoder_->Reset();
    if (ret != AVCS_ERR_OK) {
        videoEncObj->isStop_.store(false);
        AVCODEC_LOGE("Video encoder reset failed");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    videoEncObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_GetSurface(OH_AVCodec *codec, OHNativeWindow **window)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr && window != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    OHOS::sptr<OHOS::Surface> surface = videoEncObj->videoEncoder_->CreateInputSurface();
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, AV_ERR_OPERATE_NOT_PERMIT,
                             "Video encoder get surface failed!");

    *window = CreateNativeWindowFromSurface(&surface);
    CHECK_AND_RETURN_RET_LOG(*window != nullptr, AV_ERR_INVALID_VAL, "Video encoder get surface failed!");
    videoEncObj->isInputSurfaceMode_ = true;

    return AV_ERR_OK;
}

OH_AVFormat *OH_VideoEncoder_GetOutputDescription(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, nullptr,
        "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video encoder is nullptr!");

    Format format;
    int32_t ret = videoEncObj->videoEncoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Video encoder get output description failed!");

    videoEncObj->outputFormat_ = new(std::nothrow) OH_AVFormat(format);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->outputFormat_ != nullptr, nullptr,
        "Video encoder get output description failed!");

    return reinterpret_cast<OH_AVFormat *>(videoEncObj->outputFormat_.GetRefPtr());
}

OH_AVErrCode OH_VideoEncoder_FreeOutputData(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "Video encoder free output data failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_NotifyEndOfStream(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->NotifyEos();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder notify end of stream failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_SetParameter(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "Format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::AVCODEC_MAGIC_FORMAT, AV_ERR_INVALID_VAL,
                             "Format magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder set parameter failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_SetCallback(
    struct OH_AVCodec *codec, struct OH_AVCodecAsyncCallback callback, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(callback.onError != nullptr,
        AV_ERR_INVALID_VAL, "Callback onError is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback.onNeedInputData != nullptr,
        AV_ERR_INVALID_VAL, "Callback onNeedInputData is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback.onNeedOutputData != nullptr,
        AV_ERR_INVALID_VAL, "Callback onNeedOutputData is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback.onStreamChanged != nullptr,
        AV_ERR_INVALID_VAL, "Callback onStreamChanged is nullptr");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    videoEncObj->callback_ = std::make_shared<NativeVideoEncoderCallback>(codec, callback, userData);

    int32_t ret = videoEncObj->videoEncoder_->SetCallback(videoEncObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder set callback failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_PushInputData(struct OH_AVCodec *codec, uint32_t index, OH_AVCodecBufferAttr attr)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(attr.size >= 0, AV_ERR_INVALID_VAL, "Invalid buffer size!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    if (attr.flags != AVCODEC_BUFFER_FLAGS_CODEC_DATA) {
        if (videoEncObj->isFirstFrameIn_) {
            AVCodecTrace::TraceBegin("OH::FirstFrame", attr.pts);
            videoEncObj->isFirstFrameIn_ = false;
        } else {
            AVCodecTrace::TraceBegin("OH::Frame", attr.pts);
        }
    }

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.pts;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    enum AVCodecBufferFlag bufferFlag = static_cast<enum AVCodecBufferFlag>(attr.flags);

    int32_t ret = videoEncObj->videoEncoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder push input data failed!");
    if (bufferFlag == AVCODEC_BUFFER_FLAG_EOS) {
        videoEncObj->isEOS_.store(true);
    }

    return AV_ERR_OK;
}

OH_AVFormat *OH_VideoEncoder_GetInputDescription(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, nullptr,
        "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video encoder is nullptr!");

    Format format;
    int32_t ret = videoEncObj->videoEncoder_->GetInputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Video encoder get input description failed!");

    videoEncObj->inputputFormat_ = new(std::nothrow) OH_AVFormat(format);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->inputputFormat_ != nullptr, nullptr,
        "Video encoder get input description failed!");

    return reinterpret_cast<OH_AVFormat *>(videoEncObj->inputputFormat_.GetRefPtr());
}

OH_AVErrCode OH_VideoEncoder_IsValid(OH_AVCodec *codec, bool *isValid)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(isValid != nullptr, AV_ERR_INVALID_VAL, "Input isValid is nullptr!");
    *isValid = true;
    return AV_ERR_OK;
}