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
    if (size < static_cast<int32_t>(nalSizeLen_) || sample == nullptr) {
        return false;
    }

    uint64_t naluSize = 0;
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
    if (size < static_cast<int32_t>(nalSizeLen_) || sample == nullptr) {
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
    uint32_t data = RbspGetBits<uint32_t, uint64_t>(size + 1);
    return data > 0 ?  data - 1 : 0;
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