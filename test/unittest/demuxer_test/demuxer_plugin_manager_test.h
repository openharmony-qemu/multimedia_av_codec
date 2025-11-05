/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DEMUXER_PLUGIN_MANAGER_UNIT_TEST_H
#define DEMUXER_PLUGIN_MANAGER_UNIT_TEST_H
#include "mock/mock_base_stream_demuxer.h"
#include "gtest/gtest.h"
#include "demuxer_plugin_manager.h"
#include "stream_demuxer.h"
#include "plugin/plugin_manager_v2.h"

namespace OHOS {
namespace Media {
class DemuxerPluginManagerUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

protected:
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager_{ nullptr };
    std::shared_ptr<DataSourceImpl> dataSourceImpl_{ nullptr };
    std::shared_ptr<MockBaseStreamDemuxer> streamDemuxer_{ nullptr };

private:
    bool CreateDataSource(const std::string& filePath);
    bool CreateDemuxerPluginByName(const std::string& typeName, const std::string& filePath, int probSize);
    bool PluginSelectTracks();
    bool PluginReadSample(uint32_t idx, uint32_t& flag);
    void CountFrames(uint32_t index, uint32_t flag);
    void SetEosValue();
    bool isEOS(std::map<uint32_t, bool>& countFlag);
    void RemoveValue();
    bool ResultAssert(uint32_t frames0, uint32_t frames1, uint32_t keyFrames0, uint32_t keyFrames1);
    bool PluginReadAllSample();

    int streamId_ = 0;
    std::map<uint32_t, uint32_t> frames_;
    std::map<uint32_t, uint32_t> keyFrames_;
    std::map<uint32_t, bool> eosFlag_;
    std::vector<uint32_t> selectedTrackIds_;
    std::vector<uint8_t> buffer_;

    std::shared_ptr<Media::StreamDemuxer> realStreamDemuxer_{ nullptr };
    std::shared_ptr<Media::MediaSource> mediaSource_{ nullptr };
    std::shared_ptr<Media::Source> realSource_{ nullptr };
    std::shared_ptr<Media::PluginBase> pluginBase_{ nullptr };
    std::shared_ptr<Media::DemuxerPlugin> demuxerPlugin_{ nullptr };
};
}  // namespace Media
}  // namespace OHOS
#endif