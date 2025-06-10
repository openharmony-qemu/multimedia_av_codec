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

#ifndef STREAM_PARSER_MANAGER_H
#define STREAM_PARSER_MANAGER_H

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "stream_parser.h"

namespace OHOS {
namespace Media {
namespace Plugins {
using CreateFunc = StreamParser *(*)();
using DestroyFunc = void (*)(StreamParser *);
class StreamParserManager {
public:
    static std::shared_ptr<StreamParserManager> Create(VideoStreamType streamType);
    StreamParserManager();
    StreamParserManager(const StreamParserManager &) = delete;
    StreamParserManager operator=(const StreamParserManager &) = delete;
    ~StreamParserManager();
    static bool Init(VideoStreamType videoStreamType);

    void ParseExtraData(const uint8_t *sample, int32_t size, uint8_t **extraDataBuf, int32_t *extraDataSize);
    bool IsHdrVivid();
    bool IsSyncFrame(const uint8_t *sample, int32_t size);
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
    bool ConvertExtraDataToAnnexb(uint8_t *extraData, int32_t extraDataSize);
    void ConvertPacketToAnnexb(uint8_t **hvccPacket, int32_t &hvccPacketSize, uint8_t *sideData,
        size_t sideDataSize, bool isExtradata);
    void ParseAnnexbExtraData(const uint8_t *sample, int32_t size);
    std::vector<uint8_t> GetLogInfo();
    
private:
    StreamParser *streamParser_ {nullptr};
    // .so initialize
    static void *LoadPluginFile(const std::string &path);
    static bool CheckSymbol(void *handler, VideoStreamType videoStreamType);
    VideoStreamType videoStreamType_;
    static std::mutex mtx_;
    static std::map<VideoStreamType, void *> handlerMap_;
    static std::map<VideoStreamType, CreateFunc> createFuncMap_;
    static std::map<VideoStreamType, DestroyFunc> destroyFuncMap_;
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // STREAM_PARSER_MANAGER_H