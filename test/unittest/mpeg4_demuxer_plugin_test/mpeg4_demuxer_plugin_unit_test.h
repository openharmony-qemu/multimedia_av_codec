/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MPEG4_DEMUXER_PLUGIN_UNIT_TEST_H
#define MPEG4_DEMUXER_PLUGIN_UNIT_TEST_H

#include <gtest/gtest.h>

#include <climits>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "buffer/avbuffer.h"
#include "common/media_source.h"
#include "demuxer_plugin_manager.h"
#include "ffmpeg_demuxer_plugin.h"
#include "mpeg4_demuxer_plugin.h"
#include "plugin/demuxer_plugin.h"
#include "plugin/plugin_manager_v2.h"
#include "stream_demuxer.h"

namespace OHOS {
namespace Media {
constexpr uint32_t MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE = 3840 * 2160 * 3;
constexpr uint32_t MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT = 100;

using MediaAVBuffer = OHOS::Media::AVBuffer;
using BaseDemuxerPlugin = OHOS::Media::Plugins::DemuxerPlugin;
using FFmpegDemuxerPlugin = OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin;
using MPEG4DemuxerPlugin = OHOS::Media::Plugins::MPEG4::MPEG4DemuxerPlugin;

struct AVBufferWrapper {
    std::shared_ptr<MediaAVBuffer> mediaAVBuffer;
    explicit AVBufferWrapper(uint32_t size)
    {
        if (size == 0) {
            size = MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE;
        }
        ptr = std::make_unique<uint8_t[]>(size);
        mediaAVBuffer = MediaAVBuffer::CreateAVBuffer(ptr.get(), size, 0);
    }

private:
    std::unique_ptr<uint8_t[]> ptr;
};

struct TrackReadDigest {
    uint32_t trackId = 0;
    std::string mime;
    int32_t frameCount = 0;
    int32_t keyFrameCount = 0;
    int64_t firstPts = INT64_MIN;
    int64_t lastPts = INT64_MIN;
    std::vector<uint32_t> keyFrameIndexes;
};

struct ReadDigest {
    std::vector<TrackReadDigest> tracks;
};

struct SampleDigest {
    uint32_t trackId = 0;
    std::string mime;
    int32_t nextSampleSize = 0;
    int32_t actualSize = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t duration = 0;
    uint32_t flag = 0;
};

struct SeekExpectedResult {
    int64_t seekTimeMs = 0;
    Plugins::SeekMode seekMode = Plugins::SeekMode::SEEK_PREVIOUS_SYNC;
    bool seekSuccess = false;
    std::vector<int32_t> expectedFrames;
};

enum class ReadMode : uint32_t {
    COPY,
    ZERO_COPY,
};

class Mpeg4DemuxerPluginUnitTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void TearDown() override;

protected:
    void InitFfmpegPlugin(const std::string& filePath);
    void InitFfmpegPluginURI(const std::string& filePath);
    void InitMpeg4Plugin(const std::string& filePath);
    void InitMpeg4PluginURI(const std::string& filePath);
    void ResetPlugin(std::shared_ptr<BaseDemuxerPlugin>& plugin, int& fd);
    void InitPlugin(const std::string& filePath, const std::shared_ptr<BaseDemuxerPlugin>& plugin, int& fd,
        MediaInfo& mediaInfo);
    void InitPluginURI(const std::string& filePath, const std::shared_ptr<BaseDemuxerPlugin>& plugin,
        MediaInfo& mediaInfo);

    std::vector<uint32_t> SelectSupportedTracks(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
        const MediaInfo& mediaInfo);
    SampleDigest ReadFirstSample(const std::shared_ptr<BaseDemuxerPlugin>& plugin, const MediaInfo& mediaInfo,
        ReadMode readMode);
    ReadDigest ReadAllSamples(const std::shared_ptr<BaseDemuxerPlugin>& plugin, const MediaInfo& mediaInfo,
        const std::vector<uint32_t>& selectedTrackIds, ReadMode readMode = ReadMode::COPY);
    void CompareReadDigest(const ReadDigest& lhs, const ReadDigest& rhs);
    void CheckReadResultWithExpected(const std::string& filePath, const std::vector<int32_t>& expectedFrames,
        const std::vector<int32_t>& expectedKeyFrames, const std::vector<uint32_t>& expectedKeyFrameIndexes,
        bool useUri = false);
    void CheckReadDigestWithExpected(const ReadDigest& digest, const std::vector<int32_t>& expectedFrames,
        const std::vector<int32_t>& expectedKeyFrames, const std::vector<uint32_t>& expectedKeyFrameIndexes);
    void CompareSampleDigest(const SampleDigest& lhs, const SampleDigest& rhs);
    void CompareSeekReadResult(const std::string& filePath, const std::vector<int64_t>& seekTimes,
        const std::vector<Plugins::SeekMode>& seekModes, bool useUri = false);
    void CheckSeekReadResultWithExpected(const std::string& filePath,
        const std::vector<SeekExpectedResult>& expectedResults, bool useUri = false);

    bool IsSupportedTrack(const Meta& meta) const;
    bool IsEos(const std::shared_ptr<MediaAVBuffer>& buffer) const;
    bool IsSyncFrame(const std::shared_ptr<MediaAVBuffer>& buffer) const;
    Status ReadSampleWithRetry(const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId,
        const std::shared_ptr<MediaAVBuffer>& buffer, ReadMode readMode);
    Status GetNextSampleSizeWithRetry(const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId,
        int32_t& sampleSize);
    void InitPluginsForReadTest(const std::string& filePath, bool useUri);
    ReadDigest CreateReadDigest(const MediaInfo& mediaInfo, const std::vector<uint32_t>& selectedTrackIds) const;
    bool ReadTrackSample(const std::shared_ptr<BaseDemuxerPlugin>& plugin, TrackReadDigest& trackDigest,
        const std::shared_ptr<MediaAVBuffer>& buffer, ReadMode readMode, bool& eosReached);
    void CheckSeekSuccessDigest(const ReadDigest& actualDigest, const ReadDigest& expectedDigest);
    void CheckSeekSuccessDigestWithExpected(const ReadDigest& ffmpegDigest, const ReadDigest& mpeg4Digest,
        const std::vector<int32_t>& expectedFrames);
    void CheckSeekFallbackDigest(const ReadDigest& mpeg4Digest, const std::vector<uint32_t>& selectedTrackIds,
        const std::vector<int32_t>& expectedFrames);

protected:
    int ffmpegFd_ = -1;
    int mpeg4Fd_ = -1;
    MediaInfo ffmpegMediaInfo_ {};
    MediaInfo mpeg4MediaInfo_ {};
    std::shared_ptr<BaseDemuxerPlugin> ffmpegPlugin_ {nullptr};
    std::shared_ptr<BaseDemuxerPlugin> mpeg4Plugin_ {nullptr};
};
} // namespace Media
} // namespace OHOS

#endif // MPEG4_DEMUXER_PLUGIN_UNIT_TEST_H
