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
constexpr int READ_TIMES_NS = 1010;             // 1010MS
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

void IncreaseCoverageSet(std::shared_ptr<HlsMediaDownloader> &hlsMediaDownloader)
{
    hlsMediaDownloader->SetPlayStrategy(nullptr);
    auto playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->bufferDurationForPlaying = 10; // 10s
    hlsMediaDownloader->SetPlayStrategy(playStrategy);
    hlsMediaDownloader->SetInitialBufferSize(0, 50000); // buf size 50000B
    hlsMediaDownloader->SetInitialBufferSize(0, 20000000); // buf size 20000000B

    bool isInterruptNeeded = false;
    hlsMediaDownloader->SetInterruptState(isInterruptNeeded);
    bool isTriggerAutoMode = false;
    hlsMediaDownloader->SetIsTriggerAutoMode(isTriggerAutoMode);

    hlsMediaDownloader->SetReadBlockingFlag(true);
}

void IncreaseCoverageRead(std::shared_ptr<HlsMediaDownloader> &hlsMediaDownloader)
{
    hlsMediaDownloader->GetPlayable();

    hlsMediaDownloader->GetStartedStatus();

    PlaybackInfo playbackInfo;
    hlsMediaDownloader->GetPlaybackInfo(playbackInfo);

    std::vector<StreamInfo> streams;
    hlsMediaDownloader->GetStreamInfo(streams);

    DownloadInfo downloadInfo;
    hlsMediaDownloader->GetDownloadInfo(downloadInfo);
    hlsMediaDownloader->GetDownloadInfo();

    hlsMediaDownloader->GetContentType();
    hlsMediaDownloader->GetContentLength();
    hlsMediaDownloader->GetBufferSize();
    hlsMediaDownloader->IsHlsFmp4();
    hlsMediaDownloader->GetMemorySize();
}

Status ReadData(std::shared_ptr<HlsMediaDownloader> &hlsMediaDownloader, int32_t streamID, uint32_t readLength,
    bool isEos)
{
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = streamID;
    readDataInfo.wantReadLength_ = (readLength == 0 || readLength > sizeof(g_buffRead))
        ? sizeof(g_buffRead) : readLength;
    readDataInfo.isEos_ = isEos;
    return hlsMediaDownloader->Read(g_buffRead, readDataInfo);
}

void Seek(std::shared_ptr<HlsMediaDownloader> &hlsMediaDownloader, FuzzedDataProvider *fdp)
{
    hlsMediaDownloader->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    this_thread::sleep_for(chrono::seconds(1));
    int32_t offset = fdp->ConsumeIntegral<uint32_t>();
    int32_t bufSize = fdp->ConsumeIntegral<uint32_t>();
    hlsMediaDownloader->SetInitialBufferSize(offset, bufSize);
}

std::shared_ptr<HlsMediaDownloader> CreateFuzzTestObj(bool useCase,
    FuzzedDataProvider *fdp, Plugins::Callback* &sourceCb)
{
    std::shared_ptr<HlsMediaDownloader> hlsMediaDownloader= nullptr;
    sourceCb = nullptr;

    if (fdp == nullptr) {
        std::string mimeType{AVMimeTypes::APPLICATION_M3U8.data(), AVMimeTypes::APPLICATION_M3U8.size()};
        hlsMediaDownloader = std::make_shared<HlsMediaDownloader>(mimeType, g_httpHeader);
    } else {
        const int expectBufDuration = useCase ? fdp->ConsumeIntegralInRange<uint32_t>(0, 20) : 10; // 20、10s
        bool userDefinedDuration = useCase ? !fdp->ConsumeBool() : fdp->ConsumeBool();
        hlsMediaDownloader = std::make_shared<HlsMediaDownloader>(expectBufDuration,
            userDefinedDuration, g_httpHeader, nullptr);
    }
    if (hlsMediaDownloader == nullptr) {
        return nullptr;
    }

    hlsMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    hlsMediaDownloader->SetStatusCallback(statusCallback);

    sourceCb = new SourceCallback();
    if (sourceCb == nullptr) {
        return nullptr;
    }
    hlsMediaDownloader->SetCallback(sourceCb);
    return hlsMediaDownloader;
}

bool StartFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdProvider(data, size);
    FuzzedDataProvider *fdp = &fdProvider;
    Plugins::Callback* sourceCb = nullptr;
    std::shared_ptr<HlsMediaDownloader> hlsMediaDownloader = CreateFuzzTestObj(true, nullptr, sourceCb);
    if (hlsMediaDownloader == nullptr) {
        cout << "  Memory 1 apply failed." << endl;
        return false;
    }

    IncreaseCoverageSet(hlsMediaDownloader);

    const std::string url = g_urlIPPort + g_urls[0];
    cout << "  Open url: " << url << endl;
    hlsMediaDownloader->Open(url, g_httpHeader);

    int32_t appUid = fdp->ConsumeIntegral<uint32_t>();
    hlsMediaDownloader->SetAppUid(appUid);
    int32_t streamID = 0;
    ReadData(hlsMediaDownloader, streamID, 0, false);
    IncreaseCoverageRead(hlsMediaDownloader);
    hlsMediaDownloader->SetDemuxerState(streamID);
    hlsMediaDownloader->NotifyInitSuccess();

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

        cout << " Run in size " << size << ", appUid " << appUid << ", bitRate " << bitRate
            << ", streamID " << streamID << ", duration " << duration << endl;
        appUid = fdp->ConsumeIntegral<uint32_t>();
        ReadData(hlsMediaDownloader, streamID, 10, true); // 10s
    }

    ReadData(hlsMediaDownloader, streamID, BUFF_READ_SIZE, true);
    Seek(hlsMediaDownloader, fdp);
    IncreaseCoverageSet(hlsMediaDownloader);

    bool isAsync = true;
    hlsMediaDownloader->Close(isAsync);
    hlsMediaDownloader->SetCallback(nullptr);
    delete sourceCb;
    sourceCb = nullptr;
    hlsMediaDownloader = nullptr;

    return true;
}

bool StartFuzzTestRead(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdProvider(data, size);
    FuzzedDataProvider *fdp = &fdProvider;
    Plugins::Callback* sourceCb = nullptr;
    std::shared_ptr<HlsMediaDownloader> hlsMediaDownloader = CreateFuzzTestObj(true, fdp, sourceCb);
    if (hlsMediaDownloader == nullptr) {
        cout << "  Memory 2 apply failed." << endl;
        return false;
    }

    const std::string url = g_urlIPPort + g_urls[0];
    hlsMediaDownloader->Open(url, g_httpHeader);
    hlsMediaDownloader->Resume();
    hlsMediaDownloader->Pause();
    hlsMediaDownloader->Pause();
    hlsMediaDownloader->Resume();

    int32_t streamID = 0;
    uint32_t readLen = fdp->ConsumeIntegralInRange<uint32_t>(10, BUFF_READ_SIZE); // 10B
    bool isDemuxerInit = false;
    for (int i = 0; i < READ_TIMES_5S; i++) {
        this_thread::sleep_for(chrono::milliseconds(1));
        Status status = ReadData(hlsMediaDownloader, streamID, readLen, fdp->ConsumeBool());
        if (!isDemuxerInit && status == Status::OK) {
            hlsMediaDownloader->SetDemuxerState(streamID);
            hlsMediaDownloader->NotifyInitSuccess();
            isDemuxerInit = true;
        }

        if (i % 1000 == 0) {    // 间隔 1000 ms，打印一次
            IncreaseCoverageRead(hlsMediaDownloader);

            int64_t duration = hlsMediaDownloader->GetDuration();
            int64_t durationCached = hlsMediaDownloader->GetCachedDuration();
            cout << " Run read in size " << size << ", duration " << duration
                << ", durationCached " << durationCached << endl;
        }
        if (hlsMediaDownloader->IsHlsEnd()) {
            break;
        }
    }

    int64_t seekTime = 1000; // 1000 ts
    SeekMode mode = static_cast<SeekMode>(
        fdp->ConsumeIntegralInRange<uint32_t>(0, uint32_t(SeekMode::SEEK_CLOSEST_INNER) + 1));
    hlsMediaDownloader->SeekToTime(seekTime, mode);
    hlsMediaDownloader->SetInterruptState(fdp->ConsumeBool());
    hlsMediaDownloader->Close(true);
    hlsMediaDownloader->SetCallback(nullptr);
    delete sourceCb;
    sourceCb = nullptr;
    hlsMediaDownloader = nullptr;

    return true;
}

bool StartFuzzTestMultiUrl(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdProvider(data, size);
    FuzzedDataProvider *fdp = &fdProvider;
    Plugins::Callback* sourceCb = nullptr;
    std::shared_ptr<HlsMediaDownloader> hlsMediaDownloader = CreateFuzzTestObj(false, fdp, sourceCb);
    if (hlsMediaDownloader == nullptr) {
        cout << "  Memory 3 apply failed." << endl;
        return false;
    }

    int32_t urlStart = fdp->ConsumeIntegralInRange<uint32_t>(0, sizeof(g_urls) / sizeof(g_urls[0]) - 1);
    for (int32_t index = 0; index < sizeof(g_urls) / sizeof(g_urls[0]); index++) {
        urlStart = (urlStart + index) % (sizeof(g_urls) / sizeof(g_urls[0]));
        const std::string url = g_urlIPPort + g_urls[urlStart];
        cout << " Run multi in size " << size << ", urlStart " << urlStart << ", open url: " << url << endl;
        hlsMediaDownloader->Open(url, g_httpHeader);

        int32_t streamID = 0;
        bool isDemuxerInit = false;
        for (int i = 0; i < READ_TIMES_NS; i++) {
            this_thread::sleep_for(chrono::milliseconds(1));
            Status status = ReadData(hlsMediaDownloader, streamID, BUFF_READ_SIZE, fdp->ConsumeBool());
            if (!isDemuxerInit && status == Status::OK) {
                hlsMediaDownloader->SetDemuxerState(streamID);
                hlsMediaDownloader->NotifyInitSuccess();
                isDemuxerInit = true;
            }

            if (i % 1000 == 0) {    // 间隔 1000 ms，打印一次
                IncreaseCoverageRead(hlsMediaDownloader);

                int64_t duration = hlsMediaDownloader->GetDuration();
                int64_t durationCached = hlsMediaDownloader->GetCachedDuration();
                cout << "  Times " << i << ", duration " << duration << ", durationCached " << durationCached << endl;
            }
            if (hlsMediaDownloader->IsHlsEnd()) {
                break;
            }
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
    if (data == nullptr || size <= sizeof(int64_t)) {
        return -1;
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