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
#include "openssl/sha.h"
#include "securec.h"
#include "window.h"
#include "vcodec_mock.h"

namespace OHOS {
namespace MediaAVCodec {
inline constexpr int32_t REQUEST_I_FRAME = 1;
inline constexpr int32_t REQUEST_I_FRAME_NUM = 13;
inline constexpr uint32_t BUFFER_COUNT = 59;
inline constexpr uint8_t SHA_AVC[SHA512_DIGEST_LENGTH] = {
    0x39, 0x2c, 0x3a, 0x78, 0xf0, 0xf7, 0xbe, 0xde, 0xc8, 0x2d, 0x45, 0x19, 0x9d, 0xd8, 0x3e, 0x88,
    0x5f, 0xf0, 0x0b, 0xf5, 0x14, 0x01, 0x21, 0xea, 0xd2, 0xcf, 0xe3, 0xd9, 0x35, 0x6a, 0x2c, 0x98,
    0xc6, 0x48, 0xc7, 0x90, 0xca, 0xe7, 0xc1, 0xb0, 0x4e, 0x9c, 0x05, 0x1e, 0xdd, 0x22, 0xd2, 0xe0,
    0x5e, 0x9a, 0xf8, 0xbc, 0xbe, 0x39, 0x26, 0x46, 0x6e, 0xa3, 0xcd, 0x1b, 0xbb, 0xf5, 0xc8, 0x87};
inline constexpr uint8_t SHA_HEVC[SHA512_DIGEST_LENGTH] = {
    0xb0, 0xca, 0x29, 0xa3, 0x3c, 0x8e, 0x36, 0x3f, 0xbc, 0x30, 0xa8, 0x70, 0x09, 0x29, 0xb5, 0xff,
    0x8f, 0xe2, 0xf9, 0x58, 0xc5, 0x00, 0x02, 0x7c, 0xa9, 0x05, 0xe0, 0x69, 0x09, 0xa7, 0x2e, 0xb2,
    0xdf, 0x5d, 0xf4, 0x05, 0xea, 0xde, 0xe9, 0x9b, 0x1e, 0x5b, 0x37, 0x04, 0x2f, 0x3d, 0xe9, 0x2c,
    0xb2, 0x8c, 0xc3, 0x99, 0xd4, 0xdc, 0xdf, 0xee, 0xb4, 0xd9, 0x0c, 0xd0, 0xee, 0x39, 0x94, 0x3c};
constexpr int32_t TIMESTAMP_BASE = 1000000;
constexpr int32_t DURATION_BASE = 46000;
constexpr int32_t RATIO_US_TO_NS = 1000;

inline uint8_t g_mdTest[SHA512_DIGEST_LENGTH];
inline std::atomic<uint32_t> g_shaBufferCount = 0;
inline SHA512_CTX g_ctxTest;

void UpdateSHA(std::unique_ptr<std::ofstream> &outFile, const char *addr, int32_t size, bool needCheckSHA,
               bool needDump)
{
    if (needCheckSHA) {
        ++g_shaBufferCount;
    }
    if (needCheckSHA && g_shaBufferCount < BUFFER_COUNT) {
        SHA512_Update(&g_ctxTest, addr, size);
    }
    if (needDump) {
        if (!outFile->is_open()) {
            std::cout << "output data fail" << std::endl;
        }
        (void)outFile->write(addr, size);
    }
}

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