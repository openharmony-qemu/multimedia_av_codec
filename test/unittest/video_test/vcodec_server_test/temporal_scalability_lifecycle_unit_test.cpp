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
#include <future>
#include <thread>
#include "gtest/gtest.h"
#include "codecbase.h"
#include "codec_server.h"
#include "avcodec_errors.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media;

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr auto WAIT_TIMEOUT = std::chrono::seconds(2);

template<typename Predicate>
bool WaitUntil(Predicate predicate)
{
    auto deadline = std::chrono::steady_clock::now() + WAIT_TIMEOUT;
    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        std::this_thread::yield();
    }
    return true;
}
} // namespace

class TemporalScalabilityLifecycleTest : public testing::Test {
public:
    void SetUp() override
    {
        mock_ = std::make_shared<NiceMock<CodecBaseMock>>();
        CodecBase::RegisterMock(mock_);
        server_ = std::make_shared<CodecServer>();
        server_->codecBase_ = std::make_shared<CodecBase>();
        server_->status_ = CodecServer::RUNNING;
        server_->codecType_ = AVCODEC_TYPE_VIDEO_ENCODER;
        (void)server_->ExchangeTemporalScalability(std::make_shared<TemporalScalability>("video.encoder.avc"));
    }

    void TearDown() override
    {
        (void)server_->Release();
        server_.reset();
        mock_.reset();
    }

    void CheckCallbackLifetime(bool output, bool reset, int32_t engineResult = AVCS_ERR_OK)
    {
        ON_CALL(*mock_, Reset()).WillByDefault(Return(engineResult));
        ON_CALL(*mock_, Release()).WillByDefault(Return(engineResult));
        auto buffer = AVBuffer::CreateAVBuffer();
        ASSERT_NE(buffer, nullptr);
        ASSERT_NE(buffer->meta_, nullptr);
        buffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        auto temporal = server_->GetTemporalScalability();
        std::weak_ptr<TemporalScalability> observer = temporal;
        std::unique_lock<std::shared_mutex> park(output ? temporal->frameFlagMapMutex_ : temporal->inputBufMutex_);
        auto callback = std::async(std::launch::async, [&] {
            if (output) {
                server_->OnOutputBufferAvailable(0, buffer);
            } else {
                server_->OnInputBufferAvailable(0, buffer);
            }
        });
        // Server + test owner + callback. Holding the member mutex keeps this count stable.
        bool acquired = WaitUntil([&] { return observer.use_count() == 3; });
        if (!acquired) {
            park.unlock();
            callback.get();
            FAIL() << "Callback did not retain its own reference";
        }
        auto shutdown = std::async(std::launch::async, [&] { return reset ? server_->Reset() : server_->Release(); });
        auto shutdownStatus = shutdown.wait_for(WAIT_TIMEOUT);
        EXPECT_EQ(shutdownStatus, std::future_status::ready);
        if (shutdownStatus == std::future_status::ready) {
            EXPECT_EQ(server_->GetTemporalScalability(), nullptr);
            // The test owner keeps failure cleanup safe; the other reference must be the callback's.
            EXPECT_EQ(observer.use_count(), 2);
        }
        park.unlock();
        callback.get();
        EXPECT_EQ(shutdown.get(), engineResult);
        EXPECT_EQ(observer.use_count(), 1);
        temporal.reset();
        EXPECT_TRUE(observer.expired());
    }

    void CheckEosCancellation(bool reset)
    {
        EXPECT_CALL(*mock_, NotifyEos()).Times(0);
        server_->inputParamTask_ = std::make_shared<TaskThread>("EosCancelTest");
        auto temporal = server_->GetTemporalScalability();
        auto eos = std::async(std::launch::async, [&] { return server_->NotifyEos(); });
        bool waiting = WaitUntil([&] { return temporal->eosRequested_.load(); });
        EXPECT_TRUE(waiting);
        auto shutdown = std::async(std::launch::async, [&] { return reset ? server_->Reset() : server_->Release(); });
        EXPECT_EQ(shutdown.wait_for(WAIT_TIMEOUT), std::future_status::ready);
        auto eosStatus = eos.wait_for(WAIT_TIMEOUT);
        EXPECT_EQ(eosStatus, std::future_status::ready);
        if (eosStatus != std::future_status::ready) {
            // Release a broken implementation's waiter so a failing test can still finish.
            temporal->CheckInputQueEmpty();
        }
        EXPECT_EQ(eos.get(), AVCS_ERR_INVALID_STATE);
        EXPECT_EQ(shutdown.get(), AVCS_ERR_OK);
        EXPECT_EQ(temporal->NotifyEosWhenComplete(), AVCS_ERR_INVALID_STATE);
    }

    std::shared_ptr<CodecBaseMock> mock_;
    std::shared_ptr<CodecServer> server_;
};

HWTEST_F(TemporalScalabilityLifecycleTest, InputCallbackDuringReset, TestSize.Level1)
{
    CheckCallbackLifetime(false, true);
}

HWTEST_F(TemporalScalabilityLifecycleTest, OutputCallbackDuringReset, TestSize.Level1)
{
    CheckCallbackLifetime(true, true);
}

HWTEST_F(TemporalScalabilityLifecycleTest, InputCallbackDuringRelease, TestSize.Level1)
{
    CheckCallbackLifetime(false, false);
}

HWTEST_F(TemporalScalabilityLifecycleTest, OutputCallbackDuringRelease, TestSize.Level1)
{
    CheckCallbackLifetime(true, false);
}

HWTEST_F(TemporalScalabilityLifecycleTest, FailedResetPreservesInFlightObject, TestSize.Level1)
{
    CheckCallbackLifetime(true, true, AVCS_ERR_UNKNOWN);
}

HWTEST_F(TemporalScalabilityLifecycleTest, FailedReleasePreservesInFlightObject, TestSize.Level1)
{
    CheckCallbackLifetime(false, false, AVCS_ERR_UNKNOWN);
}

HWTEST_F(TemporalScalabilityLifecycleTest, ResetCancelsEosWait, TestSize.Level1)
{
    CheckEosCancellation(true);
}

HWTEST_F(TemporalScalabilityLifecycleTest, ReleaseCancelsEosWait, TestSize.Level1)
{
    CheckEosCancellation(false);
}

HWTEST_F(TemporalScalabilityLifecycleTest, ShutdownWakesFullInputQueue, TestSize.Level1)
{
    auto temporal = server_->GetTemporalScalability();
    auto buffer = AVBuffer::CreateAVBuffer();
    ASSERT_NE(buffer, nullptr);
    auto capacity = static_cast<uint32_t>(temporal->inputIndexQueue_->Capacity());
    for (uint32_t index = 0; index < capacity; ++index) {
        temporal->StoreAVBuffer(index, buffer);
    }
    auto producer = std::async(std::launch::async, [&] { temporal->StoreAVBuffer(capacity, buffer); });
    // The queue is full; the producer cannot finish until shutdown wakes it.
    EXPECT_EQ(producer.wait_for(std::chrono::milliseconds(50)), std::future_status::timeout);
    temporal->BeginShutdown();
    auto producerStatus = producer.wait_for(WAIT_TIMEOUT);
    EXPECT_EQ(producerStatus, std::future_status::ready);
    if (producerStatus != std::future_status::ready) {
        temporal->inputIndexQueue_->Clear();
    }
    producer.get();
    EXPECT_EQ(temporal->inputBufferMap_.count(capacity), 0U);
    EXPECT_EQ(temporal->inputBufferMap_.size(), capacity);
}

HWTEST_F(TemporalScalabilityLifecycleTest, QueueCanResumeButShutdownIsTerminal, TestSize.Level1)
{
    auto temporal = server_->GetTemporalScalability();
    auto buffer = AVBuffer::CreateAVBuffer();
    ASSERT_NE(buffer, nullptr);
    temporal->SetBlockQueueActive();
    temporal->StoreAVBuffer(0, buffer);
    EXPECT_TRUE(temporal->inputBufferMap_.empty());
    temporal->SetBlockQueueActive(true);
    temporal->StoreAVBuffer(1, buffer);
    EXPECT_EQ(temporal->inputBufferMap_.count(1), 1U);
    temporal->BeginShutdown();
    temporal->SetBlockQueueActive(true);
    temporal->StoreAVBuffer(2, buffer);
    EXPECT_EQ(temporal->inputBufferMap_.count(2), 0U);
}

HWTEST_F(TemporalScalabilityLifecycleTest, RetiredReferenceDoesNotSwitchToNewSession, TestSize.Level1)
{
    auto old = server_->GetTemporalScalability();
    ASSERT_EQ(server_->Reset(), AVCS_ERR_OK);
    auto current = std::make_shared<TemporalScalability>("video.encoder.avc");
    EXPECT_EQ(server_->ExchangeTemporalScalability(current), nullptr);
    EXPECT_NE(old, server_->GetTemporalScalability());
    auto buffer = AVBuffer::CreateAVBuffer();
    ASSERT_NE(buffer, nullptr);
    old->StoreAVBuffer(0, buffer);
    EXPECT_TRUE(current->inputBufferMap_.empty());
    current->StoreAVBuffer(1, buffer);
    EXPECT_EQ(current->inputBufferMap_.count(1), 1U);
}
} // namespace MediaAVCodec
} // namespace OHOS
