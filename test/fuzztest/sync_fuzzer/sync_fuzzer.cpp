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

bool VideoEncoderCapiFuzzTest(FuzzedDataProvider *fdp, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    uint32_t index = fdp->ConsumeIntegral<uint32_t>();
    bool flag1 = fdp->ConsumeBool();
    int64_t timeout1 = flag1 ? 1 : 0;
    bool flag2 = fdp->ConsumeBool();
    int64_t timeout2 = flag2 ? 1 : 0;
    OH_AVCodec *codec = nullptr;
    OH_VideoEncoder_QueryInputBuffer(codec, &index, timeout1);
    OH_VideoEncoder_QueryOutputBuffer(codec, &index, timeout2);
    OH_VideoEncoder_GetInputBuffer(codec, index);
    OH_VideoEncoder_GetOutputBuffer(codec, index);

    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    if (videoEnc == nullptr) {
        return false;
    }
    bool flag3 = fdp->ConsumeBool();
    int64_t timeout3 = flag3 ? 1 : 0;
    bool flag4 = fdp->ConsumeBool();
    int64_t timeout4 = flag4 ? 1 : 0;    
    OH_VideoEncoder_QueryInputBuffer(videoEnc, &index, timeout3);
    OH_VideoEncoder_QueryOutputBuffer(videoEnc, &index, timeout4);
    OH_VideoEncoder_GetInputBuffer(videoEnc, index);
    OH_VideoEncoder_GetOutputBuffer(videoEnc, index);

    int32_t intData1 = fdp->ConsumeIntegral<int32_t>();
    int32_t intData2 = fdp->ConsumeIntegral<int32_t>();
    int32_t intData3 = fdp->ConsumeIntegral<int32_t>();
    int32_t intData4 = fdp->ConsumeIntegral<int32_t>();
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, intData1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, intData2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, intData3);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, intData4);
    OH_VideoEncoder_Configure(videoEnc, format);
    OH_AVFormat_Destroy(format);
    OH_VideoEncoder_Destroy(videoEnc);
    format = nullptr;
    videoEnc = nullptr;
    return true;
}

bool VideoDecoderCapiFuzzTest(FuzzedDataProvider *fdp, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    uint32_t index = fdp->ConsumeIntegral<uint32_t>();
    bool flag1 = fdp->ConsumeBool();
    int64_t timeout1 = flag1 ? 1 : 0;
    bool flag2 = fdp->ConsumeBool();
    int64_t timeout2 = flag2 ? 1 : 0;
    OH_AVCodec *codec = nullptr;
    OH_VideoDecoder_QueryInputBuffer(codec, &index, timeout1);
    OH_VideoDecoder_QueryOutputBuffer(codec, &index, timeout2);
    OH_VideoDecoder_GetInputBuffer(codec, index);
    OH_VideoDecoder_GetOutputBuffer(codec, index);

    OH_AVCodec *videoDec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    if (videoDec == nullptr) {
        return false;
    }
    bool flag3 = fdp->ConsumeBool();
    int64_t timeout3 = flag3 ? 1 : 0;
    bool flag4 = fdp->ConsumeBool();
    int64_t timeout4 = flag4 ? 1 : 0;
    OH_VideoDecoder_QueryInputBuffer(videoDec, &index, timeout3);
    OH_VideoDecoder_QueryOutputBuffer(videoDec, &index, timeout4);
    OH_VideoDecoder_GetInputBuffer(videoDec, index);
    OH_VideoDecoder_GetOutputBuffer(videoDec, index);

    int32_t intData1 = fdp->ConsumeIntegral<int32_t>();
    int32_t intData2 = fdp->ConsumeIntegral<int32_t>();
    int32_t intData3 = fdp->ConsumeIntegral<int32_t>();
    int32_t intData4 = fdp->ConsumeIntegral<int32_t>();
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, intData1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, intData2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, intData3);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, intData4);
    OH_VideoEncoder_Configure(videoDec, format);
    OH_AVFormat_Destroy(format);
    OH_VideoEncoder_Destroy(videoDec);
    format = nullptr;
    videoDec = nullptr;
    return true;
}

bool VideoEncoderInnerFuzzTest(FuzzedDataProvider *fdp, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    uint32_t index = fdp->ConsumeIntegral<uint32_t>();
    bool flag1 = fdp->ConsumeBool();
    int64_t timeout1 = flag1 ? 1 : 0;
    bool flag2 = fdp->ConsumeBool();
    int64_t timeout2 = flag2 ? 1 : 0;
    AVCodecVideoEncoderImpl encoderImpl;
    encoderImpl.QueryInputBuffer(index, timeout1);
    encoderImpl.QueryOutputBuffer(index, timeout2);
    encoderImpl.GetOutputBuffer(index);
    encoderImpl.GetInputBuffer(index);

    std::shared_ptr<AVCodecVideoEncoder> videoEnc =
        VideoEncoderFactory::CreateByMime((CodecMimeType::VIDEO_AVC).data());
    if (videoEnc == nullptr) {
        return false;
    }
    int32_t intData1 = fdp->ConsumeIntegral<int32_t>();
    int32_t intData2 = fdp->ConsumeIntegral<int32_t>();
    int32_t intData3 = fdp->ConsumeIntegral<int32_t>();
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, intData1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, intData2);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(OH_MD_KEY_ENABLE_SYNC_MODE, intData3);
    videoEnc->Configure(format);
    return true;
}

bool VideoDecoderInnerFuzzTest(FuzzedDataProvider *fdp, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    uint32_t index = fdp->ConsumeIntegral<uint32_t>();
    bool flag1 = fdp->ConsumeBool();
    int64_t timeout1 = flag1 ? 1 : 0;
    bool flag2 = fdp->ConsumeBool();
    int64_t timeout2 = flag2 ? 1 : 0;
    AVCodecVideoDecoderImpl decoderImpl;

    decoderImpl.QueryInputBuffer(index, timeout1);
    decoderImpl.QueryOutputBuffer(index, timeout2);
    decoderImpl.GetOutputBuffer(index);
    decoderImpl.GetInputBuffer(index);

    std::shared_ptr<AVCodecVideoDecoder> videoDec =
        VideoDecoderFactory::CreateByMime((CodecMimeType::VIDEO_AVC).data());
    if (videoDec == nullptr) {
        return false;
    }
    int32_t intData1 = fdp->ConsumeIntegral<int32_t>();
    int32_t intData2 = fdp->ConsumeIntegral<int32_t>();
    int32_t intData3 = fdp->ConsumeIntegral<int32_t>();
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, intData1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, intData2);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(OH_MD_KEY_ENABLE_SYNC_MODE, intData3);
    videoDec->Configure(format);
    return true;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    int32_t choose = fdp.ConsumeIntegral<int32_t>() % 4;
    if (choose == 0) {
        VideoEncoderCapiFuzzTest(&fdp, size);
    } else if (choose == 1) {
        VideoDecoderCapiFuzzTest(&fdp, size);
    } else if (choose == 2) {
        VideoEncoderInnerFuzzTest(&fdp, size);
    } else {
        VideoDecoderInnerFuzzTest(&fdp, size);
    }
    return 0;
}
