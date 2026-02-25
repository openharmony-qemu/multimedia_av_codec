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

#ifndef DEMUXER_UNITTEST_DEMUXER_PLUGIN_MOCK_H
#define DEMUXER_UNITTEST_DEMUXER_PLUGIN_MOCK_H

#include <memory>
#include "plugin/demuxer_plugin.h"

namespace OHOS {
namespace Media {
namespace Plugins {

class TestDemuxerPlugin final : public DemuxerPlugin {
public:
    TestDemuxerPlugin(Status seekToResult, Status seekToKeyFrameResult,
        Status seekToFrameByDtsResult = Status::OK, Status pauseResult = Status::OK)
        : DemuxerPlugin("test"), seekToResult_(seekToResult),
          seekToKeyFrameResult_(seekToKeyFrameResult), seekToFrameByDtsResult_(seekToFrameByDtsResult),
          pauseResult_(pauseResult)
    {
    }

    Status SetDataSource(const std::shared_ptr<DataSource>&) override
    {
        return Status::OK;
    }

    Status GetMediaInfo(MediaInfo&) override
    {
        return Status::OK;
    }

    Status GetUserMeta(std::shared_ptr<Meta>) override
    {
        return Status::OK;
    }

    Status SelectTrack(uint32_t) override
    {
        return Status::OK;
    }

    Status UnselectTrack(uint32_t) override
    {
        return Status::OK;
    }

    Status ReadSample(uint32_t, std::shared_ptr<AVBuffer>) override
    {
        return Status::OK;
    }

    Status ReadSample(uint32_t, std::shared_ptr<AVBuffer>, uint32_t) override
    {
        return Status::OK;
    }

    Status GetNextSampleSize(uint32_t, int32_t&) override
    {
        return Status::OK;
    }

    Status GetNextSampleSize(uint32_t, int32_t&, uint32_t) override
    {
        return Status::OK;
    }

    Status SeekTo(int32_t, int64_t, SeekMode mode, int64_t&) override
    {
        ++seekToCallCount_;
        lastSeekMode_ = mode;
        return seekToResult_;
    }

    Status GetLastPTSByTrackId(uint32_t, int64_t&) override
    {
        return Status::OK;
    }

    Status Pause() override
    {
        ++pauseCallCount_;
        return pauseResult_;
    }

    Status Reset() override
    {
        return Status::OK;
    }

    Status Start() override
    {
        return Status::OK;
    }

    Status Stop() override
    {
        return Status::OK;
    }

    Status Flush() override
    {
        return Status::OK;
    }

    void ResetEosStatus() override
    {
    }

    Status ParserRefUpdatePos(int64_t, bool) override
    {
        return Status::OK;
    }

    Status ParserRefInfo() override
    {
        return Status::OK;
    }

    Status GetFrameLayerInfo(std::shared_ptr<AVBuffer>, FrameLayerInfo&) override
    {
        return Status::OK;
    }

    Status GetFrameLayerInfo(uint32_t, FrameLayerInfo&) override
    {
        return Status::OK;
    }

    Status GetGopLayerInfo(uint32_t, GopLayerInfo&) override
    {
        return Status::OK;
    }

    Status GetIFramePos(std::vector<uint32_t>&) override
    {
        return Status::OK;
    }

    Status Dts2FrameId(int64_t, uint32_t&) override
    {
        return Status::OK;
    }

    Status SeekMs2FrameId(int64_t, uint32_t&) override
    {
        return Status::OK;
    }

    Status FrameId2SeekMs(uint32_t, int64_t&) override
    {
        return Status::OK;
    }

    Status GetIndexByRelativePresentationTimeUs(uint32_t, uint64_t, uint32_t&) override
    {
        return Status::OK;
    }

    Status GetRelativePresentationTimeUsByIndex(uint32_t, uint32_t, uint64_t&) override
    {
        return Status::OK;
    }

    void SetCacheLimit(uint32_t) override
    {
    }

    Status SetCachePressureCallback(CachePressureCallback) override
    {
        return Status::OK;
    }

    Status SetTrackCacheLimit(uint32_t, uint32_t, uint32_t) override
    {
        return Status::OK;
    }

    Status SeekToFrameByDts(int32_t, int64_t, SeekMode, int64_t&, uint32_t) override
    {
        ++seekToFrameByDtsCallCount_;
        return seekToFrameByDtsResult_;
    }

    Status SetDataSourceWithProbSize(const std::shared_ptr<DataSource>&, const int32_t) override
    {
        return Status::OK;
    }

    Status BoostReadThreadPriority() override
    {
        return Status::OK;
    }

    Status SetAVReadPacketStopState(bool) override
    {
        return Status::OK;
    }

    Status SeekToStart() override
    {
        return Status::OK;
    }

    Status SeekToKeyFrame(int32_t, int64_t, SeekMode mode, int64_t&, uint32_t) override
    {
        ++seekToKeyFrameCallCount_;
        lastSeekMode_ = mode;
        return seekToKeyFrameResult_;
    }

    int GetSeekToCallCount() const
    {
        return seekToCallCount_;
    }

    int GetSeekToKeyFrameCallCount() const
    {
        return seekToKeyFrameCallCount_;
    }

    int GetSeekToFrameByDtsCallCount() const
    {
        return seekToFrameByDtsCallCount_;
    }

    int GetPauseCallCount() const
    {
        return pauseCallCount_;
    }

    SeekMode GetLastSeekMode() const
    {
        return lastSeekMode_;
    }

    Status ReadSampleZeroCopy(uint32_t trackId, std::shared_ptr<AVBuffer> sample, uint32_t timeout) override
    {
        return Status::OK;
    }

private:
    Status seekToResult_ { Status::OK };
    Status seekToKeyFrameResult_ { Status::OK };
    Status seekToFrameByDtsResult_ { Status::OK };
    Status pauseResult_ { Status::OK };
    int seekToCallCount_ { 0 };
    int seekToKeyFrameCallCount_ { 0 };
    int seekToFrameByDtsCallCount_ { 0 };
    int pauseCallCount_ { 0 };
    SeekMode lastSeekMode_ { SeekMode::SEEK_NEXT_SYNC };
};

} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // DEMUXER_UNITTEST_DEMUXER_PLUGIN_MOCK_H
