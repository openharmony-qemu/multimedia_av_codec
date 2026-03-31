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

#ifndef AVC_PARSER_IMPL_H
#define AVC_PARSER_IMPL_H

#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include "stream_parser.h"
#include "rbsp_context.h"

namespace OHOS {
namespace Media {
namespace Plugins {

class AvcParserImpl : public StreamParser {
public:
    explicit AvcParserImpl() = default;
    ~AvcParserImpl() override;
    void ParseExtraData(
        const uint8_t *sample, int32_t size, uint8_t **extraDataBuf, int32_t *extraDataSize) override { return; };
    bool ConvertExtraDataToAnnexb(uint8_t *extraData, int32_t extraDataSize) override;
    void ConvertPacketToAnnexb(uint8_t **hvccPacket, int32_t &hvccPacketSize, uint8_t *sideData, size_t sideDataSize,
        bool isExtradata) override;
    bool ConvertPacketToAnnexb(const PacketConvertToBufferInfo &convertInfo) override { return false; };
    void ParseAnnexbExtraData(const uint8_t *sample, int32_t size) override { return; };
    bool IsHdrVivid() override { return false; };;
    bool IsSyncFrame(const uint8_t *sample, int32_t size) override { return false; };
    bool GetColorRange() override { return false; };;
    uint8_t GetColorPrimaries() override { return false; };;
    uint8_t GetColorTransfer() override { return false; };;
    uint8_t GetColorMatrixCoeff() override { return false; };;
    uint8_t GetProfileIdc() override { return false; };;
    uint8_t GetLevelIdc() override { return false; };;
    uint32_t GetChromaLocation() override { return false; };;
    uint32_t GetPicWidInLumaSamples() override { return false; };;
    uint32_t GetPicHetInLumaSamples() override { return false; };;
    void ResetXPSSendStatus() override;
    std::vector<uint8_t> GetLogInfo() override { return {}; };;
    uint32_t GetMaxReorderPic() override { return 0; };;
    bool IsHdr10Plus() override  { return false; };
    int32_t GetFirstFillerDataNalSize(const uint8_t *sample, int32_t size) override { return 0; };

    enum AvcNalType : uint8_t {
        AVC_IDR_W_RADL = 5,
    };

private:
    bool IsAnnexbFrame(const uint8_t *sample, int32_t size);
    void WriteStartCode(uint32_t data, std::vector<uint8_t> &vec);
    struct DataAddrInfo {
        uint32_t bytesIndex;
        uint32_t bufferSize;
    };

    DataAddrInfo dataAddrInfo_;
    std::vector<uint8_t> annexbExtraDataVec_;
    std::vector<uint8_t> annexbFrameVec_;
    uint8_t naluLength_;
    uint32_t startCode_ = 1;
    bool sendXps_ = false;
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // AVC_PARSER_IMPL_H