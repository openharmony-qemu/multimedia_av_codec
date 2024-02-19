/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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


#ifndef HLS_PLAYLIST_DOWNLOADER_UINT_TEST_H
#define HLS_PLAYLIST_DOWNLOADER_UINT_TEST_H

#include "hls/hls_playlist_downloader.h"
#include "gmock/gmock.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HlsPlayListDownloaderUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
};
// Mock Downloader class
class MockDownloader : public Downloader {
public:
    explicit MockDownloader(const std::string &name) : Downloader(name) {}
    MOCK_METHOD(void, Download, (std::shared_ptr<DownloadRequest> & request, int timeout), (override));
    MOCK_METHOD(void, Resume, (), (override));
    MOCK_METHOD(void, Pause, (), (override));
    MOCK_METHOD(void, Stop, (), (override));
    MOCK_METHOD(void, Start, (), (override));
    MOCK_METHOD(void, Cancel, (), (override));
};

// Mock PlayListChangeCallback
class MockPlayListChangeCallback : public PlayListChangeCallback {
public:
    MOCK_METHOD(void, OnPlayListChanged, (const std::vector<PlayInfo> &playList), (override));
    MOCK_METHOD(void, OnSourceKeyChange, (const uint8_t *key, size_t keyLen, const uint8_t *iv), (override));
    MOCK_METHOD(void, OnDrmInfoChanged, (const std::multimap<std::string, std::vector<uint8_t>> &drmInfos), (override));
};
std::shared_ptr<HlsPlayListDownloader> playListDownloader = std::make_shared<HlsPlayListDownloader>();
}
}
}
}
#endif