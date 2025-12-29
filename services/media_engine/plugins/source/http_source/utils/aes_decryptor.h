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

#ifndef AES_DECRYPTOR_H
#define AES_DECRYPTOR_H

#include <memory>
#include <mutex>
#include <atomic>
#include <unistd.h>
#include "openssl/aes.h"

namespace OHOS {
namespace Media {
class AesDecryptor {
public:
    AesDecryptor();
    ~AesDecryptor();
    void Init() const;
    void Decrypt(uint8_t *in, uint8_t *out, uint32_t len);
    void OnSourceKeyChange(const uint8_t* key, size_t keyLen, const uint8_t* iv);
    
    static constexpr uint64_t BLOCK_LEN = 16;
    static constexpr uint32_t KEY_BITS = 128;
    uint8_t key_[AesDecryptor::BLOCK_LEN] = {0};
    size_t keyLen_ {0};
    uint8_t iv_[AesDecryptor::BLOCK_LEN] = {0};
    uint8_t initIv_[AesDecryptor::BLOCK_LEN] = {0};

private:
    AES_KEY aesKey_;
};

} // namespace Media
} // namespace OHOS
#endif // AES_DECRYPTOR_H
