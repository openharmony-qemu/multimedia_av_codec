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
#define FUZZ_PROJECT_NAME "dashmediadownseektotime_fuzzer"

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
constexpr uint32_t BUFFER_SIZE = 1024;
}

bool DashMediaDownSeekToTimeFuzzerTest(const uint8_t *data, size_t size)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    std::string testUrl = MPD_MULTI_AUDIO_SUB;
    std::map<std::string, std::string> httpHeader;
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
    mediaDownloader->GetSeekable();

    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    unsigned char buff[BUFFER_SIZE];
    ReadDataInfo readDataInfo;
    if (streams.size() > 0) {
        readDataInfo.streamId_ = streams[0].streamId;
        readDataInfo.nextStreamId_ = streams[0].streamId;
    }
    readDataInfo.wantReadLength_ = BUFFER_SIZE;
    mediaDownloader->Read(buff, readDataInfo);

    mediaDownloader->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);

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
    OHOS::Media::Plugins::HttpPlugin::DashMediaDownSeekToTimeFuzzerTest(data, size);
    return 0;
}