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

#ifndef FFMPEG_AAC_ENCODER_PLUGIN_H
#define FFMPEG_AAC_ENCODER_PLUGIN_H

#include <functional>
#include <mutex>
#include <vector>
#include <memory>
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "ffmpeg_utils.h"
#include "ffmpeg_convert.h"
#include "ffmpeg_converter.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/audio_fifo.h"
#ifdef __cplusplus
};
#endif

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
class FFmpegAACEncoderPlugin : public CodecPlugin {
public:
    explicit FFmpegAACEncoderPlugin(const std::string& name);

    ~FFmpegAACEncoderPlugin();

    Status Init() override;

    Status Prepare() override;

    Status Reset() override;

    Status Start() override;

    Status Stop() override;

    Status SetParameter(const std::shared_ptr<Meta> &meta) override;

    Status GetParameter(std::shared_ptr<Meta> &meta) override;

    Status QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer) override;

    Status QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer) override;

    Status GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers) override;

    Status GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers) override;

    Status Flush() override;

    Status Release() override;

    Status SetDataCallback(DataCallback *dataCallback) override
    {
        dataCallback_ = dataCallback;
        return Status::OK;
    }

private:
    Status AllocateContext(const std::string &name);
    Status OpenContext();

    std::shared_ptr<AVCodecContext> GetCodecContext() const;
    Status CloseCtxLocked();
    int32_t GetMaxInputSize() const noexcept;
    Status PcmFillFrame(const std::shared_ptr<AVBuffer> &inputBuffer);
    Status PushInFifo(const std::shared_ptr<AVBuffer> &inputBuffer);
    Status ReceiveBuffer(std::shared_ptr<AVBuffer> &outBuffer);
    Status ReceivePacketSucc(std::shared_ptr<AVBuffer> &outBuffer);
    Status SendOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer);
    Status GetAdtsHeader(std::string &adtsHeader, int32_t &headerSize, std::shared_ptr<AVCodecContext> ctx,
                         int aacLength);
    Status InitFrame();
    Status InitContext();
    Status ReAllocateContext();
    bool CheckSampleRate(const int sampleRate);
    bool CheckResample() const;
    bool CheckSampleFormat();
    bool CheckBitRate() const;
    bool CheckFormat();
    bool CheckChannelLayout();
    bool AudioSampleFormat2AVSampleFormat(const AudioSampleFormat &audioFmt, AVSampleFormat &fmt);
    Status SendEncoder(const std::shared_ptr<AVBuffer> &inputBuffer);
    Status GetMetaData(const std::shared_ptr<Meta> &meta);
    Status SendFrameToFfmpeg();

    mutable std::mutex parameterMutex_{};
    Meta audioParameter_;
    bool needResample_;
    bool codecContextValid_;
    mutable std::mutex avMutex_{};
    std::shared_ptr<const AVCodec> avCodec_{};
    std::shared_ptr<AVCodecContext> avCodecContext_{};
    AVAudioFifo *fifo_;
    std::shared_ptr<AVFrame> cachedFrame_{};
    std::shared_ptr<AVPacket> avPacket_{};

    std::vector<uint8_t> paddedBuffer_{};
    size_t paddedBufferSize_{0};
    std::shared_ptr<AVBuffer> outBuffer_{nullptr};
    DataCallback *dataCallback_{nullptr};
    int64_t preBufferGroupPts_{0};
    int64_t curBufferGroupPts_{0};
    int32_t bufferNum_{1};
    int32_t bufferIndex_{1};
    int64_t bufferGroupPtsDistance{0};
    int64_t prevPts_;
    bool needReformat_{false};
    mutable std::mutex bufferMetaMutex_{};
    std::shared_ptr<Meta> bufferMeta_{nullptr};
    std::shared_ptr<Ffmpeg::Resample> resample_{nullptr};
    AVSampleFormat srcFmt_{AVSampleFormat::AV_SAMPLE_FMT_NONE};
    AudioSampleFormat audioSampleFormat_;
    AudioChannelLayout srcLayout_;
    uint32_t fullInputFrameSize_{0};
    uint32_t srcBytesPerSample_{0};

    std::string aacName_;
    int32_t channels_;
    int32_t sampleRate_;
    int64_t bitRate_;
    int32_t maxInputSize_;
    int32_t maxOutputSize_;
    FILE *outfile;
};
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // FFMPEG_AAC_ENCODER_PLUGIN_H
