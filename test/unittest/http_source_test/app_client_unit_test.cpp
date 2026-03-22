/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <chrono>
#include <condition_variable>
#include <thread>
#include <mutex>

#include "download/app_client.h"
#include "download/network_client/http_curl_client.h"
#include "gtest/gtest.h"
#include "plugin/plugin_buffer.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

namespace {
constexpr int64_t DEFAULT_UUID = 123456789;

class SourceLoader : public IMediaSourceLoader {
public:
    ~SourceLoader() override {};

    int32_t Init(std::shared_ptr<IMediaSourceLoadingRequest> &request) override
    {
        (void)request;
        return 0;
    }

    int64_t Open(const std::string &url, const std::map<std::string, std::string> &header) override
    {
        (void)url;
        (void)header;
        return DEFAULT_UUID;
    }

    int32_t Read(int64_t uuid, int64_t requestedOffset, int64_t requestedLength) override
    {
        (void)uuid;
        (void)requestedOffset;
        (void)requestedLength;
        std::unique_lock lk(mutex_);
        isWaitRespond_ = true;
        return static_cast<int32_t>(requestedLength);
    }

    int32_t Close(int64_t uuid) override
    {
        (void)uuid;
        return 0;
    }

    std::mutex mutex_;
    std::condition_variable cond_;
    bool isWaitRespond_ {false};
};

struct RequestDataInfo {
    const long startPos;
    const int len;
    const RequestInfo requestInfo;
    const HandleResponseCbFunc completeCb;
};
}

class AppClientUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp(void);
    void TearDown(void);
protected:
    static size_t RxBodyData(void* buffer, size_t size, size_t nitems, void* userParam)
    {
        (void)buffer;
        (void)userParam;
        return size * nitems;
    }

    static size_t RxHeaderData(void* buffer, size_t size, size_t nitems, void* userParam)
    {
        (void)buffer;
        (void)userParam;
        return size * nitems;
    }

    static void ThreadFinishLoading(std::shared_ptr<NetworkClient> client, int64_t uuid, LoadingRequestError error)
    {
        client->FinishLoading(uuid, error);
    }

    static void ThreadRequestData(std::shared_ptr<NetworkClient> client, int64_t uuid, LoadingRequestError error,
        const RequestDataInfo& info)
    {
        client->SetUuid(uuid);
        std::thread requestDataThread([info, &client] {
            client->RequestData(info.startPos, info.len, info.requestInfo, info.completeCb);
        });

        if (requestDataThread.joinable()) {
            requestDataThread.detach();
        }

        {
            auto loader =
                std::static_pointer_cast<SourceLoader>((std::static_pointer_cast<AppClient>(client))->sourceLoader_);
            std::unique_lock lk(loader->mutex_);
            loader->cond_.wait_for(lk, std::chrono::seconds(1), [loader] { return loader->isWaitRespond_; });
            EXPECT_TRUE(loader->isWaitRespond_);
            loader->isWaitRespond_ = false;
        }

        std::thread finishLoadingThread(ThreadFinishLoading, client, uuid, error);

        if (finishLoadingThread.joinable()) {
            finishLoadingThread.join();
        }
    }

    std::shared_ptr<AppClient> appClient_;
};

void AppClientUnitTest::SetUp(void)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    appClient_ = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient_->SetLoader(mockLoader);
}

void AppClientUnitTest::TearDown(void)
{
    appClient_.reset();
}

HWTEST_F(AppClientUnitTest, AppClient_Construct, TestSize.Level0)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    auto appClient = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient->SetLoader(mockLoader);
    EXPECT_NE(appClient->sourceLoader_, nullptr);
}

HWTEST_F(AppClientUnitTest, AppClient_Init, TestSize.Level1)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    auto appClient = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient->SetLoader(mockLoader);
    Status status = appClient->Init();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(AppClientUnitTest, AppClient_Open, TestSize.Level1)
{
    std::string url = "http://example.com/test.mp4";
    std::map<std::string, std::string> httpHeader;
    int32_t timeoutMs = 5000;

    Status status = appClient_->Open(url, httpHeader, timeoutMs);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(AppClientUnitTest, AppClient_Open_EmptyUrl, TestSize.Level1)
{
    std::string url = "";
    std::map<std::string, std::string> httpHeader;
    int32_t timeoutMs = 5000;

    Status status = appClient_->Open(url, httpHeader, timeoutMs);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(AppClientUnitTest, AppClient_Open_WithHeaders, TestSize.Level1)
{
    std::string url = "http://example.com/test.mp4";
    std::map<std::string, std::string> httpHeader;
    httpHeader["Content-Type"] = "video/mp4";
    httpHeader["User-Agent"] = "TestClient";
    int32_t timeoutMs = 10000;

    Status status = appClient_->Open(url, httpHeader, timeoutMs);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(AppClientUnitTest, AppClient_Close, TestSize.Level1)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    auto appClient = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient->SetLoader(mockLoader);
    Status status = appClient->Close(true);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(AppClientUnitTest, AppClient_Close_Sync, TestSize.Level1)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    auto appClient = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient->SetLoader(mockLoader);
    Status status = appClient->Close(false);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(AppClientUnitTest, AppClient_Deinit, TestSize.Level1)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    auto appClient = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient->SetLoader(mockLoader);
    Status status = appClient->Deinit();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(AppClientUnitTest, AppClient_GetIp, TestSize.Level1)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    auto appClient = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient->SetLoader(mockLoader);
    std::string ip;
    Status status = appClient->GetIp(ip);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(AppClientUnitTest, AppClient_SetLoader, TestSize.Level1)
{
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient_->SetLoader(mockLoader);
    int32_t appUid = 0;
    appClient_->SetAppUid(appUid);
    EXPECT_NE(appClient_->sourceLoader_, nullptr);
}

HWTEST_F(AppClientUnitTest, AppClient_SetLoader_Null, TestSize.Level1)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    auto appClient = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    std::shared_ptr<IMediaSourceLoader> nullLoader = nullptr;
    appClient->SetLoader(nullLoader);
    EXPECT_EQ(appClient->sourceLoader_, nullptr);
}

HWTEST_F(AppClientUnitTest, AppClient_SetUuid, TestSize.Level1)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    auto appClient = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient->SetLoader(mockLoader);
    int64_t uuid = DEFAULT_UUID;
    appClient->SetUuid(uuid);
    EXPECT_EQ(appClient->uuid_, uuid);
}

HWTEST_F(AppClientUnitTest, AppClient_SetUuid_Zero, TestSize.Level1)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    auto appClient = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient->SetLoader(mockLoader);
    int64_t uuid = 0;
    appClient->SetUuid(uuid);
    EXPECT_EQ(appClient->uuid_, 0);
}

HWTEST_F(AppClientUnitTest, AppClient_SetUuid_Negative, TestSize.Level1)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    auto appClient = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient->SetLoader(mockLoader);
    int64_t uuid = -1;
    appClient->SetUuid(uuid);
    EXPECT_EQ(appClient->uuid_, 0);
}

HWTEST_F(AppClientUnitTest, AppClient_GetRedirectUrl, TestSize.Level1)
{
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;
    auto appClient = std::make_shared<AppClient>(headerCallback, bodyCallback, nullptr);
    auto mockLoader = std::make_shared<SourceLoader>();
    appClient->SetLoader(mockLoader);
    std::string redirectUrl = appClient->GetRedirectUrl();
    EXPECT_TRUE(redirectUrl.empty());
}

HWTEST_F(AppClientUnitTest, AppClient_RespondHeader, TestSize.Level1)
{
    int64_t uuid = DEFAULT_UUID;
    std::map<std::string, std::string> httpHeader;
    httpHeader["Content-Type"] = "video/mp4";
    httpHeader["Content-Length"] = "1024";
    std::string redirectUrl = "";

    int32_t result = appClient_->RespondHeader(uuid, httpHeader, redirectUrl);
    EXPECT_EQ(result, 0);
}

HWTEST_F(AppClientUnitTest, AppClient_RespondHeader_WithRedirect, TestSize.Level1)
{
    int64_t uuid = DEFAULT_UUID;
    std::map<std::string, std::string> httpHeader;
    httpHeader["Content-Type"] = "video/mp4";
    std::string redirectUrl = "http://example.com/redirect.mp4";

    int32_t result = appClient_->RespondHeader(uuid, httpHeader, redirectUrl);
    EXPECT_EQ(result, 0);
}

HWTEST_F(AppClientUnitTest, AppClient_RespondHeader_EmptyHeaders, TestSize.Level1)
{
    int64_t uuid = DEFAULT_UUID;
    std::map<std::string, std::string> httpHeader;
    std::string redirectUrl = "";

    int32_t result = appClient_->RespondHeader(uuid, httpHeader, redirectUrl);
    EXPECT_EQ(result, 0);
}

HWTEST_F(AppClientUnitTest, AppClient_RespondHeader_LargeHeaders, TestSize.Level1)
{
    int64_t uuid = DEFAULT_UUID;
    std::map<std::string, std::string> httpHeader;
    for (int i = 0; i < 150; i++) {
        httpHeader["Header" + std::to_string(i)] = "Value" + std::to_string(i);
    }
    std::string redirectUrl = "";

    int32_t result = appClient_->RespondHeader(uuid, httpHeader, redirectUrl);
    EXPECT_EQ(result, 0);
}

HWTEST_F(AppClientUnitTest, AppClient_RespondData_NullMemory, TestSize.Level1)
{
    int64_t uuid = DEFAULT_UUID;
    int64_t offset = 0;
    std::shared_ptr<AVSharedMemory> memory = nullptr;

    int32_t result = appClient_->RespondData(uuid, offset, memory);
    EXPECT_EQ(result, 0);
}

HWTEST_F(AppClientUnitTest, AppClient_FinishLoading, TestSize.Level1)
{
    int64_t uuid = DEFAULT_UUID;
    LoadingRequestError state = LoadingRequestError::LOADING_ERROR_SUCCESS;

    std::thread finishLoadingThread([uuid, state, this] {
        appClient_->FinishLoading(uuid, state);
    });

    if (finishLoadingThread.joinable()) {
        finishLoadingThread.join();
    }
}

HWTEST_F(AppClientUnitTest, AppClient_FinishLoading_ErrorState, TestSize.Level1)
{
    int64_t uuid = DEFAULT_UUID;
    LoadingRequestError state = LoadingRequestError::LOADING_ERROR_NOT_READY;

    std::thread finishLoadingThread([uuid, state, this] {
        appClient_->FinishLoading(uuid, state);
    });

    if (finishLoadingThread.joinable()) {
        finishLoadingThread.join();
    }
}

HWTEST_F(AppClientUnitTest, AppClient_FinishLoading_Timeout, TestSize.Level1)
{
    int64_t uuid = DEFAULT_UUID;
    LoadingRequestError state = LoadingRequestError::LOADING_ERROR_NOT_READY;

    std::thread finishLoadingThread([uuid, state, this] {
        appClient_->FinishLoading(uuid, state);
    });

    if (finishLoadingThread.joinable()) {
        finishLoadingThread.join();
    }
}

HWTEST_F(AppClientUnitTest, AppClient_RequestData_StartPosMinusOne, TestSize.Level1)
{
    long startPos = -1;
    int len = 1024;
    RequestInfo requestInfo;
    requestInfo.url = "http://test.com/video.mp4";
    requestInfo.httpHeader = {};

    std::mutex m;
    std::condition_variable cv;
    bool callbackInvoked = false;
    auto completedCb = [&callbackInvoked, &m, &cv](int32_t clientCode, int32_t serverCode, Status status) {
        (void)clientCode;
        (void)serverCode;
        (void)status;
        std::unique_lock<std::mutex> lk(m);
        callbackInvoked = true;
        cv.notify_all();
    };

    RequestDataInfo info = {.startPos = startPos, .len = len, .requestInfo = requestInfo, .completeCb = completedCb};
    ThreadRequestData(appClient_, DEFAULT_UUID, LoadingRequestError::LOADING_ERROR_SUCCESS, info);

    {
        std::unique_lock lk(m);
        cv.wait_for(lk, std::chrono::seconds(1), [&callbackInvoked] { return callbackInvoked; });
    }

    EXPECT_TRUE(callbackInvoked);
}

