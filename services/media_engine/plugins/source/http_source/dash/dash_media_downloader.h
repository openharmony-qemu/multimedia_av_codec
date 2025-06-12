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
 
#ifndef HISTREAMER_DASH_MEDIA_DOWNLOADER_H
#define HISTREAMER_DASH_MEDIA_DOWNLOADER_H

#include "media_downloader.h"
#include "dash_mpd_downloader.h"
#include "dash_segment_downloader.h"
#include "osal/utils/steady_clock.h"
#include "osal/task/task.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
struct DashPreparedAction {
    int64_t seekPosition_ {-1};
    DashMpdBitrateParam preparedBitrateParam_;
    DashMpdTrackParam preparedAudioParam_;
    DashMpdTrackParam preparedSubtitleParam_;
};

class DashMediaDownloader : public MediaDownloader, public DashMpdCallback {
public:
    explicit DashMediaDownloader(std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader = nullptr);
    ~DashMediaDownloader() override;
    bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override;
    void Close(bool isAsync) override;
    void Pause() override;
    void Resume() override;
    Status Read(unsigned char* buff, ReadDataInfo& readDataInfo) override;
    bool SeekToTime(int64_t seekTime, SeekMode mode) override;

    size_t GetContentLength() const override;
    int64_t GetDuration() const override;
    Seekable GetSeekable() const override;
    void SetCallback(Callback* cb) override;
    void SetStatusCallback(StatusCallbackFunc cb) override;
    bool GetStartedStatus() override;
    std::vector<uint32_t> GetBitRates() override;
    bool SelectBitRate(uint32_t bitrate) override;
    Status SelectStream(int32_t streamId) override;
    void SetIsTriggerAutoMode(bool isAuto) override;
    void SeekToTs(int64_t seekTime);
    void SetDownloadErrorState() override;
    void SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy) override;
    Status GetStreamInfo(std::vector<StreamInfo>& streams) override;

    void OnMpdInfoUpdate(DashMpdEvent mpdEvent) override;
    void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos) override;
    void UpdateDownloadFinished(int streamId);
    void SetInterruptState(bool isInterruptNeeded) override;
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID) override;
    void SetDemuxerState(int32_t streamId) override;
    void GetPlaybackInfo(PlaybackInfo& playbackInfo) override;
    size_t GetBufferSize() const override;
    bool GetPlayable() override;
    void SetAppUid(int32_t appUid) override;
    bool GetBufferingTimeOut() override;
    void NotifyInitSuccess() override;
    uint64_t GetMemorySize() override;
    std::string GetContentType() override;
    Status StopBufferring(bool isAppBackground) override;

private:
    void ReceiveMpdStreamInitEvent();
    void ReceiveMpdParseOkEvent();
    void PostBufferingEvent(int streamId, BufferingInfoType type);
    void VideoSegmentDownloadFinished(int streamId);
    void GetSegmentToDownload(int downloadStreamId, bool streamSwitchFlag);
    void CleanVideoSegmentBuffer(bool &bufferCleanFlag, int64_t &remainLastNumberSeq);
    bool SelectBitrateInternal(bool bufferCleanFlag, int64_t remainLastNumberSeq);
    bool CheckAutoSelectBitrate(int streamId);
    uint32_t GetNextBitrate(std::shared_ptr<DashSegmentDownloader> segmentDownloader);
    bool AutoSelectBitrateInternal(uint32_t bitrate);
    bool IsSeekingInSwitch();
    int64_t GetVideoSeekTime(int64_t seekTimeMs);
    void SeekInternal(int64_t seekTimeMs);
    Status SelectAudio(const std::shared_ptr<DashStreamDescription> &streamDesc);
    Status SelectAudioInternal(const std::shared_ptr<DashStreamDescription> &streamDesc);
    Status SelectSubtitle(const std::shared_ptr<DashStreamDescription> &streamDesc);
    Status SelectSubtitleInternal(const std::shared_ptr<DashStreamDescription> &streamDesc);
    bool DoPreparedSwitchBitrate(bool switchBitrateOk, bool &needDownload, int &streamId);
    bool DoPreparedSwitchAudio(int &streamId);
    bool DoPreparedSwitchSubtitle(int &streamId);
    bool DoPreparedSwitchAction(bool switchBitrateOk, bool switchAudioOk, bool switchSubtitleOk, int &streamId);
    bool DoPreparedAction(int &streamId);
    void UpdateSegmentIndexAfterSidxParseOk();
    void ResetBitrateParam();
    void ResetTrackParam();
    std::shared_ptr<DashSegmentDownloader> GetSegmentDownloader(int32_t streamId);
    std::shared_ptr<DashSegmentDownloader> GetSegmentDownloaderByType(MediaAVCodec::MediaType type) const;
    void OpenInitSegment(const std::shared_ptr<DashStreamDescription> &streamDesc,
                         const std::shared_ptr<DashSegment> &seg);
    void HandleSeekReady(int32_t streamType, int32_t streamId, int64_t seekTimeMs, int32_t isEos);

private:

    Callback* callback_ {nullptr};
    StatusCallbackFunc statusCallback_ {nullptr};

    std::shared_ptr<DashMpdDownloader> mpdDownloader_ {nullptr};
    std::vector<std::shared_ptr<DashSegmentDownloader>> segmentDownloaders_;

    std::atomic<bool> isInterruptNeeded_{false};
    bool isAutoSelectBitrate_ {true};
    bool downloadErrorState_ {false};
    int64_t breakpoint_ {0};
    DashMpdBitrateParam bitrateParam_;
    DashMpdTrackParam trackParam_;
    DashPreparedAction preparedAction_;
    std::mutex switchMutex_;
    std::mutex parseSidxMutex_;
    uint64_t expectDuration_{0};
    std::mutex bufferingMutex_;
    uint32_t bufferingFlag_{0};
    uint32_t lastBufferingPercent_{0};
    double bufferDurationForPlaying_ {0};

    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader_ {nullptr};
};
}
}
}
}
#endif