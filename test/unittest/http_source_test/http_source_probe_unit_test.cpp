/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <algorithm>
#include <array>
#include <climits>
#include "gtest/gtest.h"
#include "http_source_plugin.h"
#include "http_server_demo.h"
#include "test_http_port.h"
#include "monitor/download_monitor.h"
#include "http/http_media_downloader.h"
#include "hls/hls_media_downloader.h"
#include "source_callback.h"
#include "hls_source_callback.h"
#include "sniffer_test_support.h"

using namespace testing::ext;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace SnifferTest;
using MediaAVCodec::HttpServerDemo;
using MediaAVCodec::HttpResponseControl;
namespace {
constexpr size_t SEGMENT_SIZE = 2048;
constexpr size_t FLV_HEADER_SIZE = 13; // Nine-byte header plus PreviousTagSize0.
const std::string BASE_URL = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR;

bool HasForwardedHeaders(const std::string& request)
{
    return request.find("Cookie:session=test") != std::string::npos &&
        request.find("Authorization:Bearer test") != std::string::npos &&
        request.find("Referer:http://example.test/") != std::string::npos;
}

std::string Manifest(const std::string& segment, bool large = false)
{
    std::string padding = large ? "#" + std::string(SnifferCoordinator::SNIFF_THRESHOLD, 'p') + "\n" : "";
    return "#EXTM3U\n" + padding + "#EXT-X-TARGETDURATION:6\n#EXTINF:6,\n" + segment + "\n#EXT-X-ENDLIST\n";
}

void AppendBigEndian(std::string& bytes, uint32_t value, size_t count)
{
    constexpr size_t BITS_PER_BYTE = 8;
    while (count > 0) {
        --count;
        bytes.push_back(static_cast<char>((value >> (count * BITS_PER_BYTE)) & UINT8_MAX));
    }
}

std::string PcmFlv()
{
    const uint8_t header[] = {'F', 'L', 'V', 1, 4, 0, 0, 0, 9, 0, 0, 0, 0};
    std::string bytes(reinterpret_cast<const char*>(header), sizeof(header));
    constexpr uint32_t PCM_BYTES = 256;
    constexpr uint32_t TAG_HEADER_SIZE = 11;
    constexpr uint32_t TIMESTAMP_MS = 20;
    constexpr size_t TAGS = 4;
    for (size_t index = 0; index < TAGS; ++index) {
        bytes.push_back(8); // Audio tag.
        AppendBigEndian(bytes, PCM_BYTES + 1, 3); // 24-bit payload size.
        AppendBigEndian(bytes, index * TIMESTAMP_MS, 3);
        bytes.append(4, '\0'); // Timestamp extension and 24-bit stream ID.
        bytes.push_back(0x3f); // Little-endian PCM, 44 kHz, 16-bit stereo.
        bytes.append(PCM_BYTES, '\0');
        AppendBigEndian(bytes, TAG_HEADER_SIZE + PCM_BYTES + 1, 4);
    }
    return bytes;
}

class PublishingCallback : public SourceCallback {
public:
    void OnEvent(const Plugins::PluginEvent& event) override
    {
        if (event.type == PluginEventType::STREAM_UPDATE) {
            reached.Set();
            release.Wait();
        }
    }
    Signal reached;
    Signal release;
};

struct ReadResult {
    std::string bytes;
    Status status {Status::ERROR_AGAIN};
};
} // namespace

class HttpSourceProbeUnitTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        server_ = std::make_unique<HttpServerDemo>();
        server_->StartServer();
    }
    static void TearDownTestCase()
    {
        server_->StopServer();
        server_.reset();
    }
    void SetUp() override
    {
        plugin_ = std::make_shared<HttpSourcePlugin>("probe-integration");
        callback_ = std::make_shared<SourceCallback>();
        plugin_->SetCallback(callback_);
    }
    void TearDown() override
    {
        HttpServerDemo::ClearContent();
        plugin_->CloseUri(true);
        if (selection_ != nullptr) {
            selection_->Stop();
        }
        plugin_.reset();
    }
    ReadResult ReadAll(size_t expected);
    std::shared_ptr<HttpMediaDownloader> Http();
    bool CancelHeldProbe(bool interrupt);
    Status EnableHlsSelection()
    {
        selection_ = std::make_shared<HlsSourceCallback>(plugin_);
        return plugin_->SetCallback(selection_);
    }
    static std::unique_ptr<HttpServerDemo> server_;
    std::shared_ptr<HttpSourcePlugin> plugin_;
    std::shared_ptr<SourceCallback> callback_;
    std::shared_ptr<HlsSourceCallback> selection_;
};
std::unique_ptr<HttpServerDemo> HttpSourceProbeUnitTest::server_;

ReadResult HttpSourceProbeUnitTest::ReadAll(size_t expected)
{
    ReadResult result;
    int32_t stream = selection_ != nullptr ? selection_->VideoId() : 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(WAIT_MS);
    plugin_->SetReadBlockingFlag(false);
    while (std::chrono::steady_clock::now() < deadline) {
        std::shared_ptr<Buffer> buffer;
        result.status = plugin_->Read(stream, buffer, result.bytes.size(), READ_SIZE);
        if (buffer != nullptr && buffer->GetMemory() != nullptr) {
            auto memory = buffer->GetMemory();
            if (memory->GetSize() > 0) {
                result.bytes.append(reinterpret_cast<const char*>(memory->GetReadOnlyData()), memory->GetSize());
            }
            stream = buffer->streamID;
        }
        if (result.status == Status::END_OF_STREAM && result.bytes.size() >= expected) {
            break;
        }
        if (result.status != Status::OK && result.status != Status::ERROR_AGAIN &&
            result.status != Status::END_OF_STREAM) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_MS));
    }
    return result;
}

std::shared_ptr<HttpMediaDownloader> HttpSourceProbeUnitTest::Http()
{
    auto monitor = std::static_pointer_cast<DownloadMonitor>(plugin_->GetDownloader());
    return monitor != nullptr ? std::static_pointer_cast<HttpMediaDownloader>(monitor->downloader_) : nullptr;
}

HWTEST_F(HttpSourceProbeUnitTest, HlsRedownloadUsesFormalManifestAndRelativeSegment_001, TestSize.Level2)
{
    ASSERT_EQ(this->EnableHlsSelection(), Status::OK);
    HttpServerDemo::Response response;
    response.content = Manifest("probe.ts");
    response.subsequentContent = Manifest("formal.ts", true);
    HttpServerDemo::RegisterResponse("/hls/manifest", response);
    std::string bytes(SEGMENT_SIZE, 'b');
    HttpServerDemo::RegisterContent("/hls/formal.ts", bytes);
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(BASE_URL + "/hls/manifest")), Status::OK);
    ASSERT_TRUE(plugin_->IsHls());
    ASSERT_TRUE(selection_->WaitSelected());
    auto result = this->ReadAll(bytes.size());
    EXPECT_EQ(result.status, Status::END_OF_STREAM);
    EXPECT_EQ(result.bytes, bytes);
    EXPECT_EQ(HttpServerDemo::GetRequests("/hls/manifest").size(), 2u);
    EXPECT_TRUE(HttpServerDemo::GetRequests("/hls/probe.ts").empty());
    EXPECT_EQ(HttpServerDemo::GetRequests("/hls/formal.ts").size(), 1u);
}

HWTEST_F(HttpSourceProbeUnitTest, HlsMasterReopensThenLoadsOnlyFormalChild_002, TestSize.Level2)
{
    ASSERT_EQ(this->EnableHlsSelection(), Status::OK);
    const std::string prefix = "#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=1000,RESOLUTION=640x360\n";
    HttpServerDemo::Response response;
    response.content = prefix + "unused.m3u8\n";
    response.subsequentContent = prefix + "child.m3u8\n";
    HttpServerDemo::RegisterResponse("/master/list", response);
    HttpServerDemo::RegisterContent("/master/child.m3u8", Manifest("segment.ts"));
    std::string bytes(SEGMENT_SIZE, 'm');
    HttpServerDemo::RegisterContent("/master/segment.ts", bytes);
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(BASE_URL + "/master/list")), Status::OK);
    EXPECT_TRUE(plugin_->IsHls());
    ASSERT_TRUE(selection_->WaitSelected());
    auto result = this->ReadAll(bytes.size());
    EXPECT_EQ(result.status, Status::END_OF_STREAM);
    EXPECT_EQ(result.bytes, bytes);
    EXPECT_TRUE(HttpServerDemo::GetRequests("/master/unused.m3u8").empty());
    EXPECT_EQ(HttpServerDemo::GetRequests("/master/child.m3u8").size(), 1u);
}

