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

#include "frame_detector.h"

#include <algorithm>
#include <memory>

#include "common/log.h"

namespace OHOS {
namespace Media {
using namespace std;

std::shared_ptr<FrameDetector> FrameDetector::GetFrameDetector(CodeType type)
{
    switch (type) {
        case H264:
            static std::shared_ptr<FrameDetectorH264> frameDetectorH264 = make_shared<FrameDetectorH264>();
            return frameDetectorH264;
        case H265:
            static std::shared_ptr<FrameDetectorH265> frameDetectorH265 = make_shared<FrameDetectorH265>();
            return frameDetectorH265;
        default:
            return nullptr;
    }
}

bool FrameDetector::IsContainIdrFrame(const uint8_t* buff, size_t bufSize)
{
    if (buff == nullptr) {
        return false;
    }
    using FirstByteInNalu = uint8_t;
    list<pair<size_t, FirstByteInNalu>> posOfNalUnit;
    size_t pos = 0;
    while (pos < bufSize) {
        auto pFound = search(buff + pos, buff + bufSize, begin(START_CODE), end(START_CODE));
        pos = static_cast<size_t>(distance(buff, pFound));
        if (pos == bufSize || pos + START_CODE_LEN >= bufSize) { // fail to find startCode or just at the end
            break;
        }
        posOfNalUnit.emplace_back(pos, buff[pos + START_CODE_LEN]);
        pos += START_CODE_LEN;
    }
    for (auto it = posOfNalUnit.begin(); it != posOfNalUnit.end(); ++it) {
        auto nex = next(it);
        NALUInfo nal {
            .startPos = it->first,
            .endPos = (nex == posOfNalUnit.end()) ? (bufSize) : (nex->first),
            .nalType = GetNalType(it->second),
        };
        if (IsIDR(nal.nalType)) {
            return true;
        }
    }
    return false;
}

uint8_t FrameDetectorH264::GetNalType(uint8_t byte)
{
    return byte & 0b0001'1111;
}

bool FrameDetectorH264::IsPPS(uint8_t nalType)
{
    return nalType == H264NalType::PPS;
}

bool FrameDetectorH264::IsVCL(uint8_t nalType)
{
    return nalType >= H264NalType::NON_IDR && nalType <= H264NalType::IDR;
}

bool FrameDetectorH264::IsIDR(uint8_t nalType)
{
    return nalType == H264NalType::IDR;
}

uint8_t FrameDetectorH265::GetNalType(uint8_t byte)
{
    return (byte & 0b0111'1110) >> 1;
}

bool FrameDetectorH265::IsPPS(uint8_t nalType)
{
    return nalType == H265NalType::HEVC_PPS_NUT;
}

bool FrameDetectorH265::IsVCL(uint8_t nalType)
{
    return nalType >= H265NalType::HEVC_TRAIL_N && nalType <= H265NalType::HEVC_CRA_NUT;
}

bool FrameDetectorH265::IsIDR(uint8_t nalType)
{
    return nalType == H265NalType::HEVC_IDR_W_RADL ||
           nalType == H265NalType::HEVC_IDR_N_LP ||
           nalType == H265NalType::HEVC_CRA_NUT;
}

bool FrameDetectorH265::IsPrefixSEI(uint8_t nalType)
{
    return nalType == H265NalType::HEVC_PREFIX_SEI_NUT;
}
} // namespace Media
} // namespace OHOS