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

class __attribute__((visibility("default"))) RefParser {
public:
    virtual ~RefParser() = default;
    virtual Status RefParserInit() = 0;
    virtual Status ParserNalUnits(uint8_t *nalData, int32_t nalDataSize, uint32_t frameId, int64_t dts) = 0;
    virtual Status ParserExtraData(uint8_t *extraData, int32_t extraDataSize) = 0;
    virtual Status ParserSdtpData(uint8_t *sdtpData, int32_t sdtpDataSize) = 0;
    virtual Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo) = 0;
    virtual Status GetFrameLayerInfo(int64_t dts, FrameLayerInfo &frameLayerInfo) = 0;
    virtual Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo) = 0;
};

extern "C" __attribute__((visibility("default"))) RefParser *CreateRefParser(
    CodecType codeType, std::vector<uint32_t> &IFramePos);

extern "C" __attribute__((visibility("default"))) void DestroyRefParser(RefParser *refParser);
} // Plugins
} // Media
} // OHOS
#endif // REFERENCE_PARSER_H