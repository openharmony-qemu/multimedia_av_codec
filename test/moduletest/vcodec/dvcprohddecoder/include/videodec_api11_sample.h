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
#include "videodec_api11_sample.h"
#include "native_avdemuxer.h"
#include "native_avsource.h"
#include "native_avcapability.h"
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
    int32_t StartDecoder();
    int32_t StartSyncDecoder();
    int32_t ConfigureVideoDecoder();
    int32_t ConfigureVideoDecoderNoPixelFormat();
    int32_t StartVideoDecoder();
    int32_t StartSyncVideoDecoder();
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
    int32_t CheckAndReturnBufferSize(OH_AVBuffer *buffer);
    uint32_t SendData(uint32_t bufferSize, uint32_t index, OH_AVBuffer *buffer);
    void ProcessOutputData(OH_AVBuffer *buffer, uint32_t index, int32_t size);
    int32_t CheckAttrFlag(OH_AVCodecBufferAttr attr);
    void SetAttr(OH_AVCodecBufferAttr &attr, uint32_t bufferSize);
    void OutputFunc();
    void SyncOutputFunc();
    int32_t SyncOutputFuncEos(OH_AVCodecBufferAttr attr, uint32_t index);
    void InputFuncTest();
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
    int32_t inputoutputsyncloop();
    void getFormat(const char *fileName);
    int64_t GetFileSize(const char *fileName);
    int32_t StartVideoDecoderFor263();
    int32_t PushDataFor263(uint32_t index, OH_AVBuffer *buffer);
    void InputFor263FuncTest();
    void GetVideoSupportedPixelFormats();
    void GetFormatKey();

    const char *inputDir = "";
    const char *outputDir = "/data/test/media/DvVideoDecTest.yuv";
    const char *outputDir2 = "/data/test/media/DvVideoDecTest2.yuv";
    bool sfOutput = false;
    uint32_t defaultWidth = 720;
    uint32_t defaultHeight = 480;
    uint32_t defaultPixelFormat = AV_PIXEL_FORMAT_NV12;
    double defaultFrameRate = 30.0;
    bool afterEosDestroyCodec = true;
    bool outputYuvFlag = false;
    uint32_t outFrameCount = 0;
    bool outputYuvSurface = false;
    int32_t enableSyncMode = 0;
    enum class StreamType {
        UNKNOWN = 0,
        DVCPRO_HD,
        DVCPAL,
        DVNTSC
    };
    StreamType streamType = StreamType::UNKNOWN;
    VDecAPI11Signal *signal = nullptr;
    bool useFixedDvcpalSize = true;
    uint32_t errCount = 0;
    uint32_t outCount = 0;
    bool sleepOnFPS = false;
    bool autoSwitchSurface = false;
    std::atomic<bool> isRunning = false;
    bool inputCallbackFlush = false;
    bool inputCallbackStop = false;
    bool outputCallbackFlush = false;
    bool outputCallbackStop = false;
    OH_AVFormat *trackFormat = nullptr;
    bool isOnError = false;
    bool nocaleHash = false;
    bool isGetVideoSupportedPixelFormats = false;
    bool isGetFormatKey = false;
    int isGetVideoSupportedPixelFormatsNum = 0;
    int isGetFormatKeyNum = 0;
    const char *avcodecMimeType = nullptr;
    bool isEncoder = true;
    const OH_NativeBuffer_Format *pixlFormats = nullptr;
    uint32_t pixlFormatNum = 0;
    int firstCallBackKey = 0;
private:
    std::atomic<bool> isStarted_ = false;
    int32_t trackCount_ = 0;
    uint32_t videoTrackId_ = 0;
    OH_AVBuffer *videoAvBuffer_ = nullptr;
    OH_AVFormat *sourceFormat_ = nullptr;
    OH_AVDemuxer *demuxer_ = nullptr;
    OH_AVSource *videoSource_ = nullptr;
    OH_AVMemory *memory_ = nullptr;
    int fileDescriptor_ = -1;
    bool useHDRSource_ = false;
    int32_t switchSurfaceFlag_ = 0;
    bool repeatRun_ = false;
    int64_t outTimeArray[2000] = {};
    bool rsAtTime_ = false;
    int64_t renderTimestampNs_ = 0;
    const char *fileSourcesha256_dvcprohd[64] = {
        "1b", "0c", "7f", "79", "6b", "ff", "d5", "10", "cc", "84", "5b", "da", "f0", "e4", "d5", "70",
        "5d", "d4", "57", "b3", "9e", "30", "c0", "cb", "f9", "31", "6c", "63", "39", "02", "1a", "ae",
        "cf", "4a", "ae", "e9", "ff", "e8", "2e", "c7", "10", "4e", "09", "7e", "e2", "dd", "ec", "c3",
        "a9", "48", "e0", "34", "45", "2d", "3a", "bc", "41", "ca", "fd", "5b", "c0", "d0", "bc", "90"
    };
    const char *fileSourcesha256_dvcpal[64] = {
        "cf", "83", "e1", "35", "7e", "ef", "b8", "bd", "f1", "54", "28", "50", "d6", "6d", "80", "07",
        "d6", "20", "e4", "05", "0b", "57", "15", "dc", "83", "f4", "a9", "21", "d3", "6c", "e9", "ce",
        "47", "d0", "d1", "3c", "5d", "85", "f2", "b0", "ff", "83", "18", "d2", "87", "7e", "ec", "2f",
        "63", "b9", "31", "bd", "47", "41", "7a", "81", "a5", "38", "32", "7a", "f9", "27", "da", "3e"
    };
    const char *fileSourcesha256_dvcntsc[64] = {
        "cf", "83", "e1", "35", "7e", "ef", "b8", "bd", "f1", "54", "28", "50", "d6", "6d", "80", "07",
        "d6", "20", "e4", "05", "0b", "57", "15", "dc", "83", "f4", "a9", "21", "d3", "6c", "e9", "ce",
        "47", "d0", "d1", "3c", "5d", "85", "f2", "b0", "ff", "83", "18", "d2", "87", "7e", "ec", "2f",
        "63", "b9", "31", "bd", "47", "41", "7a", "81", "a5", "38", "32", "7a", "f9", "27", "da", "3e"
    };
    int64_t syncInputWaitTime_ = -1;
    int64_t syncOutputWaitTime_ = -1;
    bool queryOutputBufferEOS_ = false;
    bool queryInputBufferEOS_ = false;
    const char **currSha256Tbl_[3] = {
        fileSourcesha256_dvcprohd,
        fileSourcesha256_dvcpal,
        fileSourcesha256_dvcntsc
    };
    int enableBlankFrame_ = 0;
    uint32_t expectCropTop_ = 0;
    uint32_t expectCropBottom_ = 0;
    uint32_t expectCropLeft_ = 0;
    uint32_t expectCropRight_ = 0;
    bool needCheckOutputDesc_ = false;
    uint32_t frameCount_ = 0;
    bool beforeEosInput_ = false;
    bool beforeEosInputInput_ = false;
    uint32_t maxSurfNum_ = 2;
    uint32_t repeatCallTime_ = 10;
    int32_t maxInputSize_ = 0;
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