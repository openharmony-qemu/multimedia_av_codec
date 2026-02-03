/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "native_avmemory.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <cmath>
#include <thread>
namespace OHOS {
namespace Media {
class DemuxerOggFuncNdkTest : public testing::Test {
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
void DemuxerOggFuncNdkTest::SetUpTestCase() {}
void DemuxerOggFuncNdkTest::TearDownTestCase() {}
void DemuxerOggFuncNdkTest::SetUp() {}
void DemuxerOggFuncNdkTest::TearDown() {}
}
}

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

static int64_t GetFileSize(const char *filename)
{
    int64_t fileSize = 0;
    if (filename != nullptr) {
        struct stat fileStatus {};
        if (stat(filename, &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

/**
 * @tc.number    : DEMUXER_OGG_FUNCTION_0010
 * @tc.name      : demux ogg and get english title from track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerOggFuncNdkTest, DEMUXER_OGG_FUNCTION_0010, TestSize.Level0)
{
    const char *file = "/data/test/media/OGG_48000_1.ogg";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    OH_AVDemuxer *demuxer = OH_AVDemuxer_CreateWithSource(source);
    EXPECT_NE(demuxer, nullptr);
    OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    const char *title = nullptr;
    OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_TITLE, &title);
    EXPECT_NE(title, nullptr);
    EXPECT_TRUE(!strcmp(title, "test"));
    OH_AVFormat_Destroy(trackFormat);
    OH_AVDemuxer_Destroy(demuxer);
    OH_AVSource_Destroy(source);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_OGG_FUNCTION_0020
 * @tc.name      : demux ogg and get english title from track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerOggFuncNdkTest, DEMUXER_OGG_FUNCTION_0020, TestSize.Level0)
{
    const char *file = "/data/test/media/OGG_48000_1.ogg";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    OH_AVDemuxer *demuxer = OH_AVDemuxer_CreateWithSource(source);
    EXPECT_NE(demuxer, nullptr);
    OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    const char *album = nullptr;
    OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_ALBUM, &album);
    EXPECT_NE(album, nullptr);
    EXPECT_TRUE(!strcmp(album, "media"));
    OH_AVFormat_Destroy(trackFormat);
    OH_AVDemuxer_Destroy(demuxer);
    OH_AVSource_Destroy(source);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_OGG_FUNCTION_0030
 * @tc.name      : demux ogg and get english title from track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerOggFuncNdkTest, DEMUXER_OGG_FUNCTION_0030, TestSize.Level0)
{
    const char *file = "/data/test/media/OGG_192K_2.ogg";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    OH_AVDemuxer *demuxer = OH_AVDemuxer_CreateWithSource(source);
    EXPECT_NE(demuxer, nullptr);
    OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    const char *title = nullptr;
    OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_TITLE, &title);
    EXPECT_NE(title, nullptr);
    EXPECT_TRUE(!strcmp(title, "老男孩"));
    OH_AVFormat_Destroy(trackFormat);
    OH_AVDemuxer_Destroy(demuxer);
    OH_AVSource_Destroy(source);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_OGG_FUNCTION_0040
 * @tc.name      : demux ogg and get english title from track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerOggFuncNdkTest, DEMUXER_OGG_FUNCTION_0040, TestSize.Level0)
{
    const char *file = "/data/test/media/OGG_192K_2.ogg";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    OH_AVDemuxer *demuxer = OH_AVDemuxer_CreateWithSource(source);
    EXPECT_NE(demuxer, nullptr);
    OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    const char *album = nullptr;
    OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_ALBUM, &album);
    EXPECT_NE(album, nullptr);
    EXPECT_TRUE(!strcmp(album, "父亲"));
    OH_AVFormat_Destroy(trackFormat);
    OH_AVDemuxer_Destroy(demuxer);
    OH_AVSource_Destroy(source);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_OGG_FUNCTION_0050
 * @tc.name      : demux ogg and get english title from track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerOggFuncNdkTest, DEMUXER_OGG_FUNCTION_0050, TestSize.Level0)
{
    const char *file = "/data/test/media/OGG_192K_2.ogg";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    OH_AVDemuxer *demuxer = OH_AVDemuxer_CreateWithSource(source);
    EXPECT_NE(demuxer, nullptr);
    OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    const char *artist = nullptr;
    OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_ARTIST, &artist);
    EXPECT_NE(artist, nullptr);
    EXPECT_TRUE(!strcmp(artist, "筷子兄弟"));
    OH_AVFormat_Destroy(trackFormat);
    OH_AVDemuxer_Destroy(demuxer);
    OH_AVSource_Destroy(source);
    close(fd);
}