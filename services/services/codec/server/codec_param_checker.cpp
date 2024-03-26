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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecParamChecker"};
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;

// Video codec checker
int32_t WidthChecker(CapabilityData &capData, Format &format, AVCodecType codecType);
int32_t HeightChecker(CapabilityData &capData, Format &format, AVCodecType codecType);
int32_t PixelFormatChecker(CapabilityData &capData, Format &format, AVCodecType codecType);
int32_t FramerateChecker(CapabilityData &capData, Format &format, AVCodecType codecType);
int32_t BitrateAndQualityChecker(CapabilityData &capData, Format &format, AVCodecType codecType);
int32_t VideoProfileChecker(CapabilityData &capData, Format &format, AVCodecType codecType);
int32_t RotaitonChecker(CapabilityData &capData, Format &format, AVCodecType codecType);

// Checkers list define
using CheckerType = int32_t (*)(CapabilityData &capData, Format &format, AVCodecType codecType);
using CheckerListType = std::vector<CheckerType>;
const CheckerListType VIDEO_ENCODER_PARAMS_CHECKER_LIST = {
    WidthChecker,
    HeightChecker,
    PixelFormatChecker,
    FramerateChecker,
    BitrateAndQualityChecker,
    VideoProfileChecker,
};

const CheckerListType VIDEO_DECODER_PARAMS_CHECKER_LIST = {
    WidthChecker,
    HeightChecker,
    PixelFormatChecker,
    FramerateChecker,
    RotaitonChecker,
};

// Checkers table
const std::unordered_map<AVCodecType, CheckerListType> CHECKERS_TABLE = {
    {AVCODEC_TYPE_VIDEO_ENCODER, VIDEO_ENCODER_PARAMS_CHECKER_LIST},
    {AVCODEC_TYPE_VIDEO_DECODER, VIDEO_DECODER_PARAMS_CHECKER_LIST},
};

// Checkers implementation
int32_t WidthChecker(CapabilityData &capData, Format &format, AVCodecType codecType)
{
    (void)codecType;
    int32_t width = 0;
    bool paramExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    CHECK_AND_RETURN_RET_LOG(paramExist, AVCS_ERR_INVALID_VAL, "Key param missing, %{public}s",
        MediaDescriptionKey::MD_KEY_WIDTH.data());     // Missing width

    bool paramValid = capData.width.InRange(width);
    CHECK_AND_RETURN_RET_LOG(paramValid, AVCS_ERR_INVALID_VAL, "Param invalid, %{public}s: %{public}d",
        MediaDescriptionKey::MD_KEY_WIDTH.data(), width);     // Invalid width

    AVCODEC_LOGI("Param valid, %{public}s: %{public}d", MediaDescriptionKey::MD_KEY_WIDTH.data(), width);
    return AVCS_ERR_OK;
}

int32_t HeightChecker(CapabilityData &capData, Format &format, AVCodecType codecType)
{
    (void)codecType;
    int32_t height = 0;
    bool paramExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    CHECK_AND_RETURN_RET_LOG(paramExist, AVCS_ERR_INVALID_VAL, "Key param missing, %{public}s",
        MediaDescriptionKey::MD_KEY_HEIGHT.data());     // Missing height

    bool paramValid = capData.height.InRange(height);
    CHECK_AND_RETURN_RET_LOG(paramValid, AVCS_ERR_INVALID_VAL, "Param invalid, %{public}s: %{public}d",
        MediaDescriptionKey::MD_KEY_HEIGHT.data(), height);     // Invalid height

    AVCODEC_LOGI("Param valid, %{public}s: %{public}d", MediaDescriptionKey::MD_KEY_HEIGHT.data(), height);
    return AVCS_ERR_OK;
}

int32_t PixelFormatChecker(CapabilityData &capData, Format &format, AVCodecType codecType)
{
    int32_t pixelFormat;
    bool paramExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, pixelFormat);
    if (!paramExist) {
        return AVCS_ERR_OK;
    }

    bool paramValid =
        std::find(capData.pixFormat.begin(), capData.pixFormat.end(), pixelFormat) != capData.pixFormat.end();
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
        bool bitrateModeValid = std::find(capData.bitrateMode.begin(), capData.bitrateMode.end(), bitrateMode) !=
            capData.bitrateMode.end();
        CHECK_AND_RETURN_RET_LOG(bitrateModeValid, AVCS_ERR_UNSUPPORT, "Param invalid, %{public}s: %{public}d",
            MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE.data(), bitrateMode);     // Invalid bitrate mode
        CHECK_AND_RETURN_RET_LOG(!(bitrateExist && bitrateMode == VideoEncodeBitrateMode::CQ),
            AVCS_ERR_INVALID_VAL, "Param invalid, in CQ mode but set bitrate!");
        CHECK_AND_RETURN_RET_LOG(!(qualityExist && bitrateMode != VideoEncodeBitrateMode::CQ),
            AVCS_ERR_INVALID_VAL, "Param invalid, not in CQ mode but set quality!");
    } else {
        if (qualityExist) {
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

    bool paramValid =
        std::find(capData.profiles.begin(), capData.profiles.end(), profile) != capData.profiles.end();
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
int32_t CodecParamChecker::CheckParamValid(Media::Format &format, AVCodecType codecType, std::string codecName)
{
    auto capData = CodecAbilitySingleton::GetInstance().GetCapabilityByName(codecName);
    CHECK_AND_RETURN_RET_LOG(capData != std::nullopt,
        AVCS_ERR_INVALID_OPERATION, "Get codec capbility from codec list failed");
    AVCODEC_SYNC_TRACE;

    auto checkers = CHECKERS_TABLE.find(codecType);
    if (checkers != CHECKERS_TABLE.end()) {
        for (const auto &checker : checkers->second) {
            int32_t ret = checker(capData.value(), format, codecType);
            CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Param check failed");
        }
    }
    (void)format;
    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS