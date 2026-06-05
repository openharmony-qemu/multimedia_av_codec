/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MPEG4_DEMUXER_PLUGIN_SAMPLE_H
#define MPEG4_DEMUXER_PLUGIN_SAMPLE_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "buffer/avbuffer.h"
#include "common/media_source.h"
#include "demuxer_plugin_manager.h"
#include "mpeg4_demuxer_plugin.h"
#include "stream_demuxer.h"

namespace OHOS {
namespace Media {

struct Mpeg4DemuxerPluginFuzzConfig
{
    bool useEmptyBuffer = false;
    bool useConfigs = false;
    bool interruptState = false;
    bool stopReadPacketState = false;
    uint32_t invalidTrackId = 0;
    uint32_t timeoutMs = 0;
    uint32_t cacheLimit = 0;
    uint32_t trackCacheLimit = 0;
    uint32_t trackCacheWindowMs = 0;
    uint32_t frameId = 0;
    uint32_t gopId = 0;
    uint64_t relativePresentationTimeUs = 0;
    int32_t probSize = 0;
    int64_t seekTimeMs = 0;
    int64_t secondSeekTimeMs = 0;
    int64_t readTimeoutMs = 0;
    int64_t dts = 0;
};

class Mpeg4AVBufferWrapper
{
public:
    explicit Mpeg4AVBufferWrapper(uint32_t capacity, bool useEmptyBuffer);
    std::shared_ptr<AVBuffer> GetBuffer() const;

private:
    std::vector<uint8_t> data_;
    std::shared_ptr<AVBuffer> buffer_ = nullptr;
};

class Mpeg4DemuxerPluginFuzzTest
{
public:
    Mpeg4DemuxerPluginFuzzTest();
    ~Mpeg4DemuxerPluginFuzzTest();

    bool InitWithData(const uint8_t *data, size_t size);
    void Run(const Mpeg4DemuxerPluginFuzzConfig &config);

private:
    using MPEG4DemuxerPlugin = Plugins::MPEG4::MPEG4DemuxerPlugin;
    using MediaInfo = Plugins::MediaInfo;

    std::shared_ptr<MPEG4DemuxerPlugin> CreatePlugin() const;
    std::shared_ptr<DataSourceImpl> CreateDataSource();
    void RunPreInitInterfaceFuzz(std::shared_ptr<MPEG4DemuxerPlugin> plugin,
                                 const Mpeg4DemuxerPluginFuzzConfig &config);
    void RunPostInitInterfaceFuzz(std::shared_ptr<MPEG4DemuxerPlugin> plugin,
                                  const Mpeg4DemuxerPluginFuzzConfig &config);
    void RunTrackInterfaceFuzz(std::shared_ptr<MPEG4DemuxerPlugin> plugin,
                               const MediaInfo &mediaInfo,
                               const Mpeg4DemuxerPluginFuzzConfig &config,
                               Mpeg4AVBufferWrapper &buffer);
    void RunSeekInterfaceFuzz(std::shared_ptr<MPEG4DemuxerPlugin> plugin,
                              const std::vector<uint32_t> &trackIds,
                              const Mpeg4DemuxerPluginFuzzConfig &config);
    void RunCacheInterfaceFuzz(std::shared_ptr<MPEG4DemuxerPlugin> plugin,
                               const std::vector<uint32_t> &trackIds,
                               const Mpeg4DemuxerPluginFuzzConfig &config);
    void RunReferenceParserInterfaceFuzz(std::shared_ptr<MPEG4DemuxerPlugin> plugin,
                                         const Mpeg4DemuxerPluginFuzzConfig &config,
                                         Mpeg4AVBufferWrapper &buffer);
    void RunConcurrentInterfaceFuzz(std::shared_ptr<MPEG4DemuxerPlugin> plugin,
                                    const std::vector<uint32_t> &trackIds,
                                    const Mpeg4DemuxerPluginFuzzConfig &config);

    const char *testFilePath_ = "/data/test/demuxerpluginmpeg4.mp4";
    int32_t fd_ = -1;
    size_t dataSize_ = 0;
    std::shared_ptr<DataSourceImpl> dataSource_ = nullptr;
};

} // namespace Media
} // namespace OHOS

#endif // MPEG4_DEMUXER_PLUGIN_SAMPLE_H
