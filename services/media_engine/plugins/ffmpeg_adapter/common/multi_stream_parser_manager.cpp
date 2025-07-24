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
#include "multi_stream_parser_manager.h"
#include <dlfcn.h>
#include "common/log.h"

namespace {
const std::string HEVC_LIB_PATH = "libav_codec_hevc_parser.z.so";
const std::string VVC_LIB_PATH = "libav_codec_vvc_parser.z.so";
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MultiStreamParserManager" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
std::mutex MultiStreamParserManager::mtx_;
std::map<VideoStreamType, void *> MultiStreamParserManager::handlerMap_ {};
std::map<VideoStreamType, CreateFunc> MultiStreamParserManager::createFuncMap_ {};
std::map<VideoStreamType, DestroyFunc> MultiStreamParserManager::destroyFuncMap_ {};

MultiStreamParserManager::~MultiStreamParserManager()
{
    for (auto &streamInfo : streamMap_) {
        VideoStreamType videoStreamType = (streamInfo.second).type;
        StreamParser* streamParser = (streamInfo.second).parser;
        if (streamParser && destroyFuncMap_.count(videoStreamType) > 0) {
            destroyFuncMap_[videoStreamType](streamParser);
            (streamInfo.second).parser = nullptr;
        }
    }
}

Status MultiStreamParserManager::Create(uint32_t trackId, VideoStreamType videoStreamType)
{
    std::lock_guard<std::mutex> lock(mtx_);
    MEDIA_LOG_D("Create parser " PUBLIC_LOG_D32, videoStreamType);
    if (!Init(videoStreamType)) {
        return Status::ERROR_UNKNOWN;
    }
    StreamParser* streamParser = createFuncMap_[videoStreamType]();
    FALSE_RETURN_V_MSG_E(streamParser != nullptr, Status::ERROR_UNKNOWN, "Create failed:" PUBLIC_LOG_D32,
        videoStreamType);
    if (streamMap_.count(trackId) > 0 && streamMap_[trackId].parser != nullptr) {
        MEDIA_LOG_W("Parser change, %{public}d->%{public}d", streamMap_[trackId].type, videoStreamType);
        if (destroyFuncMap_.count(videoStreamType) > 0) {
            destroyFuncMap_[videoStreamType](streamMap_[trackId].parser);
        }
        streamMap_[trackId].parser = nullptr;
    }
    streamMap_[trackId].type = videoStreamType;
    streamMap_[trackId].parser = streamParser;
    streamMap_[trackId].inited = false;
    return Status::OK;
}

bool MultiStreamParserManager::Init(VideoStreamType videoStreamType)
{
    if (handlerMap_.count(videoStreamType) > 0 && createFuncMap_.count(videoStreamType) > 0 &&
        destroyFuncMap_.count(videoStreamType)> 0) {
        return true;
    }

    std::string streamParserPath;
    if (videoStreamType == VideoStreamType::HEVC) {
        streamParserPath = HEVC_LIB_PATH;
    } else if (videoStreamType == VideoStreamType::VVC) {
        streamParserPath = VVC_LIB_PATH;
    } else {
        MEDIA_LOG_E("Unsupport stream parser type");
        return false;
    }
    if (handlerMap_.count(videoStreamType) == 0) {
        handlerMap_[videoStreamType] = LoadLib(streamParserPath);
    }
    if (!CheckSymbol(handlerMap_[videoStreamType], videoStreamType)) {
        MEDIA_LOG_E("Load stream parser failed");
        return false;
    }
    return true;
}

void *MultiStreamParserManager::LoadLib(const std::string &path)
{
    auto ptr = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (ptr == nullptr) {
        MEDIA_LOG_E("Dlopen failed: %{public}s", ::dlerror());
    }
    return ptr;
}

bool MultiStreamParserManager::CheckSymbol(void *handler, VideoStreamType videoStreamType)
{
    if (handler) {
        std::string createFuncName = "CreateStreamParser";
        std::string destroyFuncName = "DestroyStreamParser";
        CreateFunc createFunc = nullptr;
        DestroyFunc destroyFunc = nullptr;
        createFunc = (CreateFunc)(::dlsym(handler, createFuncName.c_str()));
        destroyFunc = (DestroyFunc)(::dlsym(handler, destroyFuncName.c_str()));
        if (createFunc && destroyFunc) {
            MEDIA_LOG_D("CreateFuncName %{public}s", createFuncName.c_str());
            MEDIA_LOG_D("DestroyFuncName %{public}s", destroyFuncName.c_str());
            createFuncMap_[videoStreamType] = createFunc;
            destroyFuncMap_[videoStreamType] = destroyFunc;
            return true;
        }
    }
    return false;
}

bool MultiStreamParserManager::ParserIsCreated(uint32_t trackId)
{
    return streamMap_.count(trackId) > 0 && streamMap_[trackId].parser != nullptr;
}

bool MultiStreamParserManager::ParserIsInited(uint32_t trackId)
{
    return ParserIsCreated(trackId) && streamMap_[trackId].inited;
}

bool MultiStreamParserManager::AllParserInited()
{
    for (auto item : streamMap_) {
        if (!ParserIsInited(item.first)) {
            return false;
        }
    }
    return true;
}

bool MultiStreamParserManager::IsHdrVivid(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), false, "Stream parser is invalid");
    return streamMap_[trackId].parser->IsHdrVivid();
}

bool MultiStreamParserManager::IsSyncFrame(uint32_t trackId, const uint8_t *sample, int32_t size)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), false, "Stream parser is invalid");
    return streamMap_[trackId].parser->IsSyncFrame(sample, size);
}

bool MultiStreamParserManager::GetColorRange(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), false, "Stream parser is invalid");
    return streamMap_[trackId].parser->GetColorRange();
}

uint8_t MultiStreamParserManager::GetColorPrimaries(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), 0, "Stream parser is invalid");
    return streamMap_[trackId].parser->GetColorPrimaries();
}

uint8_t MultiStreamParserManager::GetColorTransfer(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), 0, "Stream parser is invalid");
    return streamMap_[trackId].parser->GetColorTransfer();
}

uint8_t MultiStreamParserManager::GetColorMatrixCoeff(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), 0, "Stream parser is invalid");
    return streamMap_[trackId].parser->GetColorMatrixCoeff();
}

uint8_t MultiStreamParserManager::GetProfileIdc(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), 0, "Stream parser is invalid");
    return streamMap_[trackId].parser->GetProfileIdc();
}

uint8_t MultiStreamParserManager::GetLevelIdc(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), 0, "Stream parser is invalid");
    return streamMap_[trackId].parser->GetLevelIdc();
}

uint32_t MultiStreamParserManager::GetChromaLocation(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), 0, "Stream parser is invalid");
    return streamMap_[trackId].parser->GetChromaLocation();
}

uint32_t MultiStreamParserManager::GetPicWidInLumaSamples(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), 0, "Stream parser is invalid");
    return streamMap_[trackId].parser->GetPicWidInLumaSamples();
}

uint32_t MultiStreamParserManager::GetPicHetInLumaSamples(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), 0, "Stream parser is invalid");
    return streamMap_[trackId].parser->GetPicHetInLumaSamples();
}

void MultiStreamParserManager::ResetXPSSendStatus(uint32_t trackId)
{
    FALSE_RETURN_MSG(ParserIsInited(trackId), "Stream parser is invalid");
    streamMap_[trackId].parser->ResetXPSSendStatus();
}

bool MultiStreamParserManager::ConvertExtraDataToAnnexb(uint32_t trackId, uint8_t *extraData, int32_t extraDataSize)
{
    FALSE_RETURN_V_MSG_E(ParserIsCreated(trackId), false, "Stream parser is invalid");
    bool ret = streamMap_[trackId].parser->ConvertExtraDataToAnnexb(extraData, extraDataSize);
    if (ret) {
        streamMap_[trackId].inited = true;
    }
    return ret;
}

void MultiStreamParserManager::ConvertPacketToAnnexb(
    uint32_t trackId, uint8_t **hvccPacket, int32_t &hvccPacketSize,
    uint8_t *sideData, size_t sideDataSize, bool isExtradata)
{
    FALSE_RETURN_MSG(ParserIsInited(trackId), "Stream parser is invalid");
    streamMap_[trackId].parser->ConvertPacketToAnnexb(hvccPacket, hvccPacketSize, sideData, sideDataSize, isExtradata);
}

void MultiStreamParserManager::ParseAnnexbExtraData(uint32_t trackId, const uint8_t *sample, int32_t size)
{
    FALSE_RETURN_MSG(ParserIsInited(trackId), "Stream parser is invalid");
    streamMap_[trackId].parser->ParseAnnexbExtraData(sample, size);
}

std::vector<uint8_t> MultiStreamParserManager::GetLogInfo(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(ParserIsInited(trackId), {}, "Stream parser is invalid");
    return streamMap_[trackId].parser->GetLogInfo();
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS