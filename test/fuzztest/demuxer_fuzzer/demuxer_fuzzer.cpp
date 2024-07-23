/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"

#define FUZZ_PROJECT_NAME "demuxer_fuzzer"
namespace OHOS {
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

static void SetVarValue(OH_AVCodecBufferAttr attr, const int &tarckType, bool &audioIsEnd, bool &videoIsEnd)
{
    if (tarckType == MEDIA_TYPE_AUD && (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
        audioIsEnd = true;
    }

    if (tarckType == MEDIA_TYPE_VID && (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
        videoIsEnd = true;
    }
}

void RunNormalDemuxer()
{
    int tarckType = 0;
    int32_t trackCount;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (!source) {
        return;
    }
    OH_AVDemuxer *demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (!demuxer) {
        return;
    }

    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(source);
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount);
    for (int32_t index = 0; index < trackCount; index++) {
        OH_AVDemuxer_SelectTrackByID(demuxer, index);
    }
    static int32_t width = 3840;
    static int32_t height = 2160;
    OH_AVMemory *memory = OH_AVMemory_Create(width * height);
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, index);
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType);
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr);
            SetVarValue(attr, tarckType, audioIsEnd, videoIsEnd);
        }
    }
    OH_AVDemuxer_Destroy(demuxer);
    OH_AVSource_Destroy(source);
    close(fd);
}

void RunNormalDemuxerApi11()
{
    int tarckType = 0;
    int32_t trackCount;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (!source) {
        return;
    }
    OH_AVDemuxer *demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (!demuxer) {
        return;
    }

    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(source);
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount);
    for (int32_t index = 0; index < trackCount; index++) {
        OH_AVDemuxer_SelectTrackByID(demuxer, index);
    }
    static int32_t width = 3840;
    static int32_t height = 2160;
    OH_AVBuffer *buffer = OH_AVBuffer_Create(width * height);
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, index);
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType);
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            OH_AVDemuxer_ReadSampleBuffer(demuxer, index, buffer);
            OH_AVBuffer_GetBufferAttr(buffer, &attr);
            SetVarValue(attr, tarckType, audioIsEnd, videoIsEnd);
        }
    }
    OH_AVDemuxer_Destroy(demuxer);
    OH_AVSource_Destroy(source);
    close(fd);
}

bool DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    RunNormalDemuxer();
    RunNormalDemuxerApi11();
    // FUZZ CreateFD
    int32_t fd = *reinterpret_cast<const int32_t *>(data);
    int64_t offset = *reinterpret_cast<const int64_t *>(data);
    int64_t fileSize = *reinterpret_cast<const int64_t *>(data);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd, offset, fileSize);
    if (source) {
        OH_AVSource_Destroy(source);
    }
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}
