/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include <atomic>
#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include "gtest/gtest.h"
#include "common_tool.h"
#include "avcodec_audio_avbuffer_decoder_demo.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS;
using namespace OHOS::MediaAVCodec::AudioBufferDemo;

namespace {
class FuzzTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void FuzzTest::SetUpTestCase() {}
void FuzzTest::TearDownTestCase() {}
void FuzzTest::SetUp() {}
void FuzzTest::TearDown() {}

} // namespace

/**
 * @tc.number    : FUZZ_TEST_001
 * @tc.name      : CreateByMime - mime fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_001, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVErrCode result0;
    ADecBufferDemo *aDecBufferDemo = new ADecBufferDemo();
    CommonTool *commonTool = new CommonTool();
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        codec = aDecBufferDemo->CreateByMime(commonTool->GetRandString().c_str());
        result0 = aDecBufferDemo->Destroy(codec);
        cout << "cur run times is " << i << ", result is " << result0 << endl;
    }
    delete commonTool;
    delete aDecBufferDemo;
}

/**
 * @tc.number    : FUZZ_TEST_002
 * @tc.name      : CreateByName - mime fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_002, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVErrCode result0;
    ADecBufferDemo *aDecBufferDemo = new ADecBufferDemo();
    CommonTool *commonTool = new CommonTool();
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        codec = aDecBufferDemo->CreateByName(commonTool->GetRandString().c_str());
        result0 = aDecBufferDemo->Destroy(codec);
        cout << "cur run times is " << i << ", result is " << result0 << endl;
    }
    delete commonTool;
    delete aDecBufferDemo;
}

/**
 * @tc.number    : FUZZ_TEST_003
 * @tc.name      : Configure - channel fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_003, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channel = 1;
    int32_t sampleRate = 8000;
    OH_AVErrCode result0;
    ADecBufferDemo *aDecBufferDemo = new ADecBufferDemo();
    CommonTool *commonTool = new CommonTool();
    codec = aDecBufferDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_G711MU);
    ASSERT_NE(codec, nullptr);
    result0 = aDecBufferDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        channel = commonTool->GetRandInt();
        result0 = aDecBufferDemo->Configure(codec, format, channel, sampleRate);
    }
    result0 = aDecBufferDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete aDecBufferDemo;
}

/**
 * @tc.number    : FUZZ_TEST_004
 * @tc.name      : Configure - sampleRate fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_004, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channel = 1;
    int32_t sampleRate = 8000;
    OH_AVErrCode result0;
    ADecBufferDemo *aDecBufferDemo = new ADecBufferDemo();
    CommonTool *commonTool = new CommonTool();
    codec = aDecBufferDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_G711MU);
    ASSERT_NE(codec, nullptr);
    result0 = aDecBufferDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        sampleRate = commonTool->GetRandInt();
        result0 = aDecBufferDemo->Configure(codec, format, channel, sampleRate);
    }
    result0 = aDecBufferDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete aDecBufferDemo;
}

/**
 * @tc.number    : FUZZ_TEST_005
 * @tc.name      : PushInputData - index fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_005, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channel = 1;
    int32_t sampleRate = 8000;
    uint32_t index;
    OH_AVErrCode result0;
    ADecBufferDemo *aDecBufferDemo = new ADecBufferDemo();
    CommonTool *commonTool = new CommonTool();
    codec = aDecBufferDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_G711MU);
    ASSERT_NE(codec, nullptr);
    result0 = aDecBufferDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = aDecBufferDemo->Configure(codec, format, channel, sampleRate);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = aDecBufferDemo->Start(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    index = aDecBufferDemo->GetInputIndex();
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        index = commonTool->GetRandInt();
        result0 = aDecBufferDemo->PushInputData(codec, index);
    }
    result0 = aDecBufferDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete aDecBufferDemo;
}

/**
 * @tc.number    : FUZZ_TEST_006
 * @tc.name      : FreeOutputData - index fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_006, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channel = 1;
    int32_t sampleRate = 8000;
    uint32_t index;
    OH_AVErrCode result0;
    ADecBufferDemo *aDecBufferDemo = new ADecBufferDemo();
    CommonTool *commonTool = new CommonTool();
    codec = aDecBufferDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_G711MU);
    ASSERT_NE(codec, nullptr);
    result0 = aDecBufferDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = aDecBufferDemo->Configure(codec, format, channel, sampleRate);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = aDecBufferDemo->Start(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    index = aDecBufferDemo->GetInputIndex();
    result0 = aDecBufferDemo->PushInputDataEOS(codec, index);
    ASSERT_EQ(result0, AV_ERR_OK);
    index = aDecBufferDemo->GetOutputIndex();
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        index = commonTool->GetRandInt();
        result0 = aDecBufferDemo->FreeOutputData(codec, index);
    }
    result0 = aDecBufferDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete aDecBufferDemo;
}

/**
 * @tc.number    : FUZZ_TEST_007
 * @tc.name      : PushInputDataEOS - index fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_007, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channel = 1;
    int32_t sampleRate = 8000;
    uint32_t index;
    OH_AVErrCode result0;
    ADecBufferDemo *aDecBufferDemo = new ADecBufferDemo();
    CommonTool *commonTool = new CommonTool();
    codec = aDecBufferDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_G711MU);
    ASSERT_NE(codec, nullptr);
    result0 = aDecBufferDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = aDecBufferDemo->Configure(codec, format, channel, sampleRate);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = aDecBufferDemo->Start(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    index = aDecBufferDemo->GetInputIndex();
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        index = commonTool->GetRandInt();
        result0 = aDecBufferDemo->PushInputDataEOS(codec, index);
    }
    result0 = aDecBufferDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete aDecBufferDemo;
}
