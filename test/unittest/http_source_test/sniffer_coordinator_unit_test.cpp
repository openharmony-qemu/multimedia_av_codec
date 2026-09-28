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

#include <chrono>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <future>
#include <thread>
#include <unistd.h>
#include "sniffer/sniffer_coordinator.h"
#include "http_server_demo.h"
#include "http/http_media_downloader.h"
#include "gtest/gtest.h"

using namespace OHOS::Media::Plugins::HttpPlugin;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
namespace OHOS::Media::Plugins::HttpPlugin {
namespace {
constexpr int32_t PROBE_TEST_PORT = 46667;
// The server gate holds for ten seconds; cancellation must finish before its three-second watchdog.
constexpr int32_t HELD_TIMEOUT_MS = 200;
constexpr int32_t SHORT_TIMEOUT_MS = 100;
constexpr int32_t REQUEST_TIMEOUT_MS = 1000;
constexpr int32_t CURL_WRITE_ERROR = 23;
constexpr size_t TEST_BODY_SIZE = 512;
constexpr size_t READ_BUFFER_SIZE = 1024;
constexpr size_t LONG_XML_COMMENT_SIZE = 2048;
// An odd chunk size fragments signatures and XML delimiters across network writes.
constexpr size_t FRAGMENT_SIZE = 17;
constexpr size_t APP_TRACE_CAPACITY = 128; // Bounded trace for this one short probe/formal exchange.
constexpr size_t APP_TRACE_PREFIX_BYTES = 16;
constexpr int HEX_DIGITS_PER_BYTE = 2;
const std::string BASE = "http://127.0.0.1:" + std::to_string(PROBE_TEST_PORT);
const std::string SHORT_HLS = "#EXTM3U\n#EXT-X-TARGETDURATION:6\n#EXTINF:6,\nseg.ts\n#EXT-X-ENDLIST\n";
ProtocolProbeResult Probe(const std::string& text, bool eos = false)
{
    return StreamProtocolSniffer::Probe(reinterpret_cast<const uint8_t*>(text.data()), text.size(), eos);
}
class ProbeMemory : public OHOS::Media::AVSharedMemory {
public:
    explicit ProbeMemory(const std::string& data) : data_(data) {}
    uint8_t* GetBase() const override { return reinterpret_cast<uint8_t*>(const_cast<char*>(data_.data())); }
    int32_t GetSize() const override { return static_cast<int32_t>(data_.size()); }
    uint32_t GetFlags() const override { return FLAGS_READ_WRITE; }
private:
    std::string data_;
};

class AppLoaderTrace {
public:
    void Record(const std::string& action, const std::string& details)
    {
        auto index = next_.fetch_add(1, std::memory_order_relaxed);
        if (index >= events_.size()) {
            return;
        }
        auto& event = events_[index];
        event.microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start_).count();
        event.tid = gettid();
        event.action = action;
        event.details = details;
        event.ready.store(true, std::memory_order_release);
    }

    void Dump() const
    {
        std::ostringstream output;
        auto total = next_.load(std::memory_order_relaxed);
        output << "[AppLoaderTrace] pid=" << getpid() << " events=" << total << '\n';
        for (size_t index = 0; index < std::min(total, events_.size()); ++index) {
            const auto& event = events_[index];
            output << "[AppLoaderTrace] seq=" << index;
            if (!event.ready.load(std::memory_order_acquire)) {
                output << " pending=1\n";
                continue;
            }
            output << " t_us=" << event.microseconds << " tid=" << event.tid
                   << " event=" << event.action << ' ' << event.details << '\n';
        }
        if (total > events_.size()) {
            output << "[AppLoaderTrace] dropped=" << total - events_.size() << '\n';
        }
        std::cerr << output.str() << std::flush;
        testing::Test::RecordProperty("app_loader_diagnostics", output.str());
    }

private:
    struct Event {
        int64_t microseconds {0};
        pid_t tid {0};
        std::string action;
        std::string details;
        std::atomic<bool> ready {false};
    };
    const std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();
    std::array<Event, APP_TRACE_CAPACITY> events_;
    std::atomic<size_t> next_ {0};
};

struct ScopedAppLoaderTrace {
    ~ScopedAppLoaderTrace()
    {
        trace->Dump(); // Test-thread output also survives an early ASSERT return.
    }
    std::shared_ptr<AppLoaderTrace> trace = std::make_shared<AppLoaderTrace>();
};

std::string AppTracePath(const std::string& url)
{
    const std::string separator = "://";
    auto scheme = url.find(separator);
    auto start = url.find('/', scheme == std::string::npos ? 0 : scheme + separator.size());
    auto path = start == std::string::npos ? "/" : url.substr(start);
    return path.substr(0, path.find_first_of("?#"));
}

class ProbeLoader : public IMediaSourceLoader {
public:
    ~ProbeLoader() override
    {
        if (worker_.joinable()) {
            worker_.join();
        }
    }
    int32_t Init(std::shared_ptr<IMediaSourceLoadingRequest>& request) override
    {
        request_ = request;
        if (trace != nullptr) {
            trace->Record("Init", "request_present=" + std::to_string(request_ != nullptr));
        }
        return 0;
    }
    int64_t Open(const std::string& url, const std::map<std::string, std::string>&) override
    {
        Trace("Open.begin");
        urls.push_back(url);
        auto uuid = ++opens;
        Trace("Open.end", uuid, 0, 0, uuid);
        if (trace != nullptr) {
            trace->Record("Open.resource", "path=" + AppTracePath(url));
        }
        return uuid;
    }
    int32_t Read(int64_t uuid, int64_t offset, int64_t length) override
    {
        Trace("Read.begin", uuid, offset, length);
        if (worker_.joinable()) {
            worker_.join();
        }
        offsets.push_back(offset);
        worker_ = std::thread([this, uuid, offset] {
            if (redirect && uuid == 1) {
                Trace("RespondHeader.begin", uuid);
                auto headerResult = request_->RespondHeader(uuid, {}, "/target");
                Trace("RespondHeader.end", uuid, 0, 0, headerResult);
                Finish(uuid);
                return;
            }
            Trace("RespondHeader.begin", uuid, 0, TEST_BODY_SIZE);
            auto headerResult = request_->RespondHeader(uuid, {{"content-length", std::to_string(TEST_BODY_SIZE)}}, "");
            Trace("RespondHeader.end", uuid, 0, TEST_BODY_SIZE, headerResult);
            auto memory = std::make_shared<ProbeMemory>(std::string(TEST_BODY_SIZE - offset, uuid == 1 ? 'x' : 'y'));
            Trace("RespondData.begin", uuid, offset, memory->GetSize());
            auto dataResult = request_->RespondData(uuid, offset, memory);
            Trace("RespondData.end", uuid, offset, memory->GetSize(), dataResult);
            if (dataResult >= 0) {
                Finish(uuid);
            }
        });
        Trace("Read.end", uuid, offset, length, 0);
        return 0;
    }
    int32_t Close(int64_t uuid) override
    {
        Trace("Close.begin", uuid);
        if (worker_.joinable()) {
            worker_.join();
        }
        closed.push_back(uuid);
        Trace("Close.end", uuid, 0, 0, 0);
        return 0;
    }
    std::shared_ptr<AppLoaderTrace> trace;
    int64_t opens {0};
    bool redirect {false};
    std::vector<std::string> urls;
    std::vector<int64_t> closed;
    std::vector<int64_t> offsets;
private:
    void Trace(const char* action, int64_t uuid = 0, int64_t offset = 0, int64_t length = 0,
        std::optional<int64_t> result = std::nullopt) const
    {
        if (trace == nullptr) {
            return;
        }
        std::ostringstream details;
        details << "uuid=" << uuid << " offset=" << offset << " length=" << length << " result=";
        if (result.has_value()) {
            details << *result;
        } else {
            details << "not_returned";
        }
        trace->Record(action, details.str()); // Only in-memory recording on download callback threads.
    }

