/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <regex>
#include "network/network_typs.h"
#include "osal/filesystem/file_system.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
constexpr int FDPOS = 2;
constexpr int PLAYLIST_UPDATE_RATE = 1000 * 1000;
constexpr int MIN_PRE_PARSE_CONTENT_LEN = 10 * 1024; // 10k
constexpr int RETRY_DELTA_TIME_TO_REPORT_ERROR = 5 * 1000; // 5s
constexpr int RETRY_TIME_TO_REPORT_ERROR = 10; // 10
constexpr int HTTP_SERVER_ERROR_410 = 410; // 410 表示请求的资源已经从服务器上永久删除，不再可用

static bool isNumber(const std::string& str)
{
    return str.find_first_not_of("0123456789") == std::string::npos;
}

void PlayListDownloader::PlayListDownloaderInit()
{
    dataSave_ = [this] (uint8_t*&& data, uint32_t&& len, bool&& notBlock) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len),
            std::forward<decltype(notBlock)>(notBlock));
    };
    // this is default callback
    statusCallback_ = [this] (DownloadStatus&& status, std::shared_ptr<Downloader> d,
            std::shared_ptr<DownloadRequest>& request) {
        OnDownloadStatus(std::forward<decltype(status)>(status), downloader_,
                         std::forward<decltype(request)>(request));
    };
    updateTask_ = std::make_shared<Task>(std::string("OS_FragmentListUpdate"), "", TaskType::SINGLETON);
    updateTask_->RegisterJob([this] {
        UpdateManifest();
        size_t updateTime = GetLiveUpdateGap();
        if (updateTime > 0 && updateTime > static_cast<size_t>(PLAYLIST_UPDATE_RATE)) {
            return updateTime;
        }
        return static_cast<size_t>(PLAYLIST_UPDATE_RATE);
    });
}

PlayListDownloader::PlayListDownloader(const std::map<std::string, std::string>& httpHeader,
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader)
{
    if (sourceLoader != nullptr) {
        downloader_ = std::make_shared<Downloader>("hlsPlayList", sourceLoader);
    } else {
        downloader_ = std::make_shared<Downloader>("hlsPlayList");
    }
    httpHeader_ = httpHeader;
    PlayListDownloaderInit();
}

PlayListDownloader::PlayListDownloader(std::shared_ptr<Downloader> downloader,
    const std::map<std::string, std::string>& httpHeader) noexcept
{
    downloader_ = downloader;
    httpHeader_ = httpHeader;
    PlayListDownloaderInit();
}

void PlayListDownloader::SaveHttpHeader(const std::map<std::string, std::string>& httpHeader)
{
    httpHeader_ = httpHeader;
}

void PlayListDownloader::DoOpen(const std::string& url)
{
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                      std::shared_ptr<DownloadRequest>& request) {
        // 目标资源永久删除，需要重头重新请求
        if (request->GetServerError() == HTTP_SERVER_ERROR_410) {
            ReOpen();
            return;
        }
        if (retryStartTime_ == 0) {
            retryStartTime_ = request->GetNowTime();
        }
        int64_t nowTime = request->GetNowTime();
        bool isNeedReportError = (nowTime - retryStartTime_) >= RETRY_DELTA_TIME_TO_REPORT_ERROR
                                  || request->GetRetryTimes() >= RETRY_TIME_TO_REPORT_ERROR;
        if (isNeedReportError && eventCallback_ != nullptr) {
            MEDIA_LOG_E("fail to download m3u8.");
            eventCallback_->OnEvent({PluginEventType::CLIENT_ERROR,
                                    {NetworkClientErrorCode::ERROR_TIME_OUT}, "download m3u8"});

            return;
        }
        playList_.clear();
        statusCallback_(status, downloader_, std::forward<decltype(request)>(request));
    };

    RequestInfo requestInfo;
    requestInfo.url = url;
    requestInfo.httpHeader = httpHeader_;
    downloadRequest_ = std::make_shared<DownloadRequest>(dataSave_, realStatusCallback, requestInfo, true);
    if (downloadRequest_ == nullptr) {
        MEDIA_LOG_E("no enough memory downloadRequest_ is nullptr");
        return;
    }
    std::weak_ptr<PlayListDownloader> weakDownloader = weak_from_this();
    auto downloadDoneCallback = [weakDownloader] (const std::string& url, const std::string& location) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "handleResponseCb, PlayListDownloader is nullptr!");
        shareDownloader->UpdateDownloadFinished(url, location);
    };
    downloadRequest_->SetRequestProtocolType(RequestProtocolType::HLS);
    downloadRequest_->SetDownloadDoneCb(downloadDoneCallback);
    downloadRequest_->SetIsIndexM3u8Request(true);
    if (downloader_ != nullptr) {
        downloader_->Download(downloadRequest_, -1); // -1
        downloader_->Start();
    }
}

void PlayListDownloader::DoOpenNative(const std::string& url)
{
    std::string uri = url;
    ParseUriInfo(uri);
    char buf[size_ + 1];

    auto ret = read(fd_, buf, size_);
    buf[size_] = '\0';
    std::string m3u8(buf);
    if (ret < 0) {
        MEDIA_LOG_E("Failed to read, errno " PUBLIC_LOG_D32, static_cast<int32_t>(errno));
        return;
    }
    MEDIA_LOG_I("Read success.");
    playList_ = m3u8;
    ParseManifest(playList_);
}

bool PlayListDownloader::ParseUriInfo(const std::string& uri)
{
    if (uri.empty()) {
        MEDIA_LOG_E("uri is empty");
        return false;
    }
    MEDIA_LOG_D("uri: ");
    std::smatch fdUriMatch;
    FALSE_RETURN_V_MSG_E(std::regex_match(uri, fdUriMatch, std::regex("^fd://(.*)\\?offset=(.*)&size=(.*)")) ||
        std::regex_match(uri, fdUriMatch, std::regex("^fd://(.*)")),
        false, "Invalid fd uri format");
    FALSE_RETURN_V_MSG_E(fdUriMatch.size() >= FDPOS && isNumber(fdUriMatch[1].str()),
        false, "Invalid fd uri format");
    fd_ = std::stoi(fdUriMatch[1].str()); // 1: sub match fd subscript
    FALSE_RETURN_V_MSG_E(fd_ != -1 && FileSystem::IsRegularFile(fd_),
        false, "Invalid fd: " PUBLIC_LOG_D32, fd_);
    fileSize_ = GetFileSize(fd_);
    if (fdUriMatch.size() == 4) { // 4：4 sub match
        offset_ = std::stoll(fdUriMatch[2].str()); // 2: sub match offset subscript
        if (static_cast<uint64_t>(offset_) > fileSize_) {
            offset_ = static_cast<int64_t>(fileSize_);
        }
        size_ = static_cast<uint64_t>(std::stoll(fdUriMatch[3].str())); // 3: sub match size subscript
        uint64_t remainingSize = fileSize_ - static_cast<uint64_t>(offset_);
        if (size_ > remainingSize) {
            size_ = remainingSize;
        }
    } else {
        size_ = fileSize_;
        offset_ = 0;
    }
    position_ = static_cast<uint64_t>(offset_);
    seekable_ = FileSystem::IsSeekable(fd_) ? Seekable::SEEKABLE : Seekable::UNSEEKABLE;
    if (seekable_ == Seekable::SEEKABLE) {
        SeekTo(0);
    }
    MEDIA_LOG_D("Fd: " PUBLIC_LOG_D32 ", offset: " PUBLIC_LOG_D64 ", size: " PUBLIC_LOG_U64, fd_, offset_, size_);
    return true;
}

uint64_t PlayListDownloader::GetFileSize(int32_t fd)
{
    uint64_t fileSize = 0;
    struct stat s {};
    if (fstat(fd, &s) == 0) {
        fileSize = static_cast<uint64_t>(s.st_size);
        return fileSize;
    }
    return fileSize;
}

bool PlayListDownloader::SeekTo(uint64_t offset)
{
    if (fd_ == -1) {
        return false;
    }
    int32_t ret = lseek(fd_, offset + static_cast<uint64_t>(offset_), SEEK_SET);
    if (ret == -1) {
        MEDIA_LOG_E("seek to " PUBLIC_LOG_U64 " failed due to " PUBLIC_LOG_S, offset, strerror(errno));
        return false;
    }
    position_ = offset + static_cast<uint64_t>(offset_);
    MEDIA_LOG_D("now seek to " PUBLIC_LOG_D32, ret);
    return true;
}

bool PlayListDownloader::GetPlayListDownloadStatus()
{
    return startedDownloadStatus_;
}

uint32_t PlayListDownloader::SaveData(uint8_t* data, uint32_t len, bool notBlock)
{
    if (data == nullptr || len == 0) {
        return 0;
    }
    playList_.reserve(playList_.size() + len);
    playList_.append(reinterpret_cast<const char*>(data), len);
    startedDownloadStatus_ = true;
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, false, "downloader nullptr");
    int32_t contentlen = static_cast<int32_t>(downloader_->GetCurrentRequest()->GetFileContentLengthNoWait());
    std::string location;
    downloader_->GetCurrentRequest()->GetLocation(location);
    if (contentlen > MIN_PRE_PARSE_CONTENT_LEN) {
        PreParseManifest(location);
    }
    return len;
}

void PlayListDownloader::UpdateDownloadFinished(const std::string& url, const std::string& location)
{
    ParseManifest(location);
    playList_.clear();
    retryStartTime_ = 0;
    if (isAppBackground_) {
        downloader_->Pause(true);
    }
}

void PlayListDownloader::OnDownloadStatus(DownloadStatus status, std::shared_ptr<Downloader>&,
                                          std::shared_ptr<DownloadRequest>& request)
{
    // This should not be called normally
    MEDIA_LOG_D("Should not call this OnDownloadStatus, should call monitor.");
    if (request->GetClientError() != 0 || request->GetServerError() != 0) {
        MEDIA_LOG_E("OnDownloadStatus " PUBLIC_LOG_D32, status);
    }
}

void PlayListDownloader::SetStatusCallback(StatusCallbackFunc cb)
{
    statusCallback_ = cb;
}

void PlayListDownloader::ParseManifest(const std::string& location, bool isPreParse)
{
    MEDIA_LOG_E("Should not call this ParseManifest");
}

void PlayListDownloader::Resume()
{
    MEDIA_LOG_I("PlayListDownloader::Resume.");
    if (IsLive() && updateTask_ != nullptr) {
        MEDIA_LOG_I("updateTask_ Start.");
        updateTask_->Start();
    }
}

void PlayListDownloader::Pause(bool isAsync)
{
    MEDIA_LOG_I("PlayListDownloader::Pause.");
    if (updateTask_ == nullptr) {
        return;
    }
    if (IsLive()) {
        MEDIA_LOG_I("updateTask_ Pause.");
        if (isAsync) {
            updateTask_->PauseAsync();
        } else {
            updateTask_->Pause();
        }
    }
}

void PlayListDownloader::Close()
{
    if (IsLive()) {
        MEDIA_LOG_I("updateTask_ Close.");
        updateTask_->StopAsync();
    }
    Stop();
}

void PlayListDownloader::Stop()
{
    if (downloader_ != nullptr) {
        MEDIA_LOG_I("PlayListDownloader::Stop.");
        downloader_->Stop(true);
    }
}

void PlayListDownloader::Start()
{
    MEDIA_LOG_I("PlayListDownloader::Start.");
    if (downloader_ != nullptr) {
        downloader_->Start();
    }
}

void PlayListDownloader::Cancel()
{
    MEDIA_LOG_I("PlayListDownloader::Cancel.");
    if (downloader_ != nullptr) {
        downloader_->Cancel();
    }
    playList_.clear();
}

std::map<std::string, std::string> PlayListDownloader::GetHttpHeader()
{
    return httpHeader_;
}

void PlayListDownloader::SetCallback(Callback* cb)
{
    eventCallback_ = cb;
}

void PlayListDownloader::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
    }
    InterruptM3U8Parse(isInterruptNeeded);
}

void PlayListDownloader::SetAppUid(int32_t appUid)
{
    if (downloader_) {
        downloader_->SetAppUid(appUid);
    }
}

void PlayListDownloader::StopBufferring(bool isAppBackground)
{
    MEDIA_LOG_I("PlayListDownloader::StopBufferring.");
    if (downloader_ == nullptr) {
        MEDIA_LOG_E("PlayListDownloader:StopBufferring error.");
        return;
    }
    isAppBackground_ = isAppBackground;
    downloader_->SetAppState(isAppBackground);
    downloader_->StopBufferring();
}

}
}
}
}