/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "demuxer_engine_impl.h"
#include "securec.h"
#include "avcodec_log.h"
#include "demuxer_factory.h"
#include "avcodec_errors.h"
#include "iostream"
#include "avcodec_trace.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "DemuxerEngineImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
std::shared_ptr<IDemuxerEngine> IDemuxerEngineFactory::CreateDemuxerEngine(uintptr_t sourceAddr)
{
    AVCodecTrace trace("IDemuxerEngineFactory::CreateDemuxerEngine");
    std::shared_ptr<IDemuxerEngine> demuxerEngineImpl = std::make_shared<DemuxerEngineImpl>(sourceAddr);
    CHECK_AND_RETURN_RET_LOG(demuxerEngineImpl != nullptr, nullptr, "create DemuxerEngine implementation failed");
    return demuxerEngineImpl;
}

DemuxerEngineImpl::DemuxerEngineImpl(uintptr_t sourceAddr)
    : sourceAddr_(sourceAddr)
{
    AVCodecTrace trace("DemuxerEngineImpl::Create");
    AVCODEC_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    demuxer_ = Plugin::DemuxerFactory::Instance().CreatePlugin(sourceAddr_);
    if (demuxer_ != nullptr) {
        AVCODEC_LOGI("init demuxer engine successful");
    } else {
        AVCODEC_LOGE("init demuxer engine failed");
    }
}

DemuxerEngineImpl::~DemuxerEngineImpl()
{
    AVCODEC_LOGD("Destroy");
    demuxer_ = nullptr;

    AVCODEC_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t DemuxerEngineImpl::SelectTrackByID(uint32_t trackIndex)
{
    AVCodecTrace trace("DemuxerEngineImpl::SelectTrackByID");
    AVCODEC_LOGI("SelectTrackByID");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(demuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "demuxer_ is nullptr");
    return demuxer_->SelectTrackByID(trackIndex);
}

int32_t DemuxerEngineImpl::UnselectTrackByID(uint32_t trackIndex)
{
    AVCodecTrace trace("DemuxerEngineImpl::UnselectTrackByID");
    AVCODEC_LOGI("UnselectTrackByID");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(demuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "demuxer_ is nullptr");
    return demuxer_->UnselectTrackByID(trackIndex);
}

int32_t DemuxerEngineImpl::ReadSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample)
{
    AVCodecTrace trace("DemuxerEngineImpl::ReadSample");
    AVCODEC_LOGD("ReadSample");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(demuxer_ != nullptr, AVCS_ERR_INVALID_OPERATION, "demuxer_ is nullptr");
    return demuxer_->ReadSample(trackIndex, sample);
}

int32_t DemuxerEngineImpl::SeekToTime(int64_t millisecond, AVSeekMode mode)
{
    AVCodecTrace trace("DemuxerEngineImpl::SeekToTime");
    AVCODEC_LOGI("SeekToTime");
    std::unique_lock<std::mutex> lock(mutex_);
    return demuxer_->SeekToTime(millisecond, mode);
}

void DemuxerEngineImpl::SetDrmCallback(const std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> &callback)
{
    AVCodecTrace trace("DemuxerEngineImpl::SetDrmCallback");
    AVCODEC_LOGI("SetDrmCallback");
    (void)callback;
}
} // MediaAVCodec
} // OHOS