    void Finish(int64_t uuid)
    {
        if (trace != nullptr) {
            trace->Record("FinishLoading.begin", "uuid=" + std::to_string(uuid) + " state=" +
                std::to_string(static_cast<int32_t>(LoadingRequestError::LOADING_ERROR_SUCCESS)));
        }
        auto result = request_->FinishLoading(uuid, LoadingRequestError::LOADING_ERROR_SUCCESS);
        Trace("FinishLoading.end", uuid, 0, 0, result);
    }

    std::shared_ptr<IMediaSourceLoadingRequest> request_;
    std::thread worker_;
};

const char* AppReadStatusName(Status status)
{
    switch (status) {
        case Status::OK:
            return "OK";
        case Status::END_OF_STREAM:
            return "END_OF_STREAM";
        case Status::ERROR_AGAIN:
            return "ERROR_AGAIN";
        case Status::ERROR_UNKNOWN:
            return "ERROR_UNKNOWN";
        case Status::ERROR_NULL_POINTER:
            return "ERROR_NULL_POINTER";
        case Status::ERROR_INVALID_OPERATION:
            return "ERROR_INVALID_OPERATION";
        default:
            return "OTHER";
    }
}

using AppReadBuffer = uint8_t[READ_BUFFER_SIZE];

std::string AppReadFields(const ReadDataInfo& info, const AppReadBuffer& data, std::optional<Status> status)
{
    std::ostringstream details;
    if (status.has_value()) {
        details << "status=" << AppReadStatusName(*status) << " dec=" << static_cast<int32_t>(*status)
                << " hex=0x" << std::hex << std::setw(static_cast<int>(sizeof(uint32_t)) * HEX_DIGITS_PER_BYTE)
                << std::setfill('0') << static_cast<uint32_t>(*status) << std::dec;
    } else {
        details << "status=not_called";
    }
    details << " want=" << info.wantReadLength_ << " real=" << info.realReadLength_ << " eos=" << info.isEos_;
    details << " payload_512_y=" << (info.realReadLength_ == TEST_BODY_SIZE &&
        std::all_of(data, data + TEST_BODY_SIZE, [](uint8_t byte) { return byte == 'y'; }));
    details << " buffer_prefix_hex=";
    for (size_t index = 0; index < std::min(APP_TRACE_PREFIX_BYTES, sizeof(data)); ++index) {
        details << std::hex << std::setw(HEX_DIGITS_PER_BYTE) << std::setfill('0')
                << static_cast<uint32_t>(data[index]);
    }
    return details.str();
}

std::string AppMediaFields(const std::shared_ptr<HttpMediaDownloader>& media)
{
    std::ostringstream details;
    auto downloader = media->GetDownloader();
    details << "media_id=" << media->GetLogId() << " downloader_id="
            << (downloader != nullptr ? downloader->GetUniqueId() : 0);
    details << " ring=" << media->isRingBuffer_ << " cache_inited=" << media->isCacheBufferInited_.load()
            << " interrupt=" << media->isInterruptNeeded_.load() << " download_error="
            << media->downloadErrorState_.load() << " read_offset=" << media->readOffset_
            << " write_offset=" << media->writeOffset_.load();
    // This case never replaces the constructor-created cache. This query does not wait for download data.
    details << " cached_bytes=" << media->GetCurrentBufferSize();
    auto request = media->GetDownloadRequest();
    details << " request_present=" << (request != nullptr);
    if (request != nullptr) {
        details << " request_eos=" << request->IsEos() << " request_closed=" << request->IsClosed();
    }
    return details.str();
}

Status ReadAppWithTrace(AppLoaderTrace& trace, const std::shared_ptr<HttpMediaDownloader>& media,
    ReadDataInfo& info, AppReadBuffer& data, const char* phase)
{
    trace.Record(std::string(phase) + ".before", AppReadFields(info, data, std::nullopt) + " " + AppMediaFields(media));
    auto status = media->Read(data, info); // Exactly one real read, with its result captured once.
    trace.Record(std::string(phase) + ".after", AppReadFields(info, data, status) + " " + AppMediaFields(media));
    return status;
}

void TraceAppStatus(AppLoaderTrace& trace, DownloadStatus status, const std::shared_ptr<Downloader>& downloader,
    const std::shared_ptr<DownloadRequest>& request)
{
    std::ostringstream details;
    details << "download_status=" << static_cast<int32_t>(status) << " downloader_id="
            << (downloader != nullptr ? downloader->GetUniqueId() : 0) << " request_present=" << (request != nullptr);
    if (request != nullptr) {
        // This callback runs on the task that has just written these transport error fields.
        details << " client_error=" << request->GetClientError() << " server_error=" << request->GetServerError();
    }
    trace.Record("formal.status_callback", details.str());
}

void TraceAppRequest(AppLoaderTrace& trace, const std::shared_ptr<DownloadRequest>& request)
{
    std::ostringstream details;
    details << "request_present=" << (request != nullptr);
    if (request != nullptr) {
        // Called only after synchronous Close: plain header/error fields are no longer being written.
        details << " content_length=" << request->GetFileContentLengthNoWait()
                << " client_error=" << request->GetClientError() << " server_error=" << request->GetServerError()
                << " eos=" << request->IsEos() << " closed=" << request->IsClosed();
    }
    trace.Record("formal.request.quiesced", details.str());
}

void TraceAppHistory(AppLoaderTrace& trace, const ProbeLoader& loader)
{
    // Both downloaders have closed before the test reads the loader's ordinary vectors.
    std::ostringstream details;
    details << "opens=" << loader.opens << " offsets=";
    for (auto offset : loader.offsets) {
        details << offset << ',';
    }
    details << " closed=";
    for (auto uuid : loader.closed) {
        details << uuid << ',';
    }
    trace.Record("loader.history.quiesced", details.str());
}


std::shared_ptr<SnifferCoordinator> PendingProbe()
{
    auto sniffer = std::make_shared<SnifferCoordinator>();
    sniffer->result_ = std::make_shared<SniffResult>();
    sniffer->state_ = SnifferCoordinator::State::PROBING;
    sniffer->deadline_ = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    return sniffer;
}

class SilentProbeLoader : public ProbeLoader {
public:
    int32_t Read(int64_t, int64_t, int64_t) override { return 0; }
};

}
} // namespace OHOS::Media::Plugins::HttpPlugin

class SnifferCoordinatorUnitTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        server = std::make_unique<HttpServerDemo>();
        server->StartServer(PROBE_TEST_PORT);
    }
    static void TearDownTestCase() { server->StopServer(); server.reset(); }
    void TearDown() override { HttpServerDemo::ClearContent(); }
    bool RunHeldProbe(HttpResponseControl::Point point, bool cancel);
    static std::unique_ptr<HttpServerDemo> server;
};
std::unique_ptr<HttpServerDemo> SnifferCoordinatorUnitTest::server;

bool SnifferCoordinatorUnitTest::RunHeldProbe(HttpResponseControl::Point point, bool cancel)
{
    auto control = std::make_shared<HttpResponseControl>();
    HttpServerDemo::Response response;
    response.control = control;
    response.pausePoint = point;
    response.content = "<!--" + std::string(TEST_BODY_SIZE, 'x');
    response.chunkSize = FRAGMENT_SIZE;
    HttpServerDemo::RegisterResponse("/held-probe", response);
    auto sniffer = std::make_shared<SnifferCoordinator>();
    sniffer->Start(BASE + "/held-probe", {}, false, -1,
        cancel ? SnifferCoordinator::SNIFF_TIMEOUT_MS : HELD_TIMEOUT_MS);
    auto downloader = sniffer->sniffer_;
    bool paused = control->WaitPaused();
    std::shared_ptr<SniffResult> result;
    auto waiter = std::async(std::launch::async, [&] { return sniffer->WaitResult(result); });
    if (cancel || !paused) {
        sniffer->Stop();
    }
    bool timely = waiter.wait_for(std::chrono::milliseconds(HttpResponseControl::WAIT_MS)) ==
        std::future_status::ready;
    bool disconnected = timely && control->WaitDisconnected();
    control->Release(); // Failure cleanup never waits for the natural ten-second gate expiration.
    if (!timely) {
        sniffer->Stop();
    }
    bool ok = waiter.get();
    return paused && timely && disconnected && !ok && result != nullptr && !result->sniffOk &&
        sniffer->state_ == (cancel ? SnifferCoordinator::State::CANCELLED : SnifferCoordinator::State::TIMED_OUT) &&
        downloader != nullptr && downloader->GetClient() == nullptr && downloader->forceClosed_;
}

HWTEST_F(SnifferCoordinatorUnitTest, ShortCompleteHls, TestSize.Level1)
{
    ASSERT_LT(SHORT_HLS.size(), MIN_SNIFF_BUFFER_SIZE);
    for (size_t size : {SHORT_HLS.size(), MIN_SNIFF_BUFFER_SIZE - 1, MIN_SNIFF_BUFFER_SIZE}) {
        HttpServerDemo::RegisterContent("/opaque", SHORT_HLS + std::string(size - SHORT_HLS.size(), '\n'));
        auto sniffer = std::make_shared<SnifferCoordinator>();
        sniffer->Start(BASE + "/opaque", {});
        std::shared_ptr<SniffResult> result;
        ASSERT_TRUE(sniffer->WaitResult(result));
        EXPECT_EQ(result->type, StreamProtocolType::HLS);
        EXPECT_LE(result->bytesProbed, MIN_SNIFF_BUFFER_SIZE);
    }
}

HWTEST_F(SnifferCoordinatorUnitTest, ProbeStopsAtSmallTarget, TestSize.Level1)
{
    // A 256 KiB body exceeds the probe cap, so recognition must stop at the small target.
    HttpServerDemo::RegisterContent("/large", std::string(256 * 1024, 'x'));
    auto sniffer = std::make_shared<SnifferCoordinator>();
    sniffer->Start(BASE + "/large", {});
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(sniffer->WaitResult(result));
    EXPECT_EQ(result->type, StreamProtocolType::HTTP);
    EXPECT_EQ(result->bytesProbed, MIN_SNIFF_BUFFER_SIZE);
    EXPECT_EQ(HttpServerDemo::GetRequests("/large").size(), 1u);
}

HWTEST_F(SnifferCoordinatorUnitTest, RootBasedDashAndFragmentedPrefixes, TestSize.Level1)
{
    for (const auto& text : {std::string("<MPD "), std::string("<?xml version=\"1.0\"?><!--x--><MPD>"),
        std::string("\xEF\xBB\xBF \n<d:MPD xmlns:d=\"x\">"),
        std::string("<!DOCTYPE MPD [<!ENTITY x \"hello\">]><MPD>")}) {
        EXPECT_EQ(Probe(text).type, StreamProtocolType::DASH);
        // Include every partial root-name byte; "MPD" occupies three bytes.
        for (size_t n = 1; n < text.find("MPD") + 3; ++n) {
            EXPECT_TRUE(Probe(text.substr(0, n)).needMore);
        }
    }
    EXPECT_EQ(Probe("<?xml version=\"1.0\"?><other>MPD</other>", true).type, StreamProtocolType::HTTP);
    EXPECT_TRUE(Probe("<?xml version=\"1.0\"?>").needMore);
    EXPECT_TRUE(Probe("#EXTM").needMore);
    EXPECT_TRUE(Probe("<MPD").needMore);
}

HWTEST_F(SnifferCoordinatorUnitTest, LongXmlPreambleAndBoundary, TestSize.Level1)
{
    std::string xml = "<?xml version=\"1.0\"?><!--" + std::string(LONG_XML_COMMENT_SIZE, 'x') + "--><MPD/>";
    HttpServerDemo::Response response;
    response.content = xml;
    response.chunkSize = FRAGMENT_SIZE;
    HttpServerDemo::RegisterResponse("/xml", response);
    auto sniffer = std::make_shared<SnifferCoordinator>();
    sniffer->Start(BASE + "/xml", {});
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(sniffer->WaitResult(result));
    EXPECT_EQ(result->type, StreamProtocolType::DASH);
    EXPECT_LE(result->bytesProbed, SnifferCoordinator::SNIFF_THRESHOLD);
    EXPECT_TRUE(Probe("<!--" + std::string(SnifferCoordinator::SNIFF_THRESHOLD, 'x'), true).needMore);
}

HWTEST_F(SnifferCoordinatorUnitTest, FlvHeaderWithoutSuffix, TestSize.Level1)
{
    // FLV version 1, audio/video flags 0x05, big-endian data offset equal to the 9-byte header.
    const uint8_t data[] = {'F', 'L', 'V', 1, 5, 0, 0, 0, 9};
    for (size_t n = 1; n < sizeof(data); ++n) {
        EXPECT_TRUE(StreamProtocolSniffer::Probe(data, n).needMore);
    }
    EXPECT_TRUE(StreamProtocolSniffer::Probe(data, sizeof(data)).isFlv);
    uint8_t invalid[] = {'F', 'L', 'V', 1, 0x80, 0, 0, 0, 9}; // Reserved flag bit must invalidate FLV.
    EXPECT_FALSE(StreamProtocolSniffer::Probe(invalid, sizeof(invalid), true).isFlv);
    HttpServerDemo::RegisterContent("/stream", std::string(reinterpret_cast<const char*>(data), sizeof(data)));
    auto sniffer = std::make_shared<SnifferCoordinator>();
    sniffer->Start(BASE + "/stream", {});
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(sniffer->WaitResult(result));
    EXPECT_TRUE(result->isFlv);
}

