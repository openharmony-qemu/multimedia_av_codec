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

#ifndef HTTP_MEDIA_DOWNLOADER_UNIT_TEST_H
#define HTTP_MEDIA_DOWNLOADER_UNIT_TEST_H

#include "http/http_media_downloader.h"
#include "download/media_source_loading_request.h"
#include "gtest/gtest.h"
#include "hls/m3u8.h"
#include "source/source.h"
#include <gmock/gmock.h>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HttpMediaDownloaderUnitTest : public testing::Test {
public:

    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};
class MockDownloadRequest : public DownloadRequest {
public:
    MockDownloadRequest(const std::string& url, DataSaveFunc saveData, StatusCallbackFunc statusCallback,
                        bool requestWholeFile = false)
        : DownloadRequest(url, saveData, statusCallback, requestWholeFile)
    {
        (void)url;
        (void)saveData;
        (void)statusCallback;
        (void)requestWholeFile;
    }
    MockDownloadRequest(DataSaveFunc saveData, StatusCallbackFunc statusCallback, RequestInfo requestInfo,
                        bool requestWholeFile = false)
        : DownloadRequest(saveData, statusCallback, requestInfo, requestWholeFile)
    {
        (void)saveData;
        (void)statusCallback;
        (void)requestInfo;
        (void)requestWholeFile;
    }
    MOCK_METHOD(size_t, GetFileContentLength, (), (const));
    MOCK_METHOD(bool, IsEos, (), (const));
    MOCK_METHOD(bool, IsChunkedVod, (), (const));
    MOCK_METHOD(bool, IsServerAcceptRange, (), (const));
    MOCK_METHOD(Seekable, GetSeekable, (), (const));
    MOCK_METHOD(bool, IsChunked, (bool), (const));
    std::atomic<bool> isHeaderUpdated_ {false};
    std::atomic<bool> haveRedirectRetry_ {false};
};

class MockSourceLoader : public MediaSourceLoaderCombinations {
public:
    MockSourceLoader() : MediaSourceLoaderCombinations(nullptr) {}
    MOCK_METHOD(bool, GetenableOfflineCache, (), ());
};
}
}
}
}

#endif