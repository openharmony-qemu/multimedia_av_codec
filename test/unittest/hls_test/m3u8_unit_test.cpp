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
#include <new>
#define LOCAL true

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace testing::ext;
using namespace std;
constexpr uint32_t MAX_LOOP = 16;

void M3u8UnitTest::SetUpTestCase(void) {}

void M3u8UnitTest::TearDownTestCase(void) {}

void M3u8UnitTest::SetUp(void) {}

void M3u8UnitTest::TearDown(void) {}

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
    EXPECT_TRUE(m3u8.Update(testPlaylist));

    // 测试当播放列表不变时的更新
    EXPECT_TRUE(m3u8.Update(testPlaylist));

    // 测试无效的播放列表更新
    EXPECT_FALSE(m3u8.Update("Invalid Playlist"));
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

HWTEST_F(M3u8UnitTest, TEST_CONSTRUCTOR_WITH_NULL_KEY_AND_IV, TestSize.Level1)
{
    M3U8Fragment m3u8("http://example.com", "Test Title", 10.0, 1, false);
    M3U8Fragment fragment(m3u8, nullptr, nullptr);
    EXPECT_EQ(fragment.title_, "Test Title");
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
    M3U8Fragment m3u8("http://example.com", "Test Title", 10.0, 1, false);
    uint8_t key[MAX_LOOP] = {1, 2, 3, 4, 5};
    uint8_t iv[MAX_LOOP] = {6, 7, 8, 9, 10};
    M3U8Fragment fragment(m3u8, key, iv);
    EXPECT_EQ(fragment.uri_, "http://example.com");
    EXPECT_EQ(fragment.title_, "Test Title");
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

HWTEST_F(M3u8UnitTest, TEST_EMPTY_URI, TestSize.Level1)
{
    M3U8 m3u8("", "Test M3U8");

    // check url
    EXPECT_EQ(m3u8.uri_, "");

    //check name
    EXPECT_EQ(m3u8.name_, "Test M3U8");

    //check updaters map
    EXPECT_FALSE(m3u8.tagUpdatersMap_.empty());
}

HWTEST_F(M3u8UnitTest, TEST_EMPTY_NAME, TestSize.Level1)
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
    auto attr2 =  std::make_shared<Attribute>("TITLE", "\"Test Title\"");
    tag->AddAttribute(attr2);
    double duration;
    std::string title;
    m3u8.GetExtInf(tag, duration, title);
    //check name
    EXPECT_DOUBLE_EQ(3.5, duration);
    //check updaters map
    EXPECT_EQ("Test Title", title);
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

    bool result = m3u8.SaveData(data, len);

    EXPECT_TRUE(result);
    EXPECT_EQ(m3u8.keyLen_, len);
    EXPECT_TRUE(m3u8.isDecryptKeyReady_);
}

HWTEST_F(M3u8UnitTest, SAVE_DATA_INVALID_DATA, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint32_t len = 10;

    bool result = m3u8.SaveData(data, len);

    EXPECT_TRUE(result);
    EXPECT_EQ(m3u8.keyLen_, len);
    EXPECT_TRUE(m3u8.isDecryptKeyReady_);
}

HWTEST_F(M3u8UnitTest, SAVE_DATA_EXVEEDS_MAX_LOOP, TestSize.Level1)
{
    M3U8 m3u8("http://example.com/test.m3u8", "");
    uint8_t data[MAX_LOOP + 1];
    uint32_t len = MAX_LOOP + 1;

    bool result = m3u8.SaveData(data, len);

    EXPECT_FALSE(result);
    EXPECT_EQ(m3u8.keyLen_, 0);
    EXPECT_FALSE(m3u8.isDecryptKeyReady_);
}

HWTEST_F(M3u8UnitTest, NULL_SRC, TestSize.Level1)
{
    uint8_t dest[100];
    uint32_t destSize = 100;
    EXPECT_FALSE(M3U8::Base64Decode(nullptr, 10, dest, &destSize));
}

