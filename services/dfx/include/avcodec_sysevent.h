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

#ifndef AVCODEC_SYSEVENT_H
#define AVCODEC_SYSEVENT_H

#include <string>
#include "nocopyable.h"
#include "hisysevent.h"

namespace OHOS {
namespace MediaAVCodec {
enum class FaultType : int32_t {
    FAULT_TYPE_INVALID = -1,
    FAULT_TYPE_FREEZE = 0,
    FAULT_TYPE_CRASH = 1,
    FAULT_TYPE_INNER_ERROR = 2,
};

struct SubAbilityCount {
    uint32_t codecCount = 0;
    uint32_t muxerCount = 0;
    uint32_t sourceCount = 0;
    uint32_t demuxerCount = 0;
    uint32_t codeclistCount = 0;
};

struct CodecDfxInfo {
    int32_t clientPid;
    int32_t clientUid;
    int32_t codecInstanceId;
    std::string codecName;
    std::string codecIsVendor;
    std::string codecMode;
    int64_t encoderBitRate;
    int32_t videoWidth;
    int32_t videoHeight;
    double videoFrameRate;
    std::string videoPixelFormat;
    int32_t audioChannelCount;
    int32_t audioSampleRate;
};

class __attribute__((visibility("default"))) AVCodecEvent : public NoCopyable {
public:
    AVCodecEvent() = default;
    ~AVCodecEvent() = default;
    bool CreateMsg(const char* format, ...) __attribute__((__format__(printf, 2, 3)));
    void FaultEventWrite(const std::string& eventName,
                         OHOS::HiviewDFX::HiSysEvent::EventType type,
                         FaultType faultType,
                         const std::string& module);

private:
    std::string msg_;
};

__attribute__((visibility("default"))) void FaultEventWrite(FaultType faultType, const std::string& msg,
                                                            const std::string& module);
__attribute__((visibility("default"))) void ServiceStartEventWrite(uint32_t useTime, const std::string& module);
__attribute__((visibility("default"))) void CodecStartEventWrite(CodecDfxInfo& codecDfxInfo);
__attribute__((visibility("default"))) void CodecStopEventWrite(uint32_t clientPid, uint32_t clientUid,
                                                                uint32_t codecInstanceId);
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_SYSEVENT_H