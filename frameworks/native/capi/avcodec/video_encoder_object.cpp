/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "video_encoder_object.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "media_core.h"
#include "native_window.h"

namespace {
bool IsValidVideoEncoderMagic(AVMagic magic)
{
    return magic == AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER ||
           magic == AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER ||
           magic == AVMagic::AVCODEC_MAGIC_SECONDARY_VIDEO_ENCODER;
}
}

namespace OHOS {
namespace MediaAVCodec {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "VideoEncoderObject"};
constexpr size_t MAX_TEMPNUM = 64;

VideoEncoderObject::VideoEncoderObject(const std::shared_ptr<AVCodecVideoEncoder>& encoder)
    : OH_AVCodec(AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER), videoEncoder_(encoder)
{
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

void VideoEncoderObject::FormatToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVFormat>>& tempMap)
{
    for (auto& val : tempMap) {
        val.second->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        if (val.second->outString_ != nullptr) {
            free(val.second->outString_);
            val.second->outString_ = nullptr;
        }
        if (val.second->dumpInfo_ != nullptr) {
            free(val.second->dumpInfo_);
            val.second->dumpInfo_ = nullptr;
        }
        val.second->format_ = Media::Format();
        tempList_.push(std::move(val.second));
    }
}

void VideoEncoderObject::BufferToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>>& tempMap)
{
    for (auto& val : tempMap) {
        val.second->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        val.second->buffer_ = nullptr;
        tempList_.push(std::move(val.second));
    }
}

void VideoEncoderObject::MemoryToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>>& tempMap)
{
    for (auto& val : tempMap) {
        val.second->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        val.second->memory_ = nullptr;
        tempList_.push(std::move(val.second));
    }
}

OH_AVMemory* VideoEncoderObject::GetTransData(const uint32_t& index, std::shared_ptr<AVSharedMemory>& memory,
                                              bool isOutput)
{
    auto& memoryMap = isOutput ? outputMemoryMap_ : inputMemoryMap_;
    {
        std::shared_lock<std::shared_mutex> lock(objListMutex_);
        auto iter = memoryMap.find(index);
        if (iter != memoryMap.end() && iter->second->IsEqualMemory(memory)) {
            return reinterpret_cast<OH_AVMemory*>(iter->second.GetRefPtr());
        }
    }
    OHOS::sptr<OH_AVMemory> object = new (std::nothrow) OH_AVMemory(memory);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "AV memory create failed");

    std::lock_guard<std::shared_mutex> lock(objListMutex_);
    auto iterAndRet = memoryMap.emplace(index, object);
    if (!iterAndRet.second) {
        auto& temp = iterAndRet.first->second;
        temp->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        temp->memory_ = nullptr;
        tempList_.push(std::move(temp));
        iterAndRet.first->second = object;
        if (tempList_.size() > MAX_TEMPNUM) {
            tempList_.pop();
        }
    }
    return reinterpret_cast<OH_AVMemory*>(object.GetRefPtr());
}

OH_AVBuffer* VideoEncoderObject::GetTransData(const uint32_t& index, std::shared_ptr<AVBuffer>& buffer, bool isOutput)
{
    auto& bufferMap = isOutput ? outputBufferMap_ : inputBufferMap_;
    {
        std::shared_lock<std::shared_mutex> lock(objListMutex_);
        auto iter = bufferMap.find(index);
        if (iter != bufferMap.end() && iter->second->IsEqualBuffer(buffer)) {
            return reinterpret_cast<OH_AVBuffer*>(iter->second.GetRefPtr());
        }
    }
    OHOS::sptr<OH_AVBuffer> object = new (std::nothrow) OH_AVBuffer(buffer);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVBuffer");

    std::lock_guard<std::shared_mutex> lock(objListMutex_);
    auto iterAndRet = bufferMap.emplace(index, object);
    if (!iterAndRet.second) {
        auto& temp = iterAndRet.first->second;
        temp->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        temp->buffer_ = nullptr;
        tempList_.push(std::move(temp));
        iterAndRet.first->second = object;
        if (tempList_.size() > MAX_TEMPNUM) {
            tempList_.pop();
        }
    }
    return reinterpret_cast<OH_AVBuffer*>(object.GetRefPtr());
}

