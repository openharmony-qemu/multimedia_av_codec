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

#ifndef VIDEODEC_SAMPLE_H
#define VIDEODEC_SAMPLE_H

#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "native_avformat.h"
#include "native_avmemory.h"
#include "nocopyable.h"
#include "securec.h"
#include "surface/window.h"
#include "iconsumer_surface.h"
namespace OHOS {
namespace Media {
class VDecSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<uint32_t> outIdxQueue_;
    std::queue<OH_AVCodecBufferAttr> attrQueue_;
    std::queue<OH_AVMemory *> inBufferQueue_;
    std::queue<OH_AVMemory *> outBufferQueue_;
};

class VDecFuzzSample : public NoCopyable {
public:
    VDecFuzzSample() = default;
    ~VDecFuzzSample();
    int32_t RunVideoDec();
    const char *inpDir = "/data/test/media/1920_1080_10_30Mb.h264";
    const char *outDir = "/data/test/media/VDecTest.yuv";
    uint32_t defaultWidth = 1920;
    uint32_t defaultHeight = 1080;
    uint32_t defaultFrameRate = 30;
    uint32_t defaultRotation = 0;
    uint32_t defaultPixelFormat = AV_PIXEL_FORMAT_NV12;
    uint32_t frameCount_ = 0;
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    void SetEOS(uint32_t index);
    uint32_t SendData(uint32_t bufferSize, uint32_t index, OH_AVMemory *buffer);
    void CopyStartCode(uint8_t *frameBuffer, uint32_t bufferSize, OH_AVCodecBufferAttr &attr);
    int32_t ReadData(uint32_t index, OH_AVMemory *buffer);
    void WaitForEOS();
    int32_t ConfigureVideoDecoder();
    int32_t StartVideoDecoder();
    int64_t GetSystemTimeUs();
    int32_t CreateVideoDecoder();
    int32_t SetVideoDecoderCallback();
    int32_t Release();
    void SetParameter(int32_t data);
    void InputFuncAVCC();
    OH_AVErrCode InputFuncFUZZ(const uint8_t *data, size_t size);
    void ReleaseSignal();
    void ReleaseInFile();
    void StopInLoop();
    VDecSignal *signal_;
    uint32_t errCount = 0;
    bool isSurfMode = false;
    OH_AVCodec *vdec_;
    OHNativeWindow *nativeWindow = nullptr;
    sptr<Surface> cs = nullptr;
    sptr<Surface> ps = nullptr;
private:
    std::atomic<bool> isRunning_ { false };
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unordered_map<uint32_t, OH_AVMemory *> inBufferMap_;
    OH_AVCodecAsyncCallback cb_;
    int64_t timeStamp_ { 0 };
    int64_t lastRenderedTimeUs_ { 0 };
    bool isFirstFrame_ = true;
};
} // namespace Media
} // namespace OHOS

void VdecError(OH_AVCodec *codec, int32_t errorCode, void *userData);
void VdecFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
void VdecInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData);
void VdecOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                         void *userData);
#endif // VIDEODEC_SAMPLE_H
