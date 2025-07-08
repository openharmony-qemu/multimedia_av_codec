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
#include "venc_async_sample.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "avcapability_fuzzer"

std::string sourcePath = "/data/test/media/1280_720_nv.yuv";
uint32_t DEFAULT_WIDTH = 1280;
uint32_t DEFAULT_HEIGHT = 720;

std::shared_ptr<VideoEncAsyncSample> videoEnc = nullptr;
std::shared_ptr<FormatMock> format = nullptr;
std::shared_ptr<VEncCallbackTest> vencCallback = nullptr;

void SetAsync()
{
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format->PutIntValue(Media::Tag::AV_CODEC_ENABLE_SYNC_MODE, 0);
}

void EncPrepareSource()
{
    string prefix = "/data/test/media/";
    string fileName = "outputTest.h264";
    videoEnc->SetOutPath(prefix + fileName);
}

bool VideoEncoderFuzzTest(const uint8_t *data, size_t size)
{
    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    vencCallback = std::make_shared<VEncCallbackTest>(vencSignal);
    videoEnc = std::make_shared<VideoEncAsyncSample>(vencSignal);
    if ((vencCallback == nullptr) || (videoEnc == nullptr)) {
        return false;
    }

    format = OHOS::MediaAVCodec::FormatMockFactory::CreateFormat();
    if (!videoEnc->CreateVideoEncMockByMime(CodecMimeType::VIDEO_AVC.data())) {
        return false;
    }
    videoEnc->SetCallback(vencCallback);
    SetAsync();
    videoEnc->Configure(format);
    EncPrepareSource();
    videoEnc->Prepare();
    videoEnc->Start();

    format = nullptr;
    videoEnc = nullptr;
    vencCallback = nullptr;
    return true;
}

bool VideoEncoderResourceFuzzTest(const uint8_t *data, size_t size)
{
    std::shared_ptr<OHOS::MediaAVCodec::VEncSignal> vencSignal = std::make_shared<OHOS::MediaAVCodec::VEncSignal>();
    vencCallback = std::make_shared<OHOS::MediaAVCodec::VEncCallbackTest>(vencSignal);
    videoEnc = std::make_shared<OHOS::MediaAVCodec::VideoEncAsyncSample>(vencSignal);
    if ((vencCallback == nullptr) || (videoEnc == nullptr)) {
        return false;
    }

    format = FormatMockFactory::CreateFormat();
    if (!videoEnc->CreateVideoEncMockByMime(CodecMimeType::VIDEO_AVC.data())) {
        return false;
    }
    videoEnc->SetCallback(vencCallback);
    SetAsync();
    videoEnc->Configure(format);
    EncPrepareSource();
    videoEnc->Prepare();
    videoEnc->FuzzStart();
    int ret = videoEnc->InputFuncFUZZ(data, size);
    if (ret != 0) {
        return false;
    }

    videoEnc->Stop();
    format = nullptr;
    videoEnc = nullptr;
    vencCallback = nullptr;
    return true;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    VideoEncoderFuzzTest(data, size);
    VideoEncoderResourceFuzzTest(data, size);
    return 0;
}
