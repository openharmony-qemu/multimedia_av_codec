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
#define FUZZ_PROJECT_NAME "dashmediadownselectbit_fuzzer"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

using namespace std;
using namespace OHOS::Media;

namespace {
static const std::string MPD_SEGMENT_BASE = "http://127.0.0.1:47777/test_dash/segment_base/index.mpd";
}
std::shared_ptr<DashMediaDownloader> g_mediaDownloader = nullptr;
bool g_result = false;

bool DashMediaDownSelectBitFuzzerTest(const uint8_t *data, size_t size)
{
    g_mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    g_mediaDownloader->SetStatusCallback(statusCallback);
    g_mediaDownloader->SetIsTriggerAutoMode(true);
    std::string testUrl = MPD_SEGMENT_BASE;
    std::map<std::string, std::string> httpHeader;
    g_result = g_mediaDownloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    g_mediaDownloader->GetStreamInfo(streams);

    unsigned int switchingBitrate = 0;
    int32_t usingStreamId = *reinterpret_cast<const int32_t *>(data);
    for (auto stream : streams) {
        if (stream.type != VIDEO) {
            continue;
        }

        if (usingStreamId == -1) {
            usingStreamId = stream.streamId;
            continue;
        }

        if (stream.streamId != usingStreamId) {
            switchingBitrate = stream.bitRate;
            break;
        }
    }

    bool result = false;
    if (switchingBitrate > 0) {
        result = g_mediaDownloader->SelectBitRate(switchingBitrate);
    }

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
    OHOS::Media::Plugins::HttpPlugin::DashMediaDownSelectBitFuzzerTest(data, size);
    return 0;
}