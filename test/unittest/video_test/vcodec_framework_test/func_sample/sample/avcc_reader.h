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

#ifndef AVCC_READER_H
#define AVCC_READER_H
#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include "../../func_sample/mock/vcodec_mock.h"

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
    std::queue<std::shared_ptr<AVMemoryMock>> inMemoryQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> outMemoryQueue_;
    std::queue<std::shared_ptr<AVBufferMock>> inBufferQueue_;
    std::queue<std::shared_ptr<AVBufferMock>> outBufferQueue_;
    int32_t errorNum_ = 0;
    std::atomic<bool> isRunning_ = false;
    std::atomic<bool> isPreparing_ = true;
};

struct AvccReaderInfo {
    std::string inPath_;
    bool isH264Stream_ = true; // true: H264; false: H265
};

class AvccReader {
public:
    int32_t FillBuffer(std::shared_ptr<VDecSignal> &signal_, OH_AVCodecBufferAttr &attr);
    int32_t FillBufferExt(std::shared_ptr<VDecSignal> &signal_, OH_AVCodecBufferAttr &attr);
    void FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t naluType, bool isEosFrame);
    bool IsEOS();
    int32_t Init(const std::shared_ptr<AvccReaderInfo> &info);
    std::mutex mutex_;
    int32_t frameInputCount_ = 0;
private:
    class NalUnitReader {
    public:
        NalUnitReader(std::shared_ptr<std::ifstream> inputFile) : inputFile_(inputFile) {}
        virtual ~NalUnitReader() {};
        uint8_t const *GetNextNalUnitAddr();
        int32_t ReadNalUnit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos);
        virtual bool IsEOS() = 0;

    protected:
        NalUnitReader() {};
        virtual bool IsEOF() = 0;
        virtual void PrereadNalUnit() = 0;

        std::unique_ptr<std::vector<uint8_t>> nalUnit_ = nullptr;
        std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    };

    class AvccNalUnitReader : public NalUnitReader {
    public:
        AvccNalUnitReader(std::shared_ptr<std::ifstream> inputFile);
        bool IsEOS() override;

    private:
        bool IsEOF() override;
        void PrereadNalUnit() override;
        int32_t ToAnnexb(uint8_t *bufferAddr);
    };

    class NalDetector {
    public:
        virtual ~NalDetector() {};
        const uint8_t *GetNalTypeAddr(const uint8_t *bufferAddr);
        virtual uint8_t GetNalType(const uint8_t *bufferAddr) = 0;
        virtual bool IsXPS(uint8_t nalType) = 0;
        virtual bool IsIDR(uint8_t nalType) = 0;
        virtual bool IsVCL(uint8_t nalType) = 0;
        virtual bool IsFullVCL(uint8_t nalType, const uint8_t *nextNaluTypeAddr) = 0;
        virtual bool IsFirstSlice(const uint8_t *nextNaluTypeAddr) = 0;
    };

    class AVCNalDetector : public NalDetector {
    public:
        uint8_t GetNalType(const uint8_t *bufferAddr) override;
        bool IsXPS(uint8_t nalType) override;
        bool IsIDR(uint8_t nalType) override;
        bool IsVCL(uint8_t nalType) override;
        bool IsFullVCL(uint8_t nalType, const uint8_t *nextNaluTypeAddr) override;
        bool IsFirstSlice(const uint8_t *nextNaluTypeAddr) override;
    };

    class HEVCNalDetector : public NalDetector {
    public:
        uint8_t GetNalType(const uint8_t *bufferAddr) override;
        bool IsXPS(uint8_t nalType) override;
        bool IsIDR(uint8_t nalType) override;
        bool IsVCL(uint8_t nalType) override;
        bool IsFullVCL(uint8_t nalType, const uint8_t *nextNaluTypeAddr) override;
        bool IsFirstSlice(const uint8_t *nextNaluTypeAddr) override;
    };

    std::shared_ptr<NalUnitReader> nalUnitReader_ = nullptr;
    std::shared_ptr<NalDetector> nalDetector_ = nullptr;
};
} // MediaAVCodec
} // OHOS
#endif // AVCC_READER_H