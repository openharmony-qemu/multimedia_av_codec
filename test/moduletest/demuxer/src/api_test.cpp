/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"

#include <iostream>
#include <stdio.h>
#include <string>
#include <fcntl.h>

namespace OHOS {
namespace Media {
class DemuxerApiNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

public:
    int fd1;
    int64_t size;
};
static int32_t g_width = 3840;
static int32_t g_height = 2160;
static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
const char *FILE1 = "/data/test/media/01_video_audio.mp4";
const char *FILE2 = "/data/test/media/10sec.mp4";

const char *URI2 = "http://192.168.3.11:8080/share/audio/AAC_48000_1.aac";
const char *URI1 = "http://192.168.3.11:8080/share/audio/MP3_48000_1.mp3";

void DemuxerApiNdkTest::SetUpTestCase() {}
void DemuxerApiNdkTest::TearDownTestCase() {}

void DemuxerApiNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    fd1 = open(FILE1, O_RDONLY);
    if (FILE1 != nullptr) {
        struct stat fileStatus {};
        if (stat(FILE1, &fileStatus) == 0) {
            size = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    std::cout << fd1 << "----------" << FILE1 << "=====" << size << std::endl;
}

void DemuxerApiNdkTest::TearDown()
{
    if (memory != nullptr) {
        OH_AVMemory_Destroy(memory);
        memory = nullptr;
    }
    if (source != nullptr) {
        OH_AVSource_Destroy(source);
        source = nullptr;
    }
    if (demuxer != nullptr) {
        OH_AVDemuxer_Destroy(demuxer);
        demuxer = nullptr;
    }
    close(fd1);
}

} // namespace Media
} // namespace OHOS

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_0100
 * @tc.name      : OH_AVSource_CreateWithURI para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_0100, TestSize.Level2)
{
    source = OH_AVSource_CreateWithURI(nullptr);
    ASSERT_EQ(nullptr, source);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_0200
 * @tc.name      : OH_AVSource_CreateWithFD para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_0200, TestSize.Level2)
{
    // fd must bigger than 2
    source = OH_AVSource_CreateWithFD(2, 0, 0);
    ASSERT_EQ(nullptr, source);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_0700
 * @tc.name      : OH_AVSource_CreateWithFD para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_0700, TestSize.Level2)
{
    // fd must bigger than 2
    source = OH_AVSource_CreateWithFD(3, 0, -1);
    ASSERT_EQ(nullptr, source);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_0800
 * @tc.name      : OH_AVSource_CreateWithFD para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_0800, TestSize.Level2)
{
    // fd must bigger than 2
    source = OH_AVSource_CreateWithFD(3, -1, 1);
    ASSERT_EQ(nullptr, source);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_0300
 * @tc.name      : OH_AVSource_Destroy para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_0300, TestSize.Level2)
{
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVSource_Destroy(nullptr));
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_0400
 * @tc.name      : OH_AVSource_GetSourceFormat para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_0400, TestSize.Level2)
{
    OH_AVFormat *format = OH_AVSource_GetSourceFormat(nullptr);
    ASSERT_EQ(nullptr, format);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_0500
 * @tc.name      : OH_AVSource_GetTrackFormat para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_0500, TestSize.Level2)
{
    OH_AVFormat *format = OH_AVSource_GetTrackFormat(nullptr, 0);
    ASSERT_EQ(nullptr, format);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_0600
 * @tc.name      : OH_AVSource_GetSourceFormat para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_0600, TestSize.Level2)
{
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(nullptr, source);
    OH_AVFormat *format = OH_AVSource_GetTrackFormat(source, -1);
    ASSERT_EQ(nullptr, format);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_2300
 * @tc.name      : OH_AVSource_CreateWithFD para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_2300, TestSize.Level2)
{
    source = OH_AVSource_CreateWithFD(fd1, 0, 0);
    ASSERT_NE(nullptr, source);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_0900
 * @tc.name      : OH_AVDemuxer_CreateWithSource para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_0900, TestSize.Level2)
{
    demuxer = OH_AVDemuxer_CreateWithSource(nullptr);
    ASSERT_EQ(nullptr, demuxer);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_1000
 * @tc.name      : OH_AVDemuxer_Destroy para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_1000, TestSize.Level2)
{
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_Destroy(nullptr));
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_1100
 * @tc.name      : OH_AVDemuxer_SelectTrackByID para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_1100, TestSize.Level2)
{
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_SelectTrackByID(nullptr, 0));
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_1200
 * @tc.name      : OH_AVDemuxer_SelectTrackByID para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_1200, TestSize.Level2)
{
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(nullptr, source);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_SelectTrackByID(demuxer, -1));
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_1300
 * @tc.name      : OH_AVDemuxer_UnselectTrackByID para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_1300, TestSize.Level2)
{
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_UnselectTrackByID(nullptr, 0));
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_1400
 * @tc.name      : OH_AVDemuxer_UnselectTrackByID para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_1400, TestSize.Level2)
{
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(nullptr, source);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(nullptr, demuxer);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_UnselectTrackByID(demuxer, -1)); // unselect ok
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_1500
 * @tc.name      : OH_AVDemuxer_ReadSample para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_1500, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr bufferAttr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_ReadSample(nullptr, trackIndex, memory, &bufferAttr));
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_1600
 * @tc.name      : OH_AVDemuxer_ReadSample para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_1600, TestSize.Level2)
{
    uint32_t trackIndex = -1;
    OH_AVCodecBufferAttr bufferAttr;
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(nullptr, source);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(nullptr, demuxer);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &bufferAttr));
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_1700
 * @tc.name      : OH_AVDemuxer_ReadSample para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_1700, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    OH_AVMemory *memory1 = OH_AVMemory_Create(2);
    OH_AVCodecBufferAttr bufferAttr;
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(nullptr, source);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(nullptr, demuxer);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    ASSERT_EQ(AV_ERR_NO_MEMORY, OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory1, &bufferAttr));

    OH_AVMemory_Destroy(memory1);
    memory1 = nullptr;
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_8600
 * @tc.name      : OH_AVDemuxer_ReadSample para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_2400, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    OH_AVMemory *memory1 = OH_AVMemory_Create(2);
    OH_AVCodecBufferAttr bufferAttr;
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(nullptr, source);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(nullptr, demuxer);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory1, &bufferAttr));

    OH_AVMemory_Destroy(memory1);
    memory1 = nullptr;
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_1700
 * @tc.name      : OH_AVDemuxer_ReadSample para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_2500, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr bufferAttr;
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(nullptr, source);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(nullptr, demuxer);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_ReadSample(demuxer, trackIndex, nullptr, &bufferAttr));
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_1800
 * @tc.name      : OH_AVDemuxer_ReadSample para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_1800, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(nullptr, source);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(nullptr, demuxer);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, nullptr));
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_1900
 * @tc.name      : OH_AVDemuxer_SeekToTime para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_1900, TestSize.Level2)
{
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_SeekToTime(nullptr, 1, SEEK_MODE_NEXT_SYNC));
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_2100
 * @tc.name      : OH_AVMemory_Create para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_2100, TestSize.Level2)
{
    memory = OH_AVMemory_Create(-1);
    ASSERT_EQ(nullptr, memory);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_2200
 * @tc.name      : OH_AVMemory_Destroy para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_ILLEGAL_PARA_2200, TestSize.Level2)
{
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVMemory_Destroy(nullptr));
}

/**
 * @tc.number    : DEMUXER_API_0100
 * @tc.name      : OH_AVSource_CreateWithURI Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_0100, TestSize.Level2)
{
    OH_AVSource *source1 = OH_AVSource_CreateWithURI(const_cast<char *>(URI1));
    cout << URI1 << "-----------------------" << endl;
    ASSERT_NE(source1, nullptr);
    OH_AVSource *source2 = OH_AVSource_CreateWithURI(const_cast<char *>(URI2));
    cout << URI2 << "-----------------------" << endl;
    ASSERT_NE(source2, nullptr);
    ASSERT_EQ(AV_ERR_OK, OH_AVSource_Destroy(source1));
    source1 = nullptr;
    ASSERT_EQ(AV_ERR_OK, OH_AVSource_Destroy(source2));
    source2 = nullptr;
}

/**
 * @tc.number    : DEMUXER_API_0200
 * @tc.name      : OH_AVSource_CreateWithFD Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_0200, TestSize.Level2)
{
    OH_AVSource *source1 = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(source1, nullptr);
    int fd2 = open(FILE2, O_RDONLY);
    int64_t size2 = 0;

    struct stat fileStatus {};
    if (stat(FILE2, &fileStatus) == 0) {
        size2 = static_cast<int64_t>(fileStatus.st_size);
    }

    OH_AVSource *source2 = OH_AVSource_CreateWithFD(fd2, 0, size2);
    cout << size2 << "------------------" << endl;
    ASSERT_NE(source2, nullptr);
    ASSERT_EQ(AV_ERR_OK, OH_AVSource_Destroy(source1));
    ASSERT_EQ(AV_ERR_OK, OH_AVSource_Destroy(source2));
    source1 = nullptr;
    source2 = nullptr;
    close(fd2);
}

/**
 * @tc.number    : DEMUXER_API_0300
 * @tc.name      : OH_AVSource_Destroy Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_0300, TestSize.Level2)
{
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_EQ(AV_ERR_OK, OH_AVSource_Destroy(source));
    source = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVSource_Destroy(source));
}

/**
 * @tc.number    : DEMUXER_API_0500
 * @tc.name      : OH_AVSource_GetSourceFormat Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_0500, TestSize.Level2)
{
    OH_AVFormat *format;
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(source, nullptr);
    format = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(format, nullptr);
    format = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(format, nullptr);
}

/**
 * @tc.number    : DEMUXER_API_0700
 * @tc.name      : OH_AVSource_GetTrackFormat Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_0700, TestSize.Level2)
{
    OH_AVFormat *format;
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(source, nullptr);

    format = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(format, nullptr);
    format = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(format, nullptr);
}

/**
 * @tc.number    : DEMUXER_API_1000
 * @tc.name      : OH_AVDemuxer_Destroy Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_1000, TestSize.Level2)
{
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_Destroy(demuxer));
    demuxer = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_Destroy(demuxer));
}

/**
 * @tc.number    : DEMUXER_API_1100
 * @tc.name      : OH_AVDemuxer_SelectTrackByID Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_1100, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_API_1200
 * @tc.name      : OH_AVDemuxer_UnselectTrackByID Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_1200, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_UnselectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_UnselectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_API_1300
 * @tc.name      : OH_AVDemuxer_ReadSample Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_1300, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCodecBufferAttr attr;
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_API_1400
 * @tc.name      : OH_AVDemuxer_SeekToTime Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_1400, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    uint32_t ms = 1000;
    source = OH_AVSource_CreateWithFD(fd1, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_SeekToTime(demuxer, ms, SEEK_MODE_NEXT_SYNC);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_SeekToTime(demuxer, ms, SEEK_MODE_NEXT_SYNC);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.number    : DEMUXER_API_1500
 * @tc.name      : OH_AVMemory_Create Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_2300, TestSize.Level2)
{
    memory = OH_AVMemory_Create(1024);
    ASSERT_NE(nullptr, memory);
    memory = OH_AVMemory_Create(2);
    ASSERT_NE(nullptr, memory);
}

/**
 * @tc.number    : DEMUXER_ILLEGAL_PARA_2200
 * @tc.name      : OH_AVMemory_Destroy para error
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerApiNdkTest, DEMUXER_API_2400, TestSize.Level2)
{
    memory = OH_AVMemory_Create(2);
    ASSERT_EQ(AV_ERR_OK, OH_AVMemory_Destroy(memory));
    memory = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVMemory_Destroy(memory));
}
