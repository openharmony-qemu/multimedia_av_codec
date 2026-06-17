/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef AUDIO_FFMPEG_OPUS_PLUGIN_EXIT_H
#define AUDIO_FFMPEG_OPUS_PLUGIN_EXIT_H

#include "audio_base_codec_ext.h"
#include "opus.h"
#include "opus_types.h"
#include "opus_private.h"
#include "opus_multistream.h"
#include <mutex>
#include "common/log.h"

#ifndef LOG_DOMAIN
#define LOG_DOMAIN 0xD002B31
#endif
#ifndef LOG_TAG
#define LOG_TAG "Opus"
#endif

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_AUDIO, "Opus" };
namespace OHOS {
namespace MediaAVCodec {
class OpusPluginExt {};
class AudioOpusEncoderPlugin : public AudioBaseCodecExt {
public:
    AudioOpusEncoderPlugin();
    ~AudioOpusEncoderPlugin() override;

    int32_t Init() override;
    int32_t ProcessSendData(void *inputBuffer, int32_t *len) override;
    int32_t ProcessRecieveData(unsigned char **outBuffer, int32_t len) override;
    int32_t SetPluginParameter(int32_t key, int32_t value) override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t Flush() override;
    int32_t GetInputBufferSize() override;
    int32_t GetOutputBufferSize() override;

private:
    int32_t channels_;
    int32_t ret_;
    OpusEncoder *enc_ = nullptr;
    mutable std::mutex avMutext_;

    int32_t frameSize_;
    uint8_t *data_ = nullptr;
    int16_t *in_ = nullptr;
    int32_t samplingRate_;
    int32_t bitrate_;
    int32_t complianceLevel_;
};

class AudioOpusDecoderPlugin : public AudioBaseCodecExt {
public:
    AudioOpusDecoderPlugin();
    ~AudioOpusDecoderPlugin() override;

    int32_t Init() override;
    int32_t ProcessSendData(void *inputBuffer, int32_t *len) override;
    int32_t ProcessRecieveData(unsigned char **outBuffer, int32_t len) override;
    int32_t SetPluginParameter(int32_t key, int32_t value) override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t Flush() override;
    int32_t GetInputBufferSize() override;
    int32_t GetOutputBufferSize() override;

private:
    int32_t channels_;
    int32_t ret_;
    OpusDecoder *dec_ = nullptr;
    mutable std::mutex avMutext_;

    int32_t frameSize_;
    int16_t *data_ = nullptr;
    uint8_t *in_ = nullptr;
    int32_t samplingRate_;
};

enum struct PluginParam : int32_t {
    CHANNEL = 0,
    SAMPLE_RATE = 1,
    BIT_RATE = 2,
    COMPLEXITY = 3,
};
}  // namespace MediaAVCodec
}  // namespace OHOS

extern "C" __attribute__((visibility("default"))) int32_t OpusPluginClassEncoderCreate(
    OHOS::MediaAVCodec::AudioBaseCodecExt **pluginPtr);
extern "C" __attribute__((visibility("default"))) int32_t OpusPluginClassDecoderCreate(
    OHOS::MediaAVCodec::AudioBaseCodecExt **pluginPtr);
#endif