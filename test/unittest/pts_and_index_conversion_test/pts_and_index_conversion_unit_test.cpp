/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <string>
#include <malloc.h>
#include <sys/stat.h>
#include <cinttypes>
#include <fcntl.h>
#include <list>
#include <cmath>
#include <sys/types.h>
#include <fstream>
#include <memory>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "pts_and_index_conversion_unit_test.h"

#define LOCAL true
namespace OHOS::Media {

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::TimeAndIndex;
using TimeAndIndexConversion = OHOS::Media::TimeAndIndex::TimeAndIndexConversion;

string g_flvPath = string("/data/test/media/h264.flv");
string g_ptsConversionPath = string("/data/test/media/camera_info_parser.mp4");

void PtsAndIndexConversionTest::SetUpTestCase(void)
{
}

void PtsAndIndexConversionTest::TearDownTestCase(void)
{
}

void PtsAndIndexConversionTest::SetUp()
{
}

void PtsAndIndexConversionTest::TearDown()
{
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }

    if (index_ != 0) {
        index_ = 0;
    }

    if (trackIndex_ != 0) {
        trackIndex_ = 0;
    }

    if (relativePresentationTimeUs_ != 0) {
        relativePresentationTimeUs_ = 0;
    }

    initStatus_ = false;
}

void PtsAndIndexConversionTest::InitResource(const std::string &srtpath, Status code)
{
    int64_t fileSize = 0;
    if (!srtpath.empty()) {
        struct stat fileStatus {};
        if (stat(srtpath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    fd_ = open(srtpath.c_str(), O_RDONLY);
    if (fd_ < 0) {
        return;
    }
    std::string uri = "fd://" + std::to_string(fd_) + "?offset=0&size=" + std::to_string(fileSize);
    TimeAndIndexConversions_ = std::make_shared<TimeAndIndexConversion>();
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(uri);
    ASSERT_EQ(TimeAndIndexConversions_->SetDataSource(mediaSource), code);
    initStatus_ = true;
}

/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1000
 * @tc.desc: Get index by pts(audio track)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetIndexByRelativePresentationTimeUs_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    relativePresentationTimeUs_ = 69659;
    ASSERT_EQ(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 3);
}

/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1001
 * @tc.desc: Get index by pts(video track)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetIndexByRelativePresentationTimeUs_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    trackIndex_ = 1;
    relativePresentationTimeUs_ = 66666;
    ASSERT_EQ(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 4);
}

/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1002
 * @tc.desc: Get index by pts(not MP4)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetIndexByRelativePresentationTimeUs_1002, TestSize.Level1)
{
    InitResource(g_flvPath, Status::ERROR_UNSUPPORTED_FORMAT);
    ASSERT_TRUE(initStatus_);
    relativePresentationTimeUs_ = 69659;
    ASSERT_NE(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 0);
}

/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1003
 * @tc.desc: Get index by pts(non-standard pts & different track)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetIndexByRelativePresentationTimeUs_1003, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    trackIndex_ = 1;
    relativePresentationTimeUs_ = 166666;
    ASSERT_EQ(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 10);
}

/**
 * @tc.name: Demuxer_GetRelativePresentationTimeUsByIndex_1000
 * @tc.desc: get pts by frameIndex(audio track)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetRelativePresentationTimeUsByIndex_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    index_ = 2;
    ASSERT_EQ(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 46439);
}

/**
 * @tc.name: Demuxer_GetRelativePresentationTimeUsByIndex_1001
 * @tc.desc: get pts by frameIndex(video track)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetRelativePresentationTimeUsByIndex_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    index_ = 2;
    trackIndex_ = 1;
    ASSERT_EQ(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 33333);
}

/**
 * @tc.name: Demuxer_GetRelativePresentationTimeUsByIndex_1002
 * @tc.desc: get pts by frameIndex(not MP4)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetRelativePresentationTimeUsByIndex_1002, TestSize.Level1)
{
    InitResource(g_flvPath, Status::ERROR_UNSUPPORTED_FORMAT);
    ASSERT_TRUE(initStatus_);
    index_ = 10;
    ASSERT_NE(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 0);
}

/**
 * @tc.name: Demuxer_PtsAndFrameIndexConversion_1000
 * @tc.desc: pts and frameIndex convertion test(pts -> frameIndex -> pts)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_PtsAndFrameIndexConversion_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    relativePresentationTimeUs_ = 92879;
    ASSERT_EQ(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 4);

    relativePresentationTimeUs_ = 0;
    ASSERT_EQ(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 92879);
}

/**
 * @tc.name: Demuxer_PtsAndFrameIndexConversion_1001
 * @tc.desc: pts and frameIndex convertion test(frameIndex -> pts -> frameIndex)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_PtsAndFrameIndexConversion_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    index_ = 4;
    ASSERT_EQ(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 92879);

    index_ = 0;
    ASSERT_EQ(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 4);
}

/**
 * @tc.name: Demuxer_PTSOutOfRange_1000
 * @tc.desc: pts out of range
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_PTSOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    relativePresentationTimeUs_ = 999999999;
    ASSERT_NE(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 0);
}

/**
 * @tc.name: Demuxer_IndexOutOfRange_1000
 * @tc.desc: Index out of range
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_IndexOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    index_ = 9999999;
    ASSERT_NE(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 0);
}

/**
 * @tc.name: Demuxer_TrackOutOfRange_1000
 * @tc.desc: Track out of range
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_TrackOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    trackIndex_ = 99;
    ASSERT_NE(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 0);
    ASSERT_EQ(index_, 0);
}
}