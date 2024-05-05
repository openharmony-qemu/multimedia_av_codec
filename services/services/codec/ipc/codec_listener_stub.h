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

#ifndef CODEC_LISTENER_STUB_H
#define CODEC_LISTENER_STUB_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include "avcodec_common.h"
#include "buffer_converter.h"
#include "i_standard_codec_listener.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecListenerStub : public IRemoteStub<IStandardCodecListener> {
public:
    CodecListenerStub();
    virtual ~CodecListenerStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;

    void SetCallback(const std::shared_ptr<AVCodecCallback> &callback);
    void SetCallback(const std::shared_ptr<MediaCodecCallback> &callback);
    void SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback);

    void ClearListenerCache();
    bool WriteInputParameterToParcel(uint32_t index, MessageParcel &data);
    bool WriteInputBufferToParcel(uint32_t index, MessageParcel &data);
    bool WriteInputMemoryToParcel(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag, MessageParcel &data);

    void SetMutex(std::shared_ptr<std::recursive_mutex> &mutex);
    void SetConverter(std::shared_ptr<BufferConverter> &converter);
    void SetNeedListen(const bool needListen);

private:
    void OnInputBufferAvailable(uint32_t index, MessageParcel &data);
    void OnOutputBufferAvailable(uint32_t index, MessageParcel &data);
    bool CheckGeneration(uint64_t messageGeneration) const;

    class CodecBufferCache;
    std::unique_ptr<CodecBufferCache> inputBufferCache_;
    std::unique_ptr<CodecBufferCache> outputBufferCache_;
    std::weak_ptr<AVCodecCallback> callback_;
    std::weak_ptr<MediaCodecCallback> videoCallback_;
    std::weak_ptr<MediaCodecParameterCallback> paramCallback_;
    bool needListen_ = false;
    std::shared_ptr<std::recursive_mutex> syncMutex_;
    std::shared_ptr<BufferConverter> converter_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_LISTENER_STUB_H
