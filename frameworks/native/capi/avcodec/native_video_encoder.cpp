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
#include <queue>
#include <shared_mutex>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_trace.h"
#include "avcodec_video_encoder.h"
#include "buffer/avsharedmemory.h"
#include "buffer_utils.h"
#include "common/native_mfmagic.h"
#include "native_avbuffer.h"
#include "native_avcodec_base.h"
#include "native_avcodec_videoencoder.h"
#include "native_avmagic.h"
#include "native_window.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "NativeVideoEncoder"};
constexpr size_t MAX_TEMPNUM = 64;
} // namespace

using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
class NativeVideoEncoderCallback;
class VideoEncoderCallback;

struct VideoEncoderObject : public OH_AVCodec {
    explicit VideoEncoderObject(const std::shared_ptr<AVCodecVideoEncoder> &encoder)
        : OH_AVCodec(AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER), videoEncoder_(encoder)
    {
    }
    ~VideoEncoderObject() = default;

    void ClearBufferList();
    void StopCallback();
    void FormatToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVFormat>> &tempMap);
    void BufferToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> &tempMap);
    void MemoryToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> &tempMap);

    const std::shared_ptr<AVCodecVideoEncoder> videoEncoder_;
    std::queue<OHOS::sptr<MFObjectMagic>> tempList_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVFormat>> inputFormatMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> outputMemoryMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> inputMemoryMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> outputBufferMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> inputBufferMap_;
    std::shared_ptr<NativeVideoEncoderCallback> callback_ = nullptr;
    bool isSetMemoryCallback_ = false;
    bool isSetBufferCallback_ = false;
    std::atomic<bool> isEOS_ = false;
    bool isInputSurfaceMode_ = false;
    std::shared_mutex objListMutex_;
};

