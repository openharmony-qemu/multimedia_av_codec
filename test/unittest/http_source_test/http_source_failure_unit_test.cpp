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
#include <condition_variable>
#include <future>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "http_source_plugin.h"
#include "source_callback.h"
#include "common/media_core.h"
#include "source/source.h"
#include "sniffer_test_support.h"
#include "monitor/download_monitor.h"

using namespace testing::ext;
namespace OHOS::Media::Plugins::HttpPlugin {
namespace {
constexpr auto EVENT_WAIT = std::chrono::seconds(2);
class FailureCallback : public SourceCallback {
public:
    void OnEvent(const PluginEvent& event) override
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            events.push_back(event);
        }
        if (hook) {
            hook();
        }
        std::lock_guard<std::mutex> lock(mutex);
        completed = true;
        condition.notify_all();
    }
    bool Wait()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return condition.wait_for(lock, EVENT_WAIT, [this] { return completed; });
    }
    std::mutex mutex;
    std::condition_variable condition;
    std::vector<PluginEvent> events;
    std::function<void()> hook;
    bool completed {false};
};
}

class HttpSourceFailureUnitTest : public testing::Test {
public:
    void SetUp() override
    {
        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        ASSERT_NE(socket_, -1);
        sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        ASSERT_EQ(bind(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);
        socklen_t size = sizeof(address);
        ASSERT_EQ(getsockname(socket_, reinterpret_cast<sockaddr*>(&address), &size), 0);
        // Reserve an unused local port without listening: connect must fail deterministically.
        url_ = "http://127.0.0.1:" + std::to_string(ntohs(address.sin_port)) + "/source.m3u8";
        plugin_ = std::make_shared<HttpSourcePlugin>("failure_test");
        callback_ = std::make_shared<FailureCallback>();
    }
    void TearDown() override
    {
        if (plugin_ != nullptr) {
            plugin_->Reset();
        }
        close(socket_);
    }
    int socket_ {-1};
    std::string url_;
    std::shared_ptr<HttpSourcePlugin> plugin_;
    std::shared_ptr<FailureCallback> callback_;
};

HWTEST_F(HttpSourceFailureUnitTest, ConnectionFailureUsesCallbackAndUnknownSize, TestSize.Level1)
{
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(url_)), Status::OK);
    uint64_t size = 1;
    EXPECT_EQ(plugin_->GetSize(size), Status::OK);
    EXPECT_EQ(size, 0u);
    EXPECT_EQ(plugin_->GetDownloader(), nullptr);
    ASSERT_TRUE(callback_->Wait());
    std::lock_guard<std::mutex> lock(callback_->mutex);
    ASSERT_EQ(callback_->events.size(), 1u);
    EXPECT_EQ(callback_->events.front().type, PluginEventType::SERVER_ERROR);
    EXPECT_EQ(AnyCast<int32_t>(callback_->events.front().param), MSERR_IO_CANNOT_FIND_HOST);
}

HWTEST_F(HttpSourceFailureUnitTest, LateCallbackReceivesPendingFailureOnce, TestSize.Level1)
{
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(url_)), Status::OK);
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    ASSERT_TRUE(callback_->Wait());
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    ASSERT_EQ(plugin_->Reset(), Status::OK);
    std::lock_guard<std::mutex> lock(callback_->mutex);
    EXPECT_EQ(callback_->events.size(), 1u);
}

HWTEST_F(HttpSourceFailureUnitTest, FailedProbeReadAndPrepareTerminateWithoutEos, TestSize.Level1)
{
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(url_)), Status::OK);
    ASSERT_TRUE(callback_->Wait());
    std::shared_ptr<Buffer> buffer;
    EXPECT_EQ(plugin_->Read(buffer, 0, 1), Status::ERROR_UNKNOWN);
    EXPECT_EQ(plugin_->Prepare(), Status::ERROR_UNKNOWN);
}

