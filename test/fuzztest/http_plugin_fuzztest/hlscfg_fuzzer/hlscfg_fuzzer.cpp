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
#include "hls/hls_segment_manager.h"
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
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        (void)event;
    }

    void SetSelectBitRateFlag(bool flag, uint32_t desBitRate) override
    {
        (void)flag;
        (void)desBitRate;
    }

    bool CanAutoSelectBitRate() override
    {
        return true;
    }
};

constexpr int BUFF_READ_SIZE = 5 * 1024 * 1024; // buf size 5MB
constexpr int READ_TIMES_5S = 1000 * 5;         // 5000MS
constexpr int READ_TIMES_NS = 100;             // 100MS
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
    std::string("test_hls/testHLSEncode.m3u8"),
    std::string("test_hls/testHLSEncode_session_key.m3u8"),
    std::string("test_hls/testXMap.m3u8"),
    std::string("test_hls/testByteRange.m3u8"),
    std::string("test_hls/testMutiStream.m3u8"),
    std::string("test_hls/testHlsLive.m3u8"),
};

void IncreaseCoverageSet(std::shared_ptr<HlsSegmentManager> &hlsSegmentManager)
{
    hlsSegmentManager->SetPlayStrategy(nullptr);
    auto playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->bufferDurationForPlaying = 10; // 10s
    hlsSegmentManager->SetPlayStrategy(playStrategy);
    hlsSegmentManager->SetInitialBufferSize(0, 50000); // buf size 50000B
    hlsSegmentManager->SetInitialBufferSize(0, 20000000); // buf size 20000000B

    bool isInterruptNeeded = false;
    hlsSegmentManager->SetInterruptState(isInterruptNeeded);
    bool isTriggerAutoMode = false;
    hlsSegmentManager->SetIsTriggerAutoMode(isTriggerAutoMode);

    hlsSegmentManager->SetReadBlockingFlag(true);
}

void IncreaseCoverageRead(std::shared_ptr<HlsSegmentManager> &hlsSegmentManager)
{
    hlsSegmentManager->ReportVideoSizeChange();

    hlsSegmentManager->GetPlayable();

    hlsSegmentManager->GetStartedStatus();

    PlaybackInfo playbackInfo;
    hlsSegmentManager->GetPlaybackInfo(playbackInfo);

    std::vector<StreamInfo> streams;
    hlsSegmentManager->GetStreamInfo(streams);

    DownloadInfo downloadInfo;
    hlsSegmentManager->GetDownloadInfo(downloadInfo);
    hlsSegmentManager->GetDownloadInfo();

    hlsSegmentManager->GetContentType();
    hlsSegmentManager->GetContentLength();
    hlsSegmentManager->GetTotalBufferSize();
    hlsSegmentManager->IsHlsFmp4();
    hlsSegmentManager->GetMemorySize();
}

Status ReadData(std::shared_ptr<HlsSegmentManager> &hlsSegmentManager, int32_t streamID, uint32_t readLength,
    bool isEos)
{
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = streamID;
    readDataInfo.wantReadLength_ = (readLength == 0 || readLength > sizeof(g_buffRead))
        ? sizeof(g_buffRead) : readLength;
    readDataInfo.isEos_ = isEos;
    return hlsSegmentManager->Read(g_buffRead, readDataInfo);
}

void Seek(std::shared_ptr<HlsSegmentManager> &hlsSegmentManager, FuzzedDataProvider *fdp)
{
    hlsSegmentManager->SeekToTs(1, SeekMode::SEEK_NEXT_SYNC);
    this_thread::sleep_for(chrono::seconds(1));
    int32_t offset = fdp->ConsumeIntegral<uint32_t>();
    int32_t bufSize = fdp->ConsumeIntegral<uint32_t>();
    hlsSegmentManager->SetInitialBufferSize(offset, bufSize);
}

std::shared_ptr<HlsSegmentManager> CreateFuzzTestObj(bool useCase,
    FuzzedDataProvider *fdp, std::shared_ptr<Plugins::Callback> &sourceCb)
{
    std::shared_ptr<HlsSegmentManager> hlsSegmentManager= nullptr;
    sourceCb = nullptr;

    if (fdp == nullptr) {
        std::string mimeType{AVMimeTypes::APPLICATION_M3U8.data(), AVMimeTypes::APPLICATION_M3U8.size()};
        hlsSegmentManager = std::make_shared<HlsSegmentManager>(mimeType, HlsSegmentType::SEG_VIDEO, g_httpHeader);
    } else {
        const int expectBufDuration = useCase ? fdp->ConsumeIntegralInRange<uint32_t>(0, 20) : 10; // 20、10s
        bool userDefinedDuration = useCase ? !fdp->ConsumeBool() : fdp->ConsumeBool();
        hlsSegmentManager = std::make_shared<HlsSegmentManager>(expectBufDuration,
            userDefinedDuration, g_httpHeader);
    }
    if (hlsSegmentManager == nullptr) {
        return nullptr;
    }

    hlsSegmentManager->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    hlsSegmentManager->SetStatusCallback(statusCallback);

    sourceCb = std::make_shared<SourceCallback>();
    if (sourceCb == nullptr) {
        return nullptr;
    }
    hlsSegmentManager->SetCallback(sourceCb);
    return hlsSegmentManager;
}

bool StartFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdProvider(data, size);
    FuzzedDataProvider *fdp = &fdProvider;
    std::shared_ptr<Plugins::Callback> sourceCb = nullptr;
    std::shared_ptr<HlsSegmentManager> hlsSegmentManager = CreateFuzzTestObj(true, nullptr, sourceCb);
    if (hlsSegmentManager == nullptr) {
        cout << "  Memory 1 apply failed." << endl;
        return false;
    }

    IncreaseCoverageSet(hlsSegmentManager);

    const std::string url = g_urlIPPort + g_urls[0];
    cout << "  Open url: " << url << endl;
    hlsSegmentManager->Open(url, g_httpHeader);

    int32_t appUid = fdp->ConsumeIntegral<uint32_t>();
    hlsSegmentManager->SetAppUid(appUid);
    int32_t streamID = 0;
    ReadData(hlsSegmentManager, streamID, 0, false);
    IncreaseCoverageRead(hlsSegmentManager);
    hlsSegmentManager->SetDemuxerState(streamID);
    hlsSegmentManager->NotifyInitSuccess();

    for (int i = 0; i < 3; i++) {   // 种子，当前48字节，仅支持3轮测试
        this_thread::sleep_for(chrono::seconds(1));
        uint64_t duration = hlsSegmentManager->GetCachedDuration();
        vector<uint32_t> vecBitrates = hlsSegmentManager->GetBitRates();
        cout << "Print vec bitrates: ";
        for (auto item : vecBitrates) {
            cout << item << ", ";
        }
        cout << endl;

        int32_t bitRate = fdp->ConsumeIntegralInRange<uint32_t>(0, 50000000); // 最大支持50M带宽
        streamID = fdp->ConsumeIntegral<uint32_t>();
        hlsSegmentManager->SetCurrentBitRate(bitRate, streamID);
        hlsSegmentManager->SelectBitRate(bitRate);

        cout << " Run in size " << size << ", appUid " << appUid << ", bitRate " << bitRate
            << ", streamID " << streamID << ", duration " << duration << endl;
        appUid = fdp->ConsumeIntegral<uint32_t>();
        ReadData(hlsSegmentManager, streamID, 10, true); // 10s
    }

    ReadData(hlsSegmentManager, streamID, BUFF_READ_SIZE, true);
    Seek(hlsSegmentManager, fdp);
    IncreaseCoverageSet(hlsSegmentManager);

    bool isAsync = true;
    hlsSegmentManager->Close(isAsync);
    hlsSegmentManager = nullptr;

    return true;
}

