/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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
#define HST_LOG_TAG "PlayListDownloader"
#include "playlist_downloader.h"
#include "securec.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
PlayListDownloader::PlayListDownloader()
{
    downloader_ = std::make_shared<Downloader>("hlsPlayList");
    dataSave_ = [this] (uint8_t*&& data, uint32_t&& len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };
    // this is default callback
    statusCallback_ = [this] (DownloadStatus&& status, std::shared_ptr<Downloader> d,
            std::shared_ptr<DownloadRequest>& request) {
        OnDownloadStatus(std::forward<decltype(status)>(status), downloader_,
                         std::forward<decltype(request)>(request));
    };
    updateTask_ = std::make_shared<Task>(std::string("OS_FragmentListUpdate"), "", TaskType::SINGLETON);
    updateTask_->RegisterJob([this] { return PlayListUpdateLoop(); });
}

PlayListDownloader::~PlayListDownloader()
{
    updateTask_->Stop();
    downloader_->Stop();
}

void PlayListDownloader::SaveHttpHeader(const std::map<std::string, std::string>& httpHeader)
{
    httpHeader_ = httpHeader;
}

void PlayListDownloader::DoOpen(const std::string& url)
{
    playList_.clear();
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                      std::shared_ptr<DownloadRequest>& request) {
        statusCallback_(status, downloader_, std::forward<decltype(request)>(request));
    };

    MediaSouce mediaSouce;
    mediaSouce.url = url;
    mediaSouce.httpHeader = httpHeader_;
    downloadRequest_ = std::make_shared<DownloadRequest>(dataSave_, realStatusCallback, mediaSouce, true);
    auto downloadDoneCallback = [this] (const std::string& url, const std::string& location) {
        UpdateDownloadFinished(url, location);
    };
    downloadRequest_->SetDownloadDoneCb(downloadDoneCallback);
    downloader_->Download(downloadRequest_, -1); // -1
    downloader_->Start();
}

bool PlayListDownloader::GetPlayListDownloadStatus()
{
    return startedDownloadStatus_;
}

bool PlayListDownloader::SaveData(uint8_t* data, uint32_t len)
{
    playList_.append(reinterpret_cast<const char*>(data), len);
    startedDownloadStatus_ = true;
    return true;
}

void PlayListDownloader::UpdateDownloadFinished(const std::string& url, const std::string& location)
{
    ParseManifest(location);
}

void PlayListDownloader::OnDownloadStatus(DownloadStatus status, std::shared_ptr<Downloader>&,
                                          std::shared_ptr<DownloadRequest>& request)
{
    // This should not be called normally
    MEDIA_LOG_D("Should not call this OnDownloadStatus, should call monitor.");
    if (request->GetClientError() != NetworkClientErrorCode::ERROR_OK || request->GetServerError() != 0) {
        MEDIA_LOG_E("OnDownloadStatus " PUBLIC_LOG_D32 ", url : " PUBLIC_LOG_S,
                    status, request->GetUrl().c_str());
    }
}

void PlayListDownloader::SetStatusCallback(StatusCallbackFunc cb)
{
    statusCallback_ = cb;
}

void PlayListDownloader::ParseManifest(const std::string& location)
{
    MEDIA_LOG_E("Should not call this ParseManifest");
}

void PlayListDownloader::Resume()
{
    downloader_->Resume();
    if (IsLive()) {
        MEDIA_LOG_I("updateTask_ Start.");
        updateTask_->Start();
    }
}

void PlayListDownloader::Pause()
{
    if (IsLive()) {
        MEDIA_LOG_I("updateTask_ Pause.");
        updateTask_->Pause();
    }
    downloader_->Pause();
}

void PlayListDownloader::Close()
{
    if (IsLive()) {
        MEDIA_LOG_I("updateTask_ Close.");
        updateTask_->Stop();
    }
    downloader_->Stop();
}

void PlayListDownloader::Stop()
{
    downloader_->Stop();
}

void PlayListDownloader::Start()
{
    downloader_->Start();
}

void PlayListDownloader::Cancel()
{
    playList_.clear();
    downloader_->Cancel();
    playList_.clear();
}

std::map<std::string, std::string> PlayListDownloader::GetHttpHeader()
{
    return httpHeader_;
}
}
}
}
}