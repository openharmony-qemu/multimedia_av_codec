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

#ifndef HISTREAMER_HLS_PLAYLIST_DOWNLOADER_H
#define HISTREAMER_HLS_PLAYLIST_DOWNLOADER_H

#include <set>
#include <utility>
#include "playlist_downloader.h"
#include "m3u8.h"
#include "utils/aes_decryptor.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HlsPlayListDownloader : public PlayListDownloader {
public:
    using PlayListDownloader::PlayListDownloader;
    ~HlsPlayListDownloader() override;

    void Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override;
    void Clone(std::shared_ptr<PlayListDownloader> other) override;
    void UpdateManifest() override;
    void ParseManifest(const std::string& location, bool isPreParse = false) override;
    void SetPlayListCallback(std::weak_ptr<PlayListChangeCallback> callback) override;
    int64_t GetDuration() const override;
    std::pair<int64_t, bool> GetStartInfo() const override;
    Seekable GetSeekable() const override;
    void SelectBitRate(uint32_t bitRate) override;
    std::vector<uint32_t> GetBitRates() override;
    uint64_t GetCurrentBitRate() override;
    int GetVideoHeight() override;
    int GetVideoWidth() override;
    bool IsBitrateSame(uint32_t bitRate) override;
    bool IsMediaSame(uint32_t streamId, HlsSegmentType mediaType) override;
    uint32_t GetCurBitrate() override;
    bool IsLive() const override;
    void NotifyListChange();
    void SetMimeType(const std::string& mimeType) override;
    void PreParseManifest(const std::string& location) override;
    bool IsParseAndNotifyFinished() override;
    bool IsParseFinished() override;
    std::string GetUrl();
    std::shared_ptr<M3U8MasterPlaylist> GetMaster();
    std::shared_ptr<M3U8VariantStream> GetCurrentVariant();
    std::shared_ptr<M3U8VariantStream> GetNewVariant();
    size_t GetSegmentOffset(uint32_t tsIndex) override;
    bool GetHLSDiscontinuity() override;
    void SetInitResolution(uint32_t width, uint32_t height) override;
    size_t GetLiveUpdateGap() const override;
    void InterruptM3U8Parse(bool isInterruptNeeded) override;
    void GetStreamInfo(std::vector<StreamInfo>& streams) override;
    bool ReadFmp4Header(uint8_t* buffer, uint32_t wantLen, uint32_t& readLen, uint32_t streamId) override;
    bool IsHlsFmp4() override;
    bool IsPureByteRange() override;
    void ReOpen(void) override;
    std::shared_ptr<StreamInfo> GetStreamInfoById(int32_t streamId) override;
    int32_t GetDefaultMediaStreamId(HlsSegmentType mediaType) override;
    void SelectMedia(int32_t streamId, HlsSegmentType mediaType) override;
    void SetDefaultMedia(HlsSegmentType mediaType) override;
    void UpdateMedia(HlsSegmentType mediaType) override;
    void UpdateStreamInfo() override;
    HlsSegmentType GetSegType(uint32_t streamId) override;
    uint32_t GetCurStreamId() override;
    void SetSourceStatisticsDfx(std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr,
        bool isFmp4 = false) override;
    bool IsLiveEnd() override;
    std::shared_ptr<AesDecryptor> GetAesDecryptor(uint64_t keyIndex) override;
private:
    void UpdateMasterInfo(bool isPreParse);
    void UpdateMasterAndNotifyList(bool isPreParse);
    bool UpdatePlaylists(bool isSimple);
    bool ReadMediaHeader(const std::list<std::shared_ptr<M3U8Media>>& mediaList, uint8_t* buffer, uint32_t wantLen,
        uint32_t& readLen, uint32_t streamId);
    bool ReadStreamHeader(const std::list<std::shared_ptr<M3U8VariantStream>>& streamList, uint8_t* buffer,
        uint32_t wantLen, uint32_t& readLen, uint32_t streamId);
    void GetMediaStreams(StreamType streamType, std::vector<StreamInfo>& streams);
    void CopyFragmentInfo(PlayInfo& playInfo, std::shared_ptr<M3U8Fragment> file, uint64_t sessionKeyIndex);
    uint64_t KeyChange(std::list<std::shared_ptr<M3U8Fragment>>& files);
    void OnMasterReady(bool needAudioManager, bool needSubtitlesManager);

private:
    std::string url_ {};
    std::string urlOrigin_ {};
    std::weak_ptr<PlayListChangeCallback> callbackWeak_;
    std::shared_ptr<M3U8MasterPlaylist> master_;
    std::shared_ptr<M3U8VariantStream> currentVariant_;
    std::shared_ptr<M3U8VariantStream> newVariant_;
    std::string mimeType_;
    std::atomic<bool> isParseFinished_ {false};
    std::atomic<bool> isNotifyPlayListFinished_ {false};
    std::atomic<bool> isLiveUpdateTaskStarted_ {false};
    std::atomic<bool> needAudioManager_ {false};
    std::atomic<bool> needSubtitlesManager_ {false};
    uint32_t initResolution_ {0};
    std::mutex mediaMutex_;
    std::shared_ptr<M3U8Media> currentAudio_;
    std::shared_ptr<M3U8Media> currentSubtitles_;
    std::mutex streamIdMutex;
    std::set<uint32_t> videoStreamIds_ = std::set<uint32_t>();
    std::set<uint32_t> audioStreamIds_ = std::set<uint32_t>();
    std::set<uint32_t> subtitlesStreamIds_ = std::set<uint32_t>();
    std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> reportInfo_ {nullptr};
    std::atomic<bool> isFmp4_ {false};
    bool isLiveEnd_ {false};
    std::atomic<bool> isPreParseFinished_ {false};
    uint64_t maxSessionKeyIndex_ {0};
    std::unordered_map<uint64_t, std::shared_ptr<AesDecryptor>> aesDecryptorsMap_;
    std::shared_mutex aesDecryptorsMapMutex_;
};
}
}
}
}
#endif