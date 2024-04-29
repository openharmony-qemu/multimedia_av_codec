/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef CODEC_DRM_DECRYPT_MOCK_H
#define CODEC_DRM_DECRYPT_MOCK_H

#include <gmock/gmock.h>
#include <map>
#include <string>
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "codec_drm_decrypt.h"

namespace OHOS {
namespace MediaAVCodec {

class CodecDrmDecryptorMock : public NoCopyable {
public:
    CodecDrmDecryptorMock();
    virtual ~CodecDrmDecryptorMock() = default;
    int32_t DrmVideoCencDecrypt(std::shared_ptr<AVBuffer> &inBuf, std::shared_ptr<AVBuffer> &outBuf,
        uint32_t &dataSize);
    int32_t DrmAudioCencDecrypt(std::shared_ptr<AVBuffer> &inBuf, std::shared_ptr<AVBuffer> &outBuf,
        uint32_t &dataSize);
    void SetCodecName(const std::string &codecName);
    void SetDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag);

private:
    std::shared_ptr<MediaAVCodec::CodecDrmDecrypt> decryptor_ = nullptr;
};

} // name space MediaAVCodec
} // namespace OHOS


#endif