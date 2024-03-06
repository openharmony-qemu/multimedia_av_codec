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

#define LOCAL true

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace testing::ext;
using namespace std;

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

// 测试 IsLive 方法
HWTEST_F(M3u8UnitTest, IsLiveTest, TestSize.Level1)
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

HWTEST_F(M3u8UnitTest, ParseKeyTest, TestSize.Level1) {
    auto attributesTag = std::make_shared<AttributesTag>();
    attributesTag->AddAttribute("METHOD", "\"SAMPLE-AES\"");
    attributesTag->AddAttribute("URI", "\"https://example.com/key\"");
    attributesTag->AddAttribute("IV", "\"0123456789ABCDEF\"");

    testM3u8->ParseKey(attributesTag);

    // 验证 method_, keyUri_ 和 iv_ 是否正确设置
    ASSERT_EQ(testM3u8->method_, "SAMPLE-AES");
    ASSERT_EQ(testM3u8->keyUri_, "https://example.com/key");
    // 验证 IV 是否正确解析
    uint8_t expectedIv[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, ...};
    for (int i = 0; i < 16; ++i) {
        ASSERT_EQ(testM3u8->iv_[i], expectedIv[i]);
    }
}

HWTEST_F(M3u8UnitTest, SaveDataTest, TestSize.Level1) {
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

HWTEST_F(M3u8UnitTest, Base64DecodeTest, TestSize.Level1) {
    const uint8_t src[] = "dGVzdCBzdHJpbmc="; // base64 编码的 "test string"
    uint8_t dest[20]; // 预期解码输出的大小
    uint32_t destSize = sizeof(dest);

    bool result = testM3u8->Base64Decode(src, sizeof(src) - 1, dest, &destSize);
    ASSERT_TRUE(result);
    ASSERT_EQ(destSize, 11u); // "test string" 的长度
    ASSERT_EQ(std::string(reinterpret_cast<char*>(dest), destSize), "test string");
}

HWTEST_F(M3u8UnitTest, SetDrmInfoTest, TestSize.Level1) {
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    // 设置 keyUri_ 为有效的 base64 编码字符串
    testM3u8->keyUri_ = "base64,VALID_BASE64_ENCODED_STRING";

    bool result = testM3u8->SetDrmInfo(drmInfo);
    ASSERT_TRUE(result);
    ASSERT_FALSE(drmInfo.empty());
    // 验证 drmInfo 是否包含预期的 UUID 和解码后的数据
}

HWTEST_F(M3u8UnitTest, StoreDrmInfosTest, TestSize.Level1) {
    std::multimap<std::string, std::vector<uint8_t>> drmInfo = {{"uuid1", {1, 2, 3}}, {"uuid2", {4, 5, 6}}};
    testM3u8->StoreDrmInfos(drmInfo);
    // 验证 localDrmInfos_ 是否正确包含了 drmInfo 的内容
    for (const auto &item : drmInfo) {
        auto range = testM3u8->localDrmInfos_.equal_range(item.first);
        ASSERT_NE(range.first, testM3u8->localDrmInfos_.end());
        ASSERT_EQ(range.first->second, item.second);
    }
}

HWTEST_F(M3u8UnitTest, ProcessDrmInfosTest, TestSize.Level1) {
    testM3u8->keyUri_ = "base64,VALID_BASE64_ENCODED_STRING";
    testM3u8->ProcessDrmInfos();
    // 验证 isDecryptAble_ 是否根据 DRM 信息的处理结果正确设置
    ASSERT_EQ(testM3u8->isDecryptAble_, testM3u8->localDrmInfos_.empty());
}


}