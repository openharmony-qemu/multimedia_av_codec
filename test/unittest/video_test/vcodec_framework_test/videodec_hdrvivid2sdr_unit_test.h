/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef VIDEODEC_HDRVIVID2SDR_UNIT_TEST_H
#define VIDEODEC_HDRVIVID2SDR_UNIT_TEST_H
#include "av_common.h"
#include "meta/meta_key.h"
#include "video_processing.h"
#include "video_processing_types.h"
#include "videodec_capi_mock.h"
#ifdef VIDEODEC_ASYNC_UNIT_TEST
#include "vdec_async_sample.h"
#else
#include "vdec_sync_sample.h"
#endif

namespace TESTBASE {
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
class HdrVivid2SdrBaseSuit {
public:
    bool CreateVideoCodecByName(const std::string &decName);
    void SetFormatWithParam(VideoPixelFormat format);
    void IsPixelFormatSupported(OHOS::MediaAVCodec::VideoPixelFormat pixelFormat);
protected:
    std::shared_ptr<CodecListMock> capability_ = nullptr;
    std::shared_ptr<VideoDecSample> videoDec_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    std::shared_ptr<VDecCallbackTest> vdecCallback_ = nullptr;
    std::shared_ptr<VDecCallbackTestExt> vdecCallbackExt_ = nullptr;
};

bool HdrVivid2SdrBaseSuit::CreateVideoCodecByName(const std::string &decName)
{
    if (videoDec_->isAVBufferMode_) {
        if (videoDec_->CreateVideoDecMockByName(decName) == false ||
            videoDec_->SetCallback(vdecCallbackExt_) != AV_ERR_OK) {
            return false;
        }
    } else {
        if (videoDec_->CreateVideoDecMockByName(decName) == false ||
            videoDec_->SetCallback(vdecCallback_) != AV_ERR_OK) {
            return false;
        }
    }
    return true;
}

void HdrVivid2SdrBaseSuit::SetFormatWithParam(VideoPixelFormat format)
{
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(format));
}

void ISHDR2SDRSupported()
{
    VideoProcessing_ColorSpaceInfo sourceVideoInfo = {-1, -1, -1};
    VideoProcessing_ColorSpaceInfo destinationVideoInfo = {-1, -1, -1};
    sourceVideoInfo.metadataType = static_cast<int32_t>(OH_VIDEO_HDR_VIVID);
    sourceVideoInfo.colorSpace = static_cast<int32_t>(OH_COLORSPACE_BT2020_HLG_FULL);
    sourceVideoInfo.pixelFormat = static_cast<int32_t>(NATIVEBUFFER_PIXEL_FMT_RGBA_1010102);
    destinationVideoInfo.metadataType = static_cast<int32_t>(OH_VIDEO_NONE);
    destinationVideoInfo.colorSpace = static_cast<int32_t>(OH_COLORSPACE_BT709_LIMIT);
    destinationVideoInfo.pixelFormat = static_cast<int32_t>(NATIVEBUFFER_PIXEL_FMT_RGBA_8888);
    bool ret = OH_VideoProcessing_IsColorSpaceConversionSupported(&sourceVideoInfo, &destinationVideoInfo);
    if (!ret) {
        GTEST_SKIP() << "Unsupport func of HDR2SDR";
    }
}

void HdrVivid2SdrBaseSuit::IsPixelFormatSupported(OHOS::MediaAVCodec::VideoPixelFormat pixelFormat)
{
    auto pixelFormats = capability_->GetVideoSupportedPixelFormats();
    if (std::find(pixelFormats.begin(), pixelFormats.end(), static_cast<int32_t>(pixelFormat)) == pixelFormats.end()) {
        GTEST_SKIP() << "Unsupport pixel format = " << static_cast<int32_t>(pixelFormat);
    }
}
} // namespace TESTBASE
#endif // VIDEODEC_HDRVIVID2SDR_UNIT_TEST_H