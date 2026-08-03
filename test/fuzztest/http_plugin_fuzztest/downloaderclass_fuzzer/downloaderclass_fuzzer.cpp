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

#include <cstdint>
#include <iostream>
#include <cstring>
#include <fuzzer/FuzzedDataProvider.h>
#include "download/downloader.h"
#include "mock/media_source_loading_request.h"

using namespace OHOS;
using namespace OHOS::Media::Plugins::HttpPlugin;
using namespace OHOS::Media::Plugins;
#define FUZZ_PROJECT_NAME "downloaderclass_fuzzer"

static constexpr int32_t MAX_STRING_LEN = 256;

// FUZZ_COVER: Downloader构造函数 - name参数
void TestDownloaderConstructName(FuzzedDataProvider &fdp)
{
    std::string name = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    auto downloader = std::make_shared<Downloader>(name);
    // FUZZ_COVER: Init初始化
    downloader->Init();
    std::cout << "Downloader name: " << name << std::endl;
}

// FUZZ_COVER: Downloader构造函数 - name + sourceLoader参数
void TestDownloaderConstructSourceLoader(FuzzedDataProvider &fdp)
{
    std::string name = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    auto sourceLoader = std::make_shared<MediaSourceLoaderCombinations>(nullptr);
    auto downloader = std::make_shared<Downloader>(name, sourceLoader);
    // FUZZ_COVER: Init初始化
    downloader->Init();
    std::cout << "Downloader with sourceLoader" << std::endl;
}

// FUZZ_COVER: SetInterruptState设置中断状态
void TestSetInterruptState(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    // FUZZ_COVER: SetInterruptState设置中断
    bool isInterruptNeeded = fdp.ConsumeBool();
    downloader->SetInterruptState(isInterruptNeeded);
    std::cout << "SetInterruptState: " << isInterruptNeeded << std::endl;
}

// FUZZ_COVER: SetAppState设置App后台状态
void TestSetAppState(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    // FUZZ_COVER: SetAppState设置后台状态
    bool isAppBackground = fdp.ConsumeBool();
    downloader->SetAppState(isAppBackground);
    std::cout << "SetAppState: " << isAppBackground << std::endl;
}

// FUZZ_COVER: SetAppUid设置App UID
void TestSetAppUid(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    // FUZZ_COVER: SetAppUid设置UID
    int32_t appUid = fdp.ConsumeIntegral<int32_t>();
    downloader->SetAppUid(appUid);
    std::cout << "SetAppUid: " << appUid << std::endl;
}

// FUZZ_COVER: SetRequestSize设置请求大小
void TestSetRequestSize(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    // FUZZ_COVER: SetRequestSize设置下载请求大小
    size_t requestSize = fdp.ConsumeIntegral<size_t>();
    downloader->SetRequestSize(requestSize);
    std::cout << "SetRequestSize: " << requestSize << std::endl;
}

// FUZZ_COVER: GetIp获取IP地址
void TestGetIp(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    // FUZZ_COVER: GetIp获取IP
    std::string ip;
    downloader->GetIp(ip);
    std::cout << "GetIp: " << ip << std::endl;
}

// FUZZ_COVER: GetCurrentRequest获取当前请求
void TestGetCurrentRequest(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    // FUZZ_COVER: GetCurrentRequest获取当前请求
    const auto& request = downloader->GetCurrentRequest();
    if (request != nullptr) {
        std::cout << "CurrentRequest exists" << std::endl;
    } else {
        std::cout << "CurrentRequest is null" << std::endl;
    }
}

// FUZZ_COVER: ReStart重新开始
void TestReStart(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    // FUZZ_COVER: ReStart重新开始下载循环
    downloader->ReStart();
    
    std::cout << "ReStart called" << std::endl;
}

// FUZZ_COVER: Pause / Resume暂停和恢复
void TestPauseResume(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    // FUZZ_COVER: Pause暂停
    bool isAsync = fdp.ConsumeBool();
    downloader->Pause(isAsync);
    
    std::cout << "Pause called, isAsync: " << isAsync << std::endl;
    
    // FUZZ_COVER: Resume恢复
    downloader->Resume();
    
    std::cout << "Resume called" << std::endl;
}

// FUZZ_COVER: Cancel取消下载
void TestCancel(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    // FUZZ_COVER: Cancel取消下载
    downloader->Cancel();
    
    std::cout << "Cancel called" << std::endl;
}

// FUZZ_COVER: Stop停止下载
void TestStop(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    // FUZZ_COVER: Stop停止下载
    bool isAsync = fdp.ConsumeBool();
    downloader->Stop(isAsync);
    
    std::cout << "Stop called, isAsync: " << isAsync << std::endl;
}

// FUZZ_COVER: GetContentType获取内容类型
void TestGetContentType(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    // FUZZ_COVER: GetContentType获取内容类型
    std::string contentType = downloader->GetContentType();
    
    std::cout << "ContentType: " << contentType << std::endl;
}

// FUZZ_COVER: StopBufferring停止缓冲
void TestStopBufferring(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    // FUZZ_COVER: StopBufferring停止缓冲
    downloader->StopBufferring();
    
    std::cout << "StopBufferring called" << std::endl;
}

// FUZZ_COVER: HandleContentRange静态函数测试
void TestHandleContentRange(FuzzedDataProvider &fdp)
{
    HeaderInfo info;
    memset(&info, 0, sizeof(HeaderInfo));
    
    // FUZZ_COVER: HandleContentRange解析Content-Range header
    size_t start = fdp.ConsumeIntegral<size_t>();
    size_t end = fdp.ConsumeIntegral<size_t>();
    size_t fileLen = fdp.ConsumeIntegral<size_t>();
    
    std::string headerLine = "bytes " + std::to_string(start) + "-" +
                             std::to_string(end) + "/" + std::to_string(fileLen);
    char buffer[256];
    memset(buffer, 0, 256);
    memcpy(buffer, headerLine.c_str(), headerLine.size());
    
    char* next = nullptr;
    char* key = strtok_s(buffer, ": ", &next);
    
    bool result = Downloader::HandleContentRange(&info, key, next, 1, 100);
    std::cout << "HandleContentRange result: " << result << std::endl;
}

// FUZZ_COVER: HandleContentType静态函数测试
void TestHandleContentType(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    HeaderInfo info;
    memset(&info, 0, sizeof(HeaderInfo));
    // FUZZ_COVER: HandleContentType解析Content-Type header
    std::string contentType = fdp.ConsumeRandomLengthString(32);
    std::string headerLine = contentType;
    char buffer[64];
    memset(buffer, 0, 64);
    memcpy(buffer, headerLine.c_str(), headerLine.size());
    char* next = nullptr;
    char* key = strtok_s(buffer, ": ", &next);
    if (key != nullptr) {
        Downloader::HandleContentType(&info, key, next, 100, downloader.get());
    }
}

// FUZZ_COVER: HandleContentLength静态函数测试
void TestHandleContentLength(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    HeaderInfo info;
    memset(&info, 0, sizeof(HeaderInfo));
    
    // FUZZ_COVER: HandleContentLength解析Content-Length header
    int32_t contentLen = fdp.ConsumeIntegral<int32_t>();
    std::string headerLine = std::to_string(contentLen);
    char buffer[32];
    memset(buffer, 0, 32);
    memcpy(buffer, headerLine.c_str(), headerLine.size());
    
    char* next = nullptr;
    char* key = strtok_s(buffer, ": ", &next);
    
    bool result = Downloader::HandleContentLength(&info, key, next, downloader.get());
    std::cout << "HandleContentLength result: " << result << ", contentLen: " << info.contentLen << std::endl;
}

// FUZZ_COVER: HandleRange静态函数测试
void TestHandleRange(FuzzedDataProvider &fdp)
{
    HeaderInfo info;
    memset(&info, 0, sizeof(HeaderInfo));
    
    // FUZZ_COVER: HandleRange解析Accept-Ranges header
    std::string rangeValue = fdp.ConsumeBool() ? "bytes" : "none";
    std::string headerLine = rangeValue;
    char buffer[32];
    memset(buffer, 0, 32);
    memcpy(buffer, headerLine.c_str(), headerLine.size());
    
    char* next = nullptr;
    char* key = strtok_s(buffer, ": ", &next);
    
    bool result = Downloader::HandleRange(&info, key, next, 1, 50);
    std::cout << "HandleRange result: " << result << ", isServerAcceptRange: " << info.isServerAcceptRange << std::endl;
}

// FUZZ_COVER: HandleContentEncode静态函数测试
void TestHandleContentEncode(FuzzedDataProvider &fdp)
{
    HeaderInfo info;
    memset(&info, 0, sizeof(HeaderInfo));
    
    // FUZZ_COVER: HandleContentEncode解析Content-Encode header
    std::string encodeType = fdp.ConsumeRandomLengthString(32);
    char buffer[64];
    memset(buffer, 0, 64);
    memcpy(buffer, encodeType.c_str(), encodeType.size());
    
    char* next = nullptr;
    char* key = strtok_s(buffer, ": ", &next);
    if (key != nullptr) {
        bool result = Downloader::HandleContentEncode(&info, key, next, 1, 50);
        std::cout << "HandleContentEncode result: " << result << std::endl;
    }
}

// FUZZ_COVER: ToLower静态函数测试
void TestToLower(FuzzedDataProvider &fdp)
{
    // FUZZ_COVER: ToLower转换为小写
    std::string input = fdp.ConsumeRandomLengthString(100);
    char buffer[128];
    memset(buffer, 0, 128);
    memcpy(buffer, input.c_str(), std::min(input.size(), (size_t)127));
    
    Downloader::ToLower(buffer);
    
    std::cout << "ToLower test completed" << std::endl;
}

// FUZZ_COVER: HandleFileContentLen静态函数测试
void TestHandleFileContentLen(FuzzedDataProvider &fdp)
{
    HeaderInfo header;
    memset(&header, 0, sizeof(HeaderInfo));
    
    // FUZZ_COVER: HandleFileContentLen处理fileContentLen为0的情况
    header.fileContentLen = 0;
    header.contentLen = fdp.ConsumeIntegral<long>();
    
    Downloader::HandleFileContentLen(&header);
    
    std::cout << "HandleFileContentLen: fileContentLen=" << header.fileContentLen << ", contentLen=" << header.contentLen << std::endl;
}

// FUZZ_COVER: RxHeaderData静态函数测试 - 空指针检查
void TestRxHeaderDataNullptr(FuzzedDataProvider &fdp)
{
    // FUZZ_COVER: RxHeaderData空指针检查
    char buffer[32] = "test header";
    size_t result = Downloader::RxHeaderData(buffer, 1, 32, nullptr);
    
    std::cout << "RxHeaderData nullptr result: " << result << std::endl;
}

// FUZZ_COVER: RxBodyData静态函数测试 - 空指针检查
void TestRxBodyDataNullptr(FuzzedDataProvider &fdp)
{
    // FUZZ_COVER: RxBodyData空指针检查
    uint8_t buffer[100] = {0};
    size_t result = Downloader::RxBodyData(buffer, 1, 100, nullptr);
    
    std::cout << "RxBodyData nullptr result: " << result << std::endl;
}

// FUZZ_COVER: IsDropDataRetryRequest静态函数测试
void TestIsDropDataRetryRequest(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    auto saveData = [](uint8_t* data, uint32_t len, bool notblock) { return len; };
    auto statusCallback = [](DownloadStatus status, std::shared_ptr<Downloader>& d,
                             std::shared_ptr<DownloadRequest>& r) {};
    
    downloader->currentRequest_ = std::make_shared<DownloadRequest>("http://127.0.0.1:46666/dewu.mp4", saveData, statusCallback);
    
    // FUZZ_COVER: IsDropDataRetryRequest判断是否需要丢弃数据重试
    downloader->currentRequest_->requestWholeFile_ = fdp.ConsumeBool();
    downloader->currentRequest_->shouldSaveData_ = fdp.ConsumeBool();
    downloader->currentRequest_->retryOnGoing_ = fdp.ConsumeBool();
    downloader->currentRequest_->startPos_ = fdp.ConsumeIntegral<int64_t>();
    
    bool result = Downloader::IsDropDataRetryRequest(downloader.get());
    std::cout << "IsDropDataRetryRequest: " << result << std::endl;
}

// FUZZ_COVER: UpdateHeaderInfo静态函数测试
void TestUpdateHeaderInfo(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    auto saveData = [](uint8_t* data, uint32_t len, bool notblock) { return len; };
    auto statusCallback = [](DownloadStatus status, std::shared_ptr<Downloader>& d,
                             std::shared_ptr<DownloadRequest>& r) {};
    
    downloader->currentRequest_ = std::make_shared<DownloadRequest>("http://127.0.0.1:46666/dewu.mp4", saveData, statusCallback);
    
    // FUZZ_COVER: UpdateHeaderInfo更新header信息
    downloader->currentRequest_->isHeaderUpdated_ = fdp.ConsumeBool();
    downloader->currentRequest_->headerInfo_.contentLen = fdp.ConsumeIntegral<long>();
    downloader->currentRequest_->headerInfo_.fileContentLen = fdp.ConsumeIntegral<size_t>();
    downloader->currentRequest_->SetRequestProtocolType(
        fdp.ConsumeBool() ? RequestProtocolType::HLS : RequestProtocolType::HTTP);
    
    Downloader::UpdateHeaderInfo(downloader.get());
    
    std::cout << "UpdateHeaderInfo test completed" << std::endl;
}

// FUZZ_COVER: DropRetryData静态函数测试
void TestDropRetryData(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    auto saveData = [](uint8_t* data, uint32_t len, bool notblock) { return len; };
    auto statusCallback = [](DownloadStatus status, std::shared_ptr<Downloader>& d,
                             std::shared_ptr<DownloadRequest>& r) {};
    
    downloader->currentRequest_ = std::make_shared<DownloadRequest>("http://127.0.0.1:46666/dewu.mp4", saveData, statusCallback);
    
    // FUZZ_COVER: DropRetryData丢弃数据
    downloader->currentRequest_->startPos_ = fdp.ConsumeIntegralInRange<int64_t>(100, 100000);
    downloader->currentRequest_->dropedDataLen_ = fdp.ConsumeIntegralInRange<int64_t>(0, 100);
    downloader->currentRequest_->retryOnGoing_ = true;
    
    size_t dataLen = fdp.ConsumeIntegralInRange<size_t>(100, 1000);
    std::vector<uint8_t> buffer(dataLen, 0);
    
    size_t result = Downloader::DropRetryData(buffer.data(), dataLen, downloader.get());
    std::cout << "DropRetryData result: " << result << std::endl;
}

// FUZZ_COVER: UpdateRequestSize静态函数测试
void TestUpdateRequestSize(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    auto saveData = [](uint8_t* data, uint32_t len, bool notblock) { return len; };
    auto statusCallback = [](DownloadStatus status, std::shared_ptr<Downloader>& d,
                             std::shared_ptr<DownloadRequest>& r) {};
    
    downloader->currentRequest_ = std::make_shared<DownloadRequest>("http://127.0.0.1:46666/dewu.mp4", saveData, statusCallback);
    
    // FUZZ_COVER: UpdateRequestSize更新请求大小
    downloader->currentRequest_->endPos_ = fdp.ConsumeIntegral<int64_t>();
    downloader->currentRequest_->startPos_ = fdp.ConsumeIntegral<int64_t>();
    if (downloader->currentRequest_->startPos_ > downloader->currentRequest_->endPos_) {
        downloader->currentRequest_->startPos_ = downloader->currentRequest_->endPos_;
    }
    downloader->currentRequest_->headerInfo_.fileContentLen = fdp.ConsumeIntegral<size_t>();
    downloader->currentRequest_->bitRateToRequestSize_ = fdp.ConsumeIntegral<int32_t>();
    Downloader::UpdateRequestSize(downloader.get());
    
    std::cout << "UpdateRequestSize test completed" << std::endl;
}

// FUZZ_COVER: Seek测试
void TestSeek(FuzzedDataProvider &fdp)
{
    auto downloader = std::make_shared<Downloader>("test");
    downloader->Init();
    
    auto saveData = [](uint8_t* data, uint32_t len, bool notblock) { return len; };
    auto statusCallback = [](DownloadStatus status, std::shared_ptr<Downloader>& d,
                             std::shared_ptr<DownloadRequest>& r) {};
    
    downloader->currentRequest_ = std::make_shared<DownloadRequest>("http://127.0.0.1:46666/dewu.mp4", saveData, statusCallback);
    
    HeaderInfo headerInfo;
    memset(&headerInfo, 0, sizeof(HeaderInfo));
    headerInfo.fileContentLen = fdp.ConsumeIntegral<size_t>();
    downloader->currentRequest_->SaveHeader(&headerInfo);
    downloader->currentRequest_->isHeaderUpdated_ = true;
    
    // FUZZ_COVER: Seek设置偏移量
    int64_t offset = fdp.ConsumeIntegral<int64_t>();
    bool result = downloader->Seek(offset);
    
    std::cout << "Seek offset: " << offset << ", result: " << result << std::endl;
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(uint32_t)) {
        return 0;
    }
    
    FuzzedDataProvider fdp(data, size);
    TestDownloaderConstructName(fdp);
    TestDownloaderConstructSourceLoader(fdp);
    TestSetInterruptState(fdp);
    TestSetAppState(fdp);
    TestSetAppUid(fdp);
    TestSetRequestSize(fdp);
    TestGetIp(fdp);
    TestGetCurrentRequest(fdp);
    TestReStart(fdp);
    TestPauseResume(fdp);
    TestCancel(fdp);
    TestStop(fdp);
    TestGetContentType(fdp);
    TestStopBufferring(fdp);
    TestHandleContentRange(fdp);
    TestHandleContentType(fdp);
    TestHandleContentLength(fdp);
    TestHandleRange(fdp);
    TestHandleContentEncode(fdp);
    TestToLower(fdp);
    TestHandleFileContentLen(fdp);
    TestRxHeaderDataNullptr(fdp);
    TestRxBodyDataNullptr(fdp);
    TestIsDropDataRetryRequest(fdp);
    TestUpdateHeaderInfo(fdp);
    TestDropRetryData(fdp);
    TestUpdateRequestSize(fdp);
    TestSeek(fdp);
    return 0;
}