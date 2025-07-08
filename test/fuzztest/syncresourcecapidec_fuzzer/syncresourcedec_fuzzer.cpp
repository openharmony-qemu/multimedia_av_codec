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
#include "vdec_sync_sample.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;

#define FUZZ_PROJECT_NAME "syncresourcecapidec_fuzzer"

std::string sourcePath = "/data/test/media/720_1280_25_avcc.h264";
std::string outPath = "/data/test/media/outputTest";
uint32_t DEFAULT_WIDTH = 720;
uint32_t DEFAULT_HEIGHT = 1280;
std::shared_ptr<VideoDecSyncSample> videoDec = nullptr;
std::shared_ptr<FormatMock> format = nullptr;

void SetSync()
{
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format->PutIntValue(Media::Tag::AV_CODEC_ENABLE_SYNC_MODE, 1);
}

bool VideoDecoderFuzzTest(const uint8_t *data, size_t size)
{
    std::shared_ptr<OHOS::MediaAVCodec::VDecSignal> vdecSignal = std::make_shared<OHOS::MediaAVCodec::VDecSignal>();
    videoDec = std::make_shared<OHOS::MediaAVCodec::VideoDecSyncSample>(vdecSignal);
    if (videoDec == nullptr) {
        return false;
    }

    format = FormatMockFactory::CreateFormat();
    if (!videoDec->CreateVideoDecMockByMime(CodecMimeType::VIDEO_AVC.data())) {
        return false;
    }
    SetSync();
    videoDec->SetSource(sourcePath);
    videoDec->SetOutPath(outPath);
    videoDec->Configure(format);
    videoDec->Prepare();
    videoDec->Start();

    format = nullptr;
    videoDec = nullptr;
    return true;
}

bool VideoDecoderResourceFuzzTest(const uint8_t *data, size_t size)
{
    std::shared_ptr<OHOS::MediaAVCodec::VDecSignal> vdecSignal = std::make_shared<OHOS::MediaAVCodec::VDecSignal>();
    videoDec = std::make_shared<OHOS::MediaAVCodec::VideoDecSyncSample>(vdecSignal);
    if (videoDec == nullptr) {
        return false;
    }

    format = FormatMockFactory::CreateFormat();
    if (!videoDec->CreateVideoDecMockByMime(CodecMimeType::VIDEO_AVC.data())) {
        return false;
    }
    SetSync();
    videoDec->SetSource(sourcePath);
    videoDec->SetOutPath(outPath);
    videoDec->Configure(format);
    videoDec->Prepare();
    videoDec->FuzzStart();
    int ret = videoDec->InputFuncFUZZ(data, size);
    if (ret != 0) {
        return false;
    }

    videoDec->Stop();
    format = nullptr;
    videoDec = nullptr;
    return true;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    VideoDecoderFuzzTest(data, size);
    VideoDecoderResourceFuzzTest(data, size);
    return 0;
}
