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
#include "plugin/plugin_base.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins::HttpPlugin;

#define FUZZ_PROJECT_NAME "hlscfg_fuzzer"

namespace OHOS::Media::Plugins::HttpPlugin {

class SourceCallback : public Plugins::Callback {
    public:
        void OnEvent(const Plugins::PluginEvent &event)
        {
            (void)event;
        }
    
        void SetSelectBitRateFlag(bool flag, uint32_t desBitRate)
        {
            (void)flag;
            (void)desBitRate;
        }
    
        bool CanDoSelectBitRate()
        {
            return true;
        }
    };

constexpr int BUFF_READ_SIZE = 5 * 1024 * 1024; // buf size 5MB
constexpr int READ_TIMES_5S = 1000 * 5;         // 5000MS
constexpr int READ_TIMES_NS = 1500;             // 1500MS
unsigned char g_buffRead[BUFF_READ_SIZE];

const map<string, string> g_httpHeader = {
    {"User-Agent", "ABC"},
    {"Referer", "DEF"},
};

const std::string g_urlIPPort = "http://127.0.0.1:46666/";

const std::string g_urls[] = {
    std::string("test_cbr/test_cbr.m3u8"),
    std::string("test_cbr/1080_3M/video_1080.m3u8"),
    std::string("test_cbr/720_1M/video_720_null.m3u8"),
    std::string("test_cbr/720_1M/video_720_live.m3u8"),
    std::string("test_hls/testHLSEncode.m3u8 "),
    std::string("test_hls/testHLSEncode_session_key.m3u8"),
    std::string("test_hls/testXMap.m3u8"),
    std::string("test_hls/testByteRange.m3u8"),
    std::string("test_hls/testMutiStream.m3u8"),
    std::string("test_hls/testHlsLive.m3u8"),
};

void IncreaseCoverageSet(std::shared_ptr<HlsMediaDownloader> &hlsMediaDownloader)
{
    hlsMediaDownloader->SetPlayStrategy(nullptr);
    auto playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->bufferDurationForPlaying = 10; // 10s
    hlsMediaDownloader->SetPlayStrategy(playStrategy);
    hlsMediaDownloader->SetInitialBufferSize(0, 50000); // buf size 50000B
    hlsMediaDownloader->SetInitialBufferSize(0, 20000000); // buf size 20000000B
}

void IncreaseCoverageRead(std::shared_ptr<HlsMediaDownloader> &hlsMediaDownloader)
{
    hlsMediaDownloader->ReportVideoSizeChange();
    
    hlsMediaDownloader->GetPlayable();

    PlaybackInfo playbackInfo;
    hlsMediaDownloader->GetPlaybackInfo(playbackInfo);

    DownloadInfo downloadInfo;
    hlsMediaDownloader->GetDownloadInfo(downloadInfo);
}

void ReadData(std::shared_ptr<HlsMediaDownloader> &hlsMediaDownloader, int32_t streamID, uint32_t readLength,
    bool isEos)
{
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = streamID;
    readDataInfo.wantReadLength_ = (readLength == 0 || readLength > sizeof(g_buffRead))
        ? sizeof(g_buffRead) : readLength;
    readDataInfo.isEos_ = isEos;
    hlsMediaDownloader->Read(g_buffRead, readDataInfo);
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

    IncreaseCoverageSet(hlsMediaDownloader);

    int32_t appUid = fdp->ConsumeIntegral<uint32_t>();
    hlsMediaDownloader->SetAppUid(appUid);
    bool isInterruptNeeded = false; // fdp->ConsumeBool();
    hlsMediaDownloader->SetInterruptState(isInterruptNeeded);
    bool isTriggerAutoMode = false;
    hlsMediaDownloader->SetIsTriggerAutoMode(isTriggerAutoMode);

    const std::string url = g_urlIPPort + g_urls[0];
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

bool StartFuzzTestRead(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdProvider(data, size);
    FuzzedDataProvider *fdp = &fdProvider;

    bool userDefinedDuration = !fdp->ConsumeBool();
    const int expectBufDuration = 10; // 10s
    std::shared_ptr<HlsMediaDownloader> hlsMediaDownloader = std::make_shared<HlsMediaDownloader>(expectBufDuration,
        userDefinedDuration, g_httpHeader, nullptr);
    hlsMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    hlsMediaDownloader->SetStatusCallback(statusCallback);

    const std::string url = g_urlIPPort + g_urls[0];
    hlsMediaDownloader->Open(url, g_httpHeader);
    hlsMediaDownloader->GetSeekable();
    hlsMediaDownloader->GetSeekable();
    hlsMediaDownloader->Resume();
    hlsMediaDownloader->Pause();
    hlsMediaDownloader->Pause();
    hlsMediaDownloader->Resume();

    int32_t streamID = 0;
    for (int i = 0; i < READ_TIMES_5S; i++) {
        this_thread::sleep_for(chrono::milliseconds(1));
        ReadData(hlsMediaDownloader, streamID, BUFF_READ_SIZE, fdp->ConsumeBool());

        if (i % 1000 == 0) {    // 间隔 1000 ms，打印一次
            IncreaseCoverageRead(hlsMediaDownloader);

            int64_t duration = hlsMediaDownloader->GetDuration();
            int64_t durationCached = hlsMediaDownloader->GetCachedDuration();
            cout << " Run read in size " << size << ", duration " << duration
                << ", durationCached " << durationCached << endl;
        }
    }

    hlsMediaDownloader->GetContentType();
    hlsMediaDownloader->GetContentLength();
    int64_t seekTime = 1000; // 1000 ts
    SeekMode mode = static_cast<SeekMode>(
        fdp->ConsumeIntegralInRange<uint32_t>(0, uint32_t(SeekMode::SEEK_CLOSEST_INNER) + 1));
    hlsMediaDownloader->SeekToTime(seekTime, mode);
    hlsMediaDownloader->HandleSeekReady(0, streamID, fdp->ConsumeBool());
    hlsMediaDownloader->GetTotalBufferSize();
    hlsMediaDownloader->IsHlsFmp4();
    hlsMediaDownloader->GetMemorySize();
    hlsMediaDownloader->IsHlsEnd();
    hlsMediaDownloader->SetInterruptState(fdp->ConsumeBool());
    hlsMediaDownloader->Close(true);
    hlsMediaDownloader = nullptr;

    return true;
}

bool StartFuzzTestMultiUrl(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdProvider(data, size);
    FuzzedDataProvider *fdp = &fdProvider;

    bool userDefinedDuration = !fdp->ConsumeBool();
    const int expectBufDuration = 10; // 10s
    std::shared_ptr<HlsMediaDownloader> hlsMediaDownloader = std::make_shared<HlsMediaDownloader>(expectBufDuration,
        userDefinedDuration, g_httpHeader, nullptr);
    hlsMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    hlsMediaDownloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCb = new SourceCallback();
    hlsMediaDownloader->SetCallback(sourceCb);

    int32_t index = fdp->ConsumeIntegralInRange<uint32_t>(0, sizeof(g_urls) / sizeof(g_urls[0]) - 1);
    const std::string url = g_urlIPPort + g_urls[index];
    cout << " Run multi in size " << size << ", index " << index << ", open url: " << url << endl;
    hlsMediaDownloader->Open(url, g_httpHeader);

    int32_t streamID = 0;
    for (int i = 0; i < READ_TIMES_NS; i++) {
        this_thread::sleep_for(chrono::milliseconds(1));
        ReadData(hlsMediaDownloader, streamID, BUFF_READ_SIZE, fdp->ConsumeBool());

        if (i % 1000 == 0) {    // 间隔 1000 ms，打印一次
            IncreaseCoverageRead(hlsMediaDownloader);

            int64_t duration = hlsMediaDownloader->GetDuration();
            int64_t durationCached = hlsMediaDownloader->GetCachedDuration();
            cout << "  Times " << i << ", duration " << duration << ", durationCached " << durationCached << endl;
        }
    }

    hlsMediaDownloader->SetInterruptState(true);
    hlsMediaDownloader->Close(fdp->ConsumeBool());
    hlsMediaDownloader->SetCallback(nullptr);
    delete sourceCb;
    sourceCb = nullptr;
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