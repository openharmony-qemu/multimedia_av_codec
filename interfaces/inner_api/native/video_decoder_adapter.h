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

    int32_t Init(MediaAVCodec::AVCodecType type, bool isMimeType, const std::string &name);
    int32_t Configure(const Format &format);
    int32_t SetParameter(const Format &format);
    int32_t Start();
    int32_t Pause();
    int32_t Flush();
    int32_t Resume();
    int32_t Stop();
    int32_t Reset();
    int32_t Release();
    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &callback);

    sptr<OHOS::Media::AVBufferQueueProducer> GetInputBufferQueue();
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode);
    void OnOutputFormatChanged(const MediaAVCodec::Format &format);
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    int32_t ReleaseOutputBuffer(uint32_t index, std::shared_ptr<Pipeline::VideoSink> videoSink,
        std::shared_ptr<AVBuffer> &outputBuffer, bool doSync);
    void AquireAvailableInputBuffer();
    int32_t SetOutputSurface(sptr<Surface> videoSurface);
    int32_t GetOutputFormat(Format &format);
    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver>& receiver);

    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag);

private:
    void RenderLoop();
    std::shared_ptr<Media::AVBufferQueue> inputBufferQueue_;
    sptr<Media::AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<Media::AVBufferQueueConsumer> inputBufferQueueConsumer_;

    std::shared_ptr<MediaAVCodec::AVCodecVideoDecoder> mediaCodec_;
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> callback_;
    std::shared_ptr<AVBuffer> buffer_;

    std::unique_ptr<std::thread> readThread_ = nullptr;
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver_ {nullptr};

    std::condition_variable condBufferAvailable_;
    std::list<std::function<void()>> indexs_;
    std::mutex mutex_;
    std::atomic<bool> isThreadExit_ = true;
    std::atomic<bool> isPaused_ = false;
    std::vector<std::shared_ptr<AVBuffer>> bufferVector_;
};

class AVBufferAvailableListener : public OHOS::Media::IConsumerListener {
public:
    AVBufferAvailableListener(std::shared_ptr<VideoDecoderAdapter> videoDecoder);
    virtual ~AVBufferAvailableListener();

    void OnBufferAvailable();
private:
    std::weak_ptr<VideoDecoderAdapter> videoDecoder_;
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