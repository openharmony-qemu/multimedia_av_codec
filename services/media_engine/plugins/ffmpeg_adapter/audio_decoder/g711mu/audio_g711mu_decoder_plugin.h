/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_AUDIO_G711MU_DECODER_PLUGIN_H
#define HISTREAMER_AUDIO_G711MU_DECODER_PLUGIN_H

#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include <functional>
#include <map>
#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
};
#endif

/// End of Stream Buffer Flag
#define BUFFER_FLAG_EOS 0x00000001

namespace OHOS {
namespace Media {
namespace Plugin {
namespace G711mu {
class AudioG711muDecoderPlugin : public CodecPlugin {
public:
    explicit AudioG711muDecoderPlugin(std::string name);

    ~AudioG711muDecoderPlugin();

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

    Status SetDataCallback(DataCallback* dataCallback) override
    {
        dataCallback_ = dataCallback;
        return Status::OK;
    }

private:

    bool CheckFormat();
    mutable std::mutex parameterMutex_ {};
    Meta audioParameter_ ;
    mutable std::mutex avMutex_ {};
    std::shared_ptr<AVBuffer> outBuffer_ {nullptr};
    DataCallback* dataCallback_ {nullptr};

    int16_t G711MuLawDecode(uint8_t muLawValue);

    std::vector<int16_t> decodeResult_;
    int32_t channels_;
    int32_t sampleRate_;
    int32_t maxInputSize_;
    int32_t maxOutputSize_;
};
} // namespace G711mu
} // namespace Plugin
} // namespace Media
} // namespace OHOS

#endif // HISTREAMER_AUDIO_G711MU_DECODER_PLUGIN_H