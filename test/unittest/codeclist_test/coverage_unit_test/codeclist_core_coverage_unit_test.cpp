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
#include <string>
#include <vector>
#include "avcodec_codec_name.h"
#include "avcodec_errors.h"
#include "avcodec_list.h"
#include "codeclist_core.h"
#include "meta/meta_key.h"
#include "codecbase.h"
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace testing;
using namespace testing::ext;

namespace {
const std::string DEFAULT_VIDEO_MIME = std::string(CodecMimeType::VIDEO_AVC);
const std::string DEFAULT_CODEC_NAME = "video.H.Decoder.Name.02";
constexpr const char CODEC_VENDOR_FLAG[] = "codec_vendor_flag";
constexpr const char SAMPLE_RATE[] = "samplerate";
class CodecListUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    Format format_;
    std::shared_ptr<CapabilityData> capabilityData_ = nullptr;
};

class CodecParamCheckerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(void){};
    void TearDown(void){};
};

void CodecListUnitTest::SetUpTestCase(void) {}

void CodecListUnitTest::TearDownTestCase(void) {}

void CodecListUnitTest::SetUp(void)
{
    for (auto iter : HCODEC_CAPS) {
        if (iter.codecName == DEFAULT_CODEC_NAME) {
            capabilityData_ = std::make_shared<CapabilityData>(iter);
        }
    }
    EXPECT_NE(capabilityData_, nullptr);

    format_.PutIntValue(Tag::VIDEO_WIDTH, 4096);  // 4096: valid parameter
    format_.PutIntValue(Tag::VIDEO_HEIGHT, 4096); // 4096: valid parameter
    format_.PutIntValue(Tag::VIDEO_PIXEL_FORMAT, 1);
}

void CodecListUnitTest::TearDown(void)
{
    capabilityData_ = nullptr;
    format_ = Format();
}

/**
 * @tc.name: CheckBitrate_Valid_Test_001
 * @tc.desc: format uncontain key bitrate
 */
