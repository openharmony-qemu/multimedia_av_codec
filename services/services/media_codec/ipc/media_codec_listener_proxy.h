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

#ifndef MEDIA_CODEC_LISTENER_PROXY_H
#define MEDIA_CODEC_LISTENER_PROXY_H

#include "i_standard_media_codec_listener.h"
#include "avcodec_death_recipient.h"
#include "nocopyable.h"
#include "avbuffer.h"

namespace OHOS {
namespace MediaAVCodec {
class MediaCodecListenerCallback : public VideoCodecCallback, public NoCopyable {
public:
    explicit MediaCodecListenerCallback(const sptr<IStandardMediaCodecListener> &listener);
    virtual ~MediaCodecListenerCallback();

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnStreamChanged(const Format &format) override;
    void SurfaceModeOnBufferFilled(std::shared_ptr<Media::AVBuffer> buffer) override;
    bool FindBufferFromIndex(uint64_t index, std::shared_ptr<Media::AVBuffer> buffer);
private:
    sptr<IStandardMediaCodecListener> listener_ = nullptr;
};

class MediaCodecListenerProxy : public IRemoteProxy<IStandardMediaCodecListener>, public NoCopyable {
public:
    explicit MediaCodecListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~MediaCodecListenerProxy();

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnStreamChanged(const Format &format) override;
    void SurfaceModeOnBufferFilled(std::shared_ptr<Media::AVBuffer> buffer) override;
    bool FindBufferFromIndex(uint64_t index, std::shared_ptr<Media::AVBuffer> buffer) override;

private:
    static inline BrokerDelegator<MediaCodecListenerProxy> delegator_;
    class MediaCodecBufferCache;
    std::unique_ptr<MediaCodecBufferCache> outputBufferCache_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_CODEC_LISTENER_PROXY_H
