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

#include <cinttypes>
#include "avdemuxer_demo.h"
#include "native_drm_common.h"

namespace OHOS {
namespace MediaAVCodec {
AVDemuxerDemo::AVDemuxerDemo()
{
    printf("AVDemuxerDemo constructor \n");
}

AVDemuxerDemo::~AVDemuxerDemo()
{
    printf("AVDemuxerDemo deconstructor \n");
}

int32_t AVDemuxerDemo::Destroy()
{
    int32_t ret = static_cast<int32_t>(OH_AVDemuxer_Destroy(avdemxuer_));
    if (ret != 0) {
        printf("OH_AVDemuxer_Destroy is failed\n");
    }
    return -1;
}

int32_t AVDemuxerDemo::CreateWithSource(OH_AVSource *avsource)
{
    this->avsource_ = avsource;
    if (!this->avsource_) {
        printf("this avsource is nullptr\n");
        return -1;
    }
    avdemxuer_ = OH_AVDemuxer_CreateWithSource(avsource);
    if (!avdemxuer_) {
        printf("OH_AVDemuxer_CreateWithSource is failed\n");
        return -1;
    }
    return 0;
}

int32_t AVDemuxerDemo::SelectTrackByID(uint32_t trackIndex)
{
    int32_t ret = static_cast<int32_t>(OH_AVDemuxer_SelectTrackByID(this->avdemxuer_, trackIndex));
    if (ret != 0) {
            printf("OH_AVDemuxer_SelectTrackByID is faild \n");
    }
    return ret;
}

int32_t AVDemuxerDemo::UnselectTrackByID(uint32_t trackIndex)
{
    int32_t ret = OH_AVDemuxer_UnselectTrackByID(this->avdemxuer_, trackIndex);
    if (ret != 0) {
        printf("OH_AVDemuxer_UnselectTrackByID is faild \n");
    }
    return ret;
}

int32_t AVDemuxerDemo::PrintInfo(int32_t tracks)
{
    for (int32_t i = 0; i < tracks; i++) {
        printf("streams[%d]==>total frames=%" PRId64 ",KeyFrames=%" PRId64 "\n", i,
            frames_[i] + key_frames_[i], key_frames_[i]);
    }
    return 0;
}

int32_t AVDemuxerDemo::ReadSample(uint32_t trackIndex, OH_AVMemory *sample, OH_AVCodecBufferAttr *bufferAttr)
{
    int32_t ret = OH_AVDemuxer_ReadSample(this->avdemxuer_, trackIndex, sample, bufferAttr);
    if (ret != 0) {
        return ret;
    }
    return ret;
}

bool AVDemuxerDemo::isEOS(std::map<uint32_t, bool>& countFlag)
{
    for (auto iter = countFlag.begin(); iter != countFlag.end(); ++iter) {
        if (!iter->second) {
            return false;
        }
    }
    return true;
}

int32_t AVDemuxerDemo::ReadAllSamples(OH_AVMemory *sample, int32_t tracks)
{
    int32_t ret = -1;
    std::map<uint32_t, bool> eosFlag;
    for (int i = 0; i < tracks; i++) {
        frames_[i] = 0;
        key_frames_[i] = 0;
        eosFlag[i] = false;
    }
    while (!isEOS(eosFlag)) {
        for (int32_t i = 0; i < tracks; i++) {
            ret = ReadSample(i, sample, &bufferInfo);
            if (ret == 0 && (bufferInfo.flags & AVCODEC_BUFFER_FLAGS_EOS)) {
                eosFlag[i] = true;
                continue;
            }
            if (ret == 0 && (bufferInfo.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME)) {
                key_frames_[i]++;
            } else if (ret == 0 && (bufferInfo.flags & AVCODEC_BUFFER_FLAGS_NONE) == 0) {
                frames_[i]++;
            } else {
                printf("the value or flags is error, ret = %d\n", ret);
                printf("the bufferInfo.flags=%d,bufferInfo.size=%d,bufferInfo.pts=%" PRId64 "\n",
                bufferInfo.flags, bufferInfo.size, bufferInfo.pts);
                return ret;
            }
        }
    }
    PrintInfo(tracks);
    return ret;
}

int32_t AVDemuxerDemo::SeekToTime(int64_t millisecond, OH_AVSeekMode mode)
{
    int32_t ret = OH_AVDemuxer_SeekToTime(this->avdemxuer_, millisecond, mode);
    if (ret != 0) {
        printf("OH_AVDemuxer_SeekToTime is faild \n");
    }
    return ret;
}

static void OnDrmInfoChangedInApp(DRM_MediaKeySystemInfo *drmInfo)
{
    printf("OnDrmInfoChangedInApp \n");
    if (drmInfo == nullptr || drmInfo->psshCount > MAX_PSSH_INFO_COUNT) {
        return;
    }
    printf("OnDrmInfoChangedInApp info count: %d \n", drmInfo->psshCount);
    for (uint32_t i = 0; i < drmInfo->psshCount; i++) {
        const uint32_t uuidLen = DRM_UUID_LEN;
        printf("OnDrmInfoChangedInApp print uuid: \n");
        for (uint32_t index = 0; index < uuidLen; index++) {
            printf("%x ", drmInfo->psshInfo[i].uuid[index]);
        }
        printf(" \n");
        printf("OnDrmInfoChangedInApp print pssh length %d \n", drmInfo->psshInfo[i].dataLen);
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

static void OnDrmInfoChangedWithObjInApp(OH_AVDemuxer *demuxer, DRM_MediaKeySystemInfo *drmInfo)
{
    printf("OnDrmInfoChangedWithObjInApp \n");
    printf("OnDrmInfoChangedWithObjInApp demuxer is %p\n", static_cast<void*>(demuxer));
    if (drmInfo == nullptr || drmInfo->psshCount > MAX_PSSH_INFO_COUNT) {
        return;
    }
    printf("OnDrmInfoChangedWithObjInApp info count: %d \n", drmInfo->psshCount);
    for (uint32_t i = 0; i < drmInfo->psshCount; i++) {
        const uint32_t uuidLen = DRM_UUID_LEN;
        printf("OnDrmInfoChangedWithObjInApp print uuid: \n");
        for (uint32_t index = 0; index < uuidLen; index++) {
            printf("%x ", drmInfo->psshInfo[i].uuid[index]);
        }
        printf(" \n");
        printf("OnDrmInfoChangedWithObjInApp print pssh length %d \n", drmInfo->psshInfo[i].dataLen);
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

int32_t AVDemuxerDemo::SetDrmAppCallback()
{
    printf("SetDrmAppCallback \n");
    DRM_MediaKeySystemInfoCallback callback = &OnDrmInfoChangedInApp;
    int32_t ret = OH_AVDemuxer_SetMediaKeySystemInfoCallback(this->avdemxuer_, callback);
    printf("SetDrmAppCallback ret %d \n", ret);
    Demuxer_MediaKeySystemInfoCallback callbackObj = &OnDrmInfoChangedWithObjInApp;
    ret = OH_AVDemuxer_SetDemuxerMediaKeySystemInfoCallback(this->avdemxuer_, callbackObj);
    printf("SetDrmAppCallbackWithObj ret %d \n", ret);
    return ret;
}

void AVDemuxerDemo::GetMediaKeySystemInfo()
{
    DRM_MediaKeySystemInfo mediaKeySystemInfo;
    OH_AVDemuxer_GetMediaKeySystemInfo(this->avdemxuer_, &mediaKeySystemInfo);
    printf("GetMediaKeySystemInfo count %d", mediaKeySystemInfo.psshCount);
    for (uint32_t i = 0; i < mediaKeySystemInfo.psshCount; i++) {
        printf("GetMediaKeySystemInfo print");
        const uint32_t uuidLen = 16;
        for (uint32_t index = 0; index < uuidLen; index++) {
            printf("GetMediaKeySystemInfo print uuid %x \n", mediaKeySystemInfo.psshInfo[i].uuid[index]);
        }
        printf("GetMediaKeySystemInfo print pssh length %d \n", mediaKeySystemInfo.psshInfo[i].dataLen);
        for (uint32_t k = 0; k < mediaKeySystemInfo.psshInfo[i].dataLen; k++) {
            unsigned char *pssh = static_cast<unsigned char*>(mediaKeySystemInfo.psshInfo[i].data);
            printf("GetMediaKeySystemInfo print pssh %x \n", pssh[k]);
        }
    }
}

}  // namespace MediaAVCodec
}  // namespace OHOS
