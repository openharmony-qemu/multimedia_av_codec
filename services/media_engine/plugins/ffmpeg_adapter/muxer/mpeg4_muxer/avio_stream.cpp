/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "avio_stream.h"
#include <algorithm>
#include "common/log.h"

#ifndef _WIN32
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_MUXER, "AVIOStream"};
}
#endif // !_WIN32

namespace OHOS {
namespace Media {
namespace Plugins {
AVIOStream::AVIOStream(const std::shared_ptr<DataSink> &dataSink) : dataSink_(dataSink)
{
    if (dataSink_ != nullptr) {
        end_ = dataSink_->Seek(0ll, SEEK_END);
        dataSink_->Seek(0ll, SEEK_SET);
    }
}

AVIOStream::AVIOStream(const AVIOStream& io)
{
    dataSink_ = io.dataSink_;
    pos_ = 0;
    end_ = io.end_;
}

void AVIOStream::Write(uint8_t data)
{
    Write(&data, 1);
}

void AVIOStream::Write(uint16_t data, bool isBigEndian)
{
    uint8_t buffer[2] = { 0 };  // 2
    if (isBigEndian) {
        buffer[0] = ((data >> 8) & 0xFF);  // 8
        buffer[1] = data & 0xFF;
    } else {
        buffer[0] = data & 0xFF;
        buffer[1] = ((data >> 8) & 0xFF);  // 8
    }
    Write(buffer, 2);  // 2
}

void AVIOStream::Write(uint32_t data, bool isBigEndian)
{
    uint8_t buffer[4] = { 0 };  // 4
    if (isBigEndian) {
        buffer[0] = ((data >> 24) & 0xFF);  // 24b
        buffer[1] = ((data >> 16) & 0xFF);  // 16b
        buffer[2] = ((data >> 8) & 0xFF);   // 8b, index 2
        buffer[3] = data & 0xFF;  // index 3
    } else {
        buffer[3] = ((data >> 24) & 0xFF);  // 24b, index 3
        buffer[2] = ((data >> 16) & 0xFF);  // 16b, index 2
        buffer[1] = ((data >> 8) & 0xFF);   // 8b
        buffer[0] = data & 0xFF;
    }
    Write(buffer, 4);  // 4
}

void AVIOStream::Write(uint64_t data, bool isBigEndian)
{
    uint8_t buffer[8] = { 0 };
    if (isBigEndian) {
        buffer[0] = ((data >> 56) & 0xFF);  // 56b
        buffer[1] = ((data >> 48) & 0xFF);  // 48b, index 1
        buffer[2] = ((data >> 40) & 0xFF);  // 40b, index2
        buffer[3] = ((data >> 32) & 0xFF);  // 32b, index3
        buffer[4] = ((data >> 24) & 0xFF);  // 24b, index4
        buffer[5] = ((data >> 16) & 0xFF);  // 16b, index5
        buffer[6] = ((data >> 8) & 0xFF);   // 8b,  index6
        buffer[7] = data & 0xFF;  // index7
    } else {
        buffer[7] = ((data >> 56) & 0xFF);  // 56b, index 7
        buffer[6] = ((data >> 48) & 0xFF);  // 48b, index 6
        buffer[5] = ((data >> 40) & 0xFF);  // 40b, index 5
        buffer[4] = ((data >> 32) & 0xFF);  // 32b, index 4
        buffer[3] = ((data >> 24) & 0xFF);  // 24b, index 3
        buffer[2] = ((data >> 16) & 0xFF);  // 16b, index 2
        buffer[1] = ((data >> 8) & 0xFF);   // 8b
        buffer[0] = data & 0xFF;
    }
    Write(buffer, 8);  // 8
}

void AVIOStream::Write(std::string data)
{
    const uint8_t* buffer = reinterpret_cast<const uint8_t*>(data.data());
    int32_t size = static_cast<int32_t>(data.length() + 1);
    Write(buffer, size);
}

void AVIOStream::Write(const char *data, int32_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    const uint8_t* buffer = reinterpret_cast<const uint8_t*>(data);
    Write(buffer, size);
}

void AVIOStream::Write(const uint8_t* data, int32_t size)
{
    FALSE_RETURN_MSG((data != nullptr && size > 0), "failed to write, data is empty.");
    pos_ = dataSink_->Seek(pos_, SEEK_SET);
    int32_t writeSize = dataSink_->Write(data, size);
    if (writeSize > 0) {
        pos_ += writeSize;
        end_ = pos_ > end_ ? pos_ : end_;
    }
}

int32_t AVIOStream::Read(uint8_t *data, int32_t size)
{
    FALSE_RETURN_V_MSG_E(dataSink_ != nullptr, -1, "failed to get pos, dataSink is nullptr.");
    if (pos_ >= end_) {
        return 0;
    }
    pos_ = dataSink_->Seek(pos_, SEEK_SET);
    int32_t readSize = dataSink_->Read(data, size);
    if (readSize > 0) {
        pos_ += readSize;
    }
    return readSize;
}

void AVIOStream::Seek(int64_t offset, int32_t whence)
{
    FALSE_RETURN_MSG(dataSink_ != nullptr, "failed to seek, dataSink is nullptr.");
    if (whence == SEEK_SET) {
        pos_ = offset;
    } else if (whence == SEEK_CUR) {
        pos_ = pos_ + offset;
    } else if (whence == SEEK_END) {
        pos_ = end_ + offset;
    } else {
        MEDIA_LOG_E("failed to seek whence %{public}d.", whence);
        return;
    }
    pos_ = dataSink_->Seek(pos_, SEEK_SET);
}

int64_t AVIOStream::GetPos()
{
    FALSE_RETURN_V_MSG_E(dataSink_ != nullptr, -1, "failed to get pos, dataSink is nullptr.");
    return pos_;
}

bool AVIOStream::CanRead()
{
    FALSE_RETURN_V_MSG_E(dataSink_ != nullptr, false, "dataSink is nullptr.");
    return dataSink_->CanRead();
}
} // Plugins
} // Media
} // OHOS