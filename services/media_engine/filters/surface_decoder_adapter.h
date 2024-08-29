/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef FILTERS_SURFACE_DECODER_ADAPTER_H
#define FILTERS_SURFACE_DECODER_ADAPTER_H

#include <cstring>
#include <shared_mutex>
#include "surface.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "common/status.h"
#include "common/log.h"
#include "osal/task/task.h"
#include "avcodec_common.h"
#include "osal/task/condition_variable.h"
#include "avcodec_video_decoder.h"

namespace OHOS {
namespace MediaAVCodec {
class ICodecService;
}

namespace Media {
class DecoderAdapterCallback {
public:
    virtual ~DecoderAdapterCallback() = default;
    virtual void OnError(MediaAVCodec::AVCodecErrorType type, int32_t errorCode) = 0;
    virtual void OnOutputFormatChanged(const std::shared_ptr<Meta> &format) = 0;
    virtual void OnBufferEos(int64_t pts) = 0;
};

class SurfaceDecoderAdapter : public std::enable_shared_from_this<SurfaceDecoderAdapter>  {
public:
    explicit SurfaceDecoderAdapter();
    ~SurfaceDecoderAdapter();
public:
    Status Init(const std::string &mime);
    Status Configure(const Format &format);
    sptr<OHOS::Media::AVBufferQueueProducer> GetInputBufferQueue();
    Status SetDecoderAdapterCallback(const std::shared_ptr<DecoderAdapterCallback> &decoderAdapterCallback);
    Status SetOutputSurface(sptr<Surface> surface);
    sptr<Surface> GetInputSurface();
    Status Start();
    Status Stop();
    Status Pause();
    Status Resume();
    Status Flush();
    Status Release();
    Status SetParameter(const Format &format);
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void AcquireAvailableInputBuffer();

    std::shared_ptr<DecoderAdapterCallback> decoderAdapterCallback_;
    int64_t lastBufferPts_ = INT64_MIN;

private:
    void ReleaseBuffer();

    std::shared_ptr<MediaAVCodec::AVCodecVideoDecoder> codecServer_;
    std::shared_ptr<Media::AVBufferQueue> inputBufferQueue_;
    sptr<Media::AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<Media::AVBufferQueueConsumer> inputBufferQueueConsumer_;

    std::shared_ptr<Task> releaseBufferTask_{nullptr};
    std::mutex releaseBufferMutex_;
    std::condition_variable releaseBufferCondition_;
    std::vector<uint32_t> indexs_;
    std::vector<uint32_t> dropIndexs_;
    std::atomic<bool> isThreadExit_ = true;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FILTERS_SURFACE_ENCODER_ADAPTER_H
