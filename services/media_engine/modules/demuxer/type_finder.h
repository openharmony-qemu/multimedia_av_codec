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

#ifndef TYPE_FINDER_H
#define TYPE_FINDER_H

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include "osal/task/task.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_manager.h"
#include "plugin/plugin_info.h"

namespace OHOS {
namespace Media {
using namespace Plugins;
class TypeFinder : public std::enable_shared_from_this<TypeFinder>, public Plugins::DataSource {
public:
    TypeFinder();

    ~TypeFinder() override;

    void Init(std::string uri, uint64_t mediaDataSize, std::function<bool(uint64_t, size_t)> checkRange,
        std::function<bool(uint64_t, size_t, std::shared_ptr<Buffer>&)> peekRange);

    std::string FindMediaType();

    Status ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen) override;

    Status GetSize(uint64_t& size) override;

    Plugins::Seekable GetSeekable() override;

private:
    std::string SniffMediaType();

    std::string GuessMediaType() const;

    bool IsOffsetValid(int64_t offset) const;

    bool IsSniffNeeded(std::string uri);

    bool GetPlugins();

    void SortPlugins(const std::string& uriSuffix);

    bool sniffNeeded_;
    std::string uri_;
    uint64_t mediaDataSize_;
    std::string pluginName_;
    std::vector<std::shared_ptr<Plugins::PluginInfo>> plugins_;
    std::atomic<bool> pluginRegistryChanged_;
    std::shared_ptr<Task> task_;
    std::function<bool(uint64_t, size_t)> checkRange_;
    std::function<bool(uint64_t, size_t, std::shared_ptr<Buffer>&)> peekRange_;
    std::function<void(std::string)> typeFound_;
};
} // namespace Media
} // namespace OHOS
#endif // TYPE_FINDER_H
