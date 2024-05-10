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

#ifndef VIDEO_DECODER_ADAPTER_H
#define VIDEO_DECODER_ADAPTER_H

#include <shared_mutex>
#include <vector>
#include "surface.h"
#include "avcodec_video_decoder.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "osal/task/condition_variable.h"
#include "meta/format.h"
#include "video_sink.h"

namespace OHOS {
namespace Media {
class VideoDecoderAdapter : public std::enable_shared_from_this<VideoDecoderAdapter> {
public:
    VideoDecoderAdapter();
    virtual ~VideoDecoderAdapter();

    Status Init(MediaAVCodec::AVCodecType type, bool isMimeType, const std::string &name);
    Status Configure(const Format &format);
    int32_t SetParameter(const Format &format);
    Status Start();
    Status Flush();
    Status Stop();
    Status Reset();
    Status Release();
    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &callback);

    void PrepareInputBufferQueue();
    sptr<AVBufferQueueProducer> GetBufferQueueProducer();
    sptr<AVBufferQueueConsumer> GetBufferQueueConsumer();

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode);
    void OnOutputFormatChanged(const MediaAVCodec::Format &format);
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    int32_t ReleaseOutputBuffer(uint32_t index, bool render);
    void AquireAvailableInputBuffer();
    int32_t SetOutputSurface(sptr<Surface> videoSurface);
    int32_t GetOutputFormat(Format &format);
    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver>& receiver);

    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag);
    void OnDumpInfo(int32_t fd);

    void SetCallingInfo(int32_t appUid, int32_t appPid, std::string bundleName);

private:
    std::shared_ptr<Media::AVBufferQueue> inputBufferQueue_;
    sptr<Media::AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<Media::AVBufferQueueConsumer> inputBufferQueueConsumer_;

    std::shared_ptr<MediaAVCodec::AVCodecVideoDecoder> mediaCodec_;
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> callback_;
    std::shared_ptr<AVBuffer> buffer_;
    std::string mediaCodecName_;

    std::shared_ptr<Pipeline::EventReceiver> eventReceiver_ {nullptr};

    std::mutex mutex_;
    std::vector<std::shared_ptr<AVBuffer>> bufferVector_;

    bool isConfigured_ {false};

    int32_t appUid_;
    int32_t appPid_;
    std::string bundleName_;
};

class VideoDecoderCallback : public OHOS::MediaAVCodec::MediaCodecCallback {
public:
    explicit VideoDecoderCallback(std::shared_ptr<VideoDecoderAdapter> videoDecoder);
    virtual ~VideoDecoderCallback();

    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode);
    void OnOutputFormatChanged(const MediaAVCodec::Format &format);
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);

private:
    std::weak_ptr<VideoDecoderAdapter> videoDecoderAdapter_;
};
} // namespace Media
} // namespace OHOS
#endif // VIDEO_DECODER_ADAPTER_H
