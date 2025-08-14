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
#include "demuxer_plugin_manager.h"
#include "stream_demuxer.h"
#include "common/media_source.h"
#include "plugin/plugin_manager_v2.h"
#include "buffer/avbuffer.h"
#include "demuxerpluginflv_sample.h"

using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::Media::Plugins::Ffmpeg;

namespace OHOS {
namespace Media {
const char* TEST_FILE_PATH = "/data/test/demuxerpluginflvfuzztest.flv";
const int32_t VIDEO_HEIGHT_DEFAULT = 1280;
const int32_t VIDEO_WIDTH_DEFAULT = 720;
const uint32_t INTERFACE_TIMEOUT = 100;
const int64_t SEEK_TIME_DEFAULT = 1000;
constexpr uint32_t THREAD_PRIORITY_41 = 7;

DemuxerPluginFlvTest::DemuxerPluginFlvTest() = default;

DemuxerPluginFlvTest::~DemuxerPluginFlvTest()
{
    if (fd_ >= 0) {
        close(fd_);
    }
    fd_ = -1;
    demuxerPlugin_ = nullptr;
}

bool DemuxerPluginFlvTest::InitWithFlvData(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        return false;
    }
    fd_ = open(TEST_FILE_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
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

void DemuxerPluginFlvTest::PrepareDemuxerPlugin(MediaInfo& mediaInfo, size_t& bufferSize, AVBufferWrapper& buffer)
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

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    auto basePlugin = PluginManagerV2::Instance().CreatePluginByName("avdemux_flv");
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

    int32_t width = VIDEO_WIDTH_DEFAULT;
    int32_t height = VIDEO_HEIGHT_DEFAULT;
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

void DemuxerPluginFlvTest::OperateDemuxerPlugin(MediaInfo& mediaInfo, size_t bufferSize, AVBufferWrapper& buffer)
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
    demuxerPlugin_->SetAsyncReadThreadPriority(THREAD_PRIORITY_41, "DemuxerPluginFlvFuzzTest");
    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->ReadSample(idx, buffer.mediaAVBuffer, INTERFACE_TIMEOUT);
        demuxerPlugin_->ReadSample(idx, buffer.mediaAVBuffer);
        int32_t nextSampleSize = 0;
        int64_t pts = 0;
        demuxerPlugin_->GetNextSampleSize(idx, nextSampleSize, INTERFACE_TIMEOUT);
        demuxerPlugin_->GetLastPTSByTrackId(idx, pts);
        demuxerPlugin_->Pause();
    }

    int64_t seekTime = 0;
    demuxerPlugin_->SeekTo(-1, 0, SeekMode::SEEK_NEXT_SYNC, seekTime);
    demuxerPlugin_->Flush();
    demuxerPlugin_->SeekTo(0, SEEK_TIME_DEFAULT, SeekMode::SEEK_PREVIOUS_SYNC, seekTime);
    demuxerPlugin_->Reset();
}

void DemuxerPluginFlvTest::RunDemuxerInterfaceFuzz()
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
}

} // namespace Media
} // namespace OHOS