OH_AVFormat* VideoEncoderObject::GetTransData(const uint32_t& index, std::shared_ptr<Media::Format>& format)
{
    {
        std::shared_lock<std::shared_mutex> lock(objListMutex_);
        auto iter = inputFormatMap_.find(index);
        if (iter != inputFormatMap_.end() && iter->second->format_.GetMeta() == format->GetMeta()) {
            return reinterpret_cast<OH_AVFormat*>(iter->second.GetRefPtr());
        }
    }
    OHOS::sptr<OH_AVFormat> object = new (std::nothrow) OH_AVFormat();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVFormat");
    object->format_.SetMetaPtr(format->GetMeta());

    std::lock_guard<std::shared_mutex> lock(objListMutex_);
    auto iterAndRet = inputFormatMap_.emplace(index, object);
    if (!iterAndRet.second) {
        auto& temp = iterAndRet.first->second;
        temp->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        if (temp->outString_ != nullptr) {
            free(temp->outString_);
            temp->outString_ = nullptr;
        }
        if (temp->dumpInfo_ != nullptr) {
            free(temp->dumpInfo_);
            temp->dumpInfo_ = nullptr;
        }
        temp->format_ = Media::Format();
        tempList_.push(std::move(temp));
        iterAndRet.first->second = object;
        if (tempList_.size() > MAX_TEMPNUM) {
            tempList_.pop();
        }
    }
    return reinterpret_cast<OH_AVFormat*>(object.GetRefPtr());
}

NativeVideoEncoderCallback::NativeVideoEncoderCallback(struct OH_AVCodec* codec,
                                                       struct OH_AVCodecAsyncCallback cb,
                                                       void* userData)
    : codec_(codec), asyncCallback_(cb), userData_(userData)
{
}

NativeVideoEncoderCallback::NativeVideoEncoderCallback(struct OH_AVCodec* codec,
                                                       OH_VideoEncoder_OnNeedInputParameter onInputParameter,
                                                       void* userData)
    : codec_(codec), onInputParameter_(onInputParameter), paramUserData_(userData)
{
}

NativeVideoEncoderCallback::NativeVideoEncoderCallback(struct OH_AVCodec* codec,
                                                       struct OH_AVCodecCallback cb,
                                                       void* userData)
    : codec_(codec), callback_(cb), userData_(userData)
{
}

void NativeVideoEncoderCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    (void)errorType;

    CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
    CHECK_AND_RETURN_LOG(IsValidVideoEncoderMagic(codec_->magic_), "Codec magic error!");
    CHECK_AND_RETURN_LOG(asyncCallback_.onError != nullptr || callback_.onError != nullptr, "Callback is nullptr");
    int32_t extErr = AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(errorCode));
    if (asyncCallback_.onError != nullptr) {
        asyncCallback_.onError(codec_, extErr, userData_);
    } else if (callback_.onError != nullptr) {
        callback_.onError(codec_, extErr, userData_);
    }
}

void NativeVideoEncoderCallback::OnOutputFormatChanged(const Media::Format& format)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
    CHECK_AND_RETURN_LOG(IsValidVideoEncoderMagic(codec_->magic_), "Codec magic error!");
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

void NativeVideoEncoderCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
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

void NativeVideoEncoderCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info,
                                                         AVCodecBufferFlag flag,
                                                         std::shared_ptr<AVSharedMemory> buffer)
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

void NativeVideoEncoderCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
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

void NativeVideoEncoderCallback::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
    CHECK_AND_RETURN_LOG(IsValidVideoEncoderMagic(codec_->magic_), "Codec magic error!");
    CHECK_AND_RETURN_LOG(callback_.onNewOutputBuffer != nullptr, "Callback is nullptr");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec_);
    CHECK_AND_RETURN_LOG(videoEncObj->videoEncoder_ != nullptr, "Video encoder is nullptr!");

    OH_AVBuffer *data = videoEncObj->GetTransData(index, buffer, true);

    callback_.onNewOutputBuffer(codec_, index, data, userData_);
}

void NativeVideoEncoderCallback::OnInputParameterAvailable(uint32_t index, std::shared_ptr<Media::Format> parameter)
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

void NativeVideoEncoderCallback::StopCallback()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    codec_ = nullptr;
}

void NativeVideoEncoderCallback::UpdateCallback(const struct OH_AVCodecAsyncCallback& cb, void* userData)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    userData_ = userData;
    asyncCallback_ = cb;
}

void NativeVideoEncoderCallback::UpdateCallback(const OH_VideoEncoder_OnNeedInputParameter& onInputParameter,
                                                void* userData)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    paramUserData_ = userData;
    onInputParameter_ = onInputParameter;
}

void NativeVideoEncoderCallback::UpdateCallback(const struct OH_AVCodecCallback& cb, void* userData)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    userData_ = userData;
    callback_ = cb;
}

} // namespace MediaAVCodec
} // namespace OHOS