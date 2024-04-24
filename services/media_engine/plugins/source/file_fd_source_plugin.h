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
#include "osal/task/task.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace FileFdSource {
class FileFdSourcePlugin : public SourcePlugin {
public:
    explicit FileFdSourcePlugin(std::string name);
    ~FileFdSourcePlugin() = default;
    Status SetCallback(Callback* cb) override;
    Status SetSource(std::shared_ptr<MediaSource> source) override;
    Status Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen) override;
    Status GetSize(uint64_t& size) override;
    Seekable GetSeekable() override;
    Status SeekTo(uint64_t offset) override;
    Status Reset() override;
    void SetDemuxerState() override;
    void SetBundleName(const std::string& bundleName) override;
    void SubmitBufferingStart();
    void SubmitReadFail();
private:
    Status ParseUriInfo(const std::string& uri);
    int64_t ReadTimer();
    void CacheData();
    void StartTimerTask();
    void PauseTimerTask();
    void HandleReadFail();
    bool HandleBuffering();
    void PauseDownloadTask(bool isAsync);

    int32_t fd_ {-1};
    int64_t offset_ {0};
    uint64_t size_ {0};

    uint64_t fileSize_ {0};
    Seekable seekable_ {Seekable::SEEKABLE};
    uint64_t position_ {0};
    Callback* callback_ {nullptr};
    bool isTaskCallback_ {false};
    std::atomic<bool> isBuffering_ {false};
    uint64_t readTime_ {0};
    std::shared_ptr<Task> timerTask_;
    std::shared_ptr<Task> downloadTask_;

    bool isReadFrame_ {false};
    std::string bundleName_ {};
    bool isReadSuccess_ {false};
};
} // namespace FileSource
} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // HISTREAMER_FILE_FD_SOURCE_PLUGIN_H