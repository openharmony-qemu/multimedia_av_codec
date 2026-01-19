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

#ifndef HISTREAMER_M3U8_H
#define HISTREAMER_M3U8_H

#include <memory>
#include <string>
#include <list>
#include <unordered_map>
#include <functional>
#include <map>
#include "hls_tags.h"
#include "playlist_downloader.h"
#include "download/downloader.h"
#include "download/download_metrics_info.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
enum class M3U8MediaType : int32_t {
    M3U8_MEDIA_TYPE_INVALID = -1,
    M3U8_MEDIA_TYPE_AUDIO,
    M3U8_MEDIA_TYPE_VIDEO,
    M3U8_MEDIA_TYPE_SUBTITLES,
    M3U8_MEDIA_TYPE_CLOSED_CAPTIONS,
    M3U8_N_MEDIA_TYPES,
};

struct M3U8Fragment {
    M3U8Fragment(const std::string &uri, double duration, int sequence, bool discont);
    M3U8Fragment(const M3U8Fragment &m3u8, const uint8_t *key, const uint8_t *iv);
    std::string uri_;
    double duration_;
    int64_t sequence_;
    bool discont_ {false};
    uint8_t key_[16] { 0 };
    int iv_[16] {0};
    uint32_t offset_ {0};
    uint32_t length_ {0};
};

struct M3U8Info {
    std::string uri;
    double duration = 0;
    bool discontinuity = false;
    bool bVod {false};
};

struct M3U8 : public std::enable_shared_from_this<M3U8> {
    M3U8(const std::string &uri, const std::string &name, StatusCallbackFunc statusCallback = [](DownloadStatus,
        std::shared_ptr<Downloader>&, std::shared_ptr<DownloadRequest>&) {},
        std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader = nullptr);
    ~M3U8();
    void InitTagUpdaters();
    void InitTagUpdatersMap();
    bool Update(const std::string& playList, bool isNeedCleanFiles);
    void UpdateFromTags(std::list<std::shared_ptr<Tag>>& tags);
    void AddFile(std::shared_ptr<M3U8Fragment> fragment, size_t duration);
    void GetExtInf(const std::shared_ptr<Tag>& tag, double& duration) const;
    double GetDuration() const;
    bool IsLive() const;
    size_t GetLiveUpdateGap() const;

    std::string uri_;
    std::string name_;
    std::unordered_map<HlsTag, std::function<void(std::shared_ptr<Tag>&, M3U8Info&)>> tagUpdatersMap_;

    double targetDuration_ {0.0};
    bool bLive_ {};
    std::list<std::shared_ptr<M3U8Fragment>> files_;
    uint64_t sequence_ {1}; // default 1
    int discontSequence_ {0};
    std::string playList_;
    void ParseKey(const std::shared_ptr<AttributesTag> &tag);
    void DownloadKey();
    uint32_t SaveData(uint8_t *data, uint32_t len, bool notBlock);
    static void OnDownloadStatus(DownloadStatus status, std::shared_ptr<Downloader> &,
        std::shared_ptr<DownloadRequest> &request);
    bool SetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo);
    void StoreDrmInfos(const std::multimap<std::string, std::vector<uint8_t>>& drmInfo);
    void ProcessDrmInfos(void);
    void ParseMap(const std::shared_ptr<AttributesTag>& tag);
    void DownloadMap(const std::string& uri, size_t offset, size_t length);
    uint32_t SaveMapData(uint8_t* data, uint32_t len, bool notBlock);
    void UpdateDownloadFinished(const std::string& url, const std::string& location);
    void SetDownloadCallback(const std::shared_ptr<DownloadMetricsInfo> &callback);
    void InitDownloadHeader();

    std::shared_ptr<std::string> method_;
    std::shared_ptr<std::string> keyUri_;
    uint8_t iv_[16] { 0 };
    uint8_t key_[16] { 0 };
    size_t keyLen_ { 0 };
    std::shared_ptr<Downloader> downloader_;
    std::shared_ptr<DownloadRequest> downloadRequest_;
    DataSaveFunc dataSave_;
    StatusCallbackFunc statusCallback_;
    PlayListChangeCallback *callback_ { nullptr };
    bool startedDownloadStatus_ { false };
    bool isDecryptAble_ { false };
    bool isDecryptKeyReady_ { false };
    std::multimap<std::string, std::vector<uint8_t>> localDrmInfos_;
    M3U8Info firstFragment_;
    std::atomic<bool> isFirstFragmentReady_ {false};
    bool hasDiscontinuity_ {false};
    std::vector<size_t> segmentOffsets_;
    std::map<std::string, std::string> httpHeader_ {};
    std::atomic<size_t> minFragDuration_ {0};
    std::atomic<bool> isInterruptNeeded_{false};
    uint8_t* fmp4Header_ {nullptr};
    std::shared_ptr<Downloader> downloaderHeader_;
    DataSaveFunc dataSaveHeader_;
    std::atomic<bool> isHeaderReady_ {false};
    std::atomic<bool> isPureByteRange_ {false};
    uint32_t downloadHeaderLen_ {0};
    uint32_t fmp4HeaderLen {0};
    std::shared_ptr<DownloadRequest> downloadHeaderRequest_;
    uint32_t offset_ {0};
    uint32_t length_ {0};
    StatusCallbackFunc monitorStatusCallback_;
    ConditionVariable sleepCond_ {};
    Mutex sleepMutex_ {};
    std::shared_ptr<DownloadMetricsInfo> downloadCallback_ {nullptr};
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader_ {nullptr};
};

struct M3U8Media {
    M3U8Media(const std::string &name, const std::string &uri, std::shared_ptr<M3U8> m3u8);
    M3U8MediaType type_ {M3U8MediaType::M3U8_MEDIA_TYPE_INVALID};
    std::string groupID_;
    std::string name_;
    std::string lang_;
    std::string uri_;
    std::string channels_;
    std::string instreamId_;
    std::string characteristics_;
    bool isDefault_ {false};
    bool autoSelect_ {false};
    bool forced_ {false};
    std::shared_ptr<M3U8> m3u8_ {nullptr};
    uint32_t streamId_ {0};
};

struct M3U8VariantStream {
    M3U8VariantStream(const std::string &name, const std::string &uri, std::shared_ptr<M3U8> m3u8);
    std::string name_;
    std::string uri_;
    std::string codecs_;
    std::string audio_;
    uint64_t bandWidth_ {};
    int programID_ {};
    int width_ {};
    int height_ {};
    bool iframe_ {false};
    std::shared_ptr<M3U8> m3u8_;
    std::list<std::shared_ptr<M3U8Media>> media_;
    uint32_t streamId_ {0};
    std::shared_ptr<M3U8Media> defaultMedia_;
    bool isVideo_ {false};
};

struct M3U8MasterPlaylist {
    M3U8MasterPlaylist(const std::string& playList, const std::string& uri, uint32_t initResolution = 0,
        const std::map<std::string, std::string>& httpHeader = std::map<std::string, std::string>(),
        StatusCallbackFunc statusCallback = [](DownloadStatus, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {});
    void SetSourceloader(std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader);
    void StartParsing();
    void UpdateMediaPlaylist();
    void UpdateMasterPlaylist();
    void ParseMediaStreamInfo(std::shared_ptr<Tag>& tag);
    void BindVideoAudio();
    void GetDefaultAudioStream(const std::shared_ptr<M3U8VariantStream> &videoStream);
    void DownloadSessionKey(std::shared_ptr<Tag>& tag);
    void CreateVariantStream(const std::shared_ptr<Tag>& tag);
    void ChooseStreamByResolution();
    bool IsNearToInitResolution(const std::shared_ptr<M3U8VariantStream> &choosedStream,
    const std::shared_ptr<M3U8VariantStream> &currentStream);
    uint32_t GetResolutionDelta(uint32_t width, uint32_t height);
    void SetInterruptState(bool isInterruptNeeded);
    bool IsVideoStream(const std::string& codecs);
    void ProcessAllTags(std::list<std::shared_ptr<Tag>>& tags);
    void ProcessStreamInfoTag(std::shared_ptr<Tag> tag);
    void SetDownloadCallback(const std::shared_ptr<DownloadMetricsInfo> &callback);
    std::list<std::shared_ptr<M3U8VariantStream>> variants_;
    std::shared_ptr<M3U8VariantStream> defaultVariant_;
    std::shared_ptr<M3U8VariantStream> firstVideoStream_;
    std::string uri_;
    std::string playList_;
    double duration_ {0};
    std::atomic<bool> isSimple_ {false};
    std::atomic<bool> bLive_ {false};
    bool isDecryptAble_ { false };
    bool isDecryptKeyReady_ { false };
    uint8_t iv_[16] { 0 };
    uint8_t key_[16] { 0 };
    size_t keyLen_ { 0 };
    std::atomic<bool> isParseSuccess_ {true};
    bool hasDiscontinuity_ {false};
    std::vector<size_t> segmentOffsets_;
    std::map<std::string, std::string> httpHeader_ {};
    uint32_t initResolution_ {0};
    std::atomic<bool> isInterruptNeeded_{false};
    std::atomic<bool> isFmp4_ {false};
    std::atomic<bool> isPureByteRange_ {false};
    uint32_t defaultStreamId_ {0};
    StatusCallbackFunc monitorStatusCallback_;
    std::list<std::shared_ptr<M3U8Media>> mediaList_;
    std::shared_ptr<DownloadMetricsInfo> downloadCallback_ {nullptr};
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader_ {nullptr};
};
}
}
}
}
#endif
