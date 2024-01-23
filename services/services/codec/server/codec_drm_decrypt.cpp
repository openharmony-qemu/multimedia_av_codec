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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecServer"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {

#define DRM_VIDEO_FRAME_ARR_LEN            3
#define DRM_AMBIGUITY_ARR_LEN              3
constexpr uint32_t DRM_LEGACY_LEN = 3;
constexpr uint32_t DRM_AES_BLOCK_SIZE = 16;
constexpr uint8_t DRM_AMBIGUITY_START_NUM = 0x00;
constexpr uint8_t DRM_AMBIGUITY_END_NUM = 0x03;
constexpr uint32_t DRM_TS_FLAG_CRYPT_BYTE_BLOCK = 2;
constexpr uint32_t DRM_CRYPT_BYTE_BLOCK = 3; // 3:(1 + 2) 2: DRM_TS_FLAG_CRYPT_BYTE_BLOCK
constexpr uint32_t DRM_SKIP_BYTE_BLOCK = 9;
constexpr uint32_t DRM_H264_VIDEO_SKIP_BYTES = 35; // 35:(32 + 3) 32: bytes 3:3bytes
constexpr uint32_t DRM_H265_VIDEO_SKIP_BYTES = 68; // 68:(65 + 3) // 65: bytes 3:3bytes
constexpr uint8_t DRM_SHIFT_LEFT_NUM = 1;
constexpr uint8_t DRM_H264_VIDEO_NAL_TYPE_UMASK_NUM = 0x1f;
constexpr uint8_t DRM_H265_VIDEO_NAL_TYPE_UMASK_NUM = 0x3f;
constexpr uint8_t DRM_H265_VIDEO_START_NAL_TYPE = 0;
constexpr uint8_t DRM_H265_VIDEO_END_NAL_TYPE = 31;
constexpr uint32_t DRM_TS_SUB_SAMPLE_NUM = 2;
constexpr uint8_t DRM_H264_VIDEO_START_NAL_TYPE = 1;
constexpr uint8_t DRM_H264_VIDEO_END_NAL_TYPE = 5;
constexpr uint32_t DRM_MAX_STREAM_DATA_SIZE = 20971520; // 20971520:(20 * 1024 * 1024) 20MB, 1024:1024 bytes = 1kb

static const uint8_t VIDEO_FRAME_ARR[DRM_VIDEO_FRAME_ARR_LEN] = { 0x00, 0x00, 0x01 };
static const uint8_t AMBIGUITY_ARR[DRM_AMBIGUITY_ARR_LEN] = { 0x00, 0x00, 0x03 };

typedef enum {
    DRM_ARR_SUBSCRIPT_ZERO = 0,
    DRM_ARR_SUBSCRIPT_ONE,
    DRM_ARR_SUBSCRIPT_TWO,
    DRM_ARR_SUBSCRIPT_THREE,
} DRM_ArrSubscriptCollection;

typedef enum {
    DRM_VIDEO_AVC = 0x1,
    DRM_VIDEO_HEVC,
    DRM_VIDEO_NONE,
} DRM_CodecType;

void CodecDrmDecrypt::DrmGetSkipClearBytes(uint32_t &skipBytes) const
{
    if (codingType_ == DRM_VIDEO_AVC) {
        skipBytes = DRM_H264_VIDEO_SKIP_BYTES;
    } else if (codingType_ == DRM_VIDEO_HEVC) {
        skipBytes = DRM_H265_VIDEO_SKIP_BYTES;
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

void CodecDrmDecrypt::DrmModifyCencInfo(uint8_t *data, uint32_t &dataSize, MetaDrmCencInfo *cencInfo) const
{
    uint8_t nalType;
    uint32_t posStartIndex;
    uint32_t posEndIndex;
    uint32_t skipBytes = 0;
    uint32_t delLen = 0;
    uint32_t i;
    if (dataSize > DRM_MAX_STREAM_DATA_SIZE) {
        AVCODEC_LOGE("data size too large");
        return;
    }
    DrmGetSkipClearBytes(skipBytes);
    nalType = DrmGetFinalNalTypeAndIndex(data, dataSize, posStartIndex, posEndIndex);
    if (cencInfo->isAmbiguity == 1) {
        DrmRemoveAmbiguityBytes(data, posEndIndex, posStartIndex, dataSize);
    }
    for (i = posEndIndex - 1; i > 0; i--) {
        if (data[i] != 0) {
            break;
        }
        delLen++;
    }

    cencInfo->subSample[0].clearHeaderLen = dataSize;
    cencInfo->subSample[0].payLoadLen = 0;
    cencInfo->subSample[1].clearHeaderLen = 0;
    cencInfo->subSample[1].payLoadLen = 0;
    if (((codingType_ == DRM_VIDEO_AVC) && ((nalType == DRM_H264_VIDEO_START_NAL_TYPE) ||
        (nalType == DRM_H264_VIDEO_END_NAL_TYPE))) ||
        ((codingType_ == DRM_VIDEO_HEVC) &&
        (nalType >= DRM_H265_VIDEO_START_NAL_TYPE) && (nalType <= DRM_H265_VIDEO_END_NAL_TYPE))) {
        uint32_t clearHeaderLen = posStartIndex + skipBytes;
        uint32_t payLoadLen =
            (posEndIndex > (clearHeaderLen + delLen)) ? (posEndIndex - clearHeaderLen - delLen) : 0;
        if (payLoadLen > 0) {
            uint32_t lastClearLen = (payLoadLen % DRM_AES_BLOCK_SIZE == 0) ? DRM_AES_BLOCK_SIZE
                                    : (payLoadLen % DRM_AES_BLOCK_SIZE);
            payLoadLen = payLoadLen - lastClearLen;
            cencInfo->subSample[0].clearHeaderLen = clearHeaderLen;
            cencInfo->subSample[0].payLoadLen = payLoadLen;
            cencInfo->subSample[1].clearHeaderLen = lastClearLen + delLen + (dataSize - posEndIndex);
            cencInfo->subSample[1].payLoadLen = 0;
        }
    }
    return;
}

void CodecDrmDecrypt::DrmCencDecrypt(std::shared_ptr<AVBuffer> inBuf, std::shared_ptr<AVBuffer> outBuf,
    uint32_t &dataSize)
{
    CHECK_AND_RETURN_LOG((inBuf != nullptr && outBuf != nullptr), "DrmCencDecrypt parameter err");
    GetCodingType();
    if (inBuf->meta_ != nullptr) {
        std::vector<uint8_t> drmCencVec;
        MetaDrmCencInfo *cencInfo = nullptr;
        MetaDrmCencInfo clearCencInfo;
        bool ret = inBuf->meta_->GetData(Media::Tag::DRM_CENC_INFO, drmCencVec);
        if (ret) {
            cencInfo = reinterpret_cast<MetaDrmCencInfo *>(&drmCencVec[0]);
            if (cencInfo->encryptBlocks > DRM_CRYPT_BYTE_BLOCK || cencInfo->skipBlocks > DRM_SKIP_BYTE_BLOCK) {
                AVCODEC_LOGE("DrmCencDecrypt parameter err");
                return;
            }
            uint32_t sumBlocks = cencInfo->encryptBlocks + cencInfo->skipBlocks;
            if ((sumBlocks == DRM_TS_FLAG_CRYPT_BYTE_BLOCK) ||
                (sumBlocks == (DRM_CRYPT_BYTE_BLOCK + DRM_SKIP_BYTE_BLOCK))) {
                DrmModifyCencInfo(inBuf->memory_->GetAddr(), dataSize, cencInfo);
                cencInfo->subSampleNum = DRM_TS_SUB_SAMPLE_NUM;
                cencInfo->encryptBlocks = (cencInfo->encryptBlocks >= DRM_TS_FLAG_CRYPT_BYTE_BLOCK) ?
                    (cencInfo->encryptBlocks - DRM_TS_FLAG_CRYPT_BYTE_BLOCK) : (cencInfo->encryptBlocks);
            }
            if (cencInfo->algo == MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED) {
                cencInfo->subSampleNum = 1;
                cencInfo->subSample[0].clearHeaderLen = dataSize;
                cencInfo->subSample[0].payLoadLen = 0;
            }
        } else {
            errno_t res = memset_s(&clearCencInfo, sizeof(MetaDrmCencInfo), 0, sizeof(MetaDrmCencInfo));
            CHECK_AND_RETURN_LOG(res == EOK, "memset cenc info err");
            clearCencInfo.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED;
            clearCencInfo.subSampleNum = 1;
            clearCencInfo.subSample[0].clearHeaderLen = dataSize;
            clearCencInfo.subSample[0].payLoadLen = 0;
            cencInfo = reinterpret_cast<MetaDrmCencInfo *>(&clearCencInfo);
        }
        DecryptMediaData(cencInfo, inBuf, outBuf);
    }
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
    } else {
        codingType_ = DRM_VIDEO_NONE;
    }
}

void CodecDrmDecrypt::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
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
}

int32_t CodecDrmDecrypt::SetDrmBuffer(const std::shared_ptr<AVBuffer> &inBuf,
    const std::shared_ptr<AVBuffer> &outBuf, DrmBuffer &inDrmBuffer, DrmBuffer &outDrmBuffer)
{
    AVCODEC_LOGD("CodecDrmDecrypt SetDrmBuffer");
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

int32_t CodecDrmDecrypt::DecryptMediaData(const MetaDrmCencInfo * const cencInfo, std::shared_ptr<AVBuffer> inBuf,
    std::shared_ptr<AVBuffer> outBuf)
{
    AVCODEC_LOGI("CodecDrmDecrypt DecryptMediaData");
#ifdef SUPPORT_DRM
    std::lock_guard<std::mutex> drmLock(configMutex_);
    int32_t retCode = AVCS_ERR_INVALID_VAL;
    DrmStandard::IMediaDecryptModuleService::CryptInfo cryptInfo;
    cryptInfo.type = static_cast<DrmStandard::IMediaDecryptModuleService::CryptAlgorithmType>(cencInfo->algo);
    std::vector<uint8_t> keyIdVector(cencInfo->keyId, cencInfo->keyId + cencInfo->keyIdLen);
    cryptInfo.keyId = keyIdVector;
    std::vector<uint8_t> ivVector(cencInfo->iv, cencInfo->iv + cencInfo->ivLen);
    cryptInfo.iv = ivVector;

    cryptInfo.pattern.encryptBlocks = cencInfo->encryptBlocks;
    cryptInfo.pattern.skipBlocks = cencInfo->skipBlocks;

    for (uint32_t i = 0; i < cencInfo->subSampleNum; i++) {
        DrmStandard::IMediaDecryptModuleService::SubSample temp({ cencInfo->subSample[i].clearHeaderLen,
            cencInfo->subSample[i].payLoadLen });
        cryptInfo.subSample.emplace_back(temp);
    }

    DrmBuffer inDrmBuffer;
    DrmBuffer outDrmBuffer;
    retCode = SetDrmBuffer(inBuf, outBuf, inDrmBuffer, outDrmBuffer);
    CHECK_AND_RETURN_RET_LOG((retCode == AVCS_ERR_OK), retCode, "SetDecryptConfig failed cause SetDrmBuffer failed");

    CHECK_AND_RETURN_RET_LOG((decryptModuleProxy_ != nullptr), retCode,
        "SetDecryptConfig decryptModuleProxy_ nullptr");
    retCode = decryptModuleProxy_->DecryptMediaData(svpFlag_, cryptInfo, inDrmBuffer, outDrmBuffer);
    if (retCode != 0) {
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
