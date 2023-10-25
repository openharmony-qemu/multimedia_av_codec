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

#ifndef MEDIA_MUXER_H
#define MEDIA_MUXER_H

#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "avbuffer_queue_define.h"
#include "avbuffer_queue.h"
#include "muxer_plugin.h"

namespace OHOS {
namespace Media {
class MediaMuxer {
public:
    MediaMuxer(int32_t appUid, int32_t appPid);
    ~MediaMuxer();
    Status Init(int32_t fd, Plugin::OutputFormat format);
    Status SetParameter(const std::shared_ptr<Meta> &param);
    Status AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc);
    std::shared_ptr<AVBufferQueueProducer> GetInputBufferQueue(uint32_t trackIndex);
    Status Start();
    Status Stop();
    Status Reset();

private:
    enum class State {
        UNINITIALIZED,
        INITIALIZED,
        STARTED,
        STOPPED
    };

    bool CanAddTrack(const std::string &mimeType);
    bool CheckKeys(const std::string &mimeType, const std::shared_ptr<Meta> &trackDesc);
    std::string StateConvert(State state);

    class Track : public IAVBufferAvailableListener {
    public:
        Track() default;
        virtual ~Track();
        void Start();
        void Stop() noexcept;
        void ThreadProcessor();
        void SetBufferAvailable(bool isAvailable);
        void OnBufferAvailable(std::shared_ptr<AVBufferQueue>& outBuffer) override;
    public:
        int32_t trackId_ = -1;
        std::string mimeType_ = {};
        const std::shared_ptr<Meta> trackDesc_;
        std::shared_ptr<AVBufferQueueProducer> producer_ = nullptr;
        std::shared_ptr<AVBufferQueueConsumer> consumer_ = nullptr;
        std::shared_ptr<AVBufferQueue> bufferQ_ = nullptr;
        std::shared_ptr<Plugin::MuxerPlugin> muxer_ = nullptr;
    private:
        std::mutex mutex_;
        std::condition_variable condBufferAvailable_;
        std::condition_variable condBufferEmpty_;
        std::atomic<bool> isEmpty_ = true;
        std::atomic<bool> isBufferAvailable_ = false;
        std::unique_ptr<std::thread> thread_ = nullptr;
        bool isThreadExit_ = true;
    }

    int32_t appUid_ = -1;
    int32_t appPid_ = -1;
    int64_t fd_ = -1;
    OutputFormat format_;
    std::atomic<State> state_ = State::UNINITIALIZED;
    std::shared_ptr<Plugin::MuxerPlugin> muxer_ = nullptr;
    std::vector<std::shared_ptr<Track>> tracks_;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif