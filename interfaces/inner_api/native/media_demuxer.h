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

#ifndef MEDIA_DEMUXER_H
#define MEDIA_DEMUXER_H

#include <atomic>
#include <limits>
#include <string>
#include <shared_mutex>

#include "avcodec_common.h"
#include "buffer/avbuffer.h"
#include "common/media_source.h"
#include "demuxer/data_packer.h"
#include "demuxer/type_finder.h"
#include "filter/filter.h"
#include "meta/media_types.h"
#include "osal/task/autolock.h"
#include "osal/task/task.h"
#include "plugin/plugin_base.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_time.h"
#include "plugin/demuxer_plugin.h"

namespace OHOS {
namespace Media {
namespace {
    constexpr uint32_t TRACK_ID_DUMMY = std::numeric_limits<uint32_t>::max();
}

enum class DemuxerState {
    DEMUXER_STATE_NULL,
    DEMUXER_STATE_PARSE_HEADER,
    DEMUXER_STATE_PARSE_FIRST_FRAME,
    DEMUXER_STATE_PARSE_FRAME
};

using MediaSource = OHOS::Media::Plugins::MediaSource;
class DataPacker;
class TypeFinder;
class Source;

class MediaDemuxer;
class AVBufferQueueProducer;
class PushDataImpl {
public:
    explicit PushDataImpl(std::shared_ptr<MediaDemuxer> demuxer);
    ~PushDataImpl() = default;
    Status PushData(std::shared_ptr<Buffer>& buffer, int64_t offset);
private:
    std::weak_ptr<MediaDemuxer> demuxer_;
};

class MediaDemuxer : public std::enable_shared_from_this<MediaDemuxer>, public Plugins::Callback {
public:
    explicit MediaDemuxer();
    ~MediaDemuxer() override;

    Status SetDataSource(const std::shared_ptr<MediaSource> &source);
    Status SetOutputBufferQueue(int32_t trackId, const sptr<AVBufferQueueProducer>& producer);

    std::shared_ptr<Meta> GetGlobalMetaInfo() const;
    std::vector<std::shared_ptr<Meta>> GetStreamMetaInfo() const;

    Status SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime);
    Status Reset();
    Status Start();
    Status Stop();
    Status Pause();
    Status Resume();
    Status Flush();

    Status SelectTrack(int32_t trackId);
    Status UnselectTrack(int32_t trackId);
    Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample);
    Status GetBitRates(std::vector<uint32_t> &bitRates);
    Status SelectBitRate(uint32_t bitRate);

    Status GetMediaKeySystemInfo(std::multimap<std::string, std::vector<uint8_t>> &infos);
    void SetDrmCallback(const std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> &callback);
    void SetDemuxerState(DemuxerState state);
    void OnEvent(const Plugins::PluginEvent &event) override;

    void PushData(std::shared_ptr<Buffer>& bufferPtr, uint64_t offset);
    void SetEos();

    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver);
    bool GetDuration(int64_t& durationMs);
private:
    class DataSourceImpl;

    struct MediaMetaData {
        std::vector<std::shared_ptr<Meta>> trackMetas;
        std::shared_ptr<Meta> globalMeta;
    };

    bool isHttpSource_ = false;
    std::string videoMime_{};
    bool IsContainIdrFrame(const uint8_t* buff, size_t bufSize);

    void InitTypeFinder();
    bool CreatePlugin(std::string pluginName);
    bool InitPlugin(std::string pluginName);

    void ActivatePullMode();
    void ActivatePushMode();

    void ReportIsLiveStreamEvent();
    void MediaTypeFound(std::string pluginName);
    void InitMediaMetaData(const Plugins::MediaInfo& mediaInfo);
    bool IsOffsetValid(int64_t offset) const;
    std::shared_ptr<Meta> GetTrackMeta(uint32_t trackId);
    void HandleFrame(const AVBuffer& bufferPtr, uint32_t trackId);
    
    Status StopTask(uint32_t trackId);
    Status StopAllTask();
    Status PauseAllTask();
    Status ResumeAllTask();

    bool IsDrmInfosUpdate(const std::multimap<std::string, std::vector<uint8_t>> &info);
    Status ProcessDrmInfos();
    void HandleSourceDrmInfoEvent(const std::multimap<std::string, std::vector<uint8_t>> &info);
    bool IsLocalDrmInfosExisted();
    Status ReportDrmInfos(const std::multimap<std::string, std::vector<uint8_t>> &info);

    Plugins::Seekable seekable_;
    std::string uri_;
    uint64_t mediaDataSize_;
    std::shared_ptr<TypeFinder> typeFinder_;
    std::shared_ptr<DataPacker> dataPacker_ = nullptr;

    std::string pluginName_;
    std::shared_ptr<Plugins::DemuxerPlugin> plugin_;
    std::atomic<DemuxerState> pluginState_{DemuxerState::DEMUXER_STATE_NULL};
    std::shared_ptr<DataSourceImpl> dataSource_;
    std::shared_ptr<MediaSource> mediaSource_;
    std::shared_ptr<Source> source_;
    MediaMetaData mediaMetaData_;

    std::function<bool(uint64_t, size_t)> checkRange_;
    std::function<bool(uint64_t, size_t, std::shared_ptr<Buffer>&)> peekRange_;
    std::function<bool(uint64_t, size_t, std::shared_ptr<Buffer>&)> getRange_;

    bool PullDataWithCache(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr);
    bool PullDataWithoutCache(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr);
    void ReadLoop(uint32_t trackId);
    Status CopyFrameToUserQueue(uint32_t trackId);
    bool GetBufferFromUserQueue(uint32_t queueIndex, uint32_t size = 0);
    Status InnerReadSample(uint32_t trackId, std::shared_ptr<AVBuffer>);
    Status InnerSelectTrack(int32_t trackId);

    Mutex mapMetaMutex_{};
    std::map<uint32_t, sptr<AVBufferQueueProducer>> bufferQueueMap_;
    std::map<uint32_t, std::shared_ptr<AVBuffer>> bufferMap_;
    std::map<uint32_t, bool> eosMap_;
    std::atomic<bool> isThreadExit_ = true;
    bool useBufferQueue_ = false;

    std::shared_mutex drmMutex{};
    std::multimap<std::string, std::vector<uint8_t>> localDrmInfos_;
    std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> drmCallback_;

    std::map<uint32_t, std::unique_ptr<Task>> taskMap_;
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver_;
    int64_t lastSeekTime_{Plugins::HST_TIME_NONE};
    struct CacheData {
        std::shared_ptr<Buffer> data = nullptr;
        uint64_t offset = 0;
    };

    CacheData cacheData_;
    bool isSeeked_{false};
    uint32_t videoTrackId_{TRACK_ID_DUMMY};
    uint32_t audioTrackId_{TRACK_ID_DUMMY};
    bool firstAudio_{true};
    std::atomic<bool> isIgnoreParse_{false};
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DEMUXER_H