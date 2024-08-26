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

#ifndef HISTREAMER_FILE_FD_SOURCE_PLUGIN_H
#define HISTREAMER_FILE_FD_SOURCE_PLUGIN_H

#include <cstdio>
#include <string>
#include "plugin/source_plugin.h"
#include "plugin/plugin_buffer.h"
#include "osal/utils/ring_buffer.h"
#include "osal/task/task.h"
#include <mutex>
#include <shared_mutex>
#include "osal/utils/steady_clock.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace FileFdSource {
struct HmdfsHasCache {
    int64_t offset;
    int64_t readSize;
};

class FileFdSourcePlugin : public SourcePlugin {
public:
    explicit FileFdSourcePlugin(std::string name);
    ~FileFdSourcePlugin();
    Status SetCallback(Callback* cb) override;
    Status SetSource(std::shared_ptr<MediaSource> source) override;
    Status Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen) override;
    Status Read(int32_t streamId, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen) override;
    Status GetSize(uint64_t& size) override;
    Seekable GetSeekable() override;
    Status SeekTo(uint64_t offset) override;
    Status Reset() override;
    Status Stop() override;
    void SetDemuxerState(int32_t streamId) override;
    void SetBundleName(const std::string& bundleName) override;
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID) override;
    Status SetReadBlockingFlag(bool isAllowed) override;
    void SetInterruptState(bool isInterruptNeeded) override;
    void NotifyBufferingStart();
    void NotifyBufferingPercent();
    void NotifyBufferingEnd();
    void NotifyReadFail();
private:
    Status ParseUriInfo(const std::string& uri);
    Status ReadOfflineFile(int32_t streamId, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen);
    Status ReadOnlineFile(int32_t streamId, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen);
    Status SeekToOfflineFile(uint64_t offset);
    Status SeekToOnlineFile(uint64_t offset);
    void CacheDataLoop();
    void HasCacheData(size_t bufferSize);
    void HandleReadResult(size_t bufferSize, int size);
    std::shared_ptr<Memory> GetBufferPtr(std::shared_ptr<Buffer>& buffer, size_t expectedLen);

    void CacheData();
    bool HandleBuffering();
    void PauseDownloadTask(bool isAsync);
    inline int64_t GetLastSize(uint64_t position);
    void CheckFileType();
    void GetCurrentSpeed(int64_t curTime);
    float GetCacheTime(float num);
    void UpdateWaterLineAbove();
    void DeleteCacheBuffer(char* buffer, size_t bufferSize);
    void CheckReadTime();
    bool IsValidTime(int64_t curTime, int64_t lastTime);
    
    int32_t fd_ {-1};
    int64_t offset_ {0};
    uint64_t size_ {0};
    uint64_t fileSize_ {0};
    Seekable seekable_ {Seekable::SEEKABLE};
    std::atomic<uint64_t> position_ {0};
    Callback* callback_ {nullptr};
    std::atomic<bool> isBuffering_ {false};
    std::atomic<bool> isInterrupted_ {false};
    std::atomic<bool> isReadBlocking_ {true};
    std::atomic<bool> inSeek_ {false};
    std::shared_ptr<Task> downloadTask_;
    std::shared_mutex mutex_;
    bool isReadFrame_ {false};
    bool isSeekHit_ {false};
    std::atomic<uint64_t> cachePosition_ {0};
    int64_t waterLineAbove_ {0};
    bool isCloudFile_ {false};
    std::shared_ptr<RingBuffer> ringBuffer_;

    int64_t ringBufferSize_ {0};
    uint64_t downloadSize_ {0};
    SteadyClock steadyClock_;
    SteadyClock steadyClock2_;
    int64_t lastCheckTime_ {0};
    int32_t currentBitRate_ {0};
    float avgDownloadSpeed_ {0};
    int64_t curReadTime_ {0};
    int64_t retryTimes_ {0};
    int64_t lastReadTime_ {0};
};
} // namespace FileSource
} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // HISTREAMER_FILE_FD_SOURCE_PLUGIN_H