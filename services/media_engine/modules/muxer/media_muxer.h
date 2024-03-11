/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_MEDIA_MUXER_H
#define AVCODEC_MEDIA_MUXER_H

#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_define.h"
#include "plugin/muxer_plugin.h"

namespace OHOS {
namespace Media {
class MediaMuxer : public Plugins::Callback {
public:
    MediaMuxer(int32_t appUid, int32_t appPid);
    virtual ~MediaMuxer();
    Status Init(int32_t fd, Plugins::OutputFormat format);
    Status Init(FILE *file, Plugins::OutputFormat format);
    Status SetParameter(const std::shared_ptr<Meta> &param);
    Status SetUserMeta(const std::shared_ptr<Meta> &userMeta);
    Status AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc);
    sptr<AVBufferQueueProducer> GetInputBufferQueue(uint32_t trackIndex);
    Status Start();
    Status WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample);
    Status Stop();
    Status Reset();
    void OnEvent(const Plugins::PluginEvent &event) override;

private:
    enum class State {
        UNINITIALIZED,
        INITIALIZED,
        STARTED,
        STOPPED
    };

    std::shared_ptr<Plugins::MuxerPlugin> CreatePlugin(Plugins::OutputFormat format);
    void StartThread(const std::string &name);
    void StopThread();
    void ThreadProcessor();
    void OnBufferAvailable();
    bool CanAddTrack(const std::string &mimeType);
    bool CheckKeys(const std::string &mimeType, const std::shared_ptr<Meta> &trackDesc);
    std::string StateConvert(State state);

    class Track : public IConsumerListener {
    public:
        Track() {};
        virtual ~Track() {};
        std::shared_ptr<AVBuffer> GetBuffer();
        void ReleaseBuffer();
        void SetBufferAvailableListener(MediaMuxer *listener);
        void OnBufferAvailable() override;

    public:
        int32_t trackId_ = -1;
        std::string mimeType_ = {};
        std::shared_ptr<Meta> trackDesc_ = nullptr;
        sptr<AVBufferQueueProducer> producer_ = nullptr;
        sptr<AVBufferQueueConsumer> consumer_ = nullptr;
        std::shared_ptr<AVBufferQueue> bufferQ_ = nullptr;
        std::shared_ptr<AVBuffer> curBuffer_ = nullptr;

    private:
        std::atomic<int32_t> bufferAvailableCount_ = 0;
        MediaMuxer *listener_ = nullptr;
    };

    int32_t appUid_ = -1;
    int32_t appPid_ = -1;
    Plugins::OutputFormat format_;
    std::atomic<State> state_ = State::UNINITIALIZED;
    std::shared_ptr<Plugins::MuxerPlugin> muxer_ = nullptr;
    std::vector<sptr<Track>> tracks_;
    std::string threadName_;
    std::mutex mutex_;
    std::mutex mutexBufferAvailable_;
    std::condition_variable condBufferAvailable_;
    std::atomic<int32_t> bufferAvailableCount_ = 0;
    std::unique_ptr<std::thread> thread_ = nullptr;
    bool isThreadExit_ = true;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_MEDIA_MUXER_H