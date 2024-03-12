/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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
 
#ifndef HISTREAMER_HTTP_MEDIA_DOWNLOADER_H
#define HISTREAMER_HTTP_MEDIA_DOWNLOADER_H

#include <string>
#include <memory>
#include "osal/utils/ring_buffer.h"
#include "download/downloader.h"
#include "media_downloader.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HttpMediaDownloader : public MediaDownloader {
public:
    HttpMediaDownloader() noexcept;
    ~HttpMediaDownloader() override;
    bool Open(const std::string& url) override;
    void Close(bool isAsync) override;
    void Pause() override;
    void Resume() override;
    bool Read(unsigned char* buff, unsigned int wantReadLength, unsigned int& realReadLength, bool& isEos) override;
    bool SeekToPos(int64_t offset) override;
    size_t GetContentLength() const override;
    int64_t GetDuration() const override;
    Seekable GetSeekable() const override;
    void SetCallback(Callback* cb) override;
    void SetStatusCallback(StatusCallbackFunc cb) override;
    bool GetStartedStatus() override;
    void SetReadBlockingFlag(bool isReadBlockingAllowed) override;

private:
    bool SaveData(uint8_t* data, uint32_t len);

private:
    std::shared_ptr<RingBuffer> buffer_;
    std::shared_ptr<Downloader> downloader_;
    std::shared_ptr<DownloadRequest> downloadRequest_;
    Mutex mutex_;
    ConditionVariable cvReadWrite_;
    Callback* callback_ {nullptr};
    StatusCallbackFunc statusCallback_ {nullptr};
    bool aboveWaterline_ {false};
    bool startedPlayStatus_ {false};

    // how many microseconds to play in ring buffer.
    uint64_t bufferedDuration_ {0};
    // download rate
    // 最近的下载速率和缓存时长记录
    struct BufferDownRecord
    {
        /* data */
        uint32_t dataBits {0};
        int64_t timeoff {0};
        BufferDownRecord* next {nullptr};
    };

    BufferDownRecord* bufferDownRecord_ {nullptr};
    // buffer least
    // 最近的缓存低谷记录
    struct BufferLeastRecord
    {
        uint64_t minDuration {0};
        BufferLeastRecord* next {nullptr};
    };

    BufferLeastRecord* bufferLeastRecord_ {nullptr};
    int64_t lastWriteTime_ {0};
    int64_t lastReadTime_ {0};
    int64_t lastWriteBit_ {0};
    SteadyClock steadyClock_;

    // 下载指标打点
    std::shared_ptr<OSAL::Task> downloadReportTask_;
    // 总下载量
    uint64_t totalBits_ {0};
    // 上一统计周期的总下载量
    uint64_t lastBits_ {0};
    // 累计有效下载时长 ms
    uint32_t downloadDuringTime_ {0};
    // 累计有效时间内下载数据量 bit
    uint64_t downloadBits_ {0};
    // 全量下载速率和缓存时长记录
    struct RecordData
    {
        double downloadRate {0};
        int32_t bufferDuring {0};
        std::shared_ptr<RecordData> next {nullptr};
    };
    RecordData* recordData_ {nullptr};
    // 缓存区大小切换
    // 当前缓存区容量（切换中以切换后的缓存区容量计）
    size_t totalRingBufferSize_ {0};
    // 是否正在切换缓存区
    bool usingExtraRingBuffer_ {false};
    std::shared_ptr<RingBuffer> tmpBuffer_;
};
}
}
}
}
#endif