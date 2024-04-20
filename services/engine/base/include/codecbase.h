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
#ifndef CODECBASE_H
#define CODECBASE_H

#include <string>
#include "avcodec_common.h"
#include "buffer/avsharedmemorybase.h"
#include "surface.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "buffer/avbuffer_queue_define.h"
#include "buffer/avbuffer_queue_producer.h"
#include "foundation/multimedia/drm_framework/services/drm_service/ipc/i_keysession_service.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecBase {
public:
    CodecBase() = default;
    virtual ~CodecBase() = default;
    virtual int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback);
    virtual int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback);
    virtual int32_t Configure(const Format &format) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t SetParameter(const Format& format) = 0;
    virtual int32_t GetOutputFormat(Format &format) = 0;
    virtual int32_t QueueInputBuffer(uint32_t index, const AVCodecBufferInfo &info, AVCodecBufferFlag flag);
    virtual int32_t QueueInputBuffer(uint32_t index);
    virtual int32_t ReleaseOutputBuffer(uint32_t index) = 0;

    virtual int32_t NotifyEos();
    virtual sptr<Surface> CreateInputSurface();
    virtual int32_t SetInputSurface(sptr<Surface> surface);
    virtual int32_t SetOutputSurface(sptr<Surface> surface);
    virtual int32_t RenderOutputBuffer(uint32_t index);
    virtual int32_t SignalRequestIDRFrame();
    virtual int32_t GetInputFormat(Format& format);

    virtual std::string GetHidumperInfo()
    {
        return "";
    }

    /* API11 audio codec interface */
    virtual int32_t CreateCodecByName(const std::string &name)
    {
        (void)name;
        return AVCODEC_ERROR_EXTEND_START;
    }

    virtual int32_t Init(Media::Meta &callerInfo)
    {
        (void)callerInfo;
        return 0;
    }

    virtual int32_t Configure(const std::shared_ptr<Media::Meta> &meta)
    {
        (void)meta;
        return AVCODEC_ERROR_EXTEND_START;
    }

    virtual int32_t SetParameter(const std::shared_ptr<Media::Meta> &parameter)
    {
        (void)parameter;
        return AVCODEC_ERROR_EXTEND_START;
    }

    virtual int32_t GetOutputFormat(std::shared_ptr<Media::Meta> &parameter)
    {
        (void)parameter;
        return AVCODEC_ERROR_EXTEND_START;
    }

    virtual int32_t SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer)
    {
        (void)bufferQueueProducer;
        return AVCODEC_ERROR_EXTEND_START;
    }

    virtual int32_t Prepare()
    {
        return AVCODEC_ERROR_EXTEND_START;
    }

    virtual sptr<Media::AVBufferQueueProducer> GetInputBufferQueue()
    {
        return nullptr;
    }

    virtual void ProcessInputBuffer()
    {
        return;
    }

    virtual int32_t SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag)
    {
        (void)keySession;
        (void)svpFlag;
        return 0;
    }
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODECBASE_H