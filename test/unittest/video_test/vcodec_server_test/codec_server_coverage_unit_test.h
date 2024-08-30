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

#include "gtest/gtest.h"
#include "codecbase.h"
#include "codec_server.h"
#define EXPECT_CALL_GET_HCODEC_CAPS_MOCK                                                                               \
    EXPECT_CALL(*codecBaseMock_, GetHCapabilityList).Times(testing::AtLeast(1)).WillRepeatedly
#define EXPECT_CALL_GET_FCODEC_CAPS_MOCK                                                                               \
    EXPECT_CALL(*codecBaseMock_, GetFCapabilityList).Times(testing::AtLeast(1)).WillRepeatedly

namespace OHOS {
namespace MediaAVCodec {
constexpr uint32_t DEFAULT_WIDTH = 4096;
constexpr uint32_t DEFAULT_HEIGHT = 4096;
class CodecServerUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
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

inline void CodecServerUnitTest::SetUp(void)
{
    codecBaseMock_ = std::make_shared<CodecBaseMock>();
    CodecBase::RegisterMock(codecBaseMock_);

    server_ = std::static_pointer_cast<CodecServer>(CodecServer::Create());
    EXPECT_NE(server_, nullptr);

    validFormat_.PutIntValue(Tag::VIDEO_WIDTH, DEFAULT_WIDTH);  // 4096: valid parameter
    validFormat_.PutIntValue(Tag::VIDEO_HEIGHT, DEFAULT_HEIGHT); // 4096: valid parameter
    validFormat_.PutIntValue(Tag::VIDEO_PIXEL_FORMAT, 1);
}

inline void CodecServerUnitTest::TearDown(void)
{
    server_ = nullptr;
    codecBaseMock_ = nullptr;
    validFormat_ = Format();
}

inline void CodecServerUnitTest::CreateFCodecByName()
{
    std::string codecName = "video.F.Decoder.Name.00";

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(testing::Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(testing::Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(testing::Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName(codecName))
        .Times(1)
        .WillOnce(testing::Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(testing::Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(testing::Return(AVCS_ERR_OK));
    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
        *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

inline void CodecServerUnitTest::CreateFCodecByMime()
{
    std::string codecName = "video.F.Decoder.Name.00";
    std::string codecMime = CODEC_MIME_MOCK_00;

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(testing::Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(testing::Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(testing::Return(RetAndCaps(AVCS_ERR_OK, FCODEC_CAPS)));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateFCodecByName(codecName))
        .Times(1)
        .WillOnce(testing::Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(testing::Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(testing::Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, true, codecMime,
        *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

inline void CodecServerUnitTest::CreateHCodecByName()
{
    std::string codecName = "video.H.Encoder.Name.00";

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(testing::Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(testing::Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(testing::Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName))
        .Times(1)
        .WillOnce(testing::Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(testing::Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(testing::Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, false, codecName,
        *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

inline void CodecServerUnitTest::CreateHCodecByMime()
{
    std::string codecName = "video.H.Encoder.Name.00";
    std::string codecMime = CODEC_MIME_MOCK_00;

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(testing::Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(testing::Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(testing::Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName))
        .Times(1)
        .WillOnce(testing::Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<AVCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(testing::Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(testing::Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, true, codecMime,
        *validFormat_.GetMeta(), API_VERSION::API_VERSION_11);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}
} // name space MediaAVCodec
} // namespace OHOS