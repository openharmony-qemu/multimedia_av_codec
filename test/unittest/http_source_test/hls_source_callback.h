/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef HLS_SOURCE_CALLBACK_H
#define HLS_SOURCE_CALLBACK_H

#include "http_source_plugin.h"
#include "source_callback.h"
#include "sniffer_test_support.h"

namespace OHOS::Media::Plugins::HttpPlugin {
// Mirror the upper layer's stream-selection handshake, outside the playlist callback thread.
class HlsSourceCallback : public SourceCallback {
public:
    explicit HlsSourceCallback(std::weak_ptr<HttpSourcePlugin> plugin) : plugin_(std::move(plugin))
    {
        worker_ = std::thread([this] { Run(); });
    }

    ~HlsSourceCallback() override { Stop(); }

    void OnEvent(const Plugins::PluginEvent& event) override
    {
        if (event.type == PluginEventType::STREAM_UPDATE) {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_ = true;
            cv_.notify_all();
        }
    }

    void Stop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
            cv_.notify_all();
        }
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    bool WaitSelected() { return completed_.Wait() && selected_.load(); }
    int32_t VideoId() const { return videoId_.load(); }

private:
    void Run()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!stopping_) {
            cv_.wait(lock, [this] { return stopping_ || pending_; });
            if (stopping_) {
                break;
            }
            pending_ = false;
            lock.unlock();
            SelectStreams();
            lock.lock();
        }
    }

    void SelectStreams()
    {
        auto plugin = plugin_.lock();
        std::vector<StreamInfo> streams;
        if (plugin == nullptr || plugin->GetStreamInfo(streams, true) != Status::OK) {
            completed_.Set();
            return;
        }
        int32_t video = -1; // No selected stream; the IDs below come from the parsed manifest.
        int32_t audio = -1;
        int32_t subtitle = -1;
        for (const auto& stream : streams) {
            if (stream.type == StreamType::VIDEO && video == -1) {
                video = stream.streamId;
            } else if (stream.type == StreamType::AUDIO && audio == -1) {
                audio = stream.streamId;
            } else if (stream.type == StreamType::SUBTITLE && subtitle == -1) {
                subtitle = stream.streamId;
            }
        }
        if (video != -1) {
            plugin->SetDefaultStreamId(video, audio, subtitle);
            videoId_.store(video);
            selected_.store(true);
        }
        completed_.Set();
    }

    std::weak_ptr<HttpSourcePlugin> plugin_;
    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stopping_ {false};
    bool pending_ {false};
    std::atomic<bool> selected_ {false};
    std::atomic<int32_t> videoId_ {-1};
    SnifferTest::Signal completed_;
};
} // namespace OHOS::Media::Plugins::HttpPlugin
#endif
