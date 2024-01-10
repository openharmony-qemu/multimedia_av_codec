/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_FRAME_DETECTOR_H
#define OHOS_MEDIA_FRAME_DETECTOR_H

#include <list>
#include <string>

namespace OHOS {
namespace Media {
enum CodeType {
    H264,
    H265
};

class FrameDetector {
public:
    static std::shared_ptr<FrameDetector> GetFrameDetector(CodeType type);
    bool IsContainIdrFrame(const uint8_t* buff, size_t bufSize);

protected:
    FrameDetector() = default;
    virtual ~FrameDetector() = default;

private:
    struct NALUInfo {
        size_t startPos;
        size_t endPos;
        uint8_t nalType;
    };

    virtual uint8_t GetNalType(uint8_t byte) = 0;
    virtual bool IsPPS(uint8_t nalType) = 0;
    virtual bool IsVCL(uint8_t nalType) = 0;
    virtual bool IsIDR(uint8_t nalType) = 0;
    virtual bool IsPrefixSEI(uint8_t nalType) { return false; }

    static constexpr uint8_t START_CODE[] = {0, 0, 1};
    static constexpr size_t START_CODE_LEN = sizeof(START_CODE);
};

class FrameDetectorH264 : public FrameDetector {
public:
    FrameDetectorH264() = default;
    ~FrameDetectorH264() override = default;

private:
    enum H264NalType : uint8_t {
        UNSPECIFIED = 0,
        NON_IDR = 1,
        PARTITION_A = 2,
        PARTITION_B = 3,
        PARTITION_C = 4,
        IDR = 5,
        SEI = 6,
        SPS = 7,
        PPS = 8,
        AU_DELIMITER = 9,
        END_OF_SEQUENCE = 10,
        END_OF_STREAM = 11,
        FILLER_DATA = 12,
        SPS_EXT = 13,
        PREFIX = 14,
        SUB_SPS = 15,
        DPS = 16,
    };
    uint8_t GetNalType(uint8_t byte) override;
    bool IsPPS(uint8_t nalType) override;
    bool IsVCL(uint8_t nalType) override;
    bool IsIDR(uint8_t nalType) override;
};

class FrameDetectorH265 : public FrameDetector {
public:
    FrameDetectorH265() = default;
    ~FrameDetectorH265() override = default;

private:
    enum H265NalType : uint8_t {
        HEVC_TRAIL_N = 0,
        HEVC_TRAIL_R = 1,
        HEVC_TSA_N = 2,
        HEVC_TSA_R = 3,
        HEVC_STSA_N = 4,
        HEVC_STSA_R = 5,
        HEVC_RADL_N = 6,
        HEVC_RADL_R = 7,
        HEVC_RASL_N = 8,
        HEVC_RASL_R = 9,
        HEVC_BLA_W_LP = 16,
        HEVC_BLA_W_RADL = 17,
        HEVC_BLA_N_LP = 18,
        HEVC_IDR_W_RADL = 19,
        HEVC_IDR_N_LP = 20,
        HEVC_CRA_NUT = 21,
        HEVC_VPS_NUT = 32,
        HEVC_SPS_NUT = 33,
        HEVC_PPS_NUT = 34,
        HEVC_AUD_NUT = 35,
        HEVC_EOS_NUT = 36,
        HEVC_EOB_NUT = 37,
        HEVC_FD_NUT = 38,
        HEVC_PREFIX_SEI_NUT = 39,
        HEVC_SUFFIX_SEI_NUT = 40,
    };
    uint8_t GetNalType(uint8_t byte) override;
    bool IsPPS(uint8_t nalType) override;
    bool IsVCL(uint8_t nalType) override;
    bool IsIDR(uint8_t nalType) override;
    bool IsPrefixSEI(uint8_t nalType) override;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIA_FRAME_DETECTOR_H