/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "avcodec_server_manager.h"
#include <sstream>
#include <iomanip>
#include "avcodec_errors.h"
#include "avcodec_dump_utils.h"
#include "media_description.h"
#include "meta/format.h"
#include "avcodec_log_ex.h"
#include "codec_service_stub.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecServerDump"};
constexpr uint32_t DUMP_OFFSET_8 = 8;
enum DumpIndex : uint32_t {
    DUMP_INDEX_FORWARD_CALLER_PID          = 0x01'01'01'00,
    DUMP_INDEX_FORWARD_CALLER_UID          = 0x01'01'02'00,
    DUMP_INDEX_FORWARD_CALLER_PROCESS_NAME = 0x01'01'03'00,
    DUMP_INDEX_CALLER_PID                  = 0x01'01'04'00,
    DUMP_INDEX_CALLER_UID                  = 0x01'01'05'00,
    DUMP_INDEX_CALLER_PROCESS_NAME         = 0x01'01'06'00,
    DUMP_INDEX_INSTANCE_ID                 = 0x01'01'07'00,
    DUMP_INDEX_STATUS                      = 0x01'01'08'00,
    DUMP_INDEX_LAST_ERROR                  = 0x01'01'09'00,
    DUMP_INDEX_CREATE_TIME                 = 0x01'01'0A'00,
    DUMP_INDEX_CODEC_INFO_START            = 0x01'02'00'00,
};

const std::map<int32_t, const std::string> PIXEL_FORMAT_STRING_MAP = {
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::YUV420P), "YUV420P"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::YUVI420), "YUVI420"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12), "NV12"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV21), "NV21"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::SURFACE_FORMAT), "SURFACE_FORMAT"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::RGBA), "RGBA"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::UNKNOWN), "UNKNOWN_FORMAT"},
};

const std::map<int32_t, const std::string> SCALE_TYPE_STRING_MAP = {
    {OHOS::ScalingMode::SCALING_MODE_FREEZE, "Freeze"},
    {OHOS::ScalingMode::SCALING_MODE_SCALE_TO_WINDOW, "Scale to window"},
    {OHOS::ScalingMode::SCALING_MODE_SCALE_CROP, "Scale crop"},
    {OHOS::ScalingMode::SCALING_MODE_NO_SCALE_CROP, "No scale crop"},
};

const std::vector<std::pair<std::string_view, const std::string>> VIDEO_DUMP_TABLE = {
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_NAME, "CodecName"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_WIDTH, "Width"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_HEIGHT, "Height"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_FRAME_RATE, "FrameRate"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, "BitRate"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, "PixelFormat"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_SCALE_TYPE, "ScaleType"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, "RotationAngle"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, "MaxInputSize"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, "MaxInputBufferCount"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, "MaxOutputBufferCount"},
};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
std::string GetValueFromFormat(const Format &format, const std::string_view &key)
{
    std::string value;
    bool ret = false;
    ret = AVCodecDumpControler::GetValueFromFormat(format, key, value);
    if (ret == false) {
        return "";
    } else {
        return value;
    }
}

static std::string GetTimeString(std::time_t time)
{
    std::stringstream ss;
    struct tm timeTm;
    ss << std::put_time(localtime_r(&time, &timeTm), "%F %T");
    return ss.str();
}

