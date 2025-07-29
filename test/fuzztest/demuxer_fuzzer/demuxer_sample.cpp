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
#include "securec.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

#define DRM_UUID_LEN 16

#define MAX_PSSH_DATA_LEN 2048

typedef struct DRM_PsshInfo {
    uint8_t uuid[DRM_UUID_LEN];
    int32_t dataLen;
    uint8_t data[MAX_PSSH_DATA_LEN];
} DRM_PsshInfo;

#define MAX_PSSH_INFO_COUNT 8

typedef struct DRM_MediaKeySystemInfo {
    uint32_t psshCount;
    DRM_PsshInfo psshInfo[MAX_PSSH_INFO_COUNT];
} DRM_MediaKeySystemInfo;

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

static void SetFileValue(OH_AVCodecBufferAttr attr, bool &gReadEnd, int &keyCount)
{
    if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
        gReadEnd = true;
    } else {
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
            keyCount++;
        }
    }
}

void DrmMediaKeySystemInfoCallback(DRM_MediaKeySystemInfo *mediaKeySystemInfo)
{}

void DemuxerMediaKeySystemInfoCallback(OH_AVDemuxer *demuxer, DRM_MediaKeySystemInfo *mediaKeySystemInfo)
{}

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
    OH_AVDemuxer_SetMediaKeySystemInfoCallback(demuxer, DrmMediaKeySystemInfoCallback);
    OH_AVDemuxer_SetDemuxerMediaKeySystemInfoCallback(demuxer, DemuxerMediaKeySystemInfoCallback);
    return 0;
}

void DemuxerSample::GetAndSetFormat(const char *setLanguage, Params params)
{
    const char *stringVal;
    OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal);
    OH_AVDemuxer_SeekToTime(demuxer, params.time, SEEK_MODE_CLOSEST_SYNC);
    OH_AVDemuxer_SeekToTime(demuxer, params.time, SEEK_MODE_PREVIOUS_SYNC);
    OH_AVDemuxer_SeekToTime(demuxer, params.time, SEEK_MODE_NEXT_SYNC);
    for (int32_t index = 0; index < gTrackCount; index++) {
        OH_AVDemuxer_UnselectTrackByID(demuxer, index);
    }
    int64_t duration = 0;
    OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration);
    float currentHeight = 0;
    OH_AVFormat_GetFloatValue(sourceFormat, OH_MD_KEY_HEIGHT, &currentHeight);
    double frameRate;
    OH_AVFormat_GetDoubleValue(sourceFormat, OH_MD_KEY_FRAME_RATE, &frameRate);
    const char* language = nullptr;
    OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_LANGUAGE, &language);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    OH_AVFormat_GetBuffer(sourceFormat, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &bufferSize);
    language = OH_AVFormat_DumpInfo(sourceFormat);
    format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_TRACK_TYPE, params.setTrackType);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_DURATION, params.setDuration);
    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_HEIGHT, params.setHeight);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, params.setFrameRate);
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_LANGUAGE, setLanguage);
    char configBuffer[params.setCodecConfigSize];
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG, (uint8_t *)configBuffer, params.setCodecConfigSize);
    OH_AVFormat_Copy(format, sourceFormat);
    audioFormat = OH_AVFormat_CreateAudioFormat(OH_AVCODEC_MIMETYPE_AUDIO_AAC, params.sampleRate, params.channelCount);
    videoFormat = OH_AVFormat_CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC,
        params.setVideoWidth, params.setVideoHeight);
}

void DemuxerSample::RunNormalDemuxer(uint32_t createSize, const char *uri, const char *setLanguage, Params params)
{
    gReadEnd = false;
    DRM_MediaKeySystemInfo mediaKeySystemInfo;
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
    memory = OH_AVMemory_Create(createSize);
    while (!gReadEnd && gTrackCount > 0) {
        for (int32_t index = 0; index < gTrackCount; index++) {
            OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, index);
            if (trackFormat == nullptr) {
                gReadEnd = true;
                break;
            }
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &gTrackType);
            if (trackFormat) {
                OH_AVFormat_Destroy(trackFormat);
                trackFormat = nullptr;
            }
            const char *trackRefType = nullptr;
            const char *trackdescription = nullptr;
            int32_t *trackIds = nullptr;
            size_t bufferSize;
            OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_TRACK_REFERENCE_TYPE, &trackRefType);
            OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_TRACK_DESCRIPTION, &trackdescription);
            OH_AVFormat_GetIntBuffer(trackFormat, OH_MD_KEY_REFERENCE_TRACK_IDS, &trackIds, &bufferSize);
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
    GetAndSetFormat(setLanguage, params);
    uriSource = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    OH_AVDemuxer_GetMediaKeySystemInfo(demuxer, &mediaKeySystemInfo);
}

void DemuxerSample::RunNormalDemuxerApi11(uint32_t createSize, const char *uri, const char *setLanguage, Params params)
{
    gReadEnd = false;
    DRM_MediaKeySystemInfo mediaKeySystemInfo;
    int keyCount = 0;
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
    buffer = OH_AVBuffer_Create(createSize);
    while (!gReadEnd && gTrackCount > 0) {
        for (int32_t index = 0; index < gTrackCount; index++) {
            OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source, index);
            if (trackFormat == nullptr) {
                gReadEnd = true;
                break;
            }
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &gTrackType);
            if (trackFormat) {
                OH_AVFormat_Destroy(trackFormat);
                trackFormat = nullptr;
            }
            const char *trackRefType = nullptr;
            const char *trackdescription = nullptr;
            int32_t *trackIds = nullptr;
            size_t bufferSize;
            OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_TRACK_REFERENCE_TYPE, &trackRefType);
            OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_TRACK_DESCRIPTION, &trackdescription);
            OH_AVFormat_GetIntBuffer(trackFormat, OH_MD_KEY_REFERENCE_TRACK_IDS, &trackIds, &bufferSize);
            ret = OH_AVDemuxer_ReadSampleBuffer(demuxer, index, buffer);
            if (ret != 0) {
                gReadEnd = true;
                break;
            }
            OH_AVBuffer_GetBufferAttr(buffer, &attr);
            SetFileValue(attr, gReadEnd, keyCount);
        }
    }
    GetAndSetFormat(setLanguage, params);
    uriSource = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    OH_AVDemuxer_GetMediaKeySystemInfo(demuxer, &mediaKeySystemInfo);
}