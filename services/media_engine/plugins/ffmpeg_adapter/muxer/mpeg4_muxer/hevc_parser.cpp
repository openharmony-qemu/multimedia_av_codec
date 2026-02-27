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

#include "hevc_parser.h"
#include <algorithm>
#include <set>
#include "mpeg4_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "HevcParser" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
using namespace OHOS::Media::Plugins;
HevcParser::HevcParser() : VideoParser(VideoParserStreamType::H265)
{}

int32_t HevcParser::Init()
{
    parser_ = StreamParserManager::Create(VideoStreamType::HEVC);
    FALSE_RETURN_V_MSG_E(parser_ != nullptr, -1, "HevcParser::Create hvcc parser is failed");
    return 0;
}

int32_t HevcParser::SetConfig(const std::shared_ptr<BasicBox> &box, std::vector<uint8_t> &codecConfig)
{
    FALSE_RETURN_V_MSG_E(box != nullptr, -1, "HevcParser::Init hvcc box is null");
    hvccBasicBox_ = box;
    hvccBox_ = BasicBox::GetBoxPtr<HvccBox>(hvccBasicBox_);
    const int32_t hevcNalSizeIndex = 21;
    const size_t hevcCodecConfigSize = 23;
    if (codecConfig.size() > hevcCodecConfigSize) {
        nalSizeLen_ = static_cast<uint32_t>(codecConfig[hevcNalSizeIndex] & 0x03) + 1;
    }
    MEDIA_LOG_I("Hevc SetConfig nalSize:%{public}u, codec config size:%{public}zu", nalSizeLen_, codecConfig.size());
    return 0;
}

int32_t HevcParser::WriteFrame(const std::shared_ptr<AVIOStream> &io, const std::shared_ptr<AVBuffer> &sample)
{
    FALSE_RETURN_V_MSG_E(sample->memory_->GetAddr() != nullptr, -1, "sample is null");
    auto buffer = sample->memory_->GetAddr();
    auto size = sample->memory_->GetSize();
    if (isFirstFrame_) {
        isFirstFrame_ = false;
        FALSE_RETURN_V_MSG_E(parser_ != nullptr, -1, "HevcParser::WriteFrame hvcc parser is null");
        FALSE_RETURN_V_MSG_E(hvccBox_ != nullptr, -1, "HevcParser::WriteFrame hvcc box is null");
        if (!IsAvccHvccFrame(buffer, size) && IsAnnexbFrame(buffer, size)) {
            FALSE_RETURN_V_MSG_E(sample->flag_ & static_cast<uint32_t>(AVBufferFlag::CODEC_DATA),
                -1, "first frame flag of annex-b stream need AVCODEC_BUFFER_FLAGS_CODEC_DATA!");
            isAnnexbFrame_ = true;
            uint8_t *extra = nullptr;
            int32_t extraSize = 0;
            parser_->ParseExtraData(buffer, size, &extra, &extraSize);
            if (extraSize > 0) {
                isParserColor_ = true;
                hvccBox_->data.clear();
                hvccBox_->data.assign(extra, extra + extraSize);
            }
            MEDIA_LOG_I("avmuxer h265 first frame size:%{public}d, extra data size:%{public}d, box data"
                " size:%{public}zu", size, extraSize, hvccBox_->data.size());
            if (!(sample->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME))) {
                MEDIA_LOG_I("HevcParser::WriteFrame first frame is not sync frame, return");
                return 0;
            }
        } else if (hvccBox_->naluTypeCount_ < 1) {
            MEDIA_LOG_W("codec config not set or err! non annexb frame, must set codec config");
        }
    }
    /* HDR 10bit encode skip first FD NAL */
    if (size > 0) {
        int32_t offset = parser_->GetFirstFillerDataNalSize(buffer, size);
        MEDIA_LOG_D("skip fd nal, offset:%{public}d, frame size:%{public}d", offset, size);
        if (offset > 0 && offset < size) {
            buffer += offset;
            size -= offset;
        }
    }
    if (isAnnexbFrame_) {
        return WriteAnnexBFrame(io, buffer, size);
    }
    io->Write(buffer, size);
    return size;
}

int32_t HevcParser::WriteAnnexBFrame(const std::shared_ptr<AVIOStream> &io, const uint8_t* sample, int32_t size)
{
    uint8_t *nalStart = const_cast<uint8_t *>(sample);
    uint8_t *end = nalStart + size;
    uint8_t *nalEnd = nullptr;
    int32_t startCodeLen = 0;
    int32_t writeSize = 0;
    nalStart = FindNalStartCode(nalStart, end, startCodeLen);
    nalStart = nalStart + startCodeLen;
    while (nalStart < end) {
        nalEnd = FindNalStartCode(nalStart, end, startCodeLen);
        int32_t naluSize = static_cast<int32_t>(nalEnd - nalStart);
        writeSize += WriteSample(io, nalStart, naluSize);
        nalStart = nalEnd + startCodeLen;
    }
    return writeSize;
}

bool HevcParser::IsParserColor()
{
    return isParserColor_;
}

uint8_t HevcParser::GetColorPrimary()
{
    if (parser_) {
        return parser_->GetColorPrimaries();
    }
    return 0x02;
}

uint8_t HevcParser::GetColorTransfer()
{
    if (parser_) {
        return parser_->GetColorTransfer();
    }
    return 0x02;
}

uint8_t HevcParser::GetColorMatrixCoeff()
{
    if (parser_) {
        return parser_->GetColorMatrixCoeff();
    }
    return 0x02;
}

bool HevcParser::GetColorRange()
{
    if (parser_) {
        return parser_->GetColorRange();
    }
    return false;
}

bool HevcParser::IsHdrVivid()
{
    if (parser_) {
        return parser_->IsHdrVivid();
    }
    return false;
}

std::vector<uint8_t> HevcParser::GetLogInfo()
{
    if (parser_) {
        return parser_->GetLogInfo();
    }
    return std::vector<uint8_t>();
}
} // namespace Mpeg4
} // namespace Plugins
} // namespace Media
} // namespace OHOS