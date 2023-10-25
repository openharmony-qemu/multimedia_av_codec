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

#include "start_code_detector.h"
#include <memory>
#include <algorithm>
#include "hcodec_log.h"
using namespace std;

size_t StartCodeDetector::SetSource(const std::string &path, CodeType type)
{
    type_ = type;
    ifstream ifs(path, ios::binary);
    if (!ifs.is_open()) {
        LOGE("cannot open %s", path.c_str());
        return 0;
    }
    size_t fileSize = GetFileSizeInBytes(ifs);
    unique_ptr<uint8_t[]> buf = make_unique<uint8_t[]>(fileSize);
    ifs.read(reinterpret_cast<char *>(buf.get()), static_cast<std::streamsize>(fileSize));

    list<pair<size_t, FirstByteInNalu>> posOfFile;
    const uint8_t *pStart = buf.get();
    size_t pos = 0;
    while (pos < fileSize) {
        auto pFound = search(pStart + pos, pStart + fileSize, begin(START_CODE), end(START_CODE));
        pos = distance(pStart, pFound);
        if (pos == fileSize || pos + START_CODE_LEN >= fileSize) { // 没找到或找到的起始码正好在文件末尾
            break;
        }
        posOfFile.emplace_back(pos, pStart[pos + START_CODE_LEN]);
        pos += START_CODE_LEN;
    }
    for (auto it = posOfFile.begin(); it != posOfFile.end(); ++it) {
        auto nex = next(it);
        NALUInfo info {
            .startPos = it->first,
            .endPos = (nex == posOfFile.end()) ? (fileSize) : (nex->first),
            .nalType = GetNalType(it->second, type),
        };
        nals_.push_back(info);
    }
    BuildSampleList();
    return samples_.size();
}

void StartCodeDetector::BuildSampleList()
{
    for (auto it = nals_.begin(); it != nals_.end(); ++it) {
        Sample sample {
            .startPos = it->startPos,
            .endPos = it->endPos,
            .isCsd = false,
            .isIdr = false,
        };
        if (IsCsd(*it, type_)) {
            sample.isCsd = true;
            while (true) {
                auto nex = next(it);
                if (nex == nals_.end()) {
                    break;
                }
                if (IsCsd(*nex, type_)) {
                    sample.endPos = nex->endPos;
                    it++;
                } else {
                    break;
                }
            }
        } else if (IsIdr(*it, type_)) {
            sample.isIdr = true;
        }
        if (sample.isCsd) {
            csdIdxList_.push_back(samples_.size());
        } else if (sample.isIdr) {
            idrIdxList_.push_back(samples_.size());
        }
        sample.idx = samples_.size();
        samples_.push_back(sample);
    }
}

size_t StartCodeDetector::GetFileSizeInBytes(ifstream &ifs)
{
    ifs.seekg(0, ifstream::end);
    auto len = ifs.tellg();
    ifs.seekg(0, ifstream::beg);
    return static_cast<size_t>(len);
}

uint8_t StartCodeDetector::GetNalType(uint8_t byte, CodeType type)
{
    switch (type) {
        case H264: {
            return byte & 0b0001'1111;
        }
        case H265: {
            return (byte & 0b0111'1110) >> 1;
        }
        default: {
            return 0;
        }
    }
}

bool StartCodeDetector::IsCsd(const NALUInfo &nalu, CodeType type)
{
    uint8_t nalType = nalu.nalType;
    switch (type) {
        case H264:
            return nalType == static_cast<uint8_t>(H264NalType::SPS) ||
                   nalType == static_cast<uint8_t>(H264NalType::PPS);
        case H265:
            return nalType == static_cast<uint8_t>(H265NalType::HEVC_VPS_NUT) ||
                   nalType == static_cast<uint8_t>(H265NalType::HEVC_SPS_NUT) ||
                   nalType == static_cast<uint8_t>(H265NalType::HEVC_PPS_NUT);
        default:
            return false;
    }
}

bool StartCodeDetector::IsIdr(const NALUInfo &nalu, CodeType type)
{
    uint8_t nalType = nalu.nalType;
    switch (type) {
        case H264:
            return nalType == static_cast<uint8_t>(H264NalType::IDR);
        case H265:
            return nalType == static_cast<uint8_t>(H265NalType::HEVC_IDR_W_RADL) ||
                   nalType == static_cast<uint8_t>(H265NalType::HEVC_IDR_N_LP) ||
                   nalType == static_cast<uint8_t>(H265NalType::HEVC_CRA_NUT);
        default:
            return false;
    }
}

bool StartCodeDetector::SeekTo(size_t sampleIdx)
{
    if (sampleIdx >= samples_.size()) {
        return false;
    }

    auto idrIter = find_if(idrIdxList_.rbegin(), idrIdxList_.rend(), [sampleIdx](size_t idrIdx) {
        return idrIdx <= sampleIdx;
    });
    if (idrIter == idrIdxList_.rend()) {
        return false;
    }
    size_t idrIdx = *idrIter;

    auto csdIter = find_if(csdIdxList_.rbegin(), csdIdxList_.rend(), [idrIdx](size_t csdIdx) {
        return csdIdx < idrIdx;
    });
    if (csdIter == csdIdxList_.rend()) {
        return false;
    }
    size_t csdIdx = *csdIter;
    waitingCsd_ = csdIdx;
    nextSampleIdx_ = idrIdx;
    LOGI("csd idx=%zu, idr idx=%zu, target sample idx=%zu", csdIdx, idrIdx, sampleIdx);
    return true;
}

std::optional<Sample> StartCodeDetector::PeekNextSample()
{
    if (waitingCsd_.has_value()) {
        return samples_[waitingCsd_.value()];
    }
    if (nextSampleIdx_ >= samples_.size()) {
        return std::nullopt;
    }
    return samples_[nextSampleIdx_];
}

void StartCodeDetector::MoveToNext()
{
    if (waitingCsd_.has_value()) {
        waitingCsd_ = nullopt;
        return;
    }
    nextSampleIdx_++;
}