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

#ifndef FUNC_SAMPLE_ENCODER_BASE_H
#define FUNC_SAMPLE_ENCODER_BASE_H
#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include "securec.h"
#include "window.h"
#include "vcodec_mock.h"

namespace OHOS {
namespace MediaAVCodec {
struct VEncSignal {
public:
    std::mutex mutex_;
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::shared_mutex syncMutex_;
    std::condition_variable cond_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIndexQueue_;
    std::queue<uint32_t> outIndexQueue_;

    // API 9
    std::queue<OH_AVCodecBufferAttr> outAttrQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> inMemoryQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> outMemoryQueue_;

    // API 11
    std::queue<std::shared_ptr<AVBufferMock>> inBufferQueue_;
    std::queue<std::shared_ptr<AVBufferMock>> outBufferQueue_;
    std::queue<std::shared_ptr<FormatMock>> inFormatQueue_;
    std::queue<std::shared_ptr<FormatMock>> inAttrQueue_;

    int32_t errorNum_ = 0;
    std::atomic<bool> isRunning_ = false;
    std::atomic<bool> isPreparing_ = true;
};

struct LtrParam {
    bool enableUseLtr = false;
    int32_t ltrInterval;
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

class VEncParamWithAttrCallbackTest : public MediaCodecParameterWithAttrCallbackMock {
public:
    explicit VEncParamWithAttrCallbackTest(std::shared_ptr<VEncSignal> signal);
    virtual ~VEncParamWithAttrCallbackTest();
    void OnInputParameterWithAttrAvailable(uint32_t index, std::shared_ptr<FormatMock> attribute,
                                        std::shared_ptr<FormatMock> parameter) override;

private:
    std::shared_ptr<VEncSignal> signal_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FUNC_SAMPLE_ENCODER_BASE_H