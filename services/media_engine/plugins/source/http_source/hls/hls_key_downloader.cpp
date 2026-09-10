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
#define HST_LOG_TAG "HlsKeyDownloader"

#include "hls_key_downloader.h"
#include "m3u8.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

constexpr uint32_t MAX_KEY_LEN = 16;
constexpr int32_t KEY_DOWNLOAD_TIMEOUT_MS = 10000;

KeyDownloader::KeyDownloader(std::shared_ptr<AesDecryptorManager> aesDecryptorManager,
                             const std::map<std::string, std::string>& httpHeader,
                             const std::string& baseUri,
                             std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader)
    : aesDecryptorManager_(std::move(aesDecryptorManager)),
      httpHeader_(httpHeader),
      baseUri_(baseUri),
      sourceLoader_(std::move(sourceLoader))
{
}

KeyDownloader::~KeyDownloader()
{
    SetInterruptState(true);
}

void KeyDownloader::StartSequentialDownload(std::vector<KeyInfo> pendingKeys)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        for (auto& keyInfo : pendingKeys) {
            pendingKeys_.push(std::move(keyInfo));
        }
        if (pendingKeys_.empty()) {
        return;
    }
    }

    if (downloader_ == nullptr) {
        if (sourceLoader_ != nullptr) {
            downloader_ = std::make_shared<Downloader>("hlsSeqKey", sourceLoader_);
        } else {
            downloader_ = std::make_shared<Downloader>("hlsSeqKey");
        }
        if (downloader_ != nullptr) {
            downloader_->Init();
        }
    }

    // Start downloading if not already in progress
    if (!isDownloading_.load()) {
        DownloadNextKey();
    }
}

void KeyDownloader::DownloadNextKey()
{
    KeyInfo keyInfo;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (pendingKeys_.empty()) {
            isDownloading_.store(false);
            MEDIA_LOG_I("All key downloads completed");
            return;
        }
        keyInfo = pendingKeys_.front();
        pendingKeys_.pop();
        currentKey_ = keyInfo;
    }

    if (isInterruptNeeded_.load()) {
        isDownloading_.store(false);
        return;
    }

    isDownloading_.store(true);
    SubmitKeyDownload(keyInfo);
}

void KeyDownloader::SubmitKeyDownload(const KeyInfo& keyInfo)
{
    auto weakSelf = weak_from_this();

    auto keyDataSave = [weakSelf](uint8_t *data, uint32_t len, bool notBlock,
        uint64_t keyIndex) -> uint32_t {
        auto self = weakSelf.lock();
        FALSE_RETURN_V_MSG_E(self != nullptr, 0u, "KeyDownloader destructed");
        return self->OnKeyDataSaved(data, len, notBlock, keyIndex);
    };

    auto downloadDoneCb = [weakSelf](const std::string& url, const std::string& location) {
        auto self = weakSelf.lock();
        if (self != nullptr) {
            self->OnKeyDownloadDone(url, location);
        }
    };

    // Status callback
    auto statusCallback = [weakSelf](DownloadStatus status, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) -> bool {
        if (status == DownloadStatus::PARTTAL_DOWNLOAD) {
            auto self = weakSelf.lock();
            if (self != nullptr) {
                MEDIA_LOG_E("Key download failed, continuing to next key");
                self->OnKeyDownloadDone("", "");
            }
        }
        return true;
    };

    std::string realKeyUrl = UriJoin(baseUri_, keyInfo.uri_);
    RequestInfo requestInfo;
    requestInfo.url = realKeyUrl;
    requestInfo.httpHeader = httpHeader_;
    requestInfo.timeoutMs = KEY_DOWNLOAD_TIMEOUT_MS;

    auto downloadRequest = std::make_shared<DownloadRequest>(
        keyInfo.index_, keyDataSave, statusCallback, requestInfo, true);
    downloadRequest->SetIsAuthRequest(true);
    downloadRequest->SetRequestType(RequestType::HLS_KEY);
    downloadRequest->SetRequestProtocolType(RequestProtocolType::HLS);
    downloadRequest->SetDownloadDoneCb(downloadDoneCb);

    MEDIA_LOG_I("Start downloading key, index: " PUBLIC_LOG_U64 ", url: " PUBLIC_LOG_S,
        keyInfo.index_, realKeyUrl.c_str());

    downloader_->Download(downloadRequest, -1);
    downloader_->Start();
}

uint32_t KeyDownloader::OnKeyDataSaved(uint8_t* data, uint32_t len, bool notBlock, uint64_t keyIndex)
{
    FALSE_RETURN_V_MSG_E(data != nullptr, 0u, "key data is nullptr");
    if (len > 0 && currentKey_.keyLen_ + len <= MAX_KEY_LEN) {
        NZERO_RETURN_V(memcpy_s(currentKey_.key_ + currentKey_.keyLen_,
            sizeof(currentKey_.key_) - currentKey_.keyLen_, data, len), 0);
        currentKey_.keyLen_ += len;
        // Create decryptor and mark ready only when full key is accumulated
        if (currentKey_.keyLen_ == MAX_KEY_LEN) {
            if (aesDecryptorManager_ != nullptr) {
            aesDecryptorManager_->CreateOneAesDecryptor(currentKey_);
            }

            {
                std::lock_guard<std::mutex> lock(readyMutex_);
                readyKeys_.insert(currentKey_.index_);
            }
            readyCond_.notify_all();
            MEDIA_LOG_I("Key download completed, index: " PUBLIC_LOG_U64, keyIndex);
        }
        return len;
    }
    MEDIA_LOG_E("Invalid key data length: " PUBLIC_LOG_U32 ", accumulated: " PUBLIC_LOG_U32,
        len, static_cast<uint32_t>(currentKey_.keyLen_));
    return 0;
}

void KeyDownloader::OnKeyDownloadDone(const std::string& url, const std::string& location)
{
    if (!isInterruptNeeded_.load()) {
        DownloadNextKey();
    } else {
        isDownloading_.store(false);
    }
}

void KeyDownloader::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_.store(isInterruptNeeded);
    if (isInterruptNeeded) {
        isDownloading_.store(false);
        readyCond_.notify_all();
        if (downloader_ != nullptr) {
            downloader_->SetInterruptState(isInterruptNeeded);
        }
    }
}

bool KeyDownloader::WaitForKey(uint64_t keyIndex, uint32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(readyMutex_);
    if (readyKeys_.count(keyIndex) > 0) {
        return true;
    }
    readyCond_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
        [this, keyIndex]() {return readyKeys_.count(keyIndex) > 0 || isInterruptNeeded_.load(); });
    return readyKeys_.count(keyIndex) > 0;
}

std::string KeyDownloader::UriJoin(std::string& baseUrl, const std::string& uri)
{
    if ((uri.find("http://") != std::string::npos) || (uri.find("https://") != std::string::npos)) {
        return uri;
    } else if (uri.find("//") == 0) { // start with "//"
        return baseUrl.substr(0, baseUrl.find('/')) + uri;
    } else if (uri.find('/') == 0) {
        auto pos = baseUrl.find('/', strlen("https://"));
        return baseUrl.substr(0, pos) + uri;
    } else {
        std::string::size_type pos = baseUrl.find('?');
        std::string uriMain = (pos != std::string::npos) ? baseUrl.substr(0, pos) : baseUrl;
        pos = uriMain.rfind('/');
        if (pos == std::string::npos) {
            return uri;
        }
        return uriMain.substr(0, pos + 1) + uri;
    }
}
} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS