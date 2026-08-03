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

#include "codec_server_coverage_unit_test.h"
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <vector>
#include "avcodec_errors.h"
#include "codeclist_core.h"
#include "media_description.h"
#include "meta/meta_key.h"
#include "buffer/avbuffer_common.h"
#include "ui/rs_surface_node.h"
#include "window_manager.h"
#include "window_option.h"
#ifdef SUPPORT_DRM
#include "imedia_key_session_service.h"
#endif
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
constexpr uint32_t BUFFER_IS_EOS = 1;
}

namespace OHOS {
namespace MediaAVCodec {
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
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));
    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
        *validFormat_.GetMeta());
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
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, true, codecMime,
        *validFormat_.GetMeta());
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
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
        *validFormat_.GetMeta());
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
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, true, codecMime,
        *validFormat_.GetMeta());
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
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_DECODER, true, codecMime,
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    EXPECT_NE(server_->codecBase_, nullptr);
}

/**
 * @tc.name: Codec_Server_Constructor_006
 * @tc.desc: 1. sucess to create video decoder of hcodec by mime
 *           2. failed to init video hcodec
 *           3. return fcodec
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_006, TestSize.Level1)
{
    std::string codecMime = CODEC_MIME_MOCK_01;
    std::string hcodecName = "video.H.Decoder.Name.01";
    std::string fcodecName = "video.F.Decoder.Name.01";

    EXPECT_CALL(*codecBaseMock_, Init).Times(2).WillOnce(Return(AVCS_ERR_UNKNOWN)).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(2);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(hcodecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName(fcodecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_DECODER, true, codecMime, *validFormat_.GetMeta());
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
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr))).Times(0);

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
            *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_002
 * @tc.desc: 1. create hcodec by name
 *           2. SetCallback of MediaCodecCallback return error
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
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_INVALID_OPERATION));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
            *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
}

/**
 * @tc.name: Codec_Server_Constructor_Invalid_003
 * @tc.desc: invalid mime type
 */
HWTEST_F(CodecServerUnitTest, Codec_Server_Constructor_Invalid_003, TestSize.Level1)
{
    std::string codecMime = "test";
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, true, codecMime, *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: State_Test_Configure_001
 * @tc.desc: codec Configure
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
 * @tc.desc: Configure in invalid state
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
 * @tc.desc: Configure with codecBase is nullptr
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
 * @tc.desc: Configure return err
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
 * @tc.desc: codec Start
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
 * @tc.desc: Start in invalid state
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
 * @tc.desc: Start with codecBase is nullptr
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
 * @tc.desc: Start return err
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
    auto consumer = Surface::CreateSurfaceAsConsumer();
    auto p = consumer->GetProducer();
    return Surface::CreateSurfaceAsProducer(p);
}

/**
 * @tc.name: CreateInputSurface_Valid_Test_001
 * @tc.desc: codec CreateInputSurface in valid state
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
 * @tc.desc: CreateInputSurface in invalid state
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
 * @tc.desc: CreateInputSurface with codecBase is nullptr
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
 * @tc.desc: codec SetInputSurface
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
 * @tc.name: SetInputSurface_Valid_Test_002
 * @tc.desc: codec SetInputSurface
 */
HWTEST_F(CodecServerUnitTest, SetInputSurface_Valid_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isModeConfirmed_ = false;
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        EXPECT_CALL(*codecBaseMock_, SetInputSurface(surface)).Times(1).WillOnce(Return(AVCS_ERR_OK));
        int32_t ret = server_->SetInputSurface(surface);
        EXPECT_EQ(ret, AVCS_ERR_OK);
    }
}

/**
 * @tc.name: SetInputSurface_Invalid_Test_001
 * @tc.desc: SetInputSurface in invalid state
 */
HWTEST_F(CodecServerUnitTest, SetInputSurface_Invalid_Test_001, TestSize.Level1)
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
 * @tc.desc: SetInputSurface with codecBase is nullptr
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
 * @tc.desc: codec SetOutputSurface
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
 * @tc.desc: codec SetOutputSurface
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
 * @tc.name: SetOutputSurface_Valid_Test_003
 * @tc.desc: codec SetOutputSurface
 */
HWTEST_F(CodecServerUnitTest, SetOutputSurface_Valid_Test_003, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->postProcessing_ = std::make_unique<CodecServer::PostProcessingType>(server_->codecBase_);
    server_->isModeConfirmed_ = false;
    server_->status_ = CodecServer::CodecStatus::CONFIGURED;
    sptr<Surface> surface = CreateSurface();
    if (surface != nullptr) {
        EXPECT_CALL(*codecBaseMock_, SetOutputSurface(surface)).Times(0);
        server_->SetOutputSurface(surface);
    }
}

/**
 * @tc.name: SetOutputSurface_Invalid_Test_001
 * @tc.desc: SetOutputSurface in invalid mode
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
 * @tc.desc: SetOutputSurface in invalid state
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
 * @tc.desc: SetOutputSurface in invalid state
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
 * @tc.desc: SetInputSurface with codecBase is nullptr
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
 * @tc.desc: QueueInputBuffer in invalid state
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
 * @tc.desc: QueueInputParameter in invalid state
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
 * @tc.desc: codec GetOutputFormat
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
 * @tc.desc: GetOutputFormat in invalid state
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
 * @tc.desc: GetOutputFormat with codecBase is nullptr
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
 * @tc.desc: codec GetInputFormat in valid state
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
 * @tc.desc: GetInputFormat in invalid state
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
 * @tc.desc: GetInputFormat with codecBase is nullptr
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
 * @tc.desc: OnOutputFormatChanged videoCb_ is not nullptr
 */
HWTEST_F(CodecServerUnitTest, OnOutputFormatChanged_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    EXPECT_CALL(*mock, OnOutputFormatChanged).Times(1);
    server_->OnOutputFormatChanged(validFormat_);
    server_->videoCb_ = nullptr;
}

