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

#include "basic_box.h"
#include <type_traits>

#ifndef _WIN32
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "Box" };
constexpr int32_t BOX_FLAGS_LEN = 3;
constexpr int32_t BOX_TYPE_LEN = 4;
constexpr uint32_t BOX_HEAD_LEN = 8;
constexpr uint32_t BOX_BASE_LEN = 12;
}
#endif // !_WIN32

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
BasicBox::BasicBox(uint32_t size, std::string type)
    : size_(size), type_(std::move(type))
{
}

uint32_t BasicBox::GetSize()
{
    if (!needWrite_) {
        MEDIA_LOG_I("get size set 0, %{public}s box", type_.c_str());
        return 0;
    }
    size_ = BOX_HEAD_LEN;  // box head size: 8 B
    AppendChildSize();
    return size_;
}

void BasicBox::SetType(std::string type)
{
    type_ = type;
}

std::string BasicBox::GetType()
{
    return type_;
}

void BasicBox::NeedWrite(bool flag)
{
    needWrite_ = flag;
}

bool BasicBox::AddChild(std::shared_ptr<BasicBox> box)
{
    if (box == nullptr) {
        MEDIA_LOG_D("%{public}s child box is nullptr!", type_.c_str());
        return false;
    }
    if (box->type_.length() >= 4 && childBoxes_.find(box->type_) == childBoxes_.end()) {  // box type min len: 4
        childBoxes_.emplace(box->type_, box);
        childTypes_.emplace_back(box->type_);
        return true;
    }
    MEDIA_LOG_E("%{public}s add child box %{public}s failed!", type_.c_str(), box->type_.c_str());
    return false;
}

std::shared_ptr<BasicBox> BasicBox::GetChild(std::string typePath)
{
    std::string type = "";
    size_t pos = typePath.find('.', 0);
    if (pos != std::string::npos) {
        type = typePath.substr(0, pos);
    } else {
        type = typePath.substr(0);
    }
    if (childBoxes_.find(type) == childBoxes_.end()) {
        MEDIA_LOG_D("get child box %{public}s fail!", type.c_str());
        return nullptr;
    }
    if (pos == std::string::npos) {
        return childBoxes_[type];
    }
    return childBoxes_[type]->GetChild(typePath.substr(pos + 1));
}

size_t BasicBox::GetChildCount()
{
    return childBoxes_.size();
}

int64_t BasicBox::Write(std::shared_ptr<AVIOStream> io)
{
    if (!needWrite_) {
        MEDIA_LOG_I("not write %{public}s box", type_.c_str());
        return io->GetPos();
    }
    int64_t pos = io->GetPos();
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);  // box type len: 4 B
    WriteChild(io);
    FALSE_RETURN_V_MSG_E(io->GetPos() - pos == size_, io->GetPos(), "%{public}s box size fail!", type_.c_str());
    return io->GetPos();
}

void BasicBox::WriteChild(std::shared_ptr<AVIOStream> io)
{
    for (auto& type : childTypes_) {
        childBoxes_[type]->Write(io);
    }
}

void BasicBox::AppendChildSize()
{
    for (auto &box : childBoxes_) {
        size_ += box.second->GetSize();
    }
}

FullBox::FullBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : BasicBox(size, std::move(type)), version_(version)
{
    flags_[0] = (flags >> 16) & 0xFF;  // 16b
    flags_[1] = (flags >> 8) & 0xFF;   // 8b
    flags_[2] = flags & 0xFF;          // index2
}

uint32_t FullBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    AppendChildSize();
    return size_;
}

int64_t FullBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);  // flags 3b
    WriteChild(io);
    return io->GetPos();
}

void FullBox::SetVersion(uint8_t version)
{
    version_ = version;
}

uint32_t AnyBox::GetSize()
{
    // size_ 不包含box 长度和type的字节数
    size_ = getSizeFunc_();
    AppendChildSize();
    return size_ + BOX_HEAD_LEN;
}

