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

#include "native_video_codec.h"
#include "avbuffer.h"
#include "avcodec_dfx.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_video_codec.h"
#include "native_avcodec_base.h"
#include "native_avmagic.h"
#include "native_window.h"
#include <list>
#include <mutex>
#include <shared_mutex>

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeVideoCodec"};
}

using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
class NativeVideoCodecCallback;

struct VideoCodecObject : public OH_AVCodec {
    explicit VideoCodecObject(const std::shared_ptr<VideoCodec> &codec)
        : OH_AVCodec(AVMagic::AVCODEC_MAGIC_VIDEO_CODEC), videoCodec_(codec)
    {
    }
    ~VideoCodecObject() = default;

    const std::shared_ptr<VideoCodec> videoCodec_;
    std::list<OHOS::sptr<OH_AVBuffer>> bufferObjList_;
    std::shared_ptr<NativeVideoCodecCallback> callback_ = nullptr;
    std::atomic<bool> isFlushing_ = false;
    std::atomic<bool> isFlushed_ = false;
    std::atomic<bool> isStop_ = false;
    std::atomic<bool> isEOS_ = false;
    bool isInputSurfaceMode_ = false;
    bool isOutputSurfaceMode_ = false;
    std::atomic<bool> isFirstFrameIn_ = true;
    std::atomic<bool> isFirstFrameOut_ = true;
    std::shared_mutex bufferObjListMutex_;
};

class NativeVideoCodecCallback : public VideoCodecCallback {
public:
    NativeVideoCodecCallback(OH_AVCodec *codec, struct OH_AVCodecCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData)
    {
    }
    virtual ~NativeVideoCodecCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        (void)errorType;

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(callback_.onError != nullptr, "Callback is nullptr");
        int32_t extErr = AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(errorCode));
        callback_.onError(codec_, extErr, userData_);
    }

    void OnOutputFormatChanged(const Format &format) override
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(callback_.onStreamChanged != nullptr, "Callback is nullptr");
        OHOS::sptr<OH_AVFormat> object = new (std::nothrow) OH_AVFormat(format);
        callback_.onStreamChanged(codec_, reinterpret_cast<OH_AVFormat *>(object.GetRefPtr()), userData_);
    }

    void SurfaceModeOnBufferFilled(std::shared_ptr<AVBuffer> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(callback_.onBufferFilled != nullptr, "Callback is nullptr");

        struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoCodecObj->videoCodec_ != nullptr, "Video codec is nullptr!");

        if (videoCodecObj->isFlushing_.load() || videoCodecObj->isFlushed_.load() || videoCodecObj->isStop_.load() ||
            videoCodecObj->isEOS_.load()) {
            AVCODEC_LOGD("At flush or stop, ignore");
            return;
        }
        int64_t pts = buffer->pts_;
        int32_t flag = buffer->flag_;
        struct OH_AVCodecBufferAttr bufferAttr {
            pts, buffer->memory_->GetSize(), buffer->memory_->GetOffset(), flag
        };
        OH_AVBuffer *data = nullptr;
        data = GetOutputData(codec_, buffer);
        callback_.onBufferFilled(codec_, index, data, &bufferAttr, userData_);
    }

    void StopCallback()
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        codec_ = nullptr;
    }

private:
    OH_AVBuffer *GetTransData(struct OH_AVCodec **codec, std::shared_ptr<AVBuffer> buffer)
    {
        CHECK_AND_RETURN_RET_LOG((*codec) != nullptr, nullptr, "Codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG((*codec)->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, nullptr,
                                 "Codec magic error!");
        struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(*codec);
        CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, nullptr, "Video codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "Buffer is nullptr, get buffer failed!");
        {
            std::shared_lock<std::shared_mutex> lock(videoCodecObj->memoryObjListMutex_);
            for (auto &memoryObj : videoCodecObj->bufferObjList_) {
                if (memoryObj->IsEqualMemory(buffer)) {
                    return reinterpret_cast<OH_AVBuffer *>(memoryObj.GetRefPtr());
                }
            }
        }

        OHOS::sptr<OH_AVBuffer> object = new (std::nothrow) OH_AVBuffer(buffer);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "AV buffer create failed");

        std::lock_guard<std::shared_mutex> lock(videoCodecObj->memoryObjListMutex_);
        videoCodecObj->bufferObjList_.push_back(object);
        return reinterpret_cast<OH_AVBuffer *>(object.GetRefPtr());
    }

    OH_AVBuffer *GetOutputData(struct OH_AVCodec *codec, std::shared_ptr<AVBuffer> buffer)
    {
        return GetTransData(&codec, buffer, false);
    }

    struct OH_AVCodec *codec_;
    struct OH_AVCodecCallback callback_;
    void *userData_;
    std::shared_mutex mutex_;
};

OH_AVCodec *OH_VideoCodec_CreateByMime(const char *mime, bool isEncoder)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "Mime is nullptr!");

    std::shared_ptr<VideoCodec> videoCodec = VideoCodecFactory::CreateByMime(isEncoder, mime);
    CHECK_AND_RETURN_RET_LOG(videoCodec != nullptr, nullptr, "Video codec create by mime failed!");

    struct VideoCodecObject *object = new (std::nothrow) VideoCodecObject(videoCodec);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Video codec create by mime failed!");
    return object;
}

OH_AVCodec *OH_VideoCodec_CreateByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "Name is nullptr!");

    std::shared_ptr<VideoCodec> videoCodec = VideoCodecFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(videoCodec != nullptr, nullptr, "Video codec create by name failed!");

    struct VideoCodecObject *object = new (std::nothrow) VideoCodecObject(videoCodec);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Video codec create by name failed!");

    return object;
}

