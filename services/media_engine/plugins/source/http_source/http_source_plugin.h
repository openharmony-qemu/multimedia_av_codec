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
#ifndef HISTREAMER_HTTP_SOURCE_PLUGIN_H
#define HISTREAMER_HTTP_SOURCE_PLUGIN_H

#include <memory>
#include "media_downloader.h"
#include "meta/media_types.h"
#include "plugin/source_plugin.h"
#include "meta/media_types.h"
#include "download/http_curl_client.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HttpSourcePlugin : public SourcePlugin {
public:
    explicit HttpSourcePlugin(const std::string &name) noexcept;
    ~HttpSourcePlugin() override;
    Status Init() override;
    Status Deinit() override;
    Status Prepare() override;
    Status Reset() override;
    Status Start() override;
    Status Stop() override;
    Status GetParameter(std::shared_ptr<Meta> &meta) override;
    Status SetParameter(const std::shared_ptr<Meta> &meta) override;
    Status SetCallback(Callback* cb) override;
    Status SetSource(std::shared_ptr<MediaSource> source) override;
    Status Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen) override;
    Status GetSize(uint64_t& size) override;
    Seekable GetSeekable() override;
    Status SeekTo(uint64_t offset) override;
    Status SeekToTime(int64_t seekTime, SeekMode mode) override;
    Status GetDuration(int64_t& duration) override;
    bool IsSeekToTimeSupported() override;
    Status GetBitRates(std::vector<uint32_t>& bitRates) override;
    Status SelectBitRate(uint32_t bitRate) override;
    Status SetReadBlockingFlag(bool isReadBlockingAllowed) override;
    void SetDemuxerState() override;
    void SetDownloadErrorState() override;
    void SetInterruptState(bool isInterruptNeeded) override;

private:
    void CloseUri();

    uint32_t bufferSize_;
    uint32_t waterline_;
    uint32_t seekErrorCount_{0};
    Callback* callback_ {};
    std::shared_ptr<MediaDownloader> downloader_;
    Mutex mutex_ {};
    bool delayReady {true};
    std::string uri_ {};
    std::map<std::string, std::string> httpHeader_ {};
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif