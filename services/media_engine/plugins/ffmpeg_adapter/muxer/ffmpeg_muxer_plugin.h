/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_FFMPEG_MUXER_PLUGIN_H
#define AVCODEC_FFMPEG_MUXER_PLUGIN_H

#include <mutex>
#include <unordered_map>
#include "plugin/muxer_plugin.h"

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
namespace Plugin {
namespace Ffmpeg {
class FFmpegMuxerPlugin : public MuxerPlugin {
public:
    explicit FFmpegMuxerPlugin(std::string name);
    ~FFmpegMuxerPlugin() override;

    Status SetDataSink(const std::shared_ptr<DataSink> &dataSink) override;
    Status SetParameter(std::shared_ptr<Meta> param) override;
    Status AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc) override;
    Status Start() override;
    Status WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample) override;
    Status Stop() override;
    Status Reset() override;

private:
    Status SetCodecParameterOfTrack(AVStream *stream, const std::shared_ptr<Meta> &trackDesc);
    Status SetCodecParameterColor(AVStream* stream, const std::shared_ptr<Meta> &trackDesc);
    Status SetCodecParameterCuva(AVStream* stream, const std::shared_ptr<Meta> &trackDesc);
    Status AddAudioTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc, AVCodecID codeID);
    Status AddVideoTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc, AVCodecID codeID, bool isCover);
    bool IsAvccSample(const uint8_t* sample, int32_t size);
    Status SetCodecConfigToCodecPar(uint32_t trackIndex, bool isAnnexB = false);
    static int32_t IoRead(void *opaque, uint8_t *buf, int bufSize);
    static int32_t IoWrite(void *opaque, uint8_t *buf, int bufSize);
    static int64_t IoSeek(void *opaque, int64_t offset, int whence);
    static AVIOContext *InitAvIoCtx(const std::shared_ptr<DataSink> &dataSink, int writeFlags);
    static void DeInitAvIoCtx(AVIOContext *ptr);
    static int32_t IoOpen(AVFormatContext *s, AVIOContext **pb, const char *url, int flags, AVDictionary **options);
    static void IoClose(AVFormatContext *s, AVIOContext *pb);

private:
    struct IOContext {
        std::shared_ptr<DataSink> dataSink_ {};
        int64_t pos_ {0};
        int64_t end_ {0};
    };
    std::shared_ptr<AVPacket> cachePacket_ {};
    std::shared_ptr<AVOutputFormat> outputFormat_ {};
    std::shared_ptr<AVFormatContext> formatContext_ {};
    VideoRotation rotation_ { VIDEO_ROTATION_0 };
    bool isWriteHeader_ {false};
    std::unordered_map<int32_t, std::pair<bool, std::vector<uint8_t>>> codecConfigs_;
    std::mutex mutex_;
};
} // namespace Ffmpeg
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_FFMPEG_MUXER_PLUGIN_H
