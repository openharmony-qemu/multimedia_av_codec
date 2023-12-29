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

#define HST_LOG_TAG "DataSinkFile"

#include "data_sink_file.h"
#include <fcntl.h>
#include "common/log.h"

namespace OHOS {
namespace Media {
DataSinkFile::DataSinkFile(FILE *file) : file_(file), pos_(0), end_(-1), isCanRead_(true)
{
    end_ = fseek(file_, 0L, SEEK_END);
    if (fseek(file_, 0L, SEEK_SET) < 0) {
        MEDIA_LOG_E("failed to construct, file is  %{public}p, error is %{public}s", file_, strerror(errno));
    }
}

DataSinkFile::~DataSinkFile()
{
    if (file_ != nullptr) {
    }
}

int32_t DataSinkFile::Read(uint8_t *buf, int32_t bufSize)
{
    FALSE_RETURN_V_MSG_E(file_ != nullptr, -1, "failed to read, file is  %{public}p", file_);
    if (pos_ >= end_) {
        return 0;
    }

    FALSE_RETURN_V_MSG_E(fseek(file_, pos_, SEEK_SET) >= 0, -1, "failed to seek, %{public}s", strerror(errno));
    int32_t size = fread(buf, 1, bufSize, file_);
    FALSE_RETURN_V_MSG_E(size >= 0, -1, "failed to read, %{public}s", strerror(errno));

    pos_ = pos_ + size;
    return size;
}

int32_t DataSinkFile::Write(const uint8_t *buf, int32_t bufSize)
{
    FALSE_RETURN_V_MSG_E(file_ != nullptr, -1, "failed to read, file is  %{public}p", file_);

    FALSE_RETURN_V_MSG_E(fseek(file_, pos_, SEEK_SET) >= 0, -1, "failed to seek, %{public}s", strerror(errno));
    int32_t size = fwrite(buf, 1, bufSize, file_);
    FALSE_RETURN_V_MSG_E(size == bufSize, -1, "failed to write, %{public}s", strerror(errno));

    pos_ = pos_ + size;
    end_ = pos_ > end_ ? pos_ : end_;
    return size;
}

int64_t DataSinkFile::Seek(int64_t offset, int whence)
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

int64_t DataSinkFile::GetCurrentPosition() const
{
    return pos_;
}

bool DataSinkFile::CanRead()
{
    return isCanRead_;
}
} // namespace Media
} // namespace OHOS