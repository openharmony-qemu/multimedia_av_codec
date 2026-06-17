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

#ifndef AUDIO_OPUS_DECODER_PLUGIN_H
#define AUDIO_OPUS_DECODER_PLUGIN_H

#include <functional>
#include <map>
#include <mutex>
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "opus_plugin_base.h"
#include <dlfcn.h>
#include <vector>
#include <memory>

#ifdef DUMP_RAW_DATA
#include <fstream>
#endif

/// End of Stream Buffer Flag
#define BUFFER_FLAG_EOS 0x00000001

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Opus {
enum ParaType : int32_t {
    CHANNEL = 0,
    SAMPLE_RATE = 1,
    BIT_RATE = 2,
    COMPLEXITY = 3,
    STREAMS = 4,
    COUPLED = 5,
    MAPPING = 6,
};

class AudioOpusDecoderPlugin : public CodecPlugin {
public:
    explicit AudioOpusDecoderPlugin(std::string name);

    ~AudioOpusDecoderPlugin();

    Status Init() override;

    Status Prepare() override;

    Status Reset() override;

    Status Start() override;

    Status Stop() override;

    Status SetParameter(const std::shared_ptr<Meta> &parameter) override;

    Status GetParameter(std::shared_ptr<Meta> &parameter) override;

    Status QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputAvBuffer) override;

    Status QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer) override;

    Status GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers) override;

    Status GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers) override;

    Status Flush() override;

    Status Release() override;

    Status SetDataCallback(DataCallback* dataCallback) override
    {
        dataCallback_ = dataCallback;
        return Status::OK;
    }

private:
    Status SetDecoderConfig(const std::shared_ptr<Meta> &parameter);

    Status SkipSamplesProcess(
        std::vector<uint8_t> &skipInfo, std::shared_ptr<AVBuffer> &outputBuffer, uint32_t decSamples);

    MediaAVCodec::AudioOpusDecoderPluginBase *pluginCodecPtr_;
    mutable std::mutex avMutex_ {};
    mutable std::mutex paramMutex_ {};
    uint8_t *fbytes_;
    int32_t decodeLen_;
    uint8_t *codeData_;
    DataCallback* dataCallback_ {nullptr};
    std::shared_ptr<AVBuffer> outBuffer_ {nullptr};
    mutable std::mutex bufferMetaMutex_ {};
    int32_t maxInputSize_;
    int32_t maxOutputSize_;
    int32_t channels_;
    int32_t sampleRate_;
    int64_t pts_;
    Meta audioParameter_;
    std::shared_ptr<Meta> bufferMeta_ {nullptr};
    AudioSampleFormat sampleFormat_;
    bool isMultistream_;
    int32_t streams_;
    int32_t coupled_;
    std::vector<uint8_t> mapping_ = {};
    uint32_t skipSamples_ = 0;
    bool isEnableSkipSamples_ = false;
};
}  // namespace Opus
}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS

#endif // AUDIO_OPUS_DECODER_PLUGIN_H