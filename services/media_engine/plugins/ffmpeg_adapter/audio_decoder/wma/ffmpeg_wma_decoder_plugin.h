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
#ifndef FFMPEG_WMA_DECODER_PLUGIN_H
#define FFMPEG_WMA_DECODER_PLUGIN_H

#include "ffmpeg_base_decoder.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {

class FFmpegWMADecoderPlugin : public CodecPlugin {
public:
    explicit FFmpegWMADecoderPlugin(const std::string& name);
    ~FFmpegWMADecoderPlugin();

    Status Init() override;
    Status Prepare() override;
    Status Reset() override;
    Status Start() override;
    Status Stop() override;

    Status SetParameter(const std::shared_ptr<Meta> &meta) override;
    Status GetParameter(std::shared_ptr<Meta> &meta) override;

    Status QueueInputBuffer(const std::shared_ptr<AVBuffer> &in) override;
    Status QueueOutputBuffer(std::shared_ptr<AVBuffer> &out) override;

    Status GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &) override
	{
        return Status::OK;
    }
	
    Status GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &) override
	{
        return Status::OK;
    }

    Status Flush() override;
    Status Release() override;

    Status SetDataCallback(DataCallback *cb) override
    {
        dataCallback_ = cb;
        base_->SetCallback(dataCallback_);
        return Status::OK;
    }

private:
    bool CheckFormat(const std::shared_ptr<Meta> &meta);
    bool CheckChannelCount(const std::shared_ptr<Meta> &meta);
    bool CheckSampleRate(const std::shared_ptr<Meta> &meta) const;
    bool CheckSampleFormat(const std::shared_ptr<Meta> &meta);

    int32_t GetInputBufferSize();
    int32_t GetOutputBufferSize();

private:
    std::string ffCodecName_;
    int32_t channels_ {0};
    DataCallback *dataCallback_ {nullptr};
    std::unique_ptr<FfmpegBaseDecoder> base_;
};

} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif