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
#include "gtest/gtest.h"

#include "demuxer_test_helper.h"
#include "demuxer_unit_test.h"
#include "file_info.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
using namespace testing::ext;

FileBaseInfo CombineFileBaseInfo(std::string fileName,
    SourceInfoMap sourceInfoMap, TrackInfoMap trackInfoMap, ReadInfoMap readInfoMap, SeekInfoMap seekInfoMap)
{
    FileBaseInfo fileInfo = FileBaseInfo();
    if (sourceInfoMap.count(fileName) > 0) {
        fileInfo.sourceInfo = sourceInfoMap[fileName];
    }
    if (trackInfoMap.count(fileName) > 0) {
        fileInfo.trackInfo = trackInfoMap[fileName];
    }
    if (readInfoMap.count(fileName) > 0) {
        fileInfo.expectReadInfo = readInfoMap[fileName];
    }
    // link: order of file_info.h - SeekInfoMap - std::vector<SeekInfoRecord>
    std::map<int32_t, SeekMode> seekModeIndexMap = {
        {0, SeekMode::SEEK_NEXT_SYNC},
        {1, SeekMode::SEEK_PREVIOUS_SYNC},
        {2, SeekMode::SEEK_CLOSEST_SYNC},
    };
    if (seekInfoMap.count(fileName) > 0) {
        fileInfo.expectSeekInfo = SeekInfo();
        for (auto iter : seekInfoMap[fileName]) {
            int64_t timeStamp = iter.first;
            fileInfo.expectSeekInfo[timeStamp] = std::map<SeekMode, SeekInfoRecord>();
            std::vector<SeekInfoRecord> seekInfo = iter.second;
            for (auto index = 0; index < seekInfo.size() && index < seekModeIndexMap.size(); ++index) {
                fileInfo.expectSeekInfo[timeStamp][seekModeIndexMap[index]] = seekInfo[index];
            }
        }
    }
    return fileInfo;
}

/**
 * @tc.name: AVDemuxer_Container_Mp4_040101
 * @tc.desc: Test local file (CheckFormat, Read, Seek)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, AVDemuxer_Container_Mp4_040101, TestSize.Level1)
{
    std::shared_ptr<DemuxerTestHelper> testHelper = std::make_shared<DemuxerTestHelper>();
    ASSERT_NE(testHelper, nullptr);
    FileBaseInfo fileInfo = CombineFileBaseInfo("test_264_B_Gop25_4sec_cover.mp4",
        sourceInfoMap, trackInfoMap, readInfoMap, seekInfoMap);
    ASSERT_EQ(testHelper->TestFile("test_264_B_Gop25_4sec_cover.mp4", SourceType::LOCAL, fileInfo), AV_ERR_OK);
}

/**
 * @tc.name: AVDemuxer_Container_Mp4_140101
 * @tc.desc: Test url file (CheckFormat, Read, Seek)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, AVDemuxer_Container_Mp4_140101, TestSize.Level1)
{
    std::shared_ptr<DemuxerTestHelper> testHelper = std::make_shared<DemuxerTestHelper>();
    ASSERT_NE(testHelper, nullptr);
    FileBaseInfo fileInfo = CombineFileBaseInfo("test_264_B_Gop25_4sec_cover.mp4",
        sourceInfoMap, trackInfoMap, readInfoMap, seekInfoMap);
    ASSERT_EQ(testHelper->TestFile("test_264_B_Gop25_4sec_cover.mp4", SourceType::URI, fileInfo), AV_ERR_OK);
}
} // namespace
} // namespace MediaAVCodec
} // namespace OHOS