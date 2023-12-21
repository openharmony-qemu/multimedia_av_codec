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

#include "avdemuxer_impl.h"
#include <fcntl.h>
#include <unistd.h>
#include <functional>
#include <sys/types.h>
#include "securec.h"
#include "avcodec_log.h"
#include "buffer/avsharedmemorybase.h"
#include "avcodec_trace.h"
#include "i_avcodec_service.h"
#include "avcodec_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVDemuxerImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<AVDemuxer> AVDemuxerFactory::CreateWithSource(std::shared_ptr<AVSource> source)
{
    AVCodecTrace trace("AVDemuxerFactory::CreateWithSource");

    std::shared_ptr<AVDemuxerImpl> demuxerImpl = std::make_shared<AVDemuxerImpl>();
    CHECK_AND_RETURN_RET_LOG(demuxerImpl != nullptr, nullptr, "New AVDemuxerImpl failed");
    
    int32_t ret = demuxerImpl->Init(source);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init AVDemuxerImpl failed");

    return demuxerImpl;
}

int32_t AVDemuxerImpl::Init(std::shared_ptr<AVSource> source)
{
    AVCodecTrace trace("AVDemuxer::Init");

    CHECK_AND_RETURN_RET_LOG(source != nullptr, AVCS_ERR_INVALID_VAL,
        "Init AVDemuxerImpl failed because source is nullptr");
    AVCODEC_LOGI("Init AVDemuxerImpl for source %{private}s", source->sourceUri.c_str());

    uintptr_t sourceAddr;
    int32_t ret = source->GetSourceAddr(sourceAddr);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Init AVDemuxerImpl failed because get source address failed");
    sourceUri_ = source->sourceUri;

    demuxerEngine_ = IDemuxerEngineFactory::CreateDemuxerEngine(sourceAddr);
    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Create demuxer engine failed");
    return AVCS_ERR_OK;
}

AVDemuxerImpl::AVDemuxerImpl()
{
    AVCODEC_LOGD("AVDemuxerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVDemuxerImpl::~AVDemuxerImpl()
{
    AVCODEC_LOGI("Destroy AVDemuxerImpl for source %{private}s", sourceUri_.c_str());
    if (demuxerEngine_ != nullptr) {
        demuxerEngine_ = nullptr;
    }
    AVCODEC_LOGD("AVDemuxerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVDemuxerImpl::SelectTrackByID(uint32_t trackIndex)
{
    AVCodecTrace trace("AVDemuxer::SelectTrackByID");

    AVCODEC_LOGI("select track: trackIndex=%{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist");
    return demuxerEngine_->SelectTrackByID(trackIndex);
}

int32_t AVDemuxerImpl::UnselectTrackByID(uint32_t trackIndex)
{
    AVCodecTrace trace("AVDemuxer::UnselectTrackByID");

    AVCODEC_LOGI("unselect track: trackIndex=%{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist");
    return demuxerEngine_->UnselectTrackByID(trackIndex);
}

int32_t AVDemuxerImpl::ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
    AVCodecBufferInfo &info, uint32_t &flag)
{
    AVCodecTrace trace("AVDemuxer::ReadSample");

    AVCODEC_LOGD("ReadSample: trackIndex=%{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist");

    CHECK_AND_RETURN_RET_LOG(sample != nullptr, AVCS_ERR_INVALID_VAL,
        "Copy sample failed because sample buffer is nullptr!");

    CHECK_AND_RETURN_RET_LOG(sample->GetSize() > 0, AVCS_ERR_INVALID_VAL,
        "Copy sample failed, Memory must be greater than 0");
    return demuxerEngine_->ReadSample(trackIndex, sample, info, flag);
}

int32_t AVDemuxerImpl::ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
    AVCodecBufferInfo &info, AVCodecBufferFlag &flag)
{
    uint32_t innerFlag;
    int32_t ret = ReadSample(trackIndex, sample, info, innerFlag);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
    if (innerFlag & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME) {
        flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME;
    } else if (innerFlag & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS) {
        flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS;
    }
    return AVCS_ERR_OK;
}

int32_t AVDemuxerImpl::SeekToTime(int64_t millisecond, AVSeekMode mode)
{
    AVCodecTrace trace("AVDemuxer::SeekToTime");

    AVCODEC_LOGI("seek to time: millisecond=%{public}" PRId64 "; mode=%{public}d", millisecond, mode);

    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist");

    CHECK_AND_RETURN_RET_LOG(millisecond >= 0, AVCS_ERR_INVALID_VAL,
        "Seek failed because input millisecond is negative!");
    
    return demuxerEngine_->SeekToTime(millisecond, mode);
}
} // namespace MediaAVCodec
} // namespace OHOS