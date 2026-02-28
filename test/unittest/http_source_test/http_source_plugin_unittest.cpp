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
    void SetCallback(Callback* cb) override
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
    MockCallback mockCallback;
    plugin_->callback_ = &mockCallback;
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
}
}
}
}