class NativeVideoEncoderCallback : public AVCodecCallback,
                                   public MediaCodecCallback,
                                   public MediaCodecParameterCallback {
public:
    NativeVideoEncoderCallback(OH_AVCodec *codec, struct OH_AVCodecAsyncCallback cb, void *userData)
        : codec_(codec), asyncCallback_(cb), userData_(userData)
    {
    }

    NativeVideoEncoderCallback(OH_AVCodec *codec, OH_VideoEncoder_OnNeedInputParameter onInputParameter, void *userData)
        : codec_(codec), onInputParameter_(onInputParameter), userData_(userData)
    {
    }

    NativeVideoEncoderCallback(OH_AVCodec *codec, struct OH_AVCodecCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData)
    {
    }
    virtual ~NativeVideoEncoderCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        (void)errorType;

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, "Codec magic error!");
        CHECK_AND_RETURN_LOG(asyncCallback_.onError != nullptr || callback_.onError != nullptr, "Callback is nullptr");
        int32_t extErr = AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(errorCode));
        if (asyncCallback_.onError != nullptr) {
            asyncCallback_.onError(codec_, extErr, userData_);
        } else if (callback_.onError != nullptr) {
            callback_.onError(codec_, extErr, userData_);
        }
    }

    void OnOutputFormatChanged(const Format &format) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, "Codec magic error!");
        CHECK_AND_RETURN_LOG(asyncCallback_.onStreamChanged != nullptr || callback_.onStreamChanged != nullptr,
                             "Callback is nullptr");
        OHOS::sptr<OH_AVFormat> object = new (std::nothrow) OH_AVFormat(format);
        CHECK_AND_RETURN_LOG(object != nullptr, "OH_AVFormat create failed");
        // The object lifecycle is controlled by the current function stack
        if (asyncCallback_.onStreamChanged != nullptr) {
            asyncCallback_.onStreamChanged(codec_, reinterpret_cast<OH_AVFormat *>(object.GetRefPtr()), userData_);
        } else if (callback_.onStreamChanged != nullptr) {
            callback_.onStreamChanged(codec_, reinterpret_cast<OH_AVFormat *>(object.GetRefPtr()), userData_);
        }
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, "Codec magic error!");
        CHECK_AND_RETURN_LOG(asyncCallback_.onNeedInputData != nullptr, "Callback is nullptr");

        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoEncObj->videoEncoder_ != nullptr, "Context video encoder is nullptr!");

        if (videoEncObj->isEOS_.load() || videoEncObj->isInputSurfaceMode_) {
            AVCODEC_LOGD("At eos or surface mode, no buffer available");
            return;
        }
        OH_AVMemory *data = GetTransData(codec_, index, buffer, false);
        asyncCallback_.onNeedInputData(codec_, index, data, userData_);
    }

    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, "Codec magic error!");
        CHECK_AND_RETURN_LOG(asyncCallback_.onNeedOutputData != nullptr, "Callback is nullptr");

        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoEncObj->videoEncoder_ != nullptr, "Context video encoder is nullptr!");

        struct OH_AVCodecBufferAttr bufferAttr {
            info.presentationTimeUs, info.size, info.offset, flag
        };
        // The bufferInfo lifecycle is controlled by the current function stack
        OH_AVMemory *data = GetTransData(codec_, index, buffer, true);

        if (!((flag == AVCODEC_BUFFER_FLAG_CODEC_DATA) || (flag == AVCODEC_BUFFER_FLAG_EOS))) {
            AVCodecTrace::TraceEnd("OH::Frame", info.presentationTimeUs);
        }
        asyncCallback_.onNeedOutputData(codec_, index, data, &bufferAttr, userData_);
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, "Codec magic error!");
        CHECK_AND_RETURN_LOG(callback_.onNeedInputBuffer != nullptr, "Callback is nullptr");

        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoEncObj->videoEncoder_ != nullptr, "Context video encoder is nullptr!");

        if (videoEncObj->isEOS_.load() || videoEncObj->isInputSurfaceMode_) {
            AVCODEC_LOGD("At eos or surface mode, no buffer available");
            return;
        }
        OH_AVBuffer *data = GetTransData(codec_, index, buffer, false);
        callback_.onNeedInputBuffer(codec_, index, data, userData_);
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, "Codec magic error!");
        CHECK_AND_RETURN_LOG(callback_.onNewOutputBuffer != nullptr, "Callback is nullptr");

        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoEncObj->videoEncoder_ != nullptr, "Video encoder is nullptr!");

        OH_AVBuffer *data = GetTransData(codec_, index, buffer, true);

        if (!((buffer->flag_ == AVCODEC_BUFFER_FLAG_CODEC_DATA) || (buffer->flag_ == AVCODEC_BUFFER_FLAG_EOS))) {
            AVCodecTrace::TraceEnd("OH::Frame", buffer->pts_);
        }
        callback_.onNewOutputBuffer(codec_, index, data, userData_);
    }

    void OnInputParameterAvailable(uint32_t index, std::shared_ptr<Format> parameter) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr!");
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, "Codec magic error!");
        CHECK_AND_RETURN_LOG(onInputParameter_ != nullptr, "Callback is nullptr");

        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoEncObj->videoEncoder_ != nullptr, "Video encoder is nullptr!");
        if (videoEncObj->isEOS_.load()) {
            AVCODEC_LOGD("At eos state, no buffer available");
            return;
        }
        OH_AVFormat *data = GetTransData(codec_, index, parameter);
        onInputParameter_(codec_, index, data, userData_);
    }

    void StopCallback()
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        codec_ = nullptr;
    }

    void UpdateCallback(struct OH_AVCodecAsyncCallback cb, void *userData)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        userData_ = userData;
        asyncCallback_ = cb;
    }

    void UpdateCallback(OH_VideoEncoder_OnNeedInputParameter onInputParameter, void *userData)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        userData_ = userData;
        onInputParameter_ = onInputParameter;
    }

    void UpdateCallback(struct OH_AVCodecCallback cb, void *userData)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        userData_ = userData;
        callback_ = cb;
    }

private:
    OH_AVMemory *GetTransData(struct OH_AVCodec *codec, uint32_t &index, std::shared_ptr<AVSharedMemory> &memory,
                              bool isOutput)
    {
        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
        auto &memoryMap = isOutput ? videoEncObj->outputMemoryMap_ : videoEncObj->inputMemoryMap_;
        {
            std::shared_lock<std::shared_mutex> lock(videoEncObj->objListMutex_);
            auto iter = memoryMap.find(index);
            if (iter != memoryMap.end() && iter->second->IsEqualMemory(memory)) {
                return reinterpret_cast<OH_AVMemory *>(iter->second.GetRefPtr());
            }
        }

        OHOS::sptr<OH_AVMemory> object = new (std::nothrow) OH_AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "AV memory create failed");

        std::lock_guard<std::shared_mutex> lock(videoEncObj->objListMutex_);
        auto iterAndRet = memoryMap.emplace(index, object);
        if (!iterAndRet.second) {
            auto &temp = iterAndRet.first->second;
            temp->magic_ = MFMagic::MFMAGIC_UNKNOWN;
            temp->memory_ = nullptr;
            videoEncObj->tempList_.push(std::move(temp));
            iterAndRet.first->second = object;
            if (videoEncObj->tempList_.size() > MAX_TEMPNUM) {
                videoEncObj->tempList_.pop();
            }
        }
        return reinterpret_cast<OH_AVMemory *>(object.GetRefPtr());
    }

    OH_AVBuffer *GetTransData(struct OH_AVCodec *codec, uint32_t index, std::shared_ptr<AVBuffer> &buffer,
                              bool isOutput)
    {
        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
        auto &bufferMap = isOutput ? videoEncObj->outputBufferMap_ : videoEncObj->inputBufferMap_;
        {
            std::shared_lock<std::shared_mutex> lock(videoEncObj->objListMutex_);
            auto iter = bufferMap.find(index);
            if (iter != bufferMap.end() && iter->second->IsEqualBuffer(buffer)) {
                return reinterpret_cast<OH_AVBuffer *>(iter->second.GetRefPtr());
            }
        }

        OHOS::sptr<OH_AVBuffer> object = new (std::nothrow) OH_AVBuffer(buffer);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVBuffer");

        std::lock_guard<std::shared_mutex> lock(videoEncObj->objListMutex_);
        auto iterAndRet = bufferMap.emplace(index, object);
        if (!iterAndRet.second) {
            auto &temp = iterAndRet.first->second;
            temp->magic_ = MFMagic::MFMAGIC_UNKNOWN;
            temp->buffer_ = nullptr;
            videoEncObj->tempList_.push(std::move(temp));
            iterAndRet.first->second = object;
            if (videoEncObj->tempList_.size() > MAX_TEMPNUM) {
                videoEncObj->tempList_.pop();
            }
        }
        return reinterpret_cast<OH_AVBuffer *>(object.GetRefPtr());
    }

    OH_AVFormat *GetTransData(struct OH_AVCodec *codec, uint32_t index, std::shared_ptr<Format> &parameter)
    {
        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
        {
            std::shared_lock<std::shared_mutex> lock(videoEncObj->objListMutex_);
            auto iter = videoEncObj->inputFormatMap_.find(index);
            if (iter != videoEncObj->inputFormatMap_.end() && iter->second->format_.GetMeta() == parameter->GetMeta()) {
                return reinterpret_cast<OH_AVFormat *>(iter->second.GetRefPtr());
            }
        }
        OHOS::sptr<OH_AVFormat> object = new (std::nothrow) OH_AVFormat();
        object->format_ = std::move(*parameter);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVFormat");

        std::lock_guard<std::shared_mutex> lock(videoEncObj->objListMutex_);
        auto iterAndRet = videoEncObj->inputFormatMap_.emplace(index, object);
        if (!iterAndRet.second) {
            auto &temp = iterAndRet.first->second;
            temp->magic_ = MFMagic::MFMAGIC_UNKNOWN;
            if (temp->outString_ != nullptr) {
                free(temp->outString_);
                temp->outString_ = nullptr;
            }
            if (temp->dumpInfo_ != nullptr) {
                free(temp->dumpInfo_);
                temp->dumpInfo_ = nullptr;
            }
            temp->format_ = Format();
            videoEncObj->tempList_.push(std::move(temp));
            iterAndRet.first->second = object;
            if (videoEncObj->tempList_.size() > MAX_TEMPNUM) {
                videoEncObj->tempList_.pop();
            }
        }
        return reinterpret_cast<OH_AVFormat *>(object.GetRefPtr());
    }

    struct OH_AVCodec *codec_ = nullptr;
    struct OH_AVCodecAsyncCallback asyncCallback_ = {nullptr, nullptr, nullptr, nullptr};
    struct OH_AVCodecCallback callback_ = {nullptr, nullptr, nullptr, nullptr};
    OH_VideoEncoder_OnNeedInputParameter onInputParameter_ = nullptr;
    void *userData_ = nullptr;
    std::shared_mutex mutex_;
};

void VideoEncoderObject::ClearBufferList()
{
    std::lock_guard<std::shared_mutex> lock(objListMutex_);
    if (isSetBufferCallback_) {
        BufferToTempFunc(inputBufferMap_);
        BufferToTempFunc(outputBufferMap_);
        inputBufferMap_.clear();
        outputBufferMap_.clear();
    } else if (isSetMemoryCallback_) {
        MemoryToTempFunc(inputMemoryMap_);
        MemoryToTempFunc(outputMemoryMap_);
        inputMemoryMap_.clear();
        outputMemoryMap_.clear();
    }
    if (inputFormatMap_.size() > 0) {
        FormatToTempFunc(inputFormatMap_);
        inputFormatMap_.clear();
    }
    while (tempList_.size() > MAX_TEMPNUM) {
        tempList_.pop();
    }
}

void VideoEncoderObject::StopCallback()
{
    if (callback_ == nullptr) {
        return;
    }
    callback_->StopCallback();
}

void VideoEncoderObject::FormatToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVFormat>> &tempMap)
{
    for (auto &val : tempMap) {
        val.second->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        if (val.second->outString_ != nullptr) {
            free(val.second->outString_);
            val.second->outString_ = nullptr;
        }
        if (val.second->dumpInfo_ != nullptr) {
            free(val.second->dumpInfo_);
            val.second->dumpInfo_ = nullptr;
        }
        val.second->format_ = Format();
        tempList_.push(std::move(val.second));
    }
}

void VideoEncoderObject::BufferToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> &tempMap)
{
    for (auto &val : tempMap) {
        val.second->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        val.second->buffer_ = nullptr;
        tempList_.push(std::move(val.second));
    }
}

void VideoEncoderObject::MemoryToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> &tempMap)
{
    for (auto &val : tempMap) {
        val.second->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        val.second->memory_ = nullptr;
        tempList_.push(std::move(val.second));
    }
}

namespace OHOS {
namespace MediaAVCodec {
#ifdef __cplusplus
extern "C" {
#endif
struct OH_AVCodec *OH_VideoEncoder_CreateByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "Mime is nullptr!");

    std::shared_ptr<AVCodecVideoEncoder> videoEncoder = VideoEncoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, nullptr, "Video encoder create by mime failed");

    struct VideoEncoderObject *object = new (std::nothrow) VideoEncoderObject(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Video encoder create by mime failed");

    return object;
}

struct OH_AVCodec *OH_VideoEncoder_CreateByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "Name is nullptr!");

    std::shared_ptr<AVCodecVideoEncoder> videoEncoder = VideoEncoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, nullptr, "Video encoder create by name failed");

    struct VideoEncoderObject *object = new (std::nothrow) VideoEncoderObject(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Video encoder create by name failed");

    return object;
}

OH_AVErrCode OH_VideoEncoder_Destroy(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);

    if (videoEncObj != nullptr && videoEncObj->videoEncoder_ != nullptr) {
        int32_t ret = videoEncObj->videoEncoder_->Release();
        videoEncObj->StopCallback();
        videoEncObj->ClearBufferList();
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
    CHECK_AND_RETURN_RET_LOG(format->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL, "Format magic error!");

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

    int32_t ret = videoEncObj->videoEncoder_->Stop();
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Video encoder stop failed");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    videoEncObj->ClearBufferList();
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Flush(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Flush();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder flush failed!");
    videoEncObj->ClearBufferList();
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Reset(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Reset();
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Video encoder reset failed");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    videoEncObj->ClearBufferList();
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
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, AV_ERR_OPERATE_NOT_PERMIT, "Video encoder get surface failed!");

    *window = CreateNativeWindowFromSurface(&surface);
    CHECK_AND_RETURN_RET_LOG(*window != nullptr, AV_ERR_INVALID_VAL, "Video encoder get surface failed!");
    videoEncObj->isInputSurfaceMode_ = true;

    return AV_ERR_OK;
}

OH_AVFormat *OH_VideoEncoder_GetOutputDescription(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, nullptr, "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video encoder is nullptr!");

    Format format;
    int32_t ret = videoEncObj->videoEncoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Video encoder get output description failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Video encoder get output description failed!");
    avFormat->format_ = format;

    return avFormat;
}

OH_AVErrCode OH_VideoEncoder_FreeOutputData(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");
    CHECK_AND_RETURN_RET_LOG(videoEncObj->callback_ != nullptr, AV_ERR_INVALID_STATE,
                             "The callback of OH_AVMemory is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder free output data failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_FreeOutputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");
    CHECK_AND_RETURN_RET_LOG(videoEncObj->isSetBufferCallback_, AV_ERR_INVALID_STATE,
                             "The callback of OH_AVBuffer is nullptr!");

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

    videoEncObj->isEOS_.store(true);
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_SetParameter(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "Format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL, "Format magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder set parameter failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_SetCallback(struct OH_AVCodec *codec, struct OH_AVCodecAsyncCallback callback,
                                         void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    if (videoEncObj->callback_ != nullptr) {
        videoEncObj->callback_->UpdateCallback(callback, userData);
    } else {
        videoEncObj->callback_ = std::make_shared<NativeVideoEncoderCallback>(codec, callback, userData);
    }
    int32_t ret =
        videoEncObj->videoEncoder_->SetCallback(std::static_pointer_cast<AVCodecCallback>(videoEncObj->callback_));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder set callback failed!");
    videoEncObj->isSetMemoryCallback_ = true;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_RegisterCallback(struct OH_AVCodec *codec, struct OH_AVCodecCallback callback,
                                              void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(callback.onNewOutputBuffer != nullptr, AV_ERR_INVALID_VAL,
                             "Callback onNewOutputBuffer is nullptr");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    if (videoEncObj->callback_ != nullptr) {
        videoEncObj->callback_->UpdateCallback(callback, userData);
    } else {
        videoEncObj->callback_ = std::make_shared<NativeVideoEncoderCallback>(codec, callback, userData);
    }
    int32_t ret =
        videoEncObj->videoEncoder_->SetCallback(std::static_pointer_cast<MediaCodecCallback>(videoEncObj->callback_));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder register callback failed!");
    videoEncObj->isSetBufferCallback_ = true;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_RegisterParameterCallback(OH_AVCodec *codec,
                                                       OH_VideoEncoder_OnNeedInputParameter onInputParameter,
                                                       void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(onInputParameter != nullptr, AV_ERR_INVALID_VAL, "Callback onInputParameter is nullptr");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");

    if (videoEncObj->callback_ != nullptr) {
        videoEncObj->callback_->UpdateCallback(onInputParameter, userData);
    } else {
        videoEncObj->callback_ = std::make_shared<NativeVideoEncoderCallback>(codec, onInputParameter, userData);
    }
    int32_t ret = videoEncObj->videoEncoder_->SetCallback(
        std::static_pointer_cast<MediaCodecParameterCallback>(videoEncObj->callback_));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder register parameter callback failed!");
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
    CHECK_AND_RETURN_RET_LOG(videoEncObj->callback_ != nullptr, AV_ERR_INVALID_STATE,
                             "The callback of OH_AVMemory is nullptr!");

    if (!((attr.flags == AVCODEC_BUFFER_FLAG_CODEC_DATA) || (attr.flags == AVCODEC_BUFFER_FLAG_EOS))) {
        AVCodecTrace::TraceBegin("OH::Frame", attr.pts);
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

OH_AVErrCode OH_VideoEncoder_PushInputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");
    CHECK_AND_RETURN_RET_LOG(videoEncObj->isSetBufferCallback_, AV_ERR_INVALID_STATE,
                             "The callback of OH_AVBuffer is nullptr!");

    {
        std::shared_lock<std::shared_mutex> lock(videoEncObj->objListMutex_);
        auto bufferIter = videoEncObj->inputBufferMap_.find(index);
        CHECK_AND_RETURN_RET_LOG(bufferIter != videoEncObj->inputBufferMap_.end(), AV_ERR_INVALID_VAL,
                                 "Invalid buffer index");
        auto buffer = bufferIter->second->buffer_;
        if (buffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
            videoEncObj->isEOS_.store(true);
            AVCODEC_LOGD("Set eos status to true");
        }
        if (!((buffer->flag_ == AVCODEC_BUFFER_FLAG_CODEC_DATA) || (buffer->flag_ == AVCODEC_BUFFER_FLAG_EOS))) {
            AVCodecTrace::TraceBegin("OH::Frame", buffer->pts_);
        }
    }

    int32_t ret = videoEncObj->videoEncoder_->QueueInputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "videoEncoder QueueInputBuffer failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_PushInputParameter(OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->QueueInputParameter(index);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "videoEncoder QueueInputParameter failed!");
    return AV_ERR_OK;
}

OH_AVFormat *OH_VideoEncoder_GetInputDescription(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, nullptr, "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video encoder is nullptr!");

    Format format;
    int32_t ret = videoEncObj->videoEncoder_->GetInputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Video encoder get input description failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Video encoder get input description failed!");
    avFormat->format_ = format;

    return avFormat;
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
}
} // namespace MediaAVCodec
} // namespace OHOS