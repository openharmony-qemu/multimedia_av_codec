/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "source_callback.h"
#include "http_server_demo.h"
#include "sniffer_test_support.h"
#include "monitor/download_monitor.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr size_t TEN = 10;
constexpr int32_t ROUTING_TEST_PORT = 48668;
constexpr size_t ROUTING_BODY_SIZE = 512;
const std::string ROUTING_BASE = "http://127.0.0.1:" + std::to_string(ROUTING_TEST_PORT);

class MockSourceLoader : public Plugins::IMediaSourceLoader {
public:
    int32_t Init(std::shared_ptr<Plugins::IMediaSourceLoadingRequest> &request) override { return 0; }
    int64_t Open(const string &url, const map<string, string> &header) override { return 0; }
    int32_t Read(int64_t uuid, int64_t requestedOffset, int64_t requestedLength) override { return 0; }
    int32_t Close(int64_t uuid) override { return 0; }
};

class MockMediaDownloader : public MediaDownloader {
public:
    MockMediaDownloader() = default;
    virtual ~MockMediaDownloader() = default;
    bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override
    {
        openCalled_ = true;
        return openResult_;
    }
    void Init() override { initCalled_ = true; }
    void Close(bool isAsync) override { closeCalled_ = true; }
    void Pause() override { pauseCalled_ = true; }
    void Resume() override { resumeCalled_ = true; }
    Status Read(unsigned char* buff, ReadDataInfo& readDataInfo) override
    {
        readCalled_ = true;
        readDataInfo.realReadLength_ = TEN;
        return readStatus_;
    }
    uint64_t GetBufferSize() const override { return 0; }
    size_t GetContentLength() const override { return contentLength_; }
    int64_t GetDuration() const override { return duration_; }
    Seekable GetSeekable() const override { return seekable_; }
    void SetCallback(const std::shared_ptr<Callback>& cb) override { setCallbackCalled_ = true; }
    void SetStatusCallback(StatusCallbackFunc cb) override { (void)cb; }
    void SetInterruptState(bool isInterruptNeeded) override { setInterruptStateCalled_ = true; }
    void SetAppUid(int32_t appUid) override
    {
        setAppUidCalled_ = true;
        lastAppUid_ = appUid;
    }
    bool SelectBitRate(uint32_t bitRate) override { return bitRate == 0 ? false : true; }
    void SetIsTriggerAutoMode(bool isAuto) override { setIsTriggerAutoModeCalled_ = true; }
    bool AutoSelectBitRate(uint32_t bitRate) override { return bitRate == 0 ? false : true; }
    size_t GetSegmentOffset(StreamType streamType = StreamType::VIDEO) override { return segmentOffset_; }
    bool GetHLSDiscontinuity() override { return hlsDiscontinuity_; }
    void SetSourceStatisticsDfx(std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr) override
    {
        setSourceStatisticsDfxCalled_ = true;
    }
    Status GetStreamInfo(std::vector<StreamInfo>& streams, bool isUpdate = false) override
    {
        getStreamInfoCalled_ = true;
        getStreamInfoIsUpdate_ = isUpdate;
        return getStreamInfoStatus_;
    }
    Status SelectStream(int32_t streamId) override
    {
        selectStreamCalled_ = true;
        return selectStreamStatus_;
    }
    void SetDemuxerState(int32_t streamId) override { setDemuxerStateCalled_ = true; }
    void SetDownloadErrorState() override { setDownloadErrorStateCalled_ = true; }
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID) override { return setCurrentBitRateStatus_; }
    void SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy) override { setPlayStrategyCalled_ = true; }
    Status StopBufferring(bool isAppBackground) override { return stopBufferringStatus_; }
    std::pair<int64_t, bool> GetStartInfo() const override { return startInfo_; }
    void WaitForBufferingEnd() override { waitForBufferingEndCalled_ = true; }
    void NotifyInitSuccess() override { notifyInitSuccessCalled_ = true; }
    uint64_t GetCachedDuration() override { return cachedDuration_; }
    void RestartAndClearBuffer() override { restartAndClearBufferCalled_ = true; }
    bool IsFlvLive() override { return isFlvLive_; }
    void SetStartPts(int64_t startPts) override { setStartPtsCalled_ = true; }
    void SetExtraCache(uint64_t cacheDuration) override { setExtraCacheCalled_ = true; }
    void SetMediaStreams(const MediaStreamList& mediaStreams) override { setMediaStreamsCalled_ = true; }
    std::string GetContentType() override { return contentType_; }
    bool IsHlsFmp4() override { return isHlsFmp4_; }
    uint64_t GetMemorySize() override { return memorySize_; }
    std::string GetCurUrl() override { return curUrl_; }
    bool IsHlsEnd(int32_t streamId = -1) override { return isHlsEnd_; }
    std::string GetMaskUrlByReadIndex(int32_t streamId) override { return maskUrl_; }
    void SetDefaultStreamId(int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId) override
    {
        setDefaultStreamIdCalled_ = true;
        lastVideoId_ = videoStreamId;
        lastAudioId_ = audioStreamId;
        lastSubtitleId_ = subTitleStreamId;
    }
    bool SeekToPos(int64_t offset, bool& isSeekHit) override
    {
        isSeekHit = seekHit_;
        return seekToPosResult_;
    }
    bool SeekToTime(int64_t seekTime, SeekMode mode) override { return seekToTimeResult_; }
    bool MediaSeekTimeByStreamId(int64_t seekTime, SeekMode mode, int32_t streamId) override
    {
        return mediaSeekTimeByStreamIdResult_;
    }
    std::vector<uint32_t> GetBitRates() override { return bitRates_; }
    void GetDownloadInfo(DownloadInfo& downloadInfo) override { getDownloadInfoCalled_ = true; }
    void GetPlaybackInfo(PlaybackInfo& playbackInfo) override { getPlaybackInfoCalled_ = true; }
    void SetMediaDuration(int64_t duration) override { setMediaDurationCalled_ = true; }
    void SetReadBlockingFlag(bool isReadBlockingAllowed) override { setReadBlockingFlagCalled_ = true; }
    bool SetInitialBufferSize(int32_t offset, int32_t size) override { return setInitialBufferSizeResult_; }
    std::vector<SeekRange> GetSeekableRanges() const override { return seekableRanges_; }

    bool openCalled_ {false};
    bool openResult_ {true};
    bool initCalled_ {false};
    bool closeCalled_ {false};
    bool pauseCalled_ {false};
    bool resumeCalled_ {false};
    bool readCalled_ {false};
    Status readStatus_ {Status::OK};
    bool setCallbackCalled_ {false};
    bool setInterruptStateCalled_ {false};
    bool setAppUidCalled_ {false};
    int32_t lastAppUid_ {-1};
    bool setIsTriggerAutoModeCalled_ {false};
    bool getStreamInfoCalled_ {false};
    bool getStreamInfoIsUpdate_ {false};
    Status getStreamInfoStatus_ {Status::OK};
    bool selectStreamCalled_ {false};
    Status selectStreamStatus_ {Status::OK};
    bool setDemuxerStateCalled_ {false};
    bool setDownloadErrorStateCalled_ {false};
    Status setCurrentBitRateStatus_ {Status::OK};
    bool setPlayStrategyCalled_ {false};
    Status stopBufferringStatus_ {Status::OK};
    std::pair<int64_t, bool> startInfo_ {0, false};
    bool waitForBufferingEndCalled_ {false};
    bool notifyInitSuccessCalled_ {false};
    uint64_t cachedDuration_ {0};
    bool restartAndClearBufferCalled_ {false};
    bool isFlvLive_ {false};
    bool setStartPtsCalled_ {false};
    bool setExtraCacheCalled_ {false};
    bool setMediaStreamsCalled_ {false};
    std::string contentType_ {"video/mp4"};
    bool isHlsFmp4_ {false};
    uint64_t memorySize_ {0};
    std::string curUrl_ {""};
    bool isHlsEnd_ {false};
    std::string maskUrl_ {""};
    bool setDefaultStreamIdCalled_ {false};
    int32_t lastVideoId_ {-1};
    int32_t lastAudioId_ {-1};
    int32_t lastSubtitleId_ {-1};
    bool seekToPosResult_ {true};
    bool seekHit_ {false};
    bool seekToTimeResult_ {true};
    bool mediaSeekTimeByStreamIdResult_ {true};
    std::vector<uint32_t> bitRates_ {1000, 2000, 3000};
    bool getDownloadInfoCalled_ {false};
    bool getPlaybackInfoCalled_ {false};
    bool setMediaDurationCalled_ {false};
    bool setReadBlockingFlagCalled_ {false};
    bool setInitialBufferSizeResult_ {true};
    std::vector<SeekRange> seekableRanges_ {};
    bool setSourceStatisticsDfxCalled_ {false};
    size_t contentLength_ {4};
    size_t segmentOffset_ {1000};  // nanoseconds; HttpSourcePlugin converts to microseconds (/ US_2_NS)
    int64_t duration_ {1000};
    Seekable seekable_ {Seekable::SEEKABLE};
    bool hlsDiscontinuity_ {true};
};

class MockCallback : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override { (void)event; }
};
}

class HttpSourcePluginUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp(void);
    void TearDown(void);
protected:
    std::shared_ptr<HttpSourcePlugin> plugin_;
};

void HttpSourcePluginUnitTest::SetUp(void)
{
    plugin_ = std::make_shared<HttpSourcePlugin>("mock");
}

void HttpSourcePluginUnitTest::TearDown(void)
{
    plugin_.reset();
}

/**
 * @tc.name  : Test SetDownloaderBySource API
 * @tc.number: SetDownloaderBySource_001
 * @tc.desc  : Null source loader → sniffer uses curl → unreachable URL → fail gracefully (no crash)
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    // Port 1 is immediately refused → sniffer curl fails fast (3 retries, each instant) → no downloader.
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>("http://127.0.0.1:1/mock");
    ASSERT_EQ(nullptr, source->sourceLoader_);
    plugin_->SetDownloaderBySource(source);
    ASSERT_EQ(nullptr, plugin_->loaderCombinations_);
    ASSERT_EQ(nullptr, plugin_->downloader_) << "sniffer fails on unreachable URL → no downloader";

}

/**
 * @tc.name  : Test IsSeekToTimeSupported API
 * @tc.number: IsSeekToTimeSupported_001
 * @tc.desc  : Test mimeType_== AVMimeTypes::APPLICATION_M3U8
 */
HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported_001, TestSize.Level1)
{
    plugin_->protocolType_ = StreamProtocolType::HLS;
    ASSERT_NE(nullptr, plugin_);
    plugin_->mimeType_ = AVMimeTypes::APPLICATION_M3U8;
    ASSERT_EQ(true, plugin_->IsSeekToTimeSupported());
}

/**
 * @tc.name  : Test SeekTo API
 * @tc.number: SeekTo_001
 * @tc.desc  : Test offset > downloader_->GetContentLength() && downloader_->GetContentLength() != 0
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekTo_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_ = std::make_shared<MockMediaDownloader>();
    plugin_->seekErrorCount_ = 5;
    auto mockCallback = std::make_shared<MockCallback>();
    plugin_->callback_ = mockCallback;
    uint64_t mockOffset {5};
    ASSERT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SeekTo(mockOffset));
    plugin_->seekErrorCount_ = 0;
    ASSERT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SeekTo(mockOffset));
}

/**
 * @tc.name  : Test SelectBitRate API
 * @tc.number: SelectBitRate_001
 * @tc.desc  : Test downloader_->SelectBitRate(bitRate) == false
 */
HWTEST_F(HttpSourcePluginUnitTest, SelectBitRate_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_ = std::make_shared<MockMediaDownloader>();
    ASSERT_EQ(Status::ERROR_UNKNOWN, plugin_->SelectBitRate(0));
}

/**
 * @tc.name  : Test AutoSelectBitRate API
 * @tc.number: AutoSelectBitRate_001
 * @tc.desc  : Test downloader_->AutoSelectBitRate(bitRate) == false
 */
HWTEST_F(HttpSourcePluginUnitTest, AutoSelectBitRate_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_ = std::make_shared<MockMediaDownloader>();
    ASSERT_EQ(Status::ERROR_UNKNOWN, plugin_->AutoSelectBitRate(0));
}

/**
 * @tc.name  : Test AutoSelectBitRate API
 * @tc.number: AutoSelectBitRate_002
 * @tc.desc  : Test downloader_->AutoSelectBitRate(bitRate) == true
 */
HWTEST_F(HttpSourcePluginUnitTest, AutoSelectBitRate_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_ = std::make_shared<MockMediaDownloader>();
    ASSERT_EQ(Status::OK, plugin_->AutoSelectBitRate(1));
}

/**
 * @tc.name  : Test GetSegmentOffset API
 * @tc.number: GetSegmentOffset_001
 * @tc.desc  : Test GetSegmentOffset interface
 */
HWTEST_F(HttpSourcePluginUnitTest, GetSegmentOffset_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    ASSERT_EQ(0, plugin_->GetSegmentOffset());
    plugin_->downloader_ = std::make_shared<MockMediaDownloader>();
    ASSERT_EQ(1, plugin_->GetSegmentOffset());
}

/**
 * @tc.name  : Test GetHLSDiscontinuity API
 * @tc.number: GetHLSDiscontinuity_001
 * @tc.desc  : Test GetHLSDiscontinuity interface
 */
HWTEST_F(HttpSourcePluginUnitTest, GetHLSDiscontinuity_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    ASSERT_EQ(false, plugin_->GetHLSDiscontinuity());
    plugin_->downloader_ = std::make_shared<MockMediaDownloader>();
    ASSERT_EQ(true, plugin_->GetHLSDiscontinuity());
}

/**
 * @tc.name  : Test GetStreamInfo API
 * @tc.number: GetStreamInfo_001
 * @tc.desc  : Test forwarding to downloader_->GetStreamInfo with isUpdate = true
 */
HWTEST_F(HttpSourcePluginUnitTest, GetStreamInfo_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mockDownloader = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mockDownloader;
    std::vector<StreamInfo> streams;
    auto ret = plugin_->GetStreamInfo(streams, true);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_TRUE(mockDownloader->getStreamInfoCalled_);
    ASSERT_TRUE(mockDownloader->getStreamInfoIsUpdate_);
}

/**
 * @tc.name  : Test GetStreamInfo API
 * @tc.number: GetStreamInfo_002
 * @tc.desc  : Test downloader_ == nullptr returns OK and does nothing
 */
HWTEST_F(HttpSourcePluginUnitTest, GetStreamInfo_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    std::vector<StreamInfo> streams;
    auto ret = plugin_->GetStreamInfo(streams, true);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_TRUE(streams.empty());
}

/**
 * @tc.name  : Test SetDefaultStreamId API
 * @tc.number: SetDefaultStreamId_001
 * @tc.desc  : Test forwarding to downloader_->SetDefaultStreamId
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDefaultStreamId_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mockDownloader = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mockDownloader;
    int32_t videoId = 1;
    int32_t audioId = 2;
    int32_t subtitleId = 3;
    plugin_->SetDefaultStreamId(videoId, audioId, subtitleId);
    ASSERT_TRUE(mockDownloader->setDefaultStreamIdCalled_);
    ASSERT_EQ(videoId, mockDownloader->lastVideoId_);
    ASSERT_EQ(audioId, mockDownloader->lastAudioId_);
    ASSERT_EQ(subtitleId, mockDownloader->lastSubtitleId_);
}

/**
 * @tc.name  : Test SetDefaultStreamId API
 * @tc.number: SetDefaultStreamId_002
 * @tc.desc  : Test downloader_ == nullptr does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDefaultStreamId_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    int32_t videoStreamId = 1;
    int32_t audioStreamId = 2;
    int32_t subTitleStreamId = 3;
    plugin_->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    ASSERT_FALSE(plugin_->isInterruptNeeded_);
}

/**
 * @tc.name  : Test InitSourceLoader API
 * @tc.number: InitSourceLoader_001
 * @tc.desc  : Test source->GetSourceLoader() == nullptr does not create loaderCombinations_
 */
HWTEST_F(HttpSourcePluginUnitTest, InitSourceLoader_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    std::string uri {"mock"};
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    ASSERT_EQ(nullptr, source->sourceLoader_);
    plugin_->InitSourceLoader(source);
    ASSERT_EQ(nullptr, plugin_->loaderCombinations_);
    ASSERT_TRUE(plugin_->httpHeader_.empty());
}

/**
 * @tc.name  : Test CreateDashDownloader API
 * @tc.number: CreateDashDownloader_001
 * @tc.desc  : Test CreateDashDownloader returns non-null downloader
 */
HWTEST_F(HttpSourcePluginUnitTest, CreateDashDownloader_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->loaderCombinations_ = nullptr;
    auto downloader = plugin_->CreateDashDownloader();
    ASSERT_NE(nullptr, downloader);
    ASSERT_TRUE(plugin_->delayReady_);
}

/**
 * @tc.name  : Test CreateHlsDownloader API
 * @tc.number: CreateHlsDownloader_001
 * @tc.desc  : Test CreateHlsDownloader returns non-null downloader when playStrategy_ is nullptr
 */
HWTEST_F(HttpSourcePluginUnitTest, CreateHlsDownloader_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->playStrategy_ = nullptr;
    plugin_->loaderCombinations_ = nullptr;
    auto downloader = plugin_->CreateHlsDownloader();
    ASSERT_NE(nullptr, downloader);
    ASSERT_TRUE(plugin_->delayReady_);
}

/**
 * @tc.name  : Test InitSourceLoader API
 * @tc.number: InitSourceLoader_002
 * @tc.desc  : Test InitSourceLoader with sourceLoader != nullptr creates loaderCombinations_
 */
HWTEST_F(HttpSourcePluginUnitTest, InitSourceLoader_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    std::string uri {"mock"};
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    source->sourceLoader_ = std::make_shared<MockSourceLoader>();
    ASSERT_NE(nullptr, source->GetSourceLoader());
    plugin_->InitSourceLoader(source);
    ASSERT_NE(nullptr, plugin_->loaderCombinations_);
    ASSERT_TRUE(plugin_->httpHeader_.empty());
}

/**
 * @tc.name  : Test GetDownloader/SetDownloader thread safety
 * @tc.number: GetDownloader_SetDownloader_001
 * @tc.desc  : Test SetDownloader then GetDownloader returns same downloader
 */
HWTEST_F(HttpSourcePluginUnitTest, GetDownloader_SetDownloader_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mockDownloader = std::make_shared<MockMediaDownloader>();
    plugin_->SetDownloader(mockDownloader);
    auto result = plugin_->GetDownloader();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(static_cast<size_t>(4), result->GetContentLength());
}

/**
 * @tc.name  : Test GetDownloader with no downloader set
 * @tc.number: GetDownloader_001
 * @tc.desc  : Test GetDownloader returns nullptr when no downloader set
 */
HWTEST_F(HttpSourcePluginUnitTest, GetDownloader_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto result = plugin_->GetDownloader();
    ASSERT_EQ(nullptr, result);
    ASSERT_TRUE(plugin_->uri_.empty());
}

/**
 * @tc.name  : Test Prepare API
 * @tc.number: Prepare_001
 * @tc.desc  : Test Prepare returns ERROR_DELAY_READY when delayReady_ is true
 */
HWTEST_F(HttpSourcePluginUnitTest, Prepare_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->delayReady_ = true;
    ASSERT_EQ(Status::ERROR_DELAY_READY, plugin_->Prepare());
}

/**
 * @tc.name  : Test Prepare API
 * @tc.number: Prepare_002
 * @tc.desc  : Test Prepare returns OK when delayReady_ is false
 */
HWTEST_F(HttpSourcePluginUnitTest, Prepare_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->delayReady_ = false;
    ASSERT_EQ(Status::OK, plugin_->Prepare());
}

/**
 * @tc.name  : Test Init API
 * @tc.number: Init_001
 * @tc.desc  : Test Init returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, Init_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    ASSERT_EQ(Status::OK, plugin_->Init());
}

/**
 * @tc.name  : Test Deinit API
 * @tc.number: Deinit_001
 * @tc.desc  : Test Deinit with downloader set calls CloseUri
 */
HWTEST_F(HttpSourcePluginUnitTest, Deinit_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->Deinit());
    ASSERT_TRUE(mock->closeCalled_);
    ASSERT_EQ(nullptr, plugin_->downloader_);
}

/**
 * @tc.name  : Test Deinit API
 * @tc.number: Deinit_002
 * @tc.desc  : Test Deinit without downloader does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, Deinit_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::OK, plugin_->Deinit());
    ASSERT_EQ(nullptr, plugin_->downloader_);
}

/**
 * @tc.name  : Test Start API
 * @tc.number: Start_001
 * @tc.desc  : Test Start returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, Start_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    ASSERT_EQ(Status::OK, plugin_->Start());
}

/**
 * @tc.name  : Test Stop API
 * @tc.number: Stop_001
 * @tc.desc  : Test Stop with downloader set calls CloseUri async
 */
HWTEST_F(HttpSourcePluginUnitTest, Stop_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->Stop());
    ASSERT_TRUE(mock->closeCalled_);
}

/**
 * @tc.name  : Test Stop API
 * @tc.number: Stop_002
 * @tc.desc  : Test Stop without downloader does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, Stop_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::OK, plugin_->Stop());
    ASSERT_EQ(nullptr, plugin_->downloader_);
}

/**
 * @tc.name  : Test Reset API
 * @tc.number: Reset_001
 * @tc.desc  : Test Reset with downloader set calls CloseUri sync
 */
HWTEST_F(HttpSourcePluginUnitTest, Reset_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->Reset());
    ASSERT_TRUE(mock->closeCalled_);
    ASSERT_EQ(nullptr, plugin_->downloader_);
}

/**
 * @tc.name  : Test Reset API
 * @tc.number: Reset_002
 * @tc.desc  : Test Reset without downloader does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, Reset_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::OK, plugin_->Reset());
    ASSERT_EQ(nullptr, plugin_->downloader_);
}

/**
 * @tc.name  : Test GetContentType API
 * @tc.number: GetContentType_001
 * @tc.desc  : Test GetContentType with downloader returns content type
 */
HWTEST_F(HttpSourcePluginUnitTest, GetContentType_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->contentType_ = "video/mp4";
    plugin_->downloader_ = mock;
    ASSERT_EQ("video/mp4", plugin_->GetContentType());
}

/**
 * @tc.name  : Test GetContentType API
 * @tc.number: GetContentType_002
 * @tc.desc  : Test GetContentType with null downloader returns empty string
 */
HWTEST_F(HttpSourcePluginUnitTest, GetContentType_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ("", plugin_->GetContentType());
}

/**
 * @tc.name  : Test SetReadBlockingFlag API
 * @tc.number: SetReadBlockingFlag_001
 * @tc.desc  : Test SetReadBlockingFlag with downloader calls mock method
 */
HWTEST_F(HttpSourcePluginUnitTest, SetReadBlockingFlag_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SetReadBlockingFlag(true));
    ASSERT_TRUE(mock->setReadBlockingFlagCalled_);
}

/**
 * @tc.name  : Test SetReadBlockingFlag API
 * @tc.number: SetReadBlockingFlag_002
 * @tc.desc  : Test SetReadBlockingFlag with null downloader returns ERROR_UNKNOWN
 */
HWTEST_F(HttpSourcePluginUnitTest, SetReadBlockingFlag_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::ERROR_UNKNOWN, plugin_->SetReadBlockingFlag(true));
}

/**
 * @tc.name  : Test SelectStream API
 * @tc.number: SelectStream_001
 * @tc.desc  : Test SelectStream with downloader forwards to mock
 */
HWTEST_F(HttpSourcePluginUnitTest, SelectStream_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->selectStreamStatus_ = Status::OK;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SelectStream(1));
    ASSERT_TRUE(mock->selectStreamCalled_);
}

/**
 * @tc.name  : Test SelectStream API
 * @tc.number: SelectStream_002
 * @tc.desc  : Test SelectStream with null downloader returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, SelectStream_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::OK, plugin_->SelectStream(1));
}

/**
 * @tc.name  : Test Pause API
 * @tc.number: Pause_001
 * @tc.desc  : Test Pause with downloader and M3U8 uri calls downloader Pause
 */
HWTEST_F(HttpSourcePluginUnitTest, Pause_001, TestSize.Level1)
{
    plugin_->protocolType_ = StreamProtocolType::HLS;
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    plugin_->uri_ = "http://test.com/opaque";
    ASSERT_EQ(Status::OK, plugin_->Pause());
    ASSERT_TRUE(mock->pauseCalled_);
}

/**
 * @tc.name  : Test Pause API
 * @tc.number: Pause_002
 * @tc.desc  : Test Pause with downloader and non-M3U8 uri does not call Pause
 */
HWTEST_F(HttpSourcePluginUnitTest, Pause_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    plugin_->uri_ = "http://test.com/test.mp4";
    ASSERT_EQ(Status::OK, plugin_->Pause());
    ASSERT_FALSE(mock->pauseCalled_);
}

/**
 * @tc.name  : Test Pause API
 * @tc.number: Pause_003
 * @tc.desc  : Test Pause with null downloader returns OK without crash
 */
HWTEST_F(HttpSourcePluginUnitTest, Pause_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    plugin_->uri_ = "http://test.com/test.m3u8";
    ASSERT_EQ(Status::OK, plugin_->Pause());
}

/**
 * @tc.name  : Test Resume API
 * @tc.number: Resume_001
 * @tc.desc  : Test Resume with downloader and M3U8 uri calls downloader Resume
 */
HWTEST_F(HttpSourcePluginUnitTest, Resume_001, TestSize.Level1)
{
    plugin_->protocolType_ = StreamProtocolType::HLS;
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    plugin_->uri_ = "http://test.com/opaque";
    ASSERT_EQ(Status::OK, plugin_->Resume());
    ASSERT_TRUE(mock->resumeCalled_);
}

/**
 * @tc.name  : Test Resume API
 * @tc.number: Resume_002
 * @tc.desc  : Test Resume with downloader and non-M3U8 uri does not call Resume
 */
HWTEST_F(HttpSourcePluginUnitTest, Resume_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    plugin_->uri_ = "http://test.com/test.mp4";
    ASSERT_EQ(Status::OK, plugin_->Resume());
    ASSERT_FALSE(mock->resumeCalled_);
}

/**
 * @tc.name  : Test Resume API
 * @tc.number: Resume_003
 * @tc.desc  : Test Resume with null downloader returns OK without crash
 */
HWTEST_F(HttpSourcePluginUnitTest, Resume_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    plugin_->uri_ = "http://test.com/test.m3u8";
    ASSERT_EQ(Status::OK, plugin_->Resume());
}

/**
 * @tc.name  : Test GetParameter API
 * @tc.number: GetParameter_001
 * @tc.desc  : Test GetParameter returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, GetParameter_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    ASSERT_EQ(Status::OK, plugin_->GetParameter(meta));
}

/**
 * @tc.name  : Test SetParameter API
 * @tc.number: SetParameter_001
 * @tc.desc  : Test SetParameter returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, SetParameter_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    ASSERT_EQ(Status::OK, plugin_->SetParameter(meta));
}

/**
 * @tc.name  : Test SetMediaDuration API
 * @tc.number: SetMediaDuration_001
 * @tc.desc  : Test SetMediaDuration with downloader and positive duration calls mock
 */
HWTEST_F(HttpSourcePluginUnitTest, SetMediaDuration_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SetMediaDuration(1000));
    ASSERT_TRUE(mock->setMediaDurationCalled_);
}

/**
 * @tc.name  : Test SetMediaDuration API
 * @tc.number: SetMediaDuration_002
 * @tc.desc  : Test SetMediaDuration with downloader and zero duration does not call mock
 */
HWTEST_F(HttpSourcePluginUnitTest, SetMediaDuration_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SetMediaDuration(0));
    ASSERT_FALSE(mock->setMediaDurationCalled_);
}

/**
 * @tc.name  : Test SetMediaDuration API
 * @tc.number: SetMediaDuration_003
 * @tc.desc  : Test SetMediaDuration with null downloader returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, SetMediaDuration_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::OK, plugin_->SetMediaDuration(1000));
}

/**
 * @tc.name  : Test SetMediaDuration API
 * @tc.number: SetMediaDuration_004
 * @tc.desc  : Test SetMediaDuration with downloader and negative duration does not call mock
 */
HWTEST_F(HttpSourcePluginUnitTest, SetMediaDuration_004, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SetMediaDuration(-1));
    ASSERT_FALSE(mock->setMediaDurationCalled_);
}

/**
 * @tc.name  : Test SetCallback API
 * @tc.number: SetCallback_001
 * @tc.desc  : Test SetCallback with downloader forwards to mock
 */
HWTEST_F(HttpSourcePluginUnitTest, SetCallback_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    auto cb = std::make_shared<SourceCallback>();
    ASSERT_EQ(Status::OK, plugin_->SetCallback(cb));
    ASSERT_TRUE(mock->setCallbackCalled_);
}

/**
 * @tc.name  : Test SetCallback API
 * @tc.number: SetCallback_002
 * @tc.desc  : Test SetCallback without downloader returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, SetCallback_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    auto cb = std::make_shared<SourceCallback>();
    ASSERT_EQ(Status::OK, plugin_->SetCallback(cb));
}

/**
 * @tc.name  : Test Read API
 * @tc.number: Read_001
 * @tc.desc  : Test Read with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, Read_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    std::shared_ptr<Buffer> buffer = nullptr;
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->Read(1, buffer, 0, 100));
}

/**
 * @tc.name  : Test Read API
 * @tc.number: Read_002
 * @tc.desc  : Test Read with null buffer creates buffer and reads
 */
HWTEST_F(HttpSourcePluginUnitTest, Read_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    std::shared_ptr<Buffer> buffer = nullptr;
    ASSERT_EQ(Status::OK, plugin_->Read(1, buffer, 0, 100));
    ASSERT_NE(nullptr, buffer);
    ASSERT_TRUE(mock->readCalled_);
}

/**
 * @tc.name  : Test Read API
 * @tc.number: Read_003
 * @tc.desc  : Test Read with empty buffer allocates memory and reads
 */
HWTEST_F(HttpSourcePluginUnitTest, Read_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    auto buffer = std::make_shared<Buffer>();
    ASSERT_EQ(Status::OK, plugin_->Read(1, buffer, 0, 100));
    ASSERT_TRUE(mock->readCalled_);
}

/**
 * @tc.name  : Test Read API
 * @tc.number: Read_004
 * @tc.desc  : Test Read with non-empty buffer uses GetMemory and reads
 */
HWTEST_F(HttpSourcePluginUnitTest, Read_004, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    auto buffer = std::make_shared<Buffer>();
    auto mem = buffer->AllocMemory(nullptr, 100);
    ASSERT_NE(nullptr, mem);
    ASSERT_EQ(Status::OK, plugin_->Read(1, buffer, 0, 100));
    ASSERT_TRUE(mock->readCalled_);
}

/**
 * @tc.name  : Test Read API (3-arg version)
 * @tc.number: Read_005
 * @tc.desc  : Test 3-arg Read with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, Read_005, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    std::shared_ptr<Buffer> buffer = nullptr;
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->Read(buffer, 0, 100));
}

/**
 * @tc.name  : Test GetSize API
 * @tc.number: GetSize_001
 * @tc.desc  : Test GetSize with downloader returns content length
 */
HWTEST_F(HttpSourcePluginUnitTest, GetSize_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->contentLength_ = 4000;
    plugin_->downloader_ = mock;
    uint64_t size = 0;
    ASSERT_EQ(Status::OK, plugin_->GetSize(size));
    ASSERT_EQ(4000u, size);
}

/**
 * @tc.name  : Test GetSize API
 * @tc.number: GetSize_002
 * @tc.desc  : Test GetSize with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, GetSize_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    uint64_t size = 0;
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->GetSize(size));
}

/**
 * @tc.name  : Test GetSeekable API
 * @tc.number: GetSeekable_001
 * @tc.desc  : Test GetSeekable with downloader returns seekable value
 */
HWTEST_F(HttpSourcePluginUnitTest, GetSeekable_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Seekable::SEEKABLE, plugin_->GetSeekable());
}

/**
 * @tc.name  : Test GetSeekable API
 * @tc.number: GetSeekable_002
 * @tc.desc  : Test GetSeekable with null downloader returns INVALID
 */
HWTEST_F(HttpSourcePluginUnitTest, GetSeekable_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Seekable::INVALID, plugin_->GetSeekable());
}

/**
 * @tc.name  : Test SetInterruptState API
 * @tc.number: SetInterruptState_001
 * @tc.desc  : Test SetInterruptState with downloader forwards to mock
 */
HWTEST_F(HttpSourcePluginUnitTest, SetInterruptState_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    plugin_->SetInterruptState(true);
    ASSERT_TRUE(mock->setInterruptStateCalled_);
    ASSERT_TRUE(plugin_->isInterruptNeeded_);
}

/**
 * @tc.name  : Test SetInterruptState API
 * @tc.number: SetInterruptState_002
 * @tc.desc  : Test SetInterruptState without downloader does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, SetInterruptState_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    plugin_->SetInterruptState(true);
    ASSERT_TRUE(plugin_->isInterruptNeeded_);
}

/**
 * @tc.name  : Test SeekTo API
 * @tc.number: SeekTo_002
 * @tc.desc  : Test SeekTo with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekTo_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->SeekTo(0));
}

/**
 * @tc.name  : Test SeekTo API
 * @tc.number: SeekTo_003
 * @tc.desc  : Test SeekTo with unseekable downloader returns ERROR_INVALID_OPERATION
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekTo_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::UNSEEKABLE;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::ERROR_INVALID_OPERATION, plugin_->SeekTo(0));
}

/**
 * @tc.name  : Test SeekTo API
 * @tc.number: SeekTo_004
 * @tc.desc  : Test SeekTo with valid offset and SeekToPos true returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekTo_004, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->contentLength_ = 4000;
    mock->seekToPosResult_ = true;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SeekTo(100));
}

/**
 * @tc.name  : Test SeekTo API
 * @tc.number: SeekTo_005
 * @tc.desc  : Test SeekTo with SeekToPos false returns ERROR_UNKNOWN
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekTo_005, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->contentLength_ = 4000;
    mock->seekToPosResult_ = false;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::ERROR_UNKNOWN, plugin_->SeekTo(100));
}

/**
 * @tc.name  : Test SeekTo API
 * @tc.number: SeekTo_006
 * @tc.desc  : Test SeekTo with offset exceeding content and error count below threshold
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekTo_006, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->contentLength_ = 100;
    plugin_->downloader_ = mock;
    plugin_->seekErrorCount_ = 2;
    plugin_->callback_.reset();
    ASSERT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SeekTo(200));
    ASSERT_EQ(3, plugin_->seekErrorCount_);
}

/**
 * @tc.name  : Test SeekTo API
 * @tc.number: SeekTo_007
 * @tc.desc  : Test SeekTo with content length zero does normal seek
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekTo_007, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->contentLength_ = 0;
    mock->seekToPosResult_ = true;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SeekTo(200));
}

/**
 * @tc.name  : Test SeekToTime API
 * @tc.number: SeekToTime_001
 * @tc.desc  : Test SeekToTime with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC));
}

/**
 * @tc.name  : Test SeekToTime API
 * @tc.number: SeekToTime_002
 * @tc.desc  : Test SeekToTime with unseekable downloader returns ERROR_INVALID_OPERATION
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::UNSEEKABLE;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::ERROR_INVALID_OPERATION, plugin_->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC));
}

/**
 * @tc.name  : Test SeekToTime API
 * @tc.number: SeekToTime_003
 * @tc.desc  : Test SeekToTime with seekTime exceeding duration returns ERROR_INVALID_PARAMETER
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->duration_ = 1000;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SeekToTime(2000, SeekMode::SEEK_NEXT_SYNC));
}

/**
 * @tc.name  : Test SeekToTime API
 * @tc.number: SeekToTime_004
 * @tc.desc  : Test SeekToTime with valid seek and SeekToTime true returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_004, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->duration_ = 1000;
    mock->seekToTimeResult_ = true;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SeekToTime(500, SeekMode::SEEK_NEXT_SYNC));
}

/**
 * @tc.name  : Test SeekToTime API
 * @tc.number: SeekToTime_005
 * @tc.desc  : Test SeekToTime with SeekToTime false returns ERROR_UNKNOWN
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_005, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->duration_ = 1000;
    mock->seekToTimeResult_ = false;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::ERROR_UNKNOWN, plugin_->SeekToTime(500, SeekMode::SEEK_NEXT_SYNC));
}

/**
 * @tc.name  : Test SeekToTime API
 * @tc.number: SeekToTime_006
 * @tc.desc  : Test SeekToTime with negative duration flips sign
 */
HWTEST_F(HttpSourcePluginUnitTest, SeekToTime_006, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->duration_ = -2000;
    mock->seekToTimeResult_ = true;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SeekToTime(500, SeekMode::SEEK_NEXT_SYNC));
}

/**
 * @tc.name  : Test MediaSeekTimeByStreamId API
 * @tc.number: MediaSeekTimeByStreamId_001
 * @tc.desc  : Test with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->MediaSeekTimeByStreamId(0, SeekMode::SEEK_NEXT_SYNC, 0));
}

/**
 * @tc.name  : Test MediaSeekTimeByStreamId API
 * @tc.number: MediaSeekTimeByStreamId_002
 * @tc.desc  : Test with unseekable downloader returns ERROR_INVALID_OPERATION
 */
HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::UNSEEKABLE;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::ERROR_INVALID_OPERATION, plugin_->MediaSeekTimeByStreamId(0, SeekMode::SEEK_NEXT_SYNC, 0));
}

/**
 * @tc.name  : Test MediaSeekTimeByStreamId API
 * @tc.number: MediaSeekTimeByStreamId_003
 * @tc.desc  : Test with seekTime exceeding duration returns ERROR_INVALID_PARAMETER
 */
HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->duration_ = 1000;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->MediaSeekTimeByStreamId(2000, SeekMode::SEEK_NEXT_SYNC, 0));
}

/**
 * @tc.name  : Test MediaSeekTimeByStreamId API
 * @tc.number: MediaSeekTimeByStreamId_004
 * @tc.desc  : Test with valid seek and result true returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_004, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->duration_ = 1000;
    mock->mediaSeekTimeByStreamIdResult_ = true;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->MediaSeekTimeByStreamId(500, SeekMode::SEEK_NEXT_SYNC, 0));
}

/**
 * @tc.name  : Test MediaSeekTimeByStreamId API
 * @tc.number: MediaSeekTimeByStreamId_005
 * @tc.desc  : Test with result false returns ERROR_UNKNOWN
 */
HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_005, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->duration_ = 1000;
    mock->mediaSeekTimeByStreamIdResult_ = false;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::ERROR_UNKNOWN, plugin_->MediaSeekTimeByStreamId(500, SeekMode::SEEK_NEXT_SYNC, 0));
}

/**
 * @tc.name  : Test MediaSeekTimeByStreamId API
 * @tc.number: MediaSeekTimeByStreamId_006
 * @tc.desc  : Test with negative duration flips sign
 */
HWTEST_F(HttpSourcePluginUnitTest, MediaSeekTimeByStreamId_006, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->seekable_ = Seekable::SEEKABLE;
    mock->duration_ = -2000;
    mock->mediaSeekTimeByStreamIdResult_ = true;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->MediaSeekTimeByStreamId(500, SeekMode::SEEK_NEXT_SYNC, 0));
}

/**
 * @tc.name  : Test GetDuration API
 * @tc.number: GetDuration_001
 * @tc.desc  : Test GetDuration with downloader returns duration
 */
HWTEST_F(HttpSourcePluginUnitTest, GetDuration_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->duration_ = 5000;
    plugin_->downloader_ = mock;
    int64_t duration = 0;
    ASSERT_EQ(Status::OK, plugin_->GetDuration(duration));
    ASSERT_EQ(5000, duration);
}

/**
 * @tc.name  : Test GetDuration API
 * @tc.number: GetDuration_002
 * @tc.desc  : Test GetDuration with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, GetDuration_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    int64_t duration = 0;
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->GetDuration(duration));
}

/**
 * @tc.name  : Test GetStartInfo API
 * @tc.number: GetStartInfo_001
 * @tc.desc  : Test GetStartInfo with downloader returns start info
 */
HWTEST_F(HttpSourcePluginUnitTest, GetStartInfo_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->startInfo_ = {100000000, true};  // nanoseconds; HttpSourcePlugin converts to milliseconds (/ MS_2_NS)
    plugin_->downloader_ = mock;
    std::pair<int64_t, bool> startInfo {0, false};
    ASSERT_EQ(Status::OK, plugin_->GetStartInfo(startInfo));
    ASSERT_EQ(100, startInfo.first);
    ASSERT_TRUE(startInfo.second);
}

/**
 * @tc.name  : Test GetStartInfo API
 * @tc.number: GetStartInfo_002
 * @tc.desc  : Test GetStartInfo with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, GetStartInfo_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    std::pair<int64_t, bool> startInfo {0, false};
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->GetStartInfo(startInfo));
}

/**
 * @tc.name  : Test GetBitRates API
 * @tc.number: GetBitRates_001
 * @tc.desc  : Test GetBitRates with downloader returns bit rates
 */
HWTEST_F(HttpSourcePluginUnitTest, GetBitRates_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    std::vector<uint32_t> bitRates;
    ASSERT_EQ(Status::OK, plugin_->GetBitRates(bitRates));
    ASSERT_EQ(3u, bitRates.size());
    ASSERT_EQ(1000u, bitRates[0]);
}

/**
 * @tc.name  : Test GetBitRates API
 * @tc.number: GetBitRates_002
 * @tc.desc  : Test GetBitRates with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, GetBitRates_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    std::vector<uint32_t> bitRates;
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->GetBitRates(bitRates));
}

/**
 * @tc.name  : Test SelectBitRate API
 * @tc.number: SelectBitRate_002
 * @tc.desc  : Test SelectBitRate with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, SelectBitRate_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->SelectBitRate(1));
}

/**
 * @tc.name  : Test SelectBitRate API
 * @tc.number: SelectBitRate_003
 * @tc.desc  : Test SelectBitRate with non-zero bitRate returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, SelectBitRate_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SelectBitRate(1));
    ASSERT_TRUE(mock->setIsTriggerAutoModeCalled_);
}

/**
 * @tc.name  : Test AutoSelectBitRate API
 * @tc.number: AutoSelectBitRate_003
 * @tc.desc  : Test AutoSelectBitRate with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, AutoSelectBitRate_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->AutoSelectBitRate(1));
}

/**
 * @tc.name  : Test GetDownloadInfo API
 * @tc.number: GetDownloadInfo_001
 * @tc.desc  : Test GetDownloadInfo with downloader forwards to mock
 */
HWTEST_F(HttpSourcePluginUnitTest, GetDownloadInfo_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    DownloadInfo downloadInfo;
    ASSERT_EQ(Status::OK, plugin_->GetDownloadInfo(downloadInfo));
    ASSERT_TRUE(mock->getDownloadInfoCalled_);
}

/**
 * @tc.name  : Test GetDownloadInfo API
 * @tc.number: GetDownloadInfo_002
 * @tc.desc  : Test GetDownloadInfo with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, GetDownloadInfo_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    DownloadInfo downloadInfo;
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->GetDownloadInfo(downloadInfo));
}

/**
 * @tc.name  : Test GetPlaybackInfo API
 * @tc.number: GetPlaybackInfo_001
 * @tc.desc  : Test GetPlaybackInfo with downloader forwards to mock
 */
HWTEST_F(HttpSourcePluginUnitTest, GetPlaybackInfo_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    PlaybackInfo playbackInfo;
    ASSERT_EQ(Status::OK, plugin_->GetPlaybackInfo(playbackInfo));
    ASSERT_TRUE(mock->getPlaybackInfoCalled_);
}

/**
 * @tc.name  : Test GetPlaybackInfo API
 * @tc.number: GetPlaybackInfo_002
 * @tc.desc  : Test GetPlaybackInfo with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, GetPlaybackInfo_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    PlaybackInfo playbackInfo;
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->GetPlaybackInfo(playbackInfo));
}

/**
 * @tc.name  : Test GetSeekableRanges API
 * @tc.number: GetSeekableRanges_001
 * @tc.desc  : Test GetSeekableRanges with downloader returns ranges
 */
HWTEST_F(HttpSourcePluginUnitTest, GetSeekableRanges_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    auto ranges = plugin_->GetSeekableRanges();
    ASSERT_TRUE(ranges.empty());
}

/**
 * @tc.name  : Test GetSeekableRanges API
 * @tc.number: GetSeekableRanges_002
 * @tc.desc  : Test GetSeekableRanges with null downloader returns empty
 */
HWTEST_F(HttpSourcePluginUnitTest, GetSeekableRanges_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    auto ranges = plugin_->GetSeekableRanges();
    ASSERT_TRUE(ranges.empty());
}

/**
 * @tc.name  : Test SetDemuxerState API
 * @tc.number: SetDemuxerState_001
 * @tc.desc  : Test SetDemuxerState with downloader forwards to mock
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDemuxerState_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    plugin_->SetDemuxerState(1);
    ASSERT_TRUE(mock->setDemuxerStateCalled_);
}

/**
 * @tc.name  : Test SetDemuxerState API
 * @tc.number: SetDemuxerState_002
 * @tc.desc  : Test SetDemuxerState with null downloader does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDemuxerState_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    plugin_->SetDemuxerState(1);
    ASSERT_FALSE(plugin_->isInterruptNeeded_);
}

/**
 * @tc.name  : Test SetCurrentBitRate API
 * @tc.number: SetCurrentBitRate_001
 * @tc.desc  : Test SetCurrentBitRate with downloader returns mock status
 */
HWTEST_F(HttpSourcePluginUnitTest, SetCurrentBitRate_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->setCurrentBitRateStatus_ = Status::OK;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SetCurrentBitRate(100, 1));
}

/**
 * @tc.name  : Test SetCurrentBitRate API
 * @tc.number: SetCurrentBitRate_002
 * @tc.desc  : Test SetCurrentBitRate with null downloader returns ERROR_INVALID_OPERATION
 */
HWTEST_F(HttpSourcePluginUnitTest, SetCurrentBitRate_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::ERROR_INVALID_OPERATION, plugin_->SetCurrentBitRate(100, 1));
}

/**
 * @tc.name  : Test SetSourceInitialBufferSize API
 * @tc.number: SetSourceInitialBufferSize_001
 * @tc.desc  : Test SetSourceInitialBufferSize with downloader returns mock result
 */
HWTEST_F(HttpSourcePluginUnitTest, SetSourceInitialBufferSize_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->setInitialBufferSizeResult_ = true;
    plugin_->downloader_ = mock;
    ASSERT_TRUE(plugin_->SetSourceInitialBufferSize(0, 100));
}

/**
 * @tc.name  : Test SetSourceInitialBufferSize API
 * @tc.number: SetSourceInitialBufferSize_002
 * @tc.desc  : Test SetSourceInitialBufferSize with null downloader returns false
 */
HWTEST_F(HttpSourcePluginUnitTest, SetSourceInitialBufferSize_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_FALSE(plugin_->SetSourceInitialBufferSize(0, 100));
}

/**
 * @tc.name  : Test StopBufferring API
 * @tc.number: StopBufferring_001
 * @tc.desc  : Test StopBufferring with downloader returns mock status
 */
HWTEST_F(HttpSourcePluginUnitTest, StopBufferring_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->stopBufferringStatus_ = Status::OK;
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->StopBufferring(true));
}

/**
 * @tc.name  : Test StopBufferring API
 * @tc.number: StopBufferring_002
 * @tc.desc  : Test StopBufferring with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, StopBufferring_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->StopBufferring(true));
}

/**
 * @tc.name  : Test WaitForBufferingEnd API
 * @tc.number: WaitForBufferingEnd_001
 * @tc.desc  : Test WaitForBufferingEnd with downloader forwards to mock
 */
HWTEST_F(HttpSourcePluginUnitTest, WaitForBufferingEnd_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    plugin_->WaitForBufferingEnd();
    ASSERT_TRUE(mock->waitForBufferingEndCalled_);
}

/**
 * @tc.name  : Test WaitForBufferingEnd API
 * @tc.number: WaitForBufferingEnd_002
 * @tc.desc  : Test WaitForBufferingEnd with null downloader does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, WaitForBufferingEnd_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    plugin_->WaitForBufferingEnd();
    ASSERT_FALSE(plugin_->isInterruptNeeded_);
}

/**
 * @tc.name  : Test NotifyInitSuccess API
 * @tc.number: NotifyInitSuccess_001
 * @tc.desc  : Test NotifyInitSuccess with downloader forwards to mock
 */
HWTEST_F(HttpSourcePluginUnitTest, NotifyInitSuccess_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    plugin_->NotifyInitSuccess();
    ASSERT_TRUE(mock->notifyInitSuccessCalled_);
}

/**
 * @tc.name  : Test NotifyInitSuccess API
 * @tc.number: NotifyInitSuccess_002
 * @tc.desc  : Test NotifyInitSuccess with null downloader does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, NotifyInitSuccess_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    plugin_->NotifyInitSuccess();
    ASSERT_FALSE(plugin_->isInterruptNeeded_);
}

/**
 * @tc.name  : Test SetStartPts API
 * @tc.number: SetStartPts_001
 * @tc.desc  : Test SetStartPts with downloader returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, SetStartPts_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SetStartPts(100));
    ASSERT_TRUE(mock->setStartPtsCalled_);
}

/**
 * @tc.name  : Test SetStartPts API
 * @tc.number: SetStartPts_002
 * @tc.desc  : Test SetStartPts with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, SetStartPts_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->SetStartPts(100));
}

/**
 * @tc.name  : Test SetExtraCache API
 * @tc.number: SetExtraCache_001
 * @tc.desc  : Test SetExtraCache with downloader returns OK
 */
HWTEST_F(HttpSourcePluginUnitTest, SetExtraCache_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    ASSERT_EQ(Status::OK, plugin_->SetExtraCache(1000));
    ASSERT_TRUE(mock->setExtraCacheCalled_);
}

/**
 * @tc.name  : Test SetExtraCache API
 * @tc.number: SetExtraCache_002
 * @tc.desc  : Test SetExtraCache with null downloader returns ERROR_NULL_POINTER
 */
HWTEST_F(HttpSourcePluginUnitTest, SetExtraCache_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(Status::ERROR_NULL_POINTER, plugin_->SetExtraCache(1000));
}

/**
 * @tc.name  : Test GetCachedDuration API
 * @tc.number: GetCachedDuration_001
 * @tc.desc  : Test GetCachedDuration with downloader returns cached duration
 */
HWTEST_F(HttpSourcePluginUnitTest, GetCachedDuration_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->cachedDuration_ = 5000;
    plugin_->downloader_ = mock;
    ASSERT_EQ(5000u, plugin_->GetCachedDuration());
}

/**
 * @tc.name  : Test GetCachedDuration API
 * @tc.number: GetCachedDuration_002
 * @tc.desc  : Test GetCachedDuration with null downloader returns 0
 */
HWTEST_F(HttpSourcePluginUnitTest, GetCachedDuration_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(0u, plugin_->GetCachedDuration());
}

/**
 * @tc.name  : Test RestartAndClearBuffer API
 * @tc.number: RestartAndClearBuffer_001
 * @tc.desc  : Test RestartAndClearBuffer with downloader forwards to mock
 */
HWTEST_F(HttpSourcePluginUnitTest, RestartAndClearBuffer_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    plugin_->RestartAndClearBuffer();
    ASSERT_TRUE(mock->restartAndClearBufferCalled_);
}

/**
 * @tc.name  : Test RestartAndClearBuffer API
 * @tc.number: RestartAndClearBuffer_002
 * @tc.desc  : Test RestartAndClearBuffer with null downloader does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, RestartAndClearBuffer_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    plugin_->RestartAndClearBuffer();
    ASSERT_FALSE(plugin_->isInterruptNeeded_);
}

/**
 * @tc.name  : Test IsFlvLive API
 * @tc.number: IsFlvLive_001
 * @tc.desc  : Test IsFlvLive with downloader returns mock value
 */
HWTEST_F(HttpSourcePluginUnitTest, IsFlvLive_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->isFlvLive_ = true;
    plugin_->downloader_ = mock;
    ASSERT_TRUE(plugin_->IsFlvLive());
}

/**
 * @tc.name  : Test IsFlvLive API
 * @tc.number: IsFlvLive_002
 * @tc.desc  : Test IsFlvLive with null downloader returns false
 */
HWTEST_F(HttpSourcePluginUnitTest, IsFlvLive_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_FALSE(plugin_->IsFlvLive());
}

/**
 * @tc.name  : Test IsHlsFmp4 API
 * @tc.number: IsHlsFmp4_001
 * @tc.desc  : Test IsHlsFmp4 with downloader returns mock value
 */
HWTEST_F(HttpSourcePluginUnitTest, IsHlsFmp4_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->isHlsFmp4_ = true;
    plugin_->downloader_ = mock;
    ASSERT_TRUE(plugin_->IsHlsFmp4());
}

/**
 * @tc.name  : Test IsHlsFmp4 API
 * @tc.number: IsHlsFmp4_002
 * @tc.desc  : Test IsHlsFmp4 with null downloader returns false
 */
HWTEST_F(HttpSourcePluginUnitTest, IsHlsFmp4_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_FALSE(plugin_->IsHlsFmp4());
}

/**
 * @tc.name  : Test GetMemorySize API
 * @tc.number: GetMemorySize_001
 * @tc.desc  : Test GetMemorySize with downloader returns mock value
 */
HWTEST_F(HttpSourcePluginUnitTest, GetMemorySize_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->memorySize_ = 1024;
    plugin_->downloader_ = mock;
    ASSERT_EQ(1024u, plugin_->GetMemorySize());
}

/**
 * @tc.name  : Test GetMemorySize API
 * @tc.number: GetMemorySize_002
 * @tc.desc  : Test GetMemorySize with null downloader returns 0
 */
HWTEST_F(HttpSourcePluginUnitTest, GetMemorySize_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ(0u, plugin_->GetMemorySize());
}

/**
 * @tc.name  : Test GetCurUrl API
 * @tc.number: GetCurUrl_001
 * @tc.desc  : Test GetCurUrl with downloader returns mock url
 */
HWTEST_F(HttpSourcePluginUnitTest, GetCurUrl_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->curUrl_ = "http://test.com/video.mp4";
    plugin_->downloader_ = mock;
    ASSERT_EQ("http://test.com/video.mp4", plugin_->GetCurUrl());
}

/**
 * @tc.name  : Test GetCurUrl API
 * @tc.number: GetCurUrl_002
 * @tc.desc  : Test GetCurUrl with null downloader returns empty string
 */
HWTEST_F(HttpSourcePluginUnitTest, GetCurUrl_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ("", plugin_->GetCurUrl());
}

/**
 * @tc.name  : Test IsHlsEnd API
 * @tc.number: IsHlsEnd_001
 * @tc.desc  : Test IsHlsEnd with downloader returns mock value
 */
HWTEST_F(HttpSourcePluginUnitTest, IsHlsEnd_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->isHlsEnd_ = true;
    plugin_->downloader_ = mock;
    ASSERT_TRUE(plugin_->IsHlsEnd(1));
}

/**
 * @tc.name  : Test IsHlsEnd API
 * @tc.number: IsHlsEnd_002
 * @tc.desc  : Test IsHlsEnd with null downloader returns false
 */
HWTEST_F(HttpSourcePluginUnitTest, IsHlsEnd_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_FALSE(plugin_->IsHlsEnd(1));
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_001
 * @tc.desc  : Test GetMaskUrlByReadIndex with downloader returns mock url
 */
HWTEST_F(HttpSourcePluginUnitTest, GetMaskUrlByReadIndex_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->maskUrl_ = "http://test.com/mask.mp4";
    plugin_->downloader_ = mock;
    ASSERT_EQ("http://test.com/mask.mp4", plugin_->GetMaskUrlByReadIndex(1));
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_002
 * @tc.desc  : Test GetMaskUrlByReadIndex with null downloader returns empty
 */
HWTEST_F(HttpSourcePluginUnitTest, GetMaskUrlByReadIndex_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    ASSERT_EQ("", plugin_->GetMaskUrlByReadIndex(1));
}

/**
 * @tc.name  : Test IsHls API
 * @tc.number: IsHls_001
 * @tc.desc  : Test IsHls with M3U8 mimeType returns true
 */
HWTEST_F(HttpSourcePluginUnitTest, IsHls_001, TestSize.Level1)
{
    plugin_->protocolType_ = StreamProtocolType::HLS;
    ASSERT_NE(nullptr, plugin_);
    plugin_->mimeType_ = AVMimeTypes::APPLICATION_M3U8;
    ASSERT_TRUE(plugin_->IsHls());
}

/**
 * @tc.name  : Test IsHls API
 * @tc.number: IsHls_002
 * @tc.desc  : Test IsHls with M3U8 uri returns true
 */
HWTEST_F(HttpSourcePluginUnitTest, IsHls_002, TestSize.Level1)
{
    plugin_->protocolType_ = StreamProtocolType::HLS;
    ASSERT_NE(nullptr, plugin_);
    plugin_->mimeType_ = "";
    plugin_->uri_ = "http://test.com/video.m3u8";
    ASSERT_TRUE(plugin_->IsHls());
}

/**
 * @tc.name  : Test IsHls API
 * @tc.number: IsHls_003
 * @tc.desc  : Test IsHls with non-HLS uri returns false
 */
HWTEST_F(HttpSourcePluginUnitTest, IsHls_003, TestSize.Level1)
{
    plugin_->protocolType_ = StreamProtocolType::HTTP;
    ASSERT_NE(nullptr, plugin_);
    plugin_->mimeType_ = "";
    plugin_->uri_ = "http://test.com/video.mp4";
    ASSERT_FALSE(plugin_->IsHls());
}

/**
 * @tc.name  : Test IsAllowedProtocol API
 * @tc.number: IsAllowedProtocol_001
 * @tc.desc  : Test IsAllowedProtocol with http url returns true
 */
HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    ASSERT_TRUE(plugin_->IsAllowedProtocol("http://test.com/video.mp4"));
}

/**
 * @tc.name  : Test IsAllowedProtocol API
 * @tc.number: IsAllowedProtocol_002
 * @tc.desc  : Test IsAllowedProtocol with file:// url returns false
 */
HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    ASSERT_FALSE(plugin_->IsAllowedProtocol("file://test.com/video.mp4"));
}

/**
 * @tc.name  : Test IsAllowedProtocol API
 * @tc.number: IsAllowedProtocol_003
 * @tc.desc  : Test IsAllowedProtocol with short url returns true
 */
HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    ASSERT_TRUE(plugin_->IsAllowedProtocol("f"));
}

/**
 * @tc.name  : Test IsAllowedProtocol API
 * @tc.number: IsAllowedProtocol_004
 * @tc.desc  : Test IsAllowedProtocol with case-insensitive FILE:// returns false
 */
HWTEST_F(HttpSourcePluginUnitTest, IsAllowedProtocol_004, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    ASSERT_FALSE(plugin_->IsAllowedProtocol("FILE://test.com/video.mp4"));
}

/**
 * @tc.name  : Test IsSeekToTimeSupported API
 * @tc.number: IsSeekToTimeSupported_002
 * @tc.desc  : Test IsSeekToTimeSupported with DASH uri returns true
 */
HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported_002, TestSize.Level1)
{
    plugin_->protocolType_ = StreamProtocolType::DASH;
    ASSERT_NE(nullptr, plugin_);
    plugin_->mimeType_ = "";
    plugin_->uri_ = "http://test.com/video.mpd";
    ASSERT_TRUE(plugin_->IsSeekToTimeSupported());
}

/**
 * @tc.name  : Test IsSeekToTimeSupported API
 * @tc.number: IsSeekToTimeSupported_003
 * @tc.desc  : Test IsSeekToTimeSupported with M3U8 uri returns true
 */
HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported_003, TestSize.Level1)
{
    plugin_->protocolType_ = StreamProtocolType::HLS;
    ASSERT_NE(nullptr, plugin_);
    plugin_->mimeType_ = "";
    plugin_->uri_ = "http://test.com/video.m3u8";
    ASSERT_TRUE(plugin_->IsSeekToTimeSupported());
}

/**
 * @tc.name  : Test IsSeekToTimeSupported API
 * @tc.number: IsSeekToTimeSupported_004
 * @tc.desc  : Test IsSeekToTimeSupported with non-HLS/DASH uri returns false
 */
HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported_004, TestSize.Level1)
{
    plugin_->protocolType_ = StreamProtocolType::HTTP;
    ASSERT_NE(nullptr, plugin_);
    plugin_->mimeType_ = "";
    plugin_->uri_ = "http://test.com/video.mp4";
    ASSERT_FALSE(plugin_->IsSeekToTimeSupported());
}

/**
 * @tc.name  : Test SetSource API
 * @tc.number: SetSource_001
 * @tc.desc  : Test SetSource with already set downloader returns ERROR_INVALID_OPERATION
 */
HWTEST_F(HttpSourcePluginUnitTest, SetSource_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_ = std::make_shared<MockMediaDownloader>();
    std::string uri {"http://test.com/video.mp4"};
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    ASSERT_EQ(Status::ERROR_INVALID_OPERATION, plugin_->SetSource(source));
}

/**
 * @tc.name  : Test SetSource API
 * @tc.number: SetSource_002
 * @tc.desc  : Test SetSource with null source returns ERROR_INVALID_OPERATION
 */
HWTEST_F(HttpSourcePluginUnitTest, SetSource_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    ASSERT_EQ(Status::ERROR_INVALID_OPERATION, plugin_->SetSource(nullptr));
}

/**
 * @tc.name  : Test SetSource API
 * @tc.number: SetSource_003
 * @tc.desc  : Test SetSource with disallowed protocol returns ERROR_INVALID_OPERATION
 */
HWTEST_F(HttpSourcePluginUnitTest, SetSource_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    std::string uri {"file://test.com/video.mp4"};
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    ASSERT_EQ(Status::ERROR_INVALID_OPERATION, plugin_->SetSource(source));
}

/**
 * @tc.name  : Test UserDefinedDuration via CreateHlsDownloader
 * @tc.number: CreateHlsDownloader_002
 * @tc.desc  : Test CreateHlsDownloader with strategy and positive duration
 */
HWTEST_F(HttpSourcePluginUnitTest, CreateHlsDownloader_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->playStrategy_ = std::make_shared<PlayStrategy>();
    plugin_->playStrategy_->duration = 50;
    plugin_->loaderCombinations_ = nullptr;
    auto downloader = plugin_->CreateHlsDownloader();
    ASSERT_NE(nullptr, downloader);
    ASSERT_TRUE(plugin_->delayReady_);
}

/**
 * @tc.name  : Test SetDownloaderBySource with HTTP uri
 * @tc.number: SetDownloaderBySource_002
 * @tc.desc  : Test SetDownloaderBySource with HTTP uri creates http downloader
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_002, TestSize.Level1)
{
    MediaAVCodec::HttpServerDemo server;
    server.StartServer(ROUTING_TEST_PORT);
    MediaAVCodec::HttpServerDemo::RegisterContent("/probe", std::string(512, 'x'));
    ASSERT_NE(nullptr, plugin_);
    std::string uri = "http://127.0.0.1:" + std::to_string(ROUTING_TEST_PORT) + "/probe";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    plugin_->SetDownloaderBySource(source);
    ASSERT_NE(nullptr, plugin_->downloader_);
    ASSERT_EQ(uri, plugin_->uri_);
    MediaAVCodec::HttpServerDemo::ClearContent();
}

/**
 * @tc.name  : Test SetDownloaderBySource with DASH uri
 * @tc.number: SetDownloaderBySource_003
 * @tc.desc  : Test SetDownloaderBySource with DASH uri creates dash downloader
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_003, TestSize.Level1)
{
    MediaAVCodec::HttpServerDemo server;
    server.StartServer(ROUTING_TEST_PORT);
    MediaAVCodec::HttpServerDemo::RegisterContent("/probe",
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\" mediaPresentationDuration=\"PT6S\">"
        "<Period><AdaptationSet mimeType=\"video/mp4\"><Representation id=\"v\" bandwidth=\"1000\">"
        "<BaseURL>media.mp4</BaseURL></Representation></AdaptationSet></Period></MPD>");
    ASSERT_NE(nullptr, plugin_);
    std::string uri = "http://127.0.0.1:" + std::to_string(ROUTING_TEST_PORT) + "/probe";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    ASSERT_EQ(plugin_->SetSource(source), Status::OK);
    ASSERT_NE(nullptr, plugin_->downloader_);
    ASSERT_FALSE(plugin_->delayReady_);
    EXPECT_EQ(plugin_->protocolType_.load(), StreamProtocolType::DASH);
    int64_t duration = 0;
    constexpr int64_t EXPECTED_DURATION = 6000000000; // Six seconds in the downloader's nanosecond time base.
    bool parsed = SnifferTest::WaitFor([&] {
        return plugin_->GetDuration(duration) == Status::OK && duration == EXPECTED_DURATION;
    });
    plugin_->CloseUri(true);
    MediaAVCodec::HttpServerDemo::ClearContent();
    EXPECT_TRUE(parsed);
}

/**
 * @tc.name  : Test SetDownloaderBySource with HLS uri
 * @tc.number: SetDownloaderBySource_004
 * @tc.desc  : Test SetDownloaderBySource with HLS uri creates hls downloader
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_004, TestSize.Level1)
{
    MediaAVCodec::HttpServerDemo server;
    server.StartServer(ROUTING_TEST_PORT);
    MediaAVCodec::HttpServerDemo::RegisterContent("/probe",
        "#EXTM3U\n#EXT-X-TARGETDURATION:6\n#EXTINF:6,\nseg.ts\n#EXT-X-ENDLIST\n");
    MediaAVCodec::HttpServerDemo::RegisterContent("/seg.ts", std::string(512, 's')); // Small opaque segment.
    ASSERT_NE(nullptr, plugin_);
    std::string uri = "http://127.0.0.1:" + std::to_string(ROUTING_TEST_PORT) + "/probe";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    ASSERT_EQ(plugin_->SetSource(source), Status::OK);
    ASSERT_NE(nullptr, plugin_->downloader_);
    ASSERT_FALSE(plugin_->delayReady_);
    EXPECT_TRUE(plugin_->IsHls());
    int64_t duration = 0;
    constexpr int64_t EXPECTED_DURATION = 6000000000; // Six seconds in nanoseconds.
    bool parsed = SnifferTest::WaitFor([&] {
        return plugin_->GetDuration(duration) == Status::OK && duration == EXPECTED_DURATION;
    });
    plugin_->CloseUri(true);
    MediaAVCodec::HttpServerDemo::ClearContent();
    EXPECT_TRUE(parsed);
}

/**
 * @tc.name  : Test SetDownloaderBySource with non-http uri
 * @tc.number: SetDownloaderBySource_005
 * @tc.desc  : Test SetDownloaderBySource with non-http uri does not set downloader
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_005, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    std::string uri {"mock_uri"};
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    plugin_->SetDownloaderBySource(source);
    ASSERT_EQ(nullptr, plugin_->downloader_);
    ASSERT_TRUE(plugin_->delayReady_);
}

/**
 * @tc.name  : Test SetDownloaderBySource with playStrategy
 * @tc.number: SetDownloaderBySource_006
 * @tc.desc  : Test SetDownloaderBySource with M3U8 and playStrategy set calls SetPlayStrategy
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_006, TestSize.Level1)
{
    MediaAVCodec::HttpServerDemo server;
    server.StartServer(ROUTING_TEST_PORT);
    MediaAVCodec::HttpServerDemo::RegisterContent("/probe",
        "#EXTM3U\n#EXT-X-TARGETDURATION:6\n#EXTINF:6,\nseg.ts\n#EXT-X-ENDLIST\n");
    ASSERT_NE(nullptr, plugin_);
    std::string uri = "http://127.0.0.1:" + std::to_string(ROUTING_TEST_PORT) + "/probe";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    auto strategy = std::make_shared<PlayStrategy>();
    source->SetPlayStrategy(strategy);
    plugin_->SetDownloaderBySource(source);
    ASSERT_NE(nullptr, plugin_->downloader_);
    ASSERT_NE(nullptr, plugin_->playStrategy_);
    ASSERT_EQ(uri, plugin_->uri_);
    MediaAVCodec::HttpServerDemo::ClearContent();
}

/**
 * @tc.name  : Test InitSourceLoader with Cookie header
 * @tc.number: InitSourceLoader_003
 * @tc.desc  : Test InitSourceLoader with Cookie and offline cache enabled
 */
HWTEST_F(HttpSourcePluginUnitTest, InitSourceLoader_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    std::string uri {"mock"};
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    source->sourceLoader_ = std::make_shared<MockSourceLoader>();
    plugin_->httpHeader_["Cookie"] = "session=abc";
    plugin_->InitSourceLoader(source);
    ASSERT_NE(nullptr, plugin_->loaderCombinations_);
    ASSERT_EQ(1u, plugin_->httpHeader_.size());
}

/**
 * @tc.name  : Test InitSourceLoader with sourceLoader and no cookie
 * @tc.number: InitSourceLoader_004
 * @tc.desc  : Test InitSourceLoader with sourceLoader and no cookie, offline cache disabled
 */
HWTEST_F(HttpSourcePluginUnitTest, InitSourceLoader_004, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    std::string uri {"mock"};
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    source->sourceLoader_ = std::make_shared<MockSourceLoader>();
    plugin_->InitSourceLoader(source);
    ASSERT_NE(nullptr, plugin_->loaderCombinations_);
    ASSERT_TRUE(plugin_->httpHeader_.empty());
}

/**
 * @tc.name  : Test InitSourcePlugin with a disallowed protocol
 * @tc.number: InitSourcePlugin_001
 * @tc.desc  : Reject file:// before probing without creating a downloader or a pending network failure.
 */
HWTEST_F(HttpSourcePluginUnitTest, InitSourcePlugin_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    const std::string uri = "file:///unused-for-protocol-rejection.m3u8";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    auto status = plugin_->InitSourcePlugin(source);
    EXPECT_EQ(Status::ERROR_INVALID_OPERATION, status);
    EXPECT_EQ(plugin_->GetDownloader(), nullptr);
    EXPECT_EQ(plugin_->GetActiveSniffer(), nullptr);
    EXPECT_EQ(plugin_->GetProbeFailure(), nullptr);
}

/**
 * @tc.name  : Test PlayStrategyInit with redirectUrl set
 * @tc.number: PlayStrategyInit_001
 * @tc.desc  : Test PlayStrategyInit uses redirectUrl when set
 */
HWTEST_F(HttpSourcePluginUnitTest, PlayStrategyInit_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->redirectUrl_ = "http://test.com/redirect.mp4";
    std::string uri {"http://test.com/original.mp4"};
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    auto strategy = plugin_->PlayStrategyInit(source);
    ASSERT_EQ("http://test.com/redirect.mp4", plugin_->uri_);
}

/**
 * @tc.name  : Test CloseUri sync with downloader
 * @tc.number: CloseUri_001
 * @tc.desc  : Test CloseUri sync with downloader sets downloader to null
 */
HWTEST_F(HttpSourcePluginUnitTest, CloseUri_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    plugin_->uri_ = "http://test.com/video.mp4";
    plugin_->CloseUri(false);
    ASSERT_TRUE(mock->closeCalled_);
    ASSERT_EQ(nullptr, plugin_->downloader_);
    ASSERT_EQ("", plugin_->uri_);
}

/**
 * @tc.name  : Test CloseUri async with downloader
 * @tc.number: CloseUri_002
 * @tc.desc  : Test CloseUri async with downloader does not null downloader
 */
HWTEST_F(HttpSourcePluginUnitTest, CloseUri_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    plugin_->downloader_ = mock;
    plugin_->uri_ = "http://test.com/video.mp4";
    plugin_->CloseUri(true);
    ASSERT_TRUE(mock->closeCalled_);
    ASSERT_NE(nullptr, plugin_->downloader_);
    ASSERT_EQ("", plugin_->uri_);
}

/**
 * @tc.name  : Test CloseUri without downloader
 * @tc.number: CloseUri_003
 * @tc.desc  : Test CloseUri without downloader does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, CloseUri_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->downloader_.reset();
    plugin_->uri_ = "http://test.com/video.mp4";
    plugin_->CloseUri(false);
    ASSERT_EQ(nullptr, plugin_->downloader_);
    ASSERT_EQ("", plugin_->uri_);
}

/**
 * @tc.name  : Test SetDownloadErrorState API
 * @tc.number: SetDownloadErrorState_001
 * @tc.desc  : Test SetDownloadErrorState is a no-op
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloadErrorState_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->SetDownloadErrorState();
    ASSERT_FALSE(plugin_->isInterruptNeeded_);
}

/**
 * @tc.name  : Test Destructor with null reportInfo
 * @tc.number: Destructor_001
 * @tc.desc  : Test destructor with null reportInfo does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, Destructor_001, TestSize.Level1)
{
    auto plugin = std::make_shared<HttpSourcePlugin>("mock");
    ASSERT_NE(nullptr, plugin);
    plugin->reportInfo_.reset();
    plugin->downloader_ = std::make_shared<MockMediaDownloader>();
    ASSERT_TRUE(plugin->delayReady_);
    plugin.reset();
    ASSERT_EQ(nullptr, plugin);
}

/**
 * @tc.name  : Test SetCurrentBitRate with null reportInfo
 * @tc.number: SetCurrentBitRate_003
 * @tc.desc  : Test SetCurrentBitRate with null reportInfo does not crash
 */
HWTEST_F(HttpSourcePluginUnitTest, SetCurrentBitRate_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    auto mock = std::make_shared<MockMediaDownloader>();
    mock->setCurrentBitRateStatus_ = Status::OK;
    plugin_->downloader_ = mock;
    plugin_->reportInfo_.reset();
    ASSERT_EQ(Status::OK, plugin_->SetCurrentBitRate(200, 1));
}

/**
 * @tc.name  : Test SetDownloaderBySource with null source
 * @tc.number: SetDownloaderBySource_007
 * @tc.desc  : Test SetDownloaderBySource with null source returns early
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_007, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    plugin_->SetDownloaderBySource(nullptr);
    ASSERT_EQ(nullptr, plugin_->downloader_);
    ASSERT_TRUE(plugin_->uri_.empty());
}

/**
 * @tc.name  : Test SetDownloaderBySource HTTP with callback
 * @tc.number: SetDownloaderBySource_008
 * @tc.desc  : Test SetDownloaderBySource with HTTP uri and callback set
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_008, TestSize.Level1)
{
    MediaAVCodec::HttpServerDemo server;
    server.StartServer(ROUTING_TEST_PORT);
    MediaAVCodec::HttpServerDemo::RegisterContent("/probe", std::string(ROUTING_BODY_SIZE, 'x'));
    ASSERT_NE(nullptr, plugin_);
    auto cb = std::make_shared<SourceCallback>();
    plugin_->callback_ = cb;
    std::string uri = ROUTING_BASE + "/probe";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    plugin_->SetDownloaderBySource(source);
    ASSERT_NE(nullptr, plugin_->downloader_);
    EXPECT_EQ(uri, plugin_->uri_);
    auto monitor = std::static_pointer_cast<DownloadMonitor>(plugin_->GetDownloader());
    EXPECT_EQ(monitor->callback_.lock(), cb);
    auto requests = MediaAVCodec::HttpServerDemo::GetRequests("/probe");
    MediaAVCodec::HttpServerDemo::ClearContent();
    EXPECT_EQ(requests.size(), 1u);
}

/**
 * @tc.name  : Test SetDownloaderBySource HTTP with play strategy
 * @tc.number: SetDownloaderBySource_009
 * @tc.desc  : Test SetDownloaderBySource with HTTP uri and play strategy with duration
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_009, TestSize.Level1)
{
    MediaAVCodec::HttpServerDemo server;
    server.StartServer(ROUTING_TEST_PORT);
    MediaAVCodec::HttpServerDemo::RegisterContent("/probe", std::string(ROUTING_BODY_SIZE, 'x'));
    ASSERT_NE(nullptr, plugin_);
    std::string uri = ROUTING_BASE + "/probe";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    auto strategy = std::make_shared<PlayStrategy>();
    strategy->duration = 50;
    source->SetPlayStrategy(strategy);
    plugin_->SetDownloaderBySource(source);
    ASSERT_NE(nullptr, plugin_->downloader_);
    ASSERT_NE(nullptr, plugin_->playStrategy_);
    ASSERT_EQ(50u, plugin_->playStrategy_->duration);
    auto requests = MediaAVCodec::HttpServerDemo::GetRequests("/probe");
    MediaAVCodec::HttpServerDemo::ClearContent();
    EXPECT_EQ(requests.size(), 1u);
}

/**
 * @tc.name  : Test SetDownloaderBySource HTTP with media streams
 * @tc.number: SetDownloaderBySource_010
 * @tc.desc  : Test SetDownloaderBySource with HTTP uri and media streams
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_010, TestSize.Level1)
{
    MediaAVCodec::HttpServerDemo server;
    server.StartServer(ROUTING_TEST_PORT);
    MediaAVCodec::HttpServerDemo::RegisterContent("/stream1.mp4", std::string(ROUTING_BODY_SIZE, 'x'));
    MediaAVCodec::HttpServerDemo::RegisterContent("/stream2.mp4", std::string(ROUTING_BODY_SIZE, 'x'));
    ASSERT_NE(nullptr, plugin_);
    std::string uri = ROUTING_BASE + "/probe";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    auto stream1 = std::make_shared<PlayMediaStream>();
    stream1->bitrate = 2000;
    stream1->url = ROUTING_BASE + "/stream1.mp4";
    auto stream2 = std::make_shared<PlayMediaStream>();
    stream2->bitrate = 1000;
    stream2->url = ROUTING_BASE + "/stream2.mp4";
    source->AddMediaStream(stream1);
    source->AddMediaStream(stream2);
    plugin_->SetDownloaderBySource(source);
    ASSERT_NE(nullptr, plugin_->downloader_);
    EXPECT_EQ(stream2->url, plugin_->uri_);
    EXPECT_TRUE(MediaAVCodec::HttpServerDemo::GetRequests("/stream1.mp4").empty());
    EXPECT_TRUE(MediaAVCodec::HttpServerDemo::GetRequests("/probe").empty());
    auto requests = MediaAVCodec::HttpServerDemo::GetRequests("/stream2.mp4");
    MediaAVCodec::HttpServerDemo::ClearContent();
    EXPECT_EQ(requests.size(), 1u);
}

/**
 * @tc.name  : Test SetDownloaderBySource HTTP with callback and strategy
 * @tc.number: SetDownloaderBySource_011
 * @tc.desc  : Test SetDownloaderBySource with HTTP uri, callback, and strategy
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_011, TestSize.Level1)
{
    MediaAVCodec::HttpServerDemo server;
    server.StartServer(ROUTING_TEST_PORT);
    MediaAVCodec::HttpServerDemo::RegisterContent("/probe", std::string(ROUTING_BODY_SIZE, 'x'));
    ASSERT_NE(nullptr, plugin_);
    auto cb = std::make_shared<SourceCallback>();
    plugin_->callback_ = cb;
    std::string uri = ROUTING_BASE + "/probe";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    auto strategy = std::make_shared<PlayStrategy>();
    strategy->duration = 30;
    source->SetPlayStrategy(strategy);
    plugin_->SetDownloaderBySource(source);
    ASSERT_NE(nullptr, plugin_->downloader_);
    ASSERT_NE(nullptr, plugin_->playStrategy_);
    ASSERT_EQ(30u, plugin_->playStrategy_->duration);
    auto monitor = std::static_pointer_cast<DownloadMonitor>(plugin_->GetDownloader());
    EXPECT_EQ(monitor->callback_.lock(), cb);
    auto requests = MediaAVCodec::HttpServerDemo::GetRequests("/probe");
    MediaAVCodec::HttpServerDemo::ClearContent();
    EXPECT_EQ(requests.size(), 1u);
}

/**
 * @tc.name  : Test SetDownloaderBySource with media streams same bitrate
 * @tc.number: SetDownloaderBySource_012
 * @tc.desc  : Test sort lambda with same bitrate but different resolution
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_012, TestSize.Level1)
{
    MediaAVCodec::HttpServerDemo server;
    server.StartServer(ROUTING_TEST_PORT);
    MediaAVCodec::HttpServerDemo::RegisterContent("/stream1.mp4", std::string(ROUTING_BODY_SIZE, 'x'));
    MediaAVCodec::HttpServerDemo::RegisterContent("/stream2.mp4", std::string(ROUTING_BODY_SIZE, 'x'));
    ASSERT_NE(nullptr, plugin_);
    std::string uri = ROUTING_BASE + "/probe";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    auto stream1 = std::make_shared<PlayMediaStream>();
    stream1->bitrate = 1000;
    stream1->width = 1920;
    stream1->height = 1080;
    stream1->url = ROUTING_BASE + "/stream1.mp4";
    auto stream2 = std::make_shared<PlayMediaStream>();
    stream2->bitrate = 1000;
    stream2->width = 1280;
    stream2->height = 720;
    stream2->url = ROUTING_BASE + "/stream2.mp4";
    source->AddMediaStream(stream1);
    source->AddMediaStream(stream2);
    plugin_->SetDownloaderBySource(source);
    ASSERT_NE(nullptr, plugin_->downloader_);
    EXPECT_EQ(stream2->url, plugin_->uri_);
    EXPECT_TRUE(MediaAVCodec::HttpServerDemo::GetRequests("/stream1.mp4").empty());
    EXPECT_TRUE(MediaAVCodec::HttpServerDemo::GetRequests("/probe").empty());
    auto requests = MediaAVCodec::HttpServerDemo::GetRequests("/stream2.mp4");
    MediaAVCodec::HttpServerDemo::ClearContent();
    EXPECT_EQ(requests.size(), 1u);
}
}
}
}
}
