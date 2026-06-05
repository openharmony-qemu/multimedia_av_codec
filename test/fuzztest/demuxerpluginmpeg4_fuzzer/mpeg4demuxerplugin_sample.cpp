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

#include "mpeg4demuxerplugin_sample.h"

#include <algorithm>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include "common/media_source.h"
#include "meta/meta.h"
#include "source/source.h"

namespace OHOS {
namespace Media {
namespace {
constexpr uint32_t DEFAULT_BUFFER_SIZE = 1024 * 1024;
constexpr uint32_t MIN_BUFFER_SIZE = 1;
constexpr uint32_t MAX_TRACKS_TO_FUZZ = 8;
constexpr uint32_t CONCURRENT_LOOP_COUNT = 2;
constexpr int64_t MAX_SEEK_TIME_MS = 60 * 1000;
constexpr int64_t MAX_READ_TIMEOUT_MS = 1000;

int64_t ClampNonNegative(int64_t value, int64_t upper)
{
    if (value < 0) {
        return 0;
    }
    return std::min(value, upper);
}

uint32_t ClampTrackCount(size_t trackCount)
{
    return static_cast<uint32_t>(
        std::min(trackCount, static_cast<size_t>(MAX_TRACKS_TO_FUZZ)));
}

std::vector<SeekMode> GetSeekModes()
{
    return {
        SeekMode::SEEK_PREVIOUS_SYNC, SeekMode::SEEK_NEXT_SYNC,
        SeekMode::SEEK_CLOSEST_SYNC,  SeekMode::SEEK_CLOSEST,
        SeekMode::SEEK_CLOSEST_INNER,
    };
}
} // namespace

Mpeg4AVBufferWrapper::Mpeg4AVBufferWrapper(uint32_t capacity,
                                           bool useEmptyBuffer)
{
    if (useEmptyBuffer) {
        buffer_ = AVBuffer::CreateAVBuffer();
        return;
    }
    capacity = std::max(capacity, MIN_BUFFER_SIZE);
    capacity = std::min(capacity, DEFAULT_BUFFER_SIZE);
    data_.resize(capacity);
    buffer_ = AVBuffer::CreateAVBuffer(data_.data(),
                                       static_cast<int32_t>(data_.size()), 0);
}

std::shared_ptr<AVBuffer> Mpeg4AVBufferWrapper::GetBuffer() const
{
    return buffer_;
}

Mpeg4DemuxerPluginFuzzTest::Mpeg4DemuxerPluginFuzzTest() = default;

Mpeg4DemuxerPluginFuzzTest::~Mpeg4DemuxerPluginFuzzTest()
{
    if (fd_ >= 0) {
        close(fd_);
    }
    fd_ = -1;
    dataSource_ = nullptr;
    (void)remove(testFilePath_);
}

bool Mpeg4DemuxerPluginFuzzTest::InitWithData(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }
    fd_ = open(testFilePath_, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd_ < 0) {
        return false;
    }
    ssize_t len = write(fd_, data, size);
    if (len != static_cast<ssize_t>(size)) {
        close(fd_);
        fd_ = -1;
        return false;
    }
    lseek(fd_, 0, SEEK_SET);
    dataSize_ = size;
    return true;
}

std::shared_ptr<Mpeg4DemuxerPluginFuzzTest::MPEG4DemuxerPlugin>
Mpeg4DemuxerPluginFuzzTest::CreatePlugin() const
{
    return std::make_shared<MPEG4DemuxerPlugin>("MPEG4DemuxerPluginFuzz");
}

std::shared_ptr<DataSourceImpl> Mpeg4DemuxerPluginFuzzTest::CreateDataSource()
{
    if (fd_ < 0 || dataSize_ == 0) {
        return nullptr;
    }
    std::string uri = "fd://" + std::to_string(fd_) +
                      "?offset=0&size=" + std::to_string(dataSize_);
    std::shared_ptr<MediaSource> mediaSource =
        std::make_shared<MediaSource>(uri);
    std::shared_ptr<StreamDemuxer> streamDemuxer =
        std::make_shared<StreamDemuxer>();
    std::shared_ptr<Source> source = std::make_shared<Source>();
    source->SetSource(mediaSource);
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    streamDemuxer->Init(uri);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
    dataSource_ = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    return dataSource_;
}

void Mpeg4DemuxerPluginFuzzTest::RunPreInitInterfaceFuzz(
    std::shared_ptr<MPEG4DemuxerPlugin> plugin,
    const Mpeg4DemuxerPluginFuzzConfig &config)
{
    if (plugin == nullptr) {
        return;
    }
    MediaInfo mediaInfo;
    std::shared_ptr<Meta> userMeta = std::make_shared<Meta>();
    Mpeg4AVBufferWrapper buffer(DEFAULT_BUFFER_SIZE, config.useEmptyBuffer);
    int64_t realSeekTime = 0;
    int32_t nextSampleSize = 0;
    int64_t lastPts = 0;
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    uint32_t cacheSize = 0;
    uint32_t frameCount = 0;
    int32_t probeOffset = 0;
    int32_t probeSize = 0;

    plugin->GetMediaInfo(mediaInfo);
    plugin->GetUserMeta(userMeta);
    plugin->SelectTrack(config.invalidTrackId);
    plugin->UnselectTrack(config.invalidTrackId);
    plugin->ReadSample(config.invalidTrackId, buffer.GetBuffer());
    plugin->ReadSample(config.invalidTrackId, buffer.GetBuffer(),
                       config.timeoutMs);
    plugin->ReadSampleZeroCopy(config.invalidTrackId, buffer.GetBuffer(),
                               config.timeoutMs);
    plugin->GetNextSampleSize(config.invalidTrackId, nextSampleSize);
    plugin->GetNextSampleSize(config.invalidTrackId, nextSampleSize,
                              config.timeoutMs);
    plugin->GetLastPTSByTrackId(config.invalidTrackId, lastPts);
    plugin->SeekTo(static_cast<int32_t>(config.invalidTrackId),
                   config.seekTimeMs, SeekMode::SEEK_PREVIOUS_SYNC,
                   realSeekTime);
    plugin->SeekToStart();
    plugin->SeekToKeyFrame(static_cast<int32_t>(config.invalidTrackId),
                           config.seekTimeMs, SeekMode::SEEK_PREVIOUS_SYNC,
                           realSeekTime, config.timeoutMs);
    plugin->SeekToFrameByDts(static_cast<int32_t>(config.invalidTrackId),
                             config.seekTimeMs, SeekMode::SEEK_CLOSEST,
                             realSeekTime, config.timeoutMs);
    plugin->GetDrmInfo(drmInfo);
    plugin->GetCurrentCacheSize(config.invalidTrackId, cacheSize);
    plugin->GetCurrentCacheFrameCount(config.invalidTrackId, frameCount);
    plugin->GetProbeSize(probeOffset, probeSize);
    plugin->SetReadTimeoutForInitSeek(config.readTimeoutMs);
    plugin->SetInterruptState(config.interruptState);
    plugin->SetAVReadPacketStopState(config.stopReadPacketState);
    plugin->BoostReadThreadPriority();
    plugin->Flush();
    plugin->Pause();
    plugin->ResetEosStatus();
    plugin->Reset();
}

void Mpeg4DemuxerPluginFuzzTest::RunTrackInterfaceFuzz(
    std::shared_ptr<MPEG4DemuxerPlugin> plugin, const MediaInfo &mediaInfo,
    const Mpeg4DemuxerPluginFuzzConfig &config, Mpeg4AVBufferWrapper &buffer)
{
    if (plugin == nullptr) {
        return;
    }
    uint32_t trackCount = ClampTrackCount(mediaInfo.tracks.size());
    std::vector<uint32_t> trackIds;
    for (uint32_t trackId = 0; trackId < trackCount; ++trackId) {
        trackIds.push_back(trackId);
        plugin->SelectTrack(trackId);
    }
    plugin->UnselectTrack(config.invalidTrackId);
    plugin->SelectTrack(config.invalidTrackId);
    for (uint32_t trackId : trackIds) {
        int32_t nextSampleSize = 0;
        int64_t lastPts = 0;
        plugin->GetNextSampleSize(trackId, nextSampleSize);
        plugin->GetNextSampleSize(trackId, nextSampleSize, config.timeoutMs);
        plugin->GetLastPTSByTrackId(trackId, lastPts);
        plugin->ReadSample(trackId, buffer.GetBuffer());
        plugin->ReadSample(trackId, buffer.GetBuffer(), config.timeoutMs);
        plugin->ReadSampleZeroCopy(trackId, buffer.GetBuffer(),
                                   config.timeoutMs);
    }
}

void Mpeg4DemuxerPluginFuzzTest::RunSeekInterfaceFuzz(
    std::shared_ptr<MPEG4DemuxerPlugin> plugin,
    const std::vector<uint32_t> &trackIds,
    const Mpeg4DemuxerPluginFuzzConfig &config)
{
    if (plugin == nullptr) {
        return;
    }
    int64_t realSeekTime = 0;
    int64_t seekTime = ClampNonNegative(config.seekTimeMs, MAX_SEEK_TIME_MS);
    int64_t secondSeekTime =
        ClampNonNegative(config.secondSeekTimeMs, MAX_SEEK_TIME_MS);
    for (SeekMode mode : GetSeekModes()) {
        plugin->SeekTo(-1, seekTime, mode, realSeekTime);
        if (!trackIds.empty()) {
            plugin->SeekTo(static_cast<int32_t>(trackIds.front()),
                           secondSeekTime, mode, realSeekTime);
        }
    }
    plugin->SeekToStart();
    for (uint32_t trackId : trackIds) {
        plugin->SeekToKeyFrame(static_cast<int32_t>(trackId), seekTime,
                               SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime,
                               config.timeoutMs);
        plugin->SeekToFrameByDts(static_cast<int32_t>(trackId), seekTime,
                                 SeekMode::SEEK_CLOSEST, realSeekTime,
                                 config.timeoutMs);
    }
}

void Mpeg4DemuxerPluginFuzzTest::RunCacheInterfaceFuzz(
    std::shared_ptr<MPEG4DemuxerPlugin> plugin,
    const std::vector<uint32_t> &trackIds,
    const Mpeg4DemuxerPluginFuzzConfig &config)
{
    if (plugin == nullptr) {
        return;
    }
    plugin->SetCacheLimit(config.cacheLimit);
    std::weak_ptr<MPEG4DemuxerPlugin> weakPlugin = plugin;
    plugin->SetCachePressureCallback(
        [weakPlugin](uint32_t trackId, uint32_t bytes) {
            (void)bytes;
            auto plugin = weakPlugin.lock();
            if (plugin == nullptr) {
                return;
            }
            uint32_t size = 0;
            uint32_t frameCount = 0;
            plugin->GetCurrentCacheSize(trackId, size);
            plugin->GetCurrentCacheFrameCount(trackId, frameCount);
        });
    plugin->SetTrackCacheLimit(config.invalidTrackId, config.trackCacheLimit,
                               config.trackCacheWindowMs);
    for (uint32_t trackId : trackIds) {
        uint32_t size = 0;
        uint32_t frameCount = 0;
        plugin->SetTrackCacheLimit(trackId, config.trackCacheLimit,
                                   config.trackCacheWindowMs);
        plugin->GetCurrentCacheSize(trackId, size);
        plugin->GetCurrentCacheFrameCount(trackId, frameCount);
    }
}

void Mpeg4DemuxerPluginFuzzTest::RunReferenceParserInterfaceFuzz(
    std::shared_ptr<MPEG4DemuxerPlugin> plugin,
    const Mpeg4DemuxerPluginFuzzConfig &config, Mpeg4AVBufferWrapper &buffer)
{
    if (plugin == nullptr) {
        return;
    }
    FrameLayerInfo frameLayerInfo;
    GopLayerInfo gopLayerInfo;
    std::vector<uint32_t> iFramePos;
    uint32_t frameId = 0;
    int64_t seekMs = 0;
    uint32_t index = 0;
    uint64_t relativePresentationTimeUs = 0;

    plugin->IsRefParserSupported();
    plugin->ParserRefUpdatePos(
        ClampNonNegative(config.seekTimeMs, MAX_SEEK_TIME_MS),
        config.interruptState);
    plugin->ParserRefInfo();
    plugin->GetFrameLayerInfo(buffer.GetBuffer(), frameLayerInfo);
    plugin->GetFrameLayerInfo(config.frameId, frameLayerInfo);
    plugin->GetGopLayerInfo(config.gopId, gopLayerInfo);
    plugin->GetIFramePos(iFramePos);
    plugin->Dts2FrameId(config.dts, frameId);
    plugin->SeekMs2FrameId(
        ClampNonNegative(config.seekTimeMs, MAX_SEEK_TIME_MS), frameId);
    plugin->FrameId2SeekMs(config.frameId, seekMs);
    plugin->GetIndexByRelativePresentationTimeUs(
        config.invalidTrackId, config.relativePresentationTimeUs, index);
    plugin->GetRelativePresentationTimeUsByIndex(
        config.invalidTrackId, config.frameId, relativePresentationTimeUs);
}

void Mpeg4DemuxerPluginFuzzTest::RunConcurrentInterfaceFuzz(
    std::shared_ptr<MPEG4DemuxerPlugin> plugin,
    const std::vector<uint32_t> &trackIds,
    const Mpeg4DemuxerPluginFuzzConfig &config)
{
    if (plugin == nullptr || trackIds.empty()) {
        return;
    }
    uint32_t trackId = trackIds.front();
    std::thread readThread([plugin, trackId, &config]() {
        Mpeg4AVBufferWrapper buffer(DEFAULT_BUFFER_SIZE, config.useEmptyBuffer);
        for (uint32_t i = 0; i < CONCURRENT_LOOP_COUNT; ++i) {
            int32_t nextSampleSize = 0;
            plugin->GetNextSampleSize(trackId, nextSampleSize,
                                      config.timeoutMs);
            plugin->ReadSample(trackId, buffer.GetBuffer(), config.timeoutMs);
        }
    });
    std::thread seekThread([plugin, trackId, &config]() {
        for (uint32_t i = 0; i < CONCURRENT_LOOP_COUNT; ++i) {
            int64_t realSeekTime = 0;
            uint32_t cacheSize = 0;
            plugin->SeekTo(
                static_cast<int32_t>(trackId),
                ClampNonNegative(config.seekTimeMs, MAX_SEEK_TIME_MS),
                SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
            plugin->Flush();
            plugin->GetCurrentCacheSize(trackId, cacheSize);
        }
    });
    readThread.join();
    seekThread.join();
}

void Mpeg4DemuxerPluginFuzzTest::RunPostInitInterfaceFuzz(
    std::shared_ptr<MPEG4DemuxerPlugin> plugin,
    const Mpeg4DemuxerPluginFuzzConfig &config)
{
    if (plugin == nullptr) {
        return;
    }
    MediaInfo mediaInfo;
    std::shared_ptr<Meta> userMeta = std::make_shared<Meta>();
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    plugin->Start();
    plugin->Stop();
    plugin->GetMediaInfo(mediaInfo);
    plugin->GetUserMeta(userMeta);
    plugin->GetDrmInfo(drmInfo);
    plugin->SetInterruptState(config.interruptState);
    plugin->SetAVReadPacketStopState(config.stopReadPacketState);
    plugin->SetReadTimeoutForInitSeek(
        ClampNonNegative(config.readTimeoutMs, MAX_READ_TIMEOUT_MS));

    std::vector<uint32_t> trackIds;
    uint32_t trackCount = ClampTrackCount(mediaInfo.tracks.size());
    for (uint32_t trackId = 0; trackId < trackCount; ++trackId) {
        trackIds.push_back(trackId);
    }
    Mpeg4AVBufferWrapper buffer(DEFAULT_BUFFER_SIZE, config.useEmptyBuffer);
    RunTrackInterfaceFuzz(plugin, mediaInfo, config, buffer);
    RunCacheInterfaceFuzz(plugin, trackIds, config);
    RunSeekInterfaceFuzz(plugin, trackIds, config);
    RunReferenceParserInterfaceFuzz(plugin, config, buffer);
    RunConcurrentInterfaceFuzz(plugin, trackIds, config);
    plugin->Pause();
    for (uint32_t trackId : trackIds) {
        plugin->UnselectTrack(trackId);
    }
    plugin->Flush();
    plugin->ResetEosStatus();
}

void Mpeg4DemuxerPluginFuzzTest::Run(const Mpeg4DemuxerPluginFuzzConfig &config)
{
    if (fd_ < 0) {
        return;
    }
    std::shared_ptr<MPEG4DemuxerPlugin> plugin = CreatePlugin();
    RunPreInitInterfaceFuzz(plugin, config);

    std::shared_ptr<DataSourceImpl> dataSource = CreateDataSource();
    if (dataSource == nullptr) {
        return;
    }
    plugin = CreatePlugin();
    plugin->SetCacheLimit(config.cacheLimit);
    plugin->SetInterruptState(config.interruptState);
    plugin->SetAVReadPacketStopState(config.stopReadPacketState);
    plugin->SetReadTimeoutForInitSeek(
        ClampNonNegative(config.readTimeoutMs, MAX_READ_TIMEOUT_MS));
    plugin->SetDataSourceWithProbSize(dataSource, config.probSize);

    plugin = CreatePlugin();
    std::shared_ptr<Meta> configs =
        config.useConfigs ? std::make_shared<Meta>() : nullptr;
    if (plugin->SetDataSource(dataSource, configs) != Status::OK) {
        return;
    }
    RunPostInitInterfaceFuzz(plugin, config);
    plugin->Reset();
}

} // namespace Media
} // namespace OHOS
