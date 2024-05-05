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

#ifndef HLS_MEDIA_DOWNLOADER_UINT_TEST_H
#define HLS_MEDIA_DOWNLOADER_UINT_TEST_H

#include "hls/hls_media_downloader.h"
#include "hls/hls_playlist_downloader.h"
#include "gtest/gtest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HlsMediaDownloaderUnitTest : public testing::Test {

public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
protected:
    HlsMediaDownloader* hlsMediaDownloader;
};
constexpr uint32_t RING_BUFFER_SIZE = 5 * 1024 * 1024;
constexpr uint32_t MAX_BUFFER_SIZE = 20 * 1024 * 1024;
}
}
}
}
#endif