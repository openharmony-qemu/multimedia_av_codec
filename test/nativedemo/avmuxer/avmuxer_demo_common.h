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

#ifndef AVMUXER_DEMO_COMMON_H
#define AVMUXER_DEMO_COMMON_H
#ifdef __cplusplus
extern "C" {
#endif
// only for demo
struct AudioTrackParam {
    const char *fileName;
    const char *mimeType;
    int sampleRate;
    int channels;
    int frameSize;
};

struct VideoTrackParam {
    const char *fileName;
    const char *mimeType;
    int width;
    int height;
    double frameRate;
    int videoDelay;
    int colorPrimaries;
    int colorTransfer;
    int colorMatrixCoeff;
    int colorRange;
    int isHdrVivid;
};

struct FdListStr {
    int start[0];
    int outputFd;
    int inAudioFd;
    int inVideoFd;
    int inCoverFd;
};

extern struct AudioTrackParam g_audioMpegPar;
extern struct AudioTrackParam g_audioAacPar;
extern struct AudioTrackParam g_audioAmrNbPar;
extern struct AudioTrackParam g_audioAmrWbPar;
extern struct VideoTrackParam g_videoH264Par;
extern struct VideoTrackParam g_videoMpeg4Par;
extern struct VideoTrackParam g_videoH265Par;
extern struct VideoTrackParam g_videoHdrPar;
extern struct VideoTrackParam g_jpegCoverPar;
extern struct VideoTrackParam g_pngCoverPar;
extern struct VideoTrackParam g_bmpCoverPar;
extern const char *RUN_NORMAL;
extern const char *RUN_MUL_THREAD;

long long GetTimestamp(void);
#ifdef __cplusplus
}
#endif
#endif