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
#include "mpd_parser/dash_mpd_parser.h"
#include "mpd_parser/dash_mpd_manager.h"
#include "mpd_parser/dash_period_manager.h"
#include "mpd_parser/dash_adpt_set_manager.h"
#include "mpd_parser/i_dash_mpd_node.h"
#include "dash_mpd_downloader.h"
#include "base64_utils.h"
#define FUZZ_PROJECT_NAME "dashmpddownfinished_fuzzer"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
static const std::string MPD_SEGMENT_LIST_TIMELINE = "http://127.0.0.1:47777/test_dash/segment_list/index_timeline.mpd";
}
using namespace std;
using namespace OHOS::Media;

bool DashMpdDownFinishedFuzzerTest(const uint8_t *data, size_t size)
{
    auto downloader = std::make_shared<DashMpdDownloader>();
    downloader->Init();
    std::string testUrl = MPD_SEGMENT_LIST_TIMELINE;
    downloader->Open(testUrl);
    downloader->GetSeekable();
    downloader->GetDuration();
    downloader->UpdateDownloadFinished(testUrl);
    downloader->Close(*reinterpret_cast<bool *>(data););
    downloader = nullptr;
    return false;
}

}
}
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::Plugins::HttpPlugin::DashMpdDownFinishedFuzzerTest(data, size);
    return 0;
}