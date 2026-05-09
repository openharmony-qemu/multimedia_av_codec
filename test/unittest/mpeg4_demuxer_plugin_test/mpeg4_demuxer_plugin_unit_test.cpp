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
#include "mpeg4_sample_memory.h"

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
constexpr uint32_t WEAK_NETWORK_FAIL_DELAY_MS = 20;
constexpr int64_t WEAK_NETWORK_FIRST_READ_FAIL_OFFSET = 0;
constexpr size_t WEAK_NETWORK_TIMEOUT_RECOVER_FAIL_COUNT = 1;
constexpr size_t WEAK_NETWORK_THREAD_RETRY_FAIL_COUNT = 20;
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

std::shared_ptr<DataSourceImpl> CreateFileDataSource(const std::string& filePath, int& fd)
{
    struct stat fileStatus {};
    if (stat(filePath.c_str(), &fileStatus) != 0) {
        ADD_FAILURE() << "Failed to stat file: " << filePath;
        return nullptr;
    }
    fd = open(filePath.c_str(), O_RDONLY);
    if (fd < 0) {
        ADD_FAILURE() << "Failed to open file: " << filePath;
        return nullptr;
    }

    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto mediaSource = std::make_shared<MediaSource>(uri);
    auto streamDemuxer = std::make_shared<StreamDemuxer>();
    auto source = std::make_shared<Source>();
    if (source->SetSource(mediaSource) != Status::OK) {
        ADD_FAILURE() << "Set source failed";
        return nullptr;
    }
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    if (streamDemuxer->Init(uri) != Status::OK) {
        ADD_FAILURE() << "Init stream demuxer failed";
        return nullptr;
    }
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
    return std::make_shared<DataSourceImpl>(streamDemuxer, 0);
}

std::shared_ptr<Plugins::MPEG4::Sample> CreateMpeg4Sample(int32_t size, bool hasData)
{
    auto sample = std::make_shared<Plugins::MPEG4::Sample>();
    sample->pts = 0;
    sample->dts = 0;
    sample->duration = 0;
    sample->flag = Plugins::MPEG4::Sample::NONE;
    sample->size = size;
    if (hasData && size > 0) {
        sample->data = std::make_unique<uint8_t[]>(size);
        for (int32_t index = 0; index < size; ++index) {
            sample->data[index] = static_cast<uint8_t>(index);
        }
    }
    return sample;
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
    const std::string& filePath, int64_t failOffset, size_t maxFailCount, uint32_t failDelayMs)
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
    streamDemuxer->SetFailDelay(failDelayMs);
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
 * @tc.name: Mpeg4DemuxerPlugin_LifecycleMetaAndStubInterfaces_001
 * @tc.desc: Verify MPEG4 demuxer lifecycle, meta, probe, drm and reference-parser interfaces
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_LifecycleMetaAndStubInterfaces_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);

    MediaInfo mediaInfo;
    EXPECT_EQ(plugin->GetMediaInfo(mediaInfo), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin->Start(), Status::OK);
    EXPECT_EQ(plugin->Stop(), Status::OK);
    EXPECT_EQ(plugin->Pause(), Status::OK);
    EXPECT_EQ(plugin->Flush(), Status::OK);
    EXPECT_EQ(plugin->Reset(), Status::OK);
    plugin->ResetEosStatus();
    plugin->SetCacheLimit(1024);
    plugin->SetInterruptState(false);
    EXPECT_EQ(plugin->SetReadTimeoutForInitSeek(10), Status::OK);
    EXPECT_EQ(plugin->SetAVReadPacketStopState(false), Status::OK);
    EXPECT_EQ(plugin->SetCachePressureCallback(nullptr), Status::ERROR_UNKNOWN);
    EXPECT_EQ(plugin->SetTrackCacheLimit(0, 1024), Status::ERROR_UNKNOWN);

    int32_t offset = -1;
    int32_t size = -1;
    EXPECT_TRUE(plugin->GetProbeSize(offset, size));
    EXPECT_EQ(offset, 0);
    EXPECT_GT(size, 0);

    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    EXPECT_EQ(plugin->GetDrmInfo(drmInfo), Status::OK);
    EXPECT_TRUE(plugin->IsRefParserSupported());
    EXPECT_EQ(plugin->ParserRefUpdatePos(0), Status::OK);
    EXPECT_EQ(plugin->ParserRefInfo(), Status::OK);

    auto buffer = MediaAVBuffer::CreateAVBuffer();
    FrameLayerInfo frameLayerInfo;
    GopLayerInfo gopLayerInfo;
    std::vector<uint32_t> iFramePos;
    uint32_t frameId = 0;
    int64_t seekMs = 0;
    EXPECT_EQ(plugin->GetFrameLayerInfo(buffer, frameLayerInfo), Status::OK);
    EXPECT_EQ(plugin->GetFrameLayerInfo(frameId, frameLayerInfo), Status::OK);
    EXPECT_EQ(plugin->GetGopLayerInfo(0, gopLayerInfo), Status::OK);
    EXPECT_EQ(plugin->GetIFramePos(iFramePos), Status::OK);
    EXPECT_EQ(plugin->Dts2FrameId(0, frameId), Status::OK);
    EXPECT_EQ(plugin->SeekMs2FrameId(0, frameId), Status::OK);
    EXPECT_EQ(plugin->FrameId2SeekMs(frameId, seekMs), Status::OK);

    int64_t realSeekTime = 0;
    EXPECT_EQ(plugin->SeekToKeyFrame(0, 0, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(plugin->SeekToFrameByDts(0, 0, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0), Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetDataSourceWithProbSize_001
 * @tc.desc: Verify SetDataSourceWithProbSize parameter checks and normal initialization path
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetDataSourceWithProbSize_001, TestSize.Level1)
{
    int fd = -1;
    auto dataSource = CreateFileDataSource(TEST_MP4_PATH_H264_B, fd);
    ASSERT_NE(dataSource, nullptr);

    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->SetDataSourceWithProbSize(nullptr, 0), Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(plugin->SetDataSourceWithProbSize(dataSource, -1), Status::ERROR_INVALID_PARAMETER);
    auto setDataSourceWithProbeRet = plugin->SetDataSourceWithProbSize(dataSource, 1024);
    if (setDataSourceWithProbeRet != Status::OK) {
        EXPECT_EQ(setDataSourceWithProbeRet, Status::ERROR_INVALID_PARAMETER);
        ASSERT_EQ(plugin->SetDataSource(dataSource), Status::OK);
    }

    MediaInfo mediaInfo;
    EXPECT_EQ(plugin->GetMediaInfo(mediaInfo), Status::OK);
    EXPECT_FALSE(mediaInfo.tracks.empty());
    EXPECT_EQ(plugin->SetDataSourceWithProbSize(dataSource, 1024), Status::ERROR_WRONG_STATE);
    EXPECT_EQ(plugin->Reset(), Status::OK);
    if (fd >= 0) {
        close(fd);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_InvalidParameterAndTrackState_001
 * @tc.desc: Verify invalid parameters and select/unselect state handling
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_InvalidParameterAndTrackState_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    auto trackId = selectedTrackIds.front();
    auto invalidTrackId = static_cast<uint32_t>(mpeg4MediaInfo_.tracks.size());

    EXPECT_EQ(mpeg4Plugin_->SelectTrack(trackId), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->SelectTrack(invalidTrackId), Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->UnselectTrack(invalidTrackId), Status::ERROR_INVALID_PARAMETER);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    int32_t sampleSize = 0;
    EXPECT_EQ(mpeg4Plugin_->GetNextSampleSize(invalidTrackId, sampleSize, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(invalidTrackId, buffer.mediaAVBuffer, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, nullptr, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->ReadSampleZeroCopy(trackId, nullptr, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);

    ASSERT_EQ(mpeg4Plugin_->UnselectTrack(trackId), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->UnselectTrack(trackId), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->SelectTrack(trackId), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SyncReadInterfaces_001
 * @tc.desc: Verify synchronous GetNextSampleSize and ReadSample interfaces
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SyncReadInterfaces_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    auto trackId = selectedTrackIds.front();

    int32_t sampleSize = 0;
    ASSERT_EQ(mpeg4Plugin_->GetNextSampleSize(trackId, sampleSize), Status::OK);
    EXPECT_GT(sampleSize, 0);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    ASSERT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer), Status::OK);
    EXPECT_FALSE(IsEos(buffer.mediaAVBuffer));
    EXPECT_GT(buffer.mediaAVBuffer->memory_->GetSize(), 0U);

    EXPECT_EQ(mpeg4Plugin_->GetNextSampleSize(trackId, sampleSize, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(mpeg4Plugin_->ReadSampleZeroCopy(trackId, MediaAVBuffer::CreateAVBuffer(),
        MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AsyncCacheAndControlInterfaces_001
 * @tc.desc: Verify async cache query, pause, flush and reset related interfaces
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AsyncCacheAndControlInterfaces_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    auto trackId = selectedTrackIds.front();

    uint32_t cacheSize = 0;
    uint32_t frameCount = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheSize(trackId, cacheSize), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheFrameCount(trackId, frameCount), Status::OK);
    EXPECT_EQ(cacheSize, 0U);
    EXPECT_EQ(frameCount, 0U);

    mpeg4Plugin_->SetCacheLimit(1);
    int32_t sampleSize = 0;
    ASSERT_EQ(GetNextSampleSizeWithRetry(mpeg4Plugin_, trackId, sampleSize), Status::OK);
    EXPECT_GT(sampleSize, 0);
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheSize(trackId, cacheSize), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheFrameCount(trackId, frameCount), Status::OK);
    EXPECT_GT(cacheSize, 0U);
    EXPECT_GT(frameCount, 0U);

    int64_t lastPts = 0;
    EXPECT_EQ(mpeg4Plugin_->GetLastPTSByTrackId(trackId, lastPts), Status::OK);
    EXPECT_NE(lastPts, INT64_MIN);
    EXPECT_EQ(mpeg4Plugin_->Pause(), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->Flush(), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheFrameCount(trackId, frameCount), Status::OK);
    EXPECT_EQ(frameCount, 0U);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, trackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);
    EXPECT_FALSE(IsEos(buffer.mediaAVBuffer));
    EXPECT_EQ(mpeg4Plugin_->Reset(), Status::OK);

    MediaInfo mediaInfo;
    EXPECT_EQ(mpeg4Plugin_->GetMediaInfo(mediaInfo), Status::OK);
    EXPECT_TRUE(mediaInfo.tracks.empty());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleMemoryValidBranch_001
 * @tc.desc: Verify MPEG4SampleMemory valid sample and boundary offset branches
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleMemoryValidBranch_001, TestSize.Level1)
{
    constexpr int32_t sampleSize = 8;
    constexpr int32_t startOffset = 3;
    auto sample = CreateMpeg4Sample(sampleSize, true);
    ASSERT_NE(sample, nullptr);
    ASSERT_NE(sample->data, nullptr);

    Plugins::MPEG4::MPEG4SampleMemory memory(sample, startOffset);
    EXPECT_EQ(memory.GetMemoryType(), MemoryType::CUSTOM_MEMORY);
    EXPECT_EQ(memory.GetAddr(), sample->data.get() + startOffset);
    EXPECT_EQ(memory.GetCapacity(), sampleSize - startOffset);
    EXPECT_EQ(memory.GetSize(), sampleSize - startOffset);
    EXPECT_EQ(memory.GetOffset(), 0);
    EXPECT_EQ(memory.Write(nullptr, sampleSize), 0);

    Plugins::MPEG4::MPEG4SampleMemory endOffsetMemory(sample, sampleSize);
    EXPECT_EQ(endOffsetMemory.GetAddr(), sample->data.get() + sampleSize);
    EXPECT_EQ(endOffsetMemory.GetCapacity(), 0);
    EXPECT_EQ(endOffsetMemory.GetSize(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleMemoryInvalidBranch_001
 * @tc.desc: Verify MPEG4SampleMemory invalid sample, data, size and offset branches
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleMemoryInvalidBranch_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleMemory nullSampleMemory(nullptr);
    EXPECT_EQ(nullSampleMemory.GetAddr(), nullptr);
    EXPECT_EQ(nullSampleMemory.GetCapacity(), 0);
    EXPECT_EQ(nullSampleMemory.GetSize(), 0);
    EXPECT_EQ(nullSampleMemory.GetOffset(), 0);

    auto sampleWithoutData = CreateMpeg4Sample(8, false);
    Plugins::MPEG4::MPEG4SampleMemory nullDataMemory(sampleWithoutData);
    EXPECT_EQ(nullDataMemory.GetAddr(), nullptr);
    EXPECT_EQ(nullDataMemory.GetCapacity(), 0);
    EXPECT_EQ(nullDataMemory.GetSize(), 0);

    auto negativeSizeSample = CreateMpeg4Sample(-1, false);
    negativeSizeSample->data = std::make_unique<uint8_t[]>(1);
    Plugins::MPEG4::MPEG4SampleMemory negativeSizeMemory(negativeSizeSample);
    EXPECT_EQ(negativeSizeMemory.GetAddr(), nullptr);
    EXPECT_EQ(negativeSizeMemory.GetCapacity(), 0);
    EXPECT_EQ(negativeSizeMemory.GetSize(), 0);

    auto validSample = CreateMpeg4Sample(8, true);
    Plugins::MPEG4::MPEG4SampleMemory negativeOffsetMemory(validSample, -1);
    EXPECT_EQ(negativeOffsetMemory.GetAddr(), nullptr);
    EXPECT_EQ(negativeOffsetMemory.GetCapacity(), 0);
    EXPECT_EQ(negativeOffsetMemory.GetSize(), 0);

    Plugins::MPEG4::MPEG4SampleMemory overflowOffsetMemory(validSample, validSample->size + 1);
    EXPECT_EQ(overflowOffsetMemory.GetAddr(), nullptr);
    EXPECT_EQ(overflowOffsetMemory.GetCapacity(), 0);
    EXPECT_EQ(overflowOffsetMemory.GetSize(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AsyncThreadControlInterfaces_001
 * @tc.desc: Verify async read thread priority and stop-state interfaces
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AsyncThreadControlInterfaces_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    auto trackId = selectedTrackIds.front();

    EXPECT_EQ(mpeg4Plugin_->BoostReadThreadPriority(), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->BoostReadThreadPriority(), Status::ERROR_WRONG_STATE);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, trackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);
    EXPECT_FALSE(IsEos(buffer.mediaAVBuffer));

    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    trackId = selectedTrackIds.front();
    EXPECT_EQ(mpeg4Plugin_->SetAVReadPacketStopState(true), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_WRONG_STATE);
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
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_ThreadRetryRecover_001
 * @tc.desc: Verify async read thread keeps running when ReadSampleData returns ERROR_AGAIN
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_ThreadRetryRecover_001, TestSize.Level1)
{
    CompareWeakNetworkReadResult(TEST_MP4_PATH_H264_B, 2560656, WEAK_NETWORK_THREAD_RETRY_FAIL_COUNT);
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
        WEAK_NETWORK_TIMEOUT_RECOVER_FAIL_COUNT, WEAK_NETWORK_FAIL_DELAY_MS);
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
