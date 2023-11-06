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

#include "muxer_factory.h"
#include <utility>
#include <algorithm>
#include <dirent.h>
#include "muxer_plugin.h"
#include "avcodec_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MuxerFactory"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
static std::string g_libFileHead = "libav_codec_plugin_";
static std::string g_fileSeparator = "/";
static std::string g_fileMark = "Muxer";
static std::string g_libFileTail = AV_CODEC_PLUGIN_FILE_TAIL;

MuxerFactory::MuxerFactory()
{
    RegisterPlugins();
}

MuxerFactory::~MuxerFactory()
{
    UnregisterAllPlugins();
}

std::shared_ptr<Muxer> MuxerFactory::CreatePlugin(uint32_t outputFormat)
{
    AVCODEC_LOGD("CreatePlugin: outputFormat %{public}d", outputFormat);
    std::string pluginName;
    int32_t maxProb = 0;
    for (auto& name : registerData_->registerNames) {
        std::shared_ptr<PluginRegInfo> regInfo = registerData_->registerTable[name];
        if (regInfo->pluginDef->pluginType == PluginType::MUXER) {
            auto prob = regInfo->pluginDef->sniffer(name, outputFormat);
            if (prob > maxProb) {
                maxProb = prob;
                pluginName = name;
            }
        }
    }
    AVCODEC_LOGD("maxProb %{public}d, pluginName %{public}s", maxProb, pluginName.c_str());
    if (!pluginName.empty()) {
        std::shared_ptr<PluginRegInfo> regInfo = registerData_->registerTable[pluginName];
        auto plugin = regInfo->pluginDef->creator(pluginName);
        return std::shared_ptr<Muxer>(
                new Muxer(regInfo->packageDef->pkgVersion, regInfo->pluginDef->apiVersion, plugin));
    } else {
        AVCODEC_LOGE("No plugins matching output format - %{public}d", outputFormat);
    }
    return nullptr;
}

void MuxerFactory::RegisterPlugins()
{
    RegisterDynamicPlugins(AV_CODEC_PLUGIN_PATH);
}

void MuxerFactory::RegisterDynamicPlugins(const char* libDirPath)
{
    DIR* libDir = opendir(libDirPath);
    if (libDir) {
        struct dirent* lib = nullptr;
        std::shared_ptr<PluginLoader> loader = nullptr;
        while ((lib = readdir(libDir))) {
            if (lib->d_name[0] == '.') {
                continue;
            }
            std::string libName = lib->d_name;
            AVCODEC_LOGD("libName %{public}s", libName.c_str());
            if (libName.find(g_libFileHead) != 0 ||
                libName.find(g_fileMark) == std::string::npos ||
                libName.compare(libName.size() - g_libFileTail.size(), g_libFileTail.size(), g_libFileTail)) {
                continue;
            }
            std::string pluginName =
                libName.substr(g_libFileHead.size(), libName.size() - g_libFileHead.size() - g_libFileTail.size());
            std::string libPath = libDirPath + g_fileSeparator + lib->d_name;
            loader = PluginLoader::Create(pluginName, libPath);
            if (loader) {
                loader->FetchRegisterFunction()(std::make_shared<RegisterImpl>(registerData_, loader));
                registeredLoaders_.push_back(loader);
            }
        }
        closedir(libDir);
    }
}

void MuxerFactory::UnregisterAllPlugins()
{
    for (auto& loader : registeredLoaders_) {
        loader->FetchUnregisterFunction()();
        loader.reset();
    }
    registeredLoaders_.clear();
    registerData_->registerNames.clear();
    registerData_->registerTable.clear();
}

bool MuxerFactory::RegisterData::IsExist(const std::string& name)
{
    return registerTable.find(name) != registerTable.end();
}

Status MuxerFactory::RegisterImpl::AddPlugin(const PluginDefBase& def)
{
    CHECK_AND_RETURN_RET_LOG(Verification(def), Status::ERROR_INVALID_DATA,
        "The plugin type is not muxer, or plugin rank < 0, or plugin rank > 100");
    CHECK_AND_RETURN_RET_LOG(VersionMatched(def), Status::ERROR_UNKNOWN,
        "The plugin version is not matched");
    CHECK_AND_RETURN_RET_LOG(!registerData->IsExist(def.name),
        Status::ERROR_PLUGIN_ALREADY_EXISTS, "The plugin already exists");

    auto& pluginDef = (MuxerPluginDef&)(def);
    std::shared_ptr<PluginRegInfo> regInfo = std::make_shared<PluginRegInfo>();
    regInfo->packageDef = packageDef;
    regInfo->pluginDef = std::make_shared<MuxerPluginDef>(pluginDef);
    regInfo->loader = std::move(pluginLoader);

    registerData->registerTable[pluginDef.name] = regInfo;
    registerData->registerNames.push_back(pluginDef.name);
    return Status::NO_ERROR;
}

Status MuxerFactory::RegisterImpl::AddPackage(const PackageDef& def)
{
    return SetPackageDef(def);
}

Status MuxerFactory::RegisterImpl::SetPackageDef(const PackageDef& def)
{
    packageDef = std::make_shared<PackageDef>(def);
    return Status::NO_ERROR;
}

bool MuxerFactory::RegisterImpl::Verification(const PluginDefBase& def)
{
    if (def.rank < 0 || def.rank > 100) { // 100
        return false;
    }
    return (def.pluginType == PluginType::MUXER);
}

bool MuxerFactory::RegisterImpl::VersionMatched(const PluginDefBase& def)
{
    int major = (def.apiVersion >> 16) & 0xFFFF; // 16
    int minor = def.apiVersion & 0xFFFF;
    uint32_t version = MUXER_API_VERSION;
    int coreMajor = (version >> 16) & 0xFFFF; // 16
    int coreMinor = version & 0xFFFF;
    return (major == coreMajor) && (minor <= coreMinor);
}
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS