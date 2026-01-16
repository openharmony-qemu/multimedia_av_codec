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

#ifndef VIDEO_PARSER_H
#define VIDEO_PARSER_H

#include <iostream>
#include <memory>
#include <vector>
#include "avcodec_common.h"
#include "avio_stream.h"
#include "meta/any.h"
#include "common/log.h"
#include "stream_parser_manager.h"
#include "basic_box.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class RbspContext;
class VideoParser {
public:
    enum VideoParserStreamType {
        MPEG4,
        H264,
        H265
    };

    explicit VideoParser(VideoParserStreamType streamType);
    virtual ~VideoParser() = default;
    virtual int32_t WriteFrame(const std::shared_ptr<AVIOStream> &io, const std::shared_ptr<AVBuffer> &sample);
    virtual Any GetPointer() {return Any();}
    virtual int32_t SetConfig(const std::shared_ptr<BasicBox> &box, std::vector<uint8_t> &codecConfig) {return 0;};

public:
    template <class T>
    static T* GetVideoParserPtr(std::shared_ptr<VideoParser> &parser)
    {
        static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "VideoParser" };
        FALSE_RETURN_V_MSG_W(parser != nullptr, nullptr, "GetVideoParserPtr parser is nullptr!");
        auto anyPtr = parser->GetPointer();
        FALSE_RETURN_V_MSG_W(Any::IsSameTypeWith<T*>(anyPtr), nullptr,
            "GetVideoParserPtr type err! type:%{public}s", anyPtr.TypeName().data());
        return AnyCast<T*>(anyPtr);
    }

protected:
    int32_t WriteSample(const std::shared_ptr<AVIOStream> &io, const uint8_t *sample, int32_t size);
    uint8_t GetNalType(uint8_t nalHeader);
    bool IsAvccHvccFrame(const uint8_t *sample, int32_t size);
    bool IsAnnexbFrame(const uint8_t* sample, int32_t size);
    std::shared_ptr<RbspContext> ParseRbsp(const uint8_t *buf, int32_t size);
    uint32_t nalSizeLen_ = 0x04;

private:
    VideoParserStreamType streamType_;
};

class RbspContext {
public:
    RbspContext() = default;
    ~RbspContext();
    void InitRbsp(const std::vector<uint8_t> &buf);
    bool RbspCheckSize(int32_t size);
    void RbspSkipBits(int32_t size);
    uint32_t RbspGetUeGolomb();
    int32_t RbspGetSeGolomb();

    template <typename T = uint8_t, typename U = uint32_t>
    T RbspGetBits(int32_t size)
    {
        if (!RbspCheckSize(size)) {
            RbspSkipBits(size);
            return 0;
        }

        uint8_t *buf = buf_.data();
        U temp = *(reinterpret_cast<U *>(buf + byteIndex_));

        int32_t dataSize = sizeof(temp);
        U reverseData = 0;
        for (int32_t i = 0; i < dataSize; ++i) {
            reverseData = (reverseData << 8) | (temp & 0xFF);  // 8
            temp = temp >> 8;  // 8
        }

        T data = static_cast<T>((reverseData << bitIndex_) >> (8 * dataSize - size));
        RbspSkipBits(size);
        return data;
    }

private:
    uint8_t RbspGetBit();

private:
    std::vector<uint8_t> buf_;
    int32_t size_;
    int32_t byteIndex_;
    int32_t bitIndex_;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif