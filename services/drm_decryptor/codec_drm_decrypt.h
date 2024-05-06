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

#ifndef CODEC_DRM_DECRYPT_H
#define CODEC_DRM_DECRYPT_H

#include <mutex>
#include "buffer/avbuffer.h"
#include "meta/meta.h"
#include "foundation/multimedia/drm_framework/services/drm_service/ipc/i_keysession_service.h"
#include "foundation/multimedia/drm_framework/services/drm_service/ipc/i_mediadecryptmodule_service.h"

namespace OHOS {
namespace MediaAVCodec {

using namespace Media;
using MetaDrmSubSample = Plugins::MetaDrmSubSample;
using MetaDrmCencInfo = Plugins::MetaDrmCencInfo;
using MetaDrmCencAlgorithm = Plugins::MetaDrmCencAlgorithm;
using MetaDrmCencInfoMode = Plugins::MetaDrmCencInfoMode;
using DrmBuffer = DrmStandard::IMediaDecryptModuleService::DrmBuffer;

enum SvpMode : int32_t {
    SVP_CLEAR = -1, /* it's not a protection video */
    SVP_FALSE, /* it's a protection video but not need secure decoder */
    SVP_TRUE, /* it's a protection video and need secure decoder */
};

class CodecDrmDecrypt {
public:
    int32_t DrmVideoCencDecrypt(std::shared_ptr<AVBuffer> &inBuf, std::shared_ptr<AVBuffer> &outBuf,
        uint32_t &dataSize);
    int32_t DrmAudioCencDecrypt(std::shared_ptr<AVBuffer> &inBuf, std::shared_ptr<AVBuffer> &outBuf,
        uint32_t &dataSize);
    void SetCodecName(const std::string &codecName);
    void SetDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag);

private:
    void GetCodingType();
    void DrmGetSkipClearBytes(uint32_t &skipBytes) const;
    int32_t DrmGetNalTypeAndIndex(const uint8_t *data, uint32_t dataSize, uint8_t &nalType, uint32_t &posIndex) const;
    static void DrmGetSyncHeaderIndex(const uint8_t *data, uint32_t dataSize, uint32_t &posIndex);
    uint8_t DrmGetFinalNalTypeAndIndex(const uint8_t *data, uint32_t dataSize, uint32_t &posStartIndex,
        uint32_t &posEndIndex) const;
    static void DrmRemoveAmbiguityBytes(uint8_t *data, uint32_t &posEndIndex, uint32_t offset, uint32_t &dataSize);
    void DrmModifyCencInfo(std::shared_ptr<AVBuffer> inBuf, uint32_t &dataSize, uint8_t isAmbiguity,
        MetaDrmCencInfo *cencInfo) const;
    static void SetDrmAlgoAndBlocks(uint8_t algo, MetaDrmCencInfo *cencInfo);
    static int DrmFindAvsCeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos, uint32_t index);
    static int DrmFindHevcCeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos, uint32_t index);
    static int DrmFindH264CeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos, uint32_t index);
    int DrmFindCeiNalUnit(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos, uint32_t index) const;
    int DrmFindCeiPos(const uint8_t *data, uint32_t dataSize, uint32_t &ceiStartPos, uint32_t &ceiEndPos) const;
    static void DrmFindEncryptionFlagPos(const uint8_t *data, uint32_t dataSize, uint32_t &pos);
    static int DrmGetKeyId(uint8_t *data, uint32_t &dataSize, uint32_t &pos, MetaDrmCencInfo *cencInfo);
    static int DrmGetKeyIv(const uint8_t *data, uint32_t dataSize, uint32_t &pos, MetaDrmCencInfo *cencInfo);
    static int DrmParseDrmDescriptor(const uint8_t *data, uint32_t dataSize, uint32_t &pos, uint8_t drmDescriptorFlag,
        MetaDrmCencInfo *cencInfo);
    static void DrmSetKeyInfo(const uint8_t *data, uint32_t dataSize, uint32_t ceiStartPos, uint8_t &isAmbiguity,
        MetaDrmCencInfo *cencInfo);
    void DrmGetCencInfo(std::shared_ptr<AVBuffer> inBuf, uint32_t dataSize, uint8_t &isAmbiguity,
        MetaDrmCencInfo *cencInfo) const;
    int32_t DecryptMediaData(const MetaDrmCencInfo * const cencInfo, std::shared_ptr<AVBuffer> &inBuf,
        std::shared_ptr<AVBuffer> &outBuf);
    static int32_t SetDrmBuffer(const std::shared_ptr<AVBuffer> &inBuf, const std::shared_ptr<AVBuffer> &outBuf,
        DrmBuffer &inDrmBuffer, DrmBuffer &outDrmBuffer);

private:
    std::mutex configMutex_;
    std::string codecName_;
    int32_t codingType_ = 0;
    sptr<DrmStandard::IMediaKeySessionService> keySessionServiceProxy_;
    sptr<DrmStandard::IMediaDecryptModuleService> decryptModuleProxy_;
    int32_t svpFlag_ = SVP_CLEAR;
    MetaDrmCencInfoMode mode_ = MetaDrmCencInfoMode::META_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET;
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_DRM_DECRYPT_H
