/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file expect in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <vector>
#include "avcodec_errors.h"
#include "codec_server.h"
#include "codecbase.h"
#include "codeclist_core.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing;
using namespace testing::ext;

namespace {
class CodecServerUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    void CreateFCodecByName();
    void CreateFCodecByMime();
    void CreateHCodecByName();
    void CreateHCodecByMime();
    std::shared_ptr<CodecBaseMock> codecBaseMock_ = nullptr;
    std::shared_ptr<CodecListCoreMock> codeclistCoreMock_ = nullptr;
    std::shared_ptr<CodecServer> server_ = nullptr;
};

void CodecServerUnitTest::SetUpTestCase(void) {}

void CodecServerUnitTest::TearDownTestCase(void) {}

void CodecServerUnitTest::SetUp(void)
{
    codecBaseMock_ = std::make_shared<CodecBaseMock>();
    CodecBase::RegisterMock(codecBaseMock_);
    codeclistCoreMock_ = std::make_shared<CodecListCoreMock>();
    CodecListCore::RegisterMock(codeclistCoreMock_);

    server_ = std::static_pointer_cast<CodecServer>(CodecServer::Create());
    EXPECT_NE(server_, nullptr);
}

void CodecServerUnitTest::TearDown(void)
{
    // mock object
    codecBaseMock_ = nullptr;
    codeclistCoreMock_ = nullptr;
}

void CodecServerUnitTest::CreateFCodecByName()
{
    std::string codecName = "codec_name_mock";
    EXPECT_CALL(*codeclistCoreMock_, CodecListCoreCtor()).Times(1);
    EXPECT_CALL(*codeclistCoreMock_, CodecListCoreDtor()).Times(1);
    EXPECT_CALL(*codeclistCoreMock_, FindCodecType(codecName))
        .Times(1)
        .WillOnce(Return(CodecType::AVCODEC_VIDEO_CODEC));

    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName(codecName)).Times(1);
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CodecBaseDtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName, API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

void CodecServerUnitTest::CreateFCodecByMime()
{
    std::string codecName = "codec_name_mock";
    std::string codecMime = "codec_mime_mock";
    EXPECT_CALL(*codeclistCoreMock_, CodecListCoreCtor()).Times(AtLeast(1));
    EXPECT_CALL(*codeclistCoreMock_, CodecListCoreDtor()).Times(AtLeast(1));
    EXPECT_CALL(*codeclistCoreMock_, FindEncoder).Times(1).WillOnce(Return(codecName));
    EXPECT_CALL(*codeclistCoreMock_, FindCodecType(codecName))
        .Times(1)
        .WillOnce(Return(CodecType::AVCODEC_VIDEO_CODEC));

    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName(codecName)).Times(1);
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CodecBaseDtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, true, codecMime, API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

void CodecServerUnitTest::CreateHCodecByName()
{
    std::string codecName = "codec_name_mock";
    EXPECT_CALL(*codeclistCoreMock_, CodecListCoreCtor()).Times(1);
    EXPECT_CALL(*codeclistCoreMock_, CodecListCoreDtor()).Times(1);
    EXPECT_CALL(*codeclistCoreMock_, FindCodecType(codecName)).Times(1).WillOnce(Return(CodecType::AVCODEC_HCODEC));

    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CodecBaseDtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName, API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

void CodecServerUnitTest::CreateHCodecByMime()
{
    std::string codecName = "codec_name_mock";
    std::string codecMime = "codec_mime_mock";
    EXPECT_CALL(*codeclistCoreMock_, CodecListCoreCtor()).Times(AtLeast(1));
    EXPECT_CALL(*codeclistCoreMock_, CodecListCoreDtor()).Times(AtLeast(1));
    EXPECT_CALL(*codeclistCoreMock_, FindEncoder).Times(1).WillOnce(Return(codecName));
    EXPECT_CALL(*codeclistCoreMock_, FindCodecType(codecName)).Times(1).WillOnce(Return(CodecType::AVCODEC_HCODEC));

    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CodecBaseDtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, true, codecMime, API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: codec_server_constructor_001
 * @tc.desc: create video encoder of fcodec by name
 */
HWTEST_F(CodecServerUnitTest, codec_server_constructor_001, TestSize.Level1)
{
    CreateFCodecByName();
    EXPECT_NE(server_->codecBase_, nullptr);
    server_ = nullptr;
}

/**
 * @tc.name: codec_server_constructor_002
 * @tc.desc: create video encoder of fcodec by mime
 */
HWTEST_F(CodecServerUnitTest, codec_server_constructor_002, TestSize.Level1)
{
    CreateFCodecByMime();
    EXPECT_NE(server_->codecBase_, nullptr);
    server_ = nullptr;
}

/**
 * @tc.name: codec_server_constructor_003
 * @tc.desc: create video encoder of hcodec by name
 */
HWTEST_F(CodecServerUnitTest, codec_server_constructor_003, TestSize.Level1)
{
    CreateHCodecByName();
    EXPECT_NE(server_->codecBase_, nullptr);
    server_ = nullptr;
}

/**
 * @tc.name: codec_server_constructor_004
 * @tc.desc: create video encoder of hcodec by mime
 */
HWTEST_F(CodecServerUnitTest, codec_server_constructor_004, TestSize.Level1)
{
    CreateHCodecByMime();
    EXPECT_NE(server_->codecBase_, nullptr);
    server_ = nullptr;
}
} // namespace