/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <thread>
#include <chrono>
#include "mock/source.h"
#include "media_demuxer_pts_functions_unittest.h"

#define LOCAL true
namespace OHOS::Media {
using namespace std;
using namespace testing;
using namespace testing::ext;
const static int32_t NUM_TEST = 0;
const static int32_t NUM_TEST2 = 2;
const static int32_t ID_TEST = 1;
const static int32_t INVALID_PTS_DATA = -1;
const static int64_t MAX_PTS_DIFFER_THRESHOLD_US = 10000000;
void MediaDemuxerPtsUnitTest::SetUpTestCase(void)
{
}

void MediaDemuxerPtsUnitTest::TearDownTestCase(void)
{
}

void MediaDemuxerPtsUnitTest::SetUp()
{
    demuxerPtr_ = std::make_shared<MediaDemuxer>();
    demuxerPtr_->source_ = std::make_shared<Source>();
    demuxerPtr_->demuxerPluginManager_ = std::make_shared<DemuxerPluginManager>();
}

void MediaDemuxerPtsUnitTest::TearDown()
{
    if (demuxerPtr_ != nullptr) {
        if (demuxerPtr_->source_ != nullptr) {
            testing::Mock::VerifyAndClearExpectations(demuxerPtr_->source_.get());
            testing::Mock::AllowLeak(demuxerPtr_->source_.get());
        }
        demuxerPtr_->source_.reset();
        demuxerPtr_->subtitleSource_.reset();
        demuxerPtr_->demuxerPluginManager_.reset();
    }
    demuxerPtr_ = nullptr;
}

/**
 * @tc.name: Test HandleAutoMaintainPts  API
 * @tc.number: HandleAutoMaintainPts_001
 * @tc.desc: Test isAutoMaintainPts_ == false
 *           Test baseInfo == nullptr
 */
HWTEST_F(MediaDemuxerPtsUnitTest, HandleAutoMaintainPts_001, TestSize.Level0)
{
    ASSERT_NE(demuxerPtr_, nullptr);
    auto mockAvbuffer = std::make_shared<AVBuffer>();
    int32_t trackId = NUM_TEST;
    demuxerPtr_->isAutoMaintainPts_ = false;
    demuxerPtr_->HandleAutoMaintainPts(NUM_TEST, mockAvbuffer);
    demuxerPtr_->isAutoMaintainPts_ = true;
    demuxerPtr_->maintainBaseInfos_[trackId] = nullptr;
    demuxerPtr_->HandleAutoMaintainPts(NUM_TEST, mockAvbuffer);
    EXPECT_EQ(demuxerPtr_->maintainBaseInfos_[trackId], nullptr);
}

/**
 * @tc.name: Test HandleAutoMaintainPts  API
 * @tc.number: HandleAutoMaintainPts_002
 * @tc.desc: Test diff < 0
 *           Test baseInfo->segmentOffset == INVALID_PTS_DATA
 *           Test baseInfo->segmentOffset != offset
 */
HWTEST_F(MediaDemuxerPtsUnitTest, HandleAutoMaintainPts_002, TestSize.Level0)
{
    ASSERT_NE(demuxerPtr_, nullptr);
    auto mockAvbuffer = std::make_shared<AVBuffer>();
    int32_t trackId = NUM_TEST;
    mockAvbuffer->pts_ = NUM_TEST;
    demuxerPtr_->isAutoMaintainPts_ = true;
    auto maintainBaseInfo = std::make_shared<MediaDemuxer::MaintainBaseInfo>();
    maintainBaseInfo->lastPts = ID_TEST;
    maintainBaseInfo->segmentOffset = INVALID_PTS_DATA;
    maintainBaseInfo->basePts = NUM_TEST;
    EXPECT_CALL(*(demuxerPtr_->source_), GetSegmentOffset()).WillRepeatedly(Return(ID_TEST));
    demuxerPtr_->maintainBaseInfos_[trackId] = maintainBaseInfo;
    demuxerPtr_->HandleAutoMaintainPts(trackId, mockAvbuffer);
    EXPECT_EQ(maintainBaseInfo->segmentOffset, NUM_TEST);
}

/**
 * @tc.name: Test HandleAutoMaintainPts  API
 * @tc.number: HandleAutoMaintainPts_003
 * @tc.desc: Test baseInfo->segmentOffset != INVALID_PTS_DATA
 *           Test diff > MAX_PTS_DIFFER_THRESHOLD_US
 *           Test baseInfo->isLastPtsChange == true
 *           Test baseInfo->segmentOffset != offset
 *           Test Test baseInfo->isLastPtsChange == false
 */
HWTEST_F(MediaDemuxerPtsUnitTest, HandleAutoMaintainPts_003, TestSize.Level0)
{
    ASSERT_NE(demuxerPtr_, nullptr);
    auto mockAvbuffer = std::make_shared<AVBuffer>();
    mockAvbuffer->pts_ = MAX_PTS_DIFFER_THRESHOLD_US;
    int32_t trackId = NUM_TEST;
    demuxerPtr_->isAutoMaintainPts_ = true;
    auto maintainBaseInfo = std::make_shared<MediaDemuxer::MaintainBaseInfo>();
    maintainBaseInfo->lastPts = INVALID_PTS_DATA;
    maintainBaseInfo->segmentOffset = NUM_TEST2;
    maintainBaseInfo->isLastPtsChange = true;
    EXPECT_CALL(*(demuxerPtr_->source_), GetSegmentOffset()).WillRepeatedly(Return(ID_TEST));
    demuxerPtr_->maintainBaseInfos_[trackId] = maintainBaseInfo;
    demuxerPtr_->HandleAutoMaintainPts(trackId, mockAvbuffer);
    EXPECT_EQ(maintainBaseInfo->isLastPtsChange, false);

    // Test baseInfo->isLastPtsChange == false
    maintainBaseInfo->isLastPtsChange = false;
    maintainBaseInfo->lastPts = INVALID_PTS_DATA;
    maintainBaseInfo->segmentOffset = NUM_TEST2;
    maintainBaseInfo->lastPtsModifyedMax = ID_TEST;
    demuxerPtr_->HandleAutoMaintainPts(trackId, mockAvbuffer);
    EXPECT_NE(maintainBaseInfo->lastPtsModifyedMax, ID_TEST);
}

/**
 * @tc.name: Test HandleAutoMaintainPts  API
 * @tc.number: HandleAutoMaintainPts_004
 * @tc.desc: Test baseInfo->segmentOffset != INVALID_PTS_DATA && diff <= MAX_PTS_DIFFER_THRESHOLD_US
 */
HWTEST_F(MediaDemuxerPtsUnitTest, HandleAutoMaintainPts_004, TestSize.Level0)
{
    ASSERT_NE(demuxerPtr_, nullptr);
    auto mockAvbuffer = std::make_shared<AVBuffer>();
    int32_t trackId = NUM_TEST;
    demuxerPtr_->isAutoMaintainPts_ = true;
    auto maintainBaseInfo = std::make_shared<MediaDemuxer::MaintainBaseInfo>();
    maintainBaseInfo->lastPts = INVALID_PTS_DATA;
    maintainBaseInfo->segmentOffset = NUM_TEST;
    maintainBaseInfo->lastPtsModifyedMax = ID_TEST;
    maintainBaseInfo->isLastPtsChange = true;
    EXPECT_CALL(*(demuxerPtr_->source_), GetSegmentOffset()).WillRepeatedly(Return(ID_TEST));
    demuxerPtr_->maintainBaseInfos_[trackId] = maintainBaseInfo;
    demuxerPtr_->HandleAutoMaintainPts(trackId, mockAvbuffer);
    EXPECT_EQ(maintainBaseInfo->isLastPtsChange, false);
}

/**
 * @tc.name: Test InitPtsInfo  API
 * @tc.number: InitPtsInfo_001
 * @tc.desc: Test source_->GetHLSDiscontinuity() == false
 */
HWTEST_F(MediaDemuxerPtsUnitTest, InitPtsInfo_001, TestSize.Level0)
{
    ASSERT_NE(demuxerPtr_, nullptr);
    auto mockAvbuffer = std::make_shared<AVBuffer>();
    int32_t trackId = NUM_TEST;
    std::shared_ptr<MediaDemuxer::MaintainBaseInfo> maintainBaseInfo = nullptr;
    demuxerPtr_->maintainBaseInfos_[trackId] = maintainBaseInfo;
    demuxerPtr_->InitPtsInfo();
    EXPECT_EQ(demuxerPtr_->source_->GetHLSDiscontinuity(), false);
}

/**
 * @tc.name: Test InitMediaStartPts  API
 * @tc.number: InitMediaStartPts_001
 * @tc.desc: Test trackInfo == nullptr
 *           Test trackInfo->GetData(Tag::MIME_TYPE, mime) == false
 */
HWTEST_F(MediaDemuxerPtsUnitTest, InitMediaStartPts_001, TestSize.Level0)
{
    ASSERT_NE(demuxerPtr_, nullptr);
    std::string mime;
    demuxerPtr_->mediaMetaData_.trackMetas.push_back(nullptr);
    auto realMeta = std::make_shared<Meta>();
    demuxerPtr_->mediaMetaData_.trackMetas.push_back(realMeta);
    demuxerPtr_->InitMediaStartPts();
    EXPECT_EQ(demuxerPtr_->mediaMetaData_.trackMetas[ID_TEST]->GetData(Tag::MIME_TYPE, mime), false);
}

/**
 * @tc.name: Test TranscoderInitMediaStartPts  API
 * @tc.number: TranscoderInitMediaStartPts_001
 * @tc.desc: Test trackInfo == nullptr
 *           Test trackInfo->GetData(Tag::MIME_TYPE, mime) == false
 */
HWTEST_F(MediaDemuxerPtsUnitTest, TranscoderInitMediaStartPts_001, TestSize.Level0)
{
    ASSERT_NE(demuxerPtr_, nullptr);
    std::string mime;
    demuxerPtr_->mediaMetaData_.trackMetas.push_back(nullptr);
    auto realMeta = std::make_shared<Meta>();
    demuxerPtr_->mediaMetaData_.trackMetas.push_back(realMeta);
    demuxerPtr_->TranscoderInitMediaStartPts();
    EXPECT_EQ(demuxerPtr_->mediaMetaData_.trackMetas[ID_TEST]->GetData(Tag::MIME_TYPE, mime), false);
}
} // OHOS::Media