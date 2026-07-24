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

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;

class MockSourceLoader : public Plugins::IMediaSourceLoader {
public:
    int32_t Init(std::shared_ptr<Plugins::IMediaSourceLoadingRequest> &request) override { return 0; }
    int64_t Open(const string &url, const map<string, string> &header) override { return 0; }
    int32_t Read(int64_t uuid, int64_t requestedOffset, int64_t requestedLength) override { return 0; }
    int32_t Close(int64_t uuid) override { return 0; }
};

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
class MockMediaDownloader : public MediaDownloader {
public:
    MockMediaDownloader() = default;
    virtual ~MockMediaDownloader() = default;
    bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override
    {
        (void)url;
        (void)httpHeader;
        return true;
    }
    void Init() override
    {
        return;
    }
    void Close(bool isAsync) override
    {
        (void)isAsync;
        return;
    }
    void Pause() override
    {
        return;
    }
    void Resume() override
    {
        return;
    }
    Status Read(unsigned char* buff, ReadDataInfo& readDataInfo) override
    {
        (void)buff;
        (void)readDataInfo;
        return Status::OK;
    }
    uint64_t GetBufferSize() const override
    {
        return 0;
    }
    bool GetBufferingTimeOut() override
    {
        return true;
    }
    size_t GetContentLength() const override
    {
        return contentLength_;
    }
    int64_t GetDuration() const override
    {
        return 0;
    }
    Seekable GetSeekable() const override
    {
        return Seekable::SEEKABLE;
    }
    void SetCallback(const std::shared_ptr<Callback>& cb) override
    {
        (void)cb;
        return;
    }
    void SetStatusCallback(StatusCallbackFunc cb) override
    {
        (void)cb;
        return;
    }
    bool GetStartedStatus() override
    {
        return true;
    }
    void SetInterruptState(bool isInterruptNeeded) override
    {
        (void)isInterruptNeeded;
        return;
    }
    void SetAppUid(int32_t appUid) override
    {
        (void)appUid;
        return;
    }
    bool SelectBitRate(uint32_t bitRate) override
    {
        return bitRate == 0 ? false : true;
    }
    void SetIsTriggerAutoMode(bool isAuto) override
    {
        (void)isAuto;
        return;
    }
    bool AutoSelectBitRate(uint32_t bitRate) override
    {
        return bitRate == 0 ? false : true;
    }
    size_t GetSegmentOffset() override
    {
        return segmentOffset_;
    }
    bool GetHLSDiscontinuity() override
    {
        return true;
    }
    void SetSourceStatisticsDfx(std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr) override
    {
        return;
    }
    Status GetStreamInfo(std::vector<StreamInfo>& streams, bool isUpdate = false) override
    {
        (void)streams;
        getStreamInfoCalled_ = true;
        getStreamInfoIsUpdate_ = isUpdate;
        return Status::OK;
    }
    void SetDefaultStreamId(int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId) override
    {
        setDefaultStreamIdCalled_ = true;
        lastVideoId_ = videoStreamId;
        lastAudioId_ = audioStreamId;
        lastSubtitleId_ = subTitleStreamId;
    }
    bool getStreamInfoCalled_ {false};
    bool getStreamInfoIsUpdate_ {false};
    bool setDefaultStreamIdCalled_ {false};
    int32_t lastVideoId_ {-1};
    int32_t lastAudioId_ {-1};
    int32_t lastSubtitleId_ {-1};
private:
    size_t contentLength_ {4};
    size_t segmentOffset_ {1};
};

class MockCallback : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        (void)event;
        return;
    }
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
 * @tc.desc  : Test source->GetSourceLoader() == nullptr and mimeType_== AVMimeTypes::APPLICATION_M3U8
 */
HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    std::string uri {"mock"};
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    ASSERT_EQ(nullptr, source->sourceLoader_);
    source->mimeType_ = AVMimeTypes::APPLICATION_M3U8;
    plugin_->SetDownloaderBySource(source);
    ASSERT_EQ(nullptr, plugin_->loaderCombinations_);
    ASSERT_NE(nullptr, plugin_->downloader_);
}

/**
 * @tc.name  : Test IsSeekToTimeSupported API
 * @tc.number: IsSeekToTimeSupported_001
 * @tc.desc  : Test mimeType_== AVMimeTypes::APPLICATION_M3U8
 */
HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported_001, TestSize.Level1)
{
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
 * @tc.name  : Test CheckIsM3U8Uri API
 * @tc.number: CheckIsM3U8Uri_001
 * @tc.desc  : Test uri_.empty() == true
 */
HWTEST_F(HttpSourcePluginUnitTest, CheckIsM3U8Uri_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, plugin_);
    std::string mockUri;
    plugin_->uri_.swap(mockUri);
    ASSERT_EQ(false, plugin_->CheckIsM3U8Uri());
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
    SUCCEED();
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
}
}
}
}
}