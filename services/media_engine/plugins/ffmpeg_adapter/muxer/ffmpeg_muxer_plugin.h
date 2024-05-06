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
class FFmpegMuxerPlugin : public MuxerPlugin {
public:
    explicit FFmpegMuxerPlugin(std::string name);
    ~FFmpegMuxerPlugin() override;

    Status SetDataSink(const std::shared_ptr<DataSink> &dataSink) override;
    Status SetParameter(const std::shared_ptr<Meta> &param) override;
    Status SetUserMeta(const std::shared_ptr<Meta> &userMeta) override;
    Status AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc) override;
    Status Start() override;
    Status WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample) override;
    Status Stop() override;
    Status Reset() override;

private:
    Status SetRotation(std::shared_ptr<Meta> param);
    Status SetLocation(std::shared_ptr<Meta> param);
    Status SetMetaData(std::shared_ptr<Meta> param);

private:
    Status SetCodecParameterOfTrack(AVStream *stream, const std::shared_ptr<Meta> &trackDesc);
    Status SetCodecParameterExtra(AVStream *stream, const uint8_t *extraData, int32_t extraDataSize);
    Status SetCodecParameterColor(AVStream* stream, const std::shared_ptr<Meta> &trackDesc);
    Status SetCodecParameterColorByParser(AVStream* stream);
    Status SetCodecParameterCuva(AVStream* stream, const std::shared_ptr<Meta> &trackDesc);
    Status SetCodecParameterCuvaByParser(AVStream *stream);
    Status SetDisplayMatrix(AVStream* stream);
    Status AddAudioTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc, AVCodecID codeID);
    Status AddVideoTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc, AVCodecID codeID, bool isCover);
    Status WriteNormal(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample);
    Status WriteVideoSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample);
    std::vector<uint8_t> TransAnnexbToMp4(const uint8_t *sample, int32_t size);
    uint8_t *FindNalStartCode(const uint8_t *buf, const uint8_t *end, int32_t &startCodeLen);
    bool IsAvccSample(const uint8_t* sample, int32_t size, int32_t nalSizeLen);
    Status SetNalSizeLen(AVStream *stream, const std::vector<uint8_t> &codecConfig);
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

    struct VideoSampleInfo {
        bool isFirstFrame_ {true};
        bool isNeedTransData_ {false};
        std::vector<uint8_t> extraData_ {};
        int32_t nalSizeLen_ {0x04};
    };

    std::shared_ptr<AVPacket> cachePacket_ {};
    std::shared_ptr<AVOutputFormat> outputFormat_ {};
    std::shared_ptr<AVFormatContext> formatContext_ {};
    VideoRotation rotation_ { VIDEO_ROTATION_0 };
    bool isWriteHeader_ {false};
    bool isHdrVivid_ = {false};
    bool isColorSet_ = {false};
    bool isFastStart_ = {false};
    std::shared_ptr<StreamParserManager> hevcParser_ {nullptr};
    std::unordered_map<int32_t, VideoSampleInfo> videoTracksInfo_;
    std::mutex mutex_;
};
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_FFMPEG_MUXER_PLUGIN_H
