/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <fcntl.h>
#include <sys/stat.h>
#include "demuxerinner_sample.h"
#include "native_avcodec_base.h"

namespace OHOS {
namespace MediaAVCodec {
DemuxerInnerSample::~DemuxerInnerSample()
{
    if (fd > 0) {
        close(fd);
        fd = -1;
    }
    if (demuxer_ != nullptr) {
        demuxer_ = nullptr;
    }
    if (avsource_ != nullptr) {
        avsource_ = nullptr;
    }
    if (avBuffer != nullptr) {
        avBuffer = nullptr;
    }
}

static int64_t GetFileSize(const char *fileName)
{
    int64_t fileSize = 0;
    if (fileName != nullptr) {
        struct stat fileStatus {};
        if (stat(fileName, &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}


int DemuxerInnerSample::CreateDemuxer()
{
    fd = open(filePath, O_RDONLY);
    int64_t size = GetFileSize(filePath);
    this->avsource_ = AVSourceFactory::CreateWithFD(fd, 0, size);
    if (!avsource_) {
        close(fd);
        fd = -1;
        return -1;
    }
    this->demuxer_ = AVDemuxerFactory::CreateWithSource(avsource_);
    if (!demuxer_) {
        close(fd);
        fd = -1;
        return -1;
    }
    return 0;
}

void DemuxerInnerSample::RunNormalDemuxerInner(uint32_t createSize)
{
    bool gReadEnd = false;
    Format sourceFormat;
    Format trackFormat;
    int ret = CreateDemuxer();
    if (ret < 0) {
        return;
    }
    ret = this->avsource_->GetSourceFormat(sourceFormat);
    if (ret != 0) {
        return;
    }
    sourceFormat.GetIntValue(OH_MD_KEY_TRACK_COUNT, trackCount);
    for (int32_t index = 0; index < trackCount; index++) {
        ret = this->avsource_->GetTrackFormat(trackFormat, index);
        if (ret != 0) {
            return;
        }
        trackFormat.GetIntValue(Media::Tag::VIDEO_HDR_TYPE, hdrType);
        ret = this->demuxer_->SelectTrackByID(index);
    }
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, createSize);
    while (!gReadEnd && trackCount > 0) {
        for (int32_t index = 0; index < trackCount; index++) {
            ret = this->demuxer_->ReadSampleBuffer(index, avBuffer);
            if (ret != 0) {
                gReadEnd = true;
                break;
            }
            if (avBuffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
                gReadEnd = true;
            }
        }
    }
}
}
}