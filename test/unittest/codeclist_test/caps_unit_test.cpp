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

#include "caps_unit_test.h"
#include "gtest/gtest.h"
#ifdef CODECLIST_CAPI_UNIT_TEST
#include "native_avmagic.h"
#endif

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec::CodecListTestParam;

namespace OHOS {
namespace MediaAVCodec {
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
#endif

#ifdef CODECLIST_INNER_UNIT_TEST
std::vector<std::shared_ptr<VideoCaps>> CapsUnitTest::GetVideoDecoderCaps()
{
    std::vector<std::shared_ptr<VideoCaps>> ret;
    for (auto it : videoDecoderList) {
        CapabilityData *capabilityData = avCodecList_->GetCapability(it, false, AVCodecCategory::AVCODEC_NONE);
        ret.push_back(std::make_shared<VideoCaps>(capabilityData));
    }
    return ret;
}

std::vector<std::shared_ptr<VideoCaps>> CapsUnitTest::GetVideoEncoderCaps()
{
    std::vector<std::shared_ptr<VideoCaps>> ret;
    if (isHardIncluded_) {
        for (auto it : videoEncoderList) {
            CapabilityData *capabilityData = avCodecList_->GetCapability(it, true, AVCodecCategory::AVCODEC_NONE);
            ret.push_back(std::make_shared<VideoCaps>(capabilityData));
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
    } else if (codecName.compare("OMX.hisi.video.encoder.avc") == 0) {
        CheckAVEncAVC(videoCaps);
    }
}

void CapsUnitTest::CheckAVDecH264(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::VIDEO_AVC, videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_EQ(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.minVal, videoCaps->GetSupportedWidth().minVal);
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.maxVal, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.minVal, videoCaps->GetSupportedHeight().minVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.maxVal, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.maxVal, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(2, videoCaps->GetSupportedFormats().size());  // 2: supported formats count
    EXPECT_EQ(3, videoCaps->GetSupportedProfiles().size()); // 3: supported profile count
    EXPECT_EQ(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
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
    EXPECT_EQ(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.minVal, videoCaps->GetSupportedWidth().minVal);
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.maxVal, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.minVal, videoCaps->GetSupportedHeight().minVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.maxVal, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.maxVal, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(2, videoCaps->GetSupportedFormats().size()); // 2: supported formats count
    EXPECT_EQ(1, videoCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(false, videoCaps->IsSizeSupported(videoCaps->GetSupportedWidth().minVal - 1,
                                                videoCaps->GetSupportedHeight().maxVal));
}

void CapsUnitTest::CheckAVDecMpeg2Video(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::VIDEO_MPEG2, videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_EQ(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.minVal, videoCaps->GetSupportedWidth().minVal);
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.maxVal, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.minVal, videoCaps->GetSupportedHeight().minVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.maxVal, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.maxVal, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(2, videoCaps->GetSupportedFormats().size());  // 2: supported formats count
    EXPECT_EQ(2, videoCaps->GetSupportedProfiles().size()); // 2: supported profile count
    EXPECT_EQ(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(0, videoCaps->IsSizeAndRateSupported(videoCaps->GetSupportedWidth().minVal,
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
    EXPECT_EQ(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.minVal, videoCaps->GetSupportedWidth().minVal);
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.maxVal, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.minVal, videoCaps->GetSupportedHeight().minVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.maxVal, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.maxVal, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(2, videoCaps->GetSupportedFormats().size());  // 2: supported formats count
    EXPECT_EQ(2, videoCaps->GetSupportedProfiles().size()); // 2: supported profile count
    EXPECT_EQ(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
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
    EXPECT_EQ(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(DEFAULT_WIDTH_ALIGNMENT, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(DEFAULT_HEIGHT_ALIGNMENT, videoCaps->GetSupportedHeightAlignment());
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.minVal, videoCaps->GetSupportedWidth().minVal);
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.maxVal, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.minVal, videoCaps->GetSupportedHeight().minVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.maxVal, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.minVal, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.maxVal, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(4, videoCaps->GetSupportedFormats().size());  // 4: supported formats count
    EXPECT_EQ(3, videoCaps->GetSupportedProfiles().size()); // 3: supported profile count
    EXPECT_EQ(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
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
    EXPECT_EQ(DEFAULT_BITRATE_RANGE_ENC.minVal, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(DEFAULT_BITRATE_RANGE_ENC.maxVal, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(DEFAULT_ALIGNMENT_ENC, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(DEFAULT_ALIGNMENT_ENC, videoCaps->GetSupportedHeightAlignment());
    EXPECT_EQ(DEFAULT_WIDTH_RANGE_ENC.minVal, videoCaps->GetSupportedWidth().minVal);
    EXPECT_GE(DEFAULT_WIDTH_RANGE_ENC.maxVal, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE_ENC.minVal, videoCaps->GetSupportedHeight().minVal);
    EXPECT_LE(DEFAULT_HEIGHT_RANGE_ENC.maxVal, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.maxVal, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(DEFAULT_VIDEO_QUALITY_RANGE.minVal, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(DEFAULT_VIDEO_QUALITY_RANGE.maxVal, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_TRUE(!videoCaps->GetSupportedFormats().empty());
    EXPECT_GE(DEFAULT_BITRATEMODE_ENC, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(false, videoCaps->IsSizeAndRateSupported(videoCaps->GetSupportedWidth().minVal,
                                                       videoCaps->GetSupportedHeight().maxVal,
                                                       videoCaps->GetSupportedFrameRate().maxVal + 1));
}

void CapsUnitTest::CheckAVEncMpeg4(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_ENCODER, videoCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::VIDEO_MPEG4, videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_EQ(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.minVal, videoCaps->GetSupportedWidth().minVal);
    EXPECT_EQ(DEFAULT_WIDTH, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.minVal, videoCaps->GetSupportedHeight().minVal);
    EXPECT_EQ(DEFAULT_HEIGHT, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.minVal, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.maxVal, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(2, videoCaps->GetSupportedFormats().size());     // 2: supported formats count
    EXPECT_EQ(2, videoCaps->GetSupportedProfiles().size());    // 2: supported profile count
    EXPECT_EQ(2, videoCaps->GetSupportedBitrateMode().size()); // 2: supported bitretemode count
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
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
    EXPECT_EQ(MIN_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal); // 320000: max supported bitrate
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT_MP3, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(12, audioCaps->GetSupportedSampleRates().size()); // 12: supported samplerate count
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecAAC(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_AAC, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(8000, audioCaps->GetSupportedBitrate().minVal);   // 8000: min supported bitrate
    EXPECT_EQ(960000, audioCaps->GetSupportedBitrate().maxVal); // 960000: max supported bitrate
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);      // 8: max channal count
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(12, audioCaps->GetSupportedSampleRates().size()); // 12: supported samplerate count
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecVorbis(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_VORBIS, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(32000, audioCaps->GetSupportedBitrate().minVal);  // 32000: min supported bitrate
    EXPECT_EQ(500000, audioCaps->GetSupportedBitrate().maxVal); // 500000: max supported bitrate
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);      // 8: max channal count
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(12, audioCaps->GetSupportedSampleRates().size()); // 12: supported samplerate count
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecFlac(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_FLAC, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(2100000, audioCaps->GetSupportedBitrate().maxVal); // 2100000: max supported bitrate
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);       // 8: max channal count
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(13, audioCaps->GetSupportedSampleRates().size()); // 13: supported samplerate count
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecOpus(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_OPUS, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(1, audioCaps->GetSupportedSampleRates().size());
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVDecVivid(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_AVS3DA, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(AUDIO_MIN_BIT_RATE_VIVID_DECODER, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(AUDIO_MAX_BIT_RATE_VIVID_DECODER, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(AUDIO_MAX_CHANNEL_COUNT_VIVID, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(AUDIO_SAMPLE_RATE_COUNT_VIVID, audioCaps->GetSupportedSampleRates().size());
}

void CapsUnitTest::CheckAVEncAAC(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_AAC, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(8000, audioCaps->GetSupportedBitrate().minVal); // 8000: supported min bitrate
    EXPECT_EQ(MAX_AUDIO_BITRATE_AAC, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(DEFAULT_SAMPLE_RATE_SIZE, audioCaps->GetSupportedSampleRates().size()); // 11: supported samplerate count
    EXPECT_EQ(1, audioCaps->GetSupportedProfiles().size());
    auto profilesVec = audioCaps->GetSupportedProfiles();
    EXPECT_EQ(0, profilesVec.at(0));
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void CapsUnitTest::CheckAVEncOpus(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ(CodecMimeType::AUDIO_OPUS, audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(1, audioCaps->GetSupportedSampleRates().size());
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
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
        EXPECT_LE(levelsNum, AVC_LEVEL_51 + 1);
        for (int32_t j = 0; j < levelsNum; j++) {
            int32_t level = levels[j];
            EXPECT_GE(level, AVC_LEVEL_1);
            EXPECT_LE(level, AVC_LEVEL_51);
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
        EXPECT_LE(levelsNum, AVC_LEVEL_51 + 1);
        for (int32_t j = 0; j < levelsNum; j++) {
            int32_t level = levels[j];
            EXPECT_GE(level, AVC_LEVEL_1);
            EXPECT_LE(level, AVC_LEVEL_51);
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
        EXPECT_LE(levelsNum, AVC_LEVEL_51 + 1);
        for (int32_t j = 0; j < levelsNum; j++) {
            int32_t level = levels[j];
            EXPECT_GE(level, AVC_LEVEL_1);
            EXPECT_LE(level, AVC_LEVEL_51);
        }
    }
}

#endif
} // namespace MediaAVCodec
} // namespace OHOS
