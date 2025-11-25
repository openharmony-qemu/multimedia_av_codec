/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
 
#ifndef HISTREAMER_HLS_MEDIA_DOWNLOADER_H
#define HISTREAMER_HLS_MEDIA_DOWNLOADER_H

#include <mutex>
#include <thread>
#include <unistd.h>
#include <utility>
#include "playlist_downloader.h"
#include "download/downloader.h"
#include "media_downloader.h"
#include "osal/utils/ring_buffer.h"
#include "osal/utils/steady_clock.h"
#include "openssl/aes.h"
#include "osal/task/task.h"
#include "common/media_source.h"
#include "common/media_core.h"
#include "utils/media_cached_buffer.h"
#include "utils/write_bitrate_caculator.h"
#include "osal/task/mutex.h"
#include "osal/task/condition_variable.h"
#include "av_common.h"
#include "hls_segment_manager.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

class HlsMediaDownloader : public MediaDownloader,
    public std::enable_shared_from_this<HlsMediaDownloader> {
public:
    explicit HlsMediaDownloader(int expectBufferDuration, bool userDefinedDuration,
        const std::map<std::string, std::string>& httpHeader = std::map<std::string, std::string>(),
        std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader = nullptr);
    explicit HlsMediaDownloader(std::string mimeType,
        const std::map<std::string, std::string>& httpHeader = std::map<std::string, std::string>());
    ~HlsMediaDownloader() override;
    void Init() override;
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
    bool SelectBitRate(uint32_t bitRate) override;
    void SetIsTriggerAutoMode(bool isAuto) override;
    void SetReadBlockingFlag(bool isReadBlockingAllowed) override;
    void AutoSelectBitrate(uint32_t bitRate);
    void SetInterruptState(bool isInterruptNeeded) override;
    void GetPlaybackInfo(PlaybackInfo& playbackInfo) override;
    void GetDownloadInfo(DownloadInfo& downloadInfo) override;
    std::pair<int32_t, int32_t> GetDownloadInfo() override;
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID) override;
    uint64_t GetBufferSize() const override;
    bool GetPlayable() override;
    bool GetBufferingTimeOut() override;
    bool GetReadTimeOut(bool isDelay) override;
    void SetAppUid(int32_t appUid) override;
    size_t GetSegmentOffset() override;
    bool GetHLSDiscontinuity() override;
    Status StopBufferring(bool isAppBackground) override;
    void WaitForBufferingEnd() override;
    void SetIsReportedErrorCode() override;
    void SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy) override;
    bool SetInitialBufferSize(int32_t offset, int32_t size) override;
    void NotifyInitSuccess() override;
    uint64_t GetCachedDuration() override;
    Status GetStreamInfo(std::vector<StreamInfo>& streams) override;
    bool IsHlsFmp4() override;
    uint64_t GetMemorySize() override;
    std::string GetContentType() override;
    bool IsHlsEnd(int32_t streamId = -1) override;
    Status SelectStream(int32_t streamId) override;
    void PostAllEvent(HlsSegEvent event);
    void PostBufferingEvent(HlsSegmentType mediaType, BufferingInfoType type);

private:
    void SetDemuxerState(int32_t streamId) override;
    void SetDownloadErrorState() override;
    std::shared_ptr<HlsSegmentManager> GetSegmentManager(uint32_t streamId);
    void OnMasterReady(bool needAudioManager, bool needSubTitleManager);

private:
    Callback* callback_ {nullptr};
    std::shared_ptr<HlsSegmentManager> videoSegManager_ {nullptr};
    std::shared_ptr<HlsSegmentManager> audioSegManager_ {nullptr};
    uint32_t bufferingFlag_ {0};
    std::mutex bufferingMutex_;
};
}
}
}
}
#endif