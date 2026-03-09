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

#include "avc_parser.h"
#include <set>
#include "mpeg4_utils.h"

#ifndef _WIN32
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "AvcParser" };
}
#endif // !_WIN32

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
AvcParser::AvcParser() : VideoParser(VideoParserStreamType::H264)
{}

int32_t AvcParser::SetConfig(const std::shared_ptr<BasicBox> &box, std::vector<uint8_t> &codecConfig)
{
    FALSE_RETURN_V_MSG_E(box != nullptr, -1, "AvcParser::Init avcc box is null");
    avccBasicBox_ = box;
    avccBox_ = BasicBox::GetBoxPtr<AvccBox>(avccBasicBox_);
    UpdateNeedParser();
    const int32_t h264NalSizeIndex = 4;
    const size_t h264CodecConfigSize = 6;
    if (codecConfig.size() > h264CodecConfigSize) {
        nalSizeLen_ = static_cast<uint32_t>(codecConfig[h264NalSizeIndex] & 0x03) + 1;
    }
    MEDIA_LOG_I("H264 SetConfig nalSize:%{public}u, codec config size:%{public}zu", nalSizeLen_, codecConfig.size());
    return 0;
}

int32_t AvcParser::WriteFrame(const std::shared_ptr<AVIOStream> &io, const std::shared_ptr<AVBuffer> &sample)
{
    if (!IsAvccHvccFrame(sample->memory_->GetAddr(), sample->memory_->GetSize()) &&
        IsAnnexbFrame(sample->memory_->GetAddr(), sample->memory_->GetSize())) {
        return WriteAnnexBFrame(io, sample);
    }
    io->Write(sample->memory_->GetAddr(), sample->memory_->GetSize());
    return sample->memory_->GetSize();
}

int32_t AvcParser::WriteAnnexBFrame(const std::shared_ptr<AVIOStream> &io, const std::shared_ptr<AVBuffer> &sample)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr && sample->memory_->GetAddr() != nullptr,
        -1, "WriteAnnexBFrame error parameter");
    const uint8_t* nalStart = sample->memory_->GetAddr();
    const uint8_t* end = nalStart + sample->memory_->GetSize();
    const uint8_t* nalEnd = nullptr;
    int32_t startCodeLen = 0;
    int32_t writeSize = 0;
    nalStart = FindNalStartCode(nalStart, end, startCodeLen);
    nalStart = nalStart + startCodeLen;
    while (nalStart < end) {
        nalEnd = FindNalStartCode(nalStart, end, startCodeLen);
        int32_t naluSize = static_cast<int32_t>(nalEnd - nalStart);
        if (!isFirstFrame_ || (sample->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) != 0) {
            writeSize += WriteSample(io, nalStart, naluSize);
        }
        AvcNalType nalType = static_cast<AvcNalType>(GetNalType(nalStart[0]));
        if (needParse_.find(nalType) != needParse_.end() && needParse_[nalType].first) {
            (this->*needParse_[nalType].second)(nalStart, naluSize);
        }
        nalStart = nalEnd + startCodeLen;
    }
    UpdateNeedParser();
    if (isFirstFrame_) {
        isFirstFrame_ = false;
        if ((sample->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) == 0) {
            MEDIA_LOG_I("H264 first frame is not sync frame, return");
        }
    }
    return writeSize;
}

int32_t AvcParser::ParseSps(const uint8_t* sample, int32_t size)
{
    constexpr int32_t index2 = 2;
    constexpr int32_t index3 = 3;
    if (size < 4 || avccBox_ == nullptr) {  // min size 4
        return 0;
    }

    if (spsCount_ >= 0x1f || size > UINT16_MAX) {
        return 0;
    }

    if (avccBox_->sps_.size() == 0) {
        auto rbspContext = ParseRbsp(sample, size);
        ParseSpsInfo(rbspContext);
    } else if (avccBox_->profileIdc_ != sample[1] ||
        avccBox_->profileCompat_ != sample[index2] ||
        avccBox_->levelIdc_ != sample[index3]) {
        MEDIA_LOG_E("Inconsistent profile/level!");
        return 0;
    }

    std::vector<uint8_t> sps(0x02, 0);
    sps[1] = static_cast<uint8_t>(static_cast<uint32_t>(size) & 0xff);
    sps[0] = static_cast<uint8_t>((static_cast<uint32_t>(size) >> 0x08) & 0xff);
    sps.insert(sps.end(), sample, sample + size);
    avccBox_->sps_.emplace_back(sps);
    ++spsCount_;
    avccBox_->spsCount_ = (0xe0 | spsCount_);

    return 0;
}

