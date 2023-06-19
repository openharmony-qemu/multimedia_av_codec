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

#ifndef CODEC_LISTENER_PROXY_H
#define CODEC_LISTENER_PROXY_H

#include "i_standard_codec_listener.h"
#include "avcodec_death_recipient.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class CodecListenerCallback : public AVCodecCallback, public NoCopyable {
public:
    explicit CodecListenerCallback(const sptr<IStandardCodecListener> &listener);
    virtual ~CodecListenerCallback();

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer);
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer);

private:
    sptr<IStandardCodecListener> listener_ = nullptr;
};

class CodecListenerProxy : public IRemoteProxy<IStandardCodecListener>, public NoCopyable {
public:
    explicit CodecListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~CodecListenerProxy();

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override;

private:
    static inline BrokerDelegator<CodecListenerProxy> delegator_;
    class CodecBufferCache;
    std::unique_ptr<CodecBufferCache> inputBufferCache_;
    std::unique_ptr<CodecBufferCache> outputBufferCache_;
};
} // namespace Media
} // namespace OHOS
#endif // CODEC_LISTENER_PROXY_H
