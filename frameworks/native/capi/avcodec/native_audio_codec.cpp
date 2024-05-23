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
#include "avcodec_audio_codec.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "common/native_mfmagic.h"
#include "buffer/avsharedmemory.h"
#include "native_avcodec_audiocodec.h"
#include "native_avcodec_base.h"
#include "native_avmagic.h"
#include "avcodec_codec_name.h"
#include "avcodec_audio_codec_impl.h"
#ifdef SUPPORT_DRM
#include "foundation/multimedia/drm_framework/interfaces/kits/c/drm_capi/common/native_drm_object.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "NativeAudioCodec"};
constexpr uint32_t MAX_LENGTH = 255;
}

using namespace OHOS::MediaAVCodec;
class NativeAudioCodec;

struct AudioCodecObject : public OH_AVCodec {
    explicit AudioCodecObject(const std::shared_ptr<AVCodecAudioCodecImpl> &decoder)
        : OH_AVCodec(AVMagic::AVCODEC_MAGIC_AUDIO_DECODER), audioCodec_(decoder)
    {
    }
    ~AudioCodecObject() = default;

    const std::shared_ptr<AVCodecAudioCodecImpl> audioCodec_;
    std::list<OHOS::sptr<OH_AVMemory>> memoryObjList_;
    std::list<OHOS::sptr<OH_AVBuffer>> bufferObjList_;
    std::shared_ptr<NativeAudioCodec> callback_ = nullptr;
    std::atomic<bool> isFlushing_ = false;
    std::atomic<bool> isFlushed_ = false;
    std::atomic<bool> isStop_ = false;
    std::atomic<bool> isEOS_ = false;
    std::shared_mutex memoryObjListMutex_;
};

class NativeAudioCodec : public MediaCodecCallback {
public:
    NativeAudioCodec(OH_AVCodec *codec, struct OH_AVCodecCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData)
    {
    }
    virtual ~NativeAudioCodec() = default;

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

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (codec_ != nullptr && callback_.onNeedInputBuffer != nullptr) {
            struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec_);
            CHECK_AND_RETURN_LOG(audioCodecObj->audioCodec_ != nullptr, "audioCodec_ is nullptr!");
            if (audioCodecObj->isFlushing_.load() || audioCodecObj->isFlushed_.load() ||
                audioCodecObj->isStop_.load() || audioCodecObj->isEOS_.load()) {
                AVCODEC_LOGD("At flush, eos or stop, no buffer available");
                return;
            }
            OH_AVBuffer *data = GetTransData(codec_, index, buffer);
            callback_.onNeedInputBuffer(codec_, index, data, userData_);
        }
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (codec_ != nullptr && callback_.onNewOutputBuffer != nullptr) {
            struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec_);
            CHECK_AND_RETURN_LOG(audioCodecObj->audioCodec_ != nullptr, "audioCodec_ is nullptr!");
            if (audioCodecObj->isFlushing_.load() || audioCodecObj->isFlushed_.load() ||
                audioCodecObj->isStop_.load()) {
                AVCODEC_LOGD("At flush or stop, ignore");
                return;
            }
            OH_AVBuffer *data = GetTransData(codec_, index, buffer);
            callback_.onNewOutputBuffer(codec_, index, data, userData_);
        }
    }

    void StopCallback()
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        codec_ = nullptr;
    }

