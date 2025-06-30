/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "demuxer_filter_unittest.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing;
using namespace testing::ext;
static const int32_t NUM_1 = 1;

void DemuxerFilterUnitTest::SetUpTestCase(void) {}

void DemuxerFilterUnitTest::TearDownTestCase(void) {}

void DemuxerFilterUnitTest::SetUp(void)
{
    demuxerFilter_ = std::make_shared<DemuxerFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_DEMUXER);
}

void DemuxerFilterUnitTest::TearDown(void)
{
    demuxerFilter_ = nullptr;
}

/**
 * @tc.name  : Test DoPause
 * @tc.number: DoPause_001
 * @tc.desc  : Test state_ == FilterState::FROZEN
 */
HWTEST_F(DemuxerFilterUnitTest, DoPause_001, TestSize.Level0)
{
    ASSERT_NE(demuxerFilter_, nullptr);
    demuxerFilter_->state_ = FilterState::FROZEN;
    auto ret = demuxerFilter_->DoPause();
    EXPECT_EQ(demuxerFilter_->state_, FilterState::PAUSED);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test HandleTrackInfos
 * @tc.number: HandleTrackInfos_001
 * @tc.desc  : Test (!meta->GetData(Tag::MIME_TYPE, mime)) == true
 */
HWTEST_F(DemuxerFilterUnitTest, HandleTrackInfos_001, TestSize.Level0)
{
    ASSERT_NE(demuxerFilter_, nullptr);
    std::vector<std::shared_ptr<Meta>> trackInfos;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::MIME_TYPE, "test");
    trackInfos.push_back(meta);
    int32_t count = NUM_1;
    auto ret = demuxerFilter_->HandleTrackInfos(trackInfos, count);
    EXPECT_EQ(ret, Status::OK);
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS