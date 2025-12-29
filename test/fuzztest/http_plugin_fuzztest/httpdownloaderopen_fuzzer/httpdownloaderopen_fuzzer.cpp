/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include <test_template.h>
#include <fuzzer/FuzzedDataProvider.h>
#include "http/http_media_downloader.h"
#include "http_server_mock.h"
#include <iostream>
using namespace OHOS;
using namespace OHOS::Media::Plugins::HttpPlugin;
using namespace OHOS::Media::Plugins;
#define FUZZ_PROJECT_NAME "httpdownloader_fuzzer"

static constexpr int32_t MAX_BUFFER_SIZE_FUZZ = 1024 * 1024 * 2;

bool TestHttpDownloaderOpenFuzz(const uint8_t *data, const size_t size)
{
    if (data == nullptr || size < sizeof(int64_t)) {
        return false;
    }
    const std::string url = "http://127.0.0.1:46666/dewu.mp4";
    std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<OHOS::Media::Plugins::HttpPlugin::HttpMediaDownloader>(url, 4, nullptr);  // 4
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(url, httpHeader);
    unsigned char buff[MAX_BUFFER_SIZE_FUZZ];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = GetData<int32_t>();
    readDataInfo.wantReadLength_ = GetData<unsigned int>();
    readDataInfo.wantReadLength_ %= MAX_BUFFER_SIZE_FUZZ;
    readDataInfo.isEos_ = GetData<bool>();
    bool blockingFlag = GetData<bool>();
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->SetReadBlockingFlag(blockingFlag);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    return true;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (!InitServer()) {
        std::cout << "Init server error" << std::endl;
        return -1;
    }
    g_baseFuzzData = data;
    g_baseFuzzSize = size;
    g_baseFuzzPos = 0;
    TestHttpDownloaderOpenFuzz(data, size);
    if (!CloseServer()) {
        std::cout << "Close server error" << std::endl;
        return -1;
    }
    return 0;
}