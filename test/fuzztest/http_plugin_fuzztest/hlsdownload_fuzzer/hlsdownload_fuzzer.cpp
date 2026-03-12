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
#include <fuzzer/FuzzedDataProvider.h>
#include "hls/hls_segment_manager.h"
#include "http_server_mock.h"
#include "test_template.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::Media::Plugins::HttpPlugin;

#define FUZZ_PROJECT_NAME "hlsm3u8mutate_fuzzer"

namespace OHOS::Media::Plugins::HttpPlugin {

const map<string, string> g_httpHeader = {
    {"User-Agent", "ABC"},
    {"Referer", "DEF"},
};

bool StartFuzzTest(FuzzedDataProvider *fdp, size_t size)
{
    bool userDefinedDuration = true;
    const int expectBufDuration = 10; // 10s
    std::shared_ptr<HlsSegmentManager> hlsSegmentManager = std::make_shared<HlsSegmentManager>(expectBufDuration,
        userDefinedDuration, g_httpHeader);
    hlsSegmentManager->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    hlsSegmentManager->SetStatusCallback(statusCallback);

    int32_t appUid = fdp->ConsumeIntegral<uint32_t>();
    hlsSegmentManager->SetAppUid(appUid);
    bool isInterruptNeeded = false; // fdp->ConsumeBool();
    hlsSegmentManager->SetInterruptState(isInterruptNeeded);

    std::string url = "http://127.0.0.1:46666/test_cbr/";
    url += FAKE_FUZZ_M3U8;  // 虚假m3u8文件，需通过 fuzz data 喂入
    cout << "  Open url: " << url << endl;
    hlsSegmentManager->Open(url, g_httpHeader);

    uint64_t duration = hlsSegmentManager->GetCachedDuration();
    int32_t bitRate = fdp->ConsumeIntegralInRange<uint32_t>(0, 50000000); // 最大支持50M带宽
    int32_t streamID = fdp->ConsumeIntegral<uint32_t>();
    hlsSegmentManager->SetCurrentBitRate(bitRate, streamID);
    hlsSegmentManager->SelectBitRate(bitRate);

    cout << " Run in size " << size << ", appUid " << appUid << ", isInterrupt " << isInterruptNeeded
    << ", bitRate " << bitRate << ", streamID " << streamID << ", duration " << duration << endl;
    this_thread::sleep_for(chrono::seconds(1));

    bool isAsync = true; // fdp->ConsumeBool();
    hlsSegmentManager->Close(isAsync);
    hlsSegmentManager = nullptr;

    return true;
}

bool SegMentFuzzTest(FuzzedDataProvider *fdp)
{
    std::string mimeType = "audio";
    std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };
    std::shared_ptr<HlsSegmentManager> hlsSegmentManager = std::make_shared<HlsSegmentManager>(mimeType,
        HlsSegmentType::SEG_VIDEO, httpHeader);
    std::shared_ptr<HlsSegmentManager> otherSegmentManager = std::make_shared<HlsSegmentManager>(mimeType,
        HlsSegmentType::SEG_AUDIO, httpHeader);
    std::multimap<std::string, std::vector<uint8_t>> drmInfos;
    hlsSegmentManager->Clone(otherSegmentManager);
    std::string url = "http://127.0.0.1:46666/test_cbr/";
    url += FAKE_FUZZ_M3U8;  // 虚假m3u8文件，需通过 fuzz data 喂入
    hlsSegmentManager->Open(url, g_httpHeader);
    hlsSegmentManager->SetIsReportedErrorCode();
    int32_t streamId = fdp->ConsumeIntegral<int32_t>();
    hlsSegmentManager->SelectMedia(streamId, HlsSegmentType::SEG_AUDIO);
    hlsSegmentManager->StartMediaDownload(streamId, HlsSegmentType::SEG_AUDIO);
    hlsSegmentManager->OnDrmInfoChanged(drmInfos);
    hlsSegmentManager->GetDownloadRateAndSpeed();
    bool isDelay = fdp->ConsumeIntegral<bool>();
    hlsSegmentManager->GetReadTimeOut(isDelay);
    hlsSegmentManager->GetSegmentOffset();
    hlsSegmentManager->GetHLSDiscontinuity();
    bool isAppBackground = fdp->ConsumeIntegral<bool>();
    hlsSegmentManager->StopBufferring(isAppBackground);
    hlsSegmentManager->WaitForBufferingEnd();
    hlsSegmentManager->GetTotalTsBuffersize();
    hlsSegmentManager->GetStreamInfoById(streamId);
    hlsSegmentManager->GetDefaultMediaStreamId(HlsSegmentType::SEG_AUDIO);
    hlsSegmentManager->GetSegType(streamId);
    bool isAsync = true; // fdp->ConsumeBool();
    hlsSegmentManager->Close(isAsync);
    hlsSegmentManager = nullptr;
    return true;
}

} // namespace OHOS::Media::Plugins::HttpPlugin


/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (!InitServer(data, size)) {
        cout << "Init server error" << endl;
        return -1;
    }
    g_baseFuzzData = data;
    g_baseFuzzSize = size;
    g_baseFuzzPos = 0;
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::Plugins::HttpPlugin::StartFuzzTest(&fdp, size);
    OHOS::Media::Plugins::HttpPlugin::SegMentFuzzTest(data, size);
    if (!CloseServer()) {
        cout << "Close server error" << endl;
        return -1;
    }
    return 0;
}