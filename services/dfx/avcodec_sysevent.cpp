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

#include <avcodec_sysevent.h>
#include <unistd.h>
#include <unordered_map>
#include "securec.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "dump_usage.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecDFX"};
constexpr char HISYSEVENT_DOMAIN_AVCODEC[] = "AV_CODEC";

const std::unordered_map<OHOS::MediaAVCodec::FaultType, std::string> FAULT_TYPE_TO_STRING = {
    {OHOS::MediaAVCodec::FaultType::FAULT_TYPE_FREEZE,          "Freeze"},
    {OHOS::MediaAVCodec::FaultType::FAULT_TYPE_CRASH,           "Crash"},
    {OHOS::MediaAVCodec::FaultType::FAULT_TYPE_INNER_ERROR,     "Inner error"},
};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
void FaultEventWrite(FaultType faultType, const std::string& msg, const std::string& module)
{
    CHECK_AND_RETURN_LOG(faultType >= FaultType::FAULT_TYPE_FREEZE && faultType < FaultType::FAULT_TYPE_END,
        "Invalid fault type: %{public}d", faultType);
    HiSysEventWrite(HISYSEVENT_DOMAIN_AVCODEC, "FAULT",
                    OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
                    "MODULE", module,
                    "FAULTTYPE", FAULT_TYPE_TO_STRING.at(faultType),
                    "MSG", msg);
}

void ServiceStartEventWrite(uint32_t useTime, const std::string& module)
{
    OHOS::HiviewDFX::DumpUsage dumpUse;
    uint64_t useMemory = dumpUse.GetPss(getprocpid());
    HiSysEventWrite(HISYSEVENT_DOMAIN_AVCODEC, "SERVICE_START_INFO",
                    OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR, "MODULE", module.c_str(), "TIME", useTime,
                    "MEMORY", useMemory);
}

void CodecStartEventWrite(CodecDfxInfo& codecDfxInfo)
{
    HiSysEventWrite(HISYSEVENT_DOMAIN_AVCODEC, "CODEC_START_INFO",
                    OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                    "CLIENT_PID",           codecDfxInfo.clientPid,
                    "CLIENT_UID",           codecDfxInfo.clientUid,
                    "CODEC_INSTANCE_ID",    codecDfxInfo.codecInstanceId,
                    "CODEC_NAME",           codecDfxInfo.codecName,
                    "CODEC_IS_VENDOR",      codecDfxInfo.codecIsVendor,
                    "CODEC_MODE",           codecDfxInfo.codecMode,
                    "ENCODER_BITRATE",      codecDfxInfo.encoderBitRate,
                    "VIDEO_WIDTH",          codecDfxInfo.videoWidth,
                    "VIDEO_HEIGHT",         codecDfxInfo.videoHeight,
                    "VIDEO_FRAMERATE",      codecDfxInfo.videoFrameRate,
                    "VIDEO_PIXEL_FORMAT",   codecDfxInfo.videoPixelFormat,
                    "AUDIO_CHANNEL_COUNT",  codecDfxInfo.audioChannelCount,
                    "AUDIO_SAMPLE_RATE",    codecDfxInfo.audioSampleRate);
}

void CodecStopEventWrite(uint32_t clientPid, uint32_t clientUid, uint32_t codecInstanceId)
{
    HiSysEventWrite(HISYSEVENT_DOMAIN_AVCODEC, "CODEC_STOP_INFO",
                    OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                    "CLIENT_PID", clientPid, "CLIENT_UID", clientUid, "CODEC_INSTANCE_ID", codecInstanceId);
}

void FaultDemuxerEventWrite(DemuxerFaultInfo& demuxerFaultInfo)
{
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, "DEMUXER_FAILURE",
                    OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
                    "APP_NAME",         demuxerFaultInfo.appName,
                    "INSTANCE_ID",      demuxerFaultInfo.instanceId,
                    "CALLER_TYPE",      demuxerFaultInfo.callerType,
                    "SOURCE_TYPE",      demuxerFaultInfo.sourceType,
                    "CONTAINER_FORMAT", demuxerFaultInfo.containerFormat,
                    "STREAM_TYPE",      demuxerFaultInfo.streamType,
                    "ERROR_MESG",       demuxerFaultInfo.errMsg);
}

void FaultAudioCodecEventWrite(AudioCodecFaultInfo& audioCodecFaultInfo)
{
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, "AUDIO_CODEC_FAILURE",
                    OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
                    "APP_NAME",    audioCodecFaultInfo.appName,
                    "INSTANCE_ID", audioCodecFaultInfo.instanceId,
                    "CALLER_TYPE", audioCodecFaultInfo.callerType,
                    "AUDIO_CODEC", audioCodecFaultInfo.audioCodec,
                    "ERROR_MESG",  audioCodecFaultInfo.errMsg);
}

void FaultVideoCodecEventWrite(VideoCodecFaultInfo& videoCodecFaultInfo)
{
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, "VIDEO_CODEC_FAILURE",
                    OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
                    "APP_NAME",    videoCodecFaultInfo.appName,
                    "INSTANCE_ID", videoCodecFaultInfo.instanceId,
                    "CALLER_TYPE", videoCodecFaultInfo.callerType,
                    "VIDEO_CODEC", videoCodecFaultInfo.videoCodec,
                    "ERROR_MESG",  videoCodecFaultInfo.errMsg);
}

void FaultMuxerEventWrite(MuxerFaultInfo& muxerFaultInfo)
{
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, "MUXER_FAILURE",
                    OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
                    "APP_NAME",         muxerFaultInfo.appName,
                    "INSTANCE_ID",      muxerFaultInfo.instanceId,
                    "CALLER_TYPE",      muxerFaultInfo.callerType,
                    "VIDEO_CODEC",      muxerFaultInfo.videoCodec,
                    "AUDIO_CODEC",      muxerFaultInfo.audioCodec,
                    "CONTAINER_FORMAT", muxerFaultInfo.containerFormat,
                    "ERROR_MESG",       muxerFaultInfo.errMsg);
}

void FaultRecordAudioEventWrite(AudioSourceFaultInfo& audioSourceFaultInfo)
{
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, "RECORD_AUDIO_FAILURE",
                    OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
                    "APP_NAME",          audioSourceFaultInfo.appName,
                    "INSTANCE_ID",       audioSourceFaultInfo.instanceId,
                    "AUDIO_SOURCE_TYPE", audioSourceFaultInfo.audioSourceType,
                    "ERROR_MESG",        audioSourceFaultInfo.errMsg);
}
} // namespace MediaAVCodec
} // namespace OHOS
