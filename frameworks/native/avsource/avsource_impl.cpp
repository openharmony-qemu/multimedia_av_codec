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
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include "avcodec_common.h"
#include "i_avcodec_service.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_dfx.h"
#include "avsource_impl.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVSourceImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<AVSource> AVSourceFactory::CreateWithURI(const std::string &uri)
{
    AVCodecTrace trace("AVSourceFactory::CreateWithURI");
    
    AVCODEC_LOGI("create source with uri: uri=%{private}s", uri.c_str());

    std::shared_ptr<AVSourceImpl> sourceImpl = std::make_shared<AVSourceImpl>();
    CHECK_AND_RETURN_RET_LOG(sourceImpl != nullptr, nullptr, "New AVSourceImpl failed");

    int32_t ret = sourceImpl->InitWithURI(uri);

    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init AVSourceImpl failed");

    return sourceImpl;
}

std::shared_ptr<AVSource> AVSourceFactory::CreateWithFD(int32_t fd, int64_t offset, int64_t size)
{
    AVCodecTrace trace("AVSourceFactory::CreateWithFD");

    AVCODEC_LOGI("create source with fd: fd=%{private}d, offset=%{public}" PRId64 ", size=%{public}" PRId64,
        fd, offset, size);

    std::shared_ptr<AVSourceImpl> sourceImpl = std::make_shared<AVSourceImpl>();
    CHECK_AND_RETURN_RET_LOG(sourceImpl != nullptr, nullptr, "New AVSourceImpl failed");

    int32_t ret = sourceImpl->InitWithFD(fd, offset, size);

    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init AVSourceImpl failed");

    return sourceImpl;
}

int32_t AVSourceImpl::InitWithURI(const std::string &uri)
{
    AVCodecTrace trace("AVSource::InitWithURI");

    sourceEngine_ = ISourceEngineFactory::CreateSourceEngine(uri);
    CHECK_AND_RETURN_RET_LOG(sourceEngine_ != nullptr,  AVCS_ERR_INVALID_OPERATION, "Create source engine failed");

    int32_t ret = sourceEngine_->Init();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK,  AVCS_ERR_INVALID_OPERATION, "Init source engine failed");

    ret = sourceEngine_->GetTrackCount(trackCount_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK,  AVCS_ERR_INVALID_OPERATION, "Get track count failed");

    sourceUri = uri;
    return AVCS_ERR_OK;
}

int32_t AVSourceImpl::InitWithFD(int32_t fd, int64_t offset, int64_t size)
{
    AVCodecTrace trace("AVSource::InitWithFD");

    CHECK_AND_RETURN_RET_LOG(fd > STDERR_FILENO, AVCS_ERR_INVALID_VAL,
        "Create source with uri failed because input fd is illegal, fd must be greater than 2!");
    CHECK_AND_RETURN_RET_LOG(offset >= 0, AVCS_ERR_INVALID_VAL,
        "Create source with fd failed because input offset is negative");
    CHECK_AND_RETURN_RET_LOG(size > 0, AVCS_ERR_INVALID_VAL,
        "Create source with fd failed because input size must be greater than zero");
    int32_t flag = fcntl(fd, F_GETFL, 0);
    CHECK_AND_RETURN_RET_LOG(flag >= 0, AVCS_ERR_INVALID_VAL, "get fd status failed");
    CHECK_AND_RETURN_RET_LOG(
        (static_cast<uint32_t>(flag) & static_cast<uint32_t>(O_WRONLY)) != static_cast<uint32_t>(O_WRONLY),
        AVCS_ERR_INVALID_VAL, "Fd not be permitted to read ");
    CHECK_AND_RETURN_RET_LOG(lseek(fd, 0, SEEK_CUR) != -1, AVCS_ERR_INVALID_VAL, "Fd is not seekable");
    
    std::string uri = "fd://" + std::to_string(fd) + "?offset=" + \
        std::to_string(offset) + "&size=" + std::to_string(size);

    return InitWithURI(uri);
}

AVSourceImpl::AVSourceImpl()
{
    AVCODEC_LOGD("AVSourceImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVSourceImpl::~AVSourceImpl()
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGI("Destroy AVSourceImpl for source %{private}s", sourceUri.c_str());
    if (sourceEngine_ != nullptr) {
        sourceEngine_ = nullptr;
    }
    AVCODEC_LOGD("AVSourceImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVSourceImpl::GetSourceAddr(uintptr_t &addr)
{
    CHECK_AND_RETURN_RET_LOG(sourceEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Source engine does not exist!");
    addr = sourceEngine_->GetSourceAddr();
    return AVCS_ERR_OK;
}

int32_t AVSourceImpl::GetSourceFormat(Format &format)
{
    AVCodecTrace trace("AVSource::GetSourceFormat");

    CHECK_AND_RETURN_RET_LOG(sourceEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Source engine does not exist!");
    
    return sourceEngine_->GetSourceFormat(format);
}

int32_t AVSourceImpl::GetTrackFormat(Format &format, uint32_t trackIndex)
{
    AVCodecTrace trace("AVSource::GetTrackFormat");

    AVCODEC_LOGI("get track format: trackIndex=%{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(sourceEngine_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Source engine does not exist!");

    bool isValid = (trackIndex < trackCount_);
    CHECK_AND_RETURN_RET_LOG(isValid, AVCS_ERR_INVALID_VAL, "track index is invalid!");

    return sourceEngine_->GetTrackFormat(format, trackIndex);
}
} // namespace MediaAVCodec
} // namespace OHOS