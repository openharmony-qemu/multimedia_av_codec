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

#ifndef STREAM_PARSER_H
#define STREAM_PARSER_H

#include <cstdint>

namespace OHOS {
namespace Media {
namespace Plugins {
class StreamParser {
public:
    explicit StreamParser() = default;
    virtual ~StreamParser() = default;
    virtual void ParseExtraData(const uint8_t *sample, int32_t size,
                                uint8_t **extraDataBuf, int32_t *extraDataSize) = 0;
    virtual void ConvertExtraDataToAnnexb(uint8_t *extraData, int32_t extraDataSize) = 0;
    virtual void ConvertPacketToAnnexb(uint8_t **hvccPacket, int32_t &hvccPacketSize, uint8_t *sideData,
        size_t sideDataSize, bool isExtradata) = 0;
    virtual void ParseAnnexbExtraData(const uint8_t *sample, int32_t size) = 0;
    virtual void ResetXPSSendStatus();
    virtual bool IsHdrVivid() = 0;
    virtual bool IsSyncFrame(const uint8_t *sample, int32_t size) = 0;
    virtual bool GetColorRange() = 0;
    virtual uint8_t GetColorPrimaries() = 0;
    virtual uint8_t GetColorTransfer() = 0;
    virtual uint8_t GetColorMatrixCoeff() = 0;
    virtual uint8_t GetProfileIdc() = 0;
    virtual uint8_t GetLevelIdc() = 0;
    virtual uint32_t GetChromaLocation() = 0;
    virtual uint32_t GetPicWidInLumaSamples() = 0;
    virtual uint32_t GetPicHetInLumaSamples() = 0;
};
} // Plugins
} // Media
} // OHOS
#endif // STREAM_PARSER_H