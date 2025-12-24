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

#ifndef BOX_PARSER_H
#define BOX_PARSER_H

#include "basic_box.h"
#include "basic_track.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class BoxParser {
public:
    explicit BoxParser(uint32_t fileType);
    ~BoxParser();
    std::shared_ptr<BasicBox> MoovBoxGenerate();
    void AddTrakBox(std::shared_ptr<BasicTrack> track);
    void AddUdtaBox();

private:
    std::shared_ptr<BasicBox> MvhdBoxGenerate();
    std::shared_ptr<BasicBox> UdtaBoxGenerate();
    std::shared_ptr<BasicBox> MetaBoxGenerate();
    std::shared_ptr<BasicBox> IlstBoxGenerate();
    std::shared_ptr<BasicBox> TrakBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> TkhdBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> EdtsBoxGenerate();
    std::shared_ptr<BasicBox> MdiaBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> MdhdBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> HdlrBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> MinfBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> StblBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> StsdBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> AudioBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> VideoBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> EsdsBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> AvccBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> HvccBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> ColrBoxGenerate(std::shared_ptr<BasicTrack> track);
    std::shared_ptr<BasicBox> CuvvBoxGenerate(std::shared_ptr<BasicTrack> track);
    uint32_t CalculateEsDescr(uint32_t size);
    uint32_t fileType_;
    std::shared_ptr<BasicBox> moov_ = nullptr;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif