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

#define HST_LOG_TAG "StreamParserManager"

#include "stream_parser_manager.h"
#include <dlfcn.h>
#include "common/log.h"

namespace {
const std::string HEVC_LIB_PATH = "libav_codec_hevc_parser.z.so";
}

namespace OHOS {
namespace Media {
namespace Plugins {
void *StreamParserManager::handler_ = nullptr;
StreamParserManager::CreateFunc StreamParserManager::createFunc_ = nullptr;
StreamParserManager::DestroyFunc StreamParserManager::destroyFunc_ = nullptr;
std::mutex StreamParserManager::mtx_;

StreamParserManager::~StreamParserManager()
{
    if (streamParser_) {
        destroyFunc_(streamParser_);
        streamParser_ = nullptr;
    }
}

bool StreamParserManager::Init(StreamType streamType)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (!handler_) {
        std::string streamParserPath;
        if (streamType == StreamType::HEVC) {
            streamParserPath = HEVC_LIB_PATH;
        } else {
            MEDIA_LOG_E("Unsupport stream parser type");
            return false;
        }
        if (!CheckSymbol(LoadPluginFile(streamParserPath))) {
            MEDIA_LOG_E("Load stream parser so fail");
            return false;
        }
    }
    return true;
}

std::shared_ptr<StreamParserManager> StreamParserManager::Create(StreamType streamType)
{
    MEDIA_LOG_I("Create stream parser: " PUBLIC_LOG_D32, streamType);
    std::shared_ptr<StreamParserManager> loader = std::make_shared<StreamParserManager>();
    if (!loader->Init(streamType)) {
        return nullptr;
    }
    loader->streamParser_ = loader->createFunc_();
    if (!loader->streamParser_) {
        MEDIA_LOG_E("createFunc_ fail");
        return nullptr;
    }
    return loader;
}

void StreamParserManager::ParseExtraData(const uint8_t *sample, int32_t size,
    uint8_t **extraDataBuf, int32_t *extraDataSize)
{
    FALSE_RETURN_MSG(streamParser_ != nullptr, "stream parser is null!");
    streamParser_->ParseExtraData(sample, size, extraDataBuf, extraDataSize);
}

bool StreamParserManager::IsHdrVivid()
{
    FALSE_RETURN_V_MSG_E(streamParser_ != nullptr, false, "stream parser is null!");
    return streamParser_->IsHdrVivid();
}

bool StreamParserManager::IsSyncFrame(const uint8_t *sample, int32_t size)
{
    FALSE_RETURN_V_MSG_E(streamParser_ != nullptr, false, "stream parser is null!");
    return streamParser_->IsSyncFrame(sample, size);
}

bool StreamParserManager::GetColorRange()
{
    FALSE_RETURN_V_MSG_E(streamParser_ != nullptr, false, "stream parser is null!");
    return streamParser_->GetColorRange();
}

uint8_t StreamParserManager::GetColorPrimaries()
{
    FALSE_RETURN_V_MSG_E(streamParser_ != nullptr, 0, "stream parser is null!");
    return streamParser_->GetColorPrimaries();
}

uint8_t StreamParserManager::GetColorTransfer()
{
    FALSE_RETURN_V_MSG_E(streamParser_ != nullptr, 0, "stream parser is null!");
    return streamParser_->GetColorTransfer();
}

uint8_t StreamParserManager::GetColorMatrixCoeff()
{
    FALSE_RETURN_V_MSG_E(streamParser_ != nullptr, 0, "stream parser is null!");
    return streamParser_->GetColorMatrixCoeff();
}

uint8_t StreamParserManager::GetProfileIdc()
{
    FALSE_RETURN_V_MSG_E(streamParser_ != nullptr, 0, "stream parser is null!");
    return streamParser_->GetProfileIdc();
}

uint8_t StreamParserManager::GetLevelIdc()
{
    FALSE_RETURN_V_MSG_E(streamParser_ != nullptr, 0, "stream parser is null!");
    return streamParser_->GetLevelIdc();
}

uint32_t StreamParserManager::GetChromaLocation()
{
    FALSE_RETURN_V_MSG_E(streamParser_ != nullptr, 0, "stream parser is null!");
    return streamParser_->GetChromaLocation();
}

uint32_t StreamParserManager::GetPicWidInLumaSamples()
{
    FALSE_RETURN_V_MSG_E(streamParser_ != nullptr, 0, "stream parser is null!");
    return streamParser_->GetPicWidInLumaSamples();
}

uint32_t StreamParserManager::GetPicHetInLumaSamples()
{
    FALSE_RETURN_V_MSG_E(streamParser_ != nullptr, 0, "stream parser is null!");
    return streamParser_->GetPicHetInLumaSamples();
}

void StreamParserManager::ConvertExtraDataToAnnexb(uint8_t *extraData, int32_t extraDataSize)
{
    FALSE_RETURN_MSG(streamParser_ != nullptr, "stream parser is null!");
    streamParser_->ConvertExtraDataToAnnexb(extraData, extraDataSize);
}

void StreamParserManager::ConvertPacketToAnnexb(uint8_t **hvccPacket, int32_t &hvccPacketSize, uint8_t *sideData,
    size_t sideDataSize, bool isExtradata)
{
    FALSE_RETURN_MSG(streamParser_ != nullptr, "stream parser is null!");
    streamParser_->ConvertPacketToAnnexb(hvccPacket, hvccPacketSize, sideData, sideDataSize, isExtradata);
}

void StreamParserManager::ParseAnnexbExtraData(const uint8_t *sample, int32_t size)
{
    FALSE_RETURN_MSG(streamParser_ != nullptr, "stream parser is null!");
    streamParser_->ParseAnnexbExtraData(sample, size);
}

void StreamParserManager::ResetXPSSendStatus()
{
    FALSE_RETURN_MSG(streamParser_ != nullptr, "stream parser is null!");
    streamParser_->ResetXPSSendStatus();
}

void *StreamParserManager::LoadPluginFile(const std::string &path)
{
    auto ptr = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (ptr == nullptr) {
        MEDIA_LOG_E("dlopen failed due to %{public}s", ::dlerror());
    }
    handler_ = ptr;
    return ptr;
}

bool StreamParserManager::CheckSymbol(void *handler)
{
    if (handler) {
        std::string createFuncName = "CreateStreamParser";
        std::string destroyFuncName = "DestroyStreamParser";
        CreateFunc createFunc = nullptr;
        DestroyFunc destroyFunc = nullptr;
        createFunc = (CreateFunc)(::dlsym(handler, createFuncName.c_str()));
        destroyFunc = (DestroyFunc)(::dlsym(handler, destroyFuncName.c_str()));
        if (createFunc && destroyFunc) {
            MEDIA_LOG_D("CheckSymbol:  createFuncName %{public}s", createFuncName.c_str());
            MEDIA_LOG_D("CheckSymbol:  destroyFuncName %{public}s", destroyFuncName.c_str());
            createFunc_ = createFunc;
            destroyFunc_ = destroyFunc;
            return true;
        }
    }
    return false;
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS