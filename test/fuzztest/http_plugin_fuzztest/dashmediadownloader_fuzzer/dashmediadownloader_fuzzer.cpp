/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <atomic>
#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include "dash_media_downloader.h"
#define FUZZ_PROJECT_NAME "dashmediadownloader_fuzzer"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

using namespace std;
using namespace OHOS::Media;
namespace {
static const std::string MPD_MULTI_AUDIO_SUB = "http://127.0.0.1:47777/test_dash/segment_base/index_audio_subtitle.mpd";
constexpr int32_t WAIT_FOR_SIDX_TIME = 100 * 1000; // wait sidx download and parse for 100ms
constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;
constexpr uint32_t DEFAULT_DURATION = 20;
class DashCallbackMock1 : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        switch (event.type) {
            case PluginEventType::STREAM_UPDATE: {
                int videoId = -1;
                int audioId = -1;
                int textId = -1;
                mediaDownloader->SetDefaultStreamId(videoId, audioId, textId);
                break;
            }
            default:break;
        }
    }
    void SetDownload(std::shared_ptr<DashMediaDownloader> downloader)
    {
        mediaDownloader = downloader;
    }
    std::shared_ptr<DashMediaDownloader> mediaDownloader;
};

struct StreamSwitchContext {
    int32_t defaultVideoStreamId;
    int32_t defaultAudioStreamId;
    int32_t defaultSubtitleStreamId;
    int32_t usingStreamId;
    int32_t switchingStreamId;
};

void ConfigureDownloader(const std::shared_ptr<DashMediaDownloader> &mediaDownloader)
{
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);

    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = DEFAULT_WIDTH;
    playStrategy->height = DEFAULT_HEIGHT;
    playStrategy->duration = DEFAULT_DURATION;
    playStrategy->audioLanguage = "eng";
    playStrategy->subtitleLanguage = "en_GB";
    mediaDownloader->SetPlayStrategy(playStrategy);
}

void ResolveStreamIds(const std::vector<StreamInfo> &streams, StreamSwitchContext &ctx)
{
    for (auto stream : streams) {
        if (stream.type == VIDEO && ctx.defaultVideoStreamId == -1) {
            ctx.defaultVideoStreamId = stream.streamId;
        }
        if (stream.type == AUDIO && ctx.defaultAudioStreamId == -1) {
            ctx.defaultAudioStreamId = stream.streamId;
        }
        if (stream.type == SUBTITLE && ctx.defaultSubtitleStreamId == -1) {
            ctx.defaultSubtitleStreamId = stream.streamId;
        }
        if (stream.type != SUBTITLE) {
            continue;
        }
        if (ctx.usingStreamId == -1) {
            ctx.usingStreamId = stream.streamId;
            continue;
        }
        if (stream.streamId != ctx.usingStreamId) {
            ctx.switchingStreamId = stream.streamId;
            break;
        }
    }

    if (ctx.defaultVideoStreamId == -1 && !streams.empty()) {
        ctx.defaultVideoStreamId = streams.front().streamId;
    }
}

void TrySwitchStream(const std::shared_ptr<DashMediaDownloader> &mediaDownloader, int32_t defaultVideoStreamId,
    int32_t defaultAudioStreamId, int32_t switchingStreamId)
{
    if (switchingStreamId == -1) {
        return;
    }

    mediaDownloader->SetDefaultStreamId(defaultVideoStreamId, defaultAudioStreamId, switchingStreamId);
    Status status = mediaDownloader->SelectStream(switchingStreamId);
    (void)status;
    mediaDownloader->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
}
}

bool DashMediaDownloaderFuzzerTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    auto cb = std::make_shared<DashCallbackMock1>();
    cb->SetDownload(mediaDownloader);
    mediaDownloader->SetCallback(cb);
    mediaDownloader->SetIsTriggerAutoMode(true);
    std::string testUrl = MPD_MULTI_AUDIO_SUB;
    std::map<std::string, std::string> httpHeader;
    ConfigureDownloader(mediaDownloader);

    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();

    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    std::vector<StreamInfo> updatedStreams;
    mediaDownloader->GetStreamInfo(updatedStreams, true);
    StreamSwitchContext ctx = {
        -1,
        -1,
        -1,
        *reinterpret_cast<const int32_t *>(data),
        -1,
    };
    ResolveStreamIds(streams, ctx);
    mediaDownloader->SetDefaultStreamId(ctx.defaultVideoStreamId,
        ctx.defaultAudioStreamId, ctx.defaultSubtitleStreamId);
    TrySwitchStream(mediaDownloader, ctx.defaultVideoStreamId, ctx.defaultAudioStreamId, ctx.switchingStreamId);

    usleep(WAIT_FOR_SIDX_TIME);
    mediaDownloader->Close(false);
    mediaDownloader = nullptr;
    return true;
}

}
}
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::Plugins::HttpPlugin::DashMediaDownloaderFuzzerTest(data, size);
    return 0;
}