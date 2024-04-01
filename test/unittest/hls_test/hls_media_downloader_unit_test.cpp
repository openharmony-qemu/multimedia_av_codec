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
#include "hls_media_downloader_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace std;
using namespace testing::ext;

// 黑白球視頻地址
const std::string TEST_URI = "http://TEST.m3u8";

void HlsMediaDownloaderUnitTest::SetUpTestCase(void)
{
}

void HlsMediaDownloaderUnitTest::TearDownTestCase(void)
{
}

void HlsMediaDownloaderUnitTest ::SetUp(void) {}

void HlsMediaDownloaderUnitTest ::TearDown(void) {}

HWTEST_F(HlsMediaDownloaderUnitTest, SetBufferSizeTest_001, TestSize.Level1)
{
    int testDuration = 30;
    std::shared_ptr<HlsMediaDownloader> tmpDownloader = std::make_shared<HlsMediaDownloader>(testDuration);
    size_t expectBufferSize = 30 * 1024 * 1024;
    EXPECT_EQ(expectBufferSize, tmpDownloader->GetTotalBufferSize());
}

HWTEST_F(HlsMediaDownloaderUnitTest, SetBufferSizeTest_002, TestSize.Level1)
{
    int testDuration = 10;
    std::shared_ptr<HlsMediaDownloader> tmpDownloader = std::make_shared<HlsMediaDownloader>(testDuration);
    size_t expectBufferSize = 10 * 1024 * 1024;
    EXPECT_EQ(expectBufferSize, tmpDownloader->GetTotalBufferSize());
}
}