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
    if (uriSource != nullptr) {
        OH_AVSource_Destroy(uriSource);
        uriSource = nullptr;
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
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    if (audioFormat != nullptr) {
        OH_AVFormat_Destroy(audioFormat);
        audioFormat = nullptr;
    }
    if (videoFormat != nullptr) {
        OH_AVFormat_Destroy(videoFormat);
        videoFormat = nullptr;
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

int DemuxerSample::CreateDemuxer()
{
    fd = open(filePath, O_RDONLY);
    int64_t size = GetFileSize(filePath);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (!source) {
        close(fd);
        fd = 0;
        return -1;
    }
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (!demuxer) {
        OH_AVSource_Destroy(source);
        source = nullptr;
        close(fd);
        fd = 0;
        return -1;
    }
    return 0;
}

void DemuxerSample::RunNormalDemuxer()
{
    gReadEnd = false;
    int ret = CreateDemuxer();
    if (ret < 0) {
        return;
    }
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    if (sourceFormat == nullptr) {
        return;
    }
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &gTrackCount);
    for (int32_t index = 0; index < gTrackCount; index++) {
        OH_AVDemuxer_SelectTrackByID(demuxer, index);
    }
    memory = OH_AVMemory_Create(gHeight * gWidth);
    while (!gReadEnd && gTrackCount > 0) {
        for (int32_t index = 0; index < gTrackCount; index++) {
            OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, index);
            if (trackFormat == nullptr) {
                gReadEnd = true;
                break;
            }
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &gTrackType);
            if (gTrackType == MEDIA_TYPE_AUD) {
                OH_AVFormat_GetIntValue(trackFormat, "audio.soundbed.channels.number", &gTrackCount);
                OH_AVFormat_GetIntValue(trackFormat, "audio.hoa.order", &gTrackCount);
                OH_AVFormat_GetIntValue(trackFormat, "audio_object_number_key", &gTrackCount);
            }
            if (trackFormat) {
                OH_AVFormat_Destroy(trackFormat);
                trackFormat = nullptr;
            }
            ret = OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr);
            if (ret != 0) {
                gReadEnd = true;
                break;
            }
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                gReadEnd = true;
                break;
            }
        }
    }
    for (int32_t index = 0; index < gTrackCount; index++) {
        OH_AVDemuxer_UnselectTrackByID(demuxer, index);
    }
}