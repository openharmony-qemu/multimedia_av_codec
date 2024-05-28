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
};

class DashMediaDownloader : public MediaDownloader, public DashMpdCallback {
public:
    DashMediaDownloader() noexcept;
    ~DashMediaDownloader() override;
    bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override;
    void Close(bool isAsync) override;
    void Pause() override;
    void Resume() override;
    bool Read(unsigned char* buff, ReadDataInfo& readDataInfo) override;
    bool SeekToTime(int64_t seekTime, SeekMode mode) override;

    size_t GetContentLength() const override;
    int64_t GetDuration() const override;
    Seekable GetSeekable() const override;
    void SetCallback(Callback* cb) override;
    void SetStatusCallback(StatusCallbackFunc cb) override;
    bool GetStartedStatus() override;
    std::vector<uint32_t> GetBitRates() override;
    bool SelectBitRate(uint32_t bitrate) override;
    void SetIsTriggerAutoMode(bool isAuto) override;
    void SeekToTs(int64_t seekTime);
    void SetDownloadErrorState() override;
    void SetPlayStrategy(PlayStrategy* playStrategy) override;
    Status GetStreamInfo(std::vector<StreamInfo>& streams) override;

    void OnMpdInfoUpdate(DashMpdEvent mpdEvent) override;
    void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos) override;
    void UpdateDownloadFinished(int streamId);
    void SetInterruptState(bool isInterruptNeeded) override;

private:
    void ReceiveMpdStreamInitEvent();
    void ReceiveMpdParseOkEvent();
    void VideoSegmentDownloadFinished(int streamId);
    void GetSegmentToDownload(int downloadStreamId, bool streamSwitchFlag);
    bool SelectBitrateInternal(uint32_t bitrate);
    bool CheckAutoSelectBitrate(int streamId);
    uint32_t GetNextBitrate(std::shared_ptr<DashSegmentDownloader> segmentDownloader)
    bool AutoSelectBitrateInternal(uint32_t bitrate);
    void SeekInternal(int64_t seekTimeMs);
    bool DoPreparedAction();
    void ResetBitrateParam();
    std::shared_ptr<DashSegmentDownloader> GetSegmentDownloader(int32_t streamId);
    std::shared_ptr<DashSegmentDownloader> GetSegmentDownloaderByType(MediaAVCodec::MediaType type);
    void OpenInitSegment(const std::shared_ptr<DashStreamDescription> &streamDesc,
                                                            const std::shared_ptr<DashSegment> &seg);

private:

    Callback* callback_ {nullptr};
    StatusCallbackFunc statusCallback_ {nullptr};

    std::shared_ptr<DashMpdDownloader> mpdDownloader_;
    std::vector<std::shared_ptr<DashSegmentDownloader>> segmentDownloaders_;

    bool isAutoSelectBitrate_ {true};
    bool downloadErrorState_ {false};
    int64_t breakpoint_ {0};
    DashMpdBitrateParam bitrateParam_;
    DashPreparedAction preparedAction_;
    std::mutex switchMutex_;
    uint64_t expectDuration_{0};
};
}
}
}
}
#endif