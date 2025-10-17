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

using namespace OHOS;
using namespace OHOS::Media::Plugins::HttpPlugin;
using namespace OHOS::Media::Plugins;
#define FUZZ_PROJECT_NAME "httpdownloader_fuzzer"

static constexpr int32_t MAX_BUFFER_SIZE_FUZZ = 1024 * 1024 * 2;
static uint8_t g_buffer[MAX_BUFFER_SIZE_FUZZ];

void TestHttpDownloaderFuzz(FuzzedDataProvider &fdp)
{
    static const std::string TEST_URI_PATH = "http://127.0.0.1:46666/xxx.mp4";
    static const std::map<std::string, std::string> httpHeader = {
        {"User-Agent", "ABC"},
        {"Referer", "DEF"},
    };
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = GetData<int32_t>();
    readDataInfo.wantReadLength_ = GetData<uint32_t>();
    readDataInfo.wantReadLength_ %= MAX_BUFFER_SIZE_FUZZ;
    readDataInfo.isEos_ = GetData<bool>();
    std::map<std::string, std::string> header = std::map<std::string, std::string>();
    std::string fuzzString = fdp.ConsumeRandomLengthString();
    std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::HttpMediaDownloader> downloader =
        std::make_shared<OHOS::Media::Plugins::HttpPlugin::HttpMediaDownloader>(TEST_URI_PATH, 5, nullptr);  // 5
    std::string testUrl = TEST_URI_PATH + fuzzString;
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 640;  // 640
    playStrategy->height = 480;  // 480
    downloader->Open(TEST_URI_PATH, httpHeader);
    downloader->SetPlayStrategy(playStrategy);
    downloader->Read(g_buffer, readDataInfo);
    uint64_t extraCacheDuration = GetData<uint64_t>();
    downloader->SetExtraCache(extraCacheDuration);
    int32_t initOffset = GetData<int32_t>();
    int32_t initSize = GetData<int32_t>();
    downloader->SetInitialBufferSize(initOffset, initSize);
    int64_t seekOffset = GetData<int64_t>();
    bool isSeekHit = GetData<bool>();
    downloader->SeekToPos(seekOffset, isSeekHit);
    uint32_t bitRate = GetData<uint32_t>();
    downloader->SelectBitRate(bitRate);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    g_baseFuzzData = data;
    g_baseFuzzSize = size;
    g_baseFuzzPos = 0;
    FuzzedDataProvider fdp(data, size);
    TestHttpDownloaderFuzz(fdp);
    return 0;
}