bool StartFuzzTestRead(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdProvider(data, size);
    FuzzedDataProvider *fdp = &fdProvider;
    std::shared_ptr<Plugins::Callback> sourceCb = nullptr;
    std::shared_ptr<HlsSegmentManager> hlsSegmentManager = CreateFuzzTestObj(true, fdp, sourceCb);
    if (hlsSegmentManager == nullptr) {
        cout << "  Memory 2 apply failed." << endl;
        return false;
    }

    const std::string url = g_urlIPPort + g_urls[0];
    hlsSegmentManager->Open(url, g_httpHeader);
    hlsSegmentManager->Resume();
    hlsSegmentManager->Pause();
    hlsSegmentManager->Pause();
    hlsSegmentManager->Resume();

    int32_t streamID = 0;
    uint32_t readLen = fdp->ConsumeIntegralInRange<uint32_t>(10, BUFF_READ_SIZE); // 10B
    bool isDemuxerInit = false;
    for (int i = 0; i < READ_TIMES_5S; i++) {
        this_thread::sleep_for(chrono::milliseconds(1));
        Status status = ReadData(hlsSegmentManager, streamID, readLen, fdp->ConsumeBool());
        if (!isDemuxerInit && status == Status::OK) {
            hlsSegmentManager->SetDemuxerState(streamID);
            hlsSegmentManager->NotifyInitSuccess();
            isDemuxerInit = true;
        }

        if (i % 1000 == 0) {    // 间隔 1000 ms，打印一次
            IncreaseCoverageRead(hlsSegmentManager);

            int64_t duration = hlsSegmentManager->GetDuration();
            int64_t durationCached = hlsSegmentManager->GetCachedDuration();
            cout << " Run read in size " << size << ", duration " << duration
                << ", durationCached " << durationCached << endl;
        }
        if (hlsSegmentManager->IsHlsEnd()) {
            break;
        }
    }

    int64_t seekTime = 1000; // 1000 ts
    SeekMode mode = static_cast<SeekMode>(
        fdp->ConsumeIntegralInRange<uint32_t>(0, uint32_t(SeekMode::SEEK_CLOSEST_INNER) + 1));
    hlsSegmentManager->SeekToTime(seekTime, mode);
    hlsSegmentManager->HandleSeekReady(streamID, fdp->ConsumeBool());
    hlsSegmentManager->SetInterruptState(fdp->ConsumeBool());
    hlsSegmentManager->Close(true);
    hlsSegmentManager = nullptr;

    return true;
}

bool StartFuzzTestMultiUrl(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdProvider(data, size);
    FuzzedDataProvider *fdp = &fdProvider;
    std::shared_ptr<Plugins::Callback> sourceCb = nullptr;
    std::shared_ptr<HlsSegmentManager> hlsSegmentManager = CreateFuzzTestObj(false, fdp, sourceCb);
    if (hlsSegmentManager == nullptr) {
        cout << "  Memory 3 apply failed." << endl;
        return false;
    }

    int32_t urlStart = fdp->ConsumeIntegralInRange<uint32_t>(0, sizeof(g_urls) / sizeof(g_urls[0]) - 1);
    for (int32_t index = 0; index < sizeof(g_urls) / sizeof(g_urls[0]); index++) {
        urlStart = (urlStart + index) % (sizeof(g_urls) / sizeof(g_urls[0]));
        const std::string url = g_urlIPPort + g_urls[urlStart];
        cout << " Run multi in size " << size << ", urlStart " << urlStart << ", open url: " << url << endl;
        hlsSegmentManager->Open(url, g_httpHeader);
        int32_t streamID = 0;
        bool isDemuxerInit = false;
        for (int i = 0; i < READ_TIMES_NS; i++) {
            this_thread::sleep_for(chrono::milliseconds(1));
            Status status = ReadData(hlsSegmentManager, streamID, BUFF_READ_SIZE, fdp->ConsumeBool());
            if (!isDemuxerInit && status == Status::OK) {
                hlsSegmentManager->SetDemuxerState(streamID);
                hlsSegmentManager->NotifyInitSuccess();
                isDemuxerInit = true;
            }

            if (i % 1000 == 0) {    // 间隔 1000 ms，打印一次
                IncreaseCoverageRead(hlsSegmentManager);

                int64_t duration = hlsSegmentManager->GetDuration();
                int64_t durationCached = hlsSegmentManager->GetCachedDuration();
                cout << "  Times " << i << ", duration " << duration << ", durationCached " << durationCached << endl;
            }
            if (hlsSegmentManager->IsHlsEnd()) {
                break;
            }
        }
    }

    hlsSegmentManager->SetInterruptState(true);
    hlsSegmentManager->Close(fdp->ConsumeBool());
    hlsSegmentManager = nullptr;

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

    OHOS::Media::Plugins::HttpPlugin::StartFuzzTestRead(data, size);

    OHOS::Media::Plugins::HttpPlugin::StartFuzzTestMultiUrl(data, size);

    if (!CloseServer()) {
        cout << "Close server error" << endl;
        return -1;
    }
    return 0;
}