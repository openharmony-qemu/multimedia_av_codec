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

#ifndef PLUGIN_INTF_MUXER_PLUGIN_H
#define PLUGIN_INTF_MUXER_PLUGIN_H

#include "media_description.h"
#include "av_common.h"
#include "avcodec_common.h"
#include "plugin_base.h"
#include "plugin_definition.h"
#include "data_sink.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
struct MuxerPlugin : public PluginBase {
    explicit MuxerPlugin(std::string &&name) : PluginBase(std::move(name)) {}
    virtual Status SetDataSink(const std::shared_ptr<DataSink>& dataSink) = 0;
    virtual Status SetRotation(int32_t rotation) = 0;
    virtual Status AddTrack(int32_t &trackIndex, const MediaDescription &trackDesc) = 0;
    virtual Status Start() = 0;
    virtual Status WriteSample(uint32_t trackIndex, const uint8_t *sample,
        AVCodecBufferInfo info, AVCodecBufferFlag flag) = 0;
    virtual Status Stop() = 0;
    Status SetCallback(Callback* cb)
    {
        (void)cb;
        return Status::ERROR_UNIMPLEMENTED;
    }
};

/// Muxer plugin api major number.
#define MUXER_API_VERSION_MAJOR (1)

/// Muxer plugin api minor number
#define MUXER_API_VERSION_MINOR (0)

/// Muxer plugin version
#define MUXER_API_VERSION MAKE_VERSION(MUXER_API_VERSION_MAJOR, MUXER_API_VERSION_MINOR)

/// Muxer create function
using MuxerPluginCreatorFunc = std::shared_ptr<MuxerPlugin>(*)(const std::string& name);
/// Muxer sniff function
using MuxerPluginSnifferFunc = int32_t (*)(const std::string& name, uint32_t outputFormat);

struct MuxerPluginDef : public PluginDefBase {
    MuxerPluginCreatorFunc creator {nullptr}; ///< Muxer plugin create function.
    MuxerPluginSnifferFunc sniffer {nullptr}; ///< Muxer plugin sniff function.
    MuxerPluginDef()
    {
        apiVersion = MUXER_API_VERSION; ///< Muxer plugin version.
        pluginType = PluginType::MUXER; ///< Plugin type, MUST be MUXER.
    }
};
} // Plugin
} // MediaAVCodec
} // OHOS
#endif // PLUGIN_INTF_MUXER_PLUGIN_H
