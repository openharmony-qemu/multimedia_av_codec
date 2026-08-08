/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
#include "securec.h"
#include <openssl/crypto.h>

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_PLAYER, "HiStreamerAesDec" };
}

namespace OHOS {
namespace Media {
AesDecryptor::AesDecryptor()
{
    aesKey_.rounds = 0;
    for (size_t i = 0; i < sizeof(aesKey_.rd_key) / sizeof(aesKey_.rd_key[0]); ++i) {
        aesKey_.rd_key[i] = 0;
    }
}

AesDecryptor::~AesDecryptor()
{
    Reset();
}

void AesDecryptor::Init()
{
}

void AesDecryptor::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ResetInner();
}

void AesDecryptor::ResetInner()
{
    keyLen_ = 0;
    NZERO_LOG(memset_s(iv_, BLOCK_LEN, 0, BLOCK_LEN));
    NZERO_LOG(memset_s(initIv_, BLOCK_LEN, 0, BLOCK_LEN));
    NZERO_LOG(memset_s(key_, BLOCK_LEN, 0, BLOCK_LEN));
    OPENSSL_cleanse(&aesKey_, sizeof(aesKey_));
}

void AesDecryptor::OnSourceKeyChange(const uint8_t* key, size_t keyLen, const uint8_t* iv)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (keyLen == 0 || !key || !iv) {
        keyLen_ = 0;
        MEDIA_LOG_W("invalid key or iv.");
        return;
    }
    keyLen_ = keyLen;
    NZERO_LOG(memcpy_s(iv_, BLOCK_LEN, iv, BLOCK_LEN));
    NZERO_LOG(memcpy_s(initIv_, BLOCK_LEN, iv, BLOCK_LEN));
    NZERO_LOG(memcpy_s(key_, BLOCK_LEN, key, keyLen > BLOCK_LEN ? BLOCK_LEN : keyLen));
    int ret = AES_set_decrypt_key(key_, KEY_BITS, &aesKey_);
    if (ret != 0) {
        MEDIA_LOG_E("AES_set_decrypt_key failed, ret: " PUBLIC_LOG_D32, ret);
        ResetInner();
    }
}

void AesDecryptor::Decrypt(uint8_t *in, uint8_t *out, uint32_t len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!in || !out || !len) {
        return;
    }
    if (keyLen_ == 0) {
        return;
    }
    if (len % BLOCK_LEN != 0) {
        MEDIA_LOG_E("Decrypt len " PUBLIC_LOG_U32 " is not aligned to BLOCK_LEN", len);
        return;
    }
    AES_cbc_encrypt(in, out, len, &aesKey_, iv_, AES_DECRYPT);
}

void AesDecryptor::SetIv(const uint8_t* iv, size_t ivLen)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!iv || ivLen < BLOCK_LEN) {
        return;
    }
    auto ret = memcpy_s(iv_, BLOCK_LEN, iv, BLOCK_LEN);
    if (ret != 0) {
        MEDIA_LOG_E("set iv failed, ret: %{public}d", ret);
    }
    ret = memcpy_s(initIv_, BLOCK_LEN, iv, BLOCK_LEN);
    if (ret != 0) {
        MEDIA_LOG_E("set init iv failed, ret: %{public}d", ret);
    }
}

void AesDecryptor::ResetIv()
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto ret = memcpy_s(iv_, BLOCK_LEN, initIv_, BLOCK_LEN);
    if (ret != 0) {
        MEDIA_LOG_E("reset iv failed, ret: %{public}d", ret);
    }
}
} // namespace Media
} // namespace OHOS
