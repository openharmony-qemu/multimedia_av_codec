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

#include "avcodec_errors.h"
#include <map>
#include <string>

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
using ErrorMessageFunc = std::function<std::string(const std::string &param1, const std::string &param2)>;
const std::map<AVCodecServiceErrCode, std::string> AVCS_ERRCODE_INFOS = {
    {AVCS_ERR_OK,                                    "success"},
    {AVCS_ERR_NO_MEMORY,                             "no memory"},
    {AVCS_ERR_INVALID_OPERATION,                     "operation not be permitted"},
    {AVCS_ERR_INVALID_VAL,                           "invalid argument"},
    {AVCS_ERR_UNKNOWN,                               "unkown error"},
    {AVCS_ERR_SERVICE_DIED,                          "avcodec service died"},
    {AVCS_ERR_CREATE_AVCODEC_STUB_FAILED,            "create avcodec sub service failed"},
    {AVCS_ERR_INVALID_STATE,                         "the state is not support this operation"},
    {AVCS_ERR_UNSUPPORT,                             "unsupport interface"},
    {AVCS_ERR_UNSUPPORT_AUD_SRC_TYPE,                "unsupport audio source type"},
    {AVCS_ERR_UNSUPPORT_AUD_SAMPLE_RATE,             "unsupport audio sample rate"},
    {AVCS_ERR_UNSUPPORT_AUD_CHANNEL_NUM,             "unsupport audio channel"},
    {AVCS_ERR_UNSUPPORT_AUD_ENC_TYPE,                "unsupport audio encoder type"},
    {AVCS_ERR_UNSUPPORT_AUD_PARAMS,                  "unsupport audio params(other params)"},
    {AVCS_ERR_UNSUPPORT_VID_SRC_TYPE,                "unsupport video source type"},
    {AVCS_ERR_UNSUPPORT_VID_ENC_TYPE,                "unsupport video encoder type"},
    {AVCS_ERR_UNSUPPORT_VID_PARAMS,                  "unsupport video params(other params)"},
    {AVCS_ERR_UNSUPPORT_FILE_TYPE,                   "unsupport file format type"},
    {AVCS_ERR_UNSUPPORT_PROTOCOL_TYPE,               "unsupport protocol type"},
    {AVCS_ERR_UNSUPPORT_VID_DEC_TYPE,                "unsupport video decoder type"},
    {AVCS_ERR_UNSUPPORT_AUD_DEC_TYPE,                "unsupport audio decoder type"},
    {AVCS_ERR_UNSUPPORT_STREAM,                      "internal data stream error"},
    {AVCS_ERR_UNSUPPORT_SOURCE,                      "unsupport source type"},
    {AVCS_ERR_AUD_ENC_FAILED,                        "audio encode failed"},
    {AVCS_ERR_AUD_RENDER_FAILED,                     "audio render failed"},
    {AVCS_ERR_VID_ENC_FAILED,                        "video encode failed"},
    {AVCS_ERR_AUD_DEC_FAILED,                        "audio decode failed"},
    {AVCS_ERR_VID_DEC_FAILED,                        "video decode failed"},
    {AVCS_ERR_MUXER_FAILED,                          "stream avmuxer failed"},
    {AVCS_ERR_DEMUXER_FAILED,                        "stream demuxer or parser failed"},
    {AVCS_ERR_OPEN_FILE_FAILED,                      "open file failed"},
    {AVCS_ERR_FILE_ACCESS_FAILED,                    "read or write file failed"},
    {AVCS_ERR_START_FAILED,                          "audio or video start failed"},
    {AVCS_ERR_PAUSE_FAILED,                          "audio or video pause failed"},
    {AVCS_ERR_STOP_FAILED,                           "audio or video stop failed"},
    {AVCS_ERR_SEEK_FAILED,                           "audio or video seek failed"},
    {AVCS_ERR_NETWORK_TIMEOUT,                       "network timeout"},
    {AVCS_ERR_NOT_FIND_FILE,                         "not find a file"},
    {AVCS_ERR_NOT_ENOUGH_DATA,                       "audio output buffer not enough of a pack"},
    {AVCS_ERR_END_OF_STREAM,                         "end of stream"},
    {AVCS_ERR_CONFIGURE_MISMATCH_CHANNEL_COUNT,      "missing channel count attribute in configure"},
    {AVCS_ERR_MISMATCH_SAMPLE_RATE,                  "missing sample rate attribute in configure"},
    {AVCS_ERR_MISMATCH_BIT_RATE,                     "missing bit rate attribute in configure"},
    {AVCS_ERR_CONFIGURE_ERROR,                       "compression level incorrect in flac encoder"},
    {AVCS_ERR_DECRYPT_FAILED,                        "decrypt protected content failed"},
    {AVCS_ERR_EXTEND_START,                          "extend start error code"}};

const std::map<AVCodecServiceErrCode, OH_AVErrCode> AVCSERRCODE_TO_OHAVCODECERRCODE = {
    {AVCS_ERR_OK,                                  AV_ERR_OK},
    {AVCS_ERR_NO_MEMORY,                           AV_ERR_NO_MEMORY},
    {AVCS_ERR_INVALID_OPERATION,                   AV_ERR_OPERATE_NOT_PERMIT},
    {AVCS_ERR_INVALID_VAL,                         AV_ERR_INVALID_VAL},
    {AVCS_ERR_INVALID_DATA,                        AV_ERR_INVALID_VAL},
    {AVCS_ERR_UNKNOWN,                             AV_ERR_UNKNOWN},
    {AVCS_ERR_SERVICE_DIED,                        AV_ERR_SERVICE_DIED},
    {AVCS_ERR_CREATE_AVCODEC_STUB_FAILED,          AV_ERR_UNKNOWN},
    {AVCS_ERR_INVALID_STATE,                       AV_ERR_INVALID_STATE},
    {AVCS_ERR_UNSUPPORT,                           AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_AUD_SRC_TYPE,              AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_AUD_SAMPLE_RATE,           AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_AUD_CHANNEL_NUM,           AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_AUD_ENC_TYPE,              AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_AUD_PARAMS,                AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_VID_SRC_TYPE,              AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_VID_ENC_TYPE,              AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_VID_PARAMS,                AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_FILE_TYPE,                 AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_PROTOCOL_TYPE,             AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_VID_DEC_TYPE,              AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_AUD_DEC_TYPE,              AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_STREAM,                    AV_ERR_UNSUPPORT},
    {AVCS_ERR_UNSUPPORT_SOURCE,                    AV_ERR_UNSUPPORT},
    {AVCS_ERR_AUD_RENDER_FAILED,                   AV_ERR_UNSUPPORT},
    {AVCS_ERR_AUD_ENC_FAILED,                      AV_ERR_UNKNOWN},
    {AVCS_ERR_VID_ENC_FAILED,                      AV_ERR_UNKNOWN},
    {AVCS_ERR_AUD_DEC_FAILED,                      AV_ERR_UNKNOWN},
    {AVCS_ERR_VID_DEC_FAILED,                      AV_ERR_UNKNOWN},
    {AVCS_ERR_MUXER_FAILED,                        AV_ERR_UNKNOWN},
    {AVCS_ERR_DEMUXER_FAILED,                      AV_ERR_UNKNOWN},
    {AVCS_ERR_OPEN_FILE_FAILED,                    AV_ERR_IO},
    {AVCS_ERR_FILE_ACCESS_FAILED,                  AV_ERR_IO},
    {AVCS_ERR_START_FAILED,                        AV_ERR_UNKNOWN},
    {AVCS_ERR_PAUSE_FAILED,                        AV_ERR_UNKNOWN},
    {AVCS_ERR_STOP_FAILED,                         AV_ERR_UNKNOWN},
    {AVCS_ERR_SEEK_FAILED,                         AV_ERR_UNKNOWN},
    {AVCS_ERR_NETWORK_TIMEOUT,                     AV_ERR_TIMEOUT},
    {AVCS_ERR_NOT_FIND_FILE,                       AV_ERR_UNSUPPORT},
    {AVCS_ERR_NOT_ENOUGH_DATA,                     AV_ERR_UNKNOWN},
    {AVCS_ERR_END_OF_STREAM,                       AV_ERR_UNKNOWN},
    {AVCS_ERR_CONFIGURE_MISMATCH_CHANNEL_COUNT,    AV_ERR_UNSUPPORT},
    {AVCS_ERR_MISMATCH_SAMPLE_RATE,                AV_ERR_UNSUPPORT},
    {AVCS_ERR_MISMATCH_BIT_RATE,                   AV_ERR_UNSUPPORT},
    {AVCS_ERR_CONFIGURE_ERROR,                     AV_ERR_UNSUPPORT},
    {AVCS_ERR_EXTEND_START,                        AV_ERR_EXTEND_START},
    {AVCS_ERR_DECRYPT_FAILED,                      AV_ERR_DRM_DECRYPT_FAILED},
    };

const std::map<OH_AVErrCode, std::string> OHAVCODECERRCODE_INFOS = {
    {AV_ERR_OK,                    "success"},
    {AV_ERR_NO_MEMORY,             "no memory"},
    {AV_ERR_OPERATE_NOT_PERMIT,    "operation not be permitted"},
    {AV_ERR_INVALID_VAL,           "invalid argument"},
    {AV_ERR_IO,                    "IO error"},
    {AV_ERR_TIMEOUT,               "network timeout"},
    {AV_ERR_UNKNOWN,               "unkown error"},
    {AV_ERR_SERVICE_DIED,          "avcodec service died"},
    {AV_ERR_INVALID_STATE,         "the state is not support this operation"},
    {AV_ERR_UNSUPPORT,             "unsupport interface"},
    {AV_ERR_EXTEND_START,          "extend err start"},
    {AV_ERR_DRM_DECRYPT_FAILED,    "decrypt failed"},
};

const std::map<Status, AVCodecServiceErrCode> STATUS_TO_AVCSERRCODE = {
    {Status::END_OF_STREAM, AVCodecServiceErrCode::AVCS_ERR_OK},
    {Status::OK, AVCodecServiceErrCode::AVCS_ERR_OK},
    {Status::NO_ERROR, AVCodecServiceErrCode::AVCS_ERR_OK},
    {Status::ERROR_UNKNOWN, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_PLUGIN_ALREADY_EXISTS, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_INCOMPATIBLE_VERSION, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_NO_MEMORY, AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY},
    {Status::ERROR_WRONG_STATE, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION},
    {Status::ERROR_UNIMPLEMENTED, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT},
    {Status::ERROR_INVALID_PARAMETER, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
    {Status::ERROR_INVALID_DATA, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
    {Status::ERROR_MISMATCHED_TYPE, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
    {Status::ERROR_TIMED_OUT, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_UNSUPPORTED_FORMAT, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT_FILE_TYPE},
    {Status::ERROR_NOT_ENOUGH_DATA, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_NOT_EXISTED, AVCodecServiceErrCode::AVCS_ERR_OPEN_FILE_FAILED},
    {Status::ERROR_AGAIN, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_PERMISSION_DENIED, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_NULL_POINTER, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
    {Status::ERROR_INVALID_OPERATION, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION},
    {Status::ERROR_CLIENT, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_SERVER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_DELAY_READY, AVCodecServiceErrCode::AVCS_ERR_OK},
    {Status::ERROR_INVALID_STATE, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION},
    {Status::ERROR_INVALID_BUFFER_SIZE, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_UNEXPECTED_MEMORY_TYPE, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_CREATE_BUFFER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_NULL_POINT_BUFFER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_INVALID_BUFFER_ID, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_INVALID_BUFFER_STATE, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_NO_FREE_BUFFER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_NO_DIRTY_BUFFER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_NO_CONSUMER_LISTENER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_NULL_BUFFER_QUEUE, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_WAIT_TIMEOUT, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_OUT_OF_RANGE, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_NULL_SURFACE, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_SURFACE_INNER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_NULL_SURFACE_BUFFER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_IPC_WRITE_INTERFACE_TOKEN, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
    {Status::ERROR_IPC_SEND_REQUEST, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN}};

std::string ErrorMessageOk(const std::string &param1, const std::string &param2)
{
    (void)param1;
    (void)param2;
    return "success";
}

std::string ErrorMessageNoPermission(const std::string &param1, const std::string &param2)
{
    std::string message = "Try to do " + param1 + " failed. User should request permission " + param2 + " first.";
    return message;
}

std::string ErrorMessageInvalidParameter(const std::string &param1, const std::string &param2)
{
    (void)param2;
    std::string message = "The Parameter " + param1 + " is invalid. Please check the type and range.";
    return message;
}

std::string ErrorMessageUnsupportCapability(const std::string &param1, const std::string &param2)
{
    (void)param2;
    std::string message = "Function " + param1 + " can not work correctly due to limited device capability.";
    return message;
}

std::string ErrorMessageNoMemory(const std::string &param1, const std::string &param2)
{
    (void)param2;
    std::string message = "Create " + param1 + " failed due to system memory.";
    return message;
}

std::string ErrorMessageOperateNotPermit(const std::string &param1, const std::string &param2)
{
    (void)param2;
    std::string message = "The operate " + param1 + " failed due to not permit in current state.";
    return message;
}

std::string ErrorMessageIO(const std::string &param1, const std::string &param2)
{
    (void)param2;
    std::string message = "IO error happened due to " + param1 + ".";
    return message;
}

std::string ErrorMessageTimeout(const std::string &param1, const std::string &param2)
{
    std::string message = "Timeout happend when " + param1 + " due to " + param2 + ".";
    return message;
}

std::string ErrorMessageServiceDied(const std::string &param1, const std::string &param2)
{
    (void)param1;
    (void)param2;
    std::string message = "AVCodec Serviced Died.";
    return message;
}

std::string ErrorMessageUnsupportFormat(const std::string &param1, const std::string &param2)
{
    (void)param2;
    std::string message = "The format " + param1 + " is not support.";
    return message;
}

std::string AVCSErrorToString(AVCodecServiceErrCode code)
{
    if (AVCS_ERRCODE_INFOS.count(code) != 0) {
        return AVCS_ERRCODE_INFOS.at(code);
    }

    return "unkown error";
}

std::string OHAVErrCodeToString(OH_AVErrCode code)
{
    if (OHAVCODECERRCODE_INFOS.count(code) != 0) {
        return OHAVCODECERRCODE_INFOS.at(code);
    }

    return "unkown error";
}

std::string AVCSErrorToOHAVErrCodeString(AVCodecServiceErrCode code)
{
    if (AVCS_ERRCODE_INFOS.count(code) != 0 && AVCSERRCODE_TO_OHAVCODECERRCODE.count(code) != 0) {
        OH_AVErrCode extCode = AVCSERRCODE_TO_OHAVCODECERRCODE.at(code);
        if (OHAVCODECERRCODE_INFOS.count(extCode) != 0) {
            return OHAVCODECERRCODE_INFOS.at(extCode);
        }
    }

    return "unkown error";
}

OH_AVErrCode AVCSErrorToOHAVErrCode(AVCodecServiceErrCode code)
{
    if (AVCS_ERRCODE_INFOS.count(code) != 0 && AVCSERRCODE_TO_OHAVCODECERRCODE.count(code) != 0) {
        return AVCSERRCODE_TO_OHAVCODECERRCODE.at(code);
    }

    return AV_ERR_UNKNOWN;
}

AVCodecServiceErrCode StatusToAVCodecServiceErrCode(Status code)
{
    if (STATUS_TO_AVCSERRCODE.count(code) != 0) {
        return STATUS_TO_AVCSERRCODE.at(code);
    }

    return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
}
} // namespace MediaAVCodec
} // namespace OHOS
