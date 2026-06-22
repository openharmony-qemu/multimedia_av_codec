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

#ifndef HISTREAMER_AUDIO_OPUS_ENCODER_PLUGIN_H
#define HISTREAMER_AUDIO_OPUS_ENCODER_PLUGIN_H

#include <functional>
#include <map>
#include <mutex>
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "opus_plugin_base.h"

#include <dlfcn.h>
#include <vector>
#include <memory>

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
};

class AudioOpusEncoderPlugin : public CodecPlugin {
public:
    explicit AudioOpusEncoderPlugin(std::string name);

    ~AudioOpusEncoderPlugin();

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

    Status SendOutputBuffer(std::shared_ptr<AVBuffer>& outputBuffer);

    uint8_t *fbytes_;
    int32_t encodeLen_;
    mutable std::mutex avMutex_ {};
    mutable std::mutex paramMutex_ {};
    std::shared_ptr<AVBuffer> outBuffer_ {nullptr};
    DataCallback* dataCallback_ {nullptr};
    Meta audioParameter_;
    MediaAVCodec::AudioOpusEncoderPluginBase *pluginCodecPtr_;
    int32_t channels_;
    int32_t sampleRate_;
    int64_t bitrate_;
    int32_t maxInputSize_;
    int32_t maxOutputSize_;
    int64_t pts_;
    int32_t ptsMode_ = DEFAULT_ENCODE_PTS_MODE;
    bool isFirstInputPts_ = true;
    int64_t prevPts_ = 0;
    mutable std::mutex bufferMetaMutex_ {};
    std::shared_ptr<Meta> bufferMeta_ {nullptr};
    uint8_t *codeData_;
    std::vector<uint8_t> pcmCache_;
    int32_t maxInputBufferSize_ {-1};
    bool enableCache_ {false};
    AudioSampleFormat srcFmt_;
    bool isEnableFormatConvert_ {false};
    std::vector<uint8_t> convertBuf_;
    bool CheckChannel(const std::shared_ptr<Meta> &parameter);
    bool CheckFormat(const std::shared_ptr<Meta> &parameter);
    bool CheckBitrate(const std::shared_ptr<Meta> &parameter);
    bool CheckSamplerate(const std::shared_ptr<Meta> &parameter);
};
}  // namespace Opus
}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS

#endif // HISTREAMER_AUDIO_OPUS_ENCODER_PLUGIN_H