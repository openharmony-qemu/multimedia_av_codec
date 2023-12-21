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
#ifndef DEDEMUXER_PLUGIN_H
#define DEDEMUXER_PLUGIN_H

#include <string>
#include "avcodec_common.h"
#include "plugin_base.h"
#include "plugin_definition.h"
#include "buffer/avbuffer.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
using namespace Media;
struct DemuxerPlugin : public PluginBase {
    explicit DemuxerPlugin() : PluginBase("Demuxer") {}
    virtual int32_t InitWithSource(uintptr_t sourceAddr) = 0;
    virtual int32_t SelectTrackByID(uint32_t index) = 0;
    virtual int32_t UnselectTrackByID(uint32_t index) = 0;
    virtual int32_t ReadSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample) = 0;
    virtual int32_t SeekToTime(int64_t millisecond, AVSeekMode mode) = 0;
    Status SetCallback(Callback* cb)
    {
        (void)cb;
        return Status::ERROR_UNIMPLEMENTED;
    };
};

/// Demuxer plugin api major number.
#define DEMUXER_API_VERSION_MAJOR (1)

/// Demuxer plugin api minor number
#define DEMUXER_API_VERSION_MINOR (0)

/// Demuxer plugin version
#define DEMUXER_API_VERSION MAKE_VERSION(DEMUXER_API_VERSION_MAJOR, DEMUXER_API_VERSION_MINOR)

/// Demuxer create function
using DemuxerPluginCreatorFunc = std::shared_ptr<DemuxerPlugin>(*)();
/// Demuxer sniff function
using DemuxerPluginSnifferFunc = int32_t (*)(const std::string& name);

struct DemuxerPluginDef : public PluginDefBase {
    DemuxerPluginCreatorFunc creator {nullptr}; ///< Demuxer plugin create function.
    DemuxerPluginSnifferFunc sniffer {nullptr}; ///< Demuxer plugin sniff function.
    DemuxerPluginDef()
    {
        apiVersion = DEMUXER_API_VERSION; ///< Demuxer plugin version.
        pluginType = PluginType::DEMUXER; ///< Plugin type, MUST be DEMUXER.
    }
};
} // namepsace Plugin
} // namespace MediaAVCodec
} // namespace OHOS
#endif // DEDEMUXER_PLUGIN_H