HWTEST_F(HttpSourceFailureUnitTest, CloseInvalidatesPendingFailureBeforeCallbackRegistration, TestSize.Level1)
{
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(url_)), Status::OK);
    ASSERT_EQ(plugin_->Reset(), Status::OK);
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    uint64_t size = 1;
    EXPECT_EQ(plugin_->GetSize(size), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin_->GetProbeFailure(), nullptr);
    std::lock_guard<std::mutex> lock(callback_->mutex);
    EXPECT_EQ(callback_->events.size(), 0u);
}

HWTEST_F(HttpSourceFailureUnitTest, CallbackCanResetWithoutWaitingForItself, TestSize.Level1)
{
    Status reset = Status::ERROR_UNKNOWN;
    std::weak_ptr<HttpSourcePlugin> weak = plugin_;
    callback_->hook = [weak, &reset] {
        auto plugin = weak.lock();
        if (plugin != nullptr) {
            reset = plugin->Reset();
        }
    };
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(url_)), Status::OK);
    ASSERT_TRUE(callback_->Wait());
    EXPECT_EQ(reset, Status::OK);
    EXPECT_EQ(plugin_->GetProbeFailure(), nullptr);
}

HWTEST_F(HttpSourceFailureUnitTest, CallbackCanReleaseLastPluginOwner, TestSize.Level1)
{
    std::weak_ptr<HttpSourcePlugin> weak = plugin_;
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    auto* target = plugin_.get();
    callback_->hook = [this] { plugin_.reset(); };
    ASSERT_EQ(target->SetSource(std::make_shared<MediaSource>(url_)), Status::OK);
    ASSERT_TRUE(callback_->Wait());
    EXPECT_TRUE(weak.expired());
}

HWTEST_F(HttpSourceFailureUnitTest, ExternalCloseDrainsClaimedNotification, TestSize.Level1)
{
    SnifferTest::Signal entered;
    SnifferTest::Signal release;
    SnifferTest::Signal closing;
    callback_->hook = [&] { entered.Set(); release.Wait(); };
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(url_)), Status::OK);
    bool arrived = entered.Wait();
    if (!arrived) {
        release.Set();
    }
    auto close = std::async(std::launch::async, [&] { closing.Set(); return plugin_->Reset(); });
    bool started = closing.Wait();
    constexpr auto DRAIN_CHECK = std::chrono::milliseconds(50);
    auto pending = close.wait_for(DRAIN_CHECK);
    release.Set();
    EXPECT_TRUE(arrived);
    EXPECT_TRUE(started);
    EXPECT_EQ(pending, std::future_status::timeout);
    EXPECT_EQ(close.get(), Status::OK);
    ASSERT_TRUE(callback_->Wait());
    std::lock_guard<std::mutex> lock(callback_->mutex);
    EXPECT_EQ(callback_->events.size(), 1u);
}

HWTEST_F(HttpSourceFailureUnitTest, SourceWrapperPreservesCallbackAndUnknownSize, TestSize.Level1)
{
    auto source = std::make_shared<OHOS::Media::Source>();
    source->plugin_ = plugin_;
    source->SetCallback(callback_);
    ASSERT_EQ(source->InitPlugin(std::make_shared<MediaSource>(url_)), Status::OK);
    uint64_t size = 1;
    EXPECT_EQ(source->GetSize(size), Status::OK);
    EXPECT_EQ(size, 0u);
    EXPECT_EQ(source->Prepare(), Status::ERROR_UNKNOWN);
    ASSERT_TRUE(callback_->Wait());
    std::lock_guard<std::mutex> lock(callback_->mutex);
    ASSERT_EQ(callback_->events.size(), 1u);
    EXPECT_EQ(AnyCast<int32_t>(callback_->events.front().param), MSERR_IO_CANNOT_FIND_HOST);
}

HWTEST_F(HttpSourceFailureUnitTest, FrozenNetworkFailureSurvivesCleanupResponse, TestSize.Level1)
{
    auto probe = SnifferTest::Pending();
    constexpr int32_t TLS_FAILURE = 35;
    probe->MarkReady(false, TLS_FAILURE);
    ASSERT_FALSE(probe->SelectNextAttemptLocked());
    probe->MarkReady(false, SnifferTest::ABORT_ERROR);
    auto failure = probe->GetFailure();
    EXPECT_EQ(failure.kind, ProbeFailureKind::NETWORK);
    EXPECT_EQ(failure.clientCode, TLS_FAILURE);
    EXPECT_EQ(failure.serverCode, 0);
    EXPECT_EQ(failure.routeId, probe->GetRouteId());
}

HWTEST_F(HttpSourceFailureUnitTest, TimeoutDecisionSurvivesCleanupAndStop, TestSize.Level1)
{
    auto probe = SnifferTest::Pending();
    probe->deadline_ = std::chrono::steady_clock::now();
    uint8_t byte = 0;
    EXPECT_EQ(probe->OnData(&byte, 1), 0u);
    probe->MarkReady(false, SnifferTest::ABORT_ERROR);
    probe->Stop();
    auto failure = probe->GetFailure();
    EXPECT_EQ(failure.kind, ProbeFailureKind::TIMEOUT);
    EXPECT_EQ(failure.clientCode, 0);
    auto event = BuildProbeFailureEvent(failure, url_);
    EXPECT_EQ(AnyCast<int32_t>(event.param), MSERR_IO_CONNECTION_TIMEOUT);
}

HWTEST_F(HttpSourceFailureUnitTest, ProbeMappingMatchesEveryExistingMonitorEntry, TestSize.Level1)
{
    DownloadMonitor monitor(nullptr);
    for (const auto& entry : monitor.clientErrorCodeMap_) {
        auto event = BuildProbeFailureEvent({ProbeFailureKind::NETWORK, entry.first, 0, 0}, url_);
        EXPECT_EQ(AnyCast<int32_t>(event.param), entry.second) << entry.first;
    }
    for (const auto& entry : monitor.serverErrorCodeMap_) {
        auto event = BuildProbeFailureEvent({ProbeFailureKind::NETWORK, 0, entry.first, 0}, url_);
        EXPECT_EQ(AnyCast<int32_t>(event.param), entry.second) << entry.first;
    }
    constexpr int32_t UNKNOWN_NETWORK_FAILURE = 999;
    int32_t code = MSERR_DATA_SOURCE_IO_ERROR;
    monitor.GetClientMediaServiceErrorCode(UNKNOWN_NETWORK_FAILURE, code);
    auto event = BuildProbeFailureEvent({ProbeFailureKind::NETWORK, UNKNOWN_NETWORK_FAILURE, 0, 0}, url_);
    EXPECT_EQ(AnyCast<int32_t>(event.param), code);
}

HWTEST_F(HttpSourceFailureUnitTest, CancelledProbeDoesNotBecomeNetworkFailure, TestSize.Level1)
{
    auto probe = SnifferTest::Pending();
    probe->Stop();
    probe->MarkReady(false, SnifferTest::ABORT_ERROR);
    EXPECT_EQ(probe->GetFailure().kind, ProbeFailureKind::CANCELLED);
    EXPECT_EQ(probe->GetFailure().clientCode, 0);
}

HWTEST_F(HttpSourceFailureUnitTest, SuccessfulShortWriteDoesNotCreateFailure, TestSize.Level1)
{
    auto probe = SnifferTest::Pending();
    auto bytes = std::string(SnifferTest::HLS);
    EXPECT_EQ(probe->OnData(reinterpret_cast<uint8_t*>(bytes.data()), bytes.size()), 0u);
    probe->MarkReady(false, SnifferTest::WRITE_ERROR);
    EXPECT_EQ(probe->GetFailure().kind, ProbeFailureKind::NONE);
    EXPECT_EQ(probe->GetFailure().clientCode, 0);
    EXPECT_TRUE(probe->result_->sniffOk);
}

HWTEST_F(HttpSourceFailureUnitTest, LocalFdFailureStaysOutOfNetworkCallback, TestSize.Level1)
{
    auto probe = std::make_shared<SnifferCoordinator>();
    probe->Start("fd://invalid", {}, true);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(probe->WaitResult(result));
    EXPECT_EQ(probe->GetFailure().kind, ProbeFailureKind::LOCAL_INPUT);
}

HWTEST_F(HttpSourceFailureUnitTest, NewRouteDropsPreviousPendingError, TestSize.Level1)
{
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(url_)), Status::OK);
    auto previous = plugin_->GetProbeFailure();
    ASSERT_NE(previous, nullptr);
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(url_ + "?generation=2")), Status::OK);
    auto current = plugin_->GetProbeFailure();
    ASSERT_NE(current, nullptr);
    EXPECT_NE(previous, current);
    previous->SetCallback(callback_);
    previous->Notify();
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    ASSERT_TRUE(callback_->Wait());
    ASSERT_EQ(plugin_->Reset(), Status::OK);
    std::lock_guard<std::mutex> lock(callback_->mutex);
    EXPECT_EQ(callback_->events.size(), 1u);
}

HWTEST_F(HttpSourceFailureUnitTest, InterruptDropsPendingError, TestSize.Level1)
{
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(url_)), Status::OK);
    plugin_->SetInterruptState(true);
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    EXPECT_EQ(plugin_->GetProbeFailure(), nullptr);
    std::lock_guard<std::mutex> lock(callback_->mutex);
    EXPECT_EQ(callback_->events.size(), 0u);
}

HWTEST_F(HttpSourceFailureUnitTest, ExpiredCallbackCanBeReplacedBeforeDelivery, TestSize.Level1)
{
    auto expired = std::make_shared<FailureCallback>();
    ASSERT_EQ(plugin_->SetCallback(expired), Status::OK);
    expired.reset();
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(url_)), Status::OK);
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    ASSERT_TRUE(callback_->Wait());
    ASSERT_EQ(plugin_->Reset(), Status::OK);
    std::lock_guard<std::mutex> lock(callback_->mutex);
    EXPECT_EQ(callback_->events.size(), 1u);
}

HWTEST_F(HttpSourceFailureUnitTest, InvalidSourceRemainsSynchronous, TestSize.Level1)
{
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    EXPECT_EQ(plugin_->SetSource(nullptr), Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(plugin_->SetSource(std::make_shared<MediaSource>("file:///forbidden")), Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(plugin_->GetProbeFailure(), nullptr);
    std::lock_guard<std::mutex> lock(callback_->mutex);
    EXPECT_EQ(callback_->events.size(), 0u);
}

HWTEST_F(HttpSourceFailureUnitTest, FailedReadClearsPreviouslyPopulatedBuffer, TestSize.Level1)
{
    ASSERT_EQ(plugin_->SetCallback(callback_), Status::OK);
    ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(url_)), Status::OK);
    auto buffer = Buffer::CreateDefaultBuffer(1);
    ASSERT_NE(buffer, nullptr);
    auto memory = buffer->GetMemory();
    ASSERT_NE(memory, nullptr);
    memory->UpdateDataSize(1);
    EXPECT_EQ(plugin_->Read(buffer, 0, 1), Status::ERROR_UNKNOWN);
    EXPECT_EQ(memory->GetSize(), 0u);
}

HWTEST_F(HttpSourceFailureUnitTest, UnreachablePathsHaveIdenticalErrorChannelRegardlessOfSuffix, TestSize.Level1)
{
    const std::vector<std::string> paths {"/movie.mp4", "/playlist.m3u8", "/manifest.mpd", "/no-extension"};
    const auto base = url_.substr(0, url_.find_last_of('/'));
    size_t reported = 0;
    for (const auto& path : paths) {
        auto observer = std::make_shared<FailureCallback>();
        ASSERT_EQ(plugin_->SetCallback(observer), Status::OK);
        ASSERT_EQ(plugin_->SetSource(std::make_shared<MediaSource>(base + path)), Status::OK);
        ASSERT_TRUE(observer->Wait());
        ASSERT_EQ(plugin_->Reset(), Status::OK);
        std::lock_guard<std::mutex> lock(observer->mutex);
        ASSERT_EQ(observer->events.size(), 1u);
        EXPECT_EQ(AnyCast<int32_t>(observer->events.front().param), MSERR_IO_CANNOT_FIND_HOST);
        reported += observer->events.size();
    }
    EXPECT_EQ(reported, paths.size());
}
} // namespace OHOS::Media::Plugins::HttpPlugin
