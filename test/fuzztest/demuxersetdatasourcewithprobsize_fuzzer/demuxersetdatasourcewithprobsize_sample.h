/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef DEMUXER_SET_DATA_SOURCE_WITH_PROB_SIZE_H
#define DEMUXER_SET_DATA_SOURCE_WITH_PROB_SIZE_H

#include "stream_demuxer.h"
#include "demuxer_plugin_manager.h"
#include "plugin/plugin_manager_v2.h"

namespace OHOS {
namespace Media {
class DemuxerPluginTest {
public:
    bool CreateDataSource(const std::string& filePath);
    bool CreateDemuxerPluginByName(const std::string& typeName, const std::string& filePath, int probSize);
    bool PluginSelectTracks();
    bool PluginReadSample(uint32_t idx, uint32_t& flag);
    bool PluginReadAllSample();
    bool Run(const std::string& typeName, const std::string& filePath, int probSize);
    bool PluginReplay();
    bool PluginSeekToKeyFrame(int32_t trackId, int64_t seekTime, SeekMode mode,
        int64_t& realSeekTime, uint32_t timeoutMs);

    int streamId_ = 0;
    std::vector<uint32_t> selectedTrackIds_;
    std::vector<uint8_t> buffer_;
    int64_t seekTime_ = 0;
    uint32_t timeOutMs_ = 0;

    std::shared_ptr<Media::StreamDemuxer> realStreamDemuxer_{ nullptr };
    std::shared_ptr<Media::MediaSource> mediaSource_{ nullptr };
    std::shared_ptr<Media::Source> realSource_{ nullptr };
    std::shared_ptr<Media::PluginBase> pluginBase_{ nullptr };
    std::shared_ptr<DataSourceImpl> dataSourceImpl_{ nullptr };
};
} // namespace Media
} // namespace OHOS

#endif // DEMUXER_SET_DATA_SOURCE_WITH_PROB_SIZE_H
