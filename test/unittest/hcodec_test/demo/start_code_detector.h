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

#ifndef UTIL_STARTCODEDETECTOR_H
#define UTIL_STARTCODEDETECTOR_H

#include <string>
#include <list>
#include <vector>

using FirstByteInNalu = uint8_t;
using PositionPair = std::pair<size_t, size_t>;

enum CodeType {
    H264,
    H265
};
struct NaluUnit {
    size_t startPos;
    size_t endPos;
    bool isCsd;
    bool isEos;
};

class HCodecDemuxer {
public:
    HCodecDemuxer() = default;
    ~HCodecDemuxer() = default;

    bool LoadNaluListFromPath(const std::string &path, CodeType type);
    NaluUnit GetNext(CodeType type, int offset = -1);
    std::optional<PositionPair> FindReferenceInfo(size_t seekPos);
    size_t GetNaluCursor() const { return mNalCursor; }
    size_t GetTotalNaluCnt() const { return mNals.size(); };
private:
    struct NALUInfo {
        size_t offset;
        size_t size;
        uint8_t nalType;
    };
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
private:
    size_t mNalCursor = 0;
    std::vector<NALUInfo> mNals;
    std::vector<size_t> mXpsIndex;
    std::vector<size_t> mIdrIndex;
    static constexpr uint8_t START_CODE[] = {0, 0, 1};
    static constexpr size_t START_CODE_LEN = sizeof(START_CODE);
private:
    static std::list<std::pair<size_t, FirstByteInNalu>> FindAllNalInfo(const uint8_t *pStart, size_t length,
                                                                        size_t *numOfBytesShouldRoleBack);
    static size_t GetFileSizeInBytes(std::ifstream &ifs);
    static uint8_t GetNalType(uint8_t byte, CodeType type);
    static bool IsXpsStart(const NALUInfo &nalu, CodeType type);
    static bool IsCsd(const NALUInfo &nalu, CodeType type);
    static bool IsIdr(const NALUInfo &nalu, CodeType type);
};

#endif // UTIL_STARTCODEDETECTOR_H
