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

#include "codec_drm_decrypt.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "securec.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecDrmDecrypt"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {

#define DRM_VIDEO_FRAME_ARR_LEN            3
#define DRM_AMBIGUITY_ARR_LEN              3
#define DRM_USER_DATA_REGISTERED_UUID_SIZE 16
constexpr uint32_t DRM_LEGACY_LEN = 3;
constexpr uint32_t DRM_AES_BLOCK_SIZE = 16;
constexpr uint8_t DRM_AMBIGUITY_START_NUM = 0x00;
constexpr uint8_t DRM_AMBIGUITY_END_NUM = 0x03;
constexpr uint32_t DRM_CRYPT_BYTE_BLOCK = 1;
constexpr uint32_t DRM_SKIP_BYTE_BLOCK = 9;
constexpr uint32_t DRM_H264_VIDEO_SKIP_BYTES = 35; // 35:(32 + 3) 32: bytes 3:3bytes
constexpr uint32_t DRM_H265_VIDEO_SKIP_BYTES = 68; // 68:(65 + 3) // 65: bytes 3:3bytes
constexpr uint32_t DRM_AVS3_VIDEO_SKIP_BYTES = 4; // 4:(1 + 3) // 1: bytes 3:3bytes
constexpr uint8_t DRM_SHIFT_LEFT_NUM = 1;
constexpr uint8_t DRM_H264_VIDEO_NAL_TYPE_UMASK_NUM = 0x1f;
constexpr uint8_t DRM_H265_VIDEO_NAL_TYPE_UMASK_NUM = 0x3f;
constexpr uint8_t DRM_H265_VIDEO_START_NAL_TYPE = 0;
constexpr uint8_t DRM_H265_VIDEO_END_NAL_TYPE = 31;
constexpr uint32_t DRM_TS_SUB_SAMPLE_NUM = 2;
constexpr uint8_t DRM_H264_VIDEO_START_NAL_TYPE = 1;
constexpr uint8_t DRM_H264_VIDEO_END_NAL_TYPE = 5;
constexpr uint32_t DRM_MAX_STREAM_DATA_SIZE = 20971520; // 20971520:(20 * 1024 * 1024) 20MB, 1024:1024 bytes = 1kb
constexpr uint32_t DRM_H265_PAYLOAD_TYPE_OFFSET = 5;
constexpr uint8_t DRM_AVS_FLAG = 0xb5;
constexpr uint8_t DRM_USER_DATA_UNREGISTERED_TAG = 0x05;
constexpr uint32_t DRM_MIN_DRM_INFO_LEN = 2;
constexpr uint32_t DRM_INVALID_START_POS = 0xffffffff;

static const uint8_t VIDEO_FRAME_ARR[DRM_VIDEO_FRAME_ARR_LEN] = { 0x00, 0x00, 0x01 };
static const uint8_t AMBIGUITY_ARR[DRM_AMBIGUITY_ARR_LEN] = { 0x00, 0x00, 0x03 };
static const uint8_t USER_REGISTERED_UUID[DRM_USER_DATA_REGISTERED_UUID_SIZE] = {
    0x70, 0xc1, 0xdb, 0x9f, 0x66, 0xae, 0x41, 0x27, 0xbf, 0xc0, 0xbb, 0x19, 0x81, 0x69, 0x4b, 0x66
};

typedef enum {
    DRM_ARR_SUBSCRIPT_ZERO = 0,
    DRM_ARR_SUBSCRIPT_ONE,
    DRM_ARR_SUBSCRIPT_TWO,
    DRM_ARR_SUBSCRIPT_THREE,
} DRM_ArrSubscriptCollection;

typedef enum {
    DRM_VIDEO_AVC = 0x1,
    DRM_VIDEO_HEVC,
    DRM_VIDEO_AVS,
    DRM_VIDEO_NONE,
} DRM_CodecType;

void CodecDrmDecrypt::DrmGetSkipClearBytes(uint32_t &skipBytes) const
{
    if (codingType_ == DRM_VIDEO_AVC) {
        skipBytes = DRM_H264_VIDEO_SKIP_BYTES;
    } else if (codingType_ == DRM_VIDEO_HEVC) {
        skipBytes = DRM_H265_VIDEO_SKIP_BYTES;
    } else if (codingType_ == DRM_VIDEO_AVS) {
        skipBytes = DRM_AVS3_VIDEO_SKIP_BYTES;
    }
    return;
}

int32_t CodecDrmDecrypt::DrmGetNalTypeAndIndex(const uint8_t *data, uint32_t dataSize,
    uint8_t &nalType, uint32_t &posIndex) const
{
    uint32_t i;
    nalType = 0;
    for (i = posIndex; (i + DRM_LEGACY_LEN) < dataSize; i++) {
        if ((data[i] != VIDEO_FRAME_ARR[DRM_ARR_SUBSCRIPT_ZERO]) ||
            (data[i + DRM_ARR_SUBSCRIPT_ONE] != VIDEO_FRAME_ARR[DRM_ARR_SUBSCRIPT_ONE]) ||
            (data[i + DRM_ARR_SUBSCRIPT_TWO] != VIDEO_FRAME_ARR[DRM_ARR_SUBSCRIPT_TWO])) {
            continue;
        }
        if (codingType_ == DRM_VIDEO_AVC) {
            nalType = data[i + DRM_ARR_SUBSCRIPT_THREE] & DRM_H264_VIDEO_NAL_TYPE_UMASK_NUM;
            if ((nalType == DRM_H264_VIDEO_START_NAL_TYPE) ||
                (nalType == DRM_H264_VIDEO_END_NAL_TYPE)) {
                posIndex = i;
                return 0;
            }
        } else if (codingType_ == DRM_VIDEO_HEVC) {
            nalType = (data[i + DRM_ARR_SUBSCRIPT_THREE] >> DRM_SHIFT_LEFT_NUM) & DRM_H265_VIDEO_NAL_TYPE_UMASK_NUM;
            if ((nalType >= DRM_H265_VIDEO_START_NAL_TYPE) &&
                (nalType <= DRM_H265_VIDEO_END_NAL_TYPE)) {
                posIndex = i;
                return 0;
            }
        } else if (codingType_ == DRM_VIDEO_AVS) {
            nalType = data[i + DRM_ARR_SUBSCRIPT_THREE];
            if (nalType == 0) {
                posIndex = i;
                return 0;
            }
        } else {
            nalType = 0;
        }
    }
    posIndex = i;
    return -1;
}

void CodecDrmDecrypt::DrmGetSyncHeaderIndex(const uint8_t *data, uint32_t dataSize, uint32_t &posIndex)
{
    uint32_t i;
    for (i = posIndex; (i + DRM_LEGACY_LEN) < dataSize; i++) {
        if ((data[i] != VIDEO_FRAME_ARR[DRM_ARR_SUBSCRIPT_ZERO]) ||
            (data[i + DRM_ARR_SUBSCRIPT_ONE] != VIDEO_FRAME_ARR[DRM_ARR_SUBSCRIPT_ONE]) ||
            (data[i + DRM_ARR_SUBSCRIPT_TWO] != VIDEO_FRAME_ARR[DRM_ARR_SUBSCRIPT_TWO])) {
            continue;
        }
        posIndex = i;
        return;
    }
    posIndex = dataSize;
    return;
}

uint8_t CodecDrmDecrypt::DrmGetFinalNalTypeAndIndex(const uint8_t *data, uint32_t dataSize,
    uint32_t &posStartIndex, uint32_t &posEndIndex) const
{
    uint32_t skipBytes = 0;
    uint8_t tmpNalType = 0;
    uint32_t tmpPosIndex = 0;
    uint8_t nalType = 0;
    posStartIndex = 0;
    posEndIndex = dataSize;
    DrmGetSkipClearBytes(skipBytes);
    while (1) { // 1 true
        int32_t ret = DrmGetNalTypeAndIndex(data, dataSize, tmpNalType, tmpPosIndex);
        if (ret == 0) {
            nalType = tmpNalType;
            posStartIndex = tmpPosIndex;
            tmpPosIndex += DRM_LEGACY_LEN;
            DrmGetSyncHeaderIndex(data, dataSize, tmpPosIndex);
            posEndIndex = tmpPosIndex;
            if (tmpPosIndex > posStartIndex + skipBytes + DRM_AES_BLOCK_SIZE) {
                break;
            } else {
                nalType = 0;
                posStartIndex = dataSize;
                posEndIndex = dataSize;
            }
        } else {
            nalType = 0;
            posStartIndex = dataSize;
            posEndIndex = dataSize;
            break;
        }
    }
    return nalType;
}

void CodecDrmDecrypt::DrmRemoveAmbiguityBytes(uint8_t *data, uint32_t &posEndIndex, uint32_t offset,
    uint32_t &dataSize)
{
    uint32_t len = posEndIndex;
    uint32_t i;
    for (i = offset; (i + DRM_LEGACY_LEN) < len; i++) {
        if ((data[i] == AMBIGUITY_ARR[DRM_ARR_SUBSCRIPT_ZERO]) &&
            (data[i + DRM_ARR_SUBSCRIPT_ONE] == AMBIGUITY_ARR[DRM_ARR_SUBSCRIPT_ONE]) &&
            (data[i + DRM_ARR_SUBSCRIPT_TWO] == AMBIGUITY_ARR[DRM_ARR_SUBSCRIPT_TWO])) {
            if (data[i + DRM_ARR_SUBSCRIPT_THREE] >= DRM_AMBIGUITY_START_NUM &&
                data[i + DRM_ARR_SUBSCRIPT_THREE] <= DRM_AMBIGUITY_END_NUM) {
                errno_t res = memmove_s(data + i + DRM_ARR_SUBSCRIPT_TWO, len - (i + DRM_ARR_SUBSCRIPT_TWO),
                    data + i + DRM_ARR_SUBSCRIPT_THREE, len - (i + DRM_ARR_SUBSCRIPT_THREE));
                CHECK_AND_RETURN_LOG(res == EOK, "memmove data err");
                len -= 1;
                i++;
            }
        }
    }
    dataSize = dataSize - (posEndIndex - len);
    posEndIndex = len;
    return;
}

