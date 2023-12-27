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

#ifndef MEDIA_AVCODEC_AUDIO_CODEC_H
#define MEDIA_AVCODEC_AUDIO_CODEC_H

#include "avcodec_common.h"
#include "meta/format.h"
#include "meta/meta.h"
#include "buffer/avbuffer_queue_producer.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecAudioCodec {
public:
    virtual ~AVCodecAudioCodec() = default;

    virtual Media::Status Configure(const std::shared_ptr<Media::Meta> &meta);

    virtual Media::Status SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer);

    virtual Media::Status Prepare();

    virtual sptr<Media::AVBufferQueueProducer> GetInputBufferQueue();

    virtual Media::Status Start();

    virtual Media::Status Stop();

    virtual Media::Status Flush();

    virtual Media::Status Reset();

    virtual Media::Status Release();

    virtual Media::Status NotifyEos();

    virtual Media::Status SetParameter(const std::shared_ptr<Media::Meta> &parameter);

    virtual Media::Status GetOutputFormat(std::shared_ptr<Media::Meta> &parameter);

    virtual void ProcessInputBuffer();
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