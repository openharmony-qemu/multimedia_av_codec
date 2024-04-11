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

#ifndef VENC_SAMPLE_H
#define VENC_SAMPLE_H
#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include "securec.h"
#include "surface/window.h"
#include "vcodec_mock.h"

namespace OHOS {
namespace MediaAVCodec {
struct VEncSignal {
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
    std::queue<std::shared_ptr<AVMemoryMock>> inMemoryQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> outMemoryQueue_;
    std::queue<std::shared_ptr<AVBufferMock>> inBufferQueue_;
    std::queue<std::shared_ptr<AVBufferMock>> outBufferQueue_;
    std::queue<std::shared_ptr<FormatMock>> inFormatQueue_;
    int32_t errorNum_ = 0;
    std::atomic<bool> isRunning_ = false;
    std::atomic<bool> isPreparing_ = true;
};

class VEncCallbackTest : public AVCodecCallbackMock {
public:
    explicit VEncCallbackTest(std::shared_ptr<VEncSignal> signal);
    virtual ~VEncCallbackTest();
    void OnError(int32_t errorCode) override;
    void OnStreamChanged(std::shared_ptr<FormatMock> format) override;
    void OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data) override;
    void OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr) override;

private:
    std::shared_ptr<VEncSignal> signal_ = nullptr;
};

class VEncCallbackTestExt : public MediaCodecCallbackMock {
public:
    explicit VEncCallbackTestExt(std::shared_ptr<VEncSignal> signal);
    virtual ~VEncCallbackTestExt();
    void OnError(int32_t errorCode) override;
    void OnStreamChanged(std::shared_ptr<FormatMock> format) override;
    void OnNeedInputData(uint32_t index, std::shared_ptr<AVBufferMock> data) override;
    void OnNewOutputData(uint32_t index, std::shared_ptr<AVBufferMock> data) override;

private:
    std::shared_ptr<VEncSignal> signal_ = nullptr;
};

class VEncParamCallbackTest : public MediaCodecParameterCallbackMock {
public:
    explicit VEncParamCallbackTest(std::shared_ptr<VEncSignal> signal);
    virtual ~VEncParamCallbackTest();
    void OnInputParameterAvailable(uint32_t index, std::shared_ptr<FormatMock> parameter) override;

private:
    std::shared_ptr<VEncSignal> signal_ = nullptr;
};

class VideoEncSample : public NoCopyable {
public:
    explicit VideoEncSample(std::shared_ptr<VEncSignal> signal);
    virtual ~VideoEncSample();
    bool CreateVideoEncMockByMime(const std::string &mime);
    bool CreateVideoEncMockByName(const std::string &name);

    int32_t Release();
    int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb);
    int32_t SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb);
    int32_t SetCallback(std::shared_ptr<MediaCodecParameterCallbackMock> cb);
    int32_t Configure(std::shared_ptr<FormatMock> format);
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    std::shared_ptr<FormatMock> GetOutputDescription();
    std::shared_ptr<FormatMock> GetInputDescription();
    int32_t SetParameter(std::shared_ptr<FormatMock> format);
    int32_t NotifyEos();
    int32_t PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr);
    int32_t FreeOutputData(uint32_t index);
    int32_t PushInputBuffer(uint32_t index);
    int32_t PushInputParameter(uint32_t index);
    int32_t FreeOutputBuffer(uint32_t index);
    int32_t CreateInputSurface();
    bool IsValid();

    void SetOutPath(const std::string &path);
    void SetIsHdrVivid(bool isHdrVivid);

private:
    void FlushInner();
    void PrepareInner();
    void WaitForEos();

    void InputFuncSurface();
    int32_t InputProcess(OH_NativeBuffer *nativeBuffer, OHNativeWindowBuffer *ohNativeWindowBuffer);

    int32_t ReadOneFrame();

    void RunInner();
    void InputParamLoopFunc();

    void OutputLoopFunc();
    void InputLoopFunc();
    int32_t OutputLoopInner();
    int32_t InputLoopInner();

    void RunInnerExt();
    void OutputLoopFuncExt();
    void InputLoopFuncExt();
    int32_t OutputLoopInnerExt();
    int32_t InputLoopInnerExt();
    std::shared_ptr<VideoEncMock> videoEnc_ = nullptr;
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::ofstream> outFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::unique_ptr<std::thread> inputSurfaceLoop_;
    std::shared_ptr<VEncSignal> signal_ = nullptr;
    std::string inPath_;
    std::string outPath_;
    std::string outSurfacePath_;
    int32_t frameInputCount_ = 0;
    int32_t frameOutputCount_ = 0;
    bool isAVBufferMode_ = false;
    bool isFirstFrame_ = true;
    bool isSurfaceMode_ = false;
    bool isHdrVivid_ = false;
    bool isSetParamCallback_ = false;
    bool isTemporalScalabilitySyncIdr_ = false;
    int64_t time_ = 0;
    sptr<Surface> consumer_ = nullptr;
    sptr<Surface> producer_ = nullptr;
    OHNativeWindow *nativeWindow_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VENC_SAMPLE_H