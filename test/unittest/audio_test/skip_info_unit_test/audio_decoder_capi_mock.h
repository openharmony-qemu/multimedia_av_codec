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
#ifndef AUDIO_DECODER_CAPI_MOCK
#define AUDIO_DECODER_CAPI_MOCK
#include <mutex>
#include <queue>
#include <thread>
#include "audio_decoder_mock_base.h"
#include "native_avcodec_audiocodec.h"
#include "native_avformat.h"
#include "native_avcodec_base.h"
#include "native_avbuffer.h"

namespace OHOS {
namespace MediaAVCodec {
struct DecSignal {
    std::mutex inMutex;
    std::mutex outMutex;
    std::condition_variable inCond;
    std::condition_variable outCond;
    std::queue<uint32_t> inQueue;
    std::queue<uint32_t> outQueue;
    std::queue<OH_AVBuffer*> inBufferQueue;
    std::queue<OH_AVBuffer*> outBufferQueue;
};

class AudioDecoderCapiMock : public AudioDecoderMockBase {
public:
    AudioDecoderCapiMock()
    {
        decSignal_ = std::make_shared<DecSignal>();
    }
    ~AudioDecoderCapiMock() override = default;
    int32_t Create(const char *mimetype) override;
    int32_t CreateByName(const char *name) override;
    int32_t Start(int32_t sampleRate, int32_t channel, std::vector<uint8_t> *codecConfig) override;
    int32_t Stop() override;
    int32_t DecodeInput(const uint8_t *dataIn, uint32_t inSizeBytes, std::vector<uint8_t> *skipInfo) override;
    int32_t DecodeOutput(uint8_t *dataOut, int32_t &outSizeBytes) override;

private:
    std::shared_ptr<DecSignal> decSignal_ = nullptr;
    OH_AVCodec *audioDec_ = nullptr;
    bool hasStart_ = false;
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif  // AUDIO_DECODER_CAPI_MOCK