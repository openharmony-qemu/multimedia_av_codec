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
#include "meta/meta_key.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
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

private:
    Format validFormat_;
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

    validFormat_.PutIntValue(Tag::VIDEO_WIDTH, 4096); // 4096: valid parameter
    validFormat_.PutIntValue(Tag::VIDEO_HEIGHT, 4096); // 4096: valid parameter
    validFormat_.PutIntValue(Tag::VIDEO_PIXEL_FORMAT, 1);
}

void CodecServerUnitTest::TearDown(void)
{
    server_ = nullptr;
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
 * @tc.name: Codec_Server_Constructor_001
 * @tc.desc: create video encoder of fcodec by name
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_001, TestSize.Level1)
{
    CreateFCodecByName();
    EXPECT_NE(server_->codecBase_, nullptr);
}

/**
 * @tc.name: Codec_Server_Constructor_002
 * @tc.desc: create video encoder of fcodec by mime
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_002, TestSize.Level1)
{
    CreateFCodecByMime();
    EXPECT_NE(server_->codecBase_, nullptr);
}

/**
 * @tc.name: Codec_Server_Constructor_003
 * @tc.desc: create video encoder of hcodec by name
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_003, TestSize.Level1)
{
    CreateHCodecByName();
    EXPECT_NE(server_->codecBase_, nullptr);
}

/**
 * @tc.name: Codec_Server_Constructor_004
 * @tc.desc: create video encoder of hcodec by mime
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_004, TestSize.Level1)
{
    CreateHCodecByMime();
    EXPECT_NE(server_->codecBase_, nullptr);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_001
 * @tc.desc: 1. create hcodec by name
 *           2. CodecFactory return nullptr
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_Invalid_001, TestSize.Level1)
{
    std::string codecName = "codec_name_mock";
    EXPECT_CALL(*codeclistCoreMock_, CodecListCoreCtor()).Times(1);
    EXPECT_CALL(*codeclistCoreMock_, CodecListCoreDtor()).Times(1);
    EXPECT_CALL(*codeclistCoreMock_, FindCodecType(codecName)).Times(1).WillOnce(Return(CodecType::AVCODEC_HCODEC));

    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName)).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr))).Times(0);
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr))).Times(0);

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName, API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_002
 * @tc.desc: 1. create hcodec by name
 *           2. SetCallback of AVCodecCallback return error
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_Invalid_002, TestSize.Level1)
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
        .WillOnce(Return(AVCS_ERR_UNKNOWN));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr))).Times(0);

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName, API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_003
 * @tc.desc: 1. create hcodec by name
 *           2. SetCallback of MediaCodecCallback return error
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_Invalid_003, TestSize.Level1)
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
        .WillOnce(Return(AVCS_ERR_UNKNOWN));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName, API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_004
 * @tc.desc: 1. create hcodec by name
 *           2. SetCallback of MediaCodecCallback return error
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_Invalid_004, TestSize.Level1)
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
        .WillOnce(Return(AVCS_ERR_UNKNOWN));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName, API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_005
 * @tc.desc: 1. invalid audio codecname
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_Invalid_005, TestSize.Level1)
{
    std::string codecName = "AudioDecoder.InvaildName";
    int32_t ret = server_->Init(AVCODEC_TYPE_AUDIO_ENCODER, false, codecName, API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
}

/**
 * @tc.name: State_Test_Configure_001
 * @tc.desc: 1. codec Configure
 */
HWTEST_F(CodecServerUnitTest, State_Test_Configure_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::INITIALIZED;

    EXPECT_CALL(*codecBaseMock_, Configure()).Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Configure(validFormat_);
    EXPECT_EQ(ret, AVCS_ERR_OK);
    EXPECT_EQ(server_->status_, CodecServer::CodecStatus::CONFIGURED);
}

/**
 * @tc.name: State_Test_Configure_Ivalid_001
 * @tc.desc: 1. Configure in invalid state
 */
HWTEST_F(CodecServerUnitTest, State_Test_Ivalid_Configure_001, TestSize.Level1)
{
    // valid: INITIALIZED
    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::UNINITIALIZED, CodecServer::CodecStatus::CONFIGURED,
        CodecServer::CodecStatus::RUNNING,       CodecServer::CodecStatus::FLUSHED,
        CodecServer::CodecStatus::END_OF_STREAM, CodecServer::CodecStatus::ERROR,
    };
    CreateHCodecByMime();
    for (auto &val : testList) {
        server_->status_ = val;
        int32_t ret = server_->Configure(validFormat_);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE) << "state: " << val << "\n";
    }
}

/**
 * @tc.name: State_Test_Configure_Ivalid_002
 * @tc.desc: 1. Configure with codecBase is nullptr
 */
HWTEST_F(CodecServerUnitTest, State_Test_Ivalid_Configure_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->codecBase_ = nullptr;

    int32_t ret = server_->Configure(validFormat_);
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: State_Test_Configure_Ivalid_003
 * @tc.desc: 1. Configure return err
 */
HWTEST_F(CodecServerUnitTest, State_Test_Ivalid_Configure_003, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::INITIALIZED;

    int32_t err = AVCS_ERR_UNKNOWN;
    EXPECT_CALL(*codecBaseMock_, Configure()).Times(1).WillOnce(Return(err));

    int32_t ret = server_->Configure(validFormat_);
    EXPECT_EQ(ret, err);
    EXPECT_EQ(server_->status_, CodecServer::CodecStatus::ERROR);
}

/**
 * @tc.name: State_Test_Start_001
 * @tc.desc: 1. codec Start
 */
HWTEST_F(CodecServerUnitTest, State_Test_Start_001, TestSize.Level1)
{
    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::CONFIGURED,
        CodecServer::CodecStatus::FLUSHED,
    };
    CreateHCodecByMime();
    EXPECT_CALL(*codecBaseMock_, Start()).Times(testList.size()).WillRepeatedly(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, GetOutputFormat()).Times(testList.size()).WillRepeatedly(Return(AVCS_ERR_OK));
    for (auto &val : testList) {
        server_->status_ = val;
        int32_t ret = server_->Start();
        EXPECT_EQ(ret, AVCS_ERR_OK);
        EXPECT_EQ(server_->status_, CodecServer::CodecStatus::RUNNING);
    }
}

/**
 * @tc.name: State_Test_Start_Ivalid_001
 * @tc.desc: 1. Start in invalid state
 */
HWTEST_F(CodecServerUnitTest, State_Test_Ivalid_Start_001, TestSize.Level1)
{
    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::INITIALIZED, CodecServer::CodecStatus::UNINITIALIZED,
        CodecServer::CodecStatus::RUNNING,     CodecServer::CodecStatus::END_OF_STREAM,
        CodecServer::CodecStatus::ERROR,
    };
    CreateHCodecByMime();
    for (auto &val : testList) {
        server_->status_ = val;
        int32_t ret = server_->Start();
        EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE) << "state: " << val << "\n";
    }
}

/**
 * @tc.name: State_Test_Start_Ivalid_002
 * @tc.desc: 1. Start with codecBase is nullptr
 */
HWTEST_F(CodecServerUnitTest, State_Test_Ivalid_Start_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::FLUSHED;
    server_->codecBase_ = nullptr;

    int32_t ret = server_->Start();
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: State_Test_Start_Ivalid_003
 * @tc.desc: 1. Start return err
 */
HWTEST_F(CodecServerUnitTest, State_Test_Ivalid_Start_003, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::FLUSHED;

    int32_t err = AVCS_ERR_UNKNOWN;
    EXPECT_CALL(*codecBaseMock_, Start).Times(1).WillOnce(Return(err));

    int32_t ret = server_->Start();
    EXPECT_EQ(ret, err);
    EXPECT_EQ(server_->status_, CodecServer::CodecStatus::ERROR);
}
} // namespace