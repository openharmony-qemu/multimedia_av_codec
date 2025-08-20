/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef DEMUXER_PLUGIN_FLV_TEST_H
#define DEMUXER_PLUGIN_FLV_TEST_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include "ffmpeg_demuxer_plugin.h"
#include "qos.h"

#define DEFAULT_BUFFSIZE (3840 * 2160 * 3)
using MediaAVBuffer = OHOS::Media::AVBuffer;
using MediaInfo = OHOS::Media::Plugins::MediaInfo;
namespace OHOS {
namespace Media {
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

class DemuxerPluginFlvTest {
public:
    DemuxerPluginFlvTest();
    ~DemuxerPluginFlvTest();
    bool InitWithFlvData(const uint8_t* data, size_t size);
    void RunDemuxerInterfaceFuzz();
private:
    void PrepareDemuxerPlugin(MediaInfo& mediaInfo, size_t& bufferSize, AVBufferWrapper& buffer);
    void OperateDemuxerPlugin(MediaInfo& mediaInfo, size_t bufferSize, AVBufferWrapper& buffer);

    int32_t fd_ = -1;
    size_t dataSize_ = 0;
    std::shared_ptr<Plugins::Ffmpeg::FFmpegDemuxerPlugin> demuxerPlugin_ = nullptr;
};

} // namespace Media
} // namespace OHOS

#endif // DEMUXER_PLUGIN_FLV_TEST_H