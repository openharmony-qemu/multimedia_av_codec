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
 
#ifndef HISTREAMER_HLS_MEDIA_DOWNLOADER_H
#define HISTREAMER_HLS_MEDIA_DOWNLOADER_H

#include "playlist_downloader.h"
#include "media_downloader.h"
#include "osal/utils/ring_buffer.h"
#include "osal/utils/steady_clock.h"
#include "openssl/aes.h"
#include "osal/task/task.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HlsMediaDownloader : public MediaDownloader, public PlayListChangeCallback {
public:
    HlsMediaDownloader() noexcept;
    ~HlsMediaDownloader() override = default;
    bool Open(const std::string& url) override;
    void Close(bool isAsync) override;
    void Pause() override;
    void Resume() override;
    bool Read(unsigned char* buff, unsigned int wantReadLength, unsigned int& realReadLength, bool& isEos) override;
    bool SeekToTime(int64_t seekTime) override;

    size_t GetContentLength() const override;
    int64_t GetDuration() const override;
    Seekable GetSeekable() const override;
    void SetCallback(Callback* cb) override;
    void OnPlayListChanged(const std::vector<PlayInfo>& playList) override;
    void SetStatusCallback(StatusCallbackFunc cb) override;
    bool GetStartedStatus() override;
    std::vector<uint32_t> GetBitRates() override;
    bool SelectBitRate(uint32_t bitRate) override;
    void OnSourceKeyChange(const uint8_t *key, size_t keyLen, const uint8_t *iv) override;
    void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos) override;
    void SetIsTriggerAutoMode(bool isAuto) override;
    void SetReadBlockingFlag(bool isReadBlockingAllowed) override;
    void SeekToTs(int64_t seekTime);
    void PutRequestIntoDownloader(const PlayInfo& playInfo);
    void UpdateDownloadFinished(const std::string &url);
    constexpr static int RING_BUFFER_SIZE = 1 * 1024 * 1024;

private:
    bool SaveData(uint8_t* data, uint32_t len);

private:
    std::shared_ptr<RingBuffer> buffer_;
    std::shared_ptr<Downloader> downloader_;
    std::shared_ptr<DownloadRequest> downloadRequest_;

    Callback* callback_ {nullptr};
    DataSaveFunc dataSave_;
    StatusCallbackFunc statusCallback_;
    bool startedPlayStatus_ {false};

    std::shared_ptr<PlayListDownloader> playListDownloader_;

    std::shared_ptr<BlockingQueue<PlayInfo>> playList_;
    std::map<std::string, bool> fragmentDownloadStart;
    std::map<std::string, bool> fragmentPushed;
    std::deque<PlayInfo> backPlayList_;
    bool isSelectingBitrate_ {false};
    bool isDownloadStarted_ {false};
    static constexpr uint32_t DECRYPT_UNIT_LEN = 16;
    uint8_t afterAlignRemainedBuffer_[DECRYPT_UNIT_LEN] {0};
    uint32_t afterAlignRemainedLength_ = 0;
    uint64_t totalLen_ = 0;
    std::string curUrl_;
    uint8_t key_[16] = {0};
    size_t keyLen_ {0};
    uint8_t iv_[16] = {0};
    AES_KEY aesKey_;
    uint8_t decryptCache_[RING_BUFFER_SIZE] {0};
    uint8_t decryptBuffer_[RING_BUFFER_SIZE] {0};
    int havePlayedTsNum_ = 0;
    bool isAutoSelectBitrate_ {true};
    int64_t seekTime_ = 0;
    bool isNeedStopPlayListTask_ {false};
};
}
}
}
}
#endif