private:
    OH_AVBuffer *GetTransData(struct OH_AVCodec *codec, uint32_t index, std::shared_ptr<AVBuffer> buffer)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER ||
            codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_ENCODER, nullptr, "magic error!");

        struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, nullptr, "audioc odec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "get output buffer is nullptr!");

        {
            std::shared_lock<std::shared_mutex> lock(audioCodecObj->memoryObjListMutex_);
            for (auto &bufferObj : audioCodecObj->bufferObjList_) {
                if (bufferObj->IsEqualBuffer(buffer)) {
                    return reinterpret_cast<OH_AVBuffer *>(bufferObj.GetRefPtr());
                }
            }
        }

        OHOS::sptr<OH_AVBuffer> object = new (std::nothrow) OH_AVBuffer(buffer);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVBuffer");

        std::lock_guard<std::shared_mutex> lock(audioCodecObj->memoryObjListMutex_);
        audioCodecObj->bufferObjList_.push_back(object);
        return reinterpret_cast<OH_AVBuffer *>(object.GetRefPtr());
    }
    struct OH_AVCodec *codec_;
    struct OH_AVCodecCallback callback_;
    void *userData_;
    std::shared_mutex mutex_;
};

namespace OHOS {
namespace MediaAVCodec {
#ifdef __cplusplus
extern "C" {
#endif

struct OH_AVCodec *OH_AudioCodec_CreateByMime(const char *mime, bool isEncoder)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "input mime is nullptr!");
    CHECK_AND_RETURN_RET_LOG(strlen(mime) < MAX_LENGTH, nullptr, "input mime is too long!");
    std::shared_ptr<AVCodecAudioCodecImpl> audioCodec = std::make_shared<AVCodecAudioCodecImpl>();
    CHECK_AND_RETURN_RET_LOG(audioCodec != nullptr, nullptr, "failed to AudioCodecFactory::CreateByMime");
    AVCodecType type = AVCODEC_TYPE_AUDIO_DECODER;
    if (isEncoder) {
        type = AVCODEC_TYPE_AUDIO_ENCODER;
    }
    int32_t ret = audioCodec->Init(type, true, mime);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "failed to init AVCodecAudioCodecImpl");
    struct AudioCodecObject *object = new (std::nothrow) AudioCodecObject(audioCodec);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioCodecObject");

    return object;
}

struct OH_AVCodec *OH_AudioCodec_CreateByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "input name is nullptr!");
    CHECK_AND_RETURN_RET_LOG(strlen(name) < MAX_LENGTH, nullptr, "input name is too long!");
    std::shared_ptr<AVCodecAudioCodecImpl> audioCodec = std::make_shared<AVCodecAudioCodecImpl>();
    CHECK_AND_RETURN_RET_LOG(audioCodec != nullptr, nullptr, "failed to AudioCodecFactory::CreateByMime");
    std::string codecMimeName = name;
    std::string targetName = name;
    if (targetName.compare(AVCodecCodecName::AUDIO_DECODER_API9_AAC_NAME) == 0) {
        codecMimeName = AVCodecCodecName::AUDIO_DECODER_AAC_NAME;
    } else if (targetName.compare(AVCodecCodecName::AUDIO_ENCODER_API9_AAC_NAME) == 0) {
        codecMimeName = AVCodecCodecName::AUDIO_ENCODER_AAC_NAME;
    }
    AVCodecType type = AVCODEC_TYPE_AUDIO_ENCODER;
    if (codecMimeName.find("Encoder") != codecMimeName.npos) {
        type = AVCODEC_TYPE_AUDIO_ENCODER;
    } else {
        type = AVCODEC_TYPE_AUDIO_DECODER;
    }
    int32_t ret = audioCodec->Init(type, false, name);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "failed to init AVCodecAudioCodecImpl");
    struct AudioCodecObject *object = new(std::nothrow) AudioCodecObject(audioCodec);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioCodecObject");

    return object;
}

OH_AVErrCode OH_AudioCodec_Destroy(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    AVCODEC_LOGI("OH_AudioCodec_Destroy enter");
    if (audioCodecObj != nullptr && audioCodecObj->audioCodec_ != nullptr) {
        if (audioCodecObj->callback_ != nullptr) {
            audioCodecObj->callback_->StopCallback();
        }
        {
            std::lock_guard<std::shared_mutex> lock(audioCodecObj->memoryObjListMutex_);
            audioCodecObj->memoryObjList_.clear();
        }
        audioCodecObj->isStop_.store(true);
        int32_t ret = audioCodecObj->audioCodec_->Release();
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGE("audioCodec Release failed!");
            delete codec;
            return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
        }
    } else {
        AVCODEC_LOGD("audioCodec_ is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioCodec_Configure(struct OH_AVCodec *codec, const OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, AV_ERR_INVALID_VAL, "audioCodec is nullptr!");

    int32_t ret = audioCodecObj->audioCodec_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioCodec Configure failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioCodec_Prepare(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, AV_ERR_INVALID_VAL, "audioCodec_ is nullptr!");

    int32_t ret = audioCodecObj->audioCodec_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioCodec Prepare failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioCodec_Start(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, AV_ERR_INVALID_VAL, "audioCodec_ is nullptr!");
    audioCodecObj->isStop_.store(false);
    audioCodecObj->isEOS_.store(false);
    audioCodecObj->isFlushed_.store(false);
    int32_t ret = audioCodecObj->audioCodec_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioCodec Start failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioCodec_Stop(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, AV_ERR_INVALID_VAL, "audioCodec_ is nullptr!");

    audioCodecObj->isStop_.store(true);
    AVCODEC_LOGD("set stop status to true");

    int32_t ret = audioCodecObj->audioCodec_->Stop();
    if (ret != AVCS_ERR_OK) {
        audioCodecObj->isStop_.store(false);
        AVCODEC_LOGE("audioCodec Stop failed!, set stop status to false");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }
    std::lock_guard<std::shared_mutex> lock(audioCodecObj->memoryObjListMutex_);
    audioCodecObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioCodec_Flush(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, AV_ERR_INVALID_VAL, "audioCodec_ is nullptr!");
    audioCodecObj->isFlushing_.store(true);
    AVCODEC_LOGD("Set flush status to true");
    int32_t ret = audioCodecObj->audioCodec_->Flush();
    if (ret != AVCS_ERR_OK) {
        audioCodecObj->isFlushing_.store(false);
        audioCodecObj->isFlushed_.store(false);
        AVCODEC_LOGE("audioCodec Flush failed! Set flush status to false");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }

    audioCodecObj->isFlushed_.store(true);
    audioCodecObj->isFlushing_.store(false);
    AVCODEC_LOGD("set flush status to false");
    std::lock_guard<std::shared_mutex> lock(audioCodecObj->memoryObjListMutex_);
    audioCodecObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioCodec_Reset(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, AV_ERR_INVALID_VAL, "audioCodec_ is nullptr!");
    audioCodecObj->isStop_.store(true);
    AVCODEC_LOGD("Set stop status to true");

    int32_t ret = audioCodecObj->audioCodec_->Reset();
    if (ret != AVCS_ERR_OK) {
        audioCodecObj->isStop_.store(false);
        AVCODEC_LOGE("audioCodec Reset failed! Set stop status to false");
        return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
    }

    std::lock_guard<std::shared_mutex> lock(audioCodecObj->memoryObjListMutex_);
    audioCodecObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioCodec_PushInputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, AV_ERR_INVALID_VAL, "audioCodec_ is nullptr!");

    int32_t ret = audioCodecObj->audioCodec_->QueueInputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioCodec QueueInputBuffer failed!");
    return AV_ERR_OK;
}

OH_AVFormat *OH_AudioCodec_GetOutputDescription(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER, nullptr, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, nullptr, "audioCodec_ is nullptr!");

    Format format;
    int32_t ret = audioCodecObj->audioCodec_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "audioCodec GetOutputFormat failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "audioCodec OH_AVFormat_Create failed!");
    avFormat->format_ = format;

    return avFormat;
}

OH_AVErrCode OH_AudioCodec_FreeOutputBuffer(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, AV_ERR_INVALID_VAL, "audioCodec_ is nullptr!");

    int32_t ret = audioCodecObj->audioCodec_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "audioCodec ReleaseOutputBuffer failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioCodec_SetParameter(struct OH_AVCodec *codec, const OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, AV_ERR_INVALID_VAL, "audioCodec_ is nullptr!");

    int32_t ret = audioCodecObj->audioCodec_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioCodec SetParameter failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioCodec_RegisterCallback(OH_AVCodec *codec, OH_AVCodecCallback callback, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(callback.onError != nullptr,
        AV_ERR_INVALID_VAL, "Callback onError is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback.onNeedInputBuffer != nullptr,
        AV_ERR_INVALID_VAL, "Callback onNeedInputBuffer is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback.onNewOutputBuffer != nullptr,
        AV_ERR_INVALID_VAL, "Callback onNewOutputBuffer is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback.onStreamChanged != nullptr,
        AV_ERR_INVALID_VAL, "Callback onStreamChanged is nullptr");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, AV_ERR_INVALID_VAL, "audioCodec_ is nullptr!");

    audioCodecObj->callback_ = std::make_shared<NativeAudioCodec>(codec, callback, userData);

    int32_t ret = audioCodecObj->audioCodec_->SetCallback(audioCodecObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "audioCodec SetCallback failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioCodec_IsValid(OH_AVCodec *codec, bool *isValid)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "Magic error!");
    CHECK_AND_RETURN_RET_LOG(isValid != nullptr, AV_ERR_INVALID_VAL, "Input isValid is nullptr!");
    *isValid = true;
    return AV_ERR_OK;
}

#ifdef SUPPORT_DRM
OH_AVErrCode OH_AudioCodec_SetDecryptionConfig(OH_AVCodec *codec, MediaKeySession *mediaKeySession,
    bool secureAudio)
{
    AVCODEC_LOGI("OH_AudioCodec_SetDecryptionConfig");
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::AVCODEC_MAGIC_AUDIO_DECODER,
        AV_ERR_INVALID_VAL, "magic error!");

    struct AudioCodecObject *audioCodecObj = reinterpret_cast<AudioCodecObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioCodecObj->audioCodec_ != nullptr, AV_ERR_INVALID_VAL, "audioCodec_ is nullptr!");

    DrmStandard::MediaKeySessionObject *sessionObject =
        reinterpret_cast<DrmStandard::MediaKeySessionObject *>(mediaKeySession);
    CHECK_AND_RETURN_RET_LOG(sessionObject != nullptr, AV_ERR_INVALID_VAL, "sessionObject is nullptr!");
    AVCODEC_LOGD("DRM sessionObject impl :0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(sessionObject));

    CHECK_AND_RETURN_RET_LOG(sessionObject->sessionImpl_ != nullptr, AV_ERR_INVALID_VAL,
        "sessionObject->impl is nullptr!");
    AVCODEC_LOGD("DRM impl :0x%{public}06" PRIXPTR " Instances create",
        FAKE_POINTER(sessionObject->sessionImpl_.GetRefPtr()));
    CHECK_AND_RETURN_RET_LOG(sessionObject->sessionImpl_ ->GetMediaKeySessionServiceProxy() != nullptr,
        AV_ERR_INVALID_VAL, "MediaKeySessionServiceProxy is nullptr!");

    int32_t ret = audioCodecObj->audioCodec_->SetAudioDecryptionConfig(
        sessionObject->sessionImpl_->GetMediaKeySessionServiceProxy(), secureAudio);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "audioCodec SetAudioDecryptionConfig failed!");

    return AV_ERR_OK;
}
#else
OH_AVErrCode OH_AudioCodec_SetDecryptionConfig(OH_AVCodec *codec, MediaKeySession *mediaKeySession,
    bool secureAudio)
{
    AVCODEC_LOGI("OH_AudioCodec_SetDecryptionConfig");
    (void)codec;
    (void)mediaKeySession;
    (void)secureAudio;
    return AV_ERR_OK;
}
#endif

#ifdef __cplusplus
};
#endif
} // namespace MediaAVCodec
} // namespace OHOS