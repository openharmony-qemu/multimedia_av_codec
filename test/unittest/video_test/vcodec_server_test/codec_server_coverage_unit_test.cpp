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
#include "unittest_utils.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"
#include "wm/window.h"
#include "media_description.h"
#define EXPECT_CALL_GET_HCODEC_CAPS_MOCK                                                                               \
    EXPECT_CALL(*codecBaseMock_, GetHCapabilityList).Times(AtLeast(1)).WillRepeatedly
#define EXPECT_CALL_GET_FCODEC_CAPS_MOCK                                                                               \
    EXPECT_CALL(*codecBaseMock_, GetFCapabilityList).Times(AtLeast(1)).WillRepeatedly
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
    std::shared_ptr<CodecServer> server_ = nullptr;

private:
    Format validFormat_;
};

class CodecParamCheckerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(void){};
    void TearDown(void){};
};

void CodecServerUnitTest::SetUpTestCase(void) {}

void CodecServerUnitTest::TearDownTestCase(void) {}

void CodecServerUnitTest::SetUp(void)
{
    codecBaseMock_ = std::make_shared<CodecBaseMock>();
    CodecBase::RegisterMock(codecBaseMock_);

    server_ = std::static_pointer_cast<CodecServer>(CodecServer::Create());
    EXPECT_NE(server_, nullptr);

    validFormat_.PutIntValue(Tag::VIDEO_WIDTH, 4096);  // 4096: valid parameter
    validFormat_.PutIntValue(Tag::VIDEO_HEIGHT, 4096); // 4096: valid parameter
    validFormat_.PutIntValue(Tag::VIDEO_PIXEL_FORMAT, 1);
}

void CodecServerUnitTest::TearDown(void)
{
    server_ = nullptr;
    // mock object
    codecBaseMock_ = nullptr;
    validFormat_ = Format();
}

void CodecServerUnitTest::CreateFCodecByName()
{
    std::string codecName = "video.F.Decoder.Name.00";

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName(codecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
        *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

void CodecServerUnitTest::CreateFCodecByMime()
{
    std::string codecName = "video.F.Decoder.Name.00";
    std::string codecMime = CODEC_MIME_MOCK_00;

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName(codecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, true, codecMime,
        *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

void CodecServerUnitTest::CreateHCodecByName()
{
    std::string codecName = "video.H.Encoder.Name.00";

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
        *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

void CodecServerUnitTest::CreateHCodecByMime()
{
    std::string codecName = "video.H.Encoder.Name.00";
    std::string codecMime = CODEC_MIME_MOCK_00;

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, true, codecMime,
        *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
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
 * @tc.name: Codec_Server_Constructor_005
 * @tc.desc: 1. failed to create video decoder of hcodec by mime
 *           2. return fcodec
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_005, TestSize.Level1)
{
    std::string codecMime = CODEC_MIME_MOCK_01;
    std::string hcodecName = "video.H.Decoder.Name.01";
    std::string fcodecName = "video.F.Decoder.Name.01";

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(hcodecName)).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName(fcodecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_DECODER, true, codecMime,
        *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    EXPECT_NE(server_->codecBase_, nullptr);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_001
 * @tc.desc: 1. create hcodec by name
 *           2. CodecFactory return nullptr
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_Invalid_001, TestSize.Level1)
{
    std::string codecName = "video.H.Encoder.Name.00";

    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName)).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr))).Times(0);
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr))).Times(0);

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
            *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_002
 * @tc.desc: 1. create hcodec by name
 *           2. SetCallback of AVCodecCallback return error
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_Invalid_002, TestSize.Level1)
{
    std::string codecName = "video.H.Encoder.Name.00";

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_INVALID_OPERATION));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr))).Times(0);

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
            *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_003
 * @tc.desc: 1. create hcodec by name
 *           2. SetCallback of MediaCodecCallback return error
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_Invalid_003, TestSize.Level1)
{
    std::string codecName = "video.H.Encoder.Name.00";

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_INVALID_OPERATION));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
            *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_004
 * @tc.desc: 1. create hcodec by name
 *           2. SetCallback of MediaCodecCallback return error
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_Invalid_004, TestSize.Level1)
{
    std::string codecName = "video.H.Encoder.Name.00";

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_INVALID_OPERATION));

    int32_t ret =
        server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
            *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_005
 * @tc.desc: 1. invalid audio codecname
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_Invalid_005, TestSize.Level1)
{
    std::string codecName = "AudioDecoder.InvaildName";
    int32_t ret =
        server_->Init(AVCODEC_TYPE_AUDIO_ENCODER, false, codecName,
            *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
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
 * @tc.name: State_Test_Configure_Invalid_001
 * @tc.desc: 1. Configure in invalid state
 */
HWTEST_F(CodecServerUnitTest, State_Test_Invalid_Configure_001, TestSize.Level1)
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
 * @tc.name: State_Test_Configure_Invalid_002
 * @tc.desc: 1. Configure with codecBase is nullptr
 */
HWTEST_F(CodecServerUnitTest, State_Test_Invalid_Configure_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->codecBase_ = nullptr;

    int32_t ret = server_->Configure(validFormat_);
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: State_Test_Configure_Invalid_003
 * @tc.desc: 1. Configure return err
 */
HWTEST_F(CodecServerUnitTest, State_Test_Invalid_Configure_003, TestSize.Level1)
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
 * @tc.name: State_Test_Start_Invalid_001
 * @tc.desc: 1. Start in invalid state
 */
HWTEST_F(CodecServerUnitTest, State_Test_Invalid_Start_001, TestSize.Level1)
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
 * @tc.name: State_Test_Start_Invalid_002
 * @tc.desc: 1. Start with codecBase is nullptr
 */
HWTEST_F(CodecServerUnitTest, State_Test_Invalid_Start_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::FLUSHED;
    server_->codecBase_ = nullptr;

    int32_t ret = server_->Start();
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: State_Test_Start_Invalid_003
 * @tc.desc: 1. Start return err
 */
HWTEST_F(CodecServerUnitTest, State_Test_Invalid_Start_003, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::FLUSHED;

    int32_t err = AVCS_ERR_UNKNOWN;
    EXPECT_CALL(*codecBaseMock_, Start).Times(1).WillOnce(Return(err));

    int32_t ret = server_->Start();
    EXPECT_EQ(ret, err);
    EXPECT_EQ(server_->status_, CodecServer::CodecStatus::ERROR);
}

sptr<Surface> CreateSurface()
{
    sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
    option->SetWindowRect({0, 0, 1280, 1000}); // 1280: width, 1000: height
    option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_LAUNCHING);
    option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
    auto window = Rosen::Window::Create("vcodec_unittest", option);
    if (window == nullptr || window->GetSurfaceNode() == nullptr) {
        std::cout << "Fatal: Create window fail" << std::endl;
        return nullptr;
    }
    window->Show();
    return window->GetSurfaceNode()->GetSurface();
}

/**
 * @tc.name: CreateInputSurface_Valid_Test_001
 * @tc.desc: 1. codec CreateInputSurface in valid state
 */
HWTEST_F(CodecServerUnitTest, CreateInputSurface_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        EXPECT_CALL(*codecBaseMock_, CreateInputSurface()).Times(1).WillOnce(Return(surface));
        sptr<Surface> ret = server_->CreateInputSurface();
        EXPECT_EQ(ret, surface);
    }
}

/**
 * @tc.name: CreateInputSurface_Invalid_Test_001
 * @tc.desc: 1. CreateInputSurface in invalid state
 */
HWTEST_F(CodecServerUnitTest, CreateInputSurface_Invalid_Test_001, TestSize.Level1)
{
    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::INITIALIZED, CodecServer::CodecStatus::UNINITIALIZED,
        CodecServer::CodecStatus::RUNNING,     CodecServer::CodecStatus::END_OF_STREAM,
        CodecServer::CodecStatus::ERROR,       CodecServer::CodecStatus::FLUSHED,
    };
    CreateHCodecByMime();

    for (auto &val : testList) {
        server_->status_ = val;
        sptr<Surface> ret = server_->CreateInputSurface();
        EXPECT_EQ(ret, nullptr) << "state: " << val << "\n";
    }
}

/**
 * @tc.name: CreateInputSurface_Invalid_Test_002
 * @tc.desc: 2. CreateInputSurface with codecBase is nullptr
 */
HWTEST_F(CodecServerUnitTest, CreateInputSurface_Invalid_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    server_->codecBase_ = nullptr;
    sptr<Surface> ret = server_->CreateInputSurface();
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name: SetInputSurface_Valid_Test_001
 * @tc.desc: 1. codec SetInputSurface
 */
HWTEST_F(CodecServerUnitTest, SetInputSurface_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        EXPECT_CALL(*codecBaseMock_, SetInputSurface(surface)).Times(1).WillOnce(Return(AVCS_ERR_OK));
        int32_t ret = server_->SetInputSurface(surface);
        EXPECT_EQ(ret, AVCS_ERR_OK);
    }
}

/**
 * @tc.name: SetInputSurface_Invalid_Test_S001
 * @tc.desc: 1. SetInputSurface in invalid state
 */
HWTEST_F(CodecServerUnitTest, SetInputSurface_Invalid_Test_S001, TestSize.Level1)
{
    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::INITIALIZED, CodecServer::CodecStatus::UNINITIALIZED,
        CodecServer::CodecStatus::RUNNING,     CodecServer::CodecStatus::END_OF_STREAM,
        CodecServer::CodecStatus::ERROR,       CodecServer::CodecStatus::FLUSHED,
    };
    CreateHCodecByMime();

    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        for (auto &val : testList) {
            server_->status_ = val;
            int32_t ret = server_->SetInputSurface(surface);
            EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE) << "state: " << val << "\n";
        }
    }
}

/**
 * @tc.name: SetInputSurface_Invalid_Test_002
 * @tc.desc: 2. SetInputSurface with codecBase is nullptr
 */
HWTEST_F(CodecServerUnitTest, SetInputSurface_Invalid_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    server_->codecBase_ = nullptr;
    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        int32_t ret = server_->SetInputSurface(surface);
        EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
    }
}

/**
 * @tc.name: SetOutputSurface_Valid_Test_001
 * @tc.desc: 1. codec SetOutputSurface
 */
HWTEST_F(CodecServerUnitTest, SetOutputSurface_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isModeConfirmed_ = true;
    server_->isSurfaceMode_ = true;

    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::CONFIGURED, CodecServer::CodecStatus::FLUSHED,
        CodecServer::CodecStatus::RUNNING,     CodecServer::CodecStatus::END_OF_STREAM,
    };
    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        EXPECT_CALL(*codecBaseMock_, SetOutputSurface(surface))
        .Times(testList.size())
        .WillRepeatedly(Return(AVCS_ERR_OK));

        for (auto &val : testList) {
            server_->status_ = val;
            int32_t ret = server_->SetOutputSurface(surface);
            EXPECT_EQ(ret, AVCS_ERR_OK) << "state: " << val << "\n";
        }
    }
}

/**
 * @tc.name: SetOutputSurface_Valid_Test_002
 * @tc.desc: 2. codec SetOutputSurface
 */
HWTEST_F(CodecServerUnitTest, SetOutputSurface_Valid_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isModeConfirmed_ = false;
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        EXPECT_CALL(*codecBaseMock_, SetOutputSurface(surface)).Times(1).WillOnce(Return(AVCS_ERR_OK));
        int32_t ret = server_->SetOutputSurface(surface);
        EXPECT_EQ(ret, AVCS_ERR_OK);
    }
}

/**
 * @tc.name: SetOutputSurface_Invalid_Test_001
 * @tc.desc: 1. SetOutputSurface in invalid mode
 */
HWTEST_F(CodecServerUnitTest, SetOutputSurface_Invalid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isModeConfirmed_ = true;
    server_->isSurfaceMode_ = false;
    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        int32_t ret = server_->SetOutputSurface(surface);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
    }
}

/**
 * @tc.name: SetOutputSurface_Invalid_Test_002
 * @tc.desc: 2. SetOutputSurface in invalid state
 */
HWTEST_F(CodecServerUnitTest, SetOutputSurface_Invalid_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isModeConfirmed_ = true;
    server_->isSurfaceMode_ = true;
    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::INITIALIZED,
        CodecServer::CodecStatus::UNINITIALIZED,
        CodecServer::CodecStatus::ERROR,
    };

    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        int32_t ret = server_->SetOutputSurface(surface);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE);
    }
}

/**
 * @tc.name: SetOutputSurface_Invalid_Test_003
 * @tc.desc: 3. SetOutputSurface in invalid state
 */
HWTEST_F(CodecServerUnitTest, SetOutputSurface_Invalid_Test_003, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isModeConfirmed_ = false;
    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::INITIALIZED, CodecServer::CodecStatus::UNINITIALIZED,
        CodecServer::CodecStatus::RUNNING,     CodecServer::CodecStatus::END_OF_STREAM,
        CodecServer::CodecStatus::ERROR,       CodecServer::CodecStatus::FLUSHED,
    };

    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        for (auto &val : testList) {
            server_->status_ = val;
            int32_t ret = server_->SetOutputSurface(surface);
            EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE) << "state: " << val << "\n";
        }
    }
}

/**
 * @tc.name: SetOutputSurface_Invalid_Test_004
 * @tc.desc: 4. SetInputSurface with codecBase is nullptr
 */
HWTEST_F(CodecServerUnitTest, SetOutputSurface_Invalid_Test_004, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isModeConfirmed_ = false;
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    server_->codecBase_ = nullptr;
    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        int32_t ret = server_->SetOutputSurface(surface);
        EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
    }
}

/**
 * @tc.name: QueueInputBuffer_Invalid_Test_001
 * @tc.desc: 1. QueueInputBuffer in invalid state
 */
HWTEST_F(CodecServerUnitTest, QueueInputBuffer_Invalid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    uint32_t index = 1;
    int32_t ret = server_->QueueInputBuffer(index);
    EXPECT_EQ(ret, AVCS_ERR_UNSUPPORT);
}

/**
 * @tc.name: QueueInputParameter_Invalid_Test_001
 * @tc.desc: 1. QueueInputParameter in invalid state
 */
HWTEST_F(CodecServerUnitTest, QueueInputParameter_Invalid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    uint32_t index = 1;
    int32_t ret = server_->QueueInputParameter(index);
    EXPECT_EQ(ret, AVCS_ERR_UNSUPPORT);
}

/**
 * @tc.name: GetOutputFormat_Valid_Test_001
 * @tc.desc: 1. codec GetOutputFormat
 */
HWTEST_F(CodecServerUnitTest, GetOutputFormat_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::UNINITIALIZED;
    int32_t ret = server_->GetOutputFormat(validFormat_);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE);
}

/**
 * @tc.name: GetOutputFormat_Invalid_Test_001
 * @tc.desc: 1. GetOutputFormat in invalid state
 */
HWTEST_F(CodecServerUnitTest, GetOutputFormat_Invalid_Test_001, TestSize.Level1)
{
    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::INITIALIZED, CodecServer::CodecStatus::CONFIGURED,
        CodecServer::CodecStatus::RUNNING,     CodecServer::CodecStatus::END_OF_STREAM,
        CodecServer::CodecStatus::ERROR,       CodecServer::CodecStatus::FLUSHED,
    };
    CreateHCodecByMime();

    EXPECT_CALL(*codecBaseMock_, GetOutputFormat())
        .Times(testList.size())
        .WillRepeatedly(Return(AVCS_ERR_OK));

    for (auto &val : testList) {
        server_->status_ = val;
        int32_t ret = server_->GetOutputFormat(validFormat_);
        EXPECT_EQ(ret, AVCS_ERR_OK) << "state: " << val << "\n";
    }
}

/**
 * @tc.name: GetOutputFormat_Invalid_Test_002
 * @tc.desc: 2. GetOutputFormat with codecBase is nullptr
 */
HWTEST_F(CodecServerUnitTest, GetOutputFormat_Invalid_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::RUNNING;
    server_->codecBase_ = nullptr;
    int32_t ret = server_->GetOutputFormat(validFormat_);
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: GetInputFormat_Valid_Test_001
 * @tc.desc: 1. codec GetInputFormat in valid state
 */
HWTEST_F(CodecServerUnitTest, GetInputFormat_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::CONFIGURED,
        CodecServer::CodecStatus::RUNNING,
        CodecServer::CodecStatus::FLUSHED,
        CodecServer::CodecStatus::END_OF_STREAM,
    };

    EXPECT_CALL(*codecBaseMock_, GetInputFormat).Times(testList.size()).WillRepeatedly(Return(AVCS_ERR_OK));
    for (auto &val : testList) {
        server_->status_ = val;
        int32_t ret = server_->GetInputFormat(validFormat_);
        EXPECT_EQ(ret, AVCS_ERR_OK) << "state: " << val << "\n";
    }
}

/**
 * @tc.name: GetInputFormat_Invalid_Test_001
 * @tc.desc: 1. GetInputFormat in invalid state
 */
HWTEST_F(CodecServerUnitTest, GetInputFormat_Invalid_Test_001, TestSize.Level1)
{
    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::INITIALIZED,
        CodecServer::CodecStatus::UNINITIALIZED,
        CodecServer::CodecStatus::ERROR,
    };
    CreateHCodecByMime();
    for (auto &val : testList) {
        server_->status_ = val;
        int32_t ret = server_->GetInputFormat(validFormat_);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE) << "state: " << val << "\n";
    }
}

/**
 * @tc.name: GetInputFormat_Invalid_Test_002
 * @tc.desc: 2. GetInputFormat with codecBase is nullptr
 */
HWTEST_F(CodecServerUnitTest, GetInputFormat_Invalid_Test_002, TestSize.Level1)
{
    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::CONFIGURED,
        CodecServer::CodecStatus::RUNNING,
        CodecServer::CodecStatus::FLUSHED,
        CodecServer::CodecStatus::END_OF_STREAM,
    };
    CreateHCodecByMime();
    server_->codecBase_ = nullptr;
    for (auto &val : testList) {
        server_->status_ = val;
        int32_t ret = server_->GetInputFormat(validFormat_);
        EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY) << "state: " << val << "\n";
    }
}

/**
 * @tc.name: OnOutputFormatChanged_Valid_Test_001
 * @tc.desc: 1. OnOutputFormatChanged videoCb_ is not nullptr
 */
HWTEST_F(CodecServerUnitTest, OnOutputFormatChanged_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    EXPECT_CALL(*mock, OnOutputFormatChanged).Times(1);
    server_->OnOutputFormatChanged(validFormat_);
}

/**
 * @tc.name: OnOutputFormatChanged_Valid_Test_002
 * @tc.desc: 2. OnOutputFormatChanged codecCb_ is not nullptr, videoCb_ is nullptr
 */
HWTEST_F(CodecServerUnitTest, OnOutputFormatChanged_Valid_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->videoCb_ = nullptr;
    auto mock = std::make_shared<AVCodecCallbackMock>();
    server_->codecCb_ = mock;
    EXPECT_CALL(*mock, OnOutputFormatChanged).Times(1);
    server_->OnOutputFormatChanged(validFormat_);
}

/**
 * @tc.name: OnInputBufferAvailable_Valid_Test_001
 * @tc.desc: 1. OnInputBufferAvailable temporalScalability_ is not nullptr
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->temporalScalability_ = std::make_shared<TemporalScalability>();
    uint32_t index = 1;
    uint8_t data[100];
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    server_->OnInputBufferAvailable(index, buffer);
}

/**
 * @tc.name: OnInputBufferAvailable_Valid_Test_002
 * @tc.desc: 2. OnInputBufferAvailable temporalScalability_ and videoCb_ is nullptr
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_Valid_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->temporalScalability_ = nullptr;
    server_->videoCb_ = nullptr;
    uint32_t index = 1;
    uint8_t data[100];
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    server_->OnInputBufferAvailable(index, buffer);
}

/**
 * @tc.name: OnInputBufferAvailable_Valid_Test_003
 * @tc.desc: 3. OnInputBufferAvailable temporalScalability_ and drmDecryptor_ is nullptr,
 * isCreateSurface_ is false, videoCb_ is not nullptr
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_Valid_Test_003, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->temporalScalability_ = nullptr;
    server_->isCreateSurface_ = false;
    server_->videoCb_ = mock;
    server_->drmDecryptor_ = nullptr;
    EXPECT_CALL(*mock, OnInputBufferAvailable).Times(1);

    uint32_t index = 1;
    uint8_t data[100];
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    server_->OnInputBufferAvailable(index, buffer);
}

/**
 * @tc.name: OnInputBufferAvailable_Valid_Test_004
 * @tc.desc: 4. OnInputBufferAvailable temporalScalability_ is nullptr,
 * isCreateSurface_ is false, drmDecryptor_ and videoCb_ is not nullptr
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_Valid_Test_004, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->temporalScalability_ = nullptr;
    server_->isCreateSurface_ = false;
    server_->videoCb_ = mock;
    server_->drmDecryptor_ = std::make_shared<CodecDrmDecrypt>();
    EXPECT_CALL(*mock, OnInputBufferAvailable).Times(1);

    uint32_t index = 1;
    uint8_t data[100];
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    auto codecBaseCallback = std::make_shared<VCodecBaseCallback>(server_);
    codecBaseCallback->OnInputBufferAvailable(index, buffer);
}

/**
 * @tc.name: OnError_Valid_Test_001
 * @tc.desc: VCodecBaseCallback OnError test
 */
HWTEST_F(CodecServerUnitTest, OnError_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    auto codecBaseCallback = std::make_shared<VCodecBaseCallback>(server_);
    AVCodecErrorType errorType = AVCODEC_ERROR_INTERNAL;
    int32_t errorCode = AVCS_ERR_OK;
    codecBaseCallback->OnError(errorType, errorCode);
}

/**
 * @tc.name: DumpInfo_Valid_Test_001
 * @tc.desc: 1. DumpInfo codec type is video
 */
HWTEST_F(CodecServerUnitTest, DumpInfo_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->codecName_ = "video";
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    int32_t fileFd = -1;

    EXPECT_CALL(*codecBaseMock_, GetOutputFormat()).Times(1).WillOnce(Return(AVCS_ERR_OK));
    int32_t ret = server_->DumpInfo(fileFd);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: DumpInfo_Valid_Test_002
 * @tc.desc: 2. DumpInfo codec type is audio
 */
HWTEST_F(CodecServerUnitTest, DumpInfo_Valid_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->codecName_ = "audio";
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    int32_t fileFd = -1;

    EXPECT_CALL(*codecBaseMock_, GetOutputFormat()).Times(1).WillOnce(Return(AVCS_ERR_OK));
    int32_t ret = server_->DumpInfo(fileFd);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: DumpInfo_Valid_Test_003
 * @tc.desc: 3. DumpInfo codec type is default
 */
HWTEST_F(CodecServerUnitTest, DumpInfo_Valid_Test_003, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    int32_t fileFd = -1;

    EXPECT_CALL(*codecBaseMock_, GetOutputFormat()).Times(1).WillOnce(Return(AVCS_ERR_OK));
    int32_t ret = server_->DumpInfo(fileFd);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: DumpInfo_Valid_Test_004
 * @tc.desc: 4. DumpInfo fileFd is zero
 */
HWTEST_F(CodecServerUnitTest, DumpInfo_Valid_Test_004, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    int32_t fileFd = 0;

    EXPECT_CALL(*codecBaseMock_, GetOutputFormat()).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, GetHidumperInfo()).Times(1);
    int32_t ret = server_->DumpInfo(fileFd);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: MergeFormat_Valid_Test_001
 * @tc.desc: 1. MergeFormat format key type FORMAT_TYPE_INT32
 */
HWTEST_F(CodecParamCheckerTest, MergeFormat_Valid_Test_001, TestSize.Level1)
{
    Format format;
    Format oldFormat;
    uint32_t quality = 10;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, quality);

    CodecParamChecker codecParamChecker;
    codecParamChecker.MergeFormat(format, oldFormat);

    format = Format();
    oldFormat = Format();
}

/**
 * @tc.name: MergeFormat_Valid_Test_002
 * @tc.desc: 2. MergeFormat format key type FORMAT_TYPE_INT64
 */
HWTEST_F(CodecParamCheckerTest, MergeFormat_Valid_Test_002, TestSize.Level1)
{
    Format format;
    Format oldFormat;
    uint64_t bitrate = 300000;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, bitrate);

    CodecParamChecker codecParamChecker;
    codecParamChecker.MergeFormat(format, oldFormat);

    format = Format();
    oldFormat = Format();
}

/**
 * @tc.name: MergeFormat_Valid_Test_003
 * @tc.desc: 3. MergeFormat format key type FORMAT_TYPE_DOUBLE
 */
HWTEST_F(CodecParamCheckerTest, MergeFormat_Valid_Test_003, TestSize.Level1)
{
    Format format;
    Format oldFormat;
    double framRate = 30.0;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, framRate);

    CodecParamChecker codecParamChecker;
    codecParamChecker.MergeFormat(format, oldFormat);

    format = Format();;
    oldFormat = Format();
}

} // namespace