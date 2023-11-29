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
#include "native_object.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeAVDemuxer"};
}

using namespace OHOS::MediaAVCodec;

struct DemuxerObject : public OH_AVDemuxer {
    explicit DemuxerObject(const std::shared_ptr<AVDemuxer> &demuxer)
        : OH_AVDemuxer(AVMagic::AVCODEC_MAGIC_AVDEMUXER), demuxer_(demuxer) {}
    ~DemuxerObject() = default;

    const std::shared_ptr<AVDemuxer> demuxer_;
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
    
    CHECK_AND_RETURN_RET_LOG(sample != nullptr, AV_ERR_INVALID_VAL,
        "Read sample failed because input sample is nullptr!");
    CHECK_AND_RETURN_RET_LOG(info != nullptr, AV_ERR_INVALID_VAL,
        "Read sample failed because input info is nullptr!");

    struct DemuxerObject *demuxerObj = reinterpret_cast<DemuxerObject *>(demuxer);
    CHECK_AND_RETURN_RET_LOG(demuxerObj->demuxer_ != nullptr, AV_ERR_INVALID_VAL,
        "New DemuxerObject failed when read sample!");

    struct AVCodecBufferInfo bufferInfoInner;
    AVCodecBufferFlag bufferFlag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
    int32_t ret = demuxerObj->demuxer_->ReadSample(trackIndex, sample->memory_, bufferInfoInner, bufferFlag);
    info->pts = bufferInfoInner.presentationTimeUs;
    info->size = bufferInfoInner.size;
    info->offset = bufferInfoInner.offset;
    info->flags =static_cast<uint32_t>(bufferFlag);

    CHECK_AND_RETURN_RET_LOG(ret != AVCS_ERR_NO_MEMORY, AV_ERR_NO_MEMORY,
        "demuxer_ ReadSample failed! sample size is too small to copy full frame data");
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "demuxer_ ReadSample failed!");

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

    int32_t ret = demuxerObj->demuxer_->SeekToTime(millisecond, static_cast<AVSeekMode>(mode));
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "demuxer_ SeekToTime failed!");

    return AV_ERR_OK;
}