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

#include "native_avdemuxer.h"
#include <memory>
#include "av_common.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avdemuxer.h"
#include "common/native_mfmagic.h"
#include "native_avmagic.h"
#include "native_mfmagic.h"
#include "native_object.h"
#include "native_drm_common.h"
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER_MUXER, "NativeAVDemuxer"};
}

using namespace OHOS::MediaAVCodec;

namespace NativeDrmTools {
static int32_t ProcessApplicationDrmInfo(DRM_MediaKeySystemInfo *info,
    const std::multimap<std::string, std::vector<uint8_t>> &drmInfoMap)
{
    uint32_t count = drmInfoMap.size();
    if (count <= 0 || count > MAX_PSSH_INFO_COUNT) {
        return AV_ERR_INVALID_VAL;
    }
    CHECK_AND_RETURN_RET_LOG(info != nullptr, AV_ERR_INVALID_VAL, "info is nullptr");

    info->psshCount = count;
    uint32_t index = 0;
    for (auto &item : drmInfoMap) {
        const uint32_t step = 2;
        errno_t ret = memset_s(info->psshInfo[index].uuid, DRM_UUID_LEN, 0x00, DRM_UUID_LEN);
        CHECK_AND_RETURN_RET_LOG(ret == EOK, AV_ERR_INVALID_VAL, "memset_s uuid failed");
        for (uint32_t i = 0; i < item.first.size(); i += step) {
            std::string byteString = item.first.substr(i, step);
            unsigned char uuidByte = (unsigned char)strtol(byteString.c_str(), NULL, DRM_UUID_LEN);
            info->psshInfo[index].uuid[i / step] = uuidByte;
        }

        info->psshInfo[index].dataLen = static_cast<int32_t>(item.second.size());

        ret = memset_s(info->psshInfo[index].data, MAX_PSSH_DATA_LEN, 0x00, MAX_PSSH_DATA_LEN);
        CHECK_AND_RETURN_RET_LOG(ret == EOK, AV_ERR_INVALID_VAL, "memset_s pssh failed");
        CHECK_AND_RETURN_RET_LOG(item.second.size() <= MAX_PSSH_DATA_LEN, AV_ERR_INVALID_VAL, "pssh is too large");
        ret = memcpy_s(info->psshInfo[index].data, item.second.size(), static_cast<const void *>(item.second.data()),
            item.second.size());
        CHECK_AND_RETURN_RET_LOG(ret == EOK, AV_ERR_INVALID_VAL, "pssh is too large");

        index++;
    }
    return AV_ERR_OK;
}
} // namespace NativeDrmTools

class NativeDemuxerCallback;
struct DemuxerObject : public OH_AVDemuxer {
    explicit DemuxerObject(const std::shared_ptr<AVDemuxer> &demuxer)
        : OH_AVDemuxer(AVMagic::AVCODEC_MAGIC_AVDEMUXER), demuxer_(demuxer) {}
    ~DemuxerObject() = default;

    const std::shared_ptr<AVDemuxer> demuxer_;
    std::shared_ptr<NativeDemuxerCallback> callback_;
};

class NativeDemuxerCallback : public AVDemuxerCallback {
public:
    explicit NativeDemuxerCallback(OH_AVDemuxer *demuxer,
        DRM_MediaKeySystemInfoCallback cb) : demuxer_(demuxer), callback_(cb), callbackObj_(nullptr)
    {
    }

    explicit NativeDemuxerCallback(OH_AVDemuxer *demuxer,
        Demuxer_MediaKeySystemInfoCallback cbObj) : demuxer_(demuxer), callback_(nullptr), callbackObj_(cbObj)
    {
    }

    virtual ~NativeDemuxerCallback() = default;

    void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>> &drmInfo) override
    {
        AVCODEC_LOGI("NativeDemuxerCallback OnDrmInfoChanged is on call");
        std::unique_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN_LOG(demuxer_ != nullptr, "demuxer_ is nullptr");

        DRM_MediaKeySystemInfo info;
        int32_t ret = NativeDrmTools::ProcessApplicationDrmInfo(&info, drmInfo);
        CHECK_AND_RETURN_LOG(ret == AV_ERR_OK, "ProcessApplicationDrmInfo failed");

        CHECK_AND_RETURN_LOG(callback_ != nullptr || callbackObj_ != nullptr, "hasn't register any drm callback!");
        if (callback_ != nullptr) {
            callback_(&info);
        }
        if (callbackObj_ != nullptr) {
            callbackObj_(demuxer_, &info);
        }
    }

private:
    std::shared_mutex mutex_;
    struct OH_AVDemuxer *demuxer_;
    DRM_MediaKeySystemInfoCallback callback_;
    Demuxer_MediaKeySystemInfoCallback callbackObj_;
};

struct OH_AVDemuxer *OH_AVDemuxer_CreateWithSource(OH_AVSource *source)
{
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "Create demuxer failed because input source is nullptr!");

    struct AVSourceObject *sourceObj = reinterpret_cast<AVSourceObject *>(source);
    CHECK_AND_RETURN_RET_LOG(sourceObj != nullptr, nullptr,
        "Create demuxer failed because new sourceObj is nullptr!");

    std::shared_ptr<AVDemuxer> demuxer = AVDemuxerFactory::CreateWithSource(sourceObj->source_);
    CHECK_AND_RETURN_RET_LOG(demuxer != nullptr, nullptr, "New demuxer with source by AVDemuxerFactory failed!");

    struct DemuxerObject *object = new(std::nothrow) DemuxerObject(demuxer);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "New demuxerObject failed when create demuxer!");

    return object;
}

