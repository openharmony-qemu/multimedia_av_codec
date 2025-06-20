/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DEMUXER_PLUGIN_UNIT_TEST_H
#define DEMUXER_PLUGIN_UNIT_TEST_H

#define DEFAULT_BUFFSIZE (3840 * 2160 * 3)
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <map>
#include <sys/stat.h>
#include <fcntl.h>
#include "stream_demuxer.h"
#include "common/media_source.h"
#include "demuxer_plugin_manager.h"
#include "plugin/plugin_manager_v2.h"
#include "ffmpeg_demuxer_plugin.h"

using MediaAVBuffer = OHOS::Media::AVBuffer;
namespace OHOS {
namespace Media {

class DemuxerPluginUnitTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InitResource(const std::string &filePath, std::string pluginName);
    void InitWeakNetworkDemuxerPlugin(
        const std::string& filePath, std::string pluginName, int64_t failOffset, size_t maxFailCount);
    void SetInitValue();
    bool isEOS(std::map<uint32_t, bool>& countFlag);
    bool CheckKeyFrameIndex(std::vector<uint32_t> keyFrameIndexList, const uint32_t frameIndex, const bool isKeyFrame);
    void CountFrames(uint32_t index);
    void SetEosValue();
    void ReadData();
    void RemoveValue();
protected:
    int fd_ = -1;
    std::shared_ptr<OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin> demuxerPlugin_ = nullptr;
    bool initStatus_ = false;
    int32_t nbStreams_ = 0;
    int32_t videoHeight_ = 0;
    int32_t videoWidth_ = 0;
    int32_t numbers_ = 0;
    std::map<uint32_t, int32_t> frames_;
    std::map<uint32_t, int32_t> keyFrames_;
    std::map<uint32_t, bool> eosFlag_;
    MediaInfo mediaInfo_;
    uint32_t flag_;
};

struct AVBufferWrapper {
    std::shared_ptr<MediaAVBuffer> mediaAVBuffer;
    explicit AVBufferWrapper(uint32_t size)
    {
        if (size == 0) {
            size = DEFAULT_BUFFSIZE;
        }
        ptr = std::make_unique<uint8_t []>(size);
        mediaAVBuffer = MediaAVBuffer::CreateAVBuffer(ptr.get(), size, 0);
    }
private:
    AVBufferWrapper() = delete;
    std::unique_ptr<uint8_t []> ptr;
};

class SourceCallback : public Plugins::Callback {
public:
    explicit SourceCallback(std::shared_ptr<DemuxerPluginManager> demuxerPluginManager)
        : demuxerPluginManager_(demuxerPluginManager) {}
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        switch (event.type) {
            case PluginEventType::INITIAL_BUFFER_SUCCESS: {
                demuxerPluginManager_->NotifyInitialBufferingEnd(true);
                break;
            }
            default:
                break;
        }
    }
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager_;
};

class StreamDemuxerPullDataFailMock : public StreamDemuxer {
public:
    StreamDemuxerPullDataFailMock(size_t failOffset, size_t maxFailCount)
        : StreamDemuxer(), failOffset_(failOffset), maxFailCount_(maxFailCount) {}
private:
    Status CallbackReadAt(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
        size_t expectedLen) override
    {
        num += expectedLen;
        if (num > failOffset_ && failCount_ < maxFailCount_) {
            failCount_++;
            return Status::ERROR_AGAIN;
        }
        return StreamDemuxer::CallbackReadAt(streamID, offset, buffer, expectedLen);
    }
    size_t failOffset_ = 0;
    size_t maxFailCount_ = 0;
    size_t failCount_ = 0;
    int num = 0;
};

} // namespace Media
} // namespace OHOS

#endif // DEMUXER_PLUGIN_UNIT_TEST_H