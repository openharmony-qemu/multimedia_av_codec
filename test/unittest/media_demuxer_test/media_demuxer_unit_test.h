/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef MEDIA_DEMUXER_UNIT_TEST_H
#define MEDIA_DEMUXER_UNIT_TEST_H

#include "media_demuxer.h"
#include "demuxer_plugin_manager.h"
#include "stream_demuxer.h"
#include "gtest/gtest.h"
#include "source/source.h"
#include "common/media_source.h"
#include "buffer/avbuffer_queue.h"

namespace OHOS {
namespace Media {
class MediaDemuxerUnitTest : public testing::Test {
public:

    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

class DemuxerPluginMock : public Plugins::DemuxerPlugin {
public:
    explicit DemuxerPluginMock(std::string name) : DemuxerPlugin(std::move(name)) {
        
    }
    ~DemuxerPluginMock() override {

    }
    virtual Status Reset() override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status Start() override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status Stop() override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status Flush() override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status SetDataSource(const std::shared_ptr<DataSource>& source) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status GetMediaInfo(MediaInfo& mediaInfo) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status GetUserMeta(std::shared_ptr<Meta> meta) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status SelectTrack(uint32_t trackId) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status UnselectTrack(uint32_t trackId) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode, int64_t& realSeekTime) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status GetNextSampleSize(uint32_t trackId, int32_t& size) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status GetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual void ResetEosStatus() override {
        return;
    }
    virtual Status ParserRefUpdatePos(int64_t timeStampMs, bool isForward = true) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status ParserRefInfo() override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status GetIFramePos(std::vector<uint32_t> &IFramePos) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status Dts2FrameId(int64_t dts, uint32_t &frameId, bool offset = true) override {
        return Status::ERROR_UNKNOWN;
    }
    virtual Status GetFrameIndexByPresentationTimeUs(uint32_t trackIndex,
        int64_t presentationTimeUs, uint32_t &frameIndex) override {
            return Status::ERROR_UNKNOWN;
        }
   virtual  Status GetPresentationTimeUsByFrameIndex(uint32_t trackIndex,
        uint32_t frameIndex, int64_t &presentationTimeUs) override {
            return Status::ERROR_UNKNOWN;
        }
};

}
}

#endif