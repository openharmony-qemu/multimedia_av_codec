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

#include "avmuxer_impl.h"
#include <unistd.h>
#include <fcntl.h>
#include "securec.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER_MUXER, "AVMuxerImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<AVMuxer> AVMuxerFactory::CreateAVMuxer(int32_t fd, Plugins::OutputFormat format)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(fd >= 0, nullptr, "fd %{public}d is error!", fd);
    uint32_t fdPermission = static_cast<uint32_t>(fcntl(fd, F_GETFL, 0));
    CHECK_AND_RETURN_RET_LOG((fdPermission & O_RDWR) == O_RDWR, nullptr, "No permission to read and write fd");
    CHECK_AND_RETURN_RET_LOG(lseek(fd, 0, SEEK_CUR) != -1, nullptr, "The fd is not seekable");

    std::shared_ptr<AVMuxerImpl> impl = std::make_shared<AVMuxerImpl>();

    int32_t ret = impl->Init(fd, format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init avmuxer implementation failed");
    return impl;
}

AVMuxerImpl::AVMuxerImpl()
{
    AVCODEC_LOGD("AVMuxerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMuxerImpl::~AVMuxerImpl()
{
    AVCODEC_LOGD("AVMuxerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVMuxerImpl::Init(int32_t fd, Plugins::OutputFormat format)
{
    AVCODEC_SYNC_TRACE;
    muxerEngine_ = std::make_shared<Media::MediaMuxer>(getuid(), getprocpid());
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_NO_MEMORY, "Create AVMuxer Engine failed");
    return StatusConvert(muxerEngine_->Init(fd, format));
}

int32_t AVMuxerImpl::SetParameter(const std::shared_ptr<Meta> &param)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    CHECK_AND_RETURN_RET_LOG(param != nullptr, AVCS_ERR_INVALID_VAL, "Invalid parameter");
    return StatusConvert(muxerEngine_->SetParameter(param));
}

int32_t AVMuxerImpl::SetUserMeta(const std::shared_ptr<Meta> &userMeta)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    CHECK_AND_RETURN_RET_LOG(userMeta != nullptr, AVCS_ERR_INVALID_VAL, "Invalid parameter");
    return StatusConvert(muxerEngine_->SetUserMeta(userMeta));
}

int32_t AVMuxerImpl::AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    CHECK_AND_RETURN_RET_LOG(trackDesc != nullptr, AVCS_ERR_INVALID_VAL, "Invalid track format");
    return StatusConvert(muxerEngine_->AddTrack(trackIndex, trackDesc));
}

sptr<AVBufferQueueProducer> AVMuxerImpl::GetInputBufferQueue(uint32_t trackIndex)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, nullptr, "AVMuxer Engine does not exist");
    return muxerEngine_->GetInputBufferQueue(trackIndex);
}

int32_t AVMuxerImpl::Start()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    return StatusConvert(muxerEngine_->Start());
}

int32_t AVMuxerImpl::WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample)
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    CHECK_AND_RETURN_RET_LOG(sample != nullptr && sample->memory_ != nullptr &&
        sample->memory_->GetSize() >= 0, AVCS_ERR_INVALID_VAL, "Invalid memory");
    return StatusConvert(muxerEngine_->WriteSample(trackIndex, sample));
}

int32_t AVMuxerImpl::Stop()
{
    AVCODEC_SYNC_TRACE;
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    return StatusConvert(muxerEngine_->Stop());
}

int32_t AVMuxerImpl::StatusConvert(Media::Status status)
{
    const static std::unordered_map<Media::Status, int32_t> table = {
        {Status::END_OF_STREAM, AVCodecServiceErrCode::AVCS_ERR_OK},
        {Status::OK, AVCodecServiceErrCode::AVCS_ERR_OK},
        {Status::NO_ERROR, AVCodecServiceErrCode::AVCS_ERR_OK},
        {Status::ERROR_UNKNOWN, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Status::ERROR_PLUGIN_ALREADY_EXISTS, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Status::ERROR_INCOMPATIBLE_VERSION, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Status::ERROR_NO_MEMORY, AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY},
        {Status::ERROR_WRONG_STATE, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION},
        {Status::ERROR_UNIMPLEMENTED, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT},
        {Status::ERROR_INVALID_PARAMETER, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Status::ERROR_INVALID_DATA, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Status::ERROR_MISMATCHED_TYPE, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Status::ERROR_TIMED_OUT, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Status::ERROR_UNSUPPORTED_FORMAT, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT_FILE_TYPE},
        {Status::ERROR_NOT_ENOUGH_DATA, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Status::ERROR_NOT_EXISTED, AVCodecServiceErrCode::AVCS_ERR_OPEN_FILE_FAILED},
        {Status::ERROR_AGAIN, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Status::ERROR_PERMISSION_DENIED, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Status::ERROR_NULL_POINTER, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Status::ERROR_INVALID_OPERATION, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION},
        {Status::ERROR_CLIENT, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Status::ERROR_SERVER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Status::ERROR_DELAY_READY, AVCodecServiceErrCode::AVCS_ERR_OK},
        {Status::ERROR_INVALID_BUFFER_SIZE, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
    };
    auto ite = table.find(status);
    if (ite == table.end()) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    return ite->second;
}
} // namespace MediaAVCodec
} // namespace OHOS