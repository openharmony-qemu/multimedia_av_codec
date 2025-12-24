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
#include <fcntl.h>
#include "gtest/gtest.h"

namespace OHOS {
namespace Media {
class VDecAPI11Signal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<uint32_t> outIdxQueue_;
    std::queue<OH_AVCodecBufferAttr> attrQueue_;
    std::queue<OH_AVBuffer *> inBufferQueue_;
    std::queue<OH_AVBuffer *> outBufferQueue_;
};

class VDecAPI11Sample : public NoCopyable {
public:
    VDecAPI11Sample() = default;
    ~VDecAPI11Sample();
    int32_t RunVideoDec_Surface(std::string codeName = "");
    int32_t RunVideoDec(std::string codeName = "");
    const char *INP_DIR = "";
    const char *OUT_DIR = "/data/test/media/Rv30DecTest.yuv";
    const char *OUT_DIR2 = "/data/test/media/Rv30DecTest2.yuv";
    bool SF_OUTPUT = false;
    uint32_t DEFAULT_WIDTH = 160;
    uint32_t DEFAULT_HEIGHT = 120;
    uint32_t originalWidth = 0;
    uint32_t originalHeight = 0;
    bool SURFACE_OUTPUT = false;
    uint32_t defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    uint32_t REPEAT_CALL_TIME = 10;
    uint32_t MAX_SURF_NUM = 2;
    double DEFAULT_FRAME_RATE = 15.0;
    uint32_t DEFAULT_RANGE_FLAG = 0;
    bool BEFORE_EOS_INPUT = false;
    bool BEFORE_EOS_INPUT_INPUT = false;
    bool AFTER_EOS_DESTORY_CODEC = true;
    uint32_t frameCount_ = 0;
    bool outputYuvFlag = false;
    uint32_t repeat_time = 0;
    uint32_t outFrameCount = 0;
    // 解码输出数据预期
    bool needCheckOutputDesc = false;
    uint32_t expectCropTop = 0;
    uint32_t expectCropBottom = 0;
    uint32_t expectCropLeft = 0;
    uint32_t expectCropRight = 0;
    bool outputYuvSurface = false;
    int32_t enbleSyncMode = 0;
    int enbleBlankFrame = 0;
    int64_t syncInputWaitTime = -1;
    int64_t syncOutputWaitTime = -1;
    bool queryOutputBufferEOS = false;
    bool queryInputBufferEOS = false;
    const char *fileSourcesha256_rv30[64] = {"30", "d7", "bc", "a0", "27", "97", "ed",
        "ce", "67", "8d", "7d", "47", "8e", "0a", "ac", "01", "ba", "36", "9b", "53", "aa",
        "89", "d0", "f3", "b7", "fd", "90", "bf", "0b", "02", "a2", "2b", "20", "14", "3b",
        "4e", "d5", "4d", "35", "ea", "b8", "a3", "12", "70", "e9", "8d", "eb", "87", "5c",
        "91", "c7", "da", "31", "16", "e9", "2f", "45", "4b", "63", "5b", "30", "a9", "86",
        "eb"
    };
    const char *fileSourcesha256_2[64] = {"e8", "97", "37", "f6", "9d", "e3", "0b",
        "a4", "eb", "74", "e6", "bd", "6d", "cd", "86", "05", "72", "f0", "bf", "e0", "8a",
        "8e", "97", "07", "e4", "73", "3c", "5a", "6d", "d4", "e3", "3a", "68", "44", "cf",
        "ff", "93", "f5", "60", "c5", "d0", "ce", "63", "a1", "ff", "ae", "48", "84", "4a",
        "88", "14", "88", "a6", "6a", "5b", "9d", "bc", "38", "7c", "52", "0e", "4d", "bb",
        "81"
    };

    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t state_EOS();
    void SetEOS(uint32_t index, OH_AVBuffer *buffer);
    int32_t ConfigureVideoDecoderNoPixelFormat();
    void WaitForEOS();
    int32_t ConfigureVideoDecoder();
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
    VDecAPI11Signal *signal_;
    uint32_t errCount = 0;
    uint32_t outCount = 0;
    int64_t renderTimestampNs = 0;
    bool rsAtTime = false;
    int64_t outTimeArray[2000] = {};
    bool sleepOnFPS = false;
    bool repeatRun = false;
    int64_t decode_count = 0;
    int64_t start_time = 0;
    int32_t maxInputSize = 0;
    int64_t end_time = 0;
    bool autoSwitchSurface = false;
    int32_t switchSurfaceFlag = 0;
    std::atomic<bool> isRunning_ { false };
    bool inputCallbackFlush = false;
    bool inputCallbackStop = false;
    bool outputCallbackFlush = false;
    bool outputCallbackStop = false;
    bool useHDRSource = false;
    OH_AVFormat *trackFormat = nullptr;
    bool isH263Change = false;
    bool isonError = false;
    void getFormat(const char *fileName);
    int64_t GetFileSize(const char *fileName);
    int g_fd = -1;
    OH_AVMemory *memory = nullptr;
    OH_AVSource *videoSource = nullptr;
    OH_AVDemuxer *demuxer = nullptr;
    OH_AVFormat *sourceFormat = nullptr;
    OH_AVBuffer *videoAvBuffer = nullptr;
    int32_t g_trackCount;
    int32_t g_videoTrackId;
    std::atomic<bool> isStarted_{false};
    int32_t StartVideoDecoderFor263();
    int32_t PushDataFor263(uint32_t index, OH_AVBuffer *buffer);
    void InputFor263FuncTest();
    bool readChunkHeader(char* chunkId, uint32_t& chunkSize);
    bool findMoviChunk();
    uint32_t moviChunkPos;
    uint32_t moviChunkSize;
    bool NocaleHash = false;
private:
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::unordered_map<uint32_t, OH_AVBuffer *> inBufferMap_;
    std::unordered_map<uint32_t, OH_AVBuffer *> outBufferMap_;
    OH_AVCodec *vdec_;
    OH_AVCodecCallback cb_;
    int64_t timeStamp_ { 0 };
    int64_t lastRenderedTimeUs_ { 0 };
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
