/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "http_source_plugin.h"
#include "gtest/gtest.h"
#include "http_server_demo.h"
#include "source_callback.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
class MockMediaDownloaderForSeek : public MediaDownloader {
public:
    MockMediaDownloaderForSeek() = default;
    virtual ~MockMediaDownloaderForSeek() = default;
    bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override
    {
        (void)url;
        (void)httpHeader;
        return true;
    }
    void Init() override {}
    void Close(bool isAsync) override {(void)isAsync;}
    void Pause() override { pauseCalled_ = true; }
    void Resume() override { resumeCalled_ = true; }
    Status Read(unsigned char* buff, ReadDataInfo& readDataInfo) override
    {
        (void)buff;
        (void)readDataInfo;
        return Status::OK;
    }
    uint64_t GetBufferSize() const override { return 0; }
    size_t GetContentLength() const override { return contentLength_; }
    int64_t GetDuration() const override { return duration_; }
    Seekable GetSeekable() const override { return seekable_; }
    void SetCallback(const std::shared_ptr<Callback>& cb) override {(void)cb;}
    void SetStatusCallback(StatusCallbackFunc cb) override {(void)cb;}
    void SetInterruptState(bool isInterruptNeeded) override {(void)isInterruptNeeded;}
    void SetAppUid(int32_t appUid) override {(void)appUid;}
    bool SelectBitRate(uint32_t bitRate) override { return bitRate == 0 ? false : true; }
    void SetIsTriggerAutoMode(bool isAuto) override {(void)isAuto;}
    bool AutoSelectBitRate(uint32_t bitRate) override { return bitRate == 0 ? false : true; }
    size_t GetSegmentOffset() override { return 0; }
    bool GetHLSDiscontinuity() override { return false; }
    void SetSourceStatisticsDfx(std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr) override
    {
        (void)rpInfoPtr;
    }
    bool SeekToTime(int64_t seekTime, SeekMode mode) override
    {
        (void)seekTime;
        (void)mode;
        return seekToTimeResult_;
    }
    bool MediaSeekTimeByStreamId(int64_t seekTime, SeekMode mode, int32_t streamId) override
    {
        (void)seekTime;
        (void)mode;
        (void)streamId;
        return seekToTimeResult_;
    }

    int64_t duration_ = 1000;
    Seekable seekable_ = Seekable::SEEKABLE;
    bool seekToTimeResult_ = true;

    void SetReadBlockingFlag(bool isReadBlockingAllowed) override
    {
        setReadBlockingFlagCalled_ = true;
    }
    Status GetStreamInfo(std::vector<StreamInfo>& streams, bool isUpdate = false) override
    {
        return getStreamInfoStatus_;
    }
    Status SelectStream(int32_t streamId) override
    {
        return selectStreamStatus_;
    }
    void SetDemuxerState(int32_t streamId) override
    {
        setDemuxerStateCalled_ = true;
    }
    void SetDownloadErrorState() override
    {
        setDownloadErrorStateCalled_ = true;
    }
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID) override
    {
        return setCurrentBitRateStatus_;
    }
    void SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy) override
    {
        setPlayStrategyCalled_ = true;
    }
    Status StopBufferring(bool isAppBackground) override
    {
        return stopBufferringStatus_;
    }
    std::pair<int64_t, bool> GetStartInfo() const override
    {
        return startInfo_;
    }
    void WaitForBufferingEnd() override
    {
        waitForBufferingEndCalled_ = true;
    }
    void NotifyInitSuccess() override
    {
        notifyInitSuccessCalled_ = true;
    }
    uint64_t GetCachedDuration() override
    {
        return cachedDuration_;
    }
    void RestartAndClearBuffer() override
    {
        restartAndClearBufferCalled_ = true;
    }
    bool IsFlvLive() override
    {
        return isFlvLive_;
    }
    void SetStartPts(int64_t startPts) override
    {
        setStartPtsCalled_ = true;
    }
    void SetExtraCache(uint64_t cacheDuration) override
    {
        setExtraCacheCalled_ = true;
    }
    void SetMediaStreams(const MediaStreamList& mediaStreams) override
    {
        setMediaStreamsCalled_ = true;
    }
    std::string GetContentType() override
    {
        return contentType_;
    }
    bool IsHlsFmp4() override
    {
        return isHlsFmp4_;
    }
    uint64_t GetMemorySize() override
    {
        return memorySize_;
    }
    std::string GetCurUrl() override
    {
        return curUrl_;
    }
    bool IsHlsEnd(int32_t streamId = -1) override
    {
        return isHlsEnd_;
    }
    std::string GetMaskUrlByReadIndex(int32_t streamId) override
    {
        return maskUrl_;
    }
    void SetDefaultStreamId(int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId) override
    {
        setDefaultStreamIdCalled_ = true;
    }
    bool SeekToPos(int64_t offset, bool& isSeekHit) override
    {
        isSeekHit = seekHit_;
        return seekToPosResult_;
    }
    std::vector<uint32_t> GetBitRates() override
    {
        return bitRates_;
    }
    void GetDownloadInfo(DownloadInfo& downloadInfo) override
    {
        getDownloadInfoCalled_ = true;
    }
    void GetPlaybackInfo(PlaybackInfo& playbackInfo) override
    {
        getPlaybackInfoCalled_ = true;
    }
    void SetMediaDuration(int64_t duration) override
    {
        setMediaDurationCalled_ = true;
    }

    bool pauseCalled_ = false;
    bool resumeCalled_ = false;
    bool setReadBlockingFlagCalled_ = false;
    Status getStreamInfoStatus_ = Status::OK;
    Status selectStreamStatus_ = Status::OK;
    bool setDemuxerStateCalled_ = false;
    bool setDownloadErrorStateCalled_ = false;
    Status setCurrentBitRateStatus_ = Status::OK;
    bool setPlayStrategyCalled_ = false;
    Status stopBufferringStatus_ = Status::OK;
    std::pair<int64_t, bool> startInfo_ = {0, false};
    bool waitForBufferingEndCalled_ = false;
    bool notifyInitSuccessCalled_ = false;
    uint64_t cachedDuration_ = 0;
    bool restartAndClearBufferCalled_ = false;
    bool isFlvLive_ = false;
    bool setStartPtsCalled_ = false;
    bool setExtraCacheCalled_ = false;
    bool setMediaStreamsCalled_ = false;
    std::string contentType_ = "video/mp4";
    bool isHlsFmp4_ = false;
    uint64_t memorySize_ = 0;
    std::string curUrl_ = "http://test.com/video.mp4";
    bool isHlsEnd_ = false;
    std::string maskUrl_ = "";
    bool setDefaultStreamIdCalled_ = false;
    bool seekToPosResult_ = true;
    bool seekHit_ = false;
    std::vector<uint32_t> bitRates_ = {1000, 2000, 3000};
    bool getDownloadInfoCalled_ = false;
    bool getPlaybackInfoCalled_ = false;
    bool setMediaDurationCalled_ = false;

    size_t contentLength_ = 4000;
};
}

const uint32_t TIME_WAIT = 500;
const std::string MP4_SEGMENT_BASE = "http://127.0.0.1:46666/dewu.mp4";
static const std::string MPD_SEGMENT_BASE = "http://127.0.0.1:46666/segment_base/index.mpd";
static const std::string M3U8_SEGMENT_BASE = "http://127.0.0.1:46666/test_hls/testHLSEncode.m3u8";
const std::string FLV_SEGMENT_BASE = "http://127.0.0.1:46666/h264.flv";

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server;
class HttpSourcePluginUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
protected:
    std::shared_ptr<HttpSourcePlugin> httpSourcePlugin;
    std::shared_ptr<MediaSource> mediaSource;
    std::shared_ptr<Meta> meta;
    std::shared_ptr<Buffer> buffer;
};

void HttpSourcePluginUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
}

void HttpSourcePluginUnitTest::TearDownTestCase(void)
{
    g_server->StopServer();
    g_server = nullptr;
}

void HttpSourcePluginUnitTest::SetUp(void)
{
    meta = std::make_shared<Meta>();
    httpSourcePlugin = std::make_shared<HttpSourcePlugin>("test");
    buffer = std::make_shared<Buffer>();
    mediaSource = nullptr;
}

void HttpSourcePluginUnitTest::TearDown(void)
{
    if (httpSourcePlugin) {
        httpSourcePlugin->Deinit();
        OSAL::SleepFor(TIME_WAIT);
    }
    mediaSource.reset();
    httpSourcePlugin.reset();
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_M3U8_pause_resume, TestSize.Level1)
{
    httpSourcePlugin->SelectStream(0);
    httpSourcePlugin->Pause();
    httpSourcePlugin->Resume();
    mediaSource = std::make_shared<MediaSource>(M3U8_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    httpSourcePlugin->SelectStream(0);
    httpSourcePlugin->Pause();
    httpSourcePlugin->Resume();
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_MP4_SetPlayStrategy, TestSize.Level1)
{
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1920;
    playStrategy->height = 1080;
    playStrategy->duration = 100;
    playStrategy->preferHDR = false;
    mediaSource = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    mediaSource->SetPlayStrategy(playStrategy);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_M3U8_SetPlayStrategy, TestSize.Level1)
{
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1920;
    playStrategy->height = 1080;
    playStrategy->duration = 100;
    playStrategy->preferHDR = false;
    mediaSource = std::make_shared<MediaSource>(M3U8_SEGMENT_BASE);
    mediaSource->SetPlayStrategy(playStrategy);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_MPD_SetPlayStrategy, TestSize.Level1)
{
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1920;
    playStrategy->height = 1080;
    playStrategy->duration = 100;
    playStrategy->preferHDR = false;
    mediaSource = std::make_shared<MediaSource>(MPD_SEGMENT_BASE);
    mediaSource->SetPlayStrategy(playStrategy);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_MP4, TestSize.Level0)
{
    mediaSource = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->SeekTo(10);
    httpSourcePlugin->SeekTo(100000);
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_MP4_DumuxState, TestSize.Level0)
{
    mediaSource = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->SetDemuxerState(0);
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->SeekTo(10);
    httpSourcePlugin->SeekTo(100000);
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_FUNC, TestSize.Level1)
{
    uint64_t size;
    int64_t duration;
    std::vector<uint32_t> bitRates;
    httpSourcePlugin->GetDuration(duration);
    httpSourcePlugin->GetBitRates(bitRates);
    httpSourcePlugin->GetSize(size);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->SetCurrentBitRate(10, 0);
    mediaSource = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->GetSize(size);
    httpSourcePlugin->GetDuration(duration);
    httpSourcePlugin->SelectBitRate(10);
    httpSourcePlugin->GetBitRates(bitRates);
    httpSourcePlugin->SetDemuxerState(0);
    httpSourcePlugin->SetCurrentBitRate(10, 0);
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_INFO, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    DownloadInfo downloadInfo;
    PlaybackInfo playbackInfo;
    httpSourcePlugin->GetDownloadInfo(downloadInfo);
    httpSourcePlugin->GetPlaybackInfo(playbackInfo);
    httpSourcePlugin->GetStreamInfo(streams);
    mediaSource = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->GetStreamInfo(streams);
    httpSourcePlugin->SetReadBlockingFlag(true);
    httpSourcePlugin->SetInterruptState(true);
    httpSourcePlugin->GetDownloadInfo(downloadInfo);
    httpSourcePlugin->GetPlaybackInfo(playbackInfo);
    httpSourcePlugin->SetDownloadErrorState();
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_MPD, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(MPD_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->SeekTo(10);
    httpSourcePlugin->SeekTo(100000);
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_M3U8, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(M3U8_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    httpSourcePlugin->SeekToTime(100000, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_M3U8_SetDemuxerState, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(M3U8_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->SetDemuxerState(0);
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    httpSourcePlugin->SeekToTime(100000, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, Prepare_IsTrue, TestSize.Level1)
{
    httpSourcePlugin->delayReady_ = true;
    Status status = httpSourcePlugin->Prepare();
    EXPECT_EQ(status, Status::ERROR_DELAY_READY);
}

HWTEST_F(HttpSourcePluginUnitTest, Prepare_IsFalse, TestSize.Level1)
{
    httpSourcePlugin->delayReady_ = false;
    Status status = httpSourcePlugin->Prepare();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_Reset, TestSize.Level1)
{
    Status status = httpSourcePlugin->Reset();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetReadBlockingFlag_01, TestSize.Level1)
{
    bool isReadBlockingAllowed = true;
    Status status = httpSourcePlugin->SetReadBlockingFlag(isReadBlockingAllowed);
    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

HWTEST_F(HttpSourcePluginUnitTest, SetReadBlockingFlag_02, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    bool isReadBlockingAllowed = true;
    Status status = httpSourcePlugin->SetReadBlockingFlag(isReadBlockingAllowed);
    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_Start, TestSize.Level1)
{
    Status status = httpSourcePlugin->Start();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_Stop, TestSize.Level1)
{
    Status status = httpSourcePlugin->Stop();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_GetParameter, TestSize.Level1)
{
    Status status = httpSourcePlugin->GetParameter(meta);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_SetParameter_Basic, TestSize.Level1)
{
    Status status = httpSourcePlugin->SetParameter(meta);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_SetParameter6, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(M3U8_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    meta->SetData(Tag::MEDIA_DURATION, static_cast<int64_t>(1000));
    Status status = httpSourcePlugin->SetParameter(meta);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_SetCallback, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    auto sourceCallback = std::make_shared<SourceCallback>();
    Status status = httpSourcePlugin->SetCallback(sourceCallback);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource, TestSize.Level1)
{
    httpSourcePlugin->SetDownloaderBySource(nullptr);
    ASSERT_EQ(httpSourcePlugin->downloader_, nullptr);
    ASSERT_EQ(httpSourcePlugin->uri_, "");
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported1, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "video/mp4";
    httpSourcePlugin->uri_ = "http://example.com/video.m3u8";
    EXPECT_TRUE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported2, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "video/mp4";
    httpSourcePlugin->uri_ = "http://example.com/video.mpd";
    EXPECT_TRUE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported3, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "video/mp4";
    httpSourcePlugin->uri_ = "http://example.com/video.mp4";
    EXPECT_FALSE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported4, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "application/x-mpegURL";
    httpSourcePlugin->uri_ = "http://example.com/video.mp4";
    EXPECT_FALSE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported5, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "https://cdn-audio.qimao.com/103631e612434cb62a10227bccc5d596/687daf28/tts/5/30/10/"
    "1h8m3u8rtbJpBe63EHmhJwyKzaNDe.mp3?expire=1753150104";
    EXPECT_FALSE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported6, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http://xxx/xxxx?autotype=m3u8";
    EXPECT_TRUE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported7, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http://xxx/aaa.doplaylist?autotype=m3u8";
    EXPECT_TRUE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported8, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http://xxx/aaa.doplaylist?autotypem3u8";
    EXPECT_FALSE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported9, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http:aaa.doplaylist?autotype=m3u8";
    EXPECT_FALSE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported10, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http://autotype=m3u8";
    EXPECT_TRUE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported11, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http://1500005450.vod2.myqcloud.com/playlist_eof.m3u8#maxBufferSize=30000";
    EXPECT_TRUE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, Read_IsNull, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->Read(1, buffer, 0, 10);
    EXPECT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, SetInterruptState1, TestSize.Level1)
{
    bool isInterruptNeeded = true;
    httpSourcePlugin->SetInterruptState(isInterruptNeeded);
    EXPECT_EQ(httpSourcePlugin->isInterruptNeeded_, isInterruptNeeded);
}

HWTEST_F(HttpSourcePluginUnitTest, SetInterruptState2, TestSize.Level1)
{
    bool isInterruptNeeded = false;
    httpSourcePlugin->SetInterruptState(isInterruptNeeded);
    EXPECT_EQ(httpSourcePlugin->isInterruptNeeded_, isInterruptNeeded);
}

HWTEST_F(HttpSourcePluginUnitTest, SEEK_TO_TIME_BY_STREAMID_001, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(M3U8_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->MediaSeekTimeByStreamId(1, SeekMode::SEEK_NEXT_SYNC, 1);
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, GET_SEEKABLE_RANGES_001, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    auto ranges = httpSourcePlugin->GetSeekableRanges();
    EXPECT_EQ(ranges.size(), 0);
}

HWTEST_F(HttpSourcePluginUnitTest, GET_SEEKABLE_RANGES_002, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(M3U8_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    auto ranges = httpSourcePlugin->GetSeekableRanges();
    EXPECT_TRUE(httpSourcePlugin);
}

HWTEST_F(HttpSourcePluginUnitTest, INIT_HTTP_SOURCE_001, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(FLV_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    MediaStreamList mediaStreams;
    std::shared_ptr<PlayMediaStream> mediaStreamA = std::make_shared<PlayMediaStream>();
    mediaStreamA->width = 480;
    mediaStreamA->height = 360;
    mediaStreamA->bitrate = 3200;
    mediaStreamA->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamA);
    std::shared_ptr<PlayMediaStream> mediaStreamB = std::make_shared<PlayMediaStream>();
    mediaStreamB->width = 640;
    mediaStreamB->height = 480;
    mediaStreamB->bitrate = 4800;
    mediaStreamB->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamB);
    std::shared_ptr<PlayMediaStream> mediaStreamC = std::make_shared<PlayMediaStream>();
    mediaStreamC->width = 640;
    mediaStreamC->height = 480;
    mediaStreamC->bitrate = 4000;
    mediaStreamC->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamC);
    std::shared_ptr<PlayMediaStream> mediaStreamD = std::make_shared<PlayMediaStream>();
    mediaStreamD->width = 1280;
    mediaStreamD->height = 720;
    mediaStreamD->bitrate = 8000;
    mediaStreamD->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamD);
    std::shared_ptr<PlayMediaStream> mediaStreamE = std::make_shared<PlayMediaStream>();
    mediaStreamE->width = 1280;
    mediaStreamE->height = 720;
    mediaStreamE->bitrate = 4000;
    mediaStreamE->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamE);

    mediaSource->playMediaStreamVec_ = mediaStreams;
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    ASSERT_NE(httpSourcePlugin->downloader_, nullptr);
    ASSERT_NE(httpSourcePlugin->uri_, "");
}

HWTEST_F(HttpSourcePluginUnitTest, SetMediaDuration_001, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    int64_t mediaDuration = 1000;
    Status status = httpSourcePlugin->SetMediaDuration(mediaDuration);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetMediaDuration_002, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    int64_t mediaDuration = 0;
    Status status = httpSourcePlugin->SetMediaDuration(mediaDuration);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetMediaDuration_003, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(FLV_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    int64_t mediaDuration = 1000;
    Status status = httpSourcePlugin->SetMediaDuration(mediaDuration);
    EXPECT_EQ(status, Status::OK);
    EXPECT_NE(httpSourcePlugin->downloader_, nullptr);
}

HWTEST_F(HttpSourcePluginUnitTest, SetMediaDuration_004, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(FLV_SEGMENT_BASE);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    int64_t mediaDuration = 0;
    Status status = httpSourcePlugin->SetMediaDuration(mediaDuration);
    EXPECT_EQ(status, Status::OK);
    EXPECT_NE(httpSourcePlugin->downloader_, nullptr);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_001, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->SeekToTime(1000, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_002, TestSize.Level1)
{
    auto mockDownloader = std::make_shared<MockMediaDownloaderForSeek>();
    mockDownloader->seekable_ = Seekable::UNSEEKABLE;
    httpSourcePlugin->downloader_ = mockDownloader;
    Status status = httpSourcePlugin->SeekToTime(1000, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_003, TestSize.Level1)
{
    auto mockDownloader = std::make_shared<MockMediaDownloaderForSeek>();
    mockDownloader->seekable_ = Seekable::SEEKABLE;
    mockDownloader->duration_ = 500;
    httpSourcePlugin->downloader_ = mockDownloader;
    Status status = httpSourcePlugin->SeekToTime(1000, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_EQ(status, Status::ERROR_INVALID_PARAMETER);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_004, TestSize.Level1)
{
    auto mockDownloader = std::make_shared<MockMediaDownloaderForSeek>();
    mockDownloader->seekable_ = Seekable::SEEKABLE;
    mockDownloader->duration_ = 2000;
    mockDownloader->seekToTimeResult_ = false;
    httpSourcePlugin->downloader_ = mockDownloader;
    Status status = httpSourcePlugin->SeekToTime(1000, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_005, TestSize.Level1)
{
    auto mockDownloader = std::make_shared<MockMediaDownloaderForSeek>();
    mockDownloader->seekable_ = Seekable::SEEKABLE;
    mockDownloader->duration_ = 2000;
    mockDownloader->seekToTimeResult_ = true;
    httpSourcePlugin->downloader_ = mockDownloader;
    Status status = httpSourcePlugin->SeekToTime(1000, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_001, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->MediaSeekTimeByStreamId(1000, SeekMode::SEEK_NEXT_SYNC, 0);
    EXPECT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_002, TestSize.Level1)
{
    auto mockDownloader = std::make_shared<MockMediaDownloaderForSeek>();
    mockDownloader->seekable_ = Seekable::UNSEEKABLE;
    httpSourcePlugin->downloader_ = mockDownloader;
    Status status = httpSourcePlugin->MediaSeekTimeByStreamId(1000, SeekMode::SEEK_NEXT_SYNC, 0);
    EXPECT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_003, TestSize.Level1)
{
    auto mockDownloader = std::make_shared<MockMediaDownloaderForSeek>();
    mockDownloader->seekable_ = Seekable::SEEKABLE;
    mockDownloader->duration_ = 500;
    httpSourcePlugin->downloader_ = mockDownloader;
    Status status = httpSourcePlugin->MediaSeekTimeByStreamId(1000, SeekMode::SEEK_NEXT_SYNC, 0);
    EXPECT_EQ(status, Status::ERROR_INVALID_PARAMETER);
}

HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_004, TestSize.Level1)
{
    auto mockDownloader = std::make_shared<MockMediaDownloaderForSeek>();
    mockDownloader->seekable_ = Seekable::SEEKABLE;
    mockDownloader->duration_ = 2000;
    mockDownloader->seekToTimeResult_ = false;
    httpSourcePlugin->downloader_ = mockDownloader;
    Status status = httpSourcePlugin->MediaSeekTimeByStreamId(1000, SeekMode::SEEK_NEXT_SYNC, 0);
    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_005, TestSize.Level1)
{
    auto mockDownloader = std::make_shared<MockMediaDownloaderForSeek>();
    mockDownloader->seekable_ = Seekable::SEEKABLE;
    mockDownloader->duration_ = 2000;
    mockDownloader->seekToTimeResult_ = true;
    httpSourcePlugin->downloader_ = mockDownloader;
    Status status = httpSourcePlugin->MediaSeekTimeByStreamId(1000, SeekMode::SEEK_NEXT_SYNC, 0);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_http_001, TestSize.Level1)
{
    std::string url = "http://example.com/video.mp4";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_https_001, TestSize.Level1)
{
    std::string url = "https://example.com/video.mp4";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_file_001, TestSize.Level1)
{
    std::string url = "file:///storage/video.mp4";
    EXPECT_FALSE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_file_002, TestSize.Level1)
{
    std::string url = "file://C:/Users/test/video.mp4";
    EXPECT_FALSE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_empty_001, TestSize.Level1)
{
    std::string url = "";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_relative_path_001, TestSize.Level1)
{
    std::string url = "/storage/video.mp4";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_relative_path_002, TestSize.Level1)
{
    std::string url = "video.mp4";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_rtsp_001, TestSize.Level1)
{
    std::string url = "rtsp://example.com/stream";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_rtmp_001, TestSize.Level1)
{
    std::string url = "rtmp://example.com/stream";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_m3u8_001, TestSize.Level1)
{
    std::string url = "http://example.com/video.m3u8";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_mpd_001, TestSize.Level1)
{
    std::string url = "http://example.com/video.mpd";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_case_sensitive_001, TestSize.Level1)
{
    std::string url = "FILE:///storage/video.mp4";
    EXPECT_FALSE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_case_sensitive_002, TestSize.Level1)
{
    std::string url = "File:///storage/video.mp4";
    EXPECT_FALSE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_case_sensitive_003, TestSize.Level1)
{
    std::string url = "fILE:///storage/video.mp4";
    EXPECT_FALSE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_case_sensitive_004, TestSize.Level1)
{
    std::string url = "HTTP://example.com/video.mp4";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_case_sensitive_005, TestSize.Level1)
{
    std::string url = "Https://example.com/video.mp4";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_case_sensitive_006, TestSize.Level1)
{
    std::string url = "RTSP://example.com/stream";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_with_params_001, TestSize.Level1)
{
    std::string url = "http://example.com/video.mp4?token=abc&expire=123";
    EXPECT_TRUE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_file_with_params_001, TestSize.Level1)
{
    std::string url = "file:///storage/video.mp4?param=value";
    EXPECT_FALSE(httpSourcePlugin->IsAllowedProtocol(url));
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_file_protocol_001, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>("file:///storage/video.mp4");
    Status status = httpSourcePlugin->SetSource(mediaSource);
    EXPECT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_file_protocol_002, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>("FILE:///storage/video.mp4");
    Status status = httpSourcePlugin->SetSource(mediaSource);
    EXPECT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_file_protocol_003, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>("File:///storage/video.mp4");
    Status status = httpSourcePlugin->SetSource(mediaSource);
    EXPECT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_file_protocol_004, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>("file://C:/Users/test/video.mp4");
    Status status = httpSourcePlugin->SetSource(mediaSource);
    EXPECT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_file_protocol_005, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>("FILE://C:/Users/test/video.mp4");
    Status status = httpSourcePlugin->SetSource(mediaSource);
    EXPECT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_http_protocol_001, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    Status status = httpSourcePlugin->SetSource(mediaSource);
    EXPECT_NE(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_http_protocol_002, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>("HTTP://127.0.0.1:46666/dewu.mp4");
    Status status = httpSourcePlugin->SetSource(mediaSource);
    EXPECT_NE(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_https_protocol_001, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>("HTTPS://example.com/video.mp4");
    Status status = httpSourcePlugin->SetSource(mediaSource);
    EXPECT_NE(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, Init_Basic, TestSize.Level1)
{
    Status status = httpSourcePlugin->Init();
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, GetContentType_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    std::string result = httpSourcePlugin->GetContentType();
    ASSERT_EQ(result, "");
}

HWTEST_F(HttpSourcePluginUnitTest, GetContentType_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    std::string result = httpSourcePlugin->GetContentType();
    ASSERT_EQ(result, "video/mp4");
}

HWTEST_F(HttpSourcePluginUnitTest, GetStreamInfo_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    std::vector<StreamInfo> streams;
    Status status = httpSourcePlugin->GetStreamInfo(streams);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, GetStreamInfo_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    std::vector<StreamInfo> streams;
    Status status = httpSourcePlugin->GetStreamInfo(streams);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SelectStream_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->SelectStream(0);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SelectStream_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SelectStream(1);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, GetSize_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    uint64_t size = 999;
    Status status = httpSourcePlugin->GetSize(size);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, GetSize_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->contentLength_ = 8000;
    httpSourcePlugin->downloader_ = mock;
    uint64_t size = 0;
    Status status = httpSourcePlugin->GetSize(size);
    ASSERT_EQ(status, Status::OK);
    ASSERT_EQ(size, (uint64_t)8000);
}

HWTEST_F(HttpSourcePluginUnitTest, GetSeekable_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Seekable seekable = httpSourcePlugin->GetSeekable();
    ASSERT_EQ(seekable, Seekable::INVALID);
}

HWTEST_F(HttpSourcePluginUnitTest, GetSeekable_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->seekable_ = Seekable::SEEKABLE;
    httpSourcePlugin->downloader_ = mock;
    Seekable seekable = httpSourcePlugin->GetSeekable();
    ASSERT_EQ(seekable, Seekable::SEEKABLE);
}

HWTEST_F(HttpSourcePluginUnitTest, GetDuration_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    int64_t duration = 999;
    Status status = httpSourcePlugin->GetDuration(duration);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, GetDuration_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->duration_ = 5000;
    httpSourcePlugin->downloader_ = mock;
    int64_t duration = 0;
    Status status = httpSourcePlugin->GetDuration(duration);
    ASSERT_EQ(status, Status::OK);
    ASSERT_EQ(duration, (int64_t)5000);
}

HWTEST_F(HttpSourcePluginUnitTest, GetStartInfo_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    std::pair<int64_t, bool> startInfo;
    Status status = httpSourcePlugin->GetStartInfo(startInfo);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, GetStartInfo_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    std::pair<int64_t, bool> startInfo;
    Status status = httpSourcePlugin->GetStartInfo(startInfo);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, GetBitRates_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    std::vector<uint32_t> bitRates;
    Status status = httpSourcePlugin->GetBitRates(bitRates);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, GetBitRates_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    std::vector<uint32_t> bitRates;
    Status status = httpSourcePlugin->GetBitRates(bitRates);
    ASSERT_EQ(status, Status::OK);
    ASSERT_EQ(bitRates.size(), (size_t)3);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekTo_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->SeekTo(100);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekTo_Unseekable, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->seekable_ = Seekable::UNSEEKABLE;
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SeekTo(100);
    ASSERT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekTo_OffsetExceedsContent, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->contentLength_ = 4000;
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SeekTo(5000);
    ASSERT_EQ(status, Status::ERROR_INVALID_PARAMETER);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekTo_SeekToPosFalse, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->contentLength_ = 4000;
    mock->seekToPosResult_ = false;
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SeekTo(100);
    ASSERT_EQ(status, Status::ERROR_UNKNOWN);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekTo_SeekToPosTrue, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->contentLength_ = 4000;
    mock->seekToPosResult_ = true;
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SeekTo(100);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekTo_OffsetZeroContentLength, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->contentLength_ = 0;
    mock->seekToPosResult_ = true;
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SeekTo(5000);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekTo_SeekErrorCountExceedsWithCallback, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->contentLength_ = 4000;
    httpSourcePlugin->downloader_ = mock;
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetCallback(sourceCallback);
    for (int i = 0; i < 6; i++) {
        httpSourcePlugin->SeekTo(5000);
    }
    ASSERT_EQ(httpSourcePlugin->seekErrorCount_, 6);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekTo_SeekErrorCountExceedsWithoutCallback, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->contentLength_ = 4000;
    httpSourcePlugin->downloader_ = mock;
    for (int i = 0; i < 6; i++) {
        httpSourcePlugin->SeekTo(5000);
    }
    ASSERT_EQ(httpSourcePlugin->seekErrorCount_, 6);
}

HWTEST_F(HttpSourcePluginUnitTest, SelectBitRate_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->SelectBitRate(1000);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, SelectBitRate_True, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SelectBitRate(1000);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SelectBitRate_False, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SelectBitRate(0);
    ASSERT_EQ(status, Status::ERROR_UNKNOWN);
}

HWTEST_F(HttpSourcePluginUnitTest, AutoSelectBitRate_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->AutoSelectBitRate(1000);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, AutoSelectBitRate_True, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->AutoSelectBitRate(1000);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, AutoSelectBitRate_False, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->AutoSelectBitRate(0);
    ASSERT_EQ(status, Status::ERROR_UNKNOWN);
}

HWTEST_F(HttpSourcePluginUnitTest, GetDownloadInfo_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    DownloadInfo downloadInfo;
    Status status = httpSourcePlugin->GetDownloadInfo(downloadInfo);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, GetDownloadInfo_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    DownloadInfo downloadInfo;
    Status status = httpSourcePlugin->GetDownloadInfo(downloadInfo);
    ASSERT_EQ(status, Status::OK);
    ASSERT_TRUE(mock->getDownloadInfoCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, GetPlaybackInfo_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    PlaybackInfo playbackInfo;
    Status status = httpSourcePlugin->GetPlaybackInfo(playbackInfo);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, GetPlaybackInfo_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    PlaybackInfo playbackInfo;
    Status status = httpSourcePlugin->GetPlaybackInfo(playbackInfo);
    ASSERT_EQ(status, Status::OK);
    ASSERT_TRUE(mock->getPlaybackInfoCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, SetCurrentBitRate_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->SetCurrentBitRate(1000, 0);
    ASSERT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SetCurrentBitRate_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SetCurrentBitRate(1000, 0);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, GetSegmentOffset_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    size_t offset = httpSourcePlugin->GetSegmentOffset();
    ASSERT_EQ(offset, (size_t)0);
}

HWTEST_F(HttpSourcePluginUnitTest, GetSegmentOffset_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    size_t offset = httpSourcePlugin->GetSegmentOffset();
    ASSERT_EQ(offset, (size_t)0);
}

HWTEST_F(HttpSourcePluginUnitTest, GetHLSDiscontinuity_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    bool result = httpSourcePlugin->GetHLSDiscontinuity();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, GetHLSDiscontinuity_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    bool result = httpSourcePlugin->GetHLSDiscontinuity();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSourceInitialBufferSize_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    bool result = httpSourcePlugin->SetSourceInitialBufferSize(0, 100);
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSourceInitialBufferSize_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    bool result = httpSourcePlugin->SetSourceInitialBufferSize(0, 100);
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, StopBufferring_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->StopBufferring(false);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, StopBufferring_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->StopBufferring(false);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, WaitForBufferingEnd_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->WaitForBufferingEnd();
    ASSERT_TRUE(mock->waitForBufferingEndCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, NotifyInitSuccess_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->NotifyInitSuccess();
    ASSERT_TRUE(mock->notifyInitSuccessCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, SetStartPts_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->SetStartPts(1000);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, SetStartPts_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SetStartPts(1000);
    ASSERT_EQ(status, Status::OK);
    ASSERT_TRUE(mock->setStartPtsCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, SetExtraCache_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->SetExtraCache(1000);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, SetExtraCache_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SetExtraCache(1000);
    ASSERT_EQ(status, Status::OK);
    ASSERT_TRUE(mock->setExtraCacheCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, GetCachedDuration_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    uint64_t result = httpSourcePlugin->GetCachedDuration();
    ASSERT_EQ(result, (uint64_t)0);
}

HWTEST_F(HttpSourcePluginUnitTest, GetCachedDuration_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->cachedDuration_ = 5000;
    httpSourcePlugin->downloader_ = mock;
    uint64_t result = httpSourcePlugin->GetCachedDuration();
    ASSERT_EQ(result, (uint64_t)5000);
}

HWTEST_F(HttpSourcePluginUnitTest, RestartAndClearBuffer_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->RestartAndClearBuffer();
    ASSERT_TRUE(mock->restartAndClearBufferCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, IsFlvLive_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    bool result = httpSourcePlugin->IsFlvLive();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, IsFlvLive_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    bool result = httpSourcePlugin->IsFlvLive();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, IsHlsFmp4_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    bool result = httpSourcePlugin->IsHlsFmp4();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, IsHlsFmp4_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    bool result = httpSourcePlugin->IsHlsFmp4();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, GetMemorySize_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    uint64_t result = httpSourcePlugin->GetMemorySize();
    ASSERT_EQ(result, (uint64_t)0);
}

HWTEST_F(HttpSourcePluginUnitTest, GetMemorySize_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->memorySize_ = 1024;
    httpSourcePlugin->downloader_ = mock;
    uint64_t result = httpSourcePlugin->GetMemorySize();
    ASSERT_EQ(result, (uint64_t)1024);
}

HWTEST_F(HttpSourcePluginUnitTest, IsHlsEnd_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    bool result = httpSourcePlugin->IsHlsEnd(0);
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, IsHlsEnd_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    bool result = httpSourcePlugin->IsHlsEnd(0);
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, GetMaskUrlByReadIndex_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    std::string result = httpSourcePlugin->GetMaskUrlByReadIndex(0);
    ASSERT_EQ(result, "");
}

HWTEST_F(HttpSourcePluginUnitTest, GetMaskUrlByReadIndex_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    std::string result = httpSourcePlugin->GetMaskUrlByReadIndex(0);
    ASSERT_EQ(result, "");
}

HWTEST_F(HttpSourcePluginUnitTest, SetDefaultStreamId_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    int32_t videoId = 1, audioId = 2, subId = 3;
    httpSourcePlugin->SetDefaultStreamId(videoId, audioId, subId);
    ASSERT_TRUE(mock->setDefaultStreamIdCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, SetDemuxerState_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->SetDemuxerState(0);
    ASSERT_TRUE(mock->setDemuxerStateCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, SetReadBlockingFlag_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SetReadBlockingFlag(true);
    ASSERT_EQ(status, Status::OK);
    ASSERT_TRUE(mock->setReadBlockingFlagCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, Pause_WithM3U8Uri, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->uri_ = "http://example.com/test.m3u8";
    Status status = httpSourcePlugin->Pause();
    ASSERT_EQ(status, Status::OK);
    ASSERT_TRUE(mock->pauseCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, Pause_WithNonM3U8Uri, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->uri_ = "http://example.com/test.mp4";
    Status status = httpSourcePlugin->Pause();
    ASSERT_EQ(status, Status::OK);
    ASSERT_FALSE(mock->pauseCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, Resume_WithM3U8Uri, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->uri_ = "http://example.com/test.m3u8";
    Status status = httpSourcePlugin->Resume();
    ASSERT_EQ(status, Status::OK);
    ASSERT_TRUE(mock->resumeCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, Resume_WithNonM3U8Uri, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->uri_ = "http://example.com/test.mp4";
    Status status = httpSourcePlugin->Resume();
    ASSERT_EQ(status, Status::OK);
    ASSERT_FALSE(mock->resumeCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, Stop_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->uri_ = "http://example.com/test.mp4";
    Status status = httpSourcePlugin->Stop();
    ASSERT_EQ(status, Status::OK);
    ASSERT_EQ(httpSourcePlugin->uri_, "");
}

HWTEST_F(HttpSourcePluginUnitTest, Reset_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->uri_ = "http://example.com/test.mp4";
    Status status = httpSourcePlugin->Reset();
    ASSERT_EQ(status, Status::OK);
    ASSERT_EQ(httpSourcePlugin->uri_, "");
}

HWTEST_F(HttpSourcePluginUnitTest, Deinit_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->uri_ = "http://example.com/test.mp4";
    Status status = httpSourcePlugin->Deinit();
    ASSERT_EQ(status, Status::OK);
    ASSERT_EQ(httpSourcePlugin->uri_, "");
}

HWTEST_F(HttpSourcePluginUnitTest, IsHls_M3U8MimeType, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = AVMimeTypes::APPLICATION_M3U8;
    bool result = httpSourcePlugin->IsHls();
    ASSERT_TRUE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, IsHls_M3U8Uri, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "video/mp4";
    httpSourcePlugin->uri_ = "http://example.com/test.m3u8";
    bool result = httpSourcePlugin->IsHls();
    ASSERT_TRUE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, IsHls_NotHls, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "video/mp4";
    httpSourcePlugin->uri_ = "http://example.com/test.mp4";
    bool result = httpSourcePlugin->IsHls();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, Read_BufferIsNull, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    std::shared_ptr<Buffer> nullBuffer = nullptr;
    Status status = httpSourcePlugin->Read(1, nullBuffer, 0, 100);
    ASSERT_EQ(status, Status::OK);
    ASSERT_NE(nullBuffer, nullptr);
}

HWTEST_F(HttpSourcePluginUnitTest, GetCurUrl_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    std::string url = httpSourcePlugin->GetCurUrl();
    ASSERT_EQ(url, "http://test.com/video.mp4");
}

HWTEST_F(HttpSourcePluginUnitTest, GetCurUrl_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    std::string url = httpSourcePlugin->GetCurUrl();
    ASSERT_EQ(url, "");
}

HWTEST_F(HttpSourcePluginUnitTest, SetCallback_WithDownloader, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    auto sourceCallback = std::make_shared<SourceCallback>();
    Status status = httpSourcePlugin->SetCallback(sourceCallback);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetMediaDuration_WithDownloaderPositive, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SetMediaDuration(1000);
    ASSERT_EQ(status, Status::OK);
    ASSERT_TRUE(mock->setMediaDurationCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, CheckIsM3U8Uri_WithQueryM3U8, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http://xxx/aaaa?autotype=m3u8";
    httpSourcePlugin->mimeType_ = "video/mp4";
    bool result = httpSourcePlugin->IsHls();
    ASSERT_TRUE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, CheckIsM3U8Uri_NoDotNoM3U8, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http://example.com/justvideo";
    httpSourcePlugin->mimeType_ = "video/mp4";
    bool result = httpSourcePlugin->IsHls();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, CheckIsM3U8Uri_WithDotButNotM3U8, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http://example.com/video.mp4";
    httpSourcePlugin->mimeType_ = "video/mp4";
    bool result = httpSourcePlugin->IsHls();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, CheckIsM3U8Uri_EmptyUri, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "";
    httpSourcePlugin->mimeType_ = "video/mp4";
    bool result = httpSourcePlugin->IsHls();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, CheckIsM3U8Uri_NoSlash, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "noslashuri";
    httpSourcePlugin->mimeType_ = "video/mp4";
    bool result = httpSourcePlugin->IsHls();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, CheckIsM3U8Uri_DotM3U8WithParams, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http://example.com/video.m3u8?token=abc";
    httpSourcePlugin->mimeType_ = "video/mp4";
    bool result = httpSourcePlugin->IsHls();
    ASSERT_TRUE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, CheckIsM3U8Uri_DotNotM3U8WithM3U8Param, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http://example.com/video.mp4?type=m3u8";
    httpSourcePlugin->mimeType_ = "video/mp4";
    bool result = httpSourcePlugin->IsHls();
    ASSERT_TRUE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, CheckIsM3U8Uri_DotNotM3U8WithNoM3U8Param, TestSize.Level1)
{
    httpSourcePlugin->uri_ = "http://example.com/video.mp4?type=mp4";
    httpSourcePlugin->mimeType_ = "video/mp4";
    bool result = httpSourcePlugin->IsHls();
    ASSERT_FALSE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, Read_NullDownloaderTwoArg, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->Read(buffer, 0, 100);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_Twice, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    httpSourcePlugin->SetSource(mediaSource);
    Status status = httpSourcePlugin->SetSource(mediaSource);
    ASSERT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_NullSource, TestSize.Level1)
{
    Status status = httpSourcePlugin->SetSource(nullptr);
    ASSERT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_NegativeDuration, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->duration_ = -2;
    mock->seekToTimeResult_ = true;
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_NegativeDuration, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->duration_ = -2;
    mock->seekToTimeResult_ = true;
    httpSourcePlugin->downloader_ = mock;
    Status status = httpSourcePlugin->MediaSeekTimeByStreamId(1, SeekMode::SEEK_NEXT_SYNC, 0);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, Read_BufferNotEmpty, TestSize.Level1)
{
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    auto buf = std::make_shared<Buffer>();
    auto mem = buf->AllocMemory(nullptr, 100);
    ASSERT_NE(mem, nullptr);
    Status status = httpSourcePlugin->Read(1, buf, 0, 100);
    ASSERT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_M3U8StrategyZeroDuration, TestSize.Level1)
{
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1920;
    playStrategy->height = 1080;
    playStrategy->duration = 0;
    mediaSource = std::make_shared<MediaSource>(M3U8_SEGMENT_BASE);
    mediaSource->SetPlayStrategy(playStrategy);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpSourcePlugin->SetSource(mediaSource);
    httpSourcePlugin->SetCallback(sourceCallback);
    ASSERT_NE(httpSourcePlugin->downloader_, nullptr);
    ASSERT_NE(httpSourcePlugin->uri_, "");
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_HttpNoCallback, TestSize.Level1)
{
    mediaSource = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    Status status = httpSourcePlugin->SetSource(mediaSource);
    ASSERT_NE(status, Status::ERROR_INVALID_OPERATION);
    ASSERT_NE(httpSourcePlugin->downloader_, nullptr);
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported_M3U8MimeType, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = AVMimeTypes::APPLICATION_M3U8;
    httpSourcePlugin->uri_ = "http://example.com/test.mp4";
    bool result = httpSourcePlugin->IsSeekToTimeSupported();
    ASSERT_TRUE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, WaitForBufferingEnd_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    httpSourcePlugin->WaitForBufferingEnd();
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->WaitForBufferingEnd();
    ASSERT_TRUE(mock->waitForBufferingEndCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, NotifyInitSuccess_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    httpSourcePlugin->NotifyInitSuccess();
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->NotifyInitSuccess();
    ASSERT_TRUE(mock->notifyInitSuccessCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, RestartAndClearBuffer_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    httpSourcePlugin->RestartAndClearBuffer();
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->RestartAndClearBuffer();
    ASSERT_TRUE(mock->restartAndClearBufferCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, SetDefaultStreamId_NullDownloader, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    int32_t videoId = 1, audioId = 2, subId = 3;
    httpSourcePlugin->SetDefaultStreamId(videoId, audioId, subId);
    auto mock = std::make_shared<MockMediaDownloaderForSeek>();
    httpSourcePlugin->downloader_ = mock;
    httpSourcePlugin->SetDefaultStreamId(videoId, audioId, subId);
    ASSERT_TRUE(mock->setDefaultStreamIdCalled_);
}

HWTEST_F(HttpSourcePluginUnitTest, SetSource_FlvStrategyZeroDuration, TestSize.Level1)
{
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1920;
    playStrategy->height = 1080;
    playStrategy->duration = 0;
    mediaSource = std::make_shared<MediaSource>(FLV_SEGMENT_BASE);
    mediaSource->SetPlayStrategy(playStrategy);
    httpSourcePlugin->SetSource(mediaSource);
    ASSERT_NE(httpSourcePlugin->downloader_, nullptr);
    ASSERT_NE(httpSourcePlugin->uri_, "");
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_ShortUrl, TestSize.Level1)
{
    std::string url = "f";
    bool result = httpSourcePlugin->IsAllowedProtocol(url);
    ASSERT_TRUE(result);
}

HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_ExactFilePrefix, TestSize.Level1)
{
    std::string url = "file://";
    bool result = httpSourcePlugin->IsAllowedProtocol(url);
    ASSERT_FALSE(result);
}
}
}
}
}