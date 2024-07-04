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

#include "gtest/gtest.h"
#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include "inner_demuxer_parser_sample.h"
#include "avcodec_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;

namespace {
static const string TEST_FILE_PATH = "/data/test/media/";
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
string g_file_one_i_frame_avc = TEST_FILE_PATH + string("demuxer_parser_one_i_frame_avc.mp4");
string g_file_all_i_frame_avc = TEST_FILE_PATH + string("demuxer_parser_all_i_frame_avc.mp4");
string g_file_ipb_frame_avc = TEST_FILE_PATH + string("demuxer_parser_ipb_frame_avc.mp4");
string g_file_sdtp_frame_avc = TEST_FILE_PATH + string("demuxer_parser_sdtp_frame_avc.mp4");
string g_file_ltr_frame_avc = TEST_FILE_PATH + string("demuxer_parser_ltr_frame_avc.mp4");
string g_file_2_layer_frame_avc = TEST_FILE_PATH + string("demuxer_parser_2_layer_frame_avc.mp4");
string g_file_3_layer_frame_avc = TEST_FILE_PATH + string("demuxer_parser_3_layer_frame_avc.mp4");
string g_file_4_layer_frame_avc = TEST_FILE_PATH + string("demuxer_parser_4_layer_frame_avc.mp4");
string g_test_file_ts_avc = TEST_FILE_PATH + string("h264_aac_640.ts");
string g_test_file_ts_hevc = TEST_FILE_PATH + string("h265_mp3_640.ts");
string g_file_one_i_frame_hevc = TEST_FILE_PATH + string("demuxer_parser_one_i_frame_hevc.mp4");
string g_file_all_i_frame_hevc = TEST_FILE_PATH + string("demuxer_parser_all_i_frame_hevc.mp4");
string g_file_ipb_frame_hevc = TEST_FILE_PATH + string("demuxer_parser_ipb_frame_hevc.mp4");
string g_file_sdtp_frame_hevc = TEST_FILE_PATH + string("demuxer_parser_sdtp_frame_hevc.mp4");
string g_file_ltr_frame_hevc = TEST_FILE_PATH + string("demuxer_parser_ltr_frame_hevc.mp4");
string g_file_2_layer_frame_hevc = TEST_FILE_PATH + string("demuxer_parser_2_layer_frame_hevc.mp4");
string g_file_3_layer_frame_hevc = TEST_FILE_PATH + string("demuxer_parser_3_layer_frame_hevc.mp4");
string g_file_4_layer_frame_hevc = TEST_FILE_PATH + string("demuxer_parser_4_layer_frame_hevc.mp4");
} // namespace

namespace {
class InnerParsercNdkTest : public testing::Test {
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

void InnerParsercNdkTest::SetUpTestCase() {}
void InnerParsercNdkTest::TearDownTestCase() {}
void InnerParsercNdkTest::SetUp() {}
void InnerParsercNdkTest::TearDown() {}
} // namespace

namespace {
/**
 * @tc.number    : DEMUXER_REFERENCE_H264_API_0010
 * @tc.name      : StartRefParser with non-existent pts
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_API_0010, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_file_3_layer_frame_avc);
    ASSERT_EQ(AVCS_ERR_UNKNOWN, parserSample->demuxer_->StartReferenceParser(-999999));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_API_0020
 * @tc.name      : StartRefParser with unsupported ts resources
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_API_0020, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_test_file_ts_avc);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_FILE_TYPE, parserSample->demuxer_->StartReferenceParser(0));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_API_0030
 * @tc.name      : GetFrameLayerInfo with empty buffer
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_API_0030, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_file_3_layer_frame_avc);
    ASSERT_EQ(AVCS_ERR_OK, parserSample->demuxer_->StartReferenceParser(0));
    FrameLayerInfo frameLayerInfo;
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, parserSample->demuxer_->GetFrameLayerInfo(parserSample->avBuffer, frameLayerInfo));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_API_0040
 * @tc.name      : GetFrameLayerInfo with unsupported ts resources
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_API_0040, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_test_file_ts_avc);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_FILE_TYPE, parserSample->demuxer_->StartReferenceParser(0));
    parserSample->InitParameter(MP4Scene::ONE_I_FRAME_AVC);
    FrameLayerInfo frameLayerInfo;
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, parserSample->demuxer_->GetFrameLayerInfo(parserSample->avBuffer, frameLayerInfo));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_API_0050
 * @tc.name      : GetGopLayerInfo with error gopID
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_API_0050, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_file_3_layer_frame_avc);
    ASSERT_EQ(AVCS_ERR_OK, parserSample->demuxer_->StartReferenceParser(0));
    GopLayerInfo gopLayerInfo;
    ASSERT_EQ(AVCS_ERR_TRY_AGAIN, parserSample->demuxer_->GetGopLayerInfo(99999, gopLayerInfo));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_API_0060
 * @tc.name      : GetFrameLayerInfo with unsupported ts resources
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_API_0060, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_test_file_ts_avc);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_FILE_TYPE, parserSample->demuxer_->StartReferenceParser(0));
    GopLayerInfo gopLayerInfo;
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, parserSample->demuxer_->GetGopLayerInfo(1, gopLayerInfo));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0010
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in an I-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0010, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_one_i_frame_avc);
    parserSample->InitParameter(MP4Scene::ONE_I_FRAME_AVC);
    parserSample->usleepTime = 1000000;
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0020
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in all I-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0020, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_all_i_frame_avc);
    parserSample->InitParameter(MP4Scene::ALL_I_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0030
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in IPB-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0030, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ipb_frame_avc);
    parserSample->InitParameter(MP4Scene::IPB_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0040
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in sdtp-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0040, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_sdtp_frame_avc);
    parserSample->InitParameter(MP4Scene::SDTP_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0060
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in 2-layer-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0060, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_2_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::TWO_LAYER_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0070
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in 4-layer-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0070, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_4_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::FOUR_LAYER_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0080
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in Ltr-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0080, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ltr_frame_avc);
    parserSample->InitParameter(MP4Scene::LTR_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0090
 * @tc.name      : Seek forward to the end of the vedio for 1000ms in gop10 fps30 3-layer-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0090, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ltr_frame_avc);
    parserSample->InitParameter(MP4Scene::LTR_FRAME_AVC);
    for (int64_t i = 0; i < parserSample->duration; i = i + 1000000) {
        parserSample->specified_pts = i / 1000.0;
        ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::SPECIFIED_PTS));
    }
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0100
 * @tc.name      : forward to the end,seek backward to the beginning in gop30 fps30 3-layer-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0100, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ltr_frame_avc);
    parserSample->InitParameter(MP4Scene::LTR_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::END_PTS));
    for (int32_t i = parserSample->duration / 1000.0; i >= 0 ; i = i - 1000) {
        parserSample->specified_pts = i;
        ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
    }
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0110
 * @tc.name      : Seek back and forth according to the preset direction and duration in 3-layer-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0110, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_AVC);
    for (int32_t i = 0; i < 10; i++) {
        ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
    }
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0120
 * @tc.name      : from the beginning to the end,from the end to the beginning repeat 10 times in 3-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0120, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_AVC);
    for (int32_t i = 0; i < 10; i++) {
        ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::END_PTS));
        ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::START_PTS));
    }
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0130
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs is the start position in 3-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0130, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::START_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0140
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in gop30 fps30 3-layer-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0140, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}


/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0150
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions frame for startTimeMs in an I
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0150, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_one_i_frame_avc);
    parserSample->InitParameter(MP4Scene::ONE_I_FRAME_AVC);
    parserSample->usleepTime = 1000000;
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0160
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions frame for startTimeMs in all I
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0160, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_all_i_frame_avc);
    parserSample->InitParameter(MP4Scene::ALL_I_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0170
 * @tc.name      : Pts corresponding to the Nth for startTimeMs=0 in 3-layer-frame double speed play seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0170, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::START_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0180
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions for startTimeMs in 3-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0180, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0190
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs is the last frame in 3-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0190, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::END_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0200
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions for startTimeMs in 4-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0200, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_4_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::FOUR_LAYER_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0210
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions for startTimeMs in 2-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0210, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_2_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::TWO_LAYER_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0220
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions frame for startTimeMs in Ltr-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0220, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ltr_frame_avc);
    parserSample->InitParameter(MP4Scene::LTR_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0230
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions for startTimeMs in gop30 IPB
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0230, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ipb_frame_avc);
    parserSample->InitParameter(MP4Scene::IPB_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H264_FUNC_0240
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions frame for startTimeMs in sdtp
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H264_FUNC_0240, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_sdtp_frame_avc);
    parserSample->InitParameter(MP4Scene::SDTP_FRAME_AVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_API_0010
 * @tc.name      : StartRefParser with non-existent pts
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_API_0010, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_file_3_layer_frame_hevc);
    ASSERT_EQ(AVCS_ERR_UNKNOWN, parserSample->demuxer_->StartReferenceParser(-999999));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_API_0020
 * @tc.name      : StartRefParser with unsupported ts resources
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_API_0020, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_test_file_ts_hevc);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_FILE_TYPE, parserSample->demuxer_->StartReferenceParser(0));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_API_0030
 * @tc.name      : GetFrameLayerInfo with empty buffer
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_API_0030, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_file_3_layer_frame_hevc);
    ASSERT_EQ(AVCS_ERR_OK, parserSample->demuxer_->StartReferenceParser(0));
    FrameLayerInfo frameLayerInfo;
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, parserSample->demuxer_->GetFrameLayerInfo(parserSample->avBuffer, frameLayerInfo));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_API_0040
 * @tc.name      : GetFrameLayerInfo with unsupported ts resources
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_API_0040, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_test_file_ts_hevc);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_FILE_TYPE, parserSample->demuxer_->StartReferenceParser(0));
    parserSample->InitParameter(MP4Scene::ONE_I_FRAME_HEVC);
    FrameLayerInfo frameLayerInfo;
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, parserSample->demuxer_->GetFrameLayerInfo(parserSample->avBuffer, frameLayerInfo));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_API_0050
 * @tc.name      : GetGopLayerInfo with error gopID
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_API_0050, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_file_3_layer_frame_hevc);
    ASSERT_EQ(AVCS_ERR_OK, parserSample->demuxer_->StartReferenceParser(0));
    GopLayerInfo gopLayerInfo;
    ASSERT_EQ(AVCS_ERR_TRY_AGAIN, parserSample->demuxer_->GetGopLayerInfo(99999, gopLayerInfo));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_API_0060
 * @tc.name      : GetFrameLayerInfo with unsupported ts resources
 * @tc.desc      : api test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_API_0060, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    auto parserSample = make_unique<InnerDemuxerParserSample>(g_test_file_ts_hevc);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_FILE_TYPE, parserSample->demuxer_->StartReferenceParser(0));
    GopLayerInfo gopLayerInfo;
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, parserSample->demuxer_->GetGopLayerInfo(1, gopLayerInfo));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0010
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in an I-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0010, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_one_i_frame_hevc);
    parserSample->InitParameter(MP4Scene::ONE_I_FRAME_HEVC);
    parserSample->usleepTime = 1000000;
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0020
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in all I-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0020, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_all_i_frame_hevc);
    parserSample->InitParameter(MP4Scene::ALL_I_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0030
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in IPB-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0030, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ipb_frame_hevc);
    parserSample->InitParameter(MP4Scene::IPB_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0040
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in sdtp-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0040, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_sdtp_frame_hevc);
    parserSample->InitParameter(MP4Scene::SDTP_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0060
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in 2-layer-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0060, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = nullptr;
    parserSample = make_shared<InnerDemuxerParserSample>(g_file_2_layer_frame_hevc);
    parserSample->InitParameter(MP4Scene::TWO_LAYER_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0070
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in 4-layer-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0070, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = nullptr;
    parserSample = make_shared<InnerDemuxerParserSample>(g_file_4_layer_frame_hevc);
    parserSample->InitParameter(MP4Scene::FOUR_LAYER_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0080
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in Ltr-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0080, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ltr_frame_hevc);
    parserSample->InitParameter(MP4Scene::LTR_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0090
 * @tc.name      : Seek forward to the end of the vedio for 1000ms in gop10 fps30 3-layer-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0090, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ltr_frame_hevc);
    parserSample->InitParameter(MP4Scene::LTR_FRAME_HEVC);
    for (int64_t i = 0; i < parserSample->duration; i = i + 1000000) {
        parserSample->specified_pts = i / 1000.0;
        ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::SPECIFIED_PTS));
    }
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0100
 * @tc.name      : Seek to the end,then backward to the beginning for 1000ms in gop30 fps30 3-layer-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0100, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ltr_frame_hevc);
    parserSample->InitParameter(MP4Scene::LTR_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::END_PTS));
    for (int32_t i = parserSample->duration / 1000.0; i >= 0 ; i = i - 1000) {
        parserSample->specified_pts = i;
        ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
    }
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0110
 * @tc.name      : Seek back and forth according to the preset direction and duration in gop60 fps30 3-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0110, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = nullptr;
    parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_hevc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_HEVC);
    for (int32_t i = 0; i < 10; i++) {
        ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
    }
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0120
 * @tc.name      : from the beginning to the end and from the end to the beginning repeat 10 times in 3-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0120, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = nullptr;
    parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_hevc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_HEVC);
    for (int32_t i = 0; i < 10; i++) {
        ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::END_PTS));
        ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::START_PTS));
    }
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0130
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs is the start position in 3-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0130, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = nullptr;
    parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_hevc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::START_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0140
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs in gop30 fps30 3-layer-frame seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0140, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = nullptr;
    parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_hevc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
}


/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0150
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions frame for startTimeMs in an I
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0150, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_one_i_frame_hevc);
    parserSample->InitParameter(MP4Scene::ONE_I_FRAME_HEVC);
    parserSample->usleepTime = 1000000;
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0160
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions for startTimeMs in all I-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0160, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_all_i_frame_hevc);
    parserSample->InitParameter(MP4Scene::ALL_I_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0170
 * @tc.name      : Pts corresponding to the Nth for startTimeMs=0 in 3-layer-frame double speed play seek scene
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0170, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = nullptr;
    parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_hevc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::START_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0180
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions for startTimeMs in 3-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0180, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = nullptr;
    parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_hevc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0190
 * @tc.name      : Pts corresponding to the Nth frame for startTimeMs is the last frame in 3-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0190, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = nullptr;
    parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_hevc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::END_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0200
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions frame in 4-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0200, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = nullptr;
    parserSample = make_shared<InnerDemuxerParserSample>(g_file_4_layer_frame_hevc);
    parserSample->InitParameter(MP4Scene::FOUR_LAYER_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0210
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions frame in 2-layer-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0210, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = nullptr;
    parserSample = make_shared<InnerDemuxerParserSample>(g_file_2_layer_frame_hevc);
    parserSample->InitParameter(MP4Scene::TWO_LAYER_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0220
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions frame in Ltr-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0220, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ltr_frame_hevc);
    parserSample->InitParameter(MP4Scene::LTR_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0230
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions frame in gop30 IPB-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0230, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_ipb_frame_hevc);
    parserSample->InitParameter(MP4Scene::IPB_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

/**
 * @tc.number    : DEMUXER_REFERENCE_H265_FUNC_0240
 * @tc.name      : Randomly generating Pts corresponding to the N existing positions frame in sdtp-frame
 * @tc.desc      : func test
 */
HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_H265_FUNC_0240, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_sdtp_frame_hevc);
    parserSample->InitParameter(MP4Scene::SDTP_FRAME_HEVC);
    ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::RANDOM_PTS));
}

HWTEST_F(InnerParsercNdkTest, DEMUXER_REFERENCE_RELI_0010, TestSize.Level3)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    shared_ptr<InnerDemuxerParserSample> parserSample = make_shared<InnerDemuxerParserSample>(g_file_3_layer_frame_avc);
    parserSample->InitParameter(MP4Scene::THREE_LAYER_FRAME_AVC);
    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(parserSample->RunSeekScene(WorkPts::RANDOM_PTS));
    }
    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(parserSample->RunSpeedScene(WorkPts::START_PTS));
    }
}

} // namespace