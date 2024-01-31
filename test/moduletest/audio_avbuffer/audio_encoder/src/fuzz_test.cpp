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
#include "avcodec_audio_avbuffer_encoder_demo.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS;
using namespace OHOS::MediaAVCodec::AudioAacEncDemo;

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
    AudioBufferAacEncDemo *audioBufferAacEncDemo = new AudioBufferAacEncDemo();
    CommonTool *commonTool = new CommonTool();
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        codec = audioBufferAacEncDemo->CreateByMime(commonTool->GetRandString().c_str());
        result0 = audioBufferAacEncDemo->Destroy(codec);
        cout << "cur run times is " << i << ", result is " << result0 << endl;
    }
    delete commonTool;
    delete audioBufferAacEncDemo;
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
    AudioBufferAacEncDemo *audioBufferAacEncDemo = new AudioBufferAacEncDemo();
    CommonTool *commonTool = new CommonTool();
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        codec = audioBufferAacEncDemo->CreateByName(commonTool->GetRandString().c_str());
        result0 = audioBufferAacEncDemo->Destroy(codec);
        cout << "cur run times is " << i << ", result is " << result0 << endl;
    }
    delete commonTool;
    delete audioBufferAacEncDemo;
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
    int32_t channel = 2;
    int32_t sampleRate = 48000;
    int64_t bitRate = 15000;
    int32_t sampleFormat = SAMPLE_S16LE;
    int32_t sampleBit = 16;
    int32_t complexity = 10;
    OH_AVErrCode result0;
    AudioBufferAacEncDemo *audioBufferAacEncDemo = new AudioBufferAacEncDemo();
    CommonTool *commonTool = new CommonTool();
    codec = audioBufferAacEncDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    ASSERT_NE(codec, nullptr);
    result0 = audioBufferAacEncDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        channel = commonTool->GetRandInt();
        result0 = audioBufferAacEncDemo->Configure(codec, format, channel, sampleRate, bitRate, sampleFormat, sampleBit,
                                                   complexity);
    }
    result0 = audioBufferAacEncDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete audioBufferAacEncDemo;
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
    int32_t channel = 2;
    int32_t sampleRate = 48000;
    int64_t bitRate = 15000;
    int32_t sampleFormat = SAMPLE_S16LE;
    int32_t sampleBit = 16;
    int32_t complexity = 10;
    OH_AVErrCode result0;
    AudioBufferAacEncDemo *audioBufferAacEncDemo = new AudioBufferAacEncDemo();
    CommonTool *commonTool = new CommonTool();
    codec = audioBufferAacEncDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    ASSERT_NE(codec, nullptr);
    result0 = audioBufferAacEncDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        sampleRate = commonTool->GetRandInt();
        result0 = audioBufferAacEncDemo->Configure(codec, format, channel, sampleRate, bitRate, sampleFormat, sampleBit,
                                                   complexity);
    }
    result0 = audioBufferAacEncDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete audioBufferAacEncDemo;
}

/**
 * @tc.number    : FUZZ_TEST_005
 * @tc.name      : Configure - bitRate fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_005, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channel = 2;
    int32_t sampleRate = 48000;
    int64_t bitRate = 15000;
    int32_t sampleFormat = SAMPLE_S16LE;
    int32_t sampleBit = 16;
    int32_t complexity = 10;
    OH_AVErrCode result0;
    AudioBufferAacEncDemo *audioBufferAacEncDemo = new AudioBufferAacEncDemo();
    CommonTool *commonTool = new CommonTool();
    codec = audioBufferAacEncDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    ASSERT_NE(codec, nullptr);
    result0 = audioBufferAacEncDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        bitRate = commonTool->GetRandInt();
        result0 = audioBufferAacEncDemo->Configure(codec, format, channel, sampleRate, bitRate, sampleFormat, sampleBit,
                                                   complexity);
    }
    result0 = audioBufferAacEncDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete audioBufferAacEncDemo;
}

/**
 * @tc.number    : FUZZ_TEST_006
 * @tc.name      : Configure - sampleFormat fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_006, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channel = 2;
    int32_t sampleRate = 48000;
    int64_t bitRate = 15000;
    int32_t sampleFormat = SAMPLE_S16LE;
    int32_t sampleBit = 16;
    int32_t complexity = 10;
    OH_AVErrCode result0;
    AudioBufferAacEncDemo *audioBufferAacEncDemo = new AudioBufferAacEncDemo();
    CommonTool *commonTool = new CommonTool();
    codec = audioBufferAacEncDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    ASSERT_NE(codec, nullptr);
    result0 = audioBufferAacEncDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        sampleFormat = commonTool->GetRandInt();
        result0 = audioBufferAacEncDemo->Configure(codec, format, channel, sampleRate, bitRate, sampleFormat, sampleBit,
                                                   complexity);
    }
    result0 = audioBufferAacEncDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete audioBufferAacEncDemo;
}

/**
 * @tc.number    : FUZZ_TEST_007
 * @tc.name      : Configure - complexity fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_007, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channel = 2;
    int32_t sampleRate = 48000;
    int64_t bitRate = 15000;
    int32_t sampleFormat = SAMPLE_S16LE;
    int32_t sampleBit = 16;
    int32_t complexity = 10;
    OH_AVErrCode result0;
    AudioBufferAacEncDemo *audioBufferAacEncDemo = new AudioBufferAacEncDemo();
    CommonTool *commonTool = new CommonTool();
    codec = audioBufferAacEncDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    ASSERT_NE(codec, nullptr);
    result0 = audioBufferAacEncDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        complexity = commonTool->GetRandInt();
        result0 = audioBufferAacEncDemo->Configure(codec, format, channel, sampleRate, bitRate, sampleFormat, sampleBit,
                                                   complexity);
    }
    result0 = audioBufferAacEncDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete audioBufferAacEncDemo;
}

/**
 * @tc.number    : FUZZ_TEST_008
 * @tc.name      : PushInputData - index fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_008, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channel = 2;
    int32_t sampleRate = 48000;
    int64_t bitRate = 15000;
    int32_t sampleFormat = SAMPLE_S16LE;
    int32_t sampleBit = 16;
    int32_t complexity = 10;
    uint32_t index;
    OH_AVErrCode result0;
    AudioBufferAacEncDemo *audioBufferAacEncDemo = new AudioBufferAacEncDemo();
    CommonTool *commonTool = new CommonTool();
    codec = audioBufferAacEncDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    ASSERT_NE(codec, nullptr);
    result0 = audioBufferAacEncDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = audioBufferAacEncDemo->Configure(codec, format, channel, sampleRate, bitRate, sampleFormat, sampleBit,
                                               complexity);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = audioBufferAacEncDemo->Start(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    index = audioBufferAacEncDemo->GetInputIndex();
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        index = commonTool->GetRandInt();
        result0 = audioBufferAacEncDemo->PushInputData(codec, index);
    }
    result0 = audioBufferAacEncDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete audioBufferAacEncDemo;
}

/**
 * @tc.number    : FUZZ_TEST_009
 * @tc.name      : PushInputDataEOS - index fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_009, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channel = 2;
    int32_t sampleRate = 48000;
    int64_t bitRate = 15000;
    int32_t sampleFormat = SAMPLE_S16LE;
    int32_t sampleBit = 16;
    int32_t complexity = 10;
    uint32_t index;
    OH_AVErrCode result0;
    AudioBufferAacEncDemo *audioBufferAacEncDemo = new AudioBufferAacEncDemo();
    CommonTool *commonTool = new CommonTool();
    codec = audioBufferAacEncDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    ASSERT_NE(codec, nullptr);
    result0 = audioBufferAacEncDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = audioBufferAacEncDemo->Configure(codec, format, channel, sampleRate, bitRate, sampleFormat, sampleBit,
                                               complexity);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = audioBufferAacEncDemo->Start(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    index = audioBufferAacEncDemo->GetInputIndex();
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        index = commonTool->GetRandInt();
        result0 = audioBufferAacEncDemo->PushInputDataEOS(codec, index);
    }
    result0 = audioBufferAacEncDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete audioBufferAacEncDemo;
}

/**
 * @tc.number    : FUZZ_TEST_010
 * @tc.name      : FreeOutputData - index fuzz test
 * @tc.desc      : param fuzz test
 */
HWTEST_F(FuzzTest, FUZZ_TEST_010, TestSize.Level2)
{
    OH_AVCodec *codec = nullptr;
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channel = 2;
    int32_t sampleRate = 48000;
    int64_t bitRate = 15000;
    int32_t sampleFormat = SAMPLE_S16LE;
    int32_t sampleBit = 16;
    int32_t complexity = 10;
    uint32_t index;
    OH_AVErrCode result0;
    AudioBufferAacEncDemo *audioBufferAacEncDemo = new AudioBufferAacEncDemo();
    CommonTool *commonTool = new CommonTool();
    codec = audioBufferAacEncDemo->CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    ASSERT_NE(codec, nullptr);
    result0 = audioBufferAacEncDemo->SetCallback(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = audioBufferAacEncDemo->Configure(codec, format, channel, sampleRate, bitRate, sampleFormat, sampleBit,
                                               complexity);
    ASSERT_EQ(result0, AV_ERR_OK);
    result0 = audioBufferAacEncDemo->Start(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    index = audioBufferAacEncDemo->GetInputIndex();
    result0 = audioBufferAacEncDemo->PushInputDataEOS(codec, index);
    ASSERT_EQ(result0, AV_ERR_OK);
    index = audioBufferAacEncDemo->GetOutputIndex();
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        index = commonTool->GetRandInt();
        result0 = audioBufferAacEncDemo->FreeOutputData(codec, index);
    }
    result0 = audioBufferAacEncDemo->Destroy(codec);
    ASSERT_EQ(result0, AV_ERR_OK);
    delete commonTool;
    delete audioBufferAacEncDemo;
}

/**
 * @tc.number    : FUZZ_TEST_011
 * @tc.name      : RunCase file fuzz test
 * @tc.desc      : file fuzz test
 */
// HWTEST_F(FuzzTest, FUZZ_TEST_011, TestSize.Level2)
// {
// 	std::string inputFile;
// 	std::string outputFile;
// 	bool result0;
// 	string fuzzList[] = {
// 			"Radetzky1.pcm", "Radetzky2.pcm", "Radetzky3.pcm", "Radetzky4.pcm", "Radetzky5.pcm",
// 			"Radetzky6.pcm", "Radetzky7.pcm", "Radetzky8.pcm", "Radetzky9.pcm", "Radetzky10.pcm",
// 			"Radetzky11.pcm", "Radetzky12.pcm", "Radetzky13.pcm", "Radetzky14.pcm", "Radetzky15.pcm",
// 			"Radetzky16.pcm", "Radetzky17.pcm", "Radetzky18.pcm", "Radetzky19.pcm", "Radetzky20.pcm",
// 			"Radetzky21.pcm", "Radetzky22.pcm", "Radetzky23.pcm", "Radetzky24.pcm", "Radetzky25.pcm",
// 			"Radetzky26.pcm", "Radetzky27.pcm", "Radetzky28.pcm", "Radetzky29.pcm", "Radetzky30.pcm",
// 			"Radetzky31.pcm", "Radetzky32.pcm", "Radetzky33.pcm", "Radetzky34.pcm", "Radetzky35.pcm",
// 			"Radetzky36.pcm", "Radetzky37.pcm", "Radetzky38.pcm", "Radetzky39.pcm", "Radetzky40.pcm",
// 			"Radetzky41.pcm", "Radetzky42.pcm", "Radetzky43.pcm", "Radetzky44.pcm", "Radetzky45.pcm",
// 			"Radetzky46.pcm", "Radetzky47.pcm", "Radetzky48.pcm", "Radetzky49.pcm", "Radetzky50.pcm",
// 			"Radetzky51.pcm", "Radetzky52.pcm", "Radetzky53.pcm", "Radetzky54.pcm", "Radetzky55.pcm",
// 			"Radetzky56.pcm", "Radetzky57.pcm", "Radetzky58.pcm", "Radetzky59.pcm", "Radetzky60.pcm",
// 			"Radetzky61.pcm", "Radetzky62.pcm", "Radetzky63.pcm", "Radetzky64.pcm", "Radetzky65.pcm",
// 			"Radetzky66.pcm", "Radetzky67.pcm", "Radetzky68.pcm", "Radetzky69.pcm", "Radetzky70.pcm",
// 			"Radetzky71.pcm", "Radetzky72.pcm", "Radetzky73.pcm", "Radetzky74.pcm", "Radetzky75.pcm",
// 			"Radetzky76.pcm", "Radetzky77.pcm", "Radetzky78.pcm", "Radetzky79.pcm", "Radetzky80.pcm",
// 			"Radetzky81.pcm", "Radetzky82.pcm", "Radetzky83.pcm", "Radetzky84.pcm", "Radetzky85.pcm",
// 			"Radetzky86.pcm", "Radetzky87.pcm", "Radetzky88.pcm", "Radetzky89.pcm", "Radetzky90.pcm",
// 			"Radetzky91.pcm", "Radetzky92.pcm", "Radetzky93.pcm", "Radetzky94.pcm", "Radetzky95.pcm",
// 			"Radetzky96.pcm", "Radetzky97.pcm", "Radetzky98.pcm", "Radetzky99.pcm", "Radetzky100.pcm",
// 			"Radetzky101.pcm", "Radetzky102.pcm", "Radetzky103.pcm", "Radetzky104.pcm", "Radetzky105.pcm",
// 			"Radetzky106.pcm", "Radetzky107.pcm", "Radetzky108.pcm", "Radetzky109.pcm", "Radetzky110.pcm",
// 			"Radetzky111.pcm", "Radetzky112.pcm", "Radetzky113.pcm", "Radetzky114.pcm", "Radetzky115.pcm",
// 			"Radetzky116.pcm", "Radetzky117.pcm", "Radetzky118.pcm", "Radetzky119.pcm", "Radetzky120.pcm",
// 			"Radetzky121.pcm", "Radetzky122.pcm", "Radetzky123.pcm", "Radetzky124.pcm", "Radetzky125.pcm",
// 			"Radetzky126.pcm", "Radetzky127.pcm", "Radetzky128.pcm", "Radetzky129.pcm", "Radetzky130.pcm",
// 			"Radetzky131.pcm", "Radetzky132.pcm", "Radetzky133.pcm", "Radetzky134.pcm", "Radetzky135.pcm",
// 			"Radetzky136.pcm", "Radetzky137.pcm", "Radetzky138.pcm", "Radetzky139.pcm", "Radetzky140.pcm",
// 			"Radetzky141.pcm", "Radetzky142.pcm", "Radetzky143.pcm", "Radetzky144.pcm", "Radetzky145.pcm",
// 			"Radetzky146.pcm", "Radetzky147.pcm", "Radetzky148.pcm", "Radetzky149.pcm", "Radetzky150.pcm",
// 			"Radetzky151.pcm", "Radetzky152.pcm", "Radetzky153.pcm", "Radetzky154.pcm", "Radetzky155.pcm",
// 			"Radetzky156.pcm", "Radetzky157.pcm", "Radetzky158.pcm", "Radetzky159.pcm", "Radetzky160.pcm",
// 			"Radetzky161.pcm", "Radetzky162.pcm", "Radetzky163.pcm", "Radetzky164.pcm", "Radetzky165.pcm",
// 			"Radetzky166.pcm", "Radetzky167.pcm", "Radetzky168.pcm", "Radetzky169.pcm", "Radetzky170.pcm",
// 			"Radetzky171.pcm", "Radetzky172.pcm", "Radetzky173.pcm", "Radetzky174.pcm", "Radetzky175.pcm",
// 			"Radetzky176.pcm", "Radetzky177.pcm", "Radetzky178.pcm", "Radetzky179.pcm", "Radetzky180.pcm",
// 			"Radetzky181.pcm", "Radetzky182.pcm", "Radetzky183.pcm", "Radetzky184.pcm", "Radetzky185.pcm",
// 			"Radetzky186.pcm", "Radetzky187.pcm", "Radetzky188.pcm", "Radetzky189.pcm", "Radetzky190.pcm",
// 			"Radetzky191.pcm", "Radetzky192.pcm", "Radetzky193.pcm", "Radetzky194.pcm", "Radetzky195.pcm",
// 			"Radetzky196.pcm", "Radetzky197.pcm", "Radetzky198.pcm", "Radetzky199.pcm", "Radetzky200.pcm",
// 	};
// 	AudioBufferAacEncDemo* audioBufferAacEncDemo = new AudioBufferAacEncDemo();
// 	for (int32_t i = 0; i < FUZZ_FILE_LENGTH; i++) {
// 		result0 = audioBufferAacEncDemo->RunCase(fuzzList[i], "FUZZ_TEST_011.dat");
// 		ASSERT_EQ(result0, true);
// 	}
// 	delete audioBufferAacEncDemo;
// }
