/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "securec.h"
#include "fuzzer/FuzzedDataProvider"

#include <iostream>

#include "media_demuxer.h"

#define FUZZ_PROJECT_NAME "mediademuxerptsfunctions_fuzzer"
using namespace std;
using namespace OHOS::Media;
namespace OHOS {
const char *MP4_PATH = "/data/test/fuzz_create.mp4";
const int64_t EXPECT_SIZE = 37;
const int64_t SELECT_TRACK = 4;
bool CheckDataValidity(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    int32_t fd = open(MP4_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    int len = write(fd, data, size - 36);
    if (len <= 0) {
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

bool MediaDemuxerPtsFunctionsFuzzTest(const uint8_t *data, size_t size)
{
    if (!CheckDataValidity(data, size)) {
        return false;
    }

    std::shared_ptr<MediaDemuxer> mediaDemuxer =
        std::make_shared<MediaDemuxer>();

    std::shared_ptr<MediaSource> mediaSource =
        std::make_shared<MediaSource>(MP4_PATH);
    mediaDemuxer->SetDataSource(mediaSource);

    FuzzedDataProvider fdp(data, size);
    int32_t trackId = fdp.ConsumeIntegral<int32_t>() % SELECT_TRACK;
    
    std::shared_ptr<Media::AVBufferQueue> implBufferQueue_ =
        Media::AVBufferQueue::Create(size, Media::MemoryType::SHARED_MEMORY, "InnerDemo");  // 4
    sptr<AVBufferQueueProducer> producer = implBufferQueue_->GetProducer();
    mediaDemuxer->SetOutputBufferQueue(trackId, producer);
    
    mediaDemuxer->Start();
    mediaDemuxer->InitPtsInfo();
    mediaDemuxer->InitMediaStartPts();
    mediaDemuxer->TranscoderInitMediaStartPts();

    mediaDemuxer->SetSubtitleSource(std::make_shared<MediaSource>(MP4_PATH));
    mediaDemuxer->SetTranscoderMode();
    std::shared_ptr<AVBuffer> sample = AVBuffer::CreateAVBuffer();
    mediaDemuxer->HandleAutoMaintainPts(trackId, sample);
    int ret = remove(MP4_PATH);
    if (ret != 0) {
        return false;
    }
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::MediaDemuxerPtsFunctionsFuzzTest(data, size);
    return 0;
}