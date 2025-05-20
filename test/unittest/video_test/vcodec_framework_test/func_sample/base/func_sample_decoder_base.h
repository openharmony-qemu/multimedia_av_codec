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

#ifndef FUNC_SAMPLE_DECODER_BASE_H
#define FUNC_SAMPLE_DECODER_BASE_H
#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <thread>
#include "avcc_reader.h"
#include "securec.h"
#include "openssl/sha.h"
#include "vcodec_mock.h"

namespace OHOS {
namespace MediaAVCodec {
struct VDecSignal {
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
    std::queue<OH_AVCodecBufferAttr> outAttrQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> inMemoryQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> outMemoryQueue_;
    std::queue<std::shared_ptr<AVBufferMock>> inBufferQueue_;
    std::queue<std::shared_ptr<AVBufferMock>> outBufferQueue_;
    int32_t errorNum = 0;
    std::atomic<bool> isRunning_ = false;
    std::atomic<bool> isPreparing_ = true;

    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t wStride_ = 0;
    int32_t hStride_ = 0;
};

enum VCodecDataProducerType : int32_t {
    H263_STREAM = 1 << 0,
    AVC_STREAM = 1 << 1,
    HEVC_STREAM = 1 << 2,
    MPEG2_STREAM = 1 << 3,
    MPEG4_STREAM = 1 << 4
};

class VDecCallbackTest : public AVCodecCallbackMock {
public:
    explicit VDecCallbackTest(std::shared_ptr<VDecSignal> signal);
    ~VDecCallbackTest() override;
    void OnError(int32_t errorCode) override;
    void OnStreamChanged(std::shared_ptr<FormatMock> format) override;
    void OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data) override;
    void OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr) override;

private:
    std::shared_ptr<VDecSignal> signal_ = nullptr;
};

class VDecCallbackTestExt : public MediaCodecCallbackMock {
public:
    explicit VDecCallbackTestExt(std::shared_ptr<VDecSignal> signal);
    ~VDecCallbackTestExt() override;
    void OnError(int32_t errorCode) override;
    void OnStreamChanged(std::shared_ptr<FormatMock> format) override;
    void OnNeedInputData(uint32_t index, std::shared_ptr<AVBufferMock> data) override;
    void OnNewOutputData(uint32_t index, std::shared_ptr<AVBufferMock> data) override;

private:
    std::shared_ptr<VDecSignal> signal_ = nullptr;
};

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(Surface *cs, std::string_view name, bool needCheckSHA = false);
    ~TestConsumerListener();
    void OnBufferAvailable() override;

private:
    int64_t timestamp_ = 0;
    Rect damage_ = {};
    Surface *cs_ = nullptr;
    bool needCheckSHA_ = false;
    std::unique_ptr<std::ofstream> outFile_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FUNC_SAMPLE_DECODER_BASE_H