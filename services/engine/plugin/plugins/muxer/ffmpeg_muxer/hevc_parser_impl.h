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

#ifndef HEVC_PARSER_IMPL_H
#define HEVC_PARSER_IMPL_H

#include <cstdint>
#include <vector>
#include <memory>
#include <map>
#include "hevc_parser.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
namespace Ffmpeg {
class RbspContext;
class HevcParserImpl : public HevcParser {
public:
    explicit HevcParserImpl() = default;
    ~HevcParserImpl() override;
    void ParseExtraData(const uint8_t *sample, int32_t size, uint8_t **extraDataBuf, int32_t *extraDataSize) override;

    enum HevcNalType : uint8_t {
        HEVC_VPS_NAL_UNIT = 32,
        HEVC_SPS_NAL_UNIT = 33,
        HEVC_PPS_NAL_UNIT = 34,
        HEVC_PREFIX_SEI_NAL_UNIT = 39,
        HEVC_SUFFIX_SEI_NAL_UNIT = 40
    };

private:
    void ResetExtraData();
    void WriteExtraData();
    bool IsHvccFrame(const uint8_t *sample, int32_t size);
    bool IsAnnexbFrame(const uint8_t *sample, int32_t size);
	void AddNaluData(const uint8_t *buf, int32_t size);
    uint8_t *FindNalStartCode(const uint8_t *buf, const uint8_t *end, int32_t &startCodeLen);
    uint8_t GetNalType(uint8_t nalHeader);
    std::shared_ptr<RbspContext> ParseRbsp(const uint8_t *buf, int32_t size);
    void ParseVps(const std::shared_ptr<RbspContext> &rbspContext);
    void ParseProfileTierLevel(const std::shared_ptr<RbspContext> &rbspContext, uint8_t maxSubLayers);
    void SkipProfileTierLevelLayer(const std::shared_ptr<RbspContext> &rbspContext, uint8_t maxSubLayers);
	void ParseSps(const std::shared_ptr<RbspContext> &rbspContext);
    void SpsSkipScalingList(const std::shared_ptr<RbspContext> &rbspContext);
	int32_t ParseSpsPicSet(const std::shared_ptr<RbspContext> &rbspContext, uint32_t maxPicOrder);
    int32_t ParseRefPicSet(const std::shared_ptr<RbspContext> &rbspContext, int32_t index,
                           std::vector<uint32_t> &shortPicSetsArray);
    int32_t ParseVuiInfo(const std::shared_ptr<RbspContext> &rbspContext, uint8_t maxSubLayers);
	void ParseColorInfo(const std::shared_ptr<RbspContext> &rbspContext);
    void ParseVuiHrdParams(const std::shared_ptr<RbspContext> &rbspContext, uint8_t flag, uint8_t maxSubLayers);
	void SkipSubPicInfo(const std::shared_ptr<RbspContext> &rbspContext, uint8_t subPicFlag);
    void SpsSkipSubLayerHrdParams(const std::shared_ptr<RbspContext> &rbspContext, uint32_t cpbCount,
                                  uint8_t subPicFlag);
    void ParsePps(const std::shared_ptr<RbspContext> &rbspContext);
	void ParseParallelismType(const std::shared_ptr<RbspContext> &rbspContext);
    void SkipGolomb(const std::shared_ptr<RbspContext> &rbspContext, int32_t num);
    using ParseFuncType = void (HevcParserImpl::*)(const std::shared_ptr<RbspContext> &rbspContext);

    template <typename T>
    void Write(T data)
    {
        int32_t size = sizeof(data);
        for (int32_t i = size - 1; i >= 0; --i) {
            extraData_.emplace_back((data >> i * 0x08) & 0xFF);
        }
    }

private:
    const std::map<HevcNalType, ParseFuncType> nalParseType_ = {
        { HEVC_VPS_NAL_UNIT, &HevcParserImpl::ParseVps },
        { HEVC_SPS_NAL_UNIT, &HevcParserImpl::ParseSps },
        { HEVC_PPS_NAL_UNIT, &HevcParserImpl::ParsePps }
    };

    struct NaluData {
        uint8_t type_ = 0;
        uint16_t count_ = 0;
        std::vector<std::vector<uint8_t>> nalu_; // 2 bytes size + content
    };

    struct ExtraDataInfo {
        // hev1 type (version_ | 0x80)
        uint8_t version_ = 0x01;
        uint8_t profileSpace_ = 0;
        uint8_t tierFlag_ = 0;
        uint8_t profileIdc_ = 0;
        uint32_t profileCompatFlags_ = 0xFFFFFFFF;
        uint64_t constraintIndicatorFlags_ = 0xFFFFFFFFFFFF;
        uint8_t levelIdc_ = 0;
        uint16_t segmentIdc_ = 0;
        uint8_t parallelismType_ = 0;
        uint8_t chromaFormat_ = 0;
        uint8_t bitDepthLuma_ = 0;
        uint8_t bitDepthChroma_ = 0;
        uint16_t avgFrameRate_ = 0;
        uint8_t constFrameRate_ = 0;
        uint8_t numTemporalLayers_ = 0;
        uint8_t temporalIdNested_ = 0;
        uint8_t lenSizeMinusOne_ = 0x03;
        uint8_t naluTypeCount_ = 0;
        std::vector<NaluData> naluDataArray_;
    };

    /*
     * video_full_range_flag    u(1)
     * colour_primaries         u(8)
     * transfer_characteristics u(8)
     * matrix_coeffs            u(8)
    */
    uint8_t colorPrimaries_ = 0x02;
    uint8_t colorTransfer_ = 0x02;;
    uint8_t colorMatrixCoeff_ = 0x02;
    bool isParserColor_ = false;
    bool colorRange_ = false;
    ExtraDataInfo extraDataInfo_;
    std::vector<uint8_t> extraData_;
};

class RbspContext {
public:
    RbspContext(const std::vector<uint8_t> &buf);
    ~RbspContext();
    bool RbspCheckSize(int32_t size);
    void RbspSkipBits(int32_t size);
    uint32_t RbspGetUeGolomb();
    int32_t RbspGetSeGolomb();
    int32_t RbspGetLeftBitsNum();

    template <typename T = uint8_t, typename U = uint32_t>
    T RbspGetBits(int32_t size)
    {
        if (!RbspCheckSize(size)) {
            RbspSkipBits(size);
            return 0;
        }

        uint8_t *buf = buf_.data();
        U temp = *(reinterpret_cast<U *>(buf + byteIndex_));
        
        int32_t dataSize = sizeof(temp);
        U reverseData = 0;
        for (int32_t i = 0; i < dataSize; ++i) {
            reverseData = (reverseData << 0x08) | (temp & 0xFF);
            temp = temp >> 0x08;
        }

        T data = static_cast<T>((reverseData << bitIndex_) >> (0x08 * dataSize - size));
        RbspSkipBits(size);
        return data;
    }

private:
    uint8_t RbspGetBit();

private:
    std::vector<uint8_t> buf_;
    int32_t size_;
    int32_t byteIndex_;
    int32_t bitIndex_;
};
} // Ffmpeg
} // Plugin
} // MediaAVCodec
} // OHOS
#endif // HEVC_PARSER_IMPL_H