/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "avcodec_e2e_demo_api10.h"

#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <list>
#include "av_common.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "native_avcodec_videodecoder.h"
#include "native_avcodec_videoencoder.h"
#include "media_description.h"
#include "native_avformat.h"
#include "native_avcodec_base.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::E2EDemo;
using namespace std;
constexpr int64_t MICRO_IN_SECOND = 1000000L;
constexpr int32_t AUDIO_BUFFER_SIZE = 1024 * 1024;
constexpr float FRAME_INTERVAL_TIMES = 1.5;
constexpr double DEFAULT_FRAME_RATE = 25.0;
typedef struct FrameInfo {
    uint32_t index;
    int32_t pts;
}FrameInfo;
static list<FrameInfo> frameList;

static bool FrameCompare(const FrameInfo &f1, const FrameInfo &f2)
{
    return f1.pts < f2.pts;
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

static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)userData;
    cout<<"error :"<<errorCode<<endl;
}

static void OnDecStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
}

static void OnDecInputDataAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    AVCodecE2EDemoAPI10 *demo = static_cast<AVCodecE2EDemoAPI10*>(userData);
    OH_AVCodecBufferAttr info;
    OH_AVDemuxer_ReadSample(demo->demuxer, demo->videoTrackID, data, &info);
    OH_VideoDecoder_PushInputData(codec, index, info);
}

static void sortFrame(OH_AVCodec *codec, uint32_t index, int32_t pts, uint32_t duration)
{
    FrameInfo info = {index, pts};
    frameList.emplace_back(info);
    if (frameList.size() > 1) {
        frameList.sort(FrameCompare);
        FrameInfo first = frameList.front();
        auto it = frameList.begin();
        FrameInfo second = *(++it);
        if (second.pts - first.pts <= (duration * FRAME_INTERVAL_TIMES)) {
            OH_VideoDecoder_RenderOutputData(codec, first.index);
            frameList.pop_front();
        }
    }
}

static void OnDecOutputDataAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
                                     OH_AVCodecBufferAttr *attr, void *userData)
{
    AVCodecE2EDemoAPI10 *demo = static_cast<AVCodecE2EDemoAPI10*>(userData);
    if (attr->flags & AVCODEC_BUFFER_FLAGS_EOS) {
        frameList.sort(FrameCompare);
        while (frameList.size() > 0) {
            FrameInfo first = frameList.front();
            OH_VideoDecoder_RenderOutputData(codec, first.index);
            frameList.pop_front();
        }
        OH_VideoEncoder_NotifyEndOfStream(demo->enc);
        return;
    }
    sortFrame(codec, index, attr->pts, demo->frameDuration);
}

static void OnEncStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout<<"format changed"<<endl;
}

static void OnEncInputDataAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)codec;
    (void)index;
    (void)data;
    (void)userData;
}

static void OnEncOutputDataAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
                                     OH_AVCodecBufferAttr *attr, void *userData)
{
    AVCodecE2EDemoAPI10 *demo = static_cast<AVCodecE2EDemoAPI10*>(userData);
    if (attr->flags & AVCODEC_BUFFER_FLAGS_EOS) {
        demo->isFinish.store(true);
        demo->waitCond.notify_all();
        return;
    }
    OH_AVMuxer_WriteSample(demo->muxer, demo->videoTrackID, data, *attr);
    OH_VideoDecoder_FreeOutputData(codec, index);
}

AVCodecE2EDemoAPI10::AVCodecE2EDemoAPI10(const char *file)
{
    fd = open(file, O_RDONLY);
    outFd = open("./output.mp4", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    int64_t size = GetFileSize(file);
    inSource = OH_AVSource_CreateWithFD(fd, 0, size);
    if (!inSource) {
        cout << "create source failed" << endl;
    }
    demuxer = OH_AVDemuxer_CreateWithSource(inSource);
    muxer = OH_AVMuxer_Create(outFd, AV_OUTPUT_FORMAT_MPEG_4);
    if (!muxer || !demuxer) {
        cout << "create muxer demuxer failed" << endl;
    }
    dec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    enc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    if (!enc || !dec) {
        cout << "create codec failed" << endl;
    }
    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(inSource);
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount);
    for (int32_t index = 0; index < trackCount; index++) {
        OH_AVDemuxer_SelectTrackByID(demuxer, index);
        OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(inSource, index);
        int32_t muxTrack = 0;
        OH_AVMuxer_AddTrack(muxer, &muxTrack, trackFormat);
        int32_t trackType = 0;
        OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType);
        if (trackType == MEDIA_TYPE_VID) {
            int32_t rotation = 0;
            double frameRate = 0.0;
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_ROTATION, &rotation);
            OH_AVFormat_GetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, &frameRate);
            if (frameRate <= 0) {
                frameRate = DEFAULT_FRAME_RATE;
            }
            frameDuration = MICRO_IN_SECOND / frameRate;
            OH_AVMuxer_SetRotation(muxer, rotation);
            videoTrackID = index;
            OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
            OH_VideoDecoder_Configure(dec, trackFormat);
            OH_VideoEncoder_Configure(enc, trackFormat);
        } else {
            audioTrackID = index;
        }
        OH_AVFormat_Destroy(trackFormat);
    }
    OH_AVFormat_Destroy(sourceFormat);
}

AVCodecE2EDemoAPI10::~AVCodecE2EDemoAPI10()
{
    if (dec) {
        OH_VideoDecoder_Destroy(dec);
    }
    if (enc) {
        OH_VideoEncoder_Destroy(enc);
    }
    if (muxer) {
        OH_AVMuxer_Destroy(muxer);
    }
    if (demuxer) {
        OH_AVDemuxer_Destroy(demuxer);
    }
    if (inSource) {
        OH_AVSource_Destroy(inSource);
    }
    close(fd);
    close(outFd);
}

void AVCodecE2EDemoAPI10::Configure()
{
    OH_AVCodecAsyncCallback encCallback;
    encCallback.onError = OnError;
    encCallback.onStreamChanged = OnEncStreamChanged;
    encCallback.onNeedInputData = OnEncInputDataAvailable;
    encCallback.onNeedOutputData = OnEncOutputDataAvailable;
    OH_VideoEncoder_SetCallback(enc, encCallback, this);

    OH_AVCodecAsyncCallback decCallback;
    decCallback.onError = OnError;
    decCallback.onStreamChanged = OnDecStreamChanged;
    decCallback.onNeedInputData = OnDecInputDataAvailable;
    decCallback.onNeedOutputData = OnDecOutputDataAvailable;
    OH_VideoDecoder_SetCallback(dec, decCallback, this);
    OHNativeWindow *window = nullptr;
    OH_VideoEncoder_GetSurface(enc, &window);
    OH_VideoDecoder_SetSurface(dec, window);
    isFinish.store(false);
}

void AVCodecE2EDemoAPI10::WriteAudioTrack()
{
    OH_AVMemory *buffer = nullptr;
    buffer = OH_AVMemory_Create(AUDIO_BUFFER_SIZE);
    while (true) {
        if (isFinish.load()) {
            break;
        }
        OH_AVCodecBufferAttr info;
        OH_AVDemuxer_ReadSample(demuxer, audioTrackID, buffer, &info);
        if (info.flags & AVCODEC_BUFFER_FLAGS_EOS) {
            break;
        }
        OH_AVMuxer_WriteSample(muxer, audioTrackID, buffer, info);
    }
    OH_AVMemory_Destroy(buffer);
}

void AVCodecE2EDemoAPI10::Start()
{
    OH_VideoDecoder_Prepare(dec);
    OH_VideoEncoder_Prepare(enc);
    OH_AVMuxer_Start(muxer);
    OH_VideoEncoder_Start(enc);
    OH_VideoDecoder_Start(dec);
    if (audioTrackID != -1) {
        audioThread = make_unique<thread>(&AVCodecE2EDemoAPI10::WriteAudioTrack, this);
    }
}

void AVCodecE2EDemoAPI10::WaitForEOS()
{
    std::mutex waitMtx;
    unique_lock<mutex> lock(waitMtx);
    waitCond.wait(lock, [this]() {
        return isFinish.load();
    });
    if (audioThread) {
        audioThread->join();
    }
    cout << "task finish" << endl;
}

void AVCodecE2EDemoAPI10::Stop()
{
    OH_VideoDecoder_Stop(dec);
    OH_VideoEncoder_Stop(enc);
    OH_AVMuxer_Stop(muxer);
}