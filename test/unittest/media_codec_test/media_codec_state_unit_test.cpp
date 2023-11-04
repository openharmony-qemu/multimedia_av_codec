/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
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

#ifdef CLIENT_SUPPORT_CODEC
#undef CLIENT_SUPPORT_CODEC
#endif
#include <dirent.h>
#include <memory>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "avbuffer.h"
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_info.h"
#include "avcodec_list.h"
#include "media_codec_sample.h"
#include "media_description.h"
#include "meta.h"
#include "meta_key.h"
#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include "native_averrors.h"
#include "nocopyable.h"
#include "unittest_log.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Media;

namespace {
enum VCodecTestCode : int32_t { ADEC_ACC, AEN_ACC, SWDEC_AVC, HWDEC_AVC, HWDEC_HEVC };
constexpr uint32_t DEFAULT_WIDTH = 320;
constexpr uint32_t DEFAULT_HEIGHT = 240;
constexpr int32_t DEFAULT_BUFFER_NUM = 8;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace MediaCodecStateUT {
class MediaCodecUnitTest : public testing::TestWithParam<int32_t> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

private:
    bool CreateByNameWithParam();
    void SetFormat();
    std::shared_ptr<CodecSample> codec_ = nullptr;
    std::shared_ptr<CodecCallback> callback_ = nullptr;
    std::shared_ptr<Meta> meta_ = nullptr;
    std::shared_ptr<AVBufferQueue> outputQueue_ = nullptr;
};

void MediaCodecUnitTest::SetUpTestCase(void) {}

void MediaCodecUnitTest::TearDownTestCase(void) {}

void MediaCodecUnitTest::SetUp(void)
{
    std::cout << "[SetUp]: SetUp!!!, test: ";
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testName = testInfo_->name();
    std::cout << testName << std::endl;

    string prefix = "/data/test/media/";
    auto check = [](char it) { return it == '/'; };
    (void)testName.erase(std::remove_if(testName.begin(), testName.end(), check), testName.end());

    codec_ = std::make_shared<CodecSample>();
    codec_->SetOutPath(prefix + testName);

    callback_ = make_shared<CodecSampleCallback>(codec_);
    outputQueue_ = AVBufferQueue::Create(DEFAULT_BUFFER_NUM);
}

void MediaCodecUnitTest::TearDown(void)
{
    meta_ = nullptr;
    codec_ = nullptr;
    std::cout << "[TearDown]: over!!!" << std::endl;
}

bool MediaCodecUnitTest::CreateByNameWithParam()
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    std::shared_ptr<AVCodecInfo> codecInfo = nullptr;
    CapabilityData *capabilityData = nullptr;
    switch (GetParam()) {
        case VCodecTestCode::ADEC_ACC: {
            capabilityData =
                codeclist->GetCapability(CodecMimeType::AUDIO_AAC.data(), false, AVCodecCategory::AVCODEC_NONE);
            codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
            break;
        }
        case VCodecTestCode::AEN_ACC: {
            capabilityData =
                codeclist->GetCapability(CodecMimeType::AUDIO_AAC.data(), true, AVCodecCategory::AVCODEC_NONE);
            codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
            break;
        }
        case VCodecTestCode::SWDEC_AVC: {
            capabilityData =
                codeclist->GetCapability(CodecMimeType::VIDEO_AVC.data(), false, AVCodecCategory::AVCODEC_SOFTWARE);
            codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
            codec_->SetInPath("/data/test/media/avc_320_240_10s.dat");
            break;
        }
        case VCodecTestCode::HWDEC_AVC: {
            capabilityData =
                codeclist->GetCapability(CodecMimeType::VIDEO_AVC.data(), false, AVCodecCategory::AVCODEC_HARDWARE);
            codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
            codec_->SetInPath("/data/test/media/avc_320_240_10s.dat");
            break;
        }
        case VCodecTestCode::HWDEC_HEVC: {
            capabilityData =
                codeclist->GetCapability(CodecMimeType::VIDEO_HEVC.data(), true, AVCodecCategory::AVCODEC_HARDWARE);
            codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
            codec_->SetInPath("/data/test/media/hevc_320x240_60.dat");
            break;
        }
        default:
            return false;
    }
    std::string codecName = codecInfo->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    bool ret = codec_->CreateByName(codecName);
    EXPECT_TRUE(ret);
    return ret;
}

void MediaCodecUnitTest::SetFormat()
{
    meta_ = std::make_shared<Meta>();
    switch (GetParam()) {
        case VCodecTestCode::SWDEC_AVC:
        case VCodecTestCode::HWDEC_AVC: {
            meta_->SetData(std::string(Tag::VIDEO_WIDTH), "123456");
            meta_->SetData(Tag::VIDEO_HEIGHT, DEFAULT_HEIGHT);
            break;
        }
        case VCodecTestCode::HWDEC_HEVC: {
            meta_ = std::make_shared<Meta>();
            meta_->SetData(Tag::VIDEO_WIDTH, DEFAULT_WIDTH);
            meta_->SetData(Tag::VIDEO_HEIGHT, DEFAULT_HEIGHT);
            break;
        }
        default: {
            meta_ = nullptr;
            return;
        }
    }
}

INSTANTIATE_TEST_SUITE_P(, MediaCodecUnitTest, testing::Values(HWDEC_AVC));

/**
 * @tc.name: MediaCodec_Create_001
 * @tc.desc: media codec create
 * @tc.type: FUNC
 */
HWTEST_P(MediaCodecUnitTest, MediaCodec_Create_001, TestSize.Level1)
{
    EXPECT_TRUE(CreateByNameWithParam());
    SetFormat();
    EXPECT_EQ(Status::OK, codec_->Configure(meta_));
    EXPECT_EQ(Status::OK, codec_->SetCodecCallback(callback_));
    EXPECT_EQ(Status::OK, codec_->SetOutputBufferQueue(outputQueue_));
    EXPECT_EQ(Status::OK, codec_->Prepare());
    EXPECT_NE(nullptr, codec_->GetInputBufferQueue());
    EXPECT_EQ(Status::OK, codec_->Start());
    EXPECT_EQ(Status::OK, codec_->Flush());
    EXPECT_EQ(Status::OK, codec_->Start());
    EXPECT_EQ(Status::OK, codec_->Reset());
    EXPECT_EQ(Status::OK, codec_->Release());

    // Plugin::SrcInputType val = OHOS::Media::Plugin::SrcInputType::AUD_MIC;
    // meta_->Set<std::string(Tag::SRC_INPUT_TYPE)>(val);

    // using SrcInputValueType = Meta::ValueInfo<Tag::SRC_INPUT_TYPE>::type;
    // uint32_t v1;
    // string key = "video.width";
    // meta_->Get<key.c_str()>(v1);
    // std::string_view typeName = OHOS::Media::Any::GetTypeName<key.c_str()>();
    // cout << typeName << endl;

    // ValueInfo<key.c_str()>::type;
    
    
    // std::string str = "321";
    // meta_->SetData(std::string(Tag::VIDEO_WIDTH), uint32_t(123456));
    // meta_->GetData(std::string(Tag::VIDEO_WIDTH), str);
    // std::cout << str << std::endl;

    // uint32_t width = 123;
    // meta_->SetData(std::string(Tag::VIDEO_WIDTH), uint32_t(123456));
    // meta_->GetData(std::string(Tag::VIDEO_WIDTH), width);
    // std::cout << width << std::endl;
}
} // namespace MediaCodecStateUT
} // namespace MediaAVCodec
} // namespace OHOS