/**
 * @tc.name: OnOutputFormatChanged_Valid_Test_002
 * @tc.desc: OnOutputFormatChanged videoCb_ is not nullptr
 */
HWTEST_F(CodecServerUnitTest, OnOutputFormatChanged_Valid_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    EXPECT_CALL(*mock, OnError(AVCODEC_ERROR_FRAMEWORK_FAILED, AVCS_ERR_INVALID_STATE)).Times(1);
    server_->OnError(AVCODEC_ERROR_FRAMEWORK_FAILED, AVCS_ERR_INVALID_STATE);
    server_->videoCb_ = nullptr;
}

/**
 * @tc.name: OnInputBufferAvailable_AVBuffer_Test_001
 * @tc.desc: 1. OnInputBufferAvailable videoCb_ is not nullptr
 *           2. isCreateSurface_ is false
 *           3. isSetParameterCb_ is true
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_AVBuffer_Test_001, TestSize.Level1)
{
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    server_->isCreateSurface_ = false;
    server_->isSetParameterCb_ = true;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_CALL(*mock, OnInputBufferAvailable(index, buffer)).Times(1);
    server_->OnInputBufferAvailable(index, buffer);
    server_->videoCb_ = nullptr;
}

/**
 * @tc.name: OnInputBufferAvailable_AVBuffer_Test_002
 * @tc.desc: 1. OnInputBufferAvailable videoCb_ is not nullptr
 *           2. isCreateSurface_ is false
 *           3. isSetParameterCb_ is false
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_AVBuffer_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    server_->isCreateSurface_ = false;
    server_->isSetParameterCb_ = false;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_CALL(*mock, OnInputBufferAvailable(index, buffer)).Times(1);
    server_->OnInputBufferAvailable(index, buffer);
    server_->videoCb_ = nullptr;
}

/**
 * @tc.name: OnInputBufferAvailable_AVBuffer_Test_003
 * @tc.desc: 1. OnInputBufferAvailable videoCb_ is not nullptr
 *           2. isCreateSurface_ is true
 *           3. isSetParameterCb_ is false
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_AVBuffer_Test_003, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    server_->isCreateSurface_ = true;
    server_->isSetParameterCb_ = false;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_CALL(*mock, OnInputBufferAvailable).Times(0);
    server_->OnInputBufferAvailable(index, buffer);
    server_->videoCb_ = nullptr;
}

/**
 * @tc.name: OnInputBufferAvailable_AVBuffer_Test_004
 * @tc.desc: 1. OnInputBufferAvailable videoCb_ is not nullptr
 *           2. isCreateSurface_ is true
 *           3. isSetParameterCb_ is true
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_AVBuffer_Test_004, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    server_->isCreateSurface_ = true;
    server_->isSetParameterCb_ = true;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_CALL(*mock, OnInputBufferAvailable(index, buffer)).Times(1);
    server_->OnInputBufferAvailable(index, buffer);
    server_->videoCb_ = nullptr;
}

/**
 * @tc.name: OnInputBufferAvailable_AVBuffer_Test_005
 * @tc.desc: 1. OnInputBufferAvailable videoCb_ is nullptr
 *           2. isCreateSurface_ is true
 *           3. isSetParameterCb_ is false
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_AVBuffer_Test_005, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = nullptr;
    server_->isCreateSurface_ = true;
    server_->isSetParameterCb_ = false;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_CALL(*mock, OnInputBufferAvailable).Times(0);
    server_->OnInputBufferAvailable(index, buffer);
}

/**
 * @tc.name: OnInputBufferAvailable_AVBuffer_Test_006
 * @tc.desc: 1. OnInputBufferAvailable videoCb_ is nullptr
 *           2. isCreateSurface_ is true
 *           3. isSetParameterCb_ is true
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_AVBuffer_Test_006, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = nullptr;
    server_->isCreateSurface_ = true;
    server_->isSetParameterCb_ = true;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_CALL(*mock, OnInputBufferAvailable).Times(0);
    server_->OnInputBufferAvailable(index, buffer);
}

/* @tc.name: OnInputBufferAvailable_AVBuffer_Test_007
 * @tc.desc: 1. OnInputBufferAvailable videoCb_ is nullptr
 *           2. isCreateSurface_ is false
 *           3. isSetParameterCb_ is true
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_AVBuffer_Test_007, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = nullptr;
    server_->isCreateSurface_ = false;
    server_->isSetParameterCb_ = true;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_CALL(*mock, OnInputBufferAvailable).Times(0);
    server_->OnInputBufferAvailable(index, buffer);
}

/**
 * @tc.name: OnInputBufferAvailable_AVBuffer_Test_008
 * @tc.desc: 1. OnInputBufferAvailable videoCb_ is nullptr
 *           2. isCreateSurface_ is false
 *           3. isSetParameterCb_ is false
 */
HWTEST_F(CodecServerUnitTest, OnInputBufferAvailable_AVBuffer_Test_008, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = nullptr;
    server_->isCreateSurface_ = false;
    server_->isSetParameterCb_ = false;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_CALL(*mock, OnInputBufferAvailable(index, buffer)).Times(0);
    server_->OnInputBufferAvailable(index, buffer);
}

/**
 * @tc.name: OnError_Valid_Test_001
 * @tc.desc: CodecBaseCallback OnError test
 */
HWTEST_F(CodecServerUnitTest, OnError_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    auto codecBaseCallback = std::make_shared<CodecBaseCallback>(server_);
    AVCodecErrorType errorType = AVCODEC_ERROR_INTERNAL;
    int32_t errorCode = AVCS_ERR_OK;
    codecBaseCallback->OnError(errorType, errorCode);
}

/**
 * @tc.name: DrmVideoCencDecrypt_Test_001
 * @tc.desc: DrmVideoCencDecrypt function test
 * @tc.type: FUNC
 */
HWTEST_F(CodecServerUnitTest, DrmVideoCencDecrypt_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    server_->isCreateSurface_ = true;
    server_->isSetParameterCb_ = true;
    server_->drmDecryptor_ = std::make_shared<CodecDrmDecrypt>();
    uint32_t index = 0;
    int32_t testSize = DEFAULT_HEIGHT * DEFAULT_WIDTH * 3 / 2; // NV12 YUVI420
    MemoryFlag memFlag = MEMORY_READ_WRITE;
    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(memFlag);
    if (avAllocator != nullptr) {
        std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(avAllocator, testSize);
        buffer->memory_->SetSize(testSize - 1);
        server_->OnInputBufferAvailable(index, buffer);
        int32_t ret = server_->DrmVideoCencDecrypt(index);
        EXPECT_EQ(ret, AVCS_ERR_OK);
        server_->OnInputBufferAvailable(index, buffer);
        ret = server_->DrmVideoCencDecrypt(index);
        EXPECT_EQ(ret, AVCS_ERR_OK);
    }
}

/**
 * @tc.name: NotifyBackGround_Valid_Test_001
 * @tc.desc: NotifyBackGround valid progress
 */
HWTEST_F(CodecServerUnitTest, NotifyBackGround_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isModeConfirmed_ = true;

    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::RUNNING,
        CodecServer::CodecStatus::END_OF_STREAM,
        CodecServer::CodecStatus::FLUSHED,
    };

    for (auto &val : testList) {
        server_->status_ = val;
        EXPECT_EQ(AVCS_ERR_OK, server_->NotifyMemoryRecycle());
    }
}

/**
 * @tc.name: NotifyBackGround_Invalid_Test_001
 * @tc.desc: NotifyBackGround invalid progress - wrong status of codec
 */
HWTEST_F(CodecServerUnitTest, NotifyBackGround_Invalid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isModeConfirmed_ = true;

    std::vector<CodecServer::CodecStatus> testList = {
        CodecServer::CodecStatus::INITIALIZED,
        CodecServer::CodecStatus::UNINITIALIZED,
        CodecServer::CodecStatus::ERROR,
    };

    for (auto &val : testList) {
        server_->status_ = val;
        EXPECT_NE(AVCS_ERR_OK, server_->NotifyMemoryRecycle());
    }
}

/**
 * @tc.name: NotifyForeGround_Valid_Test_001
 * @tc.desc: NotifyForeGround valid progress
 */
HWTEST_F(CodecServerUnitTest, NotifyForeGround_Valid_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isModeConfirmed_ = true;
    EXPECT_EQ(AVCS_ERR_OK, server_->NotifyMemoryWriteBack());
}

/**
 * @tc.name: MergeFormat_Valid_Test_001
 * @tc.desc: MergeFormat format key type FORMAT_TYPE_INT32
 */
HWTEST_F(CodecParamCheckerTest, MergeFormat_Valid_Test_001, TestSize.Level1)
{
    Format format;
    Format oldFormat;
    constexpr int32_t quality = 10;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, quality);

    CodecParamChecker codecParamChecker;
    codecParamChecker.MergeFormat(format, oldFormat);

    int32_t oldFormatQuality = 0;
    bool ret = oldFormat.GetIntValue(MediaDescriptionKey::MD_KEY_QUALITY, oldFormatQuality);
    EXPECT_TRUE(ret);
    EXPECT_EQ(oldFormatQuality, quality);

    format = Format();
    oldFormat = Format();
}

/**
 * @tc.name: MergeFormat_Valid_Test_002
 * @tc.desc: MergeFormat format key type FORMAT_TYPE_INT64
 */
HWTEST_F(CodecParamCheckerTest, MergeFormat_Valid_Test_002, TestSize.Level1)
{
    Format format;
    Format oldFormat;
    constexpr int64_t bitrate = 300000;
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitrate);

    CodecParamChecker codecParamChecker;
    codecParamChecker.MergeFormat(format, oldFormat);

    int64_t oldFormatBitrate = 0;
    bool ret = oldFormat.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, oldFormatBitrate);
    EXPECT_TRUE(ret);
    EXPECT_EQ(oldFormatBitrate, bitrate);

    format = Format();
    oldFormat = Format();
}

/**
 * @tc.name: MergeFormat_Valid_Test_003
 * @tc.desc: MergeFormat format key type FORMAT_TYPE_DOUBLE
 */
HWTEST_F(CodecParamCheckerTest, MergeFormat_Valid_Test_003, TestSize.Level1)
{
    Format format;
    Format oldFormat;
    constexpr double framRate = 30.0;
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, framRate);

    CodecParamChecker codecParamChecker;
    codecParamChecker.MergeFormat(format, oldFormat);

    double oldFormatFramRate = 0;
    bool ret = oldFormat.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, oldFormatFramRate);
    EXPECT_TRUE(ret);
    EXPECT_EQ(oldFormatFramRate, framRate);

    format = Format();
    oldFormat = Format();
}

/**
 * @tc.name: SetAudioDecryptionConfig_Test_001
 * @tc.desc: SetAudioDecryptionConfig with keySession = nullptr
 *           and svpFlag = true or false, expect return AVCS_ERR_OK.
 */