void CodecDrmDecrypt::DrmModifyCencInfo(std::shared_ptr<AVBuffer> inBuf, uint32_t &dataSize, uint8_t isAmbiguity,
    MetaDrmCencInfo *cencInfo) const
{
    uint8_t nalType;
    uint32_t posStartIndex;
    uint32_t posEndIndex;
    uint32_t skipBytes = 0;
    uint32_t delLen = 0;
    uint32_t i;
    if (inBuf->memory_ == nullptr || inBuf->memory_->GetAddr() == nullptr || dataSize == 0 ||
        dataSize > DRM_MAX_STREAM_DATA_SIZE) {
        return;
    }
    uint8_t *data = inBuf->memory_->GetAddr();
    DrmGetSkipClearBytes(skipBytes);
    nalType = DrmGetFinalNalTypeAndIndex(data, dataSize, posStartIndex, posEndIndex);
    if (isAmbiguity == 1) {
        DrmRemoveAmbiguityBytes(data, posEndIndex, posStartIndex, dataSize);
    }
    for (i = posEndIndex - 1; i > 0; i--) {
        if (data[i] != 0) {
            break;
        }
        delLen++;
    }

    cencInfo->subSamples[0].clearHeaderLen = dataSize;
    cencInfo->subSamples[0].payLoadLen = 0;
    cencInfo->subSamples[1].clearHeaderLen = 0;
    cencInfo->subSamples[1].payLoadLen = 0;
    if (((codingType_ == DRM_VIDEO_AVC) && ((nalType == DRM_H264_VIDEO_START_NAL_TYPE) ||
        (nalType == DRM_H264_VIDEO_END_NAL_TYPE))) ||
        ((codingType_ == DRM_VIDEO_HEVC) &&
        (nalType >= DRM_H265_VIDEO_START_NAL_TYPE) && (nalType <= DRM_H265_VIDEO_END_NAL_TYPE)) ||
        ((codingType_ == DRM_VIDEO_AVS) && (nalType == 0))) {
        uint32_t clearHeaderLen = posStartIndex + skipBytes;
        uint32_t payLoadLen =
            (posEndIndex > (clearHeaderLen + delLen)) ? (posEndIndex - clearHeaderLen - delLen) : 0;
        if (payLoadLen > 0) {
            uint32_t lastClearLen = (payLoadLen % DRM_AES_BLOCK_SIZE == 0) ? DRM_AES_BLOCK_SIZE
                                    : (payLoadLen % DRM_AES_BLOCK_SIZE);
            payLoadLen = payLoadLen - lastClearLen;
            cencInfo->subSamples[0].clearHeaderLen = clearHeaderLen;
            cencInfo->subSamples[0].payLoadLen = payLoadLen;
            cencInfo->subSamples[1].clearHeaderLen = lastClearLen + delLen + (dataSize - posEndIndex);
            cencInfo->subSamples[1].payLoadLen = 0;
        }
    }
    return;
}

void CodecDrmDecrypt::SetDrmAlgoAndBlocks(uint8_t algo, MetaDrmCencInfo *cencInfo)
{
    if (algo == 0x1) { // 0x1:SM4-SAMPL SM4S
        cencInfo->algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_SM4_CBC;
        cencInfo->encryptBlocks = DRM_CRYPT_BYTE_BLOCK;
        cencInfo->skipBlocks = DRM_SKIP_BYTE_BLOCK;
    } else if (algo == 0x2) { // 0x2:AES CBCS
        cencInfo->algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_AES_CBC;
        cencInfo->encryptBlocks = DRM_CRYPT_BYTE_BLOCK;
        cencInfo->skipBlocks = DRM_SKIP_BYTE_BLOCK;
    } else if (algo == 0x5) { // 0x5:AES CBC1
        cencInfo->algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_AES_CBC;
        cencInfo->encryptBlocks = 0;
        cencInfo->skipBlocks = 0;
    } else if (algo == 0x3) { // 0x3:SM4-CBC SM4C
        cencInfo->algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_SM4_CBC;
        cencInfo->encryptBlocks = 0;
        cencInfo->skipBlocks = 0;
    } else if (algo == 0x0) { // 0x0:NONE
        cencInfo->algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED;
        cencInfo->encryptBlocks = 0;
        cencInfo->skipBlocks = 0;
    }
    return;
}

int CodecDrmDecrypt::DrmFindAvsCeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos,
    uint32_t index)
{
    uint32_t i = index;
    /*
     * only squence_header allowed, cei is in first extension_and_user_data after squence_header.
     * data[i + DRM_LEGACY_LEN] is the nal unit header(00~b8),
     * 0xb0 means Squence header, 0xb5 means video extension, 0xb1 undefined, others are frames
     */
    if (((data[i + DRM_LEGACY_LEN] > 0) && (data[i + DRM_LEGACY_LEN] < 0xb8)) &&
        (data[i + DRM_LEGACY_LEN] != 0xb0) && (data[i + DRM_LEGACY_LEN] != 0xb5) &&
        (data[i + DRM_LEGACY_LEN] != 0xb1)) {
        AVCODEC_LOGD("avs frame found");
        return 0;
    }
    if ((data[i + DRM_LEGACY_LEN] == 0xb5) && (i + DRM_LEGACY_LEN + 1 < dataSize)) {
        /* extension_and_user_data found, 0xd0: extension user data tag, 0xf0: the higher 4 bits */
        if ((data[i + DRM_LEGACY_LEN + 1] & 0xf0) == 0xd0) {
            ceiStartPos = i;
            AVCODEC_LOGD("cei found, packet start pos:%{public}d", ceiStartPos);
        } else {
            AVCODEC_LOGE("cei not found, type=0x%{public}x", (data[i + DRM_LEGACY_LEN + 1] & 0xf0));
        }
    }
    return -1;
}

int CodecDrmDecrypt::DrmFindHevcCeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos,
    uint32_t index)
{
    uint32_t i = index;
    uint8_t nalType = (data[i + DRM_LEGACY_LEN] >> DRM_SHIFT_LEFT_NUM) & DRM_H265_VIDEO_NAL_TYPE_UMASK_NUM;
    AVCODEC_LOGD("nal type=%{public}x", nalType);
    if (nalType <= DRM_H265_VIDEO_END_NAL_TYPE) { // nal type: 0 ~ 31 are slice nal units and reserved units
        /* sei is not after frame data. */
        AVCODEC_LOGD("h265 frame found");
        return 0;
    } else if (nalType == 39) { // 39: SEI nal unit
        if (data[i + DRM_H265_PAYLOAD_TYPE_OFFSET] == DRM_USER_DATA_UNREGISTERED_TAG) {
            ceiStartPos = i;
        }
    }
    if (ceiStartPos != DRM_INVALID_START_POS) {
        uint32_t startPos = i + DRM_H265_PAYLOAD_TYPE_OFFSET;
        uint32_t endPos = i + DRM_H265_PAYLOAD_TYPE_OFFSET;
        ceiStartPos = DRM_INVALID_START_POS;
        DrmGetSyncHeaderIndex(data, dataSize, endPos);
        for (; (startPos + DRM_USER_DATA_REGISTERED_UUID_SIZE < endPos); startPos++) {
            if (memcmp(data + startPos, USER_REGISTERED_UUID,
                DRM_USER_DATA_REGISTERED_UUID_SIZE) == 0) {
                ceiStartPos = i;
                AVCODEC_LOGD("cei found, packet start pos:%{public}d", ceiStartPos);
                break;
            }
        }
    }
    return -1;
}

int CodecDrmDecrypt::DrmFindH264CeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos,
    uint32_t index)
{
    uint32_t i = index;
    uint8_t nalType = data[i + DRM_LEGACY_LEN] & DRM_H264_VIDEO_NAL_TYPE_UMASK_NUM;
    AVCODEC_LOGD("nal type=%{public}x", nalType);
    if ((nalType >= DRM_H264_VIDEO_START_NAL_TYPE) && (nalType <= DRM_H264_VIDEO_END_NAL_TYPE)) {
        /* sei is not after frame data. */
        AVCODEC_LOGD("h264 frame found");
        return 0;
    } else if ((nalType == 39) || (nalType == 6)) { // 39 or 6 is SEI nal unit tag
        if ((i + DRM_LEGACY_LEN + 1 < dataSize) &&
            (data[i + DRM_LEGACY_LEN + 1] == DRM_USER_DATA_UNREGISTERED_TAG)) {
            ceiStartPos = i;
        }
    }
    if (ceiStartPos != DRM_INVALID_START_POS) {
        uint32_t startPos = i + DRM_LEGACY_LEN + 1;
        uint32_t endPos = i + DRM_LEGACY_LEN + 1;
        ceiStartPos = DRM_INVALID_START_POS;
        DrmGetSyncHeaderIndex(data, dataSize, endPos);
        for (; (startPos + DRM_USER_DATA_REGISTERED_UUID_SIZE < endPos); startPos++) {
            if (memcmp(data + startPos, USER_REGISTERED_UUID,
                DRM_USER_DATA_REGISTERED_UUID_SIZE) == 0) {
                ceiStartPos = i;
                AVCODEC_LOGD("cei found, packet start pos:%{public}d", ceiStartPos);
                break;
            }
        }
    }
    return -1;
}

int CodecDrmDecrypt::DrmFindCeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos,
    uint32_t index) const
{
    int ret = 0;
    if (codingType_ == DRM_VIDEO_AVS) {
        ret = DrmFindAvsCeiNalUnit(data, dataSize, ceiStartPos, index);
    } else if (codingType_ == DRM_VIDEO_HEVC) {
        ret = DrmFindHevcCeiNalUnit(data, dataSize, ceiStartPos, index);
    } else if (codingType_ == DRM_VIDEO_AVC) {
        ret = DrmFindH264CeiNalUnit(data, dataSize, ceiStartPos, index);
    }
    return ret;
}

int CodecDrmDecrypt::DrmFindCeiPos(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos,
    uint32_t &ceiEndPos) const
{
    uint32_t i;
    for (i = 0; (i + DRM_LEGACY_LEN) < dataSize; i++) {
        /*the start code prefix is 0x000001*/
        if ((data[i] == VIDEO_FRAME_ARR[DRM_ARR_SUBSCRIPT_ZERO]) &&
            (data[i + DRM_ARR_SUBSCRIPT_ONE] == VIDEO_FRAME_ARR[DRM_ARR_SUBSCRIPT_ONE]) &&
            (data[i + DRM_ARR_SUBSCRIPT_TWO] == VIDEO_FRAME_ARR[DRM_ARR_SUBSCRIPT_TWO])) {
            uint32_t startPos = DRM_INVALID_START_POS;
            if (ceiStartPos != DRM_INVALID_START_POS) {
                ceiEndPos = i;
                AVCODEC_LOGD("cei found, start pos:%{public}x end pos:%{public}x", ceiStartPos, ceiEndPos);
            }
            /* found a nal unit, process nal to find the cei.*/
            if (!DrmFindCeiNalUnit(data, dataSize, startPos, i)) {
                break;
            }
            if (startPos != DRM_INVALID_START_POS) {
                ceiStartPos = startPos;
                ceiEndPos = DRM_INVALID_START_POS;
            }
            i += DRM_LEGACY_LEN;
        }
    }
    if ((ceiStartPos != DRM_INVALID_START_POS) && (ceiEndPos != DRM_INVALID_START_POS) &&
        (ceiStartPos < ceiEndPos) && (ceiEndPos <= dataSize)) {
        return 1; // 1 true
    }
    return 0;
}

void CodecDrmDecrypt::DrmFindEncryptionFlagPos(const uint8_t *data, uint32_t dataSize, uint32_t &pos)
{
    uint32_t offset = pos;
    if (data[offset + DRM_LEGACY_LEN] == DRM_AVS_FLAG) {
        offset += DRM_LEGACY_LEN; //skip 0x00 0x00 0x01
        offset += 2; // 2 skip this flag
    } else {
        for (; (offset + DRM_USER_DATA_REGISTERED_UUID_SIZE < dataSize); offset++) {
            if (memcmp(data + offset, USER_REGISTERED_UUID, DRM_USER_DATA_REGISTERED_UUID_SIZE) == 0) {
                offset += DRM_USER_DATA_REGISTERED_UUID_SIZE;
                break;
            }
        }
    }
    if (offset < dataSize) {
        pos = offset;
    }
    return;
}

int CodecDrmDecrypt::DrmGetKeyId(uint8_t *data, uint32_t &dataSize, uint32_t &pos, MetaDrmCencInfo *cencInfo)
{
    uint32_t offset = pos;
    if (offset >= dataSize) {
        AVCODEC_LOGE("cei data too short");
        return -1;
    }
    uint8_t encryptionFlag = (data[offset] & 0x80) >> 7; // 0x80 get encryptionFlag & 7 get bits
    uint8_t nextKeyIdFlag = (data[offset] & 0x40) >> 6; // 0x40 get nextKeyIdFlag & 6 get bits
    offset += 1; // 1 skip flag
    DrmRemoveAmbiguityBytes(data, dataSize, offset, dataSize);

    if (encryptionFlag != 0) {
        if ((offset + META_DRM_KEY_ID_SIZE) > dataSize) {
            AVCODEC_LOGE("cei data too short");
            return -1;
        }
        errno_t res = memcpy_s(cencInfo->keyId, META_DRM_KEY_ID_SIZE, data + offset, META_DRM_KEY_ID_SIZE);
        if (res != EOK) {
            AVCODEC_LOGE("copy keyid err");
            return -1;
        }
        cencInfo->keyIdLen = META_DRM_KEY_ID_SIZE;
        offset += META_DRM_KEY_ID_SIZE;
    } else {
        cencInfo->algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED;
    }
    if (nextKeyIdFlag == 1) {
        offset += META_DRM_KEY_ID_SIZE;
    }
    pos = offset;
    return 0;
}

int CodecDrmDecrypt::DrmGetKeyIv(const uint8_t *data, uint32_t dataSize, uint32_t &pos, MetaDrmCencInfo *cencInfo)
{
    uint32_t offset = pos;
    if (offset >= dataSize) {
        AVCODEC_LOGE("cei data too short");
        return -1;
    }
    uint32_t ivLen = data[offset];
    offset += 1; // 1 skip iv len
    if (offset + ivLen > dataSize) {
        AVCODEC_LOGE("cei data too short");
        return -1;
    } else {
        errno_t res = memcpy_s(cencInfo->iv, ivLen, data + offset, ivLen);
        if (res != EOK) {
            AVCODEC_LOGE("copy iv err");
            return -1;
        }
        cencInfo->ivLen = ivLen;
        offset += ivLen;
    }
    pos = offset;
    return 0;
}

int CodecDrmDecrypt::DrmParseDrmDescriptor(const uint8_t *data, uint32_t dataSize, uint32_t &pos,
    uint8_t drmDescriptorFlag, MetaDrmCencInfo *cencInfo)
{
    uint32_t offset = pos;
    if (drmDescriptorFlag == 0) {
        return 0;
    }
    if (offset + DRM_MIN_DRM_INFO_LEN >= dataSize) {
        AVCODEC_LOGE("cei data too short");
        return -1;
    }
    uint8_t videoAlgo = data[offset + DRM_MIN_DRM_INFO_LEN] & 0x0f; // video algo offset
    SetDrmAlgoAndBlocks(videoAlgo, cencInfo);
    offset = offset + DRM_MIN_DRM_INFO_LEN;
    pos = offset;
    return 0;
}

