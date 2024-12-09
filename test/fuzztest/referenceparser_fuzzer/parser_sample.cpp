/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "parser_sample.h"
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include "native_avcodec_base.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace OHOS::MediaAVCodec;

ParserSample::~ParserSample()
{
    if (fd > 0) {
        close(fd);
        fd = 0;
    }
    if (source != nullptr) {
        source = nullptr;
    }
    if (demuxer != nullptr) {
        demuxer = nullptr;
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

int ParserSample::CreateDemuxer(uint32_t buffersize, int64_t ptsForPtsIndex, int64_t frameIndex)
{
    Format sourceFormat;
    Format trackFormat;
    uint64_t presentationTimeUs = 0;
    uint32_t indexForPtsIndex = 0;
    fd = open(filePath, O_RDONLY);
    source = AVSourceFactory::CreateWithFD(fd, 0, GetFileSize(filePath));
    if (!source) {
        close(fd);
        fd = 0;
        return -1;
    }
    demuxer = AVDemuxerFactory::CreateWithSource(source);
    if (!demuxer) {
        source = nullptr;
        close(fd);
        fd = 0;
        return -1;
    }
    int32_t ret = source->GetSourceFormat(sourceFormat);
    if (ret != 0) {
        return -1;
    }
    if (!sourceFormat.GetIntValue(OH_MD_KEY_TRACK_COUNT, gTrackCount)) {
        return -1;
    }
    for (int32_t index = 0; index < gTrackCount; index++) {
        demuxer->SelectTrackByID(index);
        demuxer->GetRelativePresentationTimeUsByIndex(index, frameIndex, presentationTimeUs);
        demuxer->GetIndexByRelativePresentationTimeUs(index, ptsForPtsIndex, indexForPtsIndex);
    }
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    return 0;
}

void ParserSample::RunReferenceParser(int64_t pts, int64_t ptsForPtsIndex, int64_t frameIndex, uint32_t createSize)
{
    int ret = CreateDemuxer(createSize, ptsForPtsIndex, frameIndex);
    if (ret < 0) {
        return;
    }
    ret = demuxer->StartReferenceParser(pts);
    if (ret != 0) {
        return;
    }
    ret = demuxer->ReadSampleBuffer(createSize, avBuffer);
    if (ret != 0) {
        return;
    }
    FrameLayerInfo frameLayerInfo;
    ret = demuxer->GetFrameLayerInfo(avBuffer, frameLayerInfo);
    if (ret != 0) {
        return;
    }
    GopLayerInfo gopLayerInfo;
    ret = demuxer->GetGopLayerInfo(frameLayerInfo.gopId, gopLayerInfo);
    if (ret != 0) {
        printf("GetGopLayerInfo fail\n");
    }
}