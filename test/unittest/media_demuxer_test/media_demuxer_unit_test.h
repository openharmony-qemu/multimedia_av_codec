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
        name_ = name;
    }
    ~DemuxerPluginMock()
    {
    }
    virtual Status Reset()
    {
        return mapStatus_[name_];
    }
    virtual Status Start()
    {
        return mapStatus_[name_];
    }
    virtual Status Stop()
    {
        return mapStatus_[name_];
    }
    virtual Status Flush()
    {
        return mapStatus_[name_];
    }
    virtual Status SetDataSource(const std::shared_ptr<DataSource>& source)
    {
        return mapStatus_[name_];
    }
    virtual Status GetMediaInfo(MediaInfo& mediaInfo)
    {
        return mapStatus_[name_];
    }
    virtual Status GetUserMeta(std::shared_ptr<Meta> meta)
    {
        return mapStatus_[name_];
    }
    virtual Status SelectTrack(uint32_t trackId)
    {
        return mapStatus_[name_];
    }
    virtual Status UnselectTrack(uint32_t trackId)
    {
        return mapStatus_[name_];
    }
    virtual Status SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode,
        int64_t& realSeekTime)
    {
        return mapStatus_[name_];
    }
    virtual Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
    {
        return mapStatus_[name_];
    }
    virtual Status GetNextSampleSize(uint32_t trackId, int32_t& size)
    {
        return mapStatus_[name_];
    }
    virtual Status GetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo)
    {
        return mapStatus_[name_];
    }
    virtual void ResetEosStatus()
    {
        return;
    }
    virtual Status ParserRefUpdatePos(int64_t timeStampMs, bool isForward = true)
    {
        return mapStatus_[name_];
    }
    virtual Status ParserRefInfo() override
    {
        return mapStatus_[name_];
    }
    virtual Status GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample,
        FrameLayerInfo &frameLayerInfo)
    {
        return mapStatus_[name_];
    }
    virtual Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
    {
        return mapStatus_[name_];
    }
    virtual Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
    {
        return mapStatus_[name_];
    }
    virtual Status GetIFramePos(std::vector<uint32_t> &IFramePos)
    {
        return mapStatus_[name_];
    }
    virtual Status Dts2FrameId(int64_t dts, uint32_t &frameId, bool offset = true)
    {
        return mapStatus_[name_];
    }
    virtual Status GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
        const uint64_t relativePresentationTimeUs, uint32_t &index)
    {
        return mapStatus_[name_];
    }
    virtual  Status GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
        const uint32_t index, uint64_t &relativePresentationTimeUs)
    {
        return mapStatus_[name_];
    }
    virtual void SetCacheLimit(uint32_t limitSize)
    {
        return;
    }
private:
    std::map<std::string, Status> mapStatus_;
    std::string name_;
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
    virtual Status SetSource(std::shared_ptr<MediaSource> source)
    {
        return mapStatus_[name_];
    }
    virtual Status Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
    {
        return mapStatus_[name_];
    }
    virtual Status Read(int32_t streamId, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
    {
        return mapStatus_[name_];
    }
    virtual Status GetSize(uint64_t& size)
    {
        return mapStatus_[name_];
    }
    virtual Seekable GetSeekable()
    {
        return Seekable::SEEKABLE;
    }
    virtual Status SeekTo(uint64_t offset)
    {
        return mapStatus_[name_];
    }
    virtual Status Reset()
    {
        return mapStatus_[name_];
    }
private:
    std::map<std::string, Status> mapStatus_;
    std::string name_;
};

}
}

#endif