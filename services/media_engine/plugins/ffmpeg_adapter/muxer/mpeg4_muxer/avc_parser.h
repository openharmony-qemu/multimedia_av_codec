/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef MPEG4_MUXER_AVC_PARSER_H
#define MPEG4_MUXER_AVC_PARSER_H

#include "video_parser.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class AvcParser : public VideoParser {
public:
    AvcParser();
    int32_t WriteFrame(const std::shared_ptr<AVIOStream> &io, const std::shared_ptr<AVBuffer> &sample) override;
    using ParserFunc = int32_t(AvcParser::*)(const uint8_t *sample, int32_t size);
    int32_t SetConfig(const std::shared_ptr<BasicBox> &box, std::vector<uint8_t> &codecConfig) override;

private:
    int32_t WriteAnnexBFrame(const std::shared_ptr<AVIOStream> &io, const std::shared_ptr<AVBuffer> &sample);
    int32_t ParseSps(const uint8_t* sample, int32_t size);
    int32_t ParsePps(const uint8_t* sample, int32_t size);
    int32_t ParseSpsExt(const uint8_t* sample, int32_t size);
    int32_t ParseSpsInfo(const std::shared_ptr<RbspContext>& rbspContext);
    inline void UpdateNeedParser();

private:
    enum AvcNalType : uint8_t {
        AVC_SPS_NAL_UNIT = 7,
        AVC_PPS_NAL_UNIT = 8,
        AVC_SPS_EXT_NAL_UNIT = 13,
    };

    uint8_t spsCount_ = 0;
    std::shared_ptr<BasicBox> avccBasicBox_ = nullptr;
    AvccBox *avccBox_ = nullptr;
    std::unordered_map<AvcNalType, std::pair<bool, ParserFunc>> needParse_ = {
        {AVC_SPS_NAL_UNIT, {true, &AvcParser::ParseSps}},
        {AVC_PPS_NAL_UNIT, {true, &AvcParser::ParsePps}},
        {AVC_SPS_EXT_NAL_UNIT, {true, &AvcParser::ParseSpsExt}}
    };
    bool isFirstFrame_ = true;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif