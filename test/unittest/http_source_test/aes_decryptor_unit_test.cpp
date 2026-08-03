/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"
#include "openssl/aes.h"
#include "securec.h"
#include "utils/aes_decryptor.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

namespace {
constexpr uint32_t SMALL_BLOCK_LEN = 8;
constexpr uint32_t BLOCK_LEN = 16;
constexpr uint32_t LARGE_BLOCK_LEN = 32;
constexpr uint32_t HUGE_BLOCK_LEN = 64;
constexpr uint32_t AES_KEY_BITLEN = 128;
}

class AesDecryptorUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp(void);
    void TearDown(void);
protected:
    std::shared_ptr<AesDecryptor> decryptor_;
};

void AesDecryptorUnitTest::SetUp(void)
{
    decryptor_ = std::make_shared<AesDecryptor>();
}

void AesDecryptorUnitTest::TearDown(void)
{
    decryptor_.reset();
}

HWTEST_F(AesDecryptorUnitTest, Init_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    EXPECT_EQ(decryptor_->keyLen_, 0u);
    decryptor_->Init();
    EXPECT_EQ(decryptor_->keyLen_, 0u);
}

HWTEST_F(AesDecryptorUnitTest, OnSourceKeyChange_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);
    EXPECT_EQ(decryptor_->keyLen_, BLOCK_LEN);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->key_[i], key[i]);
        EXPECT_EQ(decryptor_->iv_[i], iv[i]);
        EXPECT_EQ(decryptor_->initIv_[i], iv[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, OnSourceKeyChange_002, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t iv[BLOCK_LEN] = {0};
    decryptor_->OnSourceKeyChange(nullptr, 0, iv);
    EXPECT_EQ(decryptor_->keyLen_, 0u);
}

HWTEST_F(AesDecryptorUnitTest, OnSourceKeyChange_003, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[SMALL_BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, SMALL_BLOCK_LEN, iv);
    EXPECT_EQ(decryptor_->keyLen_, SMALL_BLOCK_LEN);
    for (size_t i = 0; i < SMALL_BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->key_[i], key[i]);
    }
    for (size_t i = SMALL_BLOCK_LEN; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->key_[i], 0u);
    }
}

HWTEST_F(AesDecryptorUnitTest, OnSourceKeyChange_004, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[LARGE_BLOCK_LEN] = {0};
    for (int i = 0; i < LARGE_BLOCK_LEN; i++) {
        key[i] = static_cast<uint8_t>(i);
    }
    uint8_t iv[BLOCK_LEN] = {0};
    decryptor_->OnSourceKeyChange(key, LARGE_BLOCK_LEN, iv);
    EXPECT_EQ(decryptor_->keyLen_, LARGE_BLOCK_LEN);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->key_[i], key[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t plaintext[BLOCK_LEN] = "HelloWorld12345";
    uint8_t ciphertext[BLOCK_LEN] = {0};
    uint8_t decrypted[BLOCK_LEN] = {0};

    AES_KEY encryptKey;
    AES_set_encrypt_key(key, AES_KEY_BITLEN, &encryptKey);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, ciphertext, BLOCK_LEN, &encryptKey, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);

    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_002, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t plaintext[LARGE_BLOCK_LEN] = "HelloWorld12345TestData32Bytes";
    uint8_t ciphertext[LARGE_BLOCK_LEN] = {0};
    uint8_t decrypted[LARGE_BLOCK_LEN] = {0};

    AES_KEY encryptKey;
    AES_set_encrypt_key(key, AES_KEY_BITLEN, &encryptKey);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, ciphertext, LARGE_BLOCK_LEN, &encryptKey, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext, decrypted, LARGE_BLOCK_LEN);

    for (size_t i = 0; i < LARGE_BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_003, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    EXPECT_EQ(decryptor_->keyLen_, BLOCK_LEN);
    decryptor_->Decrypt(nullptr, nullptr, BLOCK_LEN);
    EXPECT_EQ(decryptor_->keyLen_, BLOCK_LEN);
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_004, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t buffer[BLOCK_LEN] = {0};
    uint8_t original[BLOCK_LEN] = {0};
    (void)memcpy_s(original, BLOCK_LEN, buffer, BLOCK_LEN);
    decryptor_->Decrypt(buffer, nullptr, BLOCK_LEN);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(buffer[i], original[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_005, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t buffer[BLOCK_LEN] = {
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,
        0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00
    };
    uint8_t original[BLOCK_LEN] = {0};
    (void)memcpy_s(original, BLOCK_LEN, buffer, BLOCK_LEN);
    decryptor_->Decrypt(nullptr, buffer, BLOCK_LEN);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(buffer[i], original[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_006, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t buffer[BLOCK_LEN] = "TestData123456";
    uint8_t original[BLOCK_LEN] = {0};
    (void)memcpy_s(original, BLOCK_LEN, buffer, BLOCK_LEN);
    decryptor_->Decrypt(buffer, buffer, 0);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(buffer[i], original[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_MultipleBlocks_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t plaintext[HUGE_BLOCK_LEN] = {0};
    for (int i = 0; i < HUGE_BLOCK_LEN; i++) plaintext[i] = static_cast<uint8_t>(i);
    uint8_t ciphertext[HUGE_BLOCK_LEN] = {0};
    uint8_t decrypted[HUGE_BLOCK_LEN] = {0};

    AES_KEY encryptKey;
    AES_set_encrypt_key(key, AES_KEY_BITLEN, &encryptKey);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, ciphertext, HUGE_BLOCK_LEN, &encryptKey, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext, decrypted, HUGE_BLOCK_LEN);

    for (size_t i = 0; i < HUGE_BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_MultipleBlocks_002, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t plaintext[AES_KEY_BITLEN] = {0};
    for (int i = 0; i < AES_KEY_BITLEN; i++) plaintext[i] = static_cast<uint8_t>(i % 256);
    uint8_t ciphertext[AES_KEY_BITLEN] = {0};
    uint8_t decrypted[AES_KEY_BITLEN] = {0};

    AES_KEY encryptKey;
    AES_set_encrypt_key(key, AES_KEY_BITLEN, &encryptKey);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, ciphertext, AES_KEY_BITLEN, &encryptKey, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext, decrypted, AES_KEY_BITLEN);

    for (size_t i = 0; i < AES_KEY_BITLEN; i++) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, IvReset_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t plaintext[BLOCK_LEN] = "HelloWorld12345";
    uint8_t ciphertext[BLOCK_LEN] = {0};
    uint8_t decrypted1[BLOCK_LEN] = {0};
    uint8_t decrypted2[BLOCK_LEN] = {0};

    AES_KEY encryptKey;
    AES_set_encrypt_key(key, AES_KEY_BITLEN, &encryptKey);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, ciphertext, BLOCK_LEN, &encryptKey, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext, decrypted1, BLOCK_LEN);

    (void)memcpy_s(decryptor_->iv_, BLOCK_LEN, decryptor_->initIv_, BLOCK_LEN);

    decryptor_->Decrypt(ciphertext, decrypted2, BLOCK_LEN);

    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted2[i], decrypted1[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, KeyChange_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key1[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    decryptor_->OnSourceKeyChange(key1, BLOCK_LEN, iv);
    EXPECT_EQ(decryptor_->keyLen_, BLOCK_LEN);

    uint8_t key2[BLOCK_LEN] = {
        0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
        0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0
    };
    decryptor_->OnSourceKeyChange(key2, BLOCK_LEN, iv);
    EXPECT_EQ(decryptor_->keyLen_, BLOCK_LEN);

    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->key_[i], key2[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, KeyChange_002, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv1[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    uint8_t iv2[BLOCK_LEN] = {
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f
    };

    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv1);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->iv_[i], iv1[i]);
        EXPECT_EQ(decryptor_->initIv_[i], iv1[i]);
    }

    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv2);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->iv_[i], iv2[i]);
        EXPECT_EQ(decryptor_->initIv_[i], iv2[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_WithInvalidKey_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    decryptor_->OnSourceKeyChange(nullptr, 0, nullptr);
    EXPECT_EQ(decryptor_->keyLen_, 0u);

    uint8_t buffer[LARGE_BLOCK_LEN] = {0};
    for (int i = 0; i < LARGE_BLOCK_LEN; i++) buffer[i] = static_cast<uint8_t>(i);
    uint8_t output[LARGE_BLOCK_LEN] = {0};
    uint8_t original[LARGE_BLOCK_LEN] = {0};
    (void)memcpy_s(original, LARGE_BLOCK_LEN, output, LARGE_BLOCK_LEN);

    decryptor_->Decrypt(buffer, output, LARGE_BLOCK_LEN);
    for (size_t i = 0; i < LARGE_BLOCK_LEN; i++) {
        EXPECT_EQ(output[i], original[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_WithZeroKeyLen_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(nullptr, 0, iv);
    EXPECT_EQ(decryptor_->keyLen_, 0u);

    uint8_t ciphertext[BLOCK_LEN] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
    };
    uint8_t decrypted[BLOCK_LEN] = {0};
    uint8_t original[BLOCK_LEN] = {0};
    (void)memcpy_s(original, BLOCK_LEN, decrypted, BLOCK_LEN);

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted[i], original[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_WithValidKeyButZeroKeyLen_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, 0, iv);
    EXPECT_EQ(decryptor_->keyLen_, 0u);

    uint8_t ciphertext[BLOCK_LEN] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
    };
    uint8_t decrypted[BLOCK_LEN] = {0};
    uint8_t original[BLOCK_LEN] = {0};
    (void)memcpy_s(original, BLOCK_LEN, decrypted, BLOCK_LEN);

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted[i], original[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_LargeData_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    const size_t dataSize = 1024;
    uint8_t plaintext[dataSize] = {0};
    for (size_t i = 0; i < dataSize; i++) plaintext[i] = static_cast<uint8_t>(i % 256);
    uint8_t ciphertext[dataSize] = {0};
    uint8_t decrypted[dataSize] = {0};

    AES_KEY encryptKey;
    AES_set_encrypt_key(key, AES_KEY_BITLEN, &encryptKey);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, ciphertext, dataSize, &encryptKey, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext, decrypted, dataSize);

    for (size_t i = 0; i < dataSize; i++) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_SequentialBlocks_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t plaintext[48] = "Block1TestDataBlock2TestDataBlock3TestData";
    uint8_t ciphertext[48] = {0};
    uint8_t decrypted[48] = {0};

    AES_KEY encryptKey;
    AES_set_encrypt_key(key, AES_KEY_BITLEN, &encryptKey);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, ciphertext, 48, &encryptKey, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);
    decryptor_->Decrypt(ciphertext + BLOCK_LEN, decrypted + BLOCK_LEN, BLOCK_LEN);
    decryptor_->Decrypt(ciphertext + LARGE_BLOCK_LEN, decrypted + LARGE_BLOCK_LEN, BLOCK_LEN);

    uint8_t fullDecrypted[48] = {0};
    (void)memcpy_s(decryptor_->iv_, BLOCK_LEN, decryptor_->initIv_, BLOCK_LEN);
    decryptor_->Decrypt(ciphertext, fullDecrypted, 48);

    for (size_t i = 0; i < 48; i++) {
        EXPECT_EQ(decrypted[i], fullDecrypted[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, ZeroKey_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {0};
    uint8_t iv[BLOCK_LEN] = {0};
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t plaintext[BLOCK_LEN] = "HelloWorld12345";
    uint8_t ciphertext[BLOCK_LEN] = {0};
    uint8_t decrypted[BLOCK_LEN] = {0};

    AES_KEY encryptKey;
    AES_set_encrypt_key(key, AES_KEY_BITLEN, &encryptKey);
    uint8_t encIv[BLOCK_LEN] = {0};
    AES_cbc_encrypt(plaintext, ciphertext, BLOCK_LEN, &encryptKey, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);

    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, SameInOutBuffer_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t plaintext[BLOCK_LEN] = "HelloWorld12345";
    uint8_t buffer[BLOCK_LEN] = {0};

    AES_KEY encryptKey;
    AES_set_encrypt_key(key, AES_KEY_BITLEN, &encryptKey);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, buffer, BLOCK_LEN, &encryptKey, encIv, AES_ENCRYPT);

    (void)memcpy_s(decryptor_->iv_, BLOCK_LEN, decryptor_->initIv_, BLOCK_LEN);
    decryptor_->Decrypt(buffer, buffer, BLOCK_LEN);

    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(buffer[i], plaintext[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, MultipleDecryptorInstances_001, TestSize.Level1)
{
    auto decryptor1 = std::make_shared<AesDecryptor>();
    auto decryptor2 = std::make_shared<AesDecryptor>();

    ASSERT_NE(decryptor1, nullptr);
    ASSERT_NE(decryptor2, nullptr);

    uint8_t key1[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t key2[BLOCK_LEN] = {
        0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
        0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    decryptor1->OnSourceKeyChange(key1, BLOCK_LEN, iv);
    decryptor2->OnSourceKeyChange(key2, BLOCK_LEN, iv);

    uint8_t plaintext[BLOCK_LEN] = "HelloWorld12345";
    uint8_t ciphertext1[BLOCK_LEN] = {0};
    uint8_t ciphertext2[BLOCK_LEN] = {0};

    AES_KEY encryptKey1, encryptKey2;
    AES_set_encrypt_key(key1, AES_KEY_BITLEN, &encryptKey1);
    AES_set_encrypt_key(key2, AES_KEY_BITLEN, &encryptKey2);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, ciphertext1, BLOCK_LEN, &encryptKey1, encIv, AES_ENCRYPT);
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, ciphertext2, BLOCK_LEN, &encryptKey2, encIv, AES_ENCRYPT);

    uint8_t decrypted1[BLOCK_LEN] = {0};
    uint8_t decrypted2[BLOCK_LEN] = {0};

    decryptor1->Decrypt(ciphertext1, decrypted1, BLOCK_LEN);
    decryptor2->Decrypt(ciphertext2, decrypted2, BLOCK_LEN);

    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted1[i], plaintext[i]);
        EXPECT_EQ(decrypted2[i], plaintext[i]);
    }

    decryptor1.reset();
    decryptor2.reset();
}

HWTEST_F(AesDecryptorUnitTest, OnSourceKeyChange_MinKeyLen_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[1] = {0x01};
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, 1, iv);
    EXPECT_EQ(decryptor_->keyLen_, 1u);
    EXPECT_EQ(decryptor_->key_[0], key[0]);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->iv_[i], iv[i]);
        EXPECT_EQ(decryptor_->initIv_[i], iv[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, OnSourceKeyChange_NullIv_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, nullptr);
    EXPECT_EQ(decryptor_->keyLen_, 0);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->key_[i], 0);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_WithoutInit_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    EXPECT_EQ(decryptor_->keyLen_, 0u);

    uint8_t ciphertext[BLOCK_LEN] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
    };
    uint8_t decrypted[BLOCK_LEN] = {0};
    uint8_t original[BLOCK_LEN] = {0};
    (void)memcpy_s(original, BLOCK_LEN, decrypted, BLOCK_LEN);

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted[i], original[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, KeyChange_DecryptVerify_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key1[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t key2[BLOCK_LEN] = {
        0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
        0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    decryptor_->OnSourceKeyChange(key1, BLOCK_LEN, iv);

    uint8_t plaintext1[BLOCK_LEN] = "TestDataKeyOne!";
    uint8_t ciphertext1[BLOCK_LEN] = {0};
    uint8_t decrypted1[BLOCK_LEN] = {0};

    AES_KEY encryptKey1;
    AES_set_encrypt_key(key1, AES_KEY_BITLEN, &encryptKey1);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext1, ciphertext1, BLOCK_LEN, &encryptKey1, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext1, decrypted1, BLOCK_LEN);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted1[i], plaintext1[i]);
    }

    decryptor_->OnSourceKeyChange(key2, BLOCK_LEN, iv);

    uint8_t plaintext2[BLOCK_LEN] = "TestDataKeyTwo!";
    uint8_t ciphertext2[BLOCK_LEN] = {0};
    uint8_t decrypted2[BLOCK_LEN] = {0};

    AES_KEY encryptKey2;
    AES_set_encrypt_key(key2, AES_KEY_BITLEN, &encryptKey2);
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext2, ciphertext2, BLOCK_LEN, &encryptKey2, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext2, decrypted2, BLOCK_LEN);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted2[i], plaintext2[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, OnSourceKeyChange_RepeatInit_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key1[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t key2[BLOCK_LEN] = {
        0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
        0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    decryptor_->OnSourceKeyChange(key1, BLOCK_LEN, iv);
    decryptor_->OnSourceKeyChange(key1, BLOCK_LEN, iv);
    decryptor_->OnSourceKeyChange(key2, BLOCK_LEN, iv);

    uint8_t plaintext[BLOCK_LEN] = "RepeatInitTest!";
    uint8_t ciphertext[BLOCK_LEN] = {0};
    uint8_t decrypted[BLOCK_LEN] = {0};

    AES_KEY encryptKey;
    AES_set_encrypt_key(key2, AES_KEY_BITLEN, &encryptKey);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, ciphertext, BLOCK_LEN, &encryptKey, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_BoundaryLen_002, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t plaintext[BLOCK_LEN] = "BoundaryLenTest";
    uint8_t ciphertext[BLOCK_LEN] = {0};
    uint8_t decrypted[BLOCK_LEN] = {0};

    AES_KEY encryptKey;
    AES_set_encrypt_key(key, AES_KEY_BITLEN, &encryptKey);
    uint8_t encIv[BLOCK_LEN];
    (void)memcpy_s(encIv, BLOCK_LEN, iv, BLOCK_LEN);
    AES_cbc_encrypt(plaintext, ciphertext, BLOCK_LEN, &encryptKey, encIv, AES_ENCRYPT);

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, Decrypt_IvUpdateAfterDecrypt_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t ciphertext1[BLOCK_LEN] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
    };
    uint8_t ciphertext2[BLOCK_LEN] = {
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
    };
    uint8_t decrypted[BLOCK_LEN] = {0};

    decryptor_->Decrypt(ciphertext1, decrypted, BLOCK_LEN);

    bool ivChanged = false;
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        if (decryptor_->iv_[i] != iv[i]) {
            ivChanged = true;
            break;
        }
    }
    EXPECT_TRUE(ivChanged);

    uint8_t ivAfterFirst[BLOCK_LEN] = {0};
    (void)memcpy_s(ivAfterFirst, BLOCK_LEN, decryptor_->iv_, BLOCK_LEN);

    decryptor_->Decrypt(ciphertext2, decrypted, BLOCK_LEN);
    bool isIvChangedAgain = false;
    for (size_t i = 0; i < BLOCK_LEN; i++) {
        if (decryptor_->iv_[i] != ivAfterFirst[i]) {
            isIvChangedAgain = true;
            break;
        }
    }
    EXPECT_TRUE(isIvChangedAgain);
}

HWTEST_F(AesDecryptorUnitTest, InitIvPreserved_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t ciphertext[BLOCK_LEN] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
    };
    uint8_t decrypted[BLOCK_LEN] = {0};

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);

    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->initIv_[i], iv[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, KeyPreserved_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t ciphertext[BLOCK_LEN] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
    };
    uint8_t decrypted[BLOCK_LEN] = {0};

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);
    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);

    for (size_t i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->key_[i], key[i]);
    }
}

