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
#include "native_drm_tools.h"
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeAVDemuxer"};
}

using namespace OHOS::MediaAVCodec;

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
    explicit NativeDemuxerCallback(OH_AVDemuxer *demuxer, struct OH_AVDemuxerCallback cb,
        void *userData) : demuxer_(demuxer), callback_(cb), userData_(userData)
    {
    }

    virtual ~NativeDemuxerCallback() = default;

    void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>> &drmInfo) override
    {
        AVCODEC_LOGI("NativeDemuxerCallback OnDrmInfoChanged is on call");
        std::unique_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN_LOG(demuxer_ != nullptr, "demuxer_ is nullptr");

        OH_DrmInfo *info = (struct OH_DrmInfo *)malloc(sizeof(struct OH_DrmInfo));
        CHECK_AND_RETURN_LOG(info != NULL, "Malloc DrmInfo failed");
        int32_t ret = NativeDrmTools::MallocAndCopyDrmInfo(info, drmInfo);
        CHECK_AND_RETURN_LOG(ret == AV_ERR_OK, "Malloc And Copy DrmInfo failed");

        CHECK_AND_RETURN_LOG(callback_.onDrmInfoChanged != nullptr, "DrmInfoChanged Callback is nullptr");
        callback_.onDrmInfoChanged(demuxer_, info, userData_);
        NativeDrmTools::FreeDrmInfo(info, drmInfo.size());
        if (info != NULL) {
            free(info);
            info = NULL;
        }
    }

private:
    std::shared_mutex mutex_;
    struct OH_AVDemuxer *demuxer_;
    struct OH_AVDemuxerCallback callback_;
    void *userData_;
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

OH_AVErrCode OH_AVDemuxer_SetMediaKeySystemInfoCallback(OH_AVDemuxer *demuxer, OH_AVDemuxerCallback callback,
    void *userData)
{
    CHECK_AND_RETURN_RET_LOG(demuxer != nullptr, AV_ERR_INVALID_VAL, "Seek failed because input demuxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(demuxer->magic_ == AVMagic::AVCODEC_MAGIC_AVDEMUXER, AV_ERR_INVALID_VAL, "magic error!");

    struct DemuxerObject *demuxerObj = reinterpret_cast<DemuxerObject *>(demuxer);
    CHECK_AND_RETURN_RET_LOG(demuxerObj->demuxer_ != nullptr, AV_ERR_INVALID_VAL,
        "New DemuxerObject failed when set callback!");

    demuxerObj->callback_ = std::make_shared<NativeDemuxerCallback>(demuxer, callback, userData);
    int32_t ret = demuxerObj->demuxer_->SetCallback(demuxerObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "demuxer_ set callback failed!");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVDemuxer_GetMediaKeySystemInfo(OH_AVDemuxer *demuxer, OH_DrmInfo **mediaKeySystemInfo)
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

    ret = NativeDrmTools::MallocAndCopyDrmInfo(*mediaKeySystemInfo, drmInfos);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "Malloc And Copy DrmInfo failed");

    return AV_ERR_OK;
}