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

#ifdef SUPPORT_CODEC_VC1
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
#endif

struct Msvideo1ReaderInfo {
    std::string inPath;
};
class Msvideo1Reader : public DataProducerBase {
public:
    int32_t FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr) override;
    void FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t naluType, bool isEosFrame);
    bool IsEOS();
    int32_t Init(const std::shared_ptr<Msvideo1ReaderInfo> &info);
    std::mutex mutex_;
    int32_t frameInputCount_ = 0;
private:
    class Msvideo1UnitReader {
    public:
        explicit Msvideo1UnitReader(std::shared_ptr<std::ifstream> inputFile) : inputFile_(inputFile) {}
        virtual ~Msvideo1UnitReader() {};
        uint8_t const *GetNextMsvideo1UnitAddr();
        virtual int32_t ReadMsvideo1Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos);
        virtual bool IsEOS() = 0;
        virtual void PrereadFile() = 0;
        virtual void PrereadMsvideo1Unit();

    protected:
        Msvideo1UnitReader() {};
        virtual bool IsEOF() = 0;

        std::unique_ptr<std::vector<uint8_t>> msvideo1Unit_ = nullptr;
        std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    };

    class Msvideo1MetaUnitReader : public Msvideo1UnitReader {
    public:
        explicit Msvideo1MetaUnitReader(std::shared_ptr<std::ifstream> inputFile);
        int32_t ReadMsvideo1Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos) override;
        bool IsEOS() override;
        void PrereadFile() override;
        void PrereadMsvideo1Unit() override;
    private:
        bool IsEOF() override;
        uint8_t* GetDelimiterPos(uint8_t* addrstart, uint8_t* addrend);
        std::unique_ptr<uint8_t []> prereadBuffer_ = nullptr;
        uint32_t prereadBufferSize_ = 0;
        uint32_t pPrereadBuffer_ = 0;
        uint32_t frameIndex_ = 0;
    };

    class Msvideo1Detector {
    public:
        uint8_t* GetDelimiterPos(uint8_t* addrstart, uint8_t* addrend);
        const uint8_t *GetMsvideo1TypeAddr(const uint8_t *bufferAddr);
        uint8_t GetMsvideo1Type(const uint8_t *bufferAddr);
        bool IsI(uint8_t msvideo1Type);
    };

    std::shared_ptr<Msvideo1UnitReader> msvideo1UnitReader_ = nullptr;
    std::shared_ptr<Msvideo1Detector> msvideo1Detector_ = nullptr;
};

struct Wmv3ReaderInfo {
    std::string inPath;
    bool isMainStream = false;
};

class Wmv3Reader : public DataProducerBase {
public:
    int32_t FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr) override;
    void FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t frameType, bool isEosFrame);
    bool IsEOS();
    int32_t Init(const std::shared_ptr<Wmv3ReaderInfo> &info);
    std::mutex mutex_;
    int32_t frameInputCount_ = 0;
private:
    class Wmv3UnitReader {
    public:
        explicit Wmv3UnitReader(std::shared_ptr<std::ifstream> inputFile) : inputFile_(inputFile) {}
        virtual ~Wmv3UnitReader() {};
        uint8_t const *GetNextWmv3UnitAddr();
        virtual int32_t ReadWmv3Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos) = 0;
        virtual bool IsEOS() = 0;
        virtual void PrereadFile() = 0;

    protected:
        Wmv3UnitReader() {};
        virtual bool IsEOF() = 0;
        std::unique_ptr<std::vector<uint8_t>> wmv3Unit_ = nullptr;
        std::shared_ptr<std::ifstream> inputFile_ = nullptr;
        bool isMainStream_ = false;
    };

    class Wmv3MetaUnitReader : public Wmv3UnitReader {
    public:
        explicit Wmv3MetaUnitReader(std::shared_ptr<std::ifstream> inputFile, bool isMainStream);
        int32_t ReadWmv3Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEos) override;
        bool IsEOS() override;
        void PrereadFile() override;
        void PrereadWmv3Unit();

    private:
        bool IsEOF() override;
        uint32_t GetFrameLenth(uint32_t index);
        std::unique_ptr<uint8_t []> prereadBuffer_ = nullptr;
        uint32_t prereadBufferSize_ = 0;
        uint32_t pPrereadBuffer_ = 0;
        uint32_t frameIndex_ = 0;
    };

    std::shared_ptr<Wmv3UnitReader> wmv3UnitReader_ = nullptr;
};

#ifdef SUPPORT_CODEC_VP8
struct Vp8ReaderInfo {
    std::string inPath;
};

class Vp8Reader : public DataProducerBase {
public:
    int32_t Init(const std::shared_ptr<Vp8ReaderInfo>& info);
    int32_t FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr) override;
    void FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t vp8Type, bool isEosFrame);
    bool IsEOS();

    std::mutex mutex_;
    int32_t frameInputCount_ = 0;

private:
    class Vp8UnitReader {
    public:
        explicit Vp8UnitReader(std::shared_ptr<std::ifstream> inputFile) : inputFile_(inputFile) {}
        virtual ~Vp8UnitReader() = default;
        uint8_t const *GetNextVp8UnitAddr();
        virtual int32_t ReadVp8Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame) = 0;
        virtual bool IsEOS() = 0;

    protected:
        Vp8UnitReader() = default;
        virtual bool IsEOF() = 0;
        virtual void PrereadVp8Unit() = 0;

        std::unique_ptr<std::vector<uint8_t>> vp8Unit_ = nullptr;
        std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    };

    class Vp8MetaUnitReader : public Vp8UnitReader {
    public:
        explicit Vp8MetaUnitReader(std::shared_ptr<std::ifstream> inputFile);
        bool IsEOS() override;
        int32_t ReadVp8Unit(uint8_t* bufferAddr, int32_t& bufferSize, bool& isEosFrame) override;

    private:
        bool IsEOF() override;
        void PrereadVp8Unit() override;

        std::unique_ptr<uint8_t[]> prereadBuffer_ = nullptr;
        uint32_t prereadBufferSize_ = 0;
        uint32_t pPrereadBuffer_ = 0;
        uint32_t frameIndex_ = 0;
    };

    class Vp8Detector {
    public:
        virtual ~Vp8Detector() = default;
        const uint8_t *GetVp8TypeAddr(const uint8_t *bufferAddr);
        uint8_t GetVp8Type(const uint8_t *bufferAddr);
        bool IsKeyFrame(uint8_t vp8Type);
    };

    class IvfUnitReader : public Vp8UnitReader {
    public:
        explicit IvfUnitReader(std::shared_ptr<std::ifstream> inputFile);
        bool IsEOS() override;
        int32_t ReadVp8Unit(uint8_t* bufferAddr, int32_t& bufferSize, bool& isEosFrame) override;

    private:
        bool IsEOF() override;
        void PrereadVp8Unit() override;
        bool ParseIvfFileHeader();
        bool ParseIvfFrameHeader(uint32_t& frameSize);

        bool fileHeaderParsed_ = false;
        uint32_t frameIndex_ = 0;
        uint32_t totalFrames_ = 0;
    };

    std::shared_ptr<Vp8UnitReader> vp8UnitReader_ = nullptr;
    std::shared_ptr<Vp8Detector> vp8Detector_ = nullptr;
};
#endif

#ifdef SUPPORT_CODEC_VP9
struct Vp9ReaderInfo {
    std::string inPath;
};

class Vp9Reader : public DataProducerBase {
public:
    int32_t Init(const std::shared_ptr<Vp9ReaderInfo>& info);
    int32_t FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr) override;
    void FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t vp9Type, bool isEosFrame);
    bool IsEOS();

    std::mutex mutex_;
    int32_t frameInputCount_ = 0;

private:
    class Vp9UnitReader {
    public:
        explicit Vp9UnitReader(std::shared_ptr<std::ifstream> inputFile) : inputFile_(inputFile) {}
        virtual ~Vp9UnitReader() = default;
        uint8_t const *GetNextVp9UnitAddr();
        virtual int32_t ReadVp9Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame) = 0;
        virtual bool IsEOS() = 0;

    protected:
        Vp9UnitReader() = default;
        virtual bool IsEOF() = 0;
        virtual void PrereadVp9Unit() = 0;

        std::unique_ptr<std::vector<uint8_t>> vp9Unit_ = nullptr;
        std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    };

    class Vp9MetaUnitReader : public Vp9UnitReader {
    public:
        explicit Vp9MetaUnitReader(std::shared_ptr<std::ifstream> inputFile);
        bool IsEOS() override;
        int32_t ReadVp9Unit(uint8_t* bufferAddr, int32_t& bufferSize, bool& isEosFrame) override;

    private:
        bool IsEOF() override;
        void PrereadVp9Unit() override;
        std::unique_ptr<uint8_t[]> prereadBuffer_ = nullptr;
        uint32_t prereadBufferSize_ = 0;
        uint32_t pPrereadBuffer_ = 0;
        uint32_t frameIndex_ = 0;
    };

    class Vp9Detector {
    public:
        virtual ~Vp9Detector() = default;
        const uint8_t *GetVp9TypeAddr(const uint8_t *bufferAddr);
        uint8_t GetVp9Type(const uint8_t *bufferAddr);
        bool IsKeyFrame(uint8_t vp9Type);
    };

    class IvfUnitReader : public Vp9UnitReader {
    public:
        explicit IvfUnitReader(std::shared_ptr<std::ifstream> inputFile);
        bool IsEOS() override;
        int32_t ReadVp9Unit(uint8_t* bufferAddr, int32_t& bufferSize, bool& isEosFrame) override;

    private:
        bool IsEOF() override;
        void PrereadVp9Unit() override;
        bool ParseIvfFileHeader();
        bool ParseIvfFrameHeader(uint32_t& frameSize);
        bool fileHeaderParsed_ = false;
        uint32_t frameIndex_ = 0;
        uint32_t totalFrames_ = 0;
    };

    std::shared_ptr<Vp9UnitReader> vp9UnitReader_ = nullptr;
    std::shared_ptr<Vp9Detector> vp9Detector_ = nullptr;
};
#endif

#ifdef SUPPORT_CODEC_AV1
struct Av1ReaderInfo {
    std::string inPath;
};

class Av1Reader : public DataProducerBase {
public:
    int32_t Init(const std::shared_ptr<Av1ReaderInfo>& info);
    int32_t FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr) override;
    void FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t av1Type, bool isEosFrame);
    bool IsEOS();

    std::mutex mutex_;
    int32_t frameInputCount_ = 0;

private:
    class Av1UnitReader {
    public:
        explicit Av1UnitReader(std::shared_ptr<std::ifstream> inputFile) : inputFile_(inputFile) {}
        virtual ~Av1UnitReader() = default;
        uint8_t const *GetNextAv1UnitAddr();
        int32_t ReadAv1Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame);
        virtual bool IsEOS() = 0;

    protected:
        Av1UnitReader() = default;
        virtual bool IsEOF() = 0;
        virtual void PrereadAv1Unit() = 0;

        std::unique_ptr<std::vector<uint8_t>> av1Unit_ = nullptr;
        std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    };

    class Av1MetaUnitReader : public Av1UnitReader {
    public:
        explicit Av1MetaUnitReader(std::shared_ptr<std::ifstream> inputFile);
        bool IsEOS() override;
        int32_t ReadAv1Unit(uint8_t* bufferAddr, int32_t& bufferSize, bool& isEosFrame);

    private:
        bool IsEOF() override;
        void PrereadAv1Unit() override;
        std::unique_ptr<uint8_t[]> prereadBuffer_ = nullptr;
        uint32_t prereadBufferSize_ = 0;
        uint32_t pPrereadBuffer_ = 0;
        uint32_t frameIndex_ = 0;
    };

    class Av1Detector {
    public:
        virtual ~Av1Detector() = default;
        const uint8_t *GetAv1TypeAddr(const uint8_t *bufferAddr);
        uint8_t GetAv1Type(const uint8_t *bufferAddr);
        bool IsKeyFrame(uint8_t av1Type);
    };

    class IvfUnitReader : public Av1UnitReader {
    public:
        explicit IvfUnitReader(std::shared_ptr<std::ifstream> inputFile);
        bool IsEOS() override;
        int32_t ReadAv1Unit(uint8_t* bufferAddr, int32_t& bufferSize, bool& isEosFrame);

    private:
        bool IsEOF() override;
        void PrereadAv1Unit() override;
        bool ParseIvfFileHeader();
        bool ParseIvfFrameHeader(uint32_t& frameSize);
        bool fileHeaderParsed_ = false;
        uint32_t frameIndex_ = 0;
        uint32_t totalFrames_ = 0;
    };

    std::shared_ptr<Av1UnitReader> av1UnitReader_ = nullptr;
    std::shared_ptr<Av1Detector> av1Detector_ = nullptr;
};
#endif
} // MediaAVCodec
} // OHOS
#endif // AVCC_READER_H