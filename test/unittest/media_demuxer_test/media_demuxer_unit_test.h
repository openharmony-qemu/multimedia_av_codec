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
    explicit DemuxerPluginMock(std::string name) : DemuxerPlugin(name)
    {
        mapStatus_["StatusOK"] = Status::OK;
        mapStatus_["StatusErrorUnknown"] = Status::ERROR_UNKNOWN;
        mapStatus_["StatusErrorNoMemory"] = Status::ERROR_NO_MEMORY;
        mapStatus_["StatusAgain"] = Status::ERROR_AGAIN;
        mapStatus_["StatusErrorNullPoint"] = Status::ERROR_NULL_POINTER;
        mapStatus_["StatusErrorNotEnoughData"] = Status::ERROR_NOT_ENOUGH_DATA;
        name_ = name;
    }
    ~DemuxerPluginMock()
    {
    }
    Status Reset() override
    {
        return mapStatus_[name_];
    }
    Status Start() override
    {
        return mapStatus_[name_];
    }
    Status Stop() override
    {
        return mapStatus_[name_];
    }
    Status Flush() override
    {
        return mapStatus_[name_];
    }
    Status SetDataSource(const std::shared_ptr<DataSource>& source) override
    {
        return mapStatus_[name_];
    }
    Status SetDataSourceWithProbSize(const std::shared_ptr<DataSource>& source,
        const int32_t probSize) override
    {
        return mapStatus_[name_];
    }
    Status GetMediaInfo(MediaInfo& mediaInfo) override
    {
        return mapStatus_[name_];
    }
    Status GetUserMeta(std::shared_ptr<Meta> meta) override
    {
        return mapStatus_[name_];
    }
    Status SelectTrack(uint32_t trackId) override
    {
        return mapStatus_[name_];
    }
    Status UnselectTrack(uint32_t trackId) override
    {
        return mapStatus_[name_];
    }
    Status SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode,
        int64_t& realSeekTime) override
    {
        return mapStatus_[name_];
    }
    Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample) override
    {
        return mapStatus_[name_];
    }
    Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample, uint32_t timeout) override
    {
        return mapStatus_[name_];
    }
    Status GetNextSampleSize(uint32_t trackId, int32_t& size) override
    {
        return mapStatus_[name_];
    }
    Status GetNextSampleSize(uint32_t trackId, int32_t& size, uint32_t timeout) override
    {
        return mapStatus_[name_];
    }
    Status GetLastPTSByTrackId(uint32_t trackId, int64_t &lastPTS) override
    {
        return mapStatus_[name_];
    }
    Status Pause() override
    {
        return mapStatus_[name_];
    }
    Status GetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo) override
    {
        return mapStatus_[name_];
    }
    void ResetEosStatus() override
    {
        return;
    }
    bool IsRefParserSupported() override
    {
        return false;
    }
    Status ParserRefUpdatePos(int64_t timeStampMs, bool isForward = true) override
    {
        return mapStatus_[name_];
    }
    Status ParserRefInfo() override
    {
        return mapStatus_[name_];
    }
    Status GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample,
        FrameLayerInfo &frameLayerInfo) override
    {
        return mapStatus_[name_];
    }
    Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo) override
    {
        return mapStatus_[name_];
    }
    Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo) override
    {
        return mapStatus_[name_];
    }
    Status GetIFramePos(std::vector<uint32_t> &IFramePos) override
    {
        return mapStatus_[name_];
    }
    Status Dts2FrameId(int64_t dts, uint32_t &frameId) override
    {
        return mapStatus_[name_];
    }
    Status SeekMs2FrameId(int64_t seekMs, uint32_t &frameId) override
    {
        return mapStatus_[name_];
    }
    Status FrameId2SeekMs(uint32_t frameId, int64_t &seekMs) override
    {
        return mapStatus_[name_];
    }
    Status GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
        const uint64_t relativePresentationTimeUs, uint32_t &index) override
    {
        return mapStatus_[name_];
    }
    Status GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
        const uint32_t index, uint64_t &relativePresentationTimeUs) override
    {
        return mapStatus_[name_];
    }
    void SetCacheLimit(uint32_t limitSize) override
    {
        return;
    }
    bool GetProbeSize(int32_t &offset, int32_t &size) override
    {
        offset = 0;
        size = 5000000; // cache for 5000000
        return true;
    }
    Status BoostReadThreadPriority() override
    {
        return mapStatus_[name_];
    }
    Status SetAVReadPacketStopState(bool state) override
    {
        return mapStatus_[name_];
    }
    Status SeekToFirstFrame() override
    {
        return mapStatus_[name_];
    }
private:
    std::map<std::string, Status> mapStatus_;
    std::string name_;
};

template<size_t MaxFailCount>
class DemuxerPluginSetDataSourceFailMock : public DemuxerPluginMock {
public:
    explicit DemuxerPluginSetDataSourceFailMock(std::string name) : DemuxerPluginMock(name)
    {
    }
    ~DemuxerPluginSetDataSourceFailMock()
    {
    }
    Status SetDataSource(const std::shared_ptr<DataSource>& source) override
    {
        if (failCount_ < MaxFailCount) {
            failCount_++;
            return Status::ERROR_NOT_ENOUGH_DATA;
        }
        return Status::OK;
    }
private:
    size_t failCount_ = 0;
};

class StreamDemuxerMock : public StreamDemuxer {
    bool SetSourceInitialBufferSize(int32_t offset, int32_t size) override
    {
        return true;
    }
};

class SourcePluginMock : public Plugins::SourcePlugin {
public:
    explicit SourcePluginMock(std::string name) : SourcePlugin(name)
    {
        mapStatus_["StatusOK"] = Status::OK;
        mapStatus_["StatusErrorUnknown"] = Status::ERROR_UNKNOWN;
        mapStatus_["StatusErrorNoMemory"] = Status::ERROR_NO_MEMORY;
        mapStatus_["StatusAgain"] = Status::ERROR_AGAIN;
        mapStatus_["StatusErrorNullPoint"] = Status::ERROR_NULL_POINTER;
        name_ = name;
    }
    ~SourcePluginMock()
    {
    }
    Status SetSource(std::shared_ptr<MediaSource> source) override
    {
        return mapStatus_[name_];
    }
    Status Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen) override
    {
        return mapStatus_[name_];
    }
    Status Read(int32_t streamId, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen) override
    {
        return mapStatus_[name_];
    }
    Status GetSize(uint64_t& size) override
    {
        return mapStatus_[name_];
    }
    Seekable GetSeekable() override
    {
        return Seekable::SEEKABLE;
    }
    Status SeekTo(uint64_t offset) override
    {
        return mapStatus_[name_];
    }
    Status Reset() override
    {
        return mapStatus_[name_];
    }

private:
    std::map<std::string, Status> mapStatus_;
    std::string name_;
};

class SourceCallback : public Plugins::Callback {
public:
    explicit SourceCallback(std::shared_ptr<DemuxerPluginManager> demuxerPluginManager)
        : demuxerPluginManager_(demuxerPluginManager) {}
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        switch (event.type) {
            case PluginEventType::INITIAL_BUFFER_SUCCESS: {
                demuxerPluginManager_->NotifyInitialBufferingEnd(true);
                break;
            }
            default:
                break;
        }
    }
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager_;
};

template<size_t FailOffset, size_t MaxFailCount>
class StreamDemuxerPullDataFailMock : public StreamDemuxer {
public:
    StreamDemuxerPullDataFailMock() : StreamDemuxer() {}
private:
    Status CallbackReadAt(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
        size_t expectedLen) override
    {
        num += expectedLen;
        if (num > FailOffset && failCount_ < MaxFailCount) {
            failCount_++;
            return Status::ERROR_AGAIN;
        }
        return StreamDemuxer::CallbackReadAt(streamID, offset, buffer, expectedLen);
    }
    size_t failCount_ = 0;
    int num = 0;
};
}
}

#endif