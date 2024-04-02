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

#include "codeclist_parcel.h"
#include "avcodec_log.h"
#include "meta/meta.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecListParcel"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
bool CodecListParcel::Marshalling(MessageParcel &parcel, CapabilityData &capabilityData)
{
    (void)parcel.WriteString(capabilityData.codecName);
    (void)parcel.WriteString(capabilityData.mimeType);
    (void)parcel.WriteBool(capabilityData.isVendor);
    (void)parcel.WriteInt32(capabilityData.codecType);
    (void)parcel.WriteInt32(capabilityData.maxInstance);
    (void)parcel.WriteInt32(capabilityData.bitrate.minVal);
    (void)parcel.WriteInt32(capabilityData.bitrate.maxVal);
    (void)parcel.WriteInt32(capabilityData.channels.minVal);
    (void)parcel.WriteInt32(capabilityData.channels.maxVal);
    (void)parcel.WriteInt32(capabilityData.complexity.minVal);
    (void)parcel.WriteInt32(capabilityData.complexity.maxVal);
    (void)parcel.WriteInt32(capabilityData.alignment.width);
    (void)parcel.WriteInt32(capabilityData.alignment.height);
    (void)parcel.WriteInt32(capabilityData.width.minVal);
    (void)parcel.WriteInt32(capabilityData.width.maxVal);
    (void)parcel.WriteInt32(capabilityData.height.minVal);
    (void)parcel.WriteInt32(capabilityData.height.maxVal);
    (void)parcel.WriteInt32(capabilityData.frameRate.minVal);
    (void)parcel.WriteInt32(capabilityData.frameRate.maxVal);
    (void)parcel.WriteInt32(capabilityData.encodeQuality.minVal);
    (void)parcel.WriteInt32(capabilityData.encodeQuality.maxVal);
    (void)parcel.WriteInt32(capabilityData.blockPerFrame.minVal);
    (void)parcel.WriteInt32(capabilityData.blockPerFrame.maxVal);
    (void)parcel.WriteInt32(capabilityData.blockPerSecond.minVal);
    (void)parcel.WriteInt32(capabilityData.blockPerSecond.maxVal);
    (void)parcel.WriteInt32(capabilityData.blockSize.width);
    (void)parcel.WriteInt32(capabilityData.blockSize.height);
    (void)parcel.WriteInt32Vector(capabilityData.sampleRate);
    (void)parcel.WriteInt32Vector(capabilityData.pixFormat);
    (void)parcel.WriteInt32Vector(capabilityData.bitDepth);
    (void)parcel.WriteInt32Vector(capabilityData.profiles);
    (void)parcel.WriteInt32Vector(capabilityData.bitrateMode);
    (void)Marshalling(parcel, capabilityData.measuredFrameRate);
    (void)Marshalling(parcel, capabilityData.profileLevelsMap);
    (void)Marshalling(parcel, capabilityData.featuresMap);
    AVCODEC_LOGD("success to Marshalling capabilityDataArray");

    return true;
}

bool CodecListParcel::Marshalling(MessageParcel &parcel, const std::map<ImgSize, Range> &mapSizeToRange)
{
    parcel.WriteUint32(mapSizeToRange.size());
    for (auto it = mapSizeToRange.begin(); it != mapSizeToRange.end(); it++) {
        (void)parcel.WriteInt32(it->first.width);
        (void)parcel.WriteInt32(it->first.height);
        (void)parcel.WriteInt32(it->second.minVal);
        (void)parcel.WriteInt32(it->second.maxVal);
    }
    return true;
}

bool CodecListParcel::Marshalling(MessageParcel &parcel, const std::map<int32_t, std::vector<int32_t>> &mapIntToVec)
{
    parcel.WriteUint32(mapIntToVec.size());
    for (auto it = mapIntToVec.begin(); it != mapIntToVec.end(); it++) {
        (void)parcel.WriteInt32(it->first);
        (void)parcel.WriteInt32Vector(it->second);
    }
    return true;
}

bool CodecListParcel::Marshalling(MessageParcel &parcel, const std::map<int32_t, Format> &mapIntToFormat)
{
    parcel.WriteUint32(mapIntToFormat.size());
    for (auto it = mapIntToFormat.begin(); it != mapIntToFormat.end(); it++) {
        (void)parcel.WriteInt32(it->first);
        Format format = it->second;
        format.GetMeta()->ToParcel(parcel);
    }
    return true;
}

