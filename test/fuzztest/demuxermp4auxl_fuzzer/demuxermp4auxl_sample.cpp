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
#include "demuxermp4auxl_sample.h"
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

const int32_t WIDTH = 3840;
const int32_t HEIGHT = 2160;

DemuxerMp4AuxlSample::~DemuxerMp4AuxlSample()
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

int DemuxerMp4AuxlSample::CreateDemuxer()
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

void DemuxerMp4AuxlSample::GetFormat(OH_AVFormat* format)
{
    int trackType = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_TRACK_TYPE, &trackType);
    int64_t duration = 0;
    OH_AVFormat_GetLongValue(format, OH_MD_KEY_DURATION, &duration);
    float currentHeight = 0;
    OH_AVFormat_GetFloatValue(format, OH_MD_KEY_HEIGHT, &currentHeight);
    double frameRate;
    OH_AVFormat_GetDoubleValue(format, OH_MD_KEY_FRAME_RATE, &frameRate);
    const char* language = nullptr;
    OH_AVFormat_GetStringValue(format, OH_MD_KEY_LANGUAGE, &language);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    OH_AVFormat_GetBuffer(format, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &bufferSize);
    const char* trackDesc = nullptr;
    OH_AVFormat_GetStringValue(format, OH_MD_KEY_TRACK_DESCRIPTION, &trackDesc);
    const char* referenceType = nullptr;
    OH_AVFormat_GetStringValue(format, OH_MD_KEY_TRACK_REFERENCE_TYPE, &referenceType);
    int32_t* referenceIds = nullptr;
    size_t referenceIdsSize = 0;
    OH_AVFormat_GetIntBuffer(format, OH_MD_KEY_REFERENCE_TRACK_IDS, &referenceIds, &referenceIdsSize);
}

void DemuxerMp4AuxlSample::RunNormalDemuxer(uint32_t createSize, int64_t time)
{
    bool gReadEnd = false;
    int ret = CreateDemuxer();
    if (ret < 0) {
        return;
    }
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    if (sourceFormat == nullptr) {
        return;
    }
    int32_t trackCount;
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount);
    for (int32_t index = 0; index < trackCount; index++) {
        OH_AVDemuxer_SelectTrackByID(demuxer, index);
    }

    OH_AVBuffer *buffer = OH_AVBuffer_Create(WIDTH * HEIGHT);
    while (!gReadEnd && trackCount > 0) {
        for (int32_t index = 0; index < trackCount; index++) {
            OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, index);
            if (trackFormat == nullptr) {
                gReadEnd = true;
                break;
            }
            GetFormat(trackFormat);
            if (trackFormat) {
                OH_AVFormat_Destroy(trackFormat);
                trackFormat = nullptr;
            }
            ret = OH_AVDemuxer_ReadSampleBuffer(demuxer, index, buffer);
            if (ret != 0) {
                gReadEnd = true;
                break;
            }
            OH_AVCodecBufferAttr attr;
            OH_AVBuffer_GetBufferAttr(buffer, &attr);
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                gReadEnd = true;
                break;
            }
        }
    }
    if (buffer != nullptr) {
        OH_AVBuffer_Destroy(buffer);
        buffer = nullptr;
    }
    OH_AVDemuxer_SeekToTime(demuxer, time, SEEK_MODE_CLOSEST_SYNC);
    OH_AVDemuxer_SeekToTime(demuxer, time, SEEK_MODE_PREVIOUS_SYNC);
    OH_AVDemuxer_SeekToTime(demuxer, time, SEEK_MODE_NEXT_SYNC);
}