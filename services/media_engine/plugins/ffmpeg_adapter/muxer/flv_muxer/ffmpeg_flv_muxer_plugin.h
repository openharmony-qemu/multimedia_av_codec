/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_FFMPEG_FLV_MUXER_PLUGIN_H
#define AVCODEC_FFMPEG_FLV_MUXER_PLUGIN_H

#include <mutex>
#include <unordered_map>
#include <string>
#include <sstream>
#include <iomanip>
#include "plugin/muxer_plugin.h"
#include "stream_parser_manager.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
class FFmpegFlvMuxerPlugin : public MuxerPlugin {
public:
    explicit FFmpegFlvMuxerPlugin(std::string name);
    ~FFmpegFlvMuxerPlugin() override;

    Status SetDataSink(const std::shared_ptr<DataSink> &dataSink) override;
    Status SetParameter(const std::shared_ptr<Meta> &param) override;
    Status SetUserMeta(const std::shared_ptr<Meta> &userMeta) override;
    Status AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc) override;
    Status Start() override;
    Status WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample) override;
    Status Stop() override;
    Status Reset() override;

private:
    Status SetLocation(std::shared_ptr<Meta> param);
    Status SetMetaData(std::shared_ptr<Meta> param);

private:
    Status SetCodecParameterOfAudioTrack(AVStream* stream, const std::shared_ptr<Meta> &trackDesc);
    Status SetCodecParameterOfVideoTrack(AVStream* stream, const std::shared_ptr<Meta> &trackDesc);
    Status SetCodecParameterExtra(AVStream* stream, const uint8_t* extraData, int32_t extraDataSize);
    Status SetCodecParameterColor(AVStream* stream, const std::shared_ptr<Meta> &trackDesc);
    Status SetCodecParameterColorByParser(AVStream* stream);
    Status SetCodecParameterVideoDelay(AVStream* stream);
    Status AddAudioTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc, AVCodecID codeID);
    Status AddVideoTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc, AVCodecID codeID);
    Status WriteNormal(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample, int32_t dataOffset);
    void CheckChannelLayout(uint64_t channelLayout, const std::shared_ptr<Meta> &trackDesc);

private:
    std::shared_ptr<AVPacket> cachePacket_ {};
    std::shared_ptr<AVOutputFormat> outputFormat_ {};
    std::shared_ptr<AVFormatContext> formatContext_ {};
    bool isWriteHeader_ {false};
    bool isColorSet_ = {false};
    bool canReadFile_ = {false};
    bool isFirstFrameVideo_ {true};
    std::shared_ptr<StreamParserManager> hevcParser_ {nullptr};
    std::mutex mutex_;
};
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_FFMPEG_FLV_MUXER_PLUGIN_H
