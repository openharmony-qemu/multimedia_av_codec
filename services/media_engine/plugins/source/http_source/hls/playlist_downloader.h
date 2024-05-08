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

#ifndef HISTREAMER_PLAYLIST_DOWNLOADER_H
#define HISTREAMER_PLAYLIST_DOWNLOADER_H

#include <vector>
#include <map>
#include "download/downloader.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
struct PlayInfo {
    std::string url_;
    double duration_;
    int64_t startTimePos_ {0};
};
struct PlayListChangeCallback {
    virtual ~PlayListChangeCallback() = default;
    virtual void OnPlayListChanged(const std::vector<PlayInfo>& playList) = 0;
    virtual void OnSourceKeyChange(const uint8_t* key, size_t keyLen, const uint8_t* iv) = 0;
    virtual void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos) = 0;
    virtual void OnFirstTsReady(const std::string& url, const double& duration) = 0;
};
class PlayListDownloader {
public:
    PlayListDownloader();
    virtual ~PlayListDownloader();

    virtual void Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) = 0;
    virtual void UpdateManifest() = 0;
    virtual void ParseManifest(const std::string& location) = 0;
    virtual int64_t PlayListUpdateLoop() = 0;
    virtual void SetPlayListCallback(PlayListChangeCallback* callback) = 0;
    virtual int64_t GetDuration() const = 0;
    virtual Seekable GetSeekable() const = 0;
    virtual void SelectBitRate(uint32_t bitRate) = 0;
    virtual std::vector<uint32_t> GetBitRates() = 0;
    virtual uint64_t GetCurrentBitRate() = 0;
    virtual int GetVedioHeight() = 0;
    virtual int GetVedioWidth() = 0;
    virtual bool IsBitrateSame(uint32_t bitRate) = 0;
    virtual uint32_t GetCurBitrate() = 0;
    virtual bool IsLive() const = 0;
    virtual int32_t GetVideoWidth() const = 0;
    virtual int32_t GetVideoHeight() const = 0;
    virtual void SetInterruptState(bool isInterruptNeeded) = 0;
    void Resume();
    void Pause();
    void Close();
    void Stop();
    void Start();
    void Cancel();
    void SetStatusCallback(StatusCallbackFunc cb);
    bool GetPlayListDownloadStatus();
    void UpdateDownloadFinished(const std::string& url, const std::string& location);
    std::map<std::string, std::string> GetHttpHeader();

protected:
    bool SaveData(uint8_t* data, uint32_t len);
    static void OnDownloadStatus(DownloadStatus status, std::shared_ptr<Downloader>&,
                          std::shared_ptr<DownloadRequest>& request);
    void DoOpen(const std::string& url);
    void SaveHttpHeader(const std::map<std::string, std::string>& httpHeader);

protected:
    std::shared_ptr<Downloader> downloader_;
    std::shared_ptr<DownloadRequest> downloadRequest_;
    std::shared_ptr<Task> updateTask_;
    DataSaveFunc dataSave_;
    StatusCallbackFunc statusCallback_;
    std::string playList_;
    bool startedDownloadStatus_ {false};
    std::map<std::string, std::string> httpHeader_ {};
};
}
}
}
}
#endif