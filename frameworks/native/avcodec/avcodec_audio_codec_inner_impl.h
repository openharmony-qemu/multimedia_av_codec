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
#ifndef AVCODEC_AUDIO_CODEC_INNER_IMPL_H
#define AVCODEC_AUDIO_CODEC_INNER_IMPL_H

#include "avcodec_audio_codec.h"
#include "nocopyable.h"
#include "i_avcodec_service.h"
#include "drm_i_keysession_service.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecAudioCodecInnerImpl : public AVCodecAudioCodec, public NoCopyable {
public:
    AVCodecAudioCodecInnerImpl();
    ~AVCodecAudioCodecInnerImpl();

    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name);

    int32_t Configure(const std::shared_ptr<Media::Meta> &meta) override;

    int32_t SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer) override;

    int32_t Prepare() override;

    sptr<Media::AVBufferQueueProducer> GetInputBufferQueue() override;

    sptr<Media::AVBufferQueueConsumer> GetInputBufferQueueConsumer() override;

    sptr<Media::AVBufferQueueProducer> GetOutputBufferQueueProducer() override;

    void ProcessInputBufferInner(bool isTriggeredByOutPort, bool isFlushed, uint32_t &bufferStatus) override;

    int32_t Start() override;

    int32_t Stop() override;

    int32_t Flush() override;

    int32_t Reset() override;

    int32_t Release() override;

    int32_t NotifyEos() override;

    int32_t SetParameter(const std::shared_ptr<Media::Meta> &parameter) override;

    int32_t GetOutputFormat(std::shared_ptr<Media::Meta> &parameter) override;

    int32_t ChangePlugin(const std::string &mime, bool isEncoder, const std::shared_ptr<Media::Meta> &meta) override;

    int32_t SetCodecCallback(const std::shared_ptr<MediaCodecCallback> &codecCallback) override;

    int32_t SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag) override;

    void ProcessInputBuffer() override;

    void SetDumpInfo(bool isDump, uint64_t instanceId) override;

private:
    std::shared_ptr<ICodecService> codecService_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_AUDIO_CODEC_INNER_IMPL_H