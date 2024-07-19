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

#include "http_source_plugin.h"
#include "gtest/gtest.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

const uint32_t BUFFERING_SIZE_ = 1024;
const uint32_t WATERLINE_HIGH_ = 512;

class HttpSourcePluginUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
protected:
    std::shared_ptr<HttpSourcePlugin> httpSourcePlugin;
    std::shared_ptr<Meta> meta;
    std::shared_ptr<Buffer> buffer;
};

void HttpSourcePluginUnitTest::SetUpTestCase(void)
{
}

void HttpSourcePluginUnitTest::TearDownTestCase(void)
{
}

void HttpSourcePluginUnitTest::SetUp(void)
{
    meta = std::make_shared<Meta>();
    httpSourcePlugin = std::make_shared<HttpSourcePlugin>("test");
    buffer = std::make_shared<Buffer>();
}

void HttpSourcePluginUnitTest::TearDown(void)
{
    httpSourcePlugin.reset();
}

HWTEST_F(HttpSourcePluginUnitTest, Prepare_IsTrue, TestSize.Level1)
{
    httpSourcePlugin->delayReady = true;
    Status status = httpSourcePlugin->Prepare();
    EXPECT_EQ(status, Status::ERROR_DELAY_READY);
}

HWTEST_F(HttpSourcePluginUnitTest, Prepare_IsFalse, TestSize.Level1)
{
    httpSourcePlugin->delayReady = false;
    Status status = httpSourcePlugin->Prepare();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_Reset, TestSize.Level1)
{
    Status status = httpSourcePlugin->Reset();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetReadBlockingFlag_01, TestSize.Level1)
{
    bool isReadBlockingAllowed = true;
    Status status = httpSourcePlugin->SetReadBlockingFlag(isReadBlockingAllowed);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetReadBlockingFlag_02, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    bool isReadBlockingAllowed = true;
    Status status = httpSourcePlugin->SetReadBlockingFlag(isReadBlockingAllowed);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_Start, TestSize.Level1)
{
    Status status = httpSourcePlugin->Start();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_Stop, TestSize.Level1)
{
    Status status = httpSourcePlugin->Stop();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_GetParameter, TestSize.Level1)
{
    Status status = httpSourcePlugin->GetParameter(meta);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_SetParameter1, TestSize.Level1)
{
    meta->SetData(Tag::BUFFERING_SIZE, BUFFERING_SIZE_);
    meta->SetData(Tag::WATERLINE_HIGH, WATERLINE_HIGH_);
    Status status = httpSourcePlugin->SetParameter(meta);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_SetParameter2, TestSize.Level1)
{
    meta->SetData(Tag::WATERLINE_HIGH, WATERLINE_HIGH_);
    Status status = httpSourcePlugin->SetParameter(meta);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_SetParameter3, TestSize.Level1)
{
    meta->SetData(Tag::BUFFERING_SIZE, BUFFERING_SIZE_);
    Status status = httpSourcePlugin->SetParameter(meta);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_SetCallback, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->SetCallback(httpSourcePlugin->callback_);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource, TestSize.Level1)
{
    httpSourcePlugin->SetDownloaderBySource(nullptr);
    EXPECT_EQ(httpSourcePlugin->downloader_, nullptr);
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported1, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "video/mp4";
    httpSourcePlugin->uri_ = "http://example.com/video.m3u8";
    EXPECT_TRUE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported2, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "video/mp4";
    httpSourcePlugin->uri_ = "http://example.com/video.mpd";
    EXPECT_TRUE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported3, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "video/mp4";
    httpSourcePlugin->uri_ = "http://example.com/video.mp4";
    EXPECT_FALSE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported4, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "application/x-mpegURL";
    httpSourcePlugin->uri_ = "http://example.com/video.mp4";
    EXPECT_FALSE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, Read_IsNull, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->Read(1, buffer, 0, 10);
    EXPECT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, SetInterruptState1, TestSize.Level1)
{
    bool isInterruptNeeded = true;
    httpSourcePlugin->SetInterruptState(isInterruptNeeded);
    EXPECT_EQ(httpSourcePlugin->isInterruptNeeded_, isInterruptNeeded);
}

HWTEST_F(HttpSourcePluginUnitTest, SetInterruptState2, TestSize.Level1)
{
    bool isInterruptNeeded = false;
    httpSourcePlugin->SetInterruptState(isInterruptNeeded);
    EXPECT_EQ(httpSourcePlugin->isInterruptNeeded_, isInterruptNeeded);
}

}
}
}
}