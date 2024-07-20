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

#define HST_LOG_TAG "ReferenceParserManager"

#include <unistd.h>
#include <dlfcn.h>
#include "common/log.h"
#include "reference_parser_manager.h"

namespace {
const std::string REFERENCE_LIB_PATH = "libav_codec_reference_parser.z.so";
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "HiStreamer"};
}

namespace OHOS {
namespace Media {
namespace Plugins {
void *ReferenceParserManager::handler_ = nullptr;
ReferenceParserManager::CreateFunc ReferenceParserManager::createFunc_ = nullptr;
ReferenceParserManager::DestroyFunc ReferenceParserManager::destroyFunc_ = nullptr;
std::mutex ReferenceParserManager::mtx_;

ReferenceParserManager::~ReferenceParserManager()
{
    if (referenceParser_) {
        destroyFunc_(referenceParser_);
        referenceParser_ = nullptr;
    }
}

bool ReferenceParserManager::Init()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (!handler_) {
        if (!CheckSymbol(LoadPluginFile(REFERENCE_LIB_PATH))) {
            MEDIA_LOG_E("Load Reference parser so fail");
            return false;
        }
    }
    return true;
}

std::shared_ptr<ReferenceParserManager> ReferenceParserManager::Create(CodecType codecType,
                                                                       std::vector<uint32_t> &IFramePos)
{
    std::shared_ptr<ReferenceParserManager> loader = std::make_shared<ReferenceParserManager>();
    if (!loader->Init()) {
        return nullptr;
    }
    loader->referenceParser_ = loader->createFunc_(codecType, IFramePos);
    if (!loader->referenceParser_) {
        MEDIA_LOG_E("createFunc_ fail");
        return nullptr;
    }
    return loader;
}

Status ReferenceParserManager::ParserNalUnits(uint8_t *nalData, int32_t nalDataSize, uint32_t frameId, int64_t dts)
{
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "reference parser is null!");
    return referenceParser_->ParserNalUnits(nalData, nalDataSize, frameId, dts);
}

Status ReferenceParserManager::ParserExtraData(uint8_t *extraData, int32_t extraDataSize)
{
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "reference parser is null!");
    return referenceParser_->ParserExtraData(extraData, extraDataSize);
}

Status ReferenceParserManager::ParserSdtpData(uint8_t *sdtpData, int32_t sdtpDataSize)
{
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "reference parser is null!");
    return referenceParser_->ParserSdtpData(sdtpData, sdtpDataSize);
}

Status ReferenceParserManager::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "reference parser is null!");
    return referenceParser_->GetFrameLayerInfo(frameId, frameLayerInfo);
}

Status ReferenceParserManager::GetFrameLayerInfo(int64_t dts, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "reference parser is null!");
    return referenceParser_->GetFrameLayerInfo(dts, frameLayerInfo);
}

Status ReferenceParserManager::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "reference parser is null!");
    return referenceParser_->GetGopLayerInfo(gopId, gopLayerInfo);
}

void *ReferenceParserManager::LoadPluginFile(const std::string &path)
{
    FALSE_RETURN_V_MSG_E(path.length() > 0, nullptr, "path is invalid.");
    auto ptr = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (ptr == nullptr) {
        MEDIA_LOG_E("dlopen failed due to %{public}s", ::dlerror());
    }
    handler_ = ptr;
    return ptr;
}

bool ReferenceParserManager::CheckSymbol(void *handler)
{
    if (handler) {
        std::string createFuncName = "CreateRefParser";
        std::string destroyFuncName = "DestroyRefParser";
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