HWTEST_F(SnifferCoordinatorUnitTest, RedirectsAndRequestHeaders, TestSize.Level1)
{
    HttpServerDemo::RegisterContent("/target", SHORT_HLS);
    // Permanent/temporary redirects, including method-preserving HTTP status variants.
    for (int code : {301, 302, 307, 308}) {
        HttpServerDemo::Response response;
        response.status = code;
        response.content = "ignored redirect body";
        response.headers["Location"] = "/target";
        auto path = "/redirect" + std::to_string(code);
        HttpServerDemo::RegisterResponse(path, response);
        auto sniffer = std::make_shared<SnifferCoordinator>();
        sniffer->Start(BASE + path, {{"Cookie", "probe=1"}, {"Authorization", "Bearer probe"}});
        std::shared_ptr<SniffResult> result;
        ASSERT_TRUE(sniffer->WaitResult(result));
        EXPECT_EQ(result->type, StreamProtocolType::HLS);
        auto requests = HttpServerDemo::GetRequests(path);
        ASSERT_EQ(requests.size(), 1u);
        EXPECT_NE(requests[0].find("Cookie:probe=1"), std::string::npos);
        EXPECT_NE(requests[0].find("Authorization:Bearer probe"), std::string::npos);
    }
}

HWTEST_F(SnifferCoordinatorUnitTest, RedirectMessagesNeverSelectTheFinalProtocol_001, TestSize.Level2)
{
    const std::vector<std::string> messages {"#EXTM3U\n", "<MPD/>"};
    std::vector<StreamProtocolType> types;
    for (size_t index = 0; index < messages.size(); ++index) {
        auto path = "/message-" + std::to_string(index);
        auto target = path + "-final";
        HttpServerDemo::Response redirect;
        redirect.status = 302; // An intermediate body intentionally resembles another media protocol.
        redirect.content = messages[index];
        redirect.headers["Location"] = target;
        HttpServerDemo::RegisterResponse(path, redirect);
        HttpServerDemo::RegisterContent(target, std::string(TEST_BODY_SIZE, 'f'));
        auto probe = std::make_shared<SnifferCoordinator>();
        probe->Start(BASE + path, {});
        std::shared_ptr<SniffResult> result;
        ASSERT_TRUE(probe->WaitResult(result));
        types.push_back(result->type);
        EXPECT_EQ(probe->attempt_, 2u); // Initial request plus the existing Downloader's reported 302.
        EXPECT_EQ(probe->redirects_, 1);
    }
    EXPECT_EQ(types, std::vector<StreamProtocolType>(messages.size(), StreamProtocolType::HTTP));
}

HWTEST_F(SnifferCoordinatorUnitTest, ChunkedUsesOrdinaryCompletionNotifications, TestSize.Level1)
{
    for (const auto& content : {SHORT_HLS, std::string("abc")}) {
        HttpServerDemo::Response response;
        response.chunked = true;
        response.content = content;
        response.chunkSize = 3; // Split the HLS signature across multiple HTTP chunks.
        HttpServerDemo::RegisterResponse("/chunked", response);
        auto sniffer = std::make_shared<SnifferCoordinator>();
        sniffer->Start(BASE + "/chunked", {}, false, -1, REQUEST_TIMEOUT_MS);
        auto downloader = sniffer->sniffer_;
        std::shared_ptr<SniffResult> result;
        if (content == SHORT_HLS) {
            ASSERT_TRUE(sniffer->WaitResult(result));
            EXPECT_EQ(result->type, StreamProtocolType::HLS);
        } else {
            // The ordinary chunked path marks EOS without a DownloadDone callback.
            EXPECT_FALSE(sniffer->WaitResult(result));
            EXPECT_EQ(sniffer->state_, SnifferCoordinator::State::TIMED_OUT);
            EXPECT_TRUE(downloader->GetCurrentRequest()->IsEos());
        }
    }
}

HWTEST_F(SnifferCoordinatorUnitTest, HttpErrorBodyUsesOrdinaryDataPath, TestSize.Level1)
{
    HttpServerDemo::Response response;
    response.status = 404; // HTTP Not Found still exercises the ordinary body callback path.
    response.content = SHORT_HLS + std::string(TEST_BODY_SIZE, 'x');
    HttpServerDemo::RegisterResponse("/missing", response);
    auto sniffer = std::make_shared<SnifferCoordinator>();
    sniffer->Start(BASE + "/missing", {});
    std::shared_ptr<SniffResult> result;
    // No probe-only HTTP status filtering: the ordinary data path forwards this body.
    ASSERT_TRUE(sniffer->WaitResult(result));
    EXPECT_EQ(result->type, StreamProtocolType::HLS);
    EXPECT_GT(result->bytesProbed, 0u);
}

HWTEST_F(SnifferCoordinatorUnitTest, TimeoutAndCancellation, TestSize.Level1)
{
    EXPECT_TRUE(this->RunHeldProbe(HttpResponseControl::Point::BEFORE_HEADERS, false));
    EXPECT_TRUE(this->RunHeldProbe(HttpResponseControl::Point::BEFORE_BODY, false));
    EXPECT_TRUE(this->RunHeldProbe(HttpResponseControl::Point::AFTER_FIRST_CHUNK, false));
    auto cancelled = std::make_shared<SnifferCoordinator>();
    cancelled->Stop();
    cancelled->Start(BASE + "/never", {});
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(cancelled->WaitResult(result));
    EXPECT_TRUE(HttpServerDemo::GetRequests("/never").empty());
}

HWTEST_F(SnifferCoordinatorUnitTest, LocalFdOffsetDoesNotMove, TestSize.Level1)
{
    char path[] = "/data/test/media/sniff_fd_XXXXXX";
    int fd = mkstemp(path);
    ASSERT_GE(fd, 0);
    std::string bytes = "prefix!" + SHORT_HLS + "tail";
    EXPECT_EQ(write(fd, bytes.data(), bytes.size()), static_cast<ssize_t>(bytes.size()));
    EXPECT_EQ(lseek(fd, 3, SEEK_SET), 3); // An unrelated cursor position must survive explicit-offset probing.
    auto sniffer = std::make_shared<SnifferCoordinator>();
    // Skip the seven-byte "prefix!" and restrict the FD range to the manifest.
    sniffer->Start("fd://" + std::to_string(fd) + "?offset=7&size=" + std::to_string(SHORT_HLS.size()), {}, true);
    std::shared_ptr<SniffResult> result;
    bool ok = sniffer->WaitResult(result);
    EXPECT_EQ(lseek(fd, 0, SEEK_CUR), 3);
    close(fd);
    unlink(path);
    ASSERT_TRUE(ok);
    EXPECT_EQ(result->type, StreamProtocolType::HLS);
}

HWTEST_F(SnifferCoordinatorUnitTest, CancelActiveRequest, TestSize.Level1)
{
    EXPECT_TRUE(this->RunHeldProbe(HttpResponseControl::Point::BEFORE_HEADERS, true));
    EXPECT_TRUE(this->RunHeldProbe(HttpResponseControl::Point::BEFORE_BODY, true));
    EXPECT_TRUE(this->RunHeldProbe(HttpResponseControl::Point::AFTER_FIRST_CHUNK, true));
}

HWTEST_F(SnifferCoordinatorUnitTest, AppLoaderProbeAndFormalRequestHaveSeparateLifetimes, TestSize.Level1)
{
    ScopedAppLoaderTrace diagnostics;
    auto trace = diagnostics.trace;
    auto loader = std::make_shared<ProbeLoader>();
    loader->trace = trace;
    auto combinations = std::make_shared<MediaSourceLoaderCombinations>(loader);
    auto sniffer = std::make_shared<SnifferCoordinator>(combinations);
    trace->Record("probe.start", "route_id=" + std::to_string(sniffer->GetRouteId()));
    sniffer->Start("http://app/resource", {});
    std::shared_ptr<SniffResult> result;
    auto probed = sniffer->WaitResult(result);
    trace->Record("probe.wait.end", "ok=" + std::to_string(probed) + " type=" +
        std::to_string(result != nullptr ? static_cast<int32_t>(result->type) : -1));
    ASSERT_TRUE(probed);
    ASSERT_EQ(loader->closed.size(), 1u);
    EXPECT_EQ(loader->closed[0], 1);
    auto media = std::make_shared<HttpMediaDownloader>("http://app/resource", 1, combinations, false);
    media->Init();
    media->SetStatusCallback([trace](DownloadStatus status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) { TraceAppStatus(*trace, status, downloader, request); });
    trace->Record("formal.open.begin", "path=/resource");
    auto opened = media->Open("http://app/resource", {});
    trace->Record("formal.open.end", "ok=" + std::to_string(opened));
    ASSERT_TRUE(opened);
    media->SetDemuxerState(0); // Default stream ID; use the demuxer read phase for this short resource.
    uint8_t data[READ_BUFFER_SIZE] {};
    ReadDataInfo info;
    info.wantReadLength_ = sizeof(data);
    EXPECT_EQ(ReadAppWithTrace(*trace, media, info, data, "Read1"), OHOS::Media::Status::OK);
    EXPECT_EQ(info.realReadLength_, TEST_BODY_SIZE);
    EXPECT_EQ(std::string(reinterpret_cast<char*>(data), info.realReadLength_), std::string(TEST_BODY_SIZE, 'y'));
    ReadDataInfo eosInfo; // Each read uses fresh output fields, as HttpSourcePlugin::Read does.
    eosInfo.wantReadLength_ = sizeof(data);
    EXPECT_EQ(ReadAppWithTrace(*trace, media, eosInfo, data, "Read2"), OHOS::Media::Status::END_OF_STREAM);
    EXPECT_EQ(eosInfo.realReadLength_, 0u);
    trace->Record("formal.media_close.begin", "synchronous=1");
    media->Close(false);
    trace->Record("formal.media_close.end", "synchronous=1");
    TraceAppRequest(*trace, media->GetDownloadRequest());
    media.reset();
    TraceAppHistory(*trace, *loader);
    EXPECT_EQ(loader->opens, 2);
    ASSERT_EQ(loader->offsets.size(), 2u);
    EXPECT_EQ(loader->offsets[0], 0);
    EXPECT_EQ(loader->offsets[1], 0);
    ASSERT_EQ(loader->closed.size(), 2u);
    EXPECT_EQ(loader->closed[1], 2);
}

HWTEST_F(SnifferCoordinatorUnitTest, AppLoaderRedirectOpensNewUuidAtTarget, TestSize.Level1)
{
    auto loader = std::make_shared<ProbeLoader>();
    loader->redirect = true;
    auto combinations = std::make_shared<MediaSourceLoaderCombinations>(loader);
    auto sniffer = std::make_shared<SnifferCoordinator>(combinations);
    sniffer->Start("http://app/resource", {});
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(sniffer->WaitResult(result));
    EXPECT_EQ(result->type, StreamProtocolType::HTTP);
    ASSERT_EQ(loader->urls.size(), 2u);
    EXPECT_EQ(loader->urls[0], "http://app/resource");
    EXPECT_EQ(loader->urls[1], "http://app/target");
    EXPECT_EQ(loader->closed, (std::vector<int64_t> {1, 2}));
    EXPECT_EQ(loader->offsets, (std::vector<int64_t> {0, 0}));
}

HWTEST_F(SnifferCoordinatorUnitTest, BothRequestsCarryAuthenticationHeaders, TestSize.Level1)
{
    HttpServerDemo::RegisterContent("/auth", std::string(TEST_BODY_SIZE, 'a'));
    std::map<std::string, std::string> headers {
        {"Cookie", "session=test"}, {"Authorization", "Bearer test"}, {"Referer", "http://example.test/"}};
    auto sniffer = std::make_shared<SnifferCoordinator>();
    sniffer->Start(BASE + "/auth", headers);
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(sniffer->WaitResult(result));
    auto media = std::make_shared<HttpMediaDownloader>(BASE + "/auth", 1, nullptr, false);
    media->Init();
    media->SetStatusCallback([](DownloadStatus, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {});
    ASSERT_TRUE(media->Open(BASE + "/auth", headers));
    uint8_t data[READ_BUFFER_SIZE] {};
    ReadDataInfo info;
    info.wantReadLength_ = sizeof(data);
    EXPECT_EQ(media->Read(data, info), OHOS::Media::Status::OK);
    EXPECT_EQ(info.realReadLength_, TEST_BODY_SIZE);
    ReadDataInfo eosInfo; // Do not reuse the preceding read's realReadLength_ output as a new input.
    eosInfo.wantReadLength_ = sizeof(data);
    EXPECT_EQ(media->Read(data, eosInfo), OHOS::Media::Status::END_OF_STREAM);
    EXPECT_EQ(eosInfo.realReadLength_, 0u);
    media->Close(false);
    auto requests = HttpServerDemo::GetRequests("/auth");
    std::vector<bool> authenticated;
    for (const auto& request : requests) {
        authenticated.push_back(request.find("Cookie:session=test") != std::string::npos &&
            request.find("Authorization:Bearer test") != std::string::npos &&
            request.find("Referer:http://example.test/") != std::string::npos);
    }
    EXPECT_EQ(authenticated, (std::vector<bool> {true, true}));
}

HWTEST_F(SnifferCoordinatorUnitTest, UndecidablePreambleIsBounded, TestSize.Level1)
{
    // The unfinished comment exceeds the probe cap and must fail without growing the buffer.
    HttpServerDemo::RegisterContent("/unknown", "<!--" + std::string(20000, 'x'));
    auto sniffer = std::make_shared<SnifferCoordinator>();
    sniffer->Start(BASE + "/unknown", {});
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(sniffer->WaitResult(result));
    EXPECT_EQ(result->bytesProbed, SnifferCoordinator::SNIFF_THRESHOLD);
}


HWTEST_F(SnifferCoordinatorUnitTest, SuccessfulShortWriteResponse23PreservesResult, TestSize.Level1)
{
    auto sniffer = PendingProbe();
    std::string bytes(MIN_SNIFF_BUFFER_SIZE, 'x');
    EXPECT_EQ(sniffer->OnData(reinterpret_cast<uint8_t*>(bytes.data()), bytes.size()), 0u);
    EXPECT_EQ(sniffer->state_, SnifferCoordinator::State::SUCCEEDED);
    sniffer->MarkReady(false, CURL_WRITE_ERROR);
    EXPECT_EQ(sniffer->lastError_, CURL_WRITE_ERROR);
    EXPECT_EQ(sniffer->state_, SnifferCoordinator::State::SUCCEEDED);
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(sniffer->WaitResult(result));
    EXPECT_TRUE(result->sniffOk);
    EXPECT_EQ(result->bytesProbed, MIN_SNIFF_BUFFER_SIZE);
    EXPECT_TRUE(std::all_of(sniffer->buffer_.begin(), sniffer->buffer_.end(), [](uint8_t v) { return v == 0; }));
}

HWTEST_F(SnifferCoordinatorUnitTest, TimeoutWithPartialPrefixCannotBecomeSuccessOn23, TestSize.Level1)
{
    auto sniffer = PendingProbe();
    std::string prefix = "<!--" + std::string(300, 'x'); // Exceed the first probe step but leave XML incomplete.
    EXPECT_EQ(sniffer->OnData(reinterpret_cast<uint8_t*>(prefix.data()), prefix.size()), prefix.size());
    sniffer->deadline_ = std::chrono::steady_clock::now();
    uint8_t next = 'x';
    EXPECT_EQ(sniffer->OnData(&next, 1), 0u);
    sniffer->MarkReady(false, CURL_WRITE_ERROR);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(sniffer->WaitResult(result));
    EXPECT_EQ(sniffer->state_, SnifferCoordinator::State::TIMED_OUT);
    EXPECT_FALSE(result->sniffOk);
    EXPECT_EQ(result->bytesProbed, prefix.size());
}

HWTEST_F(SnifferCoordinatorUnitTest, CancellationResponse23CannotStartFormalDownload, TestSize.Level1)
{
    auto sniffer = PendingProbe();
    sniffer->Stop();
    sniffer->Stop();
    uint8_t next = 'x';
    EXPECT_EQ(sniffer->OnData(&next, 1), 0u);
    sniffer->MarkReady(false, CURL_WRITE_ERROR);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(sniffer->WaitResult(result));
    EXPECT_EQ(sniffer->state_, SnifferCoordinator::State::CANCELLED);
    EXPECT_EQ(result->bytesProbed, 0u);
}

HWTEST_F(SnifferCoordinatorUnitTest, UnexpectedWriteError23IsNotIgnoredOrRetried, TestSize.Level1)
{
    auto sniffer = PendingProbe();
    sniffer->MarkReady(false, CURL_WRITE_ERROR);
    EXPECT_EQ(sniffer->state_, SnifferCoordinator::State::FAILED);
    EXPECT_FALSE(SnifferCoordinator::IsRetryableCurlError(CURL_WRITE_ERROR));
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(sniffer->WaitResult(result));
    EXPECT_FALSE(result->sniffOk);
}

HWTEST_F(SnifferCoordinatorUnitTest, EosAfterDeadlineCannotAcceptShortManifest, TestSize.Level1)
{
    auto sniffer = PendingProbe();
    std::string prefix = "<!-- unfinished";
    sniffer->OnData(reinterpret_cast<uint8_t*>(prefix.data()), prefix.size());
    sniffer->deadline_ = std::chrono::steady_clock::now();
    sniffer->MarkReady(true);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(sniffer->WaitResult(result));
    EXPECT_EQ(sniffer->state_, SnifferCoordinator::State::TIMED_OUT);
}

HWTEST_F(SnifferCoordinatorUnitTest, NoAppDataStillTimesOutAndClosesItsUuid, TestSize.Level1)
{
    auto loader = std::make_shared<SilentProbeLoader>();
    auto combinations = std::make_shared<MediaSourceLoaderCombinations>(loader);
    auto sniffer = std::make_shared<SnifferCoordinator>(combinations);
    sniffer->Start("http://app/no-response", {}, false, -1, SHORT_TIMEOUT_MS);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(sniffer->WaitResult(result));
    EXPECT_EQ(sniffer->state_, SnifferCoordinator::State::TIMED_OUT);
    EXPECT_EQ(sniffer->sniffer_, nullptr);
    EXPECT_TRUE(sniffer->finalized_);
    sniffer->Stop();
    sniffer.reset();
    EXPECT_EQ(loader->closed, (std::vector<int64_t> {1}));
}

HWTEST_F(SnifferCoordinatorUnitTest, ActualHttpShortWriteReports23AfterSuccess, TestSize.Level1)
{
    // A body much larger than the probe cap forces the real transport's short-write response.
    HttpServerDemo::RegisterContent("/response23", std::string(128 * 1024, 'x'));
    auto sniffer = std::make_shared<SnifferCoordinator>();
    sniffer->Start(BASE + "/response23", {});
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(sniffer->WaitResult(result));
    EXPECT_EQ(sniffer->lastError_, CURL_WRITE_ERROR);
    EXPECT_EQ(sniffer->state_, SnifferCoordinator::State::SUCCEEDED);
    EXPECT_EQ(sniffer->sniffer_, nullptr);
    EXPECT_EQ(HttpServerDemo::GetRequests("/response23").size(), 1u);
}

HWTEST_F(SnifferCoordinatorUnitTest, CancelAfterRecognitionVetoesFormalStart, TestSize.Level1)
{
    auto sniffer = PendingProbe();
    std::string bytes(MIN_SNIFF_BUFFER_SIZE, 'x');
    sniffer->OnData(reinterpret_cast<uint8_t*>(bytes.data()), bytes.size());
    sniffer->Stop();
    sniffer->MarkReady(false, CURL_WRITE_ERROR);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(sniffer->WaitResult(result));
    EXPECT_EQ(sniffer->state_, SnifferCoordinator::State::SUCCEEDED);
    EXPECT_TRUE(sniffer->finalized_);
}


HWTEST_F(SnifferCoordinatorUnitTest, CurlCompletionCallbackCanDeinitClient, TestSize.Level1)
{
    HttpServerDemo::RegisterContent("/callback-close", std::string(TEST_BODY_SIZE, 'x'));
    auto receive = [](void*, size_t size, size_t count, void*) { return size * count; };
    auto client = NetworkClient::GetInstance(receive, receive, nullptr);
    ASSERT_EQ(client->Init(), OHOS::Media::Status::OK);
    ASSERT_EQ(client->Open(BASE + "/callback-close", {}, REQUEST_TIMEOUT_MS), OHOS::Media::Status::OK);
    bool completed = false;
    RequestInfo info;
    EXPECT_EQ(client->RequestData(-1, 0, info,
        [&](int32_t, int32_t, const std::string&, OHOS::Media::Status) {
            EXPECT_EQ(client->Deinit(), OHOS::Media::Status::OK);
            completed = true;
        }), OHOS::Media::Status::OK);
    EXPECT_TRUE(completed);
}


HWTEST_F(SnifferCoordinatorUnitTest, SavedCallbacksRejectExpiredCoordinator, TestSize.Level1)
{
    HttpServerDemo::RegisterContent("/weak-callback", std::string(TEST_BODY_SIZE, 'x'));
    auto sniffer = std::make_shared<SnifferCoordinator>();
    std::weak_ptr<SnifferCoordinator> weakSniffer = sniffer;
    sniffer->Start(BASE + "/weak-callback", {});
    auto downloader = sniffer->sniffer_;
    ASSERT_NE(downloader, nullptr);
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(sniffer->WaitResult(result));
    // ForceClose has stopped the task; the retained request keeps the real lambdas alive.
    auto request = downloader->GetCurrentRequest();
    ASSERT_NE(request, nullptr);
    auto save = request->saveData_;
    auto status = request->statusCallback_;
    auto done = request->downloadDoneCallback_;
    ASSERT_TRUE(save);
    ASSERT_TRUE(status);
    ASSERT_TRUE(done);
    sniffer.reset();
    EXPECT_TRUE(weakSniffer.expired());
    uint8_t byte = 'x';
    EXPECT_EQ(save(&byte, 1, false), 0u);
    status(DownloadStatus::PARTTAL_DOWNLOAD, downloader, request);
    done("", "");
}
