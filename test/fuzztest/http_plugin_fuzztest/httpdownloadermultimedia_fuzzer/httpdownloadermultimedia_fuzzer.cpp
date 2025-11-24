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

#include <cstdint>
#include <iostream>
#include <test_template.h>
#include <fuzzer/FuzzedDataProvider.h>
#include "http/http_media_downloader.h"
#include "http_server_mock.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

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

class SourceLoader : public IMediaSourceLoader {
public:
    ~SourceLoader() {};

    int32_t Init(std::shared_ptr<IMediaSourceLoadingRequest> &request)
    {
        return 1;
    };
    
    int64_t Open(const std::string &url, const std::map<std::string, std::string> &header)
    {
        return 1;
    };

    int32_t Read(int64_t uuid, int64_t requestedOffset, int64_t requestedLength)
    {
        return 1;
    };

    int32_t Close(int64_t uuid)
    {
        return 1;
    };
};
}
}
}
}

using namespace OHOS;
using namespace OHOS::Media::Plugins::HttpPlugin;
using namespace OHOS::Media::Plugins;
#define FUZZ_PROJECT_NAME "httpdownloader_fuzzer"
const std::string MP4_SEGMENT_BASE = "http://127.0.0.1:46666/dewu.mp4";
const std::string MP4_NULL_SEGMENT_BASE = "http://127.0.0.1:46666/dewuNull.mp4";
const std::string FLV_SEGMENT_BASE = "http://127.0.0.1:46666/h264.flv";

static constexpr int32_t MAX_BUFFER_SIZE_FUZZ = 1024 * 1024 * 2;
static uint8_t g_buffer[MAX_BUFFER_SIZE_FUZZ];
static const std::map<std::string, std::string> g_httpHeader = {
    {"User-Agent", "ABC"},
    {"Referer", "DEF"},
};

void TestHttpDownloaderFuzz(FuzzedDataProvider &fdp)
{
    static const std::string testUriPath = "http://127.0.0.1:46666/";
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = fdp.ConsumeIntegral<int32_t>();
    readDataInfo.wantReadLength_ = fdp.ConsumeIntegral<uint32_t>();
    readDataInfo.wantReadLength_ %= MAX_BUFFER_SIZE_FUZZ;
    readDataInfo.isEos_ = fdp.ConsumeIntegral<bool>();
    std::map<std::string, std::string> header = std::map<std::string, std::string>();
    std::string fuzzString = fdp.ConsumeRandomLengthString();
    std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<OHOS::Media::Plugins::HttpPlugin::HttpMediaDownloader>(testUriPath, 5, nullptr);  // 5
    httpMediaDownloader->Init();
    httpMediaDownloader->readOffset_ = 0;
    httpMediaDownloader->minReadOffset_ = 1;
    httpMediaDownloader->UpdateMinAndMaxReadOffset();
    std::string testUrl = testUriPath + fuzzString;
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 640;  // 640
    playStrategy->height = 480;  // 480
    MediaStreamList mediaStreams;
    auto stream1 = std::make_shared<PlayMediaStream>();
    stream1->url = "http://127.0.0.1:46666/dewu.mp4";
    stream1->width = 1280;  // 1280
    stream1->height = 720;  // 720
    stream1->bitrate = 30000;  // 30000
    mediaStreams.push_back(stream1);
    auto stream2 = std::make_shared<PlayMediaStream>();
    stream2->url = "http://127.0.0.1:46666/dewu.mp4";
    stream2->width = 1920;  // 1920
    stream2->height = 1080;  // 1080
    stream2->bitrate = 40000;  // 40000
    mediaStreams.push_back(stream2);
    httpMediaDownloader.SetMediaStreams(mediaStreams);
    httpMediaDownloader->Open(testUriPath, g_httpHeader);
    httpMediaDownloader->SetPlayStrategy(playStrategy);
    httpMediaDownloader->Read(g_buffer, readDataInfo);
    uint64_t extraCacheDuration = fdp.ConsumeIntegral<uint64_t>();
    httpMediaDownloader->SetExtraCache(extraCacheDuration);
    int32_t initOffset = fdp.ConsumeIntegral<int32_t>();
    int32_t initSize = fdp.ConsumeIntegral<int32_t>();
    httpMediaDownloader->SetInitialBufferSize(initOffset, initSize);
    int64_t seekOffset = fdp.ConsumeIntegral<int64_t>();
    bool isSeekHit = fdp.ConsumeIntegral<bool>();
    httpMediaDownloader->SeekToPos(seekOffset, isSeekHit);
    uint32_t bitRate = fdp.ConsumeIntegral<uint32_t>();
    httpMediaDownloader->SelectBitRate(bitRate);
}

void CallHttpDownloaderFuncs(std::shared_ptr<HttpMediaDownloader> httpMediaDownloader, FuzzedDataProvider &fdp)
{
    if (httpMediaDownloader == nullptr) {
        return;
    }
    httpMediaDownloader->isFirstFrameArrived_ = true;
    httpMediaDownloader->UpdateWaterLineAbove();
    httpMediaDownloader->GetStartedStatus();
    httpMediaDownloader->GetBuffer();
    httpMediaDownloader->GetReadFrame();
    httpMediaDownloader->SetIsReportedErrorCode();
    httpMediaDownloader->SetStartPts(0);
    std::string outString;
    std::string sizeString = "offset";
    std::string numString = "0";
    httpMediaDownloader->AddParamForUrl(outString, sizeString, numString);
    httpMediaDownloader->GetResolutionDelta(fdp.ConsumeIntegral<uint32_t>(), fdp.ConsumeIntegral<uint32_t>());
    httpMediaDownloader->IsFlvLive();
    httpMediaDownloader->GetBufferSize();
    httpMediaDownloader->GetStatusCallbackFunc();
    httpMediaDownloader->GetDownloadRateAndSpeed();
    httpMediaDownloader->GetPlayable();
    httpMediaDownloader->GetReadTimeOut(true);
    httpMediaDownloader->SetAppUid(fdp.ConsumeIntegral<int32_t>());
    httpMediaDownloader->GetCacheDuration(0);
    httpMediaDownloader->GetCacheDuration(1);
    httpMediaDownloader->SetIsTriggerAutoMode(fdp.ConsumeIntegral<bool>());
    httpMediaDownloader->CheckAutoSelectBitrate();
    httpMediaDownloader->IsAutoSelectConditionOk();
    httpMediaDownloader->OnClientErrorEvent();
    httpMediaDownloader->GetMemorySize();
    httpMediaDownloader->GetCurUrl();
    httpMediaDownloader->SetInterruptState(fdp.ConsumeIntegral<bool>());
    httpMediaDownloader->StopBufferring(fdp.ConsumeIntegral<bool>());
    httpMediaDownloader->RestartAndClearBuffer();
    httpMediaDownloader->ClearBuffer();
    httpMediaDownloader->Close(true);
    httpMediaDownloader->isCacheBufferInited_ = false;
    httpMediaDownloader->WaitCacheBufferInit();
    httpMediaDownloader = nullptr;
}