int64_t AnyBox::Write(std::shared_ptr<AVIOStream> io)
{
    int64_t pos = io->GetPos();
    uint32_t size = size_ + BOX_HEAD_LEN;
    io->Write(size);
    io->Write(type_.c_str(), BOX_TYPE_LEN);  // 4
    writeFunc_(io);
    WriteChild(io);
    FALSE_RETURN_V_MSG_E(io->GetPos() - pos == size, io->GetPos(), "%{public}s write box size err! size:"
        PUBLIC_LOG_D64 ", " PUBLIC_LOG_U32, type_.c_str(), io->GetPos() - pos, size);
    return io->GetPos();
}

MvhdBox::MvhdBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags) // 120/108
{
}

uint32_t MvhdBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    version_ = duration_ < UINT32_MAX ? 0 : 1;
    size_ += version_ == 1 ? 108 : 96;  // 108B  96B
    AppendChildSize();
    return size_;
}

int64_t MvhdBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    if (version_ == 1) {
        io->Write(creationTime_);
        io->Write(modificationTime_);
        io->Write(timeScale_);
        io->Write(duration_);
    } else {
        io->Write(static_cast<uint32_t>(creationTime_));
        io->Write(static_cast<uint32_t>(modificationTime_));
        io->Write(timeScale_);
        io->Write(static_cast<uint32_t>(duration_));
    }
    io->Write(rate_);
    io->Write(volume_);
    io->Write(reserved1_, sizeof(reserved1_));
    for (int32_t i = 0; i < 3; ++i) {  // 3
        for (int32_t j = 0; j < 3; ++j) {  // 3
            io->Write(matrix_[i][j]);
        }
    }
    io->Write(reserved2_, sizeof(reserved2_));
    io->Write(nextTrackId_);
    WriteChild(io);
    return io->GetPos();
}

TkhdBox::TkhdBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags) // 104/92
{
}

uint32_t TkhdBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    version_ = duration_ < UINT32_MAX ? 0 : 1;
    size_ += version_ == 1 ? 92 : 80;  // 92 80
    AppendChildSize();
    return size_;
}

int64_t TkhdBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    if (version_ == 1) {
        io->Write(creationTime_);
        io->Write(modificationTime_);
        io->Write(trackIndex_);
        io->Write(reserved1_);
        io->Write(duration_);
    } else {
        io->Write(static_cast<uint32_t>(creationTime_));
        io->Write(static_cast<uint32_t>(modificationTime_));
        io->Write(trackIndex_);
        io->Write(reserved1_);
        io->Write(static_cast<uint32_t>(duration_));
    }
    io->Write(reserved2_, sizeof(reserved2_));
    io->Write(layer_);
    io->Write(alternateGroup_);
    io->Write(volume_);
    io->Write(reserved3_);
    for (int32_t i = 0; i < 3; ++i) {  // 3
        for (int32_t j = 0; j < 3; ++j) {  // 3
            io->Write(matrix_[i][j]);
        }
    }
    io->Write(width_);
    io->Write(height_);
    WriteChild(io);
    return io->GetPos();
}

ElstBox::ElstBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t ElstBox::GetSize()
{
    size_ = BOX_BASE_LEN + 4;  // 4: bytes of entryCount_
    if (version_ == 1) {
        size_ += data_.size() * 20;  // 20
    } else {
        size_ += data_.size() * 12;  // 12
    }
    AppendChildSize();
    return size_;
}

int64_t ElstBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(entryCount_);
    for (auto &data : data_) {
        if (version_ == 1) {
            io->Write(data.segmentDuration_);
            io->Write(data.mediaTime_);
            io->Write(data.mediaRateInteger_);
            io->Write(data.mediaRateFraction_);
        } else {
            io->Write(static_cast<uint32_t>(data.segmentDuration_));
            io->Write(static_cast<uint32_t>(data.mediaTime_));
            io->Write(data.mediaRateInteger_);
            io->Write(data.mediaRateFraction_);
        }
    }
    WriteChild(io);
    return io->GetPos();
}

MdhdBox::MdhdBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t MdhdBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    version_ = duration_ < UINT32_MAX ? 0 : 1;
    size_ += version_ == 1 ? 32 : 20;  // 32 20
    AppendChildSize();
    return size_;
}

int64_t MdhdBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    if (version_ == 1) {
        io->Write(creationTime_);
        io->Write(modificationTime_);
        io->Write(timeScale_);
        io->Write(duration_);
    } else {
        io->Write(static_cast<uint32_t>(creationTime_));
        io->Write(static_cast<uint32_t>(modificationTime_));
        io->Write(timeScale_);
        io->Write(static_cast<uint32_t>(duration_));
    }
    io->Write(language_);
    io->Write(quality_);
    WriteChild(io);
    return io->GetPos();
}

HdlrBox::HdlrBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t HdlrBox::GetSize()
{
    size_ = BOX_BASE_LEN + 20;  // hdlr len : 20
    size_ += name_.length() + 1;
    AppendChildSize();
    return size_;
}

int64_t HdlrBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(predefined_);
    io->Write(handlerType_);
    io->Write(reserved_[0]);
    io->Write(reserved_[1]);
    io->Write(reserved_[2]);  // 2
    io->Write(name_); // 要写字符串的结束符\x00
    WriteChild(io);
    return io->GetPos();
}

VmhdBox::VmhdBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t VmhdBox::GetSize()
{
    size_ = BOX_BASE_LEN + 8;  // 8
    AppendChildSize();
    return size_;
}

int64_t VmhdBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(graphicsMode_);
    for (auto &color : opColor_) {
        io->Write(color);
    }
    WriteChild(io);
    return io->GetPos();
}

SmhdBox::SmhdBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t SmhdBox::GetSize()
{
    size_ = BOX_BASE_LEN + 4;  // 4
    AppendChildSize();
    return size_;
}

int64_t SmhdBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(balance_);
    io->Write(reserved_);
    WriteChild(io);
    return io->GetPos();
}

DrefBox::DrefBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t DrefBox::GetSize()
{
    size_ = BOX_BASE_LEN + 4;  // 4
    AppendChildSize();
    return size_;
}

int64_t DrefBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(entryCount_);
    WriteChild(io);
    return io->GetPos();
}

StsdBox::StsdBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t StsdBox::GetSize()
{
    size_ = BOX_BASE_LEN + 4;  // 4
    AppendChildSize();
    return size_;
}

int64_t StsdBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(entryCount_);
    WriteChild(io);
    return io->GetPos();
}

AudioBox::AudioBox(uint32_t size, std::string type)
    : BasicBox(size, std::move(type)) // mp4a
{
}

uint32_t AudioBox::GetSize()
{
    size_ = BOX_HEAD_LEN + 28;  // 28
    AppendChildSize();
    return size_;
}

int64_t AudioBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(reserved1_);
    io->Write(reserved2_);
    io->Write(dataRefIndex_);
    io->Write(version_);
    io->Write(revisionLevel_);
    io->Write(reserved3_);
    io->Write(channels_);
    io->Write(sampleSize_);
    io->Write(predefined_);
    io->Write(packetSize_);
    io->Write(sampleRate_);
    io->Write(reserved4_);
    WriteChild(io);
    return io->GetPos();
}

EsdsBox::EsdsBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t EsdsBox::GetSize()
{
    size_ = BOX_BASE_LEN + 32;  // 32
    if (codecConfig_.size() > 0) {
        size_ += (codecConfig_.size() + 5); // 5
    }
    AppendChildSize();
    return size_;
}

int64_t EsdsBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(esDescrTag_);
    io->Write(esDescr_);
    io->Write(esId_);
    io->Write(esFlags_);
    io->Write(dcDescrTag_);
    io->Write(dcDescr_);
    io->Write(objectType_);
    io->Write(streamType_);
    io->Write(bufferSize_, sizeof(bufferSize_));
    io->Write(maxBitrate_);
    io->Write(avgBitrate_);
    if (codecConfig_.size() > 0) {
        io->Write(dsInfoTag_);
        io->Write(dsInfo_);
        io->Write(codecConfig_.data(), codecConfig_.size());
    }
    io->Write(slcDescrTag_);
    io->Write(slcDescr_);
    io->Write(slcData_);
    WriteChild(io);
    return io->GetPos();
}

BtrtBox::BtrtBox(uint32_t size, std::string type)
    : BasicBox(size, std::move(type))
{
}

uint32_t BtrtBox::GetSize()
{
    size_ = BOX_HEAD_LEN + 12;  // 12
    AppendChildSize();
    return size_;
}

int64_t BtrtBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(bufferSize_);
    io->Write(maxBitrate_);
    io->Write(avgBitrate_);
    WriteChild(io);
    return io->GetPos();
}

VideoBox::VideoBox(uint32_t size, std::string type)
    : BasicBox(size, std::move(type)) // mp4v, avc1, hvc1
{
}

uint32_t VideoBox::GetSize()
{
    size_ = BOX_HEAD_LEN + 78;  // 78
    AppendChildSize();
    return size_;
}

int64_t VideoBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(reserved1_);
    io->Write(reserved2_);
    io->Write(dataRefIndex_);
    io->Write(version_);
    io->Write(revision_);
    io->Write(reserved3_[0]);
    io->Write(reserved3_[1]);
    io->Write(reserved3_[2]);  // 2
    io->Write(width_);
    io->Write(height_);
    io->Write(horiz_);
    io->Write(vert_);
    io->Write(dataSize_);
    io->Write(frameCount_);
    io->Write(compressorLen_);
    io->Write(compressor_, 31);  // 31
    io->Write(depth_);
    io->Write(predefined_);
    WriteChild(io);
    return io->GetPos();
}

AvccBox::AvccBox(uint32_t size, std::string type)
    : BasicBox(size, std::move(type)) // avcC
{
}

uint32_t AvccBox::GetSize()
{
    size_ = BOX_HEAD_LEN;
    size_ += 6;  // 6
    for (auto &sps : sps_) {
        size_ += sps.size();
    }
    size_ += 1;
    for (auto &pps : pps_) {
        size_ += pps.size();
    }
    if (profileIdc_ != 0x42 && profileIdc_ != 0x4D && profileIdc_ != 0x58) {
        size_ += 4;  // 4
        for (auto &spsExt : spsExt_) {
            size_ += spsExt.size();
        }
    }
    AppendChildSize();
    return size_;
}

int64_t AvccBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(profileIdc_);
    io->Write(profileCompat_);
    io->Write(levelIdc_);
    io->Write(naluSize_);
    io->Write(spsCount_);
    for (auto &sps : sps_) {
        io->Write(sps.data(), sps.size());
    }
    io->Write(ppsCount_);
    for (auto &pps : pps_) {
        io->Write(pps.data(), pps.size());
    }
    if (profileIdc_ != 0x42 && profileIdc_ != 0x4D && profileIdc_ != 0x58) {
        io->Write(chromaFormat_);
        io->Write(bitDepthLuma_);
        io->Write(bitDepthChroma_);
        io->Write(spsExtCount_);
        for (auto &spsExt : spsExt_) {
            io->Write(spsExt.data(), spsExt.size());
        }
    }
    WriteChild(io);
    return io->GetPos();
}

HvccBox::HvccBox(uint32_t size, std::string type)
    : BasicBox(size, std::move(type))
{
    data.clear();
}

uint32_t HvccBox::GetSize()
{
    size_ = BOX_HEAD_LEN;
    if (data.size() > 0) {
        size_ += data.size();
    } else {
        size_ += 23;  // 23
        for (auto &naluType : naluType_) {
            size_ += 3;  // 3
            for (auto &nalu: naluType.nalu_) {
                size_ += nalu.size();
            }
        }
        AppendChildSize();
    }
    return size_;
}

int64_t HvccBox::Write(std::shared_ptr<AVIOStream> io)
{
    if (segmentIdc_ == 0) {
        parallelismType_ = 0;
    }

    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    if (data.size() > 0) {
        io->Write(data.data(), data.size());
    } else {
        io->Write(version_);
        io->Write(static_cast<uint8_t>((profileSpace_ << 6) | (tierFlag_ << 5) | profileIdc_));  // 6 5
        io->Write(profileCompatFlags_);
        io->Write(static_cast<uint32_t>(constraintIndicatorFlags_ >> 16));  // 16
        io->Write(static_cast<uint16_t>(constraintIndicatorFlags_));
        io->Write(levelIdc_);
        io->Write(static_cast<uint16_t>(segmentIdc_ | 0xF000));
        io->Write(static_cast<uint8_t>(parallelismType_ | 0xFC));
        io->Write(static_cast<uint8_t>(chromaFormat_ | 0xFC));
        io->Write(static_cast<uint8_t>(bitDepthLuma_ | 0xF8));
        io->Write(static_cast<uint8_t>(bitDepthChroma_ | 0xF8));
        io->Write(avgFrameRate_);
        io->Write(static_cast<uint8_t>((constFrameRate_ << 6) | (numTemporalLayers_ << 3) |  // 6 3
            (temporalIdNested_ << 2) | lenSizeMinusOne_));  // 2
        io->Write(naluTypeCount_);
        for (auto &naluType : naluType_) {
            io->Write(naluType.type_);
            io->Write(naluType.count_);
            for (auto &nalu : naluType.nalu_) {
                io->Write(nalu.data(), nalu.size());
            }
        }
    }
    WriteChild(io);
    return io->GetPos();
}

PaspBox::PaspBox(uint32_t size, std::string type)
    : BasicBox(size, std::move(type))
{
}

uint32_t PaspBox::GetSize()
{
    size_ = BOX_HEAD_LEN;
    size_ += 8;  // 8
    for (auto& box : childBoxes_) {
        size_ += box.second->GetSize();
    }
    return size_;
}

int64_t PaspBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(hSpacing_);
    io->Write(vSpacing_);
    WriteChild(io);
    return io->GetPos();
}

ColrBox::ColrBox(uint32_t size, std::string type)
    : BasicBox(size, std::move(type))
{
}

uint32_t ColrBox::GetSize()
{
    size_ = BOX_HEAD_LEN;
    size_ += 11;  // 11
    for (auto& box : childBoxes_) {
        size_ += box.second->GetSize();
    }
    return size_;
}

int64_t ColrBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(colorType_);
    io->Write(colorPrimaries_);
    io->Write(colorTransfer_);
    io->Write(colorMatrixCoeff_);
    io->Write(colorRange_);
    WriteChild(io);
    return io->GetPos();
}

CuvvBox::CuvvBox(uint32_t size, std::string type)
    : BasicBox(size, std::move(type))
{
}

uint32_t CuvvBox::GetSize()
{
    size_ = BOX_HEAD_LEN;
    size_ += 22;  // 22
    for (auto& box : childBoxes_) {
        size_ += box.second->GetSize();
    }
    return size_;
}

int64_t CuvvBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(cuvaVersion_);
    io->Write(terminalProvideCode_);
    io->Write(terminalProvideOriCode_);
    for (auto &reserved : reserved_) {
        io->Write(reserved);
    }
    WriteChild(io);
    return io->GetPos();
}

SttsBox::SttsBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags) // stts, ctts
{
}

void SttsBox::AddData(uint32_t data)
{
    size_t size = timeToSamples_.size();
    if (size > 0 && timeToSamples_[size - 1].second == data) {
        timeToSamples_[size - 1].first++;
    } else {
        entryCount_++;
        timeToSamples_.emplace_back(std::pair<uint32_t, uint32_t>(1, data));
    }
}

uint32_t SttsBox::GetSize()
{
    if (type_.compare("ctts") == 0 && entryCount_ == 0) {
        return 0;
    }
    size_ = BOX_BASE_LEN;
    size_ += 4;  // 4 byte
    size_ += 8 * timeToSamples_.size();  // 8 byte
    AppendChildSize();
    return size_;
}

int64_t SttsBox::Write(std::shared_ptr<AVIOStream> io)
{
    if (type_.compare("ctts") == 0 && entryCount_ == 0) {
        return io->GetPos();
    }
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(entryCount_);
    for (auto &sample : timeToSamples_) {
        io->Write(sample.first);
        io->Write(sample.second);
    }
    WriteChild(io);
    return io->GetPos();
}

StscBox::StscBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t StscBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    size_ += 4;  // 4
    size_ += 12 * chunks_.size();  // chunks len: 12
    AppendChildSize();
    return size_;
}

int64_t StscBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(entryCount_);
    for (auto &chunk : chunks_) {
        io->Write(chunk.chunkNum_);
        io->Write(chunk.samplesPerChunk_);
        io->Write(chunk.descrIndex_);
    }
    WriteChild(io);
    return io->GetPos();
}

StszBox::StszBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t StszBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    size_ += 8;  // 8
    size_ += 4 * samples_.size();  // 4
    AppendChildSize();
    return size_;
}

int64_t StszBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(sampleSize_);
    io->Write(sampleCount_);
    for (auto &sample : samples_) {
        io->Write(sample);
    }
    WriteChild(io);
    return io->GetPos();
}

StcoBox::StcoBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags) // stco, co64
{
}

uint32_t StcoBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    size_ += 4;  // 4
    if (isCo64_ || chunksOffset64_.size() > 0) {
        isCo64_ = true;
        type_ = "co64";
        size_ += 8 * chunksOffset32_.size();  // 8
        size_ += 8 * chunksOffset64_.size();  // 8
    } else {
        size_ += 4 * chunksOffset32_.size();  // 4
    }
    AppendChildSize();
    return size_;
}

int64_t StcoBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(chunkCount_);
    if (isCo64_) {
        for (auto &chunksOffset32 : chunksOffset32_) {
            io->Write(static_cast<uint64_t>(chunksOffset32) + offset_);
        }
        for (auto &chunksOffset64 : chunksOffset64_) {
            io->Write(chunksOffset64 + offset_);
        }
    } else {
        for (auto &chunksOffset32 : chunksOffset32_) {
            io->Write(chunksOffset32 + static_cast<uint32_t>(offset_));
        }
    }
    WriteChild(io);
    return io->GetPos();
}

StssBox::StssBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags) // stss, stps
{
}

uint32_t StssBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    size_ += 4;  // 4
    size_ += 4 * syncIndex_.size();  // 4
    AppendChildSize();
    return size_;
}

int64_t StssBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(syncCount_);
    for (auto &syncIndex : syncIndex_) {
        io->Write(syncIndex);
    }
    WriteChild(io);
    return io->GetPos();
}

SgpdBox::SgpdBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t SgpdBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    size_ += 12;  // 12
    size_ += 2 * distances_.size();  // 2
    AppendChildSize();
    return size_;
}

int64_t SgpdBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(groupType_);
    io->Write(defaultLen_);
    io->Write(entryCount_);
    for (auto &distance : distances_) {
        io->Write(distance);
    }
    WriteChild(io);
    return io->GetPos();
}

SbgpBox::SbgpBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t SbgpBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    size_ += 8;  // 8
    size_ += 8 * descriptions_.size();  // 8
    AppendChildSize();
    return size_;
}

int64_t SbgpBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(groupType_);
    io->Write(entryCount_);
    for (auto &description : descriptions_) {
        io->Write(description.first);
        io->Write(description.second);
    }
    WriteChild(io);
    return io->GetPos();
}

KeysBox::KeysBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t KeysBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    size_ += 4;  // 4
    AppendChildSize();
    return size_;
}

int64_t KeysBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(keyCount_);
    WriteChild(io);
    return io->GetPos();
}

DataBox::DataBox(uint32_t size, std::string type)
    : BasicBox(size, std::move(type))
{
}

uint32_t DataBox::GetSize()
{
    size_ = BOX_HEAD_LEN;
    size_ += 8;  // 8
    size_ += data_.size();
    AppendChildSize();
    return size_;
}

int64_t DataBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(dataType_);
    io->Write(default_);
    io->Write(data_.data(), data_.size());
    WriteChild(io);
    return io->GetPos();
}

LociBox::LociBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t LociBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    size_ += 16; // 16
    size_ += place_.length() + 1;
    size_ += astronomicalBody_.length() + 1;
    AppendChildSize();
    return size_;
}

int64_t LociBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    io->Write(static_cast<uint16_t>(0));  // lang
    io->Write(place_);
    io->Write(static_cast<uint8_t>(0));  // role of place, 0:shooting 1:real 2:fictional
    io->Write(static_cast<uint32_t>(longitude_));
    io->Write(static_cast<uint32_t>(latitude_));
    io->Write(static_cast<uint32_t>(altitude_));  // altitude
    io->Write(astronomicalBody_);
    io->Write(static_cast<uint8_t>(0));
    WriteChild(io);
    return io->GetPos();
}

SdtpBox::SdtpBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t SdtpBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    size_ += static_cast<uint32_t>(dependencyFlags_.size());
    AppendChildSize();
    return size_;
}

int64_t SdtpBox::Write(std::shared_ptr<AVIOStream> io)
{
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    for (uint8_t flag : dependencyFlags_) {
        io->Write(flag);
    }
    WriteChild(io);
    return io->GetPos();
}

InfeBox::InfeBox(uint32_t size, std::string type, uint8_t version, uint32_t flags)
    : FullBox(size, std::move(type), version, flags)
{
}

uint32_t InfeBox::GetSize()
{
    size_ = BOX_BASE_LEN;
    if (version_ == 0 || version_ == 1) {
        size_ += 4;  // int16 * 2 = 4
        size_ += static_cast<uint32_t>(itemName_.length() + 1);
        size_ += static_cast<uint32_t>(contentType_.length() + 1);
        size_ += static_cast<uint32_t>(contentEncoding_.length() + 1);
        if (version_ == 1) {
            size_ += static_cast<uint32_t>(sizeof(uint32_t));
        }
    } else if (version_ >= 2) {  // 2
        size_ += (version_ == 2 ? 2 : 4);  // 2 4
        size_ += static_cast<uint32_t>(6 + itemName_.length() + 1);  // 6
        if (itemType_.compare(0, 4, "mime") == 0) {  // 4
            size_ += static_cast<uint32_t>(contentType_.length() + 1);
            size_ += static_cast<uint32_t>(contentEncoding_.length() + 1);
        } else {
            size_ += 4;  // 4
        }
    }
    AppendChildSize();
    return size_;
}

int64_t InfeBox::Write(std::shared_ptr<AVIOStream> io)
{
    int64_t pos = io->GetPos();
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(version_);
    io->Write(flags_, BOX_FLAGS_LEN);
    if (version_ == 0 || version_ == 1) {
        io->Write(static_cast<uint16_t>(1));  // item id
        io->Write(static_cast<uint16_t>(0));  // item protection idex: 0
        io->Write(itemName_);
        io->Write(contentType_);
        io->Write(contentEncoding_);
        if (version_ == 1) {
            io->Write(static_cast<uint32_t>(1));
        }
    } else if (version_ >= 2) {  // 2
        if (version_ == 2) {     // 2
            io->Write(static_cast<uint16_t>(1));
        } else {
            io->Write(static_cast<uint32_t>(1));
        }
        io->Write(static_cast<uint16_t>(0));  // item protection index
        io->Write(itemType_.data(), BOX_TYPE_LEN);
        io->Write(itemName_);
        if (itemType_.compare(0, 4, "mime") == 0) {  // 4
            io->Write(contentType_);
            io->Write(contentEncoding_);
        } else {
            io->Write("uri", 3);  // 3
            io->Write(static_cast<uint8_t>(0));
        }
    }
    WriteChild(io);
    FALSE_RETURN_V_MSG_E(io->GetPos() - pos == size_, io->GetPos(), "%{public}s box size fail!", type_.data());
    return io->GetPos();
}

TrefBox::TrefBox(uint32_t size, std::string type)
    : BasicBox(size, std::move(type))
{
}

uint32_t TrefBox::GetSize()
{
    size_ = static_cast<uint32_t>(srcTrackIds_.size() * sizeof(uint32_t)) + BOX_HEAD_LEN + BOX_HEAD_LEN;
    AppendChildSize();
    return size_;
}

int64_t TrefBox::Write(std::shared_ptr<AVIOStream> io)
{
    uint32_t dataSize = static_cast<uint32_t>(srcTrackIds_.size() * sizeof(uint32_t)) + BOX_HEAD_LEN;
    io->Write(size_);
    io->Write(type_.data(), BOX_TYPE_LEN);
    io->Write(dataSize);
    io->Write(trefTag_);
    for (auto id : srcTrackIds_) {
        io->Write(id);
    }
    WriteChild(io);
    return io->GetPos();
}
} // Mpeg4
} // Plugins
} // Media
} // OHOS