bool CodecListParcel::Unmarshalling(MessageParcel &parcel, CapabilityData &capabilityData)
{
    capabilityData.codecName = parcel.ReadString();
    capabilityData.mimeType = parcel.ReadString();
    capabilityData.isVendor = parcel.ReadBool();
    capabilityData.codecType = parcel.ReadInt32();
    capabilityData.maxInstance = parcel.ReadInt32();
    capabilityData.bitrate.minVal = parcel.ReadInt32();
    capabilityData.bitrate.maxVal = parcel.ReadInt32();
    capabilityData.channels.minVal = parcel.ReadInt32();
    capabilityData.channels.maxVal = parcel.ReadInt32();
    capabilityData.complexity.minVal = parcel.ReadInt32();
    capabilityData.complexity.maxVal = parcel.ReadInt32();
    capabilityData.alignment.width = parcel.ReadInt32();
    capabilityData.alignment.height = parcel.ReadInt32();
    capabilityData.width.minVal = parcel.ReadInt32();
    capabilityData.width.maxVal = parcel.ReadInt32();
    capabilityData.height.minVal = parcel.ReadInt32();
    capabilityData.height.maxVal = parcel.ReadInt32();
    capabilityData.frameRate.minVal = parcel.ReadInt32();
    capabilityData.frameRate.maxVal = parcel.ReadInt32();
    capabilityData.encodeQuality.minVal = parcel.ReadInt32();
    capabilityData.encodeQuality.maxVal = parcel.ReadInt32();
    capabilityData.blockPerFrame.minVal = parcel.ReadInt32();
    capabilityData.blockPerFrame.maxVal = parcel.ReadInt32();
    capabilityData.blockPerSecond.minVal = parcel.ReadInt32();
    capabilityData.blockPerSecond.maxVal = parcel.ReadInt32();
    capabilityData.blockSize.width = parcel.ReadInt32();
    capabilityData.blockSize.height = parcel.ReadInt32();
    parcel.ReadInt32Vector(&capabilityData.sampleRate);
    parcel.ReadInt32Vector(&capabilityData.pixFormat);
    parcel.ReadInt32Vector(&capabilityData.bitDepth);
    parcel.ReadInt32Vector(&capabilityData.profiles);
    parcel.ReadInt32Vector(&capabilityData.bitrateMode);
    CHECK_AND_RETURN_RET_LOG(Unmarshalling(parcel, capabilityData.measuredFrameRate), false,
                             "failed to Unmarshalling measuredFrameRate vector");
    CHECK_AND_RETURN_RET_LOG(Unmarshalling(parcel, capabilityData.profileLevelsMap), false,
                             "failed to Unmarshalling profileLevels map");
    CHECK_AND_RETURN_RET_LOG(Unmarshalling(parcel, capabilityData.featuresMap), false,
                             "failed to Unmarshalling features map");
    AVCODEC_LOGD("success to Unmarshalling capabilityDataArray");
    return true;
}

bool CodecListParcel::Unmarshalling(MessageParcel &parcel, std::map<ImgSize, Range> &mapSizeToRange)
{
    uint32_t size = parcel.ReadUint32();
    CHECK_AND_RETURN_RET_LOG(size <= MAX_MAP_SIZE, false, "mapSizeToRange is invalid");
    for (uint32_t index = 0; index < size; index++) {
        ImgSize key;
        Range values;
        key.width = parcel.ReadInt32();
        key.height = parcel.ReadInt32();
        values.minVal = parcel.ReadInt32();
        values.maxVal = parcel.ReadInt32();
        mapSizeToRange.insert(std::make_pair(key, values));
    }
    return true;
}

bool CodecListParcel::Unmarshalling(MessageParcel &parcel, std::map<int32_t, std::vector<int32_t>> &mapIntToVec)
{
    uint32_t size = parcel.ReadUint32();
    CHECK_AND_RETURN_RET_LOG(size <= MAX_MAP_SIZE, false, "mapIntToVec is invalid");
    for (uint32_t index = 0; index < size; index++) {
        int32_t key;
        std::vector<int32_t> values;
        key = parcel.ReadInt32();
        parcel.ReadInt32Vector(&values);
        mapIntToVec.insert(std::make_pair(key, values));
    }
    return true;
}

bool CodecListParcel::Unmarshalling(MessageParcel &parcel, std::map<int32_t, Format> &mapIntToFormat)
{
    uint32_t size = parcel.ReadUint32();
    CHECK_AND_RETURN_RET_LOG(size <= MAX_MAP_SIZE, false, "mapIntToFormat is invalid");
    for (uint32_t index = 0; index < size; index++) {
        int32_t key = parcel.ReadInt32();
        Format format;
        auto meta = std::make_shared<Meta>();
        meta->FromParcel(parcel);
        format.SetMeta(std::move(meta));
        mapIntToFormat.insert(std::make_pair(key, format));
    }
    return true;
}
} // namespace MediaAVCodec
} // namespace OHOS
