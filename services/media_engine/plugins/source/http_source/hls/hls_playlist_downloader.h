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

#ifndef HISTREAMER_HLS_PLAYLIST_DOWNLOADER_H
#define HISTREAMER_HLS_PLAYLIST_DOWNLOADER_H

#include "playlist_downloader.h"
#include "m3u8.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HlsPlayListDownloader : public PlayListDownloader {
public:
    HlsPlayListDownloader() = default;
    ~HlsPlayListDownloader() override = default;

    void Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override;
    void UpdateManifest() override;
    void ParseManifest(const std::string& location) override;
    int64_t PlayListUpdateLoop() override;
    void SetPlayListCallback(PlayListChangeCallback* callback) override;
    int64_t GetDuration() const override;
    Seekable GetSeekable() const override;
    void SelectBitRate(uint32_t bitRate) override;
    std::vector<uint32_t> GetBitRates() override;
    uint64_t GetCurrentBitRate() override;
    int GetVedioHeight() override;
    int GetVedioWidth() override;
    bool IsBitrateSame(uint32_t bitRate) override;
    uint32_t GetCurBitrate() override;
    bool IsLive() const override;
    void NotifyListChange();
    int32_t GetVideoWidth() const override;
    int32_t GetVideoHeight() const override;
    void FirstTsUpdateLoop();
    void SetInterruptState(bool isInterruptNeeded) override;
    std::string GetUrl();
    std::shared_ptr<M3U8MasterPlaylist> GetMaster();
    std::shared_ptr<M3U8VariantStream> GetCurrentVariant();
    std::shared_ptr<M3U8VariantStream> GetNewVariant();

private:
    std::string url_ {};
    std::atomic<bool> isInterruptNeeded_{false};
    PlayListChangeCallback* callback_ {nullptr};
    std::shared_ptr<M3U8MasterPlaylist> master_;
    std::shared_ptr<M3U8VariantStream> currentVariant_;
    std::shared_ptr<M3U8VariantStream> newVariant_;
    std::shared_ptr<Task> firstTsTask_;
};
}
}
}
}
#endif