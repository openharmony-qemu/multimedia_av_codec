/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_AUDIO_FFMPEG_ENCODER_PLUGIN_H
#define HISTREAMER_AUDIO_FFMPEG_ENCODER_PLUGIN_H

#include <functional>
#include <map>
#include <mutex>
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "ffmpeg_utils.h"
#include "ffmpeg_convert.h"

#include <vector>
#include <memory>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
};
#endif

#define CODEC_API_VERSION (1)
/// End of Stream Buffer Flag
#define BUFFER_FLAG_EOS 0x00000001

namespace OHOS {
namespace Media {
namespace Plugin {
namespace Ffmpeg {
class AudioFfmpegEncoderPlugin : public CodecPlugin {
public:
    explicit AudioFfmpegEncoderPlugin(std::string name);

    ~AudioFfmpegEncoderPlugin();

     Status Init() override;

    // Status Deinit() override;

    Status Prepare() override;

    Status Reset() override;

    Status Start() override;

    Status Stop() override;

    // Status GetParameter(Tag tag, ValueType& value) override;

    Status SetParameter(const std::shared_ptr<Meta> parameter) override;

    Status GetParameter(std::shared_ptr<Meta> parameter) override;

    Status QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer) override;

    Status QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer) override;

    Status GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers) override;

    Status GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers) override;

    Status Flush() override;

    Status Release() override;

    Status SetDataCallback(DataCallback* dataCallback) override
    {
        dataCallback_ = dataCallback;
        return Status::OK;
    }

private:

    Status OpenCtxLocked();

    Status CloseCtxLocked();

    Status StopLocked();

    Status ResetLocked();

    Status DeInitLocked();
/*
    template <typename T>
    Status FindInParameterMapThenAssignLocked(Tag tag, T& assign);
*/
    Status SendBufferLocked(const std::shared_ptr<AVBuffer>& inputBuffer);

    Status ReceiveFrameSucc(const std::shared_ptr<AVBuffer>& ioInfo, const std::shared_ptr<AVPacket>& packet);

    Status ReceiveBuffer();

    Status ReceiveBufferLocked(const std::shared_ptr<AVBuffer>& ioInfo);

    Status SendOutputBuffer();

    bool CheckReformat();

    void FillInFrameCache(const std::shared_ptr<AVMemory>& mem);

    mutable std::mutex parameterMutex_ {};
    Meta audioParameter_ ;

    mutable std::mutex avMutex_ {};
    std::shared_ptr<const AVCodec> avCodec_ {};
    std::shared_ptr<AVCodecContext> avCodecContext_ {};
    std::shared_ptr<AVFrame> cachedFrame_ {};
    std::shared_ptr<AVPacket> avPacket_ {};

    std::vector<uint8_t> paddedBuffer_ {};
    // size_t paddedBufferSize_ {0};
    std::shared_ptr<AVBuffer> outBuffer_ {nullptr};
    DataCallback* dataCallback_ {nullptr};
    // int64_t preBufferGroupPts_ {0};
    // int64_t curBufferGroupPts_ {0};
    // int32_t bufferNum_ {1};
    // int32_t bufferIndex_ {1};
    // int64_t bufferGroupPtsDistance {0};
    uint64_t prev_pts_;
    bool needReformat_ {false};
    mutable std::mutex bufferMetaMutex_ {};
    std::shared_ptr<Meta> bufferMeta_ {nullptr};
    std::shared_ptr<Ffmpeg::Resample> resample_ {nullptr};
    AVSampleFormat srcFmt_ {AVSampleFormat::AV_SAMPLE_FMT_NONE};
    int32_t fullInputFrameSize_ {0};
    int32_t srcBytesPerSample_ {0};

    std::string aacName_;
    int32_t channels_;
    int32_t sampleRate_;
    int64_t bit_rate_;
    int32_t maxInputSize_;
};
} // namespace Ffmpeg

/**
 * @brief Describes the codec plugin information.
 *
 * @since 1.0
 * @version 1.0
 */
struct CodecEncPluginDef : public PluginDefBase {
    CodecMode codecMode {CodecMode::SOFTWARE};
    PluginCreatorFunc<CodecPlugin> creator {nullptr}; ///< Codec plugin create function.
    CodecEncPluginDef()
    {
        apiVersion = CODEC_API_VERSION; ///< Codec plugin version
    }
};

} // namespace Plugin
} // namespace Media
} // namespace OHOS

#endif // HISTREAMER_AUDIO_FFMPEG_ENCODER_PLUGIN_H
