/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cstdint>
#include "meta/meta.h"
#include "common/native_mfmagic.h"
#include "native_cencinfo.h"

#define FUZZ_PROJECT_NAME "avcencinfo_fuzzer"

using namespace std;
using namespace OHOS::Media;

#define AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cond, ret)                   \
    do {                                                                    \
        if (!(cond)) {                                                      \
            return ret;                                                     \
        }                                                                   \
    } while (0)

namespace OHOS {
namespace AvCencInfoFuzzer {

bool CencInfoCreateFuzzTest(const uint8_t *data, size_t size)
{
    (void)data;
    (void)size;
    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);
    return true;
}

bool CencInfoDestroyFuzzTest(const uint8_t *data, size_t size)
{
    (void)data;
    (void)size;
    OH_AVErrCode errNo = AV_ERR_OK;
    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);
    errNo = OH_AVCencInfo_Destroy(cencInfo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    return true;
}

bool CencInfoSetAlgorithm(void)
{
    OH_AVErrCode errNo = AV_ERR_OK;
    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);

    errNo = OH_AVCencInfo_SetAlgorithm(cencInfo, DRM_ALG_CENC_UNENCRYPTED);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetAlgorithm(cencInfo, DRM_ALG_CENC_AES_CTR);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetAlgorithm(cencInfo, DRM_ALG_CENC_AES_WV);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetAlgorithm(cencInfo, DRM_ALG_CENC_AES_CBC);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetAlgorithm(cencInfo, DRM_ALG_CENC_SM4_CBC);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetAlgorithm(cencInfo, DRM_ALG_CENC_SM4_CTR);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetAlgorithm(cencInfo, static_cast<enum DrmCencAlgorithm>(DRM_ALG_CENC_SM4_CTR + 1));
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_Destroy(cencInfo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    return true;
}

bool CencInfoSetAlgorithmFuzzTest(const uint8_t *data, size_t size)
{
    (void)size;
    OH_AVErrCode errNo = AV_ERR_OK;
    DrmCencAlgorithm algo = static_cast<enum DrmCencAlgorithm>(*data);
    static uint8_t cencInfoSetAlgorithmFuzzTestFlag = 0;
    if (cencInfoSetAlgorithmFuzzTestFlag == 0) {
        CencInfoSetAlgorithm();
        cencInfoSetAlgorithmFuzzTestFlag = 1;
    }
    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);