HWTEST_F(AppClientUnitTest, AppClient_RequestData_ZeroLength, TestSize.Level1)
{
    long startPos = 0;
    int len = 0;
    RequestInfo requestInfo;
    requestInfo.url = "http://test.com/video.mp4";
    requestInfo.httpHeader = {};

    std::mutex m;
    std::condition_variable cv;
    bool callbackInvoked = false;
    auto completedCb = [&callbackInvoked, &m, &cv](int32_t clientCode, int32_t serverCode, Status status) {
        (void)clientCode;
        (void)serverCode;
        (void)status;
        std::unique_lock<std::mutex> lk(m);
        callbackInvoked = true;
        cv.notify_all();
    };

    RequestDataInfo info = {.startPos = startPos, .len = len, .requestInfo = requestInfo, .completeCb = completedCb};
    ThreadRequestData(appClient_, DEFAULT_UUID, LoadingRequestError::LOADING_ERROR_SUCCESS, info);

    {
        std::unique_lock lk(m);
        cv.wait_for(lk, std::chrono::seconds(1), [&callbackInvoked] { return callbackInvoked; });
    }

    EXPECT_TRUE(callbackInvoked);
}

HWTEST_F(AppClientUnitTest, AppClient_RequestData_NegativeLength, TestSize.Level1)
{
    long startPos = 0;
    int len = -1;
    RequestInfo requestInfo;
    requestInfo.url = "http://test.com/video.mp4";
    requestInfo.httpHeader = {};

    std::mutex m;
    std::condition_variable cv;
    bool callbackInvoked = false;
    auto completedCb = [&callbackInvoked, &m, &cv](int32_t clientCode, int32_t serverCode, Status status) {
        (void)clientCode;
        (void)serverCode;
        (void)status;
        std::unique_lock<std::mutex> lk(m);
        callbackInvoked = true;
        cv.notify_all();
    };

    RequestDataInfo info = {.startPos = startPos, .len = len, .requestInfo = requestInfo, .completeCb = completedCb};
    ThreadRequestData(appClient_, DEFAULT_UUID, LoadingRequestError::LOADING_ERROR_SUCCESS, info);

    {
        std::unique_lock lk(m);
        cv.wait_for(lk, std::chrono::seconds(1), [&callbackInvoked] { return callbackInvoked; });
    }

    EXPECT_TRUE(callbackInvoked);
}

HWTEST_F(AppClientUnitTest, AppClient_RequestData_LargeLength, TestSize.Level1)
{
    long startPos = 0;
    int len = 10 * 1024 * 1024;
    RequestInfo requestInfo;
    requestInfo.url = "http://test.com/video.mp4";
    requestInfo.httpHeader = {};

    std::mutex m;
    std::condition_variable cv;
    bool callbackInvoked = false;
    auto completedCb = [&callbackInvoked, &m, &cv](int32_t clientCode, int32_t serverCode, Status status) {
        (void)clientCode;
        (void)serverCode;
        (void)status;
        std::unique_lock<std::mutex> lk(m);
        callbackInvoked = true;
        cv.notify_all();
    };

    RequestDataInfo info = {.startPos = startPos, .len = len, .requestInfo = requestInfo, .completeCb = completedCb};
    ThreadRequestData(appClient_, DEFAULT_UUID, LoadingRequestError::LOADING_ERROR_SUCCESS, info);

    {
        std::unique_lock lk(m);
        cv.wait_for(lk, std::chrono::seconds(1), [&callbackInvoked] { return callbackInvoked; });
    }

    EXPECT_TRUE(callbackInvoked);
}

HWTEST_F(AppClientUnitTest, AppClient_RequestData_NegativeStartPos, TestSize.Level1)
{
    long startPos = -100;
    int len = 1024;
    RequestInfo requestInfo;
    requestInfo.url = "http://test.com/video.mp4";
    requestInfo.httpHeader = {};

    std::mutex m;
    std::condition_variable cv;
    bool callbackInvoked = false;
    auto completedCb = [&callbackInvoked, &m, &cv](int32_t clientCode, int32_t serverCode, Status status) {
        (void)clientCode;
        (void)serverCode;
        (void)status;
        std::unique_lock<std::mutex> lk(m);
        callbackInvoked = true;
        cv.notify_all();
    };

    RequestDataInfo info = {.startPos = startPos, .len = len, .requestInfo = requestInfo, .completeCb = completedCb};
    ThreadRequestData(appClient_, DEFAULT_UUID, LoadingRequestError::LOADING_ERROR_SUCCESS, info);

    {
        std::unique_lock lk(m);
        cv.wait_for(lk, std::chrono::seconds(1), [&callbackInvoked] { return callbackInvoked; });
    }

    EXPECT_TRUE(callbackInvoked);
}

HWTEST_F(AppClientUnitTest, AppClient_RequestData_LargeStartPos, TestSize.Level1)
{
    long startPos = 1024 * 1024 * 1024;
    int len = 1024;
    RequestInfo requestInfo;
    requestInfo.url = "http://test.com/video.mp4";
    requestInfo.httpHeader = {};

    std::mutex m;
    std::condition_variable cv;
    bool callbackInvoked = false;
    auto completedCb = [&callbackInvoked, &m, &cv](int32_t clientCode, int32_t serverCode, Status status) {
        (void)clientCode;
        (void)serverCode;
        (void)status;
        std::unique_lock<std::mutex> lk(m);
        callbackInvoked = true;
        cv.notify_all();
    };

    RequestDataInfo info = {.startPos = startPos, .len = len, .requestInfo = requestInfo, .completeCb = completedCb};
    ThreadRequestData(appClient_, DEFAULT_UUID, LoadingRequestError::LOADING_ERROR_SUCCESS, info);

    {
        std::unique_lock lk(m);
        cv.wait_for(lk, std::chrono::seconds(1), [&callbackInvoked] { return callbackInvoked; });
    }

    EXPECT_TRUE(callbackInvoked);
}

HWTEST_F(AppClientUnitTest, AppClient_Multiple_Sequential, TestSize.Level1)
{
    Status status1 = appClient_->Init();
    EXPECT_EQ(status1, Status::OK);

    std::string url = "http://example.com/test.mp4";
    std::map<std::string, std::string> httpHeader;
    Status status2 = appClient_->Open(url, httpHeader, 5000);
    EXPECT_EQ(status2, Status::OK);

    int64_t uuid = DEFAULT_UUID;
    appClient_->SetUuid(uuid);

    std::shared_ptr<IMediaSourceLoader> mockLoader = nullptr;
    appClient_->SetLoader(mockLoader);

    Status status3 = appClient_->Close(true);
    EXPECT_EQ(status3, Status::OK);

    Status status4 = appClient_->Deinit();
    EXPECT_EQ(status4, Status::OK);
}

HWTEST_F(AppClientUnitTest, AppClient_NetworkClient_GetAppInstance, TestSize.Level1)
{
    void *nullUserParam = nullptr;
    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;

    auto client = NetworkClient::GetAppInstance(headerCallback, bodyCallback, nullUserParam);
    EXPECT_NE(client, nullptr);
}

HWTEST_F(AppClientUnitTest, AppClient_NetworkClient_GetAppInstance_NullCallbacks, TestSize.Level1)
{
    void *nullUserParam = nullptr;
    RxHeader nullHeaderCallback = nullptr;
    RxBody nullBodyCallback = nullptr;

    auto client = NetworkClient::GetAppInstance(nullHeaderCallback, nullBodyCallback, nullUserParam);
    EXPECT_NE(client, nullptr);
}

HWTEST_F(AppClientUnitTest, AppClient_NetworkClient_GetAppInstance_WithUserParam, TestSize.Level1)
{
    int userParamValue = 42;

    RxHeader headerCallback = &RxHeaderData;
    RxBody bodyCallback = &RxBodyData;

    auto client = NetworkClient::GetAppInstance(headerCallback, bodyCallback, &userParamValue);
    EXPECT_NE(client, nullptr);
}
}
}
}
}