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
    int32_t RunVideoDec_SurfaceForAv1(std::string codeName = "");
    int32_t RunVideoDec(std::string codeName = "");
    int32_t RunVideoDecForAv1(std::string codeName = "");
    const char *INP_DIR = "";
    const char *OUT_DIR = "/data/test/media/VDecTest.yuv";
    const char *OUT_DIR2 = "/data/test/media/VDecTest2.yuv";
    bool SF_OUTPUT = false;
    uint32_t DEFAULT_WIDTH = 720;
    uint32_t DEFAULT_HEIGHT = 480;
    uint32_t originalWidth = 0;
    uint32_t originalHeight = 0;
    bool SURFACE_OUTPUT = false;
    uint32_t defaultPixelFormat = AV_PIXEL_FORMAT_NV12;
    uint32_t REPEAT_CALL_TIME = 10;
    uint32_t MAX_SURF_NUM = 2;
    double DEFAULT_FRAME_RATE = 30.0;
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
    const char *fileSourcesha256_nv12[64] = {
        "89", "fc", "6c", "42", "1a", "e7", "2f", "75", "c6", "ce", "b0", "3e", "79", "83", "65", "26",
        "dc", "6f", "ab", "9e", "24", "50", "81", "37", "32", "0a", "c9", "7a", "c9", "4f", "f5", "99",
        "88", "34", "12", "db", "c6", "80", "e8", "26", "63", "9d", "a6", "0d", "5f", "83", "4f", "e1",
        "20", "b9", "f1", "81", "05", "11", "fa", "fb", "0b", "69", "4d", "6d", "bb", "96", "9d", "78"
    };
    const char *fileSourcesha256_nv21[64] = {
        "82", "52", "c7", "ec", "21", "f2", "ad", "95", "41", "56", "b1", "6e", "9d", "15", "ea", "ae",
        "b3", "58", "39", "f6", "52", "a2", "ae", "ca", "e8", "5d", "df", "2d", "eb", "c2", "b3", "3a",
        "25", "be", "3b", "7a", "fe", "22", "8d", "dd", "8c", "dc", "8e", "20", "16", "ea", "5e", "e1",
        "27", "77", "4f", "21", "48", "ad", "8b", "d6", "28", "60", "9f", "e4", "ed", "10", "25", "1f"
    };
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
    int32_t StartSyncVideoDecoderForAv1();
    int32_t InputOutputSyncLoopForAv1();
    int32_t StartVideoDecoderFor263();
    int32_t StartVideoDecoderForAV1();
    int32_t InputOutputLoopForAv1();
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
    int32_t PushDataFor263(uint32_t index, OH_AVBuffer *buffer);
    int32_t PushDataForAV1(uint32_t index, OH_AVBuffer *buffer);
    int32_t CheckAndReturnBufferSize(OH_AVBuffer *buffer);
    uint32_t SendData(uint32_t bufferSize, uint32_t index, OH_AVBuffer *buffer);
    void ProcessOutputData(OH_AVBuffer *buffer, uint32_t index, int32_t size);
    int32_t CheckAttrFlag(OH_AVCodecBufferAttr attr);
    void SetAttr(OH_AVCodecBufferAttr &attr, uint32_t bufferSize);
    void OutputFunc();
    void SyncOutputFunc();
    int32_t SyncOutputFuncEos(OH_AVCodecBufferAttr attr, uint32_t index);
    void InputFuncTest();
    void InputFor263FuncTest();
    void InputForAV1FuncTest();
    void SyncInputFunc();
    void SyncInputFuncForAv1();
    void OutputFuncTest();
    void ReleaseSignal();
    void CreateSurface();
    void ReleaseInFile();
    void StopInloop();
    void Flush_buffer();
    void StopOutloop();
    bool IsRender();
    bool MdCompare(unsigned char *buffer, int len, const char *source[]);
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
    int32_t DEFAULT_PROFILE = VC1_PROFILE_SIMPLE;
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
    std::atomic<bool> isStarted_{false};
    bool needCheckHash = false;
private:
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::unordered_map<uint32_t, OH_AVBuffer *> inBufferMap_;
    std::unordered_map<uint32_t, OH_AVBuffer *> outBufferMap_;
    OH_AVCodec *vdec_;
    OH_AVCodecCallback cb_;
    int64_t timeStamp_ { 0};
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
