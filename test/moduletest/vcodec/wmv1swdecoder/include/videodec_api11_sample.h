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
#include <cstdint>
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
    // 解码哈希打印开关，只在需要核对哈希的用例里置 true；不匹配只打印、不判定失败
    bool needMdCompare = false;
    static constexpr int32_t HASH_BYTES = 64;
    // 解码哈希基线（SHA512，64 字节），按码流 + 像素格式各一份；全 0 表示尚未采集。
    // 采集方式：打开 needMdCompare 跑一次，日志会打印 computed hash，把 64 个字节按顺序
    // 填到对应数组里即可。基线跟设备和解码器版本绑定，所以只用于人工核对，不作为门禁。
    const uint8_t fileSourcesha256Nv12_720x480[HASH_BYTES] = {
        0x2b, 0xb8, 0x70, 0x6c, 0xaa, 0xe6, 0x0d, 0x7b,
        0x92, 0x7d, 0x6f, 0x51, 0xfc, 0xf4, 0x60, 0x79,
        0x6e, 0x16, 0x7b, 0x0b, 0x02, 0x7a, 0x56, 0x32,
        0x63, 0x56, 0xc4, 0x7c, 0xb0, 0x39, 0x6e, 0x4c,
        0xf6, 0xd0, 0xd4, 0xf6, 0xc1, 0x32, 0x0c, 0xc1,
        0xe2, 0x6a, 0xa1, 0x25, 0xd7, 0xb1, 0xa4, 0x7d,
        0xcb, 0x5e, 0x75, 0xd5, 0x67, 0xed, 0xc9, 0x48,
        0x16, 0x0b, 0xac, 0x72, 0x09, 0xf8, 0xbd, 0xc2};
    const uint8_t fileSourcesha256Nv12_176x144[HASH_BYTES] = {
        0x08, 0xe7, 0x77, 0x02, 0xa9, 0xd4, 0xa7, 0x4d,
        0xbe, 0xff, 0xc1, 0xd8, 0x0d, 0x0f, 0x2f, 0xf6,
        0x9d, 0xcf, 0xbb, 0x66, 0x4f, 0xb4, 0x63, 0xc1,
        0x41, 0xab, 0x49, 0xd8, 0x58, 0x96, 0x37, 0x16,
        0x0d, 0x28, 0x5a, 0x52, 0xdb, 0x98, 0x62, 0xc2,
        0x82, 0x1f, 0x86, 0x2e, 0x8a, 0x49, 0x21, 0x21,
        0x7e, 0xeb, 0xa8, 0xef, 0x75, 0xcc, 0x03, 0x4d,
        0x20, 0xae, 0x4b, 0xa5, 0x60, 0x70, 0xe8, 0x3d};
    const uint8_t fileSourcesha256Nv21_720x480[HASH_BYTES] = {
        0x01, 0xb1, 0x5c, 0x3d, 0x0e, 0xf7, 0x23, 0x52,
        0x37, 0x48, 0x32, 0x7b, 0xb0, 0x00, 0x9c, 0x63,
        0xfa, 0x2b, 0x2d, 0x48, 0x2d, 0xb4, 0x48, 0x23,
        0x56, 0xdc, 0x1c, 0xc5, 0xb3, 0x3a, 0xa3, 0x2f,
        0xda, 0x53, 0x8b, 0xe9, 0x44, 0x63, 0x1a, 0xb3,
        0x2e, 0xec, 0xdc, 0x77, 0xaa, 0x79, 0xb6, 0x1e,
        0xe7, 0xf1, 0xe0, 0x48, 0xe6, 0xab, 0x5c, 0x6b,
        0xa2, 0xf4, 0xd2, 0xb3, 0x1a, 0x43, 0x51, 0x1c};
    const uint8_t fileSourcesha256Yuvi420_640x480[HASH_BYTES] = {
        0xf4, 0x7d, 0x3b, 0x34, 0x5b, 0x2c, 0x84, 0xa1,
        0x43, 0x5e, 0x65, 0x6f, 0x90, 0xc8, 0x90, 0xed,
        0xe7, 0x7d, 0x1f, 0x8a, 0x9c, 0xe1, 0x3b, 0x2d,
        0x65, 0xa2, 0x8a, 0x73, 0xf9, 0x8c, 0x78, 0xf0,
        0xd4, 0xa5, 0x5e, 0x9d, 0xcf, 0x22, 0x56, 0xed,
        0x80, 0x2e, 0x01, 0x11, 0xc7, 0xfe, 0xf9, 0x49,
        0x7d, 0xb4, 0x53, 0x99, 0xe9, 0x81, 0x43, 0xe6,
        0xf2, 0x73, 0x45, 0x90, 0x93, 0x20, 0xc5, 0x9f};

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
    bool MdCompare(unsigned char *buffer, int len, const uint8_t source[]);
    bool IsBaselineEmpty(const uint8_t source[], int len);
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
