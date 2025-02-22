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
#include "hdrmuxer_sample.h"

#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <list>
#include "av_common.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "media_description.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace std;
constexpr int64_t MICRO_IN_SECOND = 1000000L;
constexpr int32_t AUDIO_BUFFER_SIZE = 1024 * 1024;
constexpr double DEFAULT_FRAME_RATE = 25.0;
constexpr uint32_t INVALID_TRACK_ID = -1;

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


void MuxerSample::InitMuxerDeMuxer(const char *mp4File)
{
    fd = open(mp4File, O_RDONLY);
    outFd = open("./output.mp4", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    int64_t size = GetFileSize(mp4File);
    inSource = OH_AVSource_CreateWithFD(fd, 0, size);
    if (!inSource) {
        cout << "create source failed" << endl;
    }

    demuxer = OH_AVDemuxer_CreateWithSource(inSource);
    muxer = OH_AVMuxer_Create(outFd, AV_OUTPUT_FORMAT_MPEG_4);
    if (!muxer || !demuxer) {
        cout << "create muxer demuxer failed" << endl;
    }

    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(inSource);
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        OH_AVDemuxer_SelectTrackByID(demuxer, index);
        OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(inSource, index);
        int32_t muxTrack = 0;
        int32_t trackType = -1;
        OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType);
        if (trackType == MEDIA_TYPE_VID) {
            double frameRate = 0.0;
            OH_AVFormat_GetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, &frameRate);
            if (frameRate <= 0) {
                frameRate = DEFAULT_FRAME_RATE;
            }
            if (frameRate != 0) {
                frameDuration = MICRO_IN_SECOND / frameRate;
            }
            videoTrackID = index;
            OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
        } else if (trackType == MEDIA_TYPE_AUD) {
            audioTrackID = index;
        }
        OH_AVMuxer_AddTrack(muxer, &muxTrack, trackFormat);
        OH_AVFormat_Destroy(trackFormat);
    }
    OH_AVFormat_Destroy(sourceFormat);
}

MuxerSample::~MuxerSample()
{
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

void MuxerSample::RunHdrMuxer(const uint8_t *data, size_t size, const char *mp4File)
{
    fuzzSize = size;
    fuzzData = data;
    InitMuxerDeMuxer(mp4File);

    Start();
    WaitForEOS();
}

void MuxerSample::WriteAudioTrack()
{
    OH_AVBuffer *buffer = nullptr;
    buffer = OH_AVBuffer_Create(AUDIO_BUFFER_SIZE);
    while (!isAudioFinish.load()) {
        OH_AVDemuxer_ReadSampleBuffer(demuxer, audioTrackID, buffer);
        OH_AVCodecBufferAttr attr;
        OH_AVBuffer_GetBufferAttr(buffer, &attr);
        if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
            break;
        }
        OH_AVMuxer_WriteSampleBuffer(muxer, audioTrackID, buffer);
    }
    OH_AVBuffer_Destroy(buffer);
}

void MuxerSample::WriteVideoTrack()
{
    OH_AVBuffer *buffer = nullptr;
    buffer = OH_AVBuffer_Create(fuzzSize);
    while (!isVideoFinish.load()) {
        uint8_t *bufferAddr = OH_AVBuffer_GetAddr(buffer);
        memcpy_s(bufferAddr, fuzzSize, fuzzData, fuzzSize);
        OH_AVCodecBufferAttr attr;
        OH_AVBuffer_GetBufferAttr(buffer, &attr);
        if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
            break;
        }
        OH_AVMuxer_WriteSampleBuffer(muxer, videoTrackID, buffer);
    }
    OH_AVBuffer_Destroy(buffer);
}

void MuxerSample::Start()
{
    OH_AVMuxer_Start(muxer);
    cout << "hdr muxer start" << endl;
    if (audioTrackID != INVALID_TRACK_ID) {
        audioThread = make_unique<thread>(&MuxerSample::WriteAudioTrack, this);
    }
    if (videoTrackID != INVALID_TRACK_ID) {
        videoThread = make_unique<thread>(&MuxerSample::WriteVideoTrack, this);
    }
}

void MuxerSample::WaitForEOS()
{
    std::mutex audioWaitMtx;
    std::mutex videoWaitMtx;
    unique_lock<mutex> audioLock(audioWaitMtx);
    unique_lock<mutex> videoLock(videoWaitMtx);
    waitCond.wait(audioLock, [this]() {
        return isAudioFinish.load();
    });
    waitCond.wait(videoLock, [this]() {
        return isVideoFinish.load();
    });
    if (audioThread) {
        audioThread->join();
    }
    if (videoThread) {
        videoThread->join();
    }
    cout << "task finish" << endl;
}

void MuxerSample::Stop()
{
    OH_AVMuxer_Stop(muxer);
}