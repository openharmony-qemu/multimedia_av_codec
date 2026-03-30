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

#include "hls_playlist_downloader_coverage_unit_test.h"
#include "http_server_demo.h"

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace testing::ext;
using namespace std;

namespace {
class HlsPlayListDownloaderCoverageTestable : public HlsPlayListDownloader {
public:
    using HlsPlayListDownloader::HlsPlayListDownloader;

    void SetPlayListForTest(const std::string& playList)
    {
        playList_ = playList;
    }
};

const static std::string TEST_URI_PATH = "http://127.0.0.1:46666/";
const static std::string M3U8_PATH_HLS = "test_hls/";
const static std::map<std::string, std::string> httpHeader = {
    {"User-Agent", "userAgent"},
    {"Referer", "DEF"},
};
std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
}

void HlsPlayListDownloaderCoverageUnitTest::SetUpTestCase(void) {}

void HlsPlayListDownloaderCoverageUnitTest::TearDownTestCase(void) {}

void HlsPlayListDownloaderCoverageUnitTest::SetUp(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
}

void HlsPlayListDownloaderCoverageUnitTest::TearDown(void)
{
    g_server->StopServer();
    g_server = nullptr;
}

/**
 * @tc.name: TEST_ENCRYPTION
 * @tc.desc: test encryption functionality with AES-128 key and keyIndex handling
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, TEST_ENCRYPTION, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_HLS + "testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    
    std::list<std::shared_ptr<M3U8Fragment>> files;
    std::ignore = downloader->KeyChange(files);
    EXPECT_GT(files.size(), 0);
    
    auto aesDecryptor = downloader->GetAesDecryptor(1);
    EXPECT_NE(aesDecryptor, nullptr);
}

/**
 * @tc.name: TEST_MEDIA_SEPARATION
 * @tc.desc: test media separation with independent audio and subtitle streams
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, TEST_MEDIA_SEPARATION, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_HLS + "testMediaSeparation.m3u8";
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams, false);
    
    bool hasAudio = false;
    bool hasSubtitle = false;
    for (const auto& stream : streams) {
        if (stream.type == StreamType::AUDIO) hasAudio = true;
        if (stream.type == StreamType::SUBTITLE) hasSubtitle = true;
    }
    EXPECT_TRUE(hasAudio);
    EXPECT_TRUE(hasSubtitle);
}

/**
 * @tc.name: TEST_DRM
 * @tc.desc: test DRM functionality with SAMPLE-AES encryption
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, TEST_DRM, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_HLS + "testDRM.m3u8";
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    
    EXPECT_FALSE(downloader->GetMaster()->isDecryptAble_);
}

/**
 * @tc.name: TEST_SESSION_KEY
 * @tc.desc: test session key functionality with EXT-X-SESSION-KEY
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, TEST_SESSION_KEY, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_HLS + "testSessionKey.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    OSAL::SleepFor(100);
    
    EXPECT_FALSE(downloader->GetMaster()->isDecryptAble_);
    
    std::list<std::shared_ptr<M3U8Fragment>> files;
    auto sessionKeyIndex = downloader->KeyChange(files);
    EXPECT_GT(sessionKeyIndex, 0);
}

/**
 * @tc.name: TEST_DISCONTINUITY
 * @tc.desc: test discontinuity tags in playlist
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, TEST_DISCONTINUITY, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_HLS + "testDiscontinuity.m3u8";
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    
    EXPECT_TRUE(downloader->GetHLSDiscontinuity());
}

/**
 * @tc.name: TEST_START_TAG
 * @tc.desc: test EXT-X-START tag with TIME-OFFSET and PRECISE attributes
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, TEST_START_TAG, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_HLS + "testStartTag.m3u8";
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    
    auto startInfo = downloader->GetStartInfo();
    EXPECT_GT(startInfo.first, 0);
    EXPECT_TRUE(startInfo.second);
}

/**
 * @tc.name: TEST_BYTERANGE
 * @tc.desc: test byte range functionality with EXT-X-BYTERANGE and EXT-X-MAP
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, TEST_BYTERANGE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_HLS + "testByteRange2.m3u8";
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    
    EXPECT_FALSE(downloader->IsPureByteRange());
}
}
