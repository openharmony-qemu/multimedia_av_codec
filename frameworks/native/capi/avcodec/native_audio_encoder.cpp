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
#include <shared_mutex>
#include "avcodec_audio_encoder.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "buffer/avsharedmemory.h"
#include "common/native_mfmagic.h"
#include "native_avcodec_audioencoder.h"
#include "native_avcodec_base.h"
#include "native_avmagic.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "NativeAudioEncoder"};
constexpr uint32_t MAX_LENGTH = 255;
}

using namespace OHOS::MediaAVCodec;
class NativeAudioEncoderCallback;

struct AudioEncoderObject : public OH_AVCodec {
    explicit AudioEncoderObject(const std::shared_ptr<AVCodecAudioEncoder> &encoder)
        : OH_AVCodec(AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER), audioEncoder_(encoder)
    {
    }
    ~AudioEncoderObject() = default;

    const std::shared_ptr<AVCodecAudioEncoder> audioEncoder_;
    std::list<OHOS::sptr<OH_AVMemory>> memoryObjList_;
    std::shared_ptr<NativeAudioEncoderCallback> callback_ = nullptr;
    std::atomic<bool> isFlushing_ = false;
    std::atomic<bool> isFlushed_ = false;
    std::atomic<bool> isStop_ = false;
    std::atomic<bool> isEOS_ = false;
    std::shared_mutex memoryObjListMutex_;
};

class NativeAudioEncoderCallback : public AVCodecCallback {
public:
    NativeAudioEncoderCallback(OH_AVCodec *codec, struct OH_AVCodecAsyncCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData)
    {
    }
    virtual ~NativeAudioEncoderCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        (void)errorType;
        if (codec_ != nullptr && callback_.onError != nullptr) {
            int32_t extErr = AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(errorCode));
            callback_.onError(codec_, extErr, userData_);
        }
    }

    void OnOutputFormatChanged(const Format &format) override
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        if (codec_ != nullptr && callback_.onStreamChanged != nullptr) {
            OHOS::sptr<OH_AVFormat> object = new (std::nothrow) OH_AVFormat(format);
            // The object lifecycle is controlled by the current function stack
            callback_.onStreamChanged(codec_, reinterpret_cast<OH_AVFormat *>(object.GetRefPtr()), userData_);
        }
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (codec_ != nullptr && callback_.onNeedInputData != nullptr) {
            struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec_);
            CHECK_AND_RETURN_LOG(audioEncObj->audioEncoder_ != nullptr, "audioEncoder_ is nullptr!");
            if (audioEncObj->isFlushing_.load() || audioEncObj->isFlushed_.load() || audioEncObj->isStop_.load() ||
                audioEncObj->isEOS_.load()) {
                AVCODEC_LOGD("At flush, eos or stop, no buffer available");
                return;
            }

            OH_AVMemory *data = GetInputData(codec_, index, buffer);
            callback_.onNeedInputData(codec_, index, data, userData_);
        }
    }

    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (codec_ != nullptr && callback_.onNeedOutputData != nullptr) {
            struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec_);
            CHECK_AND_RETURN_LOG(audioEncObj->audioEncoder_ != nullptr, "audioEncoder_ is nullptr!");
            if (audioEncObj->isFlushing_.load() || audioEncObj->isFlushed_.load() || audioEncObj->isStop_.load()) {
                AVCODEC_LOGD("At flush or stop, ignore");
                return;
            }
            struct OH_AVCodecBufferAttr bufferAttr { info.presentationTimeUs, info.size, info.offset, flag };
            bufferAttr.flags = flag;
            // The bufferInfo lifecycle is controlled by the current function stack
            OH_AVMemory *data = GetOutputData(codec_, index, buffer);
            callback_.onNeedOutputData(codec_, index, data, &bufferAttr, userData_);
        }
    }

    void StopCallback()
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        codec_ = nullptr;
    }

private:
    OH_AVMemory *GetInputData(struct OH_AVCodec *codec, uint32_t index, std::shared_ptr<AVSharedMemory> memory)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER, nullptr, "magic error!");

        struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, nullptr, "audioEncoder_ is nullptr!");
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get input buffer is nullptr!");

        {
            std::shared_lock<std::shared_mutex> lock(audioEncObj->memoryObjListMutex_);
            for (auto &memoryObj : audioEncObj->memoryObjList_) {
                if (memoryObj->IsEqualMemory(memory)) {
                    return reinterpret_cast<OH_AVMemory *>(memoryObj.GetRefPtr());
                }
            }
        }

        OHOS::sptr<OH_AVMemory> object = new (std::nothrow) OH_AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVMemory");

        std::lock_guard<std::shared_mutex> lock(audioEncObj->memoryObjListMutex_);
        audioEncObj->memoryObjList_.push_back(object);
        return reinterpret_cast<OH_AVMemory *>(object.GetRefPtr());
    }

    OH_AVMemory *GetOutputData(struct OH_AVCodec *codec, uint32_t index, std::shared_ptr<AVSharedMemory> memory)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER, nullptr, "magic error!");

        struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, nullptr, "audioEncoder_ is nullptr!");
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get output buffer is nullptr!");

        {
            std::shared_lock<std::shared_mutex> lock(audioEncObj->memoryObjListMutex_);
            for (auto &memoryObj : audioEncObj->memoryObjList_) {
                if (memoryObj->IsEqualMemory(memory)) {
                    return reinterpret_cast<OH_AVMemory *>(memoryObj.GetRefPtr());
                }
            }
        }

        OHOS::sptr<OH_AVMemory> object = new (std::nothrow) OH_AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVMemory");

        std::lock_guard<std::shared_mutex> lock(audioEncObj->memoryObjListMutex_);
        audioEncObj->memoryObjList_.push_back(object);
        return reinterpret_cast<OH_AVMemory *>(object.GetRefPtr());
    }

    struct OH_AVCodec *codec_;
    struct OH_AVCodecAsyncCallback callback_;
    void *userData_;
    std::shared_mutex mutex_;
};

namespace OHOS {
namespace MediaAVCodec {
#ifdef __cplusplus
extern "C" {
#endif

struct OH_AVCodec *OH_AudioEncoder_CreateByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "input mime is nullptr!");
    CHECK_AND_RETURN_RET_LOG(strlen(mime) < MAX_LENGTH, nullptr, "input mime is too long!");

    std::shared_ptr<AVCodecAudioEncoder> audioEncoder = AudioEncoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(audioEncoder != nullptr, nullptr, "failed to AudioEncoderFactory::CreateByMime");

    struct AudioEncoderObject *object = new (std::nothrow) AudioEncoderObject(audioEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioEncoderObject");

    return object;
}

struct OH_AVCodec *OH_AudioEncoder_CreateByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "input name is nullptr!");
    CHECK_AND_RETURN_RET_LOG(strlen(name) < MAX_LENGTH, nullptr, "input name is too long!");

    std::shared_ptr<AVCodecAudioEncoder> audioEncoder = AudioEncoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(audioEncoder != nullptr, nullptr, "failed to AudioEncoderFactory::CreateByMime");

    struct AudioEncoderObject *object = new (std::nothrow) AudioEncoderObject(audioEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioEncoderObject");

    return object;
}

OH_AVErrCode OH_AudioEncoder_Destroy(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);

    if (audioEncObj != nullptr && audioEncObj->audioEncoder_ != nullptr) {
        if (audioEncObj->callback_ != nullptr) {
            audioEncObj->callback_->StopCallback();
        }
        {
            std::lock_guard<std::shared_mutex> lock(audioEncObj->memoryObjListMutex_);
            audioEncObj->memoryObjList_.clear();
        }
        int32_t ret = audioEncObj->audioEncoder_->Release();
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGE("audioEncoder Release failed!");
            delete codec;
            return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
        }
    } else {
        AVCODEC_LOGD("audioEncoder_ is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioEncoder_Configure(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER,
        AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioEncoder Configure failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioEncoder_Prepare(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioEncoder Prepare failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioEncoder_Start(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");
    audioEncObj->isStop_.store(false);
    audioEncObj->isEOS_.store(false);
    audioEncObj->isFlushed_.store(false);
    AVCODEC_LOGD("Set stop and eos status to false");
    int32_t ret = audioEncObj->audioEncoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioEncoder Start failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioEncoder_Stop(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");
    audioEncObj->isStop_.store(true);
    AVCODEC_LOGD("Set stop status to true");

    int32_t ret = audioEncObj->audioEncoder_->Stop();
    if (ret != AVCS_ERR_OK) {
        audioEncObj->isStop_.store(false);
        AVCODEC_LOGE("audioEncoder Stop failed! Set stop status to false");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    std::lock_guard<std::shared_mutex> lock(audioEncObj->memoryObjListMutex_);
    audioEncObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioEncoder_Flush(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    audioEncObj->isFlushing_.store(true);
    AVCODEC_LOGD("Set flush status to true");

    int32_t ret = audioEncObj->audioEncoder_->Flush();
    if (ret != AVCS_ERR_OK) {
        audioEncObj->isFlushing_.store(false);
        audioEncObj->isFlushed_.store(false);
        AVCODEC_LOGE("audioEncObj Flush failed! Set flush status to false");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    audioEncObj->isFlushed_.store(true);
    audioEncObj->isFlushing_.store(false);
    AVCODEC_LOGD("Set flush status to false");
    std::lock_guard<std::shared_mutex> lock(audioEncObj->memoryObjListMutex_);
    audioEncObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioEncoder_Reset(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");
    audioEncObj->isStop_.store(true);
    int32_t ret = audioEncObj->audioEncoder_->Reset();
    if (ret != AVCS_ERR_OK) {
        audioEncObj->isStop_.store(false);
        AVCODEC_LOGE("audioEncoder Reset failed! Set stop status to false");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    std::lock_guard<std::shared_mutex> lock(audioEncObj->memoryObjListMutex_);
    audioEncObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioEncoder_PushInputData(struct OH_AVCodec *codec, uint32_t index, OH_AVCodecBufferAttr attr)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER,
        AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(attr.size >= 0, AV_ERR_INVALID_VAL, "Invalid buffer size!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.pts;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    AVCodecBufferFlag bufferFlag = static_cast<AVCodecBufferFlag>(attr.flags);

    int32_t ret = audioEncObj->audioEncoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioEncoder QueueInputBuffer failed!");
    if (bufferFlag == AVCODEC_BUFFER_FLAG_EOS) {
        audioEncObj->isEOS_.store(true);
        AVCODEC_LOGD("Set eos status to true");
    }

    return AV_ERR_OK;
}

OH_AVFormat *OH_AudioEncoder_GetOutputDescription(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER, nullptr, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, nullptr, "audioEncoder_ is nullptr!");

    Format format;
    int32_t ret = audioEncObj->audioEncoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "audioEncoder GetOutputFormat failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "audioCodec OH_AVFormat_Create failed!");
    avFormat->format_ = format;

    return avFormat;
}

OH_AVErrCode OH_AudioEncoder_FreeOutputData(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "audioEncoder ReleaseOutputBuffer failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioEncoder_SetParameter(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER,
        AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioEncoder SetParameter failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioEncoder_SetCallback(struct OH_AVCodec *codec, struct OH_AVCodecAsyncCallback callback,
                                         void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER,
        AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(callback.onError != nullptr,
        AV_ERR_INVALID_VAL, "Callback onError is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback.onNeedInputData != nullptr,
        AV_ERR_INVALID_VAL, "Callback onNeedInputData is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback.onNeedOutputData != nullptr,
        AV_ERR_INVALID_VAL, "Callback onNeedOutputData is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback.onStreamChanged != nullptr,
        AV_ERR_INVALID_VAL, "Callback onStreamChanged is nullptr");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    audioEncObj->callback_ = std::make_shared<NativeAudioEncoderCallback>(codec, callback, userData);

    int32_t ret = audioEncObj->audioEncoder_->SetCallback(audioEncObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioEncoder SetCallback failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioEncoder_IsValid(OH_AVCodec *codec, bool *isValid)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "Magic error!");
    CHECK_AND_RETURN_RET_LOG(isValid != nullptr, AV_ERR_INVALID_VAL, "Input isValid is nullptr!");
    *isValid = true;
    return AV_ERR_OK;
}

#ifdef __cplusplus
};
#endif
} // namespace MediaAVCodec
} // namespace OHOS
