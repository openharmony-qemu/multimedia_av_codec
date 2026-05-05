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

#include "mpeg4_demuxer_plugin_unit_test.h"

#include <algorithm>
#include <cstdlib>
#include <fcntl.h>
#include <numeric>
#include <sys/stat.h>
#include <unistd.h>

#include "file_server_demo.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace {
const std::string TEST_MP4_PATH_H264_B = "/data/test/media/test_264_B_Gop25_4sec_cover.mp4";
const std::string TEST_MP4_PATH_MPEG4 = "/data/test/media/MPEG4.mp4";
const std::string TEST_MP4_PATH_HEVC_AUX = "/data/test/media/muxer_auxl_265.mp4";
const std::string TEST_MP4_PATH_HEVC_AVC_AAC = "/data/test/media/muxer_auxl_265_264_aac.mp4";
const std::string TEST_FMP4_PATH_H264 = "/data/test/media/h264_fmp4.mp4";
const std::string TEST_FMP4_PATH_H265 = "/data/test/media/h265_fmp4.mp4";
const std::string TEST_MOV_PATH_HEVC_PCM = "/data/test/media/265_pcm_s16le.mov";
const std::string TEST_MOV_PATH_H264_AAC = "/data/test/media/h264_aac.mov";
const std::string TEST_MOV_PATH_H264_MP3 = "/data/test/media/h264_mp3.mov";
const std::string TEST_MOV_PATH_H264_VORBIS = "/data/test/media/h264_vorbis.mov";
const std::string TEST_MOV_PATH_MPEG4_MP2 = "/data/test/media/MPEG4_mp2.mov";
const std::string TEST_URI_PATH = "http://127.0.0.1:46666/";
const std::string TEST_RELATIVE_PATH = "/data/test/media/";
const std::string FFMPEG_MP4_PLUGIN_NAME = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
constexpr int32_t WAIT_RETRY_TIMES = 20;
constexpr uint32_t WEAK_NETWORK_SHORT_TIMEOUT_MS = 10;
constexpr int64_t WEAK_NETWORK_FIRST_READ_FAIL_OFFSET = 0;
constexpr size_t WEAK_NETWORK_TIMEOUT_RECOVER_FAIL_COUNT = 20;
std::unique_ptr<FileServerDemo> g_server = nullptr;

std::vector<SeekExpectedResult> GetExplicitSeekExpectedResults()
{
    return {
        {0, SeekMode::SEEK_NEXT_SYNC, true, {103, 174}},
        {0, SeekMode::SEEK_PREVIOUS_SYNC, true, {103, 174}},
        {0, SeekMode::SEEK_CLOSEST_SYNC, true, {103, 174}},
        {2000, SeekMode::SEEK_NEXT_SYNC, true, {53, 90}},
        {2000, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2000, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 91}},
        {1920, SeekMode::SEEK_NEXT_SYNC, true, {53, 90}},
        {1920, SeekMode::SEEK_PREVIOUS_SYNC, true, {78, 134}},
        {1920, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 90}},
        {2160, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2160, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2160, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 91}},
        {2200, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2200, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2200, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 91}},
        {2440, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2440, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2440, SeekMode::SEEK_CLOSEST_SYNC, true, {28, 47}},
        {2600, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2600, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2600, SeekMode::SEEK_CLOSEST_SYNC, true, {28, 47}},
        {2700, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2700, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2700, SeekMode::SEEK_CLOSEST_SYNC, true, {28, 47}},
        {4080, SeekMode::SEEK_NEXT_SYNC, false, {}},
        {4080, SeekMode::SEEK_PREVIOUS_SYNC, true, {3, 5}},
        {4080, SeekMode::SEEK_CLOSEST_SYNC, true, {3, 5}},
        {4100, SeekMode::SEEK_NEXT_SYNC, false, {}},
        {4100, SeekMode::SEEK_PREVIOUS_SYNC, true, {3, 5}},
        {4100, SeekMode::SEEK_CLOSEST_SYNC, true, {3, 5}},
    };
}
}

