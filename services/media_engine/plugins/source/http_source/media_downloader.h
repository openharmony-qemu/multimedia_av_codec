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
 
#ifndef HISTREAMER_MEDIA_DOWNLOADER_H
#define HISTREAMER_MEDIA_DOWNLOADER_H

#include <string>
#include <utility>
#include "avcodec_sysevent.h"
#include "plugin/plugin_base.h"
#include "meta/media_types.h"
#include "plugin/source_plugin.h"
#include "download/downloader.h"
#include "common/media_source.h"
#include "download/media_source_loading_request.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
struct ReadDataInfo {
    int32_t streamId_ = 0;
    int32_t nextStreamId_ = 0; // streamId will change after switch in dash
    uint64_t ffmpegOffset = 0;
    unsigned int wantReadLength_ = 0;
    unsigned int realReadLength_ = 0;
    bool isEos_ = false;
};

constexpr int64_t LOOP_TIMEOUT = 60; // s

class MediaDownloader {
public:
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_STREAM_SOURCE, "HiStreamer" };
    virtual ~MediaDownloader() = default;
    virtual void Init() = 0;
    virtual void SetSourceStatisticsDfx(std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr) = 0;
    virtual bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) = 0;
    virtual void Close(bool isAsync) = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual Status Read(unsigned char* buff, ReadDataInfo& readDataInfo) = 0;
    virtual bool SeekToPos(int64_t offset, bool& isSeekHit)
    {
        MEDIA_LOG_E("SeekToPos is unimplemented.");
        return false;
    }
    virtual uint64_t GetBufferSize() const = 0;
    virtual bool GetPlayable()
    {
        return false;
    }
    virtual bool GetBufferingTimeOut() = 0;
    virtual size_t GetContentLength() const = 0;
    virtual int64_t GetDuration() const = 0;
    virtual Seekable GetSeekable() const = 0;
    virtual void SetCallback(const std::shared_ptr<Callback>& cb) = 0;
    virtual void SetStatusCallback(StatusCallbackFunc cb) = 0;
    virtual bool GetStartedStatus() = 0;
    virtual void GetDownloadInfo(DownloadInfo& downloadInfo)
    {
        MEDIA_LOG_E("GetDownloadInfo is unimplemented.");
    }
    virtual std::pair<int32_t, int32_t> GetDownloadInfo()
    {
        MEDIA_LOG_E("GetDownloadInfo is unimplemented.");
        return std::make_pair(0, 0);
    }
    virtual void GetPlaybackInfo(PlaybackInfo& playbackInfo)
    {
        MEDIA_LOG_E("GetPlaybackInfo is unimplemented.");
    }
    virtual bool SeekToTime(int64_t seekTime, SeekMode mode)
    {
        MEDIA_LOG_E("SeekToTime is unimplemented.");
        return false;
    }
    virtual bool MediaSeekTimeByStreamId(int64_t seekTime, SeekMode mode, int32_t streamId)
    {
        MEDIA_LOG_E("MediaSeekTimeByStreamId is unimplemented.");
        return false;
    }
    virtual std::vector<uint32_t> GetBitRates()
    {
        MEDIA_LOG_E("GetBitRates is unimplemented.");
        return {};
    }
    virtual bool SelectBitRate(uint32_t bitRate)
    {
        MEDIA_LOG_E("SelectBitRate is unimplemented.");
        return false;
    }
    virtual bool AutoSelectBitRate(uint32_t bitRate)
    {
        MEDIA_LOG_E("AutoSelectBitRate is unimplemented.");
        return false;
    }
    virtual void SetIsTriggerAutoMode(bool isAuto)
    {
        MEDIA_LOG_E("SetIsTriggerAutoMode is unimplemented.");
    }
    virtual void SetReadBlockingFlag(bool isReadBlockingAllowed)
    {
        MEDIA_LOG_W("SetReadBlockingFlag is unimplemented.");
    }
    virtual void SetDemuxerState(int32_t streamId)
    {
        MEDIA_LOG_W("SetDemuxerState is unimplemented.");
    }
    virtual void SetDownloadErrorState()
    {
        MEDIA_LOG_W("SetDownloadErrorState is unimplemented.");
    }
    virtual Status SetCurrentBitRate(int32_t bitRate, int32_t streamID)
    {
        MEDIA_LOG_W("SetCurrentBitRate is unimplemented.");
        return Status::OK;
    }
    virtual void SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy)
    {
        MEDIA_LOG_W("SetPlayStrategy is unimplemented.");
    }
    virtual void SetInterruptState(bool isInterruptNeeded) = 0;
    virtual Status GetStreamInfo(std::vector<StreamInfo>& streams, bool isUpdate = false)
    {
        MEDIA_LOG_W("GetStreamInfo is unimplemented.");
        return Status::OK;
    }
    virtual Status SelectStream(int32_t streamId)
    {
        MEDIA_LOG_W("SelectStream is unimplemented.");
        return Status::OK;
    }

    virtual void SetAppUid(int32_t appUid) = 0;
    virtual size_t GetSegmentOffset()
    {
        return 0;
    }
    virtual bool GetHLSDiscontinuity()
    {
        return false;
    }

    virtual bool SetInitialBufferSize(int32_t offset, int32_t size)
    {
        return false;
    }

    virtual Status StopBufferring(bool isAppBackground)
    {
        MEDIA_LOG_W("StopBufferring is unimplemented.");
        return Status::OK;
    }

    virtual std::pair<int64_t, bool> GetStartInfo() const
    {
        return std::make_pair(0, false);
    }

    virtual void WaitForBufferingEnd() {}
     
    virtual void SetIsReportedErrorCode() {}
    
    virtual bool GetReadTimeOut(bool isDelay)
    {
        return false;
    }

    virtual bool IsNotRetry(const std::shared_ptr<DownloadRequest>& request)
    {
        return false;
    }

    virtual void NotifyInitSuccess() {}
    virtual uint64_t GetCachedDuration()
    {
        return 0;
    }
    virtual void RestartAndClearBuffer() {}
    virtual bool IsFlvLive()
    {
        return false;
    }

    virtual void SetStartPts(int64_t startPts)
    {
        MEDIA_LOG_W("SetStartPts is unimplemented.");
    }

    virtual void SetExtraCache(uint64_t cacheDuration)
    {
        MEDIA_LOG_W("SetExtraCache is unimplemented.");
    }

    virtual void SetMediaStreams(const MediaStreamList& mediaStreams)
    {
        MEDIA_LOG_W("SetMediaStreams is unimplemented.");
    }
    virtual std::string GetContentType()
    {
        return "";
    }
    virtual void ClearBuffer()
    {
        MEDIA_LOG_W("ClearBuffer is unimplemented.");
    }
    virtual bool IsHlsFmp4()
    {
        return false;
    }

    virtual uint64_t GetMemorySize()
    {
        return 0;
    }

    virtual std::string GetCurUrl()
    {
        return "";
    }

    virtual bool IsHlsEnd(int32_t streamId = -1)
    {
        return false;
    }

    virtual void SetDefaultStreamId(int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId) {}
};
}
}
}
}
#endif