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

#ifndef AVCODEC_SAMPLE_DATA_PRODUCER_BITSTREAM_READER_H
#define AVCODEC_SAMPLE_DATA_PRODUCER_BITSTREAM_READER_H
#include "data_producer_base.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class BitstreamReader : public DataProducerBase {
public:
    int32_t Init(const std::shared_ptr<SampleInfo> &info) override;

private:
    int32_t FillBuffer(CodecBufferInfo &bufferInfo) override;
    bool IsEOS() override;

    class NalUnitReader {
    public:
        explicit NalUnitReader(std::shared_ptr<std::ifstream> inputFile) : inputFile_(inputFile) {}
        virtual ~NalUnitReader() {};
        uint8_t const *GetNextNalUnitAddr();
        int32_t ReadNalUnit(uint8_t *bufferAddr, int32_t &bufferSize, uint32_t bufferCapacity);
        virtual bool IsEOS() = 0;

    protected:
        NalUnitReader() {};
        virtual bool IsEOF() = 0;
        virtual void PrereadNalUnit() = 0;

        std::shared_ptr<std::ifstream> inputFile_ = nullptr;
        std::unique_ptr<std::vector<uint8_t>> nalUnit_;
        uint32_t readSize_ = 0;
    };

    class AnnexbNalUnitReader : public NalUnitReader {
    public:
        explicit AnnexbNalUnitReader(std::shared_ptr<std::ifstream> inputFile);
        bool IsEOS() override;

    private:
        bool IsEOF() override;
        void PrereadFile();
        void PrereadNalUnit() override;

        std::unique_ptr<uint8_t []> prereadBuffer_ = nullptr;
        uint32_t prereadBufferSize_ = 0;
        uint32_t pPrereadBuffer_ = 0;
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
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_DATA_PRODUCER_BITSTREAM_READER_H