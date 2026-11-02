/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "m3u8_unit_test.h"
#include "http_server_demo.h"

#include <atomic>
#include <chrono>
#include <new>
#include <limits>
#include <thread>
#include <unordered_map>
#define LOCAL true

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace testing::ext;
using namespace std;
constexpr uint32_t MAX_LOOP = 16;
std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;

namespace {
constexpr size_t TEST_AES_KEY_SIZE = 16;
constexpr uint64_t TEST_SESSION_KEY_INDEX = 5; // Positive session ID, distinct from the first ordinary key.
constexpr auto KEY_DOWNLOAD_TIMEOUT = std::chrono::seconds(3);
constexpr auto KEY_DOWNLOAD_POLL_INTERVAL = std::chrono::milliseconds(10);
const std::string KEY_TEST_BASE = "http://127.0.0.1:46666"; // HttpServerDemo's default loopback port.
const std::string KEY_WITH_CALLBACK_PATH = "/m3u8/key-with-callback.bin";
const std::string SESSION_KEY_PATH = "/m3u8/session-key.bin";
const std::string INTERRUPT_KEY_PATH = "/m3u8/interrupt-key.bin";
const std::string INTERRUPT_MAP_PATH = "/m3u8/interrupt-map.mp4";
const std::string TEST_KEY_BYTES = "0123456789abcdef";
const std::string TEST_SESSION_KEY_BYTES = "fedcba9876543210";
const std::string TEST_MAP_BYTES = "local-map-header";

class ScopedM3u8Download {
public:
    ScopedM3u8Download()
        : failed_(std::make_shared<std::atomic<bool>>(false)), metrics_(std::make_shared<DownloadMetricsInfo>())
    {
        auto failed = failed_;
        StatusCallbackFunc status = [failed](DownloadStatus, std::shared_ptr<Downloader>&,
            std::shared_ptr<DownloadRequest>&) {
            failed->store(true);
        };
        owner_ = std::make_shared<M3U8>(KEY_TEST_BASE, "", std::unordered_map<std::string, std::string> {}, status);
        owner_->SetDownloadCallback(metrics_);
    }

    ScopedM3u8Download(const ScopedM3u8Download&) = delete;
    ScopedM3u8Download& operator=(const ScopedM3u8Download&) = delete;

    ~ScopedM3u8Download()
    {
        Stop();
        owner_.reset();
        MediaAVCodec::HttpServerDemo::ClearContent();
    }

    std::shared_ptr<M3U8> Owner() const
    {
        return owner_;
    }

    std::shared_ptr<DownloadMetricsInfo> Metrics() const
    {
        return metrics_;
    }

    bool HadError() const
    {
        return failed_->load();
    }

    void PrepareKey(const std::string& path)
    {
        MediaAVCodec::HttpServerDemo::RegisterContent(path, TEST_KEY_BYTES);
        // Prepare the index and key table through the normal AES-128 tag path.
        std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY,
            "METHOD=AES-128,URI=\"" + path + "\"");
        owner_->PrepareDecrptionKeys(tag);
    }

    void PrepareSessionKey()
    {
        MediaAVCodec::HttpServerDemo::RegisterContent(SESSION_KEY_PATH, TEST_SESSION_KEY_BYTES);
        owner_->keyUri_ = std::make_shared<std::string>(KEY_TEST_BASE + SESSION_KEY_PATH);
        owner_->isSessionKey_ = true;
        owner_->sessionKeyIndex_ = TEST_SESSION_KEY_INDEX;
    }

    bool WaitForKey() const
    {
        auto request = owner_->downloadRequest_;
        if (request == nullptr) {
            return false;
        }
        auto deadline = std::chrono::steady_clock::now() + KEY_DOWNLOAD_TIMEOUT;
        while (std::chrono::steady_clock::now() < deadline) {
            if (HadError()) {
                return false;
            }
            if (owner_->isDecryptKeyReady_.load() && request->IsEos()) {
                return true;
            }
            std::this_thread::sleep_for(KEY_DOWNLOAD_POLL_INTERVAL);
        }
        return false;
    }

    void Stop()
    {
        if (stopped_) {
            return;
        }
        // Keep the M3U8 owner alive until both download jobs and their callbacks have stopped.
        if (owner_->downloader_ != nullptr) {
            owner_->downloader_->Stop(false);
        }
        if (owner_->downloaderHeader_ != nullptr) {
            owner_->downloaderHeader_->Stop(false);
        }
        stopped_ = true;
    }

private:
    std::shared_ptr<std::atomic<bool>> failed_;
    std::shared_ptr<DownloadMetricsInfo> metrics_;
    std::shared_ptr<M3U8> owner_;
    bool stopped_ {false};
};
} // namespace

void M3u8UnitTest::SetUpTestCase(void) {}

void M3u8UnitTest::TearDownTestCase(void) {}

void M3u8UnitTest::SetUp(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
}

void M3u8UnitTest::TearDown(void)
{
    g_server->StopServer();
    g_server = nullptr;
}

HWTEST_F(M3u8UnitTest, Init_Tag_Updaters_Map_001, TestSize.Level1)
{
    double duration = testM3u8->GetDuration();
    bool isLive = testM3u8->IsLive();
    EXPECT_EQ(duration, 0.0);
    EXPECT_EQ(isLive, false);
}

HWTEST_F(M3u8UnitTest, is_live_001, TestSize.Level1)
{
    EXPECT_EQ(testM3u8->GetDuration(), 0.0);
}

HWTEST_F(M3u8UnitTest, ConstructorTest, TestSize.Level1)
{
    M3U8 m3u8(testUri, testName);
    // 检查 URI 和名称是否正确设置
    ASSERT_EQ(m3u8.uri_, testUri);
    ASSERT_EQ(m3u8.name_, testName);
    // 这里可以添加更多的断言来检查初始化的状态
}

// 测试 Update 方法
HWTEST_F(M3u8UnitTest, UpdateTest, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "TestPlaylist");
    std::string testPlaylist = "#EXTM3U\n#EXT-X-TARGETDURATION:10\n...";

    // 测试有效的播放列表更新
    EXPECT_TRUE(m3u8.Update(testPlaylist, true));

    // 测试当播放列表不变时的更新
    EXPECT_TRUE(m3u8.Update(testPlaylist, true));

    // 测试无效的播放列表更新
    EXPECT_FALSE(m3u8.Update("Invalid Playlist", true));
}

HWTEST_F(M3u8UnitTest, UPDATE_FROM_TAGS_TEST, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "TestPlaylist");
    //
    std::list<std::shared_ptr<Tag>> tags;
    m3u8.UpdateFromTags(tags);
    EXPECT_TRUE(m3u8.bLive_);
    m3u8.isDecryptAble_ = true;
    m3u8.UpdateFromTags(tags);
    //
    EXPECT_TRUE(m3u8.bLive_);
    EXPECT_EQ(m3u8.files_.size(), 0);
    m3u8.isDecryptAble_ = false;
    m3u8.UpdateFromTags(tags);
    
    //
    EXPECT_TRUE(m3u8.bLive_);
    EXPECT_EQ(m3u8.files_.size(), 0);
}

HWTEST_F(M3u8UnitTest, UPDATE_FROM_TAGS_TEST_NULL_UPDATER, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "TestPlaylist");
    //
    std::list<std::shared_ptr<Tag>> tags;
    tags.emplace_back(std::make_shared<Tag>(HlsTag::EXTXSESSIONKEY));
    m3u8.tagUpdatersMap_[HlsTag::EXTXSESSIONKEY] = nullptr;
    m3u8.UpdateFromTags(tags);
    EXPECT_EQ(m3u8.files_.size(), 0);
}

HWTEST_F(M3u8UnitTest, TEST_CONSTRUCTOR_WITH_NULL_KEY_AND_IV, TestSize.Level1)
{
    M3U8Fragment m3u8("http://example.com", 10.0, 1, false);
    M3U8Fragment fragment(m3u8, nullptr, nullptr);
    EXPECT_EQ(fragment.duration_, 10.0);
    EXPECT_EQ(fragment.sequence_, 1);
    EXPECT_EQ(fragment.discont_, false);
    for (int i = 0; i < static_cast<int>(MAX_LOOP); i++) {
        EXPECT_NE(fragment.iv_[i], -1);
        EXPECT_NE(fragment.key_[i], -1);
    }
}

HWTEST_F(M3u8UnitTest, TEST_CONSTRUCTOR_WITH_VALID_KEY_AND_IV, TestSize.Level1)
{
    M3U8Fragment m3u8("http://example.com", 10.0, 1, false);
    uint8_t key[MAX_LOOP] = {1, 2, 3, 4, 5};
    uint8_t iv[MAX_LOOP] = {6, 7, 8, 9, 10};
    M3U8Fragment fragment(m3u8, key, iv);
    EXPECT_EQ(fragment.uri_, "http://example.com");
    EXPECT_EQ(fragment.duration_, 10.0);
    EXPECT_EQ(fragment.sequence_, 1);
    EXPECT_EQ(fragment.discont_, false);
    //
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(fragment.iv_[i], iv[i]);
    }
}

HWTEST_F(M3u8UnitTest, TEST_CONSTRUCTOR, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "Test M3U8");

    // check url
    EXPECT_EQ(m3u8.uri_, "http://example.com/test.m3u8");

    //check name
    EXPECT_EQ(m3u8.name_, "Test M3U8");

    //check updaters map
    EXPECT_FALSE(m3u8.tagUpdatersMap_.empty());
}

HWTEST_F(M3u8UnitTest, TEST_EMPTY_URI, TestSize.Level0)
{
    M3U8 m3u8("", "Test M3U8");

    // check url
    EXPECT_EQ(m3u8.uri_, "");

    //check name
    EXPECT_EQ(m3u8.name_, "Test M3U8");

    //check updaters map
    EXPECT_FALSE(m3u8.tagUpdatersMap_.empty());
}

HWTEST_F(M3u8UnitTest, TEST_EMPTY_NAME, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");

    // check url
    EXPECT_EQ(m3u8.uri_, "http://example.com/test.m3u8");

    //check name
    EXPECT_EQ(m3u8.name_, "");

    //check updaters map
    EXPECT_FALSE(m3u8.tagUpdatersMap_.empty());
}

// test get ext
HWTEST_F(M3u8UnitTest, GET_EXT_INF_TEST, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    auto tag = std::make_shared<ValuesListTag>(HlsTag::EXTINF, "3.5,");
    double duration {0};
    m3u8.GetExtInf(tag, duration);
    // check name
    EXPECT_DOUBLE_EQ(3.5, duration);
}

HWTEST_F(M3u8UnitTest, PARSE_KEY_METHOD_ATTRIBUTE, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDISCONTINUITY, "123");
    auto attr1 =  std::make_shared<Attribute>("METHOD", "AES-128");
    tag->AddAttribute(attr1);
    m3u8.ParseKey(tag);
    EXPECT_EQ(*m3u8.method_, "AES-128");
}

HWTEST_F(M3u8UnitTest, PARSE_KEY_URI_ATTRIBUTE, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDISCONTINUITY, "123");
    auto attr1 =  std::make_shared<Attribute>("URI", "http://example.com/key.bin");
    tag->AddAttribute(attr1);
    m3u8.ParseKey(tag);
    EXPECT_EQ(*m3u8.keyUri_, "http://example.com/key.bin");
}

HWTEST_F(M3u8UnitTest, PARSE_KEY_IV_ATTRIBUTE, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDISCONTINUITY, "123");
    auto attr1 =  std::make_shared<Attribute>("IV", "0x12345678");
    tag->AddAttribute(attr1);
    m3u8.ParseKey(tag);
    uint8_t vec[4] {0x12, 0x34, 0x56, 0x78};
    EXPECT_NE(m3u8.iv_, vec);
}

HWTEST_F(M3u8UnitTest, PARSE_KEY_NO_ATTRIBUTE, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDISCONTINUITY, "123");
    m3u8.ParseKey(tag);
    EXPECT_EQ(m3u8.method_, nullptr);
    EXPECT_EQ(m3u8.keyUri_, nullptr);
    uint8_t vec[4] {0, 0, 0, 0};
    EXPECT_NE(m3u8.iv_, vec);
}

HWTEST_F(M3u8UnitTest, SAVE_DATA_VALID_DATA, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint32_t len = 10;
    m3u8.isSessionKey_ = true;

    bool result = m3u8.SaveData(data, len, false, 0);

    EXPECT_TRUE(result);
    EXPECT_EQ(m3u8.keyLen_, len);
    EXPECT_TRUE(m3u8.isDecryptKeyReady_);
}

HWTEST_F(M3u8UnitTest, SAVE_DATA_INVALID_DATA, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isSessionKey_ = true;
    uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint32_t len = 10;

    bool result = m3u8.SaveData(data, len, false, 0);

    EXPECT_TRUE(result);
    EXPECT_EQ(m3u8.keyLen_, len);
    EXPECT_TRUE(m3u8.isDecryptKeyReady_);
}

HWTEST_F(M3u8UnitTest, SAVE_DATA_EXVEEDS_MAX_LOOP, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    uint8_t data[MAX_LOOP + 1];
    uint32_t len = MAX_LOOP + 1;

    bool result = m3u8.SaveData(data, len, false, 0);

    EXPECT_FALSE(result);
    EXPECT_EQ(m3u8.keyLen_, 0);
    EXPECT_FALSE(m3u8.isDecryptKeyReady_);
}

HWTEST_F(M3u8UnitTest, SET_DRM_INFO_NULL_KEY_URI, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    EXPECT_FALSE(m3u8.SetDrmInfo(drmInfo));
}

HWTEST_F(M3u8UnitTest, SET_DRM_INFO_INVALID_KEY_URI, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.keyUri_ = std::make_shared<std::string>("invalid");
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    EXPECT_FALSE(m3u8.SetDrmInfo(drmInfo));
}

HWTEST_F(M3u8UnitTest, SET_DRM_INFO_VALID_KEY_URI, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.keyUri_ = std::make_shared<std::string>("base64,valid");
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    EXPECT_FALSE(m3u8.SetDrmInfo(drmInfo));
}

HWTEST_F(M3u8UnitTest, SET_DRM_INFO_PSSH_SIZE_SMALL, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.keyUri_ = std::make_shared<std::string>("base64,valid");
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    EXPECT_FALSE(m3u8.SetDrmInfo(drmInfo));
}

HWTEST_F(M3u8UnitTest, SET_DRM_INFO_PSSH_SIZE_VALID, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.keyUri_ = std::make_shared<std::string>("base64,valid");
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    EXPECT_FALSE(m3u8.SetDrmInfo(drmInfo));
}

HWTEST_F(M3u8UnitTest, SET_DRM_INFOS, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;

    // test no DRM info
    m3u8.StoreDrmInfos(drmInfo);
    EXPECT_TRUE(m3u8.localDrmInfos_.empty());

    // test new drm info
    std::vector<uint8_t> pssh = {1, 2, 3, 4, 5};
    drmInfo.insert(std::make_pair("uuid1", pssh));
    m3u8.StoreDrmInfos(drmInfo);
    EXPECT_EQ(1, m3u8.localDrmInfos_.size());
    EXPECT_EQ(pssh, m3u8.localDrmInfos_.begin()->second);

    // test existed drm info
    drmInfo.insert(std::make_pair("uuid1", pssh));
    m3u8.StoreDrmInfos(drmInfo);
    EXPECT_EQ(1, m3u8.localDrmInfos_.size());
    EXPECT_EQ(pssh, m3u8.localDrmInfos_.begin()->second);

    // test diff drm info
    std::vector<uint8_t> pssh2 = {6, 7, 8, 9, 10};
    drmInfo.insert(std::make_pair("uuid1", pssh2));
    m3u8.StoreDrmInfos(drmInfo);
    EXPECT_EQ(2, m3u8.localDrmInfos_.size());
    EXPECT_EQ(pssh2, (++m3u8.localDrmInfos_.begin())->second);
}

HWTEST_F(M3u8UnitTest, TestPlaylistStartWithEXTM3U, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("#EXTM3U", "uri");
    playlist.StartParsing();
    EXPECT_EQ(playlist.playList_, "#EXTM3U");
}

HWTEST_F(M3u8UnitTest, TestPlaylistNotStartWithEXTM3U, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("playlist", "uri");
    playlist.StartParsing();
    EXPECT_EQ(playlist.playList_, "playlist");
}

HWTEST_F(M3u8UnitTest, TestPlaylistContainsEXTINF, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("\n#EXTM3U:", "uri");
    playlist.StartParsing();
    EXPECT_EQ(playlist.playList_, "\n#EXTM3U:");
}

HWTEST_F(M3u8UnitTest, TestPlaylistNotContainsEXTINF, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("playlist", "uri");
    playlist.StartParsing();
    EXPECT_EQ(playlist.playList_, "playlist");
}

HWTEST_F(M3u8UnitTest, UpdateMediaPlaylistTest, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("playlist", "uri");
    playlist.StartParsing();
    playlist.UpdateMediaPlaylist();

    // test variants contains new stream
    EXPECT_EQ(1, playlist.variants_.size());

    // test defaultVariant_ point new stream
    EXPECT_EQ(playlist.variants_.front(), playlist.defaultVariant_);

    // test duration updated
    EXPECT_EQ(0, playlist.duration_);

    // test bLive_ updated
    EXPECT_FALSE(playlist.bLive_);

    // isSimple updated
    EXPECT_TRUE(playlist.isSimple_);
}

HWTEST_F(M3u8UnitTest, UpdateMasterPlaylist_default, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("test", "http://test.com/test");
    playlist.StartParsing();
    playlist.UpdateMediaPlaylist();
    EXPECT_EQ(playlist.defaultVariant_, playlist.variants_.front());
}
// 测试 IsLive 方法
HWTEST_F(M3u8UnitTest, IS_LIVE_TEST, TestSize.Level1)
{
    M3U8 m3u8(testUri, "LivePlaylist");
    EXPECT_FALSE(m3u8.IsLive());
}

// 测试 M3U8Fragment 构造函数
HWTEST_F(M3u8UnitTest, M3U8FragmentConstructorTest, TestSize.Level1)
{
    double testDuration = 10.0;
    int testSequence = 1;
    bool testDiscont = false;
    M3U8Fragment fragment(testUri, testDuration, testSequence, testDiscont);
    ASSERT_EQ(fragment.uri_, testUri);
    ASSERT_DOUBLE_EQ(fragment.duration_, testDuration);
    ASSERT_EQ(fragment.sequence_, testSequence);
    ASSERT_EQ(fragment.discont_, testDiscont);
}

HWTEST_F(M3u8UnitTest, ParseKeyTest, TestSize.Level1)
{
    auto attributesTag = std::make_shared<AttributesTag>(HlsTag::EXTINF, "1234");
    auto uriAttr = std::make_shared<Attribute>("METHOD", "\"SAMPLE-AES\"");
    attributesTag->AddAttribute(uriAttr);
    auto uriAttr1 = std::make_shared<Attribute>("URI", "\"https://example.com/key\"");
    attributesTag->AddAttribute(uriAttr1);
    auto uriAttr2 = std::make_shared<Attribute>("IV", "\"0123456789ABCDEF\"");
    attributesTag->AddAttribute(uriAttr2);

    testM3u8->ParseKey(attributesTag);

    // 验证 method_, keyUri_ 和 iv_ 是否正确设置
    auto testStr = std::make_shared<std::string>("SAMPLE-AES");
    ASSERT_EQ(*(testM3u8->method_), *testStr);
    auto testStr1 = std::make_shared<std::string>("https://example.com/key");
    ASSERT_EQ(*(testM3u8->keyUri_), *testStr1);
}

HWTEST_F(M3u8UnitTest, SaveDataTest, TestSize.Level1)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    bool result = testM3u8->SaveData(data, sizeof(data), false, 0);
    ASSERT_TRUE(result);

    // 验证 key_ 成员变量中的数据是否与传入的数据一致
    for (size_t i = 0; i < sizeof(data); ++i) {
        ASSERT_EQ(testM3u8->key_[i], data[i]);
    }

    // 验证 keyLen_ 是否被正确设置
    ASSERT_EQ(testM3u8->keyLen_, sizeof(data));
}

HWTEST_F(M3u8UnitTest, SetDrmInfoTest, TestSize.Level1)
{
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    // 设置 keyUri_ 为有效的 base64 编码字符串
    testM3u8->keyUri_ = std::make_shared<std::string>("base64,VALID_BASE64_ENCODED_STRING");

    bool result = testM3u8->SetDrmInfo(drmInfo);
    ASSERT_FALSE(result);
    ASSERT_TRUE(drmInfo.empty());
    // 验证 drmInfo 是否包含预期的 UUID 和解码后的数据
}

HWTEST_F(M3u8UnitTest, StoreDrmInfosTest, TestSize.Level1)
{
    std::multimap<std::string, std::vector<uint8_t>> drmInfo = { { "uuid1", { 1, 2, 3 } }, { "uuid2", { 4, 5, 6 } } };
    testM3u8->StoreDrmInfos(drmInfo);
    // 验证 localDrmInfos_ 是否正确包含了 drmInfo 的内容
    for (const auto &item : drmInfo) {
        auto range = testM3u8->localDrmInfos_.equal_range(item.first);
        ASSERT_NE(range.first, testM3u8->localDrmInfos_.end());
        ASSERT_EQ(range.first->second, item.second);
    }
}

HWTEST_F(M3u8UnitTest, ProcessDrmInfosTest, TestSize.Level1)
{
    testM3u8->keyUri_ = std::make_shared<std::string>("base64,VALID_BASE64_ENCODED_STRING");
    testM3u8->ProcessDrmInfos();
    // 验证 isDecryptAble_ 是否根据 DRM 信息的处理结果正确设置
    ASSERT_EQ(testM3u8->isDecryptAble_, testM3u8->localDrmInfos_.empty());
}

HWTEST_F(M3u8UnitTest, GetLiveUpdateGap001, TestSize.Level1)
{
    M3U8 m3u8(testUri, "");
    size_t time = m3u8.GetLiveUpdateGap();
    EXPECT_EQ(0, time);
    m3u8.minFragDuration_ = 1;
    time = m3u8.GetLiveUpdateGap();
    EXPECT_EQ(0, time);
}

HWTEST_F(M3u8UnitTest, UPDATE_MEDIA_PLAYLIST_001, TestSize.Level1)
{
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com/key");
    master->StartParsing();
    master->UpdateMediaPlaylist();
    EXPECT_EQ(master->isSimple_, true);
}

HWTEST_F(M3u8UnitTest, CHOOSE_STREAM_BY_RESOLUTION_001, TestSize.Level1)
{
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com/key");
    master->StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("test", "https://example.com/key",
        std::make_shared<M3U8>("https://example.com/key", "test"));
    stream->bandWidth_ = 1000;
    stream->width_ = 480;
    stream->height_ = 640;
    master->variants_.emplace_back(stream);
    master->variants_.emplace_back(nullptr);
    auto stream1 = std::make_shared<M3U8VariantStream>("test", "https://example.com/key",
        std::make_shared<M3U8>("https://example.com/key", "test"));
    stream1->bandWidth_ = 2000;
    stream1->width_ = 480;
    stream1->height_ = 640;
    master->variants_.emplace_back(stream1);
    auto stream2 = std::make_shared<M3U8VariantStream>("test", "https://example.com/key",
        std::make_shared<M3U8>("https://example.com/key", "test"));
    stream2->bandWidth_ = 2000;
    stream2->width_ = 720;
    stream2->height_ = 1080;
    master->variants_.emplace_back(stream2);
    auto stream4 = std::make_shared<M3U8VariantStream>("test", "https://example.com/key",
        std::make_shared<M3U8>("https://example.com/key", "test"));
    stream4->bandWidth_ = 3000;
    stream4->width_ = 720;
    stream4->height_ = 1080;
    master->variants_.emplace_back(stream4);
    auto stream5 = std::make_shared<M3U8VariantStream>("test", "https://example.com/key",
        std::make_shared<M3U8>("https://example.com/key", "test"));
    stream4->bandWidth_ = 3000;
    stream4->width_ = 1080;
    stream4->height_ = 1920;
    master->variants_.emplace_back(stream4);
    master->defaultVariant_ = stream;
    master->ChooseStreamByResolution();
    uint32_t resolution = 480 * 720;
    master->initResolution_ = resolution;
    master->ChooseStreamByResolution();
    resolution = 720 * 1080;
    master->initResolution_ = resolution;
    master->ChooseStreamByResolution();
    resolution = 1080 * 1920;
    master->initResolution_ = resolution;
    master->ChooseStreamByResolution();
    EXPECT_EQ(master->defaultVariant_->width_, 1080);
    EXPECT_EQ(master->defaultVariant_->height_, 1920);
}

HWTEST_F(M3u8UnitTest, DOWNLOAD_STATUS_001, TestSize.Level1)
{
    M3U8 m3u8(testUri, "");
    std::shared_ptr<Downloader> downloader = std::make_shared<Downloader>("hlsPlayList");
    downloader->Init();
    std::shared_ptr<DownloadRequest> request = std::make_shared<DownloadRequest>("", nullptr, nullptr,  false);
    m3u8.OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, downloader, request);
    request->clientError_ = 52;
    m3u8.OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, downloader, request);
    request->serverError_ = 403;
    m3u8.OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, downloader, request);
    request->clientError_ = 0;
    m3u8.OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, downloader, request);
    EXPECT_EQ(request->serverError_, 403);
}

HWTEST_F(M3u8UnitTest, PARSE_MAP_001, TestSize.Level1)
{
    std::shared_ptr<AttributesTag> tag =
        std::make_shared<AttributesTag>(HlsTag::EXTXMAP, "#EXT-X-MAP:URI=\"main.mp4\",BYTERANGE=\"718@0\"");
    M3U8 m3u8(testUri, "");
    m3u8.isInterruptNeeded_ = true;
    m3u8.isHeaderReady_ = false;
    m3u8.ParseMap(tag);
    EXPECT_NE(m3u8.fmp4Header_, nullptr);
    EXPECT_EQ(m3u8.fmp4HeaderLen, 719u);

    M3U8 m3u81(testUri, "");
    m3u81.isHeaderReady_ = true;
    m3u81.ParseMap(tag);
    EXPECT_EQ(m3u81.fmp4Header_, nullptr);
    EXPECT_EQ(m3u81.fmp4HeaderLen, 0u);

    std::shared_ptr<AttributesTag> tag2 = std::make_shared<AttributesTag>(HlsTag::EXTXMAP, "#EXT-X-MAP:");
    M3U8 m3u82(testUri, "");
    m3u82.ParseMap(tag2);
    EXPECT_EQ(m3u82.fmp4Header_, nullptr);
}

HWTEST_F(M3u8UnitTest, IS_NEAR_TO_INIT_RESOLUTION_001, TestSize.Level1)
{
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com/key");
    master->StartParsing();
    auto m3u8 = std::make_shared<M3U8>(testUri, "");
    auto choosedStream = std::make_shared<M3U8VariantStream>(testUri, testUri, m3u8);
    auto currentStream = std::make_shared<M3U8VariantStream>(testUri, testUri, m3u8);
    EXPECT_EQ(master->IsNearToInitResolution(choosedStream, currentStream), false);
    EXPECT_EQ(master->IsNearToInitResolution(choosedStream, nullptr), false);
    EXPECT_EQ(master->IsNearToInitResolution(nullptr, nullptr), false);
}

HWTEST_F(M3u8UnitTest, SetInterruptState_001, TestSize.Level1)
{
    ScopedM3u8Download download;
    download.PrepareKey(INTERRUPT_KEY_PATH);
    MediaAVCodec::HttpServerDemo::RegisterContent(INTERRUPT_MAP_PATH, TEST_MAP_BYTES);
    const std::string url = KEY_TEST_BASE + INTERRUPT_MAP_PATH;
    auto m3u8 = download.Owner();
    ASSERT_GT(m3u8->keyIndex_ + m3u8->totalKeyIndex_, 0u);
    m3u8->DownloadKey(true);
    ASSERT_TRUE(download.WaitForKey());
    m3u8->DownloadMap(url, 0, 0);
    ASSERT_NE(m3u8->downloadHeaderRequest_, nullptr);
    ASSERT_NE(m3u8->downloadRequest_, nullptr);
    EXPECT_EQ(m3u8->downloadHeaderRequest_->startPos_, 0);
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", url);
    master->StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("test", url,
    std::make_shared<M3U8>(url, "test"));
    stream->streamId_ = 5;
    master->defaultVariant_ = stream;
    master->defaultVariant_->m3u8_ = nullptr;
    master->SetInterruptState(true);
    EXPECT_EQ(master->defaultVariant_->streamId_, 5u);
    master->defaultVariant_ = nullptr;
    master->SetInterruptState(true);
    EXPECT_EQ(master->defaultVariant_, nullptr);
    download.Stop();
    EXPECT_FALSE(download.HadError());
}

HWTEST_F(M3u8UnitTest, IsVideoStream_001, TestSize.Level1)
{
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com/key");
    master->StartParsing();
    std::string codecs = "mp4a.40.2,avc1.77.31";
    EXPECT_EQ(master->IsVideoStream(codecs), true);
    codecs = "mp4a.40.2";
    EXPECT_EQ(master->IsVideoStream(codecs), false);
    codecs = "avc1.77.31";
    EXPECT_EQ(master->IsVideoStream(codecs), true);
    codecs = "";
    EXPECT_EQ(master->IsVideoStream(codecs), false);
    codecs = "   ,   ,   ";
    EXPECT_EQ(master->IsVideoStream(codecs), false);
    codecs = "  ,  .  ,  .  ";
    EXPECT_EQ(master->IsVideoStream(codecs), false);
}

HWTEST_F(M3u8UnitTest, ParseAttributes_FRAME_RATE_001, TestSize.Level1)
{
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com/key");
    master->StartParsing();

    std::string attrField =
        "BANDWIDTH=1000000,RESOLUTION=1920x1080,CODECS=\"avc1.42E01E\",FRAME-RATE=30";
    auto item = std::make_shared<AttributesTag>(HlsTag::EXTXSTREAMINF, attrField);
    auto stream = std::make_shared<M3U8VariantStream>("test", "http://example.com/test.m3u8", nullptr);

    master->ParseAttributes(item, stream);

    EXPECT_EQ(stream->bandWidth_, 1000000);
    EXPECT_EQ(stream->width_, 1920);
    EXPECT_EQ(stream->height_, 1080);
    EXPECT_EQ(stream->frameRate_, 30);
}

HWTEST_F(M3u8UnitTest, ChooseStreamByStream_001, TestSize.Level1)
{
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com/key");
    master->StartParsing();

    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    auto stream1 = std::make_shared<M3U8VariantStream>("s1", "uri1", m3u8);
    auto stream2 = std::make_shared<M3U8VariantStream>("s2", "uri2", m3u8);
    auto stream3 = std::make_shared<M3U8VariantStream>("s3", "uri3", m3u8);

    stream1->streamId_ = 1;
    stream2->streamId_ = 2;
    stream3->streamId_ = 3;

    master->variants_.push_back(stream1);
    master->variants_.push_back(stream2);
    master->variants_.push_back(stream3);
    master->defaultVariant_ = stream1;

    master->ChooseStreamByStream(2);
    EXPECT_EQ(master->defaultVariant_, stream2);

    master->ChooseStreamByStream(-1);
    EXPECT_EQ(master->defaultVariant_, stream2);

    master->ChooseStreamByStream(100);
    EXPECT_EQ(master->defaultVariant_, stream2);
}


