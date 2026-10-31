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
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include "securec.h"
#include "native_avcodec_videodecoder.h"
#include "nocopyable.h"
#include "native_avmemory.h"
#include "native_avformat.h"
#include "native_averrors.h"
#include "native_avcapability.h"
#include "surface/window.h"
#include "iconsumer_surface.h"

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

/**
 * WMV1 为 MS-MPEG4 V1 码流，帧与帧之间没有起始码，也不依赖 codec config（extradata），
 * 因此这里只记录每帧的字节数，用于把裸流切分成一帧一帧送给解码器。
 */
typedef struct WMV1_StreamData {
    std::vector<uint32_t> frameBtyes;
    uint32_t frameCount = 0;
} WMV1_StreamData;

class VDecAPI11Sample : public NoCopyable {
public:
    VDecAPI11Sample() = default;
    ~VDecAPI11Sample();
    int32_t RunVideoDec_Surface(std::string codeName = "");
    int32_t RunVideoDec(std::string codeName = "");
    const char *INP_DIR = "/data/test/media/wmv1_720x480_25.wmv1";
    const char *OUT_DIR = "/data/test/media/VDecTest.yuv";
    const char *OUT_DIR2 = "/data/test/media/VDecTest2.yuv";
    bool SF_OUTPUT = false;
    uint32_t DEFAULT_WIDTH = 720;
    uint32_t DEFAULT_HEIGHT = 480;
    uint32_t originalWidth = 0;
    uint32_t originalHeight = 0;
    bool SURFACE_OUTPUT = false;
    uint32_t defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    uint32_t REPEAT_CALL_TIME = 10;
    uint32_t MAX_SURF_NUM = 2;
    double DEFAULT_FRAME_RATE = 25.0;
    uint32_t DEFAULT_RANGE_FLAG = 0;
    bool BEFORE_EOS_INPUT = false;              // 0800 测试用例
    bool BEFORE_EOS_INPUT_INPUT = false;        // 0900 测试用例
    bool AFTER_EOS_DESTORY_CODEC = true;        // 1000 测试用例 结束不销毁codec
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
    // 解码内容校验开关，只在需要校验哈希的用例里置 true
    bool needMdCompare = false;
    // 解码哈希基线（SHA512，64 字节按 16 进制逐字节存放），按“码流 + 像素格式”各一份。
    // 数组留空时只打印实际哈希、不做比较；采集方式：打开 needMdCompare 跑一次，日志会打印
    // computed hash，把 64 个字节按顺序粘到对应数组里即可，无需改代码。
    const char *fileSourcesha256Nv12_720x480[64] = {"2b", "b8", "70", "6c", "aa", "e6", "0d", "7b",
                                                    "92", "7d", "6f", "51", "fc", "f4", "60", "79",
                                                    "6e", "16", "7b", "0b", "02", "7a", "56", "32",
                                                    "63", "56", "c4", "7c", "b0", "39", "6e", "4c",
                                                    "f6", "d0", "d4", "f6", "c1", "32", "0c", "c1",
                                                    "e2", "6a", "a1", "25", "d7", "b1", "a4", "7d",
                                                    "cb", "5e", "75", "d5", "67", "ed", "c9", "48",
                                                    "16", "0b", "ac", "72", "09", "f8", "bd", "c2"};
    const char *fileSourcesha256Nv12_176x144[64] = {"08", "e7", "77", "02", "a9", "d4", "a7", "4d",
                                                    "be", "ff", "c1", "d8", "0d", "0f", "2f", "f6",
                                                    "9d", "cf", "bb", "66", "4f", "b4", "63", "c1",
                                                    "41", "ab", "49", "d8", "58", "96", "37", "16",
                                                    "0d", "28", "5a", "52", "db", "98", "62", "c2",
                                                    "82", "1f", "86", "2e", "8a", "49", "21", "21",
                                                    "7e", "eb", "a8", "ef", "75", "cc", "03", "4d",
                                                    "20", "ae", "4b", "a5", "60", "70", "e8", "3d"};
    const char *fileSourcesha256Nv21_720x480[64] = {"01", "b1", "5c", "3d", "0e", "f7", "23", "52",
                                                    "37", "48", "32", "7b", "b0", "00", "9c", "63",
                                                    "fa", "2b", "2d", "48", "2d", "b4", "48", "23",
                                                    "56", "dc", "1c", "c5", "b3", "3a", "a3", "2f",
                                                    "da", "53", "8b", "e9", "44", "63", "1a", "b3",
                                                    "2e", "ec", "dc", "77", "aa", "79", "b6", "1e",
                                                    "e7", "f1", "e0", "48", "e6", "ab", "5c", "6b",
                                                    "a2", "f4", "d2", "b3", "1a", "43", "51", "1c"};
    const char *fileSourcesha256Yuvi420_640x480[64] = {"f4", "7d", "3b", "34", "5b", "2c", "84", "a1",
                                                       "43", "5e", "65", "6f", "90", "c8", "90", "ed",
                                                       "e7", "7d", "1f", "8a", "9c", "e1", "3b", "2d",
                                                       "65", "a2", "8a", "73", "f9", "8c", "78", "f0",
                                                       "d4", "a5", "5e", "9d", "cf", "22", "56", "ed",
                                                       "80", "2e", "01", "11", "c7", "fe", "f9", "49",
                                                       "7d", "b4", "53", "99", "e9", "81", "43", "e6",
                                                       "f2", "73", "45", "90", "93", "20", "c5", "9f"};

    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t state_EOS();
    void SetEOS(uint32_t index, OH_AVBuffer *buffer);
    void WaitForEOS();
    int32_t ConfigureVideoDecoder();
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
    int32_t DecodeSetSurface();
    int32_t PushData(uint32_t index, OH_AVBuffer *buffer);
    int32_t CheckAndReturnBufferSize(OH_AVBuffer *buffer);
    uint32_t SendData(uint32_t bufferSize, uint32_t index, OH_AVBuffer *buffer);
    void ProcessOutputData(OH_AVBuffer *buffer, uint32_t index, int32_t size);
    int32_t CheckAttrFlag(OH_AVCodecBufferAttr attr);
    void SetAttr(OH_AVCodecBufferAttr &attr, uint32_t bufferSize);
    void EnforceAutoSwitchSurface();
    void SyncOutputFunc();
    int32_t SyncOutputFuncEos(OH_AVCodecBufferAttr attr, uint32_t index);
    void InputFuncTest();
    void SyncInputFunc();
    void OutputFuncTest();
    void CreateSurface();
    void ReleaseInFile();
    void StopInloop();
    void Flush_buffer();
    void StopOutloop();
    bool MdCompare(unsigned char *buffer, int len, const char *source[]);
    bool IsBaselineEmpty(const char *source[], int len);
    void DumpHashValue(const char *pixName);
    void CheckDecodeHash();
    void FlushStatus();
    void GetVideoSupportedPixelFormats();
    void GetFormatKey();
    bool isGetVideoSupportedPixelFormats = false;
    bool isGetFormatKey = false;
    int isGetVideoSupportedPixelFormatsNum = 0;
    int isGetFormatKeyNum = 0;
    const char *avcodecMimeType = nullptr;
    bool isEncoder = true;
    const OH_NativeBuffer_Format *pixlFormats = nullptr;
    uint32_t pixlFormatNum = 0;
    int firstCallBackKey = 0;
    VDecAPI11Signal *signal_;
    WMV1_StreamData streamData_;
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
    bool isWmv1Change = false;
    bool isonError = false;
private:
    std::unique_ptr<std::vector<uint8_t>> wmv1Unit_;
    std::unique_ptr<std::ifstream> inFile_;
    uint32_t frameIndex_ = 0;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::unordered_map<uint32_t, OH_AVBuffer *> inBufferMap_;
    std::unordered_map<uint32_t, OH_AVBuffer *> outBufferMap_;
    OH_AVCodec *vdec_;
    OH_AVCodecCallback cb_;
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
#endif // VIDEODEC_SAMPLE_H
