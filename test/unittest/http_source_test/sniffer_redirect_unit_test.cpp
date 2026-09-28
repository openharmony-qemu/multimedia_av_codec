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
#include <vector>
#include "gtest/gtest.h"
#include "sniffer_test_support.h"

using namespace testing::ext;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace SnifferTest;
namespace {
constexpr int32_t REDIRECT = 302; // The existing Downloader reports explicit redirects using this code.
constexpr int32_t REDIRECT_LIMIT = 8; // The probe allows eight redirects after its initial request.

std::shared_ptr<SnifferCoordinator> PendingWithDownloader()
{
    auto probe = Pending();
    probe->sniffer_ = std::make_shared<Downloader>("redirect_policy");
    return probe;
}
} // namespace

class SnifferRedirectUnitTest : public testing::Test {};

HWTEST_F(SnifferRedirectUnitTest, Reported302PrecedesShortWriteError_001, TestSize.Level2)
{
    auto probe = PendingWithDownloader();
    probe->MarkReady(false, WRITE_ERROR, REDIRECT);
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::PROBING);
    EXPECT_TRUE(probe->responseReady_);
    EXPECT_EQ(probe->lastError_, WRITE_ERROR);
    EXPECT_TRUE(probe->SelectNextAttemptLocked());
    EXPECT_EQ(probe->redirects_, 1);
    EXPECT_EQ(probe->retries_, 0);
}

HWTEST_F(SnifferRedirectUnitTest, OtherReportedStatusesDoNotRedirect_002, TestSize.Level2)
{
    // Raw statuses are not reconstructed here; only Downloader's normalized 302 is a redirect signal.
    const std::vector<int32_t> statuses {301, 307, 308, 404, 500}; // Other redirects, Not Found, Server Error.
    std::vector<int32_t> observed;
    for (int32_t status : statuses) {
        auto probe = PendingWithDownloader();
        probe->MarkReady(false, 0, status);
        EXPECT_FALSE(probe->SelectNextAttemptLocked());
        EXPECT_EQ(probe->state_, SnifferCoordinator::State::FAILED);
        observed.push_back(probe->redirects_);
    }
    EXPECT_EQ(observed, std::vector<int32_t>(statuses.size(), 0));
}

HWTEST_F(SnifferRedirectUnitTest, PublishedSuccessIgnoresLate302_003, TestSize.Level2)
{
    auto probe = PendingWithDownloader();
    std::string bytes(HLS);
    EXPECT_EQ(probe->OnData(reinterpret_cast<uint8_t*>(bytes.data()), bytes.size()), 0u);
    probe->MarkReady(true);
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::SUCCEEDED);
    probe->MarkReady(false, WRITE_ERROR, REDIRECT);
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(probe->WaitResult(result));
    EXPECT_EQ(result->type, StreamProtocolType::HLS);
    EXPECT_EQ(probe->redirects_, 0);
}

HWTEST_F(SnifferRedirectUnitTest, CancelledProbeIgnoresLate302_004, TestSize.Level2)
{
    auto probe = PendingWithDownloader();
    probe->Stop();
    probe->MarkReady(false, WRITE_ERROR, REDIRECT);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(probe->WaitResult(result));
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::CANCELLED);
    EXPECT_FALSE(probe->responseReady_);
    EXPECT_EQ(probe->redirects_, 0);
}

HWTEST_F(SnifferRedirectUnitTest, ExpiredBudgetRejects302_005, TestSize.Level2)
{
    auto probe = PendingWithDownloader();
    probe->deadline_ = std::chrono::steady_clock::now();
    probe->MarkReady(false, 0, REDIRECT);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(probe->WaitResult(result));
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::TIMED_OUT);
    EXPECT_EQ(probe->redirects_, 0);
}

HWTEST_F(SnifferRedirectUnitTest, RedirectLimitDoesNotResetBudget_006, TestSize.Level2)
{
    auto probe = PendingWithDownloader();
    auto deadline = probe->deadline_;
    probe->redirects_ = REDIRECT_LIMIT;
    probe->MarkReady(false, 0, REDIRECT);
    EXPECT_FALSE(probe->SelectNextAttemptLocked());
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::FAILED);
    EXPECT_EQ(probe->redirects_, REDIRECT_LIMIT);
    EXPECT_EQ(probe->deadline_, deadline);
}

HWTEST_F(SnifferRedirectUnitTest, MissingTargetStopsBeforeAnotherAttempt_007, TestSize.Level2)
{
    auto probe = PendingWithDownloader();
    auto previous = probe->sniffer_;
    previous->currentRequest_ = std::make_shared<DownloadRequest>(
        "http://probe.test/start", DataSaveFunc {}, StatusCallbackFunc {}, true);
    probe->MarkReady(false, 0, REDIRECT);
    EXPECT_FALSE(probe->RestartAttempt(previous));
    EXPECT_TRUE(previous->forceClosed_);
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::FAILED);
    EXPECT_EQ(probe->attempt_, 0u);
}

HWTEST_F(SnifferRedirectUnitTest, ShortWriteWithout302StillFails_008, TestSize.Level2)
{
    auto probe = PendingWithDownloader();
    probe->MarkReady(false, WRITE_ERROR);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(probe->WaitResult(result));
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::FAILED);
    EXPECT_EQ(probe->redirects_, 0);
}
} // namespace OHOS::Media::Plugins::HttpPlugin
