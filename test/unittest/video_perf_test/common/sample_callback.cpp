#include "sample_callback.h"
#include "av_codec_sample_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SampleCallback"};
constexpr int LIMIT_LOGD_FREQUENCY = 50;
}

void OnCodecError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    AVCODEC_LOGE("On decoder error, error code: %{public}d", errorCode);
}

void OnCodecFormatChange(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    AVCODEC_LOGW("On decoder format change");
}

void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    if (userData == nullptr) {
        return;
    }
    CodecUserData *codecUserData = static_cast<CodecUserData *>(userData);
    codecUserData->inputFrameCount_++;
    AVCODEC_LOGD_LIMIT(LIMIT_LOGD_FREQUENCY, "FrameCount: %{public}d",
        codecUserData->inputFrameCount_);
    std::unique_lock<std::mutex> lock(codecUserData->inputMutex_);
    codecUserData->inputBufferInfoQueue_.emplace(index, data);
    codecUserData->inputCond_.notify_all();
}

void OnOutputBufferAvailable(OH_AVCodec * codec, uint32_t index, OH_AVMemory *data,
                             OH_AVCodecBufferAttr *attr, void *userData)
{
    if (userData == nullptr) {
        return;
    }
    CodecUserData *codecUserData = static_cast<CodecUserData *>(userData);
    codecUserData->outputFrameCount_++;
    AVCODEC_LOGD_LIMIT(LIMIT_LOGD_FREQUENCY, "FrameCount: %{public}d",
        codecUserData->outputFrameCount_);
    std::unique_lock<std::mutex> lock(codecUserData->outputMutex_);
    codecUserData->outputBufferInfoQueue_.emplace(index, data, *attr);
    codecUserData->outputCond_.notify_all();
}

void onNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    if (userData == nullptr) {
        return;
    }
    CodecUserData *codecUserData = static_cast<CodecUserData *>(userData);
    codecUserData->inputFrameCount_++;
    AVCODEC_LOGD_LIMIT(LIMIT_LOGD_FREQUENCY, "FrameCount: %{public}d",
        codecUserData->inputFrameCount_);
    std::unique_lock<std::mutex> lock(codecUserData->inputMutex_);
    codecUserData->inputBufferInfoQueue_.emplace(index, buffer);
    codecUserData->inputCond_.notify_all();
}

void onNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    if (userData == nullptr) {
        return;
    }
    CodecUserData *codecUserData = static_cast<CodecUserData *>(userData);
    codecUserData->outputFrameCount_++;
    AVCODEC_LOGD_LIMIT(LIMIT_LOGD_FREQUENCY, "FrameCount: %{public}d",
        codecUserData->outputFrameCount_);
    std::unique_lock<std::mutex> lock(codecUserData->outputMutex_);
    codecUserData->outputBufferInfoQueue_.emplace(index, buffer);
    codecUserData->outputCond_.notify_all();
}