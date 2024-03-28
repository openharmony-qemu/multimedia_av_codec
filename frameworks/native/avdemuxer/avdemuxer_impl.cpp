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
#include "meta/media_types.h"
#include "avcodec_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER_MUXER, "AVDemuxerImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
using namespace Media::Plugins;
std::shared_ptr<AVDemuxer> AVDemuxerFactory::CreateWithSource(std::shared_ptr<AVSource> source)
{
    AVCODEC_SYNC_TRACE;

    std::shared_ptr<AVDemuxerImpl> demuxerImpl = std::make_shared<AVDemuxerImpl>();
    CHECK_AND_RETURN_RET_LOG(demuxerImpl != nullptr, nullptr, "New AVDemuxerImpl failed");
    
    int32_t ret = demuxerImpl->Init(source);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init AVDemuxerImpl failed");

    return demuxerImpl;
}

int32_t AVDemuxerImpl::Init(std::shared_ptr<AVSource> source)
{
    AVCODEC_SYNC_TRACE;

    CHECK_AND_RETURN_RET_LOG(source != nullptr, AVCS_ERR_INVALID_VAL,
        "Init AVDemuxerImpl failed because source is nullptr");
    AVCODEC_LOGD("Init AVDemuxerImpl for source %{private}s", source->sourceUri.c_str());

    demuxerEngine_ = source->demuxerEngine;
    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Init demuxer engine failed");
    return AVCS_ERR_OK;
}

AVDemuxerImpl::AVDemuxerImpl()
{
    AVCODEC_LOGD("AVDemuxerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVDemuxerImpl::~AVDemuxerImpl()
{
    AVCODEC_LOGD("Destroy AVDemuxerImpl for source %{private}s", sourceUri_.c_str());
    if (demuxerEngine_ != nullptr) {
        demuxerEngine_ = nullptr;
    }
    AVCODEC_LOGD("AVDemuxerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVDemuxerImpl::SelectTrackByID(uint32_t trackIndex)
{
    AVCODEC_SYNC_TRACE;

    AVCODEC_LOGD("select track: trackIndex=%{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist");
    return StatusToAVCodecServiceErrCode(demuxerEngine_->SelectTrack(trackIndex));
}

int32_t AVDemuxerImpl::UnselectTrackByID(uint32_t trackIndex)
{
    AVCODEC_SYNC_TRACE;

    AVCODEC_LOGD("unselect track: trackIndex=%{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist");
    return StatusToAVCodecServiceErrCode(demuxerEngine_->UnselectTrack(trackIndex));
}

int32_t AVDemuxerImpl::ReadSampleBuffer(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample)
{
    AVCODEC_SYNC_TRACE;

    AVCODEC_LOGD("ReadSampleBuffer: trackIndex=%{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist");

    CHECK_AND_RETURN_RET_LOG(sample != nullptr && sample->memory_ != nullptr, AVCS_ERR_INVALID_VAL,
        "Read sample failed because sample buffer is nullptr!");

    return StatusToAVCodecServiceErrCode(demuxerEngine_->ReadSample(trackIndex, sample));
}

int32_t AVDemuxerImpl::ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
    AVCodecBufferInfo &info, uint32_t &flag)
{
    AVCODEC_SYNC_TRACE;

    AVCODEC_LOGD("ReadSample: trackIndex=%{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist");

    CHECK_AND_RETURN_RET_LOG(sample != nullptr, AVCS_ERR_INVALID_VAL,
        "Read sample failed because sample buffer is nullptr!");

    CHECK_AND_RETURN_RET_LOG(sample->GetSize() > 0, AVCS_ERR_INVALID_VAL,
        "Read sample failed, Memory must be greater than 0");
    
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(
        sample->GetBase(), sample->GetSize(), sample->GetSize());
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->memory_ != nullptr, AVCS_ERR_INVALID_VAL,
        "Read sample failed because buffer is nullptr!");
    Status ret = demuxerEngine_->ReadSample(trackIndex, buffer);

    info.presentationTimeUs = buffer->pts_;
    info.size = buffer->memory_->GetSize();
    info.offset = 0;
    flag = buffer->flag_;
    return StatusToAVCodecServiceErrCode(ret);
}

int32_t AVDemuxerImpl::ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
    AVCodecBufferInfo &info, AVCodecBufferFlag &flag)
{
    AVCODEC_SYNC_TRACE;

    CHECK_AND_RETURN_RET_LOG(sample != nullptr, AVCS_ERR_INVALID_VAL,
        "Read sample failed because sample buffer is nullptr!");
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(
        sample->GetBase(), sample->GetSize(), sample->GetSize());
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->memory_ != nullptr, AVCS_ERR_INVALID_VAL,
        "Read sample failed because buffer is nullptr!");

    int32_t ret = ReadSampleBuffer(trackIndex, buffer);
    info.presentationTimeUs = buffer->pts_;
    info.size = buffer->memory_->GetSize();
    info.offset = 0;
    
    AVBufferFlag innerFlag = AVBufferFlag::NONE;
    if (buffer->flag_ & (uint32_t)(AVBufferFlag::SYNC_FRAME)) {
        innerFlag = AVBufferFlag::SYNC_FRAME;
    } else if (buffer->flag_ & (uint32_t)(AVBufferFlag::EOS)) {
        innerFlag = AVBufferFlag::EOS;
    }
    flag = static_cast<AVCodecBufferFlag>(innerFlag);
    return ret;
}

int32_t AVDemuxerImpl::SeekToTime(int64_t millisecond, SeekMode mode)
{
    AVCODEC_SYNC_TRACE;

    AVCODEC_LOGD("seek to time: millisecond=%{public}" PRId64 "; mode=%{public}d", millisecond, mode);

    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist");

    CHECK_AND_RETURN_RET_LOG(millisecond >= 0, AVCS_ERR_INVALID_VAL,
        "Seek failed because input millisecond is negative!");
    
    int64_t realTime = 0;
    return StatusToAVCodecServiceErrCode(demuxerEngine_->SeekTo(millisecond, mode, realTime));
}

int32_t AVDemuxerImpl::SetCallback(const std::shared_ptr<AVDemuxerCallback> &callback)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGI("AVDemuxer::SetCallback");
    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION,
        "Demuxer engine does not exist");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_INVALID_VAL,
        "SetCallback failed because callback is nullptr!");
    demuxerEngine_->SetDrmCallback(callback);
    return AVCS_ERR_OK;
}

int32_t AVDemuxerImpl::GetMediaKeySystemInfo(std::multimap<std::string, std::vector<uint8_t>> &infos)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGI("AVDemuxer::GetMediaKeySystemInfo");
    CHECK_AND_RETURN_RET_LOG(demuxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION,
        "Demuxer engine does not exist");
    demuxerEngine_->GetMediaKeySystemInfo(infos);
    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS