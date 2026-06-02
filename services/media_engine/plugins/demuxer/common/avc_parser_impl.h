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
    bool ParseMaxReorderPicFromAvcConfig(const uint8_t *extraData, int32_t extraDataSize);
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
    uint32_t GetMaxReorderPic() override { return maxReorderPic_; };;
    bool IsHdr10Plus() override  { return false; };
    int32_t GetFirstFillerDataNalSize(const uint8_t *sample, int32_t size) override { return 0; };

    enum AvcNalType : uint8_t {
        AVC_NAL_SLICE = 1,
        AVC_NAL_IDR_SLICE = 5,
        AVC_NAL_SEI = 6,
        AVC_NAL_SPS = 7,
        AVC_NAL_PPS = 8,
    };

    enum PsSource : int8_t {
        PS_OUT_OF_BAND = -1,
        PS_NONE = 0,
        PS_IN_BAND = 1,
    };

private:
    struct DataAddrInfo {
        uint32_t bytesIndex;
        uint32_t bufferSize;
    };

    struct NalUnitInfo {
        const uint8_t *data;
        uint32_t size;
        uint8_t nalType;
    };

    std::vector<uint8_t> spsVec_;
    std::vector<uint8_t> ppsVec_;
    std::vector<uint8_t> annexbExtraDataVec_;
    std::vector<uint8_t> annexbFrameVec_;
    uint8_t naluLength_ = 0;
    uint32_t maxReorderPic_ = 0;
    bool extradataParsed_ = false;
    bool newIdr_ = false;
    bool idrSpsSeen_ = false;
    bool idrPpsSeen_ = false;

    bool IsAnnexbFrame(const uint8_t *sample, int32_t size);
    void WriteStartCode(std::vector<uint8_t> &vec);
    void AppendOutOfBandPs(std::vector<uint8_t> &vec, const std::vector<uint8_t> &ps);
    void AppendNalUnit(std::vector<uint8_t> &vec, const uint8_t *data, uint32_t size, PsSource ps);
    void ConvertExtradata(uint8_t **packet, int32_t &packetSize);
    void ConvertPacketCore(uint8_t **packet, int32_t &packetSize);
    void FilterPsFromPacket(const uint8_t *packet, int32_t packetSize);
    bool ParseNalUnits(std::vector<NalUnitInfo> &nalUnits, const uint8_t *packet, int32_t packetSize);
    void SavePs(std::vector<uint8_t> &dst, const uint8_t *nal, uint32_t nalSize, bool first);
    bool SavePsVector(uint8_t *extraData, int32_t extraDataSize, std::vector<DataAddrInfo> src, AvcNalType nalType);
    void HandleNal(NalUnitInfo nal);
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // AVC_PARSER_IMPL_H
