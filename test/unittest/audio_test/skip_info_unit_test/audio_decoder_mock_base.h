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
#ifndef AUDIO_DECODER_MOCK_BASE
#define AUDIO_DECODER_MOCK_BASE
#include <vector>

namespace OHOS {
namespace MediaAVCodec {

class AudioDecoderMockBase {
public:
    AudioDecoderMockBase() = default;
    virtual ~AudioDecoderMockBase() = default;
    virtual int32_t Create(const char *mimetype) = 0;
    virtual int32_t CreateByName(const char *name) = 0;
    virtual int32_t Start(int32_t sampleRate, int32_t channel, std::vector<uint8_t> *codecConfig) = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t DecodeInput(const uint8_t *dataIn, uint32_t inSizeBytes, std::vector<uint8_t> *skipInfo) = 0;
    virtual int32_t DecodeOutput(uint8_t *dataOut, int32_t &outSizeBytes) = 0;
    void SetBlockAlign(int32_t blockAlign)
    {
        blockAlign_ = blockAlign;
    }
    void SetPts(int64_t pts)
    {
        pts_ = pts;
    }
    int64_t GetOutputPts()
    {
        return outputPts_;
    }
    void SetFlag(uint32_t flag)
    {
        flag_ = flag;
    }
    uint32_t GetOutputFlag()
    {
        return outputFlag_;
    }
    static std::unique_ptr<AudioDecoderMockBase> CreateDecoder();
protected:
    int32_t blockAlign_ = 0;
    int64_t pts_ = 0;
    int64_t outputPts_ = 0;
    uint32_t flag_ = 0;
    uint32_t outputFlag_ = 0;
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif  // AUDIO_DECODER_MOCK_BASE