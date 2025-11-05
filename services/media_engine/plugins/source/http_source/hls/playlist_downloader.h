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

#ifndef HISTREAMER_PLAYLIST_DOWNLOADER_H
#define HISTREAMER_PLAYLIST_DOWNLOADER_H

#include <vector>
#include <map>
#include "download/downloader.h"
#include "plugin/plugin_base.h"
#include "plugin/source_plugin.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
struct PlayInfo {
    std::string url_;
    double duration_;
    int64_t startTimePos_ {0};
    uint32_t offset_ {0};
    uint32_t length_ {0};
    std::string rangeUrl_;
    uint32_t streamId_ {0};
};
struct PlayListChangeCallback {
    virtual ~PlayListChangeCallback() = default;
    virtual void OnMasterReady(bool needAudioManager, bool needSubTitleManager) = 0;
    virtual void OnPlayListChanged(const std::vector<PlayInfo>& playList) = 0;
    virtual void OnSourceKeyChange(const uint8_t* key, size_t keyLen, const uint8_t* iv) = 0;
    virtual void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos) = 0;
};
enum class HlsSegmentType : int {
    SEG_VIDEO = 0,
    SEG_AUDIO = 1,
    SEG_SUBTITLE = 2,
};
class PlayListDownloader : public std::enable_shared_from_this<PlayListDownloader> {
public:
    explicit PlayListDownloader(
        const std::map<std::string, std::string>& httpHeader = std::map<std::string, std::string>(),
        std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader = nullptr);
    explicit PlayListDownloader(std::shared_ptr<Downloader> downloader,
        const std::map<std::string, std::string>& httpHeader = std::map<std::string, std::string>()) noexcept;
    virtual ~PlayListDownloader() = default;

    virtual void Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) = 0;
    virtual void Clone(std::shared_ptr<PlayListDownloader> other) = 0;
    virtual void UpdateManifest() = 0;
    virtual void ParseManifest(const std::string& location, bool isPreParse = false) = 0;
    virtual void SetPlayListCallback(std::weak_ptr<PlayListChangeCallback> callback) = 0;
    virtual int64_t GetDuration() const = 0;
    virtual Seekable GetSeekable() const = 0;
    virtual void SelectBitRate(uint32_t bitRate) = 0;
    virtual std::vector<uint32_t> GetBitRates() = 0;
    virtual uint64_t GetCurrentBitRate() = 0;
    virtual int GetVideoHeight() = 0;
    virtual int GetVideoWidth() = 0;
    virtual bool IsBitrateSame(uint32_t bitRate) = 0;
    virtual bool IsAudioSame(uint32_t streamId) = 0;
    virtual uint32_t GetCurBitrate() = 0;
    virtual bool IsLive() const = 0;
    virtual void SetMimeType(const std::string& mimeType) = 0;
    virtual void PreParseManifest(const std::string& location) = 0;
    virtual bool IsParseAndNotifyFinished() = 0;
    virtual bool IsParseFinished() = 0;
    virtual void SetInitResolution(uint32_t width, uint32_t height) = 0;
    virtual void GetStreamInfo(std::vector<StreamInfo>& streams) = 0;
    virtual bool ReadFmp4Header(uint8_t* buffer, uint32_t wantLen, uint32_t& readLen, uint32_t streamId) = 0;
    virtual bool IsHlsFmp4() = 0;
    virtual bool IsPureByteRange() = 0;
    virtual void ReOpen(void) = 0;
    virtual std::shared_ptr<StreamInfo> GetStreamInfoById(int32_t streamId) = 0;
    virtual int32_t GetDefaultAudioStreamId() = 0;
    virtual void SelectAudio(int32_t streamId) = 0;
    virtual void SetDefaultAudio() = 0;
    virtual void UpdateAudio() = 0;

    void SetInterruptState(bool isInterruptNeeded);
    void Resume();
    void Pause(bool isAsync = false);
    void Close();
    void Stop();
    void Start();
    void Cancel();
    void SetStatusCallback(StatusCallbackFunc cb);
    bool GetPlayListDownloadStatus();
    void Init();
    void UpdateDownloadFinished(const std::string& url, const std::string& location);
    std::map<std::string, std::string> GetHttpHeader();
    void SetCallback(Callback* cb);
    void SetAppUid(int32_t appUid);
    virtual size_t GetSegmentOffset(uint32_t tsIndex)
    {
        return 0;
    }

    virtual bool GetHLSDiscontinuity()
    {
        return false;
    }
    void StopBufferring(bool isAppBackground);

    virtual size_t GetLiveUpdateGap() const
    {
        return 0;
    }
    virtual void InterruptM3U8Parse(bool isInterruptNeeded) {}

    virtual void UpdateStreamInfo() = 0;
    virtual HlsSegmentType GetSegType(uint32_t streamId) = 0;

protected:
    uint32_t SaveData(uint8_t* data, uint32_t len, bool notBlock);
    static void OnDownloadStatus(DownloadStatus status, std::shared_ptr<Downloader>&,
                          std::shared_ptr<DownloadRequest>& request);
    void DoOpen(const std::string& url);
    void DoOpenNative(const std::string& url);
    void SaveHttpHeader(const std::map<std::string, std::string>& httpHeader);
    bool ParseUriInfo(const std::string& uri);
    bool SeekTo(uint64_t offset);
    uint64_t GetFileSize(int32_t fd);

protected:
    std::shared_ptr<Downloader> downloader_;
    std::shared_ptr<DownloadRequest> downloadRequest_;
    std::shared_ptr<Task> updateTask_;
    DataSaveFunc dataSave_;
    StatusCallbackFunc statusCallback_;
    std::string playList_;
    bool startedDownloadStatus_ {false};
    std::map<std::string, std::string> httpHeader_ {};
    int32_t fd_ {-1};
    int64_t offset_ {0};
    uint64_t size_ {0};
    uint64_t fileSize_ {0};
    Seekable seekable_ {Seekable::SEEKABLE};
    uint64_t position_ {0};
    int64_t retryStartTime_ {0};
    Callback* eventCallback_ {nullptr};
    std::atomic<bool> isInterruptNeeded_{false};
    std::atomic<bool> isAppBackground_ {false};
};
}
}
}
}
#endif