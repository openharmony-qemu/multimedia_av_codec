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
#define HISTREAMER_HLS_KEY_DOWNLOADER_H

#include "hls_key_downloader.h"
#include "m3u8.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin{

constexpr uint32_t MAX_KEY_LEN = 16;
constexpr size_t AES_IV_LEN = 16;
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
    }
    if (pendingKeys_.empty()) {
        return;
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

    auto weakSelf = weak_from_this();

    auto keyDataSave = [weakSelf](uint8_t *&&data, uint32_t &&len, bool &&notBlock,
        uint64_t &&keyIndex) -> uint32_t {
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

    // Status callbakc
    auto statusCallback = [weakSeif](DownloadStatus status, std::shared_ptr<Downloader>&,
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

    std::string realKeyUrl = UriJoin(baseUri_, keyInfo.uri);
    RequestInfo requestInfo;
    requestInfo.url = realKeyUrl;
    requestInfo.httpHeader = httpHeader_;
    requestInfo.timeoutMs = KEY_DOWNLOAD_TIMEOUT_MS;

    auto downloadRequest = std::make_shared<DownloadRequest>(
        keyInfo.index_, keyDataSave, statusCallback, requestInfo, true);
    downloadRequest->SetIsAuthRequest(true);
    downloadRequest->SetRequestType(RequestType::HLS_KEY);
    downloadRequest->SetDownloadDoneCb(downloadDoneCb);

    MEDIA_LOG_I("Start downloading key, index: " PUBLIC_LOG_U64 ", url: " PUBLIC_LOG_S,
        keyInfo.index_, realKeyUrl.c_str());

    downloader_->Download(downloadRequest, -1);
    downloader_->Start();
}

uint32_t KeyDownloader::OnKeyDataSaved(uint8_t data, uint32_t len, bool noBlock, uint64_t keyIndex)
{
    FALSE_RETURN_V_MEG_E(data != nullptr, 0u, "key data is nullptr");
    if (len > 0 && len <= MAX_KEY_LEN) {
        KeyInfo keyinfo;
        keyInfo.index_ = ketIndex;
        NZERO_RETURN_V(memcpy_s(keyInfo.key_, MAX_KEY_LEN, data, len), 0);
        keyInfo.keyLen_ = len;
        NZERO_RETURN_V(memcpy_s(keyInfo.iv_, AES_IV_LEN, currentKey_.iv_, AES_IV_LEN), 0);
        keyInfo.hasExplicitIv_ = currentKey_.hasExplicitIv_;

        if (aseDecryptorManager_ != nullptr) {
            aesDecryptorManager_->CreateOneAesDecryptor(keyInfo);
        }

        {
            std::lock_guard<std::mutex> lock(readyMutex_);
            readyKeys_.insert(keyIndex);
        }
        readyCond_.notify_all();
        MEDIA_LOG_I("Key download completed, index: " PUBLIC_LOG_U64, keyIndex);
        return len;
    }
    MEDIA_LOG_E("Invalied ket data length: " PUBLIC_LOG_U32, len);
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
    isInterruptNeeded_.stroe(isInterruptNeeded);
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
    if (readKeys_.count(keyIndex) > 0 ) {
        return ture;
    }
    return readyCond_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
        [this, keyIndex]() {return readyKeys_count(keyIndex) > 0 || isInterruptNeeded_.load(); });
}

} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // HISTREAMER_HLS_KEY_DOWNLOADER_H