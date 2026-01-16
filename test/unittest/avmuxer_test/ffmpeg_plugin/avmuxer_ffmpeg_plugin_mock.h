/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef MUXER_FFMPEG_PLUGIN_MOCK_H
#define MUXER_FFMPEG_PLUGIN_MOCK_H

#include "avmuxer_mock.h"
#include "muxer_plugin.h"

namespace OHOS {
namespace MediaAVCodec {
class AVMuxerFfmpegPluginMock : public AVMuxerMock {
public:
    explicit AVMuxerFfmpegPluginMock(std::shared_ptr<Media::Plugins::DataSink> dataSink,
        std::shared_ptr<Media::Plugins::MuxerPlugin> muxer);
    ~AVMuxerFfmpegPluginMock() = default;
    int32_t Destroy() override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t AddTrack(int32_t &trackIndex, std::shared_ptr<FormatMock> &trackFormat) override;
    int32_t WriteSample(uint32_t trackIndex, const uint8_t *sample, const OH_AVCodecBufferAttr &info) override;
    int32_t WriteSampleBuffer(uint32_t trackIndex, const OH_AVBuffer *sample) override;
    int32_t SetRotation(int32_t rotation) override;
    int32_t SetTimedMetadata() override;
    int32_t SetFormat(std::shared_ptr<FormatMock> &format) override;
    int32_t SetParameter(const std::shared_ptr<Media::Meta> &param);
    int32_t SetUserMeta(const std::shared_ptr<Media::Meta> &userMate);
    int32_t AddTrack(int32_t& trackIndex, const std::shared_ptr<Media::Meta> &trackDesc);
private:
    std::shared_ptr<Media::Plugins::DataSink> dataSink_ = nullptr;
    std::shared_ptr<Media::Plugins::MuxerPlugin> muxer_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MUXER_FFMPEG_PLUGIN_MOCK_H