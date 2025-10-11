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
#include <fuzzer/FuzzedDataProvider.h>
#include "hls/hls_media_downloader.h"
#include "http_server_demo.h"
#include "http_server_mock.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins::HttpPlugin;

#define FUZZ_PROJECT_NAME "hlscfg_fuzzer"

namespace OHOS::Media::Plugins::HttpPlugin {

const map<string, string> g_httpHeader = {
    {"User-Agent", "ABC"},
    {"Referer", "DEF"},
};


bool StartFuzzTest(FuzzedDataProvider *fdp, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }

    bool userDefinedDuration = true;
    const int expectBufDuration = 10; // 10s
    std::shared_ptr<HlsMediaDownloader> hlsMediaDownloader = std::make_shared<HlsMediaDownloader>(expectBufDuration,
        userDefinedDuration, g_httpHeader, nullptr);
    
    int32_t appUid = fdp->ConsumeIntegral<uint32_t>();
    hlsMediaDownloader->SetAppUid(appUid);
    bool isInterruptNeeded = false; // fdp->ConsumeBool();
    hlsMediaDownloader->SetInterruptState(isInterruptNeeded);

    const std::string url = "http://127.0.0.1:46666/test_cbr/test_cbr.m3u8";
    cout << " Open url: " << url << endl;
    hlsMediaDownloader->Open(url, g_httpHeader);

    for (int i = 0; i < 3; i++) {   // 种子，当前48字节，仅支持3轮测试
        this_thread::sleep_for(chrono::seconds(1));
        uint64_t duration = hlsMediaDownloader->GetCachedDuration();
        vector<uint32_t> vecBitrates = hlsMediaDownloader->GetBitRates();
        cout << "Print vec bitrates: ";
        for (auto item : vecBitrates) {
            cout << item << ", ";
        }
        cout << endl;

        int32_t bitRate = fdp->ConsumeIntegralInRange<uint32_t>(0, 50000000); // 最大支持50M带宽
        int32_t streamID = fdp->ConsumeIntegral<uint32_t>();
        hlsMediaDownloader->SetCurrentBitRate(bitRate, streamID);
        hlsMediaDownloader->SelectBitRate(bitRate);

        cout << " Run in size " << size << ", appUid " << appUid << ", isInterrupt " << isInterruptNeeded
        << ", bitRate " << bitRate << ", streamID " << streamID << ", duration " << duration << endl;
        appUid = fdp->ConsumeIntegral<uint32_t>();
    }

    bool isAsync = true; // fdp->ConsumeBool();
    hlsMediaDownloader->Close(isAsync);
    hlsMediaDownloader = nullptr;

    return true;
}

} // namespace OHOS::Media::Plugins::HttpPlugin

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    if (!InitServer()) {
        cout << "Init server error" << endl;
        return -1;
    }

    OHOS::Media::Plugins::HttpPlugin::StartFuzzTest(&fdp, size);

    if (!CloseServer()) {
        cout << "Close server error" << endl;
        return -1;
    }
    return 0;
}