HWTEST_F(M3u8UnitTest, GetCodec_001, TestSize.Level1)
{
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com/key");
    master->StartParsing();

    std::string codecs = "mp4a.40.2,avc1.77.31";
    EXPECT_EQ(master->GetCodec(StreamType::VIDEO, codecs), "avc1.77.31");
    EXPECT_EQ(master->GetCodec(StreamType::AUDIO, codecs), "mp4a.40.2");

    codecs = "  mp4a.40.2  ,  hev1.1.6.l93.b0  ";
    EXPECT_EQ(master->GetCodec(StreamType::VIDEO, codecs), "hev1.1.6.l93.b0");
    EXPECT_EQ(master->GetCodec(StreamType::AUDIO, codecs), "mp4a.40.2");
}

HWTEST_F(M3u8UnitTest, GetCodec_002, TestSize.Level1)
{
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com/key");
    master->StartParsing();

    EXPECT_EQ(master->GetCodec(StreamType::VIDEO, ""), "");
    EXPECT_EQ(master->GetCodec(StreamType::AUDIO, ""), "");
    EXPECT_EQ(master->GetCodec(StreamType::VIDEO, "mp4a.40.2"), "");
    EXPECT_EQ(master->GetCodec(StreamType::AUDIO, "avc1.77.31"), "");
}

HWTEST_F(M3u8UnitTest, GetMimeType_001, TestSize.Level1)
{
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com/key");
    master->StartParsing();

    EXPECT_EQ(master->GetMimeType("avc1.77.31,mp4a.40.2"), "video/avc1");
    EXPECT_EQ(master->GetMimeType("  hev1.1.6.l93.b0  ,  mp4a.40.2"), "video/hev1");
}

HWTEST_F(M3u8UnitTest, GetMimeType_002, TestSize.Level1)
{
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com/key");
    master->StartParsing();

    EXPECT_EQ(master->GetMimeType(""), "");
    EXPECT_EQ(master->GetMimeType("mp4a.40.2"), "");
    EXPECT_EQ(master->GetMimeType("  ,  .  ,  "), "");
}

HWTEST_F(M3u8UnitTest, ADDFILE_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    M3U8Fragment fragment("http://example.com", 10.0, 1, false);
    m3u8.minFragDuration_ = 1;
    m3u8.isDecryptAble_ = true;
    m3u8.method_ = std::make_shared<std::string>("NONE");
    m3u8.keyIndex_ = 1;
    m3u8.AddFile(std::make_shared<M3U8Fragment>(fragment), 5);
    EXPECT_EQ(m3u8.files_.front()->keyIndex_, UINT64_MAX);
}

HWTEST_F(M3u8UnitTest, PREPAREDECRYPTIONKEYS_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "TestPlaylist");
    std::string testPlaylist = "#EXTM3U\n#EXT-X-KEY:METHOD=AES-128,URI=\"abcdata:textplain;base\"";
    EXPECT_TRUE(m3u8.Update(testPlaylist, true));
}

HWTEST_F(M3u8UnitTest, PREPAREDECRYPTIONKEYS_002, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "TestPlaylist");
    std::string testPlaylist = "#EXTM3U\n#EXT-X-KEY:METHOD=AES-128,URI=\"data:text/plain;base\"";
    EXPECT_TRUE(m3u8.Update(testPlaylist, true));
}

HWTEST_F(M3u8UnitTest, PARSE_DATE_RANGE_TAG_NULL_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "2024-01-01T00:00:00";
    m3u8.ParseDateRange(nullptr);
    EXPECT_TRUE(m3u8.dateRanges_.empty());
}

HWTEST_F(M3u8UnitTest, PARSE_DATE_RANGE_EMPTY_PROGRAM_DATE_TIME_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDATERANGE, "");
    m3u8.ParseDateRange(tag);
    EXPECT_TRUE(m3u8.dateRanges_.empty());
}

HWTEST_F(M3u8UnitTest, PARSE_DATE_RANGE_NO_ATTRIBUTES_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "2024-01-01T00:00:00";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDATERANGE, "");
    m3u8.ParseDateRange(tag);
    EXPECT_EQ(m3u8.dateRanges_.size(), 1u);
    EXPECT_TRUE(m3u8.dateRanges_.front()->id.empty());
    EXPECT_EQ(m3u8.dateRanges_.front()->duration, 0);
}

HWTEST_F(M3u8UnitTest, PARSE_DATE_RANGE_ALL_ATTRIBUTES_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "2024-01-01T00:00:00";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDATERANGE, "");
    auto idAttr = std::make_shared<Attribute>("ID", "\"test-id\"");
    tag->AddAttribute(idAttr);
    auto classAttr = std::make_shared<Attribute>("CLASS", "\"test-class\"");
    tag->AddAttribute(classAttr);
    auto startDateAttr = std::make_shared<Attribute>("START-DATE", "\"2024-01-01T00:01:00\"");
    tag->AddAttribute(startDateAttr);
    auto durationAttr = std::make_shared<Attribute>("DURATION", "60.5");
    tag->AddAttribute(durationAttr);
    auto assetListAttr = std::make_shared<Attribute>("X-ASSET-LIST", "\"http://example.com/assets\"");
    tag->AddAttribute(assetListAttr);
    m3u8.ParseDateRange(tag);
    EXPECT_EQ(m3u8.dateRanges_.size(), 1u);
    auto& dr = m3u8.dateRanges_.front();
    EXPECT_EQ(dr->id, "test-id");
    EXPECT_EQ(dr->classify, "test-class");
    EXPECT_EQ(dr->start, 60000);
    EXPECT_EQ(dr->duration, 60000);
    EXPECT_EQ(dr->assetList, "http://example.com/assets");
}

HWTEST_F(M3u8UnitTest, PARSE_DATE_RANGE_NULL_ATTR_IN_LIST_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "2024-01-01T00:00:00";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDATERANGE, "");
    auto idAttr = std::make_shared<Attribute>("ID", "\"test-id\"");
    tag->AddAttribute(idAttr);
    std::shared_ptr<Attribute> nullAttr = nullptr;
    tag->AddAttribute(nullAttr);
    m3u8.ParseDateRange(tag);
    EXPECT_EQ(m3u8.dateRanges_.size(), 1u);
    EXPECT_EQ(m3u8.dateRanges_.front()->contents.size(), 1u);
}

HWTEST_F(M3u8UnitTest, PARSE_DATE_RANGE_PARSE_FAIL_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "invalid-datetime";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDATERANGE, "");
    auto startDateAttr = std::make_shared<Attribute>("START-DATE", "\"1970-01-01T00:01:00\"");
    tag->AddAttribute(startDateAttr);
    m3u8.ParseDateRange(tag);
    EXPECT_EQ(m3u8.dateRanges_.size(), 1u);
    EXPECT_EQ(m3u8.dateRanges_.front()->start, 60000);
}

HWTEST_F(M3u8UnitTest, PARSE_DATE_RANGE_MULTIPLE_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "2024-01-01T00:00:00";
    auto tag1 = std::make_shared<AttributesTag>(HlsTag::EXTXDATERANGE, "");
    auto idAttr1 = std::make_shared<Attribute>("ID", "\"range1\"");
    tag1->AddAttribute(idAttr1);
    m3u8.ParseDateRange(tag1);
    auto tag2 = std::make_shared<AttributesTag>(HlsTag::EXTXDATERANGE, "");
    auto idAttr2 = std::make_shared<Attribute>("ID", "\"range2\"");
    tag2->AddAttribute(idAttr2);
    m3u8.ParseDateRange(tag2);
    EXPECT_EQ(m3u8.dateRanges_.size(), 2u);
}

HWTEST_F(M3u8UnitTest, CALCULATE_DATE_TIME_OFFSET_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    EXPECT_EQ(m3u8.CalculateDateTimeOffset("", ""), 0);
    EXPECT_EQ(m3u8.CalculateDateTimeOffset("invalid", "invalid"), 0);
    EXPECT_EQ(m3u8.CalculateDateTimeOffset("2024-01-01T00:00:00", "2024-01-01T00:01:00"), 60000);
    EXPECT_EQ(m3u8.CalculateDateTimeOffset("2024-01-01T00:00:00", "2024-01-02T00:00:00"), 86400000);
    EXPECT_EQ(m3u8.CalculateDateTimeOffset("2024-01-01T00:01:00", "2024-01-01T00:00:00"), -60000);
}

HWTEST_F(M3u8UnitTest, InitTagUpdatersMap_TableDriven_002, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    EXPECT_EQ(m3u8.tagUpdatersMap_.size(), 14);
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXPLAYLISTTYPE), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXTARGETDURATION), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXMEDIASEQUENCE), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXDISCONTINUITYSEQUENCE), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTINF), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::URI), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXBYTERANGE), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXDISCONTINUITY), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXKEY), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXMAP), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXSKIP), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXSTART), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXPROGRAMDATETIME), m3u8.tagUpdatersMap_.end());
    EXPECT_NE(m3u8.tagUpdatersMap_.find(HlsTag::EXTXDATERANGE), m3u8.tagUpdatersMap_.end());
}

HWTEST_F(M3u8UnitTest, UpdatePlaylistType_VOD, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.bLive_ = true;
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::EXTXPLAYLISTTYPE, "VOD");
    M3U8Info info;
    m3u8.UpdatePlaylistType(tag, info);
    EXPECT_FALSE(m3u8.bLive_);
}

HWTEST_F(M3u8UnitTest, UpdatePlaylistType_Event, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::EXTXPLAYLISTTYPE, "EVENT");
    M3U8Info info;
    m3u8.UpdatePlaylistType(tag, info);
    EXPECT_TRUE(m3u8.bLive_);
}

HWTEST_F(M3u8UnitTest, UpdatePlaylistType_NullTag, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.bLive_ = true;
    std::shared_ptr<Tag> nullTag;
    M3U8Info info;
    m3u8.UpdatePlaylistType(nullTag, info);
    EXPECT_TRUE(m3u8.bLive_);
}

HWTEST_F(M3u8UnitTest, UpdatePlaylistType_WrongSection, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.bLive_ = true;
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY, "METHOD=AES-128");
    M3U8Info info;
    m3u8.UpdatePlaylistType(tag, info);
    EXPECT_TRUE(m3u8.bLive_);
}

HWTEST_F(M3u8UnitTest, UpdateTargetDuration_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::EXTXTARGETDURATION, "10");
    M3U8Info info;
    m3u8.UpdateTargetDuration(tag, info);
    EXPECT_DOUBLE_EQ(m3u8.targetDuration_, 10.0);
}

HWTEST_F(M3u8UnitTest, UpdateTargetDuration_NullTag, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.targetDuration_ = 5.0;
    std::shared_ptr<Tag> nullTag;
    M3U8Info info;
    m3u8.UpdateTargetDuration(nullTag, info);
    EXPECT_DOUBLE_EQ(m3u8.targetDuration_, 5.0);
}

HWTEST_F(M3u8UnitTest, UpdateTargetDuration_WrongSection, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.targetDuration_ = 5.0;
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY, "METHOD=AES-128");
    M3U8Info info;
    m3u8.UpdateTargetDuration(tag, info);
    EXPECT_DOUBLE_EQ(m3u8.targetDuration_, 5.0);
}

HWTEST_F(M3u8UnitTest, UpdateMediaSequence_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::EXTXMEDIASEQUENCE, "123");
    M3U8Info info;
    m3u8.UpdateMediaSequence(tag, info);
    EXPECT_EQ(m3u8.sequence_, 123);
    EXPECT_EQ(m3u8.initSequence_, 123);
}

HWTEST_F(M3u8UnitTest, UpdateMediaSequence_NullTag, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.sequence_ = 100;
    m3u8.initSequence_ = 100;
    std::shared_ptr<Tag> nullTag;
    M3U8Info info;
    m3u8.UpdateMediaSequence(nullTag, info);
    EXPECT_EQ(m3u8.sequence_, 100);
    EXPECT_EQ(m3u8.initSequence_, 100);
}

HWTEST_F(M3u8UnitTest, UpdateDiscontSequence_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::EXTXDISCONTINUITYSEQUENCE, "5");
    M3U8Info info;
    m3u8.UpdateDiscontSequence(tag, info);
    EXPECT_EQ(m3u8.discontSequence_, 5);
    EXPECT_TRUE(info.discontinuity);
}

HWTEST_F(M3u8UnitTest, UpdateDiscontSequence_NullTag, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.discontSequence_ = 7;
    std::shared_ptr<Tag> nullTag;
    M3U8Info info;
    m3u8.UpdateDiscontSequence(nullTag, info);
    EXPECT_EQ(m3u8.discontSequence_, 7);
}

HWTEST_F(M3u8UnitTest, UpdateExtInf_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<ValuesListTag>(HlsTag::EXTINF, "3.5,");
    M3U8Info info;
    m3u8.UpdateExtInf(tag, info);
    EXPECT_DOUBLE_EQ(info.duration, 3.5);
}

HWTEST_F(M3u8UnitTest, UpdateExtInf_NullTag, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    M3U8Info info;
    info.duration = 2.0;
    std::shared_ptr<Tag> nullTag;
    m3u8.UpdateExtInf(nullTag, info);
    EXPECT_DOUBLE_EQ(info.duration, 2.0);
}

HWTEST_F(M3u8UnitTest, UpdateUri_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::URI, "test.ts");
    M3U8Info info;
    m3u8.UpdateUri(tag, info);
    EXPECT_FALSE(info.uri.empty());
}

HWTEST_F(M3u8UnitTest, UpdateUri_NullTag, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    M3U8Info info;
    info.uri = "http://x";
    std::shared_ptr<Tag> nullTag;
    m3u8.UpdateUri(nullTag, info);
    EXPECT_EQ(info.uri, "http://x");
}

HWTEST_F(M3u8UnitTest, UpdateByteRange_HeaderReady, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::EXTXBYTERANGE, "100@0");
    m3u8.isHeaderReady_.store(true);
    M3U8Info info;
    m3u8.UpdateByteRange(tag, info);
    EXPECT_EQ(m3u8.length_, 100);
    EXPECT_FALSE(m3u8.isPureByteRange_.load());
}

HWTEST_F(M3u8UnitTest, UpdateByteRange_NoHeaderReady, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::EXTXBYTERANGE, "100@0");
    m3u8.isHeaderReady_.store(false);
    M3U8Info info;
    m3u8.UpdateByteRange(tag, info);
    EXPECT_TRUE(m3u8.isPureByteRange_.load());
}

HWTEST_F(M3u8UnitTest, UpdateByteRange_NullTag, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.offset_ = 50;
    m3u8.length_ = 200;
    std::shared_ptr<Tag> nullTag;
    M3U8Info info;
    m3u8.UpdateByteRange(nullTag, info);
    EXPECT_EQ(m3u8.offset_, 50);
    EXPECT_EQ(m3u8.length_, 200);
}

HWTEST_F(M3u8UnitTest, UpdateDiscontinuity_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<Tag>(HlsTag::EXTXDISCONTINUITY);
    int origDiscont = m3u8.discontSequence_;
    M3U8Info info;
    m3u8.UpdateDiscontinuity(tag, info);
    EXPECT_EQ(m3u8.discontSequence_, origDiscont + 1);
    EXPECT_TRUE(info.discontinuity);
}

HWTEST_F(M3u8UnitTest, UpdateMap_InvalidInput, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    size_t origFiles = m3u8.files_.size();
    std::shared_ptr<Tag> nullTag;
    M3U8Info info;
    m3u8.UpdateMap(nullTag, info);
    EXPECT_EQ(m3u8.files_.size(), origFiles);
    std::shared_ptr<Tag> wrongTag = std::make_shared<SingleValueTag>(HlsTag::URI, "test.ts");
    m3u8.UpdateMap(wrongTag, info);
    EXPECT_EQ(m3u8.files_.size(), origFiles);
}

HWTEST_F(M3u8UnitTest, UpdateSkip_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXSKIP, "SKIPPED-SEGMENTS=3");
    M3U8Info info;
    m3u8.UpdateSkip(tag, info);
    EXPECT_EQ(m3u8.skipSegmentsNum_, 3);
}

HWTEST_F(M3u8UnitTest, UpdateSkip_NullTag, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.skipSegmentsNum_ = 10;
    std::shared_ptr<Tag> nullTag;
    M3U8Info info;
    m3u8.UpdateSkip(nullTag, info);
    EXPECT_EQ(m3u8.skipSegmentsNum_, 10);
}

HWTEST_F(M3u8UnitTest, UpdateSkip_WrongSection, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.skipSegmentsNum_ = 10;
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::URI, "test.ts");
    M3U8Info info;
    m3u8.UpdateSkip(tag, info);
    EXPECT_EQ(m3u8.skipSegmentsNum_, 10);
}

HWTEST_F(M3u8UnitTest, UpdateStart_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXSTART, "TIME-OFFSET=10.5,PRECISE=YES");
    M3U8Info info;
    m3u8.UpdateStart(tag, info);
    EXPECT_DOUBLE_EQ(m3u8.timeOffset_, 10.5);
    EXPECT_TRUE(m3u8.precise_);
    EXPECT_TRUE(m3u8.isStart_);
}

HWTEST_F(M3u8UnitTest, UpdateStart_NullTag, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.timeOffset_ = 8.0;
    m3u8.isStart_ = true;
    std::shared_ptr<Tag> nullTag;
    M3U8Info info;
    m3u8.UpdateStart(nullTag, info);
    EXPECT_DOUBLE_EQ(m3u8.timeOffset_, 8.0);
    EXPECT_TRUE(m3u8.isStart_);
}

HWTEST_F(M3u8UnitTest, UpdateStart_WrongSection, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.timeOffset_ = 8.0;
    m3u8.isStart_ = true;
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::URI, "test.ts");
    M3U8Info info;
    m3u8.UpdateStart(tag, info);
    EXPECT_DOUBLE_EQ(m3u8.timeOffset_, 8.0);
    EXPECT_TRUE(m3u8.isStart_);
}

HWTEST_F(M3u8UnitTest, UpdateProgramDateTime_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::EXTXPROGRAMDATETIME, "2024-01-01T00:00:00Z");
    M3U8Info info;
    m3u8.UpdateProgramDateTime(tag, info);
    EXPECT_EQ(m3u8.currentProgramDateTime_, "2024-01-01T00:00:00Z");
}

HWTEST_F(M3u8UnitTest, UpdateProgramDateTime_NullTag, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "2023-01-01";
    std::shared_ptr<Tag> nullTag;
    M3U8Info info;
    m3u8.UpdateProgramDateTime(nullTag, info);
    EXPECT_EQ(m3u8.currentProgramDateTime_, "2023-01-01");
}

HWTEST_F(M3u8UnitTest, UpdateDateRange_InvalidInput, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    size_t origRange = m3u8.dateRanges_.size();
    std::shared_ptr<Tag> nullTag;
    M3U8Info info;
    m3u8.UpdateDateRange(nullTag, info);
    EXPECT_EQ(m3u8.dateRanges_.size(), origRange);
    std::shared_ptr<Tag> wrongTag = std::make_shared<SingleValueTag>(HlsTag::URI, "test.ts");
    m3u8.UpdateDateRange(wrongTag, info);
    EXPECT_EQ(m3u8.dateRanges_.size(), origRange);
}

HWTEST_F(M3u8UnitTest, MasterPlaylist_WrongSection, TestSize.Level0)
{
    M3U8MasterPlaylist playlist("", "https://example.com/test");
    playlist.StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("test", "https://example.com/key",
        std::make_shared<M3U8>("https://example.com/key", "test"));
    playlist.variants_.emplace_back(stream);
    playlist.isStart_ = true;
    std::shared_ptr<Tag> wrongTag = std::make_shared<Tag>(HlsTag::EXTXDISCONTINUITY);
    size_t origVariantSize = playlist.variants_.size();
    playlist.CreateVariantStream(wrongTag);
    EXPECT_EQ(playlist.variants_.size(), origVariantSize);
    playlist.ParseMediaStreamInfo(wrongTag);
    EXPECT_EQ(playlist.variants_.size(), origVariantSize);
    playlist.ParseStart(wrongTag);
    EXPECT_TRUE(playlist.isStart_);
}

HWTEST_F(M3u8UnitTest, KeyInfos_Secure_Cleanup_In_Destructor, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    uint8_t testKey[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    for (uint64_t i = 1; i <= 3; ++i) {
        KeyInfo keyInfo;
        keyInfo.index_ = i;
        (void)memcpy_s(keyInfo.key_, sizeof(keyInfo.key_), testKey, sizeof(testKey));
        keyInfo.keyLen_ = sizeof(testKey);
        m3u8->keyInfos_[i] = keyInfo;
    }
    EXPECT_EQ(m3u8->keyInfos_.size(), 3);
    for (const auto& [index, keyInfo] : m3u8->keyInfos_) {
        EXPECT_EQ(memcmp(keyInfo.key_, testKey, sizeof(testKey)), 0);
    }
    m3u8.reset();
    EXPECT_EQ(m3u8, nullptr);
}

HWTEST_F(M3u8UnitTest, InitializeDownloadHeader_LengthGreaterThanZero_001, TestSize.Level1)
{
    std::string url = "http://example.com/test.m3u8";
    auto m3u8 = std::make_shared<M3U8>(testUri, "");
    m3u8->uri_ = url;
    m3u8->DownloadMap(url, 100, 718);
    EXPECT_NE(m3u8->downloadHeaderRequest_, nullptr);
    EXPECT_EQ(m3u8->downloadHeaderRequest_->startPos_, 100);
    EXPECT_EQ(m3u8->downloadHeaderRequest_->endPos_, 817);
}

HWTEST_F(M3u8UnitTest, InitializeDownloadHeader_LengthEqualsZero_001, TestSize.Level1)
{
    std::string url = "http://example.com/test.m3u8";
    auto m3u8 = std::make_shared<M3U8>(testUri, "");
    m3u8->uri_ = url;
    m3u8->DownloadMap(url, 0, 0);
    EXPECT_NE(m3u8->downloadHeaderRequest_, nullptr);
    EXPECT_EQ(m3u8->downloadHeaderRequest_->startPos_, 0);
    EXPECT_EQ(m3u8->downloadHeaderRequest_->endPos_, -1);
}

HWTEST_F(M3u8UnitTest, InitializeDownloadHeader_OffsetLengthOverflow_001, TestSize.Level1)
{
    std::string url = "http://example.com/test.m3u8";
    auto m3u8 = std::make_shared<M3U8>(testUri, "");
    m3u8->uri_ = url;
    m3u8->DownloadMap(url, SIZE_MAX, 1);
    EXPECT_EQ(m3u8->downloadHeaderRequest_->startPos_, 0);
    EXPECT_EQ(m3u8->downloadHeaderRequest_->endPos_, -1);
}
/**
 * @tc.name  : Test hasExplicitIv_ set by ParseKey with IV attribute
 * @tc.number: HasExplicitIv_ParseKey_IV_Attribute
 * @tc.desc  : Test ParseKey sets hasExplicitIv_ = true when IV attribute present
 */
HWTEST_F(M3u8UnitTest, HasExplicitIv_ParseKey_IV_Attribute, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    std::string keyTagAttr = "METHOD=AES-128,URI=\"http://key.example.com/key\",IV=0x0102030405060708090a0b0c0d0e0f10";
    std::shared_ptr<AttributesTag> keyTag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY, keyTagAttr);
    m3u8->ParseKey(keyTag);
    EXPECT_TRUE(m3u8->hasExplicitIv_.load());
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(m3u8->iv_[i], i + 1);
    }
}

/**
 * @tc.name  : Test hasExplicitIv_ false without IV attribute
 * @tc.number: HasExplicitIv_ParseKey_No_IV
 * @tc.desc  : Test ParseKey keeps hasExplicitIv_ = false when no IV attribute
 */
HWTEST_F(M3u8UnitTest, HasExplicitIv_ParseKey_No_IV, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    std::string keyTagAttr = "METHOD=AES-128,URI=\"http://key.example.com/key\"";
    std::shared_ptr<AttributesTag> keyTag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY, keyTagAttr);
    m3u8->ParseKey(keyTag);
    EXPECT_FALSE(m3u8->hasExplicitIv_.load());
}

/**
 * @tc.name  : Test ProcessInfo per-fragment IV without explicit IV
 * @tc.number: ProcessInfo_PerFragment_IV_NoExplicit
 * @tc.desc  : Test ProcessInfo computes per-fragment IV from sequence number when hasExplicitIv_ = false
 */
HWTEST_F(M3u8UnitTest, ProcessInfo_PerFragment_IV_NoExplicit, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    m3u8->isDecryptAble_ = true;
    m3u8->hasExplicitIv_.store(false);
    m3u8->sequence_ = 1;
    KeyInfo keyInfo;
    keyInfo.index_ = 0;
    uint8_t keyData[16] = {1};
    (void)memcpy_s(keyInfo.key_, sizeof(keyInfo.key_), keyData, sizeof(keyData));
    keyInfo.keyLen_ = 16;
    m3u8->keyInfos_[0] = keyInfo;
    m3u8->isDecryptKeyReady_.store(true);
    M3U8Info info;
    info.uri = "http://example.com/seg1.ts";
    info.duration = 10.0;
    info.discontinuity = false;
    size_t duration = 0;
    m3u8->ProcessInfo(info, duration);
    EXPECT_EQ(m3u8->sequence_, 2u);
    EXPECT_FALSE(info.uri == "http://example.com/seg1.ts");
    EXPECT_EQ(info.duration, 0.0);
    EXPECT_EQ(duration, static_cast<size_t>(10.0 * 1000.0 * 1000.0));
    EXPECT_EQ(m3u8->files_.size(), 1u);
}

/**
 * @tc.name  : Test ProcessInfo with explicit IV
 * @tc.number: ProcessInfo_Explicit_IV
 * @tc.desc  : Test ProcessInfo uses iv_ directly when hasExplicitIv_ = true
 */
HWTEST_F(M3u8UnitTest, ProcessInfo_Explicit_IV, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    m3u8->isDecryptAble_ = true;
    m3u8->hasExplicitIv_.store(true);
    uint8_t expectedIv[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    (void)memcpy_s(m3u8->iv_, sizeof(m3u8->iv_), expectedIv, sizeof(expectedIv));
    m3u8->sequence_ = 100;
    KeyInfo keyInfo;
    keyInfo.index_ = 0;
    uint8_t keyData[16] = {1};
    (void)memcpy_s(keyInfo.key_, sizeof(keyInfo.key_), keyData, sizeof(keyData));
    keyInfo.keyLen_ = 16;
    m3u8->keyInfos_[0] = keyInfo;
    m3u8->isDecryptKeyReady_.store(true);
    M3U8Info info;
    info.uri = "http://example.com/seg1.ts";
    info.duration = 10.0;
    info.discontinuity = false;
    size_t duration = 0;
    m3u8->ProcessInfo(info, duration);
    EXPECT_EQ(m3u8->sequence_, 101u);
    EXPECT_FALSE(info.uri == "http://example.com/seg1.ts");
    EXPECT_EQ(info.duration, 0.0);
    EXPECT_EQ(duration, static_cast<size_t>(10.0 * 1000.0 * 1000.0));
    EXPECT_EQ(m3u8->files_.size(), 1u);
}

/**
 * @tc.name  : Test ProcessInfo with isDecryptAble_ = false
 * @tc.number: ProcessInfo_NoDecrypt
 * @tc.desc  : Test ProcessInfo creates plain fragment without key/IV when isDecryptAble_ = false
 */
HWTEST_F(M3u8UnitTest, ProcessInfo_NoDecrypt, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    m3u8->isDecryptAble_ = false;
    m3u8->sequence_ = 1;
    M3U8Info info;
    info.uri = "http://example.com/seg1.ts";
    info.duration = 10.0;
    info.discontinuity = false;
    size_t duration = 0;
    m3u8->ProcessInfo(info, duration);
    EXPECT_EQ(m3u8->sequence_, 2u);
    EXPECT_FALSE(info.uri == "http://example.com/seg1.ts");
    EXPECT_EQ(info.duration, 0.0);
    EXPECT_EQ(duration, static_cast<size_t>(10.0 * 1000.0 * 1000.0));
    EXPECT_EQ(m3u8->files_.size(), 1u);
    EXPECT_TRUE(m3u8->isFirstFragmentReady_.load());
}

/**
 * @tc.name  : Test isSessionKey_ SaveData path
 * @tc.number: IsSessionKey_SaveData
 * @tc.desc  : Test SaveData with isSessionKey_ = true skips keyMutex_ and keyInfos_
 */
HWTEST_F(M3u8UnitTest, IsSessionKey_SaveData, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    m3u8->isSessionKey_ = true;
    uint8_t data[16] = {0x01};
    bool result = m3u8->SaveData(data, 16, false, 0);
    EXPECT_TRUE(result);
    EXPECT_TRUE(m3u8->isDecryptKeyReady_.load());
}

/**
 * @tc.name  : Test SaveData with invalid length
 * @tc.number: SaveData_Invalid_Length
 * @tc.desc  : Test SaveData returns 0 when len > MAX_LOOP or len == 0
 */
HWTEST_F(M3u8UnitTest, SaveData_Invalid_Length, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    uint8_t data[32] = {0};
    bool result = m3u8->SaveData(data, 32, false, 0);
    EXPECT_FALSE(result);

    result = m3u8->SaveData(data, 0, false, 0);
    EXPECT_FALSE(result);
}

/**
 * @tc.name  : Test ProcessInfo sequence normal increment
 * @tc.number: ProcessInfo_Sequence_Normal_Increment
 * @tc.desc  : Test ProcessInfo increments sequence_ when sequence_ < uint64 max
 */
HWTEST_F(M3u8UnitTest, ProcessInfo_Sequence_Normal_Increment, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    m3u8->isDecryptAble_ = false;
    m3u8->sequence_ = 1;
    M3U8Info info;
    info.uri = "http://example.com/seg1.ts";
    info.duration = 10.0;
    info.discontinuity = false;
    size_t duration = 0;
    m3u8->ProcessInfo(info, duration);
    EXPECT_EQ(m3u8->sequence_, 2u);
    EXPECT_EQ(m3u8->files_.size(), 1u);
}

/**
 * @tc.name  : Test ProcessInfo sequence overflow skips increment
 * @tc.number: ProcessInfo_Sequence_Overflow
 * @tc.desc  : Test ProcessInfo skips increment when sequence_ == uint64 max
 */
HWTEST_F(M3u8UnitTest, ProcessInfo_Sequence_Overflow, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    m3u8->isDecryptAble_ = false;
    m3u8->sequence_ = std::numeric_limits<uint64_t>::max();
    M3U8Info info;
    info.uri = "http://example.com/seg1.ts";
    info.duration = 10.0;
    info.discontinuity = false;
    size_t duration = 0;
    m3u8->ProcessInfo(info, duration);
    EXPECT_EQ(m3u8->sequence_, std::numeric_limits<uint64_t>::max());
    EXPECT_EQ(m3u8->files_.size(), 1u);
}

/**
 * @tc.name  : Test ProcessInfo duration normal accumulation
 * @tc.number: ProcessInfo_Duration_Normal_Accumulation
 * @tc.desc  : Test ProcessInfo accumulates duration when no overflow
 */
HWTEST_F(M3u8UnitTest, ProcessInfo_Duration_Normal_Accumulation, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    m3u8->isDecryptAble_ = false;
    m3u8->sequence_ = 1;
    M3U8Info info;
    info.uri = "http://example.com/seg1.ts";
    info.duration = 10.0;
    info.discontinuity = false;
    size_t duration = 0;
    m3u8->ProcessInfo(info, duration);
    EXPECT_EQ(duration, static_cast<size_t>(10.0 * 1000.0 * 1000.0));
    EXPECT_EQ(m3u8->files_.size(), 1u);
}

/**
 * @tc.name  : Test ProcessInfo duration overflow skips accumulation
 * @tc.number: ProcessInfo_Duration_Overflow
 * @tc.desc  : Test ProcessInfo skips duration accumulation when overflow would occur
 */
HWTEST_F(M3u8UnitTest, ProcessInfo_Duration_Overflow, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    m3u8->isDecryptAble_ = false;
    m3u8->sequence_ = 1;
    M3U8Info info;
    info.uri = "http://example.com/seg1.ts";
    info.duration = 10.0;
    info.discontinuity = false;
    size_t duration = std::numeric_limits<size_t>::max();
    m3u8->ProcessInfo(info, duration);
    EXPECT_EQ(duration, std::numeric_limits<size_t>::max());
    EXPECT_EQ(m3u8->files_.size(), 1u);
}

HWTEST_F(M3u8UnitTest, ExtractNumber_AllBranches_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    uint64_t maxVal = std::numeric_limits<uint64_t>::max();
    EXPECT_EQ(m3u8.ExtractNumber(""), maxVal);
    EXPECT_EQ(m3u8.ExtractNumber("no_dot_here"), maxVal);
    EXPECT_EQ(m3u8.ExtractNumber("seg123.ts"), 123u);
    EXPECT_EQ(m3u8.ExtractNumber("no_digits.ts"), maxVal);
    EXPECT_EQ(m3u8.ExtractNumber("seg456.ts?q=1"), 456u);
    EXPECT_EQ(m3u8.ExtractNumber("http://x?q=1"), maxVal);
}

HWTEST_F(M3u8UnitTest, ShouldSkipSegment_AllBranches_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    EXPECT_FALSE(m3u8.ShouldSkipSegment("seg5.ts", 0, 5));
    EXPECT_TRUE(m3u8.ShouldSkipSegment("seg5.ts", 3, 5));
    EXPECT_FALSE(m3u8.ShouldSkipSegment("seg8.ts", 3, 5));
    EXPECT_FALSE(m3u8.ShouldSkipSegment("seg4.ts", 3, 5));
}

HWTEST_F(M3u8UnitTest, SetDrmInfo_ValidPssh_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.keyUri_ = std::make_shared<std::string>("base64," + std::string(38, 'A') + "==");
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    EXPECT_TRUE(m3u8.SetDrmInfo(drmInfo));
    EXPECT_EQ(drmInfo.size(), 1u);
    EXPECT_EQ(drmInfo.begin()->first, std::string(32, '0'));
    EXPECT_EQ(drmInfo.begin()->second.size(), 28u);
}

HWTEST_F(M3u8UnitTest, ProcessDrmInfos_Success_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.keyUri_ = std::make_shared<std::string>("base64," + std::string(38, 'A') + "==");
    m3u8.ProcessDrmInfos();
    EXPECT_FALSE(m3u8.isDecryptAble_);
    EXPECT_EQ(m3u8.localDrmInfos_.size(), 1u);
}

HWTEST_F(M3u8UnitTest, SaveMapData_AllBranches_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    EXPECT_EQ(m3u8.SaveMapData(nullptr, 10, false), 0u);
    uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    EXPECT_EQ(m3u8.SaveMapData(data, 10, false), 0u);
    m3u8.fmp4Header_ = new uint8_t[16];
    m3u8.fmp4HeaderLen = 16;
    EXPECT_EQ(m3u8.SaveMapData(data, 10, false), 10u);
    EXPECT_EQ(m3u8.downloadHeaderLen_, 10u);
    uint8_t data2[10] = {2};
    EXPECT_EQ(m3u8.SaveMapData(data2, 10, false), 6u);
    EXPECT_EQ(m3u8.downloadHeaderLen_, 16u);
    EXPECT_EQ(m3u8.SaveMapData(data2, 10, false), 0u);
    EXPECT_EQ(m3u8.SaveMapData(data, 0, false), 0u);
    delete[] m3u8.fmp4Header_;
    m3u8.fmp4Header_ = nullptr;
    m3u8.fmp4HeaderLen = 0;
    m3u8.downloadHeaderLen_ = 0;
}

HWTEST_F(M3u8UnitTest, ParseKey_LargeIv_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY, "METHOD=AES-128,IV=0x" + std::string(40, '1'));
    m3u8.ParseKey(tag);
    EXPECT_TRUE(m3u8.hasExplicitIv_.load());
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(m3u8.iv_[i], 0x11);
    }
}

HWTEST_F(M3u8UnitTest, SaveData_KeyIndexOutOfBounds_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isSessionKey_ = false;
    uint8_t data[16] = {1};
    EXPECT_EQ(m3u8.SaveData(data, 16, false, 5), 16u);
    EXPECT_EQ(m3u8.keyLen_, 16u);
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(m3u8.key_[i], data[i]);
    }
}

HWTEST_F(M3u8UnitTest, DownloadKey_NullKeyUri_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.keyUri_ = nullptr;
    m3u8.DownloadKey(true);
    EXPECT_EQ(m3u8.downloader_, nullptr);
    EXPECT_EQ(m3u8.keyLen_, 0u);
    EXPECT_FALSE(m3u8.isDecryptKeyReady_.load());
}

HWTEST_F(M3u8UnitTest, Update_MasterPlaylistDetected_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::string playList = "#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=1000\nhttp://x.m3u8";
    EXPECT_FALSE(m3u8.Update(playList, true));
}

HWTEST_F(M3u8UnitTest, Update_NoCleanFiles_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::string playList = "#EXTM3U\n#EXT-X-TARGETDURATION:10\n#EXTINF:5.0,\nseg.ts\n#EXT-X-ENDLIST";
    EXPECT_TRUE(m3u8.Update(playList, false));
    EXPECT_FALSE(m3u8.files_.empty());
    EXPECT_FALSE(m3u8.bLive_);
}

HWTEST_F(M3u8UnitTest, Update_WithDefine_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8?token=abc", "");
    m3u8.tagMasterMap_ = {{"KEY", "VAL"}};
    std::string playList = "#EXTM3U\n#EXT-X-DEFINE:IMPORT=KEY\n#EXTINF:5.0,\nseg.ts";
    EXPECT_TRUE(m3u8.Update(playList, true));
    EXPECT_FALSE(m3u8.files_.empty());
}

HWTEST_F(M3u8UnitTest, UpdateFromTags_Interrupt_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::list<std::shared_ptr<Tag>> tags;
    tags.emplace_back(std::make_shared<Tag>(HlsTag::EXTXENDLIST));
    tags.emplace_back(std::make_shared<SingleValueTag>(HlsTag::URI, "seg.ts"));
    m3u8.isInterruptNeeded_ = true;
    m3u8.UpdateFromTags(tags);
    EXPECT_EQ(m3u8.files_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, UpdateFromTags_SkipSegment_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.skipSegmentsNum_ = 3;
    m3u8.initSequence_ = 5;
    std::list<std::shared_ptr<Tag>> tags;
    tags.emplace_back(std::make_shared<SingleValueTag>(HlsTag::URI, "seg5.ts"));
    m3u8.UpdateFromTags(tags);
    EXPECT_EQ(m3u8.files_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, UpdateFromTags_VariousTags_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::list<std::shared_ptr<Tag>> tags;
    tags.emplace_back(std::make_shared<Tag>(HlsTag::EXTXENDLIST));
    tags.emplace_back(std::make_shared<Tag>(HlsTag::EXTXDISCONTINUITY));
    tags.emplace_back(std::make_shared<SingleValueTag>(HlsTag::URI, "seg.ts"));
    m3u8.UpdateFromTags(tags);
    EXPECT_TRUE(m3u8.hasDiscontinuity_);
    EXPECT_EQ(m3u8.files_.size(), 1u);
    EXPECT_FALSE(m3u8.bLive_);
}

HWTEST_F(M3u8UnitTest, UpdateFromTags_WithExistingFiles_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.files_.emplace_back(std::make_shared<M3U8Fragment>("http://x", 5.0, 1, false));
    std::list<std::shared_ptr<Tag>> tags;
    m3u8.UpdateFromTags(tags);
    EXPECT_EQ(m3u8.files_.size(), 1u);
}

HWTEST_F(M3u8UnitTest, CreateVariantStream_Valid_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::string attr = "URI=\"http://example.com/v1.m3u8\",BANDWIDTH=1000,CODECS=\"avc1.42E01E\"";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXSTREAMINF, attr);
    playlist.CreateVariantStream(tag);
    EXPECT_EQ(playlist.variants_.size(), 1u);
    EXPECT_TRUE(playlist.variants_.front()->isVideo_);
    EXPECT_NE(playlist.defaultVariant_, nullptr);
    EXPECT_NE(playlist.firstVideoStream_, nullptr);
}

HWTEST_F(M3u8UnitTest, CreateVariantStream_IFrame_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::string attr = "URI=\"http://example.com/v1.m3u8\",BANDWIDTH=1000";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXIFRAMESTREAMINF, attr);
    playlist.CreateVariantStream(tag);
    EXPECT_EQ(playlist.variants_.size(), 1u);
    EXPECT_TRUE(playlist.variants_.front()->iframe_);
}

HWTEST_F(M3u8UnitTest, ParseMediaStreamInfo_ValidAudio_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::string attr = "URI=\"http://example.com/a.m3u8\",TYPE=AUDIO,GROUP-ID=\"audio1\",NAME=\"eng\","
        "LANGUAGE=\"en\",DEFAULT=YES,AUTOSELECT=YES,FORCED=YES,CHANNELS=\"2\",INSTREAM-ID=\"CC1\","
        "CHARACTERISTICS=\"public\"";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXMEDIA, attr);
    std::shared_ptr<Tag> tagPtr = tag;
    playlist.ParseMediaStreamInfo(tagPtr);
    EXPECT_EQ(playlist.audioList_.size(), 1u);
    auto& media = playlist.audioList_.front();
    EXPECT_EQ(media->groupID_, "audio1");
    EXPECT_EQ(media->name_, "eng");
    EXPECT_EQ(media->lang_, "en");
    EXPECT_EQ(media->channels_, "2");
    EXPECT_EQ(media->instreamId_, "CC1");
    EXPECT_EQ(media->characteristics_, "public");
    EXPECT_TRUE(media->isDefault_);
    EXPECT_TRUE(media->autoSelect_);
    EXPECT_TRUE(media->forced_);
    EXPECT_EQ(media->type_, M3U8MediaType::M3U8_MEDIA_TYPE_AUDIO);
}

HWTEST_F(M3u8UnitTest, ParseMediaStreamInfo_Subtitles_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::string attr = "URI=\"http://example.com/s.m3u8\",TYPE=SUBTITLES,GROUP-ID=\"sub1\",NAME=\"es\"";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXMEDIA, attr);
    std::shared_ptr<Tag> tagPtr = tag;
    playlist.ParseMediaStreamInfo(tagPtr);
    EXPECT_EQ(playlist.subtitlesList_.size(), 1u);
    EXPECT_EQ(playlist.subtitlesList_.front()->type_, M3U8MediaType::M3U8_MEDIA_TYPE_SUBTITLES);
}

HWTEST_F(M3u8UnitTest, ParseStart_Valid_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::string attr = "TIME-OFFSET=15,PRECISE=YES";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXSTART, attr);
    std::shared_ptr<Tag> tagPtr = tag;
    playlist.ParseStart(tagPtr);
    EXPECT_DOUBLE_EQ(playlist.timeOffset_, 15.0);
    EXPECT_TRUE(playlist.precise_);
    EXPECT_TRUE(playlist.isStart_);
}

HWTEST_F(M3u8UnitTest, BindVideoMedia_EmptyAudioId_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto media = std::make_shared<M3U8Media>("a", "http://example.com/a.m3u8", nullptr);
    media->groupID_ = "audio1";
    media->forced_ = true;
    playlist.audioList_.push_back(media);
    auto stream = std::make_shared<M3U8VariantStream>("v", "http://example.com/v.m3u8", nullptr);
    playlist.variants_.push_back(stream);
    playlist.BindVideoMedia("audio");
    EXPECT_EQ(stream->audioMedia_.size(), 1u);
    EXPECT_EQ(stream->defaultAudio_, media);
}

HWTEST_F(M3u8UnitTest, BindVideoMedia_MatchedGroup_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto media = std::make_shared<M3U8Media>("a", "uri", nullptr);
    media->groupID_ = "grp1";
    media->isDefault_ = true;
    playlist.audioList_.push_back(media);
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    stream->codecs_ = "avc1";
    stream->audio_ = "grp1";
    playlist.variants_.push_back(stream);
    playlist.BindVideoMedia("audio");
    EXPECT_EQ(stream->audioMedia_.size(), 1u);
    EXPECT_EQ(stream->audioMedia_.front()->originCodec_, "avc1");
}

HWTEST_F(M3u8UnitTest, BindVideoMedia_UnmatchedGroup_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto media = std::make_shared<M3U8Media>("a", "uri", nullptr);
    media->groupID_ = "grp1";
    playlist.audioList_.push_back(media);
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    stream->audio_ = "grp2";
    playlist.variants_.push_back(stream);
    playlist.BindVideoMedia("audio");
    EXPECT_EQ(stream->audioMedia_.size(), 1u);
    EXPECT_EQ(stream->audioMedia_.front(), media);
}

HWTEST_F(M3u8UnitTest, GetDefaultMedieStream_Forced_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    auto media1 = std::make_shared<M3U8Media>("a", "uri", nullptr);
    auto media2 = std::make_shared<M3U8Media>("b", "uri", nullptr);
    media2->forced_ = true;
    stream->audioMedia_ = {media1, media2};
    playlist.GetDefaultMedieStream(stream, "audio");
    EXPECT_EQ(stream->defaultAudio_, media2);
}

HWTEST_F(M3u8UnitTest, GetDefaultMedieStream_Default_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    auto media1 = std::make_shared<M3U8Media>("a", "uri", nullptr);
    media1->autoSelect_ = true;
    auto media2 = std::make_shared<M3U8Media>("b", "uri", nullptr);
    media2->isDefault_ = true;
    stream->audioMedia_ = {media1, media2};
    playlist.GetDefaultMedieStream(stream, "audio");
    EXPECT_EQ(stream->defaultAudio_, media2);
}

HWTEST_F(M3u8UnitTest, GetDefaultMedieStream_AutoSelect_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    auto media1 = std::make_shared<M3U8Media>("a", "uri", nullptr);
    media1->autoSelect_ = true;
    stream->audioMedia_ = {media1};
    playlist.GetDefaultMedieStream(stream, "audio");
    EXPECT_EQ(stream->defaultAudio_, media1);
}

HWTEST_F(M3u8UnitTest, GetDefaultMedieStream_Fallback_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    auto media1 = std::make_shared<M3U8Media>("a", "uri", nullptr);
    stream->audioMedia_ = {media1};
    playlist.GetDefaultMedieStream(stream, "audio");
    EXPECT_EQ(stream->defaultAudio_, media1);
}

HWTEST_F(M3u8UnitTest, UpdateMasterPlaylist_NoDefault_001, TestSize.Level1)
{
    std::string pl = "#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=5000000,CODECS=\"mp4a.40.2\"\nhttp://x/v.m3u8";
    M3U8MasterPlaylist playlist(pl, "http://example.com/master.m3u8");
    playlist.StartParsing();
    EXPECT_NE(playlist.defaultVariant_, nullptr);
    EXPECT_EQ(playlist.firstVideoStream_, nullptr);
    EXPECT_EQ(playlist.variants_.size(), 1u);
    EXPECT_EQ(playlist.variants_.front()->bandWidth_, 5000000u);
}

HWTEST_F(M3u8UnitTest, UpdateMasterPlaylist_FirstVideo_001, TestSize.Level1)
{
    std::string pl = "#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=5000000,CODECS=\"avc1.42E01E\"\nhttp://x/v1.m3u8\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=5000000,CODECS=\"avc1.42E01E\"\nhttp://x/v2.m3u8";
    M3U8MasterPlaylist playlist(pl, "http://example.com/master.m3u8");
    playlist.StartParsing();
    EXPECT_NE(playlist.defaultVariant_, nullptr);
    EXPECT_NE(playlist.firstVideoStream_, nullptr);
    EXPECT_EQ(playlist.defaultVariant_, playlist.firstVideoStream_);
}

