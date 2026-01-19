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
        (void) url;
        (void) header;
        return 1;
    };

    int32_t Read(int64_t uuid, int64_t requestedOffset, int64_t requestedLength)
    {
        (void) uuid;
        (void) requestedOffset;
        (void) requestedLength;
        return 1;
    };

    int32_t Close(int64_t uuid)
    {
        (void) uuid;
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
constexpr int32_t WAIT_FOR_SIDX_TIME = 1 * 10;
static uint8_t g_buffer[MAX_BUFFER_SIZE_FUZZ];
static const std::map<std::string, std::string> g_httpHeader = {
    {"User-Agent", "ABC"},
    {"Referer", "DEF"},
};

std::shared_ptr<HttpMediaDownloader> InitializeAndDownload()
{
    const std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 4, nullptr); // 4
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    Callback* sourceCallback = new SourceCallback();
    httpMediaDownloader->SetCallback(sourceCallback);
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    ReadDataInfo readDataInfo;
    for (int i = 0; i < 800; i++) { // 800
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10
        httpMediaDownloader->DownloadReport();
        readDataInfo.streamId_ = 0;
        readDataInfo.wantReadLength_ = 10240; // 10240
        readDataInfo.isEos_ = false;
        httpMediaDownloader->Read(g_buffer, readDataInfo);
        if (i == 3) { // 3
            httpMediaDownloader->SetDemuxerState(0);
        }
    }
    return httpMediaDownloader;
}

void PostDownloadSetup(std::shared_ptr<HttpMediaDownloader> httpMediaDownloader)
{
    httpMediaDownloader->GetCurUrl();
    httpMediaDownloader->GetMemorySize();
    httpMediaDownloader->ClearBuffer();
    bool isAuto = GetData<bool>();
    httpMediaDownloader->SetIsTriggerAutoMode(isAuto);
    httpMediaDownloader->GetContentType();
    uint32_t bitRate = GetData<uint32_t>();
    httpMediaDownloader->AutoSelectBitRate(bitRate);
    httpMediaDownloader->SelectBitRate(bitRate);
    int64_t startPts = GetData<int64_t>();
    httpMediaDownloader->SetStartPts(startPts);
    httpMediaDownloader->IsFlvLive();
    httpMediaDownloader->RestartAndClearBuffer();
    httpMediaDownloader->GetCachedDuration();
    httpMediaDownloader->NotifyInitSuccess();
    int32_t offset = GetData<int32_t>();
    int32_t cursize = GetData<int32_t>();
    httpMediaDownloader->SetInitialBufferSize(offset, cursize);
    httpMediaDownloader->IsNotRetry(httpMediaDownloader->downloadRequest_);
    httpMediaDownloader->SetIsReportedErrorCode();
    httpMediaDownloader->WaitForBufferingEnd();
    bool isAppBackground = GetData<bool>();
    httpMediaDownloader->StopBufferring(isAppBackground);
    int32_t appUid = GetData<int32_t>();
    httpMediaDownloader->SetAppUid(appUid);
    bool isDelay = GetData<bool>();
    httpMediaDownloader->GetReadTimeOut(isDelay);
    httpMediaDownloader->GetBufferingTimeOut();
    httpMediaDownloader->GetPlayable();
    httpMediaDownloader->GetBufferSize();
    int32_t curbitRate = GetData<int32_t>();
    int32_t streamID = GetData<int32_t>();
    httpMediaDownloader->SetCurrentBitRate(curbitRate, streamID);
    httpMediaDownloader->DownloadReport();
    uint32_t len = GetData<uint32_t>();
    httpMediaDownloader->OnWriteBuffer(len);
    httpMediaDownloader->GetDownloadRateAndSpeed();
    httpMediaDownloader->GetStatusCallbackFunc();
    httpMediaDownloader->GetDownloadErrorState();
    httpMediaDownloader->GetReadFrame();
    httpMediaDownloader->GetBuffer();
    httpMediaDownloader->GetDownloadInfo();
}

void PostDownloadCleanup(std::shared_ptr<HttpMediaDownloader> httpMediaDownloader)
{
    bool isInterruptNeeded = GetData<bool>();
    httpMediaDownloader->SetInterruptState(isInterruptNeeded);
    httpMediaDownloader->SetDownloadErrorState();
    int32_t streamId = GetData<int32_t>();
    httpMediaDownloader->SetDemuxerState(streamId);
    bool isReadBlockingAllowed = GetData<bool>();
    httpMediaDownloader->SetReadBlockingFlag(isReadBlockingAllowed);
    httpMediaDownloader->GetStartedStatus();
    httpMediaDownloader->GetSeekable();
    httpMediaDownloader->GetDuration();
    httpMediaDownloader->GetContentLength();
    bool isSeekHit = GetData<bool>();
    int64_t offset1 = GetData<int64_t>();
    httpMediaDownloader->SeekToPos(offset1, isSeekHit);
    httpMediaDownloader->Pause();
    httpMediaDownloader->Resume();
    usleep(WAIT_FOR_SIDX_TIME);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (!InitServer()) {
        std::cout << "Init server error" << std::endl;
        return -1;
    }
    g_baseFuzzData = data;
    g_baseFuzzSize = size;
    g_baseFuzzPos = 0;
    if (data != nullptr) {
        std::shared_ptr<HttpMediaDownloader> httpMediaDownloader = InitializeAndDownload();
        PostDownloadSetup(httpMediaDownloader);
        PostDownloadCleanup(httpMediaDownloader);
    }
    if (!CloseServer()) {
        std::cout << "Close server error" << std::endl;
        return -1;
    }
    return 0;
}