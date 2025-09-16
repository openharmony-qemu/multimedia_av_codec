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
#include <string>
#include "avcodec_common.h"
#include "data_producer_base.h"
#include "native_avbuffer_info.h"

namespace OHOS {
namespace MediaAVCodec {

struct MpegReaderInfo {
    std::string inPath;
    bool isMpeg2Stream = true; // true: Mpeg2; false: Mpeg4
};

class MpegReader : public DataProducerBase {
public:
    int32_t FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr) override;
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


struct H263ReaderInfo {
    std::string inPath;
};


class H263Reader : public DataProducerBase {
public:
    int32_t FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr) override;
    void FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t mpegType, bool isEosFrame);
    bool IsEOS();
    int32_t Init(const std::shared_ptr<H263ReaderInfo> &info);
    std::mutex mutex_;
    int32_t frameInputCount_ = 0;
private:
    class H263UnitReader {
    public:
        explicit H263UnitReader(std::shared_ptr<std::ifstream> inputFile) : inputFile_(inputFile) {}
        virtual ~H263UnitReader() {};
        uint8_t const *GetNextH263UnitAddr();
        virtual int32_t ReadH263Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos) = 0;
        virtual bool IsEOS() = 0;
        virtual void PrereadFile() = 0;

    protected:
        H263UnitReader() {};
        virtual bool IsEOF() = 0;
        std::unique_ptr<std::vector<uint8_t>> h263Unit_ = nullptr;
        std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    };

    class H263MetaUnitReader : public H263UnitReader {
    public:
        explicit H263MetaUnitReader(std::shared_ptr<std::ifstream> inputFile);
        int32_t ReadH263Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos) override;
        bool IsEOS() override;
        void PrereadFile() override;
        void PrereadH263Unit();

    private:
        bool IsEOF() override;
        uint8_t* GetDelimiterPos(uint8_t* addrstart, uint8_t* addrend);
        std::unique_ptr<uint8_t []> prereadBuffer_ = nullptr;
        uint32_t prereadBufferSize_ = 0;
        uint32_t pPrereadBuffer_ = 0;
    };
    
    class H263Detector {
    public:
        uint8_t* GetDelimiterPos(uint8_t* addrstart, uint8_t* addrend);
        const uint8_t *GetH263TypeAddr(const uint8_t *bufferAddr);
        uint8_t GetH263Type(const uint8_t *bufferAddr);
        bool IsI(uint8_t h263Type);
    };

    std::shared_ptr<H263UnitReader> h263UnitReader_ = nullptr;
    std::shared_ptr<H263Detector> h263Detector_ = nullptr;
};


struct AvccReaderInfo {
    std::string inPath;
    bool isAvcStream = true; // true: AVC; false: HEVC
};

class AvccReader : public DataProducerBase {
public:
    int32_t FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr) override;
    int32_t KeepFillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr);
    void FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t naluType, bool isEosFrame);
    bool IsEOS();
    bool CheckFillBuffer(uint8_t naluType);
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

struct Vc1ReaderInfo {
    std::string inPath;
};
class Vc1Reader : public DataProducerBase {
public:
    int32_t FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr) override;
    void FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t naluType, bool isEosFrame);
    bool IsEOS();
    int32_t Init(const std::shared_ptr<Vc1ReaderInfo> &info);
    std::mutex mutex_;
    int32_t frameInputCount_ = 0;
private:
    class Vc1UnitReader {
    public:
        explicit Vc1UnitReader(std::shared_ptr<std::ifstream> inputFile) : inputFile_(inputFile) {}
        virtual ~Vc1UnitReader() {};
        uint8_t const *GetNextVc1UnitAddr();
        virtual int32_t ReadVc1Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos);
        virtual bool IsEOS() = 0;
        virtual void PrereadFile() = 0;
        virtual void PrereadVc1Unit();

    protected:
        Vc1UnitReader() {};
        virtual bool IsEOF() = 0;

        std::unique_ptr<std::vector<uint8_t>> vc1Unit_ = nullptr;
        std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    };

    class Vc1MetaUnitReader : public Vc1UnitReader {
    public:
        explicit Vc1MetaUnitReader(std::shared_ptr<std::ifstream> inputFile);
        int32_t ReadVc1Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos) override;
        bool IsEOS() override;
        void PrereadFile() override;
        void PrereadVc1Unit() override;
    private:
        bool IsEOF() override;
        uint8_t* GetDelimiterPos(uint8_t* addrstart, uint8_t* addrend);
        uint8_t* FindNextStartCode(uint8_t* start, uint8_t* end);
        uint8_t GetVc1UnitType(uint8_t* startCode);
        std::unique_ptr<uint8_t []> prereadBuffer_ = nullptr;
        uint32_t prereadBufferSize_ = 0;
        uint32_t pPrereadBuffer_ = 0;
        uint32_t frameIndex_ = 0;
    };
    class Vc1Detector {
    public:
        uint8_t* GetDelimiterPos(uint8_t* addrstart, uint8_t* addrend);
        const uint8_t *GetVc1TypeAddr(const uint8_t *bufferAddr);
        uint8_t GetVc1Type(const uint8_t *bufferAddr);
        bool IsI(uint8_t vc1Type);
    };

    std::shared_ptr<Vc1UnitReader> vc1UnitReader_ = nullptr;
    std::shared_ptr<Vc1Detector> vc1Detector_ = nullptr;
};
} // MediaAVCodec
} // OHOS
#endif // AVCC_READER_H