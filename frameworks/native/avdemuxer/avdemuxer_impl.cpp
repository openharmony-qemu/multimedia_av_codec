/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "AVDemuxerImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
using namespace Media::Plugins;
std::shared_ptr<AVDemuxer> AVDemuxerFactory::CreateWithSource(std::shared_ptr<AVSource> source)
{
    AVCODEC_SYNC_TRACE;

    std::shared_ptr<AVDemuxerImpl> demuxerImpl = std::make_shared<AVDemuxerImpl>();
    CHECK_AND_RETURN_RET_LOG(demuxerImpl != nullptr, nullptr, "New avdemuxer failed");
    
    int32_t ret = demuxerImpl->Init(source);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init avdemuxer failed");

    return demuxerImpl;
}

int32_t AVDemuxerImpl::Init(std::shared_ptr<AVSource> source)
{
    AVCODEC_SYNC_TRACE;

    CHECK_AND_RETURN_RET_LOG(source != nullptr, AVCS_ERR_INVALID_VAL, "AVSource is nullptr");
    AVCODEC_LOGD("Init avdemuxer for %{private}s", source->sourceUri.c_str());

    mediaDemuxer_ = source->mediaDemuxer;
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Init mediaDemuxer failed");
    return AVCS_ERR_OK;
}

AVDemuxerImpl::AVDemuxerImpl()
{
    AVCODEC_LOGD("Create instances 0x%{public}06" PRIXPTR, FAKE_POINTER(this));
}

AVDemuxerImpl::~AVDemuxerImpl()
{
    if (mediaDemuxer_ != nullptr) {
        mediaDemuxer_->OnInterrupted(true);
    }
    std::unique_lock<std::shared_mutex> lock(mutex_);
    mediaDemuxer_ = nullptr;
    AVCODEC_LOGD("Destroy instances 0x%{public}06" PRIXPTR, FAKE_POINTER(this));
}

int32_t AVDemuxerImpl::SelectTrackByID(uint32_t trackIndex)
{
    AVCODEC_SYNC_TRACE;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("Select track %{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    return StatusToAVCodecServiceErrCode(mediaDemuxer_->SelectTrack(trackIndex));
}

int32_t AVDemuxerImpl::UnselectTrackByID(uint32_t trackIndex)
{
    AVCODEC_SYNC_TRACE;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("Unselect track %{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    return StatusToAVCodecServiceErrCode(mediaDemuxer_->UnselectTrack(trackIndex));
}

int32_t AVDemuxerImpl::ReadSampleBuffer(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample)
{
    AVCODEC_SYNC_TRACE;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("ReadSampleBuffer for track %{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");

    CHECK_AND_RETURN_RET_LOG(sample != nullptr && sample->memory_ != nullptr, AVCS_ERR_INVALID_VAL,
        "Sample buffer is nullptr");

    return StatusToAVCodecServiceErrCode(mediaDemuxer_->ReadSample(trackIndex, sample));
}

int32_t AVDemuxerImpl::ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
    AVCodecBufferInfo &info, uint32_t &flag)
{
    AVCODEC_SYNC_TRACE;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("ReadSample for track %{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");

    CHECK_AND_RETURN_RET_LOG(sample != nullptr, AVCS_ERR_INVALID_VAL, "Sample buffer is nullptr");

    CHECK_AND_RETURN_RET_LOG(sample->GetSize() > 0, AVCS_ERR_INVALID_VAL, "Sample size must be greater than 0");
    
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(
        sample->GetBase(), sample->GetSize(), sample->GetSize());
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->memory_ != nullptr, AVCS_ERR_INVALID_VAL,
        "Buffer is nullptr");
    Status ret = mediaDemuxer_->ReadSample(trackIndex, buffer);

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

    CHECK_AND_RETURN_RET_LOG(sample != nullptr, AVCS_ERR_INVALID_VAL, "Sample buffer is nullptr");
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(
        sample->GetBase(), sample->GetSize(), sample->GetSize());
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->memory_ != nullptr, AVCS_ERR_INVALID_VAL,
        "Buffer is nullptr");

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

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("Seek to time: millisecond=%{public}" PRId64 "; mode=%{public}d", millisecond, mode);

    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");

    CHECK_AND_RETURN_RET_LOG(millisecond >= 0, AVCS_ERR_INVALID_VAL, "Millisecond is negative");
    
    int64_t realTime = 0;
    return StatusToAVCodecServiceErrCode(mediaDemuxer_->SeekTo(millisecond, mode, realTime));
}

int32_t AVDemuxerImpl::SetCallback(const std::shared_ptr<AVDemuxerCallback> &callback)
{
    AVCODEC_SYNC_TRACE;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("AVDemuxer::SetCallback");
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_INVALID_VAL, "Callback is nullptr");
    mediaDemuxer_->SetDrmCallback(callback);
    return AVCS_ERR_OK;
}

int32_t AVDemuxerImpl::GetMediaKeySystemInfo(std::multimap<std::string, std::vector<uint8_t>> &infos)
{
    AVCODEC_SYNC_TRACE;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("AVDemuxer::GetMediaKeySystemInfo");
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    mediaDemuxer_->GetMediaKeySystemInfo(infos);
    return AVCS_ERR_OK;
}

int32_t AVDemuxerImpl::StartReferenceParser(int64_t startTimeMs)
{
    AVCODEC_SYNC_TRACE;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("AVDemuxer::StartReferenceParser");
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    return StatusToAVCodecServiceErrCode(mediaDemuxer_->StartReferenceParser(startTimeMs));
}

int32_t AVDemuxerImpl::GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo)
{
    AVCODEC_SYNC_TRACE;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("AVDemuxer::GetFrameLayerInfo");
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    return StatusToAVCodecServiceErrCode(mediaDemuxer_->GetFrameLayerInfo(videoSample, frameLayerInfo));
}

int32_t AVDemuxerImpl::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    AVCODEC_SYNC_TRACE;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("AVDemuxer::GetGopLayerInfo");
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    return StatusToAVCodecServiceErrCode(mediaDemuxer_->GetGopLayerInfo(gopId, gopLayerInfo));
}

int32_t AVDemuxerImpl::GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
    const uint64_t relativePresentationTimeUs, uint32_t &index)
{
    AVCODEC_SYNC_TRACE;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("GetIndexByRelativePresentationTimeUs");
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    int32_t ret = StatusToAVCodecServiceErrCode(mediaDemuxer_->GetIndexByRelativePresentationTimeUs(trackIndex,
        relativePresentationTimeUs, index));
    return ret;
}

int32_t AVDemuxerImpl::GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
    const uint32_t index, uint64_t &relativePresentationTimeUs)
{
    AVCODEC_SYNC_TRACE;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    AVCODEC_LOGD("GetRelativePresentationTimeUsByIndex");
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    int32_t ret = StatusToAVCodecServiceErrCode(mediaDemuxer_->GetRelativePresentationTimeUsByIndex(trackIndex,
        index, relativePresentationTimeUs));
    return ret;
}

int32_t AVDemuxerImpl::GetCurrentCacheSize(uint32_t trackIndex, uint32_t& size)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("AVDemuxer::GetCurrentCacheSize");
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    return StatusToAVCodecServiceErrCode(mediaDemuxer_->GetCurrentCacheSize(trackIndex, size));
}
} // namespace MediaAVCodec
} // namespace OHOS