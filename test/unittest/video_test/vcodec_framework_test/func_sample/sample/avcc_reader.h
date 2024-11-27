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
    int32_t errorNum = 0;
    std::atomic<bool> isRunning_ = false;
    std::atomic<bool> isPreparing_ = true;
};

struct MpegReaderInfo {
    std::string inPath;
    bool isMpeg2Stream = true; // true: Mpeg2; false: Mpeg4
};

class MpegReader {
public:
    int32_t FillBuffer(std::shared_ptr<VDecSignal> &signal_, OH_AVCodecBufferAttr &attr);
    int32_t FillBufferExt(std::shared_ptr<VDecSignal> &signal_, OH_AVCodecBufferAttr &attr);
    void FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t mpegType, bool isEosFrame);
    bool IsEOS();
    int32_t Init(const std::shared_ptr<MpegReaderInfo> &info);
    std::mutex mutex_;
    int32_t frameInputCount_ = 0;
private:
    class MpegUnitReader {
    public:
        explicit MpegUnitReader(std::shared_ptr<std::ifstream> inputFile) : inputFile_(inputFile) {}
        virtual ~MpegUnitReader() {};
        uint8_t const *GetNextMpegUnitAddr();
        virtual int32_t ReadMpegUnit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos) = 0;
        virtual bool IsEOS() = 0;
        virtual void PrereadFile() = 0;

    protected:
        MpegUnitReader() {};
        virtual bool IsEOF() = 0;
        std::unique_ptr<std::vector<uint8_t>> mpegUnit_ = nullptr;
        std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    };

    class Mpeg2MetaUnitReader : public MpegUnitReader {
    public:
        explicit Mpeg2MetaUnitReader(std::shared_ptr<std::ifstream> inputFile);
        int32_t ReadMpegUnit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos) override;
        bool IsEOS() override;
        void PrereadFile() override;
        void PrereadMpeg2Unit();
    private:
        bool IsEOF() override;
        std::unique_ptr<uint8_t []> prereadBuffer_ = nullptr;
        uint32_t prereadBufferSize_ = 0;
        uint32_t pPrereadBuffer_ = 0;
    };

    class Mpeg4MetaUnitReader : public MpegUnitReader {
    public:
        explicit Mpeg4MetaUnitReader(std::shared_ptr<std::ifstream> inputFile);
        int32_t ReadMpegUnit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos) override;
        bool IsEOS() override;
        void PrereadFile() override;
        void PrereadMpeg4Unit();

    private:
        bool IsEOF() override;
        std::unique_ptr<uint8_t []> prereadBuffer_ = nullptr;
        uint32_t prereadBufferSize_ = 0;
        uint32_t pPrereadBuffer_ = 0;
    };

    class MpegDetector {
    public:
        virtual ~MpegDetector() {};
        virtual const uint8_t *GetMpegTypeAddr(const uint8_t *bufferAddr) = 0;
        virtual uint8_t GetMpegType(const uint8_t *bufferAddr) = 0;
        virtual bool IsI(uint8_t mpegType) = 0;
    };
    
    class Mpeg2Detector : public MpegDetector {
    public:
        const uint8_t *GetMpegTypeAddr(const uint8_t *bufferAddr) override;
        uint8_t GetMpegType(const uint8_t *bufferAddr) override;
        bool IsI(uint8_t mpegType) override;
    };

    class Mpeg4Detector : public MpegDetector {
    public:
        const uint8_t *GetMpegTypeAddr(const uint8_t *bufferAddr) override;
        uint8_t GetMpegType(const uint8_t *bufferAddr) override;
        bool IsI(uint8_t mpegType) override;
    };

    std::shared_ptr<MpegUnitReader> mpegUnitReader_ = nullptr;
    std::shared_ptr<MpegDetector> mpegDetector_ = nullptr;
};

struct AvccReaderInfo {
    std::string inPath;
    bool isH264Stream = true; // true: H264; false: H265
};

class AvccReader {
public:
    int32_t FillBuffer(std::shared_ptr<VDecSignal> &signal_, OH_AVCodecBufferAttr &attr);
    int32_t FillBufferExt(std::shared_ptr<VDecSignal> &signal_, OH_AVCodecBufferAttr &attr);
    void FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t naluType, bool isEosFrame);
    bool CheckFillBuffer(uint8_t naluType);
    bool IsEOS();
    int32_t Init(const std::shared_ptr<AvccReaderInfo> &info);
    std::mutex mutex_;
    int32_t frameInputCount_ = 0;
private:
    class NalUnitReader {
    public:
        explicit NalUnitReader(std::shared_ptr<std::ifstream> inputFile) : inputFile_(inputFile) {}
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
        explicit AvccNalUnitReader(std::shared_ptr<std::ifstream> inputFile);
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
        virtual bool IsFirstSlice(const uint8_t *nalTypeAddr) = 0;
        bool IsFullVCL(uint8_t nalType, const uint8_t *nextNalTypeAddr);
    };

    class AVCNalDetector : public NalDetector {
    public:
        uint8_t GetNalType(const uint8_t *bufferAddr) override;
        bool IsXPS(uint8_t nalType) override;
        bool IsIDR(uint8_t nalType) override;
        bool IsVCL(uint8_t nalType) override;
        bool IsFirstSlice(const uint8_t *nalTypeAddr) override;
    };

    class HEVCNalDetector : public NalDetector {
    public:
        uint8_t GetNalType(const uint8_t *bufferAddr) override;
        bool IsXPS(uint8_t nalType) override;
        bool IsIDR(uint8_t nalType) override;
        bool IsVCL(uint8_t nalType) override;
        bool IsFirstSlice(const uint8_t *nalTypeAddr) override;
    };

    std::shared_ptr<NalUnitReader> nalUnitReader_ = nullptr;
    std::shared_ptr<NalDetector> nalDetector_ = nullptr;
};
} // MediaAVCodec
} // OHOS
#endif // AVCC_READER_H