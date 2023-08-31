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
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include "securec.h"
#include "vcodec_mock.h"

namespace OHOS {
namespace MediaAVCodec {
struct VDecSignal {
public:
    std::mutex mutex_;
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable cond_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIndexQueue_;
    std::queue<uint32_t> outIndexQueue_;
    std::queue<OH_AVCodecBufferAttr> outAttrQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> inBufferQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> outBufferQueue_;
    int32_t errorNum_ = 0;
    std::atomic<bool> isRunning_ = false;
};

class VDecCallbackTest : public AVCodecCallbackMock {
public:
    explicit VDecCallbackTest(std::shared_ptr<VDecSignal> signal);
    virtual ~VDecCallbackTest();
    void OnError(int32_t errorCode) override;
    void OnStreamChanged(std::shared_ptr<FormatMock> format) override;
    void OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data) override;
    void OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr) override;

private:
    std::shared_ptr<VDecSignal> signal_ = nullptr;
};

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(sptr<Surface> cs, std::string_view name);
    ~TestConsumerListener();
    void OnBufferAvailable() override;

private:
    int64_t timestamp_ = 0;
    Rect damage_ = {};
    sptr<Surface> cs_ = nullptr;
    std::unique_ptr<std::ofstream> outFile_;
};

class VideoDecSample : public NoCopyable {
public:
    explicit VideoDecSample(std::shared_ptr<VDecSignal> signal);
    virtual ~VideoDecSample();
    bool CreateVideoDecMockByMime(const std::string &mime);
    bool CreateVideoDecMockByName(const std::string &name);

    int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb);
    int32_t SetOutputSurface();
    int32_t Configure(std::shared_ptr<FormatMock> format);
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    std::shared_ptr<FormatMock> GetOutputDescription();
    int32_t SetParameter(std::shared_ptr<FormatMock> format);
    int32_t PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr);
    int32_t RenderOutputData(uint32_t index);
    int32_t FreeOutputData(uint32_t index);
    bool IsValid();

    void SetOutPath(const std::string &path);
    void SetSource(const std::string &path);

private:
    void FlushInner();
    void RunInner();
    void PrepareInner();
    void OutputLoopFunc();
    void InputLoopFunc();
    int32_t OutputLoopInner();
    int32_t InputLoopInner();
    std::shared_ptr<VideoDecMock> videoDec_ = nullptr;
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::ofstream> outFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::shared_ptr<VDecSignal> signal_ = nullptr;
    std::string inPath_;
    std::string outPath_;
    std::string outSurfacePath_;
    uint32_t datSize_ = 0;
    uint32_t frameInputCount_ = 0;
    uint32_t frameOutputCount_ = 0;
    bool isFirstFrame_ = true;
    bool isSurfaceMode_ = false;
    bool isDump_ = true;
    int64_t time_ = 0;
    sptr<Surface> consumer_ = nullptr;
    sptr<Surface> producer_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VDEC_SAMPLE_H