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

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <fuzzer/FuzzedDataProvider.h>
#include "securec.h"
#include "demuxer_plugin_manager.h"
#include "stream_demuxer.h"

#define FUZZ_PROJECT_NAME "demuxerpluginmanager_fuzzer"
using namespace std;
using namespace OHOS::Media;
namespace OHOS {
const char *MP4_PATH = "/data/test/fuzz_create.mp4";
const int64_t INVALID_STREAM_OR_TRACK_ID = -1;
const int64_t AUDIO_TRACK = 1;
const int64_t VIDEO_TRACK = 0;
const int64_t SUBTITLE_TRACK = 2;
const int64_t EXPECT_SIZE = 37;
const int64_t SELECT_TRACK = 4;
bool CheckDataValidity(FuzzedDataProvider *fdp, size_t dataSize)
{
    if (dataSize < EXPECT_SIZE) {
        return false;
    }
    int32_t fd = open(MP4_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    uint8_t *pstream = nullptr;
    uint16_t framesize = dataSize - EXPECT_SIZE;
    pstream = (uint8_t *)malloc(framesize * sizeof(uint8_t));
    if (!pstream) {
        std::cerr << "Memory alloction failed" << std::endl;
        close(fd);
        return false;
    }
    fdp->ConsumeData(pstream, framesize);
    int len = write(fd, pstream, framesize);
    if (len <= 0) {
        close(fd);
        free(pstream);
        pstream = nullptr;
        return false;
    }
    close(fd);
    free(pstream);
    pstream = nullptr;
    return true;
}

bool DemuxerPluginManagerFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (!CheckDataValidity(&fdp, size)) {
        return false;
    }

    std::shared_ptr<OHOS::Media::DemuxerPluginManager> demuxerPluginManager_ =
        std::make_shared<OHOS::Media::DemuxerPluginManager>();

    std::shared_ptr<OHOS::Media::BaseStreamDemuxer> streamDemuxer =
        std::make_shared<StreamDemuxer>();
    std::shared_ptr<MediaSource> mediaSource =
        std::make_shared<MediaSource>(MP4_PATH);

    std::shared_ptr<Source> source = std::make_shared<Source>();
    auto res = source->SetSource(mediaSource);
    if (res != Status::OK) {
        return false;
    }

    std::vector<StreamInfo> streams;
    demuxerPluginManager_->SetIsHlsFmp4(false);

    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    streamDemuxer->Init(MP4_PATH);

    Media::Plugins::StreamInfo videoInfo;
    videoInfo.streamId = fdp.ConsumeIntegral<int32_t>();
    videoInfo.bitRate = fdp.ConsumeIntegral<uint32_t>();
    videoInfo.videoWidth = fdp.ConsumeIntegral<int32_t>();
    videoInfo.videoHeight =  fdp.ConsumeIntegral<int32_t>();
    videoInfo.videoType = Media::Plugins::VideoType::VIDEO_TYPE_SDR;
    videoInfo.type = OHOS::Media::Plugins::StreamType::VIDEO;
    Media::Plugins::StreamInfo audioInfo;
    audioInfo.streamId = fdp.ConsumeIntegral<int32_t>();
    audioInfo.bitRate = fdp.ConsumeIntegral<uint32_t>();
    audioInfo.videoWidth = fdp.ConsumeIntegral<int32_t>();
    audioInfo.videoHeight =  fdp.ConsumeIntegral<int32_t>();
    audioInfo.videoType = Media::Plugins::VideoType::VIDEO_TYPE_SDR;
    audioInfo.type = OHOS::Media::Plugins::StreamType::AUDIO;

    Media::Plugins::StreamInfo subtitleInfo;
    subtitleInfo.streamId = fdp.ConsumeIntegral<int32_t>();
    subtitleInfo.type = OHOS::Media::Plugins::StreamType::SUBTITLE;
    streams.push_back(videoInfo);
    streams.push_back(audioInfo);
    streams.push_back(subtitleInfo);

    int32_t streamID = fdp.ConsumeIntegral<int32_t>();
    int32_t trackId = fdp.ConsumeIntegral<int32_t>();
    int32_t innerTrackId = fdp.ConsumeIntegral<int32_t>();
    demuxerPluginManager_->InitDefaultPlay(streams);
    demuxerPluginManager_->InitDefaultPlay(streams);
    demuxerPluginManager_->SetResetEosStatus(fdp.ConsumeBool());
    demuxerPluginManager_->GetTrackInfoByStreamID(streamID, trackId, innerTrackId);
    if (innerTrackId != trackId) {
        return false;
    }
    TrackType trackType;
    switch (fdp.ConsumeIntegral<int32_t>() % SELECT_TRACK) {
        case VIDEO_TRACK:
            trackType = TrackType::TRACK_VIDEO;
            break;
        case AUDIO_TRACK:
            trackType = TrackType::TRACK_AUDIO;
            break;
        case SUBTITLE_TRACK:
            trackType = TrackType::TRACK_SUBTITLE;
            break;
        default:
            trackType = TrackType::TRACK_INVALID;
            break;
    }
    demuxerPluginManager_->GetTrackInfoByStreamID(streamID, trackId, innerTrackId, trackType);
    if (innerTrackId != trackId) {
        return false;
    }
    int32_t innerTrackIndex = demuxerPluginManager_->GetInnerTrackIDByTrackID(trackId);
    if (innerTrackIndex != trackId) {
        return false;
    }
    int32_t cusTypeStreamId = demuxerPluginManager_->GetStreamIDByTrackType(trackType);
    if (trackType == TRACK_VIDEO) {
        if (cusTypeStreamId != videoInfo.streamId) {
            return false;
        }
    } else if (trackType == TRACK_AUDIO) {
        if (cusTypeStreamId != audioInfo.streamId) {
            return false;
        }
    } else if (trackType == TRACK_SUBTITLE) {
        if (cusTypeStreamId != subtitleInfo.streamId) {
            return false;
        }
    } else {
        if (cusTypeStreamId != INVALID_STREAM_OR_TRACK_ID) {
            return false;
        }
    }

    demuxerPluginManager_->StopPlugin(streamID, streamDemuxer);
    bool isRebooted = false;
    demuxerPluginManager_->RebootPlugin(streamID, trackType, streamDemuxer, isRebooted);
    int64_t realSeekTime = fdp.ConsumeIntegral<int64_t>();
    demuxerPluginManager_->SingleStreamSeekTo(realSeekTime,
        Plugins::SeekMode::SEEK_CLOSEST_SYNC, streamID, realSeekTime);
    demuxerPluginManager_->Start();
    demuxerPluginManager_->Stop();
    demuxerPluginManager_->Flush();
    demuxerPluginManager_->Reset();

    Plugins::MediaInfo mediaInfo;
    demuxerPluginManager_->LoadCurrentAllPlugin(streamDemuxer, mediaInfo);
    demuxerPluginManager_->GetCurrentBitRate();
    demuxerPluginManager_->GetTrackTypeByTrackID(trackId);
    demuxerPluginManager_->AddExternalSubtitle();
    demuxerPluginManager_->AddExternalSubtitle();
    demuxerPluginManager_->SetApiVersion(fdp.ConsumeIntegral<int32_t>());

    int ret = remove(MP4_PATH);
    if (ret != 0) {
        return false;
    }
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DemuxerPluginManagerFuzzTest(data, size);
    return 0;
}