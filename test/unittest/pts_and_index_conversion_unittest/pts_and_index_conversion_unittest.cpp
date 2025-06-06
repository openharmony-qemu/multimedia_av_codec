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

#include "pts_and_index_conversion_unittest.h"

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
#include "avcodec_errors.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace testing;
using namespace std;
namespace OHOS {
namespace Media {
const static int32_t ID_TEST = 1;
const static int32_t NUM_TEST = 0;
const static int32_t INVALID_MODE = 4;
using MediaSource = OHOS::Media::Plugins::MediaSource;

void PtsAndIndexConversionUnitTest::SetUpTestCase(void)
{
}

void PtsAndIndexConversionUnitTest::TearDownTestCase(void)
{
}

void PtsAndIndexConversionUnitTest::SetUp()
{
    timeAndIndexConversions_ = std::make_shared<TimeAndIndexConversion>();
}

void PtsAndIndexConversionUnitTest::TearDown()
{
    timeAndIndexConversions_ = nullptr;
}

/**
 * @tc.name: Test PTSAndIndexConvertSttsAndCttsProcess  API
 * @tc.number: PTSAndIndexConvertSttsAndCttsProcess_001
 * @tc.desc: Test sttsIndex < sttsCount && cttsIndex < cttsCount
 *           Test sttsCurNum == 0
 */
HWTEST_F(PtsAndIndexConversionUnitTest, PTSAndIndexConvertSttsAndCttsProcess_001, TestSize.Level0)
{
    ASSERT_NE(timeAndIndexConversions_, nullptr);
    TimeAndIndexConversion::TrakInfo testInfo;
    testInfo.timeScale = ID_TEST;
    TimeAndIndexConversion::STTSEntry testSEntry;
    testSEntry.sampleCount = ID_TEST;
    testSEntry.sampleDelta = ID_TEST;
    TimeAndIndexConversion::CTTSEntry testCEntry;
    testCEntry.sampleCount = ID_TEST;
    testCEntry.sampleOffset = ID_TEST;
    testInfo.sttsEntries.push_back(testSEntry);
    testInfo.cttsEntries.push_back(testCEntry);
    timeAndIndexConversions_->trakInfoVec_.push_back(testInfo);
    timeAndIndexConversions_->curConvertTrakInfoIndex_ = NUM_TEST;
    TimeAndIndexConversion::IndexAndPTSConvertMode mode =
        TimeAndIndexConversion::IndexAndPTSConvertMode::GET_FIRST_PTS;
    int64_t absolutePTS = NUM_TEST;
    uint32_t index = NUM_TEST;
    auto ret = timeAndIndexConversions_->PTSAndIndexConvertSttsAndCttsProcess(mode, absolutePTS, index);
    auto sttsIndex = NUM_TEST;
    auto sttsCount = timeAndIndexConversions_->trakInfoVec_[timeAndIndexConversions_->curConvertTrakInfoIndex_]
        .sttsEntries.size();
    EXPECT_GE(sttsCount, sttsIndex);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Test PTSAndIndexConvertSttsAndCttsProcess  API
 * @tc.number: PTSAndIndexConvertSttsAndCttsProcess_002
 * @tc.desc: Test cttsCurNum == 0
 *           Test cttsIndex >= cttsCount
 *           Test cttsCurNum == 0
 *           Test sttsCurNum == 0
 */
HWTEST_F(PtsAndIndexConversionUnitTest, PTSAndIndexConvertSttsAndCttsProcess_002, TestSize.Level0)
{
    ASSERT_NE(timeAndIndexConversions_, nullptr);
    TimeAndIndexConversion::TrakInfo testInfo;
    testInfo.timeScale = ID_TEST;
    TimeAndIndexConversion::STTSEntry testSEntry;
    testSEntry.sampleCount = NUM_TEST;
    testSEntry.sampleDelta = ID_TEST;
    TimeAndIndexConversion::STTSEntry testSEntry2;
    testSEntry2.sampleCount = NUM_TEST;
    testSEntry2.sampleDelta = NUM_TEST;
    TimeAndIndexConversion::CTTSEntry testCEntry;
    testCEntry.sampleCount = NUM_TEST;
    testCEntry.sampleOffset = ID_TEST;
    TimeAndIndexConversion::CTTSEntry testCEntry2;
    testCEntry2.sampleCount = NUM_TEST;
    testCEntry2.sampleOffset = NUM_TEST;
    testInfo.sttsEntries.push_back(testSEntry);
    testInfo.sttsEntries.push_back(testSEntry2);
    testInfo.cttsEntries.push_back(testCEntry);
    testInfo.cttsEntries.push_back(testCEntry2);
    timeAndIndexConversions_->trakInfoVec_.push_back(testInfo);
    timeAndIndexConversions_->curConvertTrakInfoIndex_ = NUM_TEST;
    TimeAndIndexConversion::IndexAndPTSConvertMode mode =
        TimeAndIndexConversion::IndexAndPTSConvertMode::GET_FIRST_PTS;
    int64_t absolutePTS = NUM_TEST;
    uint32_t index = NUM_TEST;
    auto ret = timeAndIndexConversions_->PTSAndIndexConvertSttsAndCttsProcess(mode, absolutePTS, index);
    auto cttsIndex = NUM_TEST;
    uint32_t cttsCurNum = timeAndIndexConversions_->trakInfoVec_[timeAndIndexConversions_->curConvertTrakInfoIndex_]
        .cttsEntries[cttsIndex].sampleCount;
    EXPECT_EQ(cttsCurNum, NUM_TEST);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Test PTSAndIndexConvertOnlySttsProcess  API
 * @tc.number: PTSAndIndexConvertOnlySttsProcess_001
 * @tc.desc: Test sttsCurNum == 0
 *           Test sttsCurNum == 0, sttsIndex == sttsCount
 */
HWTEST_F(PtsAndIndexConversionUnitTest, PTSAndIndexConvertOnlySttsProcess_001, TestSize.Level0)
{
    ASSERT_NE(timeAndIndexConversions_, nullptr);
    TimeAndIndexConversion::TrakInfo testInfo;
    testInfo.timeScale = ID_TEST;
    TimeAndIndexConversion::STTSEntry testSEntry;
    testSEntry.sampleCount = ID_TEST;
    testSEntry.sampleDelta = ID_TEST;
    testInfo.sttsEntries.push_back(testSEntry);
    timeAndIndexConversions_->trakInfoVec_.push_back(testInfo);
    timeAndIndexConversions_->curConvertTrakInfoIndex_ = NUM_TEST;
    TimeAndIndexConversion::IndexAndPTSConvertMode mode =
        TimeAndIndexConversion::IndexAndPTSConvertMode::GET_FIRST_PTS;
    int64_t absolutePTS = NUM_TEST;
    uint32_t index = NUM_TEST;
    auto ret = timeAndIndexConversions_->PTSAndIndexConvertOnlySttsProcess(mode, absolutePTS, index);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Test PTSAndIndexConvertSwitchProcess  API
 * @tc.number: PTSAndIndexConvertSwitchProcess_001
 * @tc.desc: Test case GET_FIRST_PTS
 *           Test case default
 */
HWTEST_F(PtsAndIndexConversionUnitTest, PTSAndIndexConvertSwitchProcess_001, TestSize.Level0)
{
    ASSERT_NE(timeAndIndexConversions_, nullptr);
    TimeAndIndexConversion::IndexAndPTSConvertMode mode =
        static_cast<TimeAndIndexConversion::IndexAndPTSConvertMode>(INVALID_MODE);
    int64_t pts = ID_TEST;
    int64_t absolutePTS = ID_TEST;
    uint32_t index = ID_TEST;
    timeAndIndexConversions_->absolutePTSIndexZero_ = INVALID_MODE;
    timeAndIndexConversions_->PTSAndIndexConvertSwitchProcess(mode, pts, absolutePTS, index);
    mode = TimeAndIndexConversion::IndexAndPTSConvertMode::GET_FIRST_PTS;
    timeAndIndexConversions_->PTSAndIndexConvertSwitchProcess(mode, pts, absolutePTS, index);
    EXPECT_EQ(timeAndIndexConversions_->absolutePTSIndexZero_, ID_TEST);
}
} // Media
} // OHOS