HWTEST_F(AesDecryptorUnitTest, KeyLenPreserved_001, TestSize.Level1)
{
    ASSERT_NE(decryptor_, nullptr);
    uint8_t key[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t iv[BLOCK_LEN] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    decryptor_->OnSourceKeyChange(key, BLOCK_LEN, iv);

    uint8_t ciphertext[BLOCK_LEN] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
    };
    uint8_t decrypted[BLOCK_LEN] = {0};

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);
    EXPECT_EQ(decryptor_->keyLen_, BLOCK_LEN);

    decryptor_->Decrypt(ciphertext, decrypted, BLOCK_LEN);
    EXPECT_EQ(decryptor_->keyLen_, BLOCK_LEN);
}

/**
 * @tc.name  : Test SetIv API
 * @tc.number: SetIv_001
 * @tc.desc  : Test SetIv with null iv pointer does not crash
 */
HWTEST_F(AesDecryptorUnitTest, SetIv_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, decryptor_);
    uint8_t ivData[BLOCK_LEN] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    decryptor_->SetIv(ivData, BLOCK_LEN);
    decryptor_->SetIv(nullptr, BLOCK_LEN);
    for (int i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->iv_[i], ivData[i]);
    }
}

/**
 * @tc.name  : Test SetIv API
 * @tc.number: SetIv_002
 * @tc.desc  : Test SetIv with valid iv copies data correctly
 */
HWTEST_F(AesDecryptorUnitTest, SetIv_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, decryptor_);
    uint8_t ivData[BLOCK_LEN] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    decryptor_->SetIv(ivData, BLOCK_LEN);
    for (int i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->iv_[i], ivData[i]);
    }
}

/**
 * @tc.name  : Test SetIv with insufficient ivLen
 * @tc.number: SetIv_003
 * @tc.desc  : Test SetIv returns early when ivLen < BLOCK_LEN
 */
HWTEST_F(AesDecryptorUnitTest, SetIv_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, decryptor_);
    uint8_t ivData[BLOCK_LEN] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    decryptor_->SetIv(ivData, BLOCK_LEN);
    uint8_t shortIv[8] = {0};
    decryptor_->SetIv(shortIv, sizeof(shortIv));
    for (int i = 0; i < BLOCK_LEN; i++) {
        EXPECT_EQ(decryptor_->iv_[i], ivData[i]);
    }
}

}
}