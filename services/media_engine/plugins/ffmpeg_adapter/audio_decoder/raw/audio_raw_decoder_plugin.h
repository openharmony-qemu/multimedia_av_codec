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
#ifndef AUDIO_RAW_DECODER_PLUGIN_H
#define AUDIO_RAW_DECODER_PLUGIN_H
#include "plugin/codec_plugin.h"
namespace OHOS {
namespace Media {
namespace Plugins {
namespace Audio {
class AudioRawDecoderPlugin : public CodecPlugin {
    public:
    explicit AudioRawDecoderPlugin(const std::string &name);

    ~AudioRawDecoderPlugin();

    Status Init() override;

    Status Prepare() override;

    Status Reset() override;

    Status Start() override;

    Status Stop() override;

    Status SetParameter(const std::shared_ptr<Meta> &parameter) override;

    Status GetParameter(std::shared_ptr<Meta> &parameter) override;

    Status QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer) override;

    Status QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer) override;

    Status GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers) override;

    Status GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers) override;

    Status Flush() override;

    Status Release() override;

    Status SetDataCallback(DataCallback *dataCallback) override
    {
        dataCallback_ = dataCallback;
        return Status::OK;
    }

private:
    Status GetMetaData(const std::shared_ptr<Meta> &meta);
    bool CheckFormat();
    int32_t GetFormatBytes(AudioSampleFormat format);
    void SetOutputBasicInfo(std::shared_ptr<AVBuffer> &outputBuffer);
    Status ConvertF64BEToF32LE(const uint8_t *ptr, int32_t &size);
    Status F64BEToF32LE(const uint8_t *src, float *des);
    double F64BEToDouble(const uint8_t *src);

private:
    AudioSampleFormat audioSampleFormat_;
    AudioSampleFormat srcSampleFormat_;
    int32_t pts_;
    int32_t channels_;
    int32_t sampleRate_;
    int32_t offset_;
    int32_t frameSize_;
    int32_t maxInputSize_;
    uint32_t maxOutputSize_;
    int32_t durationTime_;
    bool eosFlag_;
    DataCallback *dataCallback_;
    std::shared_ptr<Meta> format_;
    std::vector<uint8_t> inputBuffer_;
};
} // namespace OHOS
} // namespace Media
} // namespace Plugins
} // namespace Audio
#endif