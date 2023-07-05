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

    std::shared_ptr<AVMuxerImpl> impl = std::make_shared<AVMuxerImpl>(fd, format);

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init avmuxer implementation failed");
    return impl;
}

AVMuxerImpl::AVMuxerImpl(int32_t fd, OutputFormat format) : fd_(fd), format_(format)
{
    (void)fd_;
    (void)format_;
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

int32_t AVMuxerImpl::Init()
{
    muxerEngine_ = IMuxerEngineFactory::CreateMuxerEngine(getuid(), getpid(), fd_, format_);
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_NO_MEMORY, "Create AVMuxer Engine failed");
    return AVCS_ERR_OK;
}

int32_t AVMuxerImpl::SetRotation(int32_t rotation)
{
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    return muxerEngine_->SetRotation(rotation);
}

int32_t AVMuxerImpl::AddTrack(int32_t &trackIndex, const MediaDescription &trackDesc)
{
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    return muxerEngine_->AddTrack(trackIndex, trackDesc);
}

int32_t AVMuxerImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    return muxerEngine_->Start();
}

int32_t AVMuxerImpl::WriteSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
    AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    CHECK_AND_RETURN_RET_LOG(sample != nullptr && info.offset >= 0 && info.size >= 0 &&
        sample->GetSize() >= (info.offset + info.size), AVCS_ERR_INVALID_VAL, "Invalid memory");
    return muxerEngine_->WriteSample(trackIndex, sample, info, flag);
}

int32_t AVMuxerImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "AVMuxer Engine does not exist");
    return muxerEngine_->Stop();
}
} // namespace MediaAVCodec
} // namespace OHOS