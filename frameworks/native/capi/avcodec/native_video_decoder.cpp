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
#include <unordered_map>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_trace.h"
#include "avcodec_video_decoder.h"
#include "buffer/avsharedmemory.h"
#include "common/native_mfmagic.h"
#include "native_avbuffer.h"
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_avmagic.h"
#include "native_window.h"
#include "hiappevent_util.h"

#ifdef SUPPORT_DRM
#include "native_drm_object.h"
#endif
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "NativeVideoDecoder"};
constexpr size_t MAX_TEMPNUM = 64;
constexpr uint32_t MAX_LENGTH = 255;

using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace OHOS::DrmStandard;
class NativeVideoDecoderCallback;

struct VideoDecoderObject : public OH_AVCodec {
    explicit VideoDecoderObject(const std::shared_ptr<AVCodecVideoDecoder> &decoder)
        : OH_AVCodec(AVMagic::AVCODEC_MAGIC_VIDEO_DECODER), videoDecoder_(decoder)
    {
    }
    ~VideoDecoderObject() = default;

    void ClearBufferList();
    void StopCallback();
    void BufferToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> &tempMap);
    void MemoryToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> &tempMap);
    OH_AVBuffer *GetTransData(const uint32_t &index, std::shared_ptr<AVBuffer> &buffer, bool isOutput);
    OH_AVMemory *GetTransData(const uint32_t &index, std::shared_ptr<AVSharedMemory> &memory, bool isOutput);

    const std::shared_ptr<AVCodecVideoDecoder> videoDecoder_;
    std::queue<OHOS::sptr<MFObjectMagic>> tempList_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> outputMemoryMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> inputMemoryMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> outputBufferMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> inputBufferMap_;
    std::shared_ptr<NativeVideoDecoderCallback> callback_ = nullptr;
    bool isSetMemoryCallback_ = false;
    bool isOutputSurfaceMode_ = false;
    std::shared_mutex objListMutex_;
};

class NativeVideoDecoderCallback : public AVCodecCallback, public MediaCodecCallback {
public:
    NativeVideoDecoderCallback(OH_AVCodec *codec, struct OH_AVCodecAsyncCallback cb, void *userData)
        : codec_(codec), asyncCallback_(cb), userData_(userData)
    {
    }
    NativeVideoDecoderCallback(OH_AVCodec *codec, struct OH_AVCodecCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData)
    {
    }
    virtual ~NativeVideoDecoderCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        (void)errorType;

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, "Codec magic error!");
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
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, "Codec magic error!");
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
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, "Codec magic error!");
        CHECK_AND_RETURN_LOG(asyncCallback_.onNeedInputData != nullptr, "Callback is nullptr");

        struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoDecObj->videoDecoder_ != nullptr, "Video decoder is nullptr!");

        OH_AVMemory *data = videoDecObj->GetTransData(index, buffer, false);
        asyncCallback_.onNeedInputData(codec_, index, data, userData_);
    }

    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, "Codec magic error!");
        CHECK_AND_RETURN_LOG(asyncCallback_.onNeedOutputData != nullptr, "Callback is nullptr");

        struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoDecObj->videoDecoder_ != nullptr, "Video decoder is nullptr!");

        struct OH_AVCodecBufferAttr bufferAttr {
            info.presentationTimeUs, info.size, info.offset, flag
        };
        OH_AVMemory *data = nullptr;
        if (!videoDecObj->isOutputSurfaceMode_) {
            data = videoDecObj->GetTransData(index, buffer, true);
        }

        asyncCallback_.onNeedOutputData(codec_, index, data, &bufferAttr, userData_);
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, "Codec magic error!");
        CHECK_AND_RETURN_LOG(callback_.onNeedInputBuffer != nullptr, "Callback is nullptr");

        struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoDecObj->videoDecoder_ != nullptr, "Video decoder is nullptr!");

        OH_AVBuffer *data = videoDecObj->GetTransData(index, buffer, false);
        callback_.onNeedInputBuffer(codec_, index, data, userData_);
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        CHECK_AND_RETURN_LOG(codec_ != nullptr, "Codec is nullptr");
        CHECK_AND_RETURN_LOG(codec_->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, "Codec magic error!");
        CHECK_AND_RETURN_LOG(callback_.onNewOutputBuffer != nullptr, "Callback is nullptr");

        struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec_);
        CHECK_AND_RETURN_LOG(videoDecObj->videoDecoder_ != nullptr, "Video decoder is nullptr!");

        OH_AVBuffer *data = nullptr;
        data = videoDecObj->GetTransData(index, buffer, true);

        callback_.onNewOutputBuffer(codec_, index, data, userData_);
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
    void *userData_ = nullptr;
    std::shared_mutex mutex_;
};

