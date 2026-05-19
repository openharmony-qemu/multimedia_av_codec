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

#ifndef FFMPEG_BASE_ENCODER_H
#define FFMPEG_BASE_ENCODER_H

#include <mutex>
#include <fstream>
#include "nocopyable.h"
#include "common/ffmpeg_convert.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "plugin/codec_plugin.h"
#include "ffmpeg_utils.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/opt.h>
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
};
#endif

/// End of Stream Buffer Flag
#define BUFFER_FLAG_EOS 1

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
class FFmpegBaseEncoder : NoCopyable {
public:
    FFmpegBaseEncoder();
    ~FFmpegBaseEncoder();
    Status ProcessSendData(const std::shared_ptr<AVBuffer> &inputBuffer);
    Status ProcessReceiveData(std::shared_ptr<AVBuffer> &outputBuffer);
    Status Stop();
    Status Reset();
    Status Release();
    Status Flush();

    Status AllocateContext(const std::string &name);
    Status InitContext(const std::shared_ptr<Meta> &format);
    Status OpenContext();
    Status InitFrame();

    std::shared_ptr<AVCodecContext> GetCodecContext() const;
    Status CloseCtxLocked();
    int32_t GetMaxInputSize() const noexcept;
    int32_t GetMaxInputBufferSize() const noexcept;
    void SetCallback(DataCallback *callback);
    void SetPtsMode(int32_t mode);
    void SetFormatConvert(bool enable, AudioSampleFormat srcFormat, AudioSampleFormat dstFormat);

private:
    int32_t maxInputSize_;
    std::shared_ptr<AVCodec> avCodec_;
    std::shared_ptr<AVCodecContext> avCodecContext_;
    std::shared_ptr<AVFrame> cachedFrame_;
    std::shared_ptr<AVPacket> avPacket_;
    mutable std::mutex avMutext_;
    std::mutex parameterMutex_;
    std::shared_ptr<Meta> format_;
    std::shared_ptr<Meta> bufferMeta_ {nullptr};
    int64_t prevPts_;
    int32_t ptsMode_ = DEFAULT_ENCODE_PTS_MODE;
    bool isFirstInputPts_ = true;
    std::vector<uint8_t> pcmCache_;
    int32_t maxInputBufferSize_ = -1;
    bool enableCache_ = false;
    bool isEnableFormatConvert_ = false;
    DataCallback *dataCallback_{nullptr};
    std::shared_ptr<AVBuffer> outBuffer_ {nullptr};

private:
    Status SendBuffer(const std::shared_ptr<AVBuffer> &inputBuffer);
    Status ReceiveBuffer(std::shared_ptr<AVBuffer> &outputBuffer);
    Status ReceivePacketSucc(std::shared_ptr<AVBuffer> &outputBuffer);
    Status SendOutputBuffer(std::shared_ptr<AVBuffer>& outputBuffer);
    Status PcmFillFrame(const std::shared_ptr<AVBuffer> &inputBuffer);
    Status FillFrameFromCache(bool isEos = false);
    Status SendCachedFrames();
    Status SendEosBuffer();
    Status ReAllocateContext();
    bool codecContextValid_;
    uint32_t frameBytes_ = 1;
    mutable std::mutex bufferMetaMutex_ {};
    int32_t channelsBytesPerSample_ {1};
    AudioSampleFormat srcSampleFormat_ = AudioSampleFormat::INVALID_WIDTH;
    AudioSampleFormat dstSampleFormat_ = AudioSampleFormat::INVALID_WIDTH;
    int32_t srcBytesPerSample_ = 1;
    std::vector<uint8_t> convertBuffer_;
};
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // FFMPEG_BASE_ENCODER_H
