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
#include "avcodec_log.h"
#include "meta/meta_key.h"
#include "native_avcodec_base.h"
#include "native_avmagic.h"
#include "unittest_utils.h"
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
    void SetNV12Format();
    void SetNV21Format();
    void SetRGBAFormat();
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

void HdrVivid2SdrBaseSuit::SetNV12Format()
{
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
}

void HdrVivid2SdrBaseSuit::SetNV21Format()
{
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV21));
}

void HdrVivid2SdrBaseSuit::SetRGBAFormat()
{
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
}
} // TESTBASE
#endif //VIDEODEC_HDRVIVID2SDR_UNIT_TEST_H