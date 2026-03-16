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

#include "codeclist_core.h"
#include <cmath> // fabs
#include <algorithm>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "codec_ability_singleton.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecListCore"};
constexpr float EPSINON = 0.0001;

const std::vector<std::string_view> MIME_VEC = {
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_AVC,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_HEVC,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_VVC,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_MPEG2,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_H263,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_MPEG4,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_RV30,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_RV40,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_MJPEG,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_VP8,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_VP9,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_MSVIDEO1,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_AV1,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_VC1,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_WMV3,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_WVC1,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_MPEG1,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_DVVIDEO,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_RAWVIDEO,
    OHOS::MediaAVCodec::CodecMimeType::VIDEO_CINEPAK,

    OHOS::MediaAVCodec::CodecMimeType::AUDIO_AMR_NB,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_AMR_WB,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_MPEG,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_AAC,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_VORBIS,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_OPUS,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_FLAC,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_RAW,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_G711MU,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_G711A,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_GSM_MS,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_GSM,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_COOK,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_AC3,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_WMAV1,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_WMAV2,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_WMAPRO,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_MS,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_QT,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_WAV,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_DK3,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_DK4,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_WS,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_SMJPEG,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_DAT4,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_MTAF,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_ADX,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_AFC,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_AICA,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_CT,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_DTK,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_G722,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_G726,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_G726LE,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_AMV,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_APC,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_ISS,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_OKI,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_IMA_RAD,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_PSX,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_SBPRO_2,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_SBPRO_3,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_SBPRO_4,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_THP,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_THP_LE,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_XA,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ADPCM_YAMAHA,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_EAC3,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ALAC,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_AVS3DA,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_LBVC,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_APE,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_MIMETYPE_L2HC,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_VIVID,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_ILBC,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_TRUEHD,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_TWINVQ,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_DVAUDIO,
    OHOS::MediaAVCodec::CodecMimeType::AUDIO_DTS,
};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
CodecListCore::CodecListCore()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecListCore::~CodecListCore()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool CodecListCore::CheckBitrate(const Format &format, const CapabilityData &data)
{
    int64_t targetBitrate;
    if (!format.ContainKey("bitrate")) {
        AVCODEC_LOGD("The bitrate of the format are not specified");
        return true;
    }
    (void)format.GetLongValue("bitrate", targetBitrate);
    if (data.bitrate.minVal > targetBitrate || data.bitrate.maxVal < targetBitrate) {
        return false;
    }
    return true;
}

bool CodecListCore::CheckVideoResolution(const Format &format, const CapabilityData &data)
{
    int32_t targetWidth;
    int32_t targetHeight;
    if ((!format.ContainKey("width")) || (!format.ContainKey("height"))) {
        AVCODEC_LOGD("The width and height of the format are not specified");
        return true;
    }
    (void)format.GetIntValue("width", targetWidth);
    (void)format.GetIntValue("height", targetHeight);
    if (data.width.minVal > targetWidth || data.width.maxVal < targetWidth || data.height.minVal > targetHeight ||
        data.height.maxVal < targetHeight) {
        return false;
    }
    return true;
}

bool CodecListCore::CheckVideoPixelFormat(const Format &format, const CapabilityData &data)
{
    int32_t targetPixelFormat;
    if (!format.ContainKey("pixel_format")) {
        AVCODEC_LOGD("The pixel_format of the format are not specified");
        return true;
    }
    (void)format.GetIntValue("pixel_format", targetPixelFormat);
    if (find(data.pixFormat.begin(), data.pixFormat.end(), targetPixelFormat) == data.pixFormat.end()) {
        return false;
    }
    return true;
}

bool CodecListCore::CheckVideoFrameRate(const Format &format, const CapabilityData &data)
{
    if (!format.ContainKey("frame_rate")) {
        AVCODEC_LOGD("The frame_rate of the format are not specified");
        return true;
    }

    switch (format.GetValueType(std::string_view("frame_rate"))) {
        case FORMAT_TYPE_INT32: {
            int32_t targetFrameRateInt;
            (void)format.GetIntValue("frame_rate", targetFrameRateInt);
            if (data.frameRate.minVal > targetFrameRateInt || data.frameRate.maxVal < targetFrameRateInt) {
                return false;
            }
            break;
        }
        case FORMAT_TYPE_DOUBLE: {
            double targetFrameRateDouble;
            (void)format.GetDoubleValue("frame_rate", targetFrameRateDouble);
            double minValDouble {data.frameRate.minVal};
            double maxValDouble {data.frameRate.maxVal};
            if ((minValDouble > targetFrameRateDouble && fabs(minValDouble - targetFrameRateDouble) >= EPSINON) ||
                (maxValDouble < targetFrameRateDouble && fabs(maxValDouble - targetFrameRateDouble) >= EPSINON)) {
                return false;
            }
            break;
        }
        default:
            break;
    }
    return true;
}

bool CodecListCore::CheckAudioChannel(const Format &format, const CapabilityData &data)
{
    int32_t targetChannel;
    if (!format.ContainKey("channel_count")) {
        AVCODEC_LOGD("The channel_count of the format are not specified");
        return true;
    }
    (void)format.GetIntValue("channel_count", targetChannel);
    if (data.channels.minVal > targetChannel || data.channels.maxVal < targetChannel) {
        return false;
    }
    return true;
}

bool CodecListCore::CheckAudioSampleRate(const Format &format, const CapabilityData &data)
{
    int32_t targetSampleRate;
    if (!format.ContainKey("samplerate")) {
        AVCODEC_LOGD("The samplerate of the format are not specified");
        return true;
    }
    (void)format.GetIntValue("samplerate", targetSampleRate);
    if (find(data.sampleRate.begin(), data.sampleRate.end(), targetSampleRate) == data.sampleRate.end()) {
        return false;
    }
    return true;
}

bool CodecListCore::IsVideoCapSupport(const Format &format, const CapabilityData &data)
{
    return CheckVideoResolution(format, data) && CheckVideoPixelFormat(format, data) &&
           CheckVideoFrameRate(format, data) && CheckBitrate(format, data);
}

bool CodecListCore::IsAudioCapSupport(const Format &format, const CapabilityData &data)
{
    return CheckAudioChannel(format, data) && CheckAudioSampleRate(format, data) && CheckBitrate(format, data);
}

// mime是必要参数
std::string CodecListCore::FindCodec(const Format &format, bool isEncoder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!format.ContainKey("codec_mime")) {
        AVCODEC_LOGD("Get MimeType from format failed");
        return "";
    }
    std::string targetMimeType;
    (void)format.GetStringValue("codec_mime", targetMimeType);

    AVCodecType codecType = AVCODEC_TYPE_NONE;
    bool isVideo = targetMimeType.find("video") != std::string::npos;
    if (isVideo) {
        codecType = isEncoder ? AVCODEC_TYPE_VIDEO_ENCODER : AVCODEC_TYPE_VIDEO_DECODER;
    } else {
        codecType = isEncoder ? AVCODEC_TYPE_AUDIO_ENCODER : AVCODEC_TYPE_AUDIO_DECODER;
    }

    int isVendor = -1;
    bool isVendorKey = format.ContainKey("codec_vendor_flag");
    if (isVendorKey) {
        (void)format.GetIntValue("codec_vendor_flag", isVendor);
    }
    std::vector<CapabilityData> capabilityDataArray = CodecAbilitySingleton::GetInstance().GetCapabilityArray();
    std::unordered_map<std::string, std::vector<size_t>> mimeCapIdxMap =
        CodecAbilitySingleton::GetInstance().GetMimeCapIdxMap();
    if (mimeCapIdxMap.find(targetMimeType) == mimeCapIdxMap.end()) {
        return "";
    }
    std::vector<size_t> capsIdx = mimeCapIdxMap.at(targetMimeType);
    for (auto iter = capsIdx.begin(); iter != capsIdx.end(); iter++) {
        CapabilityData capsData = capabilityDataArray[*iter];
        if (capsData.codecType != codecType || capsData.mimeType != targetMimeType ||
            (isVendorKey && capsData.isVendor != isVendor)) {
            continue;
        }
        if (isVideo) {
            if (IsVideoCapSupport(format, capsData)) {
                return capsData.codecName;
            }
        } else {
            if (IsAudioCapSupport(format, capsData)) {
                return capsData.codecName;
            }
        }
    }
    return "";
}

std::string CodecListCore::FindEncoder(const Format &format)
{
    return FindCodec(format, true);
}

std::string CodecListCore::FindDecoder(const Format &format)
{
    return FindCodec(format, false);
}

