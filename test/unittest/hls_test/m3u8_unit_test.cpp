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

#include <new>
#include <limits>
#define LOCAL true

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace testing::ext;
using namespace std;
constexpr uint32_t MAX_LOOP = 16;
std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
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
    EXPECT_GE(duration, 0.0);
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

    M3U8 m3u81(testUri, "");
    m3u81.isHeaderReady_ = true;
    m3u81.ParseMap(tag);
    EXPECT_EQ(m3u81.fmp4Header_, nullptr);

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
    std::string url = "http://example.com/test.m3u8";
    auto m3u8 = std::make_shared<M3U8>(testUri, "");
    m3u8->uri_ = url;
    m3u8->keyUri_ = std::make_shared<std::string>(url);
    m3u8->keyIndex_ = 1;
    m3u8->DownloadKey(true);
    m3u8->DownloadMap(url, 0, 0);
    EXPECT_NE(m3u8->downloadHeaderRequest_, nullptr);
    EXPECT_NE(m3u8->downloadRequest_, nullptr);
    std::shared_ptr<M3U8MasterPlaylist> master = std::make_shared<M3U8MasterPlaylist>("", url);
    master->StartParsing();
    auto stream = std::make_shared<M3U8VariantStream>("test", url,
    std::make_shared<M3U8>(url, "test"));
    master->defaultVariant_ = stream;
    master->defaultVariant_->m3u8_ = nullptr;
    master->SetInterruptState(true);
    EXPECT_NE(master->defaultVariant_, nullptr);
    master->defaultVariant_ = nullptr;
    master->SetInterruptState(true);
    EXPECT_EQ(master->defaultVariant_, nullptr);
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
    auto startDateAttr = std::make_shared<Attribute>("START-DATE", "\"2024-01-01T00:01:00\"");
    tag->AddAttribute(startDateAttr);
    m3u8.ParseDateRange(tag);
    EXPECT_EQ(m3u8.dateRanges_.size(), 1u);
    EXPECT_GT(m3u8.dateRanges_.front()->start, 0);
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
    EXPECT_TRUE(true);
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
    EXPECT_NE(duration, 0u);
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
    EXPECT_NE(duration, 0u);
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
    EXPECT_NE(duration, 0u);
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
}
