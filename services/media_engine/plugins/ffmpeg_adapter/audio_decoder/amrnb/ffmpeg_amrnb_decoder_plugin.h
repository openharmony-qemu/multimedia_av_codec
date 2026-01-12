/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef FFMPEG_AMRNB_DECODER_PLUGIN_H
#define FFMPEG_AMRNB_DECODER_PLUGIN_H
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "ffmpeg_base_decoder.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
class FFmpegAmrnbDecoderPlugin : public CodecPlugin {
public:
    explicit FFmpegAmrnbDecoderPlugin(const std::string& name);

    ~FFmpegAmrnbDecoderPlugin();

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

    Status ExtractOneAmrnbFrame();

    Status SetDataCallback(DataCallback *dataCallback) override
    {
        dataCallback_ = dataCallback;
        basePlugin->SetCallback(dataCallback_);
        return Status::OK;
    }

private:
    Status CheckInit(const std::shared_ptr<Meta> &format);
    int32_t GetInputBufferSize();
    int32_t GetOutputBufferSize();

private:
    std::mutex amrMutex_;
    int channels;
    int sampleRate;
    DataCallback *dataCallback_{nullptr};
    std::unique_ptr<FfmpegBaseDecoder> basePlugin;
    int64_t nextPts_ = 0;
    std::vector<uint8_t> inputBuf_;
    std::shared_ptr<AVCodec> avCodec_;
    std::shared_ptr<AVFrame> frame_;
    std::shared_ptr<AVCodecContext> avCodecContext_;
    AudioSampleFormat sampleFormat_ = INVALID_WIDTH;
    int32_t nbSamplesPerFrame_ = 0;
    std::shared_ptr<AVPacket> avPacket_;
    bool eosFlushed_ = false;
};
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif