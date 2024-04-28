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

#ifndef AVSOURCE_UNIT_TEST_H
#define AVSOURCE_UNIT_TEST_H

#include "gtest/gtest.h"
#include "avsource_mock.h"

namespace OHOS {
namespace MediaAVCodec {
class AVSourceUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
    int64_t GetFileSize(const std::string &fileName);
    int32_t OpenFile(const std::string &fileName);
    void ResetFormatValue();

    void InitResource(const std::string &path, bool local);
    void CheckHevcInfo(const std::string resName);

protected:
    std::shared_ptr<AVSourceMock> source_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    int32_t fd_ = -1;
    int64_t size_ = 0;
    uint32_t trackIndex_ = 0;
    int32_t streamsCount_ = 0;
    int32_t vTrackIdx_ = 0;
    int32_t aTrackIdx_ = 0;
    uint8_t *addr_ = nullptr;
    size_t buffSize_ = 0;

    struct FormatValue {
        // source format
        std::string title = "";
        std::string artist = "";
        std::string album = "";
        std::string albumArtist = "";
        std::string date = "";
        std::string comment = "";
        std::string genre = "";
        std::string copyright = "";
        std::string description = "";
        std::string language = "";
        std::string lyrics = "";
        int64_t duration = 0;
        int32_t trackCount = 0;
        std::string author = "";
        std::string composer = "";
        int32_t hasVideo = -1;
        int32_t hasAudio = -1;
        int32_t hasSubtitle = -1;
        int32_t fileType = 0;
        // track format
        std::string codecMime = "";
        int32_t trackType = 0;
        int32_t width = 0;
        int32_t height = 0;
        int32_t aacIsAdts = -1;
        int32_t sampleRate = 0;
        int32_t channelCount = 0;
        int64_t bitRate = 0;
        int32_t audioSampleFormat = 0;
        double frameRate = 0;
        int32_t rotationAngle = 0;
        int64_t channelLayout = 0;
        int32_t hdrType = 0;
        int32_t codecProfile = 0;
        int32_t codecLevel = 0;
        int32_t colorPrimaries = 0;
        int32_t transferCharacteristics = 0;
        int32_t rangeFlag = 0;
        int32_t matrixCoefficients = 0;
        int32_t chromaLocation = 0;
        // hevc format
        int32_t profile = 0;
        int32_t level = 0;
        int32_t colorPri = 0;
        int32_t colorTrans = 0;
        int32_t colorMatrix = 0;
        int32_t colorRange = 0;
        int32_t chromaLoc = 0;
        int32_t isHdrVivid = 0;
    };
    FormatValue formatVal_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVSOURCE_UNIT_TEST_H