void HttpDownloaderRun(FuzzedDataProvider &fdp)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 4, nullptr);  // 4
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    Callback* sourceCallback = new SourceCallback();
    httpMediaDownloader->SetCallback(sourceCallback);
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, g_httpHeader);
    httpMediaDownloader->GetSeekable();
    ReadDataInfo readDataInfo;
    for (int i = 0; i < 800; i++) {  // 800
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 10ms
        httpMediaDownloader->DownloadReport();
        readDataInfo.streamId_ = 0;
        readDataInfo.wantReadLength_ = 100 * 1024;  // 100 * 1024
        readDataInfo.isEos_ = false;
        httpMediaDownloader->Read(g_buffer, readDataInfo);
        if (i == 3) {  // 3
            httpMediaDownloader->SetDemuxerState(0);
        }
    }
    httpMediaDownloader->isLargeOffsetSpan_ = true;
    httpMediaDownloader->isMinAndMaxOffsetUpdate_ = true;
    httpMediaDownloader->isNeedClearHasRead_ = true;
    httpMediaDownloader->UpdateMinAndMaxReadOffset();
    httpMediaDownloader->HandleSeekHit(fdp.ConsumeIntegral<int64_t>());
    httpMediaDownloader->SetCurrentBitRate(fdp.ConsumeIntegral<uint32_t>(), fdp.ConsumeIntegral<uint32_t>());
    httpMediaDownloader->recordSpeedCount_ = 0;
    auto dlInfo = httpMediaDownloader->GetDownloadInfo();
    httpMediaDownloader->recordSpeedCount_ = 1;
    dlInfo = httpMediaDownloader->GetDownloadInfo();
    DownloadInfo downloadInfo;
    httpMediaDownloader->GetDownloadInfo(downloadInfo);
    PlaybackInfo playbackInfo;
    httpMediaDownloader->GetPlaybackInfo(playbackInfo);
    CallHttpDownloaderFuncs(httpMediaDownloader, fdp);
}

void HttpDownloaderFlvRun(FuzzedDataProvider &fdp)
{
    const std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 4, nullptr);  // 4
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    Callback* sourceCallback = new SourceCallback();
    httpMediaDownloader->SetCallback(sourceCallback);
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    ReadDataInfo readDataInfo;
    for (int i = 0; i < 800; i++) {  // 800
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 10ms
        httpMediaDownloader->DownloadReport();
        readDataInfo.streamId_ = 0;
        readDataInfo.wantReadLength_ = 10240;  // 10240
        readDataInfo.isEos_ = false;
        httpMediaDownloader->Read(g_buffer, readDataInfo);
        if (i == 3) {  // 3
            httpMediaDownloader->SetDemuxerState(0);
        }
    }
    httpMediaDownloader->ChangeDownloadPos(true);
    httpMediaDownloader->ChangeDownloadPos(false);
    DownloadInfo downloadInfo;
    httpMediaDownloader->GetDownloadInfo(downloadInfo);
    httpMediaDownloader->OnClientErrorEvent();
    httpMediaDownloader->IsFlvLive();
    httpMediaDownloader->SetInterruptState(fdp.ConsumeIntegral<bool>());
    httpMediaDownloader->SeekRingBuffer(fdp.ConsumeIntegralInRange<int64_t>(0, 10240));  // 10240
    httpMediaDownloader->SetReadBlockingFlag(false);
    httpMediaDownloader->AutoSelectBitRate(10);  // 10
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
}

void DownloaderFuzz(uint8_t *data, size_t size)
{
    g_baseFuzzData = data;
    g_baseFuzzSize = size;
    g_baseFuzzPos = 0;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = g_httpHeader;
    auto realStatusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<Downloader> downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    std::shared_ptr<DownloadRequest> request =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->Retry(request);
    downloader->isAppBackground_ = GetData<bool>();
    downloader->isDestructor_ = GetData<bool>();
    downloader->Retry(request);
    downloader->GetContentType();
    downloader->ReStart();
    downloader->Cancel();

    std::shared_ptr<SourceLoader> loader = std::make_shared<SourceLoader>();
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader =
        std::make_shared<MediaSourceLoaderCombinations>(loader);
    downloader = std::make_shared<Downloader>("test", sourceLoader);
    downloader->Init();
    downloader->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->appPreviousRequestUrl_ = MP4_SEGMENT_BASE;
    downloader->OpenAppUri();
    downloader->currentRequest_->startPos_ = 0;
    downloader->DropRetryData(g_buffer, 1024, downloader.get());  // 1024
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    TestHttpDownloaderFuzz(fdp);

    if (!InitServer()) {
        std::cout << "Init server error" << std::endl;
        return -1;
    }
    HttpDownloaderRun(fdp);
    HttpDownloaderFlvRun(fdp);
    DownloaderFuzz(data, size);
    if (!CloseServer()) {
        std::cout << "Close server error" << std::endl;
        return -1;
    }
    return 0;
}