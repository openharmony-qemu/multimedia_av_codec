/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef DEMUXER_DATA_READER_H
#define DEMUXER_DATA_READER_H

#include <vector>
#include "common/status.h"
#include "plugin/plugin_definition.h"

namespace OHOS {
namespace Media {
namespace Plugins {
class DemuxerDataReader {
public:
    Status SetDataReader(const std::shared_ptr<DataSource>& source);
    Status SetReadTimeoutForInitSeek(int64_t timeoutMs);
    Status ReadUintData(int64_t offset, uint8_t* buffer, size_t size);
    Status ReadFully(int64_t offset, uint8_t* buffer, size_t size);
    void NotifyInitializationCompleted();
private:
    bool IsReadTimeoutExpired();
    Status ReadFromCache(int64_t offset, uint8_t* buffer, size_t size);
    Status FillReadCache(int64_t offset);
    void UpdateInitDownloadData(size_t dataSize);

    std::shared_ptr<DataSource> dataSource_ {nullptr};
    bool readTimeoutConfigured_ {false};
    int64_t readTimeoutMs_ {0};
    int64_t readStartTimeMs_ {0};
    std::vector<uint8_t> readCache_;
    int64_t readCacheOffset_ {-1};
    size_t readCacheSize_ {0};
    uint64_t initDownloadDataSize_ {0};
    bool initCompleted_ {false};
};

class DemuxerBitReader {
public:
    DemuxerBitReader(const uint8_t* data, size_t size)
        : data_(data), byteOffset_(0), bitOffset_(0), totalBytes_(size) {}
    uint16_t ShowBits(uint8_t numBits);
    uint8_t ReadBits(uint8_t numBits);
    bool HasBits(size_t numBits) const;
    bool SkipBits(uint8_t numBits);
private:
    const uint8_t* data_; // Pointer to the raw data buffer
    size_t byteOffset_;   // Current byte offset indicating the position within the buffer
    uint8_t bitOffset_;   // Current bit offset within the byte (0-7)
    size_t totalBytes_;   // Total number of bytes
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // DEMUXER_LOG_COMPRESSOR_H