HWTEST_F(CodecServerUnitTest, SetAudioDecryptionConfig_Test_001, TestSize.Level1)
{
    bool svpFlag = true;
    sptr<DrmStandard::IMediaKeySessionService> keySession = nullptr;

    int32_t ret = codecBaseMock_->SetAudioDecryptionConfig(keySession, svpFlag);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    svpFlag = false;
    ret = codecBaseMock_->SetAudioDecryptionConfig(keySession, svpFlag);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: HandleOutputBufferAvailability_Test_001
 * @tc.desc: Test HandleOutputBufferAvailability with nullptr buffer
 */
HWTEST_F(CodecServerUnitTest, HandleOutputBufferAvailability_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = nullptr;
    server_->HandleOutputBufferAvailability(index, buffer);
}

/**
 * @tc.name: HandleOutputBufferAvailability_Test_002
 * @tc.desc: Test HandleOutputBufferAvailability with sfdManager_ and MakePostDecodeDecision returns false
 */
HWTEST_F(CodecServerUnitTest, HandleOutputBufferAvailability_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->sfdManager_ = std::make_unique<SFD::SmartFluencyDecodingManager>("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_NE(buffer, nullptr);
    server_->HandleOutputBufferAvailability(index, buffer);
}

/**
 * @tc.name: HandleOutputBufferAvailability_Test_003
 * @tc.desc: Test HandleOutputBufferAvailability with isSurfaceMode_ and decoder and valid memory
 */
HWTEST_F(CodecServerUnitTest, HandleOutputBufferAvailability_Test_003, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->codecType_ = AVCODEC_TYPE_VIDEO_DECODER;
    server_->isSurfaceMode_ = true;
    uint32_t index = 1;
    AVBufferConfig bufferConfig;
    bufferConfig.size = 1024;
    bufferConfig.capacity = 1024;
    bufferConfig.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(bufferConfig);
    EXPECT_NE(buffer, nullptr);
    EXPECT_NE(buffer->memory_, nullptr);
    server_->HandleOutputBufferAvailability(index, buffer);
    EXPECT_EQ(buffer->memory_, nullptr);
}

/**
 * @tc.name: HandleOutputBufferAvailability_Test_004
 * @tc.desc: Test HandleOutputBufferAvailability with isLocalReleaseMode_ true
 */
HWTEST_F(CodecServerUnitTest, HandleOutputBufferAvailability_Test_004, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isLocalReleaseMode_ = true;
    server_->isReleaseFree_.store(false);
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_NE(buffer, nullptr);
    server_->HandleOutputBufferAvailability(index, buffer);
    EXPECT_EQ(server_->indexs_.size(), 1u);
    server_->isReleaseFree_.store(true);
    server_->releaseBufferCondition_.notify_all();
}

/**
 * @tc.name: HandleOutputBufferAvailability_Test_005
 * @tc.desc: Test HandleOutputBufferAvailability with videoCb_ nullptr
 */
HWTEST_F(CodecServerUnitTest, HandleOutputBufferAvailability_Test_005, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->videoCb_ = nullptr;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_NE(buffer, nullptr);
    server_->HandleOutputBufferAvailability(index, buffer);
}

/**
 * @tc.name: HandleOutputBufferAvailability_Test_006
 * @tc.desc: Test HandleOutputBufferAvailability with valid videoCb_
 */
HWTEST_F(CodecServerUnitTest, HandleOutputBufferAvailability_Test_006, TestSize.Level1)
{
    CreateHCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_NE(buffer, nullptr);
    EXPECT_CALL(*mock, OnOutputBufferAvailable(index, buffer)).Times(1);
    server_->HandleOutputBufferAvailability(index, buffer);
    server_->videoCb_ = nullptr;
}

/**
 * @tc.name: HandleOutputBufferAvailability_Test_007
 * @tc.desc: Test HandleOutputBufferAvailability with encoder
 */
HWTEST_F(CodecServerUnitTest, HandleOutputBufferAvailability_Test_007, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->codecType_ = AVCODEC_TYPE_VIDEO_ENCODER;
    server_->isSurfaceMode_ = true;
    uint32_t index = 1;
    AVBufferConfig bufferConfig;
    bufferConfig.size = 1024;
    bufferConfig.capacity = 1024;
    bufferConfig.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(bufferConfig);
    EXPECT_NE(buffer, nullptr);
    EXPECT_NE(buffer->memory_, nullptr);
    server_->HandleOutputBufferAvailability(index, buffer);
    EXPECT_NE(buffer->memory_, nullptr);
}

/**
 * @tc.name: ReleaseBuffer_Test_001
 * @tc.desc: Test ReleaseBuffer with isReleaseFree_ true
 */
HWTEST_F(CodecServerUnitTest, ReleaseBuffer_Test_001, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isLocalReleaseMode_ = true;
    server_->isReleaseFree_.store(true);
    server_->ReleaseBuffer();
}

/**
 * @tc.name: ReleaseBuffer_Test_002
 * @tc.desc: Test ReleaseBuffer with empty indexs_
 */
HWTEST_F(CodecServerUnitTest, ReleaseBuffer_Test_002, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isLocalReleaseMode_ = true;
    server_->isReleaseFree_.store(false);
    server_->indexs_.clear();
    std::thread notifyThread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server_->isReleaseFree_.store(true);
        server_->releaseBufferCondition_.notify_all();
    });
    server_->ReleaseBuffer();
    notifyThread.join();
}

/**
 * @tc.name: ReleaseBuffer_Test_003
 * @tc.desc: Test ReleaseBuffer with nullptr buffer
 */
HWTEST_F(CodecServerUnitTest, ReleaseBuffer_Test_003, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->isLocalReleaseMode_ = true;
    server_->isReleaseFree_.store(false);
    uint32_t index = 1;
    server_->indexs_.push_back(std::make_pair(index, nullptr));
    std::thread notifyThread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server_->isReleaseFree_.store(true);
        server_->releaseBufferCondition_.notify_all();
    });
    server_->ReleaseBuffer();
    notifyThread.join();
}

/**
 * @tc.name: ReleaseBuffer_Test_004
 * @tc.desc: Test ReleaseBuffer with EOS buffer
 */
HWTEST_F(CodecServerUnitTest, ReleaseBuffer_Test_004, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::RUNNING;
    server_->isLocalReleaseMode_ = true;
    server_->isReleaseFree_.store(false);
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_NE(buffer, nullptr);
    buffer->flag_ = BUFFER_IS_EOS;
    server_->indexs_.push_back(std::make_pair(index, buffer));
    EXPECT_CALL(*mock, OnOutputBufferAvailable(index, buffer)).Times(1);
    std::thread notifyThread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server_->isReleaseFree_.store(true);
        server_->releaseBufferCondition_.notify_all();
    });
    server_->ReleaseBuffer();
    notifyThread.join();
    server_->videoCb_ = nullptr;
}

