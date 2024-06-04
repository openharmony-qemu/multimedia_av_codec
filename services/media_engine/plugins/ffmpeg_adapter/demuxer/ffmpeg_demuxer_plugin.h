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

#ifndef FFMPEG_DEMUXER_PLUGIN_H
#define FFMPEG_DEMUXER_PLUGIN_H

#include <atomic>
#include <vector>
#include <thread>
#include <map>
#include <shared_mutex>
#include "buffer/avbuffer.h"
#include "plugin/demuxer_plugin.h"
#include "block_queue_pool.h"
#include "stream_parser_manager.h"
#include "meta/meta.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/dict.h"
#include "libavutil/opt.h"
#include "libavutil/parseutils.h"
#include "libavcodec/bsf.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
class FFmpegDemuxerPlugin : public DemuxerPlugin {
public:
    explicit FFmpegDemuxerPlugin(std::string name);
    ~FFmpegDemuxerPlugin() override;
    Status Reset() override;
    Status Start() override;
    Status Stop() override;
    Status Flush() override;
    Status SetDataSource(const std::shared_ptr<DataSource>& source) override;
    Status GetMediaInfo(MediaInfo& mediaInfo) override;
    Status GetUserMeta(std::shared_ptr<Meta> meta) override;
    Status SelectTrack(uint32_t trackId) override;
    Status UnselectTrack(uint32_t trackId) override;
    Status SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode, int64_t& realSeekTime) override;
    Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample) override;
    Status GetNextSampleSize(uint32_t trackId, int32_t& size) override;
    Status GetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo) override;

private:
    void ConvertCsdToAnnexb(const AVStream& avStream, Meta &format);
    int64_t GetFileDuration(const AVFormatContext& avFormatContext);
    int64_t GetStreamDuration(const AVStream& avStream);

    static int AVReadPacket(void* opaque, uint8_t* buf, int bufSize);
    static int AVWritePacket(void* opaque, uint8_t* buf, int bufSize);
    static int64_t AVSeek(void* opaque, int64_t offset, int whence);
    AVIOContext* AllocAVIOContext(int flags);
    void InitAVFormatContext();

    void InitBitStreamContext(const AVStream& avStream);
    void ConvertAvcToAnnexb(AVPacket& pkt);
    void PushEOSToAllCache();
    void ShowSelectedTracks();
    bool IsInSelectedTrack(const uint32_t trackId);
    Status ReadPacketToCacheQueue(const uint32_t readId);
    void AddPacketToCacheQueue(AVPacket *pkt);
    Status SetDrmCencInfo(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket);
    Status ConvertAVPacketToSample(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket);
    void ConvertAvccToAnnexb(std::shared_ptr<AVBuffer> sample, AVPacket* avpacket,
        std::shared_ptr<SamplePacket> dstSamplePacket);
    Status ReadEosSample(std::shared_ptr<AVBuffer> sample);
    Status WriteBuffer(std::shared_ptr<AVBuffer> outBuffer, int64_t pts, uint32_t flag, const uint8_t *writeData,
        int32_t writeSize);
    void ParseDrmInfo(const MetaDrmInfo *const metaDrmInfo, int32_t drmInfoSize,
        std::multimap<std::string, std::vector<uint8_t>>& drmInfo);
    bool GetNextFrame(const uint8_t *data, const uint32_t size);
    bool NeedCombineFrame(uint32_t trackId);
    AVPacket* CombinePackets(std::shared_ptr<SamplePacket> samplePacket);
    void ConvertHevcToAnnexb(AVPacket& pkt, std::shared_ptr<SamplePacket> samplePacket);

    int FindNaluSpliter(int size, const uint8_t *data);
    bool CanDropAvcPkt(const AVPacket& pkt);
    bool CanDropHevcPkt(const AVPacket& pkt);
    void SetDropTag(const AVPacket& pkt, std::shared_ptr<AVBuffer> sample, AVCodecID codecId);

    struct IOContext {
        std::shared_ptr<DataSource> dataSource {nullptr};
        int64_t offset {0};
        uint64_t fileSize {0};
        bool eos {false};
        std::atomic<bool> timeout {false};
    };

    std::mutex mutex_ {};
    std::shared_mutex sharedMutex_;
    std::unordered_map<uint32_t, std::shared_ptr<std::mutex>> trackMtx_;
    Seekable seekable_;
    IOContext ioContext_;
    std::vector<uint32_t> selectedTrackIds_;
    BlockQueuePool cacheQueue_;

    std::shared_ptr<AVInputFormat> pluginImpl_ {nullptr};
    std::shared_ptr<AVFormatContext> formatContext_ {nullptr};
    std::shared_ptr<AVBSFContext> avbsfContext_ {nullptr};
    std::shared_ptr<StreamParserManager> streamParser_ {nullptr};
    bool streamParserInited_ {false};

    void GetVideoFirstKeyFrame(uint32_t trackIndex);
    void ParseHEVCMetadataInfo(const AVStream& avStream, Meta &format);
    AVPacket *firstFrame_ = nullptr;
};
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // FFMPEG_DEMUXER_PLUGIN_H