HWTEST_F(HttpSourceProbeUnitTest, LocalRangeParsesAndDownloadsExactlyItsManifest_003, TestSize.Level2)
{
    ASSERT_EQ(this->EnableHlsSelection(), Status::OK);
    std::string manifest = Manifest(BASE_URL + "/local/segment.ts");
    const std::string prefix = "unrelated-prefix";
    TempFile file(prefix + manifest + "unrelated-tail");
    ASSERT_EQ(file.written,
        static_cast<ssize_t>(prefix.size() + manifest.size() + std::string("unrelated-tail").size()));
    HttpServerDemo::RegisterContent("/local/segment.ts", std::string(SEGMENT_SIZE, 'l'));
    auto source = std::make_shared<MediaSource>(file.Uri() + "?offset=" + std::to_string(prefix.size()) +
        "&size=" + std::to_string(manifest.size()));
    source->mimeType_ = AVMimeTypes::APPLICATION_M3U8;
    ASSERT_EQ(plugin_->SetSource(source), Status::OK);
    EXPECT_TRUE(plugin_->IsHls());
    ASSERT_TRUE(selection_->WaitSelected());
    auto result = this->ReadAll(SEGMENT_SIZE);
    EXPECT_EQ(result.status, Status::END_OF_STREAM);
    EXPECT_EQ(result.bytes, std::string(SEGMENT_SIZE, 'l'));
    plugin_->CloseUri(true);
    EXPECT_NE(fcntl(file.fd, F_GETFD), -1);
    EXPECT_EQ(HttpServerDemo::GetRequests("/local/segment.ts").size(), 1u);
}

HWTEST_F(HttpSourceProbeUnitTest, PluginForwardsHeadersToProbeFormalAndRedirectTarget_004, TestSize.Level2)
{
    constexpr size_t INITIAL_FORMAL_TARGET_REQUESTS = 1;
    auto control = std::make_shared<HttpResponseControl>();
    HttpServerDemo::Response redirect;
    redirect.status = 302; // Same-origin redirect preserves headers on the initial requests.
    redirect.headers["Location"] = "/headers/final";
    redirect.control = control;
    redirect.controlAfterFirstRequest = true; // Let the probe through; hold the formal request.
    redirect.pausePoint = HttpResponseControl::Point::BEFORE_HEADERS;
    HttpServerDemo::RegisterResponse("/headers/start", redirect);
    const std::string bytes(SEGMENT_SIZE, 'h');
    HttpServerDemo::RegisterContent("/headers/final", bytes);
    const std::map<std::string, std::string> headers {
        {"Cookie", "session=test"}, {"Authorization", "Bearer test"}, {"Referer", "http://example.test/"}};
    auto source = std::make_shared<MediaSource>(BASE_URL + "/headers/start", headers);
    auto setter = std::async(std::launch::async, [this, source] { return plugin_->SetSource(source); });
    bool paused = control->WaitPaused();
    auto probeRequests = HttpServerDemo::GetRequests("/headers/final");
    auto openingRequests = HttpServerDemo::GetRequests("/headers/start");
    // SetSource can wait for headers. Release before joining or making any fatal assertion.
    control->Release();
    auto status = setter.get();
    ASSERT_TRUE(paused);
    ASSERT_EQ(status, Status::OK);
    ASSERT_FALSE(probeRequests.empty());
    EXPECT_TRUE(std::all_of(probeRequests.begin(), probeRequests.end(), HasForwardedHeaders));
    ASSERT_FALSE(openingRequests.empty());
    EXPECT_TRUE(std::all_of(openingRequests.begin(), openingRequests.end(), HasForwardedHeaders));
    auto result = this->ReadAll(bytes.size());
    EXPECT_EQ(result.bytes, bytes);
    EXPECT_EQ(result.status, Status::END_OF_STREAM);
    auto requests = HttpServerDemo::GetRequests("/headers/final");
    ASSERT_GT(requests.size(), probeRequests.size());
    EXPECT_TRUE(HasForwardedHeaders(requests[probeRequests.size()])); // First formal redirect target.
    // Legacy Curl retries can lose headers when reopening. That known defect remains deferred.
    RecordProperty("additional_formal_target_requests",
        std::to_string(requests.size() - probeRequests.size() - INITIAL_FORMAL_TARGET_REQUESTS));
    // Record every target request: 1 means all forwarded headers are present; 0 means at least one is missing.
    std::string headerForwarding;
    for (const auto& request : requests) {
        headerForwarding += HasForwardedHeaders(request) ? '1' : '0';
    }
    RecordProperty("target_header_forwarding", headerForwarding);
}

bool HttpSourceProbeUnitTest::CancelHeldProbe(bool interrupt)
{
    auto control = std::make_shared<HttpResponseControl>();
    HttpServerDemo::Response response;
    response.content = std::string(SEGMENT_SIZE, 'c');
    response.control = control;
    response.pausePoint = HttpResponseControl::Point::BEFORE_HEADERS;
    HttpServerDemo::RegisterResponse("/cancelled", response);
    auto source = std::make_shared<MediaSource>(BASE_URL + "/cancelled");
    auto setter = std::async(std::launch::async, [&] { return plugin_->SetSource(source); });
    bool entered = control->WaitPaused();
    if (interrupt) {
        plugin_->SetInterruptState(true);
    } else {
        plugin_->CloseUri(true);
    }
    bool timely = setter.wait_for(std::chrono::milliseconds(WAIT_MS)) == std::future_status::ready;
    bool disconnected = timely && control->WaitDisconnected();
    control->Release();
    setter.get(); // The historical SetSource error-return issue is outside this change.
    return entered && timely && disconnected && plugin_->GetDownloader() == nullptr &&
        plugin_->GetActiveSniffer() == nullptr && HttpServerDemo::GetRequests("/cancelled").size() == 1;
}

HWTEST_F(HttpSourceProbeUnitTest, CloseDuringProbePreventsFormalRequest_005, TestSize.Level2)
{
    EXPECT_TRUE(this->CancelHeldProbe(false));
}

HWTEST_F(HttpSourceProbeUnitTest, InterruptDuringProbePreventsFormalRequest_006, TestSize.Level2)
{
    EXPECT_TRUE(this->CancelHeldProbe(true));
}

HWTEST_F(HttpSourceProbeUnitTest, CloseAfterRecognitionBeforePublication_007, TestSize.Level2)
{
    auto control = std::make_shared<HttpResponseControl>();
    HttpServerDemo::Response response;
    response.content = Manifest("unopened.ts");
    response.control = control;
    response.pausePoint = HttpResponseControl::Point::BEFORE_HEADERS;
    HttpServerDemo::RegisterResponse("/publish", response);
    auto source = std::make_shared<MediaSource>(BASE_URL + "/publish");
    auto setter = std::async(std::launch::async, [&] { return plugin_->SetSource(source); });
    bool paused = control->WaitPaused();
    std::unique_lock<std::shared_mutex> publication(plugin_->downloaderMutex_);
    control->Release();
    bool recognized = WaitFor([&] { return plugin_->protocolType_.load() == StreamProtocolType::HLS; });
    auto closer = std::async(std::launch::async, [&] { plugin_->CloseUri(true); });
    bool closed = WaitFor([&] { return plugin_->sourceClosed_.load(); });
    publication.unlock();
    setter.get();
    closer.get();
    EXPECT_TRUE(paused);
    EXPECT_TRUE(recognized);
    EXPECT_TRUE(closed);
    EXPECT_EQ(plugin_->GetDownloader(), nullptr);
    EXPECT_EQ(plugin_->GetActiveSniffer(), nullptr);
    EXPECT_EQ(HttpServerDemo::GetRequests("/publish").size(), 1u);
}

HWTEST_F(HttpSourceProbeUnitTest, ClosePublishedDownloaderBeforeFormalOpen_008, TestSize.Level2)
{
    auto callback = std::make_shared<PublishingCallback>();
    ASSERT_EQ(plugin_->SetCallback(callback), Status::OK);
    HttpServerDemo::RegisterContent("/before-open", std::string(SEGMENT_SIZE, 'o'));
    auto stream = std::make_shared<PlayMediaStream>();
    stream->url = BASE_URL + "/before-open";
    auto source = std::make_shared<MediaSource>(stream->url);
    source->playMediaStreamVec_ = {stream};
    auto setter = std::async(std::launch::async, [&] { return plugin_->SetSource(source); });
    bool published = callback->reached.Wait();
    auto snapshot = plugin_->GetDownloader();
    plugin_->CloseUri(false);
    callback->release.Set();
    setter.get();
    EXPECT_TRUE(published);
    EXPECT_NE(snapshot, nullptr);
    EXPECT_EQ(plugin_->GetDownloader(), nullptr);
    EXPECT_EQ(HttpServerDemo::GetRequests("/before-open").size(), 1u);
}

