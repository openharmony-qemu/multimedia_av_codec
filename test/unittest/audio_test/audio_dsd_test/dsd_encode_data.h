/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DSD_ENCODE_DATA_H
#define DSD_ENCODE_DATA_H

#include <cstdint>
#include <vector>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace DsdTest {
// A minimal deterministic DSD "encoder" for unit tests.
//
// A DSD stream is a plain sequence of 1-bit samples packed 8-per-byte, without any
// bitstream syntax, so encoding a known signal is just writing the right byte pattern:
//   0x00 -> sustained 0 bits -> decodes to full-scale negative DC (-1.0)
//   0xFF -> sustained 1 bits -> decodes to full-scale positive DC (+1.0)
//   0x69 -> the DSD silence marker (see DSD_SILENCE in libavcodec/dsddec.c)
// The stream is LSB-first and channel-planar (the DSF container layout), which matches
// the default OH_MD_KEY_AUDIO_DSD_CODEC_TYPE = DSD_CODEC_LSBF_PLANAR.

constexpr uint32_t DSD_SEGMENT_SIZE = 512;  // bytes per segment
constexpr uint32_t DSD_SEGMENT_COUNT = 4;   // silence | negative DC | positive DC | square wave
constexpr uint32_t DSD_TOTAL_SIZE = DSD_SEGMENT_SIZE * DSD_SEGMENT_COUNT;

inline std::vector<uint8_t> EncodeFlat(uint8_t value, uint32_t byteCount)
{
    return std::vector<uint8_t>(byteCount, value);
}

inline uint8_t ReverseBits(uint8_t value)
{
    uint8_t result = 0;
    for (int i = 0; i < 8; i++) {  // 8
        result = static_cast<uint8_t>((result << 1) | ((value >> i) & 1));
    }
    return result;
}

// Bit-reverse every byte. Decoding the reversed stream with the MSBF decoder produces
// exactly the same output as decoding the original stream with the LSBF decoder, because
// ctables_lsbf[i][w] == ctables_msbf[i][reverse(w)] (see libavcodec/dsd.c table init).
inline std::vector<uint8_t> BitReverseBytes(const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> out(data.size());
    for (size_t i = 0; i < data.size(); i++) {
        out[i] = ReverseBits(data[i]);
    }
    return out;
}

inline std::vector<uint8_t> EncodePattern(const std::vector<uint8_t> &pattern, uint32_t repeatCount)
{
    std::vector<uint8_t> data;
    data.reserve(pattern.size() * repeatCount);
    for (uint32_t i = 0; i < repeatCount; i++) {
        data.insert(data.end(), pattern.begin(), pattern.end());
    }
    return data;
}

// Interleave two mono streams sample-wise: a0, b0, a1, b1, ... (raw DSD interleaved layout).
inline std::vector<uint8_t> InterleaveChannels(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b)
{
    std::vector<uint8_t> data;
    data.reserve(a.size() + b.size());
    for (size_t i = 0; i < a.size(); i++) {
        data.push_back(a[i]);
        data.push_back(b[i]);
    }
    return data;
}

// Concatenate two mono streams channel-wise: [a plane][b plane] (raw DSD planar layout).
inline std::vector<uint8_t> MakePlanar(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b)
{
    std::vector<uint8_t> data = a;
    data.insert(data.end(), b.begin(), b.end());
    return data;
}

// Stream with rising/falling edges carried by non-palindromic bytes (0x0F/0xF0), so the
// LSBF and MSBF interpretations of the same bytes differ.
inline std::vector<uint8_t> BuildBitOrderStream()
{
    std::vector<uint8_t> stream;
    for (int i = 0; i < 8; i++) {  // 8
        auto rising = EncodeFlat(0x00, 128);  // 128
        rising.push_back(0x0F);
        auto falling = EncodeFlat(0xFF, 128);  // 128
        falling.push_back(0xF0);
        stream.insert(stream.end(), rising.begin(), rising.end());
        stream.insert(stream.end(), falling.begin(), falling.end());
    }
    return stream;
}

// Alternating full-scale square wave: halfPeriodBytes of 0xFF followed by 0x00, repeated.
inline std::vector<uint8_t> EncodeSquare(uint32_t halfPeriodBytes, uint32_t halfPeriodCount)
{
    std::vector<uint8_t> data;
    data.reserve(halfPeriodBytes * halfPeriodCount);
    for (uint32_t i = 0; i < halfPeriodCount; i++) {
        uint8_t value = (i % 2 == 0) ? 0xFF : 0x00;
        data.insert(data.end(), halfPeriodBytes, value);
    }
    return data;
}

// Fixed test stream: [0] silence, [1] negative DC, [2] positive DC, [3] square wave
// (32-byte half period). One DSD byte per channel decodes to exactly one PCM sample.
inline std::vector<uint8_t> BuildTestStream()
{
    std::vector<uint8_t> stream;
    stream.reserve(DSD_TOTAL_SIZE);
    auto append = [&stream](const std::vector<uint8_t> &segment) {
        stream.insert(stream.end(), segment.begin(), segment.end());
    };
    append(EncodeFlat(0x69, DSD_SEGMENT_SIZE));
    append(EncodeFlat(0x00, DSD_SEGMENT_SIZE));
    append(EncodeFlat(0xFF, DSD_SEGMENT_SIZE));
    append(EncodeSquare(32, DSD_SEGMENT_SIZE / 32));  // 32
    return stream;
}
} // namespace DsdTest
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // DSD_ENCODE_DATA_H