int32_t AvcParser::ParsePps(const uint8_t* sample, int32_t size)
{
    if (size < 4 || avccBox_ == nullptr) {  // 4
        return 0;
    }

    if (avccBox_->ppsCount_ == 0xff || size > UINT16_MAX) {
        return 0;
    }

    std::vector<uint8_t> pps(2, 0);  // 2
    pps[1] = static_cast<uint8_t>(static_cast<uint32_t>(size) & 0xff);
    pps[0] = static_cast<uint8_t>((static_cast<uint32_t>(size) >> 0x08) & 0xff);
    pps.insert(pps.end(), sample, sample + size);
    avccBox_->pps_.emplace_back(pps);
    ++avccBox_->ppsCount_;

    return 0;
}

int32_t AvcParser::ParseSpsExt(const uint8_t* sample, int32_t size)
{
    if (size < 4 || avccBox_ == nullptr) {  // 4
        return 0;
    }

    if (avccBox_->spsExtCount_ == 0xff) {
        return 0;
    }

    std::vector<uint8_t> spsExt(2, 0);  // 2
    spsExt[1] = static_cast<uint8_t>(static_cast<uint32_t>(size) & 0xff);
    spsExt[0] = static_cast<uint8_t>((static_cast<uint32_t>(size) >> 0x08) & 0xff);
    spsExt.insert(spsExt.end(), sample, sample + size);
    avccBox_->spsExt_.emplace_back(spsExt);
    ++avccBox_->spsExtCount_;

    return 0;
}

int32_t AvcParser::ParseSpsInfo(const std::shared_ptr<RbspContext>& rbspContext)
{
    std::set<uint8_t> profile = { 44, 83, 86, 100, 110, 118, 122, 128, 134, 138, 139, 244 };
    rbspContext->RbspSkipBits(8); // Nal Header u(16) 8 b
    avccBox_->profileIdc_ = rbspContext->RbspGetBits(8);  // 8
    avccBox_->profileCompat_ = rbspContext->RbspGetBits(8);  // 8
    avccBox_->levelIdc_ = rbspContext->RbspGetBits(8);  // 8
    rbspContext->RbspGetUeGolomb(); // id

    if (profile.count(avccBox_->profileIdc_) > 0) {
        avccBox_->chromaFormat_ = static_cast<uint8_t>(rbspContext->RbspGetUeGolomb()) | 0xFC;
        if (avccBox_->chromaFormat_ == 0xFF) {
            rbspContext->RbspSkipBits(1); // color transform flag
        }
        avccBox_->bitDepthLuma_ = static_cast<uint8_t>(rbspContext->RbspGetUeGolomb()) | 0xF8;
        avccBox_->bitDepthChroma_ = static_cast<uint8_t>(rbspContext->RbspGetUeGolomb()) | 0xF8;
        rbspContext->RbspSkipBits(1); // transform by pass flag
    } else {
        avccBox_->chromaFormat_ = 0xFD;
        avccBox_->bitDepthLuma_ = 0xF8;
        avccBox_->bitDepthLuma_ = 0xF8;
    }

    return 0;
}

void AvcParser::UpdateNeedParser()
{
    if (needParse_[AVC_SPS_NAL_UNIT].first &&
        avccBox_ != nullptr &&
        (avccBox_->spsCount_ & 0x1F) > 0) {
        needParse_[AVC_SPS_NAL_UNIT].first = false;
    }

    if (needParse_[AVC_PPS_NAL_UNIT].first &&
        avccBox_ != nullptr &&
        avccBox_->ppsCount_ > 0) {
        needParse_[AVC_PPS_NAL_UNIT].first = false;
    }

    if (needParse_[AVC_SPS_EXT_NAL_UNIT].first &&
        avccBox_ != nullptr && avccBox_->spsExtCount_ > 0) {
        needParse_[AVC_SPS_EXT_NAL_UNIT].first = false;
    }
}
} // Mpeg4
} // Plugins
} // Media
} // OHOS