OH_AVErrCode OH_AVDemuxer_Destroy(OH_AVDemuxer *demuxer)
{
    CHECK_AND_RETURN_RET_LOG(demuxer != nullptr, AV_ERR_INVALID_VAL,
        "Destroy demuxer failed because input demuxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(demuxer->magic_ == AVMagic::AVCODEC_MAGIC_AVDEMUXER, AV_ERR_INVALID_VAL, "magic error!");

    delete demuxer;
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVDemuxer_SelectTrackByID(OH_AVDemuxer *demuxer, uint32_t trackIndex)
{
    CHECK_AND_RETURN_RET_LOG(demuxer != nullptr, AV_ERR_INVALID_VAL,
        "Select track failed because input demuxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(demuxer->magic_ == AVMagic::AVCODEC_MAGIC_AVDEMUXER, AV_ERR_INVALID_VAL, "magic error!");

    struct DemuxerObject *demuxerObj = reinterpret_cast<DemuxerObject *>(demuxer);
    CHECK_AND_RETURN_RET_LOG(demuxerObj->demuxer_ != nullptr, AV_ERR_INVALID_VAL,
        "New DemuxerObject failed when select track!");

    int32_t ret = demuxerObj->demuxer_->SelectTrackByID(trackIndex);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "demuxer_ SelectTrackByID failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AVDemuxer_UnselectTrackByID(OH_AVDemuxer *demuxer, uint32_t trackIndex)
{
    CHECK_AND_RETURN_RET_LOG(demuxer != nullptr, AV_ERR_INVALID_VAL,
        "Unselect track failed because input demuxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(demuxer->magic_ == AVMagic::AVCODEC_MAGIC_AVDEMUXER, AV_ERR_INVALID_VAL, "magic error!");
    
    struct DemuxerObject *demuxerObj = reinterpret_cast<DemuxerObject *>(demuxer);
    CHECK_AND_RETURN_RET_LOG(demuxerObj->demuxer_ != nullptr, AV_ERR_INVALID_VAL,
        "New DemuxerObject failed when unselect track!");

    int32_t ret = demuxerObj->demuxer_->UnselectTrackByID(trackIndex);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "demuxer_ UnselectTrackByID failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AVDemuxer_ReadSample(OH_AVDemuxer *demuxer, uint32_t trackIndex,
    OH_AVMemory *sample, OH_AVCodecBufferAttr *info)
{
    CHECK_AND_RETURN_RET_LOG(demuxer != nullptr, AV_ERR_INVALID_VAL,
        "Read sample failed because input demuxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(demuxer->magic_ == AVMagic::AVCODEC_MAGIC_AVDEMUXER, AV_ERR_INVALID_VAL, "magic error!");
    
    CHECK_AND_RETURN_RET_LOG(sample != nullptr && sample->memory_ != nullptr, AV_ERR_INVALID_VAL,
        "Read sample failed because input sample is nullptr!");
    CHECK_AND_RETURN_RET_LOG(info != nullptr, AV_ERR_INVALID_VAL,
        "Read sample failed because input info is nullptr!");

    struct DemuxerObject *demuxerObj = reinterpret_cast<DemuxerObject *>(demuxer);
    CHECK_AND_RETURN_RET_LOG(demuxerObj->demuxer_ != nullptr, AV_ERR_INVALID_VAL,
        "New DemuxerObject failed when read sample!");

    struct AVCodecBufferInfo bufferInfoInner;
    uint32_t bufferFlag = (uint32_t)(AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE);
    int32_t ret = demuxerObj->demuxer_->ReadSample(trackIndex, sample->memory_, bufferInfoInner, bufferFlag);
    info->pts = bufferInfoInner.presentationTimeUs;
    info->size = bufferInfoInner.size;
    info->offset = bufferInfoInner.offset;
    info->flags = bufferFlag;

    CHECK_AND_RETURN_RET_LOG(ret != AVCS_ERR_NO_MEMORY, AV_ERR_NO_MEMORY,
        "demuxer_ ReadSample failed! sample size is too small to copy full frame data");
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "demuxer_ ReadSample failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AVDemuxer_ReadSampleBuffer(OH_AVDemuxer *demuxer, uint32_t trackIndex, OH_AVBuffer *sample)
{
    CHECK_AND_RETURN_RET_LOG(demuxer != nullptr, AV_ERR_INVALID_VAL,
        "Read sample failed because input demuxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(demuxer->magic_ == AVMagic::AVCODEC_MAGIC_AVDEMUXER, AV_ERR_INVALID_VAL, "magic error!");
    
    CHECK_AND_RETURN_RET_LOG(sample != nullptr && sample->buffer_ != nullptr, AV_ERR_INVALID_VAL,
        "Read sample failed because input sample is nullptr!");

    struct DemuxerObject *demuxerObj = reinterpret_cast<DemuxerObject *>(demuxer);
    CHECK_AND_RETURN_RET_LOG(demuxerObj->demuxer_ != nullptr, AV_ERR_INVALID_VAL,
        "New DemuxerObject failed when read sample!");

    int32_t ret = demuxerObj->demuxer_->ReadSampleBuffer(trackIndex, sample->buffer_);
    CHECK_AND_RETURN_RET_LOG(ret != AVCS_ERR_NO_MEMORY, AV_ERR_NO_MEMORY,
        "demuxer_ ReadSampleBuffer failed! sample size is too small to copy full frame data");
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "demuxer_ ReadSampleBuffer failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AVDemuxer_SeekToTime(OH_AVDemuxer *demuxer, int64_t millisecond, OH_AVSeekMode mode)
{
    CHECK_AND_RETURN_RET_LOG(demuxer != nullptr, AV_ERR_INVALID_VAL, "Seek failed because input demuxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(demuxer->magic_ == AVMagic::AVCODEC_MAGIC_AVDEMUXER, AV_ERR_INVALID_VAL, "magic error!");

    CHECK_AND_RETURN_RET_LOG(millisecond >= 0, AV_ERR_INVALID_VAL,
        "Seek failed because input millisecond is negative!");

    struct DemuxerObject *demuxerObj = reinterpret_cast<DemuxerObject *>(demuxer);
    CHECK_AND_RETURN_RET_LOG(demuxerObj->demuxer_ != nullptr, AV_ERR_INVALID_VAL,
        "New DemuxerObject failed when seek!");

    int32_t ret = demuxerObj->demuxer_->SeekToTime(millisecond, static_cast<OHOS::Media::SeekMode>(mode));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "demuxer_ SeekToTime failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AVDemuxer_SetMediaKeySystemInfoCallback(OH_AVDemuxer *demuxer,
    DRM_MediaKeySystemInfoCallback callback)
{
    CHECK_AND_RETURN_RET_LOG(demuxer != nullptr, AV_ERR_INVALID_VAL, "Seek failed because input demuxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(demuxer->magic_ == AVMagic::AVCODEC_MAGIC_AVDEMUXER, AV_ERR_INVALID_VAL, "magic error!");

    struct DemuxerObject *demuxerObj = reinterpret_cast<DemuxerObject *>(demuxer);
    CHECK_AND_RETURN_RET_LOG(demuxerObj->demuxer_ != nullptr, AV_ERR_INVALID_VAL,
        "New DemuxerObject failed when set callback!");

    demuxerObj->callback_ = std::make_shared<NativeDemuxerCallback>(demuxer, callback);
    int32_t ret = demuxerObj->demuxer_->SetCallback(demuxerObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "demuxer_ set callback failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVDemuxer_SetDemuxerMediaKeySystemInfoCallback(OH_AVDemuxer *demuxer,
    Demuxer_MediaKeySystemInfoCallback callback)
{
    CHECK_AND_RETURN_RET_LOG(demuxer != nullptr, AV_ERR_INVALID_VAL, "Seek failed because input demuxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(demuxer->magic_ == AVMagic::AVCODEC_MAGIC_AVDEMUXER, AV_ERR_INVALID_VAL, "magic error!");

    struct DemuxerObject *demuxerObj = reinterpret_cast<DemuxerObject *>(demuxer);
    CHECK_AND_RETURN_RET_LOG(demuxerObj->demuxer_ != nullptr, AV_ERR_INVALID_VAL,
        "New DemuxerObject failed when set callback!");

    demuxerObj->callback_ = std::make_shared<NativeDemuxerCallback>(demuxer, callback);
    int32_t ret = demuxerObj->demuxer_->SetCallback(demuxerObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "demuxer_ set callback failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVDemuxer_GetMediaKeySystemInfo(OH_AVDemuxer *demuxer, DRM_MediaKeySystemInfo *mediaKeySystemInfo)
{
    CHECK_AND_RETURN_RET_LOG(demuxer != nullptr, AV_ERR_INVALID_VAL, "Seek failed because input demuxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(demuxer->magic_ == AVMagic::AVCODEC_MAGIC_AVDEMUXER, AV_ERR_INVALID_VAL, "magic error!");

    struct DemuxerObject *demuxerObj = reinterpret_cast<DemuxerObject *>(demuxer);
    CHECK_AND_RETURN_RET_LOG(demuxerObj != nullptr, AV_ERR_INVALID_VAL,
        "New DemuxerObject failed when set callback!");
    CHECK_AND_RETURN_RET_LOG(demuxerObj->demuxer_ != nullptr, AV_ERR_INVALID_VAL,
        "New DemuxerObject failed cause impl is null when set callback!");

    std::multimap<std::string, std::vector<uint8_t>> drmInfos;
    int32_t ret = demuxerObj->demuxer_->GetMediaKeySystemInfo(drmInfos);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "demuxer_ GetMediaKeySystemInfo failed!");
    CHECK_AND_RETURN_RET_LOG(!drmInfos.empty(), AV_ERR_OK, "DrmInfo is null");

    ret = NativeDrmTools::ProcessApplicationDrmInfo(mediaKeySystemInfo, drmInfos);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ProcessApplicationDrmInfo failed");

    return AV_ERR_OK;
}