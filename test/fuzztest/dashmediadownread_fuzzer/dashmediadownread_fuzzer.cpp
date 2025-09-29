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
#define FUZZ_PROJECT_NAME "dashmediadownread_fuzzer"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

using namespace std;
using namespace OHOS::Media;
namespace {
static const std::string MPD_SEGMENT_LIST = "http://127.0.0.1:47777/test_dash/segment_list/index.mpd";
constexpr uint32_t BUFFER_SIZE = 1024;
}

bool DashMediaDownReadFuzzerTest(const uint8_t *data, size_t size)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    std::string testUrl = MPD_SEGMENT_LIST;
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);

    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();

    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);

    unsigned char buff[BUFFER_SIZE];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = streams[0].streamId;
    readDataInfo.nextStreamId_ = streams[0].streamId;
    readDataInfo.wantReadLength_ = BUFFER_SIZE;
    mediaDownloader->Read(buff, readDataInfo);
    mediaDownloader->SetDownloadErrorState();
    mediaDownloader->Read(buff, readDataInfo);
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
    OHOS::Media::Plugins::HttpPlugin::DashMediaDownReadFuzzerTest(data, size);
    return 0;
}