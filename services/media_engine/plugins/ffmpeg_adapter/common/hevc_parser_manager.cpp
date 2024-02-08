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

#define HST_LOG_TAG "HevcParserManager"

#include "hevc_parser_manager.h"
#include <dlfcn.h>
#include "common/log.h"

namespace {
const std::string HEVC_LIB_PATH = "libav_codec_hevc_parser.z.so";
}

namespace OHOS {
namespace Media {
namespace Plugins {
void *HevcParserManager::handler_ = nullptr;
HevcParserManager::CreateFunc HevcParserManager::createFunc_ = nullptr;
HevcParserManager::DestroyFunc HevcParserManager::destroyFunc_ = nullptr;
std::mutex HevcParserManager::mtx_;

HevcParserManager::~HevcParserManager()
{
    if (hevcParser_) {
        destroyFunc_(hevcParser_);
        hevcParser_ = nullptr;
    }
}

bool HevcParserManager::Init()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (!handler_) {
        if (!CheckSymbol(LoadPluginFile(HEVC_LIB_PATH))) {
            MEDIA_LOG_E("Load .so fail");
            return false;
        }
    }
    return true;
}

std::shared_ptr<HevcParserManager> HevcParserManager::Create()
{
    std::shared_ptr<HevcParserManager> loader = std::make_shared<HevcParserManager>();
    if (!loader->Init()) {
        return nullptr;
    }
    loader->hevcParser_ = loader->createFunc_();
    if (!loader->hevcParser_) {
        MEDIA_LOG_E("createFunc_ fail");
        return nullptr;
    }
    return loader;
}

void HevcParserManager::ParseExtraData(const uint8_t *sample, int32_t size,
    uint8_t **extraDataBuf, int32_t *extraDataSize)
{
    FALSE_RETURN_MSG(hevcParser_ != nullptr, "hevc parser is null!");
    hevcParser_->ParseExtraData(sample, size, extraDataBuf, extraDataSize);
}

bool HevcParserManager::IsHdrVivid()
{
    FALSE_RETURN_V_MSG_E(hevcParser_ != nullptr, false, "hevc parser is null!");
    return hevcParser_->IsHdrVivid();
}

bool HevcParserManager::GetColorRange()
{
    FALSE_RETURN_V_MSG_E(hevcParser_ != nullptr, false, "hevc parser is null!");
    return hevcParser_->GetColorRange();
}

uint8_t HevcParserManager::GetColorPrimaries()
{
    FALSE_RETURN_V_MSG_E(hevcParser_ != nullptr, 0, "hevc parser is null!");
    return hevcParser_->GetColorPrimaries();
}

uint8_t HevcParserManager::GetColorTransfer()
{
    FALSE_RETURN_V_MSG_E(hevcParser_ != nullptr, 0, "hevc parser is null!");
    return hevcParser_->GetColorTransfer();
}

uint8_t HevcParserManager::GetColorMatrixCoeff()
{
    FALSE_RETURN_V_MSG_E(hevcParser_ != nullptr, 0, "hevc parser is null!");
    return hevcParser_->GetColorMatrixCoeff();
}

uint8_t HevcParserManager::GetProfileIdc()
{
    FALSE_RETURN_V_MSG_E(hevcParser_ != nullptr, 0, "hevc parser is null!");
    return hevcParser_->GetProfileIdc();
}

uint8_t HevcParserManager::GetLevelIdc()
{
    FALSE_RETURN_V_MSG_E(hevcParser_ != nullptr, 0, "hevc parser is null!");
    return hevcParser_->GetLevelIdc();
}

uint32_t HevcParserManager::GetChromaLocation()
{
    FALSE_RETURN_V_MSG_E(hevcParser_ != nullptr, 0, "hevc parser is null!");
    return hevcParser_->GetChromaLocation();
}

uint32_t HevcParserManager::GetPicWidInLumaSamples()
{
    FALSE_RETURN_V_MSG_E(hevcParser_ != nullptr, 0, "hevc parser is null!");
    return hevcParser_->GetPicWidInLumaSamples();
}

uint32_t HevcParserManager::GetPicHetInLumaSamples()
{
    FALSE_RETURN_V_MSG_E(hevcParser_ != nullptr, 0, "hevc parser is null!");
    return hevcParser_->GetPicHetInLumaSamples();
}

void HevcParserManager::ConvertExtraDataToAnnexb(uint8_t *extraData, int32_t extraDataSize)
{
    FALSE_RETURN_MSG(hevcParser_ != nullptr, "hevc parser is null!");
    hevcParser_->ConvertExtraDataToAnnexb(extraData, extraDataSize);
}

void HevcParserManager::ConvertPacketToAnnexb(uint8_t **hvccPacket, int32_t &hvccPacketSize)
{
    FALSE_RETURN_MSG(hevcParser_ != nullptr, "hevc parser is null!");
    hevcParser_->ConvertPacketToAnnexb(hvccPacket, hvccPacketSize);
}

void HevcParserManager::ParseAnnexbExtraData(const uint8_t *sample, int32_t size)
{
    FALSE_RETURN_MSG(hevcParser_ != nullptr, "hevc parser is null!");
    hevcParser_->ParseAnnexbExtraData(sample, size);
}

void HevcParserManager::ResetXPSSendStatus()
{
    FALSE_RETURN_MSG(hevcParser_ != nullptr, "hevc parser is null!");
    hevcParser_->ResetXPSSendStatus();
}

void *HevcParserManager::LoadPluginFile(const std::string &path)
{
    auto ptr = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (ptr == nullptr) {
        MEDIA_LOG_E("dlopen failed due to %{public}s", ::dlerror());
    }
    handler_ = ptr;
    return ptr;
}

bool HevcParserManager::CheckSymbol(void *handler)
{
    if (handler) {
        std::string createFuncName = "CreateHevcParser";
        std::string destroyFuncName = "DestroyHevcParser";
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