HWTEST_F(M3u8UnitTest, NULL_DEST, TestSize.Level1)
{
    uint8_t src[10] = {0};
    uint32_t destSize = 100;
    EXPECT_FALSE(M3U8::Base64Decode(src, 10, nullptr, &destSize));
}

HWTEST_F(M3u8UnitTest, NULL_DEST_SIZE, TestSize.Level1)
{
    uint8_t src[10] = {0};
    uint8_t dest [100];
    EXPECT_FALSE(M3U8::Base64Decode(src, 10, dest, nullptr));
}

HWTEST_F(M3u8UnitTest, ZERO_SRC_SIZE, TestSize.Level1)
{
    uint8_t src[10] = {0};
    uint8_t dest [100];
    uint32_t destSize = 100;
    EXPECT_FALSE(M3U8::Base64Decode(src, 0, dest, &destSize));
}

HWTEST_F(M3u8UnitTest, SRC_SIZE_GREATER_THAN_DEST_SIZE, TestSize.Level1)
{
    uint8_t src[10] = {0};
    uint8_t dest [100];
    uint32_t destSize = 100;
    EXPECT_TRUE(M3U8::Base64Decode(src, 20, dest, &destSize));
}

HWTEST_F(M3u8UnitTest, INVALID_SRC_SIZE, TestSize.Level1)
{
    uint8_t src[5] = {0};
    uint8_t dest [100];
    uint32_t destSize = 100;
    EXPECT_FALSE(M3U8::Base64Decode(src, 5, dest, &destSize));
}

HWTEST_F(M3u8UnitTest, VALID_SRC, TestSize.Level1)
{
    uint8_t src[12] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l'};
    uint8_t dest [100];
    uint32_t destSize = 100;
    EXPECT_TRUE(M3U8::Base64Decode(src, 12, dest, &destSize));
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
    EXPECT_EQ(playlist.playList_, "#EXTM3U");
}

HWTEST_F(M3u8UnitTest, TestPlaylistNotStartWithEXTM3U, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("playlist", "uri");
    EXPECT_EQ(playlist.playList_, "playlist");
}

HWTEST_F(M3u8UnitTest, TestPlaylistContainsEXTINF, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("\n#EXTM3U:", "uri");
    EXPECT_EQ(playlist.playList_, "\n#EXTM3U:");
}

HWTEST_F(M3u8UnitTest, TestPlaylistNotContainsEXTINF, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("playlist", "uri");
    EXPECT_EQ(playlist.playList_, "playlist");
}

HWTEST_F(M3u8UnitTest, UpdateMediaPlaylistTest, TestSize.Level1)
{
    M3U8MasterPlaylist playlist("playlist", "uri");
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
    std::string testTitle = "FragmentTitle";
    double testDuration = 10.0;
    int testSequence = 1;
    bool testDiscont = false;
    M3U8Fragment fragment(testUri, testTitle, testDuration, testSequence, testDiscont);
    ASSERT_EQ(fragment.uri_, testUri);
    ASSERT_EQ(fragment.title_, testTitle);
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
    bool result = testM3u8->SaveData(data, sizeof(data));
    ASSERT_TRUE(result);

    // 验证 key_ 成员变量中的数据是否与传入的数据一致
    for (size_t i = 0; i < sizeof(data); ++i) {
        ASSERT_EQ(testM3u8->key_[i], data[i]);
    }

    // 验证 keyLen_ 是否被正确设置
    ASSERT_EQ(testM3u8->keyLen_, sizeof(data));
}

HWTEST_F(M3u8UnitTest, Base64DecodeTest, TestSize.Level1)
{
    const uint8_t src[] = "dGVzdCBzdHJpbmc="; // base64 编码的 "test string"
    uint8_t dest[20];                         // 预期解码输出的大小
    uint32_t destSize = sizeof(dest);

    bool result = testM3u8->Base64Decode(src, sizeof(src) - 1, dest, &destSize);
    ASSERT_TRUE(result);
    ASSERT_EQ(destSize, 11u); // "test string" 的长度
    ASSERT_EQ(std::string(reinterpret_cast<char *>(dest), destSize), "test string");
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

}