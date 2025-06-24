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

using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
class NativeVideoEncoderCallback;

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
    OH_AVBuffer *GetTransData(const uint32_t &index, std::shared_ptr<AVBuffer> &buffer, bool isOutput);
    OH_AVMemory *GetTransData(const uint32_t &index, std::shared_ptr<AVSharedMemory> &memory, bool isOutput);
    OH_AVFormat *GetTransData(const uint32_t &index, std::shared_ptr<Format> &format);

    const std::shared_ptr<AVCodecVideoEncoder> videoEncoder_;
    std::queue<OHOS::sptr<MFObjectMagic>> tempList_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVFormat>> inputFormatMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> outputMemoryMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> inputMemoryMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> outputBufferMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> inputBufferMap_;
    std::shared_ptr<NativeVideoEncoderCallback> callback_ = nullptr;
    bool isSetMemoryCallback_ = false;
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
        : codec_(codec), onInputParameter_(onInputParameter), paramUserData_(userData)
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

        if (videoEncObj->isInputSurfaceMode_) {
            AVCODEC_LOGD("At surface mode, no buffer available");
            return;
        }
        OH_AVMemory *data = videoEncObj->GetTransData(index, buffer, false);
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
        OH_AVMemory *data = videoEncObj->GetTransData(index, buffer, true);

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

        OH_AVBuffer *data = videoEncObj->GetTransData(index, buffer, false);
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

        OH_AVBuffer *data = videoEncObj->GetTransData(index, buffer, true);

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

        OH_AVFormat *data = videoEncObj->GetTransData(index, parameter);
        onInputParameter_(codec_, index, data, paramUserData_);
    }

    void StopCallback()
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        codec_ = nullptr;
    }

    void UpdateCallback(const struct OH_AVCodecAsyncCallback &cb, void *userData)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        userData_ = userData;
        asyncCallback_ = cb;
    }

    void UpdateCallback(const OH_VideoEncoder_OnNeedInputParameter &onInputParameter, void *userData)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        paramUserData_ = userData;
        onInputParameter_ = onInputParameter;
    }

    void UpdateCallback(const struct OH_AVCodecCallback &cb, void *userData)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        userData_ = userData;
        callback_ = cb;
    }

private:
    struct OH_AVCodec *codec_ = nullptr;
    struct OH_AVCodecAsyncCallback asyncCallback_ = {nullptr, nullptr, nullptr, nullptr};
    struct OH_AVCodecCallback callback_ = {nullptr, nullptr, nullptr, nullptr};
    OH_VideoEncoder_OnNeedInputParameter onInputParameter_ = nullptr;
    void *userData_ = nullptr;
    void *paramUserData_ = nullptr;
    std::shared_mutex mutex_;
};

OH_AVMemory *VideoEncoderObject::GetTransData(const uint32_t &index, std::shared_ptr<AVSharedMemory> &memory,
                                              bool isOutput)
{
    auto &memoryMap = isOutput ? this->outputMemoryMap_ : this->inputMemoryMap_;
    {
        std::shared_lock<std::shared_mutex> lock(this->objListMutex_);
        auto iter = memoryMap.find(index);
        if (iter != memoryMap.end() && iter->second->IsEqualMemory(memory)) {
            return reinterpret_cast<OH_AVMemory *>(iter->second.GetRefPtr());
        }
    }
    OHOS::sptr<OH_AVMemory> object = new (std::nothrow) OH_AVMemory(memory);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "AV memory create failed");

    std::lock_guard<std::shared_mutex> lock(this->objListMutex_);
    auto iterAndRet = memoryMap.emplace(index, object);
    if (!iterAndRet.second) {
        auto &temp = iterAndRet.first->second;
        temp->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        temp->memory_ = nullptr;
        this->tempList_.push(std::move(temp));
        iterAndRet.first->second = object;
        if (this->tempList_.size() > MAX_TEMPNUM) {
            this->tempList_.pop();
        }
    }
    return reinterpret_cast<OH_AVMemory *>(object.GetRefPtr());
}

OH_AVBuffer *VideoEncoderObject::GetTransData(const uint32_t &index, std::shared_ptr<AVBuffer> &buffer, bool isOutput)
{
    auto &bufferMap = isOutput ? this->outputBufferMap_ : this->inputBufferMap_;
    {
        std::shared_lock<std::shared_mutex> lock(this->objListMutex_);
        auto iter = bufferMap.find(index);
        if (iter != bufferMap.end() && iter->second->IsEqualBuffer(buffer)) {
            return reinterpret_cast<OH_AVBuffer *>(iter->second.GetRefPtr());
        }
    }
    OHOS::sptr<OH_AVBuffer> object = new (std::nothrow) OH_AVBuffer(buffer);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVBuffer");

    std::lock_guard<std::shared_mutex> lock(this->objListMutex_);
    auto iterAndRet = bufferMap.emplace(index, object);
    if (!iterAndRet.second) {
        auto &temp = iterAndRet.first->second;
        temp->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        temp->buffer_ = nullptr;
        this->tempList_.push(std::move(temp));
        iterAndRet.first->second = object;
        if (this->tempList_.size() > MAX_TEMPNUM) {
            this->tempList_.pop();
        }
    }
    return reinterpret_cast<OH_AVBuffer *>(object.GetRefPtr());
}

OH_AVFormat *VideoEncoderObject::GetTransData(const uint32_t &index, std::shared_ptr<Format> &format)
{
    {
        std::shared_lock<std::shared_mutex> lock(this->objListMutex_);
        auto iter = this->inputFormatMap_.find(index);
        if (iter != this->inputFormatMap_.end() && iter->second->format_.GetMeta() == format->GetMeta()) {
            return reinterpret_cast<OH_AVFormat *>(iter->second.GetRefPtr());
        }
    }
    OHOS::sptr<OH_AVFormat> object = new (std::nothrow) OH_AVFormat();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVFormat");
    object->format_.SetMetaPtr(format->GetMeta());

    std::lock_guard<std::shared_mutex> lock(this->objListMutex_);
    auto iterAndRet = this->inputFormatMap_.emplace(index, object);
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
        this->tempList_.push(std::move(temp));
        iterAndRet.first->second = object;
        if (this->tempList_.size() > MAX_TEMPNUM) {
            this->tempList_.pop();
        }
    }
    return reinterpret_cast<OH_AVFormat *>(object.GetRefPtr());
}

void VideoEncoderObject::ClearBufferList()
{
    std::lock_guard<std::shared_mutex> lock(objListMutex_);
    if (inputBufferMap_.size() > 0) {
        BufferToTempFunc(inputBufferMap_);
        inputBufferMap_.clear();
    }
    if (outputBufferMap_.size() > 0) {
        BufferToTempFunc(outputBufferMap_);
        outputBufferMap_.clear();
    }
    if (inputMemoryMap_.size() > 0) {
        MemoryToTempFunc(inputMemoryMap_);
        inputMemoryMap_.clear();
    }
    if (outputMemoryMap_.size() > 0) {
        MemoryToTempFunc(outputMemoryMap_);
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
} // namespace

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

    int32_t bitrateMode = -1;
    if (OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, &bitrateMode) &&
        bitrateMode == SQR) {
        int64_t bitrate;
        int64_t maxBitrate;
        bool bitrateExist = OH_AVFormat_GetLongValue(format, OH_MD_KEY_BITRATE, &bitrate);
        bool maxBitrateExist = OH_AVFormat_GetLongValue(format, OH_MD_KEY_MAX_BITRATE, &maxBitrate);
        if (bitrateExist && !maxBitrateExist) {
            AVCODEC_LOGW("In SQR bitrate mode, param %{public}s is not set, param %{public}s will be used instead",
                OH_MD_KEY_MAX_BITRATE, OH_MD_KEY_BITRATE);
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_MAX_BITRATE, bitrate);
        }
    }

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
    CHECK_AND_RETURN_RET_LOG(videoEncObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
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
    CHECK_AND_RETURN_RET_LOG(!videoEncObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
                             "Not support the callback of OH_AVMemory!");

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
    CHECK_AND_RETURN_RET_LOG(videoEncObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
                             "The callback of OH_AVMemory is nullptr!");

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.pts;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    enum AVCodecBufferFlag bufferFlag = static_cast<enum AVCodecBufferFlag>(attr.flags);

    int32_t ret = videoEncObj->videoEncoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video encoder push input data failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_PushInputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");
    CHECK_AND_RETURN_RET_LOG(!videoEncObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
                             "Not support the callback of OH_AVMemory!");
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

OH_AVErrCode OH_VideoEncoder_QueryInputBuffer(struct OH_AVCodec *codec, uint32_t *index, int64_t timeoutUs)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");
    int32_t ret = videoEncObj->videoEncoder_->QueryInputBuffer(*index, timeoutUs);
    switch (ret) {
        case AVCS_ERR_TRY_AGAIN:
            return AV_ERR_TRY_AGAIN_LATER;
        case AVCS_ERR_OK:
            return AV_ERR_OK;
        default:
            AVCODEC_LOGE("Video encoder query input data failed!");
    }
    return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
}

OH_AVErrCode OH_VideoEncoder_QueryOutputBuffer(struct OH_AVCodec *codec, uint32_t *index, int64_t timeoutUs)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "Video encoder is nullptr!");
    int32_t ret = videoEncObj->videoEncoder_->QueryOutputBuffer(*index, timeoutUs);
    switch (ret) {
        case AVCS_ERR_TRY_AGAIN:
            return AV_ERR_TRY_AGAIN_LATER;
        case AVCS_ERR_OK:
            return AV_ERR_OK;
        default:
            AVCODEC_LOGE("Video encoder query output data failed!");
    }
    return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
}

OH_AVBuffer *OH_VideoEncoder_GetInputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, nullptr, "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video encoder is nullptr!");

    std::shared_ptr<AVBuffer> buffer = videoEncObj->videoEncoder_->GetInputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "Buffer is nullptr, idx:%{public}u", index);

    return videoEncObj->GetTransData(index, buffer, false);
}

OH_AVBuffer *OH_VideoEncoder_GetOutputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER, nullptr, "Codec magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "Video encoder is nullptr!");

    std::shared_ptr<AVBuffer> buffer = videoEncObj->videoEncoder_->GetOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "Buffer is nullptr, idx:%{public}u", index);

    return videoEncObj->GetTransData(index, buffer, true);
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