    errNo = OH_AVCencInfo_SetAlgorithm(cencInfo, algo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_Destroy(cencInfo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    return true;
}

bool CencInfoSetKeyIdAndIv(void)
{
    OH_AVErrCode errNo = AV_ERR_OK;
    uint32_t keyIdLen = DRM_KEY_ID_SIZE;
    uint8_t keyId[] = {
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    uint32_t ivLen = DRM_KEY_IV_SIZE;
    uint8_t iv[] = {
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};

    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);
    errNo = OH_AVCencInfo_SetKeyIdAndIv(cencInfo, keyId, keyIdLen, iv, ivLen);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    errNo = OH_AVCencInfo_Destroy(cencInfo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    return true;
}

bool CencInfoSetKeyIdAndIvFuzzTest(const uint8_t *data, size_t size)
{
    OH_AVErrCode errNo = AV_ERR_OK;
    static uint8_t cencInfoSetKeyIdAndIvFuzzTestFlag = 0;
    if (cencInfoSetKeyIdAndIvFuzzTestFlag == 0) {
        CencInfoSetKeyIdAndIv();
        cencInfoSetKeyIdAndIvFuzzTestFlag = 1;
    }
    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);

    errNo = OH_AVCencInfo_SetKeyIdAndIv(cencInfo, const_cast<uint8_t *>(data), static_cast<uint32_t>(size),
        const_cast<uint8_t *>(data), static_cast<uint32_t>(size));
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_Destroy(cencInfo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    return true;
}

bool CencInfoSetSubsampleInfo(void)
{
    OH_AVErrCode errNo = AV_ERR_OK;
    uint32_t encryptedBlockCount = 0;
    uint32_t skippedBlockCount = 0;
    uint32_t firstEncryptedOffset = 0;
    uint32_t subsampleCount = 1;
    DrmSubsample subsamples[1] = { {0x10, 0x16} };

    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);
    errNo = OH_AVCencInfo_SetSubsampleInfo(cencInfo, encryptedBlockCount, skippedBlockCount, firstEncryptedOffset,
        subsampleCount, subsamples);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    errNo = OH_AVCencInfo_Destroy(cencInfo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    return true;
}

bool CencInfoSetSubsampleInfoFuzzTest(const uint8_t *data, size_t size)
{
    (void)size;
    OH_AVErrCode errNo = AV_ERR_OK;
    uint32_t encryptedBlockCount = static_cast<uint32_t>(*data);
    uint32_t skippedBlockCount = static_cast<uint32_t>(*data);
    uint32_t firstEncryptedOffset = static_cast<uint32_t>(*data);
    uint32_t subsampleCount = static_cast<uint32_t>(*data);
    DrmSubsample subsamples[DRM_KEY_MAX_SUB_SAMPLE_NUM];
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(subsampleCount <= DRM_KEY_MAX_SUB_SAMPLE_NUM, false);
    for (uint32_t i = 0; i < subsampleCount; i++) {
        subsamples[i].clearHeaderLen = static_cast<uint32_t>(*data);
        subsamples[i].payLoadLen = static_cast<uint32_t>(*data);
    }
    static uint8_t cencInfoSetSubsampleInfoFuzzTestFlag = 0;
    if (cencInfoSetSubsampleInfoFuzzTestFlag == 0) {
        CencInfoSetSubsampleInfo();
        cencInfoSetSubsampleInfoFuzzTestFlag = 1;
    }
    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);

    errNo = OH_AVCencInfo_SetSubsampleInfo(cencInfo, encryptedBlockCount, skippedBlockCount, firstEncryptedOffset,
        subsampleCount, subsamples);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_Destroy(cencInfo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    return true;
}

bool CencInfoSetMode(void)
{
    OH_AVErrCode errNo = AV_ERR_OK;

    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);

    errNo = OH_AVCencInfo_SetMode(cencInfo, DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetMode(cencInfo, DRM_CENC_INFO_KEY_IV_SUBSAMPLES_NOT_SET);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetMode(cencInfo,
        static_cast<enum DrmCencInfoMode>(DRM_CENC_INFO_KEY_IV_SUBSAMPLES_NOT_SET + 1));
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_Destroy(cencInfo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    return true;
}

bool CencInfoSetModeFuzzTest(const uint8_t *data, size_t size)
{
    (void)size;
    OH_AVErrCode errNo = AV_ERR_OK;
    DrmCencInfoMode mode = static_cast<enum DrmCencInfoMode>(*data);
    static uint8_t cencInfoSetModeFuzzTestFlag = 0;
    if (cencInfoSetModeFuzzTestFlag == 0) {
        CencInfoSetMode();
        cencInfoSetModeFuzzTestFlag = 1;
    }
    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);

    errNo = OH_AVCencInfo_SetMode(cencInfo, mode);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_Destroy(cencInfo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    return true;
}

bool CencInfoSetAVBuffer(void)
{
    uint32_t keyIdLen = DRM_KEY_ID_SIZE;
    uint8_t keyId[] = {
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    uint32_t ivLen = DRM_KEY_IV_SIZE;
    uint8_t iv[] = {
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    uint32_t encryptedBlockCount = 0;
    uint32_t skippedBlockCount = 0;
    uint32_t firstEncryptedOffset = 0;
    uint32_t subsampleCount = 1;
    DrmSubsample subsamples[1] = { {0x10, 0x16} };
    OH_AVErrCode errNo = AV_ERR_OK;
    MemoryFlag memFlag = MEMORY_READ_WRITE;

    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(memFlag);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(avAllocator != nullptr, false);

    std::shared_ptr<AVBuffer> inBuf = AVBuffer::CreateAVBuffer(avAllocator, static_cast<int32_t>(DRM_KEY_ID_SIZE));
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(inBuf != nullptr, false);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(inBuf->memory_ != nullptr, false);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(inBuf->memory_->GetCapacity() == DRM_KEY_ID_SIZE, false);

    struct OH_AVBuffer *buffer = new (std::nothrow) OH_AVBuffer(inBuf);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(buffer != nullptr, false);

    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);

    errNo = OH_AVCencInfo_SetAlgorithm(cencInfo, DRM_ALG_CENC_AES_CTR);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetKeyIdAndIv(cencInfo, keyId, keyIdLen, iv, ivLen);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetSubsampleInfo(cencInfo, encryptedBlockCount, skippedBlockCount, firstEncryptedOffset,
        subsampleCount, subsamples);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetMode(cencInfo, DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetAVBuffer(cencInfo, buffer);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_Destroy(cencInfo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    delete buffer;
    return true;
}

bool CencInfoSetAVBufferFuzzTest(const uint8_t *data, size_t size)
{
    OH_AVErrCode errNo = AV_ERR_OK;
    MemoryFlag memFlag = MEMORY_READ_WRITE;
    DrmCencAlgorithm algo = static_cast<enum DrmCencAlgorithm>(*data);
    DrmCencInfoMode mode = static_cast<enum DrmCencInfoMode>(*data);
    uint32_t encryptedBlockCount = static_cast<uint32_t>(*data);
    uint32_t skippedBlockCount = static_cast<uint32_t>(*data);
    uint32_t firstEncryptedOffset = static_cast<uint32_t>(*data);
    uint32_t subsampleCount = static_cast<uint32_t>(*data);
    DrmSubsample subsamples[DRM_KEY_MAX_SUB_SAMPLE_NUM];
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(subsampleCount <= DRM_KEY_MAX_SUB_SAMPLE_NUM, false);
    for (uint32_t i = 0; i < subsampleCount; i++) {
        subsamples[i].clearHeaderLen = static_cast<uint32_t>(*data);
        subsamples[i].payLoadLen = static_cast<uint32_t>(*data);
    }
    static uint8_t cencInfoSetAVBufferFuzzTestFlag = 0;
    if (cencInfoSetAVBufferFuzzTestFlag == 0) {
        CencInfoSetAVBuffer();
        cencInfoSetAVBufferFuzzTestFlag = 1;
    }

    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(memFlag);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(avAllocator != nullptr, false);

    std::shared_ptr<AVBuffer> inBuf = AVBuffer::CreateAVBuffer(avAllocator, static_cast<int32_t>(DRM_KEY_ID_SIZE));
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(inBuf != nullptr, false);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(inBuf->memory_ != nullptr, false);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(inBuf->memory_->GetCapacity() == DRM_KEY_ID_SIZE, false);

    struct OH_AVBuffer *buffer = new (std::nothrow) OH_AVBuffer(inBuf);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(buffer != nullptr, false);

    OH_AVCencInfo *cencInfo = OH_AVCencInfo_Create();
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(cencInfo != nullptr, false);

    errNo = OH_AVCencInfo_SetAlgorithm(cencInfo, algo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetKeyIdAndIv(cencInfo, const_cast<uint8_t *>(data), static_cast<uint32_t>(size),
        const_cast<uint8_t *>(data), static_cast<uint32_t>(size));
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetSubsampleInfo(cencInfo, encryptedBlockCount, skippedBlockCount, firstEncryptedOffset,
        subsampleCount, subsamples);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetMode(cencInfo, mode);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_SetAVBuffer(cencInfo, buffer);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);

    errNo = OH_AVCencInfo_Destroy(cencInfo);
    AV_CENC_INFO_FUZZ_CHECK_AND_RETURN_RET(errNo == AV_ERR_OK, false);
    delete buffer;
    return true;
}

} // namespace AvCencInfoFuzzer
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }
    if (size < sizeof(int64_t)) {
        return 0;
    }
    OHOS::AvCencInfoFuzzer::CencInfoCreateFuzzTest(data, size);
    OHOS::AvCencInfoFuzzer::CencInfoDestroyFuzzTest(data, size);
    OHOS::AvCencInfoFuzzer::CencInfoSetAlgorithmFuzzTest(data, size);
    OHOS::AvCencInfoFuzzer::CencInfoSetKeyIdAndIvFuzzTest(data, size);
    OHOS::AvCencInfoFuzzer::CencInfoSetSubsampleInfoFuzzTest(data, size);
    OHOS::AvCencInfoFuzzer::CencInfoSetModeFuzzTest(data, size);
    OHOS::AvCencInfoFuzzer::CencInfoSetAVBufferFuzzTest(data, size);
    return 0;
}
