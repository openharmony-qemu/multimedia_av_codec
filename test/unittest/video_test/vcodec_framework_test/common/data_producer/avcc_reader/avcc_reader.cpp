/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "avcc_reader.h"
#include <algorithm>
#include <functional>
#include <thread>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "securec.h"
#include "unittest_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "AvccReader"};
constexpr uint8_t AVCC_FRAME_HEAD_LEN = 4;
constexpr uint8_t ANNEXB_FRAME_HEAD[] = {0, 0, 1};
constexpr uint8_t ANNEXB_FRAME_HEAD_LEN = sizeof(ANNEXB_FRAME_HEAD);
constexpr uint32_t MAX_NALU_SIZE = 2 * 1024 * 1024; // 2Mb

constexpr uint8_t MPEG2_FRAME_HEAD[] = {0x00, 0x00, 0x01, 0x00};
constexpr uint8_t MPEG2_FRAME_HEAD_LEN = sizeof(MPEG2_FRAME_HEAD);
constexpr uint8_t MPEG2_SEQUENCE_HEAD[] = {0x00, 0x00, 0x01, 0xb3};
constexpr uint8_t MPEG2_SEQUENCE_HEAD_LEN = sizeof(MPEG2_SEQUENCE_HEAD);
constexpr uint8_t MPEG2_TYPE_OFFEST = 1;
constexpr uint8_t MPEG4_FRAME_HEAD[] = {0x00, 0x00, 0x01, 0xb6};
constexpr uint8_t MPEG4_FRAME_HEAD_LEN = sizeof(MPEG4_FRAME_HEAD);
constexpr uint8_t MPEG4_SEQUENCE_HEAD[] = {0x00, 0x00, 0x01, 0xb0};
constexpr uint8_t MPEG4_SEQUENCE_HEAD_LEN = sizeof(MPEG4_SEQUENCE_HEAD);
constexpr uint32_t PREREAD_BUFFER_SIZE = 5 * 1024 * 1024;
#ifdef SUPPORT_CODEC_VC1
constexpr uint8_t VC1_FRAME_HEAD[] = {0x00, 0x00, 0x01, 0x0D};
constexpr uint8_t VC1_FRAME_HEAD_LEN = sizeof(VC1_FRAME_HEAD);
constexpr uint8_t VC1_SEQUENCE_HEAD[] = {0x00, 0x00, 0x01, 0x0F};
constexpr uint8_t VC1_SEQUENCE_HEAD_LEN = sizeof(VC1_SEQUENCE_HEAD);
constexpr uint8_t VC1_FRAME_TYPE_OFFSET = 4;
constexpr uint8_t VC1_FRAME_TYPE_MASK = 0xC0;
constexpr uint8_t VC1_FRAME_TYPE_I_BITS = 0x00;
constexpr uint8_t VC1_FRAME_TYPE_P_BITS = 0x40;
constexpr uint8_t VC1_FRAME_TYPE_B_BITS = 0x80;
constexpr uint8_t VC1_FRAME_TYPE_BI_BITS = 0xC0;
#endif
constexpr uint8_t H263_HEAD_0[] = {0x00, 0x00, 0x80};
constexpr uint8_t H263_HEAD_1[] = {0x00, 0x00, 0x81};
constexpr uint8_t H263_HEAD_2[] = {0x00, 0x00, 0x82};
constexpr uint8_t H263_HEAD_3[] = {0x00, 0x00, 0x83};
constexpr uint8_t H263_HEAD_LEN = sizeof(H263_HEAD_0);
constexpr uint8_t H263_HEAD_MASK_4_1 = 0x1c;
constexpr uint8_t H263_HEAD_MASK_4_2 = 0x02;
constexpr uint8_t H263_HEAD_MASK_5_1 = 0x80;
constexpr uint8_t H263_HEAD_MASK_5_2 = 0x70;
constexpr uint8_t H263_OFFSET_4 = 4;
constexpr uint8_t H263_OFFSET_5 = 5;
constexpr uint8_t H263_OFFSET_7 = 7;

static inline int64_t GetTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    // 1000'000: second to micro second; 1000: nano second to micro second
    return (static_cast<int64_t>(now.tv_sec) * 1000'000 + (now.tv_nsec / 1000));
}

enum AvcNalType {
    AVC_UNSPECIFIED = 0,
    AVC_NON_IDR = 1,
    AVC_PARTITION_A = 2,
    AVC_PARTITION_B = 3,
    AVC_PARTITION_C = 4,
    AVC_IDR = 5,
    AVC_SEI = 6,
    AVC_SPS = 7,
    AVC_PPS = 8,
    AVC_AU_DELIMITER = 9,
    AVC_END_OF_SEQUENCE = 10,
    AVC_END_OF_STREAM = 11,
    AVC_FILLER_DATA = 12,
    AVC_SPS_EXT = 13,
    AVC_PREFIX = 14,
    AVC_SUB_SPS = 15,
    AVC_DPS = 16,
};

enum HevcNalType {
    HEVC_TRAIL_N = 0,
    HEVC_TRAIL_R = 1,
    HEVC_TSA_N = 2,
    HEVC_TSA_R = 3,
    HEVC_STSA_N = 4,
    HEVC_STSA_R = 5,
    HEVC_RADL_N = 6,
    HEVC_RADL_R = 7,
    HEVC_RASL_N = 8,
    HEVC_RASL_R = 9,
    HEVC_BLA_W_LP = 16,
    HEVC_BLA_W_RADL = 17,
    HEVC_BLA_N_LP = 18,
    HEVC_IDR_W_RADL = 19,
    HEVC_IDR_N_LP = 20,
    HEVC_CRA_NUT = 21,
    HEVC_VPS_NUT = 32,
    HEVC_SPS_NUT = 33,
    HEVC_PPS_NUT = 34,
    HEVC_AUD_NUT = 35,
    HEVC_EOS_NUT = 36,
    HEVC_EOB_NUT = 37,
    HEVC_FD_NUT = 38,
    HEVC_PREFIX_SEI_NUT = 39,
    HEVC_SUFFIX_SEI_NUT = 40,
};
enum Mpeg2Type {
    M2V_UNSPECIFIED = 0,
    M2V_I = 1,
    M2V_P = 2,
    M2V_B = 3,
};
enum Mpeg4Type {
    MPEG4_UNSPECIFIED = 0,
    MPEG4_I = 1,
    MPEG4_P = 2,
    MPEG4_B = 3,
    MPEG4_S = 4,
};

enum H263Type {
    H263_I = 1,
};

}
#ifdef SUPPORT_CODEC_VC1
enum Vc1Type {
    VC1_UNSPECIFIED = 0,
    VC1_I = 1,
    VC1_P = 2,
    VC1_B = 3,
    VC1_BI = 4,
    VC1_SEQUENCE_HEADER = 5
};

const uint32_t ES_VC1[] = {
    71081,   872,  9379,   873,  7636,  2986, 12386,  2681, 11515,  3501,
    15447,  5825, 31644,   974,  6955,  1109, 11921,  3872, 21700,  6866,
    25652,  3342, 12210,  4197, 26221,  3458, 17692,  8808, 22743,  6121,
    11523,  6179, 22633,  7516, 18487,  8944, 22299,  7144, 13954,  9655,
    14014,  9049, 24359,  7392, 14676,  7329, 14565,  8923, 28404,  5313,
    6847,  8560, 19182,  8421, 13264, 10958, 13251, 12530, 29942,  2791,
    9097,  8470, 18520,  8780, 16887,  5324, 22401,  2892, 14301,  3613,
    16986,  4114, 11144,  4059, 24930, 11041, 15902,  7232, 19542,  5389,
    16219, 12110, 18493,  8535, 14945,  9285, 17928,  9122, 14303,  9261,
    17332,  8728, 17236,  9626, 18029,  8883, 14945,  9593, 21497,  6390,
    64545, 10327, 24077,  5530, 11122,  6549, 17798,  5695, 17412,  7018,
    20456,  6944, 17396,  8961, 21515,  6663, 14276,  7537, 18621,  5730,
    19280,  9244, 23206,  8465, 14002,  8156, 20449,  6302, 16562,  7690,
    18290, 10085, 17183,  8373, 19956,  6743, 16892,  8294, 18638,  9820,
    14532,  7499, 18569,  5908, 16738,  6519, 18738,  4992, 16174,  7342,
    16226,  6638, 15332,  7388, 15707,  6931, 15729,  8090, 16553,  6486,
    14363,  8398, 15360,  9129, 19442,  6968, 13829,  5722, 13051,  6121,
    14784,  7229, 17461,  7132, 17121,  6489, 16583,  6504, 14229,  9211};

const uint32_t ES_VC1_LENGTH = sizeof(ES_VC1) / sizeof(ES_VC1[0]);
#endif
enum Msvideo1Type {
    MSVIDEO1_UNSPECIFIED = 0,
    MSVIDEO1_I = 1,
    MSVIDEO1_P = 2,
    MSVIDEO1_B = 3,
    MSVIDEO1_SEQUENCE_HEADER = 4
};

const uint32_t ES_MSVIDEO1[] = { // MSVideo1_FRAME_SIZE
    71138, 688, 464, 516, 544, 1068, 918, 918, 2064, 1930, 2054, 5586, 8944, 4166, 2408, 2412, 3992, 2920, 6298, 10034,
    7966, 5232, 6034, 6810, 6056, 4596, 68262, 4710, 6898, 9684, 7652, 8212, 7346, 8180, 8442, 12220, 14172, 12276,
    9654, 13196, 9256, 11018, 13604, 17192, 16130, 15114, 13782, 13178, 15818, 16344, 17206, 18240, 67830, 9656, 17508,
    15316, 17730, 22148, 17666, 23746, 24178, 20300, 21336, 20726, 20424, 22642, 24638, 26080, 26420, 25320, 25918,
    25494, 24944, 25014, 25174, 29054, 27588, 27146, 68198, 21700, 25190, 28046, 27862, 29896, 30380, 29384, 28690,
    30040, 28378, 30026, 29556, 30210, 30360, 29440, 30268, 30362, 29960, 31402, 29878, 31522, 30636, 30468, 29894,
    30820, 67378, 27712, 30714, 31158, 31278, 31096, 30618, 30368, 30762, 31456, 30958, 32154, 31696, 31016, 31070,
    30932, 31736, 31072, 31274, 33122, 32108, 31686, 30804, 32294, 31804, 30386, 67490, 29728, 32366, 30910, 29974,
    30070, 29994, 28146, 28334, 32336, 31624, 29350, 29084, 27642, 28070, 24904, 24678, 28902, 28248, 26988, 24898,
    24388, 25856, 21954, 21776, 26250, 64678, 21098, 22818, 21480, 21980, 20350, 21144, 23298, 23008, 22654, 20398,
    21104, 20986, 18238, 18862, 21706, 21084, 20206, 17766, 16858, 18586, 14624, 16092, 18722, 19234, 17398, 63626,
    8282, 14918, 11484, 12232, 15508, 15368, 14188, 12606, 11328, 13924, 6552, 8518, 12632, 13360, 12570, 10452, 7778,
    9474, 6762, 8088, 11616, 10508, 11442, 8606, 7610, 61926, 2676, 7436, 6508, 7538, 7154, 6554, 7634, 5628, 7750,
    7766, 5290, 6674, 6308, 7332, 5682, 5502, 7330, 7496, 6876, 5432, 7854, 8146, 6720, 5920, 7322, 61078, 3004, 5356,
    5908, 6620, 5956, 7964, 6310, 7210, 6372, 5984, 6752, 7284, 6514, 5998, 4418, 14026, 4604, 6792, 6688, 6432, 7440,
    7886, 6588, 7366, 6812, 60774, 2896, 5212, 5452, 7020, 7790, 7228, 7300, 7850, 6802, 6964, 6314, 8220, 7576, 6608,
    8436, 8576, 8038, 7478, 7062, 7924, 7856, 7364, 7324, 8824, 9036, 60334, 2126, 5754, 8436, 6314, 5908, 6990, 6796,
    5120, 7104, 7204, 7258, 5566, 6968, 8288, 8204, 5680, 7092, 7878, 7776, 5258, 7376, 7588, 7720, 5366, 7194, 59466,
    2456, 3988, 5420, 6186, 6088, 4924, 6798, 7118, 6912, 4498, 7474, 8588, 7472, 5566, 7500, 8094, 7910, 6460, 9642,
    10266, 9160, 8528, 8630, 9314, 10738, 60462, 4218, 9972, 9250, 7432, 8712, 8976, 10664, 8394, 10820, 10870, 9926,
    8308, 9516, 10138, 11578, 7908, 10484, 9682, 10282, 8206, 9890, 10666, 10830, 8606, 9502, 60806, 2928, 7118, 9030,
    10226, 10326, 9166, 10496, 11342, 10556, 8966, 11048, 10532, 11146, 9730, 9840, 11244, 10960, 10282, 10106, 12926,
    9746, 10534, 9782, 11342, 10034, 62318, 2864, 8954, 9192, 10618, 9472, 12590, 10174, 11296, 10164, 12908, 11196,
    12080, 4812, 12742, 12934, 11012, 11560, 12074, 12850, 12062, 12096, 11674, 12208, 11086, 12352, 62890, 5378, 11476,
    10738, 13498, 13750, 11678, 13058, 12630, 14618, 13146, 13178, 12768, 13374, 12966, 12508, 14550, 13402, 13610,
    13496, 15460, 15374, 14924, 13892, 14760, 15878, 64830, 9660, 13818, 15586, 15076, 14938, 15738, 17072, 15752,
    15056, 15106, 16312, 14920, 15254, 14520, 16500, 14806, 14242, 14210, 17484, 15878, 14462, 15830, 17402, 16380,
    16572, 66482, 10940, 17428, 16970, 18156, 19136, 19412, 18638, 19152, 19994, 18702, 18098, 21932, 19568, 18826,
    19462, 19892, 21224, 19636, 18894, 19876, 20992, 19578, 18752, 19722, 20526, 69038, 12008, 18874, 20090, 20388,
    17808, 32954, 21126, 23562, 26324, 24672, 23910, 25462, 24210, 27064, 23206, 25686, 25332, 24742, 23390, 25786,
    23976, 24662, 22860, 24206, 23530, 75542, 17016, 20868, 21604, 24196, 21862, 23562, 22888, 21448, 21558, 22970,
    22984, 23678, 25268, 26106, 25474, 24016, 23820, 26480, 27820, 30348, 28076, 28822, 28672, 27110, 27800, 74434,
    20882, 29404, 27228, 28230, 29322, 27158, 28474, 29020, 30418, 27182, 29326, 30484, 29374, 28798, 28416, 31634,
    29860, 27748, 28356, 29606, 30096, 28078, 29444, 29550, 29550, 76210, 19970, 27300, 28694, 26852, 28906, 27834,
    28582, 27722, 25816, 26912, 26614, 26688, 26922, 28272, 28162, 27338, 25322, 26546, 26824, 25518, 25796, 27514,
    25622, 27078, 25534, 74270, 17090, 24448, 23858};

const uint32_t ES_MSVIDEO1_LENGTH = sizeof(ES_MSVIDEO1) / sizeof(ES_MSVIDEO1[0]);
namespace OHOS {
namespace MediaAVCodec {

const uint32_t ES_WMV3_NORMAL[] = {
    9673, 2294, 3049, 2229, 2859, 2735, 2901, 2881, 3056, 3570,
    2583, 2636, 2708, 2426, 2226, 2892, 3104, 3224, 2466, 2590,
    3371, 2963, 2768, 3152, 3377, 4311, 4512, 3991, 3692, 2715,
    3297, 3482, 3461, 3672, 4766, 3194, 3750, 3731, 4622, 3228,
    4155, 5918, 5879, 6822, 5330, 6465, 5614, 5274, 5416, 3847,
    5176, 3806, 4520, 4829, 3375, 3546, 3483, 4206, 4693, 3826,
    3812
};
const uint32_t ES_WMV3_NORMAL_LEN = sizeof(ES_WMV3_NORMAL) / sizeof(ES_WMV3_NORMAL[0]);

const uint32_t ES_WMV3_MAIN[] = {
    96567, 28373, 28624, 29924, 28891, 29772, 30359, 29766, 31042, 34215,
    30812, 33766, 32173, 23693, 29794, 29307, 31815, 31160, 32350, 32360,
    23243, 31439, 32176, 31647, 31064, 31641, 33174, 21929, 29088, 31025,
    29285, 29210, 30145, 30843, 31814, 30112, 29904, 31500, 32499, 30658,
    29975, 29324, 28694, 30066, 29851, 29234, 50921, 29758, 32406, 29906,
    28567, 29755, 31127, 29116, 51264, 31911, 31241, 28658, 30101, 30472,
    30589
};
const uint32_t ES_WMV3_MAIN_LEN = sizeof(ES_WMV3_MAIN) / sizeof(ES_WMV3_MAIN[0]);

int32_t MpegReader::Init(const std::shared_ptr<MpegReaderInfo> &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<std::ifstream> inputFile = std::make_unique<std::ifstream>(info->inPath.data(),
        std::ios::binary | std::ios::in);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputFile != nullptr && inputFile->is_open(),
                                      AV_ERR_INVALID_VAL, "Open input file failed");

    mpegUnitReader_ = info->isMpeg2Stream ?
        std::static_pointer_cast<MpegUnitReader>(std::make_shared<Mpeg2MetaUnitReader>(inputFile)) :
        std::static_pointer_cast<MpegUnitReader>(std::make_shared<Mpeg4MetaUnitReader>(inputFile));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mpegUnitReader_, AV_ERR_INVALID_VAL, "Mpeg unit reader create failed");

    mpegDetector_ = info->isMpeg2Stream ?
        std::static_pointer_cast<MpegDetector>(std::make_shared<Mpeg2Detector>()) :
        std::static_pointer_cast<MpegDetector>(std::make_shared<Mpeg4Detector>());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mpegDetector_, AV_ERR_INVALID_VAL, "Mpeg detector create failed");

    return AV_ERR_OK;
}

void MpegReader::FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t mpegType,
                                bool isEosFrame)
{
    attr.size += frameSize;
    attr.pts = GetTimeUs();
    attr.flags |= mpegDetector_->IsI(mpegType) ? AVCODEC_BUFFER_FLAG_SYNC_FRAME : 0;
    if (isEosFrame) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        std::cout << "Input EOS Frame, frameCount = " << (frameInputCount_ + 1) << std::endl;
    }
}

int32_t MpegReader::FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int32_t frameSize = 0;
    bool isEosFrame = false;
    auto ret = mpegUnitReader_->ReadMpegUnit(bufferAddr, frameSize, isEosFrame);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ReadMpegUnit failed");
    uint8_t mpegType = mpegDetector_->GetMpegType(mpegDetector_->GetMpegTypeAddr(bufferAddr));
    bufferAddr += frameSize;
    FillBufferAttr(attr, frameSize, mpegType, isEosFrame);

    frameInputCount_++;

    return AV_ERR_OK;
}

bool MpegReader::IsEOS()
{
    return mpegUnitReader_ ? mpegUnitReader_->IsEOS() : true;
}

uint8_t const * MpegReader::MpegUnitReader::GetNextMpegUnitAddr()
{
    CHECK_AND_RETURN_RET_LOG(mpegUnit_ != nullptr, nullptr, "mpegUnit_ is nullptr");
    return mpegUnit_->data();
}

void MpegReader::Mpeg2MetaUnitReader::PrereadFile()
{
    CHECK_AND_RETURN_LOG(prereadBuffer_, "Preread buffer is nallptr");
    inputFile_->read(reinterpret_cast<char *>(prereadBuffer_.get() + MPEG2_FRAME_HEAD_LEN), PREREAD_BUFFER_SIZE);
    prereadBufferSize_ = inputFile_->gcount() + MPEG2_FRAME_HEAD_LEN;
    pPrereadBuffer_ = MPEG2_FRAME_HEAD_LEN;
}

int32_t MpegReader::Mpeg2MetaUnitReader::ReadMpegUnit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL, "Got a invalid buffer addr");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mpegUnit_, AV_ERR_INVALID_VAL, "Mpeg unit buffer is nullptr");
    bufferSize = mpegUnit_->size();
    memcpy_s(bufferAddr, bufferSize, mpegUnit_->data(), bufferSize);

    if (!IsEOF()) {
        isEosFrame = false;
        PrereadMpeg2Unit();
    } else {
        isEosFrame = true;
        mpegUnit_->resize(0);
    }
    return AV_ERR_OK;
}

MpegReader::Mpeg2MetaUnitReader::Mpeg2MetaUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    prereadBuffer_ = std::make_unique<uint8_t []>(PREREAD_BUFFER_SIZE + MPEG2_FRAME_HEAD_LEN);
    PrereadFile();

    mpegUnit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadMpeg2Unit();
}

bool MpegReader::Mpeg2MetaUnitReader::IsEOS()
{
    return IsEOF() && mpegUnit_->empty();
}

bool MpegReader::Mpeg2MetaUnitReader::IsEOF()
{
    return (pPrereadBuffer_ == prereadBufferSize_) && (inputFile_->peek() == EOF);
}

void MpegReader::Mpeg2MetaUnitReader::PrereadMpeg2Unit()
{
    CHECK_AND_RETURN_LOG(prereadBufferSize_ > 0, "Empty file, nothing to read");
    CHECK_AND_RETURN_LOG(mpegUnit_, "Mpeg2 unit buffer is nullptr");
    auto pBuffer = mpegUnit_->data();
    uint32_t bufferSize = 0;
    mpegUnit_->resize(MAX_NALU_SIZE);
    do {
        auto pos1 = std::search(prereadBuffer_.get() + pPrereadBuffer_ + MPEG2_FRAME_HEAD_LEN,
            prereadBuffer_.get() + prereadBufferSize_, std::begin(MPEG2_FRAME_HEAD), std::end(MPEG2_FRAME_HEAD));
        uint32_t size1 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos1);
        auto pos2 = std::search(prereadBuffer_.get() + pPrereadBuffer_, prereadBuffer_.get() +
            pPrereadBuffer_ + size1, std::begin(MPEG2_SEQUENCE_HEAD), std::end(MPEG2_SEQUENCE_HEAD));
        uint32_t size = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos2);
        if (size == 0) {
            auto pos3 = std::search(prereadBuffer_.get() + pPrereadBuffer_ + size1 + MPEG2_FRAME_HEAD_LEN,
                prereadBuffer_.get() + prereadBufferSize_, std::begin(MPEG2_FRAME_HEAD), std::end(MPEG2_FRAME_HEAD));
            uint32_t size2 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos3);
            auto ret = memcpy_s(pBuffer, size2, prereadBuffer_.get() + pPrereadBuffer_, size2);
            CHECK_AND_RETURN_LOG(ret == EOK, "First Copy buffer failed");
            pPrereadBuffer_ += size2;
            bufferSize += size2;
            pBuffer += size2;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        } else if (size1 > size) {
            auto ret = memcpy_s(pBuffer, size, prereadBuffer_.get() + pPrereadBuffer_, size);
            CHECK_AND_RETURN_LOG(ret == EOK, "Last Copy buffer failed");
            pPrereadBuffer_ += size;
            bufferSize += size;
            pBuffer += size;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        } else {
            auto ret = memcpy_s(pBuffer, size1, prereadBuffer_.get() + pPrereadBuffer_, size1);
            CHECK_AND_RETURN_LOG(ret == EOK, "Comom Copy buffer failed");
            pPrereadBuffer_ += size1;
            bufferSize += size1;
            pBuffer += size1;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        }
        PrereadFile();
        auto ret = memcpy_s(prereadBuffer_.get(), MPEG2_FRAME_HEAD_LEN, pBuffer - MPEG2_FRAME_HEAD_LEN,
            MPEG2_FRAME_HEAD_LEN);
        CHECK_AND_RETURN_LOG(ret == EOK, "Buffer End Copy buffer failed");
        bufferSize -= MPEG2_FRAME_HEAD_LEN;
        pBuffer -= MPEG2_FRAME_HEAD_LEN;
        pPrereadBuffer_ = 0;
    } while (pPrereadBuffer_ != prereadBufferSize_);
    mpegUnit_->resize(bufferSize);
}

const uint8_t *MpegReader::Mpeg2Detector::GetMpegTypeAddr(const uint8_t *bufferAddr)
{
    auto pos1 = std::search(bufferAddr, bufferAddr + MPEG2_SEQUENCE_HEAD_LEN + 1,
        std::begin(MPEG2_SEQUENCE_HEAD), std::end(MPEG2_SEQUENCE_HEAD));
    auto size = std::distance(bufferAddr, pos1);
    if (size == 0) {
        return nullptr;
    }
    auto pos = std::search(bufferAddr, bufferAddr + MPEG2_FRAME_HEAD_LEN + 1,
        std::begin(MPEG2_FRAME_HEAD), std::end(MPEG2_FRAME_HEAD));
    return pos + MPEG2_FRAME_HEAD_LEN + MPEG2_TYPE_OFFEST;
}

bool MpegReader::Mpeg2Detector::IsI(uint8_t mpegType)
{
    return (mpegType == M2V_I) ? true : false;
}
uint8_t MpegReader::Mpeg2Detector::GetMpegType(const uint8_t *bufferAddr)
{
    if (bufferAddr == nullptr) {
        return 1;
    }
    uint8_t flagi =  ((*bufferAddr) & 0x38) == 0x08;
    uint8_t flagp =  ((*bufferAddr) & 0x38) == 0x08;
    uint8_t flagb =  ((*bufferAddr) & 0x38) == 0x18;
    if (flagi) {
        return 1;
    }
    if (flagp) {
        return 0;
    }
    if (flagb) {
        return 0;
    }
    return 0;
}

void MpegReader::Mpeg4MetaUnitReader::PrereadFile()
{
    CHECK_AND_RETURN_LOG(prereadBuffer_, "Preread buffer is nallptr");
    inputFile_->read(reinterpret_cast<char *>(prereadBuffer_.get() + MPEG4_FRAME_HEAD_LEN),
        PREREAD_BUFFER_SIZE);
    prereadBufferSize_ = inputFile_->gcount() + MPEG4_FRAME_HEAD_LEN;
    pPrereadBuffer_ = MPEG2_FRAME_HEAD_LEN;
}

int32_t MpegReader::Mpeg4MetaUnitReader::ReadMpegUnit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL, "Got a invalid buffer addr");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mpegUnit_, AV_ERR_INVALID_VAL, "Mpeg unit buffer is nullptr");
    bufferSize = mpegUnit_->size();
    memcpy_s(bufferAddr, bufferSize, mpegUnit_->data(), bufferSize);

    if (!IsEOF()) {
        isEosFrame = false;
        PrereadMpeg4Unit();
    } else {
        isEosFrame = true;
        mpegUnit_->resize(0);
    }
    return AV_ERR_OK;
}

MpegReader::Mpeg4MetaUnitReader::Mpeg4MetaUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    prereadBuffer_ = std::make_unique<uint8_t []>(PREREAD_BUFFER_SIZE + MPEG4_FRAME_HEAD_LEN);
    PrereadFile();

    mpegUnit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadMpeg4Unit();
}

bool MpegReader::Mpeg4MetaUnitReader::IsEOS()
{
    return IsEOF() && mpegUnit_->empty();
}

bool MpegReader::Mpeg4MetaUnitReader::IsEOF()
{
    return (pPrereadBuffer_ == prereadBufferSize_) && (inputFile_->peek() == EOF);
}

void MpegReader::Mpeg4MetaUnitReader::PrereadMpeg4Unit()
{
    CHECK_AND_RETURN_LOG(prereadBufferSize_ > 0, "Empty file, nothing to read");
    CHECK_AND_RETURN_LOG(mpegUnit_, "Mpeg4 unit buffer is nullptr");
    auto pBuffer = mpegUnit_->data();
    uint32_t bufferSize = 0;
    mpegUnit_->resize(MAX_NALU_SIZE);
    do {
        auto pos1 = std::search(prereadBuffer_.get() + pPrereadBuffer_ + MPEG4_FRAME_HEAD_LEN,
            prereadBuffer_.get() + prereadBufferSize_, std::begin(MPEG4_FRAME_HEAD), std::end(MPEG4_FRAME_HEAD));
        uint32_t size1 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos1);
        auto pos2 = std::search(prereadBuffer_.get() + pPrereadBuffer_, prereadBuffer_.get() +
            pPrereadBuffer_ + size1, std::begin(MPEG4_SEQUENCE_HEAD), std::end(MPEG4_SEQUENCE_HEAD));
        uint32_t size = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos2);
        if (size == 0) {
            auto pos3 = std::search(prereadBuffer_.get() + pPrereadBuffer_ + size1 + MPEG4_FRAME_HEAD_LEN,
                prereadBuffer_.get() + prereadBufferSize_, std::begin(MPEG4_FRAME_HEAD), std::end(MPEG4_FRAME_HEAD));
            uint32_t size2 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos3);
            auto ret = memcpy_s(pBuffer, size2, prereadBuffer_.get() + pPrereadBuffer_, size2);
            CHECK_AND_RETURN_LOG(ret == EOK, "First Copy buffer failed");
            pPrereadBuffer_ += size2;
            bufferSize += size2;
            pBuffer += size2;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        } else if (size1 > size) {
            auto ret = memcpy_s(pBuffer, size, prereadBuffer_.get() + pPrereadBuffer_, size);
            CHECK_AND_RETURN_LOG(ret == EOK, "Last Copy buffer failed");
            pPrereadBuffer_ += size;
            bufferSize += size;
            pBuffer += size;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        } else {
            auto ret = memcpy_s(pBuffer, size1, prereadBuffer_.get() + pPrereadBuffer_, size1);
            CHECK_AND_RETURN_LOG(ret == EOK, "Copy buffer failed");
            pPrereadBuffer_ += size1;
            bufferSize += size1;
            pBuffer += size1;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        }
        PrereadFile();
        auto ret = memcpy_s(prereadBuffer_.get(), MPEG4_FRAME_HEAD_LEN, pBuffer - MPEG4_FRAME_HEAD_LEN,
            MPEG4_FRAME_HEAD_LEN);
        CHECK_AND_RETURN_LOG(ret == EOK, "Copy buffer failed");
        bufferSize -= MPEG2_FRAME_HEAD_LEN;
        pBuffer -= MPEG2_FRAME_HEAD_LEN;
        pPrereadBuffer_ = 0;
    } while (pPrereadBuffer_ != prereadBufferSize_);
    mpegUnit_->resize(bufferSize);
}

const uint8_t *MpegReader::Mpeg4Detector::GetMpegTypeAddr(const uint8_t *bufferAddr)
{
    auto pos1 = std::search(bufferAddr, bufferAddr + MPEG4_SEQUENCE_HEAD_LEN + 1,
        std::begin(MPEG4_SEQUENCE_HEAD), std::end(MPEG4_SEQUENCE_HEAD));
    auto size = std::distance(bufferAddr, pos1);
    if (size == 0) {
        return nullptr;
    }
    auto pos = std::search(bufferAddr, bufferAddr + MPEG4_FRAME_HEAD_LEN + 1,
        std::begin(MPEG4_FRAME_HEAD), std::end(MPEG4_FRAME_HEAD));
    return pos + MPEG4_FRAME_HEAD_LEN;
}

bool MpegReader::Mpeg4Detector::IsI(uint8_t mpegType)
{
    return (mpegType == MPEG4_I) ? true : false;
}

uint8_t MpegReader::Mpeg4Detector::GetMpegType(const uint8_t *bufferAddr)
{
    if (bufferAddr == nullptr) {
        return 1;
    }
    uint8_t flagi =  ((*bufferAddr) & 0xc0) == 0x00;
    uint8_t flagp =  ((*bufferAddr) & 0xc0) == 0x40;
    uint8_t flagb =  ((*bufferAddr) & 0xc0) == 0x80;
    uint8_t flags =  ((*bufferAddr) & 0xc0) == 0xC0;
    if (flagi) {
        return 1;
    }
    if (flagp) {
        return 0;
    }
    if (flagb) {
        return 0;
    }
    if (flags) {
        return 0;
    }
    return 0;
}


int32_t H263Reader::Init(const std::shared_ptr<H263ReaderInfo> &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<std::ifstream> inputFile = std::make_unique<std::ifstream>(info->inPath.c_str(),
                                                std::ios::binary | std::ios::in);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputFile != nullptr && inputFile->is_open(),
                                      AV_ERR_INVALID_VAL, "Open input file failed");
    h263UnitReader_= std::static_pointer_cast<H263UnitReader>(std::make_shared<H263MetaUnitReader>(inputFile));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(h263UnitReader_, AV_ERR_INVALID_VAL, "h263 unit reader create failed");
    h263Detector_= std::static_pointer_cast<H263Detector>(std::make_shared<H263Detector>());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(h263Detector_, AV_ERR_INVALID_VAL, "h263 detector create failed");
    return AV_ERR_OK;
}

void H263Reader::FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t h263Type,
                                bool isEosFrame)
{
    attr.size += frameSize;
    attr.pts = GetTimeUs();
    attr.flags |= h263Detector_->IsI(h263Type) ? AVCODEC_BUFFER_FLAG_SYNC_FRAME : 0;
    if (isEosFrame) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        std::cout << "Input EOS Frame, frameCount = " << (frameInputCount_ + 1) << std::endl;
    }
}

int32_t H263Reader::FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int32_t frameSize = 0;
    bool isEosFrame = false;
    auto ret = h263UnitReader_->ReadH263Unit(bufferAddr, frameSize, isEosFrame);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ReadH263Unit failed");
    uint8_t h263Type = h263Detector_->GetH263Type(h263Detector_->GetH263TypeAddr(bufferAddr));
    bufferAddr += frameSize;
    FillBufferAttr(attr, frameSize, h263Type, isEosFrame);
    frameInputCount_++;
    return AV_ERR_OK;
}

bool H263Reader::IsEOS()
{
    return h263UnitReader_ ? h263UnitReader_->IsEOS() : true;
}

uint8_t const * H263Reader::H263UnitReader::GetNextH263UnitAddr()
{
    CHECK_AND_RETURN_RET_LOG(h263Unit_ != nullptr, nullptr, "h263Unit_ is nullptr");
    return h263Unit_->data();
}


int32_t H263Reader::H263MetaUnitReader::ReadH263Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL, "Got a invalid buffer addr");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(h263Unit_, AV_ERR_INVALID_VAL, "h263 unit buffer is nullptr");
    bufferSize = h263Unit_->size();
    memcpy_s(bufferAddr, bufferSize, h263Unit_->data(), bufferSize);

    if (!IsEOF()) {
        isEosFrame = false;
        PrereadH263Unit();
    } else {
        isEosFrame = true;
        h263Unit_->resize(0);
    }
    return AV_ERR_OK;
}

H263Reader::H263MetaUnitReader::H263MetaUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    prereadBuffer_ = std::make_unique<uint8_t []>(PREREAD_BUFFER_SIZE); // + MPEG2_FRAME_HEAD_LEN);
    PrereadFile();

    h263Unit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadH263Unit();
}

bool H263Reader::H263MetaUnitReader::IsEOS()
{
    return IsEOF() && h263Unit_->empty();
}

bool H263Reader::H263MetaUnitReader::IsEOF()
{
    return (pPrereadBuffer_ == prereadBufferSize_) && (inputFile_->peek() == EOF);
}

uint8_t* H263Reader::H263MetaUnitReader::GetDelimiterPos(uint8_t* addrstart, uint8_t* addrend)
{
    uint8_t* posMin = std::search(addrstart, addrend, std::begin(H263_HEAD_0), std::end(H263_HEAD_0));
    auto pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_1), std::end(H263_HEAD_1));
    if (pos1<posMin)
        posMin=pos1;
    pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_2), std::end(H263_HEAD_2));
    if (pos1<posMin)
        posMin=pos1;
    pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_3), std::end(H263_HEAD_3));
    if (pos1<posMin)
        posMin=pos1;
    return posMin;
}

void H263Reader::H263MetaUnitReader::PrereadH263Unit()
{
    CHECK_AND_RETURN_LOG(prereadBufferSize_ > 0, "Empty file, nothing to read");
    CHECK_AND_RETURN_LOG(h263Unit_, "h263 unit buffer is nullptr");
    auto pBuffer = h263Unit_->data();
    uint32_t bufferSize = 0;
    h263Unit_->resize(MAX_NALU_SIZE);
    do {
        uint8_t* pos1 = GetDelimiterPos(prereadBuffer_.get() + pPrereadBuffer_,
                                        prereadBuffer_.get() + prereadBufferSize_);
        uint32_t size1 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos1);
        auto pos2 = GetDelimiterPos(prereadBuffer_.get() + pPrereadBuffer_,
                                    prereadBuffer_.get()+ pPrereadBuffer_ + size1);
        uint32_t size = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos2);
        if (size == 0) {
            auto pos3 = GetDelimiterPos(prereadBuffer_.get() + pPrereadBuffer_ + size1 + H263_HEAD_LEN,
                                        prereadBuffer_.get() + prereadBufferSize_);
            uint32_t size2 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos3);
            auto ret = memcpy_s(pBuffer, size2, prereadBuffer_.get() + pPrereadBuffer_, size2);
            CHECK_AND_RETURN_LOG(ret == EOK, "First Copy buffer failed");
            pPrereadBuffer_ += size2;
            bufferSize += size2;
            pBuffer += size2;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        } else {
            if (size1 > size) {
                auto ret = memcpy_s(pBuffer, size, prereadBuffer_.get() + pPrereadBuffer_, size);
                CHECK_AND_RETURN_LOG(ret == EOK, "Last Copy buffer failed");
                pPrereadBuffer_ += size;
                bufferSize += size;
                pBuffer += size;
                UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
            } else {
                auto ret = memcpy_s(pBuffer, size1, prereadBuffer_.get() + pPrereadBuffer_, size1);
                CHECK_AND_RETURN_LOG(ret == EOK, "Comom Copy buffer failed");
                pPrereadBuffer_ += size1;
                bufferSize += size1;
                pBuffer += size1;
                UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
            }
        }
        PrereadFile();
        auto ret = memcpy_s(prereadBuffer_.get(), H263_HEAD_LEN, pBuffer - H263_HEAD_LEN, H263_HEAD_LEN);
        CHECK_AND_RETURN_LOG(ret == EOK, "Buffer End Copy buffer failed");
        bufferSize -= H263_HEAD_LEN;
        pBuffer -= H263_HEAD_LEN;
        pPrereadBuffer_ = 0;
    }
    while (pPrereadBuffer_ != prereadBufferSize_);
    h263Unit_->resize(bufferSize);
}

uint8_t* H263Reader::H263Detector::GetDelimiterPos(uint8_t* addrstart, uint8_t* addrend)
{
    uint8_t* posMin = std::search(addrstart, addrend, std::begin(H263_HEAD_0), std::end(H263_HEAD_0));
    auto pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_1), std::end(H263_HEAD_1));
    if (pos1 < posMin)
        posMin = pos1;
    pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_2), std::end(H263_HEAD_2));
    if (pos1 < posMin)
        posMin = pos1;
    pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_3), std::end(H263_HEAD_3));
    if (pos1 < posMin)
        posMin = pos1;
    return posMin;
}

const uint8_t* H263Reader::H263Detector::GetH263TypeAddr(const uint8_t *bufferAddr)
{
    auto pos1 = GetDelimiterPos(const_cast<uint8_t*>(bufferAddr),
                                const_cast<uint8_t*>(bufferAddr) + H263_HEAD_LEN + 1 /*prereadBufferSize_*/);
    auto size = std::distance(const_cast<uint8_t*>(bufferAddr), pos1);
    if (size == 0) {
        return nullptr;
    }
    auto pos = GetDelimiterPos(const_cast<uint8_t*>(bufferAddr), const_cast<uint8_t*>(bufferAddr) + size);
    return pos;
}

bool H263Reader::H263Detector::IsI(uint8_t h263Type)
{
    return (h263Type == H263_I) ? true : false;
}

uint8_t H263Reader::H263Detector::GetH263Type(const uint8_t *bufferAddr)
{
    if (bufferAddr == nullptr) {
        return 1;
    }
    if ((bufferAddr[H263_OFFSET_4] & H263_HEAD_MASK_4_1) != H263_HEAD_MASK_4_1) {
        return (bufferAddr[H263_OFFSET_4] & H263_HEAD_MASK_4_2) == 0 ? 1 : 0;
    }
    if ((bufferAddr[H263_OFFSET_5] & H263_HEAD_MASK_5_1) == 0) {
        return (bufferAddr[H263_OFFSET_5] & H263_HEAD_MASK_5_2) == 0 ? 1 : 0;
    }
    return (bufferAddr[H263_OFFSET_7] & H263_HEAD_MASK_4_1) == 0 ? 1 : 0;
}

void H263Reader::H263MetaUnitReader::PrereadFile()
{
    CHECK_AND_RETURN_LOG(prereadBuffer_, "Preread buffer is nallptr");
    inputFile_->read((char *)prereadBuffer_.get(), PREREAD_BUFFER_SIZE);
    prereadBufferSize_ = inputFile_->gcount(); //+ MPEG4_FRAME_HEAD_LEN
    pPrereadBuffer_ = 0; //MPEG2_FRAME_HEAD_LEN;
}


int32_t AvccReader::Init(const std::shared_ptr<AvccReaderInfo> &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<std::ifstream> inputFile = std::make_unique<std::ifstream>(info->inPath.data(),
                                                                               std::ios::binary | std::ios::in);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputFile != nullptr && inputFile->is_open(),
                                      AV_ERR_INVALID_VAL, "Open input file failed");

    nalUnitReader_ = std::static_pointer_cast<NalUnitReader>(std::make_shared<AvccNalUnitReader>(inputFile));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(nalUnitReader_, AV_ERR_INVALID_VAL, "Nal unit reader create failed");

    nalDetector_ = info->isAvcStream ?
        std::static_pointer_cast<NalDetector>(std::make_shared<AVCNalDetector>()) :
        std::static_pointer_cast<NalDetector>(std::make_shared<HEVCNalDetector>());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(nalDetector_, AV_ERR_INVALID_VAL, "Nal detector create failed");

    return AV_ERR_OK;
}

void AvccReader::FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t naluType,
                                bool isEosFrame)
{
    attr.size += frameSize;
    attr.pts = GetTimeUs();
    attr.flags |= nalDetector_->IsXPS(naluType) ? AVCODEC_BUFFER_FLAG_CODEC_DATA : 0;
    attr.flags |= nalDetector_->IsIDR(naluType) ? AVCODEC_BUFFER_FLAG_SYNC_FRAME : 0;
    if (isEosFrame) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        std::cout << "Input EOS Frame, frameCount = " << (frameInputCount_ + 1) << std::endl;
    }
}

bool AvccReader::CheckFillBuffer(uint8_t naluType)
{
    if (nalDetector_->IsXPS(naluType)) {
        return false;
    } else if (nalDetector_->IsFullVCL(naluType,
                                       nalDetector_->GetNalTypeAddr(nalUnitReader_->GetNextNalUnitAddr()))) {
        return false;
    } else if (IsEOS()) {
        return false;
    } else {
        return true;
    }
}

int32_t AvccReader::FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr)
{
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        int32_t frameSize = 0;
        bool isEosFrame = false;
        auto ret = nalUnitReader_->ReadNalUnit(bufferAddr, frameSize, isEosFrame);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ReadNalUnit failed");
        uint8_t naluType = nalDetector_->GetNalType(nalDetector_->GetNalTypeAddr(bufferAddr));
        bufferAddr += frameSize;
        FillBufferAttr(attr, frameSize, naluType, isEosFrame);
        UNITTEST_CHECK_AND_BREAK_LOG(CheckFillBuffer(naluType), "FillBuffer stop running");
    } while (true);
    frameInputCount_++;

    return AV_ERR_OK;
}

int32_t AvccReader::KeepFillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr)
{
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        int32_t frameSize = 0;
        bool isEosFrame = false;
        auto ret = nalUnitReader_->ReadNalUnit(bufferAddr, frameSize, isEosFrame);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ReadNalUnit failed");
        uint8_t naluType = nalDetector_->GetNalType(nalDetector_->GetNalTypeAddr(bufferAddr));
        bufferAddr += frameSize;
        if (isEosFrame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // 3000ms
        }
        FillBufferAttr(attr, frameSize, naluType, isEosFrame);
        UNITTEST_CHECK_AND_BREAK_LOG(CheckFillBuffer(naluType), "FillBuffer stop running");
    } while (true);
    frameInputCount_++;

    return AV_ERR_OK;
}

bool AvccReader::IsEOS()
{
    return nalUnitReader_ ? nalUnitReader_->IsEOS() : true;
}

uint8_t const * AvccReader::NalUnitReader::GetNextNalUnitAddr()
{
    CHECK_AND_RETURN_RET_LOG(nalUnit_ != nullptr, nullptr, "nalUnit_ is nullptr");
    return nalUnit_->data();
}

int32_t AvccReader::NalUnitReader::ReadNalUnit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL, "Got a invalid buffer addr");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(nalUnit_, AV_ERR_INVALID_VAL, "Nal unit buffer is nullptr");
    bufferSize = nalUnit_->size();
    memcpy_s(bufferAddr, bufferSize, nalUnit_->data(), bufferSize);

    if (!IsEOF()) {
        isEosFrame = false;
        PrereadNalUnit();
    } else {
        isEosFrame = true;
        nalUnit_->resize(0);
    }
    return AV_ERR_OK;
}

AvccReader::AvccNalUnitReader::AvccNalUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    nalUnit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadNalUnit();
}

bool AvccReader::AvccNalUnitReader::IsEOS()
{
    return IsEOF() && nalUnit_->empty();
}

bool AvccReader::AvccNalUnitReader::IsEOF()
{
    return inputFile_->peek() == EOF;
}

void AvccReader::AvccNalUnitReader::PrereadNalUnit()
{
    uint8_t len[AVCC_FRAME_HEAD_LEN] = {};
    (void)inputFile_->read(reinterpret_cast<char *>(len), AVCC_FRAME_HEAD_LEN);
    nalUnit_->resize(MAX_NALU_SIZE);
    // 0 1 2 3: avcc frame head byte offset; 8 16 24: avcc frame head bit offset
    uint32_t bufferSize = static_cast<uint32_t>((len[3]) | (len[2] << 8) | (len[1] << 16) | (len[0] << 24));
    uint8_t *bufferAddr = nalUnit_->data();

    (void)inputFile_->read(reinterpret_cast<char *>(bufferAddr + AVCC_FRAME_HEAD_LEN), bufferSize);
    ToAnnexb(bufferAddr);

    bufferSize += AVCC_FRAME_HEAD_LEN;
    nalUnit_->resize(bufferSize);
}

int32_t AvccReader::AvccNalUnitReader::ToAnnexb(uint8_t *bufferAddr)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL, "Buffer address is null");

    bufferAddr[0] = 0;
    bufferAddr[1] = 0;
    bufferAddr[2] = 0;      // 2: annexB frame head offset 2
    bufferAddr[3] = 1;      // 3: annexB frame head offset 3
    return AV_ERR_OK;
}

const uint8_t *AvccReader::NalDetector::GetNalTypeAddr(const uint8_t *bufferAddr)
{
    auto pos = std::search(bufferAddr, bufferAddr + ANNEXB_FRAME_HEAD_LEN + 1,
        std::begin(ANNEXB_FRAME_HEAD), std::end(ANNEXB_FRAME_HEAD));
    return pos + ANNEXB_FRAME_HEAD_LEN;
}

bool AvccReader::NalDetector::IsFullVCL(uint8_t nalType, const uint8_t *nextNalTypeAddr)
{
    auto nextNaluType = GetNalType(nextNalTypeAddr);
    return (IsVCL(nalType) && (
        (!IsVCL(nextNaluType)) ||
        ((IsVCL(nextNaluType)) && IsFirstSlice(nextNalTypeAddr)) // 0x80: first_slice_segment_in_pic_flag
    ));
}

uint8_t AvccReader::AVCNalDetector::GetNalType(const uint8_t *bufferAddr)
{
    return (*bufferAddr) & 0x1F; // AVC Nal offset: value & 0x1F
}

bool AvccReader::AVCNalDetector::IsXPS(uint8_t nalType)
{
    return (nalType == AVC_SPS) || (nalType == AVC_PPS) ? true : false;
}

bool AvccReader::AVCNalDetector::IsIDR(uint8_t nalType)
{
    return (nalType == AVC_IDR) ? true : false;
}

uint8_t AvccReader::HEVCNalDetector::GetNalType(const uint8_t *bufferAddr)
{
    return (((*bufferAddr) & 0x7E) >> 1);    // HEVC Nal offset: (value & 0x7E) >> 1
}

bool AvccReader::AVCNalDetector::IsVCL(uint8_t nalType)
{
    return (nalType >= AVC_NON_IDR && nalType <= AVC_IDR) ? true : false;
}

bool AvccReader::AVCNalDetector::IsFirstSlice(const uint8_t *nalTypeAddr)
{
    return (*(nalTypeAddr + 1) & 0x80) == 0x80; // *(nalTypeAddr + 1) & 0x80: AVC first_mb_in_slice
}

bool AvccReader::HEVCNalDetector::IsXPS(uint8_t nalType)
{
    return (nalType >= HEVC_VPS_NUT) && (nalType <= HEVC_PPS_NUT) ? true : false;
}

bool AvccReader::HEVCNalDetector::IsIDR(uint8_t nalType)
{
    return (nalType >= HEVC_IDR_W_RADL) && (nalType <= HEVC_CRA_NUT) ? true : false;
}

bool AvccReader::HEVCNalDetector::IsVCL(uint8_t nalType)
{
    return (nalType >= HEVC_TRAIL_N && nalType <= HEVC_CRA_NUT) ? true : false;
}

bool AvccReader::HEVCNalDetector::IsFirstSlice(const uint8_t *nalTypeAddr)
{
    return *(nalTypeAddr + 2) & 0x80; // *(nalTypeAddr + 2) & 0x80: HEVC first_slice_segment_in_pic_flag
}

#ifdef SUPPORT_CODEC_VC1
int32_t Vc1Reader::Init(const std::shared_ptr<Vc1ReaderInfo>& info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(info, AV_ERR_INVALID_VAL, "Vc1ReaderInfo is null");

    std::shared_ptr<std::ifstream> inputFile = std::make_shared<std::ifstream>(
        info->inPath, std::ios::binary | std::ios::in);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputFile && inputFile->is_open(),
        AV_ERR_INVALID_VAL, "Open input file failed");

    vc1UnitReader_ = std::static_pointer_cast<Vc1UnitReader>(
        std::make_shared<Vc1MetaUnitReader>(inputFile));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(vc1UnitReader_, AV_ERR_INVALID_VAL, "VC1 unit reader create failed");

    vc1Detector_ = std::static_pointer_cast<Vc1Detector>(
        std::make_shared<Vc1Detector>());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(vc1Detector_, AV_ERR_INVALID_VAL, "VC1 detector create failed");

    return AV_ERR_OK;
}

int32_t Vc1Reader::FillBuffer(uint8_t* bufferAddr, OH_AVCodecBufferAttr& attr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr, AV_ERR_INVALID_VAL, "Buffer address is null");

    int32_t frameSize = 0;
    bool isEosFrame = false;
    auto ret = vc1UnitReader_->ReadVc1Unit(bufferAddr, frameSize, isEosFrame);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCS_ERR_INVALID_OPERATION, "ReadVC1Unit failed");

    uint8_t vc1Type = vc1Detector_->GetVc1Type(vc1Detector_->GetVc1TypeAddr(bufferAddr));
    bufferAddr += frameSize;
    FillBufferAttr(attr, frameSize, vc1Type, isEosFrame);
    frameInputCount_++;
    return AV_ERR_OK;
}

bool Vc1Reader::IsEOS()
{
    return vc1UnitReader_ ? vc1UnitReader_->IsEOS() : true;
}

void Vc1Reader::FillBufferAttr(OH_AVCodecBufferAttr& attr, int32_t frameSize, uint8_t vc1Type, bool isEosFrame)
{
    attr.size = frameSize;
    attr.pts = GetTimeUs();
    attr.flags = 0;

    if (isEosFrame) {
        attr.flags |= AVCODEC_BUFFER_FLAG_EOS;
        std::cout << "Input EOS Frame, frameCount = " << (frameInputCount_) << std::endl;
    } else {
        if (vc1Detector_->IsI(vc1Type) || vc1Type == VC1_SEQUENCE_HEADER) {
            attr.flags |= AVCODEC_BUFFER_FLAG_SYNC_FRAME;
        }
    }
}

Vc1Reader::Vc1MetaUnitReader::Vc1MetaUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    prereadBuffer_ = std::make_unique<uint8_t[]>(PREREAD_BUFFER_SIZE);
    vc1Unit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadVc1Unit();
}

