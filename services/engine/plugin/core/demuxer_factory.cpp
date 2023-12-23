/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <iostream>
#include <utility>
#include <algorithm>
#include <dirent.h>
#include "type_cast_ext.h"
#include "avcodec_log.h"
#include "avcodec_trace.h"
#include "avcodec_errors.h"
#include "demuxer_factory.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "DemuxerFactory"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
static std::string g_libFileHead = "libav_codec_plugin_";
static std::string g_fileSeparator = "/";
static std::string g_fileMark = "Demuxer";
static std::string g_libFileTail = AV_CODEC_PLUGIN_FILE_TAIL;

DemuxerFactory::DemuxerFactory()
{
    RegisterPlugins();
}

DemuxerFactory::~DemuxerFactory()
{
    UnregisterAllPlugins();
}

std::shared_ptr<Demuxer> DemuxerFactory::CreatePlugin(uintptr_t sourceAddr)
{
    AVCODEC_LOGD("create demuxer plugin from DemuxerFactory");
    std::string pluginName;
    uint32_t maxRank = 0;
    for (auto& name : registerData_->registerNames) {
        std::shared_ptr<PluginRegInfo> regInfo = registerData_->registerTable[name];
        if (regInfo->info->pluginType == PluginType::DEMUXER) {
            auto rank = regInfo->info->rank;
            if (rank > maxRank) {
                maxRank = rank;
                pluginName = name;
            }
        } else {
            AVCODEC_LOGW("Create demuxer plugin failed, because get plugin type: %{public}d", PluginType::DEMUXER);
        }
    }

    if (!pluginName.empty()) {
        std::shared_ptr<PluginRegInfo> regInfo = registerData_->registerTable[pluginName];
        auto plugin = ReinterpretPointerCast<DemuxerPlugin>(regInfo->creator());
        int32_t ret = plugin->InitWithSource(sourceAddr);
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGE("Create demuxer plugin failed, cannot create plugin!");
            return nullptr;
        }
        AVCODEC_LOGD("create demuxer plugin successful! pluginName %{public}s(maxRank %{public}d)",
                     pluginName.c_str(), maxRank);
        return std::shared_ptr<Demuxer>(
                new Demuxer(regInfo->packageDef->pkgVersion, regInfo->info->apiVersion, plugin));
    } else {
        AVCODEC_LOGW("Create demuxer plugin failed, can not find any demuxer plugins!");
    }
    return nullptr;
}

void DemuxerFactory::RegisterPlugins()
{
    RegisterDynamicPlugins(AV_CODEC_PLUGIN_PATH);
}

void DemuxerFactory::RegisterDynamicPlugins(const char* libDirPath)
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
            if (libName.find(g_libFileHead) || libName.find(g_fileMark) == std::string::npos ||
                libName.compare(libName.size() - g_libFileTail.size(), g_libFileTail.size(), g_libFileTail)) {
                continue;
            }
            AVCODEC_LOGD("Regist dyanmic plugin: libName %{public}s", libName.c_str());

            std::string pluginName =
                libName.substr(g_libFileHead.size(), libName.size() - g_libFileHead.size() - g_libFileTail.size());
            std::string libPath = libDirPath + g_fileSeparator + lib->d_name;
            loader = PluginLoader::Create(pluginName, libPath);
            if (loader) {
                AVCODEC_LOGI("Create pluginLoader successful: %{public}s!", libName.c_str());
                loader->FetchRegisterFunction()(std::make_shared<RegisterImpl>(registerData_, loader));
                registeredLoaders_.push_back(loader);
            } else {
                AVCODEC_LOGE("Create pluginLoader failed!");
            }
        }
        closedir(libDir);
    }
}

void DemuxerFactory::UnregisterAllPlugins()
{
    for (auto& loader : registeredLoaders_) {
        loader->FetchUnregisterFunction()();
        loader.reset();
    }
    registeredLoaders_.clear();
    registerData_->registerNames.clear();
    registerData_->registerTable.clear();
}

bool DemuxerFactory::RegisterData::IsPluginExist(const std::string& name)
{
    return registerTable.find(name) != registerTable.end();
}

Status DemuxerFactory::RegisterImpl::AddPlugin(const PluginDefBase& def)
{
    if (!Verification(def)) {
        return Status::ERROR_INVALID_DATA;
    }
    if (!VersionMatched(def)) {
        return Status::ERROR_UNKNOWN;
    }
    if (registerData->IsPluginExist(def.name)) {
        return Status::ERROR_PLUGIN_ALREADY_EXISTS;
    }
    auto& pluginDef = (DemuxerPluginDef&)def;
    std::shared_ptr<PluginRegInfo> regInfo = std::make_shared<PluginRegInfo>();
    regInfo->packageDef = packageDef;
    regInfo->creator = reinterpret_cast<DemuxerPluginCreatorFunc>(pluginDef.creator);
    regInfo->sniffer = reinterpret_cast<DemuxerPluginSnifferFunc>(pluginDef.sniffer);
    
    std::shared_ptr<PluginInfo> info = std::make_shared<PluginInfo>();
    info->apiVersion = pluginDef.apiVersion;
    info->pluginType = pluginDef.pluginType;
    info->name = pluginDef.name;
    info->description = pluginDef.description;
    info->rank = pluginDef.rank;
    
    regInfo->info = info;
    regInfo->loader = std::move(pluginLoader);

    registerData->registerTable[pluginDef.name] = regInfo;
    registerData->registerNames.push_back(pluginDef.name);
    return Status::NO_ERROR;
}

Status DemuxerFactory::RegisterImpl::AddPackage(const PackageDef& def)
{
    return SetPackageDef(def);
}

Status DemuxerFactory::RegisterImpl::SetPackageDef(const PackageDef& def)
{
    packageDef = std::make_shared<PackageDef>(def);
    return Status::NO_ERROR;
}

bool DemuxerFactory::RegisterImpl::Verification(const PluginDefBase& definition)
{
    if (definition.rank < 0 || definition.rank > 100) { // 100
        return false;
    }
    return (definition.pluginType != PluginType::INVALID_TYPE);
}

bool DemuxerFactory::RegisterImpl::VersionMatched(const PluginDefBase& definition)
{
    int major = (definition.apiVersion >> 16) & 0xFFFF; // 16
    int minor = definition.apiVersion & 0xFFFF;
    uint32_t version = DEMUXER_API_VERSION;
    int coreMajor = (version >> 16) & 0xFFFF; // 16
    int coreMinor = version & 0xFFFF;
    return (major == coreMajor) && (minor <= coreMinor);
}
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS