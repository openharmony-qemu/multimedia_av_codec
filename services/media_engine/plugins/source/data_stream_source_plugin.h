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

#ifndef HISTREAMER_STREAM_SOURCE_PLUGIN_H
#define HISTREAMER_STREAM_SOURCE_PLUGIN_H

#include "plugin/plugin_buffer.h"
#include "plugin/source_plugin.h"
#include "common/media_data_source.h"
#include "common/avsharedmemorypool.h"

namespace OHOS {
namespace Media {
namespace Plugin {
namespace DataStreamSource {
class DataStreamSourcePlugin : public Plugins::SourcePlugin {
public:
    explicit DataStreamSourcePlugin(std::string name);
    ~DataStreamSourcePlugin() override;

    Status SetSource(std::shared_ptr<Plugins::MediaSource> source) override;
    Status Read(std::shared_ptr<Plugins::Buffer>& buffer, uint64_t offset, size_t expectedLen) override;
    Status GetSize(uint64_t& size) override;
    Plugins::Seekable GetSeekable() override;
    Status SeekTo(uint64_t offset) override;
    Status Reset() override;
    bool IsNeedPreDownload() override;
private:
    std::shared_ptr<Plugins::Buffer> WrapAVSharedMemory(
        const std::shared_ptr<AVSharedMemory>& avSharedMemory, int32_t realLen);
    void InitPool();
    std::shared_ptr<AVSharedMemory> GetMemory();
    void ResetPool();
    Plugins::Seekable seekable_ {Plugins::Seekable::INVALID};
    std::shared_ptr<IMediaDataSource> dataSrc_;
    std::shared_ptr<AVSharedMemoryPool> pool_;
    int64_t size_ {0};
    uint64_t offset_ {0};
    uint32_t retryTimes_ = 0;
};
} // namespace DataStreamSource
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_STREAM_SOURCE_PLUGIN_H
