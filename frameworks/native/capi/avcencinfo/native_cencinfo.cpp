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
#include "meta/meta.h"
#include "common/native_mfmagic.h"
#include "securec.h"
#include "native_cencinfo.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "NativeAVCencInfo"};
}

using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using MetaDrmCencInfo = Plugins::MetaDrmCencInfo;
using MetaDrmCencAlgorithm = Plugins::MetaDrmCencAlgorithm;
using MetaDrmCencInfoMode = Plugins::MetaDrmCencInfoMode;

struct OH_AVCencInfo {
    explicit OH_AVCencInfo()
    {
        cencInfo_.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED;
        for (int32_t i = 0; i < META_DRM_KEY_ID_SIZE; i++) {
            cencInfo_.keyId[i] = 0;
        }
        cencInfo_.keyIdLen = 0;
        for (int32_t i = 0; i < META_DRM_IV_SIZE; i++) {
            cencInfo_.iv[i] = 0;
        }
        cencInfo_.ivLen = 0;
        cencInfo_.encryptBlocks = 0;
        cencInfo_.skipBlocks = 0;
        cencInfo_.firstEncryptOffset = 0;
        cencInfo_.subSamples[0].clearHeaderLen = 0;
        cencInfo_.subSamples[0].payLoadLen = 0;
        cencInfo_.subSampleNum = 1;
        cencInfo_.mode = MetaDrmCencInfoMode::META_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET;
    }
    ~OH_AVCencInfo() = default;

    MetaDrmCencInfo cencInfo_;
};

