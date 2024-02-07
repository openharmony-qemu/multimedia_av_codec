/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef VDEC_SAMPLE_H
#define VDEC_SAMPLE_H
#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <string>
#include <thread>
#include "iconsumer_surface.h"
#include "native_avbuffer.h"
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "native_avformat.h"
#include "native_avmemory.h"
#include "securec.h"
#include "surface.h"
#include "surface_buffer.h"

namespace OHOS {
namespace MediaAVCodec {
class VideoDecSample;
struct VideoDecSignal {
public:
    explicit VideoDecSignal(std::shared_ptr<VideoDecSample> codec) : codec_(codec){};
    std::mutex eosMutex_;
    std::condition_variable eosCond_;

    std::mutex inMutex_;
    std::condition_variable inCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<OH_AVMemory *> inMemoryQueue_;
    std::queue<OH_AVBuffer *> inBufferQueue_;

    std::mutex outMutex_;
    std::condition_variable outCond_;
    std::queue<uint32_t> outQueue_;
    std::queue<OH_AVCodecBufferAttr> outAttrQueue_;
    std::queue<OH_AVMemory *> outMemoryQueue_;
    std::queue<OH_AVBuffer *> outBufferQueue_;
    std::vector<int32_t> errors_;
    std::atomic<int32_t> controlNum_ = 0;
    std::atomic<bool> isRunning_ = false;
    std::atomic<bool> isFlushing_ = false;
    std::atomic<bool> isEos_ = false;
    std::weak_ptr<VideoDecSample> codec_;
};

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(Surface *cs, std::unique_ptr<std::ofstream> &&outFile, int32_t id);
    ~TestConsumerListener();
    void OnBufferAvailable() override;

private:
    int64_t timestamp_ = 0;
    Rect damage_ = {};
    Surface *cs_ = nullptr;
    std::unique_ptr<std::ofstream> outFile_ = nullptr;
    int32_t sampleId_ = 0;
    uint32_t frameOutputCount_ = 0;
};

class VideoDecSample : public NoCopyable {
public:
    VideoDecSample();
    ~VideoDecSample();
    bool Create();

    int32_t SetCallback(OH_AVCodecAsyncCallback callback, std::shared_ptr<VideoDecSignal> &signal);
    int32_t RegisterCallback(OH_AVCodecCallback callback, std::shared_ptr<VideoDecSignal> &signal);
    int32_t SetOutputSurface();
    int32_t Configure();
    int32_t Start();
    int32_t Prepare();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    OH_AVFormat *GetOutputDescription();
    int32_t SetParameter();
    int32_t PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr);
    int32_t PushInputData(uint32_t index);
    int32_t RenderOutputData(uint32_t index);
    int32_t FreeOutputData(uint32_t index);
    int32_t IsValid(bool &isValid);

    int32_t HandleInputFrame(uint32_t &index, OH_AVCodecBufferAttr &attr);
    int32_t HandleOutputFrame(uint32_t &index, OH_AVCodecBufferAttr &attr);
    int32_t HandleInputFrame(OH_AVMemory *data, OH_AVCodecBufferAttr &attr);
    int32_t HandleOutputFrame(OH_AVMemory *data, OH_AVCodecBufferAttr &attr);
    int32_t HandleInputFrame(OH_AVBuffer *data);
    int32_t HandleOutputFrame(OH_AVBuffer *data);
    bool WaitForEos();

    bool needDump_ = true;
    bool isHardware_ = true;
    uint32_t frameCount_ = 10;
    std::string mime_ = "";
    std::string inPath_ = "720_1280_25_avcc.h264";
    std::string outPath_ = "";
    OH_AVFormat *dyFormat_ = nullptr;
    std::unique_ptr<std::thread> inputLoop_ = nullptr;
    std::unique_ptr<std::thread> outputLoop_ = nullptr;

    int32_t sampleId_ = 0;

private:
    void FlushInQueue();
    void FlushOutQueue();
    int32_t SetAVBufferAttr(OH_AVBuffer *avBuffer, OH_AVCodecBufferAttr &attr);
    int32_t HandleInputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr);
    int32_t HandleOutputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr);
    bool IsCodecData(const uint8_t *const bufferAddr);

    OH_AVCodec *codec_ = nullptr;
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::ofstream> outFile_;
    std::shared_ptr<VideoDecSignal> signal_ = nullptr;
    std::atomic<uint32_t> frameInputCount_ = 0;
    std::atomic<uint32_t> frameOutputCount_ = 0;
    int64_t time_ = 0;
    bool isAVBufferMode_ = false;
    bool isSurfaceMode_ = false;
    bool isH264Stream_ = true; // true: H264; false: H265

    OHNativeWindow *nativeWindow_ = nullptr;
    sptr<Surface> consumer_ = nullptr;
    sptr<Surface> producer_ = nullptr;
};

class HeapMemoryThread {
public:
    HeapMemoryThread();
    ~HeapMemoryThread();

private:
    void HeapMemoryLoop();
    std::unique_ptr<std::thread> heapMemoryLoop_ = nullptr;
    std::atomic<bool> isStopLoop_ = false;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VDEC_SAMPLE_H