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
int32_t ResolutionChecker(CapabilityData &capData, Format &format);
int32_t PixelFormatChecker(CapabilityData &capData, Format &format);
int32_t FramerateChecker(CapabilityData &capData, Format &format);
int32_t BitrateChecker(CapabilityData &capData, Format &format);
int32_t VideoProfileChecker(CapabilityData &capData, Format &format);
int32_t RotaitonChecker(CapabilityData &capData, Format &format);

// Checkers list define
using CheckerType = int32_t (*)(CapabilityData &capData, Format &format);
using CheckerListType = std::vector<CheckerType>;
const CheckerListType VIDEO_ENCODER_PARAMS_CHECKER_LIST = {
    ResolutionChecker,
    PixelFormatChecker,
    FramerateChecker,
    BitrateChecker,
    VideoProfileChecker,
};

const CheckerListType VIDEO_DECODER_PARAMS_CHECKER_LIST = {
    ResolutionChecker,
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
int32_t ResolutionChecker(CapabilityData &capData, Format &format)
{
    int32_t width = 0;
    int32_t height = 0;
    bool paramExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    CHECK_AND_RETURN_RET_LOG(paramExist == true, -1, "");     // Missing key param
    paramExist = format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    CHECK_AND_RETURN_RET_LOG(paramExist == true, -1, "");     // Missing key param

    bool paramValid = capData.width.InRange(width);
    CHECK_AND_RETURN_RET_LOG(paramValid == true, -1, "");     // Invalid width
    paramValid = capData.height.InRange(height);
    CHECK_AND_RETURN_RET_LOG(paramValid == true, -1, "");     // Invalid height

    return AVCS_ERR_OK;
}

int32_t PixelFormatChecker(CapabilityData &capData, Format &format)
{
    (void)capData;
    (void)format;
    return AVCS_ERR_OK;
}

int32_t FramerateChecker(CapabilityData &capData, Format &format)
{
    (void)capData;
    (void)format;
    return AVCS_ERR_OK;
}

int32_t BitrateChecker(CapabilityData &capData, Format &format)
{
    (void)capData;
    (void)format;
    return AVCS_ERR_OK;
}

int32_t VideoProfileChecker(CapabilityData &capData, Format &format)
{
    (void)capData;
    (void)format;
    return AVCS_ERR_OK;
}

int32_t RotaitonChecker(CapabilityData &capData, Format &format)
{
    (void)capData;
    (void)format;
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

    auto checkers = CHECKERS_TABLE.find(codecType);
    if (checkers != CHECKERS_TABLE.end()) {
        for (const auto &checker : checkers->second) {
            int32_t ret = checker(capData.value(), format);
            CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "");
        }
    }
    (void)format;
    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS