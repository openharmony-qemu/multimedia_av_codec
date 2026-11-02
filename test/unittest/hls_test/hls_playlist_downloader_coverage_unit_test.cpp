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
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>

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

class TestCallback : public Callback {
public:
    void OnEvent(const PluginEvent &event) override {}
};
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
    
    std::vector<M3U8Segment> files;
    std::ignore = downloader->KeyChange(files);
    ASSERT_FALSE(files.empty());
    
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
    
    std::vector<M3U8Segment> files;
    auto sessionKeyIndex = downloader->KeyChange(files);
    ASSERT_TRUE(sessionKeyIndex > 0);
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
    ASSERT_TRUE(startInfo.first > 0);
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

/**
 * @tc.name: AES_DECRYPTOR_MANAGER_BASIC
 * @tc.desc: test AesDecryptorManager get/create/hasExplicitIv
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, AES_DECRYPTOR_MANAGER_BASIC, TestSize.Level2)
{
    AesDecryptorManager mgr;
    ASSERT_EQ(mgr.GetAesDecryptorByKeyIndex(999), nullptr);
    ASSERT_EQ(mgr.HasExplicitIv(999), false);

    KeyInfo ki;
    ki.index_ = 1;
    ki.keyLen_ = 16;
    for (int i = 0; i < 16; i++) {
        ki.key_[i] = static_cast<uint8_t>(i);
        ki.iv_[i] = static_cast<uint8_t>(i + 1);
    }
    ki.hasExplicitIv_ = true;
    mgr.CreateOneAesDecryptor(ki);
    ASSERT_NE(mgr.GetAesDecryptorByKeyIndex(1), nullptr);
    ASSERT_EQ(mgr.HasExplicitIv(1), true);

    std::unordered_map<uint64_t, KeyInfo> keyInfos;
    KeyInfo ki2;
    ki2.index_ = 2;
    ki2.keyLen_ = 0;
    ki2.hasExplicitIv_ = false;
    KeyInfo ki3;
    ki3.index_ = 1;
    ki3.keyLen_ = 16;
    KeyInfo ki4;
    ki4.index_ = 3;
    ki4.keyLen_ = 16;
    ki4.hasExplicitIv_ = true;
    keyInfos[1] = ki3;
    keyInfos[2] = ki2;
    keyInfos[3] = ki4;
    mgr.CreateAesDecryptorByKeyInfos(keyInfos);
    ASSERT_NE(mgr.GetAesDecryptorByKeyIndex(1), nullptr);
    ASSERT_EQ(mgr.GetAesDecryptorByKeyIndex(2), nullptr);
    ASSERT_NE(mgr.GetAesDecryptorByKeyIndex(3), nullptr);
    ASSERT_EQ(mgr.HasExplicitIv(2), false);
    ASSERT_EQ(mgr.HasExplicitIv(3), true);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_NULL_DOWNLOADER_METHODS
 * @tc.desc: test Stop/Start/Cancel/SetInterruptState/SetAppUid/StopBufferring with null downloader
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_NULL_DOWNLOADER_METHODS, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->downloader_ = nullptr;

    downloader->Stop();
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
    downloader->Start();
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
    downloader->Cancel();
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
    downloader->SetInterruptState(true);
    ASSERT_EQ(downloader->isInterruptNeeded_.load(), true);
    downloader->SetAppUid(12345);
    ASSERT_EQ(downloader->isInterruptNeeded_.load(), true);
    downloader->StopBufferring(false);
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_WITH_DOWNLOADER_METHODS
 * @tc.desc: test Stop/Start/Cancel/SetInterruptState/SetAppUid/StopBufferring with valid downloader
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_WITH_DOWNLOADER_METHODS, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    ASSERT_NE(downloader->downloader_, nullptr);

    downloader->SetInterruptState(true);
    ASSERT_EQ(downloader->isInterruptNeeded_.load(), true);
    downloader->SetAppUid(54321);
    ASSERT_EQ(downloader->isInterruptNeeded_.load(), true);
    downloader->Stop();
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
    downloader->Start();
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
    downloader->Cancel();
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
    downloader->StopBufferring(true);
    ASSERT_EQ(downloader->isAppBackground_.load(), true);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_INIT_WITH_CALLBACK
 * @tc.desc: test Init with downloadCallback_ set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_INIT_WITH_CALLBACK, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    auto callback = std::make_shared<DownloadMetricsInfo>();
    downloader->SetDownloadCallback(callback);
    downloader->Init();
    ASSERT_EQ(downloader->downloadCallback_, callback);
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_SAVE_DATA_INVALID
 * @tc.desc: test SaveData with invalid inputs
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_SAVE_DATA_INVALID, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    uint8_t data[] = {0x23, 0x45, 0x67};

    ASSERT_EQ(downloader->SaveData(nullptr, 1, true), 0u);
    ASSERT_EQ(downloader->SaveData(data, 0, true), 0u);
    ASSERT_EQ(downloader->SaveData(data, 101 * 1024 * 1024, true), 0u);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_SAVE_DATA_NO_DOWNLOADER
 * @tc.desc: test SaveData with valid data but downloader_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_SAVE_DATA_NO_DOWNLOADER, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->downloader_ = nullptr;
    uint8_t data[] = {0x23, 0x45, 0x67};
    ASSERT_EQ(downloader->SaveData(data, 3, true), 0u);
    ASSERT_EQ(downloader->playList_.empty(), false);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_SAVE_DATA_WITH_DOWNLOADER
 * @tc.desc: test SaveData with valid data and downloader set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_SAVE_DATA_WITH_DOWNLOADER, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    uint8_t data[] = {0x23, 0x45, 0x67};
    uint32_t ret = downloader->SaveData(data, 3, true);
    ASSERT_EQ(ret, 0u);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_SAVE_DATA_OVERFLOW
 * @tc.desc: test SaveData when playList_ exceeds max size
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_SAVE_DATA_OVERFLOW, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->playList_.resize(101 * 1024 * 1024, 'x');
    uint8_t data[] = {0x23};
    ASSERT_EQ(downloader->SaveData(data, 1, true), 0u);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_PARSE_URI_INVALID
 * @tc.desc: test ParseUriInfo with invalid URIs
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_PARSE_URI_INVALID, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();

    ASSERT_EQ(downloader->ParseUriInfo(""), false);
    ASSERT_EQ(downloader->ParseUriInfo("invalid_uri"), false);
    ASSERT_EQ(downloader->ParseUriInfo("fd://?offset=0&size=100"), false);
    ASSERT_EQ(downloader->ParseUriInfo("fd://abc"), false);
    ASSERT_EQ(downloader->ParseUriInfo("fd://999999"), false);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_PARSE_URI_VALID_FD
 * @tc.desc: test ParseUriInfo with valid fd
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_PARSE_URI_VALID_FD, TestSize.Level2)
{
    const char* path = "/data/test/media/test_parse_uri.m3u8";
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_NE(fd, -1);
    const char* content = "#EXTM3U\n#EXT-X-ENDLIST\n";
    write(fd, content, strlen(content));
    fsync(fd);

    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string uri = "fd://" + std::to_string(fd);
    ASSERT_EQ(downloader->ParseUriInfo(uri), true);
    ASSERT_EQ(downloader->fd_, fd);
    ASSERT_EQ(downloader->size_, static_cast<uint64_t>(strlen(content)));
    ASSERT_EQ(downloader->offset_, 0);
    close(fd);
    remove(path);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_PARSE_URI_WITH_OFFSET_SIZE
 * @tc.desc: test ParseUriInfo with offset and size params
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_PARSE_URI_WITH_OFFSET_SIZE, TestSize.Level2)
{
    const char* path = "/data/test/media/test_parse_offset.m3u8";
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_NE(fd, -1);
    const char* content = "#EXTM3U\n#EXT-X-ENDLIST\n";
    write(fd, content, strlen(content));
    fsync(fd);

    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=5";
    ASSERT_EQ(downloader->ParseUriInfo(uri), true);
    ASSERT_EQ(downloader->fd_, fd);
    ASSERT_EQ(downloader->offset_, 0);
    ASSERT_EQ(downloader->size_, 5u);

    auto downloader2 = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader2->Init();
    std::string uri2 = "fd://" + std::to_string(fd) + "?offset=99999&size=100";
    ASSERT_EQ(downloader2->ParseUriInfo(uri2), true);
    ASSERT_EQ(downloader2->fd_, fd);
    close(fd);
    remove(path);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_PARSE_URI_NEGATIVE_SIZE
 * @tc.desc: test ParseUriInfo with negative size value
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_PARSE_URI_NEGATIVE_SIZE, TestSize.Level2)
{
    const char* path = "/data/test/media/test_parse_neg.m3u8";
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_NE(fd, -1);
    write(fd, "data", 4);
    fsync(fd);

    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=-1";
    ASSERT_EQ(downloader->ParseUriInfo(uri), true);
    ASSERT_EQ(downloader->fd_, fd);
    EXPECT_EQ(downloader->offset_, 0);
    auto parsedSize = downloader->size_;
    close(fd);
    remove(path);
    EXPECT_EQ(parsedSize, 0u);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DO_OPEN_NATIVE
 * @tc.desc: test DoOpenNative with valid file
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DO_OPEN_NATIVE, TestSize.Level2)
{
    const char* path = "/data/test/media/test_doopen_native.m3u8";
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_NE(fd, -1);
    const char* content = "#EXTM3U\n#EXT-X-ENDLIST\n";
    write(fd, content, strlen(content));
    fsync(fd);
    lseek(fd, 0, SEEK_SET);

    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string uri = "fd://" + std::to_string(fd);
    downloader->DoOpenNative(uri);
    ASSERT_EQ(downloader->playList_.empty(), false);
    close(fd);
    remove(path);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_PARSE_FD_RANGE_BOUNDS
 * @tc.desc: test shared fd parsing with clamped ranges and invalid fd
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_PARSE_FD_RANGE_BOUNDS, TestSize.Level2)
{
    const char* path = "/data/test/media/test_filesize.m3u8";
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_NE(fd, -1);
    write(fd, "12345678", 8);
    fsync(fd);

    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    auto uri = "fd://" + std::to_string(fd);
    // An eight-byte file: offset 3 leaves five bytes; an offset beyond EOF clamps to eight and zero bytes.
    EXPECT_TRUE(downloader->ParseUriInfo(uri + "?offset=3&size=99"));
    EXPECT_EQ(downloader->offset_, 3);
    EXPECT_EQ(downloader->size_, 5u);
    EXPECT_TRUE(downloader->ParseUriInfo(uri + "?offset=99&size=8"));
    EXPECT_EQ(downloader->offset_, 8);
    EXPECT_EQ(downloader->size_, 0u);
    EXPECT_FALSE(downloader->ParseUriInfo("fd://-1"));
    close(fd);
    remove(path);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_SEEK_TO
 * @tc.desc: test SeekTo with valid and invalid fd
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_SEEK_TO, TestSize.Level2)
{
    const char* path = "/data/test/media/test_seek.m3u8";
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_NE(fd, -1);
    write(fd, "12345678", 8);
    fsync(fd);

    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->fd_ = -1;
    ASSERT_EQ(downloader->SeekTo(0), false);

    downloader->fd_ = fd;
    downloader->offset_ = 0;
    ASSERT_EQ(downloader->SeekTo(0), true);
    close(fd);
    remove(path);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_UPDATE_DOWNLOAD_FINISHED
 * @tc.desc: test UpdateDownloadFinished with isAppBackground true and false
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_UPDATE_DOWNLOAD_FINISHED, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->playList_ = "some_content";
    downloader->isAppBackground_ = false;
    downloader->UpdateDownloadFinished("url", "location");
    ASSERT_EQ(downloader->playList_.empty(), true);
    ASSERT_EQ(downloader->retryStartTime_, 0);

    downloader->playList_ = "more_content";
    downloader->isAppBackground_ = true;
    downloader->UpdateDownloadFinished("url2", "loc2");
    ASSERT_EQ(downloader->playList_.empty(), true);
    ASSERT_EQ(downloader->isAppBackground_.load(), true);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_ON_DOWNLOAD_STATUS
 * @tc.desc: test OnDownloadStatus with null and valid request
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_ON_DOWNLOAD_STATUS, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::shared_ptr<Downloader> dl = nullptr;
    std::shared_ptr<DownloadRequest> nullReq = nullptr;
    PlayListDownloader::OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, dl, nullReq);
    ASSERT_EQ(nullReq, nullptr);

    DataSaveFunc saveFunc = [](uint8_t* d, uint32_t l, bool b) -> uint32_t { return 0; };
    StatusCallbackFunc statusCb = [](DownloadStatus, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    auto req = std::make_shared<DownloadRequest>(saveFunc, statusCb, RequestInfo{}, true);
    req->clientError_ = 1;
    std::shared_ptr<DownloadRequest> reqPtr = req;
    PlayListDownloader::OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, dl, reqPtr);
    ASSERT_EQ(req->GetClientError(), 1);

    req->clientError_ = 0;
    req->serverError_ = 1;
    PlayListDownloader::OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, dl, reqPtr);
    ASSERT_EQ(req->GetServerError(), 1);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_RESUME_PAUSE_CLOSE_NOT_LIVE
 * @tc.desc: test Resume/Pause/Close when IsLive is false
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_RESUME_PAUSE_CLOSE_NOT_LIVE, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    ASSERT_EQ(downloader->IsLive(), false);

    downloader->Resume();
    ASSERT_EQ(downloader->IsLive(), false);
    downloader->Pause(false);
    ASSERT_EQ(downloader->IsLive(), false);
    downloader->Pause(true);
    ASSERT_EQ(downloader->IsLive(), false);
    downloader->Close();
    ASSERT_EQ(downloader->IsLive(), false);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_PAUSE_NULL_TASK
 * @tc.desc: test Pause with updateTask_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_PAUSE_NULL_TASK, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->updateTask_ = nullptr;
    downloader->Pause(false);
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
    downloader->Pause(true);
    ASSERT_EQ(downloader->IsLive(), false);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_RESUME_PAUSE_LIVE
 * @tc.desc: test Resume/Pause/Close when IsLive is true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_RESUME_PAUSE_LIVE, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    auto master = std::make_shared<M3U8MasterPlaylist>("#EXTM3U\n", "test");
    master->bLive_ = true;
    downloader->SetThreadSafeMaster(master);
    ASSERT_EQ(downloader->IsLive(), true);

    downloader->Resume();
    ASSERT_EQ(downloader->IsLive(), true);
    downloader->Pause(true);
    ASSERT_EQ(downloader->IsLive(), true);
    downloader->Pause(false);
    ASSERT_EQ(downloader->IsLive(), true);
    downloader->Close();
    ASSERT_EQ(downloader->IsLive(), true);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_HEADERS_AND_ACCESSORS
 * @tc.desc: test GetHttpHeader/SaveHttpHeader/GetSourceLoader/SetSourceLoader/SetStatusCallback/SetCallback
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_HEADERS_AND_ACCESSORS, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();

    auto headers = downloader->GetHttpHeader();
    ASSERT_EQ(headers.count("User-Agent"), 1u);
    ASSERT_EQ(headers.at("User-Agent"), std::string("userAgent"));

    std::map<std::string, std::string> newHeader = {{"X-Test", "val"}};
    downloader->SaveHttpHeader(newHeader);
    auto headers2 = downloader->GetHttpHeader();
    ASSERT_EQ(headers2.at("X-Test"), std::string("val"));

    auto sl = downloader->GetSourceLoader();
    ASSERT_EQ(sl, nullptr);
    auto newSl = std::make_shared<MediaSourceLoaderCombinations>(nullptr);
    downloader->SetSourceLoader(newSl);
    ASSERT_EQ(downloader->GetSourceLoader(), newSl);

    StatusCallbackFunc cb = [](DownloadStatus, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    downloader->SetStatusCallback(cb);
    auto cb2 = std::make_shared<TestCallback>();
    downloader->SetCallback(cb2);
    ASSERT_NE(downloader->eventCallback_.lock(), nullptr);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_CONSTRUCTOR_WITH_SOURCE_LOADER
 * @tc.desc: test constructor with non-null sourceLoader
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_CONSTRUCTOR_WITH_SOURCE_LOADER, TestSize.Level2)
{
    auto sourceLoader = std::make_shared<MediaSourceLoaderCombinations>(nullptr);
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, sourceLoader);
    downloader->Init();
    ASSERT_NE(downloader->downloader_, nullptr);
    ASSERT_EQ(downloader->GetSourceLoader(), sourceLoader);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_CONSTRUCTOR_WITH_DOWNLOADER
 * @tc.desc: test constructor with explicit downloader
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_CONSTRUCTOR_WITH_DOWNLOADER, TestSize.Level2)
{
    auto innerDl = std::make_shared<Downloader>("testHls");
    auto downloader = std::make_shared<HlsPlayListDownloader>(innerDl, httpHeader);
    ASSERT_EQ(downloader->downloader_, innerDl);
    auto headers = downloader->GetHttpHeader();
    ASSERT_EQ(headers.count("Referer"), 1u);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_INIT_NULL_DOWNLOADER
 * @tc.desc: test Init with downloader_ set to null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_INIT_NULL_DOWNLOADER, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->downloader_ = nullptr;
    downloader->Init();
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
    ASSERT_EQ(downloader->downloader_, nullptr);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DO_OPEN_NULL_DOWNLOADER
 * @tc.desc: test DoOpen with downloader_ null (skip download)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DO_OPEN_NULL_DOWNLOADER, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->downloader_ = nullptr;
    downloader->DoOpen("http://127.0.0.1:46666/test.m3u8");
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DO_OPEN_NATIVE_INVALID_URI
 * @tc.desc: test DoOpenNative with invalid URI (ParseUriInfo fails)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DO_OPEN_NATIVE_INVALID_URI, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->DoOpenNative("invalid_uri");
    ASSERT_EQ(downloader->playList_.empty(), true);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DO_OPEN_NATIVE_READ_FAIL
 * @tc.desc: test DoOpenNative with closed fd (read fails)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DO_OPEN_NATIVE_READ_FAIL, TestSize.Level2)
{
    const char* path = "/data/test/media/test_readfail.m3u8";
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_NE(fd, -1);
    write(fd, "data", 4);
    fsync(fd);
    close(fd);
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string uri = "fd://" + std::to_string(fd);
    downloader->DoOpenNative(uri);
    ASSERT_EQ(downloader->playList_.empty(), true);
    remove(path);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_SEEK_TO_FAIL
 * @tc.desc: test SeekTo with invalid fd causing lseek failure
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_SEEK_TO_FAIL, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    int pipes[2];
    pipe(pipes);
    downloader->fd_ = pipes[0];
    downloader->offset_ = 0;
    ASSERT_EQ(downloader->SeekTo(0), false);
    close(pipes[0]);
    close(pipes[1]);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_SAVE_DATA_NEAR_FULL
 * @tc.desc: test SaveData when playList_ is near max size
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_SAVE_DATA_NEAR_FULL, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->playList_.resize(100 * 1024 * 1024 - 1, 'x');
    uint8_t data[] = {0x23, 0x45};
    ASSERT_EQ(downloader->SaveData(data, 2, true), 0u);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_ON_DOWNLOAD_STATUS_NO_ERROR
 * @tc.desc: test OnDownloadStatus with no errors
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_ON_DOWNLOAD_STATUS_NO_ERROR, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::shared_ptr<Downloader> dl = nullptr;
    DataSaveFunc saveFunc = [](uint8_t* d, uint32_t l, bool b) -> uint32_t { return 0; };
    StatusCallbackFunc statusCb = [](DownloadStatus, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    auto req = std::make_shared<DownloadRequest>(saveFunc, statusCb, RequestInfo{}, true);
    req->clientError_ = 0;
    req->serverError_ = 0;
    std::shared_ptr<DownloadRequest> reqPtr = req;
    PlayListDownloader::OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, dl, reqPtr);
    ASSERT_EQ(req->GetClientError(), 0);
    ASSERT_EQ(req->GetServerError(), 0);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_RESUME_LIVE_NO_TASK
 * @tc.desc: test Resume with IsLive true but updateTask_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_RESUME_LIVE_NO_TASK, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    auto master = std::make_shared<M3U8MasterPlaylist>("#EXTM3U\n", "test");
    master->bLive_ = true;
    downloader->SetThreadSafeMaster(master);
    downloader->updateTask_ = nullptr;
    downloader->Resume();
    ASSERT_EQ(downloader->IsLive(), true);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DATA_SAVE_CALLBACK
 * @tc.desc: test dataSave_ lambda invocation while downloader alive
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DATA_SAVE_CALLBACK, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->downloader_ = nullptr;
    uint8_t data[] = {0x23, 0x45, 0x67};
    uint32_t ret = downloader->dataSave_(data, 3, true);
    ASSERT_EQ(ret, 0u);
    ASSERT_EQ(downloader->playList_.empty(), false);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_STATUS_CALLBACK
 * @tc.desc: test statusCallback_ lambda invocation while downloader alive
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_STATUS_CALLBACK, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::shared_ptr<Downloader> dl = downloader->downloader_;
    DataSaveFunc saveFunc = [](uint8_t* d, uint32_t l, bool b) -> uint32_t { return 0; };
    StatusCallbackFunc statusCb = [](DownloadStatus, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    auto req = std::make_shared<DownloadRequest>(saveFunc, statusCb, RequestInfo{}, true);
    std::shared_ptr<DownloadRequest> reqPtr = req;
    downloader->statusCallback_(DownloadStatus::PARTTAL_DOWNLOAD, dl, reqPtr);
    ASSERT_EQ(req->GetClientError(), 0);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_PARSE_URI_OVERFLOW_FD
 * @tc.desc: test ParseUriInfo with fd number overflow
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_PARSE_URI_OVERFLOW_FD, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    ASSERT_EQ(downloader->ParseUriInfo("fd://99999999999"), false);
    ASSERT_EQ(downloader->fd_, -1);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_PARSE_URI_INVALID_OFFSET
 * @tc.desc: test ParseUriInfo with non-numeric offset and size
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_PARSE_URI_INVALID_OFFSET, TestSize.Level2)
{
    const char* path = "/data/test/media/test_invalid_offset.m3u8";
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_NE(fd, -1);
    write(fd, "data", 4);
    fsync(fd);
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string uri = "fd://" + std::to_string(fd) + "?offset=abc&size=xyz";
    ASSERT_EQ(downloader->ParseUriInfo(uri), true);
    ASSERT_EQ(downloader->fd_, fd);
    EXPECT_EQ(downloader->offset_, 0);
    auto parsedSize = downloader->size_;
    close(fd);
    remove(path);
    EXPECT_EQ(parsedSize, 0u);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_PARSE_URI_LARGE_FILE
 * @tc.desc: test ParseUriInfo with file larger than MAX_FILE_SIZE
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_PARSE_URI_LARGE_FILE, TestSize.Level2)
{
    const char* path = "/data/test/media/test_large.m3u8";
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_NE(fd, -1);
    lseek(fd, 21 * 1024 * 1024, SEEK_SET);
    write(fd, "x", 1);
    fsync(fd);
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string uri = "fd://" + std::to_string(fd);
    ASSERT_EQ(downloader->ParseUriInfo(uri), true);
    ASSERT_EQ(downloader->fd_, fd);
    ASSERT_EQ(downloader->size_, 0u);
    close(fd);
    remove(path);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_PARSE_URI_NON_SEEKABLE
 * @tc.desc: test ParseUriInfo with non-seekable fd (pipe)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_PARSE_URI_NON_SEEKABLE, TestSize.Level2)
{
    int pipes[2];
    pipe(pipes);
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string uri = "fd://" + std::to_string(pipes[0]);
    bool result = downloader->ParseUriInfo(uri);
    ASSERT_EQ(result, false);
    close(pipes[0]);
    close(pipes[1]);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DO_OPEN_REAL_STATUS_CALLBACK
 * @tc.desc: test realStatusCallback lambda in DoOpen directly
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DO_OPEN_REAL_STATUS_CALLBACK, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->downloader_ = nullptr;
    downloader->DoOpen("http://127.0.0.1:46666/test_cb.m3u8");

    auto req = downloader->downloadRequest_;
    ASSERT_NE(req, nullptr);
    std::shared_ptr<Downloader> dl = nullptr;

    req->retryTimes_ = 0;
    req->statusCallback_(DownloadStatus::PARTTAL_DOWNLOAD, dl, req);
    ASSERT_EQ(downloader->retryStartTime_ > 0, true);

    downloader->playList_ = "test_content";
    auto cb = std::make_shared<TestCallback>();
    downloader->SetCallback(cb);
    req->retryTimes_ = 100;
    req->statusCallback_(DownloadStatus::PARTTAL_DOWNLOAD, dl, req);
    ASSERT_EQ(downloader->playList_, std::string("test_content"));

    auto downloader2 = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader2->Init();
    downloader2->downloader_ = nullptr;
    downloader2->DoOpen("http://127.0.0.1:46666/test_cb2.m3u8");
    auto req2 = downloader2->downloadRequest_;
    ASSERT_NE(req2, nullptr);
    std::shared_ptr<Downloader> dl2 = nullptr;
    req2->retryTimes_ = 100;
    req2->statusCallback_(DownloadStatus::PARTTAL_DOWNLOAD, dl2, req2);
    ASSERT_EQ(downloader2->playList_.empty(), true);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DO_OPEN_410_CALLBACK
 * @tc.desc: test realStatusCallback with 410 server error (ReOpen path)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DO_OPEN_410_CALLBACK, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->downloader_ = nullptr;
    downloader->DoOpen("http://127.0.0.1:46666/test_410.m3u8");

    auto req = downloader->downloadRequest_;
    ASSERT_NE(req, nullptr);
    std::shared_ptr<Downloader> dl = nullptr;
    req->serverError_ = 410;
    req->statusCallback_(DownloadStatus::PARTTAL_DOWNLOAD, dl, req);
    ASSERT_EQ(downloader->retryStartTime_, 0);
    ASSERT_EQ(downloader->GetPlayListDownloadStatus(), false);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DO_OPEN_DONE_CALLBACK
 * @tc.desc: test downloadDoneCallback lambda in DoOpen directly
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DO_OPEN_DONE_CALLBACK, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->downloader_ = nullptr;
    downloader->DoOpen("http://127.0.0.1:46666/test_done.m3u8");

    auto req = downloader->downloadRequest_;
    ASSERT_NE(req, nullptr);
    downloader->playList_ = "some_data";
    req->downloadDoneCallback_("url", "location");
    ASSERT_EQ(downloader->playList_.empty(), true);
    ASSERT_EQ(downloader->retryStartTime_, 0);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DO_OPEN_RETRY_TIMEOUT
 * @tc.desc: test realStatusCallback with retry time exceeding threshold
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DO_OPEN_RETRY_TIMEOUT, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->downloader_ = nullptr;
    downloader->DoOpen("http://127.0.0.1:46666/test_retry_timeout.m3u8");

    auto req = downloader->downloadRequest_;
    ASSERT_NE(req, nullptr);
    std::shared_ptr<Downloader> dl = nullptr;
    downloader->retryStartTime_ = 1;
    req->retryTimes_ = 0;
    downloader->playList_ = "keep_this";
    req->statusCallback_(DownloadStatus::PARTTAL_DOWNLOAD, dl, req);
    ASSERT_EQ(downloader->playList_.empty(), true);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_UPDATE_TASK_LIVE
 * @tc.desc: test updateTask lambda execution with live stream
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_UPDATE_TASK_LIVE, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    auto master = std::make_shared<M3U8MasterPlaylist>("#EXTM3U\n", "test");
    master->bLive_ = true;
    downloader->SetThreadSafeMaster(master);
    downloader->Resume();
    OSAL::SleepFor(200);
    downloader->Close();
    ASSERT_EQ(downloader->IsLive(), true);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DATA_SAVE_AFTER_DESTROY
 * @tc.desc: test dataSave_ lambda after downloader destroyed (weak_ptr expired)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DATA_SAVE_AFTER_DESTROY, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    auto dataSave = downloader->dataSave_;
    downloader.reset();
    uint8_t data[] = {0x23, 0x45, 0x67};
    uint32_t ret = dataSave(data, 3, true);
    ASSERT_EQ(ret, 0u);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_STATUS_CB_AFTER_DESTROY
 * @tc.desc: test statusCallback_ lambda after downloader destroyed (weak_ptr expired)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_STATUS_CB_AFTER_DESTROY, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    auto statusCb = downloader->statusCallback_;
    downloader.reset();
    std::shared_ptr<Downloader> dl = nullptr;
    DataSaveFunc saveFunc = [](uint8_t*, uint32_t, bool) -> uint32_t { return 0; };
    StatusCallbackFunc dummyCb = [](DownloadStatus, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    auto req = std::make_shared<DownloadRequest>(saveFunc, dummyCb, RequestInfo{}, true);
    std::shared_ptr<DownloadRequest> reqPtr = req;
    statusCb(DownloadStatus::PARTTAL_DOWNLOAD, dl, reqPtr);
    ASSERT_EQ(req->GetClientError(), 0);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DO_OPEN_REAL_STATUS_NULL_REQ
 * @tc.desc: test realStatusCallback with null request (FALSE_RETURN early)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DO_OPEN_REAL_STATUS_NULL_REQ, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->downloader_ = nullptr;
    downloader->DoOpen("http://127.0.0.1:46666/test_null_req_cb.m3u8");
    auto req = downloader->downloadRequest_;
    ASSERT_NE(req, nullptr);
    std::shared_ptr<Downloader> dl = nullptr;
    downloader->retryStartTime_ = 42;
    std::shared_ptr<DownloadRequest> nullReq = nullptr;
    req->statusCallback_(DownloadStatus::PARTTAL_DOWNLOAD, dl, nullReq);
    ASSERT_EQ(downloader->retryStartTime_, 42);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_REAL_STATUS_CB_AFTER_DESTROY
 * @tc.desc: test realStatusCallback after downloader destroyed (weak_ptr expired)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_REAL_STATUS_CB_AFTER_DESTROY, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->downloader_ = nullptr;
    downloader->DoOpen("http://127.0.0.1:46666/test_destroy_status.m3u8");
    auto req = downloader->downloadRequest_;
    ASSERT_NE(req, nullptr);
    auto statusCb = req->statusCallback_;
    downloader.reset();
    std::shared_ptr<Downloader> dl = nullptr;
    req->serverError_ = 0;
    std::shared_ptr<DownloadRequest> reqPtr = req;
    statusCb(DownloadStatus::PARTTAL_DOWNLOAD, dl, reqPtr);
    ASSERT_EQ(req->GetServerError(), 0);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DONE_CB_AFTER_DESTROY
 * @tc.desc: test downloadDoneCallback after downloader destroyed (weak_ptr expired)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DONE_CB_AFTER_DESTROY, TestSize.Level2)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    downloader->downloader_ = nullptr;
    downloader->DoOpen("http://127.0.0.1:46666/test_destroy_done.m3u8");
    auto req = downloader->downloadRequest_;
    ASSERT_NE(req, nullptr);
    auto doneCb = req->downloadDoneCallback_;
    downloader.reset();
    req->serverError_ = 0;
    doneCb("url", "location");
    ASSERT_EQ(req->GetServerError(), 0);
}

/**
 * @tc.name: PLAYLIST_DOWNLOADER_DO_OPEN_NATIVE_WRONLY_FD
 * @tc.desc: test DoOpenNative with write-only fd causing read failure
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderCoverageUnitTest, PLAYLIST_DOWNLOADER_DO_OPEN_NATIVE_WRONLY_FD, TestSize.Level2)
{
    const char* path = "/data/test/media/test_wronly.m3u8";
    int fd = open(path, O_CREAT | O_WRONLY, 0644);
    ASSERT_NE(fd, -1);
    write(fd, "data", 4);
    fsync(fd);
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string uri = "fd://" + std::to_string(fd);
    downloader->DoOpenNative(uri);
    ASSERT_EQ(downloader->playList_.empty(), true);
    close(fd);
    remove(path);
}
}
