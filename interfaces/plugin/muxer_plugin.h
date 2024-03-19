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

#ifndef AVCODEC_MUXER_PLUGIN_H
#define AVCODEC_MUXER_PLUGIN_H

#include "plugin/plugin_definition.h"
#include "data_sink.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"

namespace OHOS {
namespace Media {
namespace Plugins {
class MuxerPlugin : public PluginBase {
public:
    explicit MuxerPlugin(std::string &&name) : PluginBase(std::move(name)) {}
    virtual Status SetDataSink(const std::shared_ptr<DataSink> &dataSink) = 0;
    virtual Status SetParameter(const std::shared_ptr<Meta> &param) = 0;
    virtual Status SetUserMeta(const std::shared_ptr<Meta> &userMeta) { return Status::ERROR_INVALID_PARAMETER; }
    virtual Status AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc) = 0;
    virtual Status Start() = 0;
    virtual Status WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample) = 0;
    virtual Status Stop() = 0;
    virtual Status Reset() = 0;
};

/// Muxer plugin api major number.
#define MUXER_API_VERSION_MAJOR (1)

/// Muxer plugin api minor number
#define MUXER_API_VERSION_MINOR (0)

/// Muxer plugin version
#define MUXER_API_VERSION MAKE_VERSION(MUXER_API_VERSION_MAJOR, MUXER_API_VERSION_MINOR)


struct MuxerPluginDef : public PluginDefBase {
    MuxerPluginDef() : PluginDefBase()
    {
        apiVersion = MUXER_API_VERSION; ///< Muxer plugin version.
        pluginType = PluginType::MUXER; ///< Plugin type, MUST be MUXER.
    }
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_MUXER_PLUGIN_H