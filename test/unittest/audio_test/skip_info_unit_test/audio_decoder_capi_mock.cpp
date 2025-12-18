/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "audio_decoder_capi_mock.h"
#include "native_avcapability.h"
#include "avcodec_common.h"
namespace OHOS {
namespace MediaAVCodec {
#define DEMO_LOG(...) printf(__VA_ARGS__);printf("\n")
static constexpr int32_t TIME_OUT_MS = 1000;
std::unique_ptr<AudioDecoderMockBase> AudioDecoderMockBase::CreateDecoder()
{
    return std::make_unique<AudioDecoderCapiMock>();
}

static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
}

static void OnOutputFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
}

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    DecSignal *signal = static_cast<DecSignal*>(userData);
    std::unique_lock<std::mutex> lock(signal->inMutex);
    signal->inQueue.push(index);
    signal->inBufferQueue.push(buffer);
    signal->inCond.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    DecSignal *signal = static_cast<DecSignal*>(userData);
    std::unique_lock<std::mutex> lock(signal->outMutex);
    signal->outQueue.push(index);
    signal->outBufferQueue.push(buffer);
    signal->outCond.notify_all();
}

static struct OH_AVCodecCallback g_cb = {
    &OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable
};

int32_t AudioDecoderCapiMock::Create(const char *mimetype)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapability(mimetype, false);
    if (capability == nullptr) {
        DEMO_LOG("AudioDecoderCapiMock::Create get capability failed! mimetype:%s", mimetype);
        return -1;
    }
    const char *codecName = OH_AVCapability_GetName(capability);
    return CreateByName(codecName);
}

int32_t AudioDecoderCapiMock::CreateByName(const char *name)
{
    audioDec_ = OH_AudioCodec_CreateByName(name);
    if (audioDec_ == nullptr) {
        DEMO_LOG("AudioDecoderCapiMock::Create Create dec failed! name:%s", name);
        return -1;
    }

    int32_t ret = OH_AudioCodec_RegisterCallback(audioDec_, g_cb, decSignal_.get());
    if (ret != 0) {
        DEMO_LOG("RegisterCallback failed!");
        return -1;
    }
    return 0;
}

int32_t AudioDecoderCapiMock::Start(int32_t sampleRate, int32_t channel, std::vector<uint8_t> *codecConfig)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channel);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    if (codecConfig != nullptr && codecConfig->size() > 0) {
        OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG, codecConfig->data(), codecConfig->size());
    }
    int32_t ret = OH_AudioCodec_Configure(audioDec_, format);
    if (ret != 0) {
        DEMO_LOG("AudioDecoderCapiMock::Start configure failed!");
    }
    ret = OH_AudioCodec_Start(audioDec_);
    hasStart_ = true;
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t AudioDecoderCapiMock::Stop()
{
    int ret = 0;
    int retCode = 0;
    if (hasStart_) {
        std::unique_lock<std::mutex> inLock(decSignal_->inMutex);
        if (decSignal_->inQueue.size() < 1) {
            decSignal_->inCond.wait(inLock, [this]() {return (decSignal_->inQueue.size() > 0);});
        }
        auto buffer = decSignal_->inBufferQueue.front();
        int32_t index = decSignal_->inQueue.front();
        OH_AVCodecBufferAttr attr;
        memset_s(&attr, sizeof(attr), 0, sizeof(attr));
        attr.size = 1;
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        ret = OH_AVBuffer_SetBufferAttr(buffer, &attr);
        if (ret != 0) {
            retCode |= 0x01;
        }

        ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
        if (ret != 0) {
            retCode |= 0x02;
        }
        decSignal_->inBufferQueue.pop();
        decSignal_->inQueue.pop();
        ret = OH_AudioCodec_Stop(audioDec_);
        if (ret != 0) {
            retCode |= 0x04;
        }
    }
    ret = OH_AudioCodec_Destroy(audioDec_);
    if (ret != 0) {
        retCode |= 0x08;
    }
    return retCode;
}

int32_t AudioDecoderCapiMock::DecodeInput(const uint8_t *dataIn, uint32_t inSizeBytes, std::vector<uint8_t> *skipInfo)
{
    int32_t ret = 0;
    uint32_t index = 0;
    outputPts_ = 0;
    outputFlag_ = 0;
    if (inSizeBytes == 0) {
        return 1;
    }
    // 解码输入
    std::unique_lock<std::mutex> inLock(decSignal_->inMutex);
    if (decSignal_->inQueue.size() < 1) {
        decSignal_->inCond.wait_for(inLock, std::chrono::milliseconds(TIME_OUT_MS),
            [this]() {return (decSignal_->inQueue.size() > 0);});
    }
    if (decSignal_->inQueue.size() < 1) {
        DEMO_LOG("AudioDecoderCapiMock::Decode wait for in buffer time out");
        return -1;
    }

    OH_AVCodecBufferAttr inAttr;
    memset_s(&inAttr, sizeof(inAttr), 0, sizeof(inAttr));
    inAttr.size = inSizeBytes;
    inAttr.flags = flag_;
    inAttr.pts = pts_;
    auto inBuffer = decSignal_->inBufferQueue.front();
    if (skipInfo != nullptr) {
        auto avFormat = OH_AVBuffer_GetParameter(inBuffer);
        if (avFormat != nullptr) {
            OH_AVFormat_SetBuffer(avFormat, OH_MD_KEY_BUFFER_SKIP_SAMPLES_INFO, skipInfo->data(), skipInfo->size());
            OH_AVBuffer_SetParameter(inBuffer, avFormat);
            OH_AVFormat_Destroy(avFormat);
        }
    }
    index = decSignal_->inQueue.front();
    if (memcpy_s(OH_AVBuffer_GetAddr(inBuffer), inSizeBytes, dataIn, inSizeBytes) != EOK) {
        DEMO_LOG("DecodeInput memcpy_s failed!");
    }
    ret = OH_AVBuffer_SetBufferAttr(inBuffer, &inAttr);
    decSignal_->inBufferQueue.pop();
    decSignal_->inQueue.pop();
    ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
    if (ret < 0) {
        DEMO_LOG("AudioDecoderCapiMock::Decode push input buffer failed!");
        return ret;
    }
    return 0;
}

int32_t AudioDecoderCapiMock::DecodeOutput(uint8_t *dataOut, int32_t &outSizeBytes)
{
    int32_t ret = 0;
    uint32_t index = 0;
    // 解码输出
    OH_AVCodecBufferAttr outAttr;
    memset_s(&outAttr, sizeof(outAttr), 0, sizeof(outAttr));
    std::unique_lock<std::mutex> outLock(decSignal_->outMutex);
    if (decSignal_->outQueue.size() < 1) {
        decSignal_->outCond.wait_for(outLock, std::chrono::milliseconds(TIME_OUT_MS),
            [this]() {return (decSignal_->outQueue.size() > 0);});
    }
    if (decSignal_->outQueue.size() < 1) {
        return decSignal_->outQueue.size();
    }

    auto outBuffer = decSignal_->outBufferQueue.front();
    ret = OH_AVBuffer_GetBufferAttr(outBuffer, &outAttr);
    if (ret != 0) {
        DEMO_LOG("DecodeOutput get buffer failed! ret:%d", ret);
    }
    if (dataOut != nullptr) {
        if (memcpy_s(dataOut, outAttr.size, OH_AVBuffer_GetAddr(outBuffer), outAttr.size) != EOK) {
            DEMO_LOG("DecodeOutput memcpy_s failed!");
        }
    }
    index = decSignal_->outQueue.front();
    decSignal_->outBufferQueue.pop();
    decSignal_->outQueue.pop();
    ret = OH_AudioCodec_FreeOutputBuffer(audioDec_, index);
    if (ret != 0) {
        DEMO_LOG("DecodeOutput free buffer failed! ret:%d", ret);
    }
    outSizeBytes = outAttr.size;
    outputFlag_ = outAttr.flags;
    outputPts_ = outAttr.pts;
    return decSignal_->outQueue.size();
}
} // namespace MediaAVCodec
} // namespace OHOS