#ifdef __cplusplus
extern "C" {
#endif

OH_AVCencInfo *OH_AVCencInfo_Create()
{
    struct OH_AVCencInfo *cencInfoObject = new (std::nothrow) OH_AVCencInfo();
    CHECK_AND_RETURN_RET_LOG(cencInfoObject != nullptr, nullptr, "failed to new OH_AVCencInfo");

    int ret = memset_s(&(cencInfoObject->cencInfo_), sizeof(MetaDrmCencInfo), 0, sizeof(MetaDrmCencInfo));
    if (ret != EOK) {
        AVCODEC_LOGE("failed to memset OH_AVCencInfo");
        delete cencInfoObject;
        return nullptr;
    }
    return cencInfoObject;
}

OH_AVErrCode OH_AVCencInfo_Destroy(OH_AVCencInfo *cencInfo)
{
    CHECK_AND_RETURN_RET_LOG(cencInfo != nullptr, AV_ERR_INVALID_VAL, "input cencInfo is nullptr!");
    delete cencInfo;
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCencInfo_SetAlgorithm(OH_AVCencInfo *cencInfo, enum DrmCencAlgorithm algo)
{
    CHECK_AND_RETURN_RET_LOG(cencInfo != nullptr, AV_ERR_INVALID_VAL, "input cencInfo is nullptr!");
    struct OH_AVCencInfo *cencInfoObject = cencInfo;

    switch (algo) {
        case DRM_ALG_CENC_UNENCRYPTED:
            cencInfoObject->cencInfo_.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED;
            break;
        case DRM_ALG_CENC_AES_CTR:
            cencInfoObject->cencInfo_.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_AES_CTR;
            break;
        case DRM_ALG_CENC_AES_WV:
            cencInfoObject->cencInfo_.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_AES_WV;
            break;
        case DRM_ALG_CENC_AES_CBC:
            cencInfoObject->cencInfo_.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_AES_CBC;
            break;
        case DRM_ALG_CENC_SM4_CBC:
            cencInfoObject->cencInfo_.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_SM4_CBC;
            break;
        case DRM_ALG_CENC_SM4_CTR:
            cencInfoObject->cencInfo_.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_SM4_CTR;
            break;
        default:
            cencInfoObject->cencInfo_.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED;
            break;
    }
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCencInfo_SetKeyIdAndIv(OH_AVCencInfo *cencInfo, uint8_t *keyId,
    uint32_t keyIdLen, uint8_t *iv, uint32_t ivLen)
{
    CHECK_AND_RETURN_RET_LOG(cencInfo != nullptr, AV_ERR_INVALID_VAL, "input cencInfo is nullptr!");
    CHECK_AND_RETURN_RET_LOG(keyId != nullptr, AV_ERR_INVALID_VAL, "input keyId is nullptr!");
    CHECK_AND_RETURN_RET_LOG(keyIdLen == DRM_KEY_ID_SIZE, AV_ERR_INVALID_VAL, "input keyIdLen is err!");
    CHECK_AND_RETURN_RET_LOG(iv != nullptr, AV_ERR_INVALID_VAL, "input iv is nullptr!");
    CHECK_AND_RETURN_RET_LOG(ivLen == DRM_KEY_IV_SIZE, AV_ERR_INVALID_VAL, "input ivLen is err!");
    struct OH_AVCencInfo *cencInfoObject = cencInfo;

    cencInfoObject->cencInfo_.keyIdLen = keyIdLen;
    int ret = memcpy_s(cencInfoObject->cencInfo_.keyId, sizeof(cencInfoObject->cencInfo_.keyId), keyId, keyIdLen);
    if (ret != EOK) {
        AVCODEC_LOGE("failed to memcpy keyId");
        return AV_ERR_INVALID_VAL;
    }

    cencInfoObject->cencInfo_.ivLen = ivLen;
    ret = memcpy_s(cencInfoObject->cencInfo_.iv, sizeof(cencInfoObject->cencInfo_.iv), iv, ivLen);
    if (ret != EOK) {
        AVCODEC_LOGE("failed to memcpy iv");
        return AV_ERR_INVALID_VAL;
    }
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCencInfo_SetSubsampleInfo(OH_AVCencInfo *cencInfo, uint32_t encryptedBlockCount,
    uint32_t skippedBlockCount, uint32_t firstEncryptedOffset, uint32_t subsampleCount, DrmSubsample *subsamples)
{
    CHECK_AND_RETURN_RET_LOG(cencInfo != nullptr, AV_ERR_INVALID_VAL, "input cencInfo is nullptr!");
    CHECK_AND_RETURN_RET_LOG(subsampleCount <= DRM_KEY_MAX_SUB_SAMPLE_NUM, AV_ERR_INVALID_VAL,
        "input subsampleCount is err!");
    CHECK_AND_RETURN_RET_LOG(subsamples != nullptr, AV_ERR_INVALID_VAL, "input subsamples is nullptr!");
    struct OH_AVCencInfo *cencInfoObject = cencInfo;

    cencInfoObject->cencInfo_.encryptBlocks = encryptedBlockCount;
    cencInfoObject->cencInfo_.skipBlocks = skippedBlockCount;
    cencInfoObject->cencInfo_.firstEncryptOffset = firstEncryptedOffset;
    cencInfoObject->cencInfo_.subSampleNum = subsampleCount;
    for (uint32_t i = 0; i < subsampleCount; i++) {
        cencInfoObject->cencInfo_.subSamples[i].clearHeaderLen = subsamples[i].clearHeaderLen;
        cencInfoObject->cencInfo_.subSamples[i].payLoadLen = subsamples[i].payLoadLen;
    }
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCencInfo_SetMode(OH_AVCencInfo *cencInfo, enum DrmCencInfoMode mode)
{
    CHECK_AND_RETURN_RET_LOG(cencInfo != nullptr, AV_ERR_INVALID_VAL, "input cencInfo is nullptr!");
    struct OH_AVCencInfo *cencInfoObject = cencInfo;

    switch (mode) {
        case DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET:
            cencInfoObject->cencInfo_.mode = MetaDrmCencInfoMode::META_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET;
            break;
        case DRM_CENC_INFO_KEY_IV_SUBSAMPLES_NOT_SET:
            cencInfoObject->cencInfo_.mode = MetaDrmCencInfoMode::META_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_NOT_SET;
            break;
        default:
            cencInfoObject->cencInfo_.mode = MetaDrmCencInfoMode::META_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET;
            break;
    }
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCencInfo_SetAVBuffer(OH_AVCencInfo *cencInfo, OH_AVBuffer *buffer)
{
    CHECK_AND_RETURN_RET_LOG(cencInfo != nullptr, AV_ERR_INVALID_VAL, "input cencInfo is nullptr!");
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->buffer_ != nullptr && buffer->buffer_->meta_ != nullptr,
        AV_ERR_INVALID_VAL, "input buffer is nullptr!");
    struct OH_AVCencInfo *cencInfoObject = cencInfo;

    std::vector<uint8_t> cencInfoVec(reinterpret_cast<uint8_t *>(&(cencInfoObject->cencInfo_)),
        (reinterpret_cast<uint8_t *>(&(cencInfoObject->cencInfo_))) + sizeof(MetaDrmCencInfo));
    buffer->buffer_->meta_->SetData(Tag::DRM_CENC_INFO, std::move(cencInfoVec));
    return AV_ERR_OK;
}

#ifdef __cplusplus
};
#endif