HWTEST_F(M3u8UnitTest, ProcessAllTags_DefaultCase_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::list<std::shared_ptr<Tag>> tags;
    tags.emplace_back(std::make_shared<Tag>(HlsTag::EXTXENDLIST));
    playlist.ProcessAllTags(tags);
    EXPECT_EQ(playlist.variants_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, ProcessAllTags_IFrameCase_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::list<std::shared_ptr<Tag>> tags;
    tags.emplace_back(std::make_shared<AttributesTag>(HlsTag::EXTXIFRAMESTREAMINF, "URI=\"http://x/v.m3u8\""));
    playlist.ProcessAllTags(tags);
    EXPECT_EQ(playlist.variants_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, GetExtInf_WrongSection_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    double duration = 5.0;
    auto tag = std::make_shared<SingleValueTag>(HlsTag::URI, "test.ts");
    m3u8.GetExtInf(tag, duration);
    EXPECT_DOUBLE_EQ(duration, 5.0);
}

HWTEST_F(M3u8UnitTest, GetDuration_WithFiles_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.files_.emplace_back(std::make_shared<M3U8Fragment>("http://x", 5.0, 1, false));
    m3u8.files_.emplace_back(std::make_shared<M3U8Fragment>("http://y", 3.5, 2, false));
    EXPECT_DOUBLE_EQ(m3u8.GetDuration(), 8.5);
}

HWTEST_F(M3u8UnitTest, AddFile_MinFragUpdate_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.minFragDuration_ = 100;
    M3U8Fragment frag("http://x", 5.0, 1, false);
    m3u8.AddFile(std::make_shared<M3U8Fragment>(frag), 50);
    EXPECT_EQ(m3u8.minFragDuration_.load(), 50u);
    EXPECT_EQ(m3u8.segmentOffsets_.size(), 1u);
}

HWTEST_F(M3u8UnitTest, AddFile_KeyIndexNonNone_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isDecryptAble_ = true;
    m3u8.method_ = std::make_shared<std::string>("AES-128");
    m3u8.keyIndex_ = 2;
    m3u8.totalKeyIndex_ = 1;
    M3U8Fragment frag("http://x", 5.0, 1, false);
    m3u8.AddFile(std::make_shared<M3U8Fragment>(frag), 50);
    EXPECT_EQ(m3u8.files_.back()->keyIndex_, 3u);
}

HWTEST_F(M3u8UnitTest, ProcessInfo_FirstFragmentReady_001, TestSize.Level1)
{
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    m3u8->isDecryptAble_ = true;
    m3u8->isFirstFragmentReady_ = true;
    m3u8->sequence_ = 1;
    m3u8->hasExplicitIv_.store(true);
    KeyInfo keyInfo;
    keyInfo.index_ = 0;
    m3u8->keyInfos_[0] = keyInfo;
    M3U8Info info;
    info.uri = "http://example.com/seg.ts";
    info.duration = 5.0;
    size_t duration = 0;
    m3u8->ProcessInfo(info, duration);
    EXPECT_EQ(m3u8->files_.size(), 1u);
    EXPECT_EQ(m3u8->sequence_, 2u);
}

HWTEST_F(M3u8UnitTest, UpdateMediaPlaylist_WithDecrypt_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("#EXTM3U\n#EXTINF:5.0,\nseg.ts", "http://example.com");
    playlist.isDecryptAble_ = true;
    playlist.isDecryptKeyReady_ = true;
    playlist.hasExplicitIv_ = true;
    playlist.keyLen_ = 16;
    for (int i = 0; i < 16; i++) {
        playlist.key_[i] = static_cast<uint8_t>(i);
        playlist.iv_[i] = static_cast<uint8_t>(i + 1);
    }
    playlist.StartParsing();
    EXPECT_TRUE(playlist.isSimple_);
    EXPECT_TRUE(playlist.isDecryptAble_);
}

HWTEST_F(M3u8UnitTest, UpdateMasterPlaylist_WithDefine_001, TestSize.Level1)
{
    std::string pl = "#EXTM3U\n#EXT-X-DEFINE:IMPORT=X\n#EXT-X-STREAM-INF:BANDWIDTH=1000,CODECS=\"avc1\"\n"
        "http://x/v.m3u8";
    M3U8MasterPlaylist playlist(pl, "http://example.com/master.m3u8?token=abc");
    playlist.StartParsing();
    EXPECT_FALSE(playlist.variants_.empty());
}

HWTEST_F(M3u8UnitTest, BindVideoMedia_NoMedia_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    playlist.variants_.push_back(stream);
    playlist.BindVideoMedia("audio");
    EXPECT_EQ(stream->audioMedia_.size(), 0u);
    playlist.BindVideoMedia("subtitles");
    EXPECT_EQ(stream->subtitlesMedia_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, GetResolutionDelta_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.initResolution_ = 1000;
    EXPECT_EQ(playlist.GetResolutionDelta(100, 5), 500u);
    EXPECT_EQ(playlist.GetResolutionDelta(10, 10), 900u);
}

HWTEST_F(M3u8UnitTest, ParseDateRange_ContentsAllAttrs_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "2024-01-01T00:00:00";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDATERANGE, "");
    auto idAttr = std::make_shared<Attribute>("ID", "\"id1\"");
    tag->AddAttribute(idAttr);
    auto classAttr = std::make_shared<Attribute>("CLASS", "\"cls\"");
    tag->AddAttribute(classAttr);
    auto startDateAttr = std::make_shared<Attribute>("START-DATE", "\"2024-01-01T00:01:00\"");
    tag->AddAttribute(startDateAttr);
    auto durationAttr = std::make_shared<Attribute>("DURATION", "30");
    tag->AddAttribute(durationAttr);
    auto assetListAttr = std::make_shared<Attribute>("X-ASSET-LIST", "\"http://x\"");
    tag->AddAttribute(assetListAttr);
    m3u8.ParseDateRange(tag);
    EXPECT_EQ(m3u8.dateRanges_.size(), 1u);
    auto& dr = m3u8.dateRanges_.front();
    EXPECT_EQ(dr->id, "id1");
    EXPECT_EQ(dr->classify, "cls");
    EXPECT_EQ(dr->start, 60000);
    EXPECT_EQ(dr->duration, 30000);
    EXPECT_EQ(dr->assetList, "http://x");
    EXPECT_EQ(dr->contents.size(), 5u);
}

// ===== UriJoin branch coverage (via UpdateUri) =====
HWTEST_F(M3u8UnitTest, UpdateUri_HttpScheme_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/playlist.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::URI, "http://other.com/seg.ts");
    M3U8Info info;
    m3u8.UpdateUri(tag, info);
    ASSERT_EQ(info.uri, "http://other.com/seg.ts");
}

HWTEST_F(M3u8UnitTest, UpdateUri_HttpsScheme_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/playlist.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::URI, "https://secure.com/seg.ts");
    M3U8Info info;
    m3u8.UpdateUri(tag, info);
    ASSERT_EQ(info.uri, "https://secure.com/seg.ts");
}

HWTEST_F(M3u8UnitTest, UpdateUri_DoubleSlash_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/playlist.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::URI, "//cdn.com/seg.ts");
    M3U8Info info;
    m3u8.UpdateUri(tag, info);
    ASSERT_EQ(info.uri, "http://cdn.com/seg.ts");
}

HWTEST_F(M3u8UnitTest, UpdateUri_SingleSlash_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/playlist.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::URI, "/path/seg.ts");
    M3U8Info info;
    m3u8.UpdateUri(tag, info);
    ASSERT_EQ(info.uri, "http://example.com/path/seg.ts");
}

HWTEST_F(M3u8UnitTest, UpdateUri_QueryInBase_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/playlist.m3u8?token=abc", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::URI, "seg.ts");
    M3U8Info info;
    m3u8.UpdateUri(tag, info);
    ASSERT_EQ(info.uri, "http://example.com/seg.ts");
}

HWTEST_F(M3u8UnitTest, UpdateUri_NoSlashInBase_001, TestSize.Level1)
{
    M3U8 m3u8("noslash", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::URI, "seg.ts");
    M3U8Info info;
    m3u8.UpdateUri(tag, info);
    ASSERT_EQ(info.uri, "seg.ts");
}

// ===== PrepareDecrptionKeys branch coverage =====
HWTEST_F(M3u8UnitTest, PrepareDecryptionKeys_SameKeySkip_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY,
        "METHOD=AES-128,URI=\"http://key.com/k\"");
    m3u8.PrepareDecrptionKeys(tag);
    EXPECT_TRUE(m3u8.isDecryptAble_);
    EXPECT_EQ(m3u8.keyInfos_.size(), 1u);
    uint64_t firstKeyIndex = m3u8.keyIndex_;
    m3u8.PrepareDecrptionKeys(tag);
    ASSERT_EQ(m3u8.keyInfos_.size(), 1u);
    ASSERT_EQ(m3u8.keyIndex_, firstKeyIndex);
}

HWTEST_F(M3u8UnitTest, PrepareDecryptionKeys_DifferentKey_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag1 = std::make_shared<AttributesTag>(HlsTag::EXTXKEY,
        "METHOD=AES-128,URI=\"http://key.com/k1\"");
    m3u8.PrepareDecrptionKeys(tag1);
    EXPECT_EQ(m3u8.keyInfos_.size(), 1u);
    std::shared_ptr<Tag> tag2 = std::make_shared<AttributesTag>(HlsTag::EXTXKEY,
        "METHOD=AES-128,URI=\"http://key.com/k2\"");
    m3u8.PrepareDecrptionKeys(tag2);
    ASSERT_EQ(m3u8.keyInfos_.size(), 2u);
}

HWTEST_F(M3u8UnitTest, PrepareDecryptionKeys_NullKeyUri_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY,
        "METHOD=AES-128");
    m3u8.PrepareDecrptionKeys(tag);
    EXPECT_TRUE(m3u8.isDecryptAble_);
    ASSERT_EQ(m3u8.keyInfos_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, PrepareDecryptionKeys_WrongSection_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<SingleValueTag>(HlsTag::URI, "test.ts");
    m3u8.PrepareDecrptionKeys(tag);
    ASSERT_FALSE(m3u8.isDecryptAble_);
}

// ===== ParseMap branch coverage =====
HWTEST_F(M3u8UnitTest, ParseMap_EmptyUri_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isInterruptNeeded_ = true;
    std::shared_ptr<AttributesTag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXMAP, "");
    auto uriAttr = std::make_shared<Attribute>("URI", "\"\"");
    tag->AddAttribute(uriAttr);
    m3u8.ParseMap(tag);
    EXPECT_EQ(m3u8.fmp4Header_, nullptr);
    ASSERT_EQ(m3u8.fmp4HeaderLen, 0u);
}

HWTEST_F(M3u8UnitTest, ParseMap_LargeLength_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isInterruptNeeded_ = true;
    m3u8.isHeaderReady_ = false;
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXMAP,
        "#EXT-X-MAP:URI=\"main.mp4\",BYTERANGE=\"200000000@0\"");
    m3u8.ParseMap(tag);
    EXPECT_EQ(m3u8.fmp4Header_, nullptr);
    ASSERT_EQ(m3u8.fmp4HeaderLen, 0u);
}

HWTEST_F(M3u8UnitTest, ParseMap_NullTag_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<AttributesTag> nullTag;
    m3u8.ParseMap(nullTag);
    EXPECT_EQ(m3u8.fmp4Header_, nullptr);
    ASSERT_EQ(m3u8.fmp4HeaderLen, 0u);
}

// ===== Update branch coverage =====
HWTEST_F(M3u8UnitTest, Update_DefineNoImport_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::string playList = "#EXTM3U\n#EXT-X-DEFINE:NAME=\"value\"\n#EXTINF:5.0,\nseg.ts";
    EXPECT_TRUE(m3u8.Update(playList, true));
    ASSERT_FALSE(m3u8.files_.empty());
}

HWTEST_F(M3u8UnitTest, Update_DefineImportEmptyMap_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8?token=abc", "");
    std::string playList = "#EXTM3U\n#EXT-X-DEFINE:IMPORT=KEY\n#EXTINF:5.0,\nseg.ts";
    EXPECT_TRUE(m3u8.Update(playList, true));
    ASSERT_FALSE(m3u8.files_.empty());
}

HWTEST_F(M3u8UnitTest, Update_DefineNoQueryInUri_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.tagMasterMap_ = {{"KEY", "VAL"}};
    std::string playList = "#EXTM3U\n#EXT-X-DEFINE:IMPORT=KEY\n#EXTINF:5.0,\nseg.ts";
    EXPECT_TRUE(m3u8.Update(playList, true));
    ASSERT_FALSE(m3u8.files_.empty());
}

// ===== CreateVariantStream branch coverage =====
HWTEST_F(M3u8UnitTest, CreateVariantStream_NoUri_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXSTREAMINF, "BANDWIDTH=1000");
    playlist.CreateVariantStream(tag);
    ASSERT_EQ(playlist.variants_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, CreateVariantStream_HighBandwidth_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::string attr = "URI=\"http://example.com/v1.m3u8\",BANDWIDTH=4000000,CODECS=\"avc1.42E01E\"";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXSTREAMINF, attr);
    playlist.CreateVariantStream(tag);
    EXPECT_EQ(playlist.variants_.size(), 1u);
    EXPECT_TRUE(playlist.variants_.front()->isVideo_);
    ASSERT_EQ(playlist.defaultVariant_, nullptr);
}

// ===== ParseAttributes branch coverage =====
HWTEST_F(M3u8UnitTest, ParseAttributes_SubtitlesAudio_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::string attr = "BANDWIDTH=1000,SUBTITLES=\"sub1\",AUDIO=\"aud1\"";
    auto item = std::make_shared<AttributesTag>(HlsTag::EXTXSTREAMINF, attr);
    auto stream = std::make_shared<M3U8VariantStream>("test", "http://example.com/test.m3u8", nullptr);
    playlist.ParseAttributes(item, stream);
    EXPECT_EQ(stream->bandWidth_, 1000u);
    EXPECT_EQ(stream->subtitles_, "sub1");
    ASSERT_EQ(stream->audio_, "aud1");
}

// ===== ParseMediaStreamInfo branch coverage =====
HWTEST_F(M3u8UnitTest, ParseMediaStreamInfo_NoUri_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXMEDIA, "TYPE=AUDIO,GROUP-ID=\"grp1\"");
    std::shared_ptr<Tag> tagPtr = tag;
    playlist.ParseMediaStreamInfo(tagPtr);
    ASSERT_EQ(playlist.audioList_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, ParseMediaStreamInfo_NoType_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXMEDIA,
        "URI=\"http://x/a.m3u8\",GROUP-ID=\"grp1\"");
    std::shared_ptr<Tag> tagPtr = tag;
    playlist.ParseMediaStreamInfo(tagPtr);
    EXPECT_EQ(playlist.audioList_.size(), 0u);
    ASSERT_EQ(playlist.subtitlesList_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, ParseMediaStreamInfo_ClosedCaptions_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXMEDIA,
        "URI=\"http://x/cc.m3u8\",TYPE=CLOSED-CAPTIONS,GROUP-ID=\"cc1\"");
    std::shared_ptr<Tag> tagPtr = tag;
    playlist.ParseMediaStreamInfo(tagPtr);
    EXPECT_EQ(playlist.audioList_.size(), 0u);
    ASSERT_EQ(playlist.subtitlesList_.size(), 0u);
}

// ===== BindVideoMedia branch coverage =====
HWTEST_F(M3u8UnitTest, BindVideoMedia_Subtitles_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto media = std::make_shared<M3U8Media>("s", "http://example.com/s.m3u8", nullptr);
    media->groupID_ = "sub1";
    media->isDefault_ = true;
    playlist.subtitlesList_.push_back(media);
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    stream->subtitles_ = "sub1";
    playlist.variants_.push_back(stream);
    playlist.BindVideoMedia("subtitles");
    EXPECT_EQ(stream->subtitlesMedia_.size(), 1u);
    ASSERT_EQ(stream->defaultSubtitles_, media);
}

