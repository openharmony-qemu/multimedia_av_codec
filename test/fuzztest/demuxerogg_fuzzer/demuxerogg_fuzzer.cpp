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
static int32_t g_width = 3840;
static int32_t g_height = 2160;
int g_trackType = 0;
int32_t g_trackCount;
OH_AVCodecBufferAttr attr;
bool g_audioEnd = false;
bool g_videoEnd = false;
const char *FILE_PATH = "/data/test/fuzz_create.ogg";
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

void ResetFlag()
{
    g_audioEnd = false;
    g_videoEnd = false;
}

static void SetVarValue(OH_AVCodecBufferAttr setAttr, const int &setTarckType, bool &setAudioIsEnd, bool &setVideoIsEnd)
{
    if (setTarckType == MEDIA_TYPE_AUD && (setAttr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
        setAudioIsEnd = true;
    }

    if (setTarckType == MEDIA_TYPE_VID && (setAttr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
        setVideoIsEnd = true;
    }
}

void RunNormalDemuxer()
{
    ResetFlag();
    int fd = open(FILE_PATH, O_RDONLY);
    int64_t size = GetFileSize(FILE_PATH);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (!source) {
        close(fd);
        return;
    }
    OH_AVDemuxer *demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (!demuxer) {
        OH_AVSource_Destroy(source);
        source = nullptr;
        close(fd);
        return;
    }
    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(source);
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        OH_AVDemuxer_SelectTrackByID(demuxer, index);
    }
    OH_AVMemory *memory = OH_AVMemory_Create(g_width * g_height);
    while (!g_audioEnd || !g_videoEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, index);
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &g_trackType);
            if ((g_audioEnd && (g_trackType == MEDIA_TYPE_AUD)) || (g_videoEnd && (g_trackType == MEDIA_TYPE_VID))) {
                OH_AVFormat_Destroy(trackFormat);
                trackFormat = nullptr;
                continue;
            }
            if (trackFormat) {
                OH_AVFormat_Destroy(trackFormat);
                trackFormat = nullptr;
            }
            OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr);
            SetVarValue(attr, g_trackType, g_audioEnd, g_videoEnd);
        }
    }
    OH_AVDemuxer_Destroy(demuxer);
    OH_AVSource_Destroy(source);
    if (sourceFormat) {
        OH_AVFormat_Destroy(sourceFormat);
    }
    if (memory) {
        OH_AVMemory_Destroy(memory);
    }
    close(fd);
}

void RunNormalDemuxerApi11()
{
    ResetFlag();
    int fd = open(FILE_PATH, O_RDONLY);
    int64_t size = GetFileSize(FILE_PATH);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (!source) {
        close(fd);
        return;
    }
    OH_AVDemuxer *demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (!demuxer) {
        close(fd);
        OH_AVSource_Destroy(source);
        source = nullptr;
        return;
    }
    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(source);
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        OH_AVDemuxer_SelectTrackByID(demuxer, index);
    }
    OH_AVBuffer *buffer = OH_AVBuffer_Create(g_width * g_height);
    while (!g_audioEnd || !g_videoEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, index);
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &g_trackType);
            if ((g_audioEnd && (g_trackType == MEDIA_TYPE_AUD)) || (g_videoEnd && (g_trackType == MEDIA_TYPE_VID))) {
                OH_AVFormat_Destroy(trackFormat);
                trackFormat = nullptr;
                continue;
            }
            if (trackFormat) {
                OH_AVFormat_Destroy(trackFormat);
                trackFormat = nullptr;
            }
            OH_AVDemuxer_ReadSampleBuffer(demuxer, index, buffer);
            OH_AVBuffer_GetBufferAttr(buffer, &attr);
            SetVarValue(attr, g_trackType, g_audioEnd, g_videoEnd);
        }
    }
    OH_AVDemuxer_Destroy(demuxer);
    OH_AVSource_Destroy(source);
    if (sourceFormat) {
        OH_AVFormat_Destroy(sourceFormat);
    }
    if (buffer) {
        OH_AVBuffer_Destroy(buffer);
    }
    close(fd);
}

bool DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    int32_t fd = open(FILE_PATH, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    int len = write(fd, data, size);
    if (len < 0) {
        return false;
    }
    close(fd);
    RunNormalDemuxer();
    RunNormalDemuxerApi11();
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
