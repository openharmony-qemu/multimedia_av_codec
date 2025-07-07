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

#ifndef MEDIA_AVCODEC_AUDIO_CODEC_H
#define MEDIA_AVCODEC_AUDIO_CODEC_H

#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "meta/format.h"
#include "meta/meta.h"
#include "buffer/avbuffer_queue_producer.h"
#include "drm_i_keysession_service.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecAudioCodec {
public:
    virtual ~AVCodecAudioCodec() = default;

    virtual int32_t Configure(const std::shared_ptr<Media::Meta> &meta);

    virtual int32_t SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer);

    virtual int32_t Prepare();

    virtual sptr<Media::AVBufferQueueProducer> GetInputBufferQueue();

    virtual int32_t Start();

    virtual int32_t Stop();

    virtual int32_t Flush();

    virtual int32_t Reset();

    virtual int32_t Release();

    virtual int32_t NotifyEos();

    virtual int32_t SetParameter(const std::shared_ptr<Media::Meta> &parameter);

    virtual int32_t GetOutputFormat(std::shared_ptr<Media::Meta> &parameter);

    virtual int32_t ChangePlugin(const std::string &mime, bool isEncoder, const std::shared_ptr<Media::Meta> &meta);

    virtual int32_t SetCodecCallback(const std::shared_ptr<MediaCodecCallback> &codecCallback);

    virtual int32_t SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag);

    virtual void ProcessInputBuffer();

    virtual void SetDumpInfo(bool isDump, uint64_t instanceId);

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

    virtual int32_t QueryInputBuffer(uint32_t *index, int32_t bufferSize, int64_t timeoutUs)
    {
        (void)index;
        (void)bufferSize;
        (void)timeoutUs;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    virtual std::shared_ptr<AVBuffer> GetInputBuffer(uint32_t index)
    {
        (void)index;
        return nullptr;
    }

    virtual std::shared_ptr<AVBuffer> GetOutputBuffer(int64_t timeoutUs)
    {
        (void)timeoutUs;
        return nullptr;
    }

    virtual int32_t PushInputBuffer(uint32_t index, bool available)
    {
        (void)index;
        (void)available;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    virtual int32_t ReleaseOutputBuffer(const std::shared_ptr<AVBuffer> &buffer)
    {
        (void)buffer;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
};

class __attribute__((visibility("default"))) AudioCodecFactory {
public:
#ifdef UNSUPPORT_CODEC
    static std::shared_ptr<AVCodecAudioCodec> CreateByMime(const std::string &mime, bool isEncoder)
    {
        (void)mime;
        return nullptr;
    }

    static std::shared_ptr<AVCodecAudioCodec> CreateByName(const std::string &name)
    {
        (void)name;
        return nullptr;
    }
#else
    /**
     * @brief Instantiate the preferred decoder of the given mime type.
     * @param mime The mime type.
     * @return Returns the preferred decoder.
     * @since 4.1
     */
    static std::shared_ptr<AVCodecAudioCodec> CreateByMime(const std::string &mime, bool isEncoder);

    /**
     * @brief Instantiates the designated decoder.
     * @param name The codec's name.
     * @return Returns the designated decoder.
     * @since 4.1
     */
    static std::shared_ptr<AVCodecAudioCodec> CreateByName(const std::string &name);
#endif
private:
    AudioCodecFactory() = default;
    ~AudioCodecFactory() = default;
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_AUDIO_CODEC_H