OH_AVMemory *VideoDecoderObject::GetTransData(const uint32_t &index, std::shared_ptr<AVSharedMemory> &memory,
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

OH_AVBuffer *VideoDecoderObject::GetTransData(const uint32_t &index, std::shared_ptr<AVBuffer> &buffer, bool isOutput)
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

void VideoDecoderObject::ClearBufferList()
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
    while (tempList_.size() > MAX_TEMPNUM) {
        tempList_.pop();
    }
}

void VideoDecoderObject::StopCallback()
{
    if (callback_ == nullptr) {
        return;
    }
    callback_->StopCallback();
}

void VideoDecoderObject::BufferToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> &tempMap)
{
    for (auto &val : tempMap) {
        val.second->magic_ = MFMagic::MFMAGIC_UNKNOWN;
        val.second->buffer_ = nullptr;
        tempList_.push(std::move(val.second));
    }
}

void VideoDecoderObject::MemoryToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> &tempMap)
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
struct OH_AVCodec *OH_VideoDecoder_CreateByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "Mime is nullptr!");
    CHECK_AND_RETURN_RET_LOG(strlen(mime) < MAX_LENGTH, nullptr, "Mime is too long!");

    static AppEventReporter appEventReporter = AppEventReporter();
    ApiInvokeRecorder apiInvokeRecorder("OH_VideoDecoder_CreateByMime", appEventReporter);

    std::shared_ptr<AVCodecVideoDecoder> videoDecoder = VideoDecoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(videoDecoder != nullptr, nullptr, "Video decoder create by mime failed!");

    struct VideoDecoderObject *object = new (std::nothrow) VideoDecoderObject(videoDecoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Video decoder create by mime failed!");

    return object;
}

struct OH_AVCodec *OH_VideoDecoder_CreateByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "Name is nullptr!");
    CHECK_AND_RETURN_RET_LOG(strlen(name) < MAX_LENGTH, nullptr, "Name is too long!");

    static AppEventReporter appEventReporter = AppEventReporter();
    ApiInvokeRecorder apiInvokeRecorder("OH_VideoDecoder_CreateByName", appEventReporter);

    std::shared_ptr<AVCodecVideoDecoder> videoDecoder = VideoDecoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(videoDecoder != nullptr, nullptr, "Video decoder create by name failed!");

    struct VideoDecoderObject *object = new (std::nothrow) VideoDecoderObject(videoDecoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Video decoder create by name failed!");

    return object;
}

OH_AVErrCode OH_VideoDecoder_Destroy(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);

    if (videoDecObj != nullptr && videoDecObj->videoDecoder_ != nullptr) {
        int32_t ret = videoDecObj->videoDecoder_->Release();
        videoDecObj->StopCallback();
        videoDecObj->ClearBufferList();
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGE("Video decoder destroy failed!");
            delete codec;
            return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
        }
    } else {
        AVCODEC_LOGD("Video decoder is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Configure(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "Format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL, "Format magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video decoder configure failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Prepare(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video decoder prepare failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Start(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    static AppEventReporter appEventReporter = AppEventReporter();
    ApiInvokeRecorder apiInvokeRecorder("OH_VideoDecoder_Start", appEventReporter);

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Start();
    apiInvokeRecorder.SetErrorCode(AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video decoder start failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Stop(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Stop();
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Video decoder stop failed");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    videoDecObj->ClearBufferList();
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Flush(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Flush();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video decoder flush failed!");
    videoDecObj->ClearBufferList();
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Reset(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Reset();
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Video decoder reset failed!");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    videoDecObj->isOutputSurfaceMode_ = false;
    videoDecObj->ClearBufferList();
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_SetSurface(OH_AVCodec *codec, OHNativeWindow *window)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(window != nullptr, AV_ERR_INVALID_VAL, "Window is nullptr!");
    CHECK_AND_RETURN_RET_LOG(window->surface != nullptr, AV_ERR_INVALID_VAL, "Input window surface is nullptr!");

    GSError gsRet = window->surface->Disconnect();
    EXPECT_AND_LOGI(gsRet == GSERROR_OK, "Disconnect success");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->SetOutputSurface(window->surface);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video decoder set output surface failed!");
    videoDecObj->isOutputSurfaceMode_ = true;

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_PushInputData(struct OH_AVCodec *codec, uint32_t index, OH_AVCodecBufferAttr attr)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");
    CHECK_AND_RETURN_RET_LOGD(attr.size >= 0, AV_ERR_INVALID_VAL, "Invalid buffer size!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
        "The callback of OH_AVMemory is nullptr!");

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.pts;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    enum AVCodecBufferFlag bufferFlag = static_cast<enum AVCodecBufferFlag>(attr.flags);

    int32_t ret = videoDecObj->videoDecoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOGD(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "Video decoder push input data failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_PushInputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
        "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "video decoder is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!videoDecObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
        "Not support the callback of OH_AVMemory!");

    int32_t ret = videoDecObj->videoDecoder_->QueueInputBuffer(index);
    CHECK_AND_RETURN_RET_LOGD(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "videoDecoder QueueInputBuffer failed!");
    return AV_ERR_OK;
}

OH_AVFormat *OH_VideoDecoder_GetOutputDescription(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, nullptr, "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, nullptr, "Video decoder is nullptr!");

    Format format;
    int32_t ret = videoDecObj->videoDecoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Video decoder get output description failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Video decoder get output description failed!");
    avFormat->format_ = format;

    return avFormat;
}

OH_AVErrCode OH_VideoDecoder_RenderOutputData(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
        "The callback of OH_AVMemory is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->ReleaseOutputBuffer(index, true);
    CHECK_AND_RETURN_RET_LOGD(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "Video decoder render output data failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_FreeOutputData(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
        "The callback of OH_AVMemory is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->ReleaseOutputBuffer(index, false);
    CHECK_AND_RETURN_RET_LOGD(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "Video decoder free output data failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_RenderOutputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");
    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!videoDecObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
        "Not support the callback of OH_AVMemory!");

    int32_t ret = videoDecObj->videoDecoder_->ReleaseOutputBuffer(index, true);
    CHECK_AND_RETURN_RET_LOGD(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "Video decoder render output data failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_RenderOutputBufferAtTime(OH_AVCodec *codec, uint32_t index, int64_t renderTimestampNs)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");
    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!videoDecObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
        "Not support the callback of OH_AVMemory!");

    int32_t ret = videoDecObj->videoDecoder_->RenderOutputBufferAtTime(index, renderTimestampNs);
    CHECK_AND_RETURN_RET_LOGD(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "Video decoder render output buffer at time failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_FreeOutputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(!videoDecObj->isSetMemoryCallback_, AV_ERR_INVALID_STATE,
        "Not support the callback of OH_AVMemory!");

    int32_t ret = videoDecObj->videoDecoder_->ReleaseOutputBuffer(index, false);
    CHECK_AND_RETURN_RET_LOGD(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "Video decoder free output data failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_QueryInputBuffer(struct OH_AVCodec *codec, uint32_t *index, int64_t timeoutUs)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");
    int32_t ret = videoDecObj->videoDecoder_->QueryInputBuffer(*index, timeoutUs);
    switch (ret) {
        case AVCS_ERR_OK:
            return AV_ERR_OK;
        case AVCS_ERR_TRY_AGAIN:
            return AV_ERR_TRY_AGAIN_LATER;
        default:
            AVCODEC_LOGD("Video decoder query input data failed!");
    }
    return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
}

OH_AVErrCode OH_VideoDecoder_QueryOutputBuffer(struct OH_AVCodec *codec, uint32_t *index, int64_t timeoutUs)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
        "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->QueryOutputBuffer(*index, timeoutUs);
    switch (ret) {
        case AVCS_ERR_OK:
            return AV_ERR_OK;
        case AVCS_ERR_TRY_AGAIN:
            return AV_ERR_TRY_AGAIN_LATER;
        case AVCS_ERR_STREAM_CHANGED:
            return AV_ERR_STREAM_CHANGED;
        default:
            AVCODEC_LOGD("Video decoder query output data failed!");
    }
    return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
}

OH_AVBuffer *OH_VideoDecoder_GetInputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, nullptr, "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->videoDecoder_ != nullptr, nullptr, "Video decoder is nullptr!");

    std::shared_ptr<AVBuffer> buffer = videoDecObj->videoDecoder_->GetInputBuffer(index);
    CHECK_AND_RETURN_RET_LOGD(buffer != nullptr, nullptr, "Buffer is nullptr, idx:%{public}u", index);

    return videoDecObj->GetTransData(index, buffer, false);
}

OH_AVBuffer *OH_VideoDecoder_GetOutputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOGD(codec != nullptr, nullptr, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOGD(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, nullptr, "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOGD(videoDecObj->videoDecoder_ != nullptr, nullptr, "Video decoder is nullptr!");

    std::shared_ptr<AVBuffer> buffer = videoDecObj->videoDecoder_->GetOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOGD(buffer != nullptr, nullptr, "Buffer is nullptr, idx:%{public}u", index);

    return videoDecObj->GetTransData(index, buffer, true);
}

OH_AVErrCode OH_VideoDecoder_SetParameter(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "Format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL, "Format magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video decoder set parameter failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_SetCallback(struct OH_AVCodec *codec, struct OH_AVCodecAsyncCallback callback,
                                         void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    if (videoDecObj->callback_ == nullptr) {
        videoDecObj->callback_ = std::make_shared<NativeVideoDecoderCallback>(codec, callback, userData);
    } else {
        std::static_pointer_cast<NativeVideoDecoderCallback>(videoDecObj->callback_)
            ->UpdateCallback(callback, userData);
    }
    int32_t ret =
        videoDecObj->videoDecoder_->SetCallback(std::static_pointer_cast<AVCodecCallback>(videoDecObj->callback_));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video decoder set callback failed!");
    videoDecObj->isSetMemoryCallback_ = true;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_RegisterCallback(struct OH_AVCodec *codec, struct OH_AVCodecCallback callback,
                                              void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(callback.onNeedInputBuffer != nullptr, AV_ERR_INVALID_VAL,
                             "Callback onNeedInputBuffer is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback.onNewOutputBuffer != nullptr, AV_ERR_INVALID_VAL,
                             "Callback onNewOutputBuffer is nullptr");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    if (videoDecObj->callback_ == nullptr) {
        videoDecObj->callback_ = std::make_shared<NativeVideoDecoderCallback>(codec, callback, userData);
    } else {
        std::static_pointer_cast<NativeVideoDecoderCallback>(videoDecObj->callback_)
            ->UpdateCallback(callback, userData);
    }
    int32_t ret =
        videoDecObj->videoDecoder_->SetCallback(std::static_pointer_cast<MediaCodecCallback>(videoDecObj->callback_));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video decoder register callback failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_IsValid(OH_AVCodec *codec, bool *isValid)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");
    CHECK_AND_RETURN_RET_LOG(isValid != nullptr, AV_ERR_INVALID_VAL, "Input isValid is nullptr!");
    *isValid = true;
    return AV_ERR_OK;
}

#ifdef SUPPORT_DRM
OH_AVErrCode OH_VideoDecoder_SetDecryptionConfig(OH_AVCodec *codec, MediaKeySession *mediaKeySession,
                                                 bool secureVideoPath)
{
    AVCODEC_LOGI("OH_VideoDecoder_SetDecryptionConfig");
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL,
                             "Codec magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "Video decoder is nullptr!");

    struct MediaKeySessionObject *sessionObject = reinterpret_cast<MediaKeySessionObject *>(mediaKeySession);
    CHECK_AND_RETURN_RET_LOG(sessionObject != nullptr, AV_ERR_INVALID_VAL, "sessionObject is nullptr!");
    AVCODEC_LOGD("DRM sessionObject impl :0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(sessionObject));

    CHECK_AND_RETURN_RET_LOG(sessionObject->sessionImpl_ != nullptr, AV_ERR_INVALID_VAL,
                             "sessionObject->impl is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->SetDecryptConfig(
        sessionObject->sessionImpl_->GetMediaKeySessionServiceProxy(), secureVideoPath);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Video decoder SetDecryptConfig failed!");

    return AV_ERR_OK;
}
#else
OH_AVErrCode OH_VideoDecoder_SetDecryptionConfig(OH_AVCodec *codec, MediaKeySession *mediaKeySession,
                                                 bool secureVideoPath)
{
    AVCODEC_LOGI("OH_VideoDecoder_SetDecryptionConfig");
    (void)codec;
    (void)mediaKeySession;
    (void)secureVideoPath;
    return AV_ERR_OK;
}
#endif
}
} // namespace MediaAVCodec
} // namespace OHOS