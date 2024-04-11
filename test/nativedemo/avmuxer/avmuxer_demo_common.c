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

#include "avmuxer_demo_common.h"
#include <sys/time.h>
#include <time.h>

struct AudioTrackParam g_audioMpegPar = {
    .fileName = "mpeg_44100_2.dat",
    .mimeType = "audio/mpeg",
    .sampleRate = 44100,
    .channels = 2,
    .frameSize = 1152,
};

struct AudioTrackParam g_audioAacPar = {
    .fileName = "aac_44100_2.dat",
    .mimeType = "audio/mp4a-latm",
    .sampleRate = 44100,
    .channels = 2,
    .frameSize = 1024,
};

struct AudioTrackParam g_audioAmrNbPar = {
    .fileName = "amrnb_8000_1.dat",
    .mimeType = "audio/3gpp",
    .sampleRate = 8000,
    .channels = 1,
    .frameSize = 1024,
};

struct AudioTrackParam g_audioAmrWbPar = {
    .fileName = "amrwb_16000_1.dat",
    .mimeType = "audio/amr-wb",
    .sampleRate = 16000,
    .channels = 1,
    .frameSize = 1024,
};

struct VideoTrackParam g_videoH264Par = {
    .fileName = "h264_720_480.dat",
    .mimeType = "video/avc",
    .width = 720,
    .height = 480,
    .frameRate = 60,
    .videoDelay = 0,
    .colorPrimaries = 2,
    .colorTransfer = 2,
    .colorMatrixCoeff = 2,
    .colorRange = 0,
    .isHdrVivid = 0,
};

struct VideoTrackParam g_videoMpeg4Par = {
    .fileName = "mpeg4_720_480.dat",
    .mimeType = "video/mp4v-es",
    .width = 720,
    .height = 480,
    .frameRate = 60,
    .videoDelay = 0,
    .colorPrimaries = 2,
    .colorTransfer = 2,
    .colorMatrixCoeff = 2,
    .colorRange = 0,
    .isHdrVivid = 0,
};

struct VideoTrackParam g_videoH265Par = {
    .fileName = "h265_720_480.dat",
    .mimeType = "video/hevc",
    .width = 720,
    .height = 480,
    .frameRate = 60,
    .videoDelay = 2,
    .colorPrimaries = 2,
    .colorTransfer = 2,
    .colorMatrixCoeff = 2,
    .colorRange = 0,
    .isHdrVivid = 0,
};

struct VideoTrackParam g_videoHdrPar = {
    .fileName = "hdr_vivid_3840_2160.dat",
    .mimeType = "video/hevc",
    .width = 3840,
    .height = 2160,
    .frameRate = 30,
    .videoDelay = 0,
    .colorPrimaries = 9,
    .colorTransfer = 18,
    .colorMatrixCoeff = 9,
    .colorRange = 0,
    .isHdrVivid = 1,
};

struct VideoTrackParam g_jpegCoverPar = {
    .fileName = "greatwall.jpg",
    .mimeType = "image/jpeg",
    .width = 352,
    .height = 288,
};

struct VideoTrackParam g_pngCoverPar = {
    .fileName = "greatwall.png",
    .mimeType = "image/png",
    .width = 352,
    .height = 288,
};

struct VideoTrackParam g_bmpCoverPar = {
    .fileName = "greatwall.bmp",
    .mimeType = "image/bmp",
    .width = 352,
    .height = 288,
};

const char *RUN_NORMAL = "normal";
const char *RUN_MUL_THREAD = "multhrd";

long long GetTimestamp(void)
{
    static const int timeScaleUs = 1000000;
    long long tmp;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    tmp = tv.tv_sec;
    tmp = tmp * timeScaleUs;
    tmp = tmp + tv.tv_usec;

    return tmp;
}