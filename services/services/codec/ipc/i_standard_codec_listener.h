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

#ifndef I_STANDARD_CODEC_LISTENER_H
#define I_STANDARD_CODEC_LISTENER_H

#include <atomic>
#include <limits>
#include "av_codec_service_ipc_interface_code.h"
#include "avcodec_common.h"
#include "buffer/avbuffer.h"
#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"

namespace OHOS {
namespace MediaAVCodec {
class IStandardCodecListener : public IRemoteBroker {
public:
    virtual ~IStandardCodecListener() = default;
    virtual void OnError(AVCodecErrorType errorType, int32_t errorCode) = 0;
    virtual void OnOutputFormatChanged(const Format &format) = 0;
    virtual void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) = 0;
    virtual void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) = 0;

    uint64_t UpdateGeneration()
    {
        return ++generation_;
    }

    uint64_t RestoreGeneration()
    {
        uint64_t expected = std::numeric_limits<uint64_t>::max();
        uint64_t desired = generation_--;
        return generation_.compare_exchange_strong(expected, desired) ? desired : expected;
    }

    uint64_t GetGeneration() const
    {
        return generation_.load();
    }

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardCodecListener");

private:
    std::atomic<uint64_t> generation_ { 0 };
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // I_STANDARD_CODEC_LISTENER_H
