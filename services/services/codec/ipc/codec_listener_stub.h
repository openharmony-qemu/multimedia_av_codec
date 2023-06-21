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

#include "i_standard_codec_listener.h"
#include "avcodec_common.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecBufferCache : public NoCopyable {
public:
    CodecBufferCache() = default;
    ~CodecBufferCache() = default;

    int32_t ReadFromParcel(uint32_t index, MessageParcel &parcel, std::shared_ptr<AVSharedMemory> &memory);

    std::shared_ptr<AVSharedMemory> ReadFromCaches(uint32_t index);
private:
    std::mutex mutex_;
    enum CacheFlag : uint8_t {
        HIT_CACHE = 1,
        UPDATE_CACHE,
        INVALIDATE_CACHE,
    };
    std::unordered_map<uint32_t, std::shared_ptr<AVSharedMemory>> caches_;
};

class CodecListenerStub : public IRemoteStub<IStandardCodecListener> {
public:
    CodecListenerStub();
    virtual ~CodecListenerStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override;
    void SetCallback(const std::shared_ptr<AVCodecCallback> &callback);
    // class CodecBufferCache;
    std::unique_ptr<CodecBufferCache> inputBufferCache_;
    std::unique_ptr<CodecBufferCache> outputBufferCache_;

private:
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_LISTENER_STUB_H
