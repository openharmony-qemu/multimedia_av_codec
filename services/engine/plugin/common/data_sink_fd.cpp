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

#include "data_sink_fd.h"
#include <fcntl.h>
#include <unistd.h>
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "DataSinkFd"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
DataSinkFd::DataSinkFd(int32_t fd) : fd_(dup(fd)), pos_(0), end_(-1)
{
    AVCODEC_LOGD("dup fd is %{public}d", fd_);
    end_ = lseek(fd_, 0, SEEK_END);
    if (lseek(fd_, 0, SEEK_SET) < 0) {
        AVCODEC_LOGE("failed to construct, fd is %{public}d, error is %{public}s", fd_, strerror(errno));
    }
}

DataSinkFd::~DataSinkFd()
{
    if (fd_ > 0) {
        AVCODEC_LOGD("close fd is %{public}d", fd_);
        close(fd_);
        fd_ = -1;
    }
}

int32_t DataSinkFd::Read(uint8_t *buf, int32_t bufSize)
{
    CHECK_AND_RETURN_RET_LOG(fd_ > 0, -1, "failed to read, fd is  %{public}d", fd_);
    if (pos_ >= end_) {
        return 0;
    }
    CHECK_AND_RETURN_RET_LOG(lseek(fd_, pos_, SEEK_SET) >= 0, -1, "failed to seek, %{public}s", strerror(errno));
    int32_t size = read(fd_, buf, bufSize);
    CHECK_AND_RETURN_RET_LOG(size >= 0, -1, "failed to read, %{public}s", strerror(errno));
    pos_ = pos_ + size;
    return size;
}

int32_t DataSinkFd::Write(uint8_t *buf, int32_t bufSize)
{
    CHECK_AND_RETURN_RET_LOG(fd_ > 0, -1, "failed to write, fd is  %{public}d", fd_);
    CHECK_AND_RETURN_RET_LOG(lseek(fd_, pos_, SEEK_SET) >= 0, -1, "failed to seek, %{public}s", strerror(errno));
    int32_t size = write(fd_, buf, bufSize);
    CHECK_AND_RETURN_RET_LOG(size == bufSize, -1, "failed to write, %{public}s", strerror(errno));
    pos_ = pos_ + size;
    end_ = pos_ > end_ ? pos_ : end_;
    return size;
}

int64_t DataSinkFd::Seek(int64_t offset, int whence)
{
    switch (whence) {
        case SEEK_SET:
            pos_ = offset;
            break;
        case SEEK_CUR:
            pos_ = pos_ + offset;
            break;
        case SEEK_END:
            pos_ = end_ + offset;
            break;
        default:
            pos_ = offset;
            break;
    }
    return pos_;
}

int64_t DataSinkFd::GetPos() const
{
    return pos_;
}
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS