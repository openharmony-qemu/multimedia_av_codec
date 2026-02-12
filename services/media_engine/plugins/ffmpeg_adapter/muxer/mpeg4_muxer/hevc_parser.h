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

#ifndef HEVC_PARSER_H
#define HEVC_PARSER_H

#include "video_parser.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class HevcParser : public VideoParser {
public:
    HevcParser();
    int32_t WriteFrame(const std::shared_ptr<AVIOStream> &io, const std::shared_ptr<AVBuffer> &sample) override;
    int32_t SetConfig(const std::shared_ptr<BasicBox> &box, std::vector<uint8_t> &codecConfig) override;
    int32_t Init();
    Any GetPointer() override {return Any(this);}
    bool IsParserColor();
    uint8_t GetColorPrimary();
    uint8_t GetColorTransfer();
    uint8_t GetColorMatrixCoeff();
    bool GetColorRange();
    bool IsHdrVivid();
    std::vector<uint8_t> GetLogInfo();

private:
    int32_t WriteAnnexBFrame(const std::shared_ptr<AVIOStream> &io, const uint8_t* sample, int32_t size);

private:
    HvccBox* hvccBox_ = nullptr;
    std::shared_ptr<BasicBox> hvccBasicBox_ = nullptr;
    std::shared_ptr<StreamParserManager> parser_ {nullptr};
    bool isParserColor_ = false;
    bool isFirstFrame_ = true;
    bool isAnnexbFrame_ = false;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif