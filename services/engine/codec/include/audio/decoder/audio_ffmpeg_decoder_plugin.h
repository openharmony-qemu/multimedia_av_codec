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

#ifndef AUDIO_FFMPEG_DECODER_PLUGIN
#define AUDIO_FFMPEG_DECODER_PLUGIN

#include <mutex>
#include "audio_base_codec.h"
#include "nocopyable.h"
#include "audio_resample.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
};
#endif

namespace OHOS {
namespace MediaAVCodec {
class AudioFfmpegDecoderPlugin : public NoCopyable {
public:
    AudioFfmpegDecoderPlugin();

    ~AudioFfmpegDecoderPlugin();

    int32_t ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer);

    int32_t ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer);

    int32_t Reset();

    int32_t Release();

    int32_t Flush();

    int32_t AllocateContext(const std::string &name);

    int32_t InitContext(const Format &format);

    int32_t OpenContext();

    Format GetFormat() const noexcept;

    std::shared_ptr<AVCodecContext> GetCodecContext() const noexcept;

    std::shared_ptr<AVPacket> GetCodecAVPacket() const noexcept;

    std::shared_ptr<AVFrame> GetCodecCacheFrame() const noexcept;

    int32_t CloseCtxLocked();

    int32_t GetMaxInputSize() const noexcept;

    bool HasExtraData() const noexcept;

    void EnableResample(AVSampleFormat destFmt);

    int32_t SetCodecExtradata();

    void DisableNeedResample();

private:
    bool hasExtra_;
    int32_t maxInputSize_;
    int32_t bufferNum_;
    int32_t bufferIndex_;
    int64_t preBufferGroupPts_;
    int64_t curBufferGroupPts_;
    int64_t bufferGroupPtsDistance;
    std::string name_;

    std::shared_ptr<AVCodec> avCodec_;
    std::shared_ptr<AVCodecContext> avCodecContext_;
    std::shared_ptr<AVFrame> cachedFrame_;
    std::shared_ptr<AVPacket> avPacket_;
    std::mutex avMutext_;
    std::mutex parameterMutex_;
    Format format_;
    std::vector<uint8_t> frameBuffer_;

    std::shared_ptr<AudioResample> resample_;
    bool needResample_;
    AVSampleFormat destFmt_;
    std::shared_ptr<AVFrame> convertedFrame_;

private:
    int32_t SendBuffer(const std::shared_ptr<AudioBufferInfo> &inputBuffer);
    int32_t ReceiveBuffer(std::shared_ptr<AudioBufferInfo> &outBuffer);
    int32_t ReceiveFrameSucc(std::shared_ptr<AudioBufferInfo> &outBuffer);
    int32_t InitResample();
    int32_t ConvertPlanarFrame(std::shared_ptr<AudioBufferInfo> &outBuffer);
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif