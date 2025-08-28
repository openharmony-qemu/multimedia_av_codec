/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <cstddef>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include "native_avcapability.h"
#include "venc_sync_sample.h"


using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;

#define FUZZ_PROJECT_NAME "syncresourcecapienc_fuzzer"

std::string sourcePath = "/data/test/media/1280_720_nv.yuv";
uint32_t DEFAULT_WIDTH = 1280;
uint32_t DEFAULT_HEIGHT = 720;
std::shared_ptr<FormatMock> format = nullptr;
std::shared_ptr<VideoEncSyncSample> videoEnc = nullptr;

void SetSync()
{
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format->PutIntValue(Media::Tag::AV_CODEC_ENABLE_SYNC_MODE, 1);
}

bool VideoEncoderFuzzTest(FuzzedDataProvider *fdp)
{
    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    videoEnc = std::make_shared<VideoEncSyncSample>(vencSignal);
    if (videoEnc == nullptr) {
        return false;
    }

    format = OHOS::MediaAVCodec::FormatMockFactory::CreateFormat();
    if (!videoEnc->CreateVideoEncMockByMime(CodecMimeType::VIDEO_AVC.data())) {
        return false;
    }
    SetSync();
    uint32_t rangeFlag = fdp->ConsumeIntegral<uint32_t>();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, rangeFlag);
    videoEnc->Configure(format);
    videoEnc->Prepare();
    videoEnc->Start();

    format = nullptr;
    videoEnc = nullptr;
    return true;
}

bool VideoEncoderResourceFuzzTest(FuzzedDataProvider *fdp)
{
    std::shared_ptr<OHOS::MediaAVCodec::VEncSignal> vencSignal = std::make_shared<OHOS::MediaAVCodec::VEncSignal>();
    videoEnc = std::make_shared<OHOS::MediaAVCodec::VideoEncSyncSample>(vencSignal);
    if (videoEnc == nullptr) {
        return false;
    }

    format = FormatMockFactory::CreateFormat();
    if (!videoEnc->CreateVideoEncMockByMime(CodecMimeType::VIDEO_AVC.data())) {
        return false;
    }
    SetSync();
    videoEnc->Configure(format);
    videoEnc->Prepare();
    videoEnc->FuzzStart();
    auto remaining_data = fdp->ConsumeRemainingBytes<uint8_t>();
    int ret = videoEnc->InputFuncFUZZ(remaining_data.data(), remaining_data.size());
    if (ret != 0) {
        return false;
    }

    videoEnc->Stop();
    format = nullptr;
    videoEnc = nullptr;
    return true;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    bool choose = fdp.ConsumeBool();
    if (choose) {
        VideoEncoderFuzzTest(&fdp);
    } else {
        VideoEncoderResourceFuzzTest(&fdp);
    }
    return 0;
}
