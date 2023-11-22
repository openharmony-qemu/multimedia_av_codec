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
#include <string>
#include "osal/task/task.h"
#include "meta/media_types.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_base.h"
#include "demuxer/data_packer.h"
#include "demuxer/type_finder.h"
#include "source/source.h"
#include "plugin/plugin_info.h"
#include "plugin/demuxer_plugin.h"

namespace OHOS {
namespace Media {
using MediaSource = OHOS::Media::Plugin::MediaSource;
class DataPacker;
class TypeFinder;
class Source;

class MediaDemuxer : public Plugin::Callback {
public:
    explicit MediaDemuxer();

    ~MediaDemuxer() override;

    Status SetDataSource(const std::shared_ptr<MediaSource> &source);

    Status SetOutputBufferQueue(int32_t trackId, const sptr<AVBufferQueueProducer>& producer);

    std::shared_ptr<Meta> GetGlobalMetaInfo() const;

    std::vector<std::shared_ptr<Meta>> GetStreamMetaInfo() const;

    Status SelectTrack(int32_t trackId);

    Status UnselectTrack(int32_t trackId);

    Status SeekTo(int64_t seekTime, Plugin::SeekMode mode, int64_t& realSeekTime);

    Status Reset();

    Status Start();

    Status Stop();

    Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample);

private:
    class DataSourceImpl;

    class PushDataImpl;

    enum class DemuxerState { DEMUXER_STATE_NULL, DEMUXER_STATE_PARSE_HEADER, DEMUXER_STATE_PARSE_FRAME };

    struct MediaMetaData {
        std::vector<std::shared_ptr<Meta>> trackMetas;
        std::shared_ptr<Meta> globalMeta;
    };

    void InitTypeFinder();

    bool CreatePlugin(std::string pluginName);

    bool InitPlugin(std::string pluginName);

    void ActivatePullMode();

    void ActivatePushMode();

    void MediaTypeFound(std::string pluginName);

    void InitMediaMetaData(const Plugin::MediaInfo& mediaInfo);

    bool IsOffsetValid(int64_t offset) const;

    std::shared_ptr<Meta> GetTrackMeta(uint32_t trackId);

    void HandleFrame(const AVBuffer& bufferPtr, uint32_t trackId);

    Status Flush();

    Plugin::Seekable seekable_;
    std::string uri_;
    uint64_t mediaDataSize_;
    std::shared_ptr<TypeFinder> typeFinder_;
    std::shared_ptr<DataPacker> dataPacker_;

    std::string pluginName_;
    std::shared_ptr<Plugin::DemuxerPlugin> plugin_;
    std::atomic<DemuxerState> pluginState_;
    std::shared_ptr<DataSourceImpl> dataSource_;
    std::shared_ptr<MediaSource> mediaSource_;
    std::shared_ptr<Source> source_;
    MediaMetaData mediaMetaData_;

    std::function<bool(uint64_t, size_t)> checkRange_;
    std::function<bool(uint64_t, size_t, std::shared_ptr<Buffer>&)> peekRange_;
    std::function<bool(uint64_t, size_t, std::shared_ptr<Buffer>&)> getRange_;

    std::mutex mutex_ {};
    void ReadLoop();
    Status CopyFrameToUserQueue();
    bool GetBufferFromUserQueue(uint32_t queueIndex, int32_t size = 0);
    Status InnerReadSample(uint32_t trackId, std::shared_ptr<AVBuffer>);
    bool AllTrackReachEOS();

    Status InnerSelectTrack(int32_t trackId);

    std::string threadReadName_;
    std::map<uint32_t, sptr<AVBufferQueueProducer>> bufferQueueMap_;
    std::map<uint32_t, std::shared_ptr<AVBuffer>> bufferMap_;
    std::map<uint32_t, bool> eosMap_;
    std::unique_ptr<std::thread> readThread_ = nullptr;
    std::atomic<bool> isThreadExit_ = true;

    bool useBufferQueue_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DEMUXER_H