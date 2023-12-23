/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef PLUGINS_CODEC_PLUGIN_H
#define PLUGINS_CODEC_PLUGIN_H

#include <vector>
#include <memory>
#include "meta/media_types.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_base.h"
#include "plugin/plugin_event.h"
#include "plugin/plugin_definition.h"
#include "meta/meta.h"
#include "common/status.h"

namespace OHOS {
namespace Media {
namespace Plugins {
class DataCallback {
public:
    virtual ~DataCallback() = default;

    virtual void OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer) = 0;

    virtual void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) = 0;

    virtual void OnEvent(const std::shared_ptr<Plugins::PluginEvent> event) = 0;
};

class CodecPlugin : public Plugins::PluginBase {
public:
    explicit CodecPlugin(std::string name) : PluginBase(std::move(name)) {}
    virtual Status GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers) = 0;

    virtual Status GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers) = 0;

    virtual Status QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer) = 0;

    virtual Status QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer) = 0;

    virtual Status SetParameter(const std::shared_ptr<Meta> &parameter) = 0;

    virtual Status GetParameter(std::shared_ptr<Meta> &parameter) = 0;

    virtual Status Start() = 0;

    virtual Status Stop() = 0;

    virtual Status Flush() = 0;

    virtual Status Reset() = 0;

    virtual Status Release() = 0;

    virtual Status SetDataCallback(DataCallback *dataCallback) = 0;
};

/// Codec plugin api major number.
#define CODEC_API_VERSION_MAJOR (1)

/// Codec plugin api minor number
#define CODEC_API_VERSION_MINOR (0)

/// Codec plugin version
#define CODEC_API_VERSION MAKE_VERSION(CODEC_API_VERSION_MAJOR, CODEC_API_VERSION_MINOR)

/**
 * @brief Describes the codec plugin information.
 *
 * @since 1.0
 * @version 1.0
 */
struct CodecPluginDef : public PluginDefBase {
    CodecPluginDef()
    {
        apiVersion = CODEC_API_VERSION;         ///< Codec plugin version
        pluginType = PluginType::AUDIO_DECODER; ///< Plugin type, MUST be AUDIO_DECODER.
    }
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // PLUGINS_MEDIA_CODEC_PLUGIN_H
