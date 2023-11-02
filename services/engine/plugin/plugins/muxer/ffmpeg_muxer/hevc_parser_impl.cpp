/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "hevc_parser_impl.h"
#include <algorithm>
#include <set>

namespace {
constexpr uint8_t START_CODE[] = {0x00, 0x00, 0x01};
constexpr uint8_t EMULATION_CODE[] = {0x00, 0x00, 0x03};
constexpr int32_t SIZE_CODE_LEN = 4;

const std::set<OHOS::MediaAVCodec::Plugin::Ffmpeg::HevcParserImpl::HevcNalType> SUPPORT_NAL_TYPE = {
    OHOS::MediaAVCodec::Plugin::Ffmpeg::HevcParserImpl::HevcNalType::HEVC_VPS_NAL_UNIT,
    OHOS::MediaAVCodec::Plugin::Ffmpeg::HevcParserImpl::HevcNalType::HEVC_SPS_NAL_UNIT,
    OHOS::MediaAVCodec::Plugin::Ffmpeg::HevcParserImpl::HevcNalType::HEVC_PPS_NAL_UNIT,
    OHOS::MediaAVCodec::Plugin::Ffmpeg::HevcParserImpl::HevcNalType::HEVC_PREFIX_SEI_NAL_UNIT,
    OHOS::MediaAVCodec::Plugin::Ffmpeg::HevcParserImpl::HevcNalType::HEVC_SUFFIX_SEI_NAL_UNIT
};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
namespace Ffmpeg {
extern "C" __attribute__((visibility("default"))) HevcParserImpl *CreateHevcParser()
{
    HevcParserImpl *hevcParser = new HevcParserImpl();
    return hevcParser;
}

extern "C" __attribute__((visibility("default"))) void DestroyHevcParser(HevcParserImpl *hevcParser)
{
    delete hevcParser;
    hevcParser = nullptr;
}

HevcParserImpl::~HevcParserImpl()
{
    ResetExtraData();
}

void HevcParserImpl::ParseExtraData(const uint8_t *sample, int32_t size,
                                    uint8_t **extraDataBuf, int32_t *extraDataSize)
{
    if (IsHvccFrame(sample, size) || !IsAnnexbFrame(sample, size)) {
        return;
    }

    ResetExtraData();

    uint8_t *nalStart = const_cast<uint8_t *>(sample);
    uint8_t *end = nalStart + size;
    uint8_t *nalEnd = nullptr;
    int32_t startCodeLen = 0;

    nalStart = FindNalStartCode(nalStart, end, startCodeLen);
    nalStart = nalStart + startCodeLen;
    while (nalStart < end) {
        nalEnd = FindNalStartCode(nalStart, end, startCodeLen);
        int32_t naluSize = static_cast<int32_t>(nalEnd - nalStart);
        HevcNalType nalType = static_cast<HevcNalType>(GetNalType(nalStart[0]));
        
        if (SUPPORT_NAL_TYPE.find(nalType) != SUPPORT_NAL_TYPE.end()) {
            AddNaluData(nalStart, naluSize);
            auto iter = nalParseType_.find(nalType);
            if (iter != nalParseType_.end()) {
                std::shared_ptr<RbspContext> rbspContext = ParseRbsp(nalStart, naluSize);
                (this->*(iter->second))(rbspContext);
            }      
        }
        nalStart = nalEnd + startCodeLen;
    }

    WriteExtraData();
    *extraDataBuf = extraData_.data();
    *extraDataSize = extraData_.size();
}

void HevcParserImpl::ResetExtraData()
{
    extraDataInfo_.version_ = 0x01;
    extraDataInfo_.profileSpace_ = 0;
    extraDataInfo_.tierFlag_ = 0;
    extraDataInfo_.profileIdc_ = 0;
    extraDataInfo_.profileCompatFlags_ = 0xFFFFFFFF;
    extraDataInfo_.constraintIndicatorFlags_ = 0xFFFFFFFFFFFF;
    extraDataInfo_.levelIdc_ = 0;
    extraDataInfo_.segmentIdc_ = 0;
    extraDataInfo_.parallelismType_ = 0;
    extraDataInfo_.chromaFormat_ = 0;
    extraDataInfo_.bitDepthLuma_ = 0;
    extraDataInfo_.bitDepthChroma_ = 0;
    extraDataInfo_.avgFrameRate_ = 0;
    extraDataInfo_.constFrameRate_ = 0;
    extraDataInfo_.numTemporalLayers_ = 0;
    extraDataInfo_.temporalIdNested_ = 0;
    extraDataInfo_.lenSizeMinusOne_ = 0x03;
    extraDataInfo_.naluTypeCount_ = 0;
    extraDataInfo_.naluDataArray_.clear();
    extraData_.clear();
}

void HevcParserImpl::WriteExtraData()
{
    if (extraDataInfo_.segmentIdc_ == 0) {
        extraDataInfo_.parallelismType_ = 0;
    }

    Write(extraDataInfo_.version_);
    Write(static_cast<uint8_t>(extraDataInfo_.profileSpace_ << 0x06 | extraDataInfo_.tierFlag_ << 0x05 |
                               extraDataInfo_.profileIdc_));
    Write(extraDataInfo_.profileCompatFlags_);
    Write(static_cast<uint32_t>(extraDataInfo_.constraintIndicatorFlags_ >> 0x10));
    Write(static_cast<uint16_t>(extraDataInfo_.constraintIndicatorFlags_));
    Write(extraDataInfo_.levelIdc_);
    Write(static_cast<uint16_t>(extraDataInfo_.segmentIdc_ | 0xF000));
    Write(static_cast<uint8_t>(extraDataInfo_.parallelismType_ | 0xFC));
    Write(static_cast<uint8_t>(extraDataInfo_.chromaFormat_ | 0xFC));
    Write(static_cast<uint8_t>(extraDataInfo_.bitDepthLuma_ | 0xF8));
    Write(static_cast<uint8_t>(extraDataInfo_.bitDepthChroma_ | 0xF8));
    Write(extraDataInfo_.avgFrameRate_);
    Write(static_cast<uint8_t>(extraDataInfo_.constFrameRate_ << 0x06 | extraDataInfo_.numTemporalLayers_ << 0x03 |
                               extraDataInfo_.temporalIdNested_ << 0x02 | extraDataInfo_.lenSizeMinusOne_));
    Write(extraDataInfo_.naluTypeCount_);

    for (auto &naluData : extraDataInfo_.naluDataArray_) {
        Write(naluData.type_);
        Write(naluData.count_);
        for (auto &nalu : naluData.nalu_) {
            extraData_.insert(extraData_.end(), nalu.begin(), nalu.end());
        }
    }
}

bool HevcParserImpl::IsHvccFrame(const uint8_t *sample, int32_t size)
{
    if (size < SIZE_CODE_LEN || sample == nullptr) {
        return false;
    }

    uint32_t naluSize = 0;
    uint64_t pos = 0;
    uint64_t cmpSize = static_cast<uint64_t>(size);
    while (pos + SIZE_CODE_LEN <= cmpSize) {
        naluSize = 0;
        for (uint64_t i = pos; i < pos + SIZE_CODE_LEN; i++) {
            naluSize = (naluSize << 0x08) | sample[i];
        }

        if (naluSize <= 1) {
            return false;
        }
        pos += (naluSize + SIZE_CODE_LEN);
    }
    return pos == cmpSize;
}

bool HevcParserImpl::IsAnnexbFrame(const uint8_t *sample, int32_t size)
{
    if (size < SIZE_CODE_LEN || sample == nullptr) {
        return false;
    }

    auto *iter = std::search(sample, sample + SIZE_CODE_LEN, START_CODE, START_CODE + sizeof(START_CODE));
    if (iter == sample || (iter == sample + 1 && sample[0] == 0x00)) {
        return true;
    }

    return false;
}

void HevcParserImpl::AddNaluData(const uint8_t *buf, int32_t size)
{
    std::vector<uint8_t> data;
    data.emplace_back(static_cast<uint8_t>((size & 0xFF00) >> 0x08));
    data.emplace_back(static_cast<uint8_t>(size & 0x00FF));
    data.insert(data.end(), buf, buf + size);

    uint8_t nalType = GetNalType(buf[0]);
    auto iter = std::find_if(extraDataInfo_.naluDataArray_.begin(), extraDataInfo_.naluDataArray_.end(),
                             [nalType](const NaluData &item) { return nalType == item.type_; });
    
    if (iter != extraDataInfo_.naluDataArray_.end()) {
        iter->count_++;    
        iter->nalu_.emplace_back(data);
    } else {
        NaluData naluData;
        naluData.type_ = nalType;
        naluData.count_ = 1;
        naluData.nalu_.emplace_back(data);
        extraDataInfo_.naluDataArray_.emplace_back(naluData);
        extraDataInfo_.naluTypeCount_++;
    }
}

uint8_t *HevcParserImpl::FindNalStartCode(const uint8_t *buf, const uint8_t *end, int32_t &startCodeLen)
{
    startCodeLen = sizeof(START_CODE);
    auto *iter = std::search(buf, end, START_CODE, START_CODE + startCodeLen);
    if (iter != end) {
        if (iter > buf && *(iter - 1) == 0x00) {
            ++startCodeLen;
            return const_cast<uint8_t *>(iter - 1);
        }
    }

    return const_cast<uint8_t *>(iter);
}

uint8_t HevcParserImpl::GetNalType(uint8_t nalHeader)
{
    return (nalHeader & 0x7E) >> 1;
}

std::shared_ptr<RbspContext> HevcParserImpl::ParseRbsp(const uint8_t *buf, int32_t size)
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
    
    std::shared_ptr<RbspContext> rbspContext = std::make_shared<RbspContext>(rbspBuf);
    return rbspContext;
}

void HevcParserImpl::ParseVps(const std::shared_ptr<RbspContext> &rbspContext)
{
    /*
     * Nal Header u(16)
     * vps_video_parameter_set_id u(4)
     * vps_reserved_three_2bits   u(2)
     * vps_max_layers_minus1      u(6)
     */
    rbspContext->RbspSkipBits(0x1C);
    uint8_t maxSubLayers = rbspContext->RbspGetBits(0x03);
    extraDataInfo_.numTemporalLayers_ = std::max(extraDataInfo_.numTemporalLayers_,
                                                 static_cast<uint8_t>(maxSubLayers + 1));
    /*
     * vps_temporal_id_nesting_flag u(1)
     * vps_reserved_0xffff_16bits   u(16)
     */
    rbspContext->RbspSkipBits(0x11);
    ParseProfileTierLevel(rbspContext, maxSubLayers);
    SkipProfileTierLevelLayer(rbspContext, maxSubLayers);
}

void HevcParserImpl::ParseProfileTierLevel(const std::shared_ptr<RbspContext> &rbspContext, uint8_t maxSubLayers)
{
    extraDataInfo_.profileSpace_ = rbspContext->RbspGetBits(0x02);
    uint8_t tierFlag = rbspContext->RbspGetBits(1);
    extraDataInfo_.profileIdc_ = std::max(extraDataInfo_.profileIdc_, rbspContext->RbspGetBits(0x05));
    extraDataInfo_.profileCompatFlags_ &= rbspContext->RbspGetBits<uint32_t, uint64_t>(0x20);
    extraDataInfo_.constraintIndicatorFlags_ &= rbspContext->RbspGetBits<uint64_t, uint64_t>(0x30);
    uint8_t levelIdc = rbspContext->RbspGetBits(0x08);
    extraDataInfo_.levelIdc_ = extraDataInfo_.tierFlag_ < tierFlag ?
                               levelIdc : std::max(extraDataInfo_.levelIdc_, levelIdc);
    extraDataInfo_.tierFlag_ = std::max(extraDataInfo_.tierFlag_, tierFlag);
}

void HevcParserImpl::SkipProfileTierLevelLayer(const std::shared_ptr<RbspContext> &rbspContext, uint8_t maxSubLayers)
{
    std::vector<uint8_t> subLayerProfile;
    std::vector<uint8_t> subLayerLevel;

    for(uint8_t i = 0; i < maxSubLayers; ++i) {
        subLayerProfile.emplace_back(rbspContext->RbspGetBits(1));
        subLayerLevel.emplace_back(rbspContext->RbspGetBits(1));
    }

    if (maxSubLayers > 0) {
        for (uint8_t i = maxSubLayers; i < 0x08; ++i) {
            rbspContext->RbspSkipBits(0x02);
        }
    }

    for (uint8_t i = 0; i < subLayerProfile.size(); ++i) {
        if (subLayerProfile[i]) {
            /*
             * sub_layer_profile_space[i]                     u(2)
             * sub_layer_tier_flag[i]                         u(1)
             * sub_layer_profile_idc[i]                       u(5)
             * sub_layer_profile_compatibility_flag[i][0..31] u(32)
             * sub_layer_progressive_source_flag[i]           u(1)
             * sub_layer_interlaced_source_flag[i]            u(1)
             * sub_layer_non_packed_constraint_flag[i]        u(1)
             * sub_layer_frame_only_constraint_flag[i]        u(1)
             * sub_layer_reserved_zero_44bits[i]              u(44)
             */
            rbspContext->RbspSkipBits(0x58);
        }

        if (subLayerLevel[i]) {
            // sub_layer_level_idc[i]
            rbspContext->RbspSkipBits(0x08);
        }
    }
}

void HevcParserImpl::ParseSps(const std::shared_ptr<RbspContext> &rbspContext)
{
    rbspContext->RbspSkipBits(0x14); // Nal Header u(16), sps_video_parameter_set_id u(4)
    uint8_t maxSubLayers = rbspContext->RbspGetBits(0x03);
    extraDataInfo_.numTemporalLayers_ = std::max(extraDataInfo_.numTemporalLayers_,
                                                 static_cast<uint8_t>(maxSubLayers + 1));
    extraDataInfo_.temporalIdNested_ = rbspContext->RbspGetBits(1);
    ParseProfileTierLevel(rbspContext, maxSubLayers);
    SkipProfileTierLevelLayer(rbspContext, maxSubLayers);
    rbspContext->RbspGetUeGolomb(); // sps_seq_parameter_set_id
    extraDataInfo_.chromaFormat_ = static_cast<uint8_t>(rbspContext->RbspGetUeGolomb());
    if (extraDataInfo_.chromaFormat_ == 0x03) {
        rbspContext->RbspSkipBits(1); // separate_colour_plane_flag u(1)
    }
    SkipGolomb(rbspContext, 0x02); // pic_width_in_luma_samples, pic_height_in_luma_samples
    if (rbspContext->RbspGetBits(1)) { // conformance_window_flag u(1)
        SkipGolomb(rbspContext, 0x04); // conf_win_left_offset, conf_win_right_offset, conf_win_top_offset, conf_win_bottom_offset
    }

    extraDataInfo_.bitDepthLuma_ = static_cast<uint8_t>(rbspContext->RbspGetUeGolomb());
    extraDataInfo_.bitDepthChroma_ = static_cast<uint8_t>(rbspContext->RbspGetUeGolomb());
    uint32_t maxPicOrder = rbspContext->RbspGetUeGolomb();
    // max_dec_pic_buffering_minus1, max_num_reorder_pics, max_latency_increase_plus1
    rbspContext->RbspGetBits(1) ? SkipGolomb(rbspContext, (maxSubLayers + 1) * 0x03) : SkipGolomb(rbspContext, 0x03);
    // log2_min_luma_coding_block_size_minus3
    // log2_diff_max_min_luma_coding_block_size
    // log2_min_transform_block_size_minus2
    // log2_diff_max_min_transform_block_size
    // max_transform_hierarchy_depth_inter
    // max_transform_hierarchy_depth_intra
    SkipGolomb(rbspContext, 0x06);
    if (rbspContext->RbspGetBits(1) && rbspContext->RbspGetBits(1)) { // scaling_list_enabled_flag u(1), sps_scaling_list_data_present_flag u(1)
        SpsSkipScalingList(rbspContext);
    }
    rbspContext->RbspSkipBits(0x02); // amp_enabled_flag u(1), sample_adaptive_offset_enabled_flag u(1)

    if (rbspContext->RbspGetBits(1)) { // pcm_enabled_flag u(1)
        rbspContext->RbspSkipBits(0x08); // pcm_sample_bit_depth_luma_minus1 u(4), pcm_sample_bit_depth_chroma_minus1 u(4)
        SkipGolomb(rbspContext, 0x02); // log2_min_pcm_luma_coding_block_size_minus3, log2_diff_max_min_pcm_luma_coding_block_size
        rbspContext->RbspSkipBits(1); // pcm_loop_filter_disabled_flag u(1)
    }
    if (ParseSpsPicSet(rbspContext, maxPicOrder) < 0) {
        return;
    }
    rbspContext->RbspSkipBits(0x02); // sps_temporal_mvp_enabled_flag u(1), strong_intra_smoothing_enabled_flag u(1)
    if (rbspContext->RbspGetBits(1)) {   // vui_parameters_present_flag u(1)
        ParseVuiInfo(rbspContext, maxSubLayers);
    }
}

void HevcParserImpl::SpsSkipScalingList(const std::shared_ptr<RbspContext> &rbspContext)
{
    constexpr int32_t maxSizeId = 4;
    int32_t maxMatrixId = 0;
    for (int32_t sizeId = 0; sizeId < maxSizeId; ++sizeId) {
        maxMatrixId = sizeId == 0x03 ? 0x02 : 0x06;
        for (int32_t matrixId = 0; matrixId < maxMatrixId; ++matrixId) {
            if (rbspContext->RbspGetBits(1) != 0) {                    // scaling_list_pred_mode_flag[i][j]
                rbspContext->RbspGetUeGolomb();                        // scaling_list_pred_matrix_id_delta[i][j]
            } else {
                int32_t num = std::min(64, 1 << (0x04 + (sizeId << 1)));
                if (sizeId > 1) {
                    rbspContext->RbspGetSeGolomb();                    // scaling_list_dc_coef_minus8[i-2][j]
                }

                for (int32_t i = 0; i < num; ++i) {
                    rbspContext->RbspGetSeGolomb();                    // scaling_list_delta_coef
                }
            }
        }
    }
}

int32_t HevcParserImpl::ParseSpsPicSet(const std::shared_ptr<RbspContext> &rbspContext, uint32_t maxPicOrder)
{
    constexpr int32_t maxShortPicSets = 64;
    uint32_t numShortPicSets = rbspContext->RbspGetUeGolomb(); // num_short_term_ref_pic_sets
    if (numShortPicSets > 0x40) {
        return -1;
    }

    std::vector<uint32_t> shortPicSetsArray(maxShortPicSets, 0);
    for (uint32_t i = 0; i < numShortPicSets; ++i) {
        if (ParseRefPicSet(rbspContext, i, shortPicSetsArray) < 0) {
            return -1;
        }
    }

    if (rbspContext->RbspGetBits(1)) { // long_term_ref_pics_present_flag u(1)
        uint32_t numLongPicsSps = rbspContext->RbspGetUeGolomb(); // num_long_term_ref_pics_sps
        if (numLongPicsSps > 0x1F) {
            return -1;
        }

        for (uint32_t i = 0; i < numLongPicsSps; ++i) {
            int32_t len = std::min(static_cast<int32_t>(maxPicOrder) + 0x04, 0x10);
            // lt_ref_pic_poc_lsb_sps[i]
            rbspContext->RbspSkipBits(len);
            // used_by_curr_pic_lt_sps_flag[i]
            rbspContext->RbspSkipBits(1);
        }
    }

    return 0;
}

int32_t HevcParserImpl::ParseRefPicSet(const std::shared_ptr<RbspContext> &rbspContext, int32_t index,
                                       std::vector<uint32_t> &shortPicSetsArray)
{
    // inter_ref_pic_set_prediction_flag u(1)
    if (index > 0 && rbspContext->RbspGetBits(1)) {
        rbspContext->RbspSkipBits(1);         // delta_rps_sign u(1)
        rbspContext->RbspGetUeGolomb();       // abs_delta_rps_minus1
    
        shortPicSetsArray[index] = 0;

        for (uint32_t i = 0; i <= shortPicSetsArray[index - 1]; ++i) {
            if (!rbspContext->RbspGetBits(1)) {
                if (rbspContext->RbspGetBits(1)) {
                    shortPicSetsArray[index]++;
                }
            } else {
                shortPicSetsArray[index]++;
            }
        }
    } else {
        uint32_t negativePics = rbspContext->RbspGetUeGolomb();
        uint32_t positivePics = rbspContext->RbspGetUeGolomb();

        if (((positivePics + negativePics) * 2) > rbspContext->RbspGetLeftBitsNum()) {
            return -1;
        }
        shortPicSetsArray[index] = negativePics + positivePics;

        for (uint32_t i = 0; i < negativePics; ++i) {
            rbspContext->RbspGetUeGolomb();       // delta_poc_s0_minus1[rps_idx]
            rbspContext->RbspSkipBits(1);         // used_by_curr_pic_s0_flag u(1)
        }

        for (uint32_t i = 0; i < positivePics; ++i) {
            rbspContext->RbspGetUeGolomb();      // delta_poc_s1_minus1[rps_idx]
            rbspContext->RbspSkipBits(1);        // used_by_curr_pic_s1_flag u(1)
        }
    }

    return 0;
}

int32_t HevcParserImpl::ParseVuiInfo(const std::shared_ptr<RbspContext> &rbspContext, uint8_t maxSubLayers)
{
    ParseColorInfo(rbspContext);
    if (rbspContext->RbspGetBits(1)) { // default_display_window_flag u(1)
        // def_disp_win_left_offset
        // def_disp_win_right_offset
        // def_disp_win_top_offset
        // def_disp_win_bottom_offset
        SkipGolomb(rbspContext, 0x04);
    }

    if (rbspContext->RbspGetBits(1)) { // vui_timing_info_present_flag u(1)
        rbspContext->RbspSkipBits(0x40); // num_units_in_tick u(32), time_scale u(32)
        if (rbspContext->RbspGetBits(1)) { // poc_proportional_to_timing_flag u(1)
            rbspContext->RbspGetUeGolomb(); // num_ticks_poc_diff_one_minus1
        }
        
        if (rbspContext->RbspGetBits(1)) { // vui_hrd_parameters_present_flag u(1)
            ParseVuiHrdParams(rbspContext, 1, maxSubLayers);
        }
    }
    
    if (rbspContext->RbspGetBits(1)) { // bitstream_restriction_flag u(1)
        /*
         * tiles_fixed_structure_flag              u(1)
         * motion_vectors_over_pic_boundaries_flag u(1)
         * restricted_ref_pic_lists_flag           u(1)
         */
        rbspContext->RbspSkipBits(0x03);
        uint32_t segmentIdc = rbspContext->RbspGetUeGolomb();
        extraDataInfo_.segmentIdc_ = std::min(extraDataInfo_.segmentIdc_, static_cast<uint16_t>(segmentIdc));
        // max_bytes_per_pic_denom
        // max_bits_per_min_cu_denom
        // log2_max_mv_length_horizontal
        // log2_max_mv_length_vertical
        SkipGolomb(rbspContext, 0x04);
    }
    return 0;
}

void HevcParserImpl::ParseColorInfo(const std::shared_ptr<RbspContext> &rbspContext)
{
    if (rbspContext->RbspGetBits(1)) { // aspect_ratio_info_present_flag u(1)
        if (rbspContext->RbspGetBits(0x08) == 0xFF) { // aspect_ratio_idc u(8)
            rbspContext->RbspSkipBits(0x20); // sar_width u(16), sar_height u(16)        
        }
    }
    
    if (rbspContext->RbspGetBits(1)) { // overscan_info_present_flag u(1)
        rbspContext->RbspSkipBits(1); // overscan_appropriate_flag u(1)
    }
    
    if (rbspContext->RbspGetBits(1)) { // video_signal_type_present_flag u(1)
        rbspContext->RbspSkipBits(0x03); // video_format u(3)
        colorRange_ = rbspContext->RbspGetBits(1) == 0 ? false : true; // video_full_range_flag u(1)
        if (rbspContext->RbspGetBits(1)) { // colour_description_present_flag u(1)
            // colour_primaries u(8), transfer_characteristics u(8), matrix_coeffs u(8)
            colorPrimaries_ = rbspContext->RbspGetBits(0x08);
            colorTransfer_ = rbspContext->RbspGetBits(0x08);
            colorMatrixCoeff_ = rbspContext->RbspGetBits(0x08);
            isParserColor_ = true;
        }
    }
    
    if (rbspContext->RbspGetBits(1)) { // chroma_loc_info_present_flag u(1)
        // chroma_sample_loc_type_top_field
        // chroma_sample_loc_type_bottom_field
        SkipGolomb(rbspContext, 0x02);
    }
    /*
     * neutral_chroma_indication_flag u(1)
     * field_seq_flag                 u(1)
     * frame_field_info_present_flag  u(1)
     */
    rbspContext->RbspSkipBits(0x03);
}

void HevcParserImpl::ParseVuiHrdParams(const std::shared_ptr<RbspContext> &rbspContext, uint8_t flag,
                                       uint8_t maxSubLayers)
{
    uint8_t nalHrdFlag = 0;
    uint8_t vclHrdFlag = 0;
    uint8_t subPicFlag = 0;

    if (flag) {
        nalHrdFlag = rbspContext->RbspGetBits(1);
        vclHrdFlag = rbspContext->RbspGetBits(1);
        if (nalHrdFlag || vclHrdFlag) {
            subPicFlag = rbspContext->RbspGetBits(1);
            SkipSubPicInfo(rbspContext, subPicFlag);
        }
    }

    for (uint8_t i = 0; i < maxSubLayers; ++i) {
        uint32_t cpbCount = 0;
        uint8_t lowDelayFlag = 0;
        // fixed_pic_rate_general_flag u(1), fixed_pic_rate_within_cvs_flag u(1)
        if (!rbspContext->RbspGetBits(1) && rbspContext->RbspGetBits(1)) {
            rbspContext->RbspGetUeGolomb(); // elemental_duration_in_tc_minus1
        } else {
            lowDelayFlag = rbspContext->RbspGetBits(1);
        }

        if (!lowDelayFlag) {
            cpbCount = rbspContext->RbspGetUeGolomb();
            if (cpbCount > 0x1F) {
                return;
            }
        }

        if (nalHrdFlag) {
            SpsSkipSubLayerHrdParams(rbspContext, cpbCount, subPicFlag);
        }

        if (vclHrdFlag) {
            SpsSkipSubLayerHrdParams(rbspContext, cpbCount, subPicFlag);
        }
    }
}

void HevcParserImpl::SkipSubPicInfo(const std::shared_ptr<RbspContext> &rbspContext, uint8_t subPicFlag)
{
    if (subPicFlag) {
        /*
         * tick_divisor_minus2                          u(8)
         * du_cpb_removal_delay_increment_length_minus1 u(5)
         * sub_pic_cpb_params_in_pic_timing_sei_flag    u(1)
         * dpb_output_delay_du_length_minus1            u(5)
         */
        rbspContext->RbspSkipBits(0x13);
    }
    
    rbspContext->RbspSkipBits(0x08); // bit_rate_scale u(4), cpb_size_scale u(4)
    if (subPicFlag) {
        rbspContext->RbspSkipBits(0x04);  // cpb_size_du_scale u(4)
    }
    
    /*
     * initial_cpb_removal_delay_length_minus1 u(5)
     * au_cpb_removal_delay_length_minus1 u(5)
     * dpb_output_delay_length_minus1 u(5)
     */
    rbspContext->RbspSkipBits(0x0F);
}

void HevcParserImpl::SpsSkipSubLayerHrdParams(const std::shared_ptr<RbspContext> &rbspContext, uint32_t cpbCount,
                                              uint8_t subPicFlag)
{
    for (uint32_t i = 0; i <= cpbCount; ++i) {
        SkipGolomb(rbspContext, 0x02); // bit_rate_value_minus1, cpb_size_value_minus1

        if (subPicFlag) {
            SkipGolomb(rbspContext, 0x02); // cpb_size_du_value_minus1, bit_rate_du_value_minus1
        }

        rbspContext->RbspSkipBits(1); // cbr_flag u(1)
    }
}

void HevcParserImpl::ParsePps(const std::shared_ptr<RbspContext> &rbspContext)
{
    rbspContext->RbspSkipBits(0x10); // Nal Header u(16)
    SkipGolomb(rbspContext, 0x02); // pps_pic_parameter_set_id, pps_seq_parameter_set_id
    /*
     * dependent_slice_segments_enabled_flag u(1)
     * output_flag_present_flag              u(1)
     * num_extra_slice_header_bits           u(3)
     * sign_data_hiding_enabled_flag         u(1)
     * cabac_init_present_flag               u(1)
     */
    rbspContext->RbspSkipBits(0x07);
    
    SkipGolomb(rbspContext, 0x02); // num_ref_idx_l0_default_active_minus1, num_ref_idx_l1_default_active_minus1
    rbspContext->RbspGetSeGolomb(); // init_qp_minus26
    /*
     * constrained_intra_pred_flag u(1)
     * transform_skip_enabled_flag u(1)
     */
    rbspContext->RbspSkipBits(0x02);

    if (rbspContext->RbspGetBits(1)) { // cu_qp_delta_enabled_flag u(1)
        rbspContext->RbspGetUeGolomb(); // diff_cu_qp_delta_depth
    }

    rbspContext->RbspGetSeGolomb(); // pps_cb_qp_offset
    rbspContext->RbspGetSeGolomb(); // pps_cr_qp_offset

    /*
     * pps_slice_chroma_qp_offsets_present_flag u(1)
     * weighted_pred_flag               u(1)
     * weighted_bipred_flag             u(1)
     * transquant_bypass_enabled_flag   u(1)
     */
    rbspContext->RbspSkipBits(0x04);

    ParseParallelismType(rbspContext);
}

void HevcParserImpl::ParseParallelismType(const std::shared_ptr<RbspContext> &rbspContext)
{
    uint8_t tilesEnabledFlag = rbspContext->RbspGetBits(1);
    uint8_t entropyEnabledFlag = rbspContext->RbspGetBits(1);

    if (entropyEnabledFlag && tilesEnabledFlag) {
        extraDataInfo_.parallelismType_ = 0;    // mixed-type parallel decoding
    } else if (entropyEnabledFlag) {
        extraDataInfo_.parallelismType_ = 0x03;    // wavefront-based parallel decoding
    } else if (tilesEnabledFlag) {
        extraDataInfo_.parallelismType_ = 0x02;    // tile-based parallel decoding
    } else {
        extraDataInfo_.parallelismType_ = 0x01;    // slice-based parallel decoding
    }
}

void HevcParserImpl::SkipGolomb(const std::shared_ptr<RbspContext> &rbspContext, int32_t num)
{
    for (int32_t i = 0; i < num; ++i) {
        rbspContext->RbspGetUeGolomb();
    }
}

RbspContext::RbspContext(const std::vector<uint8_t> &buf)
{
    buf_.assign(buf.begin(), buf.end());
    size_ = static_cast<int32_t>(buf_.size());
    byteIndex_ = 0;
    bitIndex_ = 0;
}

RbspContext::~RbspContext()
{
    buf_.clear();
}

bool RbspContext::RbspCheckSize(int32_t size)
{
    if (byteIndex_ >= size_) {
        return false;
    }

    int32_t byteIndex = byteIndex_ + (bitIndex_ + size) / 0x08;
    int32_t leftBitIndex = (bitIndex_ + size) % 0x08;
    if (byteIndex > size_ || (byteIndex == size_ && leftBitIndex > 0)) {
        return false;
    }
    return true;
}

void RbspContext::RbspSkipBits(int32_t size)
{
    if (byteIndex_ >= size_) {
        return;
    }

    byteIndex_ = byteIndex_ + (bitIndex_ + size) / 0x08;
    bitIndex_ = (bitIndex_ + size) % 0x08;
}

uint32_t RbspContext::RbspGetUeGolomb()
{
    constexpr int32_t maxSize = 32;
    int32_t size = 0;
    while (size < maxSize && RbspGetBit() == 0) {
        ++size;
        RbspSkipBits(1);
    }
    size == maxSize ? --size : size;
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

uint8_t RbspContext::RbspGetBit()
{
    uint8_t *buf = buf_.data();
    uint8_t data = *(reinterpret_cast<uint8_t *>(buf + byteIndex_));
    data = (data >> (0x07 - bitIndex_)) & 0x01;
    return data;
}

int32_t RbspContext::RbspGetLeftBitsNum()
{
    if (byteIndex_ >= size_) {
        return 0;
    }

    return  (size_ - byteIndex_) * 0x08 - bitIndex_;
}
} // namespace Ffmpeg
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS