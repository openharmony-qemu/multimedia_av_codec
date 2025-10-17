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

constexpr int BUFF_READ_SIZE = 1 * 1024 * 1024; // buf size 1MB

const map<string, string> g_httpHeader = {
    {"User-Agent", "ABC"},
    {"Referer", "DEF"},
};

void IncreaseCoverage(std::shared_ptr<HlsMediaDownloader> &hlsMediaDownloader)
{
    hlsMediaDownloader->SetPlayStrategy(nullptr);
    auto playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->bufferDurationForPlaying = 10; // 10s
    hlsMediaDownloader->SetPlayStrategy(playStrategy);
    hlsMediaDownloader->SetInitialBufferSize(0, 50000); // buf size 50000B
    hlsMediaDownloader->SetInitialBufferSize(0, 20000000); // buf size 20000000B
}

void ReadData(std::shared_ptr<HlsMediaDownloader> &hlsMediaDownloader, int32_t streamID, uint32_t readLength,
    bool isEos)
{
    unsigned char buffRead[BUFF_READ_SIZE];

    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = streamID;
    readDataInfo.wantReadLength_ = (readLength == 0 || readLength > sizeof(buffRead))  ? sizeof(buffRead) : readLength;
    readDataInfo.isEos_ = isEos;
    hlsMediaDownloader->Read(buffRead, readDataInfo);
}

void Seek(std::shared_ptr<HlsMediaDownloader> &hlsMediaDownloader, FuzzedDataProvider *fdp)
{
    hlsMediaDownloader->SeekToTs(1, SeekMode::SEEK_NEXT_SYNC);
    this_thread::sleep_for(chrono::seconds(1));
    int32_t offset = fdp->ConsumeIntegral<uint32_t>();
    int32_t bufSize = fdp->ConsumeIntegral<uint32_t>();
    hlsMediaDownloader->SetInitialBufferSize(offset, bufSize);
}

bool StartFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdProvider(data, size);
    FuzzedDataProvider *fdp = &fdProvider;

    bool userDefinedDuration = true;
    const int expectBufDuration = 10; // 10s
    std::shared_ptr<HlsMediaDownloader> hlsMediaDownloader = std::make_shared<HlsMediaDownloader>(expectBufDuration,
        userDefinedDuration, g_httpHeader, nullptr);
    hlsMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    hlsMediaDownloader->SetStatusCallback(statusCallback);

    IncreaseCoverage(hlsMediaDownloader);

    int32_t appUid = fdp->ConsumeIntegral<uint32_t>();
    hlsMediaDownloader->SetAppUid(appUid);
    bool isInterruptNeeded = false; // fdp->ConsumeBool();
    hlsMediaDownloader->SetInterruptState(isInterruptNeeded);
    bool isTriggerAutoMode = false;
    hlsMediaDownloader->SetIsTriggerAutoMode(isTriggerAutoMode);

    const std::string url = "http://127.0.0.1:46666/test_cbr/test_cbr.m3u8";
    cout << "  Open url: " << url << endl;
    hlsMediaDownloader->Open(url, g_httpHeader);
    int32_t streamID = 0;
    ReadData(hlsMediaDownloader, streamID, 0, false);

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
        streamID = fdp->ConsumeIntegral<uint32_t>();
        hlsMediaDownloader->SetCurrentBitRate(bitRate, streamID);
        hlsMediaDownloader->SelectBitRate(bitRate);

        cout << " Run in size " << size << ", appUid " << appUid << ", isInterrupt " << isInterruptNeeded
        << ", bitRate " << bitRate << ", streamID " << streamID << ", duration " << duration << endl;
        appUid = fdp->ConsumeIntegral<uint32_t>();
        ReadData(hlsMediaDownloader, streamID, 10, true); // 10s
    }

    ReadData(hlsMediaDownloader, streamID, BUFF_READ_SIZE, true);
    Seek(hlsMediaDownloader, fdp);

    bool isAsync = true; // fdp->ConsumeBool();
    hlsMediaDownloader->Close(isAsync);
    hlsMediaDownloader = nullptr;

    return true;
}

} // namespace OHOS::Media::Plugins::HttpPlugin

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size <= sizeof(int64_t)) {
        return false;
    }
    /* Run your code on data */
    if (!InitServer()) {
        cout << "Init server error" << endl;
        return -1;
    }

    OHOS::Media::Plugins::HttpPlugin::StartFuzzTest(data, size);

    if (!CloseServer()) {
        cout << "Close server error" << endl;
        return -1;
    }
    return 0;
}