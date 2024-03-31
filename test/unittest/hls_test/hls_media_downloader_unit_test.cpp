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
    std::shared_ptr<HlsMediaDownloader> tmpDownloader = std::make_shared<HlsMediaDownloader>(30);
    size_t expectBufferSize = 30 * 1024 * 1024;
    EXPECT_EQ(expectBufferSize, tmpDownloader->GetTotalBufferSize());
}

HWTEST_F(HlsMediaDownloaderUnitTest, SetBufferSizeTest_002, TestSize.Level1)
{
    int testDuration = 10;
    std::shared_ptr<HlsMediaDownloader> tmpDownloader = std::make_shared<HlsMediaDownloader>(30);
    size_t expectBufferSize = 10 * 1024 * 1024;
    EXPECT_EQ(expectBufferSize, tmpDownloader->GetTotalBufferSize());
}

void saveDataThread(std::shared_ptr<HlsMediaDownloader> tmpDownloader) {
    int32_t dataSize = 40960;
    uint8_t data[dataSize];
     for (int i=0; i<10000; i++){
        tmpDownloader->SaveData(data, dataSize);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void readThread(std::shared_ptr<HlsMediaDownloader> tmpDownloader) {
    unsigned int realReadLength = 0;
    uint32_t dataSize = 40960;
    unsigned char buff[dataSize];
    bool isEos = false;
    int wantReadLength = tmpDownloader->GetRingBufferSize();
    for (int i=0; i<10000; i++){
        tmpDownloader->Read(buff, wantReadLength, realReadLength, isEos);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

HWTEST_F(HlsMediaDownloaderUnitTest, BufferRiseDownTest_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> tmpDownloader = std::make_shared<HlsMediaDownloader>(30);
    // 创建测试数据
    const uint32_t dataSize = 1024;
    uint8_t data[dataSize];
    unsigned char buffer[dataSize];

    // 创建线程
    std::thread thread1(saveDataThread, tmpDownloader);
    std::thread thread2(readThread, tmpDownloader);
    // 等待线程完成
    thread1.join();
    thread2.join();
    size_t expectBufferSize = 1 * 1024 * 1024;
    size_t currentBufferSize = tmpDownloader->GetTotalBufferSize();
    EXPECT_GE(currentBufferSize, expectBufferSize);
}

}