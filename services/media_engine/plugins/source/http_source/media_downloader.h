/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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
#include "plugin/plugin_base.h"
#include "download/downloader.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class MediaDownloader {
public:
    virtual ~MediaDownloader() = default;
    virtual bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) = 0;
    virtual void Close(bool isAsync) = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual bool Read(unsigned char* buff, unsigned int wantReadLength, unsigned int& realReadLength, bool& isEos) = 0;
    virtual bool SeekToPos(int64_t offset)
    {
        MEDIA_LOG_E("SeekToPos is unimplemented.");
        return false;
    }
    virtual size_t GetContentLength() const = 0;
    virtual int64_t GetDuration() const = 0;
    virtual Seekable GetSeekable() const = 0;
    virtual void SetCallback(Callback* cb) = 0;
    virtual void SetStatusCallback(StatusCallbackFunc cb) = 0;
    virtual bool GetStartedStatus() = 0;
    virtual bool SeekToTime(int64_t seekTime, SeekMode mode)
    {
        MEDIA_LOG_E("SeekToTime is unimplemented.");
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
    virtual void SetIsTriggerAutoMode(bool isAuto)
    {
        MEDIA_LOG_E("SetIsTriggerAutoMode is unimplemented.");
    }
    virtual void SetReadBlockingFlag(bool isReadBlockingAllowed)
    {
        MEDIA_LOG_W("SetReadBlockingFlag is unimplemented.");
    }
    virtual void SetDemuxerState()
    {
        MEDIA_LOG_W("SetDemuxerState is unimplemented.");
    }
    virtual void SetDownloadErrorState()
    {
        MEDIA_LOG_W("SetDownloadErrorState is unimplemented.");
    }
    virtual void SetInterruptState(bool isInterruptNeeded) = 0;
};
}
}
}
}
#endif