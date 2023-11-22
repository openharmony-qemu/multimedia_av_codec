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
#include "i_avcodec_service.h"
#include "avcodec_log.h"
#include "avsharedmemorybase.h"
#include "avcodec_dfx.h"
#include "avcodec_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<AVMuxer> AVMuxerFactory::CreateAVMuxer(int32_t fd, OutputFormat format)
{
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
    if (muxerEngine_ != nullptr) {
        (void)muxerEngine_->Stop();
        muxerEngine_ = nullptr;
    }
    AVCODEC_LOGD("AVMuxerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVMuxerImpl::Init(int32_t fd, OutputFormat format)
{
    muxerEngine_ = std::make_shared<Media::MediaMuxer>(getuid(), getpid());
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_NO_MEMORY, "Create AVMuxer Engine failed");
    return StatusConvert(muxerEngine_->Init(fd, static_cast<Media::Plugin::OutputFormat>(format)));
}

int32_t AVMuxerImpl::SetRotation(int32_t rotation)
{
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    std::shared_ptr<Media::Meta> param = std::make_shared<Media::Meta>();
    param->Set<Media::Tag::VIDEO_ROTATION>(static_cast<Media::Plugin::VideoRotation>(rotation));
    return StatusConvert(muxerEngine_->SetParameter(param));
}

int32_t AVMuxerImpl::AddTrack(int32_t &trackIndex, const MediaDescription &trackDesc)
{
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    auto track = const_cast<MediaDescription&>(trackDesc);
    return StatusConvert(muxerEngine_->AddTrack(trackIndex, track.GetMeta()));
}

int32_t AVMuxerImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    return StatusConvert(muxerEngine_->Start());
}

int32_t AVMuxerImpl::WriteSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
    AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    CHECK_AND_RETURN_RET_LOG(sample != nullptr && info.offset >= 0 && info.size >= 0 &&
        sample->GetSize() >= (info.offset + info.size), AVCS_ERR_INVALID_VAL, "Invalid memory");
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(sample->GetBase() + info.offset,
        sample->GetSize(), info.size);
    buffer->pts_ = info.presentationTimeUs;
    buffer->flag_ = flag;
    return StatusConvert(muxerEngine_->WriteSample(trackIndex, buffer));
}

int32_t AVMuxerImpl::WriteSampleBuffer(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample)
{
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    return StatusConvert(muxerEngine_->WriteSample(trackIndex, sample));
}

int32_t AVMuxerImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    return StatusConvert(muxerEngine_->Stop());
}

int32_t AVMuxerImpl::StatusConvert(Media::Status status)
{
    const static std::unordered_map<Media::Status, int32_t> g_transTable = {
        {Media::Status::END_OF_STREAM, AVCodecServiceErrCode::AVCS_ERR_OK},
        {Media::Status::OK, AVCodecServiceErrCode::AVCS_ERR_OK},
        {Media::Status::NO_ERROR, AVCodecServiceErrCode::AVCS_ERR_OK},
        {Media::Status::ERROR_UNKNOWN, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Media::Status::ERROR_PLUGIN_ALREADY_EXISTS, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Media::Status::ERROR_INCOMPATIBLE_VERSION, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Media::Status::ERROR_NO_MEMORY, AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY},
        {Media::Status::ERROR_WRONG_STATE, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION},
        {Media::Status::ERROR_UNIMPLEMENTED, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT},
        {Media::Status::ERROR_INVALID_PARAMETER, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Media::Status::ERROR_INVALID_DATA, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Media::Status::ERROR_MISMATCHED_TYPE, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Media::Status::ERROR_TIMED_OUT, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Media::Status::ERROR_UNSUPPORTED_FORMAT, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT_CONTAINER_TYPE},
        {Media::Status::ERROR_NOT_ENOUGH_DATA, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Media::Status::ERROR_NOT_EXISTED, AVCodecServiceErrCode::AVCS_ERR_OPEN_FILE_FAILED},
        {Media::Status::ERROR_AGAIN, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Media::Status::ERROR_PERMISSION_DENIED, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Media::Status::ERROR_NULL_POINTER, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Media::Status::ERROR_INVALID_OPERATION, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION},
        {Media::Status::ERROR_CLIENT, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Media::Status::ERROR_SERVER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Media::Status::ERROR_DELAY_READY, AVCodecServiceErrCode::AVCS_ERR_OK},
    };
    auto ite = g_transTable.find(status);
    if (ite == g_transTable.end()) {
        return AVCS_ERR_UNKNOWN;
    }
    return ite->second;
}
} // namespace MediaAVCodec
} // namespace OHOS