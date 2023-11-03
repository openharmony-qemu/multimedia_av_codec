/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <list>
#include <mutex>
#include <shared_mutex>
#include "native_avcodec_base.h"
#include "native_avcodec_video_codec.h"
#include "native_avmagic.h"
#include "native_window.h"
#include "avcodec_video_codec.h"
#include "avbuffer.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avcodec_dfx.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeMediaCodec"};
}

using namespace OHOS::MediaAVCodec;
class NativeVideoCodecCallback;

struct VideoCodecObject : public OH_AVCodec {
    explicit VideoCodecObject(const std::shared_ptr<AVCodecMediaCodec> &codec)
        : OH_AVCodec(AVMagic::AVCODEC_MAGIC_VIDEO_CODEC), mediaCodec_(codec)
    {
    }
    ~VideoCodecObject() = default;

    const std::shared_ptr<AVCodecMediaCodec> mediaCodec_;
    std::list<OHOS::sptr<OH_AVBuffer>> bufferObjList_;
    std::shared_ptr<NativeVideoCodecCallback> callback_ = nullptr;
    std::atomic<bool> isFlushing_ = false;
    std::atomic<bool> isFlushed_ = false;
    std::atomic<bool> isStop_ = false;
    std::atomic<bool> isEOS_ = false;
    bool isOutputSurfaceMode_ = false;
    std::atomic<bool> isFirstFrameIn_ = true;
    std::atomic<bool> isFirstFrameOut_ = true;
    std::shared_mutex bufferObjListMutex_;
};

OH_AVCodec *OH_VideoCodec_CreateByMime(const char *mime, bool isEncoder)
{
    return nullptr;
}

OH_AVCodec * OH_VideoCodec_CreateByName(const char * name)
{
return nullptr;
}

OH_AVErrCode OH_VideoCodec_Destroy(OH_AVCodec *codec)
{
    return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_SetCallback(OH_AVCodec *codec, OH_AVCodecCallback callback, void *userData)
{
    return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_SetOutputBufferQueue(OH_AVCodec *codec, OH_AVBufferQueueProducer *bufferQueue)
{
    return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_SetSurface(OH_AVCodec *codec, OHNativeWindow *window)
{
    return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_Configure(OH_AVCodec * codec, OH_AVFormat * format)
{
    return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_Prepare(OH_AVCodec *codec)
{
    return OH_AVErrCode();
}

OH_AVBufferQueueProducer *OH_VideoCodec_SetOutputBufferQueue(OH_AVCodec *codec)
{
    return nullptr;
}

OH_AVErrCode OH_VideoCodec_CreateSurface(OH_AVCodec *codec, OHNativeWindow **surface)
{
    return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_Start(OH_AVCodec * codec)
{
return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_Stop(OH_AVCodec *codec)
{
    return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_Flush(OH_AVCodec *codec)
{
    return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_Reset(OH_AVCodec *codec)
{
    return OH_AVErrCode();
}

OH_AVFormat *OH_VideoCodec_GetOutputDescription(OH_AVCodec *codec)
{
    return nullptr;
}

OH_AVErrCode OH_VideoCodec_SetParameter(OH_AVCodec *codec, OH_AVFormat *format)
{
    return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_NotifyEndOfStream(OH_AVCodec *codec)
{
    return OH_AVErrCode();
}

OH_AVErrCode OH_VideoCodec_SurfaceModeReturnBuffer(OH_AVCodec *codec, OH_AVBuffer *buffer, bool available)
{
    return OH_AVErrCode();
}