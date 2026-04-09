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
    //
    
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
    auto tag = std::make_shared<AttributesTag>(HlsTag::EXTXDISCONTINUITY, "123");
    auto attr1 =  std::make_shared<Attribute>("DURATION", "3.5");
    tag->AddAttribute(attr1);
    double duration;
    m3u8.GetExtInf(tag, duration);
    //check name
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
    m3u8.keyAllDownload_ = 1;

    bool result = m3u8.SaveData(data, len, false, 0);

    EXPECT_TRUE(result);
    EXPECT_EQ(m3u8.keyLen_, len);
    EXPECT_TRUE(m3u8.isDecryptKeyReady_);
    EXPECT_EQ(m3u8.keyAllDownload_, 1);
}

HWTEST_F(M3u8UnitTest, SAVE_DATA_INVALID_DATA, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
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

HWTEST_F(M3u8UnitTest, WAIT_KEY_DOWNLOAD_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "TestPlaylist");
    m3u8.keyAllDownload_ = 1;
    m3u8.isInterruptNeeded_ = false;
    m3u8.WaitKeyDownload();
    EXPECT_EQ(m3u8.keyAllDownload_, 0);
}

/**
 * @tc.name: EXTRACT_NUMBER_001
 * @tc.desc: test ExtractNumber with valid uri containing digits before extension
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, EXTRACT_NUMBER_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    // normal: number before extension
    EXPECT_EQ(m3u8.ExtractNumber("seg123.ts"), 123u);
    // with query string
    EXPECT_EQ(m3u8.ExtractNumber("seg456.ts?t=1"), 456u);
    // no digits before dot -> MAX
    EXPECT_EQ(m3u8.ExtractNumber("seg.ts"), UINT64_MAX);
    // empty uri -> MAX
    EXPECT_EQ(m3u8.ExtractNumber(""), UINT64_MAX);
    // no dot at all -> MAX
    EXPECT_EQ(m3u8.ExtractNumber("nodot"), UINT64_MAX);
}

/**
 * @tc.name: SHOULD_SKIP_SEGMENT_001
 * @tc.desc: test ShouldSkipSegment with skipped/non-skipped segments
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, SHOULD_SKIP_SEGMENT_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    // skippedSegments == 0 -> always false
    EXPECT_FALSE(m3u8.ShouldSkipSegment("seg5.ts", 0, 3));
    // uri sequence in [initSequence, initSequence+skipped) -> true
    EXPECT_TRUE(m3u8.ShouldSkipSegment("seg3.ts", 3, 2));  // seq=3, range=[2,5)
    EXPECT_TRUE(m3u8.ShouldSkipSegment("seg4.ts", 3, 2));  // seq=4, range=[2,5)
    // uri sequence outside range -> false
    EXPECT_FALSE(m3u8.ShouldSkipSegment("seg1.ts", 3, 2)); // seq=1, range=[2,5)
    EXPECT_FALSE(m3u8.ShouldSkipSegment("seg5.ts", 3, 2)); // seq=5 == initSequence+skipped -> false
}

/**
 * @tc.name: GET_KEY_INFOS_001
 * @tc.desc: test GetKeyInfos returns copy of keyInfos_
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, GET_KEY_INFOS_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    // empty case
    std::unordered_map<uint64_t, KeyInfo> result;
    m3u8.GetKeyInfos(result);
    EXPECT_TRUE(result.empty());
    // insert one key and verify
    {
        std::unique_lock<std::shared_mutex> lock(m3u8.keyMutex_);
        KeyInfo info;
        info.index_ = 1;
        m3u8.keyInfos_[1] = info;
    }
    m3u8.GetKeyInfos(result);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result.count(1), 1u);
}

/**
 * @tc.name: CHOOSE_STREAM_BY_STREAM_001
 * @tc.desc: test ChooseStreamByStream selects stream by streamId
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, CHOOSE_STREAM_BY_STREAM_001, TestSize.Level1)
{
    auto master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com");
    auto m3u8a = std::make_shared<M3U8>("https://example.com", "a");
    auto stream1 = std::make_shared<M3U8VariantStream>("a", "https://example.com/a", m3u8a);
    stream1->streamId_ = 1;
    stream1->bandWidth_ = 1000000;
    auto stream2 = std::make_shared<M3U8VariantStream>("b", "https://example.com/b", m3u8a);
    stream2->streamId_ = 2;
    stream2->bandWidth_ = 2000000;
    master->variants_.push_back(stream1);
    master->variants_.push_back(stream2);
    master->defaultVariant_ = stream1;
    // select streamId -1 -> no change (guard)
    master->ChooseStreamByStream(-1);
    EXPECT_EQ(master->defaultVariant_, stream1);
    // select streamId 2 -> switch to stream2
    master->ChooseStreamByStream(2);
    EXPECT_EQ(master->defaultVariant_, stream2);
}

/**
 * @tc.name: GET_CODEC_001
 * @tc.desc: test GetCodec returns correct codec string for VIDEO/AUDIO type
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, GET_CODEC_001, TestSize.Level1)
{
    auto master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com");
    // video codec
    std::string result = master->GetCodec(StreamType::VIDEO, "avc1.42c01e,mp4a.40.2");
    EXPECT_FALSE(result.empty());
    // audio codec
    result = master->GetCodec(StreamType::AUDIO, "avc1.42c01e,mp4a.40.2");
    EXPECT_FALSE(result.empty());
    // empty codecs
    result = master->GetCodec(StreamType::VIDEO, "");
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: GET_MIME_TYPE_001
 * @tc.desc: test GetMimeType returns video for known video codec without mime type
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, GET_MIME_TYPE_001, TestSize.Level1)
{
    auto master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com");
    std::string result = master->GetMimeType("avc1.42c01e");
    EXPECT_EQ(result, "video/avc1");
    // audio only codec -> empty
    result = master->GetMimeType("mp4a.40.2");
    EXPECT_TRUE(result.empty());
    // empty input -> empty
    result = master->GetMimeType("");
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: BIND_VIDEO_MEDIA_001
 * @tc.desc: test BindVideoMedia with subtitles only (audioList_ empty)
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, BIND_VIDEO_MEDIA_001, TestSize.Level1)
{
    auto master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com");
    auto m3u8 = std::make_shared<M3U8>("https://example.com", "");
    auto stream = std::make_shared<M3U8VariantStream>("v", "https://example.com/v", m3u8);
    stream->streamId_ = 1;
    master->variants_.push_back(stream);
    auto media = std::make_shared<M3U8Media>("sub", "https://example.com/sub", m3u8);
    media->streamId_ = 2;
    media->type_ = M3U8MediaType::M3U8_MEDIA_TYPE_SUBTITLES;
    master->subtitlesList_.push_back(media);
    // audioList_ empty, subtitlesList_ non-empty -> subtitles path
    master->BindVideoMedia("subtitle");
    EXPECT_FALSE(stream->subtitlesMedia_.empty());
}

/**
 * @tc.name: M3U8_FRAGMENT_COPY_CONSTRUCTOR_001
 * @tc.desc: test M3U8Fragment copy constructor with non-null key/iv copies data
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, M3U8_FRAGMENT_COPY_CONSTRUCTOR_001, TestSize.Level1)
{
    M3U8Fragment orig("http://example.com/seg.ts", 5.0, 1, false);
    uint8_t key[16];
    uint8_t iv[16];
    for (int i = 0; i < 16; i++) {
        key[i] = static_cast<uint8_t>(i);
        iv[i] = static_cast<uint8_t>(i + 16);
    }
    // key and iv non-null: copy branch
    M3U8Fragment copy(orig, key, iv);
    EXPECT_EQ(copy.uri_, orig.uri_);
    EXPECT_DOUBLE_EQ(copy.duration_, orig.duration_);
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(copy.key_[i], key[i]);
        EXPECT_EQ(copy.iv_[i], iv[i]);
    }
    // key/iv null: early return branch
    M3U8Fragment nullCopy(orig, nullptr, nullptr);
    EXPECT_EQ(nullCopy.uri_, orig.uri_);
    // key_[0] stays 0 since no copy happened
    EXPECT_EQ(nullCopy.key_[0], 0);
}

/**
 * @tc.name: M3U8_UPDATE_EXT_DEFINE_001
 * @tc.desc: test M3U8::Update with EXT-X-DEFINE tag branch
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, M3U8_UPDATE_EXT_DEFINE_001, TestSize.Level1)
{
    // playlist with EXT-X-DEFINE but no IMPORT -> tagMasterMap branch false
    M3U8 m3u8("http://example.com/test.m3u8", "");
    std::string playlist =
        "#EXTM3U\n"
        "#EXT-X-DEFINE:NAME=\"var1\",VALUE=\"val1\"\n"
        "#EXT-X-TARGETDURATION:5\n"
        "#EXTINF:5.0,\n"
        "seg.ts\n"
        "#EXT-X-ENDLIST\n";
    bool ret = m3u8.Update(playlist, true);
    EXPECT_TRUE(ret);
    // playlist with EXT-X-DEFINE + IMPORT: enter tagMasterMap_ branch
    M3U8 m3u8b("http://example.com/test.m3u8?token=abc", "");
    m3u8b.tagMasterMap_["var1"] = "val1";
    std::string playlist2 =
        "#EXTM3U\n"
        "#EXT-X-DEFINE:IMPORT=\"var1\"\n"
        "#EXT-X-TARGETDURATION:5\n"
        "#EXTINF:5.0,\n"
        "seg.ts\n"
        "#EXT-X-ENDLIST\n";
    ret = m3u8b.Update(playlist2, false);
    EXPECT_TRUE(ret);
    EXPECT_FALSE(m3u8b.files_.empty());
}

/**
 * @tc.name: M3U8_SAVE_DATA_SESSION_KEY_001
 * @tc.desc: test SaveData with isSessionKey_=true (skip keyAllDownload_ decrement)
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, M3U8_SAVE_DATA_SESSION_KEY_001, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.isSessionKey_ = true;
    m3u8.keyAllDownload_.store(1);
    uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint32_t ret = m3u8.SaveData(data, 4, false, 0);
    EXPECT_EQ(ret, 4u);
    EXPECT_TRUE(m3u8.isDecryptKeyReady_);
    // isSessionKey_=true, so keyAllDownload_ should NOT be decremented
    EXPECT_EQ(m3u8.keyAllDownload_.load(), 1);
}

/**
 * @tc.name: M3U8_GET_LIVE_UPDATE_GAP_002
 * @tc.desc: test GetLiveUpdateGap returns non-zero when minFragDuration_ > 0
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, M3U8_GET_LIVE_UPDATE_GAP_002, TestSize.Level1)
{
    M3U8 m3u8(testUri, "");
    // minFragDuration_ > 0: (duration-1)/2 branch
    m3u8.minFragDuration_.store(5001);
    size_t gap = m3u8.GetLiveUpdateGap();
    EXPECT_EQ(gap, (5001 - 1) / 2);
}

/**
 * @tc.name: SAVE_MAP_DATA_002
 * @tc.desc: test SaveMapData when fmp4Header_ is already allocated (no re-alloc)
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, SAVE_MAP_DATA_002, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    constexpr uint32_t BUF_SIZE = 256;
    m3u8.fmp4Header_ = new uint8_t[BUF_SIZE];
    m3u8.fmp4HeaderLen = BUF_SIZE;
    m3u8.downloadHeaderLen_ = 0;
    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t ret = m3u8.SaveMapData(data, sizeof(data), false);
    EXPECT_EQ(ret, sizeof(data));
    EXPECT_EQ(m3u8.downloadHeaderLen_, static_cast<uint32_t>(sizeof(data)));
    // verify data was copied
    for (size_t i = 0; i < sizeof(data); i++) {
        EXPECT_EQ(m3u8.fmp4Header_[i], data[i]);
    }
}

/**
 * @tc.name: ADD_FILE_002
 * @tc.desc: test AddFile branch when minFragDuration_ is 0 (initial) and duration > 0
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, ADD_FILE_002, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    m3u8.minFragDuration_.store(0);
    auto fragment = std::make_shared<M3U8Fragment>("http://example.com/seg.ts", 5.0, 0, false);
    // duration = 5000000 (5s in us): minFragDuration_ <= 0 -> store
    m3u8.AddFile(fragment, 5000000);
    EXPECT_EQ(m3u8.minFragDuration_.load(), static_cast<size_t>(5000000));
    // second add with larger duration: 5000000 <= 10000000 so no update
    auto fragment2 = std::make_shared<M3U8Fragment>("http://example.com/seg2.ts", 10.0, 1, false);
    m3u8.AddFile(fragment2, 10000000);
    // minFragDuration_ stays 5000000
    EXPECT_EQ(m3u8.minFragDuration_.load(), static_cast<size_t>(5000000));
    // third add with smaller duration: 3000000 < 5000000 -> update
    auto fragment3 = std::make_shared<M3U8Fragment>("http://example.com/seg3.ts", 3.0, 2, false);
    m3u8.AddFile(fragment3, 3000000);
    EXPECT_EQ(m3u8.minFragDuration_.load(), static_cast<size_t>(3000000));
}

/**
 * @tc.name: IS_NEAR_TO_INIT_RESOLUTION_002
 * @tc.desc: test IsNearToInitResolution returns true when currentDelta < choosedDelta
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, IS_NEAR_TO_INIT_RESOLUTION_002, TestSize.Level1)
{
    auto master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com");
    master->StartParsing();
    auto m3u8 = std::make_shared<M3U8>(testUri, "");
    // initResolution_ = 1920*1080 = 2073600
    master->initResolution_ = 1920 * 1080;
    auto choosed = std::make_shared<M3U8VariantStream>(testUri, testUri, m3u8);
    choosed->width_ = 3840; choosed->height_ = 2160; // delta=3840*2160 - 2073600 >> 0
    auto current = std::make_shared<M3U8VariantStream>(testUri, testUri, m3u8);
    current->width_ = 1920; current->height_ = 1080; // delta = 0
    // currentDelta(0) < choosedDelta(big) -> true
    EXPECT_TRUE(master->IsNearToInitResolution(choosed, current));
    // same delta but current bandwidth smaller -> also true
    auto choosed2 = std::make_shared<M3U8VariantStream>(testUri, testUri, m3u8);
    choosed2->width_ = 1920; choosed2->height_ = 1080; // same delta=0
    choosed2->bandWidth_ = 3000000;
    current->bandWidth_ = 1000000; // smaller
    EXPECT_TRUE(master->IsNearToInitResolution(choosed2, current));
}

/**
 * @tc.name: GET_RESOLUTION_DELTA_001
 * @tc.desc: test GetResolutionDelta both branches: resolution > initResolution_ and <=
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, GET_RESOLUTION_DELTA_001, TestSize.Level1)
{
    auto master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com");
    master->initResolution_ = 1920 * 1080; // 2073600
    // resolution > initResolution_ branch
    uint32_t delta1 = master->GetResolutionDelta(3840, 2160); // 3840*2160=8294400 > 2073600
    EXPECT_EQ(delta1, 8294400u - 2073600u);
    // resolution <= initResolution_ branch
    uint32_t delta2 = master->GetResolutionDelta(640, 480); // 640*480=307200 < 2073600
    EXPECT_EQ(delta2, 2073600u - 307200u);
}

/**
 * @tc.name: GET_DEFAULT_MEDIE_STREAM_001
 * @tc.desc: test GetDefaultMedieStream: forced -> isDefault -> autoSelect -> fallback
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, GET_DEFAULT_MEDIE_STREAM_001, TestSize.Level1)
{
    auto master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com");
    auto m3u8 = std::make_shared<M3U8>(testUri, "");
    auto stream = std::make_shared<M3U8VariantStream>(testUri, testUri, m3u8);

    // build audioMedia_ with forced one
    auto mediaForced = std::make_shared<M3U8Media>("forced", "http://example.com/a1.m3u8", m3u8);
    mediaForced->streamId_ = 10;
    mediaForced->forced_ = true;
    auto mediaDefault = std::make_shared<M3U8Media>("default", "http://example.com/a2.m3u8", m3u8);
    mediaDefault->streamId_ = 11;
    mediaDefault->isDefault_ = true;
    stream->audioMedia_.push_back(mediaForced);
    stream->audioMedia_.push_back(mediaDefault);

    // forced branch: should pick forced
    master->GetDefaultMedieStream(stream, "audio");
    ASSERT_NE(stream->defaultAudio_, nullptr);
    EXPECT_EQ(stream->defaultAudio_->streamId_, 10u);

    // clear forced, keep only isDefault_ one -> isDefault branch
    stream->audioMedia_.clear();
    stream->defaultAudio_ = nullptr;
    auto mediaDefault2 = std::make_shared<M3U8Media>("def2", "http://example.com/a3.m3u8", m3u8);
    mediaDefault2->streamId_ = 12;
    mediaDefault2->isDefault_ = true;
    mediaDefault2->characteristics_ = "";
    stream->audioMedia_.push_back(mediaDefault2);
    master->GetDefaultMedieStream(stream, "audio");
    EXPECT_EQ(stream->defaultAudio_->streamId_, 12u);

    // clear, only autoSelect_ -> autoSelect branch
    stream->audioMedia_.clear();
    stream->defaultAudio_ = nullptr;
    auto mediaAuto = std::make_shared<M3U8Media>("auto", "http://example.com/a4.m3u8", m3u8);
    mediaAuto->streamId_ = 13;
    mediaAuto->autoSelect_ = true;
    stream->audioMedia_.push_back(mediaAuto);
    master->GetDefaultMedieStream(stream, "audio");
    EXPECT_EQ(stream->defaultAudio_->streamId_, 13u);

    // clear, no forced/default/autoSelect -> fallback to back()
    stream->audioMedia_.clear();
    stream->defaultAudio_ = nullptr;
    auto mediaPlain = std::make_shared<M3U8Media>("plain", "http://example.com/a5.m3u8", m3u8);
    mediaPlain->streamId_ = 14;
    stream->audioMedia_.push_back(mediaPlain);
    master->GetDefaultMedieStream(stream, "audio");
    EXPECT_EQ(stream->defaultAudio_->streamId_, 14u);
}

/**
 * @tc.name: CHOOSE_STREAM_BY_RESOLUTION_002
 * @tc.desc: test ChooseStreamByResolution when initResolution_ > 0 finds better stream
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, CHOOSE_STREAM_BY_RESOLUTION_002, TestSize.Level1)
{
    auto master = std::make_shared<M3U8MasterPlaylist>("", "https://example.com");
    master->StartParsing();
    auto m3u8 = std::make_shared<M3U8>(testUri, "");
    auto streamLow = std::make_shared<M3U8VariantStream>("low", "http://example.com/low", m3u8);
    streamLow->width_ = 640; streamLow->height_ = 480; streamLow->bandWidth_ = 1000000;
    auto streamHigh = std::make_shared<M3U8VariantStream>("high", "http://example.com/high", m3u8);
    streamHigh->width_ = 1920; streamHigh->height_ = 1080; streamHigh->bandWidth_ = 3000000;
    master->variants_.push_back(streamLow);
    master->variants_.push_back(streamHigh);
    master->defaultVariant_ = streamLow;
    // initResolution_ = 1920*1080: streamHigh exact match (delta=0) < streamLow large delta -> switch
    master->initResolution_ = 1920 * 1080;
    master->ChooseStreamByResolution();
    EXPECT_EQ(master->defaultVariant_, streamHigh);
}

/**
 * @tc.name: INIT_DOWNLOAD_HEADER_001
 * @tc.desc: test InitDownloadHeader with sourceLoader_ == nullptr branch
 * @tc.type: FUNC
 */
HWTEST_F(M3u8UnitTest, INIT_DOWNLOAD_HEADER_001, TestSize.Level1)
{
    // sourceLoader_ == nullptr -> Downloader("HlsSourceMap")
    M3U8 m3u8a("http://example.com/test.m3u8", "");
    m3u8a.InitDownloadHeader();
    EXPECT_NE(m3u8a.downloaderHeader_, nullptr);
}
}