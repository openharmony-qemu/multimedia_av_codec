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

#ifndef HEVC_PARSER_MANAGER_H
#define HEVC_PARSER_MANAGER_H

#include <string>
#include <memory>
#include "hevc_parser.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
class HevcParserManager {
public:
    explicit HevcParserManager(void *handler);

    HevcParserManager(const HevcParserManager &) = delete;

    HevcParserManager operator=(const HevcParserManager &) = delete;

    ~HevcParserManager();

    static std::shared_ptr<HevcParserManager> Create();

    void ParseExtraData(const uint8_t *sample, int32_t size, uint8_t **extraDataBuf, int32_t *extraDataSize);

    bool IsHdrVivid();

    bool GetColorRange();

    uint8_t GetColorPrimaries();

    uint8_t GetColorTransfer();
    
    uint8_t GetColorMatrixCoeff();

    uint8_t GetProfileIdc();

    uint8_t GetLevelIdc();

    uint32_t GetChromaLocation();

    uint32_t GetPicWidInLumaSamples();
    uint32_t GetPicHetInLumaSamples();

    void ResetXPSSendStatus();

    void ConvertExtraDataToAnnexb(uint8_t *extraData, int32_t extraDataSize);
    void ConvertPacketToAnnexb(uint8_t **hvccPacket, int32_t &hvccPacketSize);
    void ParseAnnexbExtraData(const uint8_t *sample, int32_t size);

private:
    static void *LoadPluginFile(const std::string &path);

    static std::shared_ptr<HevcParserManager> CheckSymbol(void *handler);

    void UnLoadPluginFile();

    using CreateFunc = Media::Plugins::HevcParser *(*)();

    using DestroyFunc = void (*)(Media::Plugins::HevcParser *);

private:
    const void *handler_;
    Media::Plugins::HevcParser *hevcParser_ {nullptr};
    CreateFunc createFunc_ {nullptr};
    DestroyFunc destroyFunc_ {nullptr};
};
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS
#endif // HEVC_PARSER_MANAGER_H