CodecType CodecListCore::FindCodecType(std::string codecName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (codecName.empty()) {
        return CodecType::AVCODEC_INVALID;
    }
    std::unordered_map<std::string, CodecType> nameCodecTypeMap =
        CodecAbilitySingleton::GetInstance().GetNameCodecTypeMap();
    if (nameCodecTypeMap.find(codecName) != nameCodecTypeMap.end()) {
        return nameCodecTypeMap.at(codecName);
    }
    return CodecType::AVCODEC_INVALID;
}

int32_t CodecListCore::GetCapability(CapabilityData &capData, const std::string &mime, const bool isEncoder,
                                     const AVCodecCategory &category)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(
        !mime.empty() && std::find(MIME_VEC.begin(), MIME_VEC.end(), mime.data()) != MIME_VEC.end(),
        AVCS_ERR_INVALID_VAL, "mime is invalid");
    AVCodecType codecType = AVCODEC_TYPE_NONE;
    bool isVideo = mime.find("video") != std::string::npos;
    if (isVideo) {
        codecType = isEncoder ? AVCODEC_TYPE_VIDEO_ENCODER : AVCODEC_TYPE_VIDEO_DECODER;
    } else {
        codecType = isEncoder ? AVCODEC_TYPE_AUDIO_ENCODER : AVCODEC_TYPE_AUDIO_DECODER;
    }
    bool isVendor = (category == AVCodecCategory::AVCODEC_HARDWARE) ? true : false;
    std::vector<CapabilityData> capsDataArray = CodecAbilitySingleton::GetInstance().GetCapabilityArray();
    std::unordered_map<std::string, std::vector<size_t>> mimeCapIdxMap =
        CodecAbilitySingleton::GetInstance().GetMimeCapIdxMap();
    CHECK_AND_RETURN_RET_LOGD(mimeCapIdxMap.find(mime) != mimeCapIdxMap.end(), AVCS_ERR_INVALID_VAL,
        "mime is not supported, mime: %{public}s", mime.c_str());
    std::vector<size_t> capsIdx = mimeCapIdxMap.at(mime);
    for (auto iter = capsIdx.begin(); iter != capsIdx.end(); iter++) {
        if (capsDataArray[*iter].codecType == codecType && capsDataArray[*iter].mimeType == mime) {
            if (category != AVCodecCategory::AVCODEC_NONE && capsDataArray[*iter].isVendor != isVendor) {
                continue;
            }
            capData = capsDataArray[*iter];
            AVCODEC_LOGI("find cap, mime: %{public}s, isEnc: %{public}d, category: %{public}d",
                mime.c_str(), isEncoder, category);
            break;
        }
    }
    CHECK_AND_RETURN_RET_LOG(!capData.codecName.empty(), AVCS_ERR_INVALID_VAL,
        "can not find cap, mime: %{public}s, mimeCapIdx size: %{public}zu, isEnc: %{public}d, category: %{public}d",
        mime.c_str(), capsIdx.size(), isEncoder, category);
    return AVCS_ERR_OK;
}

int32_t CodecListCore::GetCapabilityAt(CapabilityData &capabilityData, int32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CapabilityData> capsDataArray = CodecAbilitySingleton::GetInstance().GetCapabilityArray();
    if (index < 0 || index >= static_cast<int32_t>(capsDataArray.size())) {
        AVCODEC_LOGE("index is out of range, index: %{public}d, capa size: %{public}zu", index, capsDataArray.size());
        return AVCS_ERR_NOT_ENOUGH_DATA;
    }
    const CapabilityData &targetCap = capsDataArray[index];
    capabilityData = targetCap;
    return AVCS_ERR_OK;
}

std::vector<std::string> CodecListCore::FindCodecNameArray(const AVCodecType type, const std::string &mime)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto &codecAbility = CodecAbilitySingleton::GetInstance();

    std::unordered_map<std::string, std::vector<size_t>> mimeCapIdxMap = codecAbility.GetMimeCapIdxMap();
    std::vector<CapabilityData> capabilityArray = codecAbility.GetCapabilityArray();
    std::vector<std::string> nameArray;
    auto iter = mimeCapIdxMap.find(mime);
    CHECK_AND_RETURN_RET_LOG(iter != mimeCapIdxMap.end(), nameArray, "Can not find input mime type, %{public}s.",
                             mime.c_str());

    for (auto index : iter->second) {
        if (capabilityArray[index].codecType == type) {
            nameArray.push_back(capabilityArray[index].codecName);
        }
    }
    return nameArray;
}
} // namespace MediaAVCodec
} // namespace OHOS