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

#ifndef VCODEC_UNIT_TEST_H
#define VCODEC_UNIT_TEST_H

#include "gtest/gtest.h"
#include "demuxer_mock.h"

namespace OHOS {
namespace MediaAVCodec {
class DemuxerUnitTest : public testing::Test {
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
    bool isEOS(std::map<uint32_t, bool>& countFlag);
    void SetInitValue();
    void ReadData();
    void ReadData(int readNum, int64_t &seekTime);

protected:
    std::shared_ptr<AVSourceMock> source_ = nullptr;
    std::shared_ptr<DemuxerMock> demuxer_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    std::shared_ptr<AVMemoryMock> sharedMem_ = nullptr;
    int32_t fd_ = -1;
    AVCodecBufferInfo info_;
    uint32_t flag_;
    std::vector<uint32_t> selectedTrackIds_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VCODEC_UNIT_TEST_H