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
#include <atomic>
#include <vector>
#include "gtest/gtest.h"
#include "sniffer_test_support.h"

using namespace testing::ext;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace SnifferTest;
namespace {
constexpr int32_t REDIRECT = 302;
constexpr int32_t SHORT_BUDGET_MS = 100;
constexpr char MOCK_URL[] = "http://probe.test/start";

struct Attempt {
    explicit Attempt(int32_t error = 0, std::string bytes = HLS, bool wait = false, bool redirect = false)
        : error(error), bytes(std::move(bytes)), wait(wait), redirect(redirect) {}
    int32_t error;
    std::string bytes;
    bool wait;
    bool redirect;
    bool holdOpen {false};
};

struct Script {
    explicit Script(std::vector<Attempt> attempts) : attempts(std::move(attempts)) {}
    std::vector<Attempt> attempts;
    std::atomic<size_t> created {0};
    std::vector<std::string> events;
    std::vector<std::string> urls;
    std::vector<std::map<std::string, std::string>> headers;
    std::vector<int32_t> uids;
    std::vector<std::chrono::steady_clock::time_point> deadlines;
    std::weak_ptr<SnifferCoordinator> probe;
    std::mutex mutex;
    Signal stalled;

    void Record(const std::string& event)
    {
        std::lock_guard<std::mutex> lock(mutex);
        events.push_back(event);
    }
};

// Only the network dependency is scripted. Coordinator, Downloader, Task and receive callbacks are real.
class ScriptClient : public NetworkClient {
public:
    ScriptClient(std::weak_ptr<Downloader> downloader, std::shared_ptr<Script> script, size_t index)
        : downloader_(std::move(downloader)), script_(std::move(script)), index_(index),
          attempt_(script_->attempts[std::min(index_, script_->attempts.size() - 1)]) {}

    Status Init() override { return Status::OK; }

    Status Open(const std::string& url, const std::map<std::string, std::string>& headers, int32_t) override
    {
        {
            std::lock_guard<std::mutex> lock(script_->mutex);
            script_->urls.push_back(url);
            script_->headers.push_back(headers);
        }
        script_->Record("open" + std::to_string(index_));
        if (attempt_.holdOpen) {
            WaitClosed();
        }
        return closed_ ? Status::ERROR_INVALID_OPERATION : Status::OK;
    }

    Status RequestData(long, int, const RequestInfo&, HandleResponseCbFunc complete) override
    {
        int32_t code = attempt_.error;
        if (attempt_.wait) {
            WaitClosed();
        }
        if (closed_) {
            code = ABORT_ERROR;
        } else if (attempt_.redirect) {
            SendHeader("location:/hop" + std::to_string(index_ + 1) + "\r\n");
        } else if (!attempt_.bytes.empty()) {
            SendHeader("content-length:" + std::to_string(attempt_.bytes.size()) + "\r\n");
            auto downloader = downloader_.lock();
            auto count = Downloader::RxBodyData(attempt_.bytes.data(), 1, attempt_.bytes.size(), downloader.get());
            if (count != attempt_.bytes.size()) {
                code = WRITE_ERROR;
            }
        }
        int32_t server = attempt_.redirect ? REDIRECT : 0;
        Status status = code != 0 ? Status::ERROR_CLIENT : (server != 0 ? Status::ERROR_SERVER : Status::OK);
        complete(code, server, "", status);
        script_->Record("end" + std::to_string(index_));
        return status;
    }

    Status Close(bool) override
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
            cv_.notify_all();
        }
        script_->Record("close" + std::to_string(index_));
        return Status::OK;
    }

    Status Deinit() override { return Status::OK; }
    Status GetIp(std::string&) override { return Status::ERROR_INVALID_OPERATION; }
    void SetAppUid(int32_t uid) override
    {
        std::lock_guard<std::mutex> lock(script_->mutex);
        script_->uids.push_back(uid);
    }

private:
    void WaitClosed()
    {
        script_->stalled.Set();
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(WAIT_MS * 2), [this] { return closed_.load(); });
    }

    void SendHeader(std::string text)
    {
        auto downloader = downloader_.lock();
        Downloader::RxHeaderData(text.data(), 1, text.size(), downloader.get());
    }

    std::weak_ptr<Downloader> downloader_;
    std::shared_ptr<Script> script_;
    size_t index_;
    Attempt attempt_;
    std::atomic<bool> closed_ {false};
    std::mutex mutex_;
    std::condition_variable cv_;
};

std::shared_ptr<SnifferCoordinator> MakeProbe(const std::shared_ptr<Script>& script)
{
    auto factory = [script] {
        auto downloader = std::make_shared<Downloader>("scripted_probe");
        size_t index = script->created.fetch_add(1);
        downloader->SetClient(std::make_shared<ScriptClient>(downloader, script, index));
        auto probe = script->probe.lock();
        if (probe != nullptr) {
            std::lock_guard<std::mutex> lock(probe->readyMutex_);
            script->deadlines.push_back(probe->deadline_);
        }
        return downloader;
    };
    auto probe = std::make_shared<SnifferCoordinator>(nullptr, factory);
    script->probe = probe;
    return probe;
}

bool ClosedBeforeNextOpen(const std::vector<std::string>& events, size_t count)
{
    for (size_t index = 1; index < count; ++index) {
        auto end = std::find(events.begin(), events.end(), "end" + std::to_string(index - 1));
        auto close = std::find(events.begin(), events.end(), "close" + std::to_string(index - 1));
        auto open = std::find(events.begin(), events.end(), "open" + std::to_string(index));
        if (end == events.end() || close == events.end() || open == events.end() || end >= open || close >= open) {
            return false;
        }
    }
    return true;
}
} // namespace

class SnifferRetryUnitTest : public testing::Test {};

HWTEST_F(SnifferRetryUnitTest, RetryableErrorSetAndAttemptLimit_001, TestSize.Level2)
{
    const std::vector<int32_t> codes {6, 7, 35, 55, 56}; // libcurl resolve/connect/TLS/send/receive errors.
    std::vector<size_t> counts;
    for (int32_t code : codes) {
        EXPECT_TRUE(SnifferCoordinator::IsRetryableCurlError(code));
        auto script = std::make_shared<Script>(std::vector<Attempt> {Attempt(code, "")});
        auto probe = MakeProbe(script);
        probe->Start(MOCK_URL, {});
        std::shared_ptr<SniffResult> result;
        EXPECT_FALSE(probe->WaitResult(result));
        counts.push_back(script->created.load());
        EXPECT_EQ(probe->state_, SnifferCoordinator::State::FAILED);
        EXPECT_TRUE(ClosedBeforeNextOpen(script->events, script->created));
    }
    EXPECT_EQ(counts, std::vector<size_t>(codes.size(), SnifferCoordinator::MAX_SNIFF_RETRY + 1));
}

HWTEST_F(SnifferRetryUnitTest, PermanentErrorsDoNotRetry_002, TestSize.Level2)
{
    const std::vector<int32_t> codes {18, 23, 28, 42, 52}; // Partial/write/timeout/abort/empty reply.
    std::vector<size_t> counts;
    for (int32_t code : codes) {
        EXPECT_FALSE(SnifferCoordinator::IsRetryableCurlError(code));
        auto script = std::make_shared<Script>(std::vector<Attempt> {Attempt(code, "")});
        auto probe = MakeProbe(script);
        probe->Start(MOCK_URL, {});
        std::shared_ptr<SniffResult> result;
        EXPECT_FALSE(probe->WaitResult(result));
        EXPECT_EQ(probe->state_, SnifferCoordinator::State::FAILED);
        counts.push_back(script->created);
    }
    EXPECT_EQ(counts, std::vector<size_t>(codes.size(), 1));
}

HWTEST_F(SnifferRetryUnitTest, RetryClearsPrefixAndPreservesHeadersUidDeadline_003, TestSize.Level2)
{
    // The failed attempt leaves incomplete XML. Appending HLS to it would fail to recognize HLS.
    auto script = std::make_shared<Script>(std::vector<Attempt> {Attempt(56, "<?xml "), Attempt()});
    auto probe = MakeProbe(script);
    const std::map<std::string, std::string> headers {{"Cookie", "session=test"}, {"Authorization", "Bearer test"}};
    probe->Start(MOCK_URL, headers, false, TEST_UID);
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(probe->WaitResult(result));
    EXPECT_EQ(result->type, StreamProtocolType::HLS);
    EXPECT_EQ(result->bytesProbed, sizeof(HLS) - 1);
    EXPECT_EQ(script->urls, (std::vector<std::string> {MOCK_URL, MOCK_URL}));
    ASSERT_EQ(script->headers.size(), 2u);
    EXPECT_EQ(script->headers[0], script->headers[1]);
    auto cookie = script->headers[0].find("Cookie");
    auto authorization = script->headers[0].find("Authorization");
    ASSERT_NE(cookie, script->headers[0].end());
    ASSERT_NE(authorization, script->headers[0].end());
    EXPECT_EQ(cookie->second, "session=test");
    EXPECT_EQ(authorization->second, "Bearer test");
    EXPECT_EQ(script->uids, (std::vector<int32_t> {TEST_UID, TEST_UID}));
    ASSERT_EQ(script->deadlines.size(), 2u);
    EXPECT_EQ(script->deadlines[0], script->deadlines[1]);
    EXPECT_TRUE(ClosedBeforeNextOpen(script->events, script->created));
}

HWTEST_F(SnifferRetryUnitTest, ExplicitRedirectLimitIsIndependentOfCurl_004, TestSize.Level2)
{
    auto script = std::make_shared<Script>(std::vector<Attempt> {Attempt(0, "", false, true)});
    auto probe = MakeProbe(script);
    probe->Start(MOCK_URL, {});
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(probe->WaitResult(result));
    constexpr size_t MAX_ATTEMPTS = 9; // Initial request plus the coordinator's eight explicit redirects.
    EXPECT_EQ(script->created, MAX_ATTEMPTS);
    ASSERT_EQ(script->urls.size(), MAX_ATTEMPTS);
    EXPECT_EQ(script->urls.back(), "http://probe.test/hop8");
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::FAILED);
    EXPECT_TRUE(ClosedBeforeNextOpen(script->events, MAX_ATTEMPTS));
}

HWTEST_F(SnifferRetryUnitTest, RedirectThenRetryCanSucceedWithinOriginalBudget_005, TestSize.Level2)
{
    auto script = std::make_shared<Script>(std::vector<Attempt> {
        Attempt(0, "", false, true), Attempt(7, ""), Attempt()});
    auto probe = MakeProbe(script);
    probe->Start(MOCK_URL, {});
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(probe->WaitResult(result));
    EXPECT_EQ(result->type, StreamProtocolType::HLS);
    EXPECT_EQ(script->urls, (std::vector<std::string> {
        MOCK_URL, "http://probe.test/hop1", "http://probe.test/hop1"}));
    ASSERT_EQ(script->deadlines.size(), 3u);
    EXPECT_TRUE(std::all_of(script->deadlines.begin(), script->deadlines.end(),
        [&](auto deadline) { return deadline == script->deadlines.front(); }));
    EXPECT_TRUE(ClosedBeforeNextOpen(script->events, script->created));
}

HWTEST_F(SnifferRetryUnitTest, StalledRetryUsesOriginalDeadline_006, TestSize.Level2)
{
    auto script = std::make_shared<Script>(std::vector<Attempt> {Attempt(6, ""), Attempt(0, "", true)});
    auto probe = MakeProbe(script);
    probe->Start(MOCK_URL, {}, false, -1, SHORT_BUDGET_MS);
    std::shared_ptr<SniffResult> result;
    auto waiter = std::async(std::launch::async, [&] { return probe->WaitResult(result); });
    bool entered = script->stalled.Wait();
    bool timely = waiter.wait_for(std::chrono::milliseconds(WAIT_MS)) == std::future_status::ready;
    if (!timely) {
        probe->Stop();
    }
    EXPECT_FALSE(waiter.get());
    EXPECT_TRUE(entered);
    EXPECT_TRUE(timely);
    EXPECT_EQ(script->created, 2u);
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::TIMED_OUT);
    ASSERT_EQ(script->deadlines.size(), 2u);
    EXPECT_EQ(script->deadlines[0], script->deadlines[1]);
}

HWTEST_F(SnifferRetryUnitTest, CancelStalledRetryDoesNotStartAnotherAttempt_007, TestSize.Level2)
{
    auto script = std::make_shared<Script>(std::vector<Attempt> {Attempt(55, ""), Attempt(0, "", true)});
    auto probe = MakeProbe(script);
    probe->Start(MOCK_URL, {});
    std::shared_ptr<SniffResult> result;
    auto waiter = std::async(std::launch::async, [&] { return probe->WaitResult(result); });
    bool entered = script->stalled.Wait();
    probe->Stop();
    bool timely = waiter.wait_for(std::chrono::milliseconds(WAIT_MS)) == std::future_status::ready;
    EXPECT_FALSE(waiter.get());
    EXPECT_TRUE(entered);
    EXPECT_TRUE(timely);
    EXPECT_EQ(script->created, 2u);
    EXPECT_EQ(probe->sniffer_, nullptr);
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::CANCELLED);
}

HWTEST_F(SnifferRetryUnitTest, TimeoutBeforeAnyReceiveCallback_008, TestSize.Level2)
{
    Attempt connecting;
    connecting.holdOpen = true;
    auto script = std::make_shared<Script>(std::vector<Attempt> {connecting});
    auto probe = MakeProbe(script);
    probe->Start(MOCK_URL, {}, false, -1, SHORT_BUDGET_MS);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(probe->WaitResult(result));
    EXPECT_EQ(script->created, 1u);
    EXPECT_EQ(result->bytesProbed, 0u);
    EXPECT_EQ(probe->sniffer_, nullptr);
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::TIMED_OUT);
}

HWTEST_F(SnifferRetryUnitTest, Reported302PreservesHeadersUidAndDeadline_009, TestSize.Level2)
{
    auto script = std::make_shared<Script>(std::vector<Attempt> {Attempt(0, "", false, true), Attempt()});
    auto probe = MakeProbe(script);
    const std::map<std::string, std::string> headers {{"Cookie", "probe=test"}, {"Authorization", "Bearer probe"}};
    probe->Start(MOCK_URL, headers, false, TEST_UID);
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(probe->WaitResult(result));
    EXPECT_EQ(result->type, StreamProtocolType::HLS);
    EXPECT_EQ(result->bytesProbed, sizeof(HLS) - 1);
    EXPECT_EQ(script->created, 2u);
    EXPECT_EQ(script->urls, (std::vector<std::string> {MOCK_URL, "http://probe.test/hop1"}));
    ASSERT_EQ(script->headers.size(), 2u);
    EXPECT_EQ(script->headers[0], script->headers[1]);
    constexpr char USER_AGENT_HEADER[] = "User-Agent";
    auto defaultUa = script->headers[0].find(USER_AGENT_HEADER);
    ASSERT_NE(defaultUa, script->headers[0].end());
    constexpr char DEFAULT_UA_PREFIX[] = " AVPlayerLib "; // The version suffix depends on the running system.
    EXPECT_EQ(defaultUa->second.find(DEFAULT_UA_PREFIX), 0u);
    auto forwarded = script->headers[0];
    EXPECT_EQ(forwarded.erase(USER_AGENT_HEADER), 1u);
    EXPECT_EQ(forwarded, headers);
    EXPECT_EQ(script->uids, (std::vector<int32_t> {TEST_UID, TEST_UID}));
    ASSERT_EQ(script->deadlines.size(), 2u);
    EXPECT_EQ(script->deadlines[0], script->deadlines[1]);
    EXPECT_TRUE(ClosedBeforeNextOpen(script->events, 2));
}
} // namespace OHOS::Media::Plugins::HttpPlugin