OH_AVErrCode OH_VideoCodec_Destroy(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);

    if (videoCodecObj != nullptr && videoCodecObj->videoCodec_ != nullptr) {
        if (videoCodecObj->callback_ != nullptr) {
            videoCodecObj->callback_->StopCallback();
        }
        {
            std::lock_guard<std::shared_mutex> lock(videoCodecObj->memoryObjListMutex_);
            videoCodecObj->memoryObjList_.clear();
        }
        videoCodecObj->isStop_.store(false);
        int32_t ret = videoCodecObj->videoCodec_->Release();
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGE("Video codec destroy failed!");
            delete codec;
            return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
        }
    } else {
        AVCODEC_LOGD("Video codec is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoCodec_SetCallback(OH_AVCodec *codec, OH_AVCodecCallback callback, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");

    videoCodecObj->callback_ = std::make_shared<NativeVideoCodecCallback>(codec, callback, userData);

    int32_t ret = videoCodecObj->videoCodec_->SetCallback(videoCodecObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video codec set callback failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoCodec_SetOutputBufferQueue(OH_AVCodec *codec, OH_AVBufferQueueProducer *bufferQueue)
{
    return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_SetSurface(OH_AVCodec *codec, OHNativeWindow *window)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(window != nullptr, AV_ERR_INVALID_VAL, "Window is nullptr!");
    CHECK_AND_RETURN_RET_LOG(window->surface != nullptr, AV_ERR_INVALID_VAL, "Input window surface is nullptr!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");

    int32_t ret = videoCodecObj->videoCodec_->SetOutputSurface(window->surface);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video codec set output surface failed!");
    videoCodecObj->isOutputSurfaceMode_ = true;

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoCodec_CreateSurface(OH_AVCodec *codec, OHNativeWindow **surface)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr && window != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");

    OHOS::sptr<OHOS::Surface> surface = videoEncObj->videoEncoder_->CreateInputSurface();
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, AV_ERR_OPERATE_NOT_PERMIT, "Video codec get surface failed!");

    *window = CreateNativeWindowFromSurface(&surface);
    CHECK_AND_RETURN_RET_LOG(*window != nullptr, AV_ERR_INVALID_VAL, "Video codec get surface failed!");
    videoEncObj->isInputSurfaceMode_ = true;

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoCodec_Configure(OH_AVCodec *codec, OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "Format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::AVCODEC_MAGIC_FORMAT, AV_ERR_INVALID_VAL,
                             "Format magic error!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");

    int32_t ret = videoCodecObj->videoCodec_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video codec configure failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoCodec_Prepare(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");

    int32_t ret = videoCodecObj->videoCodec_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video codec prepare failed!");

    return AV_ERR_OK;
}

OH_AVBufferQueueProducer *OH_VideoCodec_SetOutputBufferQueue(OH_AVCodec *codec)
{
    return nullptr;
}

OH_AVErrCode OH_VideoCodec_Start(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");

    videoCodecObj->isStop_.store(false);
    videoCodecObj->isEOS_.store(false);
    videoCodecObj->isFlushed_.store(false);
    int32_t ret = videoCodecObj->videoCodec_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video codec start failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoCodec_Stop(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");

    videoCodecObj->isStop_.store(true);
    int32_t ret = videoCodecObj->videoCodec_->Stop();
    if (ret != AVCS_ERR_OK) {
        videoCodecObj->isStop_.store(false);
        AVCODEC_LOGE("Video codec stop failed");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    std::lock_guard<std::shared_mutex> lock(videoCodecObj->memoryObjListMutex_);
    videoCodecObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoCodec_Flush(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");

    videoCodecObj->isFlushing_.store(true);
    int32_t ret = videoCodecObj->videoCodec_->Flush();
    videoCodecObj->isFlushed_.store(true);
    videoCodecObj->isFlushing_.store(false);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video codec flush failed!");
    std::lock_guard<std::shared_mutex> lock(videoCodecObj->memoryObjListMutex_);
    videoCodecObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoCodec_Reset(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");

    videoCodecObj->isStop_.store(true);
    int32_t ret = videoCodecObj->videoCodec_->Reset();
    if (ret != AVCS_ERR_OK) {
        videoCodecObj->isStop_.store(false);
        AVCODEC_LOGE("Video codec reset failed!");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    std::lock_guard<std::shared_mutex> lock(videoCodecObj->memoryObjListMutex_);
    videoCodecObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

OH_AVFormat *OH_VideoCodec_GetOutputDescription(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, nullptr, "Codec magic error!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, nullptr, "Video codec is nullptr!");

    Format format;
    int32_t ret = videoCodecObj->videoCodec_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Video codec get output description failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Video codec get output description failed!");
    avFormat->format_ = format;

    return avFormat;
}

OH_AVErrCode OH_VideoCodec_SetParameter(OH_AVCodec *codec, OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "Format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::AVCODEC_MAGIC_FORMAT, AV_ERR_INVALID_VAL,
                             "Format magic error!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");

    int32_t ret = videoCodecObj->videoCodec_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video codec set parameter failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoCodec_NotifyEndOfStream(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");
    videoCodecObj->isEOS_.store(true);
    int32_t ret = videoCodecObj->videoCodec_->NotifyEos();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video codec notify end of stream failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoCodec_SurfaceModeReturnBuffer(OH_AVCodec *codec, OH_AVBuffer *buffer, bool available)
{
    CHECK_AND_RETURN_RET_LOG(isOutputSurfaceMode_ == true, AV_ERR_INVALID_STATE, "Codec is not surface mode!");
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_CODEC, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    struct VideoCodecObject *videoCodecObj = reinterpret_cast<VideoCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoCodecObj->videoCodec_ != nullptr, AV_ERR_INVALID_VAL, "Video codec is nullptr!");

    int32_t ret = videoCodecObj->videoCodec_->SurfaceModeReturnBuffer(buffer, available);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video codec push input data failed!");
    AVCodecBufferFlag flag = buffer->flag_;
    if (bufferFlag == AVCODEC_BUFFER_FLAG_EOS) {
        videoCodecObj->isEOS_.store(true);
    }

    return AV_ERR_OK;
}