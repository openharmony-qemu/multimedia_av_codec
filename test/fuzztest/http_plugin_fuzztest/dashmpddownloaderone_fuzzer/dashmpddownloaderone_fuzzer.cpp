/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "dash_mpd_downloader.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "http_server_demo.h"
#include "http_server_mock.h"
#include "test_template.h"

#define FUZZ_PROJECT_NAME "dashmpddownloader_fuzzer"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins::HttpPlugin;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

namespace {
static const std::string MPD_SEGMENT_BASE = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
static const std::string MPD_SEGMENT_TEMPLATE = "http://127.0.0.1:46666/test_dash/segment_template/index.mpd";
static const std::string MPD_SEGMENT_LIST = "http://127.0.0.1:46666/test_dash/segment_list/index.mpd";
static const std::string MPD_AUDIO_SUBTITLE = "http://127.0.0.1:46666/test_dash/segment_base/index_audio_subtitle.mpd";
static const std::string MPD_PERIOD = "http://127.0.0.1:46666/test_dash/segment_base/index_period.mpd";
static const std::string MPD_TEMPLATE_TIMELINE = "http://127.0.0.1:46666/test_dash/segment_template/index_timeline.mpd";

constexpr int32_t WAIT_FOR_SIDX_TIME = 1000 * 1000;

// 模拟DashMpdCallback
class TestDashMpdCallback : public DashMpdCallback {
public:
    void OnMpdInfoUpdate(DashMpdEvent mpdEvent) override
    {
    }

    void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos) override
    {
    }
};

bool TestParseStartNumberFunctions(const std::string &url)
{
    std::shared_ptr<DashMpdDownloader> mpdDownloader = std::make_shared<DashMpdDownloader>(nullptr);
    mpdDownloader->Init();

    std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };

    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mpdDownloader->SetStatusCallback(statusCallback);

    std::shared_ptr<TestDashMpdCallback> mpdCallback = std::make_shared<TestDashMpdCallback>();
    mpdDownloader->SetMpdCallback(mpdCallback);
    mpdDownloader->Open(url);

    usleep(WAIT_FOR_SIDX_TIME);

    mpdDownloader->GetDuration();
    mpdDownloader->GetSeekable();
    mpdDownloader->GetBitRates();

    mpdDownloader->GetBitRatesByHdr(GetData<bool>());
    mpdDownloader->GetUrl();
    mpdDownloader->GetContentType();

    std::vector<StreamInfo> streams;
    Status status = mpdDownloader->GetStreamInfo(streams);
    if (status == Status::OK && !streams.empty()) {
        for (const auto& stream : streams) {
            mpdDownloader->GetStreamByStreamId(stream.streamId);
            mpdDownloader->GetInitSegmentByStreamId(stream.streamId);
            mpdDownloader->SetCurrentNumberSeqByStreamId(stream.streamId, 1);
        }
    }

    mpdDownloader->GetInUseVideoStreamId();
    mpdDownloader->GetUsingStreamByType(MediaAVCodec::MediaType::MEDIA_TYPE_AUD);

    mpdDownloader->GetUsingStreamByType(MediaAVCodec::MediaType::MEDIA_TYPE_VID);

    std::shared_ptr<DashSegment> segment;
    mpdDownloader->GetNextSegmentByStreamId(0, segment);

    std::shared_ptr<DashSegment> seekSegment;
    int64_t seekTime = GetData<int64_t>();
    mpdDownloader->SeekToTs(0, seekTime, seekSegment);

    std::shared_ptr<DashSegment> breakPointSegment;
    mpdDownloader->GetBreakPointSegment(0, seekTime, breakPointSegment);

    DashMpdBitrateParam bitrateParam;
    int nextVideoStreamId;
    mpdDownloader->GetNextVideoStream(bitrateParam, nextVideoStreamId);

    DashMpdTrackParam trackParam;
    trackParam.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    mpdDownloader->GetNextTrackStream(trackParam);

    mpdDownloader->UpdateDownloadFinished(url);

    mpdDownloader->SetHdrStart(GetData<bool>());

    unsigned int testWidth = GetData<uint32_t>() % 1920;
    unsigned int testHeight = GetData<uint32_t>() % 1080;
    mpdDownloader->SetInitResolution(testWidth, testHeight);

    mpdDownloader->SetDefaultLang("en", MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
    mpdDownloader->SetDefaultLang("zh", MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE);

    mpdDownloader->SetInterruptState(GetData<bool>());

    std::shared_ptr<DownloadMetricsInfo> downloadCallback = std::make_shared<DownloadMetricsInfo>();
    mpdDownloader->SetDownloadCallback(downloadCallback);

    mpdDownloader->Close(false);
    mpdDownloader = nullptr;

    return true;
}

bool TestSegmentTemplateFunctions(const std::string &url)
{
    std::shared_ptr<DashMpdDownloader> mpdDownloader = std::make_shared<DashMpdDownloader>(nullptr);
    mpdDownloader->Init();

    std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };

    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mpdDownloader->SetStatusCallback(statusCallback);

    std::shared_ptr<TestDashMpdCallback> mpdCallback = std::make_shared<TestDashMpdCallback>();
    mpdDownloader->SetMpdCallback(mpdCallback);

    mpdDownloader->Open(url);
    usleep(WAIT_FOR_SIDX_TIME);

    std::vector<StreamInfo> streams;
    mpdDownloader->GetStreamInfo(streams);

    if (!streams.empty()) {
        for (const auto& stream : streams) {
            std::shared_ptr<DashStreamDescription> streamDesc = mpdDownloader->GetStreamByStreamId(stream.streamId);
            if (streamDesc != nullptr) {
                uint32_t nextSegTime = GetData<uint32_t>();
                mpdDownloader->UpdateCurrentNumberSeqByTime(streamDesc, nextSegTime);
            }
        }
    }
    mpdDownloader->Close(false);
    mpdDownloader = nullptr;

    return true;
}

bool TestMultiplePeriodsFunctions(const std::string &url)
{
    std::shared_ptr<DashMpdDownloader> mpdDownloader = std::make_shared<DashMpdDownloader>(nullptr);
    mpdDownloader->Init();

    std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };

    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mpdDownloader->SetStatusCallback(statusCallback);

    std::shared_ptr<TestDashMpdCallback> mpdCallback = std::make_shared<TestDashMpdCallback>();
    mpdDownloader->SetMpdCallback(mpdCallback);

    mpdDownloader->Open(url);
    usleep(WAIT_FOR_SIDX_TIME);

    mpdDownloader->GetDuration();
    std::vector<StreamInfo> streams;
    mpdDownloader->GetStreamInfo(streams);

    // 测试不同streamId的segment获取
    for (const auto& stream : streams) {
        std::shared_ptr<DashSegment> segment;
        mpdDownloader->GetNextSegmentByStreamId(stream.streamId, segment);
    }
    mpdDownloader->Close(false);
    mpdDownloader = nullptr;

    return true;
}

bool TestAudioSubtitleFunctions(const std::string &url)
{
    std::shared_ptr<DashMpdDownloader> mpdDownloader = std::make_shared<DashMpdDownloader>(nullptr);
    mpdDownloader->Init();

    std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };

    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mpdDownloader->SetStatusCallback(statusCallback);

    std::shared_ptr<TestDashMpdCallback> mpdCallback = std::make_shared<TestDashMpdCallback>();
    mpdDownloader->SetMpdCallback(mpdCallback);

    mpdDownloader->Open(url);
    usleep(WAIT_FOR_SIDX_TIME);

    mpdDownloader->GetUsingStreamByType(MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
    mpdDownloader->GetUsingStreamByType(MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE);
    DashMpdTrackParam audioTrackParam;
    audioTrackParam.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    mpdDownloader->GetNextTrackStream(audioTrackParam);

    DashMpdTrackParam subtitleTrackParam;
    subtitleTrackParam.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE;
    mpdDownloader->GetNextTrackStream(subtitleTrackParam);
    mpdDownloader->Close(false);
    mpdDownloader = nullptr;

    return true;
}

bool TestTimelineFunctions(const std::string &url)
{
    std::shared_ptr<DashMpdDownloader> mpdDownloader = std::make_shared<DashMpdDownloader>(nullptr);
    mpdDownloader->Init();
    std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };

    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mpdDownloader->SetStatusCallback(statusCallback);

    std::shared_ptr<TestDashMpdCallback> mpdCallback = std::make_shared<TestDashMpdCallback>();
    mpdDownloader->SetMpdCallback(mpdCallback);

    mpdDownloader->Open(url);
    usleep(WAIT_FOR_SIDX_TIME);

    std::vector<StreamInfo> streams;
    mpdDownloader->GetStreamInfo(streams);

    if (!streams.empty()) {
        for (const auto& stream : streams) {
            std::shared_ptr<DashSegment> segment;
            mpdDownloader->GetNextSegmentByStreamId(stream.streamId, segment);
            int64_t seekTime = GetData<int64_t>();
            std::shared_ptr<DashSegment> seekSegment;
            mpdDownloader->SeekToTs(stream.streamId, seekTime, seekSegment);
        }
    }
    mpdDownloader->Close(false);
    mpdDownloader = nullptr;

    return true;
}

bool TestSegmentTemplateCoverage(const std::string &url)
{
    std::shared_ptr<DashMpdDownloader> mpdDownloader = std::make_shared<DashMpdDownloader>(nullptr);
    mpdDownloader->Init();

    std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };

    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mpdDownloader->SetStatusCallback(statusCallback);

    std::shared_ptr<TestDashMpdCallback> mpdCallback = std::make_shared<TestDashMpdCallback>();
    mpdDownloader->SetMpdCallback(mpdCallback);

    mpdDownloader->Open(url);
    usleep(WAIT_FOR_SIDX_TIME);

    std::vector<StreamInfo> streams;
    mpdDownloader->GetStreamInfo(streams);

    if (!streams.empty()) {
        for (const auto& stream : streams) {
            for (int i = 0; i < 5; i++) {
                std::shared_ptr<DashSegment> segment;
                mpdDownloader->GetNextSegmentByStreamId(stream.streamId, segment);
            }
        }
    }
    mpdDownloader->Close(false);
    mpdDownloader = nullptr;

    return true;
}

bool TestEmptyPeriodAndAdaptSet(const std::string &url)
{
    std::shared_ptr<DashMpdDownloader> mpdDownloader = std::make_shared<DashMpdDownloader>(nullptr);
    mpdDownloader->Init();

    std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };

    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mpdDownloader->SetStatusCallback(statusCallback);

    std::shared_ptr<TestDashMpdCallback> mpdCallback = std::make_shared<TestDashMpdCallback>();
    mpdDownloader->SetMpdCallback(mpdCallback);

    mpdDownloader->Open(url);
    usleep(WAIT_FOR_SIDX_TIME);
    std::vector<StreamInfo> streams;
    mpdDownloader->GetStreamInfo(streams);

    for (const auto& stream : streams) {
        std::shared_ptr<DashStreamDescription> streamDesc = mpdDownloader->GetStreamByStreamId(stream.streamId);
    }

    mpdDownloader->Close(false);
    mpdDownloader = nullptr;

    return true;
}

bool TestTimelineCoverage(const std::string &url)
{
    std::shared_ptr<DashMpdDownloader> mpdDownloader = std::make_shared<DashMpdDownloader>(nullptr);
    mpdDownloader->Init();

    std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };

    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mpdDownloader->SetStatusCallback(statusCallback);

    std::shared_ptr<TestDashMpdCallback> mpdCallback = std::make_shared<TestDashMpdCallback>();
    mpdDownloader->SetMpdCallback(mpdCallback);

    mpdDownloader->Open(url);
    usleep(WAIT_FOR_SIDX_TIME);

    std::vector<StreamInfo> streams;
    mpdDownloader->GetStreamInfo(streams);

    if (!streams.empty()) {
        for (const auto& stream : streams) {
            int64_t seekTime = GetData<int64_t>() % 10000000; // 10秒以内
            std::shared_ptr<DashSegment> seekSegment;
            mpdDownloader->SeekToTs(stream.streamId, seekTime, seekSegment);
            for (int i = 0; i < 3; i++) {
                std::shared_ptr<DashSegment> segment;
                mpdDownloader->GetNextSegmentByStreamId(stream.streamId, segment);
            }
        }
    }
    mpdDownloader->Close(false);
    mpdDownloader = nullptr;

    return true;
}

bool TestBaseUrlCoverage(const std::string &url)
{
    std::shared_ptr<DashMpdDownloader> mpdDownloader = std::make_shared<DashMpdDownloader>(nullptr);
    mpdDownloader->Init();

    std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };

    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mpdDownloader->SetStatusCallback(statusCallback);

    std::shared_ptr<TestDashMpdCallback> mpdCallback = std::make_shared<TestDashMpdCallback>();
    mpdDownloader->SetMpdCallback(mpdCallback);

    mpdDownloader->Open(url);
    usleep(WAIT_FOR_SIDX_TIME);

    std::vector<StreamInfo> streams;
    mpdDownloader->GetStreamInfo(streams);

    if (!streams.empty()) {
        for (const auto& stream : streams) {
            std::shared_ptr<DashSegment> segment;
            mpdDownloader->GetNextSegmentByStreamId(stream.streamId, segment);
        }
    }
    mpdDownloader->Close(false);
    mpdDownloader = nullptr;

    return true;
}

bool DashMpdDownloaderFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t)) {
        return false;
    }

    int32_t testType = *reinterpret_cast<const int32_t *>(data);
    testType = abs(testType) % 10; // 0-9 用于选择不同的测试

    bool result = false;
    switch (testType) {
        case 0:
            result = TestParseStartNumberFunctions(MPD_SEGMENT_BASE);
            break;
        case 1:
            result = TestSegmentTemplateFunctions(MPD_SEGMENT_TEMPLATE);
            break;
        case 2:
            result = TestParseStartNumberFunctions(MPD_SEGMENT_LIST);
            break;
        case 3:
            result = TestMultiplePeriodsFunctions(MPD_PERIOD);
            break;
        case 4:
            result = TestAudioSubtitleFunctions(MPD_AUDIO_SUBTITLE);
            break;
        case 5:
            result = TestTimelineFunctions(MPD_TEMPLATE_TIMELINE);
            break;
        case 6:
            result = TestSegmentTemplateCoverage(MPD_SEGMENT_TEMPLATE);
            break;
        case 7:
            result = TestEmptyPeriodAndAdaptSet(MPD_SEGMENT_BASE);
            break;
        case 8:
            result = TestTimelineCoverage(MPD_TEMPLATE_TIMELINE);
            break;
        case 9:
            result = TestBaseUrlCoverage(MPD_SEGMENT_BASE);
            break;
        default:
            result = TestParseStartNumberFunctions(MPD_SEGMENT_TEMPLATE);
            break;
    }

    return result;
}

}
}
}
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (!InitServer()) {
        return -1;
    }
    g_baseFuzzData = data;
    g_baseFuzzSize = size;
    g_baseFuzzPos = 0;
    OHOS::Media::Plugins::HttpPlugin::DashMpdDownloaderFuzzTest(data, size);
    if (!CloseServer()) {
        return -1;
    }
    return 0;
}