HWTEST_F(CodecListUnitTest, CheckBitrate_Valid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    bool ret = codecListCore.CheckBitrate(format_, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckBitrate_Valid_Test_002
 * @tc.desc: format key bitrate in range
 */
HWTEST_F(CodecListUnitTest, CheckBitrate_Valid_Test_002, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr uint64_t bitrate = 300000;
    format_.PutLongValue(Tag::MEDIA_BITRATE, bitrate);
    bool ret = codecListCore.CheckBitrate(format_, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckBitrate_Invalid_Test_001
 * @tc.desc: format key bitrate out of range
 */
HWTEST_F(CodecListUnitTest, CheckBitrate_InValid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr uint64_t bitrate = 300000000;
    format_.PutLongValue(Tag::MEDIA_BITRATE, bitrate);
    bool ret = codecListCore.CheckBitrate(format_, *capabilityData_);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: CheckVideoResolution_Valid_Test_001
 * @tc.desc: format uncontain key width and height
 */
HWTEST_F(CodecListUnitTest, CheckVideoResolution_Valid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    Format format;
    bool ret = codecListCore.CheckVideoResolution(format, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckVideoResolution_Valid_Test_002
 * @tc.desc: format key width and height in range
 */
HWTEST_F(CodecListUnitTest, CheckVideoResolution_Valid_Test_002, TestSize.Level1)
{
    CodecListCore codecListCore;
    bool ret = codecListCore.CheckVideoResolution(format_, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckVideoResolution_Invalid_Test_001
 * @tc.desc: format key width and height out of range
 */
HWTEST_F(CodecListUnitTest, CheckVideoResolution_InValid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, 17);  // 17: invalid parameter
    format.PutIntValue(Tag::VIDEO_HEIGHT, 17);  // 17: invalid parameter
    bool ret = codecListCore.CheckVideoResolution(format, *capabilityData_);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: CheckVideoPixelFormat_Valid_Test_001
 * @tc.desc: format uncontain key pixel format
 */
HWTEST_F(CodecListUnitTest, CheckVideoPixelFormat_Valid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    Format format;
    bool ret = codecListCore.CheckVideoPixelFormat(format, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckVideoPixelFormat_Valid_Test_002
 * @tc.desc: format key pixel format in range
 */
HWTEST_F(CodecListUnitTest, CheckVideoPixelFormat_Valid_Test_002, TestSize.Level1)
{
    CodecListCore codecListCore;
    bool ret = codecListCore.CheckVideoPixelFormat(format_, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckVideoPixelFormat_Invalid_Test_001
 * @tc.desc: format key pixel format out of range
 */
HWTEST_F(CodecListUnitTest, CheckVideoPixelFormat_InValid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    Format format;
    format.PutIntValue(Tag::VIDEO_PIXEL_FORMAT, -1);  // -1: invalid parameter
    bool ret = codecListCore.CheckVideoPixelFormat(format, *capabilityData_);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: CheckVideoFrameRate_Valid_Test_001
 * @tc.desc: format uncontain key frame rate
 */
HWTEST_F(CodecListUnitTest, CheckVideoFrameRate_Valid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    bool ret = codecListCore.CheckVideoFrameRate(format_, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckVideoFrameRate_Valid_Test_002
 * @tc.desc: format key frame rate value is double
 */
HWTEST_F(CodecListUnitTest, CheckVideoFrameRate_Valid_Test_002, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr double frameRate = 60.0;
    format_.PutDoubleValue(Tag::VIDEO_FRAME_RATE, frameRate);
    bool ret = codecListCore.CheckVideoFrameRate(format_, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckVideoFrameRate_Invalid_Test_001
 * @tc.desc: format key frame rate value is double and value invalid
 */
HWTEST_F(CodecListUnitTest, CheckVideoFrameRate_Invalid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr double frameRate = 1000.0;
    format_.PutDoubleValue(Tag::VIDEO_FRAME_RATE, frameRate);
    bool ret = codecListCore.CheckVideoFrameRate(format_, *capabilityData_);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: CheckAudioChannel_Valid_Test_001
 * @tc.desc: format uncontain key channel count
 */
HWTEST_F(CodecListUnitTest, CheckAudioChannel_Valid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    bool ret = codecListCore.CheckAudioChannel(format_, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckAudioChannel_Valid_Test_002
 * @tc.desc: format key channel count in range
 */
HWTEST_F(CodecListUnitTest, CheckAudioChannel_Valid_Test_002, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr uint32_t channelCount = 17;
    format_.PutIntValue(Tag::AUDIO_CHANNEL_COUNT, channelCount);
    bool ret = codecListCore.CheckAudioChannel(format_, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckAudioChannel_Invalid_Test_001
 * @tc.desc: format key channel count out of range
 */
HWTEST_F(CodecListUnitTest, CheckAudioChannel_Invalid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr uint32_t channelCount = 10000;
    format_.PutIntValue(Tag::AUDIO_CHANNEL_COUNT, channelCount);
    bool ret = codecListCore.CheckAudioChannel(format_, *capabilityData_);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: CheckAudioSampleRate_Valid_Test_001
 * @tc.desc: format uncontain key samplerate
 */
HWTEST_F(CodecListUnitTest, CheckAudioSampleRate_Valid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    bool ret = codecListCore.CheckAudioSampleRate(format_, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckAudioSampleRate_Valid_Test_002
 * @tc.desc: format key samplerate in range
 */
HWTEST_F(CodecListUnitTest, CheckAudioSampleRate_Valid_Test_002, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr uint32_t samplerate = 1;
    format_.PutIntValue(SAMPLE_RATE, samplerate);
    bool ret = codecListCore.CheckAudioSampleRate(format_, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: CheckAudioSampleRate_Invalid_Test_001
 * @tc.desc: format key samplerate out of range
 */
HWTEST_F(CodecListUnitTest, CheckAudioSampleRate_Invalid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr uint32_t samplerate = 10;
    format_.PutIntValue(SAMPLE_RATE, samplerate);
    bool ret = codecListCore.CheckAudioSampleRate(format_, *capabilityData_);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: IsVideoCapSupport_Valid_Test_001
 * @tc.desc: format uncontain key
 */
HWTEST_F(CodecListUnitTest, IsVideoCapSupport_Valid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    Format format;
    bool ret = codecListCore.IsVideoCapSupport(format, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: IsAudioCapSupport_Valid_Test_001
 * @tc.desc: format uncontain key
 */
HWTEST_F(CodecListUnitTest, IsAudioCapSupport_Valid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    Format format;
    bool ret = codecListCore.IsAudioCapSupport(format, *capabilityData_);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: GetCapability_Valid_Test_001
 * @tc.desc: mime is empty
 */
HWTEST_F(CodecListUnitTest, GetCapability_Valid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    std::string mime = "";
    constexpr bool isEncoder = true;
    constexpr AVCodecCategory category = AVCodecCategory::AVCODEC_NONE;
    bool ret = codecListCore.GetCapability(*capabilityData_, mime, isEncoder, category);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: FindCodec_Valid_Test_001
 * @tc.desc: format uncontain key codec mime
 */
HWTEST_F(CodecListUnitTest, FindCodec_Valid_Test_001, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr bool isEncoder = true;
    std::string ret = codecListCore.FindCodec(format_, isEncoder);
    EXPECT_EQ(ret, "");
}

/**
 * @tc.name: FindCodec_Valid_Test_002
 * @tc.desc: format key codec mime in range, contain codec vendor flag and capabilityDataArray is null
 */
HWTEST_F(CodecListUnitTest, FindCodec_Valid_Test_002, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr bool isEncoder = true;
    constexpr int32_t vendorFlag = 1;
    format_.PutStringValue(Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    format_.PutIntValue(CODEC_VENDOR_FLAG, vendorFlag);
    std::string ret = codecListCore.FindCodec(format_, isEncoder);
    EXPECT_EQ(ret, "");
}

/**
 * @tc.name: FindCodec_Valid_Test_003
 * @tc.desc: format key codec mime is audio and in mimeCapIdxMap
 */
HWTEST_F(CodecListUnitTest, FindCodec_Valid_Test_003, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr bool isEncoder = false;
    format_.PutStringValue(Tag::MIME_TYPE, CodecMimeType::AUDIO_MPEG);
    std::string ret = codecListCore.FindCodec(format_, isEncoder);
    EXPECT_EQ(ret, AVCodecCodecName::AUDIO_DECODER_MP3_NAME);
}

/**
 * @tc.name: FindCodec_Valid_Test_004
 * @tc.desc: format key codec mime is audio and not in mimeCapIdxMap
 */
HWTEST_F(CodecListUnitTest, FindCodec_Valid_Test_004, TestSize.Level1)
{
    CodecListCore codecListCore;
    constexpr bool isEncoder = false;
    constexpr int32_t vendorFlag = 1;
    format_.PutStringValue(Tag::MIME_TYPE, CodecMimeType::AUDIO_MPEG);
    format_.PutIntValue(CODEC_VENDOR_FLAG, vendorFlag);
    std::string ret = codecListCore.FindCodec(format_, isEncoder);
    EXPECT_EQ(ret, "");
}
} // namespace