namespace OHOS {
namespace Media {
void Mpeg4DemuxerPluginUnitTest::SetUpTestCase()
{
    if (g_server != nullptr) {
        g_server->StopServer();
        g_server.reset();
    }
    g_server = std::make_unique<FileServerDemo>();
    g_server->StartServer();
}

void Mpeg4DemuxerPluginUnitTest::TearDownTestCase()
{
    if (g_server == nullptr) {
        return;
    }
    g_server->StopServer();
    g_server.reset();
}

void Mpeg4DemuxerPluginUnitTest::TearDown()
{
    ResetPlugin(ffmpegPlugin_, ffmpegFd_);
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
}

void Mpeg4DemuxerPluginUnitTest::ResetPlugin(std::shared_ptr<BaseDemuxerPlugin>& plugin, int& fd)
{
    if (plugin != nullptr) {
        plugin->Reset();
        plugin = nullptr;
    }
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

void Mpeg4DemuxerPluginUnitTest::InitPlugin(
    const std::string& filePath, const std::shared_ptr<BaseDemuxerPlugin>& plugin, int& fd, MediaInfo& mediaInfo)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(filePath.c_str(), &fileStatus), 0);
    fd = open(filePath.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);

    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto mediaSource = std::make_shared<MediaSource>(uri);
    auto streamDemuxer = std::make_shared<StreamDemuxer>();
    auto source = std::make_shared<Source>();
    ASSERT_EQ(source->SetSource(mediaSource), Status::OK);
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    ASSERT_EQ(streamDemuxer->Init(uri), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    ASSERT_NE(plugin, nullptr);
    ASSERT_EQ(plugin->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(plugin->GetMediaInfo(mediaInfo), Status::OK);
}

void Mpeg4DemuxerPluginUnitTest::InitPluginURI(const std::string& filePath,
    const std::shared_ptr<BaseDemuxerPlugin>& plugin, MediaInfo& mediaInfo)
{
    ASSERT_EQ(filePath.find(TEST_RELATIVE_PATH), 0U);
    auto uri = TEST_URI_PATH + filePath.substr(TEST_RELATIVE_PATH.size());
    auto mediaSource = std::make_shared<MediaSource>(uri);
    auto streamDemuxer = std::make_shared<StreamDemuxer>();
    auto source = std::make_shared<Source>();
    ASSERT_EQ(source->SetSource(mediaSource), Status::OK);
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    ASSERT_EQ(streamDemuxer->Init(uri), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    ASSERT_NE(plugin, nullptr);
    ASSERT_EQ(plugin->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(plugin->GetMediaInfo(mediaInfo), Status::OK);
}

void Mpeg4DemuxerPluginUnitTest::InitWeakNetworkPlugin(
    const std::string& filePath, const WeakNetworkPluginContext& context, int64_t failOffset, size_t maxFailCount)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(filePath.c_str(), &fileStatus), 0);
    context.fd = open(filePath.c_str(), O_RDONLY);
    ASSERT_GE(context.fd, 0);

    auto uri = "fd://" + std::to_string(context.fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    auto cb = std::make_shared<Mpeg4SourceCallback>(demuxerPluginManager);
    auto source = std::make_shared<Source>();
    source->SetCallback(cb);
    ASSERT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::vector<StreamInfo> streams;
    source->GetStreamInfo(streams);
    demuxerPluginManager->InitDefaultPlay(streams);

    std::shared_ptr<BaseStreamDemuxer> streamDemuxer =
        std::make_shared<Mpeg4StreamDemuxerPullDataFailMock>(failOffset, maxFailCount);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    ASSERT_EQ(streamDemuxer->Init(""), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    ASSERT_NE(context.plugin, nullptr);
    ASSERT_EQ(context.plugin->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(context.plugin->GetMediaInfo(context.mediaInfo), Status::OK);
}

void Mpeg4DemuxerPluginUnitTest::InitFfmpegPlugin(const std::string& filePath)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(FFMPEG_MP4_PLUGIN_NAME);
    auto ffmpegPlugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    ffmpegPlugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(ffmpegPlugin);
    InitPlugin(filePath, ffmpegPlugin_, ffmpegFd_, ffmpegMediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitFfmpegPluginURI(const std::string& filePath)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(FFMPEG_MP4_PLUGIN_NAME);
    auto ffmpegPlugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    ffmpegPlugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(ffmpegPlugin);
    InitPluginURI(filePath, ffmpegPlugin_, ffmpegMediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitMpeg4Plugin(const std::string& filePath)
{
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
    InitPlugin(filePath, mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitMpeg4PluginURI(const std::string& filePath)
{
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
    InitPluginURI(filePath, mpeg4Plugin_, mpeg4MediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitWeakNetworkFfmpegPlugin(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(FFMPEG_MP4_PLUGIN_NAME);
    auto ffmpegPlugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    ffmpegPlugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(ffmpegPlugin);
    WeakNetworkPluginContext context {ffmpegPlugin_, ffmpegFd_, ffmpegMediaInfo_};
    InitWeakNetworkPlugin(filePath, context, failOffset, maxFailCount);
}

void Mpeg4DemuxerPluginUnitTest::InitWeakNetworkMpeg4Plugin(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount)
{
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
    WeakNetworkPluginContext context {mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_};
    InitWeakNetworkPlugin(filePath, context, failOffset, maxFailCount);
}

void Mpeg4DemuxerPluginUnitTest::InitDeferredWeakNetworkMpeg4Plugin(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(filePath.c_str(), &fileStatus), 0);
    mpeg4Fd_ = open(filePath.c_str(), O_RDONLY);
    ASSERT_GE(mpeg4Fd_, 0);

    auto uri = "fd://" + std::to_string(mpeg4Fd_) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    auto cb = std::make_shared<Mpeg4SourceCallback>(demuxerPluginManager);
    auto source = std::make_shared<Source>();
    source->SetCallback(cb);
    ASSERT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::vector<StreamInfo> streams;
    source->GetStreamInfo(streams);
    demuxerPluginManager->InitDefaultPlay(streams);

    auto streamDemuxer = std::make_shared<Mpeg4StreamDemuxerPullDataFailMock>(0, 0);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    ASSERT_EQ(streamDemuxer->Init(""), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
    ASSERT_EQ(mpeg4Plugin_->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->GetMediaInfo(mpeg4MediaInfo_), Status::OK);
    streamDemuxer->ResetFailState();
    streamDemuxer->SetFailParams(static_cast<size_t>(failOffset), maxFailCount);
}

bool Mpeg4DemuxerPluginUnitTest::IsSupportedTrack(const Meta& meta) const
{
    Plugins::MediaType trackType = Plugins::MediaType::UNKNOWN;
    if (!meta.Get<Tag::MEDIA_TYPE>(trackType)) {
        return false;
    }
    std::vector<uint8_t> cover;
    if (meta.Get<Tag::MEDIA_COVER>(cover)) {
        return false;
    }
    std::string mime;
    if (meta.Get<Tag::MIME_TYPE>(mime) && mime.find("image/") == 0) {
        return false;
    }
    return trackType == Plugins::MediaType::VIDEO || trackType == Plugins::MediaType::AUDIO ||
        trackType == Plugins::MediaType::SUBTITLE || trackType == Plugins::MediaType::TIMEDMETA;
}

bool Mpeg4DemuxerPluginUnitTest::IsEos(const std::shared_ptr<MediaAVBuffer>& buffer) const
{
    return buffer != nullptr && (buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)) != 0;
}

bool Mpeg4DemuxerPluginUnitTest::IsSyncFrame(const std::shared_ptr<MediaAVBuffer>& buffer) const
{
    return buffer != nullptr && (buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) != 0;
}

std::vector<uint32_t> Mpeg4DemuxerPluginUnitTest::SelectSupportedTracks(
    const std::shared_ptr<BaseDemuxerPlugin>& plugin, const MediaInfo& mediaInfo)
{
    std::vector<uint32_t> selectedTrackIds;
    for (uint32_t trackId = 0; trackId < mediaInfo.tracks.size(); ++trackId) {
        if (!IsSupportedTrack(mediaInfo.tracks[trackId])) {
            continue;
        }
        auto ret = plugin->SelectTrack(trackId);
        EXPECT_EQ(ret, Status::OK);
        if (ret == Status::OK) {
            selectedTrackIds.push_back(trackId);
        }
    }
    EXPECT_FALSE(selectedTrackIds.empty());
    return selectedTrackIds;
}

Status Mpeg4DemuxerPluginUnitTest::ReadSampleWithRetry(
    const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId,
    const std::shared_ptr<MediaAVBuffer>& buffer, ReadMode readMode)
{
    Status ret = Status::OK;
    for (int32_t retry = 0; retry < WAIT_RETRY_TIMES; ++retry) {
        ret = (readMode == ReadMode::ZERO_COPY) ? plugin->ReadSampleZeroCopy(trackId, buffer,
            MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT) : plugin->ReadSample(trackId, buffer,
            MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT);
        if (ret != Status::ERROR_WAIT_TIMEOUT) {
            return ret;
        }
    }
    return ret;
}

Status Mpeg4DemuxerPluginUnitTest::GetNextSampleSizeWithRetry(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
    uint32_t trackId, int32_t& sampleSize)
{
    Status ret = Status::OK;
    for (int32_t retry = 0; retry < WAIT_RETRY_TIMES; ++retry) {
        ret = plugin->GetNextSampleSize(trackId, sampleSize, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT);
        if (ret != Status::ERROR_WAIT_TIMEOUT) {
            return ret;
        }
    }
    return ret;
}

void Mpeg4DemuxerPluginUnitTest::InitPluginsForReadTest(const std::string& filePath, bool useUri)
{
    if (useUri) {
        InitFfmpegPluginURI(filePath);
        InitMpeg4PluginURI(filePath);
        return;
    }
    InitFfmpegPlugin(filePath);
    InitMpeg4Plugin(filePath);
}

SampleDigest Mpeg4DemuxerPluginUnitTest::ReadFirstSample(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
    const MediaInfo& mediaInfo, ReadMode readMode)
{
    SampleDigest digest;
    auto selectedTrackIds = SelectSupportedTracks(plugin, mediaInfo);
    if (selectedTrackIds.empty()) {
        ADD_FAILURE() << "No supported track selected";
        return digest;
    }

    digest.trackId = selectedTrackIds.front();
    mediaInfo.tracks[digest.trackId].GetData(Tag::MIME_TYPE, digest.mime);

    auto sampleSizeRet = GetNextSampleSizeWithRetry(plugin, digest.trackId, digest.nextSampleSize);
    EXPECT_EQ(sampleSizeRet, Status::OK);
    if (sampleSizeRet != Status::OK) {
        return digest;
    }
    EXPECT_GT(digest.nextSampleSize, 0);

    std::shared_ptr<MediaAVBuffer> buffer;
    std::unique_ptr<AVBufferWrapper> copyBuffer;
    if (readMode == ReadMode::ZERO_COPY) {
        buffer = MediaAVBuffer::CreateAVBuffer();
    } else {
        copyBuffer = std::make_unique<AVBufferWrapper>(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
        buffer = copyBuffer->mediaAVBuffer;
    }
    if (buffer == nullptr) {
        ADD_FAILURE() << "Create AVBuffer failed";
        return digest;
    }
    auto readRet = ReadSampleWithRetry(plugin, digest.trackId, buffer, readMode);
    EXPECT_EQ(readRet, Status::OK);
    if (readRet != Status::OK) {
        return digest;
    }
    EXPECT_FALSE(IsEos(buffer));

    digest.actualSize = buffer->memory_ != nullptr ? buffer->memory_->GetSize() : 0;
    digest.pts = buffer->pts_;
    digest.dts = buffer->dts_;
    digest.duration = buffer->duration_;
    digest.flag = buffer->flag_;
    EXPECT_GT(digest.actualSize, 0);
    return digest;
}

ReadDigest Mpeg4DemuxerPluginUnitTest::ReadAllSamples(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
    const MediaInfo& mediaInfo, const std::vector<uint32_t>& selectedTrackIds, ReadMode readMode,
    const std::string& firstSampleLogPrefix)
{
    auto digest = CreateReadDigest(mediaInfo, selectedTrackIds);
    std::vector<bool> eosFlags(selectedTrackIds.size(), false);
    std::unique_ptr<AVBufferWrapper> copyBuffer;
    if (readMode == ReadMode::COPY) {
        copyBuffer = std::make_unique<AVBufferWrapper>(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    }

    while (std::find(eosFlags.begin(), eosFlags.end(), false) != eosFlags.end()) {
        for (size_t index = 0; index < selectedTrackIds.size(); ++index) {
            if (eosFlags[index]) {
                continue;
            }
            std::shared_ptr<MediaAVBuffer> buffer =
                (readMode == ReadMode::ZERO_COPY) ? MediaAVBuffer::CreateAVBuffer() : copyBuffer->mediaAVBuffer;
            bool eosReached = false;
            TrackSampleReadContext context {plugin, digest.tracks[index], buffer, readMode, firstSampleLogPrefix};
            if (!ReadTrackSample(context, eosReached)) {
                return digest;
            }
            eosFlags[index] = eosReached;
        }
    }
    return digest;
}

ReadDigest Mpeg4DemuxerPluginUnitTest::CreateReadDigest(
    const MediaInfo& mediaInfo, const std::vector<uint32_t>& selectedTrackIds) const
{
    ReadDigest digest;
    for (auto trackId : selectedTrackIds) {
        TrackReadDigest trackDigest;
        trackDigest.trackId = trackId;
        mediaInfo.tracks[trackId].GetData(Tag::MIME_TYPE, trackDigest.mime);
        digest.tracks.push_back(trackDigest);
    }
    return digest;
}

bool Mpeg4DemuxerPluginUnitTest::ReadTrackSample(const TrackSampleReadContext& context, bool& eosReached)
{
    if (context.buffer == nullptr) {
        ADD_FAILURE() << "Create AVBuffer failed";
        return false;
    }
    auto& trackDigest = context.trackDigest;
    auto ret = ReadSampleWithRetry(context.plugin, trackDigest.trackId, context.buffer, context.readMode);
    if (!(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR)) {
        ADD_FAILURE() << "ReadSample failed, trackId=" << trackDigest.trackId
                      << ", ret=" << static_cast<int32_t>(ret);
        return false;
    }
    eosReached = ret == Status::END_OF_STREAM || IsEos(context.buffer);
    if (eosReached) {
        return true;
    }
    if (trackDigest.frameCount == 0) {
        trackDigest.firstPts = context.buffer->pts_;
        if (!context.firstSampleLogPrefix.empty()) {
            int32_t memorySize = -1;
            if (context.buffer->memory_ != nullptr) {
                memorySize = static_cast<int32_t>(context.buffer->memory_->GetSize());
            }
            fprintf(stderr, "%s first sample: trackId=%u, mime=%s, pts=%lld, dts=%lld, duration=%lld, flag=0x%x, "
                "memorySize=%d\n", context.firstSampleLogPrefix.c_str(), trackDigest.trackId,
                trackDigest.mime.c_str(), static_cast<long long>(context.buffer->pts_),
                static_cast<long long>(context.buffer->dts_), static_cast<long long>(context.buffer->duration_),
                context.buffer->flag_, memorySize);
            fflush(stderr);
        }
    }
    trackDigest.lastPts = context.buffer->pts_;
    if (IsSyncFrame(context.buffer)) {
        trackDigest.keyFrameCount++;
        trackDigest.keyFrameIndexes.push_back(static_cast<uint32_t>(trackDigest.frameCount));
    }
    trackDigest.frameCount++;
    return true;
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekSuccessDigest(const ReadDigest& actualDigest,
    const ReadDigest& expectedDigest)
{
    ASSERT_EQ(actualDigest.tracks.size(), expectedDigest.tracks.size());
    for (size_t index = 0; index < actualDigest.tracks.size(); ++index) {
        EXPECT_EQ(actualDigest.tracks[index].mime, expectedDigest.tracks[index].mime);
        EXPECT_GT(actualDigest.tracks[index].frameCount, 0);
        EXPECT_LE(actualDigest.tracks[index].frameCount, expectedDigest.tracks[index].frameCount);
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekSuccessDigestWithExpected(const ReadDigest& ffmpegDigest,
    const ReadDigest& mpeg4Digest, const std::vector<int32_t>& expectedFrames)
{
    ASSERT_EQ(ffmpegDigest.tracks.size(), expectedFrames.size());
    ASSERT_EQ(mpeg4Digest.tracks.size(), expectedFrames.size());
    for (size_t index = 0; index < expectedFrames.size(); ++index) {
        EXPECT_EQ(ffmpegDigest.tracks[index].frameCount, expectedFrames[index]);
        EXPECT_EQ(mpeg4Digest.tracks[index].mime, ffmpegDigest.tracks[index].mime);
        EXPECT_GT(mpeg4Digest.tracks[index].frameCount, 0);
        EXPECT_LE(mpeg4Digest.tracks[index].frameCount, expectedFrames[index]);
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekFallbackDigest(const ReadDigest& mpeg4Digest,
    const std::vector<uint32_t>& selectedTrackIds, const std::vector<int32_t>& expectedFrames)
{
    if (expectedFrames.empty()) {
        EXPECT_EQ(mpeg4Digest.tracks.size(), selectedTrackIds.size());
        return;
    }
    ASSERT_EQ(mpeg4Digest.tracks.size(), expectedFrames.size());
    for (size_t index = 0; index < expectedFrames.size(); ++index) {
        EXPECT_GT(mpeg4Digest.tracks[index].frameCount, 0);
        EXPECT_LE(mpeg4Digest.tracks[index].frameCount, expectedFrames[index]);
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareReadDigest(const ReadDigest& lhs, const ReadDigest& rhs)
{
    ASSERT_EQ(lhs.tracks.size(), rhs.tracks.size());
    for (size_t index = 0; index < lhs.tracks.size(); ++index) {
        const auto& lhsTrack = lhs.tracks[index];
        const auto& rhsTrack = rhs.tracks[index];
        EXPECT_EQ(lhsTrack.mime, rhsTrack.mime);
        EXPECT_EQ(lhsTrack.frameCount, rhsTrack.frameCount);
        EXPECT_EQ(lhsTrack.keyFrameCount, rhsTrack.keyFrameCount);
        EXPECT_EQ(lhsTrack.firstPts, rhsTrack.firstPts);
        EXPECT_EQ(lhsTrack.lastPts, rhsTrack.lastPts);
        EXPECT_EQ(lhsTrack.keyFrameIndexes, rhsTrack.keyFrameIndexes);
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckReadResultWithExpected(const std::string& filePath,
    const std::vector<int32_t>& expectedFrames, const std::vector<int32_t>& expectedKeyFrames,
    const std::vector<uint32_t>& expectedKeyFrameIndexes, bool useUri)
{
    if (useUri) {
        InitMpeg4PluginURI(filePath);
    } else {
        InitMpeg4Plugin(filePath);
    }
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    auto digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, selectedTrackIds);
    CheckReadDigestWithExpected(digest, expectedFrames, expectedKeyFrames, expectedKeyFrameIndexes);
}

void Mpeg4DemuxerPluginUnitTest::CheckReadDigestWithExpected(const ReadDigest& digest,
    const std::vector<int32_t>& expectedFrames, const std::vector<int32_t>& expectedKeyFrames,
    const std::vector<uint32_t>& expectedKeyFrameIndexes)
{
    ASSERT_EQ(digest.tracks.size(), expectedFrames.size());
    ASSERT_EQ(digest.tracks.size(), expectedKeyFrames.size());
    for (size_t index = 0; index < digest.tracks.size(); ++index) {
        EXPECT_EQ(digest.tracks[index].frameCount, expectedFrames[index]);
        EXPECT_EQ(digest.tracks[index].keyFrameCount, expectedKeyFrames[index]);
        if (index == 0) {
            EXPECT_EQ(digest.tracks[index].keyFrameIndexes, expectedKeyFrameIndexes);
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareSampleDigest(const SampleDigest& lhs, const SampleDigest& rhs)
{
    EXPECT_EQ(lhs.mime, rhs.mime);
    if (lhs.mime.find("video/") == 0 &&
        (lhs.flag & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) != 0) {
        // FFmpeg prepends codec config on every sync frame while MPEG4Demuxer only prepends it on the first one.
        // The zero-copy test validates decodable sample attributes rather than requiring identical advertised size.
        EXPECT_GT(lhs.nextSampleSize, 0);
        EXPECT_GT(rhs.nextSampleSize, 0);
        EXPECT_LE(lhs.actualSize, rhs.nextSampleSize);
        EXPECT_LE(rhs.actualSize, lhs.nextSampleSize);
    } else {
        EXPECT_EQ(lhs.nextSampleSize, rhs.nextSampleSize);
    }
    EXPECT_EQ(lhs.actualSize, rhs.actualSize);
    EXPECT_EQ(lhs.pts, rhs.pts);
    EXPECT_EQ(lhs.dts, rhs.dts);
    EXPECT_EQ(lhs.duration, rhs.duration);
    EXPECT_EQ(lhs.flag, rhs.flag);
}

void Mpeg4DemuxerPluginUnitTest::CompareSeekReadResult(
    const std::string& filePath, const std::vector<int64_t>& seekTimes,
    const std::vector<SeekMode>& seekModes, bool useUri)
{
    for (auto seekTimeTarget : seekTimes) {
        for (auto seekMode : seekModes) {
            InitPluginsForReadTest(filePath, useUri);

            auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
            auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
            ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

            int64_t ffmpegRealSeekTime = 0;
            int64_t mpeg4RealSeekTime = 0;
            auto ffmpegRet = ffmpegPlugin_->SeekTo(-1, seekTimeTarget, seekMode, ffmpegRealSeekTime);
            auto mpeg4Ret = mpeg4Plugin_->SeekTo(-1, seekTimeTarget, seekMode, mpeg4RealSeekTime);
            ASSERT_EQ(mpeg4Ret, ffmpegRet);
            if (ffmpegRet != Status::OK) {
                ResetPlugin(ffmpegPlugin_, ffmpegFd_);
                ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
                continue;
            }
            auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
            auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
            CheckSeekSuccessDigest(mpeg4Digest, ffmpegDigest);

            ResetPlugin(ffmpegPlugin_, ffmpegFd_);
            ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareWeakNetworkReadResult(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount)
{
    InitWeakNetworkFfmpegPlugin(filePath, failOffset, maxFailCount);
    InitWeakNetworkMpeg4Plugin(filePath, failOffset, maxFailCount);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    CompareReadDigest(mpeg4Digest, ffmpegDigest);
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekReadResultWithExpected(const std::string& filePath,
    const std::vector<SeekExpectedResult>& expectedResults, bool useUri)
{
    for (const auto& expected : expectedResults) {
        InitPluginsForReadTest(filePath, useUri);

        auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
        auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
        ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

        int64_t ffmpegRealSeekTime = 0;
        int64_t mpeg4RealSeekTime = 0;
        auto ffmpegRet = ffmpegPlugin_->SeekTo(-1, expected.seekTimeMs, expected.seekMode, ffmpegRealSeekTime);
        auto mpeg4Ret = mpeg4Plugin_->SeekTo(-1, expected.seekTimeMs, expected.seekMode, mpeg4RealSeekTime);

        if (expected.seekSuccess) {
            ASSERT_EQ(ffmpegRet, Status::OK);
            ASSERT_EQ(mpeg4Ret, Status::OK);
            auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
            auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
            CheckSeekSuccessDigestWithExpected(ffmpegDigest, mpeg4Digest, expected.expectedFrames);
            ResetPlugin(ffmpegPlugin_, ffmpegFd_);
            ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
            continue;
        }
        EXPECT_NE(ffmpegRet, Status::OK);
        if (mpeg4Ret == Status::OK) {
            auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
            CheckSeekFallbackDigest(mpeg4Digest, mpeg4SelectedTracks, expected.expectedFrames);
        }

        ResetPlugin(ffmpegPlugin_, ffmpegFd_);
        ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_InitSelectTrack_001
 * @tc.desc: Directly create MPEG4DemuxerPlugin and select supported tracks
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_InitSelectTrack_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    ASSERT_GT(mpeg4MediaInfo_.tracks.size(), 0U);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    EXPECT_FALSE(selectedTrackIds.empty());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer full-frame read results on h264 mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_CompareWithFfmpeg_001, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_H264_B);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    CompareReadDigest(mpeg4Digest, ffmpegDigest);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_CompareWithFfmpeg_002
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer full-frame read results on mpeg4 mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_CompareWithFfmpeg_002, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_MPEG4);
    InitMpeg4Plugin(TEST_MP4_PATH_MPEG4);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    CompareReadDigest(mpeg4Digest, ffmpegDigest);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSampleZeroCopy_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer zero-copy first-sample result on mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSampleZeroCopy_CompareWithFfmpeg_001, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_H264_B);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);

    auto ffmpegDigest = ReadFirstSample(ffmpegPlugin_, ffmpegMediaInfo_, ReadMode::ZERO_COPY);
    auto mpeg4Digest = ReadFirstSample(mpeg4Plugin_, mpeg4MediaInfo_, ReadMode::ZERO_COPY);
    CompareSampleDigest(mpeg4Digest, ffmpegDigest);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekTo_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer seek and remaining-frame read results on mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekTo_CompareWithFfmpeg_001, TestSize.Level1)
{
    std::vector<int64_t> seekTimes = {0, 1000, 2000, 3000};
    std::vector<SeekMode> seekModes = {
        SeekMode::SEEK_PREVIOUS_SYNC,
        SeekMode::SEEK_NEXT_SYNC,
        SeekMode::SEEK_CLOSEST_SYNC,
    };
    CompareSeekReadResult(TEST_MP4_PATH_H264_B, seekTimes, seekModes);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToStart_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer SeekToStart results after reading a sample
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToStart_CompareWithFfmpeg_001, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_H264_B);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

    auto ffmpegPreSeekDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4PreSeekDigest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    CompareReadDigest(mpeg4PreSeekDigest, ffmpegPreSeekDigest);

    ASSERT_EQ(ffmpegPlugin_->SeekToStart(), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SeekToStart(), Status::OK);

    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks, ReadMode::COPY,
        "FFmpeg SeekToStart");
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks, ReadMode::COPY,
        "MPEG4 SeekToStart");
    CompareReadDigest(mpeg4Digest, ffmpegDigest);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AsyncTimeout_001
 * @tc.desc: Direct async read with zero timeout returns wait timeout when cache is empty
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AsyncTimeout_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(selectedTrackIds.front(), buffer.mediaAVBuffer, 0), Status::ERROR_WAIT_TIMEOUT);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_MP4_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer async read results on h264 mp4 under weak network
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_MP4_CompareWithFfmpeg_001, TestSize.Level1)
{
    CompareWeakNetworkReadResult(TEST_MP4_PATH_H264_B, 2560656, 3);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_MP4_CompareWithFfmpeg_002
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer async read results on mpeg4 mp4 under weak network
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_MP4_CompareWithFfmpeg_002, TestSize.Level1)
{
    CompareWeakNetworkReadResult(TEST_MP4_PATH_MPEG4, 2560656, 3);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_FMP4_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer async read results on h264 fmp4 under weak network
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_FMP4_CompareWithFfmpeg_001, TestSize.Level1)
{
    CompareWeakNetworkReadResult(TEST_FMP4_PATH_H264, 2560656, 3);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_TimeoutThenRecover_001
 * @tc.desc: Async read returns timeout under weak network and the next read can recover
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_TimeoutThenRecover_001, TestSize.Level1)
{
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, WEAK_NETWORK_FIRST_READ_FAIL_OFFSET,
        WEAK_NETWORK_TIMEOUT_RECOVER_FAIL_COUNT);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);

    auto trackId = selectedTrackIds.front();
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer, WEAK_NETWORK_SHORT_TIMEOUT_MS),
        Status::ERROR_WAIT_TIMEOUT);

    auto ret = ReadSampleWithRetry(mpeg4Plugin_, trackId, buffer.mediaAVBuffer, ReadMode::COPY);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_FALSE(IsEos(buffer.mediaAVBuffer));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0001
 * @tc.desc: Explicitly verify mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0001, TestSize.Level1)
{
    CheckReadResultWithExpected("/data/test/media/h264_double_video_audio.mp4",
        {602, 604, 433, 433}, {3, 3, 433, 433}, {0, 250, 500});
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0002
 * @tc.desc: Explicitly verify mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0002, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MP4_PATH_H264_B, {103, 174}, {5, 174}, {0, 25, 50, 75, 100});
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0003
 * @tc.desc: Explicitly verify mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0003, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MP4_PATH_MPEG4, {602, 434}, {51, 434},
        {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180, 192, 204, 216, 228, 240, 252, 264,
        276, 288, 300, 312, 324, 336, 348, 360, 372, 384, 396, 408, 420, 432, 444, 456, 468, 480, 492, 504, 516,
        528, 540, 552, 564, 576, 588, 600});
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0004
 * @tc.desc: Explicitly verify hevc mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0004, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AUX, {16}, {1}, {0});
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0005
 * @tc.desc: Explicitly verify multi-track hevc mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0005, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    std::vector<uint32_t> numbers(430);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AVC_AAC, {430, 601}, {430, 3}, numbers);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_FMP4_ExplicitFrames_0001
 * @tc.desc: Explicitly verify h264 fmp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_FMP4_ExplicitFrames_0001, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_FMP4_PATH_H264, {602, 433}, {3, 433}, {0, 250, 500});
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_FMP4_ExplicitFrames_0002
 * @tc.desc: Explicitly verify h265 fmp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_FMP4_ExplicitFrames_0002, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_FMP4_PATH_H265, {604, 433}, {3, 433}, {0, 246, 497});
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MOV_ExplicitFrames_0001
 * @tc.desc: Explicitly verify mov frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MOV_ExplicitFrames_0001, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MOV_PATH_HEVC_PCM, {604, 433}, {3, 433}, {0, 246, 497});
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MOV_ExplicitFrames_0002
 * @tc.desc: Explicitly verify mov frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MOV_ExplicitFrames_0002, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MOV_PATH_H264_AAC, {602, 434}, {3, 434}, {0, 250, 500});
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MOV_ExplicitFrames_0003
 * @tc.desc: Explicitly verify mov frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MOV_ExplicitFrames_0003, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MOV_PATH_H264_MP3, {602, 386}, {3, 386}, {0, 250, 500});
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MOV_ExplicitFrames_0004
 * @tc.desc: Explicitly verify mov frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MOV_ExplicitFrames_0004, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MOV_PATH_H264_VORBIS, {602, 609}, {3, 609}, {0, 250, 500});
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MOV_ExplicitFrames_0005
 * @tc.desc: Explicitly verify mov frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MOV_ExplicitFrames_0005, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MOV_PATH_MPEG4_MP2, {602, 385}, {51, 385},
        {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180, 192, 204, 216, 228, 240, 252, 264,
        276, 288, 300, 312, 324, 336, 348, 360, 372, 384, 396, 408, 420, 432, 444, 456, 468, 480, 492, 504, 516,
        528, 540, 552, 564, 576, 588, 600});
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0001
 * @tc.desc: Explicitly verify mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0001, TestSize.Level1)
{
    CheckReadResultWithExpected("/data/test/media/h264_double_video_audio.mp4",
        {602, 604, 433, 433}, {3, 3, 433, 433}, {0, 250, 500}, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0002
 * @tc.desc: Explicitly verify mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0002, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MP4_PATH_H264_B, {103, 174}, {5, 174}, {0, 25, 50, 75, 100}, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0003
 * @tc.desc: Explicitly verify mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0003, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MP4_PATH_MPEG4, {602, 434}, {51, 434},
        {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180, 192, 204, 216, 228, 240, 252, 264,
        276, 288, 300, 312, 324, 336, 348, 360, 372, 384, 396, 408, 420, 432, 444, 456, 468, 480, 492, 504, 516,
        528, 540, 552, 564, 576, 588, 600}, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0004
 * @tc.desc: Explicitly verify hevc mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0004, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AUX, {16}, {1}, {0}, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0005
 * @tc.desc: Explicitly verify multi-track hevc mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0005, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    std::vector<uint32_t> numbers(430);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AVC_AAC, {430, 601}, {430, 3}, numbers, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_FMP4_ExplicitFrames_0001
 * @tc.desc: Explicitly verify h264 fmp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_FMP4_ExplicitFrames_0001, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_FMP4_PATH_H264, {602, 433}, {3, 433}, {0, 250, 500}, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_FMP4_ExplicitFrames_0002
 * @tc.desc: Explicitly verify h265 fmp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_FMP4_ExplicitFrames_0002, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_FMP4_PATH_H265, {604, 433}, {3, 433}, {0, 246, 497}, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MOV_ExplicitFrames_0001
 * @tc.desc: Explicitly verify mov uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MOV_ExplicitFrames_0001, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MOV_PATH_HEVC_PCM, {604, 433}, {3, 433}, {0, 246, 497}, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MOV_ExplicitFrames_0002
 * @tc.desc: Explicitly verify mov uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MOV_ExplicitFrames_0002, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MOV_PATH_H264_AAC, {602, 434}, {3, 434}, {0, 250, 500}, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MOV_ExplicitFrames_0003
 * @tc.desc: Explicitly verify mov uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MOV_ExplicitFrames_0003, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MOV_PATH_H264_MP3, {602, 386}, {3, 386}, {0, 250, 500}, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MOV_ExplicitFrames_0004
 * @tc.desc: Explicitly verify mov uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MOV_ExplicitFrames_0004, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MOV_PATH_H264_VORBIS, {602, 609}, {3, 609}, {0, 250, 500}, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MOV_ExplicitFrames_0005
 * @tc.desc: Explicitly verify mov uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MOV_ExplicitFrames_0005, TestSize.Level1)
{
    CheckReadResultWithExpected(TEST_MOV_PATH_MPEG4_MP2, {602, 385}, {51, 385},
        {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180, 192, 204, 216, 228, 240, 252, 264,
        276, 288, 300, 312, 324, 336, 348, 360, 372, 384, 396, 408, 420, 432, 444, 456, 468, 480, 492, 504, 516,
        528, 540, 552, 564, 576, 588, 600}, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekTo_ExplicitFrames_0001
 * @tc.desc: Explicitly verify remaining frame counts after seek on mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekTo_ExplicitFrames_0001, TestSize.Level1)
{
    CheckSeekReadResultWithExpected(TEST_MP4_PATH_H264_B, GetExplicitSeekExpectedResults());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekTo_URI_ExplicitFrames_0001
 * @tc.desc: Explicitly verify remaining frame counts after seek on mp4 uri source
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekTo_URI_ExplicitFrames_0001, TestSize.Level1)
{
    CheckSeekReadResultWithExpected(TEST_MP4_PATH_H264_B, GetExplicitSeekExpectedResults(), true);
}
} // namespace Media
} // namespace OHOS
