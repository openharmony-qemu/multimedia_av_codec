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

#ifndef REFERENCE_PARSER_H
#define REFERENCE_PARSER_H

#include <cstdint>
#include <vector>
#include <map>
#include <set>
#include <iterator>
#include <memory>
#include <utility>
#include <algorithm>
#include <iostream>
#include "securec.h"
#include "common/status.h"
#include "common/log.h"
#include "common/media_core.h"

namespace OHOS {
namespace Media {
namespace Plugins {
constexpr uint8_t DEFAULT_NAL_LEN_SIZE = 4;

enum struct CodecType : int32_t {
    H264 = 0,
    H265,
};

enum SliceType {
    B_SLICE = 0,
    P_SLICE,
    I_SLICE,
    UNKNOWN_SLICE_TYPE,
};

struct PicRefInfo {
    uint32_t streamId = 0;
    uint32_t poc = 0;
    int32_t layerId = -1;
    bool isDiscardable = false;
    SliceType sliceType = SliceType::UNKNOWN_SLICE_TYPE;
    std::vector<uint32_t> refList;
};

class RefParser {
public:
    explicit RefParser(std::vector<uint32_t> &IFramePos);
    virtual ~RefParser();
    virtual Status RefParserInit();
    virtual Status ParserNalUnits(uint8_t *nalData, int32_t nalDataSize, uint32_t frameId, int64_t dts);
    virtual Status ParserExtraData(uint8_t *extraData, int32_t extraDataSize);
    virtual Status ParserSdtpData(uint8_t *sdtpData, int32_t sdtpDataSize);
    virtual Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo);
    virtual Status GetFrameLayerInfo(int64_t dts, FrameLayerInfo &frameLayerInfo);
    virtual Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo);

protected:
    void EliminateConflictInStream(uint8_t *stream, uint32_t &streamSize);
    void BuildRefTree(uint32_t gopId, uint32_t gopSize);
    void ClearInfo();
    void SaveSdtpInfo(uint8_t sampleFlags, uint32_t preIStreamId, uint32_t &disposableNum, uint32_t &disposableExtNum,
                      PicRefInfo &curPicRefInfo);
    void FillGopLayerInfo(uint32_t gopSize, uint32_t disposableNum, uint32_t disposableExtNum,
                          GopLayerInfo &gopLayerInfo);
    void DumpGopInfo(uint32_t gopId, uint32_t gopSize);
    uint8_t nalLenSize_ = DEFAULT_NAL_LEN_SIZE; // nalu长度所占字节数
    std::map<uint32_t, PicRefInfo> picRefInfo_;
    std::vector<uint32_t> IFramePos_;
    std::map<uint32_t, uint32_t> poc2StreamId_;
    std::map<uint32_t, GopLayerInfo> gopLayerInfoMap_;
    bool isEof_ = false;
    uint32_t curParserStreamId_ = 0;
    int64_t curParserDts_ = 0;
    std::map<int64_t, uint32_t> dts2streamId_;
    FILE *dumpFile_ = nullptr;
};
} // Plugins
} // Media
} // OHOS
#endif // REFERENCE_PARSER_H