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

bool AvcParserImpl::ConvertExtraDataToAnnexb(uint8_t *extraData, int32_t extraDataSize)
{
    annexbExtraDataVec_.clear();
    (void)ParseMaxReorderPicFromAvcConfig(extraData, extraDataSize);
    if (IsAnnexbFrame(extraData, extraDataSize)) {
        annexbExtraDataVec_.insert(annexbExtraDataVec_.end(), extraData, extraData + extraDataSize);
        return true;
    }
    std::vector<DataAddrInfo> addrInfoVec;
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
        dataAddrInfo_ = {rbspContext->GetBytesIndex(), copySize};
        rbspContext->RbspSkipBits(copySize * 0x08); // nalUnit
        addrInfoVec.emplace_back(dataAddrInfo_);
    }
    uint8_t numOfPPS = rbspContext->RbspGetBits(0x08);   // numOfPictureParameterSets
    for (int32_t i = 0; i < numOfPPS; ++i) {
        uint16_t naluLength = rbspContext->RbspGetBits<uint16_t, uint32_t>(0x10);  // pictureParameterSetsLength
        uint32_t copySize =
            static_cast<uint32_t>(extraDataSize) > naluLength ? naluLength : static_cast<uint32_t>(extraDataSize);
        dataAddrInfo_ = {rbspContext->GetBytesIndex(), copySize};
        rbspContext->RbspSkipBits(copySize * 0x08);  // nalUnit
        addrInfoVec.emplace_back(dataAddrInfo_);
    }
    for (auto iter : addrInfoVec) {
        WriteStartCode(annexbExtraDataVec_);
        FALSE_RETURN_V_MSG_E(
            static_cast<uint32_t>(iter.bytesIndex) + iter.bufferSize <= static_cast<uint32_t>(extraDataSize),
            false, "Error size");
        annexbExtraDataVec_.insert(
            annexbExtraDataVec_.end(), extraData + iter.bytesIndex, extraData + iter.bytesIndex + iter.bufferSize);
    }
    return true;
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

bool AvcParserImpl::ParsePacket(
    std::vector<DataAddrInfo> &dataInfoVec, bool &gotIrap, const uint8_t *packet, int32_t packetSize)
{
    std::shared_ptr<RbspContext> rbspContext = std::make_shared<RbspContext>(packet, packetSize);
    FALSE_RETURN_V_MSG_E(rbspContext != nullptr, false, "rbspContext is nullptr");
    while (rbspContext->RbspGetLeftBytesNum() > 0) {
        uint32_t naluSize = 0;
        if (rbspContext->RbspGetLeftBytesNum() < naluLength_) {
            return false;
        }
        for (int32_t i = 0; i < naluLength_; ++i) {
            naluSize = (naluSize << 0x08) | rbspContext->RbspGetBits(0x08); // 前4位表示数据大小
        }
        if (naluSize < 0x02 || naluSize > rbspContext->RbspGetLeftBytesNum()) {
            return false;
        }
        int32_t naluType = (rbspContext->RbspGetBits(0x08, false)) & 0x1f; // false 读完后不跳转
        bool isIrap = naluType == AvcNalType::AVC_IDR_W_RADL;
        gotIrap = (gotIrap || isIrap) ? true : false;
        dataAddrInfo_ = {rbspContext->GetBytesIndex(), naluSize};
        rbspContext->RbspSkipBits(naluSize * 0x08);
        dataInfoVec.emplace_back(dataAddrInfo_);
    }
    return true;
}

void AvcParserImpl::ConvertPacketCore(uint8_t **packet, int32_t &packetSize)
{
    annexbFrameVec_.clear();
    std::vector<DataAddrInfo> addrInfoVec;
    bool gotIrap = false;
    bool ret = ParsePacket(addrInfoVec, gotIrap, *packet, packetSize);
    if (!ret) {
        MEDIA_LOG_E("ParsePacket failed");
        return;
    }
    // just add xps to begin of first frame.
    if (!sendXps_ && gotIrap) {
        annexbFrameVec_.insert(
            annexbFrameVec_.end(), annexbExtraDataVec_.data(), annexbExtraDataVec_.data() + annexbExtraDataVec_.size());
        sendXps_ = true;
    }
    for (auto iter : addrInfoVec) {
        if (iter.bufferSize > static_cast<uint32_t>(packetSize)) {
            return;
        }
        WriteStartCode(annexbFrameVec_);
        annexbFrameVec_.insert(
            annexbFrameVec_.end(), *packet + iter.bytesIndex, *packet + iter.bytesIndex + iter.bufferSize);
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
    sendXps_ = false;
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS
