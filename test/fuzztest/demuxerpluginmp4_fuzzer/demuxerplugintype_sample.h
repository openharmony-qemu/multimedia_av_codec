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

#ifndef DEMUXER_PLUGIN_TYPE_TEST_H
#define DEMUXER_PLUGIN_TYPE_TEST_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include "common/media_source.h"
#include "ffmpeg_demuxer_plugin.h"
#include "demuxer_plugin_manager.h"
#include "stream_demuxer.h"
#include "plugin/plugin_manager_v2.h"

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

class DemuxerPluginTypeTest {
public:
    DemuxerPluginTypeTest();
    ~DemuxerPluginTypeTest();
    bool InitWithData(const uint8_t* data, size_t size);
    void RunDemuxerInterfaceFuzz();
    void DemuxerPlugintask(MediaInfo& mediaInfo, AVBufferWrapper& buffer);
    const char* testFilePath_ = "/data/test/demuxerpluginmp4.mp4";
    std::string demuxerPluginName_ = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    int32_t videoHeightDefault_ = 1080;
    int32_t videoWidthDefault_ = 1920;
    uint32_t interfaceTimeout_ = 100;
    int64_t seekTimeDefault_ = 1000;

protected:
    std::shared_ptr<DataSourceImpl> dataSource{ nullptr };

private:
    void PrepareDemuxerPlugin(MediaInfo& mediaInfo, size_t& bufferSize, AVBufferWrapper& buffer);
    void OperateDemuxerPlugin(MediaInfo& mediaInfo, size_t bufferSize, AVBufferWrapper& buffer);
    
    int32_t fd_ = -1;
    size_t dataSize_ = 0;
    std::shared_ptr<Plugins::Ffmpeg::FFmpegDemuxerPlugin> demuxerPlugin_ = nullptr;
};

} // namespace Media
} // namespace OHOS

#endif // DEMUXER_PLUGIN_TYPE_TEST_H