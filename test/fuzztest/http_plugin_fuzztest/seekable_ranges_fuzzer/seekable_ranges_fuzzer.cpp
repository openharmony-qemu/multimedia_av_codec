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
#include "hls/hls_media_downloader.h"
#include "http_server_mock.h"

#define FUZZ_PROJECT_NAME "seekable_ranges_fuzzer"

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

const map<string, string> g_httpHeader = {
    {"User-Agent", "ABC"},
    {"Referer", "DEF"},
};

constexpr int BUFF_READ_SIZE = 5 * 1024 * 1024;
constexpr int32_t DEFAULT_TIME = 100;

bool StartFuzzTestSeekableRanges(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    
    bool userDefinedDuration = fdp.ConsumeBool();
    const int expectBufDuration = fdp.ConsumeIntegralInRange<uint32_t>(0, 20);
    std::shared_ptr<HlsMediaDownloader> hlsMediaDownloader = std::make_shared<HlsMediaDownloader>(expectBufDuration,
        userDefinedDuration, g_httpHeader, nullptr);
    if (hlsMediaDownloader == nullptr) {
        return false;
    }

    hlsMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    hlsMediaDownloader->SetStatusCallback(statusCallback);

    auto sourceCb = std::make_shared<SourceCallback>();
    hlsMediaDownloader->SetCallback(sourceCb);

    std::string url = "http://127.0.0.1:46666/test_cbr/";
    url += FAKE_FUZZ_M3U8;
    hlsMediaDownloader->Open(url, g_httpHeader);

    int64_t mediaDuration = fdp.ConsumeIntegral<int64_t>();
    hlsMediaDownloader->SetMediaDuration(mediaDuration);

    this_thread::sleep_for(chrono::milliseconds(DEFAULT_TIME));

    std::vector<SeekRange> seekableRanges = hlsMediaDownloader->GetSeekableRanges();
    for (const auto& range : seekableRanges) {
        std::cout << "SeekRange: start=" << range.start << ", end=" << range.end << std::endl;
    }

    Seekable seekable = hlsMediaDownloader->GetSeekable();
    int64_t duration = hlsMediaDownloader->GetDuration();

    int64_t seekTime = fdp.ConsumeIntegral<int64_t>();
    SeekMode mode = static_cast<SeekMode>(fdp.ConsumeIntegralInRange<uint32_t>(0, 4));
    if (seekable == Seekable::SEEKABLE && seekTime <= duration) {
        hlsMediaDownloader->SeekToTime(seekTime, mode);
    }

    std::vector<SeekRange> rangesAfterSeek = hlsMediaDownloader->GetSeekableRanges();

    hlsMediaDownloader->Close(true);
    hlsMediaDownloader = nullptr;

    return true;
}

}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size <= sizeof(int64_t)) {
        return 0;
    }

    if (!InitServer(data, size)) {
        return 0;
    }

    OHOS::Media::Plugins::HttpPlugin::StartFuzzTestSeekableRanges(data, size);

    CloseServer();
    return 0;
}