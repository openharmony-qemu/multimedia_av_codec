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

    ~AudioRawDecoderPlugin() override;

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
    Status ConvertBEToLE(const uint8_t *ptr, int32_t &size);
    Status ConvertSampleFormat(const uint8_t *ptr, int32_t &size);
    Status ConvertS8(const uint8_t *ptr, int32_t &size);
    Status ConvertF64LE(const uint8_t *ptr, int32_t &size);
    Status ConvertS64LE(const uint8_t *ptr, int32_t &size);
    Status ConvertS8P(const uint8_t *ptr, int32_t &size);
    Status ConvertS16LEP(const uint8_t *ptr, int32_t &size);
    Status ConvertS16BEP(const uint8_t *ptr, int32_t &size);
    Status ConvertS24LEP(const uint8_t *ptr, int32_t &size);
    Status ConvertS32LEP(const uint8_t *ptr, int32_t &size);
    Status ConvertDVD(const uint8_t *ptr, int32_t &size);
    Status ConvertBluray(const uint8_t *ptr, int32_t &size);
    Status ConvertDVD16Bits(const uint8_t *ptr, int32_t &size);
    Status ConvertDVD20Bits(const uint8_t *ptr, int32_t &size);
    Status ConvertDVD24Bits(const uint8_t *ptr, int32_t &size);

    void WriteU8(int32_t idx, uint8_t value);
    void WriteS16LE(int32_t idx, int16_t value);
    void WriteS24LE(int32_t idx, int32_t value);
    void WriteS32LE(int32_t idx, int32_t value);
    void WriteF32LE(int32_t idx, float value);

    using ConvertFunc = std::function<void(int32_t, float)>;
    ConvertFunc GetConverter(const AudioSampleFormat& format);

    template <typename SourceParser>
    Status ConvertGeneric(const uint8_t* ptr, int32_t& size,
                            SourceParser normParser, bool isPlanar)
    {
        int32_t srcBytesSize = GetFormatBytes(srcSampleFormat_);
        int32_t destBytesSize = GetFormatBytes(audioSampleFormat_);
        if (srcBytesSize == 0 || destBytesSize == 0) {
            return Status::ERROR_UNKNOWN;
        }
        if (destBytesSize == 0) {
            return Status::ERROR_UNSUPPORTED_FORMAT;
        }

        if (isPlanar) {
            if (size % (channels_ * srcBytesSize) != 0) {
                return Status::ERROR_NOT_ENOUGH_DATA;
            }
        } else {
            if (size % srcBytesSize != 0) {
                return Status::ERROR_NOT_ENOUGH_DATA;
            }
        }

        int32_t totalSamples = size / (channels_ * srcBytesSize);
        int32_t outputSize = totalSamples * channels_ * destBytesSize;
        inputBuffer_.resize(outputSize);

        auto converter = GetConverter(audioSampleFormat_);
        if (!converter) {
            return Status::ERROR_UNSUPPORTED_FORMAT;
        }

        for (int32_t frame = 0; frame < totalSamples; frame++) {
            for (int32_t channel = 0; channel < channels_; channel++) {
                int32_t inputIdx = isPlanar
                    ? (channel * totalSamples + frame) * srcBytesSize
                    : (frame * channels_ + channel) * srcBytesSize;
                int32_t outputIdx = (frame * channels_ + channel) * destBytesSize;

                float normalized = normParser(&ptr[inputIdx]);

                converter(outputIdx, normalized);
            }
        }

        size = outputSize;
        return Status::OK;
    }

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