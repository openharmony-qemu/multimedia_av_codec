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

std::shared_ptr<StartCodeDetector> StartCodeDetector::Create(CodeType type)
{
    switch (type) {
        case H264:
            return make_shared<StartCodeDetectorH264>();
        case H265:
            return make_shared<StartCodeDetectorH265>();
        default:
            return nullptr;
    }
}

size_t StartCodeDetector::SetSource(const std::string &path)
{
    ifstream ifs(path, ios::binary);
    if (!ifs.is_open()) {
        LOGE("cannot open %s", path.c_str());
        return 0;
    }
    size_t fileSize = GetFileSizeInBytes(ifs);
    unique_ptr<uint8_t[]> buf = make_unique<uint8_t[]>(fileSize);
    ifs.read(reinterpret_cast<char *>(buf.get()), static_cast<std::streamsize>(fileSize));

    using FirstByteInNalu = uint8_t;
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
            .nalType = GetNalType(it->second),
        };
        nals_.push_back(info);
    }
    BuildSampleList();
    return samples_.size();
}

void StartCodeDetector::BuildSampleList()
{
    shared_ptr<Sample> sample;
    for (auto& nal : nals_) {
        if (sample == nullptr) {
            sample = make_shared<Sample>();
            sample->startPos = nal.startPos;
            sample->isCsd = false;
            sample->isIdr = false;
        }
        sample->endPos = nal.endPos;
        if (!sample->s.empty()) {
            sample->s += "+";
        }
        sample->s += to_string(nal.nalType);

        bool isPPS = IsPPS(nal.nalType);
        bool isVCL = IsVCL(nal.nalType);
        bool isIDR = IsIDR(nal.nalType);
        if (isPPS || isVCL) {  // should cut here and build one sample
            if (isPPS) {
                sample->isCsd = true;
                csdIdxList_.push_back(samples_.size());
            }
            if (isIDR) {
                sample->isIdr = true;
                idrIdxList_.push_back(samples_.size());
            }
            sample->idx = samples_.size();
            samples_.push_back(*sample);
            sample.reset();
        }
    }
}

size_t StartCodeDetector::GetFileSizeInBytes(ifstream &ifs)
{
    ifs.seekg(0, ifstream::end);
    auto len = ifs.tellg();
    ifs.seekg(0, ifstream::beg);
    return static_cast<size_t>(len);
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

uint8_t StartCodeDetectorH264::GetNalType(uint8_t byte)
{
    return byte & 0b0001'1111;
}

bool StartCodeDetectorH264::IsPPS(uint8_t nalType)
{
    return nalType == H264NalType::PPS;
}

bool StartCodeDetectorH264::IsVCL(uint8_t nalType)
{
    return nalType >= H264NalType::NON_IDR && nalType <= H264NalType::IDR;
}

bool StartCodeDetectorH264::IsIDR(uint8_t nalType)
{
    return nalType == H264NalType::IDR;
}

uint8_t StartCodeDetectorH265::GetNalType(uint8_t byte)
{
    return (byte & 0b0111'1110) >> 1;
}

bool StartCodeDetectorH265::IsPPS(uint8_t nalType)
{
    return nalType == H265NalType::HEVC_PPS_NUT;
}

bool StartCodeDetectorH265::IsVCL(uint8_t nalType)
{
    return nalType >= H265NalType::HEVC_TRAIL_N && nalType <= H265NalType::HEVC_CRA_NUT;
}

bool StartCodeDetectorH265::IsIDR(uint8_t nalType)
{
    return nalType == H265NalType::HEVC_IDR_W_RADL ||
           nalType == H265NalType::HEVC_IDR_N_LP ||
           nalType == H265NalType::HEVC_CRA_NUT;
}
