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

#include "native_averrors.h"
#include "avmemory_capi_mock.h"
#include "demuxer_capi_mock.h"
#ifdef SUPPORT_DRM
#include "native_drm_common.h"
#endif
namespace OHOS {
namespace MediaAVCodec {

#ifdef SUPPORT_DRM
static void OnMediaKeySystemInfoUpdated(DRM_MediaKeySystemInfo *drmInfo)
{
    printf("OnMediaKeySystemInfoUpdated \n");
    if (drmInfo == nullptr || drmInfo->psshCount > MAX_PSSH_INFO_COUNT) {
        return;
    }
    printf("OnMediaKeySystemInfoUpdated info count: %d \n", drmInfo->psshCount);
    for (uint32_t i = 0; i < drmInfo->psshCount; i++) {
        const uint32_t uuidLen = DRM_UUID_LEN;
        printf("OnMediaKeySystemInfoUpdated print uuid: \n");
        for (uint32_t index = 0; index < uuidLen; index++) {
            printf("%x ", drmInfo->psshInfo[i].uuid[index]);
        }
        printf(" \n");
        printf("OnMediaKeySystemInfoUpdated print pssh length %d \n", drmInfo->psshInfo[i].dataLen);
        if (drmInfo->psshInfo[i].dataLen > MAX_PSSH_DATA_LEN) {
            return;
        }
        unsigned char *pssh = static_cast<unsigned char*>(drmInfo->psshInfo[i].data);
        for (uint32_t k = 0; k < drmInfo->psshInfo[i].dataLen; k++) {
            printf("%x ", pssh[k]);
        }
        printf(" \n");
    }
}
#endif

#ifdef SUPPORT_DRM
static void OnMediaKeySystemInfoUpdatedWithObj(OH_AVDemuxer *demuxer, DRM_MediaKeySystemInfo *drmInfo)
{
    printf("OnMediaKeySystemInfoUpdatedWithObj \n");
    if (drmInfo == nullptr || drmInfo->psshCount > MAX_PSSH_INFO_COUNT) {
        return;
    }
    printf("OnMediaKeySystemInfoUpdatedWithObj info count: %d \n", drmInfo->psshCount);
    for (uint32_t i = 0; i < drmInfo->psshCount; i++) {
        const uint32_t uuidLen = DRM_UUID_LEN;
        printf("OnMediaKeySystemInfoUpdatedWithObj print uuid: \n");
        for (uint32_t index = 0; index < uuidLen; index++) {
            printf("%x ", drmInfo->psshInfo[i].uuid[index]);
        }
        printf(" \n");
        printf("OnMediaKeySystemInfoUpdatedWithObj print pssh length %d \n", drmInfo->psshInfo[i].dataLen);
        if (drmInfo->psshInfo[i].dataLen > MAX_PSSH_DATA_LEN) {
            return;
        }
        unsigned char *pssh = static_cast<unsigned char*>(drmInfo->psshInfo[i].data);
        for (uint32_t k = 0; k < drmInfo->psshInfo[i].dataLen; k++) {
            printf("%x ", pssh[k]);
        }
        printf(" \n");
    }
}
#endif


int32_t DemuxerCapiMock::Destroy()
{
    if (demuxer_ != nullptr) {
        int32_t ret = OH_AVDemuxer_Destroy(demuxer_);
        demuxer_ = nullptr;
        return ret;
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerCapiMock::SelectTrackByID(uint32_t trackIndex)
{
    if (demuxer_ != nullptr) {
        return OH_AVDemuxer_SelectTrackByID(demuxer_, trackIndex);
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerCapiMock::UnselectTrackByID(uint32_t trackIndex)
{
    if (demuxer_ != nullptr) {
        return OH_AVDemuxer_UnselectTrackByID(demuxer_, trackIndex);
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerCapiMock::ReadSample(uint32_t trackIndex, std::shared_ptr<AVMemoryMock> sample,
    AVCodecBufferInfo *bufferInfo, uint32_t &flag, bool checkBufferInfo)
{
    (void)checkBufferInfo;
    auto mem = std::static_pointer_cast<AVMemoryCapiMock>(sample);
    OH_AVMemory *avMemory = (mem != nullptr) ? mem->GetAVMemory() : nullptr;
    if (demuxer_ != nullptr) {
        OH_AVCodecBufferAttr bufferAttr;
        int32_t ret = OH_AVDemuxer_ReadSample(demuxer_, trackIndex, avMemory, &bufferAttr);
        bufferInfo->presentationTimeUs = bufferAttr.pts;
        bufferInfo->size = bufferAttr.size;
        bufferInfo->offset = bufferAttr.offset;
        flag = bufferAttr.flags;
        return ret;
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerCapiMock::SeekToTime(int64_t mSeconds, Media::SeekMode mode)
{
    if (demuxer_ != nullptr) {
        OH_AVSeekMode seekMode = static_cast<OH_AVSeekMode>(mode);
        return OH_AVDemuxer_SeekToTime(demuxer_, mSeconds, seekMode);
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerCapiMock::SetMediaKeySystemInfoCallback(bool isNull)
{
    if (demuxer_ != nullptr) {
        if (!isNull) {
#ifdef SUPPORT_DRM
            return OH_AVDemuxer_SetMediaKeySystemInfoCallback(demuxer_, &OnMediaKeySystemInfoUpdated);
        } else {
            return OH_AVDemuxer_SetMediaKeySystemInfoCallback(demuxer_, nullptr);
#endif
        }
    }
    return AV_ERR_OK;
}

int32_t DemuxerCapiMock::SetDemuxerMediaKeySystemInfoCallback(bool isNull)
{
    if (demuxer_ != nullptr) {
        if (!isNull) {
#ifdef SUPPORT_DRM
            return OH_AVDemuxer_SetDemuxerMediaKeySystemInfoCallback(demuxer_, &OnMediaKeySystemInfoUpdatedWithObj);
        } else {
            return OH_AVDemuxer_SetDemuxerMediaKeySystemInfoCallback(demuxer_, nullptr);
#endif
        }
    }
    return AV_ERR_OK;
}

int32_t DemuxerCapiMock::GetMediaKeySystemInfo()
{
    if (demuxer_ != nullptr) {
#ifdef SUPPORT_DRM
        DRM_MediaKeySystemInfo mediaKeySystemInfo;
        return OH_AVDemuxer_GetMediaKeySystemInfo(demuxer_, &mediaKeySystemInfo);
#endif
    }
    return AV_ERR_OK;
}

int32_t DemuxerCapiMock::GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
    const uint64_t relativePresentationTimeUs, uint32_t &index)
{
    return AV_ERR_OK;
}

int32_t DemuxerCapiMock::GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
    const uint32_t index, uint64_t &relativePresentationTimeUs)
{
    return AV_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS