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

#include "http_media_downloader_unit_test.h"

#define LOCAL true
namespace OHOS::Media::Plugins::HttpPlugin {

using namespace std;
using namespace testing::ext;
constexpr size_t RING_BUFFER_SIZE = 5 * 1024 * 1024;
constexpr size_t MAX_BUFFER_SIZE = 20 * 1024 * 1024;

void HttpMediaDownloaderUnitTest::SetUpTestCase(void)
{
}

void HttpMediaDownloaderUnitTest::TearDownTestCase(void)
{
}

void HttpMediaDownloaderUnitTest::SetUp()
{
}

void HttpMediaDownloaderUnitTest::TearDown()
{
}

HWTEST_F(HttpMediaDownloaderUnitTest, TestDefaultConstructor, TestSize.Level1)
{
    HttpMediaDownloader downloader;
    EXPECT_EQ(downloader.GetBufferSize(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TestDefaultConstructor_001, TestSize.Level1)
{
    HttpMediaDownloader downloader;
    EXPECT_EQ(downloader.GetBufferSize(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TestDefaultConstructorDefine, TestSize.Level1)
{
    HttpMediaDownloader downloader(10);
    EXPECT_EQ(downloader.GetBufferSize(), 2*RING_BUFFER_SIZE);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TestDefaultConstructorExceed, TestSize.Level1)
{
    HttpMediaDownloader downloader(10000);
    EXPECT_EQ(downloader.GetBufferSize(), MAX_BUFFER_SIZE);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TestDefaultConstructorLess, TestSize.Level1)
{
    HttpMediaDownloader downloader(1);
    EXPECT_EQ(downloader.GetBufferSize(), RING_BUFFER_SIZE);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TestOpenWithValidUrl, TestSize.Level1)
{
    HttpMediaDownloader downloader;
    std::map<std::string, std::string> header = {{"a", "b"}};
    Source* source = new Source();
    downloader.SetCallback(source);
    bool result = downloader.Open("http://127.0.0.0:4666/dewu.mp4", header);
    EXPECT_FALSE(result);
}

}