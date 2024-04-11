/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "native_avmuxer_demo.h"
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include "securec.h"
#include "native_avcodec_base.h"
#include "native_averrors.h"
#include "native_avformat.h"
#include "native_avmuxer.h"
#include "native_avmemory.h"
#include "native_avbuffer.h"
#include "avmuxer_demo_common.h"


#define NORMAL 0
#define THREAD 1
#define MODE_ZERO 0
#define MODE_ONE 1
#define MODE_TWO 2
#define MODE_THREE 3
#define MODE_FOUR 4
#define TYPE_BUFFER_SIZE 20
#define CONFIG_BUFFER_SIZE 0x1FFF

typedef struct AudioTrackParam AudioTrackParam;
typedef struct VideoTrackParam VideoTrackParam;
typedef struct FdListStr FdListStr;

struct WriteTrackSampleParam {
    OH_AVMuxer *muxer;
    int trackId;
    int fd;
};

struct MuxerParam {
    int outputFormat;
    char outputFormatType[TYPE_BUFFER_SIZE];
    int runMode;
    char runModeType[TYPE_BUFFER_SIZE];
    const AudioTrackParam *audioParams;
    char audioType[TYPE_BUFFER_SIZE];
    const VideoTrackParam *videoParams;
    char videoType[TYPE_BUFFER_SIZE];
    const VideoTrackParam *coverParams;
    char coverType[TYPE_BUFFER_SIZE];
};

static struct MuxerParam g_muxerParam  = {
    .outputFormat = AV_OUTPUT_FORMAT_DEFAULT,
    .outputFormatType = "",
    .runMode = NORMAL,
    .runModeType = "",
    .audioParams = NULL,
    .audioType = "",
    .videoParams = NULL,
    .videoType = "",
    .coverParams = NULL,
    .coverType = "",
};

int AddTrackAudio(OH_AVMuxer *muxer, const AudioTrackParam *param, int fdInput)
{
    if (fdInput < 0) {
        printf("unselect audio, fd is %d\n", fdInput);
        return -1;
    }
    OH_AVFormat *formatAudio = OH_AVFormat_CreateAudioFormat(param->mimeType,
        param->sampleRate, param->channels);
    if (formatAudio == NULL) {
        printf("audio format failed!\n");
        return AV_ERR_NO_MEMORY;
    }
    OH_AVFormat_SetIntValue(formatAudio, "audio_samples_per_frame", param->frameSize);
    if (param == &g_audioAacPar) {
        OH_AVFormat_SetIntValue(formatAudio, OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    }
    int extraSize = 0;
    unsigned char buffer[CONFIG_BUFFER_SIZE] = {0};
    read(fdInput, (void*)&extraSize, sizeof(extraSize));
    if (extraSize <= CONFIG_BUFFER_SIZE && extraSize > 0) {
        read(fdInput, buffer, extraSize);
        OH_AVFormat_SetBuffer(formatAudio, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
    }
    printf("AddTrackAudio audio metadata size: %d\n", extraSize);
    int trackIndex = -1;
    int ret = OH_AVMuxer_AddTrack(muxer, &trackIndex, formatAudio);
    OH_AVFormat_Destroy(formatAudio);
    if (ret != AV_ERR_OK) {
        printf("AddTrackAudio failed! mime: %s\n", param->mimeType);
        return -1;
    }
    printf("AddTrackAudio success! trackIndex: %d\n", trackIndex);
    return trackIndex;
}

int AddTrackVideo(OH_AVMuxer *muxer, const VideoTrackParam *param, int fdInput)
{
    if (fdInput < 0) {
        printf("unselect video, fd is %d\n", fdInput);
        return -1;
    }
    OH_AVFormat *formatVideo = OH_AVFormat_CreateVideoFormat(param->mimeType,
        param->width, param->height);
    if (formatVideo == NULL) {
        printf("video format failed!\n");
        return AV_ERR_NO_MEMORY;
    }
    OH_AVFormat_SetDoubleValue(formatVideo, OH_MD_KEY_FRAME_RATE, param->frameRate);
    OH_AVFormat_SetIntValue(formatVideo, "video_delay", param->videoDelay); // 不对外key
    if (param == &g_videoHdrPar) {
        OH_AVFormat_SetIntValue(formatVideo, OH_MD_KEY_COLOR_PRIMARIES, param->colorPrimaries);
        OH_AVFormat_SetIntValue(formatVideo, OH_MD_KEY_TRANSFER_CHARACTERISTICS, param->colorTransfer);
        OH_AVFormat_SetIntValue(formatVideo, OH_MD_KEY_MATRIX_COEFFICIENTS, param->colorMatrixCoeff);
        OH_AVFormat_SetIntValue(formatVideo, OH_MD_KEY_RANGE_FLAG, param->colorRange);
        OH_AVFormat_SetIntValue(formatVideo, OH_MD_KEY_VIDEO_IS_HDR_VIVID, param->isHdrVivid);
    }
    int extraSize = 0;
    unsigned char buffer[CONFIG_BUFFER_SIZE] = {0};
    read(fdInput, (void*)&extraSize, sizeof(extraSize));
    if (extraSize <= CONFIG_BUFFER_SIZE && extraSize > 0) {
        read(fdInput, buffer, extraSize);
        OH_AVFormat_SetBuffer(formatVideo, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
    }
    printf("AddTrackVideo video metadata size: %d\n", extraSize);
    int trackIndex = -1;
    int ret = OH_AVMuxer_AddTrack(muxer, &trackIndex, formatVideo);
    OH_AVFormat_Destroy(formatVideo);
    if (ret != AV_ERR_OK) {
        printf("AddTrackVideo failed! mime: %s\n", param->mimeType);
        return -1;
    }
    printf("AddTrackVideo success! trackIndex: %d\n", trackIndex);
    return trackIndex;
}

int AddTrackCover(OH_AVMuxer *muxer, const VideoTrackParam *param, int fdInput)
{
    if (fdInput < 0) {
        printf("unselect cover, fd is %d\n", fdInput);
        return -1;
    }

    OH_AVFormat *formatCover = OH_AVFormat_Create();
    if (formatCover == NULL) {
        printf("cover format failed!\n");
        return AV_ERR_NO_MEMORY;
    }
    OH_AVFormat_SetStringValue(formatCover, OH_MD_KEY_CODEC_MIME, param->mimeType);
    OH_AVFormat_SetIntValue(formatCover, OH_MD_KEY_WIDTH, param->width);
    OH_AVFormat_SetIntValue(formatCover, OH_MD_KEY_HEIGHT, param->height);
    int trackIndex = -1;
    int ret = OH_AVMuxer_AddTrack(muxer, &trackIndex, formatCover);
    OH_AVFormat_Destroy(formatCover);
    if (ret != AV_ERR_OK) {
        printf("AddTrackCover failed! mime: %s\n", param->mimeType);
        return -1;
    }
    printf("AddTrackCover success! trackIndex: %d\n", trackIndex);
    return trackIndex;
}

static bool UpdateWriteBufferInfo(int fd, OH_AVMemory **buffer, OH_AVCodecBufferAttr *info)
{
    if (fd < 0 || buffer == NULL || info == NULL) {
        return false;
    }

    int ret = read(fd, (void*)&info->pts, sizeof(info->pts));
    if (ret <= 0) {
        return false;
    }

    ret = read(fd, (void*)&info->flags, sizeof(info->flags));
    if (ret <= 0) {
        return false;
    }

    ret = read(fd, (void*)&info->size, sizeof(info->size));
    if (ret <= 0 || info->size < 0) {
        return false;
    }

    if (*buffer != NULL && OH_AVMemory_GetSize(*buffer) < info->size) {
        OH_AVMemory_Destroy(*buffer);
        *buffer = NULL;
    }
    if (*buffer == NULL) {
        *buffer = OH_AVMemory_Create(info->size);
    }
    if (*buffer == NULL) {
        printf("error create OH_AVMemory! %d\n", info->size);
        return false;
    }
    ret = read(fd, (void*)OH_AVMemory_GetAddr(*buffer), info->size);
    if (ret <= 0) {
        return false;
    }
    return true;
}

void WriteSingleTrackSample(OH_AVMuxer *muxer, int trackId, int fd)
{
    if (muxer == NULL || fd < 0 || trackId  < 0) {
        printf("WriteSingleTrackSample muxer is null or fd < 0, fd:%d\n", fd);
        return;
    }
    OH_AVMemory *buffer = NULL;
    OH_AVCodecBufferAttr info;
    memset_s(&info, sizeof(info), 0, sizeof(info));
    bool ret = UpdateWriteBufferInfo(fd, &buffer, &info);
    while (ret) {
        if (OH_AVMuxer_WriteSample(muxer, trackId, buffer, info) != AV_ERR_OK) {
            printf("OH_AVMuxer_WriteSample error!\n");
            break;
        }
        ret = UpdateWriteBufferInfo(fd, &buffer, &info);
    }

    if (buffer != NULL) {
        OH_AVMemory_Destroy(buffer);
    }
}

void *ThreadWriteTrackSample(void *param)
{
    struct WriteTrackSampleParam *wrTrackParam = (struct WriteTrackSampleParam *)param;
    WriteSingleTrackSample(wrTrackParam->muxer, wrTrackParam->trackId, wrTrackParam->fd);
    return NULL;
}

void WriteTrackSample(OH_AVMuxer *muxer, int audioTrackIndex, int videoTrackIndex, FdListStr *fdStr)
{
    if (fdStr == NULL || fdStr->inAudioFd < 0 || fdStr->inVideoFd < 0) {
        printf("WriteTrackSample start failed!\n");
        return;
    }
    printf("WriteTrackSample\n");
    OH_AVMemory *audioBuffer = NULL;
    OH_AVMemory *videoBuffer = NULL;
    OH_AVCodecBufferAttr audioInfo;
    OH_AVCodecBufferAttr videoInfo;
    memset_s(&audioInfo, sizeof(audioInfo), 0, sizeof(audioInfo));
    memset_s(&videoInfo, sizeof(videoInfo), 0, sizeof(videoInfo));
    bool audioRet = UpdateWriteBufferInfo(fdStr->inAudioFd, &audioBuffer, &audioInfo);
    bool videoRet = UpdateWriteBufferInfo(fdStr->inVideoFd, &videoBuffer, &videoInfo);
    bool isOver = false;

    while ((audioRet || videoRet) && !isOver) {
        int ret = AV_ERR_OK;
        if (audioRet && videoRet && audioInfo.pts <= videoInfo.pts) {
            ret = OH_AVMuxer_WriteSample(muxer, audioTrackIndex, audioBuffer, audioInfo);
            audioRet = UpdateWriteBufferInfo(fdStr->inAudioFd, &audioBuffer, &audioInfo);
        } else if (audioRet && videoRet) {
            ret = OH_AVMuxer_WriteSample(muxer, videoTrackIndex, videoBuffer, videoInfo);
            videoRet = UpdateWriteBufferInfo(fdStr->inVideoFd, &videoBuffer, &videoInfo);
        } else if (audioRet) {
            ret = OH_AVMuxer_WriteSample(muxer, audioTrackIndex, audioBuffer, audioInfo);
            isOver = true;
        } else {
            ret = OH_AVMuxer_WriteSample(muxer, videoTrackIndex, videoBuffer, videoInfo);
            isOver = true;
        }
        if (ret != AV_ERR_OK) {
            printf("OH_AVMuxer_WriteSample error!\n");
            break;
        }
    }
    if (audioBuffer != NULL) {
        OH_AVMemory_Destroy(audioBuffer);
    }
    if (videoBuffer != NULL) {
        OH_AVMemory_Destroy(videoBuffer);
    }
}

void WriteTrackCover(OH_AVMuxer *muxer, int coverTrackIndex, int fdInput)
{
    printf("WriteTrackCover\n");
    OH_AVCodecBufferAttr info;
    memset_s(&info, sizeof(info), 0, sizeof(info));
    struct stat fileStat;
    fstat(fdInput, &fileStat);
    info.size = fileStat.st_size;
    OH_AVBuffer *avMemBuffer = OH_AVBuffer_Create(info.size);
    if (avMemBuffer == NULL) {
        printf("OH_AVBuffer create error! size: %d \n", info.size);
        return;
    }
    if (OH_AVBuffer_SetBufferAttr(avMemBuffer, &info) != AV_ERR_OK) {
        printf("OH_AVBuffer_SetBufferAttr error!\n");
        return;
    }
    if (read(fdInput, (void*)OH_AVBuffer_GetAddr(avMemBuffer), info.size) <= 0) {
        printf("OH_AVBuffer_GetAddr error!\n");
        OH_AVBuffer_Destroy(avMemBuffer);
        return;
    }
    if (OH_AVMuxer_WriteSampleBuffer(muxer, coverTrackIndex, avMemBuffer) != AV_ERR_OK) {
        printf("OH_AVMuxer_WriteSampleBuffer error!\n");
        OH_AVBuffer_Destroy(avMemBuffer);
        return;
    }
    OH_AVBuffer_Destroy(avMemBuffer);
}

int GetInputNum(int defaultNum)
{
    int num = getchar();
    if (num == '\n') { // default
        return defaultNum;
    }
    if (ungetc(num, stdin) == EOF) {
        printf("GetInputNum ungetc failed!");
    }
    if (scanf_s("%d", &num) <= 0) {
        num = defaultNum;
    }
    while ((getchar()) != '\n') {}
    return num;
}

void NativeSelectMuxerType(void)
{
    printf("\nplese select muxer type : 0.mp4 1.m4a 2.amr 3.mp3\n");
    int num = GetInputNum(0);
    switch (num) {
        case MODE_ZERO:
            g_muxerParam.outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
            (void)snprintf_s(g_muxerParam.outputFormatType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "mp4");
            break;
        case MODE_ONE:
            g_muxerParam.outputFormat = AV_OUTPUT_FORMAT_M4A;
            (void)snprintf_s(g_muxerParam.outputFormatType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "m4a");
            break;
        case MODE_TWO:
            g_muxerParam.outputFormat = AV_OUTPUT_FORMAT_AMR;
            (void)snprintf_s(g_muxerParam.outputFormatType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "amr");
            break;
        case MODE_THREE:
            g_muxerParam.outputFormat = AV_OUTPUT_FORMAT_MP3;
            (void)snprintf_s(g_muxerParam.outputFormatType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "mp3");
            break;
        default:
            g_muxerParam.outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
            (void)snprintf_s(g_muxerParam.outputFormatType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "mp4");
            break;
    }
    printf("select mode:%d\n", num);
}

void NativeSelectRunMode(void)
{
    printf("\nplese select audio vide wrtie mode:\n");
    printf("0. audio video write in sample thread\n");
    printf("1. audio video write in different thread\n");
    int num = GetInputNum(0);
    switch (num) {
        case MODE_ZERO:
            g_muxerParam.runMode = NORMAL;
            (void)snprintf_s(g_muxerParam.runModeType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", RUN_NORMAL);
            break;
        case MODE_ONE:
            g_muxerParam.runMode = THREAD;
            (void)snprintf_s(g_muxerParam.runModeType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", RUN_MUL_THREAD);
            break;
        default:
            g_muxerParam.runMode = NORMAL;
            (void)snprintf_s(g_muxerParam.runModeType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", RUN_NORMAL);
            break;
    }
    printf("select mode:%d\n", num);
}

void NativeSelectAudio(void)
{
    printf("\nplese select audio mode: 0.noAudio 1.aac 2.mpeg 3.amr-nb 4.amr-wb\n");
    int num = GetInputNum(1);
    switch (num) {
        case MODE_ONE:
            g_muxerParam.audioParams = &g_audioAacPar;
            (void)snprintf_s(g_muxerParam.audioType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "aac");
            break;
        case MODE_TWO:
            g_muxerParam.audioParams = &g_audioMpegPar;
            (void)snprintf_s(g_muxerParam.audioType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "mpeg");
            break;
        case MODE_THREE:
            g_muxerParam.audioParams = &g_audioAmrNbPar;
            (void)snprintf_s(g_muxerParam.audioType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "amr");
            break;
        case MODE_FOUR:
            g_muxerParam.audioParams = &g_audioAmrWbPar;
            (void)snprintf_s(g_muxerParam.audioType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "amr");
            break;
        default:
            g_muxerParam.audioParams = NULL;
            (void)snprintf_s(g_muxerParam.audioType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "noAudio");
            break;
    }
    printf("select mode:%d\n", num);
}

void NativeSelectVideo(void)
{
    printf("\nplese select video mode: 0.noVideo 1.h264 2.mpeg4 3.h265 4.hdr vivid\n");
    int num = GetInputNum(1);
    switch (num) {
        case MODE_ONE:
            g_muxerParam.videoParams = &g_videoH264Par;
            (void)snprintf_s(g_muxerParam.videoType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "h264");
            break;
        case MODE_TWO:
            g_muxerParam.videoParams = &g_videoMpeg4Par;
            (void)snprintf_s(g_muxerParam.videoType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "mpeg4");
            break;
        case MODE_THREE:
            g_muxerParam.videoParams = &g_videoH265Par;
            (void)snprintf_s(g_muxerParam.videoType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "h265");
            break;
        case MODE_FOUR:
            g_muxerParam.videoParams = &g_videoHdrPar;
            (void)snprintf_s(g_muxerParam.videoType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "hdr-vivid");
            break;
        default:
            g_muxerParam.videoParams = NULL;
            (void)snprintf_s(g_muxerParam.videoType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "noVideo");
            break;
    }
    printf("select mode:%d\n", num);
}

void NativeSelectCover(void)
{
    printf("\nplese select cover mode: 0.noCover 1.jpg 2.png 3.bmp\n");
    int num = GetInputNum(1);
    switch (num) {
        case MODE_ONE:
            g_muxerParam.coverParams = &g_jpegCoverPar;
            (void)snprintf_s(g_muxerParam.coverType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "jpg");
            break;
        case MODE_TWO:
            g_muxerParam.coverParams = &g_pngCoverPar;
            (void)snprintf_s(g_muxerParam.coverType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "png");
            break;
        case MODE_THREE:
            g_muxerParam.coverParams = &g_bmpCoverPar;
            (void)snprintf_s(g_muxerParam.coverType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "bmp");
            break;
        default:
            g_muxerParam.coverParams = NULL;
            (void)snprintf_s(g_muxerParam.coverType, TYPE_BUFFER_SIZE, TYPE_BUFFER_SIZE - 1, "%s", "noCover");
            break;
    }
    printf("select mode:%d\n", num);
}

void NativeSelectMode(void)
{
    if (g_muxerParam.outputFormat != AV_OUTPUT_FORMAT_DEFAULT) {
        return;
    }

    NativeSelectMuxerType();
    NativeSelectRunMode();
    NativeSelectAudio();
    NativeSelectVideo();
    NativeSelectCover();
}

int OpenAllInputFile(FdListStr *fdStr)
{
    if (!fdStr) {
        printf("fdStr is null!\n");
        return -1;
    }

    if (g_muxerParam.audioParams) {
        fdStr->inAudioFd = open(g_muxerParam.audioParams->fileName, O_RDONLY);
        if (fdStr->inAudioFd < 0) {
            printf("open %s failed!!\n", g_muxerParam.audioParams->fileName);
        } else {
            printf("open file %s success, -fd:%d, -flags %x\n", g_muxerParam.audioParams->fileName,
                fdStr->inAudioFd, fcntl(fdStr->inAudioFd, F_GETFL, 0));
        }
    }

    if (g_muxerParam.videoParams) {
        fdStr->inVideoFd = open(g_muxerParam.videoParams->fileName, O_RDONLY);
        if (fdStr->inVideoFd < 0) {
            printf("open %s failed!!\n", g_muxerParam.videoParams->fileName);
        } else {
            printf("open file %s success, -fd:%d, -flags %x\n", g_muxerParam.videoParams->fileName,
                fdStr->inVideoFd, fcntl(fdStr->inVideoFd, F_GETFL, 0));
        }
    }

    if (g_muxerParam.coverParams) {
        fdStr->inCoverFd = open(g_muxerParam.coverParams->fileName, O_RDONLY);
        if (fdStr->inCoverFd < 0) {
            printf("open %s failed!!\n", g_muxerParam.coverParams->fileName);
        } else {
            printf("open file %s success, -fd:%d, -flags %x\n", g_muxerParam.coverParams->fileName,
                fdStr->inCoverFd, fcntl(fdStr->inCoverFd, F_GETFL, 0));
        }
    }
    return 0;
}

int DoRunMuxer(FdListStr *fdStr, OH_AVMuxer *muxer)
{
    if (fdStr == NULL || muxer == NULL) {
        printf("fdStr or  muxer is null!\n");
        return -1;
    }

    if (OH_AVMuxer_SetRotation(muxer, 0) != AV_ERR_OK) {
        printf("set failed!\n");
        return -1;
    }
    int audioTrackIndex = AddTrackAudio(muxer, g_muxerParam.audioParams, fdStr->inAudioFd);
    int videoTrackIndex = AddTrackVideo(muxer, g_muxerParam.videoParams, fdStr->inVideoFd);
    int coverTrackIndex = AddTrackCover(muxer, g_muxerParam.coverParams, fdStr->inCoverFd);

    if (OH_AVMuxer_Start(muxer) != AV_ERR_OK) {
        printf("start muxer failed!\n");
        return -1;
    }

    if (coverTrackIndex >= 0) {
        WriteTrackCover(muxer, coverTrackIndex, fdStr->inCoverFd);
    }

    if (g_muxerParam.runMode == NORMAL) {
        printf("== write audio video sample in same thread\n");
        if (audioTrackIndex >= 0 && videoTrackIndex >= 0) {
            WriteTrackSample(muxer, audioTrackIndex, videoTrackIndex, fdStr);
        } else if (audioTrackIndex >= 0) {
            WriteSingleTrackSample(muxer, audioTrackIndex, fdStr->inAudioFd);
        } else if (videoTrackIndex >= 0) {
            WriteSingleTrackSample(muxer, videoTrackIndex, fdStr->inVideoFd);
        }
    } else if (g_muxerParam.runMode == THREAD) {
        printf("== write audio video sample in different thread\n");
        pthread_t auThread;
        pthread_t viThread;

        struct WriteTrackSampleParam audioThParam = {muxer, audioTrackIndex, fdStr->inAudioFd};
        struct WriteTrackSampleParam videoThparam = {muxer, videoTrackIndex, fdStr->inVideoFd};
        pthread_create(&auThread, NULL, ThreadWriteTrackSample, &audioThParam);
        pthread_create(&viThread, NULL, ThreadWriteTrackSample, &videoThparam);

        pthread_join(viThread, NULL);
        pthread_join(auThread, NULL);
    }

    if (OH_AVMuxer_Stop(muxer) != AV_ERR_OK) {
        printf("stop muxer failed!\n");
        return -1;
    }
    printf("native avmuxer finish! fd:out:%d, audio:%d, video:%d, cover:%d\n",
        fdStr->outputFd, fdStr->inAudioFd, fdStr->inVideoFd, fdStr->inCoverFd);
    return 0;
}

void CloseAllFd(FdListStr *fdStr)
{
    printf("close fd : [");
    int fdTotalCount = sizeof(*fdStr) / sizeof(fdStr->start[0]);
    for (int i = 0; i < fdTotalCount; i++) {
        printf("%d, ", fdStr->start[i]);
        if (fdStr->start[i] > 0) {
            close(fdStr->start[i]);
            fdStr->start[i] = -1;
        }
    }
    printf("\b\b]\n");
}

int RunNativeMuxer(const char *out)
{
    FdListStr fdStr;
    int fdTotalCount = sizeof(fdStr) / sizeof(fdStr.start[0]);
    printf("fd list total size is %d\n", fdTotalCount);
    for (int i = 0; i < fdTotalCount; i++) {
        fdStr.start[i] = -1;
    }

    if (OpenAllInputFile(&fdStr) < 0) {
        CloseAllFd(&fdStr);
        return -1;
    }

    char outFileName[CONFIG_BUFFER_SIZE] = {0};
    int err = snprintf_s(outFileName, sizeof(outFileName), sizeof(outFileName) - 1, "%s_%s_%s_%s_%s.%s",
        out, g_muxerParam.runModeType, g_muxerParam.audioType, g_muxerParam.videoType,
        g_muxerParam.coverType, g_muxerParam.outputFormatType);
    if (err <= 0) {
        CloseAllFd(&fdStr);
        return -1;
    }

    fdStr.outputFd = open(outFileName, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fdStr.outputFd < 0) {
        printf("open file failed! filePath is: %s %d\n", outFileName, fdStr.outputFd);
        CloseAllFd(&fdStr);
        return -1;
    }
    printf("open file %s success, -fd:%d, -flags %x\n", outFileName, fdStr.outputFd, fcntl(fdStr.outputFd, F_GETFL, 0));
    long long testTimeStart = GetTimestamp();
    OH_AVMuxer *muxer = OH_AVMuxer_Create(fdStr.outputFd, g_muxerParam.outputFormat);
    DoRunMuxer(&fdStr, muxer);

    if (muxer != NULL) {
        OH_AVMuxer_Destroy(muxer);
        muxer = NULL;
    }

    CloseAllFd(&fdStr);
    long long testTimeEnd = GetTimestamp();
    printf("muxer used time: %lld us\n", testTimeEnd - testTimeStart);

    return 0;
}
