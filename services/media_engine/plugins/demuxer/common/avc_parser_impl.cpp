/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include "common/log.h"
#include "meta/media_types.h"
#include "avc_parser_impl.h"

namespace {
using OHOS::Media::Plugins::RbspContext;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "AvcParser"};
constexpr uint8_t START_CODE[] = {0x00, 0x00, 0x01};
constexpr int32_t SIZE_CODE_LEN = 4;
constexpr int32_t AVC_CONFIG_HEADER_SIZE = 6;
constexpr int32_t AVC_CONFIG_SPS_OFFSET = 5;
constexpr uint8_t AVC_CONFIG_SPS_COUNT_MASK = 0x1F;
constexpr int32_t AVC_NAL_HEADER_SIZE = 1;
constexpr int32_t H264_SCALING_LIST_MOD = 256;
constexpr uint32_t H264_EXTENDED_SAR = 255;
constexpr uint32_t H264_FRAME_CROP_OFFSET_COUNT = 4;
constexpr uint32_t H264_BYTE_BITS = 8;
constexpr uint32_t H264_SHORT_BITS = 16;
constexpr uint32_t H264_WORD_BITS = 32;
constexpr uint32_t H264_CHROMA_FORMAT_444 = 3;
constexpr uint32_t H264_SCALING_LIST_4X4_COUNT = 6;
constexpr uint32_t H264_SCALING_LIST_4X4_SIZE = 16;
constexpr uint32_t H264_SCALING_LIST_8X8_SIZE = 64;
constexpr uint32_t H264_DEFAULT_SCALING_LIST_COUNT = 8;
constexpr uint32_t H264_SEPARATE_COLOUR_SCALING_LIST_COUNT = 12;
constexpr uint32_t MAX_RBSP_SKIP_BYTES = std::numeric_limits<uint32_t>::max() / H264_BYTE_BITS;
constexpr int32_t DATA_DEDUPLICATION_THRESHOLD = 1024;
constexpr uint8_t SEI_TYPE_BUFFERING_PERIOD = 0;

bool SkipH264ScalingList(RbspContext& reader, uint32_t sizeOfScalingList)
{
    int32_t lastScale = 8;
    int32_t nextScale = 8;
    for (uint32_t j = 0; j < sizeOfScalingList; ++j) {
        if (nextScale != 0) {
            int32_t deltaScale = 0;
            FALSE_RETURN_V(reader.RbspReadSE(deltaScale), false);
            nextScale = (lastScale + deltaScale + H264_SCALING_LIST_MOD) % H264_SCALING_LIST_MOD;
        }
        lastScale = (nextScale == 0) ? lastScale : nextScale;
    }
    return true;
}

bool IsH264HighProfile(uint32_t profileIdc)
{
    constexpr uint32_t h264ProfileHigh = 100;
    constexpr uint32_t h264ProfileHigh10 = 110;
    constexpr uint32_t h264ProfileHigh422 = 122;
    constexpr uint32_t h264ProfileHigh444Predictive = 244;
    constexpr uint32_t h264ProfileCavlc444 = 44;
    constexpr uint32_t h264ProfileScalableBaseline = 83;
    constexpr uint32_t h264ProfileScalableHigh = 86;
    constexpr uint32_t h264ProfileMultiviewHigh = 118;
    constexpr uint32_t h264ProfileStereoHigh = 128;
    constexpr uint32_t h264ProfileMultiviewDepthHigh = 138;
    constexpr uint32_t h264ProfileEnhancedMultiviewDepthHigh = 139;
    constexpr uint32_t h264ProfileMfcHigh = 134;
    constexpr uint32_t h264ProfileMfcDepthHigh = 135;
    constexpr std::array<uint32_t, 13> highProfiles = {
        h264ProfileHigh,
        h264ProfileHigh10,
        h264ProfileHigh422,
        h264ProfileHigh444Predictive,
        h264ProfileCavlc444,
        h264ProfileScalableBaseline,
        h264ProfileScalableHigh,
        h264ProfileMultiviewHigh,
        h264ProfileStereoHigh,
        h264ProfileMultiviewDepthHigh,
        h264ProfileEnhancedMultiviewDepthHigh,
        h264ProfileMfcHigh,
        h264ProfileMfcDepthHigh,
    };
    return std::find(highProfiles.begin(), highProfiles.end(), profileIdc) != highProfiles.end();
}

bool SkipH264HighProfileSyntax(RbspContext& reader, uint32_t& tmp)
{
    uint32_t chromaFormatIdc = 0;
    FALSE_RETURN_V(reader.RbspReadUE(chromaFormatIdc), false);
    if (chromaFormatIdc == H264_CHROMA_FORMAT_444) {
        FALSE_RETURN_V(reader.RbspReadBit(tmp), false);
    }
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false);
    uint32_t seqScalingMatrixPresent = 0;
    FALSE_RETURN_V(reader.RbspReadBit(seqScalingMatrixPresent), false);
    if (!seqScalingMatrixPresent) {
        return true;
    }
    uint32_t scalingListCount = (chromaFormatIdc != H264_CHROMA_FORMAT_444) ?
        H264_DEFAULT_SCALING_LIST_COUNT : H264_SEPARATE_COLOUR_SCALING_LIST_COUNT;
    for (uint32_t i = 0; i < scalingListCount; ++i) {
        uint32_t scalingListPresent = 0;
        FALSE_RETURN_V(reader.RbspReadBit(scalingListPresent), false);
        uint32_t scalingListSize = i < H264_SCALING_LIST_4X4_COUNT ?
            H264_SCALING_LIST_4X4_SIZE : H264_SCALING_LIST_8X8_SIZE;
        if (scalingListPresent && !SkipH264ScalingList(reader, scalingListSize)) {
            return false;
        }
    }
    return true;
}

bool SkipH264PictureOrderSyntax(RbspContext& reader, uint32_t& tmp)
{
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
    uint32_t picOrderCntType = 0;
    FALSE_RETURN_V(reader.RbspReadUE(picOrderCntType), false);
    if (picOrderCntType == 0) {
        return reader.RbspReadUE(tmp);
    }
    if (picOrderCntType != 1) {
        return true;
    }
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false);
    int32_t signedTmp = 0;
    FALSE_RETURN_V(reader.RbspReadSE(signedTmp), false);
    FALSE_RETURN_V(reader.RbspReadSE(signedTmp), false);
    uint32_t cycleCount = 0;
    FALSE_RETURN_V(reader.RbspReadUE(cycleCount), false);
    for (uint32_t i = 0; i < cycleCount; ++i) {
        FALSE_RETURN_V(reader.RbspReadSE(signedTmp), false);
    }
    return true;
}

bool SkipH264VuiBeforeRestriction(RbspContext& reader, uint32_t& tmp)
{
    uint32_t flag = 0;
    FALSE_RETURN_V(reader.RbspReadBit(flag), false);
    if (flag) {
        uint32_t aspectRatioIdc = 0;
        FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, aspectRatioIdc), false);
        if (aspectRatioIdc == H264_EXTENDED_SAR) {
            FALSE_RETURN_V(reader.RbspReadBits(H264_SHORT_BITS, tmp), false);
            FALSE_RETURN_V(reader.RbspReadBits(H264_SHORT_BITS, tmp), false);
        }
    }
    FALSE_RETURN_V(reader.RbspReadBit(flag), false);
    if (flag) {
        FALSE_RETURN_V(reader.RbspReadBit(tmp), false);
    }
    FALSE_RETURN_V(reader.RbspReadBit(flag), false);
    if (flag) {
        FALSE_RETURN_V(reader.RbspReadBits(H264_CHROMA_FORMAT_444, tmp), false);
        FALSE_RETURN_V(reader.RbspReadBit(tmp), false);
        uint32_t colourDescriptionPresent = 0;
        FALSE_RETURN_V(reader.RbspReadBit(colourDescriptionPresent), false);
        if (colourDescriptionPresent) {
            FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, tmp), false);
            FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, tmp), false);
            FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, tmp), false);
        }
    }
    FALSE_RETURN_V(reader.RbspReadBit(flag), false);
    if (flag) {
        FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
        FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
    }
    return true;
}

bool SkipH264GeometrySyntax(RbspContext& reader, uint32_t& tmp)
{
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false);
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
    uint32_t frameMbsOnlyFlag = 0;
    FALSE_RETURN_V(reader.RbspReadBit(frameMbsOnlyFlag), false);
    if (!frameMbsOnlyFlag) {
        FALSE_RETURN_V(reader.RbspReadBit(tmp), false);
    }
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false);
    uint32_t frameCroppingFlag = 0;
    FALSE_RETURN_V(reader.RbspReadBit(frameCroppingFlag), false);
    if (frameCroppingFlag) {
        for (uint32_t i = 0; i < H264_FRAME_CROP_OFFSET_COUNT; ++i) {
            FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
        }
    }
    return true;
}

bool ReadH264ReorderDelayFromVui(RbspContext& reader, uint32_t& tmp, uint32_t& reorderDelay)
{
    FALSE_RETURN_V(SkipH264VuiBeforeRestriction(reader, tmp), false);
    uint32_t flag = 0;
    FALSE_RETURN_V(reader.RbspReadBit(flag), false);
    if (flag) {
        FALSE_RETURN_V(reader.RbspReadBits(H264_WORD_BITS, tmp), false);
        FALSE_RETURN_V(reader.RbspReadBits(H264_WORD_BITS, tmp), false);
        FALSE_RETURN_V(reader.RbspReadBit(tmp), false);
    }
    FALSE_RETURN_V(reader.RbspReadBit(flag), false);
    if (flag) {
        return false;
    }
    FALSE_RETURN_V(reader.RbspReadBit(flag), false);
    if (flag) {
        return false;
    }
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false);
    FALSE_RETURN_V(reader.RbspReadBit(flag), false);
    if (!flag) {
        return false;
    }
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false);
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false);
    return reader.RbspReadUE(reorderDelay);
}

bool ParseH264SpsReorderDelay(const uint8_t* sps, size_t size, uint32_t& reorderDelay)
{
    FALSE_RETURN_V(sps != nullptr && size > AVC_NAL_HEADER_SIZE, false);
    RbspContext reader(sps + AVC_NAL_HEADER_SIZE, static_cast<int32_t>(size - AVC_NAL_HEADER_SIZE), true);
    uint32_t profileIdc = 0;
    uint32_t tmp = 0;
    FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, profileIdc), false);
    FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, tmp), false);
    FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, tmp), false);
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false);

    if (IsH264HighProfile(profileIdc) && !SkipH264HighProfileSyntax(reader, tmp)) {
        return false;
    }

    FALSE_RETURN_V(SkipH264PictureOrderSyntax(reader, tmp), false);
    FALSE_RETURN_V(SkipH264GeometrySyntax(reader, tmp), false);
    uint32_t vuiParametersPresent = 0;
    FALSE_RETURN_V(reader.RbspReadBit(vuiParametersPresent), false);
    if (!vuiParametersPresent) {
        return false;
    }
    return ReadH264ReorderDelayFromVui(reader, tmp, reorderDelay);
}
}

namespace OHOS {
namespace Media {
namespace Plugins {
AvcParserImpl::~AvcParserImpl()
{
    spsVec_.clear();
    ppsVec_.clear();
    annexbExtraDataVec_.clear();
    annexbFrameVec_.clear();
}

bool AvcParserImpl::IsAnnexbFrame(const uint8_t *sample, int32_t size)
{
    FALSE_RETURN_V(size >= SIZE_CODE_LEN, false);
    FALSE_RETURN_V(sample != nullptr, false);
    auto *iter = std::search(sample, sample + SIZE_CODE_LEN, START_CODE, START_CODE + sizeof(START_CODE));
    if (iter == sample || (iter == sample + 1 && sample[0] == 0x00)) {
        return true;
    }
    return false;
}

void AvcParserImpl::WriteStartCode(std::vector<uint8_t> &vec)
{
    vec.emplace_back(0x00);
    vec.emplace_back(0x00);
    vec.emplace_back(0x00);
    vec.emplace_back(0x01);
}

void AvcParserImpl::AppendOutOfBandPs(std::vector<uint8_t> &vec, const std::vector<uint8_t> &ps)
{
    if (!ps.empty()) {
        vec.insert(vec.end(), ps.begin(), ps.end());
    }
}

void AvcParserImpl::AppendNalUnit(std::vector<uint8_t> &vec, const uint8_t *data, uint32_t size, PsSource ps)
{
    if (ps == PS_OUT_OF_BAND) {
        vec.insert(vec.end(), data, data + size);
    } else {
        WriteStartCode(vec);
        vec.insert(vec.end(), data, data + size);
    }
}

void AvcParserImpl::SavePs(std::vector<uint8_t> &dst, const uint8_t *nal, uint32_t nalSize, bool first)
{
    if (first) {
        dst.clear();
    }
    WriteStartCode(dst);
    dst.insert(dst.end(), nal, nal + nalSize);
}

bool AvcParserImpl::SavePsVector(
    uint8_t *extraData, int32_t extraDataSize, std::vector<DataAddrInfo> src, AvcNalType nalType)
{
    for (auto iter : src) {
        FALSE_RETURN_V_MSG_E(iter.bytesIndex <= static_cast<uint32_t>(extraDataSize) &&
            iter.bufferSize <= static_cast<uint32_t>(extraDataSize) - iter.bytesIndex, false, "Error size");
        if (nalType == AvcNalType::AVC_NAL_SPS) {
            WriteStartCode(spsVec_);
            spsVec_.insert(spsVec_.end(), extraData + iter.bytesIndex, extraData + iter.bytesIndex + iter.bufferSize);
        } else if (nalType == AvcNalType::AVC_NAL_PPS) {
            WriteStartCode(ppsVec_);
            ppsVec_.insert(ppsVec_.end(), extraData + iter.bytesIndex, extraData + iter.bytesIndex + iter.bufferSize);
        }
    }
    return true;
}

bool AvcParserImpl::ConvertExtraDataToAnnexb(uint8_t *extraData, int32_t extraDataSize)
{
    spsVec_.clear();
    ppsVec_.clear();
    annexbExtraDataVec_.clear();
    (void)ParseMaxReorderPicFromAvcConfig(extraData, extraDataSize);
    if (IsAnnexbFrame(extraData, extraDataSize)) {
        annexbExtraDataVec_.insert(annexbExtraDataVec_.end(), extraData, extraData + extraDataSize);
        return true;
    }
    std::vector<DataAddrInfo> spsAddrInfoVec;
    std::vector<DataAddrInfo> ppsAddrInfoVec;
    std::shared_ptr<RbspContext> rbspContext = std::make_shared<RbspContext>(extraData, extraDataSize);
    FALSE_RETURN_V_MSG_E(rbspContext != nullptr, false, "rbspContext is nullptr");

    // configurationVersion, AVCProfileIndication, profile_compatibility, AVCLevelIndication, reserverd
    rbspContext->RbspSkipBits(0x26);
    naluLength_ = rbspContext->RbspGetBits(0x02) + 0x01;  // lengthSizeMinusOne
    rbspContext->RbspSkipBits(0x03);                      // reserverd
    uint8_t numOfSPS = rbspContext->RbspGetBits(0x05);    // numOfSequenceParameterSets
    for (int32_t i = 0; i < numOfSPS; ++i) {
        uint16_t naluLength = rbspContext->RbspGetBits<uint16_t, uint32_t>(0x10); // sequenceParameterSetsLength
        uint32_t copySize =
            static_cast<uint32_t>(extraDataSize) > naluLength ? naluLength : static_cast<uint32_t>(extraDataSize);
        DataAddrInfo addrInfo = {rbspContext->GetBytesIndex(), copySize};
        rbspContext->RbspSkipBits(copySize * 0x08); // nalUnit
        spsAddrInfoVec.emplace_back(addrInfo);
    }
    uint8_t numOfPPS = rbspContext->RbspGetBits(0x08);   // numOfPictureParameterSets
    for (int32_t i = 0; i < numOfPPS; ++i) {
        uint16_t naluLength = rbspContext->RbspGetBits<uint16_t, uint32_t>(0x10);  // pictureParameterSetsLength
        uint32_t copySize =
            static_cast<uint32_t>(extraDataSize) > naluLength ? naluLength : static_cast<uint32_t>(extraDataSize);
        DataAddrInfo addrInfo = {rbspContext->GetBytesIndex(), copySize};
        rbspContext->RbspSkipBits(copySize * 0x08);  // nalUnit
        ppsAddrInfoVec.emplace_back(addrInfo);
    }
    if (extraDataSize <= DATA_DEDUPLICATION_THRESHOLD) {
        FALSE_RETURN_V_NOLOG(SavePsVector(extraData, extraDataSize, spsAddrInfoVec, AvcNalType::AVC_NAL_SPS), false);
        FALSE_RETURN_V_NOLOG(SavePsVector(extraData, extraDataSize, ppsAddrInfoVec, AvcNalType::AVC_NAL_PPS), false);
    } else {
        if (!spsAddrInfoVec.empty()) {
            FALSE_RETURN_V_NOLOG(
                SavePsVector(extraData, extraDataSize, {spsAddrInfoVec.back()}, AvcNalType::AVC_NAL_SPS), false);
        }
        if (!ppsAddrInfoVec.empty()) {
            FALSE_RETURN_V_NOLOG(
                SavePsVector(extraData, extraDataSize, {ppsAddrInfoVec.back()}, AvcNalType::AVC_NAL_PPS), false);
        }
    }
    annexbExtraDataVec_.insert(annexbExtraDataVec_.end(), spsVec_.begin(), spsVec_.end());
    annexbExtraDataVec_.insert(annexbExtraDataVec_.end(), ppsVec_.begin(), ppsVec_.end());
    SetPsFlag();
    return true;
}

void AvcParserImpl::SetPsFlag()
{
    extradataParsed_ = true;
    newIdr_ = true;
    idrSpsSeen_ = false;
    idrPpsSeen_ = false;
}

bool AvcParserImpl::ParseMaxReorderPicFromAvcConfig(const uint8_t *extraData, int32_t extraDataSize)
{
    maxReorderPic_ = 0;
    FALSE_RETURN_V(extraData != nullptr && extraDataSize >= AVC_CONFIG_HEADER_SIZE, false);
    size_t offset = AVC_CONFIG_SPS_OFFSET;
    uint8_t spsCount = extraData[offset++] & AVC_CONFIG_SPS_COUNT_MASK;
    for (uint8_t i = 0; i < spsCount; ++i) {
        if (offset + sizeof(uint16_t) > static_cast<size_t>(extraDataSize)) {
            return false;
        }
        uint16_t spsLength = (static_cast<uint16_t>(extraData[offset]) << H264_BYTE_BITS) | extraData[offset + 1];
        offset += sizeof(uint16_t);
        if (offset + spsLength > static_cast<size_t>(extraDataSize)) {
            return false;
        }
        uint32_t reorderDelay = 0;
        if (ParseH264SpsReorderDelay(extraData + offset, spsLength, reorderDelay)) {
            maxReorderPic_ = reorderDelay;
            return true;
        }
        offset += spsLength;
    }
    return false;
}

void AvcParserImpl::ConvertExtradata(uint8_t **packet, int32_t &packetSize)
{
    *packet = annexbExtraDataVec_.data();
    packetSize = static_cast<int32_t>(annexbExtraDataVec_.size());
}

bool AvcParserImpl::ParseNalUnits(std::vector<NalUnitInfo> &nalUnits, const uint8_t *packet, int32_t packetSize)
{
    const uint8_t *buf = packet;
    const uint8_t *bufEnd = packet + packetSize;
    while (buf + naluLength_ <= bufEnd) {
        uint32_t nalSize = 0;
        for (int32_t i = 0; i < naluLength_; ++i) {
            nalSize = (nalSize << 0x08) | buf[i];
        }
        buf += naluLength_;
        if (nalSize == 0) {
            continue;
        }
        if (nalSize < 0x02 || nalSize > MAX_RBSP_SKIP_BYTES ||
            static_cast<int64_t>(nalSize) > bufEnd - buf) {
            return false;
        }
        uint8_t nalType = buf[0] & 0x1f;
        nalUnits.emplace_back(NalUnitInfo{buf, nalSize, nalType});
        buf += nalSize;
    }
    return true;
}

void AvcParserImpl::FilterPsFromPacket(const uint8_t *packet, int32_t packetSize)
{
    const uint8_t *buf = packet;
    const uint8_t *bufEnd = packet + packetSize;
    int spsCount = 0;
    int ppsCount = 0;
    while (buf + naluLength_ <= bufEnd) {
        uint32_t nalSize = 0;
        for (int32_t i = 0; i < naluLength_; ++i) {
            nalSize = (nalSize << 0x08) | buf[i];
        }
        buf += naluLength_;
        if (static_cast<int64_t>(nalSize) > bufEnd - buf) {
            return;
        }
        if (nalSize == 0) {
            continue;
        }
        uint8_t unitType = buf[0] & 0x1f;
        if (unitType == AvcNalType::AVC_NAL_SPS) {
            SavePs(spsVec_, buf, nalSize, spsCount == 0);
            spsCount++;
        } else if (unitType == AvcNalType::AVC_NAL_PPS) {
            SavePs(ppsVec_, buf, nalSize, ppsCount == 0);
            ppsCount++;
        }
        buf += nalSize;
    }
}

void AvcParserImpl::HandleNal(NalUnitInfo nal)
{
    if (nal.nalType == AvcNalType::AVC_NAL_SPS) {
        idrSpsSeen_ = true;
        newIdr_ = true;
    } else if (nal.nalType == AvcNalType::AVC_NAL_PPS) {
        idrPpsSeen_ = true;
        newIdr_ = true;
        if (!idrSpsSeen_) {
            if (!spsVec_.empty()) {
                AppendOutOfBandPs(annexbFrameVec_, spsVec_);
                idrSpsSeen_ = true;
            }
        }
    }
    if (!newIdr_ && nal.nalType == AvcNalType::AVC_NAL_IDR_SLICE && (nal.size > 1 && (nal.data[1] & 0x80))) {
        newIdr_ = true;
    }
    if (nal.nalType == AvcNalType::AVC_NAL_SEI && nal.size > 1 && nal.data[1] == SEI_TYPE_BUFFERING_PERIOD &&
        !idrSpsSeen_ && !idrPpsSeen_) {
        if (!spsVec_.empty()) {
            AppendOutOfBandPs(annexbFrameVec_, spsVec_);
            idrSpsSeen_ = true;
        }
        if (!ppsVec_.empty()) {
            AppendOutOfBandPs(annexbFrameVec_, ppsVec_);
            idrPpsSeen_ = true;
        }
    }
    if (newIdr_ && nal.nalType == AvcNalType::AVC_NAL_IDR_SLICE && !idrSpsSeen_ && !idrPpsSeen_) {
        if (!spsVec_.empty()) {
            AppendOutOfBandPs(annexbFrameVec_, spsVec_);
        }
        if (!ppsVec_.empty()) {
            AppendOutOfBandPs(annexbFrameVec_, ppsVec_);
        }
        newIdr_ = false;
    } else if (newIdr_ && nal.nalType == AvcNalType::AVC_NAL_IDR_SLICE && idrSpsSeen_ && !idrPpsSeen_) {
        if (!ppsVec_.empty()) {
            AppendOutOfBandPs(annexbFrameVec_, ppsVec_);
        }
    }
    PsSource ps =
        (nal.nalType == AvcNalType::AVC_NAL_SPS || nal.nalType == AvcNalType::AVC_NAL_PPS) ? PS_IN_BAND : PS_NONE;
    AppendNalUnit(annexbFrameVec_, nal.data, nal.size, ps);
    if (nal.nalType == AvcNalType::AVC_NAL_SLICE) {
        newIdr_ = true;
        idrSpsSeen_ = false;
        idrPpsSeen_ = false;
    }
}

void AvcParserImpl::ConvertPacketCore(uint8_t **packet, int32_t &packetSize)
{
    annexbFrameVec_.clear();
    FALSE_RETURN_NOLOG(extradataParsed_);
    FilterPsFromPacket(*packet, packetSize);
    std::vector<NalUnitInfo> nalUnits;
    bool ret = ParseNalUnits(nalUnits, *packet, packetSize);
    FALSE_RETURN_MSG_W(ret, "ParseNalUnits failed");
    for (auto &nal : nalUnits) {
        HandleNal(nal);
    }
    *packet = annexbFrameVec_.data();
    packetSize = static_cast<int32_t>(annexbFrameVec_.size());
}

void AvcParserImpl::ConvertPacketToAnnexb(
    uint8_t **hvccPacket, int32_t &hvccPacketSize, uint8_t *sideData, size_t sideDataSize, bool isExtradata)
{
    (void)sideData;
    (void)sideDataSize;
    if (!hvccPacket || !*hvccPacket) {
        return;
    }
    if (isExtradata) {
        ConvertExtradata(hvccPacket, hvccPacketSize);
    } else {
        ConvertPacketCore(hvccPacket, hvccPacketSize);
    }
}

void AvcParserImpl::ResetXPSSendStatus()
{
    idrSpsSeen_ = false;
    idrPpsSeen_ = false;
    newIdr_ = extradataParsed_;
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS
