/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "native_avcodec_videodecoder.h"
#include "hcodec_log.h"

int main()
{
    OH_AVFormat *fmt = OH_AVFormat_Create();
    if (fmt == nullptr) {
        LOGE("OH_AVFormat_Create failed");
        return -1;
    }
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_WIDTH, 1280); // 1280 width of video
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_HEIGHT, 720); // 720 height of video
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    OH_AVCodec *decoder = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    if (decoder == nullptr) {
        LOGE("OH_VideoDecoder_CreateByMime failed");
        return -1;
    }
    LOGI("OH_VideoDecoder_CreateByMime succ");
    OH_AVErrCode err = OH_VideoDecoder_Configure(decoder, fmt);
    if (err != AV_ERR_OK) {
        LOGE("OH_VideoDecoder_Configure failed");
        OH_VideoDecoder_Destroy(decoder);
        return -1;
    }
    OH_AVCodecAsyncCallback cb;
    err = OH_VideoDecoder_SetCallback(decoder, cb, nullptr);
    if (err != AV_ERR_OK) {
        LOGE("OH_VideoDecoder_SetCallback failed");
        OH_VideoDecoder_Destroy(decoder);
        return -1;
    }
    err = OH_VideoDecoder_Start(decoder);
    if (err != AV_ERR_OK) {
        LOGE("OH_VideoDecoder_Start failed");
        OH_VideoDecoder_Destroy(decoder);
        return -1;
    }
    OH_VideoDecoder_Destroy(decoder);
    return 0;
}