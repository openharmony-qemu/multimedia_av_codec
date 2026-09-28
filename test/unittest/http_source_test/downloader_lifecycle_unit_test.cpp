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
#include "gtest/gtest.h"
#include "download/network_client/http_curl_client.h"
#include "sniffer_test_support.h"

using namespace testing::ext;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace SnifferTest;
namespace {
class SnapshotClient : public NetworkClient {
public:
    explicit SnapshotClient(std::string identity) : identity_(std::move(identity)) {}
    Status Init() override { return Status::OK; }
    Status Open(const std::string&, const std::map<std::string, std::string>&, int32_t) override { return Status::OK; }
    Status RequestData(long, int, const RequestInfo&, HandleResponseCbFunc) override { return Status::OK; }
    Status Close(bool) override { return Status::OK; }
    Status Deinit() override { return Status::OK; }
    void SetAppUid(int32_t) override {} // Snapshot-only dependency has no application identity.
    Status GetIp(std::string& ip) override
    {
        ip = identity_;
        return Status::OK;
    }
private:
    std::string identity_;
};
} // namespace
class DownloaderLifecycleUnitTest : public testing::Test {};

HWTEST_F(DownloaderLifecycleUnitTest, ClientReadersAndWriterKeepValidSnapshots_001, TestSize.Level2)
{
    auto downloader = std::make_shared<Downloader>("snapshot-concurrency");
    auto first = std::make_shared<SnapshotClient>("first");
    downloader->SetClient(first);
    Signal captured;
    Signal release;
    auto reader = std::async(std::launch::async, [&] {
        auto snapshot = downloader->GetClient();
        captured.Set();
        release.Wait();
        std::string identity;
        return snapshot->GetIp(identity) == Status::OK ? identity : std::string();
    });
    bool entered = captured.Wait();
    first.reset();
    auto replacement = std::make_shared<SnapshotClient>("replacement");
    downloader->SetClient(replacement);
    release.Set();
    EXPECT_TRUE(entered);
    EXPECT_EQ(reader.get(), "first");
    std::string identity;
    EXPECT_EQ(downloader->GetClient()->GetIp(identity), Status::OK);
    EXPECT_EQ(identity, "replacement");
}

HWTEST_F(DownloaderLifecycleUnitTest, ReplacedClientCanReenterGetterFromDestructor_002, TestSize.Level2)
{
    auto downloader = std::make_shared<Downloader>("destructor-query");
    auto replacement = std::make_shared<SnapshotClient>("new");
    std::weak_ptr<Downloader> weak = downloader;
    auto state = std::make_shared<ReleaseObservation>();
    auto old = std::shared_ptr<NetworkClient>(new SnapshotClient("old"), [state, weak, replacement](NetworkClient* p) {
        ObserveRelease(state, [weak, replacement] {
            auto owner = weak.lock();
            return owner != nullptr && owner->GetClient() == replacement;
        });
        delete p;
    });
    downloader->SetClient(std::move(old));
    downloader->SetClient(replacement);
    EXPECT_TRUE(state->queried.Wait());
    EXPECT_TRUE(state->replacementVisible.load());
    EXPECT_TRUE(state->outsideLock.load());
}

HWTEST_F(DownloaderLifecycleUnitTest, CurlCloseReopenAndDeinitProgressContract_003, TestSize.Level2)
{
    auto receive = [](void*, size_t size, size_t count, void*) { return size * count; };
    auto client = std::make_shared<HttpCurlClient>(receive, receive, nullptr);
    constexpr int32_t TIMEOUT_MS = 1000;
    const std::string url = "http://127.0.0.1:1/not-requested";
    ASSERT_EQ(client->Init(), Status::OK);
    ASSERT_EQ(client->Open(url, {}, TIMEOUT_MS), Status::OK);
    EXPECT_EQ(client->TransferProgress(client.get(), 0, 0, 0, 0), 0);
    EXPECT_EQ(client->Close(true), Status::OK);
    EXPECT_EQ(client->TransferProgress(client.get(), 0, 0, 0, 0), 1);
    bool completed = false;
    RequestInfo request;
    EXPECT_EQ(client->RequestData(0, 0, request,
        [&](int32_t, int32_t, const std::string&, Status) { completed = true; }), Status::ERROR_INVALID_OPERATION);
    EXPECT_FALSE(completed);
    ASSERT_EQ(client->Open(url, {}, TIMEOUT_MS), Status::OK);
    EXPECT_EQ(client->TransferProgress(client.get(), 0, 0, 0, 0), 0);
    EXPECT_EQ(client->Deinit(), Status::OK);
    EXPECT_EQ(client->Open(url, {}, TIMEOUT_MS), Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(client->TransferProgress(client.get(), 0, 0, 0, 0), 1);
    EXPECT_EQ(client->Init(), Status::OK);
    EXPECT_EQ(client->TransferProgress(client.get(), 0, 0, 0, 0), 0);
    EXPECT_EQ(client->Deinit(), Status::OK);
}
} // namespace OHOS::Media::Plugins::HttpPlugin
