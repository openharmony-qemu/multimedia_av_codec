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
#include "unittest_log.h"
#include "./codec_drm_decrypt_mock.h"

namespace OHOS {
namespace MediaAVCodec {

CodecDrmDecryptorMock::CodecDrmDecryptorMock() : decryptor_(std::make_shared<MediaAVCodec::CodecDrmDecrypt>())
{
}

int32_t CodecDrmDecryptorMock::DrmVideoCencDecrypt(std::shared_ptr<AVBuffer> &inBuf, std::shared_ptr<AVBuffer> &outBuf,
    uint32_t &dataSize)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(decryptor_ != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return decryptor_->DrmVideoCencDecrypt(inBuf, outBuf, dataSize);
}

int32_t CodecDrmDecryptorMock::DrmAudioCencDecrypt(std::shared_ptr<AVBuffer> &inBuf, std::shared_ptr<AVBuffer> &outBuf,
    uint32_t &dataSize)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(decryptor_ != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return decryptor_->DrmAudioCencDecrypt(inBuf, outBuf, dataSize);
}

void CodecDrmDecryptorMock::SetCodecName(const std::string &codecName)
{
    UNITTEST_CHECK_AND_RETURN_LOG(decryptor_ != nullptr, "mock object is nullptr");
    return decryptor_->SetCodecName(codecName);
}

void CodecDrmDecryptorMock::SetDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    const bool svpFlag)
{
    UNITTEST_CHECK_AND_RETURN_LOG(decryptor_ != nullptr, "mock object is nullptr");
    return decryptor_->SetDecryptionConfig(keySession, svpFlag);
}
} // namespace MediaAVCodec
} // namespace OHOS
