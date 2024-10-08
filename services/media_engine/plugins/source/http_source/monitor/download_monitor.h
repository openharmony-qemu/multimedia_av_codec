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

#ifndef HISTREAMER_DOWNLOAD_MONITOR_H
#define HISTREAMER_DOWNLOAD_MONITOR_H

#include <ctime>
#include <list>
#include <memory>
#include <string>
#include <set>
#include "osal/task/task.h"
#include "osal/task/mutex.h"
#include "osal/task/blocking_queue.h"
#include "osal/utils/ring_buffer.h"
#include "plugin/plugin_base.h"
#include "download/downloader.h"
#include "media_downloader.h"
#include "common/media_source.h"
#include "common/media_core.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
struct RetryRequest {
    std::shared_ptr<DownloadRequest> request;
    std::function<void()> function;
};

class DownloadMonitor : public MediaDownloader {
public:
    explicit DownloadMonitor(std::shared_ptr<MediaDownloader> downloader) noexcept;
    ~DownloadMonitor() override = default;
    bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override;
    void Close(bool isAsync) override;
    Status Read(unsigned char* buff, ReadDataInfo& readDataInfo) override;
    bool SeekToPos(int64_t offset) override;
    void Pause() override;
    void Resume() override;
    size_t GetContentLength() const override;
    int64_t GetDuration() const override;
    Seekable GetSeekable() const override;
    void SetCallback(Callback *cb) override;
    void SetStatusCallback(StatusCallbackFunc cb) override;
    bool GetStartedStatus() override;
    bool SeekToTime(int64_t seekTime, SeekMode mode) override;
    std::vector<uint32_t> GetBitRates() override;
    bool SelectBitRate(uint32_t bitRate) override;
    void SetIsTriggerAutoMode(bool isAuto) override;
    void SetReadBlockingFlag(bool isReadBlockingAllowed) override;
    void SetDemuxerState(int32_t streamId) override;
    void SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy) override;
    void SetInterruptState(bool isInterruptNeeded) override;
    Status GetStreamInfo(std::vector<StreamInfo>& streams) override;
    Status SelectStream(int32_t streamId) override;
    void GetDownloadInfo(DownloadInfo& downloadInfo) override;
    std::pair<int32_t, int32_t> GetDownloadInfo() override;
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID) override;
    void GetPlaybackInfo(PlaybackInfo& playbackInfo) override;
    size_t GetBufferSize() const override;
    void SetAppUid(int32_t appUid) override;
    bool GetPlayable() override;
    bool GetBufferingTimeOut() override;
    size_t GetSegmentOffset() override;
    bool GetHLSDiscontinuity() override;
    Status StopBufferring(bool isAppBackground) override;
    bool IsBuffering() override;

private:
    int64_t HttpMonitorLoop();
    void OnDownloadStatus(std::shared_ptr<Downloader>& downloader, std::shared_ptr<DownloadRequest>& request);
    bool NeedRetry(const std::shared_ptr<DownloadRequest>& request);
    void NotifyError(int32_t clientErrorCode, int32_t serverErrorCode);
    void GetServerMediaServiceErrorCode(int32_t errorCode, int32_t& serverCode);
    void GetClientMediaServiceErrorCode(int32_t errorCode, int32_t& clientCode);

    std::atomic<bool> isClosed_{false};

    std::shared_ptr<MediaDownloader> downloader_;
    std::list<RetryRequest> retryTasks_;
    std::atomic<bool> isPlaying_ {false};
    std::shared_ptr<Task> task_;
    time_t lastReadTime_ {0};
    Callback* callback_ {nullptr};
    Mutex taskMutex_ {};
    uint64_t haveReadData_ {0};

    std::set<int32_t> clientNotRetryErrorSet = {
        6,  // CURLE_COULDNT_RESOLVE_HOST
        22, // CURLE_HTTP_RETURNED_ERROR
        26, // CURLE_READ_ERROR
        34, // CURLE_HTTP_POST_ERROR
        35, // CURLE_SSL_CONNECT_ERROR
        52, // CURLE_GOT_NOTHING
        58, // CURLE_SSL_CERTPROBLM
        60, // CURLE_SSL_CACERT
        77, // CURLE_SSL_CACERT_BADFILE
    };

    std::set<int32_t> serverNotRetryErrorSet = {
        400, // Bad Request
        401, // Unauthorized
        403, // Forbidden
    };

    std::map<int32_t, MediaServiceErrCode> clientErrorCodeMap_ = {
        {1, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {2, MediaServiceErrCode::MSERR_DATA_SOURCE_IO_ERROR},
        {4, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {5, MediaServiceErrCode::MSERR_IO_CANNOT_FIND_HOST},
        {6, MediaServiceErrCode::MSERR_IO_CANNOT_FIND_HOST},
        {7, MediaServiceErrCode::MSERR_IO_CANNOT_FIND_HOST},
        {8, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
        {9, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {10, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {11, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {12, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {13, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {14, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {15, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {16, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
        {17, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {18, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
        {30, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {31, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {35, MediaServiceErrCode::MSERR_IO_SSL_CONNECT_FAIL},
        {37, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {38, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {39, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {41, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {45, MediaServiceErrCode::MSERR_IO_CONNECTION_TIMEOUT},
        {47, MediaServiceErrCode::MSERR_IO_CANNOT_FIND_HOST},
        {53, MediaServiceErrCode::MSERR_IO_SSL_CONNECT_FAIL},
        {54, MediaServiceErrCode::MSERR_IO_SSL_CONNECT_FAIL},
        {55, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
        {56, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
        {58, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
        {59, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
        {60, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
        {61, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {64, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {65, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
        {66, MediaServiceErrCode::MSERR_IO_SSL_CONNECT_FAIL},
        {68, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {69, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {71, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {72, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {74, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {77, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
        {78, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
        {79, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {80, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {82, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
        {83, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
        {84, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {85, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {86, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {87, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {90, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
        {91, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
        {95, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {96, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {97, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
        {98, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
        {101, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
    };

    std::map<int32_t, MediaServiceErrCode> serverErrorCodeMap_ = {
        {400, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {401, MediaServiceErrCode::MSERR_IO_NO_PERMISSION},
        {403, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {404, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
        {406, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {407, MediaServiceErrCode::MSERR_IO_NO_PERMISSION},
        {408, MediaServiceErrCode::MSERR_IO_CONNECTION_TIMEOUT},
        {409, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {410, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
        {411, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {412, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {413, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {415, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {416, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {417, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {421, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
        {422, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {424, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {425, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {428, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {429, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {431, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {451, MediaServiceErrCode::MSERR_IO_REQUEST_DENID},
        {500, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
        {501, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {502, MediaServiceErrCode::MSERR_IO_NETWORK_UNAVAILABLE},
        {503, MediaServiceErrCode::MSERR_IO_NETWORK_UNAVAILABLE},
        {504, MediaServiceErrCode::MSERR_IO_CONNECTION_TIMEOUT},
        {505, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {506, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {507, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
        {508, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
        {510, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQEST},
        {511, MediaServiceErrCode::MSERR_IO_SSL_CLIENT_CERT_NEEDED},
    };
};
}
}
}
}
#endif