void CodecDrmDecrypt::DrmSetKeyInfo(const uint8_t *data, uint32_t dataSize, uint32_t ceiStartPos,
    uint8_t &isAmbiguity, MetaDrmCencInfo *cencInfo)
{
    uint32_t totalSize = dataSize;
    uint32_t pos = ceiStartPos;
    uint8_t *ceiBuf = nullptr;
    uint8_t drmDescriptorFlag = 0;
    uint8_t drmNotAmbiguityFlag = 0;
    CHECK_AND_RETURN_LOG((dataSize != 0), "DrmSetKeyInfo dataSize is 0");
    ceiBuf = reinterpret_cast<uint8_t *>(malloc(dataSize));
    if (ceiBuf == nullptr) {
        AVCODEC_LOGE("malloc cei data failed");
        return;
    }
    errno_t res = memcpy_s(ceiBuf, dataSize, data, dataSize);
    if (res != EOK) {
        free(ceiBuf);
        return;
    }
    if (pos + DRM_LEGACY_LEN >= totalSize) {
        free(ceiBuf);
        return;
    }
    DrmFindEncryptionFlagPos(ceiBuf, totalSize, pos);
    drmDescriptorFlag = (ceiBuf[pos] & 0x20) >> 5; // 0x20 get drmDescriptorFlag & 5 get bits
    drmNotAmbiguityFlag = (ceiBuf[pos] & 0x10) >> 4; // 0x10 get drmNotAmbiguityFlag & 4 get bits
    if (drmNotAmbiguityFlag == 1) {
        isAmbiguity = 0;
    } else {
        isAmbiguity = 1;  // 1:exist ambiguity
    }

    int ret = DrmGetKeyId(ceiBuf, totalSize, pos, cencInfo);
    if (ret != 0) {
        free(ceiBuf);
        return;
    }
    ret = DrmGetKeyIv(ceiBuf, totalSize, pos, cencInfo);
    if (ret != 0) {
        free(ceiBuf);
        return;
    }
    ret = DrmParseDrmDescriptor(ceiBuf, totalSize, pos, drmDescriptorFlag, cencInfo);
    if (ret != 0) {
        free(ceiBuf);
        return;
    }
    free(ceiBuf);
    return;
}

void CodecDrmDecrypt::DrmGetCencInfo(std::shared_ptr<AVBuffer> inBuf, uint32_t dataSize, uint8_t &isAmbiguity,
    MetaDrmCencInfo *cencInfo) const
{
    int ret;
    uint32_t ceiStartPos = DRM_INVALID_START_POS;
    uint32_t ceiEndPos = DRM_INVALID_START_POS;
    if (inBuf->memory_ == nullptr || inBuf->memory_->GetAddr() == nullptr || dataSize == 0 ||
        dataSize > DRM_MAX_STREAM_DATA_SIZE) {
        AVCODEC_LOGE("DrmGetCencInfo parameter err");
        return;
    }
    uint8_t *data = inBuf->memory_->GetAddr();

    ret = DrmFindCeiPos(data, dataSize, ceiStartPos, ceiEndPos);
    if (ret) {
        DrmSetKeyInfo(data, ceiEndPos, ceiStartPos, isAmbiguity, cencInfo);
    }
    return;
}

int32_t CodecDrmDecrypt::DrmVideoCencDecrypt(std::shared_ptr<AVBuffer> &inBuf, std::shared_ptr<AVBuffer> &outBuf,
    uint32_t &dataSize)
{
    AVCODEC_LOGD("DrmVideoCencDecrypt");
    int32_t ret = AVCS_ERR_UNKNOWN;
    CHECK_AND_RETURN_RET_LOG((inBuf != nullptr && outBuf != nullptr), ret, "DrmVideoCencDecrypt parameter err");
    GetCodingType();
    if (inBuf->meta_ != nullptr) {
        std::vector<uint8_t> drmCencVec;
        MetaDrmCencInfo *cencInfo = nullptr;
        MetaDrmCencInfo clearCencInfo;
        bool res = inBuf->meta_->GetData(Media::Tag::DRM_CENC_INFO, drmCencVec);
        if (res) {
            cencInfo = reinterpret_cast<MetaDrmCencInfo *>(&drmCencVec[0]);
            if (cencInfo->encryptBlocks > DRM_CRYPT_BYTE_BLOCK || cencInfo->skipBlocks > DRM_SKIP_BYTE_BLOCK) {
                AVCODEC_LOGE("DrmVideoCencDecrypt parameter err");
                return ret;
            }
            if (cencInfo->algo == MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED) {
                cencInfo->subSampleNum = 1;
                cencInfo->subSamples[0].clearHeaderLen = dataSize;
                cencInfo->subSamples[0].payLoadLen = 0;
            }
        } else {
            errno_t errCode = memset_s(&clearCencInfo, sizeof(MetaDrmCencInfo), 0, sizeof(MetaDrmCencInfo));
            CHECK_AND_RETURN_RET_LOG(errCode == EOK, ret, "memset cenc info err");
            clearCencInfo.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED;
            clearCencInfo.subSampleNum = 1;
            clearCencInfo.subSamples[0].clearHeaderLen = dataSize;
            clearCencInfo.subSamples[0].payLoadLen = 0;
            cencInfo = reinterpret_cast<MetaDrmCencInfo *>(&clearCencInfo);
        }
        if (cencInfo->mode == MetaDrmCencInfoMode::META_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_NOT_SET ||
            mode_ == MetaDrmCencInfoMode::META_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_NOT_SET) {
            uint8_t isAmbiguity = 1;
            DrmGetCencInfo(inBuf, dataSize, isAmbiguity, cencInfo);
            DrmModifyCencInfo(inBuf, dataSize, isAmbiguity, cencInfo);
            cencInfo->subSampleNum = DRM_TS_SUB_SAMPLE_NUM;
            mode_ = cencInfo->mode;
        }
        ret = DecryptMediaData(cencInfo, inBuf, outBuf);
    }
    return ret;
}

int32_t CodecDrmDecrypt::DrmAudioCencDecrypt(std::shared_ptr<AVBuffer> &inBuf, std::shared_ptr<AVBuffer> &outBuf,
    uint32_t &dataSize)
{
    AVCODEC_LOGD("DrmAudioCencDecrypt");
    int32_t ret = AVCS_ERR_UNKNOWN;
    CHECK_AND_RETURN_RET_LOG((inBuf != nullptr && outBuf != nullptr), ret, "DrmCencDecrypt parameter err");
    if (inBuf->meta_ == nullptr) {
        return ret;
    }

    std::vector<uint8_t> drmCencVec;
    MetaDrmCencInfo *cencInfo = nullptr;
    MetaDrmCencInfo clearCencInfo;
    bool res = inBuf->meta_->GetData(Media::Tag::DRM_CENC_INFO, drmCencVec);
    if (res) {
        cencInfo = reinterpret_cast<MetaDrmCencInfo *>(&drmCencVec[0]);
        if (cencInfo->algo == MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED) {
            cencInfo->subSampleNum = 1;
            cencInfo->subSamples[0].clearHeaderLen = dataSize;
            cencInfo->subSamples[0].payLoadLen = 0;
        }
        if (cencInfo->subSampleNum == 0) {
            if (cencInfo->algo == MetaDrmCencAlgorithm::META_DRM_ALG_CENC_AES_CTR ||
                cencInfo->algo == MetaDrmCencAlgorithm::META_DRM_ALG_CENC_SM4_CTR) {
                cencInfo->subSampleNum = 1; // 1: subSampleNum
                cencInfo->subSamples[0].clearHeaderLen = 0;
                cencInfo->subSamples[0].payLoadLen = dataSize;
            }
            if (cencInfo->algo == MetaDrmCencAlgorithm::META_DRM_ALG_CENC_AES_CBC ||
                cencInfo->algo == MetaDrmCencAlgorithm::META_DRM_ALG_CENC_SM4_CBC) {
                cencInfo->subSampleNum = 2; // 2: subSampleNum
                cencInfo->subSamples[0].clearHeaderLen = 0;
                cencInfo->subSamples[0].payLoadLen = (dataSize / 16) * 16; // 16: BlockSize
                cencInfo->subSamples[1].clearHeaderLen = dataSize % 16; // 16: BlockSize
                cencInfo->subSamples[1].payLoadLen = 0;
            }
        }
    } else {
        errno_t errCode = memset_s(&clearCencInfo, sizeof(MetaDrmCencInfo), 0, sizeof(MetaDrmCencInfo));
        CHECK_AND_RETURN_RET_LOG(errCode == EOK, ret, "memset cenc info err");
        clearCencInfo.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED;
        clearCencInfo.subSampleNum = 1;
        clearCencInfo.subSamples[0].clearHeaderLen = dataSize;
        clearCencInfo.subSamples[0].payLoadLen = 0;
        cencInfo = reinterpret_cast<MetaDrmCencInfo *>(&clearCencInfo);
    }
    ret = DecryptMediaData(cencInfo, inBuf, outBuf);

    return ret;
}

void CodecDrmDecrypt::SetCodecName(const std::string &codecName)
{
    codecName_ = codecName;
}

void CodecDrmDecrypt::GetCodingType()
{
    if (codecName_.find("avc") != codecName_.npos) {
        codingType_ = DRM_VIDEO_AVC;
    } else if (codecName_.find("hevc") != codecName_.npos) {
        codingType_ = DRM_VIDEO_HEVC;
    } else if (codecName_.find("avs") != codecName_.npos) {
        codingType_ = DRM_VIDEO_AVS;
    } else {
        codingType_ = DRM_VIDEO_NONE;
    }
}

void CodecDrmDecrypt::SetDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    const bool svpFlag)
{
    AVCODEC_LOGI("CodecDrmDecrypt SetDecryptConfig");
    std::lock_guard<std::mutex> drmLock(configMutex_);
    CHECK_AND_RETURN_LOG((keySession != nullptr), "SetDecryptConfig keySession nullptr");
    keySessionServiceProxy_ = keySession;
    CHECK_AND_RETURN_LOG((keySession != nullptr), "SetDecryptConfig keySessionServiceProxy nullptr");
    keySessionServiceProxy_->CreateMediaDecryptModule(decryptModuleProxy_);
    CHECK_AND_RETURN_LOG((decryptModuleProxy_ != nullptr), "SetDecryptConfig decryptModuleProxy_ nullptr");
    if (svpFlag) {
        svpFlag_ = SVP_TRUE;
    } else {
        svpFlag_ = SVP_FALSE;
    }
    mode_ = MetaDrmCencInfoMode::META_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET;
}

int32_t CodecDrmDecrypt::SetDrmBuffer(const std::shared_ptr<AVBuffer> &inBuf,
    const std::shared_ptr<AVBuffer> &outBuf, DrmBuffer &inDrmBuffer, DrmBuffer &outDrmBuffer)
{
    AVCODEC_LOGD("CodecDrmDecrypt SetDrmBuffer");
    if (inBuf->memory_ == nullptr || outBuf->memory_ == nullptr) {
        AVCODEC_LOGE("CodecDrmDecrypt SetDrmBuffer memory_ null");
        return AVCS_ERR_NO_MEMORY;
    }
    inDrmBuffer.bufferType = static_cast<uint32_t>(inBuf->memory_->GetMemoryType());
    inDrmBuffer.fd = inBuf->memory_->GetFileDescriptor();
    inDrmBuffer.bufferLen = static_cast<uint32_t>(inBuf->memory_->GetSize());
    if (inBuf->memory_->GetCapacity() < 0) {
        AVCODEC_LOGE("CodecDrmDecrypt SetDrmBuffer input buffer failed due to GetCapacity() return -1");
        return AVCS_ERR_NO_MEMORY;
    }
    inDrmBuffer.allocLen = static_cast<uint32_t>(inBuf->memory_->GetCapacity());
    inDrmBuffer.filledLen = static_cast<uint32_t>(inBuf->memory_->GetSize());
    inDrmBuffer.offset = static_cast<uint32_t>(inBuf->memory_->GetOffset());
    inDrmBuffer.sharedMemType = static_cast<uint32_t>(inBuf->memory_->GetMemoryFlag());

    outDrmBuffer.bufferType = static_cast<uint32_t>(outBuf->memory_->GetMemoryType());
    outDrmBuffer.fd = outBuf->memory_->GetFileDescriptor();
    outDrmBuffer.bufferLen = static_cast<uint32_t>(outBuf->memory_->GetSize());
    if (outBuf->memory_->GetCapacity() < 0) {
        AVCODEC_LOGE("CodecDrmDecrypt SetDrmBuffer output buffer failed due to GetCapacity() return -1");
        return AVCS_ERR_NO_MEMORY;
    }
    outDrmBuffer.allocLen = static_cast<uint32_t>(outBuf->memory_->GetCapacity());
    outDrmBuffer.filledLen = static_cast<uint32_t>(outBuf->memory_->GetSize());
    outDrmBuffer.offset = static_cast<uint32_t>(outBuf->memory_->GetOffset());
    outDrmBuffer.sharedMemType = static_cast<uint32_t>(outBuf->memory_->GetMemoryFlag());
    return AVCS_ERR_OK;
}

int32_t CodecDrmDecrypt::DecryptMediaData(const MetaDrmCencInfo * const cencInfo, std::shared_ptr<AVBuffer> &inBuf,
    std::shared_ptr<AVBuffer> &outBuf)
{
    AVCODEC_LOGI("CodecDrmDecrypt DecryptMediaData");
#ifdef SUPPORT_DRM
    std::lock_guard<std::mutex> drmLock(configMutex_);
    int32_t retCode = AVCS_ERR_INVALID_VAL;
    DrmStandard::IMediaDecryptModuleService::CryptInfo cryptInfo;
    CHECK_AND_RETURN_RET_LOG(((cencInfo->keyIdLen <= static_cast<uint32_t>(META_DRM_KEY_ID_SIZE)) &&
        (cencInfo->ivLen <= static_cast<uint32_t>(META_DRM_IV_SIZE)) &&
        (cencInfo->subSampleNum <= static_cast<uint32_t>(META_DRM_MAX_SUB_SAMPLE_NUM))), retCode, "parameter err");
    cryptInfo.type = static_cast<DrmStandard::IMediaDecryptModuleService::CryptAlgorithmType>(cencInfo->algo);
    std::vector<uint8_t> keyIdVector(cencInfo->keyId, cencInfo->keyId + cencInfo->keyIdLen);
    cryptInfo.keyId = keyIdVector;
    std::vector<uint8_t> ivVector(cencInfo->iv, cencInfo->iv + cencInfo->ivLen);
    cryptInfo.iv = ivVector;
    cryptInfo.pattern.encryptBlocks = cencInfo->encryptBlocks;
    cryptInfo.pattern.skipBlocks = cencInfo->skipBlocks;

    for (uint32_t i = 0; i < cencInfo->subSampleNum; i++) {
        DrmStandard::IMediaDecryptModuleService::SubSample temp({ cencInfo->subSamples[i].clearHeaderLen,
            cencInfo->subSamples[i].payLoadLen });
        cryptInfo.subSample.emplace_back(temp);
    }

    DrmBuffer inDrmBuffer;
    DrmBuffer outDrmBuffer;
    retCode = SetDrmBuffer(inBuf, outBuf, inDrmBuffer, outDrmBuffer);
    CHECK_AND_RETURN_RET_LOG((retCode == AVCS_ERR_OK), retCode, "SetDecryptConfig failed cause SetDrmBuffer failed");
    retCode = AVCS_ERR_INVALID_VAL;
    CHECK_AND_RETURN_RET_LOG((decryptModuleProxy_ != nullptr), retCode,
        "SetDecryptConfig decryptModuleProxy_ nullptr");
    retCode = decryptModuleProxy_->DecryptMediaData(svpFlag_, cryptInfo, inDrmBuffer, outDrmBuffer);
    if (retCode != 0) {
        AVCODEC_LOGE("CodecDrmDecrypt decrypt failed!");
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
#else
    (void)cencInfo;
    (void)inBuf;
    (void)outBuf;
    return AVCS_ERR_OK;
#endif
}

} // namespace MediaAVCodec
} // namespace OHOS
