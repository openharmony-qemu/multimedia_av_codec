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
    FAULT_TYPE_CRASH,
    FAULT_TYPE_INNER_ERROR,
    FAULT_TYPE_END,
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

struct DemuxerFaultInfo {
    std::string appName;
    std::string instanceId;
    std::string callerType;
    int8_t sourceType;
    std::string containerFormat;
    std::string streamType;
    std::string errMsg;
};

struct MuxerFaultInfo {
    std::string appName;
    std::string instanceId;
    std::string callerType;
    std::string videoCodec;
    std::string audioCodec;
    std::string containerFormat;
    std::string errMsg;
};

struct AudioCodecFaultInfo {
    std::string appName;
    std::string instanceId;
    std::string callerType;
    std::string audioCodec;
    std::string errMsg;
};

struct VideoCodecFaultInfo {
    std::string appName;
    std::string instanceId;
    std::string callerType;
    std::string videoCodec;
    std::string errMsg;
};

struct AudioSourceFaultInfo {
    std::string appName;
    std::string instanceId;
    int32_t audioSourceType;
    std::string errMsg;
};

__attribute__((visibility("default"))) void FaultEventWrite(FaultType faultType, const std::string& msg,
                                                            const std::string& module);
__attribute__((visibility("default"))) void ServiceStartEventWrite(uint32_t useTime, const std::string& module);
__attribute__((visibility("default"))) void CodecStartEventWrite(CodecDfxInfo& codecDfxInfo);
__attribute__((visibility("default"))) void CodecStopEventWrite(uint32_t clientPid, uint32_t clientUid,
                                                                uint32_t codecInstanceId);
__attribute__((visibility("default"))) void FaultDemuxerEventWrite(DemuxerFaultInfo& demuxerFaultInfo);
__attribute__((visibility("default"))) void FaultAudioCodecEventWrite(AudioCodecFaultInfo& audioCodecFaultInfo);
__attribute__((visibility("default"))) void FaultVideoCodecEventWrite(VideoCodecFaultInfo& videoCodecFaultInfo);
__attribute__((visibility("default"))) void FaultMuxerEventWrite(MuxerFaultInfo& muxerFaultInfo);
__attribute__((visibility("default"))) void FaultRecordAudioEventWrite(AudioSourceFaultInfo& audioSourceFaultInfo);
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_SYSEVENT_H