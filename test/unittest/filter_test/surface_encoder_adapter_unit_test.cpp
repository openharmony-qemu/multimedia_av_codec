/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <ctime>
#include "surface_encoder_adapter_unit_test.h"
#include "surface_encoder_adapter.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "codec_server.h"
#include "meta/format.h"
#include "media_description.h"
#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include "avcodec_trace.h"
#include "avcodec_sysevent.h"
#include "common/log.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void SurfaceEncoderAdapterUnitTest::SetUpTestCase(void) {}

void SurfaceEncoderAdapterUnitTest::TearDownTestCase(void) {}

void SurfaceEncoderAdapterUnitTest::SetUp(void)
{
    surfaceEncoderAdapter_ = std::make_shared<SurfaceEncoderAdapter>();
}

void SurfaceEncoderAdapterUnitTest::TearDown(void)
{
    surfaceEncoderAdapter_ = nullptr;
}

/**
 * @tc.name: SurfaceEncoderAdapter_Init_0100
 * @tc.desc: Init
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Init_0100, TestSize.Level1)
{
    std::string mime = "SurfaceEncoderAdaptertest";
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->Init(mime, true);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    ret = surfaceEncoderAdapter_->Init(mime, true);
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("test");
    ret = surfaceEncoderAdapter_->Init(mime, true);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Configure_0100
 * @tc.desc: Configure
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Configure_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    EXPECT_EQ(surfaceEncoderAdapter_->Configure(meta), Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = false;
    EXPECT_EQ(surfaceEncoderAdapter_->Configure(meta), Status::OK);
    surfaceEncoderAdapter_->isTransCoderMode = true;
    EXPECT_EQ(surfaceEncoderAdapter_->Configure(meta), Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetWatermark_0100
 * @tc.desc: SetWatermark
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetWatermark_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    std::shared_ptr<AVBuffer> waterMarkBuffer = AVBuffer::CreateAVBuffer();
    EXPECT_EQ(surfaceEncoderAdapter_->SetWatermark(waterMarkBuffer), Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    EXPECT_EQ(surfaceEncoderAdapter_->SetWatermark(waterMarkBuffer), Status::ERROR_UNKNOWN);
    waterMarkBuffer = nullptr;
    EXPECT_EQ(surfaceEncoderAdapter_->SetWatermark(waterMarkBuffer), Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetEncoderAdapterCallback_0100
 * @tc.desc: SetEncoderAdapterCallback
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetEncoderAdapterCallback_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    std::shared_ptr<EncoderAdapterCallback> encoderAdapterCallback = std::make_shared<MyEncoderAdapterCallback>();
    Status ret = surfaceEncoderAdapter_->SetEncoderAdapterCallback(encoderAdapterCallback);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->SetEncoderAdapterCallback(encoderAdapterCallback);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Start_0100
 * @tc.desc: Start
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Start_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->Start();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    ret = surfaceEncoderAdapter_->Start();
    surfaceEncoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("test");
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->Start();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Stop_0100
 * @tc.desc: Stop
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Stop_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    surfaceEncoderAdapter_->isStart_ = true;
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    Status ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("test");
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Pause_0100
 * @tc.desc: Pause
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Pause_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = true;
    Status ret = surfaceEncoderAdapter_->Pause();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->isTransCoderMode = false;
    ret = surfaceEncoderAdapter_->Pause();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Resume_0100
 * @tc.desc: Resume
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Resume_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = true;
    Status ret = surfaceEncoderAdapter_->Resume();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->isTransCoderMode = false;
    ret = surfaceEncoderAdapter_->Resume();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Flush_0100
 * @tc.desc: Flush
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Flush_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->Flush();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->Flush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Reset_0100
 * @tc.desc: Reset
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Reset_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->Reset();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->Reset();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Release_0100
 * @tc.desc: Release
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Release_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->Release();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->Release();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_NotifyEos_0100
 * @tc.desc: NotifyEos
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_NotifyEos_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->NotifyEos();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->NotifyEos();
    EXPECT_EQ(ret, Status::OK);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS