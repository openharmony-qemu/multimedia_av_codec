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
#include "avcodec_video_decoder_impl.h"
#include "avcodec_video_encoder_impl.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_avcodec_videoencoder.h"

#include <cstddef>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "sync_fuzzer"

bool VideoEncoderCapiFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    uint32_t index = fdp.ConsumeIntegral<uint32_t>();
    bool flag = fdp.ConsumeBool();
    int64_t timeout = flag ? 1 : 0;
    OH_AVCodec *codec = nullptr;
    OH_VideoEncoder_QueryInputBuffer(codec, &index, timeout);
    OH_VideoEncoder_QueryOutputBuffer(codec, &index, timeout);
    OH_VideoEncoder_GetInputBuffer(codec, index);
    OH_VideoEncoder_GetOutputBuffer(codec, index);

    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    if (videoEnc == nullptr) {
        return false;
    }
    OH_VideoEncoder_QueryInputBuffer(videoEnc, &index, timeout);
    OH_VideoEncoder_QueryOutputBuffer(videoEnc, &index, timeout);
    OH_VideoEncoder_GetInputBuffer(videoEnc, index);
    OH_VideoEncoder_GetOutputBuffer(videoEnc, index);

    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, intData);
    OH_VideoEncoder_Configure(videoEnc, format);
    OH_AVFormat_Destroy(format);
    OH_VideoEncoder_Destroy(videoEnc);
    format = nullptr;
    videoEnc = nullptr;
    return true;
}

bool VideoDecoderCapiFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    uint32_t index = fdp.ConsumeIntegral<uint32_t>();
    bool flag = fdp.ConsumeBool();
    int64_t timeout = flag ? 1 : 0;
    OH_AVCodec *codec = nullptr;
    OH_VideoDecoder_QueryInputBuffer(codec, &index, timeout);
    OH_VideoDecoder_QueryOutputBuffer(codec, &index, timeout);
    OH_VideoDecoder_GetInputBuffer(codec, index);
    OH_VideoDecoder_GetOutputBuffer(codec, index);

    OH_AVCodec *videoDec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    if (videoDec == nullptr) {
        return false;
    }
    OH_VideoDecoder_QueryInputBuffer(videoDec, &index, timeout);
    OH_VideoDecoder_QueryOutputBuffer(videoDec, &index, timeout);
    OH_VideoDecoder_GetInputBuffer(videoDec, index);
    OH_VideoDecoder_GetOutputBuffer(videoDec, index);

    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, intData);
    OH_VideoEncoder_Configure(videoDec, format);
    OH_AVFormat_Destroy(format);
    OH_VideoEncoder_Destroy(videoDec);
    format = nullptr;
    videoDec = nullptr;
    return true;
}

bool VideoEncoderInnerFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    uint32_t index = fdp.ConsumeIntegral<uint32_t>();
    bool flag = fdp.ConsumeBool();
    int64_t timeout = flag ? 1 : 0;
    AVCodecVideoEncoderImpl encoderImpl;
    encoderImpl.QueryInputBuffer(index, timeout);
    encoderImpl.QueryOutputBuffer(index, timeout);
    encoderImpl.GetOutputBuffer(index);
    encoderImpl.GetInputBuffer(index);

    std::shared_ptr<AVCodecVideoEncoder> videoEnc =
        VideoEncoderFactory::CreateByMime((CodecMimeType::VIDEO_AVC).data());
    if (videoEnc == nullptr) {
        return false;
    }
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, intData);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, intData);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(OH_MD_KEY_ENABLE_SYNC_MODE, intData);
    videoEnc->Configure(format);
    return true;
}

bool VideoDecoderInnerFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    uint32_t index = fdp.ConsumeIntegral<uint32_t>();
    bool flag = fdp.ConsumeBool();
    int64_t timeout = flag ? 1 : 0;
    AVCodecVideoDecoderImpl decoderImpl;

    decoderImpl.QueryInputBuffer(index, timeout);
    decoderImpl.QueryOutputBuffer(index, timeout);
    decoderImpl.GetOutputBuffer(index);
    decoderImpl.GetInputBuffer(index);

    std::shared_ptr<AVCodecVideoDecoder> videoDec =
        VideoDecoderFactory::CreateByMime((CodecMimeType::VIDEO_AVC).data());
    if (videoDec == nullptr) {
        return false;
    }
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, intData);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, intData);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(OH_MD_KEY_ENABLE_SYNC_MODE, intData);
    videoDec->Configure(format);
    return true;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    VideoEncoderCapiFuzzTest(data, size);
    VideoDecoderCapiFuzzTest(data, size);
    VideoEncoderInnerFuzzTest(data, size);
    VideoDecoderInnerFuzzTest(data, size);
    return 0;
}