HWTEST_F(M3u8UnitTest, BindVideoMedia_NullVariant_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto media = std::make_shared<M3U8Media>("a", "uri", nullptr);
    media->groupID_ = "grp1";
    media->forced_ = true;
    playlist.audioList_.push_back(media);
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    playlist.variants_.push_back(stream);
    playlist.variants_.push_back(nullptr);
    playlist.BindVideoMedia("audio");
    EXPECT_EQ(stream->audioMedia_.size(), 1u);
    ASSERT_EQ(stream->defaultAudio_, media);
}

HWTEST_F(M3u8UnitTest, BindVideoMedia_NullMediaStream_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto media = std::make_shared<M3U8Media>("a", "uri", nullptr);
    media->groupID_ = "grp1";
    media->forced_ = true;
    playlist.audioList_.push_back(media);
    playlist.audioList_.push_back(nullptr);
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    stream->audio_ = "grp1";
    playlist.variants_.push_back(stream);
    playlist.BindVideoMedia("audio");
    EXPECT_EQ(stream->audioMedia_.size(), 1u);
    ASSERT_EQ(stream->defaultAudio_, media);
}

HWTEST_F(M3u8UnitTest, GetDefaultMedieStream_EmptyMedia_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    playlist.GetDefaultMedieStream(stream, "audio");
    EXPECT_EQ(stream->audioMedia_.size(), 0u);
    ASSERT_EQ(stream->defaultAudio_, nullptr);
}

// ===== ChooseStreamByStream branch coverage =====
HWTEST_F(M3u8UnitTest, ChooseStreamByStream_NullVariant_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    auto stream1 = std::make_shared<M3U8VariantStream>("s1", "uri1", m3u8);
    stream1->streamId_ = 1;
    playlist.variants_.push_back(stream1);
    playlist.variants_.push_back(nullptr);
    auto stream3 = std::make_shared<M3U8VariantStream>("s3", "uri3", m3u8);
    stream3->streamId_ = 3;
    playlist.variants_.push_back(stream3);
    playlist.ChooseStreamByStream(3);
    ASSERT_EQ(playlist.defaultVariant_, stream3);
}

// ===== ChooseStreamByResolution branch coverage =====
HWTEST_F(M3u8UnitTest, ChooseStreamByResolution_NullDefault_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    playlist.initResolution_ = 1000;
    playlist.ChooseStreamByResolution();
    EXPECT_EQ(playlist.variants_.size(), 0u);
    ASSERT_EQ(playlist.defaultVariant_, nullptr);
}

// ===== SetInterruptState branch coverage =====
HWTEST_F(M3u8UnitTest, SetInterruptState_WithM3u8_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto m3u8 = std::make_shared<M3U8>("http://example.com/test.m3u8", "");
    auto stream = std::make_shared<M3U8VariantStream>("test", "uri", m3u8);
    stream->streamId_ = 5;
    playlist.defaultVariant_ = stream;
    m3u8->isInterruptNeeded_ = false;
    playlist.SetInterruptState(true);
    ASSERT_TRUE(m3u8->isInterruptNeeded_.load());
}

// ===== ParseStart (master) branch coverage =====
HWTEST_F(M3u8UnitTest, ParseStart_Master_Empty_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXSTART, "");
    std::shared_ptr<Tag> tagPtr = tag;
    playlist.timeOffset_ = 99.0;
    playlist.precise_ = true;
    playlist.ParseStart(tagPtr);
    EXPECT_DOUBLE_EQ(playlist.timeOffset_, 99.0);
    EXPECT_TRUE(playlist.precise_);
    ASSERT_TRUE(playlist.isStart_);
}

// ===== GetKeyInfos direct test =====
HWTEST_F(M3u8UnitTest, GetKeyInfos_Direct_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    KeyInfo keyInfo;
    keyInfo.index_ = 1;
    keyInfo.keyLen_ = 16;
    m3u8.keyInfos_[1] = keyInfo;
    std::unordered_map<uint64_t, KeyInfo> keyInfos;
    m3u8.GetKeyInfos(keyInfos);
    EXPECT_EQ(keyInfos.size(), 1u);
    EXPECT_EQ(keyInfos.begin()->first, 1u);
    ASSERT_EQ(keyInfos.begin()->second.keyLen_, 16u);
}

// ===== SetDownloadCallback tests =====
HWTEST_F(M3u8UnitTest, SetDownloadCallback_M3U8_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    auto callback = std::make_shared<DownloadMetricsInfo>();
    callback->UpdateTotalDownloadCount();
    m3u8.SetDownloadCallback(callback);
    EXPECT_EQ(m3u8.downloadCallback_, callback);
    ASSERT_EQ(m3u8.downloadCallback_->GetTotalDownloadCount(), 1);
}

HWTEST_F(M3u8UnitTest, SetDownloadCallback_Master_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    auto callback = std::make_shared<DownloadMetricsInfo>();
    callback->UpdateTotalDownloadCount();
    playlist.SetDownloadCallback(callback);
    EXPECT_EQ(playlist.downloadCallback_, callback);
    ASSERT_EQ(playlist.downloadCallback_->GetTotalDownloadCount(), 1);
}

// ===== UpdateDownloadFinished direct test =====
HWTEST_F(M3u8UnitTest, UpdateDownloadFinished_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isHeaderReady_ = false;
    m3u8.UpdateDownloadFinished("http://example.com/map", "/redirect");
    ASSERT_TRUE(m3u8.isHeaderReady_.load());
}

// ===== GetLiveUpdateGap valid duration =====
HWTEST_F(M3u8UnitTest, GetLiveUpdateGap_ValidDuration_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.minFragDuration_ = 5;
    ASSERT_EQ(m3u8.GetLiveUpdateGap(), 2u);
}

// ===== GetResolutionDelta resolution > init =====
HWTEST_F(M3u8UnitTest, GetResolutionDelta_GreaterThanInit_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.initResolution_ = 1000;
    ASSERT_EQ(playlist.GetResolutionDelta(100, 20), 1000u);
}

// ===== IsNearToInitResolution equal delta with bandwidth =====
HWTEST_F(M3u8UnitTest, IsNearToInitResolution_EqualDelta_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.initResolution_ = 1000;
    auto m3u8 = std::make_shared<M3U8>("http://x", "");
    auto choosed = std::make_shared<M3U8VariantStream>("a", "uri", m3u8);
    auto current = std::make_shared<M3U8VariantStream>("b", "uri", m3u8);
    choosed->width_ = 10;
    choosed->height_ = 10;
    choosed->bandWidth_ = 2000;
    current->width_ = 10;
    current->height_ = 10;
    current->bandWidth_ = 1000;
    ASSERT_TRUE(playlist.IsNearToInitResolution(choosed, current));
}

// ===== SaveData not session key with valid keyIndex =====
HWTEST_F(M3u8UnitTest, SaveData_NotSessionKey_ValidKeyIndex_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isSessionKey_ = false;
    KeyInfo keyInfo;
    keyInfo.index_ = 0;
    keyInfo.keyLen_ = 0;
    m3u8.keyInfos_[0] = keyInfo;
    uint8_t data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    uint32_t result = m3u8.SaveData(data, 16, false, 0);
    EXPECT_EQ(result, 16u);
    EXPECT_EQ(m3u8.keyLen_, 16u);
    EXPECT_EQ(m3u8.keyInfos_[0].keyLen_, 16u);
    ASSERT_EQ(m3u8.keyInfos_[0].key_[0], data[0]);
}

// ===== ProcessAllTags null tag =====
HWTEST_F(M3u8UnitTest, ProcessAllTags_NullTag_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::list<std::shared_ptr<Tag>> tags;
    tags.emplace_back(nullptr);
    playlist.ProcessAllTags(tags);
    ASSERT_EQ(playlist.variants_.size(), 0u);
}

// ===== UpdateMediaPlaylist with download callback =====
HWTEST_F(M3u8UnitTest, UpdateMediaPlaylist_WithDownloadCallback_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("#EXTM3U\n#EXTINF:5.0,\nseg.ts", "http://example.com");
    auto callback = std::make_shared<DownloadMetricsInfo>();
    playlist.SetDownloadCallback(callback);
    playlist.StartParsing();
    EXPECT_TRUE(playlist.isSimple_);
    EXPECT_NE(playlist.defaultVariant_, nullptr);
    ASSERT_NE(playlist.defaultVariant_->m3u8_->downloadCallback_, nullptr);
}

// ===== DownloadMap empty uri =====
HWTEST_F(M3u8UnitTest, DownloadMap_EmptyUri_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.DownloadMap("", 0, 0);
    EXPECT_EQ(m3u8.fmp4HeaderUri_, "");
    ASSERT_EQ(m3u8.downloadHeaderRequest_, nullptr);
}

// ===== SaveMapData allocate on demand =====
HWTEST_F(M3u8UnitTest, SaveMapData_AllocateOnDemand_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.downloadHeaderRequest_ = std::make_shared<DownloadRequest>("", nullptr, nullptr, false);
    uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint32_t result = m3u8.SaveMapData(data, 10, false);
    EXPECT_EQ(result, 10u);
    EXPECT_NE(m3u8.fmp4Header_, nullptr);
    ASSERT_EQ(m3u8.fmp4HeaderLen, 1048576u);
}

// ===== Additional branch coverage batch 2 =====
HWTEST_F(M3u8UnitTest, M3U8Fragment_IvValidKeyNull_001, TestSize.Level1)
{
    M3U8Fragment m3u8("http://example.com", 10.0, 1, false);
    uint8_t iv[MAX_LOOP] = {6, 7, 8, 9, 10};
    M3U8Fragment fragment(m3u8, nullptr, iv);
    EXPECT_EQ(fragment.duration_, 10.0);
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(fragment.iv_[i], 0);
    }
}

HWTEST_F(M3u8UnitTest, UpdateSkip_NoSkippedSegments_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXSKIP, "OTHER=VALUE");
    M3U8Info info;
    m3u8.skipSegmentsNum_ = 10;
    m3u8.UpdateSkip(tag, info);
    ASSERT_EQ(m3u8.skipSegmentsNum_, 10u);
}

HWTEST_F(M3u8UnitTest, UpdateStart_NoPreciseNoTimeOffset_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXSTART, "OTHER=VALUE");
    M3U8Info info;
    m3u8.timeOffset_ = 5.0;
    m3u8.precise_ = false;
    m3u8.isStart_ = false;
    m3u8.UpdateStart(tag, info);
    EXPECT_DOUBLE_EQ(m3u8.timeOffset_, 5.0);
    EXPECT_FALSE(m3u8.precise_);
    ASSERT_TRUE(m3u8.isStart_);
}

HWTEST_F(M3u8UnitTest, ParseDateRange_NoStartDate_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "2024-01-01T00:00:00";
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDATERANGE, "");
    auto idAttr = std::make_shared<Attribute>("ID", "\"test\"");
    tag->AddAttribute(idAttr);
    auto durationAttr = std::make_shared<Attribute>("DURATION", "30");
    tag->AddAttribute(durationAttr);
    m3u8.ParseDateRange(tag);
    EXPECT_EQ(m3u8.dateRanges_.size(), 1u);
    EXPECT_EQ(m3u8.dateRanges_.front()->id, "test");
    ASSERT_EQ(m3u8.dateRanges_.front()->start, 0);
}

HWTEST_F(M3u8UnitTest, ParseKey_NullTag_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.method_ = std::make_shared<std::string>("AES-128");
    std::shared_ptr<AttributesTag> nullTag;
    m3u8.ParseKey(nullTag);
    ASSERT_EQ(*m3u8.method_, "AES-128");
}

HWTEST_F(M3u8UnitTest, SaveData_NullData_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isSessionKey_ = true;
    m3u8.keyLen_ = 10;
    uint32_t result = m3u8.SaveData(nullptr, 10, false, 0);
    ASSERT_EQ(result, 0u);
}

HWTEST_F(M3u8UnitTest, OnDownloadStatus_NullRequest_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Downloader> downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    std::shared_ptr<DownloadRequest> request = std::make_shared<DownloadRequest>("", nullptr, nullptr, false);
    request->serverError_ = 404;
    std::shared_ptr<DownloadRequest> nullReq;
    m3u8.OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, downloader, nullReq);
    ASSERT_EQ(request->serverError_, 404);
}

HWTEST_F(M3u8UnitTest, CreateVariantStream_NullTag_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::shared_ptr<Tag> nullTag;
    playlist.CreateVariantStream(nullTag);
    ASSERT_EQ(playlist.variants_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, ParseAttributes_NoBandwidth_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::string attr = "CODECS=\"avc1.42E01E\",FRAME-RATE=30";
    auto item = std::make_shared<AttributesTag>(HlsTag::EXTXSTREAMINF, attr);
    auto stream = std::make_shared<M3U8VariantStream>("test", "http://example.com/test.m3u8", nullptr);
    playlist.ParseAttributes(item, stream);
    EXPECT_EQ(stream->bandWidth_, 0u);
    EXPECT_EQ(stream->codecs_, "avc1.42E01E");
    ASSERT_EQ(stream->frameRate_, 30u);
}

HWTEST_F(M3u8UnitTest, ParseMediaStreamInfo_NullTag_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::shared_ptr<Tag> nullTag;
    playlist.ParseMediaStreamInfo(nullTag);
    ASSERT_EQ(playlist.audioList_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, ParseStart_Master_NullTag_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    playlist.timeOffset_ = 99.0;
    std::shared_ptr<Tag> nullTag;
    playlist.ParseStart(nullTag);
    ASSERT_DOUBLE_EQ(playlist.timeOffset_, 99.0);
}

HWTEST_F(M3u8UnitTest, GetDefaultMedieStream_NullStream_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto media = std::make_shared<M3U8Media>("a", "uri", nullptr);
    media->forced_ = true;
    std::shared_ptr<M3U8VariantStream> stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    stream->audioMedia_ = {media};
    std::shared_ptr<M3U8VariantStream> nullStream;
    playlist.GetDefaultMedieStream(nullStream, "audio");
    EXPECT_EQ(stream->audioMedia_.size(), 1u);
    ASSERT_EQ(stream->defaultAudio_, nullptr);
}

HWTEST_F(M3u8UnitTest, GetCodec_EmptyTokens_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    EXPECT_EQ(playlist.GetCodec(StreamType::VIDEO, "  ,  .  "), "");
    ASSERT_EQ(playlist.GetCodec(StreamType::AUDIO, "  ,  .  "), "");
}

HWTEST_F(M3u8UnitTest, InitializeDownloadHeader_EmptyUri_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.InitializeDownloadHeader("", 0, 0);
    EXPECT_EQ(m3u8.fmp4HeaderUri_, "");
    ASSERT_EQ(m3u8.downloadHeaderRequest_, nullptr);
}

HWTEST_F(M3u8UnitTest, SaveMapData_CopyLenZero_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.fmp4Header_ = new uint8_t[16];
    m3u8.fmp4HeaderLen = 16;
    m3u8.downloadHeaderLen_ = 16;
    uint8_t data[10] = {1, 2, 3, 4, 5};
    uint32_t result = m3u8.SaveMapData(data, 10, false);
    EXPECT_EQ(result, 0u);
    ASSERT_EQ(m3u8.downloadHeaderLen_, 16u);
    delete[] m3u8.fmp4Header_;
    m3u8.fmp4Header_ = nullptr;
    m3u8.fmp4HeaderLen = 0;
    m3u8.downloadHeaderLen_ = 0;
}

HWTEST_F(M3u8UnitTest, GetExtInf_NoDuration_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    auto tag = std::make_shared<ValuesListTag>(HlsTag::EXTINF, "");
    double duration = 5.0;
    m3u8.GetExtInf(tag, duration);
    ASSERT_DOUBLE_EQ(duration, 0.0);
}

HWTEST_F(M3u8UnitTest, UpdateMediaSequence_WrongSection_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.sequence_ = 100;
    m3u8.initSequence_ = 100;
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY, "METHOD=AES-128");
    M3U8Info info;
    m3u8.UpdateMediaSequence(tag, info);
    ASSERT_EQ(m3u8.sequence_, 100u);
}

HWTEST_F(M3u8UnitTest, UpdateDiscontSequence_WrongSection_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.discontSequence_ = 7;
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY, "METHOD=AES-128");
    M3U8Info info;
    m3u8.UpdateDiscontSequence(tag, info);
    ASSERT_EQ(m3u8.discontSequence_, 7);
}

HWTEST_F(M3u8UnitTest, UpdateUri_WrongSection_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    M3U8Info info;
    info.uri = "http://original";
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY, "METHOD=AES-128");
    m3u8.UpdateUri(tag, info);
    ASSERT_EQ(info.uri, "http://original");
}

HWTEST_F(M3u8UnitTest, UpdateProgramDateTime_WrongSection_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "2023-01-01";
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY, "METHOD=AES-128");
    M3U8Info info;
    m3u8.UpdateProgramDateTime(tag, info);
    ASSERT_EQ(m3u8.currentProgramDateTime_, "2023-01-01");
}

HWTEST_F(M3u8UnitTest, DownloadKey_NotKey_001, TestSize.Level1)
{
    ScopedM3u8Download download;
    download.PrepareSessionKey();
    auto m3u8 = download.Owner();
    m3u8->DownloadKey(false);
    ASSERT_NE(m3u8->downloader_, nullptr);
    ASSERT_NE(m3u8->downloadRequest_, nullptr);
    EXPECT_EQ(m3u8->downloadRequest_->GetUrl(), KEY_TEST_BASE + SESSION_KEY_PATH);
    EXPECT_EQ(m3u8->downloadRequest_->keyIndex_, TEST_SESSION_KEY_INDEX);
    EXPECT_EQ(m3u8->downloadRequest_->GetRequestType(), RequestType::HLS_KEY);
    ASSERT_TRUE(download.WaitForKey());
    download.Stop();
    EXPECT_FALSE(download.HadError());
    ASSERT_EQ(m3u8->keyLen_, TEST_AES_KEY_SIZE);
    EXPECT_EQ(std::string(reinterpret_cast<const char*>(m3u8->key_), m3u8->keyLen_), TEST_SESSION_KEY_BYTES);
    EXPECT_TRUE(m3u8->isDecryptKeyReady_.load());
}

HWTEST_F(M3u8UnitTest, UpdateMasterPlaylist_DefineNoQuery_001, TestSize.Level1)
{
    std::string pl = "#EXTM3U\n#EXT-X-DEFINE:NAME=\"val\"\n#EXT-X-STREAM-INF:BANDWIDTH=1000,CODECS=\"avc1\"\n"
        "http://x/v.m3u8";
    M3U8MasterPlaylist playlist(pl, "http://example.com/master.m3u8");
    playlist.StartParsing();
    ASSERT_FALSE(playlist.variants_.empty());
}

HWTEST_F(M3u8UnitTest, ExtractNumber_NoDigitBeforeDot_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    uint64_t maxVal = std::numeric_limits<uint64_t>::max();
    ASSERT_EQ(m3u8.ExtractNumber("abc.ts"), maxVal);
}

HWTEST_F(M3u8UnitTest, ParseMap_PreAllocatedHeader_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isInterruptNeeded_ = true;
    m3u8.isHeaderReady_ = false;
    m3u8.fmp4Header_ = new uint8_t[100];
    m3u8.fmp4HeaderLen = 100;
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXMAP,
        "#EXT-X-MAP:URI=\"main.mp4\",BYTERANGE=\"50@0\"");
    m3u8.ParseMap(tag);
    EXPECT_NE(m3u8.fmp4Header_, nullptr);
    ASSERT_EQ(m3u8.fmp4HeaderLen, 51u);
    delete[] m3u8.fmp4Header_;
    m3u8.fmp4Header_ = nullptr;
    m3u8.fmp4HeaderLen = 0;
    m3u8.downloadHeaderLen_ = 0;
}

HWTEST_F(M3u8UnitTest, BindVideoMedia_NullMediaInList_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto media1 = std::make_shared<M3U8Media>("a", "uri", nullptr);
    media1->groupID_ = "grp1";
    auto media2 = std::make_shared<M3U8Media>("b", "uri", nullptr);
    media2->groupID_ = "grp1";
    media2->isDefault_ = true;
    media2->characteristics_ = "";
    auto media3 = std::make_shared<M3U8Media>("c", "uri", nullptr);
    media3->groupID_ = "grp1";
    media3->autoSelect_ = true;
    playlist.audioList_.push_back(media1);
    playlist.audioList_.push_back(nullptr);
    playlist.audioList_.push_back(media2);
    playlist.audioList_.push_back(nullptr);
    playlist.audioList_.push_back(media3);
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    stream->audio_ = "grp1";
    playlist.variants_.push_back(stream);
    playlist.BindVideoMedia("audio");
    EXPECT_EQ(stream->audioMedia_.size(), 3u);
    ASSERT_EQ(stream->defaultAudio_, media2);
}

HWTEST_F(M3u8UnitTest, PrepareDecryptionKeys_NullTag_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isDecryptAble_ = true;
    std::shared_ptr<Tag> nullTag;
    m3u8.PrepareDecrptionKeys(nullTag);
    ASSERT_TRUE(m3u8.isDecryptAble_);
}

// ===== Final batch: targeted branch coverage =====
HWTEST_F(M3u8UnitTest, InitializeDownloadHeader_WithCallback_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    auto callback = std::make_shared<DownloadMetricsInfo>();
    m3u8.SetDownloadCallback(callback);
    m3u8.InitializeDownloadHeader("http://example.com/map.mp4", 0, 0);
    EXPECT_NE(m3u8.downloadHeaderRequest_, nullptr);
    ASSERT_EQ(m3u8.downloadHeaderRequest_->GetUrl(), "http://example.com/map.mp4");
}

HWTEST_F(M3u8UnitTest, DownloadKey_WithCallback_001, TestSize.Level1)
{
    ScopedM3u8Download download;
    download.PrepareKey(KEY_WITH_CALLBACK_PATH);
    auto m3u8 = download.Owner();
    uint64_t index = m3u8->keyIndex_ + m3u8->totalKeyIndex_;
    ASSERT_GT(index, 0u);
    std::unordered_map<uint64_t, KeyInfo> keys;
    m3u8->GetKeyInfos(keys);
    ASSERT_NE(keys.find(index), keys.end());
    m3u8->DownloadKey(true);
    ASSERT_NE(m3u8->downloader_, nullptr);
    ASSERT_NE(m3u8->downloadRequest_, nullptr);
    EXPECT_EQ(m3u8->downloadRequest_->GetUrl(), KEY_TEST_BASE + KEY_WITH_CALLBACK_PATH);
    EXPECT_EQ(m3u8->downloadRequest_->keyIndex_, index);
    EXPECT_EQ(m3u8->downloadRequest_->GetRequestType(), RequestType::HLS_KEY);
    EXPECT_EQ(m3u8->downloader_->downloadCallback_, download.Metrics());
    ASSERT_TRUE(download.WaitForKey());
    download.Stop();
    EXPECT_FALSE(download.HadError());
    EXPECT_EQ(download.Metrics()->GetTotalDownloadCount(), 1);
    ASSERT_EQ(m3u8->keyLen_, TEST_AES_KEY_SIZE);
    EXPECT_EQ(std::string(reinterpret_cast<const char*>(m3u8->key_), m3u8->keyLen_), TEST_KEY_BYTES);
    EXPECT_TRUE(m3u8->isDecryptKeyReady_.load());
    m3u8->GetKeyInfos(keys);
    auto key = keys.find(index);
    ASSERT_NE(key, keys.end());
    ASSERT_EQ(key->second.keyLen_, TEST_AES_KEY_SIZE);
    EXPECT_EQ(std::string(reinterpret_cast<const char*>(key->second.key_), key->second.keyLen_), TEST_KEY_BYTES);
}

HWTEST_F(M3u8UnitTest, ParseMediaStreamInfo_UnknownType_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXMEDIA,
        "URI=\"http://x/data.m3u8\",TYPE=DATA,GROUP-ID=\"data1\"");
    std::shared_ptr<Tag> tagPtr = tag;
    playlist.ParseMediaStreamInfo(tagPtr);
    EXPECT_EQ(playlist.audioList_.size(), 0u);
    ASSERT_EQ(playlist.subtitlesList_.size(), 0u);
}

HWTEST_F(M3u8UnitTest, GetDefaultMedieStream_NullInForcedList_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    auto media1 = std::make_shared<M3U8Media>("a", "uri", nullptr);
    media1->forced_ = true;
    stream->audioMedia_ = {media1, nullptr};
    playlist.GetDefaultMedieStream(stream, "audio");
    ASSERT_EQ(stream->defaultAudio_, media1);
}

HWTEST_F(M3u8UnitTest, GetDefaultMedieStream_NullInDefaultList_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    auto media1 = std::make_shared<M3U8Media>("a", "uri", nullptr);
    media1->isDefault_ = true;
    media1->characteristics_ = "";
    stream->audioMedia_ = {media1, nullptr};
    playlist.GetDefaultMedieStream(stream, "audio");
    ASSERT_EQ(stream->defaultAudio_, media1);
}

HWTEST_F(M3u8UnitTest, GetDefaultMedieStream_NullInAutoSelectList_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("v", "uri", nullptr);
    auto media1 = std::make_shared<M3U8Media>("a", "uri", nullptr);
    media1->autoSelect_ = true;
    stream->audioMedia_ = {media1, nullptr};
    playlist.GetDefaultMedieStream(stream, "audio");
    ASSERT_EQ(stream->defaultAudio_, media1);
}

HWTEST_F(M3u8UnitTest, UpdateByteRange_WrongSection_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.offset_ = 50;
    m3u8.length_ = 200;
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY, "METHOD=AES-128");
    M3U8Info info;
    m3u8.UpdateByteRange(tag, info);
    EXPECT_EQ(m3u8.length_, 200);
    ASSERT_EQ(m3u8.offset_, 50);
}

HWTEST_F(M3u8UnitTest, ParseMap_NonEmptyUriViaAddAttr_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isInterruptNeeded_ = true;
    m3u8.isHeaderReady_ = false;
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXMAP, "");
    auto uriAttr = std::make_shared<Attribute>("URI", "\"main.mp4\"");
    tag->AddAttribute(uriAttr);
    auto byteRangeAttr = std::make_shared<Attribute>("BYTERANGE", "10@0");
    tag->AddAttribute(byteRangeAttr);
    m3u8.ParseMap(tag);
    EXPECT_NE(m3u8.fmp4Header_, nullptr);
    ASSERT_EQ(m3u8.fmp4HeaderLen, 11u);
}

// ===== Branch coverage batch 3: targeted uncovered branches =====
HWTEST_F(M3u8UnitTest, UpdateMap_ValidAttributesTag_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isInterruptNeeded_ = true;
    m3u8.isHeaderReady_ = false;
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXMAP,
        "#EXT-X-MAP:URI=\"main.mp4\",BYTERANGE=\"50@0\"");
    M3U8Info info;
    m3u8.UpdateMap(tag, info);
    EXPECT_NE(m3u8.fmp4Header_, nullptr);
    ASSERT_EQ(m3u8.fmp4HeaderLen, 51u);
    delete[] m3u8.fmp4Header_;
    m3u8.fmp4Header_ = nullptr;
    m3u8.fmp4HeaderLen = 0;
    m3u8.downloadHeaderLen_ = 0;
}

HWTEST_F(M3u8UnitTest, UpdateDateRange_ValidAttributesTag_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.currentProgramDateTime_ = "2024-01-01T00:00:00";
    auto attrTag = std::make_shared<AttributesTag>(HlsTag::EXTXDATERANGE, "");
    auto idAttr = std::make_shared<Attribute>("ID", "\"test-id\"");
    attrTag->AddAttribute(idAttr);
    std::shared_ptr<Tag> tag = attrTag;
    M3U8Info info;
    m3u8.UpdateDateRange(tag, info);
    EXPECT_EQ(m3u8.dateRanges_.size(), 1u);
    ASSERT_EQ(m3u8.dateRanges_.front()->id, "test-id");
}

HWTEST_F(M3u8UnitTest, PrepareDecryptionKeys_NoMethodFirst_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag1 = std::make_shared<AttributesTag>(HlsTag::EXTXKEY,
        "URI=\"http://key.com/k\"");
    m3u8.PrepareDecrptionKeys(tag1);
    EXPECT_EQ(m3u8.keyInfos_.size(), 1u);
    std::shared_ptr<Tag> tag2 = std::make_shared<AttributesTag>(HlsTag::EXTXKEY,
        "METHOD=AES-128,URI=\"http://key.com/k\"");
    m3u8.PrepareDecrptionKeys(tag2);
    EXPECT_EQ(m3u8.keyInfos_.size(), 2u);
    ASSERT_EQ(*m3u8.method_, "AES-128");
}

HWTEST_F(M3u8UnitTest, SetDrmInfo_SmallValidBase64_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.keyUri_ = std::make_shared<std::string>("base64,AAAA");
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    EXPECT_FALSE(m3u8.SetDrmInfo(drmInfo));
    ASSERT_TRUE(drmInfo.empty());
}

HWTEST_F(M3u8UnitTest, ExtractNumber_DotAtZero_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    uint64_t maxVal = std::numeric_limits<uint64_t>::max();
    ASSERT_EQ(m3u8.ExtractNumber(".ts"), maxVal);
}

HWTEST_F(M3u8UnitTest, ProcessAllTags_StartCase_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::list<std::shared_ptr<Tag>> tags;
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXSTART, "TIME-OFFSET=10");
    tags.emplace_back(tag);
    playlist.ProcessAllTags(tags);
    EXPECT_DOUBLE_EQ(playlist.timeOffset_, 10.0);
    ASSERT_TRUE(playlist.isStart_);
}

HWTEST_F(M3u8UnitTest, ProcessAllTags_MediaCase_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::list<std::shared_ptr<Tag>> tags;
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXMEDIA,
        "URI=\"http://x/a.m3u8\",TYPE=AUDIO,GROUP-ID=\"grp1\"");
    tags.emplace_back(tag);
    playlist.ProcessAllTags(tags);
    ASSERT_EQ(playlist.audioList_.size(), 1u);
}

HWTEST_F(M3u8UnitTest, ProcessAllTags_StreamInfCase_001, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    std::list<std::shared_ptr<Tag>> tags;
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXSTREAMINF,
        "URI=\"http://example.com/v1.m3u8\",BANDWIDTH=1000,CODECS=\"avc1.42E01E\"");
    tags.emplace_back(tag);
    playlist.ProcessAllTags(tags);
    EXPECT_EQ(playlist.variants_.size(), 1u);
    ASSERT_TRUE(playlist.variants_.front()->isVideo_);
}

HWTEST_F(M3u8UnitTest, UpdateStart_PreciseNo_001, TestSize.Level0)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::shared_ptr<Tag> tag = std::make_shared<AttributesTag>(HlsTag::EXTXSTART, "PRECISE=NO");
    M3U8Info info;
    m3u8.UpdateStart(tag, info);
    EXPECT_FALSE(m3u8.precise_);
    ASSERT_TRUE(m3u8.isStart_);
}

HWTEST_F(M3u8UnitTest, ParseStart_Master_PreciseNo_001, TestSize.Level0)
{
    M3U8MasterPlaylist playlist("", "http://example.com");
    playlist.StartParsing();
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXSTART, "PRECISE=NO");
    std::shared_ptr<Tag> tagPtr = tag;
    playlist.ParseStart(tagPtr);
    EXPECT_FALSE(playlist.precise_);
    ASSERT_TRUE(playlist.isStart_);
}
}
