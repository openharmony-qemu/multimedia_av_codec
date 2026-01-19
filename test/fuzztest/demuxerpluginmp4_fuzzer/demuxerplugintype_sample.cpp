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

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include "demuxer_plugin_manager.h"
#include "stream_demuxer.h"
#include "common/media_source.h"
#include "plugin/plugin_manager_v2.h"
#include "buffer/avbuffer.h"
#include "demuxerplugintype_sample.h"

using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::Media::Plugins::Ffmpeg;

namespace OHOS {
namespace Media {
constexpr uint32_t THREE = 3;
DemuxerPluginTypeTest::DemuxerPluginTypeTest() = default;

DemuxerPluginTypeTest::~DemuxerPluginTypeTest()
{
    if (fd_ >= 0) {
        close(fd_);
    }
    fd_ = -1;
    demuxerPlugin_ = nullptr;
}

bool DemuxerPluginTypeTest::InitWithData(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        return false;
    }
    fd_ = open(testFilePath_, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
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

void DemuxerPluginTypeTest::PrepareDemuxerPlugin(MediaInfo& mediaInfo, size_t& bufferSize, AVBufferWrapper& buffer)
{
    std::string uri = "fd://" + std::to_string(fd_) + "?offset=0&size=" + std::to_string(dataSize_);
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(uri);
    std::shared_ptr<StreamDemuxer> streamDemuxer = std::make_shared<StreamDemuxer>();
    std::shared_ptr<Source> source = std::make_shared<Source>();
    source->SetSource(mediaSource);
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    streamDemuxer->Init(uri);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    auto basePlugin = PluginManagerV2::Instance().CreatePluginByName(demuxerPluginName_);
    demuxerPlugin_ = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    if (!demuxerPlugin_) {
        return;
    }
    if (demuxerPlugin_->SetDataSource(dataSource) != Status::OK) {
        demuxerPlugin_ = nullptr;
        return;
    }

    if (demuxerPlugin_->GetMediaInfo(mediaInfo) != Status::OK) {
        demuxerPlugin_ = nullptr;
        return;
    }

    int32_t width = videoWidthDefault_;
    int32_t height = videoHeightDefault_;
    for (const auto& track : mediaInfo.tracks) {
        int32_t w = 0;
        int32_t h = 0;
        track.GetData(Tag::VIDEO_WIDTH, w);
        track.GetData(Tag::VIDEO_HEIGHT, h);
        if (w > 0 && h > 0) {
            width = w;
            height = h;
            break;
        }
    }
    bufferSize = width * height * 3; // 3 bytes per pixel (RGB)
    buffer = AVBufferWrapper(bufferSize);
}

void DemuxerPluginTypeTest::OperateDemuxerPlugin(MediaInfo& mediaInfo, size_t bufferSize, AVBufferWrapper& buffer)
{
    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->SelectTrack(idx);
    }
    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->UnselectTrack(idx);
    }
    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->SelectTrack(idx);
    }
    demuxerPlugin_->SeekToStart();
    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->ReadSample(idx, buffer.mediaAVBuffer, interfaceTimeout_);
        demuxerPlugin_->ReadSample(idx, buffer.mediaAVBuffer);
        int32_t nextSampleSize = 0;
        int64_t pts = 0;
        demuxerPlugin_->GetNextSampleSize(idx, nextSampleSize, interfaceTimeout_);
        demuxerPlugin_->GetLastPTSByTrackId(idx, pts);
        demuxerPlugin_->Pause();
    }

    int64_t seekTime = 0;
    demuxerPlugin_->SeekTo(-1, 0, SeekMode::SEEK_NEXT_SYNC, seekTime);
    demuxerPlugin_->Flush();
    demuxerPlugin_->SeekTo(0, seekTimeDefault_, SeekMode::SEEK_PREVIOUS_SYNC, seekTime);
    demuxerPlugin_->Reset();
}

void DemuxerPluginTypeTest::DemuxerPlugintask(MediaInfo& mediaInfo, AVBufferWrapper& buffer)
{
    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->ReadSample(idx, buffer.mediaAVBuffer, interfaceTimeout_);
    }
    int64_t seekTime = 0;
    demuxerPlugin_->SeekTo(0, seekTimeDefault_, SeekMode::SEEK_PREVIOUS_SYNC, seekTime);
}

void DemuxerPluginTypeTest::RunDemuxerInterfaceFuzz()
{
    if (fd_ < 0) {
        return;
    }
    MediaInfo mediaInfo;
    size_t bufferSize = 0;
    AVBufferWrapper buffer(1); // 占位初始化
    PrepareDemuxerPlugin(mediaInfo, bufferSize, buffer);
    if (!demuxerPlugin_) {
        return;
    }
    OperateDemuxerPlugin(mediaInfo, bufferSize, buffer);
    if (demuxerPlugin_->SetDataSource(dataSource) != Status::OK) {
        demuxerPlugin_ = nullptr;
        return;
    }
    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->SelectTrack(idx);
    }
    std::thread readPluginThread1([mediaInfo, this]() {
        MediaInfo Info = mediaInfo;
        AVBufferWrapper buf(videoWidthDefault_ * videoHeightDefault_ * THREE);
        DemuxerPlugintask(Info, buf);
    });
    std::thread readPluginThread2([mediaInfo, this]() {
        MediaInfo Info1 = mediaInfo;
        AVBufferWrapper buf1(videoWidthDefault_ * videoHeightDefault_ * THREE);
        DemuxerPlugintask(Info1, buf1);
    });
    readPluginThread1.join();
    readPluginThread2.join();
    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->UnselectTrack(idx);
    }
}

} // namespace Media
} // namespace OHOS