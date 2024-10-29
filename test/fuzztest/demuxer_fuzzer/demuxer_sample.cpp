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
#include "demuxer_sample.h"
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

DemuxerSample::~DemuxerSample()
{
    if (fd > 0) {
        close(fd);
        fd = 0;
    }
    if (source != nullptr) {
        OH_AVSource_Destroy(source);
        source = nullptr;
    }
    if (demuxer != nullptr) {
        OH_AVDemuxer_Destroy(demuxer);
        demuxer = nullptr;
    }
    if (sourceFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
    }
    if (memory != nullptr) {
        OH_AVMemory_Destroy(memory);
        memory = nullptr;
    }
    if (buffer != nullptr) {
        OH_AVBuffer_Destroy(buffer);
        buffer = nullptr;
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

int DemuxerSample::CreateDemuxer() {
    fd = open(FILE_PATH, O_RDONLY);
    int64_t size = GetFileSize(FILE_PATH);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (!source) {
        close(fd);
        return -1;
    }
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (!demuxer) {
        OH_AVSource_Destroy(source);
        source = nullptr;
        close(fd);
        return -1;
    }
    return 0;
}

void DemuxerSample::RunNormalDemuxer() {
    g_readEnd = false;
    int ret = CreateDemuxer();
    if (ret < 0) {
        return;
    }
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    if (sourceFormat == nullptr) {
        return;
    }
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        OH_AVDemuxer_SelectTrackByID(demuxer, index);
    }
    memory = OH_AVMemory_Create(g_width * g_height);
    while (!g_readEnd && g_trackCount > 0) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, index);
            if (trackFormat == nullptr) {
                g_readEnd = true;
                break;
            }
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &g_trackType);
            if (trackFormat) {
                OH_AVFormat_Destroy(trackFormat);
                trackFormat = nullptr;
            }
            ret = OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr);
            if (ret != 0) {
                g_readEnd = true;
                break;
            }
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                g_readEnd = true;
                break;
            }
        }
    }
}

void DemuxerSample::RunNormalDemuxerApi11() {
    g_readEnd = false;
    int ret = CreateDemuxer();
    if (ret < 0) {
        return;
    }
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    if (sourceFormat == nullptr) {
        return;
    }
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        OH_AVDemuxer_SelectTrackByID(demuxer, index);
    }
    buffer = OH_AVBuffer_Create(g_width * g_height);
    while (!g_readEnd && g_trackCount > 0) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, index);
            if (trackFormat == nullptr) {
                g_readEnd = true;
                break;
            }
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &g_trackType);
            if (trackFormat) {
                OH_AVFormat_Destroy(trackFormat);
                trackFormat = nullptr;
            }
            ret = OH_AVDemuxer_ReadSampleBuffer(demuxer, index, buffer);
            if (ret != 0) {
                g_readEnd = true;
                break;
            }
            OH_AVBuffer_GetBufferAttr(buffer, &attr);
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                g_readEnd = true;
                break;
            }
        }
    }
}