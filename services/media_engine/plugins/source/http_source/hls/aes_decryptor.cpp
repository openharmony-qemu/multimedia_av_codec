/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "AesDecryptor"

#include "aes_decryptor.h"
#include "openssl/aes.h"
#include "common/log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_PLAYER, "AesDecryptor" };
}

namespace OHOS {
namespace Media {
AesDecryptor::~AesDecryptor()
{
    NZERO_LOG(memset_s(iv_, DECRYPT_UNIT_LEN, 0, DECRYPT_UNIT_LEN));
    NZERO_LOG(memset_s(initIv_, DECRYPT_UNIT_LEN, 0, DECRYPT_UNIT_LEN));
    NZERO_LOG(memset_s(key_, DECRYPT_UNIT_LEN, 0, DECRYPT_UNIT_LEN));

}

void AesDecryptor::Init()
{
    aesKey_.rounds = 0;
    for (size_t i = 0; i < sizeof(aesKey_.rd_key) / sizeof(aesKey_.rd_key[0]); ++i) {
        aesKey_.rd_key[i] = 0;
    }
}

void AesDecryptor::OnSourceKeyChange(const uint8_t* key, size_t keyLen, const uint8_t* iv)
{
    keyLen_ = keyLen;
    if (keyLen <= 0) {
        return;
    }
    NZERO_LOG(memcpy_s(iv_, DECRYPT_UNIT_LEN, iv, DECRYPT_UNIT_LEN));
    NZERO_LOG(memcpy_s(initIv_, DECRYPT_UNIT_LEN, iv, DECRYPT_UNIT_LEN));
    NZERO_LOG(memcpy_s(key_, DECRYPT_UNIT_LEN, key, keyLen > DECRYPT_UNIT_LEN ? DECRYPT_UNIT_LEN : keyLen));
    AES_set_decrypt_key(key_, DECRYPT_COPY_LEN, &aesKey_);
}

void AesDecryptor::Decrypt(uint8_t* decryptBuffer, uint8_t* decryptCache, uint32_t realLen)
{
    if (!decryptBuffer || !decryptCache) {
        return;
    }
    AES_cbc_encrypt(decryptBuffer, decryptCache, realLen, &aesKey_, iv_, AES_DECRYPT);
}

} // namespace Media
} // namespace OHOS
