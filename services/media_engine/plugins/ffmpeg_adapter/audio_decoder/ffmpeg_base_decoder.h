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
#ifndef FFMPEG_BASE_DECODER_H
#define FFMPEG_BASE_DECODER_H

#include <mutex>
#include <fstream>
#include "nocopyable.h"
#include "common/ffmpeg_convert.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "plugin/codec_plugin.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#ifdef __cplusplus
};
#endif

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
class FfmpegBaseDecoder : public NoCopyable {
public:
    FfmpegBaseDecoder();

    ~FfmpegBaseDecoder();

    Status ProcessSendData(const std::shared_ptr<AVBuffer> &inputBuffer);

    Status ProcessReceiveData(std::shared_ptr<AVBuffer> &outBuffer);

    Status Reset();

    Status Release();

    Status Flush();

    Status AllocateContext(const std::string &name);

    Status InitContext(const std::shared_ptr<Meta> &format);

    Status SetBlockAlignContext(const int block_align);

    Status SetBitsPerSampleContext(const int bits_per_sample);

    Status OpenContext();

    void SetMaxInputSize(int32_t setSize);

    std::shared_ptr<Meta> GetFormat() const noexcept;

    std::shared_ptr<AVCodecContext> GetCodecContext() const noexcept;

    std::shared_ptr<AVPacket> GetCodecAVPacket() const noexcept;

    std::shared_ptr<AVFrame> GetCodecCacheFrame() const noexcept;

    Status CloseCtxLocked();

    int32_t GetMaxInputSize() const noexcept;

    bool HasExtraData() const noexcept;

    void SetCallback(DataCallback *callback);

    bool CheckSampleFormat(const std::shared_ptr<Meta> &format, int32_t channels);

    Status ReceiveBuffer(std::shared_ptr<AVBuffer> &outBuffer);

private:
    bool isFirst;
    bool hasExtra_;
    bool currentFrameFormatChanged_;
    int32_t maxInputSize_;
    int64_t nextPts_;
    int64_t inputPts_;
    float durationTime_;
    std::string name_;

    std::shared_ptr<AVCodec> avCodec_;
    std::shared_ptr<AVCodecContext> avCodecContext_;
    std::shared_ptr<AVFrame> cachedFrame_;
    std::shared_ptr<AVPacket> avPacket_;
    std::mutex avMutext_;
    std::mutex parameterMutex_;
    std::shared_ptr<Meta> format_;
    std::vector<uint8_t> frameBuffer_;

    Ffmpeg::Resample resample_;
    bool needResample_;
    AVSampleFormat destFmt_;
    std::shared_ptr<AVFrame> convertedFrame_;
    DataCallback *dataCallback_{nullptr};
    std::vector<uint8_t> config_data;

private:
    Status SendBuffer(const std::shared_ptr<AVBuffer> &inputBuffer);
    Status ReceiveFrameSucc(std::shared_ptr<AVBuffer> &outBuffer);
    Status InitResample();
    Status ConvertPlanarFrame(std::shared_ptr<AVBuffer> &outBuffer);
    void EnableResample(AVSampleFormat destFmt);
    Status SetCodecExtradata(const std::shared_ptr<Meta> &format);
    void CheckFormatChange();
    void SetSampleSikpInfo(const std::shared_ptr<AVBuffer> &inputBuffer);
};
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // FFMPEG_BASE_DECODER_H