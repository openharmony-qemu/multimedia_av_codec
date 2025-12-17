/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef I_CODEC_SERVICE_H
#define I_CODEC_SERVICE_H

#include <string>
#include "avcodec_log_ex.h"
#include "avcodec_common.h"
#include "avcodec_info.h"
#include "buffer/avsharedmemory.h"
#include "refbase.h"
#include "surface.h"
#include "meta/meta.h"
#include "meta/format.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "buffer/avbuffer_queue_define.h"
#include "buffer/avbuffer_queue_producer.h"
#include "drm_i_keysession_service.h"

namespace OHOS {
namespace MediaAVCodec {
class ICodecService : public AVCodecDfxComponent {
public:
    virtual ~ICodecService() = default;

    virtual int32_t Init(AVCodecType type, bool isMimeType, const std::string &name, Media::Meta &callerInfo) = 0;
    virtual int32_t Configure(const Format &format) = 0;
    virtual int32_t SetCustomBuffer(std::shared_ptr<AVBuffer> buffer) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t GetChannelId(int32_t &channelId) = 0;
    virtual int32_t NotifyEos() = 0;
    virtual sptr<Surface> CreateInputSurface() = 0;
    virtual int32_t SetOutputSurface(sptr<Surface> surface) = 0;
    virtual int32_t SetLowPowerPlayerMode(bool isLpp) = 0;
    virtual int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) = 0;
    virtual int32_t QueueInputBuffer(uint32_t index) = 0;
    virtual int32_t QueueInputParameter(uint32_t index) = 0;
    virtual int32_t GetOutputFormat(Format &format) = 0;
    virtual int32_t ReleaseOutputBuffer(uint32_t index, bool render = false) = 0;
    virtual int32_t RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs) = 0;
    virtual int32_t SetParameter(const Format &format) = 0;
    virtual int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) = 0;
    virtual int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) = 0;
    virtual int32_t SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback) = 0;
    virtual int32_t SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback) = 0;

    virtual int32_t QueryInputBuffer(uint32_t &index, int64_t timeoutUs)
    {
        (void)index;
        (void)timeoutUs;
        return AVCODEC_ERROR_EXTEND_START;
    }

    virtual int32_t QueryOutputBuffer(uint32_t &index, int64_t timeoutUs)
    {
        (void)index;
        (void)timeoutUs;
        return AVCODEC_ERROR_EXTEND_START;
    }

    virtual std::shared_ptr<AVBuffer> GetInputBuffer(uint32_t index)
    {
        (void)index;
        return nullptr;
    }

    virtual std::shared_ptr<AVBuffer> GetOutputBuffer(uint32_t index)
    {
        (void)index;
        return nullptr;
    }

    virtual int32_t ChangePlugin(const std::string &mime, bool isEncoder, const std::shared_ptr<Media::Meta> &meta)
    {
        (void)mime;
        (void)isEncoder;
        (void)meta;
        return AVCODEC_ERROR_EXTEND_START;
    }

    virtual int32_t SetCodecCallback(const std::shared_ptr<MediaCodecCallback> &codecCallback)
    {
        (void)codecCallback;
        return AVCODEC_ERROR_EXTEND_START;
    }

    virtual void SetDumpInfo(bool isDump, uint64_t instanceId)
    {
        (void)isDump;
        (void)instanceId;
    }

    virtual int32_t GetInputFormat(Format &format) = 0;
    virtual int32_t GetCodecInfo(Format &format) = 0;
    virtual int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag)
    {
        (void)keySession;
        (void)svpFlag;
        return AVCODEC_ERROR_EXTEND_START;
    }

    /* API11 audio codec interface */
    virtual int32_t CreateCodecByName(const std::string &name)
    {
        (void)name;
        return AVCODEC_ERROR_EXTEND_START;
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
    virtual bool CheckRunning()
    {
        return false;
    }

    /* API12 audio codec interface for drm */
    virtual int32_t SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag)
    {
        (void)keySession;
        (void)svpFlag;
        return AVCODEC_ERROR_EXTEND_START;
    }

    virtual sptr<Media::AVBufferQueueConsumer> GetInputBufferQueueConsumer()
    {
        return nullptr;
    }

    virtual sptr<Media::AVBufferQueueProducer> GetOutputBufferQueueProducer()
    {
        return nullptr;
    }

    virtual void ProcessInputBufferInner(bool isTriggeredByOutPort, bool isFlushed, uint32_t &bufferStatus)
    {
        (void)isTriggeredByOutPort;
        (void)isFlushed;
        (void)bufferStatus;
    }

    virtual int32_t NotifyMemoryExchange(const bool exchangeFlag) = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // I_CODEC_SERVICE_H