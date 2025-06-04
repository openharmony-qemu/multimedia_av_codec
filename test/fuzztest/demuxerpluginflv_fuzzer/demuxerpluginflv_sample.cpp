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

DemuxerPluginFlvTest::DemuxerPluginFlvTest() = default;

DemuxerPluginFlvTest::~DemuxerPluginFlvTest()
{
    if (fd_ >= 0) {
        close(fd_);
        unlink(tmpFilePath_.c_str());
    }
    fd_ = -1;
    demuxerPlugin_ = nullptr;
}

bool DemuxerPluginFlvTest::InitWithFlvData(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        return false;
    }
    char tmpName[] = "/tmp/flv_fuzz_XXXXXX";
    fd_ = mkstemp(tmpName);
    if (fd_ < 0) {
        return false;
    }
    ssize_t len = write(fd_, data, size);
    if (len != static_cast<ssize_t>(size)) {
        close(fd_);
        unlink(tmpName);
        fd_ = -1;
        return false;
    }
    lseek(fd_, 0, SEEK_SET);
    tmpFilePath_ = tmpName;
    dataSize_ = size;
    return true;
}

void DemuxerPluginFlvTest::RunDemuxerInterfaceFuzz()
{
    if (fd_ < 0) {
        return;
    }
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
        return;
    }

    MediaInfo mediaInfo;
    if (demuxerPlugin_->GetMediaInfo(mediaInfo) != Status::OK) {
        return;
    }

    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->SelectTrack(idx);
    }
    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->UnselectTrack(idx);
    }
    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->SelectTrack(idx);
    }

    int32_t width = 1920, height = 1080;
    for (const auto& track : mediaInfo.tracks) {
        int32_t w = 0, h = 0;
        track.GetData(Tag::VIDEO_WIDTH, w);
        track.GetData(Tag::VIDEO_HEIGHT, h);
        if (w > 0 && h > 0) {
            width = w;
            height = h;
            break;
        }
    }
    size_t bufferSize = width * height * 3;
    auto buffer = AVBuffer::CreateAVBuffer(nullptr, bufferSize, 0);

    for (uint32_t idx = 0; idx < mediaInfo.tracks.size(); ++idx) {
        demuxerPlugin_->ReadSample(idx, buffer, 100);
        demuxerPlugin_->ReadSample(idx, buffer);
        int32_t nextSampleSize = 0;
        demuxerPlugin_->GetNextSampleSize(idx, nextSampleSize, 100);
    }

    int64_t seekTime = 0;
    demuxerPlugin_->SeekTo(-1, 0, SeekMode::SEEK_NEXT_SYNC, seekTime);
    demuxerPlugin_->SeekTo(0, 1000, SeekMode::SEEK_PREVIOUS_SYNC, seekTime);

    demuxerPlugin_->Reset();
}

} // namespace Media
} // namespace OHOS