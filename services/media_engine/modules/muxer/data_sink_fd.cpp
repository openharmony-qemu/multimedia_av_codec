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

#define HST_LOG_TAG "DataSinkFd"

#include "data_sink_fd.h"
#include <fcntl.h>
#include <unistd.h>
#include "common/log.h"

namespace OHOS {
namespace Media {
DataSinkFd::DataSinkFd(int32_t fd) : fd_(dup(fd)), pos_(0), end_(-1)
{
    MEDIA_LOG_D("dup fd is %{public}d", fd_);
    end_ = lseek(fd_, 0, SEEK_END);
    if (lseek(fd_, 0, SEEK_SET) < 0) {
        MEDIA_LOG_E("failed to construct, fd is %{public}d, error is %{public}s", fd_, strerror(errno));
    }
    uint32_t fdPermission = static_cast<uint32_t>(fcntl(fd_, F_GETFL, 0));
    isCanRead_ = (fdPermission & O_RDWR) == O_RDWR;
}

DataSinkFd::~DataSinkFd()
{
    if (fd_ > 0) {
        MEDIA_LOG_D("close fd is %{public}d", fd_);
        close(fd_);
        fd_ = -1;
    }
}

int32_t DataSinkFd::Read(uint8_t *buf, int32_t bufSize)
{
    FALSE_RETURN_V_MSG_E(fd_ > 0, -1, "failed to read, fd is  %{public}d", fd_);
    if (pos_ >= end_) {
        return 0;
    }
    FALSE_RETURN_V_MSG_E(lseek(fd_, pos_, SEEK_SET) >= 0, -1, "failed to seek, %{public}s", strerror(errno));
    int32_t size = read(fd_, buf, bufSize);
    FALSE_RETURN_V_MSG_E(size >= 0, -1, "failed to read, %{public}s", strerror(errno));
    pos_ = pos_ + size;
    return size;
}

int32_t DataSinkFd::Write(const uint8_t *buf, int32_t bufSize)
{
    FALSE_RETURN_V_MSG_E(fd_ > 0, -1, "failed to write, fd is  %{public}d", fd_);
    FALSE_RETURN_V_MSG_E(lseek(fd_, pos_, SEEK_SET) >= 0, -1, "failed to seek, %{public}s", strerror(errno));
    int32_t size = write(fd_, buf, bufSize);
    FALSE_RETURN_V_MSG_E(size == bufSize, -1, "failed to write, %{public}s", strerror(errno));
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

int64_t DataSinkFd::GetCurrentPosition() const
{
    return pos_;
}

bool DataSinkFd::CanRead()
{
    return isCanRead_;
}
} // namespace Media
} // namespace OHOS