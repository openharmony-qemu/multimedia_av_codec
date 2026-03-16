/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
    void getFormat(const char *fileName);
    int64_t GetFileSize(const char *fileName);
    int32_t StartVideoDecoderFor263();
    int32_t PushDataFor263(uint32_t index, OH_AVBuffer *buffer);
    void InputFor263FuncTest();
    void GetVideoSupportedPixelFormats();
    void GetFormatKey();

    const char *inputDir = "";
    const char *outputDir = "/data/test/media/CinepakDecTest.yuv";
    const char *outputDir2 = "/data/test/media/CinepakDecTest2.yuv";
    bool sfOutput = false;
    uint32_t defaultWidth = 720;
    uint32_t defaultHeight = 480;
    uint32_t defaultPixelFormat = AV_PIXEL_FORMAT_NV12;
    double defaultFrameRate = 30.0;
    bool outputYuvFlag = false;
    bool outputYuvSurface = false;
    std::atomic<bool> isRunning = false;
    VDecAPI11Signal *signal = nullptr;
    bool isonError = false;
    uint32_t errCount = 0;
    uint32_t outCount = 0;
    uint32_t outFrameCount = 0;
    bool inputCallbackFlush = false;
    bool inputCallbackStop = false;
    bool outputCallbackFlush = false;
    bool outputCallbackStop = false;
    bool afterEosDestroyCodec = true;
    bool needCheckHash = false;
    OH_AVFormat *trackFormat = nullptr;
    bool sleepOnFPS = false;
    bool autoSwitchSurface = false;
    int32_t enbleSyncMode = 0;
    bool isGetVideoSupportedPixelFormats = false;
    bool isGetFormatKey = false;
    const char *avcodecMimeType = nullptr;
    bool isEncoder = true;
    const OH_NativeBuffer_Format *pixelFormats = nullptr;
    uint32_t pixelFormatNum = 0;
private:
    uint32_t originalWidth_ = 0;
    uint32_t originalHeight_ = 0;
    uint32_t repeatCallTime_ = 10;
    uint32_t maxSurfNum_ = 2;
    uint32_t defaultRangeFlag_ = 0;
    bool beforeEosInput_ = false;
    bool beforeEosInputInput_ = false;
    uint32_t frameCount_ = 0;
    uint32_t repeatTime_ = 0;
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
    const char *fileSourcesha256Cinepak_[64] = {
        "59", "6c", "73", "3a", "ce", "ee", "e2", "1c", "a1", "f8", "7f", "42",
        "4c", "e1", "91", "49", "ae", "18", "d7", "29", "76", "08", "30", "8a",
        "b5", "ef", "e0", "c5", "ab", "79", "ec", "e5", "4c", "30", "e8", "10",
        "00", "c3", "cf", "9d", "fe", "2c", "c9", "ff", "ba", "82", "7f", "4c",
        "53", "af", "96", "31", "45", "b5", "08", "cb", "04", "33", "42", "a2",
        "33", "25", "d7", "13"
    };
    const char *fileSourcesha256_2[64] = {
        "8d", "6a", "e0", "34", "42", "9f", "20", "12", "40", "18", "d1", "64",
        "e3", "df", "a9", "2b", "36", "9e", "76", "c3", "a8", "36", "4b", "0c",
        "68", "11", "c7", "a9", "ef", "cf", "07", "6d", "01", "fb", "0f", "b0",
        "29", "d0", "d6", "20", "53", "db", "e7", "43", "b0", "53", "bd", "c4",
        "0a", "da", "6f", "fc", "6a", "d7", "ef", "8d", "de", "7c", "15", "f9",
        "11", "1f", "d9", "44"
    };
    int64_t renderTimestampNs_ = 0;
    bool rsAtTime_ = false;
    int64_t outTimeArray[2000] = {};
    bool repeatRun_ = false;
    int64_t decodeCount_ = 0;
    int64_t startTime_ = 0;
    int32_t maxInputSize_ = 0;
    int64_t endTime_ = 0;
    int32_t switchSurfaceFlag_ = 0;
    bool useHDRSource_ = false;
    bool isH263Change_ = false;
    OH_AVMemory *memory_ = nullptr;
    OH_AVSource *videoSource_ = nullptr;
    OH_AVDemuxer *demuxer_ = nullptr;
    OH_AVFormat *sourceFormat_ = nullptr;
    OH_AVBuffer *videoAvBuffer_ = nullptr;
    int32_t trackCount_ = 0;
    int32_t videoTrackId_ = 0;
    std::atomic<bool> isStarted_ = false;
    int fd_ = -1;
    std::unique_ptr<std::ifstream> inFile_ = nullptr;
    std::unique_ptr<std::thread> inputLoop_ = nullptr;
    std::unique_ptr<std::thread> outputLoop_ = nullptr;
    std::unordered_map<uint32_t, OH_AVBuffer *> inBufferMap_;
    std::unordered_map<uint32_t, OH_AVBuffer *> outBufferMap_;
    OH_AVCodec *vdec_ = nullptr;
    OH_AVCodecCallback cb_;
    int64_t timeStamp_ = 0 ;
    int64_t lastRenderedTimeUs_ = 0;
    bool isFirstFrame_ = true;
    OHNativeWindow *nativeWindow[2] = {};
    sptr<Surface> cs[2] = {};
    sptr<Surface> ps[2] = {};
    int isGetVideoSupportedPixelFormatsNum_ = 0;
    int isGetFormatKeyNum_ = 0;
    int firstCallBackKey_ = 0;
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