int32_t Vc1Reader::Vc1MetaUnitReader::ReadVc1Unit(uint8_t* bufferAddr, int32_t& bufferSize, bool& isEosFrame)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr, AV_ERR_INVALID_VAL, "Got an invalid buffer addr");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(vc1Unit_, AV_ERR_INVALID_VAL, "VC1 unit buffer is nullptr");
    bufferSize = static_cast<int32_t>(vc1Unit_->size());
    if (bufferSize > 0) {
        memcpy_s(bufferAddr, bufferSize, vc1Unit_->data(), bufferSize);
    }
    if (frameIndex_ < ES_VC1_LENGTH) {
        isEosFrame = false;
        PrereadVc1Unit();
    } else {
        isEosFrame = true;
        vc1Unit_->clear();
    }
    return AV_ERR_OK;
}

bool Vc1Reader::Vc1MetaUnitReader::IsEOS()
{
    return frameIndex_ >= ES_VC1_LENGTH;
}

bool Vc1Reader::Vc1MetaUnitReader::IsEOF()
{
    return (pPrereadBuffer_ >= prereadBufferSize_) && (inputFile_ && inputFile_->peek() == EOF);
}

void Vc1Reader::Vc1MetaUnitReader::PrereadFile()
{
    CHECK_AND_RETURN_LOG(prereadBuffer_, "Preread buffer is nullptr");
    if (!inputFile_ || !inputFile_->is_open()) {
        prereadBufferSize_ = 0;
        pPrereadBuffer_ = 0;
        return;
    }
    inputFile_->read(reinterpret_cast<char*>(prereadBuffer_.get()), PREREAD_BUFFER_SIZE);
    std::streamsize bytesRead = inputFile_->gcount();
    prereadBufferSize_ = static_cast<uint32_t>(bytesRead);
    pPrereadBuffer_ = 0;
}

uint8_t* Vc1Reader::Vc1MetaUnitReader::FindNextStartCode(uint8_t* start, uint8_t* end)
{
    uint8_t* posFrame = std::search(start, end, std::begin(VC1_FRAME_HEAD), std::end(VC1_FRAME_HEAD));
    uint8_t* posSeq = std::search(start, end, std::begin(VC1_SEQUENCE_HEAD), std::end(VC1_SEQUENCE_HEAD));
    uint8_t* posMin = end;
    if (posFrame < posMin) {
        posMin = posFrame;
    }
    if (posSeq < posMin) {
        posMin = posSeq;
    }
    return posMin;
}

void Vc1Reader::Vc1MetaUnitReader::PrereadVc1Unit()
{
    CHECK_AND_RETURN_LOG(inputFile_ && inputFile_->is_open(), "Input file not open");
    CHECK_AND_RETURN_LOG(vc1Unit_ != nullptr, "vc1 unit buffer is nullptr");
    CHECK_AND_RETURN_LOG(frameIndex_ < ES_VC1_LENGTH, "All VC1 frames have been read");

    uint32_t frameSize = ES_VC1[frameIndex_];
    vc1Unit_->resize(frameSize + VC1_FRAME_HEAD_LEN);
    auto pBuffer = vc1Unit_->data();

    memcpy_s(pBuffer, frameSize + VC1_FRAME_HEAD_LEN, VC1_FRAME_HEAD, VC1_FRAME_HEAD_LEN);
    inputFile_->read(reinterpret_cast<char*>(pBuffer + VC1_FRAME_HEAD_LEN), frameSize);
    uint32_t bytesRead = static_cast<uint32_t>(inputFile_->gcount());

    CHECK_AND_RETURN_LOG(bytesRead == frameSize,
        "Failed to read full frame. Expected: %u, Got: %u", frameSize, bytesRead);

    frameIndex_++;
}

const uint8_t* Vc1Reader::Vc1Detector::GetVc1TypeAddr(const uint8_t* bufferAddr)
{
    return bufferAddr;
}

uint8_t Vc1Reader::Vc1Detector::GetVc1Type(const uint8_t* bufferAddr)
{
    if (!bufferAddr) {
        return VC1_UNSPECIFIED;
    }

    if (std::memcmp(bufferAddr, VC1_SEQUENCE_HEAD, VC1_SEQUENCE_HEAD_LEN) == 0) {
        return VC1_SEQUENCE_HEADER;
    }

    if (std::memcmp(bufferAddr, VC1_FRAME_HEAD, VC1_FRAME_HEAD_LEN) == 0) {
        const uint8_t* typeByteAddr = bufferAddr + VC1_FRAME_TYPE_OFFSET;
        uint8_t typeBits = (*typeByteAddr) & VC1_FRAME_TYPE_MASK;

        switch (typeBits) {
            case VC1_FRAME_TYPE_I_BITS: return VC1_I;
            case VC1_FRAME_TYPE_P_BITS: return VC1_P;
            case VC1_FRAME_TYPE_B_BITS: return VC1_B;
            case VC1_FRAME_TYPE_BI_BITS: return VC1_BI;
            default: return VC1_UNSPECIFIED;
        }
    }

    return VC1_UNSPECIFIED;
}

bool Vc1Reader::Vc1Detector::IsI(uint8_t vc1Type)
{
    return (vc1Type == VC1_I || vc1Type == VC1_BI);
}

int32_t Vc1Reader::Vc1UnitReader::ReadVc1Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame)
{
    return AV_ERR_OK;
}

void Vc1Reader::Vc1UnitReader::PrereadVc1Unit()
{
    std::cout << "[Vc1UnitReader::PrereadVc1Unit] Base class implementation - should be overridden" << std::endl;
}
#endif
int32_t Msvideo1Reader::Init(const std::shared_ptr<Msvideo1ReaderInfo>& info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(info, AV_ERR_INVALID_VAL, "Msvideo1ReaderInfo is null");

    std::shared_ptr<std::ifstream> inputFile = std::make_shared<std::ifstream>(
        info->inPath, std::ios::binary | std::ios::in);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputFile && inputFile->is_open(),
        AV_ERR_INVALID_VAL, "Open input file failed");

    msvideo1UnitReader_ = std::static_pointer_cast<Msvideo1UnitReader>(
        std::make_shared<Msvideo1MetaUnitReader>(inputFile));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(msvideo1UnitReader_, AV_ERR_INVALID_VAL, "MSVIDEO1 unit reader create failed");

    msvideo1Detector_ = std::static_pointer_cast<Msvideo1Detector>(
        std::make_shared<Msvideo1Detector>());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(msvideo1Detector_, AV_ERR_INVALID_VAL, "MSVIDEO1 detector create failed");

    return AV_ERR_OK;
}

int32_t Msvideo1Reader::FillBuffer(uint8_t* bufferAddr, OH_AVCodecBufferAttr& attr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr, AV_ERR_INVALID_VAL, "Buffer address is null");

    int32_t frameSize = 0;
    bool isEosFrame = false;
    auto ret = msvideo1UnitReader_->ReadMsvideo1Unit(bufferAddr, frameSize, isEosFrame);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCS_ERR_INVALID_OPERATION, "ReadMsvideo1Unit failed");

    uint8_t msvideo1Type = msvideo1Detector_->GetMsvideo1Type(msvideo1Detector_->GetMsvideo1TypeAddr(bufferAddr));
    bufferAddr += frameSize;
    FillBufferAttr(attr, frameSize, msvideo1Type, isEosFrame);
    frameInputCount_++;
    return AV_ERR_OK;
}

bool Msvideo1Reader::IsEOS()
{
    return msvideo1UnitReader_ ? msvideo1UnitReader_->IsEOS() : true;
}

void Msvideo1Reader::FillBufferAttr(OH_AVCodecBufferAttr& attr, int32_t frameSize, uint8_t msvideo1Type,
                                    bool isEosFrame)
{
    attr.size = frameSize;
    attr.pts = GetTimeUs();
    attr.flags = 0;

    if (isEosFrame) {
        attr.flags |= AVCODEC_BUFFER_FLAG_EOS;
        std::cout << "Input EOS Frame, frameCount = " << (frameInputCount_) << std::endl;
    } else {
        if (msvideo1Detector_->IsI(msvideo1Type) || msvideo1Type == MSVIDEO1_SEQUENCE_HEADER) {
            attr.flags |= AVCODEC_BUFFER_FLAG_SYNC_FRAME;
        }
    }
}

Msvideo1Reader::Msvideo1MetaUnitReader::Msvideo1MetaUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    prereadBuffer_ = std::make_unique<uint8_t[]>(PREREAD_BUFFER_SIZE);
    msvideo1Unit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    frameIndex_ = 0;
    PrereadMsvideo1Unit();
}

int32_t Msvideo1Reader::Msvideo1MetaUnitReader::ReadMsvideo1Unit(uint8_t* bufferAddr, int32_t& bufferSize,
                                                                 bool& isEosFrame)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr, AV_ERR_INVALID_VAL, "Got an invalid buffer addr");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(msvideo1Unit_, AV_ERR_INVALID_VAL, "MSVIDEO1 unit buffer is nullptr");
    bufferSize = static_cast<int32_t>(msvideo1Unit_->size());
    if (bufferSize > 0) {
        memcpy_s(bufferAddr, bufferSize, msvideo1Unit_->data(), bufferSize);
    }
    if (frameIndex_ < ES_MSVIDEO1_LENGTH) {
        isEosFrame = false;
        PrereadMsvideo1Unit();
    } else {
        isEosFrame = true;
        msvideo1Unit_->clear();
    }
    return AV_ERR_OK;
}

int32_t Wmv3Reader::Init(const std::shared_ptr<Wmv3ReaderInfo> &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<std::ifstream> inputFile = std::make_unique<std::ifstream>(info->inPath.c_str(),
                                                std::ios::binary | std::ios::in);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputFile != nullptr && inputFile->is_open(),
                                      AV_ERR_INVALID_VAL, "Open input file failed");
    wmv3UnitReader_= std::static_pointer_cast<Wmv3UnitReader>(std::make_shared<Wmv3MetaUnitReader>(
                                      inputFile, info->isMainStream));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(wmv3UnitReader_, AV_ERR_INVALID_VAL, "wmv3 unit reader create failed");
    return AV_ERR_OK;
}

void Wmv3Reader::FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t frameType,
                                bool isEosFrame)
{
    (void) frameType;
    attr.size += frameSize;
    attr.pts = GetTimeUs();
    if (isEosFrame) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        std::cout << "Input EOS Frame, frameCount = " << (frameInputCount_ + 1) << std::endl;
    }
}

int32_t Wmv3Reader::FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int32_t frameSize = 0;
    bool isEosFrame = false;
    UNITTEST_CHECK_AND_RETURN_RET_LOG(wmv3UnitReader_ != nullptr, AV_ERR_INVALID_VAL, "wmv3UnitReader_ is nullptr");
    auto ret = wmv3UnitReader_->ReadWmv3Unit(bufferAddr, frameSize, isEosFrame);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ReadWmv3Unit failed");
    uint8_t frameType = 0x00;
    bufferAddr += frameSize;
    FillBufferAttr(attr, frameSize, frameType, isEosFrame);
    frameInputCount_++;

    return AV_ERR_OK;
}

bool Wmv3Reader::IsEOS()
{
    return wmv3UnitReader_ ? wmv3UnitReader_->IsEOS() : true;
}

uint8_t const *Wmv3Reader::Wmv3UnitReader::GetNextWmv3UnitAddr()
{
    CHECK_AND_RETURN_RET_LOG(wmv3Unit_ != nullptr, nullptr, "wmv3Unit_ is nullptr");
    return wmv3Unit_->data();
}

int32_t Wmv3Reader::Wmv3MetaUnitReader::ReadWmv3Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL, "Got a invalid buffer addr");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(wmv3Unit_, AV_ERR_INVALID_VAL, "wmv3 unit buffer is nullptr");
    bufferSize = wmv3Unit_->size();
    memcpy_s(bufferAddr, bufferSize, wmv3Unit_->data(), bufferSize);
    std::cout << "bufferSize = " << (bufferSize) << std::endl;

    if (!IsEOF()) {
        isEosFrame = false;
        PrereadWmv3Unit();
    } else {
        isEosFrame = true;
        wmv3Unit_->clear();
    }
    return AV_ERR_OK;
}

bool Msvideo1Reader::Msvideo1MetaUnitReader::IsEOS()
{
    return frameIndex_ >= ES_MSVIDEO1_LENGTH;
}

bool Msvideo1Reader::Msvideo1MetaUnitReader::IsEOF()
{
    return (pPrereadBuffer_ >= prereadBufferSize_) && (inputFile_ && inputFile_->peek() == EOF);
}

void Msvideo1Reader::Msvideo1MetaUnitReader::PrereadFile()
{
    CHECK_AND_RETURN_LOG(prereadBuffer_, "Preread buffer is nullptr");
    if (!inputFile_ || !inputFile_->is_open()) {
        prereadBufferSize_ = 0;
        pPrereadBuffer_ = 0;
        return;
    }
    inputFile_->read(reinterpret_cast<char*>(prereadBuffer_.get()), PREREAD_BUFFER_SIZE);
    std::streamsize bytesRead = inputFile_->gcount();
    prereadBufferSize_ = static_cast<uint32_t>(bytesRead);
    pPrereadBuffer_ = 0;
}

void Msvideo1Reader::Msvideo1MetaUnitReader::PrereadMsvideo1Unit()
{
    CHECK_AND_RETURN_LOG(inputFile_ && inputFile_->is_open(), "Input file not open");
    CHECK_AND_RETURN_LOG(msvideo1Unit_ != nullptr, "msvideo1 unit buffer is nullptr");
    CHECK_AND_RETURN_LOG(frameIndex_ < ES_MSVIDEO1_LENGTH, "All MSVIDEO1 frames have been read");

    uint32_t frameSize = ES_MSVIDEO1[frameIndex_];
    msvideo1Unit_->resize(frameSize);
    auto pBuffer = msvideo1Unit_->data();

    inputFile_->read(reinterpret_cast<char*>(pBuffer), frameSize);
    uint32_t bytesRead = static_cast<uint32_t>(inputFile_->gcount());

    CHECK_AND_RETURN_LOG(bytesRead == frameSize,
        "Failed to read full frame. Expected: %u, Got: %u", frameSize, bytesRead);

    frameIndex_++;
}

Wmv3Reader::Wmv3MetaUnitReader::Wmv3MetaUnitReader(std::shared_ptr<std::ifstream> inputFile, bool isMainStream)
{
    inputFile_ = inputFile;
    isMainStream_ = isMainStream;
    prereadBuffer_ = std::make_unique<uint8_t []>(PREREAD_BUFFER_SIZE);

    wmv3Unit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadWmv3Unit();
}

bool Wmv3Reader::Wmv3MetaUnitReader::IsEOS()
{
    return IsEOF() && wmv3Unit_->empty();
}

bool Wmv3Reader::Wmv3MetaUnitReader::IsEOF()
{
    if (isMainStream_) {
        return frameIndex_ >= ES_WMV3_MAIN_LEN;
    }
    return frameIndex_ >= ES_WMV3_NORMAL_LEN;
}

uint32_t Wmv3Reader::Wmv3MetaUnitReader::GetFrameLenth(uint32_t index)
{
    if (isMainStream_) {
        return ES_WMV3_MAIN[index];
    }
    return ES_WMV3_NORMAL[index];
}

void Wmv3Reader::Wmv3MetaUnitReader::PrereadWmv3Unit()
{
    CHECK_AND_RETURN_LOG(inputFile_ && inputFile_->is_open(), "Input file not open");
    CHECK_AND_RETURN_LOG(wmv3Unit_ != nullptr, "wmv3 unit buffer is nullptr");
    CHECK_AND_RETURN_LOG(frameIndex_ < (isMainStream_ ? ES_WMV3_MAIN_LEN : ES_WMV3_NORMAL_LEN),
        "All wmv3 frames have been read");

    uint32_t frameSize = GetFrameLenth(frameIndex_);
    wmv3Unit_->resize(frameSize);
    auto pBuffer = wmv3Unit_->data();

    inputFile_->read(reinterpret_cast<char*>(pBuffer), frameSize);
    uint32_t bytesRead = static_cast<uint32_t>(inputFile_->gcount());

    CHECK_AND_RETURN_LOG(bytesRead == frameSize,
        "Failed to read full frame. Expected: %u, Got: %u", frameSize, bytesRead);

    frameIndex_++;
}

const uint8_t* Msvideo1Reader::Msvideo1Detector::GetMsvideo1TypeAddr(const uint8_t* bufferAddr)
{
    return bufferAddr;
}

uint8_t Msvideo1Reader::Msvideo1Detector::GetMsvideo1Type(const uint8_t* bufferAddr)
{
    if (!bufferAddr) {
        return MSVIDEO1_UNSPECIFIED;
    }
    uint8_t type = bufferAddr[0];
    switch (type) {
        case 0x00: return MSVIDEO1_I;
        case 0x01: return MSVIDEO1_P;
        case 0x02: return MSVIDEO1_B;
        default:   return MSVIDEO1_UNSPECIFIED;
    }
}

bool Msvideo1Reader::Msvideo1Detector::IsI(uint8_t msvideo1Type)
{
    return (msvideo1Type == MSVIDEO1_I);
}

int32_t Msvideo1Reader::Msvideo1UnitReader::ReadMsvideo1Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame)
{
    return AV_ERR_OK;
}

void Msvideo1Reader::Msvideo1UnitReader::PrereadMsvideo1Unit()
{
    std::cout << "[Msvideo1UnitReader::PrereadMsvideo1Unit] Base class implementation" << std::endl;
}

void Wmv3Reader::Wmv3MetaUnitReader::PrereadFile()
{
    CHECK_AND_RETURN_LOG(prereadBuffer_, "Preread buffer is nallptr");
    inputFile_->read((char *)prereadBuffer_.get(), PREREAD_BUFFER_SIZE);
    prereadBufferSize_ = inputFile_->gcount();
    pPrereadBuffer_ = 0;
}

} // MediaAVCodec
} // OHOS