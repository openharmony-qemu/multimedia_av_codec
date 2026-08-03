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
constexpr uint32_t H264_BYTE_BITS = 8;
constexpr uint32_t H264_SHORT_BITS = 16;
constexpr uint32_t H264_WORD_BITS = 32;
constexpr uint32_t H264_CHROMA_FORMAT_444 = 3;
constexpr uint32_t H264_SCALING_LIST_4X4_COUNT = 6;
constexpr uint32_t H264_SCALING_LIST_4X4_SIZE = 16;
constexpr uint32_t H264_SCALING_LIST_8X8_SIZE = 64;
constexpr uint32_t H264_DEFAULT_SCALING_LIST_COUNT = 8;
constexpr uint32_t H264_SEPARATE_COLOUR_SCALING_LIST_COUNT = 12;
constexpr uint32_t H264_MB_SIZE = 16;
constexpr uint32_t MAX_RBSP_SKIP_BYTES = std::numeric_limits<uint32_t>::max() / H264_BYTE_BITS;
constexpr int32_t DATA_DEDUPLICATION_THRESHOLD = 1024;
constexpr uint8_t SEI_TYPE_BUFFERING_PERIOD = 0;

struct H264SarEntry {
    uint32_t width;
    uint32_t height;
};

struct H264SpsInfo {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t chromaFormatIdc = 1;
    uint32_t separateColourPlaneFlag = 0;
    uint32_t reorderDelay = 0;
    double sar = 0.0;
};

constexpr std::array<H264SarEntry, 16> H264_SAR_TABLE = {{
    {1u, 1u},     {12u, 11u},   {10u, 11u},   {16u, 11u},
    {40u, 33u},   {24u, 11u},   {20u, 11u},   {32u, 11u},
    {80u, 33u},   {18u, 11u},   {15u, 11u},   {64u, 33u},
    {160u, 99u},  {4u, 3u},     {3u, 2u},     {2u, 1u},
}};

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

bool ReadH264HighProfileSyntax(RbspContext& reader, uint32_t& tmp, H264SpsInfo& info)
{
    uint32_t chromaFormatIdc = 0;
    FALSE_RETURN_V(reader.RbspReadUE(chromaFormatIdc), false); // chroma_format_idc
    if (chromaFormatIdc == H264_CHROMA_FORMAT_444) {
        FALSE_RETURN_V(reader.RbspReadBit(tmp), false); // residual_color_transform_flag
    }
    info.chromaFormatIdc = chromaFormatIdc;
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false); // bit_depth_luma
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false); // bit_depth_chroma
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false); // transform_bypass
    uint32_t seqScalingMatrixPresent = 0;
    FALSE_RETURN_V(reader.RbspReadBit(seqScalingMatrixPresent), false); // present_flag
    if (!seqScalingMatrixPresent) {
        return true;
    }
    uint32_t scalingListCount = (info.chromaFormatIdc != H264_CHROMA_FORMAT_444) ?
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
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false); // log2_max_frame_num_minus4
    uint32_t picOrderCntType = 0;
    FALSE_RETURN_V(reader.RbspReadUE(picOrderCntType), false); // poc_type
    if (picOrderCntType == 0) {
        return reader.RbspReadUE(tmp); // t
    }
    if (picOrderCntType != 1) {
        return true;
    }
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false); // delta_pic_order_always_zero_flag
    int32_t signedTmp = 0;
    FALSE_RETURN_V(reader.RbspReadSE(signedTmp), false); // offset_for_non_ref_pic
    FALSE_RETURN_V(reader.RbspReadSE(signedTmp), false); // offset_for_top_to_bottom_field
    uint32_t cycleCount = 0;
    FALSE_RETURN_V(reader.RbspReadUE(cycleCount), false); // poc_cycle_length
    for (uint32_t i = 0; i < cycleCount; ++i) {
        FALSE_RETURN_V(reader.RbspReadSE(signedTmp), false); // offset_for_ref_frame
    }
    return true;
}

bool ReadH264VuiBeforeRestriction(RbspContext& reader, uint32_t& tmp, H264SpsInfo& info)
{
    uint32_t flag = 0;
    FALSE_RETURN_V(reader.RbspReadBit(flag), false); // aspect_ratio_info_present_flag
    if (flag) {
        uint32_t aspectRatioIdc = 0;
        FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, aspectRatioIdc), false); // aspect_ratio_idc
        if (aspectRatioIdc == H264_EXTENDED_SAR) {
            uint32_t sarWidth = 0;
            uint32_t sarHeight = 0;
            FALSE_RETURN_V(reader.RbspReadBits(H264_SHORT_BITS, sarWidth), false); // sar.num
            FALSE_RETURN_V(reader.RbspReadBits(H264_SHORT_BITS, sarHeight), false); // sar.den
            if (sarWidth > 0 && sarHeight > 0) {
                info.sar = static_cast<double>(sarWidth) / static_cast<double>(sarHeight);
            }
        }
        if (aspectRatioIdc > 0 && aspectRatioIdc <= static_cast<uint32_t>(H264_SAR_TABLE.size())) {
            const H264SarEntry& entry = H264_SAR_TABLE[aspectRatioIdc - 1];
            info.sar = static_cast<double>(entry.width) / static_cast<double>(entry.height);
        }
    }
    FALSE_RETURN_V(reader.RbspReadBit(flag), false); // overscan_info_present_flag
    if (flag) {
        FALSE_RETURN_V(reader.RbspReadBit(tmp), false); // overscan_appropriate_flag
    }
    FALSE_RETURN_V(reader.RbspReadBit(flag), false); // video_signal_type_present_flag
    if (flag) {
        FALSE_RETURN_V(reader.RbspReadBits(H264_CHROMA_FORMAT_444, tmp), false); // video_format
        FALSE_RETURN_V(reader.RbspReadBit(tmp), false); // video_full_range_flag
        uint32_t colourDescriptionPresent = 0;
        FALSE_RETURN_V(reader.RbspReadBit(colourDescriptionPresent), false); // colour_description_present_flag
        if (colourDescriptionPresent) {
            FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, tmp), false); // colour_primaries
            FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, tmp), false); // transfer_characteristics
            FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, tmp), false); // matrix_coeffs
        }
    }
    FALSE_RETURN_V(reader.RbspReadBit(flag), false); // chroma_loc_info_present_flag
    if (flag) {
        FALSE_RETURN_V(reader.RbspReadUE(tmp), false); // chroma_sample_loc_type_top_field
        FALSE_RETURN_V(reader.RbspReadUE(tmp), false); // chroma_sample_loc_type_bottom_field
    }
    return true;
}

bool ReadH264GeometrySyntax(RbspContext& reader, uint32_t& tmp, H264SpsInfo& info)
{
    uint32_t picWidthInMbsMinus1 = 0;
    FALSE_RETURN_V(reader.RbspReadUE(picWidthInMbsMinus1), false); // mb_width
    uint32_t picHeightInMapUnitsMinus1 = 0;
    FALSE_RETURN_V(reader.RbspReadUE(picHeightInMapUnitsMinus1), false); // mb_height
    uint32_t frameMbsOnlyFlag = 0;
    FALSE_RETURN_V(reader.RbspReadBit(frameMbsOnlyFlag), false); // frame_mbs_only_flag
    if (!frameMbsOnlyFlag) {
        FALSE_RETURN_V(reader.RbspReadBit(tmp), false); // mb_aff
    }
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false); // direct_8x8_inference_flag

    uint32_t frameCropLeftOffset = 0;
    uint32_t frameCropRightOffset = 0;
    uint32_t frameCropTopOffset = 0;
    uint32_t frameCropBottomOffset = 0;
    uint32_t frameCroppingFlag = 0;
    FALSE_RETURN_V(reader.RbspReadBit(frameCroppingFlag), false); // crop
    if (frameCroppingFlag) {
        FALSE_RETURN_V(reader.RbspReadUE(frameCropLeftOffset), false); // crop_left
        FALSE_RETURN_V(reader.RbspReadUE(frameCropRightOffset), false); // crop_right
        FALSE_RETURN_V(reader.RbspReadUE(frameCropTopOffset), false); // crop_top
        FALSE_RETURN_V(reader.RbspReadUE(frameCropBottomOffset), false); // crop_bottom
    }

    uint32_t frameMbsFactor = frameMbsOnlyFlag ? 1u : 2u;
    uint32_t width = (picWidthInMbsMinus1 + 1u) * H264_MB_SIZE;
    uint32_t height = (picHeightInMapUnitsMinus1 + 1u) * H264_MB_SIZE * frameMbsFactor;
    uint32_t subWidthC = 1u;
    uint32_t subHeightC = 1u;
    if (info.chromaFormatIdc == 1u) {
        subWidthC = 2u;
        subHeightC = 2u;
    } else if (info.chromaFormatIdc == 2u) {
        subWidthC = 2u;
    }
    uint32_t cropUnitX = (info.chromaFormatIdc == 0u || info.separateColourPlaneFlag) ? 1u : subWidthC;
    uint32_t cropUnitY = (info.chromaFormatIdc == 0u || info.separateColourPlaneFlag) ?
        frameMbsFactor : subHeightC * frameMbsFactor;
    uint32_t cropX = (frameCropLeftOffset + frameCropRightOffset) * cropUnitX;
    uint32_t cropY = (frameCropTopOffset + frameCropBottomOffset) * cropUnitY;
    if (width > cropX && height > cropY) {
        info.width = width - cropX;
        info.height = height - cropY;
    }
    return true;
}

// before max_dec_frame_buffering etc.
bool ReadH264ReorderDelayFromVui(RbspContext& reader, uint32_t& tmp, H264SpsInfo& info)
{
    FALSE_RETURN_V(ReadH264VuiBeforeRestriction(reader, tmp, info), false);
    uint32_t flag = 0;
    FALSE_RETURN_V(reader.RbspReadBit(flag), false); // timing_info_present_flag
    if (flag) {
        FALSE_RETURN_V(reader.RbspReadBits(H264_WORD_BITS, tmp), false); // num_units_in_tick
        FALSE_RETURN_V(reader.RbspReadBits(H264_WORD_BITS, tmp), false); // time_scale
        FALSE_RETURN_V(reader.RbspReadBit(tmp), false); // fixed_frame_rate_flag
    }
    FALSE_RETURN_V(reader.RbspReadBit(flag), false); // nal_hrd_parameters_present_flag
    if (flag) {
        return false;
    }
    FALSE_RETURN_V(reader.RbspReadBit(flag), false); // vcl_hrd_parameters_present_flag
    if (flag) {
        return false;
    }
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false); // pic_struct_present_flag
    FALSE_RETURN_V(reader.RbspReadBit(flag), false); // bitstream_restriction_flag
    if (!flag) {
        return false;
    }
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false); // motion_vectors_over_pic_boundaries_flag
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false); // max_bytes_per_pic_denom
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false); // max_bits_per_mb_denom
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false); // log2_max_mv_length_horizontal
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false); // log2_max_mv_length_vertical
    return reader.RbspReadUE(info.reorderDelay); // num_reorder_frames
}

bool ParseH264SpsInfo(const uint8_t* sps, size_t size, H264SpsInfo& info)
{
    FALSE_RETURN_V(sps != nullptr && size > AVC_NAL_HEADER_SIZE, false);
    RbspContext reader(sps + AVC_NAL_HEADER_SIZE, static_cast<int32_t>(size - AVC_NAL_HEADER_SIZE), true);
    uint32_t profileIdc = 0;
    uint32_t tmp = 0;
    FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, profileIdc), false); // profile_idc
    FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, tmp), false); // constraint + reserved
    FALSE_RETURN_V(reader.RbspReadBits(H264_BYTE_BITS, tmp), false); // level_idc
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false); // sps_id

    if (IsH264HighProfile(profileIdc) && !ReadH264HighProfileSyntax(reader, tmp, info)) {
        return false;
    }

    FALSE_RETURN_V(SkipH264PictureOrderSyntax(reader, tmp), false);
    FALSE_RETURN_V(reader.RbspReadUE(tmp), false); // ref_frame_count
    FALSE_RETURN_V(reader.RbspReadBit(tmp), false); // gaps_in_frame_num_allowed_flag
    FALSE_RETURN_V(ReadH264GeometrySyntax(reader, tmp, info), false);
    uint32_t vuiParametersPresent = 0;
    FALSE_RETURN_V(reader.RbspReadBit(vuiParametersPresent), false); // vui_parameters_present_flag
    if (!vuiParametersPresent) {
        return false;
    }
    return ReadH264ReorderDelayFromVui(reader, tmp, info);
}

size_t GetAnnexbStartCodeLength(const uint8_t *data, size_t offset, size_t size)
{
    if (offset + 3u <= size && data[offset] == 0x00 && data[offset + 1u] == 0x00 && data[offset + 2u] == 0x01) {
        return 3u;
    }
    if (offset + 4u <= size && data[offset] == 0x00 && data[offset + 1u] == 0x00 &&
        data[offset + 2u] == 0x00 && data[offset + 3u] == 0x01) {
        return 4u;
    }
    return 0u;
}

size_t FindAnnexbStartCode(const uint8_t *data, size_t offset, size_t size)
{
    for (size_t pos = offset; pos + 3u <= size; ++pos) {
        if (GetAnnexbStartCodeLength(data, pos, size) > 0u) {
            return pos;
        }
    }
    return size;
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
    ParseAvcConfigNalUnits(rbspContext, extraDataSize, spsAddrInfoVec, ppsAddrInfoVec);
    FALSE_RETURN_V_NOLOG(SaveAvcConfigPs(extraData, extraDataSize, spsAddrInfoVec, ppsAddrInfoVec), false);
    annexbExtraDataVec_.insert(annexbExtraDataVec_.end(), spsVec_.begin(), spsVec_.end());
    annexbExtraDataVec_.insert(annexbExtraDataVec_.end(), ppsVec_.begin(), ppsVec_.end());
    SetPsFlag();
    return true;
}

void AvcParserImpl::ParseAnnexbExtraData(const uint8_t *sample, int32_t size)
{
    FALSE_RETURN_MSG(sample != nullptr && size > 0, "Annexb sample is invalid");
    size_t sampleSize = static_cast<size_t>(size);
    size_t start = FindAnnexbStartCode(sample, 0u, sampleSize);
    while (start < sampleSize) {
        size_t startCodeLength = GetAnnexbStartCodeLength(sample, start, sampleSize);
        if (startCodeLength == 0u) {
            break;
        }
        size_t nalStart = start + startCodeLength;
        size_t nextStart = FindAnnexbStartCode(sample, nalStart, sampleSize);
        if (nalStart >= nextStart) {
            start = nextStart;
            continue;
        }
        const uint8_t *nal = sample + nalStart;
        size_t nalSize = nextStart - nalStart;
        uint8_t nalType = static_cast<uint8_t>(static_cast<uint32_t>(nal[0]) & 0x1Fu);
        if (nalType == AvcNalType::AVC_NAL_SPS) {
            H264SpsInfo info;
            bool parseSuccess = ParseH264SpsInfo(nal, nalSize, info);
            maxReorderPic_ = info.reorderDelay > 0 ? info.reorderDelay : maxReorderPic_;
            picWidInLumaSamples_ = info.width > 0 ? info.width : picWidInLumaSamples_;
            picHetInLumaSamples_ = info.height > 0 ? info.height : picHetInLumaSamples_;
            sar_ = info.sar > 0 ? info.sar : sar_;
            if (parseSuccess) {
                return;
            }
        }
        start = nextStart;
    }
}

void AvcParserImpl::ParseAvcConfigNalUnits(const std::shared_ptr<RbspContext> &rbspContext, int32_t extraDataSize,
    std::vector<DataAddrInfo> &spsAddrInfoVec, std::vector<DataAddrInfo> &ppsAddrInfoVec)
{
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
}

bool AvcParserImpl::SaveAvcConfigPs(uint8_t *extraData, int32_t extraDataSize,
    const std::vector<DataAddrInfo> &spsAddrInfoVec, const std::vector<DataAddrInfo> &ppsAddrInfoVec)
{
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
        H264SpsInfo info;
        bool parseSuccess = ParseH264SpsInfo(extraData + offset, spsLength, info);
        maxReorderPic_ = info.reorderDelay > 0 ? info.reorderDelay : maxReorderPic_;
        picWidInLumaSamples_ = info.width > 0 ? info.width : picWidInLumaSamples_;
        picHetInLumaSamples_ = info.height > 0 ? info.height : picHetInLumaSamples_;
        sar_ = info.sar > 0 ? info.sar : sar_;
        if (parseSuccess) {
            return true;
        }
        offset += spsLength;
    }
    return false;
}

void AvcParserImpl::ConvertExtradata(uint8_t **packet, int32_t &packetSize)
{
    if (annexbExtraDataVec_.empty()) {
        packet = nullptr;
        packetSize = 0;
        return;
    }
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

void AvcParserImpl::HandleNalPsState(NalUnitInfo nal)
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
}

void AvcParserImpl::HandleNalIdrDetect(NalUnitInfo nal)
{
    if (!newIdr_ && nal.nalType == AvcNalType::AVC_NAL_IDR_SLICE && (nal.size > 1 && (nal.data[1] & 0x80))) {
        newIdr_ = true;
    }
}

void AvcParserImpl::InsertPsBeforeIdr(NalUnitInfo nal)
{
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
        return;
    }
    if (!newIdr_ || nal.nalType != AvcNalType::AVC_NAL_IDR_SLICE) {
        return;
    }
    if (!idrSpsSeen_ && !idrPpsSeen_) {
        if (!spsVec_.empty()) {
            AppendOutOfBandPs(annexbFrameVec_, spsVec_);
        }
        if (!ppsVec_.empty()) {
            AppendOutOfBandPs(annexbFrameVec_, ppsVec_);
        }
        newIdr_ = false;
    } else if (idrSpsSeen_ && !idrPpsSeen_) {
        if (!ppsVec_.empty()) {
            AppendOutOfBandPs(annexbFrameVec_, ppsVec_);
        }
    }
}

void AvcParserImpl::HandleNal(NalUnitInfo nal)
{
    HandleNalPsState(nal);
    HandleNalIdrDetect(nal);
    InsertPsBeforeIdr(nal);
    PsSource ps =
        (nal.nalType == AvcNalType::AVC_NAL_SPS || nal.nalType == AvcNalType::AVC_NAL_PPS) ? PS_IN_BAND : PS_NONE;
    AppendNalUnit(annexbFrameVec_, nal.data, nal.size, ps);
    if (nal.nalType == AvcNalType::AVC_NAL_SLICE) {
        newIdr_ = true;
        idrSpsSeen_ = false;
        idrPpsSeen_ = false;
    }
}

bool AvcParserImpl::ConvertPacketCore(uint8_t **packet, int32_t &packetSize)
{
    annexbFrameVec_.clear();
    FALSE_RETURN_V_NOLOG(extradataParsed_, false);
    FilterPsFromPacket(*packet, packetSize);
    std::vector<NalUnitInfo> nalUnits;
    bool ret = ParseNalUnits(nalUnits, *packet, packetSize);
    FALSE_RETURN_V_MSG_W(ret, false, "ParseNalUnits failed");
    for (auto &nal : nalUnits) {
        HandleNal(nal);
    }
    *packet = annexbFrameVec_.data();
    packetSize = static_cast<int32_t>(annexbFrameVec_.size());
    return true;
}

bool AvcParserImpl::ConvertPacketToAnnexb(
    uint8_t **hvccPacket, int32_t &hvccPacketSize, uint8_t *sideData, size_t sideDataSize, bool isExtradata)
{
    (void)sideData;
    (void)sideDataSize;
    if (!hvccPacket || !*hvccPacket) {
        return false;
    }
    if (isExtradata) {
        ConvertExtradata(hvccPacket, hvccPacketSize);
        return true;
    } else {
        return ConvertPacketCore(hvccPacket, hvccPacketSize);
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
