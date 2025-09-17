/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef DEMUXER_TEST_HELPER_H
#define DEMUXER_TEST_HELPER_H

#include "meta/media_types.h"

#include "avformat/avformat_mock.h"
#include "avsource_mock.h"
#include "base_config.h"
#include "demuxer_mock.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
class DemuxerTestHelper {
public:
    ~DemuxerTestHelper();
    // group test group: init, format, read, seek
    int32_t TestFile(std::string fileAbsPath, SourceType sourceType, FileBaseInfo info);
    // single func test
    int32_t Init(std::string fileAbsPath, SourceType sourceType);
    int32_t CheckFormat();
    int32_t CheckReadSample();
    int32_t CheckSeek();
private:
    int32_t fd_ = -1;
    std::shared_ptr<AVSourceMock> source_ = nullptr;
    std::shared_ptr<DemuxerMock> demuxer_ = nullptr;
    std::shared_ptr<FormatMock> sourceFormat_ = nullptr;
    std::shared_ptr<AVBufferMock> buffer_ = nullptr;
    std::shared_ptr<AVMemoryMock> memory_ = nullptr;

    FileBaseInfo fileInfo_ {};
    int32_t trackCount_ = 0;
    // frameCount, keyFrameCount, eos
    ReadInfo trackReadInfo_ {};
    std::vector<std::string> passProcess_ = {};
    std::vector<std::string> errorProcess_ = {};

    void DumpFileInfo();

    int32_t CheckSourceFormat();
    int32_t CheckTrackFormat();
    int32_t CheckInfo(Info info, std::shared_ptr<FormatMock> format, std::string logPrefix = "");
    int32_t CheckBaseTypeInfo(Info info, std::shared_ptr<FormatMock> format, std::string logPrefix);
    int32_t CheckBufferTypeInfo(Info info, std::shared_ptr<FormatMock> format, std::string logPrefix);

    int32_t ReadAllSample();
    void CountReadFrame(int32_t trackIndex, AVBufferFlag flag);

    bool CheckSeekPos(int64_t seekTime, SeekMode seekMode);
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // DEMUXER_TEST_HELPER_H
