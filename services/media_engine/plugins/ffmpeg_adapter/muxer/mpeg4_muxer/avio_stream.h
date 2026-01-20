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

#ifndef AVIO_STREAM_H
#define AVIO_STREAM_H

#include <memory>
#include <string>
#include "plugin/data_sink.h"

namespace OHOS {
namespace Media {
namespace Plugins {
class AVIOStream {
public:
    explicit AVIOStream(const std::shared_ptr<DataSink> &dataSink);
    AVIOStream(const AVIOStream& io);
    AVIOStream& operator=(const AVIOStream&) = delete;
    ~AVIOStream() = default;
    void Write(uint8_t data);
    void Write(uint16_t data, bool isBigEndian = true);
    void Write(uint32_t data, bool isBigEndian = true);
    void Write(uint64_t data, bool isBigEndian = true);
    void Write(std::string data);
    void Write(const char *data, int32_t size);
    void Write(const uint8_t* data, int32_t size);
    int32_t Read(uint8_t *data, int32_t size);
    void Seek(int64_t offset, int whence = SEEK_SET);
    int64_t GetPos();
    bool CanRead();

private:
    std::shared_ptr<DataSink> dataSink_ = nullptr;
    int64_t pos_ = 0;
    int64_t end_ = -1;
};
} // Plugins
} // Media
} // OHOS
#endif