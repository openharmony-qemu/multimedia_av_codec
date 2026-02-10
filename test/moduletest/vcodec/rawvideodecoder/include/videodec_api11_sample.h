/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef VIDEODEC_API11_SAMPLE_H
#define VIDEODEC_API11_SAMPLE_H

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <fcntl.h>
#include "securec.h"
#include "native_avcodec_videodecoder.h"
#include "nocopyable.h"
#include "native_avmemory.h"
#include "native_avformat.h"
#include "native_averrors.h"
#include "surface/window.h"
#include "iconsumer_surface.h"
#include "native_avdemuxer.h"
#include "native_avsource.h"
#include "native_avcapability.h"
#include "libavutil/pixfmt.h"
#include "gtest/gtest.h"

namespace OHOS {
namespace Media {
class VDecAPI11Signal {
public:
    std::mutex inMutex;
    std::mutex outMutex;
    std::condition_variable inCond;
    std::condition_variable outCond;
    std::queue<uint32_t> inIdxQueue;
    std::queue<uint32_t> outIdxQueue;
    std::queue<OH_AVCodecBufferAttr> attrQueue;
    std::queue<OH_AVBuffer *> inBufferQueue;
    std::queue<OH_AVBuffer *> outBufferQueue;
};

class VDecAPI11Sample : public NoCopyable {
public:
    VDecAPI11Sample() = default;
    ~VDecAPI11Sample();
    int32_t RunVideoDec_Surface(std::string codeName = "");
    int32_t RunVideoDec(std::string codeName = "");
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t state_EOS();
    void SetEOS(uint32_t index, OH_AVBuffer *buffer);
    void WaitForEOS();
    int32_t ConfigureVideoDecoder();
    int32_t ConfigureVideoDecoderNoPixelFormat();
    int32_t StartVideoDecoder();
    int32_t StartSyncVideoDecoder();
    int32_t StartVideoDecoderReadStream();
    int64_t GetSystemTimeUs();
    int32_t CreateVideoDecoder(std::string codeName);
    int32_t SetVideoDecoderCallback();
    void testAPI();
    int32_t SwitchSurface();
    int32_t RepeatCallSetSurface();
    int32_t Release();
    int32_t SetParameter(OH_AVFormat *format);
    void CheckOutputDescription();
    void AutoSwitchSurface();
    void InputFunc();
    int32_t DecodeSetSurface();
    int32_t PushData(uint32_t index, OH_AVBuffer *buffer);
    int32_t PushDataReadStream(uint32_t index, OH_AVBuffer *buffer);
    int32_t CheckAndReturnBufferSize(OH_AVBuffer *buffer);
    uint32_t SendData(uint32_t bufferSize, uint32_t index, OH_AVBuffer *buffer);
    void ProcessOutputData(OH_AVBuffer *buffer, uint32_t index, int32_t size);
    int32_t CheckAttrFlag(OH_AVCodecBufferAttr attr);
    void SetAttr(OH_AVCodecBufferAttr &attr, uint32_t bufferSize);
    void OutputFunc();
    void SyncOutputFunc();
    int32_t SyncOutputFuncEos(OH_AVCodecBufferAttr attr, uint32_t index);
    void InputFuncTest();
    void InputReadStreamFuncTest();
    void SyncInputFunc();
    void OutputFuncTest();
    void ReleaseSignal();
    void CreateSurface();
    void ReleaseInFile();
    void StopInloop();
    void Flush_buffer();
    void StopOutloop();
    bool IsRender();
    bool MdCompare(unsigned char *buffer, int len, const char *source[]);
    int32_t inputoutputloop();
    int32_t inputoutputsyncloop();
    void getFormat(const char *fileName);
    int64_t GetFileSize(const char *fileName);
    int32_t StartVideoDecoderFor263();
    int32_t PushDataFor263(uint32_t index, OH_AVBuffer *buffer);
    void InputFor263FuncTest();
    bool readChunkHeader(char* chunkId, uint32_t& chunkSize);
    bool findMoviChunk();
    void GetVideoSupportedPixelFormats();
    void GetFormatKey();

    const char *inputDir = "";
    const char *outputDir = "/data/test/media/RawVideoDecTest.yuv";
    const char *outputDir2 = "/data/test/media/RawVideoDecTest2.yuv";
    bool sfOutput = false;
    uint32_t defaultWidth = 720;
    uint32_t defaultHeight = 480;
    uint32_t defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    bool needCheckHash = false;
    bool outputYuvFlag = false;
    uint32_t errCount = 0;
    uint32_t outFrameCount = 0;
    bool isGetVideoSupportedPixelFormats = false;
    bool isGetFormatKey = false;
    int isGetVideoSupportedPixelFormatsNum = 0;
    int isGetFormatKeyNum = 0;
    const char *avcodecMimeType = nullptr;
    bool isEncoder = true;
    const OH_NativeBuffer_Format *pixlFormats = nullptr;
    uint32_t pixlFormatNum = 0;
    int firstCallBackKey = 0;
    OH_AVFormat *trackFormat = nullptr;
    bool afterEosDestroyCodec = true;
    bool sleepOnFPS = false;
    double defaultFrameRate = 30.0;
    bool autoSwitchSurface = false;
    bool isonError = false;
    uint32_t pixFmt = AV_PIX_FMT_NV12;
    bool outputYuvSurface = false;
    int32_t enbleSyncMode = 0;
    bool inputCallbackFlush = false;
    bool inputCallbackStop = false;
    bool outputCallbackFlush = false;
    bool outputCallbackStop = false;
    VDecAPI11Signal *signal = nullptr;
    std::atomic<bool> isRunning = false;
    uint32_t outCount = 0;
private:
    const char *rawvideoInputPixFmt_ = "rawvideo_input_pix_fmt";
    uint32_t originalWidth_ = 0;
    uint32_t originalHeight_ = 0;
    uint32_t repeatCallTime_ = 10;
    uint32_t maxSurfNum_ = 2;
    uint32_t defaultRangeFlag_ = 0;
    bool beforeEosInput_ = false;
    bool beforeEosInputInput_ = false;
    uint32_t frameCount_ = 0;
    // 解码输出数据预期
    bool needCheckOutputDesc_ = false;
    uint32_t expectCropTop_ = 0;
    uint32_t expectCropBottom_ = 0;
    uint32_t expectCropLeft_ = 0;
    uint32_t expectCropRight_ = 0;
    int enbleBlankFrame_ = 0;
    int64_t syncInputWaitTime_ = -1;
    int64_t syncOutputWaitTime_ = -1;
    bool queryOutputBufferEOS_ = false;
    bool queryInputBufferEOS_ = false;
    const char *fileSourcesha256_2[64] = {
        "db", "19", "35", "63", "48", "3d", "ea", "ce", "3d", "48", "a4", "93", "5c", "70", "42", "ce",
        "28", "b5", "a6", "eb", "d3", "c7", "0d", "32", "ad", "f7", "25", "24", "72", "15", "b9", "a5",
        "54", "43", "90", "9d", "a2", "49", "58", "a6", "cb", "a4", "55", "0c", "8c", "7e", "d2", "68",
        "85", "a2", "3a", "75", "50", "80", "1c", "d0", "4e", "df", "cf", "1b", "5c", "70", "74", "a1"
    };
    int64_t renderTimestampNs_ = 0;
    bool rsAtTime_ = false;
    int64_t outTimeArray[2000] = {};
    bool repeatRun_ = false;
    int32_t maxInputSize_ = 0;
    int32_t switchSurfaceFlag_ = 0;
    bool useHDRSource_ = false;
    int fd_ = -1;
    OH_AVMemory *memory_ = nullptr;
    OH_AVSource *videoSource_ = nullptr;
    OH_AVDemuxer *demuxer_ = nullptr;
    OH_AVFormat *sourceFormat_ = nullptr;
    OH_AVBuffer *videoAvBuffer_ = nullptr;
    int32_t trackCount_ = 0;
    int32_t videoTrackId_ = 0;
    std::atomic<bool> isStarted_ = false;
    uint32_t moviChunkPos_ = 0;
    uint32_t moviChunkSize_ = 0;
    std::unique_ptr<std::ifstream> inFile_ = nullptr;
    std::unique_ptr<std::thread> inputLoop_ = nullptr;
    std::unique_ptr<std::thread> outputLoop_ = nullptr;
    std::unordered_map<uint32_t, OH_AVBuffer *> inBufferMap_;
    std::unordered_map<uint32_t, OH_AVBuffer *> outBufferMap_;
    OH_AVCodec *vdec_ = nullptr;
    OH_AVCodecCallback cb_;
    int64_t timeStamp_ = 0;
    int64_t lastRenderedTimeUs_ = 0;
    bool isFirstFrame_ = true;
    OHNativeWindow *nativeWindow[2] = {};
    sptr<Surface> cs[2] = {};
    sptr<Surface> ps[2] = {};
};
} // namespace Media
} // namespace OHOS

void VdecAPI11Error(OH_AVCodec *codec, int32_t errorCode, void *userData);
void VdecAPI11FormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
void VdecAPI11InputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData);
void VdecAPI11OutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData);
int32_t HighRand();
int32_t WidthRand();
int32_t FrameRand();
#endif // VIDEODEC_SAMPLE_H
