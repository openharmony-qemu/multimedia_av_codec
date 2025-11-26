/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include <fuzzer/FuzzedDataProvider.h>
#include "http_server_demo.h"
#include "http_server_mock.h"
#include "test_template.h"
#define FUZZ_PROJECT_NAME "dashmediadownseektotime_fuzzer"
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins::HttpPlugin;
namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {


namespace {
static const std::string MPD_MULTI_AUDIO_SUB =
    "http://127.0.0.1:46666/test_dash/segment_base2/index_audio_subtitle.mpd";
constexpr int32_t WAIT_FOR_SIDX_TIME = 1000 * 1000;
constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;
constexpr uint32_t DEFAULT_DURATION = 20;
}

bool DashMediaDownFinishedFuzzerTest(const uint8_t *data, size_t size)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    std::string testUrl = MPD_MULTI_AUDIO_SUB;
    std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };
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

    mediaDownloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    for (auto u : streams) {
        int32_t type = GetData<int32_t>()%4;
        type += 1;
        int32_t streamid = u.streamId;
        mediaDownloader->SelectStream(streamid);
        mediaDownloader->NotifyInitSuccess();
    }
    usleep(WAIT_FOR_SIDX_TIME);
    int32_t biterate = GetData<int32_t>();
    mediaDownloader->SelectBitRate(biterate);
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
    if (!InitServer()) {
        cout << "Init server error" << endl;
        return -1;
    }
    OHOS::Media::Plugins::HttpPlugin::DashMediaDownFinishedFuzzerTest(data, size);
    if (!CloseServer()) {
        cout << "Close server error" << endl;
        return -1;
    }
    return 0;
}