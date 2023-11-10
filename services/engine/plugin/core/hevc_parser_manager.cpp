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

#include "hevc_parser_manager.h"
#include <dlfcn.h>
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HevcParserManager"};
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PLUGIN_PATH) + "/libav_codec_plugin_HevcParser.z.so";
}

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
HevcParserManager::HevcParserManager(void *handler) : handler_(handler) {}

HevcParserManager::~HevcParserManager()
{
    UnLoadPluginFile();
}

std::shared_ptr<HevcParserManager> HevcParserManager::Create()
{
    if (HEVC_LIB_PATH.empty()) {
        return {};
    }

    return CheckSymbol(LoadPluginFile(HEVC_LIB_PATH));
}

void HevcParserManager::ParseExtraData(const uint8_t *sample, int32_t size,
                                       uint8_t **extraDataBuf, int32_t *extraDataSize)
{
    if (!hevcParser_) {
        return;
    }

    hevcParser_->ParseExtraData(sample, size, extraDataBuf, extraDataSize);
}

bool HevcParserManager::IsHdrVivid()
{
    if (!hevcParser_) {
        return false;
    }

    return hevcParser_->IsHdrVivid();
}

bool HevcParserManager::GetColorRange()
{
    if (!hevcParser_) {
        return false;
    }

    return hevcParser_->GetColorRange();
}

uint8_t HevcParserManager::GetColorPrimaries()
{
    if (!hevcParser_) {
        return 0;
    }

    return hevcParser_->GetColorPrimaries();
}

uint8_t HevcParserManager::GetColorTransfer()
{
    if (!hevcParser_) {
        return 0;
    }

    return hevcParser_->GetColorTransfer();
}

uint8_t HevcParserManager::GetColorMatrixCoeff()
{
    if (!hevcParser_) {
        return 0;
    }

    return hevcParser_->GetColorMatrixCoeff();
}

void *HevcParserManager::LoadPluginFile(const std::string &path)
{
    auto ptr = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (ptr == nullptr) {
        AVCODEC_LOGE("dlopen failed due to %{public}s", ::dlerror());
    }
    return ptr;
}

std::shared_ptr<HevcParserManager> HevcParserManager::CheckSymbol(void *handler)
{
    if (handler) {
        std::string createFuncName = "CreateHevcParser";
        std::string destroyFuncName = "DestroyHevcParser";
        CreateFunc createFunc = nullptr;
        DestroyFunc destroyFunc = nullptr;
        createFunc = (CreateFunc)(::dlsym(handler, createFuncName.c_str()));
        destroyFunc = (DestroyFunc)(::dlsym(handler, destroyFuncName.c_str()));
        if (createFunc && destroyFunc) {
            AVCODEC_LOGD("CheckSymbol:  createFuncName %{public}s", createFuncName.c_str());
            AVCODEC_LOGD("CheckSymbol:  destroyFuncName %{public}s", destroyFuncName.c_str());
            std::shared_ptr<HevcParserManager> loader = std::make_shared<HevcParserManager>(handler);
            loader->createFunc_ = createFunc;
            loader->destroyFunc_ = destroyFunc;
            loader->hevcParser_ = loader->createFunc_();
            return loader;
        }
    }
    return {};
}

void HevcParserManager::UnLoadPluginFile()
{
    if (handler_) {
        if (hevcParser_) {
            destroyFunc_(hevcParser_);
            hevcParser_ = nullptr;
        }
        ::dlclose(const_cast<void *>(handler_));
    }
}
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS