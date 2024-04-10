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

#include "plugin_loader.h"
#include <dlfcn.h>
#include "avcodec_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER_MUXER, "PluginLoader"};
}

using namespace OHOS::MediaAVCodec::Plugin;

// NOLINTNEXTLINE: void*
PluginLoader::PluginLoader(void* handler, std::string name) : handler_(handler), name_(std::move(name))
{
}

PluginLoader::~PluginLoader()
{
    UnLoadPluginFile();
}

std::shared_ptr<PluginLoader> PluginLoader::Create(const std::string& name, const std::string& path)
{
    if (name.empty() || path.empty()) {
        return {};
    }
    return CheckSymbol(LoadPluginFile(path), name);
}

RegisterFunc PluginLoader::FetchRegisterFunction() const
{
    return registerFunc_;
}

UnregisterFunc PluginLoader::FetchUnregisterFunction() const
{
    return unregisterFunc_;
}

void* PluginLoader::LoadPluginFile(const std::string& path)
{
    auto ptr = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (ptr == nullptr) {
        AVCODEC_LOGE("dlopen failed due to %{public}s", ::dlerror());
    }
    return ptr;
}

std::shared_ptr<PluginLoader> PluginLoader::CheckSymbol(void* handler, const std::string& name)
{
    if (handler) {
        std::string registerFuncName = "register_" + name;
        std::string unregisterFuncName = "unregister_" + name;
        RegisterFunc registerFunc = nullptr;
        UnregisterFunc unregisterFunc = nullptr;
        AVCODEC_LOGD("CheckSymbol:  registerFuncName %{public}s", registerFuncName.c_str());
        AVCODEC_LOGD("CheckSymbol:  unregisterFuncName %{public}s", unregisterFuncName.c_str());
        registerFunc = (RegisterFunc)(::dlsym(handler, registerFuncName.c_str()));
        unregisterFunc = (UnregisterFunc)(::dlsym(handler, unregisterFuncName.c_str()));
        if (registerFunc && unregisterFunc) {
            std::shared_ptr<PluginLoader> loader = std::make_shared<PluginLoader>(handler, name);
            loader->registerFunc_ = registerFunc;
            loader->unregisterFunc_ = unregisterFunc;
            return loader;
        }
    }
    return {};
}

void PluginLoader::UnLoadPluginFile()
{
    if (handler_) {
        ::dlclose(const_cast<void*>(handler_)); // NOLINT: const_cast
    }
}
