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
#include <iostream>
#include <atomic>
#include <fuzzer/FuzzedDataProvider.h>
#include "dash/dash_media_downloader.h"
#include "http_server_mock.h"

#define FUZZ_PROJECT_NAME "dash_stream_seek_fuzzer"

namespace OHOS::Media::Plugins::HttpPlugin {

class DashCallbackMock : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        switch (event.type) {
            case PluginEventType::STREAM_UPDATE: {
                int videoId = -1;
                int audioId = -1;
                int textId = -1;
                if (mediaDownloader) {
                    mediaDownloader->SetDefaultStreamId(videoId, audioId, textId);
                }
                break;
            }
            case PluginEventType::NETWORK_BITRATE_CHANGED: {
                break;
            }
            default:break;
        }
    }
    void SetDownload(std::shared_ptr<DashMediaDownloader> downloader)
    {
        mediaDownloader = downloader;
    }
    std::weak_ptr<DashMediaDownloader> mediaDownloader;
};

const std::string MPD_URL = "http://127.0.0.1:47777/test_dash/segment_base/index_audio_subtitle.mpd";
const std::map<std::string, std::string> g_httpHeader = {};
constexpr int32_t DEFAULT_TIME = 100;

bool DashStreamSeekFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);

    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    auto cb = std::make_shared<DashCallbackMock>();
    cb->SetDownload(mediaDownloader);
    mediaDownloader->SetCallback(cb);

    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    mediaDownloader->SetStatusCallback(statusCallback);

    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = fdp.ConsumeIntegral<uint32_t>();
    playStrategy->height = fdp.ConsumeIntegral<uint32_t>();
    playStrategy->duration = fdp.ConsumeIntegral<uint32_t>();
    playStrategy->audioLanguage = "eng";
    playStrategy->subtitleLanguage = "en_GB";
    mediaDownloader->SetPlayStrategy(playStrategy);

    mediaDownloader->Open(MPD_URL, g_httpHeader);

    Seekable seekable = mediaDownloader->GetSeekable();

    std::vector<SeekRange> seekableRanges = mediaDownloader->GetSeekableRanges();
    for (const auto& range : seekableRanges) {
        std::cout << "DASH SeekRange: start=" << range.start << ", end=" << range.end << std::endl;
    }

    int64_t mediaDuration = fdp.ConsumeIntegral<int64_t>();
    mediaDownloader->SetMediaDuration(mediaDuration);

    std::vector<StreamInfo> streams;
    bool isUpdate = fdp.ConsumeBool();
    mediaDownloader->GetStreamInfo(streams, isUpdate);
    mediaDownloader->GetStreamInfo(streams, false);
    mediaDownloader->GetStreamInfo(streams, true);

    int32_t videoStreamId = fdp.ConsumeIntegral<int32_t>();
    int32_t audioStreamId = fdp.ConsumeIntegral<int32_t>();
    int32_t subTitleStreamId = fdp.ConsumeIntegral<int32_t>();
    mediaDownloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);

    int32_t streamId = fdp.ConsumeIntegral<int32_t>();
    mediaDownloader->SelectStream(streamId);

    int64_t duration = mediaDownloader->GetDuration();

    if (seekable == Seekable::SEEKABLE) {
        int64_t seekTime = fdp.ConsumeIntegral<int64_t>();
        SeekMode mode = static_cast<SeekMode>(fdp.ConsumeIntegralInRange<uint32_t>(0, 4));
        mediaDownloader->SeekToTime(seekTime, mode);

        std::vector<SeekRange> rangesAfterSeek = mediaDownloader->GetSeekableRanges();
    }

    this_thread::sleep_for(chrono::milliseconds(DEFAULT_TIME));

    mediaDownloader->Close(false);
    mediaDownloader = nullptr;

    return true;
}

}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size <= sizeof(int64_t)) {
        return 0;
    }

    if (!InitServer()) {
        return 0;
    }

    OHOS::Media::Plugins::HttpPlugin::DashStreamSeekFuzzTest(data, size);

    CloseServer();
    return 0;
}