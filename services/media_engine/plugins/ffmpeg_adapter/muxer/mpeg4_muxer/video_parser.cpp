/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "video_parser.h"
#include <algorithm>
#include <vector>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {

namespace {
constexpr uint8_t EMULATION_CODE[] = {0x00, 0x00, 0x03};
constexpr uint32_t SHORT_STARTCODELEN = 3;
constexpr uint32_t LONG_STARTCODELEN = 4;
constexpr uint32_t EDGE_PROTECT_THREE = 3;
constexpr uint32_t EDGE_PROTECT_FOUR = 4;
constexpr uint32_t MAIN_MATCH_STRIDE = 4;
constexpr uint32_t NAL_START_PATTERN = 0x01000100;
constexpr uint32_t BITWISE_NOT_NAL_START_PATTERN = ~0x01000100;
constexpr uint32_t NAL_MATCH_MASK = 0x80008000U;
}

VideoParser::VideoParser(VideoParserStreamType streamType) : streamType_(streamType) {}

int32_t VideoParser::WriteFrame(const std::shared_ptr<AVIOStream> &io, const std::shared_ptr<AVBuffer> &sample)
{
    io->Write(sample->memory_->GetAddr(), sample->memory_->GetSize());
    return sample->memory_->GetSize();
}

int32_t VideoParser::WriteSample(const std::shared_ptr<AVIOStream> &io, const uint8_t *sample, int32_t size)
{
    io->Write(static_cast<uint32_t>(size));
    io->Write(sample, size);
    return size + sizeof(size);
}

uint8_t *VideoParser::FastFindNalStartCode(const uint8_t *p, const uint8_t *buf, int32_t &startCodeLen)
{
    if (p[1] == 0) {
        if (p[0] == 0 && p[2] == 1) { // 2 is the second byte index
            startCodeLen = (p > buf && *(p - 1) == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN;
            return const_cast<uint8_t*>(p - (startCodeLen - SHORT_STARTCODELEN));
        }
        if (p[2] == 0 && p[3] == 1) { // 2， 3 is byte index
            startCodeLen = (p[0] == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN;
            return const_cast<uint8_t*>(p + 1 - (startCodeLen - SHORT_STARTCODELEN));
        }
    }
    if (p[3] == 0) { // 3 is the third byte index
        if (p[2] == 0 && p[4] == 1) { // 2， 4 is byte index
            startCodeLen = (p[1] == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN;
            return const_cast<uint8_t*>(p + 2 - (startCodeLen - SHORT_STARTCODELEN));
        }
        if (p[4] == 0 && p[5] == 1) { // 4， 5 is byte index
            startCodeLen = (p[2] == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN; // 2 is the second byte index
            return const_cast<uint8_t*>(p + 3 - (startCodeLen - SHORT_STARTCODELEN));
        }
    }
    return nullptr;
}

uint8_t *VideoParser::FindNalStartCode(const uint8_t *buf, const uint8_t *end, int32_t &startCodeLen)
{
    if (end - buf < EDGE_PROTECT_THREE) {
        startCodeLen = SHORT_STARTCODELEN;
        return const_cast<uint8_t*>(end);
    }
    const uint8_t *p = buf;
    // 0x3: fetch the last 2 bits
    const uint8_t *alignedP = p + (EDGE_PROTECT_FOUR - (reinterpret_cast<uintptr_t>(p) & 0x3));
    for (end -= EDGE_PROTECT_THREE; p < alignedP && p < end; ++p) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1) { // 2 is the second byte index
            startCodeLen = (p > buf && *(p - 1) == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN;
            return const_cast<uint8_t*>(p - (startCodeLen - SHORT_STARTCODELEN));
        }
    }
    for (end -= EDGE_PROTECT_THREE; p < end; p += MAIN_MATCH_STRIDE) {
        uint32_t x = *static_cast<const uint32_t*>(static_cast<const void*>(p));
        uint32_t y = x < NAL_START_PATTERN ? (x + (BITWISE_NOT_NAL_START_PATTERN + 1)) : (x - NAL_START_PATTERN);
        if (y & (~x) & NAL_MATCH_MASK == 0) {
            continue;
        }
        auto ptr = FastFindNalStartCode(p, buf, startCodeLen);
        if (ptr != nullptr) {
            return ptr;
        }
    }
    for (end += EDGE_PROTECT_THREE; p < end; ++p) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1) { // 2 is the second byte index
            startCodeLen = (p > buf && *(p - 1) == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN;
            return const_cast<uint8_t*>(p - (startCodeLen - SHORT_STARTCODELEN));
        }
    }
    startCodeLen = SHORT_STARTCODELEN;
    return const_cast<uint8_t*>(end + SHORT_STARTCODELEN); // reset and return original end
}

uint8_t VideoParser::GetNalType(uint8_t nalHeader)
{
    switch (streamType_) {
        case VideoParserStreamType::H264: {
            return nalHeader & 0x1F;
        }
        case VideoParserStreamType::H265: {
            return (nalHeader & 0x7E) >> 1;
        }
        default: {
            return 0;
        }
    }
}

bool VideoParser::IsAvccHvccFrame(const uint8_t *sample, int32_t size)
{
    if (size < nalSizeLen_ || sample == nullptr) {
        return false;
    }

    uint32_t naluSize = 0;
    uint64_t pos = 0;
    uint64_t cmpSize = static_cast<uint64_t>(size);
    while (pos + nalSizeLen_ <= cmpSize) {
        naluSize = 0;
        for (uint64_t i = pos; i < pos + nalSizeLen_; i++) {
            naluSize = (naluSize << 8) | sample[i];  // 8
        }

        if (naluSize <= 1) {
            return false;
        }
        pos += (naluSize + nalSizeLen_);
    }
    return pos == cmpSize;
}

bool VideoParser::IsAnnexbFrame(const uint8_t* sample, int32_t size)
{
    if (size < nalSizeLen_ || sample == nullptr) {
        return false;
    }
    if (sample[0] == 0 && sample[1] == 0 &&
        (sample[2] == 1 || (sample[2] == 0 && sample[3] == 1))) {  // index 2, index 3
        return true;
    }
    return false;
}

std::shared_ptr<RbspContext> VideoParser::ParseRbsp(const uint8_t *buf, int32_t size)
{
    std::vector<uint8_t> rbspBuf;
    const uint8_t *start = buf;
    const uint8_t *end = buf + size;
    while (start < end) {
        auto iter = std::search(start, end, EMULATION_CODE, EMULATION_CODE + sizeof(EMULATION_CODE));
        if (iter != end) {
            iter = iter + sizeof(EMULATION_CODE);
            rbspBuf.insert(rbspBuf.end(), start, iter - 1);
        } else {
            rbspBuf.insert(rbspBuf.end(), start, iter);
        }

        start = iter;
    }

    std::shared_ptr<RbspContext> rbspContext = std::make_shared<RbspContext>();
    rbspContext->InitRbsp(rbspBuf);
    return rbspContext;
}

RbspContext::~RbspContext()
{
    buf_.clear();
}

void RbspContext::InitRbsp(const std::vector<uint8_t> &buf)
{
    buf_.assign(buf.begin(), buf.end());
    size_ = static_cast<int32_t>(buf_.size());
    byteIndex_ = 0;
    bitIndex_ = 0;
}

bool RbspContext::RbspCheckSize(int32_t size)
{
    int32_t byteIndex = byteIndex_ + (bitIndex_ + size) / 8;  // 8 bit
    if (byteIndex > size_) {
        return false;
    }
    return true;
}

void RbspContext::RbspSkipBits(int32_t size)
{
    if (byteIndex_ > size_) {
        return;
    }

    byteIndex_ = byteIndex_ + (bitIndex_ + size) / 8;  // 8 bit
    bitIndex_ = (bitIndex_ + size) % 8;  // 8 bit
}

uint8_t RbspContext::RbspGetBit()
{
    uint8_t *buf = buf_.data();
    uint8_t data = *(reinterpret_cast<uint8_t *>(buf + byteIndex_));
    data = (data >> (7 - bitIndex_)) & 0x01;  // 7
    return data;
}

uint32_t RbspContext::RbspGetUeGolomb()
{
    int32_t size = 0;
    while (size < 32 && RbspGetBit() == 0) {  // 32
        ++size;
        RbspSkipBits(1);
    }
    size == 32 ? --size : size;  // 32
    uint32_t data = RbspGetBits<uint32_t, uint64_t>(size + 1) - 1;
    return data;
}

int32_t RbspContext::RbspGetSeGolomb()
{
    uint32_t ueGolomb = RbspGetUeGolomb();
    // (-1)的codeNum + 1次方
    int32_t sign = ueGolomb & 0x01;
    // Ceil(k÷2)，Ceil为向上取整，k为codeNum的值
    int32_t seGolomb = (ueGolomb + 1) >> 1;
    if (!sign) {
        // (−1)k+1 Ceil(k÷2)
        seGolomb = -seGolomb;
    }
    return seGolomb;
}
} // namespace Mpeg4
} // namespace Plugins
} // namespace Media
} // namespace OHOS