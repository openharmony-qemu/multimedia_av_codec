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

#ifndef MULTI_STREAM_PARSER_MANAGER_H
#define MULTI_STREAM_PARSER_MANAGER_H

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "stream_parser.h"
#include "common/status.h"

namespace OHOS {
namespace Media {
namespace Plugins {
using CreateFunc = StreamParser *(*)();
using DestroyFunc = void (*)(StreamParser *);
class MultiStreamParserManager {
public:
    MultiStreamParserManager() {};
    ~MultiStreamParserManager();
    Status Create(uint32_t trackId, VideoStreamType streamType);

    bool ParserIsCreated(uint32_t trackId);
    bool ParserIsInited(uint32_t trackId);
    bool AllParserInited();

    bool IsHdrVivid(uint32_t trackId);
    bool IsSyncFrame(uint32_t trackId, const uint8_t *sample, int32_t size);
    bool GetColorRange(uint32_t trackId);
    uint8_t GetColorPrimaries(uint32_t trackId);
    uint8_t GetColorTransfer(uint32_t trackId);
    uint8_t GetColorMatrixCoeff(uint32_t trackId);
    uint8_t GetProfileIdc(uint32_t trackId);
    uint8_t GetLevelIdc(uint32_t trackId);
    uint32_t GetChromaLocation(uint32_t trackId);
    uint32_t GetPicWidInLumaSamples(uint32_t trackId);
    uint32_t GetPicHetInLumaSamples(uint32_t trackId);

    void ResetXPSSendStatus(uint32_t trackId);
    bool ConvertExtraDataToAnnexb(uint32_t trackId, uint8_t *extraData, int32_t extraDataSize);
    void ConvertPacketToAnnexb(uint32_t trackId, uint8_t **hvccPacket, int32_t &hvccPacketSize, uint8_t *sideData,
        size_t sideDataSize, bool isExtradata);
    void ParseAnnexbExtraData(uint32_t trackId, const uint8_t *sample, int32_t size);
    std::vector<uint8_t> GetLogInfo(uint32_t trackId);
    
private:
    static std::mutex mtx_;

    // .so initialize
    static void *LoadLib(const std::string &path);
    static bool CheckSymbol(void *handler, VideoStreamType videoStreamType);

    // parser manage
    static std::map<VideoStreamType, void *> handlerMap_;
    static std::map<VideoStreamType, CreateFunc> createFuncMap_;
    static std::map<VideoStreamType, DestroyFunc> destroyFuncMap_;

    // stream manage
    struct StreamInfo {
        VideoStreamType type;
        StreamParser *parser;
        bool inited;
        ~StreamInfo() {
            if (parser != nullptr) {
                parser = nullptr;
            }
        }
    };
    std::map<uint32_t, StreamInfo> streamMap_ {};

    bool Init(VideoStreamType streamType);
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MULTI_STREAM_PARSER_MANAGER_H