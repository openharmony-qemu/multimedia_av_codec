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

#include "avdemuxer.h"
#include "avsource.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "buffer/avsharedmemory.h"
#include "buffer/avsharedmemorybase.h"
#include "securec.h"
#include "inner_demuxer_sample.h"
#include "media_description.h"
#include "file_server_demo.h"
#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>


using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace testing::ext;

namespace {
const int64_t SOURCE_OFFSET = 0;
static std::shared_ptr<AVSource> source = nullptr;
static std::shared_ptr<AVDemuxer> demuxer = nullptr;
static unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
string g_doubleHevcPath = TEST_FILE_PATH + string("double_hevc.mp4");
string g_singleHevcPath = TEST_FILE_PATH + string("single_60.mp4");
string g_singleRkPath = TEST_FILE_PATH + string("single_rk.mp4");
string g_xmPath = TEST_FILE_PATH + string("xm.mp4");
string g_doubleVividPath = TEST_FILE_PATH + string("double_vivid.mp4");
string g_aigcMp4Path = TEST_FILE_PATH + string("aac_mpeg4_aigc.mp4");
string g_aigcFlvPath = TEST_FILE_PATH + string("aac_h265_aigc.flv");
string g_aigcMkvPath = TEST_FILE_PATH + string("aac_h265_aigc.mkv");
string g_aigcMovPath = TEST_FILE_PATH + string("gb18030_aigc.mov");
string g_aigcM4vPath = TEST_FILE_PATH + string("test_aigc.m4v");
string g_aigcM4aPath = TEST_FILE_PATH + string("audio/M4A_48000_aigc.m4a");
string g_aigcAviPath = TEST_FILE_PATH + string("profile0_level30_352x288_aigc.avi");
string g_aigcMp4256Path = TEST_FILE_PATH + string("aac_mpeg4_aigc_256.mp4");
string g_aigcMp4258Path = TEST_FILE_PATH + string("aac_mpeg4_aigc_258.mp4");
string g_flvPath = TEST_FILE_PATH + string("aac_h264.flv");
string g_mkvPath = TEST_FILE_PATH + string("aac_h265.mkv");
string g_movPath = TEST_FILE_PATH + string("gb2312.mov");
string g_m4vPath = TEST_FILE_PATH + string("test.m4v");
string g_m4aPath = TEST_FILE_PATH + string("audio/gb18030.m4a");
string g_aviPath = TEST_FILE_PATH + string("audio_2video.avi");
static int32_t g_apeVersion = 73728;
static const string AIGC_TEST_1 = "'{测Label1!:测Value1!,测2!ContentProducer:测!value2,测3!ProduceID:测!value3,"\
    "测4!ReservedCode1:测!value4,测5!ContentPropagator:测!value5,测6!PropagateID:测!value6,测7!ReservedCode2:测!value7}'";
static const string AIGC_TEST_2 = "{\"测Label1!\":\"测Value1!\",\"测2!ContentProducer\":\"测!value2\","\
    "\"测3!ProduceID\":\"测!value3\",\"测4!ReservedCode1\":\"测!value4\",\"测5!ContentPropagator\":\"测!value5\","\
    "\"测6!PropagateID\":\"测!value6\",\"测7!ReservedCode2\":\"测!value7\"}";
static const string AIGC_TEST_3 = "'{测Label1!:测Value1!,测2!ContentProducer:测!value2,测3!ProduceID:测!value3,"\
    "测4!ReservedCode1:测!value4,测5!ContentPropagator:测!value5,测6!PropagateID:测!value6,"\
    "测7!ReservedCode2:测!value701234567890123456789012345678901234567890123456789}'";
static const string AIGC_TEST_4 = "'{测Label1!:测Value1!,测2!ContentProducer:测!value2,测3!ProduceID:测!value3,"\
    "测4!ReservedCode1:测!value4,测5!ContentPropagator:测!value5,测6!PropagateID:测!value6,"\
    "测7!ReservedCode2:测!value70123456789012345678901234567890123456789012345678901}'";
} // namespace

namespace {
class DemuxerInnerFuncNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

private:
    bool CreateBufferSize();
    void GetFrameNum(int32_t trackIdx);
    std::shared_ptr<AVBuffer> avBuf_{ nullptr };
    int32_t fd_ = -1;
    int64_t size_;
    bool isVideoEosFlagForSave_ = false;
    bool isAudioEosFlagForSave_ = false;
    int32_t videoTrackIdx_ = 1;
    int32_t audioTrackIdx_ = 0;
    uint32_t videoIndexForRead_ = 0;
    uint32_t audioIndexForRead_ = 0;
};

void DemuxerInnerFuncNdkTest::SetUpTestCase()
{
    server = make_unique<FileServerDemo>();
    server->StartServer();
}
void DemuxerInnerFuncNdkTest::TearDownTestCase()
{
    server->StopServer();
}
void DemuxerInnerFuncNdkTest::SetUp() {}
void DemuxerInnerFuncNdkTest::TearDown()
{
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }

    if (source != nullptr) {
        source = nullptr;
    }

    if (demuxer != nullptr) {
        demuxer = nullptr;
    }
}

bool DemuxerInnerFuncNdkTest::CreateBufferSize()
{
    uint32_t buffersize = 1024 * 1024;
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuf_ = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    if (!avBuf_) {
        return false;
    }
    return true;
}

void DemuxerInnerFuncNdkTest::GetFrameNum(int32_t trackIdx)
{
    if (avBuf_->flag_ == MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS) {
        if (trackIdx == videoTrackIdx_) {
            isVideoEosFlagForSave_ = true;
        } else if (trackIdx == audioTrackIdx_) {
            isAudioEosFlagForSave_ = true;
        }
    } else {
        if (trackIdx == videoTrackIdx_) {
            videoIndexForRead_++;
        } else if (trackIdx == audioTrackIdx_) {
            audioIndexForRead_++;
        }
    }
}
} // namespace

namespace {
/**
 * @tc.number    : DEMUXER_ILLEGAL_FUNC_0100
 * @tc.name      : check video format from double
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_ILLEGAL_FUNC_0100, TestSize.Level0)
{
    fd_ = open(g_doubleHevcPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_doubleHevcPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);

    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longitude = 0.0;
    ASSERT_TRUE(format.GetFloatValue(Media::Tag::MEDIA_LONGITUDE, longitude));
    ASSERT_EQ(longitude, float(120.201302));

    float latitude = 0.0;
    ASSERT_TRUE(format.GetFloatValue(Media::Tag::MEDIA_LATITUDE, latitude));
    ASSERT_EQ(latitude, float(30.188101));

    string genre;
    ASSERT_TRUE(format.GetStringValue(Media::Tag::MEDIA_GENRE, genre));
    ASSERT_EQ(genre, "AAAAAA{marketing-name:\"AABBAABBAABBAABBAA\",param-hw-ext"\
    ":{camera-position-tag:2},param-use-tag:TypeNormalVideo}");

    Format metaFormat;
    ret = source->GetUserMeta(metaFormat);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::string manufacturer;
    ASSERT_TRUE(metaFormat.GetStringValue("com.abababa.manufacturer", manufacturer));
    ASSERT_EQ(manufacturer, "ABCDEF");
    std::string marketingName;
    ASSERT_TRUE(metaFormat.GetStringValue("com.abababa.marketing_name", marketingName));
    ASSERT_EQ(marketingName, "AABBAABBAABBAABBAA");
    std::string model;
    ASSERT_TRUE(metaFormat.GetStringValue("com.abababa.model", model));
    ASSERT_EQ(model, "ABABABAB");
    std::string version;
    ASSERT_TRUE(metaFormat.GetStringValue("com.abababa.version", version));
    ASSERT_EQ(version, "12");
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_FUNC_0200
 * @tc.name      : check video format from single
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_ILLEGAL_FUNC_0200, TestSize.Level0)
{
    fd_ = open(g_singleHevcPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_singleHevcPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);

    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longitude = 0.0;
    ASSERT_TRUE(format.GetFloatValue(Media::Tag::MEDIA_LONGITUDE, longitude));
    ASSERT_EQ(longitude, float(120.000000));

    float latitude = 0.0;
    ASSERT_TRUE(format.GetFloatValue(Media::Tag::MEDIA_LATITUDE, latitude));
    ASSERT_EQ(latitude, float(30.000000));

    Format metaFormat;
    ret = source->GetUserMeta(metaFormat);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_FUNC_0300
 * @tc.name      : check video format from rk
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_ILLEGAL_FUNC_0300, TestSize.Level2)
{
    fd_ = open(g_singleRkPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_singleRkPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longitude = 0.0;
    ASSERT_TRUE(format.GetFloatValue(Media::Tag::MEDIA_LONGITUDE, longitude));
    ASSERT_EQ(longitude, float(0.000000));

    float latitude = 0.0;
    ASSERT_TRUE(format.GetFloatValue(Media::Tag::MEDIA_LATITUDE, latitude));
    ASSERT_EQ(latitude, float(0.000000));
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_FUNC_0400
 * @tc.name      : check video format from xm
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_ILLEGAL_FUNC_0400, TestSize.Level2)
{
    fd_ = open(g_xmPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_xmPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    float longitude = 0.0;
    ASSERT_TRUE(format.GetFloatValue(Media::Tag::MEDIA_LONGITUDE, longitude));
    ASSERT_EQ(longitude, float(120.201302));
    float latitude = 0.0;
    ASSERT_TRUE(format.GetFloatValue(Media::Tag::MEDIA_LATITUDE, latitude));
    ASSERT_EQ(latitude, float(30.188101));
    Format metaFormat;
    ret = source->GetUserMeta(metaFormat);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_FUNC_0500
 * @tc.name      : check video format from double vivid
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_ILLEGAL_FUNC_0500, TestSize.Level2)
{
    fd_ = open(g_doubleVividPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_doubleVividPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);

    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longitude = 0.0;
    ASSERT_TRUE(format.GetFloatValue(Media::Tag::MEDIA_LONGITUDE, longitude));
    ASSERT_EQ(longitude, float(120.201401));

    float latitude = 0.0;
    ASSERT_TRUE(format.GetFloatValue(Media::Tag::MEDIA_LATITUDE, latitude));
    ASSERT_EQ(latitude, float(30.188101));

    string genre;
    ASSERT_TRUE(format.GetStringValue(Media::Tag::MEDIA_GENRE, genre));
    ASSERT_EQ(genre, "AABBAA{marketing-name:\"AABBAABBAABBAABABA\",param-AA-ext"\
    ":{camera-position-tag:2},param-use-tag:TypeNormalVideo}");
}

/**
 * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0010
 * @tc.name      : get has_timed_meta without meta video track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0010, TestSize.Level0)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 0);
}

/**
 * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0020
 * @tc.name      : get has_timed_meta without meta audio track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0020, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/NoTimedmetadataAudio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 0);
}

/**
 * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0030
 * @tc.name      : get has_timed_meta without meta video+audio track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0030, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 0);
}

/**
 * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0040
 * @tc.name      : get has_timed_meta with meta video track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0040, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/TimedmetadataVideo.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 1);
}

/**
 * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0050
 * @tc.name      : get has_timed_meta with meta audio track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0050, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/TimedmetadataAudio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 1);
}

/**
 * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0060
 * @tc.name      : get has_timed_meta with meta video+audio track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0060, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track0.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 1);
}

/**
 * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0070
 * @tc.name      : demuxer timed metadata with 1 meta track and video track file-meta track at 0
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0070, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track0.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 1);
    ASSERT_EQ(demuxerSample->CheckTimedMetaFormat(0, 1), 0);
    ASSERT_EQ(demuxerSample->CheckTimedMeta(0), 0);
}

/**
 * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0080
 * @tc.name      : demuxer timed metadata with 1 meta track and video track file-meta track at 1
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0080, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track1.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 1);
    ASSERT_EQ(demuxerSample->CheckTimedMetaFormat(1, 0), 0);
    ASSERT_EQ(demuxerSample->CheckTimedMeta(1), 0);
}

/**
 * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0090
 * @tc.name      : demuxer timed metadata with 1 meta track and video track file-meta track at 2
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0090, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track2.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 1);
    ASSERT_EQ(demuxerSample->CheckTimedMetaFormat(2, 0), 0);
    ASSERT_EQ(demuxerSample->CheckTimedMeta(2), 0);
}

/**
 * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0100
 * @tc.name      : demuxer timed metadata with 2 meta track and video track file
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0100, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata2Track2.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 1);
    ASSERT_EQ(demuxerSample->CheckTimedMetaFormat(2, 0), 0);
    ASSERT_EQ(demuxerSample->CheckTimedMetaFormat(3, 0), 0);
    ASSERT_EQ(demuxerSample->CheckTimedMeta(3), 0);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0010
 * @tc.name      : GetFrameIndexByPresentationTimeUs with ltr h264 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0010, TestSize.Level0)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ltr_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0020
 * @tc.name      : GetFrameIndexByPresentationTimeUs with one I frame h264 video track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0020, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4", true),
        AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0030
 * @tc.name      : GetFrameIndexByPresentationTimeUs with one I frame h264 video and audio track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0030, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_one_i_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0040
 * @tc.name      : GetFrameIndexByPresentationTimeUs with all I frame h264 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0040, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_all_i_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0050
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IPB h264 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0050, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ipb_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0060
 * @tc.name      : GetFrameIndexByPresentationTimeUs with 2 layer frame h264 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0060, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0070
 * @tc.name      : GetFrameIndexByPresentationTimeUs with ltr h265 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0070, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0080
 * @tc.name      : GetFrameIndexByPresentationTimeUs with one I frame h265 video track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0080, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4", true),
        AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0090
 * @tc.name      : GetFrameIndexByPresentationTimeUs with one I frame h265 video and audio track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0090, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_one_i_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0100
 * @tc.name      : GetFrameIndexByPresentationTimeUs with all I frame h265 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0100, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0110
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IPB frame h265 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0110, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0120
 * @tc.name      : GetFrameIndexByPresentationTimeUs with 2 layer frame frame h265 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0120, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0130
 * @tc.name      : GetPresentationTimeUsByFrameIndex with ltr h264 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0130, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ltr_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0140
 * @tc.name      : GetPresentationTimeUsByFrameIndex with one I frame h264 video track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0140, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4", true),
        AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0150
 * @tc.name      : GetPresentationTimeUsByFrameIndex with one I frame h264 video and audio track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0150, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_one_i_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0160
 * @tc.name      : GetPresentationTimeUsByFrameIndex with all I frame h264 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0160, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_all_i_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0170
 * @tc.name      : GetPresentationTimeUsByFrameIndex with IPB h264 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0170, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ipb_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0180
 * @tc.name      : GetPresentationTimeUsByFrameIndex with 2 layer frame h264 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0180, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0190
 * @tc.name      : GetPresentationTimeUsByFrameIndex with ltr h265 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0190, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0200
 * @tc.name      : GetPresentationTimeUsByFrameIndex with one I frame h265 video track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0200, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4", true),
        AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0210
 * @tc.name      : GetPresentationTimeUsByFrameIndex with one I frame h265 video and audio track source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0210, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_one_i_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0220
 * @tc.name      : GetPresentationTimeUsByFrameIndex with all I frame h265 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0220, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0230
 * @tc.name      : GetPresentationTimeUsByFrameIndex with IPB h265 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0230, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0240
 * @tc.name      : GetPresentationTimeUsByFrameIndex with 2 layer frame h265 source
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0240, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckPtsFromIndex(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0250
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IP h264 source, pts is a close to left value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0250, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseLeft = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0260
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IP h264 source, pts is a close to right value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0260, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseRight = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0270
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IP h264 source, pts is a close to center value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0270, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseCenter = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0280
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IP h265 source, pts is a close to left value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0280, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseLeft = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0290
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IP h265 source, pts is a close to right value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0290, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseRight = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0300
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IP h265 source, pts is a close to center value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0300, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseCenter = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0310
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IPB h264 source, pts is a close to left value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0310, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseLeft = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ipb_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0320
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IPB h264 source, pts is a close to right value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0320, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseRight = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ipb_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0330
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IPB h264 source, pts is a close to center value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0330, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseCenter = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ipb_frame_avc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0340
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IPB h265 source, pts is a close to left value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0340, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseLeft = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0350
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IP h265 source, pts is a close to right value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0350, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseRight = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_PTS_INDEX_INNER_FUNC_0360
 * @tc.name      : GetFrameIndexByPresentationTimeUs with IPB h265 source, pts is a close to center value
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_PTS_INDEX_INNER_FUNC_0360, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->isPtsCloseCenter = true;
    demuxerSample->isPtsExist = true;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->ReadSampleAndSave(), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CheckIndexFromPts(), AVCS_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_APE_INNER_FUNC_0050
 * @tc.name      : APE version >= 3.98 && <= 3.99
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_APE_INNER_FUNC_0050, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    ASSERT_EQ(demuxerSample->CheckApeSourceData("/data/test/media/audio/ape.ape", g_apeVersion), true);
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0010
 * @tc.name      : check Mp4 AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0010, TestSize.Level0)
{
    fd_ = open(g_aigcMp4Path.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_aigcMp4Path.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_TRUE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
    ASSERT_EQ(AIGC_TEST_1, stringVal);
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0010
 * @tc.name      : check Flv AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0020, TestSize.Level1)
{
    fd_ = open(g_aigcFlvPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_aigcFlvPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_TRUE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
    ASSERT_EQ(AIGC_TEST_1, stringVal);
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0030
 * @tc.name      : check Mkv AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0030, TestSize.Level1)
{
    fd_ = open(g_aigcMkvPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_aigcMkvPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_TRUE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
    ASSERT_EQ(AIGC_TEST_1, stringVal);
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0040
 * @tc.name      : check Mov AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0040, TestSize.Level1)
{
    fd_ = open(g_aigcMovPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_aigcMovPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_TRUE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
    ASSERT_EQ(AIGC_TEST_1, stringVal);
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0050
 * @tc.name      : check M4v AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0050, TestSize.Level1)
{
    fd_ = open(g_aigcM4vPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_aigcM4vPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_TRUE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
    ASSERT_EQ(AIGC_TEST_1, stringVal);
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0060
 * @tc.name      : check M4a AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0060, TestSize.Level1)
{
    fd_ = open(g_aigcM4aPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_aigcM4aPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_TRUE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
    ASSERT_EQ(AIGC_TEST_1, stringVal);
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0070
 * @tc.name      : check Avi AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0070, TestSize.Level1)
{
    fd_ = open(g_aigcAviPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_aigcAviPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_TRUE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
    ASSERT_EQ(AIGC_TEST_2, stringVal);
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0080
 * @tc.name      : check Mp4_256 AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0080, TestSize.Level2)
{
    fd_ = open(g_aigcMp4256Path.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_aigcMp4256Path.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_TRUE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
    ASSERT_EQ(AIGC_TEST_3, stringVal);
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0090
 * @tc.name      : check Mp4_258 AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0090, TestSize.Level2)
{
    fd_ = open(g_aigcMp4258Path.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_aigcMp4258Path.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    float longval = 0.0;
    int32_t intval;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    std::string stringVal = "";
    ASSERT_TRUE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
    ASSERT_EQ(AIGC_TEST_4, stringVal);
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0100
 * @tc.name      : Mp4 不含AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0100, TestSize.Level3)
{
    fd_ = open(g_doubleHevcPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_doubleHevcPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_FALSE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0110
 * @tc.name      : Flv 不含AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0110, TestSize.Level3)
{
    fd_ = open(g_flvPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_flvPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_FALSE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0120
 * @tc.name      : Mkv 不含AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0120, TestSize.Level3)
{
    fd_ = open(g_mkvPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_mkvPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_FALSE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0130
 * @tc.name      : Mov 不含AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0130, TestSize.Level3)
{
    fd_ = open(g_movPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_movPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_FALSE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0140
 * @tc.name      : M4v 不含AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0140, TestSize.Level3)
{
    fd_ = open(g_m4vPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_m4vPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_FALSE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0150
 * @tc.name      : M4a 不含AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0150, TestSize.Level3)
{
    fd_ = open(g_m4aPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_m4aPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_FALSE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
}

/**
 * @tc.number    : DEMUXER_AIGC_INNER_FUNC_0160
 * @tc.name      : Avi 不含AIGC info
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_AIGC_INNER_FUNC_0160, TestSize.Level3)
{
    fd_ = open(g_aviPath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(g_aviPath.c_str(), &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(nullptr, source);
    Format format;
    int32_t ret = source->GetSourceFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    float longval = 0.0;
    int32_t intval = 0;
    ASSERT_FALSE(format.GetFloatValue(MediaDescriptionKey::MD_KEY_AIGC, longval));
    ASSERT_FALSE(format.GetIntValue(MediaDescriptionKey::MD_KEY_AIGC, intval));
    string stringVal = "";
    ASSERT_FALSE(format.GetStringValue(MediaDescriptionKey::MD_KEY_AIGC, stringVal));
}

/**
 * @tc.number    : DEMUXER_COVER_INNER_FUNC_0010
 * @tc.name      : demuxer track with audio\video\cover
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_COVER_INNER_FUNC_0010, TestSize.Level2)
{
    int32_t trackCount = 0;
    Format trackFormat;
    Format sourceFormat;
    uint8_t *addr = nullptr;
    size_t buffSize = 0;
    fd_ = open("/data/test/media/cover_jpg.mp4", O_RDONLY);
    struct stat fileStatus {};
    if (stat("/data/test/media/cover_jpg.mp4", &fileStatus) == 0) {
        size_ = static_cast<int64_t>(fileStatus.st_size);
    }
    source = AVSourceFactory::CreateWithFD(fd_, 0, size_);
    ASSERT_NE(nullptr, source);
    demuxer = AVDemuxerFactory::CreateWithSource(source);
    ASSERT_NE(nullptr, demuxer);
    int32_t ret = source->GetSourceFormat(sourceFormat);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    sourceFormat.GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, trackCount);
    ASSERT_EQ(3, trackCount);
    for (int32_t i = 0; i < trackCount; i++) {
        ret = demuxer->SelectTrackByID(i);
    }
    ASSERT_EQ(true, CreateBufferSize());
    while (!isVideoEosFlagForSave_ || !isAudioEosFlagForSave_) {
        for (int32_t i = 0; i < trackCount; i++) {
            ret = source->GetTrackFormat(trackFormat, i);
            if (ret != 0) {
                cout << "GetTrackFormat failed!!!!" << endl;
            }
            if (((i == videoTrackIdx_) && isVideoEosFlagForSave_) ||
                ((i == audioTrackIdx_) && isAudioEosFlagForSave_)) {
                continue;
            }
            if (i == 2) {
                ASSERT_EQ(true, trackFormat.GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &addr, buffSize));
                continue;
            }
            ret = demuxer->ReadSampleBuffer(i, avBuf_);
            if (ret != 0) {
                cout << "ReadSampleBuffer failed" << i << endl;
                }
            GetFrameNum(i);
        }
    }
    ASSERT_EQ(602, videoIndexForRead_);
    ASSERT_EQ(433, audioIndexForRead_);
    close(fd_);
    fd_ = -1;
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_LOCAL_0100
 * @tc.name      : check mp4 hdr type for 264, local
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_LOCAL_0100, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/mp3_h264.mp4", true), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, false);
        ASSERT_EQ(demuxerSample->ReadSample(373, 468), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_URI_0100
 * @tc.name      : check mp4 hdr type for 264, uri
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_URI_0100, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("http://127.0.0.1:46666/mp3_h264.mp4", false), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, false);
        ASSERT_EQ(demuxerSample->ReadSample(373, 468), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_LOCAL_0200
 * @tc.name      : check mp4 hdr type for hdr10, local
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_LOCAL_0200, TestSize.Level0)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/demuxer_parser_hdr_1_hevc.mp4", true), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::HDR_10));
        ASSERT_EQ(demuxerSample->ReadSample(242, 235), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_URI_0200
 * @tc.name      : check mp4 hdr type for hdr10, uri
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_URI_0200, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("http://127.0.0.1:46666/demuxer_parser_hdr_1_hevc.mp4", false),
            AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::HDR_10));
        ASSERT_EQ(demuxerSample->ReadSample(242, 235), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_LOCAL_0300
 * @tc.name      : check mp4 hdr type for hlg, local
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_LOCAL_0300, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/hlg.mp4", true), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::HLG));
        ASSERT_EQ(demuxerSample->ReadSample(80, 0), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_URI_0300
 * @tc.name      : check mp4 hdr type for hlg, uri
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_URI_0300, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("http://127.0.0.1:46666/hlg.mp4", false), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::HLG));
        ASSERT_EQ(demuxerSample->ReadSample(80, 0), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_LOCAL_0400
 * @tc.name      : check mp4 hdr type for hlg hdrvivid, local
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_LOCAL_0400, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/hlgHdrVivid.mp4", true), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::HDR_VIVID));
        ASSERT_EQ(demuxerSample->ReadSample(66, 103), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_URI_0400
 * @tc.name      : check mp4 hdr type for hlg hdrvivid, uri
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_URI_0400, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("http://127.0.0.1:46666/hlgHdrVivid.mp4", false), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::HDR_VIVID));
        ASSERT_EQ(demuxerSample->ReadSample(66, 103), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_LOCAL_0500
 * @tc.name      : check mp4 hdr type for pq hdrvivid, local
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_LOCAL_0500, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/pqHdrvivid.mp4", true), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::HDR_VIVID));
        ASSERT_EQ(demuxerSample->ReadSample(3000, 0), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_URI_0500
 * @tc.name      : check mp4 hdr type for pq hdrvivid, uri
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_URI_0500, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("http://127.0.0.1:46666/pqHdrvivid.mp4", false), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::HDR_VIVID));
        ASSERT_EQ(demuxerSample->ReadSample(3000, 0), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_LOCAL_0600
 * @tc.name      : check mp4 hdr type for Dolby Vision, local
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_LOCAL_0600, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/dolbyvision.MOV", true), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::HDR_10));
        ASSERT_EQ(demuxerSample->ReadSample(324, 468), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_URI_0600
 * @tc.name      : check mp4 hdr type for Dolby Vision, uri
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_URI_0600, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("http://127.0.0.1:46666/dolbyvision.MOV", false), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::HDR_10));
        ASSERT_EQ(demuxerSample->ReadSample(324, 468), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_LOCAL_0700
 * @tc.name      : check mp4 hdr type for SDR, local
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_LOCAL_0700, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/mp3_h265.mp4", true), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::NONE));
        ASSERT_EQ(demuxerSample->ReadSample(372, 468), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_URI_0700
 * @tc.name      : check mp4 hdr type for SDR, uri
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_URI_0700, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("http://127.0.0.1:46666/mp3_h265.mp4", false), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, true);
        ASSERT_EQ(demuxerSample->hdrType, static_cast<int32_t>(Media::Plugins::HDRType::NONE));
        ASSERT_EQ(demuxerSample->ReadSample(372, 468), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_LOCAL_0800
 * @tc.name      : check mkv hdr type, local
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_LOCAL_0800, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/mp3_h265.mkv", true), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, false);
        ASSERT_EQ(demuxerSample->ReadSample(372, 468), AVCS_ERR_OK);
    }
}

/**
 * @tc.number    : DEMUXER_HDR10_HLG_META_URI_0800
 * @tc.name      : check mkv hdr type, uri
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerInnerFuncNdkTest, DEMUXER_HDR10_HLG_META_URI_0800, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        ASSERT_EQ(demuxerSample->InitWithFile("http://127.0.0.1:46666/mp3_h265.mkv", false), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->getHdrMetadata, false);
        ASSERT_EQ(demuxerSample->ReadSample(372, 468), AVCS_ERR_OK);
    }
}
} // namespace