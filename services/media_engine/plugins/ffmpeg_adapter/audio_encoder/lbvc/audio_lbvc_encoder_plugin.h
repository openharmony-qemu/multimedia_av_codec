/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef AUDIO_LBVC_ENCODER_PLUGIN_H
#define AUDIO_LBVC_ENCODER_PLUGIN_H

#include "hdi_codec.h"
#include "plugin/codec_plugin.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Lbvc {
class AudioLbvcEncoderPlugin : public CodecPlugin {
public:
    explicit AudioLbvcEncoderPlugin(const std::string &name);

    ~AudioLbvcEncoderPlugin();

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

private:
    AudioSampleFormat audioSampleFormat_;
    int32_t channels_;
    int32_t sampleRate_;
    int64_t bitRate_;
    uint32_t maxInputSize_;
    uint32_t maxOutputSize_;
    bool eosFlag_;
    DataCallback *dataCallback_;
    std::shared_ptr<Hdi::HdiCodec> hdiCodec_;
};
} // namespace Lbvc
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif