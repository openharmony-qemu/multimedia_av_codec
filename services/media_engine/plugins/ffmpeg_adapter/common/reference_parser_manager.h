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

#ifndef REFERENCE_PARSER_MANAGER_H
#define REFERENCE_PARSER_MANAGER_H

#include "reference_parser.h"

namespace OHOS {
namespace Media {
namespace Plugins {
class ReferenceParserManager {
public:
    static std::shared_ptr<ReferenceParserManager> Create(CodecType codecType, std::vector<uint32_t> &IFramePos);
    ReferenceParserManager() {};
    ReferenceParserManager(const ReferenceParserManager &) = delete;
    ReferenceParserManager operator=(const ReferenceParserManager &) = delete;
    ~ReferenceParserManager();
    static bool Init();

    Status ParserNalUnits(uint8_t *nalData, int32_t nalDataSize, uint32_t frameId, int64_t dts);
    Status ParserExtraData(uint8_t *extraData, int32_t extraDataSize);
    Status ParserSdtpData(uint8_t *sdtpData, int32_t sdtpDataSize);
    Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo);
    Status GetFrameLayerInfo(int64_t dts, FrameLayerInfo &frameLayerInfo);
    Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo);
    
private:
    RefParser *referenceParser_ {nullptr};
    // .so initialize
    static void *handler_;
    static void *LoadPluginFile(const std::string &path);
    static bool CheckSymbol(void *handler);
    using CreateFunc = RefParser *(*)(CodecType, std::vector<uint32_t>&);
    using DestroyFunc = void (*)(RefParser *);
    static CreateFunc createFunc_;
    static DestroyFunc destroyFunc_;
    static std::mutex mtx_;
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // REFERENCE_PARSER_MANAGER_H