/**
 * @tc.name: ReleaseBuffer_Test_005
 * @tc.desc: Test ReleaseBuffer with pts > lastBufferPts_
 */
HWTEST_F(CodecServerUnitTest, ReleaseBuffer_Test_005, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::RUNNING;
    server_->isLocalReleaseMode_ = true;
    server_->isReleaseFree_.store(false);
    server_->lastBufferPts_.store(100);
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_NE(buffer, nullptr);
    buffer->pts_ = 200;
    server_->indexs_.push_back(std::make_pair(index, buffer));
    EXPECT_CALL(*codecBaseMock_, RenderOutputBuffer(index)).Times(1).WillOnce(Return(AVCS_ERR_OK));
    std::thread notifyThread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server_->isReleaseFree_.store(true);
        server_->releaseBufferCondition_.notify_all();
    });
    server_->ReleaseBuffer();
    notifyThread.join();
}

/**
 * @tc.name: ReleaseBuffer_Test_006
 * @tc.desc: Test ReleaseBuffer with pts <= lastBufferPts_ (drop buffer)
 */
HWTEST_F(CodecServerUnitTest, ReleaseBuffer_Test_006, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::RUNNING;
    server_->isLocalReleaseMode_ = true;
    server_->isReleaseFree_.store(false);
    server_->lastBufferPts_.store(200);
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_NE(buffer, nullptr);
    buffer->pts_ = 100;
    server_->indexs_.push_back(std::make_pair(index, buffer));
    EXPECT_CALL(*codecBaseMock_, ReleaseOutputBuffer(index)).Times(1).WillOnce(Return(AVCS_ERR_OK));
    std::thread notifyThread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server_->isReleaseFree_.store(true);
        server_->releaseBufferCondition_.notify_all();
    });
    server_->ReleaseBuffer();
    notifyThread.join();
}

/**
 * @tc.name: ReleaseBuffer_Test_007
 * @tc.desc: Test ReleaseBuffer with multiple buffers
 */
HWTEST_F(CodecServerUnitTest, ReleaseBuffer_Test_007, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::RUNNING;
    server_->isLocalReleaseMode_ = true;
    server_->isReleaseFree_.store(false);
    server_->lastBufferPts_.store(100);
    
    for (uint32_t i = 0; i < 3; i++) {
        std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
        EXPECT_NE(buffer, nullptr);
        buffer->pts_ = 200 + i * 100;
        server_->indexs_.push_back(std::make_pair(i, buffer));
    }
    
    EXPECT_CALL(*codecBaseMock_, RenderOutputBuffer).Times(3).WillRepeatedly(Return(AVCS_ERR_OK));
    std::thread notifyThread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server_->isReleaseFree_.store(true);
        server_->releaseBufferCondition_.notify_all();
    });
    server_->ReleaseBuffer();
    notifyThread.join();
}

/**
 * @tc.name: ReleaseBuffer_Test_008
 * @tc.desc: Test ReleaseBuffer with mixed normal and EOS buffers
 */
HWTEST_F(CodecServerUnitTest, ReleaseBuffer_Test_008, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::RUNNING;
    server_->isLocalReleaseMode_ = true;
    server_->isReleaseFree_.store(false);
    server_->lastBufferPts_.store(100);
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    
    std::shared_ptr<AVBuffer> buffer1 = AVBuffer::CreateAVBuffer();
    EXPECT_NE(buffer1, nullptr);
    buffer1->pts_ = 200;
    server_->indexs_.push_back(std::make_pair(1, buffer1));
    
    std::shared_ptr<AVBuffer> buffer2 = AVBuffer::CreateAVBuffer();
    EXPECT_NE(buffer2, nullptr);
    buffer2->flag_ = BUFFER_IS_EOS;
    server_->indexs_.push_back(std::make_pair(2, buffer2));
    
    EXPECT_CALL(*codecBaseMock_, RenderOutputBuffer(1)).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*mock, OnOutputBufferAvailable(2, buffer2)).Times(1);
    
    std::thread notifyThread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server_->isReleaseFree_.store(true);
        server_->releaseBufferCondition_.notify_all();
    });
    server_->ReleaseBuffer();
    notifyThread.join();
    server_->videoCb_ = nullptr;
}

/**
 * @tc.name: ReleaseBuffer_Test_009
 * @tc.desc: Test ReleaseBuffer with EOS buffer and videoCb_ nullptr
 */
HWTEST_F(CodecServerUnitTest, ReleaseBuffer_Test_009, TestSize.Level1)
{
    CreateHCodecByMime();
    server_->status_ = CodecServer::CodecStatus::RUNNING;
    server_->isLocalReleaseMode_ = true;
    server_->isReleaseFree_.store(false);
    server_->videoCb_ = nullptr;
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    EXPECT_NE(buffer, nullptr);
    buffer->flag_ = BUFFER_IS_EOS;
    server_->indexs_.push_back(std::make_pair(index, buffer));
    std::thread notifyThread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server_->isReleaseFree_.store(true);
        server_->releaseBufferCondition_.notify_all();
    });
    server_->ReleaseBuffer();
    notifyThread.join();
}

// ========== UpdateCodecParamsFromFormat Tests ==========

/**
 * @tc.name: CodecServer_UpdateCodecParamsFromFormat_001
 * @tc.desc: UpdateCodecParamsFromFormat reads width, height, bitrate and lowPower=1 from Format
 */
HWTEST_F(CodecServerUnitTest, CodecServer_UpdateCodecParamsFromFormat_001, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 640);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 480);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 300000);
    format.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 1);

    server_->UpdateCodecParamsFromFormat(format);
    EXPECT_EQ(server_->codecWidth_, 640);
    EXPECT_EQ(server_->codecHeight_, 480);
    EXPECT_EQ(server_->codecBitrate_, 300000);
    EXPECT_EQ(server_->oldcodecLowpower_, 1);
}

/**
 * @tc.name: CodecServer_UpdateCodecParamsFromFormat_002
 * @tc.desc: UpdateCodecParamsFromFormat sets oldcodecLowpower_ to 0 when lowPower=0
 */
HWTEST_F(CodecServerUnitTest, CodecServer_UpdateCodecParamsFromFormat_002, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    Format format;
    format.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 0);

    server_->UpdateCodecParamsFromFormat(format);
    EXPECT_EQ(server_->oldcodecLowpower_, 0);
}

/**
 * @tc.name: CodecServer_UpdateCodecParamsFromFormat_003
 * @tc.desc: UpdateCodecParamsFromFormat clamps invalid lowPower value (e.g. 5) to -1
 */
HWTEST_F(CodecServerUnitTest, CodecServer_UpdateCodecParamsFromFormat_003, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    Format format;
    format.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 5); // invalid value

    server_->UpdateCodecParamsFromFormat(format);
    // (5 != 0 && 5 != 1) → clamped to -1, oldcodecLowpower_ = -1
    EXPECT_EQ(server_->oldcodecLowpower_, -1);
}

/**
 * @tc.name: CodecServer_UpdateCodecParamsFromFormat_004
 * @tc.desc: UpdateCodecParamsFromFormat preserves oldcodecLowpower_ when lowPower key is absent
 */
HWTEST_F(CodecServerUnitTest, CodecServer_UpdateCodecParamsFromFormat_004, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    // Set lowPower to 1 first
    Format formatInit;
    formatInit.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 1);
    server_->UpdateCodecParamsFromFormat(formatInit);
    EXPECT_EQ(server_->oldcodecLowpower_, 1);

    // Now call with Format that has no lowPower key — oldcodecLowpower_ should stay 1
    Format formatNoLowPower;
    formatNoLowPower.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1920);
    server_->UpdateCodecParamsFromFormat(formatNoLowPower);
    EXPECT_EQ(server_->oldcodecLowpower_, 1); // preserved
    EXPECT_EQ(server_->codecWidth_, 1920);
}

/**
 * @tc.name: CodecServer_UpdateCodecParamsFromFormat_005
 * @tc.desc: UpdateCodecParamsFromFormat casts int64_t bitrate to int32_t
 */
HWTEST_F(CodecServerUnitTest, CodecServer_UpdateCodecParamsFromFormat_005, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    Format format;
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 500000);
    server_->UpdateCodecParamsFromFormat(format);
    EXPECT_EQ(server_->codecBitrate_, 500000);
}

// ========== SetParameter LowPower Tests ==========

/**
 * @tc.name: CodecServer_SetParameter_LowPower_001
 * @tc.desc: SetParameter updates oldcodecLowpower_ when new lowPower differs from old value
 */
HWTEST_F(CodecServerUnitTest, CodecServer_SetParameter_LowPower_001, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetParameter()).Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    // Initial: oldcodecLowpower_ = -1 (default)
    EXPECT_EQ(server_->oldcodecLowpower_, -1);
    server_->codecType_ = AVCODEC_TYPE_NONE;
    server_->status_ = CodecServer::RUNNING;
    // SetParameter with lowPower=1 → different from -1, so oldcodecLowpower_ updates to 1
    Format format;
    format.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 1);
    server_->SetParameter(format);
    EXPECT_EQ(server_->oldcodecLowpower_, 1);
}

/**
 * @tc.name: CodecServer_SetParameter_LowPower_002
 * @tc.desc: SetParameter resets lowPower to -1 when new value equals oldcodecLowpower_
 */
HWTEST_F(CodecServerUnitTest, CodecServer_SetParameter_LowPower_002, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetParameter()).Times(2).WillRepeatedly(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);
    server_->codecType_ = AVCODEC_TYPE_NONE;
    server_->status_ = CodecServer::RUNNING;
    // First: set lowPower=1
    Format format1;
    format1.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 1);
    server_->SetParameter(format1);
    EXPECT_EQ(server_->oldcodecLowpower_, 1);

    // Second: set lowPower=1 again → same as oldcodecLowpower_, so oldcodecLowpower_ stays 1
    Format format2;
    format2.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 1);
    server_->SetParameter(format2);
    EXPECT_EQ(server_->oldcodecLowpower_, 1); // unchanged, but the local lowPower resets to -1
}

/**
 * @tc.name: CodecServer_SetParameter_LowPower_003
 * @tc.desc: SetParameter clamps invalid lowPower value (e.g. 2) to -1
 */
HWTEST_F(CodecServerUnitTest, CodecServer_SetParameter_LowPower_003, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetParameter()).Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);
    server_->codecType_ = AVCODEC_TYPE_NONE;
    server_->status_ = CodecServer::RUNNING;
    Format format;
    format.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 2); // invalid value
    server_->SetParameter(format);
    // (2 != 0 && 2 != 1) → clamped to -1
    // oldcodecLowpower_ == -1 (default), and clamped lowPower == -1 → same value, stays -1
    EXPECT_EQ(server_->oldcodecLowpower_, -1);
}

/**
 * @tc.name: CodecServer_SetParameter_LowPower_004
 * @tc.desc: SetParameter triggers UpdateInstanceInfo when bitrate changes alone
 */
HWTEST_F(CodecServerUnitTest, CodecServer_SetParameter_LowPower_004, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetParameter()).Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);
    server_->codecType_ = AVCODEC_TYPE_NONE;
    server_->status_ = CodecServer::RUNNING;
    // Set bitrate change only — isCodecParmChange=true, triggers UpdateInstanceInfo
    Format format;
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 600000);
    server_->SetParameter(format);
    EXPECT_EQ(server_->codecBitrate_, 600000);
}

/**
 * @tc.name: CodecServer_SetParameter_LowPower_005
 * @tc.desc: SetParameter triggers UpdateInstanceInfo when both bitrate and lowPower change
 */
HWTEST_F(CodecServerUnitTest, CodecServer_SetParameter_LowPower_005, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetParameter()).Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);
    server_->codecType_ = AVCODEC_TYPE_NONE;
    server_->status_ = CodecServer::RUNNING;
    Format format;
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 400000);
    format.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 1);
    server_->SetParameter(format);
    EXPECT_EQ(server_->codecBitrate_, 400000);
    EXPECT_EQ(server_->oldcodecLowpower_, 1);
}

// ========== UpdateInstanceInfo Tests ==========

/**
 * @tc.name: CodecServer_UpdateInstanceInfo_001
 * @tc.desc: UpdateInstanceInfo dispatches codec params via EventManager with codecParamMeta
 */
HWTEST_F(CodecServerUnitTest, CodecServer_UpdateInstanceInfo_001, TestSize.Level1)
{
    CreateFCodecByName();
    server_->codecType_ = AVCODEC_TYPE_NONE;

    // Set member values directly (private=public flag allows this)
    server_->codecWidth_ = 1280;
    server_->codecHeight_ = 720;
    server_->codecBitrate_ = 500000;
    server_->oldcodecLowpower_ = 1;

    // UpdateInstanceInfo now uses local codecParamMeta and sends via EventManager
    server_->UpdateInstanceInfo(1280, 720, 500000, 1);

    // Verify member values remain correctly set (no crash, method executed)
    EXPECT_EQ(server_->codecWidth_, 1280);
    EXPECT_EQ(server_->codecHeight_, 720);
    EXPECT_EQ(server_->codecBitrate_, 500000);
    EXPECT_EQ(server_->oldcodecLowpower_, 1);
}

/**
 * @tc.name: CodecServer_UpdateInstanceInfo_002
 * @tc.desc: UpdateInstanceInfo with lowpower=-1 still dispatches event correctly
 */
HWTEST_F(CodecServerUnitTest, CodecServer_UpdateInstanceInfo_002, TestSize.Level1)
{
    CreateFCodecByName();
    server_->codecType_ = AVCODEC_TYPE_NONE;

    // Set oldcodecLowpower_ to 0 and call UpdateInstanceInfo with lowPower=-1
    server_->codecWidth_ = 320;
    server_->codecHeight_ = 240;
    server_->codecBitrate_ = 100000;
    server_->oldcodecLowpower_ = 0;

    // UpdateInstanceInfo passes codecLowpower=-1 directly to EventManager via codecParamMeta
    server_->UpdateInstanceInfo(320, 240, 100000, -1);

    // Verify member values remain correctly set (no crash, method executed)
    EXPECT_EQ(server_->codecWidth_, 320);
    EXPECT_EQ(server_->codecHeight_, 240);
    EXPECT_EQ(server_->codecBitrate_, 100000);
    EXPECT_EQ(server_->oldcodecLowpower_, 0);
}

/**
 * @tc.name: CodecServer_UpdateCodecParamsFromFormat_006
 * @tc.desc: UpdateCodecParamsFromFormat preserves codecWidth_ when MD_KEY_WIDTH is absent
 */
HWTEST_F(CodecServerUnitTest, CodecServer_UpdateCodecParamsFromFormat_006, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    // Set codecWidth_ to a known value first
    Format formatInit;
    formatInit.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1280);
    server_->UpdateCodecParamsFromFormat(formatInit);
    EXPECT_EQ(server_->codecWidth_, 1280);

    // Now call with Format that has no MD_KEY_WIDTH — codecWidth_ should stay 1280
    Format formatNoWidth;
    formatNoWidth.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 720);
    server_->UpdateCodecParamsFromFormat(formatNoWidth);
    EXPECT_EQ(server_->codecWidth_, 1280); // preserved
    EXPECT_EQ(server_->codecHeight_, 720);
}

/**
 * @tc.name: CodecServer_UpdateCodecParamsFromFormat_007
 * @tc.desc: UpdateCodecParamsFromFormat preserves codecHeight_ when MD_KEY_HEIGHT is absent
 */
HWTEST_F(CodecServerUnitTest, CodecServer_UpdateCodecParamsFromFormat_007, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    // Set codecHeight_ to a known value first
    Format formatInit;
    formatInit.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 720);
    server_->UpdateCodecParamsFromFormat(formatInit);
    EXPECT_EQ(server_->codecHeight_, 720);

    // Now call with Format that has no MD_KEY_HEIGHT — codecHeight_ should stay 720
    Format formatNoHeight;
    formatNoHeight.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 640);
    server_->UpdateCodecParamsFromFormat(formatNoHeight);
    EXPECT_EQ(server_->codecWidth_, 640);
    EXPECT_EQ(server_->codecHeight_, 720); // preserved
}

/**
 * @tc.name: CodecServer_UpdateCodecParamsFromFormat_008
 * @tc.desc: UpdateCodecParamsFromFormat preserves codecBitrate_ when MD_KEY_BITRATE is absent
 */
HWTEST_F(CodecServerUnitTest, CodecServer_UpdateCodecParamsFromFormat_008, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    // Set codecBitrate_ to a known value first
    Format formatInit;
    formatInit.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 1000000);
    server_->UpdateCodecParamsFromFormat(formatInit);
    EXPECT_EQ(server_->codecBitrate_, 1000000);

    // Now call with Format that has no MD_KEY_BITRATE — codecBitrate_ should stay 1000000
    Format formatNoBitrate;
    formatNoBitrate.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 320);
    server_->UpdateCodecParamsFromFormat(formatNoBitrate);
    EXPECT_EQ(server_->codecWidth_, 320);
    EXPECT_EQ(server_->codecBitrate_, 1000000); // preserved
}

/**
 * @tc.name: CodecServer_UpdateCodecParamsFromFormat_009
 * @tc.desc: UpdateCodecParamsFromFormat with empty Format preserves all member values
 */
HWTEST_F(CodecServerUnitTest, CodecServer_UpdateCodecParamsFromFormat_009, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    // Set all values to known values first
    Format formatInit;
    formatInit.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1920);
    formatInit.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 1080);
    formatInit.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 2000000);
    formatInit.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 1);
    server_->UpdateCodecParamsFromFormat(formatInit);
    EXPECT_EQ(server_->codecWidth_, 1920);
    EXPECT_EQ(server_->codecHeight_, 1080);
    EXPECT_EQ(server_->codecBitrate_, 2000000);
    EXPECT_EQ(server_->oldcodecLowpower_, 1);

    // Call with completely empty Format — all values should be preserved
    Format emptyFormat;
    server_->UpdateCodecParamsFromFormat(emptyFormat);
    EXPECT_EQ(server_->codecWidth_, 1920);
    EXPECT_EQ(server_->codecHeight_, 1080);
    EXPECT_EQ(server_->codecBitrate_, 2000000);
    EXPECT_EQ(server_->oldcodecLowpower_, 1); // preserved since key absent
}

/**
 * @tc.name: CodecServer_SetParameter_LowPower_006
 * @tc.desc: SetParameter with no bitrate/lowPower keys
 *           does not trigger UpdateInstanceInfo (isCodecParmChange=false)
 */
HWTEST_F(CodecServerUnitTest, CodecServer_SetParameter_LowPower_006, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetParameter()).Times(2).WillRepeatedly(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false,
        "video.F.Decoder.Name.00", *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);
    server_->codecType_ = AVCODEC_TYPE_NONE;
    server_->status_ = CodecServer::RUNNING;
    // Set known values first
    Format formatInit;
    formatInit.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 800000);
    formatInit.PutIntValue(
        Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 1);
    server_->SetParameter(formatInit);
    EXPECT_EQ(server_->codecBitrate_, 800000);
    EXPECT_EQ(server_->oldcodecLowpower_, 1);

    // Call SetParameter with Format having no bitrate/lowPower
    // isCodecParmChange stays false, values unchanged
    Format formatNoChange;
    formatNoChange.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 320);
    server_->SetParameter(formatNoChange);
    // codecBitrate_ and oldcodecLowpower_ should remain unchanged
    EXPECT_EQ(server_->codecBitrate_, 800000);
    EXPECT_EQ(server_->oldcodecLowpower_, 1);
}

/**
 * @tc.name: CodecServer_SetParameter_LowPower_007
 * @tc.desc: SetParameter with lowPower=0 covers ternary (lowPower!=0) false branch
 */
HWTEST_F(CodecServerUnitTest, CodecServer_SetParameter_LowPower_007, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetParameter()).Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);

    // Initial: oldcodecLowpower_ = -1 (default)
    EXPECT_EQ(server_->oldcodecLowpower_, -1);
    server_->codecType_ = AVCODEC_TYPE_NONE;
    server_->status_ = CodecServer::RUNNING;
    // SetParameter with lowPower=0 → (0!=0 && 0!=1) → 0!=0 false → short-circuit → lowPower=0
    // oldcodecLowpower_(-1) != lowPower(0) → true → oldcodecLowpower_ = 0
    Format format;
    format.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 0);
    server_->SetParameter(format);
    EXPECT_EQ(server_->oldcodecLowpower_, 0);
}

/**
 * @tc.name: CodecServer_SetParameter_LowPower_008
 * @tc.desc: SetParameter with lowPower=0 twice covers else branch when oldcodecLowpower_==0
 */
HWTEST_F(CodecServerUnitTest, CodecServer_SetParameter_LowPower_008, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetParameter()).Times(2).WillRepeatedly(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);
    server_->codecType_ = AVCODEC_TYPE_NONE;
    server_->status_ = CodecServer::RUNNING;
    // First: set lowPower=0 → oldcodecLowpower_ updates from -1 to 0
    Format format1;
    format1.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 0);
    server_->SetParameter(format1);
    EXPECT_EQ(server_->oldcodecLowpower_, 0);
    
    // Second: set lowPower=0 again → ternary result=0, oldcodecLowpower_(0) == lowPower(0) → else: lowPower=-1
    Format format2;
    format2.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 0);
    server_->SetParameter(format2);
    EXPECT_EQ(server_->oldcodecLowpower_, 0); // stays 0, local lowPower resets to -1
}

/**
 * @tc.name: CodecServer_SetParameter_LowPower_009
 * @tc.desc: SetParameter with lowPower=0 and bitrate covers both keys present with lowPower=0
 */
HWTEST_F(CodecServerUnitTest, CodecServer_SetParameter_LowPower_009, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetParameter()).Times(1).WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);
    server_->codecType_ = AVCODEC_TYPE_NONE;
    server_->status_ = CodecServer::RUNNING;
    // Both bitrate and lowPower=0 present
    Format format;
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 200000);
    format.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 0);
    server_->SetParameter(format);
    EXPECT_EQ(server_->codecBitrate_, 200000);
    EXPECT_EQ(server_->oldcodecLowpower_, 0);
}

/**
 * @tc.name: CodecServer_SetParameter_LowPower_010
 * @tc.desc: SetParameter with lowPower=0 after lowPower=1 covers oldcodecLowpower_!=0 true
 *           when oldcodecLowpower_ was previously 1
 */
HWTEST_F(CodecServerUnitTest, CodecServer_SetParameter_LowPower_010, TestSize.Level1)
{
    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName("video.F.Decoder.Name.00"))
        .Times(1).WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetParameter()).Times(2).WillRepeatedly(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, "video.F.Decoder.Name.00",
        *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);
    server_->codecType_ = AVCODEC_TYPE_NONE;
    server_->status_ = CodecServer::RUNNING;
    // First: set lowPower=1 → oldcodecLowpower_ = 1
    Format format1;
    format1.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 1);
    server_->SetParameter(format1);
    EXPECT_EQ(server_->oldcodecLowpower_, 1);

    // Second: set lowPower=0 → oldcodecLowpower_(1) != lowPower(0) → true → oldcodecLowpower_ = 0
    Format format2;
    format2.PutIntValue(Media::Tag::VIDEO_ENCODER_WITH_LOWPOWER_CAMERA, 0);
    server_->SetParameter(format2);
    EXPECT_EQ(server_->oldcodecLowpower_, 0);
}
} // MediaAVCodec
} // namespace