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
    constexpr int32_t DEFAULT_DECODE_FRAMERATE_UPPER_LIMIT = 120;
}

using MediaSource = OHOS::Media::Plugins::MediaSource;
class BaseStreamDemuxer;
class DataPacker;
class TypeFinder;
class Source;

class AVBufferQueueProducer;

class MediaDemuxer : public std::enable_shared_from_this<MediaDemuxer>, public Plugins::Callback {
public:
    explicit MediaDemuxer();
    ~MediaDemuxer() override;

    Status SetDataSource(const std::shared_ptr<MediaSource> &source);
    void SetBundleName(const std::string& bundleName);
    Status SetOutputBufferQueue(int32_t trackId, const sptr<AVBufferQueueProducer>& producer);

    std::shared_ptr<Meta> GetGlobalMetaInfo();
    std::vector<std::shared_ptr<Meta>> GetStreamMetaInfo() const;
    std::shared_ptr<Meta> GetUserMeta();

    Status SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime);
    Status Reset();
    Status PrepareFrame(bool renderFirstFrame);
    Status Start();
    Status Stop();
    Status Pause();
    Status Resume();
    Status Flush();

    Status StartAudioTask();
    Status SelectTrack(int32_t trackId);
    Status UnselectTrack(int32_t trackId);
    Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample);
    Status GetBitRates(std::vector<uint32_t> &bitRates);
    Status SelectBitRate(uint32_t bitRate);

    Status GetMediaKeySystemInfo(std::multimap<std::string, std::vector<uint8_t>> &infos);
    void SetDrmCallback(const std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> &callback);
    void OnEvent(const Plugins::PluginEvent &event) override;
    std::map<uint32_t, sptr<AVBufferQueueProducer>> GetBufferQueueProducerMap();
    Status PauseTaskByTrackId(int32_t trackId);

    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver);
    bool GetDuration(int64_t& durationMs);
    void SetPlayerId(std::string playerId);

    Status OptimizeDecodeSlow(bool useDecodeSlowOptimization);
    Status SetDecodeFramerateUpperLimit(int32_t decodeFramerateUpperLimit, uint32_t trackId);
    Status SetSpeed(float speed);
    Status SetFrameRate(double frameRate, uint32_t trackId);
    void SetInterruptState(bool isInterruptNeeded);
    void OnDumpInfo(int32_t fd);

    bool IsLocalDrmInfosExisted();
private:
    class DataSourceImpl;

    struct MediaMetaData {
        std::vector<std::shared_ptr<Meta>> trackMetas;
        std::shared_ptr<Meta> globalMeta;
    };

    bool isHttpSource_ = false;
    std::string videoMime_{};
    bool IsContainIdrFrame(const uint8_t* buff, size_t bufSize);

    bool CreatePlugin(std::string pluginName);
    bool InitPlugin(std::string pluginName);

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
    Status ProcessVideoStartTime(uint32_t trackId, std::shared_ptr<AVBuffer> sample);
    void HandleSourceDrmInfoEvent(const std::multimap<std::string, std::vector<uint8_t>> &info);
    Status ReportDrmInfos(const std::multimap<std::string, std::vector<uint8_t>> &info);

    bool HasVideo();
    bool IsBufferDroppable(std::shared_ptr<AVBuffer> sample, uint32_t trackId);

    Plugins::Seekable seekable_;
    std::string uri_;
    uint64_t mediaDataSize_;

    std::string pluginName_;
    std::shared_ptr<Plugins::DemuxerPlugin> plugin_;
    std::shared_ptr<DataSourceImpl> dataSource_;
    std::shared_ptr<MediaSource> mediaSource_;
    std::shared_ptr<Source> source_;
    MediaMetaData mediaMetaData_;

    int64_t ReadLoop(uint32_t trackId);
    Status CopyFrameToUserQueue(uint32_t trackId);
    bool GetBufferFromUserQueue(uint32_t queueIndex, uint32_t size = 0);
    Status InnerReadSample(uint32_t trackId, std::shared_ptr<AVBuffer>);
    Status InnerSelectTrack(int32_t trackId);

    Mutex mapMetaMutex_{};
    std::map<uint32_t, sptr<AVBufferQueueProducer>> bufferQueueMap_;
    std::map<uint32_t, std::shared_ptr<AVBuffer>> bufferMap_;
    std::map<uint32_t, bool> eosMap_;
    std::map<uint32_t, uint32_t> requestBufferErrorCountMap_;
    std::atomic<bool> isThreadExit_ = true;
    bool useBufferQueue_ = false;
    bool isAccurateSeekForHLS_ = false;
    int64_t videoStartTime_{0};

    std::shared_mutex drmMutex{};
    std::multimap<std::string, std::vector<uint8_t>> localDrmInfos_;
    std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> drmCallback_;

    std::map<uint32_t, std::unique_ptr<Task>> taskMap_;
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver_;
    int64_t lastSeekTime_{Plugins::HST_TIME_NONE};
    bool isSeeked_{false};
    uint32_t videoTrackId_{TRACK_ID_DUMMY};
    uint32_t audioTrackId_{TRACK_ID_DUMMY};
    bool firstAudio_{true};

    std::atomic<bool> isStopped_ = false;
    std::atomic<bool> isPaused_ = false;
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer_;
    std::string bundleName_ {};
    std::string playerId_;

    Mutex firstFrameMutex_{};
    ConditionVariable firstFrameCond_;
    uint64_t firstFrameCount_ = 0;
    bool doPrepareFrame_{false};

    std::atomic<bool> useDecodeSlowOptimization_ {false};
    std::atomic<float> speed_ {1.0f};
    std::atomic<double> frameRate_ {0.0};
    std::atomic<int32_t> decodeFramerateUpperLimit_ {DEFAULT_DECODE_FRAMERATE_UPPER_LIMIT};
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DEMUXER_H