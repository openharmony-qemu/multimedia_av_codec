/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include <thread>
#include <vector>
#include "caps_unit_test.h"
#include "gtest/gtest.h"
#ifdef CODECLIST_CAPI_UNIT_TEST
#include "native_avmagic.h"
#endif

#include <iostream>

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec::CodecListTestParam;

namespace OHOS {
namespace MediaAVCodec {

constexpr uint32_t MAX_VIDEO_BITRATE = 300000000;
constexpr uint32_t MAX_VIDEO_FRAME_RATE = 60;
const uint32_t MIN_WIDTH = 2;
const uint32_t MIN_HEIGHT = 2;
const uint32_t MAX_WIDTH = 4095;
const uint32_t MAX_HEIGHT = 4095;
const uint32_t MAX_FRAME_RATE = 30;

void CapsUnitTest::SetUpTestCase(void) {}

void CapsUnitTest::TearDownTestCase(void) {}

void CapsUnitTest::SetUp(void)
{
    avCodecList_ = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, avCodecList_);
    codecMimeKey_ = MediaDescriptionKey::MD_KEY_CODEC_MIME;
    bitrateKey_ = MediaDescriptionKey::MD_KEY_BITRATE;
    widthKey_ = MediaDescriptionKey::MD_KEY_WIDTH;
    heightKey_ = MediaDescriptionKey::MD_KEY_HEIGHT;
    pixelFormatKey_ = MediaDescriptionKey::MD_KEY_PIXEL_FORMAT;
    frameRateKey_ = MediaDescriptionKey::MD_KEY_FRAME_RATE;
    channelCountKey_ = MediaDescriptionKey::MD_KEY_CHANNEL_COUNT;
    sampleRateKey_ = MediaDescriptionKey::MD_KEY_SAMPLE_RATE;
    CapabilityData *capabilityData =
        avCodecList_->GetCapability(DEFAULT_VIDEO_MIME, false, AVCodecCategory::AVCODEC_NONE);
    if (capabilityData != nullptr && capabilityData->isVendor) {
        isHardIncluded_ = true;
    }
}

void CapsUnitTest::TearDown(void) {}

#ifdef CODECLIST_CAPI_UNIT_TEST
std::vector<std::shared_ptr<VideoCaps>> CapsUnitTest::GetVideoDecoderCaps()
{
    std::vector<std::shared_ptr<VideoCaps>> ret;
    for (auto it : videoDecoderList) {
        auto capabilityCapi = OH_AVCodec_GetCapability(it.c_str(), false);
        ret.push_back(std::make_shared<VideoCaps>(capabilityCapi->capabilityData_));
    }
    return ret;
}

std::vector<std::shared_ptr<VideoCaps>> CapsUnitTest::GetVideoEncoderCaps()
{
    std::vector<std::shared_ptr<VideoCaps>> ret;
    if (isHardIncluded_) {
        for (auto it : videoEncoderList) {
            auto capabilityCapi = OH_AVCodec_GetCapability(it.c_str(), true);
            ret.push_back(std::make_shared<VideoCaps>(capabilityCapi->capabilityData_));
        }
    }
    return ret;
}

std::vector<std::shared_ptr<AudioCaps>> CapsUnitTest::GetAudioDecoderCaps()
{
    std::vector<std::shared_ptr<AudioCaps>> ret;
    for (auto it : audioDecoderList) {
        auto capabilityCapi = OH_AVCodec_GetCapability(it.c_str(), false);
        ret.push_back(std::make_shared<AudioCaps>(capabilityCapi->capabilityData_));
    }
    return ret;
}

std::vector<std::shared_ptr<AudioCaps>> CapsUnitTest::GetAudioEncoderCaps()
{
    std::vector<std::shared_ptr<AudioCaps>> ret;
    for (auto it : audioEncoderList) {
        auto capabilityCapi = OH_AVCodec_GetCapability(it.c_str(), true);
        ret.push_back(std::make_shared<AudioCaps>(capabilityCapi->capabilityData_));
    }
    return ret;
}

std::vector<CapabilityData *> CapsUnitTest::GetDupCaps(std::string mime, int32_t dupTimes)
{
    std::vector<CapabilityData *> ret;
    for (int32_t i = 0; i < dupTimes; i++) {
        auto capabilityCapi = OH_AVCodec_GetCapability(mime.c_str(), true);
        if (capabilityCapi == nullptr) {
            break;
        }
        ret.push_back(capabilityCapi->capabilityData_);
    }
    return ret;
}
#endif

#ifdef CODECLIST_INNER_UNIT_TEST
std::vector<std::shared_ptr<VideoCaps>> CapsUnitTest::GetVideoDecoderCaps()
{
    std::vector<std::shared_ptr<VideoCaps>> ret;
    for (auto it : videoDecoderList) {
        CapabilityData *capabilityData = avCodecList_->GetCapability(it, false, AVCodecCategory::AVCODEC_HARDWARE);
        if (capabilityData != 0) {
            ret.push_back(std::make_shared<VideoCaps>(capabilityData));
        }
        capabilityData = avCodecList_->GetCapability(it, false, AVCodecCategory::AVCODEC_SOFTWARE);
        if (capabilityData != 0) {
            ret.push_back(std::make_shared<VideoCaps>(capabilityData));
        }
    }
    return ret;
}

std::vector<std::shared_ptr<VideoCaps>> CapsUnitTest::GetVideoEncoderCaps()
{
    std::vector<std::shared_ptr<VideoCaps>> ret;
    if (isHardIncluded_) {
        for (auto it : videoEncoderList) {
            CapabilityData *capabilityData = avCodecList_->GetCapability(it, true, AVCodecCategory::AVCODEC_HARDWARE);
            if (capabilityData != 0) {
                ret.push_back(std::make_shared<VideoCaps>(capabilityData));
            }
            capabilityData = avCodecList_->GetCapability(it, true, AVCodecCategory::AVCODEC_SOFTWARE);
            if (capabilityData != 0) {
                ret.push_back(std::make_shared<VideoCaps>(capabilityData));
            }
        }
    }
    return ret;
}

std::vector<std::shared_ptr<AudioCaps>> CapsUnitTest::GetAudioDecoderCaps()
{
    std::vector<std::shared_ptr<AudioCaps>> ret;
    for (auto it : audioDecoderList) {
        CapabilityData *capabilityData = avCodecList_->GetCapability(it, false, AVCodecCategory::AVCODEC_NONE);
        ret.push_back(std::make_shared<AudioCaps>(capabilityData));
    }
    return ret;
}

std::vector<std::shared_ptr<AudioCaps>> CapsUnitTest::GetAudioEncoderCaps()
{
    std::vector<std::shared_ptr<AudioCaps>> ret;
    for (auto it : audioEncoderList) {
        CapabilityData *capabilityData = avCodecList_->GetCapability(it, true, AVCodecCategory::AVCODEC_NONE);
        ret.push_back(std::make_shared<AudioCaps>(capabilityData));
    }
    return ret;
}

std::vector<CapabilityData *> CapsUnitTest::GetDupCaps(std::string mime, int32_t dupTimes)
{
    std::vector<CapabilityData *> ret;
    for (int32_t i = 0; i < dupTimes; i++) {
        CapabilityData *capabilityData = avCodecList_->GetCapability(mime, true, AVCodecCategory::AVCODEC_NONE);
        if (capabilityData == nullptr) {
            break;
        }
        ret.push_back(capabilityData);
    }
    return ret;
}
#endif

void CapsUnitTest::CheckVideoCapsArray(const std::vector<std::shared_ptr<VideoCaps>> &videoCapsArray) const
{
    for (auto iter = videoCapsArray.begin(); iter != videoCapsArray.end(); iter++) {
        std::shared_ptr<VideoCaps> pVideoCaps = *iter;
        if (pVideoCaps == nullptr) {
            cout << "pVideoCaps is nullptr" << endl;
            break;
        }
        CheckVideoCaps(pVideoCaps);
    }
}

void CapsUnitTest::CheckVideoCaps(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps;
    videoCodecCaps = videoCaps->GetCodecInfo();
    std::string codecName = videoCodecCaps->GetName();
    EXPECT_NE("", codecName);
    cout << "video codecName is : " << codecName << endl;
    if (codecName.compare(AVCodecCodecName::VIDEO_DECODER_AVC_NAME) == 0) {
        CheckAVDecAVC(videoCaps);
    } else if (codecName.compare(AVCodecCodecName::VIDEO_DECODER_HEVC_NAME) == 0) {
        CheckAVDecHEVC(videoCaps);
    } else if (codecName.compare(AVCodecCodecName::VIDEO_DECODER_MPEG2_NAME) == 0) {
        CheckAVDecMpeg2Video(videoCaps);
    } else if (codecName.compare(AVCodecCodecName::VIDEO_DECODER_MPEG4_NAME) == 0) {
        CheckAVDecMpeg4(videoCaps);
    } else if (codecName.compare(AVCodecCodecName::VIDEO_DECODER_H263_NAME) == 0) {
        CheckAVDecH263(videoCaps);
    } else if (codecName.compare("OMX.hisi.video.encoder.avc") == 0) {
        CheckAVEncAVC(videoCaps);
    } else if (codecName.compare(AVCodecCodecName::VIDEO_DECODER_WMV3_NAME) == 0) {
        CheckAVDecWMV3(videoCaps);
    } else if (codecName.compare(AVCodecCodecName::VIDEO_DECODER_CINEPAK_NAME) == 0) {
        CheckAVDecCINEPAK(videoCaps);
    }
}

void CapsUnitTest::CheckAVDecHEVC(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::VIDEO_HEVC, videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_GE(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_GE(2, videoCaps->GetSupportedWidth().minVal); // 2: test value
    EXPECT_LE(1920, videoCaps->GetSupportedWidth().maxVal); // 1920: test value
    EXPECT_GE(2, videoCaps->GetSupportedHeight().minVal); // 2: test value
    EXPECT_LE(1920, videoCaps->GetSupportedHeight().maxVal); // 1920: test value
    EXPECT_GE(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_LE(30, videoCaps->GetSupportedFrameRate().maxVal); // 30: test value
    EXPECT_LE(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_LT(0, videoCaps->GetSupportedFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedGraphicFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_LE(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(true, videoCaps->IsSizeSupported(videoCaps->GetSupportedWidth().minVal,
                                               videoCaps->GetSupportedHeight().maxVal));
}

void CapsUnitTest::CheckAVDecH263(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::VIDEO_H263, videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_GE(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(300000000, videoCaps->GetSupportedBitrate().maxVal); // 300000000: test value
    EXPECT_LE(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_LE(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_GE(20, videoCaps->GetSupportedWidth().minVal); // 20: test value
    EXPECT_LE(2048, videoCaps->GetSupportedWidth().maxVal); // 2048: test value
    EXPECT_GE(20, videoCaps->GetSupportedHeight().minVal); // 20: test value
    EXPECT_LE(1152, videoCaps->GetSupportedHeight().maxVal); // 1152: test value
    EXPECT_GE(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_LE(60, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_LT(0, videoCaps->GetSupportedFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedGraphicFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_LE(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(false, videoCaps->IsSizeSupported(videoCaps->GetSupportedWidth().minVal - 1,
                                                videoCaps->GetSupportedHeight().maxVal));
}

void CapsUnitTest::CheckAVDecWMV3(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::VIDEO_WMV3, videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_GE(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_GE(DEFAULT_WIDTH_ALIGNMENT, videoCaps->GetSupportedWidthAlignment());
    EXPECT_GE(DEFAULT_HEIGHT_ALIGNMENT, videoCaps->GetSupportedHeightAlignment());
    EXPECT_GE(DEFAULT_WIDTH_RANGE.minVal, videoCaps->GetSupportedWidth().minVal);
    EXPECT_LE(DEFAULT_WIDTH_RANGE.maxVal, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_GE(DEFAULT_HEIGHT_RANGE.minVal, videoCaps->GetSupportedHeight().minVal);
    EXPECT_LE(DEFAULT_HEIGHT_RANGE.maxVal, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_GE(0, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_LE(MAX_VIDEO_FRAME_RATE, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_LT(0, videoCaps->GetSupportedFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_LE(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(false, videoCaps->IsSizeAndRateSupported(videoCaps->GetSupportedWidth().minVal,
                                                       videoCaps->GetSupportedHeight().maxVal,
                                                       videoCaps->GetSupportedFrameRate().maxVal + 1));
}

void CapsUnitTest::CheckAVDecMpeg2Video(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::VIDEO_MPEG2, videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_GE(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(300000000, videoCaps->GetSupportedBitrate().maxVal); // 300000000: test value
    EXPECT_LE(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_LE(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_GE(2, videoCaps->GetSupportedWidth().minVal); // 2: test value
    EXPECT_LE(4096, videoCaps->GetSupportedWidth().maxVal); // 4096: test value
    EXPECT_GE(2, videoCaps->GetSupportedHeight().minVal);
    EXPECT_LE(4096, videoCaps->GetSupportedHeight().maxVal); // 4096: test value
    EXPECT_GE(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_LE(60, videoCaps->GetSupportedFrameRate().maxVal); // 60: test value
    EXPECT_LE(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_LT(0, videoCaps->GetSupportedFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedGraphicFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_LE(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(true, videoCaps->IsSizeAndRateSupported(videoCaps->GetSupportedWidth().minVal,
                                                   videoCaps->GetSupportedHeight().maxVal,
                                                   videoCaps->GetSupportedFrameRate().maxVal));
    EXPECT_EQ(false, videoCaps->IsSizeAndRateSupported(videoCaps->GetSupportedWidth().minVal - 1,
                                                       videoCaps->GetSupportedHeight().maxVal + 1,
                                                       videoCaps->GetSupportedFrameRate().maxVal));
}

void CapsUnitTest::CheckAVDecMpeg4(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::VIDEO_MPEG4, videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_GE(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(300000000, videoCaps->GetSupportedBitrate().maxVal); // 300000000: test value
    EXPECT_LE(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_LE(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_GE(2, videoCaps->GetSupportedWidth().minVal); // 2: test value
    EXPECT_LE(4096, videoCaps->GetSupportedWidth().maxVal); // 4096: test value
    EXPECT_GE(2, videoCaps->GetSupportedHeight().minVal); // 2: test value
    EXPECT_LE(4096, videoCaps->GetSupportedHeight().maxVal); // 4096: test value
    EXPECT_GE(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_LE(60, videoCaps->GetSupportedFrameRate().maxVal); // 60: test value
    EXPECT_LE(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_LT(0, videoCaps->GetSupportedFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedGraphicFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_LE(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(false, videoCaps->IsSizeAndRateSupported(videoCaps->GetSupportedWidth().minVal,
                                                       videoCaps->GetSupportedHeight().maxVal,
                                                       videoCaps->GetSupportedFrameRate().maxVal + 1));
}

void CapsUnitTest::CheckAVDecAVC(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::VIDEO_AVC, videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_GE(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(300000000, videoCaps->GetSupportedBitrate().maxVal); // 300000000: test value
    EXPECT_GE(2, videoCaps->GetSupportedWidthAlignment()); // 2: test value
    EXPECT_GE(2, videoCaps->GetSupportedHeightAlignment()); // 2: test value
    EXPECT_GE(2, videoCaps->GetSupportedWidth().minVal); // 2: test value
    EXPECT_LE(4096, videoCaps->GetSupportedWidth().maxVal); // 4096: test value
    EXPECT_GE(2, videoCaps->GetSupportedHeight().minVal); // 2: test value
    EXPECT_LE(4096, videoCaps->GetSupportedHeight().maxVal); // 4096: test value
    EXPECT_GE(0, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_LE(120, videoCaps->GetSupportedFrameRate().maxVal); // 120: test value
    EXPECT_LE(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_LT(0, videoCaps->GetSupportedFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedGraphicFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_LE(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(false, videoCaps->IsSizeAndRateSupported(videoCaps->GetSupportedWidth().minVal,
                                                       videoCaps->GetSupportedHeight().maxVal,
                                                       videoCaps->GetSupportedFrameRate().maxVal + 1));
}

void CapsUnitTest::CheckAVEncAVC(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_ENCODER, videoCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::VIDEO_AVC, videoCodecCaps->GetMimeType());
    EXPECT_EQ(1, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(0, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(1, videoCodecCaps->IsVendor());
    EXPECT_GE(10000, videoCaps->GetSupportedBitrate().minVal); // 10000: test value
    EXPECT_LE(100000000, videoCaps->GetSupportedBitrate().maxVal); // 100000000: test value
    EXPECT_GE(4, videoCaps->GetSupportedWidthAlignment()); // 4: test value
    EXPECT_GE(4, videoCaps->GetSupportedHeightAlignment()); // 4: test value
    EXPECT_GE(144, videoCaps->GetSupportedWidth().minVal); // 144: test value
    EXPECT_LE(4096, videoCaps->GetSupportedWidth().maxVal); // 4096: test value
    EXPECT_GE(144, videoCaps->GetSupportedHeight().minVal); // 144: test value
    EXPECT_LE(4096, videoCaps->GetSupportedHeight().maxVal); // 4096: test value
    EXPECT_GE(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_LE(120, videoCaps->GetSupportedFrameRate().maxVal); // 120: test value
    EXPECT_GE(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_LE(100, videoCaps->GetSupportedEncodeQuality().maxVal); // 100: test value
    EXPECT_LE(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_LT(0, videoCaps->GetSupportedFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedGraphicFormats().size());
    EXPECT_LE(3, videoCaps->GetSupportedBitrateMode().size()); // 3: test value
    EXPECT_LE(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(false, videoCaps->IsSizeAndRateSupported(videoCaps->GetSupportedWidth().minVal,
                                                       videoCaps->GetSupportedHeight().maxVal,
                                                       videoCaps->GetSupportedFrameRate().maxVal + 1));
}

void CapsUnitTest::CheckAVDecCINEPAK(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::VIDEO_CINEPAK, videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_GE(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_LE(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_GE(MIN_WIDTH, videoCaps->GetSupportedWidth().minVal);
    EXPECT_LE(MAX_WIDTH, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_GE(MIN_HEIGHT, videoCaps->GetSupportedHeight().minVal);
    EXPECT_LE(MAX_HEIGHT, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_GE(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_LE(MAX_FRAME_RATE, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_LT(0, videoCaps->GetSupportedFormats().size());
    EXPECT_LT(0, videoCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_LE(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(false, videoCaps->IsSizeAndRateSupported(videoCaps->GetSupportedWidth().minVal,
                                                       videoCaps->GetSupportedHeight().maxVal,
                                                       videoCaps->GetSupportedFrameRate().maxVal + 1));
}

void CapsUnitTest::CheckAudioCapsArray(const std::vector<std::shared_ptr<AudioCaps>> &audioCapsArray) const
{
    for (auto iter = audioCapsArray.begin(); iter != audioCapsArray.end(); iter++) {
        std::shared_ptr<AudioCaps> pAudioCaps = *iter;
        if (pAudioCaps == nullptr) {
            cout << "pAudioCaps is nullptr" << endl;
            break;
        }
        CheckAudioCaps(pAudioCaps);
    }
}

void CapsUnitTest::CheckAudioCaps(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps;
    audioCodecCaps = audioCaps->GetCodecInfo();
    std::string codecName = audioCodecCaps->GetName();
    EXPECT_NE("", codecName);
    cout << "audio codecName is : " << codecName << endl;
    if (codecName.compare(AVCodecCodecName::AUDIO_DECODER_MP3_NAME) == 0) {
        CheckAVDecMP3(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_DECODER_AAC_NAME) == 0) {
        CheckAVDecAAC(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME) == 0) {
        CheckAVDecVorbis(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_DECODER_FLAC_NAME) == 0) {
        CheckAVDecFlac(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME) == 0) {
        CheckAVEncAAC(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_DECODER_VIVID_NAME) == 0) {
        CheckAVDecVivid(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_ENCODER_VENDOR_AAC_NAME) == 0) {
        CheckAVEncVendorAAC(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME) == 0) {
        CheckAVEncFlac(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_ENCODER_MP3_NAME) == 0) {
        CheckAVEncMP3(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_ENCODER_AMRNB_NAME) == 0) {
        CheckAVEncAmrnb(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME) == 0) {
        CheckAVDecAmrnb(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_ENCODER_AMRWB_NAME) == 0) {
        CheckAVEncAmrwb(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME) == 0) {
        CheckAVDecAmrwb(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_ENCODER_OPUS_NAME) == 0) {
        CheckAVEncOpus(audioCaps);
    } else if (codecName.compare(AVCodecCodecName::AUDIO_DECODER_OPUS_NAME) == 0) {
        CheckAVDecOpus(audioCaps);
    }
}

void CapsUnitTest::CheckAVDecMP3(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_MPEG, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(MIN_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(MAX_CHANNEL_COUNT_MP3, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVEncMP3(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_MPEG, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(8000, audioCaps->GetSupportedBitrate().minVal); // 8000: max bitrate
    EXPECT_LE(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(MAX_CHANNEL_COUNT_MP3, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecAAC(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_AAC, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(8000, audioCaps->GetSupportedBitrate().minVal);   // 8000: min supported bitrate
    EXPECT_LE(960000, audioCaps->GetSupportedBitrate().maxVal); // 960000: max supported bitrate
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);      // 8: max channal count
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecVorbis(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_VORBIS, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(32000, audioCaps->GetSupportedBitrate().minVal);  // 32000: min supported bitrate
    EXPECT_LE(500000, audioCaps->GetSupportedBitrate().maxVal); // 500000: max supported bitrate
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);      // 8: max channal count
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecFlac(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_FLAC, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(1536000, audioCaps->GetSupportedBitrate().maxVal); // 1536000: max supported bitrate
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVEncFlac(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_FLAC, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(32000, audioCaps->GetSupportedBitrate().minVal); // 32000: min supported bitrate
    EXPECT_LE(1536000, audioCaps->GetSupportedBitrate().maxVal); // 1536000: max supported bitrate
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_GE(-2, audioCaps->GetSupportedComplexity().minVal); // -2: min complexity
    EXPECT_LE(2, audioCaps->GetSupportedComplexity().maxVal); // 2: max complexity
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecOpus(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_OPUS, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(2, audioCaps->GetSupportedChannel().maxVal); // 2: opus support max 2 channels
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LT(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecVivid(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_AVS3DA, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(AUDIO_MIN_BIT_RATE_VIVID_DECODER, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(AUDIO_MAX_BIT_RATE_VIVID_DECODER, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(AUDIO_MAX_CHANNEL_COUNT_VIVID, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
}

void CapsUnitTest::CheckAVEncAAC(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_AAC, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(MAX_AUDIO_BITRATE_AAC, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LT(0, audioCaps->GetSupportedProfiles().size());
    auto profilesVec = audioCaps->GetSupportedProfiles();
    EXPECT_LE(0, profilesVec.at(0));
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVEncVendorAAC(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_AAC, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_LE(MAX_AUDIO_BITRATE_AAC, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LT(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVEncOpus(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_OPUS, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(6000, audioCaps->GetSupportedBitrate().minVal); // 6000: opus min bitrate
    EXPECT_LE(510000, audioCaps->GetSupportedBitrate().maxVal); // 510000: opus max bitrate
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(2, audioCaps->GetSupportedChannel().maxVal); // 2: opus support max 2 channels
    EXPECT_GE(1, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(10, audioCaps->GetSupportedComplexity().maxVal); // 10: opus complexity max
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LT(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LT(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVEncAmrnb(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_AMR_NB, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(4750, audioCaps->GetSupportedBitrate().minVal); // 4750: amrnb min bitrate
    EXPECT_LT(12200, audioCaps->GetSupportedBitrate().maxVal); // 12200: amrnb max bitrate
    EXPECT_GE(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LE(1, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecAmrnb(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_AMR_NB, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(4750, audioCaps->GetSupportedBitrate().minVal); // 4750: amrnb min bitrate
    EXPECT_LE(12200, audioCaps->GetSupportedBitrate().maxVal); // 12200: amrnb max bitrate
    EXPECT_LT(0, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LT(0, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVEncAmrwb(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_AMR_WB, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(6600, audioCaps->GetSupportedBitrate().minVal); // 6600: amrwb min bitrate
    EXPECT_LE(23850, audioCaps->GetSupportedBitrate().maxVal); // 23850: amrwb max bitrate
    EXPECT_LT(0, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LT(0, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecAmrwb(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_AMR_WB, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(6600, audioCaps->GetSupportedBitrate().minVal); // 6600: amrwb min bitrate
    EXPECT_LE(23850, audioCaps->GetSupportedBitrate().maxVal); // 23850: amrwb max bitrate
    EXPECT_LT(0, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LT(0, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVEncG711mu(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_G711MU, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(64000, audioCaps->GetSupportedBitrate().minVal); // 64000: g711mu only support 64000 bitrate
    EXPECT_LE(64000, audioCaps->GetSupportedBitrate().maxVal); // 64000: g711mu only support 64000 bitrate
    EXPECT_LT(0, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LT(0, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecG711mu(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_G711MU, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_GE(64000, audioCaps->GetSupportedBitrate().minVal); // 64000: g711mu only support 64000 bitrate
    EXPECT_LE(64000, audioCaps->GetSupportedBitrate().maxVal); // 64000: g711mu only support 64000 bitrate
    EXPECT_LT(0, audioCaps->GetSupportedChannel().minVal);
    EXPECT_LT(0, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_LE(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_LE(0, audioCaps->GetSupportedFormats().size());
    EXPECT_LT(0, audioCaps->GetSupportedSampleRates().size());
    EXPECT_LE(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_LE(0, audioCaps->GetSupportedLevels().size());
}

/**
 * @tc.name: AVCaps_GetVideoDecoderCaps_001
 * @tc.desc: AVCdecList GetVideoDecoderCaps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetVideoDecoderCaps_001, TestSize.Level1)
{
    std::vector<std::shared_ptr<VideoCaps>> videoDecoderArray;
    videoDecoderArray = GetVideoDecoderCaps();
    CheckVideoCapsArray(videoDecoderArray);
}

/**
 * @tc.name: AVCaps_GetVideoEncoderCaps_001
 * @tc.desc: AVCdecList GetVideoEncoderCaps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetVideoEncoderCaps_001, TestSize.Level1)
{
    std::vector<std::shared_ptr<VideoCaps>> videoEncoderArray;
    videoEncoderArray = GetVideoEncoderCaps();
    CheckVideoCapsArray(videoEncoderArray);
}

/**
 * @tc.name: AVCaps_GetAudioDecoderCaps_001
 * @tc.desc: AVCdecList GetAudioDecoderCaps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetAudioDecoderCaps_001, TestSize.Level1)
{
    std::vector<std::shared_ptr<AudioCaps>> audioDecoderArray;
    audioDecoderArray = GetAudioDecoderCaps();
    CheckAudioCapsArray(audioDecoderArray);
}

/**
 * @tc.name: AVCaps_GetDupCaps_001
 * @tc.desc: AVCdecList GetDupCaps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetDupCaps_001, TestSize.Level1)
{
    std::vector<CapabilityData *> videoCapVec = GetDupCaps(
        std::string(CodecMimeType::VIDEO_AVC), 2); // 2 is multi times
    EXPECT_EQ(videoCapVec.size(), 2); // 2 is exp cap num
    EXPECT_EQ(videoCapVec[0], videoCapVec[1]);

    std::vector<CapabilityData *> audioCapVec = GetDupCaps(
        std::string(CodecMimeType::AUDIO_AAC), 2); // 2 is multi times
    EXPECT_EQ(audioCapVec.size(), 2); // 2 is exp cap num
    EXPECT_EQ(audioCapVec[0], audioCapVec[1]);
}

/**
 * @tc.name: AVCaps_GetAudioEncoderCaps_001
 * @tc.desc: AVCdecList GetAudioEncoderCaps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetAudioEncoderCaps_001, TestSize.Level1)
{
    std::vector<std::shared_ptr<AudioCaps>> audioEncoderArray;
    audioEncoderArray = GetAudioEncoderCaps();
    CheckAudioCapsArray(audioEncoderArray);
}

/**
 * @tc.name: AVCaps_GetSupportedFrameRatesFor_001
 * @tc.desc: AVCdecList GetSupportedFrameRatesFor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetSupportedFrameRatesFor_001, TestSize.Level1)
{
    Range ret;
    std::vector<std::shared_ptr<VideoCaps>> videoDecoderArray = GetVideoDecoderCaps();
    for (auto iter = videoDecoderArray.begin(); iter != videoDecoderArray.end(); iter++) {
        std::shared_ptr<VideoCaps> pVideoCaps = *iter;
        ret = (*iter)->GetSupportedFrameRatesFor(DEFAULT_WIDTH, DEFAULT_HEIGHT);
        EXPECT_GE(ret.minVal, 0);
        int32_t maxVal;
        if (isHardIncluded_) {
            maxVal = 240;
        } else {
            maxVal = 120;
        }
        EXPECT_LE(ret.maxVal, maxVal); // 120: max framerate for video decoder
    }
    std::vector<std::shared_ptr<VideoCaps>> videoEncoderArray = GetVideoEncoderCaps();
    for (auto iter = videoEncoderArray.begin(); iter != videoEncoderArray.end(); iter++) {
        ret = (*iter)->GetSupportedFrameRatesFor(DEFAULT_WIDTH, DEFAULT_HEIGHT);
        EXPECT_GE(ret.minVal, 0);
        EXPECT_LE(ret.maxVal, DEFAULT_FRAMERATE_RANGE.maxVal);
    }
}

/**
 * @tc.name: AVCaps_GetSupportedFrameRatesFor_002
 * @tc.desc: AVCdecList GetSupportedFrameRatesFor not supported size
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetSupportedFrameRatesFor_002, TestSize.Level1)
{
    Range ret;
    std::vector<std::shared_ptr<VideoCaps>> videoDecoderArray = GetVideoDecoderCaps();
    for (auto iter = videoDecoderArray.begin(); iter != videoDecoderArray.end(); iter++) {
        std::shared_ptr<VideoCaps> pVideoCaps = *iter;
        ret = (*iter)->GetSupportedFrameRatesFor(DEFAULT_WIDTH_RANGE.maxVal + 1, DEFAULT_HEIGHT_RANGE.maxVal + 1);
        EXPECT_GE(ret.minVal, 0);
        EXPECT_LE(ret.maxVal, DEFAULT_FRAMERATE_RANGE.maxVal);
    }
    std::vector<std::shared_ptr<VideoCaps>> videoEncoderArray = GetVideoEncoderCaps();
    for (auto iter = videoEncoderArray.begin(); iter != videoEncoderArray.end(); iter++) {
        ret = (*iter)->GetSupportedFrameRatesFor(DEFAULT_WIDTH_RANGE.minVal - 1, DEFAULT_HEIGHT_RANGE.minVal - 1);
        EXPECT_GE(ret.minVal, 0);
        EXPECT_LE(ret.maxVal, DEFAULT_FRAMERATE_RANGE.maxVal);
    }
}

/**
 * @tc.name: AVCaps_GetPreferredFrameRate_001
 * @tc.desc: AVCdecList GetPreferredFrameRate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetPreferredFrameRate_001, TestSize.Level1)
{
    Range ret;
    std::vector<std::shared_ptr<VideoCaps>> videoEncoderArray = GetVideoEncoderCaps();
    for (auto iter = videoEncoderArray.begin(); iter != videoEncoderArray.end(); iter++) {
        ret = (*iter)->GetPreferredFrameRate(DEFAULT_WIDTH, DEFAULT_HEIGHT);
        EXPECT_GE(ret.minVal, 0);
    }
    std::vector<std::shared_ptr<VideoCaps>> videoDecoderArray = GetVideoDecoderCaps();
    for (auto iter = videoDecoderArray.begin(); iter != videoDecoderArray.end(); iter++) {
        ret = (*iter)->GetPreferredFrameRate(DEFAULT_WIDTH, DEFAULT_HEIGHT);
        EXPECT_GE(ret.minVal, 0);
    }
}

/**
 * @tc.name: AVCaps_GetPreferredFrameRate_002
 * @tc.desc: AVCdecList GetPreferredFrameRate for not supported size
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetPreferredFrameRate_002, TestSize.Level1)
{
    Range ret;
    std::vector<std::shared_ptr<VideoCaps>> videoEncoderArray = GetVideoEncoderCaps();
    for (auto iter = videoEncoderArray.begin(); iter != videoEncoderArray.end(); iter++) {
        ret = (*iter)->GetPreferredFrameRate(DEFAULT_WIDTH_RANGE.maxVal + 1, DEFAULT_HEIGHT_RANGE.maxVal + 1);
        EXPECT_GE(ret.minVal, 0);
    }
    std::vector<std::shared_ptr<VideoCaps>> videoDecoderArray = GetVideoDecoderCaps();
    for (auto iter = videoDecoderArray.begin(); iter != videoDecoderArray.end(); iter++) {
        ret = (*iter)->GetPreferredFrameRate(DEFAULT_WIDTH_RANGE.minVal - 1, DEFAULT_HEIGHT_RANGE.minVal - 1);
        EXPECT_GE(ret.minVal, 0);
    }
}

#ifdef CODECLIST_CAPI_UNIT_TEST

/**
 * @tc.name: AVCaps_NormalFormatsCapi_001
 * @tc.desc: supported formats normal get
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_NormalFormatsCapi_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);

    const int32_t *pixFormats = nullptr;
    uint32_t pixFormatNum = 0;
    EXPECT_EQ(OH_AVCapability_GetVideoSupportedPixelFormats(cap, &pixFormats, &pixFormatNum), AV_ERR_OK);
    EXPECT_NE(pixFormats, nullptr);
    EXPECT_GT(pixFormatNum, 0);

    const OH_NativeBuffer_Format *graphicFormats = nullptr;
    uint32_t graphicFormatNum = 0;
    EXPECT_EQ(OH_AVCapability_GetVideoSupportedNativeBufferFormats(cap, &graphicFormats, &graphicFormatNum),
              AV_ERR_OK);
    EXPECT_NE(graphicFormats, nullptr);
    EXPECT_GT(graphicFormatNum, 0);
}

/**
 * @tc.name: AVCaps_NullvalToCapi_001
 * @tc.desc: AVCdecList GetCapi for not null val
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_NullvalToCapi_001, TestSize.Level1)
{
    EXPECT_EQ(OH_AVCapability_IsHardware(nullptr), false);

    EXPECT_STREQ(OH_AVCapability_GetName(nullptr), "");

    EXPECT_EQ(OH_AVCapability_GetMaxSupportedInstances(nullptr), 0);

    const int32_t *sampleRates = nullptr;
    uint32_t sampleRateNum = 0;
    EXPECT_EQ(OH_AVCapability_GetAudioSupportedSampleRates(nullptr, &sampleRates, &sampleRateNum), AV_ERR_INVALID_VAL);
    EXPECT_EQ(sampleRates, nullptr);
    EXPECT_EQ(sampleRateNum, 0);

    EXPECT_EQ(OH_AVCapability_IsVideoSizeSupported(nullptr, DEFAULT_WIDTH, DEFAULT_HEIGHT), false);

    EXPECT_EQ(
        OH_AVCapability_AreVideoSizeAndFrameRateSupported(nullptr, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FRAMERATE),
        false);

    const int32_t *pixFormats = nullptr;
    uint32_t pixFormatNum = -1;
    EXPECT_EQ(OH_AVCapability_GetVideoSupportedPixelFormats(nullptr, &pixFormats, &pixFormatNum), AV_ERR_INVALID_VAL);
    EXPECT_EQ(pixFormats, nullptr);
    EXPECT_EQ(pixFormatNum, 0);

    const OH_NativeBuffer_Format *graphicFormats = nullptr;
    uint32_t graphicFormatNum = -1;
    EXPECT_EQ(OH_AVCapability_GetVideoSupportedNativeBufferFormats(nullptr, &graphicFormats, &graphicFormatNum),
              AV_ERR_INVALID_VAL);
    EXPECT_EQ(graphicFormats, nullptr);
    EXPECT_EQ(graphicFormatNum, 0);


    const int32_t *profiles = nullptr;
    uint32_t profileNum = -1;
    EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(nullptr, &profiles, &profileNum), AV_ERR_INVALID_VAL);
    EXPECT_EQ(profiles, nullptr);
    EXPECT_EQ(profileNum, 0);

    const int32_t *levels = nullptr;
    uint32_t levelNum = -1;
    EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(nullptr, DEFAULT_VIDEO_AVC_PROFILE, &levels, &levelNum),
              AV_ERR_INVALID_VAL);
    EXPECT_EQ(levels, nullptr);
    EXPECT_EQ(levelNum, 0);

    EXPECT_EQ(OH_AVCapability_AreProfileAndLevelSupported(nullptr, DEFAULT_VIDEO_AVC_PROFILE, AVC_LEVEL_1), false);
}

/**
 * @tc.name: AVCaps_NullvalToCapi_002
 * @tc.desc: AVCdecList GetCapi for not null val
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_NullvalToCapi_002, TestSize.Level1)
{
    OH_AVRange range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetEncoderBitrateRange(nullptr, &range), AV_ERR_INVALID_VAL);
    EXPECT_EQ(range.minVal, 0);
    EXPECT_EQ(range.maxVal, 0);

    range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetEncoderQualityRange(nullptr, &range), AV_ERR_INVALID_VAL);
    EXPECT_EQ(range.minVal, 0);
    EXPECT_EQ(range.maxVal, 0);

    range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetEncoderComplexityRange(nullptr, &range), AV_ERR_INVALID_VAL);
    EXPECT_EQ(range.minVal, 0);
    EXPECT_EQ(range.maxVal, 0);

    range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetAudioChannelCountRange(nullptr, &range), AV_ERR_INVALID_VAL);
    EXPECT_EQ(range.minVal, 0);
    EXPECT_EQ(range.maxVal, 0);

    range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoWidthRangeForHeight(nullptr, DEFAULT_HEIGHT, &range), AV_ERR_INVALID_VAL);
    EXPECT_EQ(range.minVal, 0);
    EXPECT_EQ(range.maxVal, 0);

    range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoHeightRangeForWidth(nullptr, DEFAULT_WIDTH, &range), AV_ERR_INVALID_VAL);
    EXPECT_EQ(range.minVal, 0);
    EXPECT_EQ(range.maxVal, 0);

    range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoWidthRange(nullptr, &range), AV_ERR_INVALID_VAL);
    EXPECT_EQ(range.minVal, 0);
    EXPECT_EQ(range.maxVal, 0);

    range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoHeightRange(nullptr, &range), AV_ERR_INVALID_VAL);
    EXPECT_EQ(range.minVal, 0);
    EXPECT_EQ(range.maxVal, 0);

    range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoFrameRateRange(nullptr, &range), AV_ERR_INVALID_VAL);
    EXPECT_EQ(range.minVal, 0);
    EXPECT_EQ(range.maxVal, 0);

    range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoFrameRateRangeForSize(nullptr, DEFAULT_WIDTH, DEFAULT_HEIGHT, &range),
              AV_ERR_INVALID_VAL);
    EXPECT_EQ(range.minVal, 0);
    EXPECT_EQ(range.maxVal, 0);
}

/**
 * @tc.name: AVCaps_FeatureCheck_001
 * @tc.desc: AVCaps feature check, valid input
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_FeatureCheck_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    EXPECT_NE(cap, nullptr);
    std::string nameStr = OH_AVCapability_GetName(cap);
    std::string mimeStr = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    auto it = CAPABILITY_ENCODER_HARD_NAME.find(mimeStr);
    if (it != CAPABILITY_ENCODER_HARD_NAME.end()) {
        if (nameStr.compare(it->second) == 0) {
            EXPECT_EQ(OH_AVCapability_IsFeatureSupported(cap, VIDEO_ENCODER_TEMPORAL_SCALABILITY), true);
            EXPECT_EQ(OH_AVCapability_IsFeatureSupported(cap, VIDEO_ENCODER_LONG_TERM_REFERENCE), true);
            EXPECT_EQ(OH_AVCapability_IsFeatureSupported(cap, VIDEO_LOW_LATENCY), true);
        } else {
            EXPECT_EQ(OH_AVCapability_IsFeatureSupported(cap, VIDEO_ENCODER_TEMPORAL_SCALABILITY), false);
            EXPECT_EQ(OH_AVCapability_IsFeatureSupported(cap, VIDEO_ENCODER_LONG_TERM_REFERENCE), false);
            EXPECT_EQ(OH_AVCapability_IsFeatureSupported(cap, VIDEO_LOW_LATENCY), false);
        }
    }
}

/**
 * @tc.name: AVCaps_FeatureCheck_002
 * @tc.desc: AVCaps feature check, invalid input
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_FeatureCheck_002, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    EXPECT_NE(cap, nullptr);
    EXPECT_EQ(OH_AVCapability_IsFeatureSupported(cap, static_cast<OH_AVCapabilityFeature>(-1)), false);
    EXPECT_EQ(OH_AVCapability_IsFeatureSupported(cap, static_cast<OH_AVCapabilityFeature>(4)), false);
}

/**
 * @tc.name: AVCaps_FeatureCheck_003
 * @tc.desc: AVCaps feature check, valid input
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_FeatureCheck_003, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true, HARDWARE);
    EXPECT_NE(cap, nullptr);
    const char *targetMimeType = OH_AVCapability_GetMimeType(cap);
    EXPECT_STREQ(targetMimeType, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    EXPECT_TRUE(OH_AVCapability_CheckMimeType(cap, OH_AVCODEC_MIMETYPE_VIDEO_HEVC));
    EXPECT_FALSE(OH_AVCapability_CheckMimeType(cap, OH_AVCODEC_MIMETYPE_VIDEO_AVC));
}

/**
 * @tc.name: AVCaps_GetCapabilityList_001
 * @tc.desc: AVCaps GetCapabilityList with valid codec type
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetCapabilityList_001, TestSize.Level1)
{
    OH_AVCodecType codecTypes[] = {
        OH_AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER,
        OH_AVCodecType::AVCODEC_TYPE_VIDEO_DECODER,
        OH_AVCodecType::AVCODEC_TYPE_AUDIO_ENCODER,
        OH_AVCodecType::AVCODEC_TYPE_AUDIO_DECODER
    };

    for (auto codecType : codecTypes) {
        uint32_t count = 0;
        OH_AVCapability **capList = OH_AVCodec_GetCapabilityList(codecType, &count);

        ASSERT_NE(capList, nullptr);
        ASSERT_GT(count, 0);

        for (uint32_t i = 0; i < count; i++) {
            ASSERT_NE(capList[i], nullptr);
            const char *codecName = OH_AVCapability_GetName(capList[i]);
            const char *mimeType = OH_AVCapability_GetMimeType(capList[i]);
            ASSERT_NE(codecName, nullptr);
            ASSERT_NE(mimeType, nullptr);
            EXPECT_GT(strlen(codecName), 0U);
            EXPECT_GT(strlen(mimeType), 0U);
            EXPECT_TRUE(OH_AVCapability_CheckMimeType(capList[i], mimeType));
        }
    }
}

/**
 * @tc.name: AVCaps_GetCapabilityList_MemoryOverwrite_001
 * @tc.desc: AVCaps GetCapabilityList called multiple times to check if the returned capability list is consistent and not overwritten.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetCapabilityList_MemoryOverwrite_002, TestSize.Level1)
{
    uint32_t count1 = 0;
    uint32_t count2 = 0;
    OH_AVCapability **capList1 = OH_AVCodec_GetCapabilityList(
        OH_AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, &count1);
    ASSERT_NE(capList1, nullptr);
    ASSERT_GT(count1, 0);
    ASSERT_NE(capList1[0], nullptr);



    for (uint32_t i = 0; i < count1; i++) {
        const char *name = nullptr;
        if (capList1[i] != nullptr) {
            name = OH_AVCapability_GetName(capList1[i]);
        }
    
        
    }

    const char *nameBefore = OH_AVCapability_GetName(capList1[0]);
    ASSERT_NE(nameBefore, nullptr);
    std::string firstName(nameBefore);

    OH_AVCapability **capList2 = OH_AVCodec_GetCapabilityList(
        OH_AVCodecType::AVCODEC_TYPE_AUDIO_ENCODER, &count2);
    ASSERT_NE(capList2, nullptr);
    ASSERT_GT(count2, 0);



    for (uint32_t i = 0; i < count2; i++) {
        const char *name = nullptr;
        if (capList2[i] != nullptr) {
            name = OH_AVCapability_GetName(capList2[i]);
        }


    }



    for (uint32_t i = 0; i < count1; i++) {
        const char *name = nullptr;
        if (capList1[i] != nullptr) {
            name = OH_AVCapability_GetName(capList1[i]);
        }


    }

    ASSERT_NE(capList1[0], nullptr);
    const char *nameAfter = OH_AVCapability_GetName(capList1[0]);
    ASSERT_NE(nameAfter, nullptr);






    EXPECT_STRNE(firstName.c_str(), nameAfter);
}

/**
 * @tc.name: AVCaps_GetCapabilityList_THREAD_POOL_001
 * @tc.desc: Verify that OH_AVCodec_GetCapabilityList can be invoked concurrently
 *           from multiple threads and returned capability entries are accessible.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetCapabilityList_THREAD_POOL_001, TestSize.Level2)
{
    const int32_t threadCnt = 10;
    std::vector<std::thread> threadPool;

    for (int32_t i = 0; i < threadCnt; i++) {
        threadPool.emplace_back([i]() {
            uint32_t count = 0;
            OH_AVCodecType codecType = static_cast<OH_AVCodecType>(i % 4);
            OH_AVCapability **capList = OH_AVCodec_GetCapabilityList(codecType, &count);

            EXPECT_NE(capList, nullptr);
            EXPECT_GT(count, 0);

            for (uint32_t j = 0; j < count; j++) {
                ASSERT_NE(capList[j], nullptr);
                const char *name = OH_AVCapability_GetName(capList[j]);
                EXPECT_NE(name, nullptr);
            }
        });
    }

    for (auto &th : threadPool) {
        th.join();
    }
}

/**
 * @tc.name: AVCaps_GetCapabilityList_THREAD_POOL_002
 * @tc.desc: Verify concurrent repeated calls to OH_AVCodec_GetCapabilityList with different codec types,
 *           and check that returned capability name and mime type can be queried normally.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetCapabilityList_THREAD_POOL_002, TestSize.Level2)
{
    const int32_t threadCnt = 12;
    const int32_t loopCnt = 30;
    std::vector<std::thread> threadPool;

    for (int32_t i = 0; i < threadCnt; i++) {
        threadPool.emplace_back([i, loopCnt]() {
            OH_AVCodecType codecType;
            switch (i % 4) {
                case 0:
                    codecType = OH_AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER;
                    break;
                case 1:
                    codecType = OH_AVCodecType::AVCODEC_TYPE_VIDEO_DECODER;
                    break;
                case 2:
                    codecType = OH_AVCodecType::AVCODEC_TYPE_AUDIO_ENCODER;
                    break;
                default:
                    codecType = OH_AVCodecType::AVCODEC_TYPE_AUDIO_DECODER;
                    break;
            }

            for (int32_t k = 0; k < loopCnt; k++) {
                uint32_t count = 0;
                OH_AVCapability **capList = OH_AVCodec_GetCapabilityList(codecType, &count);
                EXPECT_NE(capList, nullptr);
                EXPECT_GT(count, 0);

                for (uint32_t j = 0; j < count; j++) {
                    ASSERT_NE(capList[j], nullptr);
                    const char *mime = OH_AVCapability_GetMimeType(capList[j]);
                    const char *name = OH_AVCapability_GetName(capList[j]);
                    EXPECT_NE(mime, nullptr);
                    EXPECT_NE(name, nullptr);
                }
            }
        });
    }

    for (auto &th : threadPool) {
        th.join();
    }
}

/**
 * @tc.name: AVCaps_IsSecure_001
 * @tc.desc: Check IsSecure with valid capability, and call twice to check the result is same
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_IsSecure_001, TestSize.Level1)
{
    OH_AVCodecType codecTypes[] = {
        OH_AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER,
        OH_AVCodecType::AVCODEC_TYPE_VIDEO_DECODER,
        OH_AVCodecType::AVCODEC_TYPE_AUDIO_ENCODER,
        OH_AVCodecType::AVCODEC_TYPE_AUDIO_DECODER
    };

    for (auto codecType : codecTypes) {
        uint32_t count = 0;
        OH_AVCapability **capList = OH_AVCodec_GetCapabilityList(codecType, &count);

        ASSERT_NE(capList, nullptr);
        ASSERT_GT(count, 0);

        for (uint32_t i = 0; i < count; i++) {
            ASSERT_NE(capList[i], nullptr);

            bool secure = OH_AVCapability_IsSecure(capList[i]);
            bool secureAgain = OH_AVCapability_IsSecure(capList[i]);
            EXPECT_EQ(secure, secureAgain);
        }
    }
}

/**
 * @tc.name: AVCaps_IsSecure_002
 * @tc.desc: Check IsSecure with nullptr
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_IsSecure_002, TestSize.Level1)
{
    EXPECT_FALSE(OH_AVCapability_IsSecure(nullptr));
}

/**
 * @tc.name: AVCaps_FeatureProperties_001
 * @tc.desc: AVCaps query feature with properties
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_FeatureProperties_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    EXPECT_NE(cap, nullptr);
    std::string nameStr = OH_AVCapability_GetName(cap);
    std::string mimeStr = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    auto it = CAPABILITY_ENCODER_HARD_NAME.find(mimeStr);
    if (it != CAPABILITY_ENCODER_HARD_NAME.end()) {
        if (nameStr.compare(it->second) == 0) {
            OH_AVFormat *property = OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_LONG_TERM_REFERENCE);
            EXPECT_NE(property, nullptr);
            int ltrNum = 0;
            EXPECT_EQ(OH_AVFormat_GetIntValue(
                property, OH_FEATURE_PROPERTY_KEY_VIDEO_ENCODER_MAX_LTR_FRAME_COUNT, &ltrNum), true);
            EXPECT_GT(ltrNum, 0);
            OH_AVFormat_Destroy(property);
        } else {
            OH_AVFormat *property = OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_LONG_TERM_REFERENCE);
            EXPECT_EQ(property, nullptr);
        }
    }
}

/**
 * @tc.name: AVCaps_FeatureProperties_002
 * @tc.desc: AVCaps query feature without properties
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_FeatureProperties_002, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVFormat *property = OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_TEMPORAL_SCALABILITY);
    EXPECT_EQ(property, nullptr);
}

/**
 * @tc.name: AVCaps_FeatureProperties_003
 * @tc.desc: AVCaps query unspported feature properties
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_FeatureProperties_003, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVFormat *property = OH_AVCapability_GetFeatureProperties(cap, VIDEO_LOW_LATENCY);
    EXPECT_EQ(property, nullptr);
}

/**
 * @tc.name: AVCaps_FeatureProperties_004
 * @tc.desc: AVCaps query unspported feature properties
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_FeatureProperties_004, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true, HARDWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVFormat *property = OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
    EXPECT_EQ(property, nullptr);
}

/**
 * @tc.name: AVCaps_Levels_001
 * @tc.desc: AVCaps query H264 hw decoder supported levels for baseline\main\high profile
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_Levels_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
    EXPECT_NE(cap, nullptr);
    const int32_t *profiles = nullptr;
    uint32_t profilesNum = -1;
    EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profilesNum), AV_ERR_OK);
    EXPECT_GT(profilesNum, 0);
    for (int32_t i = 0; i < profilesNum; i++) {
        int32_t profile = profiles[i];
        EXPECT_GE(profile, AVC_PROFILE_BASELINE);
        EXPECT_LE(profile, AVC_PROFILE_MAIN);
        const int32_t *levels = nullptr;
        uint32_t levelsNum = -1;
        EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &levels, &levelsNum), AV_ERR_OK);
        EXPECT_GT(levelsNum, 0);
        EXPECT_LE(levelsNum, AVC_LEVEL_62 + 1);
        for (int32_t j = 0; j < levelsNum; j++) {
            int32_t level = levels[j];
            EXPECT_GE(level, AVC_LEVEL_1);
            EXPECT_LE(level, AVC_LEVEL_62);
        }
    }
}

/**
 * @tc.name: AVCaps_Levels_002
 * @tc.desc: AVCaps query H265 hw decoder supported levels for main\main10 profile
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_Levels_002, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    EXPECT_NE(cap, nullptr);
    const int32_t *profiles = nullptr;
    uint32_t profilesNum = -1;
    EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profilesNum), AV_ERR_OK);
    EXPECT_GT(profilesNum, 0);
    for (int32_t i = 0; i < profilesNum; i++) {
        int32_t profile = profiles[i];
        EXPECT_GE(profile, HEVC_PROFILE_MAIN);
        EXPECT_LE(profile, HEVC_PROFILE_MAIN_10_HDR10_PLUS);
        const int32_t *levels = nullptr;
        uint32_t levelsNum = -1;
        EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &levels, &levelsNum), AV_ERR_OK);
        EXPECT_GT(levelsNum, 0);
        EXPECT_LE(levelsNum, HEVC_LEVEL_62 + 1);
        for (int32_t j = 0; j < levelsNum; j++) {
            int32_t level = levels[j];
            EXPECT_GE(level, HEVC_LEVEL_1);
            EXPECT_LE(level, HEVC_LEVEL_62);
        }
    }
}

/**
 * @tc.name: AVCaps_Levels_003
 * @tc.desc: AVCaps query H264 hw encoder supported levels for baseline\main\high profile
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_Levels_003, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    EXPECT_NE(cap, nullptr);
    const int32_t *profiles = nullptr;
    uint32_t profilesNum = -1;
    EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profilesNum), AV_ERR_OK);
    EXPECT_GT(profilesNum, 0);
    for (int32_t i = 0; i < profilesNum; i++) {
        int32_t profile = profiles[i];
        EXPECT_GE(profile, AVC_PROFILE_BASELINE);
        EXPECT_LE(profile, AVC_PROFILE_MAIN);
        const int32_t *levels = nullptr;
        uint32_t levelsNum = -1;
        EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &levels, &levelsNum), AV_ERR_OK);
        EXPECT_GT(levelsNum, 0);
        EXPECT_LE(levelsNum, AVC_LEVEL_62 + 1);
        for (int32_t j = 0; j < levelsNum; j++) {
            int32_t level = levels[j];
            EXPECT_GE(level, AVC_LEVEL_1);
            EXPECT_LE(level, AVC_LEVEL_62);
        }
    }
}

/**
 * @tc.name: AVCaps_Levels_004
 * @tc.desc: AVCaps query H265 hw encoder supported levels for main\main10 profile
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_Levels_004, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true, HARDWARE);
    EXPECT_NE(cap, nullptr);
    const int32_t *profiles = nullptr;
    uint32_t profilesNum = -1;
    EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profilesNum), AV_ERR_OK);
    EXPECT_GT(profilesNum, 0);
    for (int32_t i = 0; i < profilesNum; i++) {
        int32_t profile = profiles[i];
        EXPECT_GE(profile, HEVC_PROFILE_MAIN);
        EXPECT_LE(profile, HEVC_PROFILE_MAIN_10_HDR10_PLUS);
        const int32_t *levels = nullptr;
        uint32_t levelsNum = -1;
        EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &levels, &levelsNum), AV_ERR_OK);
        EXPECT_GT(levelsNum, 0);
        EXPECT_LE(levelsNum, HEVC_LEVEL_62 + 1);
        for (int32_t j = 0; j < levelsNum; j++) {
            int32_t level = levels[j];
            EXPECT_GE(level, HEVC_LEVEL_1);
            EXPECT_LE(level, HEVC_LEVEL_62);
        }
    }
}

/**
 * @tc.name: AVCaps_Levels_005
 * @tc.desc: AVCaps query H264 sw decoder supported levels for baseline\main\high profile
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_Levels_005, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    const int32_t *profiles = nullptr;
    uint32_t profilesNum = -1;
    EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profilesNum), AV_ERR_OK);
    EXPECT_GT(profilesNum, 0);
    for (int32_t i = 0; i < profilesNum; i++) {
        int32_t profile = profiles[i];
        EXPECT_GE(profile, AVC_PROFILE_BASELINE);
        EXPECT_LE(profile, AVC_PROFILE_MAIN);
        const int32_t *levels = nullptr;
        uint32_t levelsNum = -1;
        EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &levels, &levelsNum), AV_ERR_OK);
        EXPECT_GT(levelsNum, 0);
        EXPECT_LE(levelsNum, AVC_LEVEL_62 + 1);
        for (int32_t j = 0; j < levelsNum; j++) {
            int32_t level = levels[j];
            EXPECT_GE(level, AVC_LEVEL_1);
            EXPECT_LE(level, AVC_LEVEL_62);
        }
    }
}

/**
 * @tc.name: AVCaps_MixedUse_001
 * @tc.desc: AVCaps mixed use cap, video cap to get audio info
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_MixedUse_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVRange range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetAudioChannelCountRange(cap, &range), AV_ERR_OK);
}

/**
 * @tc.name: AVCaps_MixedUse_002
 * @tc.desc: AVCaps mixed use cap, audio cap to get video info
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_MixedUse_002, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_AUDIO_AAC, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVRange range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoWidthRange(cap, &range), AV_ERR_OK);

    const OH_NativeBuffer_Format *graphicFormats = nullptr;
    uint32_t graphicFormatNum = -1;
    EXPECT_EQ(OH_AVCapability_GetVideoSupportedNativeBufferFormats(cap, &graphicFormats, &graphicFormatNum),
              AV_ERR_INVALID_VAL);
    EXPECT_EQ(graphicFormats, nullptr);
    EXPECT_EQ(graphicFormatNum, 0);
}

/**
 * @tc.name: AVCaps_MixedUse_003
 * @tc.desc: AVCaps mixed use cap, decoder cap to get encoder info
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_MixedUse_003, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVRange range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetEncoderQualityRange(cap, &range), AV_ERR_OK);
}

HWTEST_F(CapsUnitTest, AVCaps_THREAD_POOL_001, TestSize.Level2)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    const int threadCnt = 10;
    std::vector<std::thread> threadPool;
    for (int32_t i = 0; i < threadCnt; i++) {
        threadPool.emplace_back([&cap]() {
            const int32_t *profiles = nullptr;
            uint32_t profilesNum = -1;
            EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profilesNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profilesNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profilesNum), AV_ERR_OK);
        });
    }
    for (auto &th : threadPool) {
        th.join();
    }
}

HWTEST_F(CapsUnitTest, AVCaps_THREAD_POOL_002, TestSize.Level2)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    const int threadCnt = 10;
    std::vector<std::thread> threadPool;
    for (int32_t i = 0; i < threadCnt; i++) {
        threadPool.emplace_back([&cap]() {
            const int32_t *levels = nullptr;
            uint32_t levelsNum = -1;
            int32_t profile = static_cast<int32_t>(AVC_PROFILE_BASELINE);
            EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &levels, &levelsNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &levels, &levelsNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &levels, &levelsNum), AV_ERR_OK);
        });
    }
    for (auto &th : threadPool) {
        th.join();
    }
}

HWTEST_F(CapsUnitTest, AVCaps_THREAD_POOL_003, TestSize.Level2)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    const int threadCnt = 10;
    std::vector<std::thread> threadPool;
    for (int32_t i = 0; i < threadCnt; i++) {
        threadPool.emplace_back([&cap]() {
            const int32_t *sampleRates = nullptr;
            uint32_t sampleRateNum = 0;
            EXPECT_EQ(OH_AVCapability_GetAudioSupportedSampleRates(cap, &sampleRates, &sampleRateNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetAudioSupportedSampleRates(cap, &sampleRates, &sampleRateNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetAudioSupportedSampleRates(cap, &sampleRates, &sampleRateNum), AV_ERR_OK);
        });
    }
    for (auto &th : threadPool) {
        th.join();
    }
}

HWTEST_F(CapsUnitTest, AVCaps_THREAD_POOL_004, TestSize.Level2)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    const int threadCnt = 10;
    std::vector<std::thread> threadPool;
    for (int32_t i = 0; i < threadCnt; i++) {
        threadPool.emplace_back([&cap]() {
            const int32_t *pixFormats = nullptr;
            uint32_t pixFormatNum = -1;
            EXPECT_EQ(OH_AVCapability_GetVideoSupportedPixelFormats(cap, &pixFormats, &pixFormatNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetVideoSupportedPixelFormats(cap, &pixFormats, &pixFormatNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetVideoSupportedPixelFormats(cap, &pixFormats, &pixFormatNum), AV_ERR_OK);
        });
    }
    for (auto &th : threadPool) {
        th.join();
    }
}

/**
 * @tc.name: AVCaps_GetCapabilityByCategory_001
 * @tc.desc: AVCaps GetCapabilityByCategory
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetCapabilityByCategory_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
}

/**
 * @tc.name: AVCaps_GetVideoWidthRangeForHeight_001
 * @tc.desc: AVCaps query spported GetVideoWidthRangeForHeight
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetVideoWidthRangeForHeight_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVRange range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoWidthRangeForHeight(cap, DEFAULT_HEIGHT, &range), AV_ERR_OK);
    EXPECT_EQ(16, range.minVal);
    EXPECT_EQ(1920, range.maxVal);
}

/**
 * @tc.name: AVCaps_GetVideoHeightRangeForWidth_001
 * @tc.desc: AVCaps query spported GetVideoHeightRangeForWidth
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetVideoHeightRangeForWidth_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVRange range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoHeightRangeForWidth(cap, DEFAULT_WIDTH, &range), AV_ERR_OK);
    EXPECT_EQ(16, range.minVal);
    EXPECT_EQ(1080, range.maxVal);
}

/**
 * @tc.name: AVCaps_GetVideoWidthRange_001
 * @tc.desc: AVCaps query spported GetVideoWidthRange
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetVideoWidthRange_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVRange range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoWidthRange(cap, &range), AV_ERR_OK);
    EXPECT_EQ(16, range.minVal);
    EXPECT_EQ(1920, range.maxVal);
}

/**
 * @tc.name: AVCaps_GetVideoHeightRange_001
 * @tc.desc: AVCaps query spported GetVideoHeightRange
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetVideoHeightRange_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVRange range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoHeightRange(cap, &range), AV_ERR_OK);
    EXPECT_EQ(16, range.minVal);
    EXPECT_EQ(1920, range.maxVal);
}

/**
 * @tc.name: AVCaps_IsVideoSizeSupported_001
 * @tc.desc: AVCaps query spported IsVideoSizeSupported
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_IsVideoSizeSupported_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    EXPECT_EQ(OH_AVCapability_IsVideoSizeSupported(cap, DEFAULT_WIDTH, DEFAULT_HEIGHT), true);
}

/**
 * @tc.name: AVCaps_GetSupportedProfiles_GetSupportedLevelsForProfile_001
 * @tc.desc: AVCaps query supported levels for simple\main\advanced profile
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetSupportedProfiles_GetSupportedLevelsForProfile_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    const int32_t *profiles = nullptr;
    uint32_t profilesNum = -1;
    EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profilesNum), AV_ERR_OK);
    EXPECT_GT(profilesNum, 0);
    for (int32_t i = 0; i < profilesNum; i++) {
        int32_t profile = profiles[i];
        EXPECT_GE(profile, WMV3_PROFILE_SIMPLE);
        EXPECT_LE(profile, WMV3_PROFILE_MAIN);
        const int32_t *levels = nullptr;
        uint32_t levelsNum = -1;
        EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &levels, &levelsNum), AV_ERR_OK);
        EXPECT_GT(levelsNum, 0);
        EXPECT_LE(levelsNum, WMV3_LEVEL_HIGH + 1);
        for (int32_t j = 0; j < levelsNum; j++) {
            int32_t level = levels[j];
            EXPECT_GE(level, WMV3_LEVEL_LOW);
            EXPECT_LE(level, WMV3_LEVEL_HIGH);
        }
    }
}

/**
 * @tc.name: AVCaps_AreProfileAndLevelSupported_001
 * @tc.desc: AVCaps query spported AreProfileAndLevelSupported
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_AreProfileAndLevelSupported_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    EXPECT_EQ(OH_AVCapability_AreProfileAndLevelSupported(cap, WMV3_PROFILE_MAIN, WMV3_LEVEL_LOW), true);
}

/**
 * @tc.name: AVCaps_GetMaxSupportedInstances_001
 * @tc.desc: AVCaps query spported etMaxSupportedInstances
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetMaxSupportedInstances_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    EXPECT_EQ(OH_AVCapability_GetMaxSupportedInstances(cap), 64);
}

/**
 * @tc.name: AVCaps_GetVideoFrameRateRange_001
 * @tc.desc: AVCaps query spported GetVideoFrameRateRange_
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetVideoFrameRateRange_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVRange range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoFrameRateRange(cap, &range), AV_ERR_OK);
    EXPECT_EQ(0, range.minVal);
    EXPECT_EQ(30, range.maxVal);
}

/**
 * @tc.name: AVCaps_GetVideoFrameRateRangeForSize_001
 * @tc.desc: AVCaps query spported GetVideoFrameRateRangeForSize
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetVideoFrameRateRangeForSize_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVRange range = {-1, -1};
    EXPECT_EQ(OH_AVCapability_GetVideoFrameRateRangeForSize(cap, DEFAULT_WIDTH, DEFAULT_HEIGHT, &range), AV_ERR_OK);;
    EXPECT_EQ(0, range.minVal);
    EXPECT_EQ(30, range.maxVal);
}

/**
 * @tc.name: AVCaps_AreVideoSizeAndFrameRateSupported_001
 * @tc.desc: AVCaps query spported AreVideoSizeAndFrameRateSupported
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_AreVideoSizeAndFrameRateSupported_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    EXPECT_EQ(
        OH_AVCapability_AreVideoSizeAndFrameRateSupported(cap, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FRAMERATE),
        true);
}

/**
 * @tc.name: AVCaps_IsFeatureSupported_001
 * @tc.desc: AVCaps query unspported IsFeatureSupported
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_IsFeatureSupported_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    std::string nameStr = OH_AVCapability_GetName(cap);
    std::string mimeStr = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    auto it = CAPABILITY_DECODER_NAME.find(mimeStr);
    if (it != CAPABILITY_DECODER_NAME.end()) {
        if (nameStr.compare(it->second) == 0) {
            EXPECT_EQ(OH_AVCapability_IsFeatureSupported(cap, VIDEO_ENCODER_TEMPORAL_SCALABILITY), false);
            EXPECT_EQ(OH_AVCapability_IsFeatureSupported(cap, VIDEO_ENCODER_LONG_TERM_REFERENCE), false);
            EXPECT_EQ(OH_AVCapability_IsFeatureSupported(cap, VIDEO_LOW_LATENCY), false);
        }
    }
}

/**
 * @tc.name: AVCaps_GetFeatureProperties_001
 * @tc.desc: AVCaps query spported GetFeatureProperties
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetFeatureProperties_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    OH_AVFormat *property = OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
    EXPECT_EQ(property, nullptr);
}

/**
 * @tc.name: AVCaps_GetVideoSupportedPixelFormats_001
 * @tc.desc: AVCaps query spported GetVideoSupportedPixelFormats
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CapsUnitTest, AVCaps_GetVideoSupportedPixelFormats_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    const int32_t *pixFormats = nullptr;
    uint32_t pixFormatNum = -1;
    EXPECT_EQ(OH_AVCapability_GetVideoSupportedPixelFormats(cap, &pixFormats, &pixFormatNum), AV_ERR_OK);
    EXPECT_GT(pixFormatNum, 0);
    EXPECT_LE(pixFormatNum, 4);
    for (int32_t j = 0; j < pixFormatNum; j++) {
        int32_t pixFormat = pixFormats[j];
        EXPECT_GE(pixFormat, static_cast<int32_t>(VideoPixelFormat::YUVI420));
        EXPECT_LE(pixFormat, static_cast<int32_t>(VideoPixelFormat::RGBA));
    }
}

#endif
} // namespace MediaAVCodec
} // namespace OHOS
