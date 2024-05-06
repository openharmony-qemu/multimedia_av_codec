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

#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "codec_drm_decrypt.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "CodecServer"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
void CodecDrmDecrypt::DrmGetSkipClearBytes(uint32_t &skipBytes) const
{
    (void)skipBytes;
}

int32_t CodecDrmDecrypt::DrmGetNalTypeAndIndex(const uint8_t *data, uint32_t dataSize, uint8_t &nalType,
                                               uint32_t &posIndex) const
{
    (void)data;
    (void)dataSize;
    (void)nalType;
    (void)posIndex;
    return AVCS_ERR_OK;
}

void CodecDrmDecrypt::DrmGetSyncHeaderIndex(const uint8_t *data, uint32_t dataSize, uint32_t &posIndex)
{
    (void)data;
    (void)dataSize;
    (void)posIndex;
}

uint8_t CodecDrmDecrypt::DrmGetFinalNalTypeAndIndex(const uint8_t *data, uint32_t dataSize, uint32_t &posStartIndex,
                                                    uint32_t &posEndIndex) const
{
    (void)data;
    (void)dataSize;
    (void)posStartIndex;
    (void)posEndIndex;
    return 0;
}

void CodecDrmDecrypt::DrmRemoveAmbiguityBytes(uint8_t *data, uint32_t &posEndIndex, uint32_t offset, uint32_t &dataSize)
{
    (void)data;
    (void)posEndIndex;
    (void)offset;
    (void)dataSize;
}

void CodecDrmDecrypt::DrmModifyCencInfo(std::shared_ptr<AVBuffer> inBuf, uint32_t &dataSize, uint8_t isAmbiguity,
    MetaDrmCencInfo *cencInfo) const
{
    (void)inBuf;
    (void)dataSize;
    (void)isAmbiguity;
    (void)cencInfo;
}

void CodecDrmDecrypt::SetDrmAlgoAndBlocks(uint8_t algo, MetaDrmCencInfo *cencInfo)
{
    (void)algo;
    (void)cencInfo;
}

int CodecDrmDecrypt::DrmFindAvsCeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos,
    uint32_t index)
{
    (void)data;
    (void)dataSize;
    (void)ceiStartPos;
    (void)index;
    return AVCS_ERR_OK;
}

int CodecDrmDecrypt::DrmFindHevcCeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos,
    uint32_t index)
{
    (void)data;
    (void)dataSize;
    (void)ceiStartPos;
    (void)index;
    return AVCS_ERR_OK;
}

int CodecDrmDecrypt::DrmFindH264CeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos,
    uint32_t index)
{
    (void)data;
    (void)dataSize;
    (void)ceiStartPos;
    (void)index;
    return AVCS_ERR_OK;
}

int CodecDrmDecrypt::DrmFindCeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos,
    uint32_t index) const
{
    (void)data;
    (void)dataSize;
    (void)ceiStartPos;
    (void)index;
    return AVCS_ERR_OK;
}

int CodecDrmDecrypt::DrmFindCeiPos(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos,
    uint32_t &ceiEndPos) const
{
    (void)data;
    (void)dataSize;
    (void)ceiStartPos;
    (void)ceiEndPos;
    return AVCS_ERR_OK;
}

void CodecDrmDecrypt::DrmFindEncryptionFlagPos(const uint8_t *data, uint32_t dataSize, uint32_t &pos)
{
    (void)data;
    (void)dataSize;
    (void)pos;
}

int CodecDrmDecrypt::DrmGetKeyId(uint8_t *data, uint32_t &dataSize, uint32_t &pos, MetaDrmCencInfo *cencInfo)
{
    (void)data;
    (void)dataSize;
    (void)pos;
    (void)cencInfo;
    return AVCS_ERR_OK;
}

int CodecDrmDecrypt::DrmGetKeyIv(const uint8_t *data, uint32_t dataSize, uint32_t &pos, MetaDrmCencInfo *cencInfo)
{
    (void)data;
    (void)dataSize;
    (void)pos;
    (void)cencInfo;
    return AVCS_ERR_OK;
}

int CodecDrmDecrypt::DrmParseDrmDescriptor(const uint8_t *data, uint32_t dataSize, uint32_t &pos,
    uint8_t drmDescriptorFlag, MetaDrmCencInfo *cencInfo)
{
    (void)data;
    (void)dataSize;
    (void)pos;
    (void)drmDescriptorFlag;
    (void)cencInfo;
    return AVCS_ERR_OK;
}

void CodecDrmDecrypt::DrmSetKeyInfo(const uint8_t *data, uint32_t dataSize, uint32_t ceiStartPos,
    uint8_t &isAmbiguity, MetaDrmCencInfo *cencInfo)
{
    (void)data;
    (void)dataSize;
    (void)ceiStartPos;
    (void)isAmbiguity;
    (void)cencInfo;
}

void CodecDrmDecrypt::DrmGetCencInfo(std::shared_ptr<AVBuffer> inBuf, uint32_t dataSize, uint8_t &isAmbiguity,
    MetaDrmCencInfo *cencInfo) const
{
    (void)inBuf;
    (void)dataSize;
    (void)isAmbiguity;
    (void)cencInfo;
}

int32_t CodecDrmDecrypt::DrmVideoCencDecrypt(std::shared_ptr<AVBuffer> &inBuf, std::shared_ptr<AVBuffer> &outBuf,
    uint32_t &dataSize)
{
    (void)inBuf;
    (void)outBuf;
    (void)dataSize;
    return 0;
}

int32_t CodecDrmDecrypt::DrmAudioCencDecrypt(std::shared_ptr<AVBuffer> &inBuf, std::shared_ptr<AVBuffer> &outBuf,
    uint32_t &dataSize)
{
    (void)inBuf;
    (void)outBuf;
    (void)dataSize;
    return 0;
}

void CodecDrmDecrypt::SetCodecName(const std::string &codecName)
{
    (void)codecName;
}

void CodecDrmDecrypt::GetCodingType()
{
}

void CodecDrmDecrypt::SetDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    const bool svpFlag)
{
    (void)keySession;
    (void)svpFlag;
}

int32_t CodecDrmDecrypt::SetDrmBuffer(const std::shared_ptr<AVBuffer> &inBuf, const std::shared_ptr<AVBuffer> &outBuf,
                                      DrmBuffer &inDrmBuffer, DrmBuffer &outDrmBuffer)
{
    AVCODEC_LOGD("CodecDrmDecrypt SetDrmBuffer");
    (void)inBuf;
    (void)outBuf;
    (void)inDrmBuffer;
    (void)outDrmBuffer;
    return AVCS_ERR_OK;
}

int32_t CodecDrmDecrypt::DecryptMediaData(const MetaDrmCencInfo *const cencInfo, std::shared_ptr<AVBuffer> &inBuf,
                                          std::shared_ptr<AVBuffer> &outBuf)
{
    AVCODEC_LOGI("CodecDrmDecrypt DecryptMediaData");
    (void)cencInfo;
    (void)inBuf;
    (void)outBuf;
    return AVCS_ERR_OK;
}

} // namespace MediaAVCodec
} // namespace OHOS