int32_t DumpInfo(int32_t fd, sptr<IRemoteObject> remoteObject, InstanceInfo instanceInfo, int32_t instanceIndex)
{
    std::string instanceStr = std::string("    Instance_") + std::to_string(instanceIndex) + "_Info\n";
    write(fd, instanceStr.data(), instanceStr.size());

    Format codecFormat;
    auto codecStub = reinterpret_cast<CodecServiceStub*>(remoteObject.GetRefPtr());
    CHECK_AND_RETURN_RET_LOG(codecStub != nullptr, AVCS_ERR_INVALID_OPERATION,
                             "Get a null CodecServiceStub");
    int32_t ret = codecStub->GetOutputFormat(codecFormat);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Get codec format failed");
    std::string statusStr = "";
    std::string lastErrorStr = "";
    std::string codecBaseHiDumperInfo = "";
    codecStub->GetHiDumperInfo(statusStr, lastErrorStr, codecBaseHiDumperInfo);

    AVCodecDumpControler dumpControler;
    if (instanceInfo.forwardCaller.pid != -1 || !instanceInfo.forwardCaller.processName.empty()) {
        dumpControler.AddInfo(DUMP_INDEX_FORWARD_CALLER_PID,
                              "ForwardCallerPid", std::to_string(instanceInfo.forwardCaller.pid));
        dumpControler.AddInfo(DUMP_INDEX_FORWARD_CALLER_PROCESS_NAME,
                              "ForwardCallerProcessName", instanceInfo.forwardCaller.processName);
    }

    dumpControler.AddInfo(DUMP_INDEX_CALLER_PID, "CallerPid", std::to_string(instanceInfo.caller.pid));
    dumpControler.AddInfo(DUMP_INDEX_CALLER_PROCESS_NAME, "CallerProcessName", instanceInfo.caller.processName);
    dumpControler.AddInfo(DUMP_INDEX_INSTANCE_ID, "InstanceId", std::to_string(instanceInfo.instanceId));
    dumpControler.AddInfo(DUMP_INDEX_CREATE_TIME, "CreateTime", GetTimeString(instanceInfo.codecCreateTime).c_str());

    dumpControler.AddInfo(DUMP_INDEX_STATUS, "Status", statusStr);
    dumpControler.AddInfo(DUMP_INDEX_LAST_ERROR, "LastError", lastErrorStr);

    uint32_t infoIndex = 1;
    for (auto iter : VIDEO_DUMP_TABLE) {
        uint32_t dumpIndex = DUMP_INDEX_CODEC_INFO_START + (infoIndex++ << DUMP_OFFSET_8);
        if (iter.first == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT) {
            dumpControler.AddInfoFromFormatWithMapping(dumpIndex, codecFormat,
                                                       iter.first, iter.second, PIXEL_FORMAT_STRING_MAP);
        } else if (iter.first == MediaDescriptionKey::MD_KEY_SCALE_TYPE) {
            dumpControler.AddInfoFromFormatWithMapping(dumpIndex, codecFormat,
                                                       iter.first, iter.second, SCALE_TYPE_STRING_MAP);
        } else {
            dumpControler.AddInfoFromFormat(dumpIndex, codecFormat, iter.first, iter.second);
        }
    }
    std::string dumpString;
    dumpControler.GetDumpString(dumpString);
    dumpString += codecBaseHiDumperInfo;
    write(fd, dumpString.c_str(), dumpString.size());
    return AVCS_ERR_OK;
}

int32_t SimplifiedDump(int32_t fd, sptr<IRemoteObject> remoteObject, InstanceInfo instanceInfo)
{
    Format codecFormat;
    sptr<CodecServiceStub> codecStub = reinterpret_cast<CodecServiceStub*>(remoteObject.GetRefPtr());
    CHECK_AND_RETURN_RET_LOG(codecStub != nullptr, AVCS_ERR_INVALID_OPERATION,
        "reinterpret_cast to CodecServiceStub failed");
    int32_t ret = codecStub->GetOutputFormat(codecFormat);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Get codec format failed");
    std::string dumpString = "    ";
    dumpString += GetTimeString(instanceInfo.codecCreateTime).c_str();
    dumpString += " | " + std::to_string(instanceInfo.instanceId);
    dumpString += " | " + instanceInfo.GetActualCallerProcessName();
    std::string width = GetValueFromFormat(codecFormat, OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_WIDTH);
    std::string height = GetValueFromFormat(codecFormat, OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_HEIGHT);
    dumpString += " | " + width + " * " + height;
    dumpString += " | "
                + GetValueFromFormat(codecFormat, OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_NAME);
    dumpString += "\n";

    write(fd, dumpString.c_str(), dumpString.size());
    return AVCS_ERR_OK;
}

int32_t AVCodecServerManager::Dump(int32_t fd, const std::vector<std::u16string>& args)
{
    if (fd < 0) {
        return OHOS::NO_ERROR;
    }

    std::unordered_multimap<pid_t, std::pair<sptr<IRemoteObject>, InstanceInfo>> codecStubMapTemp;
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        codecStubMapTemp = codecStubMap_;
    }

    constexpr std::string_view dumpStr = "[Codec_Info]\n";
    write(fd, dumpStr.data(), dumpStr.size());

    if (args.size() > 1 && args[1] == u"simplify") {
        for (auto iter : codecStubMapTemp) {
            SimplifiedDump(fd, iter.second.first, iter.second.second);
        }
    } else {
        int32_t instanceIndex = 0;
        for (auto iter : codecStubMapTemp) {
            DumpInfo(fd, iter.second.first, iter.second.second, instanceIndex++);
        }
    }

    return OHOS::NO_ERROR;
}
}
}