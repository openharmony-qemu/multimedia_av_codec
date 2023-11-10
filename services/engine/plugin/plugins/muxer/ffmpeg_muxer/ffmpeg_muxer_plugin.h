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

#ifndef FFMPEG_MUXER_PLUGIN_H
#define FFMPEG_MUXER_PLUGIN_H

#include "muxer_plugin.h"
#include "hevc_parser_manager.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
namespace Ffmpeg {
class FFmpegMuxerPlugin : public MuxerPlugin {
public:
    explicit FFmpegMuxerPlugin(std::string name);
    ~FFmpegMuxerPlugin() override;

    Status SetDataSink(const std::shared_ptr<DataSink>& dataSink) override;
    Status SetRotation(int32_t rotation) override;
    Status AddTrack(int32_t &trackIndex, const MediaDescription &trackDesc) override;
    Status Start() override;
    Status WriteSample(uint32_t trackIndex, const uint8_t *sample,
        AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    Status Stop() override;

private:
    Status SetCodecParameterOfTrack(AVStream *stream, const MediaDescription &trackDesc);
    Status SetCodecParameterExtra(AVStream *stream, const uint8_t *extraData, int32_t extraDataSize);
    Status SetCodecParameterColor(AVStream* stream, const MediaDescription& trackDesc);
    Status SetCodecParameterColorByParser(AVStream* stream);
    Status SetCodecParameterCuva(AVStream* stream, const MediaDescription& trackDesc);
    Status SetCodecParameterCuvaByParser(AVStream *stream);
    Status AddAudioTrack(int32_t &trackIndex, const MediaDescription &trackDesc, AVCodecID codeID);
    Status AddVideoTrack(int32_t &trackIndex, const MediaDescription &trackDesc, AVCodecID codeID, bool isCover);
    Status WriteNormal(uint32_t trackIndex, const uint8_t *sample, int32_t size, int64_t pts, AVCodecBufferFlag flag);
    Status WriteVideoSample(uint32_t trackIndex, const uint8_t *sample,
        AVCodecBufferInfo info, AVCodecBufferFlag flag);
    std::vector<uint8_t> TransAnnexbToMp4(const uint8_t *sample, int32_t size);
    uint8_t *FindNalStartCode(const uint8_t *buf, const uint8_t *end, int32_t &startCodeLen);
    bool IsAvccSample(const uint8_t* sample, int32_t size);
    static int32_t IoRead(void *opaque, uint8_t *buf, int bufSize);
    static int32_t IoWrite(void *opaque, uint8_t *buf, int bufSize);
    static int64_t IoSeek(void *opaque, int64_t offset, int whence);
    static AVIOContext *InitAvIoCtx(std::shared_ptr<DataSink> dataSink, int writeFlags);
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
    };

    std::shared_ptr<AVPacket> cachePacket_ {};
    std::shared_ptr<AVOutputFormat> outputFormat_ {};
    std::shared_ptr<AVFormatContext> formatContext_ {};
    int32_t rotation_ { 0 };
    bool isWriteHeader_ {false};
    int32_t isHdrVivid_ = {0};
    bool isColorSet_ = false;
    std::shared_ptr<HevcParserManager> hevcParser_ {nullptr};
    std::unordered_map<int32_t, VideoSampleInfo> videoTracksInfo_;
};
} // Ffmpeg
} // Plugin
} // MediaAVCodec
} // OHOS
#endif // FFMPEG_MUXER_PLUGIN_H