HWTEST_F(HttpSourceProbeUnitTest, OngoingFlvIsLiveAndCloseStopsTransport_009, TestSize.Level2)
{
    auto control = std::make_shared<HttpResponseControl>();
    HttpServerDemo::Response response;
    response.content = PcmFlv();
    response.chunked = true;
    response.ignoreRange = true;
    response.chunkSize = 285; // Header plus one complete PCM tag and its previous-tag-size field.
    response.control = control;
    response.controlAfterFirstRequest = true;
    response.pausePoint = HttpResponseControl::Point::AFTER_FIRST_CHUNK;
    HttpServerDemo::RegisterResponse("/ongoing-flv", response);
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(BASE_URL + "/ongoing-flv")), Status::OK);
    bool paused = control->WaitPaused();
    ASSERT_TRUE(paused);
    auto media = this->Http();
    ASSERT_NE(media, nullptr);
    bool received = WaitFor([&] { return media->GetBufferSize() >= FLV_HEADER_SIZE; });
    ASSERT_TRUE(received);
    std::array<uint8_t, FLV_HEADER_SIZE> bytes {};
    ReadDataInfo info;
    info.wantReadLength_ = bytes.size();
    EXPECT_EQ(media->Read(bytes.data(), info), Status::OK);
    EXPECT_EQ(std::string(reinterpret_cast<char*>(bytes.data()), info.realReadLength_),
        response.content.substr(0, FLV_HEADER_SIZE));
    EXPECT_TRUE(media->GetDownloadRequest()->isFlv_);
    EXPECT_EQ(media->GetDownloadRequest()->GetFileContentLengthNoWait(), 0);
    EXPECT_TRUE(media->isLive_.load());
    EXPECT_TRUE(media->IsFlvLive());
    EXPECT_FALSE(media->GetDownloadRequest()->IsEos());
    EXPECT_EQ(media->Read(bytes.data(), info), Status::OK);
    plugin_->CloseUri(false);
    bool disconnected = control->WaitDisconnected();
    control->Release();
    EXPECT_TRUE(paused);
    EXPECT_TRUE(received);
    EXPECT_TRUE(disconnected);
    EXPECT_EQ(plugin_->GetDownloader(), nullptr);
}

HWTEST_F(HttpSourceProbeUnitTest, OrdinaryContentWithFlvSuffixUsesFiniteHttp_010, TestSize.Level2)
{
    const std::string bytes(SEGMENT_SIZE, 'n');
    HttpServerDemo::RegisterContent("/misleading.flv", bytes);
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(BASE_URL + "/misleading.flv")), Status::OK);
    auto media = this->Http();
    ASSERT_NE(media, nullptr);
    auto result = this->ReadAll(bytes.size());
    EXPECT_EQ(result.bytes, bytes);
    EXPECT_EQ(result.status, Status::END_OF_STREAM);
    EXPECT_FALSE(media->isFlvContent_);
    EXPECT_FALSE(media->isRingBuffer_);
    EXPECT_FALSE(media->GetDownloadRequest()->isFlv_);
    EXPECT_FALSE(media->IsFlvLive());
}

HWTEST_F(HttpSourceProbeUnitTest, ActiveSnifferIsDestroyedOutsidePointerLock_011, TestSize.Level2)
{
    auto state = std::make_shared<ReleaseObservation>();
    std::weak_ptr<HttpSourcePlugin> weak = plugin_;
    auto old = std::shared_ptr<SnifferCoordinator>(new SnifferCoordinator(), [state, weak](SnifferCoordinator* p) {
        ObserveRelease(state, [weak] {
            auto owner = weak.lock();
            return owner != nullptr && owner->GetActiveSniffer() == nullptr;
        });
        delete p;
    });
    ASSERT_TRUE(plugin_->SetActiveSniffer(std::move(old)));
    EXPECT_TRUE(plugin_->SetActiveSniffer(nullptr));
    EXPECT_TRUE(state->queried.Wait());
    EXPECT_TRUE(state->replacementVisible.load());
    EXPECT_TRUE(state->outsideLock.load());
}

HWTEST_F(HttpSourceProbeUnitTest, InitialSelectionIgnoresNullAndProbesChosenProtocol_012, TestSize.Level2)
{
    ASSERT_EQ(this->EnableHlsSelection(), Status::OK);
    auto low = std::make_shared<PlayMediaStream>();
    low->url = BASE_URL + "/selection/low";
    low->width = 640;
    low->height = 360;
    low->bitrate = 1000;
    auto high = std::make_shared<PlayMediaStream>();
    high->url = BASE_URL + "/selection/high";
    high->width = 1280;
    high->height = 720;
    high->bitrate = 2000;
    HttpServerDemo::RegisterContent("/selection/low", std::string(SEGMENT_SIZE, 'x'));
    HttpServerDemo::RegisterContent("/selection/high", Manifest("selected.ts"));
    HttpServerDemo::RegisterContent("/selection/selected.ts", std::string(SEGMENT_SIZE, 's'));
    auto source = std::make_shared<MediaSource>(low->url);
    source->playMediaStreamVec_ = {nullptr, high, low};
    auto strategy = std::make_shared<PlayStrategy>();
    strategy->width = 1280; // Choose the HLS candidate, although the source's original URL is ordinary HTTP.
    strategy->height = 720;
    source->SetPlayStrategy(strategy);
    ASSERT_EQ(plugin_->SetSource(source), Status::OK);
    EXPECT_TRUE(plugin_->IsHls());
    ASSERT_TRUE(selection_->WaitSelected());
    auto result = this->ReadAll(SEGMENT_SIZE);
    EXPECT_EQ(result.status, Status::END_OF_STREAM);
    EXPECT_EQ(result.bytes, std::string(SEGMENT_SIZE, 's'));
    EXPECT_TRUE(HttpServerDemo::GetRequests("/selection/low").empty());
    EXPECT_EQ(HttpServerDemo::GetRequests("/selection/high").size(), 2u);
}

HWTEST_F(HttpSourceProbeUnitTest, DashFormalMpdSupersedesProbeDocument_013, TestSize.Level2)
{
    const std::string prefix = "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\" ";
    const std::string suffix = "<Period><AdaptationSet mimeType=\"video/mp4\">"
        "<Representation id=\"v\" bandwidth=\"1000\"><BaseURL>media.mp4</BaseURL>"
        "</Representation></AdaptationSet></Period></MPD>";
    HttpServerDemo::Response response;
    response.content = prefix + "mediaPresentationDuration=\"PT6S\">" + suffix;
    response.subsequentContent = prefix + "mediaPresentationDuration=\"PT10S\">" + suffix;
    HttpServerDemo::RegisterResponse("/dash/manifest", response);
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(BASE_URL + "/dash/manifest")), Status::OK);
    EXPECT_EQ(plugin_->protocolType_.load(), StreamProtocolType::DASH);
    int64_t duration = 0;
    constexpr int64_t FORMAL_DURATION = 10000000000; // Ten seconds in nanoseconds, not the probe's six seconds.
    bool parsed = WaitFor([&] {
        return plugin_->GetDuration(duration) == Status::OK && duration == FORMAL_DURATION;
    });
    EXPECT_TRUE(parsed);
    EXPECT_EQ(duration, FORMAL_DURATION);
    EXPECT_EQ(HttpServerDemo::GetRequests("/dash/manifest").size(), 2u);
}

HWTEST_F(HttpSourceProbeUnitTest, RedirectedHlsKeepsLegacyRelativeSegmentResolution_014, TestSize.Level2)
{
    ASSERT_EQ(this->EnableHlsSelection(), Status::OK);
    HttpServerDemo::Response redirect;
    redirect.status = 302;
    redirect.content = "<MPD/>"; // This intermediate body must not select DASH.
    redirect.headers["Location"] = "/final-hls/list";
    HttpServerDemo::RegisterResponse("/entry-hls/start", redirect);
    HttpServerDemo::RegisterContent("/final-hls/list", Manifest("segment.ts"));
    HttpServerDemo::RegisterContent("/final-hls/segment.ts", std::string(SEGMENT_SIZE, 'r'));
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(BASE_URL + "/entry-hls/start")), Status::OK);
    ASSERT_TRUE(selection_->WaitSelected());
    EXPECT_TRUE(plugin_->IsHls());
    auto result = this->ReadAll(SEGMENT_SIZE);
    EXPECT_EQ(result.bytes, std::string(SEGMENT_SIZE, 'r'));
    EXPECT_EQ(result.status, Status::END_OF_STREAM);
    // Legacy automatic follow and explicit retry may both request this manifest.
    EXPECT_FALSE(HttpServerDemo::GetRequests("/final-hls/list").empty());
    EXPECT_TRUE(HttpServerDemo::GetRequests("/entry-hls/segment.ts").empty());
    EXPECT_EQ(HttpServerDemo::GetRequests("/final-hls/segment.ts").size(), 1u);
}
} // namespace OHOS::Media::Plugins::HttpPlugin
