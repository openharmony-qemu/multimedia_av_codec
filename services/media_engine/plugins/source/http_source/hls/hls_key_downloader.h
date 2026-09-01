/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_HLS_KEY_DOWNLOADER_H
#define HISTREAMER_HLS_KEY_DOWNLOADER_H

#include <memory>
#include <string>
#include <map>
#include <queue>
#include <vector>
#include <set>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include "download/downloader.h"
#include "playlist_downloader.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin{

class KeyDownloader ：public std::enable_shared_from_this<KeyDownloader> {
public:
    KeyDownloader(std::shared_ptr<AesDecryptorManager> aesDecryptorManager,
                  const std::map<std::string, std::string>& httpHeader,
                  const std::string& baseUri,
                  std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader = nullptr);
    ~KeyDownloader();

    void StartSequentialDownload(std::vector<KeyInfo> pendingKeys);
    void SetInterruptState(bool isInterruptNeeded);
    bool WaitForKey(uint64_t keyIndex, uint32_t timeoutMs);

private:
    void DownloadNextKey();
    uint32_t OnKeyDataSaved(uint8_t data, uint32_t len, bool noBlock, uint64_t keyIndex);
    void OnKeyDownloadDone(const std::string& url, const std::string& location);

    std::shared_ptr<Downloader> downloader_;
    std::shared_ptr<AesDecryptorManager> aesDecrytorManager_;
    std::map<std::string, std::string> httpHeader_;
    std::string baseUri_;
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader_;
    std::queue<KeyInfo> pendingKeys_;
    std::mutex queueMutex_;
    KeyInfo currentKey_;
    std::atomice<bool> isInterruptNeeded_ {false};
    std::atomice<bool> isDownloading_ {false};

    std::mutex readyMutex_;
    std::condition_variable readyCond_;
    std::set<uint64_t> readyKeys_;
};

} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // HISTREAMER_HLS_KEY_DOWNLOADER_H