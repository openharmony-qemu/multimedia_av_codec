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
#include <fstream>
#include <memory>
#include <algorithm>
#include "hcodec_log.h"
using namespace std;

bool HCodecDemuxer::LoadNaluListFromPath(const std::string &path, CodeType type)
{
    mNalCursor = 0;
    mNals.clear();
    mXpsIndex.clear();
    mIdrIndex.clear();

    ifstream ifs(path, ios::binary);
    if (!ifs.is_open()) {
        return false;
    }
    size_t fileSize = GetFileSizeInBytes(ifs);
    static constexpr size_t chunkSize = 10 * 1024 * 1024; // read 10MB per time
    list<pair<size_t, FirstByteInNalu>> posOfFile;
    while (true) {
        size_t currChunkPos = ifs.tellg();
        unique_ptr<uint8_t[]> buf = make_unique<uint8_t[]>(chunkSize);
        ifs.read(reinterpret_cast<char *>(buf.get()), chunkSize);
        size_t currChunkSize = ifs.gcount();
        if (currChunkSize == 0) {
            break;
        }
        size_t numOfBytesShouldRoleBack;
        list<pair<size_t, FirstByteInNalu>> posOfChunk = FindAllNalInfo(buf.get(), currChunkSize,
                                                                        &numOfBytesShouldRoleBack);
        for (auto pair: posOfChunk) {
            posOfFile.emplace_back(pair.first + currChunkPos, pair.second);
        }
        ifs.seekg(static_cast<size_t>(ifs.tellg()) - numOfBytesShouldRoleBack);
    }

    for (auto it = posOfFile.begin(); it != posOfFile.end(); ++it) {
        auto nex = next(it);
        NALUInfo info {
            .offset = it->first,
            .size = (nex == posOfFile.end()) ? (fileSize - it->first) : (nex->first - it->first),
            .nalType = GetNalType(it->second, type),
        };

        if (IsXpsStart(info, type)) {
            mXpsIndex.push_back(mNals.size());
        } else if (IsIdr(info, type)) {
            mIdrIndex.push_back(mNals.size());
        }
        mNals.push_back(info);
    }
    LOGI("nals:(%{public}zu) xps:(%{public}zu) idr:(%{public}zu)", mNals.size(), mXpsIndex.size(), mIdrIndex.size());
    return true;
}

NaluUnit HCodecDemuxer::GetNext(CodeType type, int offset)
{
    if (offset >= 0) {
        mNalCursor = static_cast<size_t>(offset);
        LOGI("jump to %{public}zu", mNalCursor);
    }
    NaluUnit unit = {
        .startPos = 0,
        .endPos = 0,
        .isCsd = false,
        .isEos = false
    };
    if (mNalCursor >= mNals.size()) {
        LOGI("no more nal, send input eos");
        unit.isEos = true;
        return unit;
    }
    const NALUInfo &nal = mNals[mNalCursor++];
    unit.startPos = nal.offset;
    unit.endPos = nal.offset + nal.size;
    if (IsCsd(nal, type)) {
        unit.isCsd = true;
        while (mNalCursor < mNals.size()) {
            const NALUInfo &one = mNals[mNalCursor++];
            if (IsCsd(one, type)) {
                unit.endPos = one.offset + one.size;
            } else {
                break;
            }
        }
    }
    LOGD("current nalu: %{public}zu, nals remain: %{public}zu", mNalCursor, mNals.size() - mNalCursor);
    return unit;
}

std::optional<PositionPair> HCodecDemuxer::FindReferenceInfo(size_t seekPos)
{
    auto idrIter = find_if(mIdrIndex.rbegin(), mIdrIndex.rend(), [seekPos](size_t pos) -> bool {
        return pos <= seekPos;
    });
    if (idrIter == mIdrIndex.rend()) {
        LOGE("Failed to find the first idr before seekPos");
        return std::nullopt;
    }
    auto xpsIter = find_if(mXpsIndex.rbegin(), mXpsIndex.rend(), [idrIter](size_t pos) -> bool {
        return pos < *idrIter;
    });
    if (xpsIter == mXpsIndex.rend()) {
        LOGE("Failed to find the first xps before seekToPos");
        return std::nullopt;
    }
    LOGE("xpsPos=%{public}zu idrPos=%{public}zu", *xpsIter, *idrIter);
    return std::make_pair(*xpsIter, *idrIter);
}

list<pair<size_t, FirstByteInNalu>> HCodecDemuxer::FindAllNalInfo(const uint8_t *pStart, size_t length,
                                                                  size_t *numOfBytesShouldRoleBack)
{
    *numOfBytesShouldRoleBack = START_CODE_LEN - 1;
    list<pair<size_t, FirstByteInNalu>> res;
    size_t pos = 0;
    while (pos < length) {
        auto pFound = search(pStart + pos, pStart + length, begin(START_CODE), end(START_CODE));
        pos = distance(pStart, pFound);
        if (pos == length) { // 没找到
            break;
        }
        if (pos + START_CODE_LEN == length) {  // 0x001正好在这段buffer的末尾，拿不到nalu的第一个字节，所以先不管这个起始码
            *numOfBytesShouldRoleBack = START_CODE_LEN;
            break;
        }
        res.emplace_back(pos, pStart[pos + START_CODE_LEN]);
        pos += START_CODE_LEN;
    }
    return res;
}

size_t HCodecDemuxer::GetFileSizeInBytes(ifstream &ifs)
{
    ifs.seekg(0, ifstream::end);
    auto len = ifs.tellg();
    ifs.seekg(0, ifstream::beg);
    return static_cast<size_t>(len);
}

uint8_t HCodecDemuxer::GetNalType(uint8_t byte, CodeType type)
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

bool HCodecDemuxer::IsXpsStart(const NALUInfo &nalu, CodeType type)
{
    uint8_t nalType = nalu.nalType;
    switch (type) {
        case H264:
            return nalType == static_cast<uint8_t>(H264NalType::SPS);
        case H265:
            return nalType == static_cast<uint8_t>(H265NalType::HEVC_VPS_NUT);
        default:
            return false;
    }
}

bool HCodecDemuxer::IsCsd(const NALUInfo &nalu, CodeType type)
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

bool HCodecDemuxer::IsIdr(const NALUInfo &nalu, CodeType type)
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