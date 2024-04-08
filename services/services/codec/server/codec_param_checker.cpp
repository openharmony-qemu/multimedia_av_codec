/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "codec_param_checker.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "meta/meta_key.h"
#include "codec_ability_singleton.h"
#include "media_description.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecParamChecker"};
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;

template<class T>
bool isSupported(std::vector<T> cap, T value)
{
    return std::find(cap.begin(), cap.end(), value) != cap.end();
}

// Video codec checker
int32_t ResolutionChecker(CapabilityData &capData, Format &format, AVCodecType codecType);
int32_t PixelFormatChecker(CapabilityData &capData, Format &format, AVCodecType codecType);
int32_t FramerateChecker(CapabilityData &capData, Format &format, AVCodecType codecType);
int32_t BitrateAndQualityChecker(CapabilityData &capData, Format &format, AVCodecType codecType);
int32_t VideoProfileChecker(CapabilityData &capData, Format &format, AVCodecType codecType);
int32_t RotaitonChecker(CapabilityData &capData, Format &format, AVCodecType codecType);

// Checkers list define
using CheckerType = int32_t (*)(CapabilityData &capData, Format &format, AVCodecType codecType);
using CheckerListType = std::vector<CheckerType>;
const CheckerListType VIDEO_ENCODER_CONFIGURE_CHECKER_LIST = {
    ResolutionChecker,
    PixelFormatChecker,
    FramerateChecker,
    BitrateAndQualityChecker,
    VideoProfileChecker,
};

const CheckerListType VIDEO_DECODER_CONFIGURE_CHECKER_LIST = {
    ResolutionChecker,
    PixelFormatChecker,
    FramerateChecker,
    RotaitonChecker,
};

const CheckerListType VIDEO_ENCODER_PARAMETER_CHECKER_LIST = {
    BitrateAndQualityChecker,
};

const std::vector<std::string_view> FORMAT_MERGE_LIST = {
    OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE,
    OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_QUALITY,
};

// Checkers table
const std::unordered_map<CodecScenario, CheckerListType> CONFIGURE_CHECKERS_TABLE = {
    {CodecScenario::CODEC_SCENARIO_ENC_NORMAL, VIDEO_ENCODER_CONFIGURE_CHECKER_LIST},
    {CodecScenario::CODEC_SCENARIO_DEC_NORMAL, VIDEO_DECODER_CONFIGURE_CHECKER_LIST},
};

const std::unordered_map<CodecScenario, CheckerListType> PARAMETER_CHECKERS_TABLE = {
    {CodecScenario::CODEC_SCENARIO_ENC_NORMAL, VIDEO_ENCODER_PARAMETER_CHECKER_LIST},
};

// Checkers implementation
int32_t ResolutionChecker(CapabilityData &capData, Format &format, AVCodecType codecType)
{
    (void)codecType;
    int32_t width = 0;
    int32_t height = 0;
    bool widthExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    bool heightExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    CHECK_AND_RETURN_RET_LOG(widthExist && heightExist, AVCS_ERR_INVALID_VAL, "Key param missing, width or height");

    bool widthValid = true;
    bool heightValid = true;
    if (capData.supportSwapWidthHeight) {
        widthValid = capData.width.InRange(width) || capData.height.InRange(width);
        heightValid = capData.width.InRange(height) || capData.height.InRange(height);
    } else {
        widthValid = capData.width.InRange(width);
        heightValid = capData.height.InRange(height);
    }
    CHECK_AND_RETURN_RET_LOG(widthValid, AVCS_ERR_INVALID_VAL, "Param invalid, width: %{public}d", width);
    CHECK_AND_RETURN_RET_LOG(heightValid, AVCS_ERR_INVALID_VAL, "Param invalid, height: %{public}d", height);
    AVCODEC_LOGI("Param valid, resolution: %{public}d * %{public}d", width, height);
    return AVCS_ERR_OK;
}

int32_t PixelFormatChecker(CapabilityData &capData, Format &format, AVCodecType codecType)
{
    int32_t pixelFormat;
    bool paramExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, pixelFormat);
    CHECK_AND_RETURN_RET_LOG(!(codecType == AVCODEC_TYPE_VIDEO_ENCODER && !paramExist),
        AVCS_ERR_INVALID_VAL, "Key param missing for encoder, %{public}s",
        MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data());     // Encoder missing pixel format
    if (!paramExist) {
        return AVCS_ERR_OK;
    }

    bool paramValid = isSupported(capData.pixFormat, pixelFormat);
    CHECK_AND_RETURN_RET_LOG(paramValid, AVCS_ERR_UNSUPPORT, "Param invalid, %{public}s: %{public}d",
        MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(), pixelFormat);     // Invalid pixel format

    AVCODEC_LOGI("Param valid, %{public}s: %{public}d", MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(), pixelFormat);
    return AVCS_ERR_OK;
}

int32_t FramerateChecker(CapabilityData &capData, Format &format, AVCodecType codecType)
{
    (void)capData;
    (void)codecType;
    double framerate;
    bool paramExist = format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, framerate);
    if (paramExist == false) {
        return AVCS_ERR_OK;
    }

    bool paramValid = framerate > 0 ? true : false;
    CHECK_AND_RETURN_RET_LOG(paramValid, AVCS_ERR_INVALID_VAL, "Param invalid, %{public}s: %{public}.2f",
        MediaDescriptionKey::MD_KEY_FRAME_RATE.data(), framerate);     // Invalid framerate

    AVCODEC_LOGI("Param valid, %{public}s: %{public}.2f", MediaDescriptionKey::MD_KEY_FRAME_RATE.data(), framerate);
    return AVCS_ERR_OK;
}

int32_t BitrateAndQualityChecker(CapabilityData &capData, Format &format, AVCodecType codecType)
{
    (void)codecType;
    int64_t bitrate;
    int32_t quality;
    int32_t bitrateMode;
    bool bitrateExist = format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitrate);
    bool qualityExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_QUALITY, quality);
    bool bitrateModeExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, bitrateMode);

    CHECK_AND_RETURN_RET_LOG(!(bitrateExist && qualityExist), AVCS_ERR_INVALID_VAL,
        "Param invalid, bitrate and quality mutually include");  // bitrate and quality mutually include

    if (bitrateExist) {
        bool bitrateValid = capData.bitrate.InRange(bitrate);
        CHECK_AND_RETURN_RET_LOG(bitrateValid, AVCS_ERR_INVALID_VAL, "Param invalid, %{public}s: %{public}d",
            MediaDescriptionKey::MD_KEY_BITRATE.data(), static_cast<int32_t>(bitrate));     // Invalid bitrate
    }
    if (qualityExist) {
        bool qualityValid = capData.bitrate.InRange(quality);
        CHECK_AND_RETURN_RET_LOG(qualityValid, AVCS_ERR_INVALID_VAL, "Param invalid, %{public}s: %{public}d",
            MediaDescriptionKey::MD_KEY_QUALITY.data(), quality);     // Invalid quality
    }

    if (bitrateModeExist) {
        bool bitrateModeValid = isSupported(capData.bitrateMode, bitrateMode);
        CHECK_AND_RETURN_RET_LOG(bitrateModeValid, AVCS_ERR_UNSUPPORT, "Param invalid, %{public}s: %{public}d",
            MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE.data(), bitrateMode);     // Invalid bitrate mode
        CHECK_AND_RETURN_RET_LOG(!(bitrateExist && bitrateMode == VideoEncodeBitrateMode::CQ),
            AVCS_ERR_INVALID_VAL, "Param invalid, in CQ mode but set bitrate!");
        CHECK_AND_RETURN_RET_LOG(!(qualityExist && bitrateMode != VideoEncodeBitrateMode::CQ),
            AVCS_ERR_INVALID_VAL, "Param invalid, not in CQ mode but set quality!");
    } else {
        if (qualityExist && isSupported(capData.bitrateMode, static_cast<int32_t>(VideoEncodeBitrateMode::CQ))) {
            bitrateMode = VideoEncodeBitrateMode::CQ;
            format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, bitrateMode);
        }
    }

    AVCODEC_LOGI("Param valid, %{public}s: %{public}d, %{public}s: %{public}d",
        MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE.data(), bitrateMode,
        bitrateMode == VideoEncodeBitrateMode::CQ ? "quality" : "bitrate",
        bitrateMode == VideoEncodeBitrateMode::CQ ? quality : static_cast<int32_t>(bitrate));
    return AVCS_ERR_OK;
}

int32_t VideoProfileChecker(CapabilityData &capData, Format &format, AVCodecType codecType)
{
    (void)codecType;
    int32_t profile;
    bool paramExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_PROFILE, profile);
    if (paramExist == false) {
        return AVCS_ERR_OK;
    }

    bool paramValid = isSupported(capData.profiles, profile);
    CHECK_AND_RETURN_RET_LOG(paramValid, AVCS_ERR_UNSUPPORT, "Param invalid, %{public}s: %{public}d",
        MediaDescriptionKey::MD_KEY_PROFILE.data(), profile);     // Invalid pixel format

    AVCODEC_LOGI("Param valid, %{public}s: %{public}d", MediaDescriptionKey::MD_KEY_PROFILE.data(), profile);
    return AVCS_ERR_OK;
}

int32_t RotaitonChecker(CapabilityData &capData, Format &format, AVCodecType codecType)
{
    (void)capData;
    (void)codecType;
    int32_t rotation;
    bool paramExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, rotation);
    if (paramExist == false) {
        return AVCS_ERR_OK;
    }

    // valid rotation: 0, 90, 180, 270
    CHECK_AND_RETURN_RET_LOG(rotation == 0 || rotation == 90 || rotation == 180 || rotation == 270,
        AVCS_ERR_UNSUPPORT, "Param invalid, %{public}s: %{public}d",
        MediaDescriptionKey::MD_KEY_ROTATION_ANGLE.data(), rotation);    //  Invalid rotation

    AVCODEC_LOGI("Param valid, %{public}s: %{public}d", MediaDescriptionKey::MD_KEY_ROTATION_ANGLE.data(), rotation);
    return AVCS_ERR_OK;
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
int32_t CodecParamChecker::CheckConfigureValid(Media::Format &format, AVCodecType codecType, const std::string &codecName, CodecScenario scenario)
{
    AVCODEC_SYNC_TRACE;
    auto capData = CodecAbilitySingleton::GetInstance().GetCapabilityByName(codecName);
    CHECK_AND_RETURN_RET_LOG(capData != std::nullopt,
        AVCS_ERR_INVALID_OPERATION, "Get codec capbility from codec list failed");

    auto checkers = CONFIGURE_CHECKERS_TABLE.find(scenario);
    CHECK_AND_RETURN_RET_LOG(checkers != CONFIGURE_CHECKERS_TABLE.end(), AVCS_ERR_UNSUPPORT,
        "This scenario can not find any checkers");

    for (const auto &checker : checkers->second) {
        auto ret = checker(capData.value(), format, codecType);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Param check failed");
    }
    return AVCS_ERR_OK;
}

int32_t CodecParamChecker::CheckParameterValid(const Media::Format &format, Media::Format &oldFormat,
                                               AVCodecType codecType, const std::string &codecName, 
                                               CodecScenario scenario)
{
    AVCODEC_SYNC_TRACE;
    auto capData = CodecAbilitySingleton::GetInstance().GetCapabilityByName(codecName);
    CHECK_AND_RETURN_RET_LOG(capData != std::nullopt,
        AVCS_ERR_INVALID_OPERATION, "Get codec capbility from codec list failed");

    auto checkers = PARAMETER_CHECKERS_TABLE.find(scenario);
    CHECK_AND_RETURN_RET_LOG(checkers != PARAMETER_CHECKERS_TABLE.end(), AVCS_ERR_UNSUPPORT,
        "This scenario can not find any checkers");

    MergeFormat(format, oldFormat);

    for (const auto &checker : checkers->second) {
        auto ret = checker(capData.value(), oldFormat, codecType);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Param check failed");
    }
    return AVCS_ERR_OK;
}

std::optional<CodecScenario> CodecParamChecker::CheckCodecScenario(const Media::Format &format, AVCodecType codecType)
{
    CodecScenario scenario = CodecScenario::CODEC_SCENARIO_DEC_NORMAL;
    if (codecType == AVCODEC_TYPE_VIDEO_ENCODER) {
        scenario = CodecScenario::CODEC_SCENARIO_ENC_NORMAL;
    }
    (void)format;
    AVCODEC_LOGI("Codec scenario is %{public}d", scenario);
    return scenario;
}

void CodecParamChecker::MergeFormat(const Media::Format &format, Media::Format &oldFormat)
{
    for (const auto& key : FORMAT_MERGE_LIST) {
        if (!format.ContainKey(key)) {
            continue;
        }
        auto keyType = format.GetValueType(key);
        switch (keyType)
        {
        case FORMAT_TYPE_INT32: {
            int32_t value;
            format.GetIntValue(key, value);
            oldFormat.PutIntValue(key, value);
            break;
        }
        case FORMAT_TYPE_INT64: {
            int64_t value;
            format.GetLongValue(key, value);
            oldFormat.PutLongValue(key, value);
            break;
        }
        case FORMAT_TYPE_FLOAT: {
            float value;
            format.GetFloatValue(key, value);
            oldFormat.PutFloatValue(key, value);
            break;
        }
        case FORMAT_TYPE_DOUBLE: {
            double value;
            format.GetDoubleValue(key, value);
            oldFormat.PutDoubleValue(key, value);
            break;
        }
        case FORMAT_TYPE_STRING: {
            std::string value;
            format.GetStringValue(key, value);
            oldFormat.PutStringValue(key, value);
            break;
        }
        default:
            break;
        }
    }
}
} // namespace MediaAVCodec
} // namespace OHOS