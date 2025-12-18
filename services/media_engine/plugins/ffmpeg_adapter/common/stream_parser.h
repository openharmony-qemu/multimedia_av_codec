/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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
enum VideoStreamType {
    HEVC = 0,
    VVC  = 1,
};

struct PacketConvertToBufferInfo {
    // 原始输入数据
    const uint8_t *srcData {nullptr};
    int32_t srcDataSize {0};

    // XPS信息（参数集信息）
    const uint8_t *xpsData {nullptr};
    int32_t xpsDataSize {0};

    // 输出缓冲区（外部申请好的）
    uint8_t *outBuffer {nullptr};
    int32_t outBufferSize {0};

    // 转换后的实际输出大小（输出参数，通过引用返回）
    int32_t &outDataSize;

    // 可选的sideData信息
    uint8_t *sideData {nullptr};
    size_t sideDataSize {0};
    bool isExtradata {false};

    // 构造函数，用于初始化引用成员
    explicit PacketConvertToBufferInfo(int32_t &outSizeRef) : outDataSize(outSizeRef) {}
};

class StreamParser {
public:
    explicit StreamParser() = default;
    virtual ~StreamParser() = default;
    virtual void ParseExtraData(const uint8_t *sample, int32_t size,
                                uint8_t **extraDataBuf, int32_t *extraDataSize) = 0;
    virtual bool ConvertExtraDataToAnnexb(uint8_t *extraData, int32_t extraDataSize) = 0;
    virtual void ConvertPacketToAnnexb(uint8_t **hvccPacket, int32_t &hvccPacketSize, uint8_t *sideData,
        size_t sideDataSize, bool isExtradata) = 0;
    virtual void ConvertPacketToAnnexb(const PacketConvertToBufferInfo &convertInfo) = 0;
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
    virtual std::vector<uint8_t> GetLogInfo() = 0;
    virtual uint32_t GetMaxReorderPic() = 0;
    virtual int32_t GetFirstFillerDataNalSize(const uint8_t *sample, int32_t size) = 0;
};
} // Plugins
} // Media
} // OHOS
#endif // STREAM_PARSER_H