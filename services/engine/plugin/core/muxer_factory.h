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

#ifndef MUXER_FACTORY_H
#define MUXER_FACTORY_H

#include <map>
#include <vector>
#include "muxer.h"
#include "plugin_loader.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
class MuxerFactory {
public:
    MuxerFactory(const MuxerFactory&) = delete;
    MuxerFactory operator=(const MuxerFactory&) = delete;
    ~MuxerFactory();
    static MuxerFactory& Instance()
    {
        static MuxerFactory impl;
        return impl;
    }

    std::shared_ptr<Muxer> CreatePlugin(uint32_t outputFormat);

private:
    MuxerFactory();

    void RegisterPlugins();
    void RegisterDynamicPlugins(const char* libDirPath);
    void UnregisterAllPlugins();

private:
    struct PluginRegInfo {
        std::shared_ptr<PackageDef> packageDef;
        std::shared_ptr<MuxerPluginDef> pluginDef;
        std::shared_ptr<PluginLoader> loader;
    };
    struct RegisterData {
        std::vector<std::string> registerNames;
        std::map<std::string, std::shared_ptr<PluginRegInfo>> registerTable;
        bool IsExist(const std::string& name);
    };
    struct RegisterImpl : PackageRegister {
        explicit RegisterImpl(std::shared_ptr<RegisterData> data, std::shared_ptr<PluginLoader> loader = nullptr)
            : pluginLoader(std::move(loader)), registerData(std::move(data)) {}

        ~RegisterImpl() override = default;

        Status AddPlugin(const PluginDefBase& def) override;
        Status AddPackage(const PackageDef& def) override;
        Status SetPackageDef(const PackageDef& def);
        bool Verification(const PluginDefBase& def);
        bool VersionMatched(const PluginDefBase& def);

        std::shared_ptr<PluginLoader> pluginLoader;
        std::shared_ptr<RegisterData> registerData;
        std::shared_ptr<PackageDef> packageDef {nullptr};
    };

    std::shared_ptr<RegisterData> registerData_ = std::make_shared<RegisterData>();
    std::vector<std::shared_ptr<PluginLoader>> registeredLoaders_;
};
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MUXER_FACTORY_H
