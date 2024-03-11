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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecServer"};
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

void CodecDrmDecrypt::DrmModifyCencInfo(uint8_t *data, uint32_t &dataSize, MetaDrmCencInfo *cencInfo) const
{
    (void)data;
    (void)dataSize;
    (void)cencInfo;
}

void CodecDrmDecrypt::DrmCencDecrypt(std::shared_ptr<AVBuffer> inBuf, std::shared_ptr<AVBuffer> outBuf,
                                     uint32_t &dataSize)
{
    (void)inBuf;
    (void)outBuf;
    (void)dataSize;
}

void CodecDrmDecrypt::SetCodecName(const std::string &codecName)
{
    (void)codecName;
}

void CodecDrmDecrypt::GetCodingType()
{
}

void CodecDrmDecrypt::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag)
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

int32_t CodecDrmDecrypt::DecryptMediaData(const MetaDrmCencInfo *const cencInfo, std::shared_ptr<AVBuffer> inBuf,
                                          std::shared_ptr<AVBuffer> outBuf)
{
    AVCODEC_LOGI("CodecDrmDecrypt DecryptMediaData");
    (void)cencInfo;
    (void)inBuf;
    (void)outBuf;
    return AVCS_ERR_OK;
}

} // namespace MediaAVCodec
} // namespace OHOS
