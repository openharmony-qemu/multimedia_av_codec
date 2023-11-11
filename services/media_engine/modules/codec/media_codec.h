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

#ifndef MODULES_MEDIA_CODEC_H
#define MODULES_MEDIA_CODEC_H

#include <memory>
#include <string>
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "buffer/avbuffer_queue_define.h"
#include "buffer/avbuffer_queue_producer.h"
#include "codec_plugin.h"
#include "codecbase_adapter.h"
#include "common/status.h"
#include "meta/meta.h"
#include "surface.h"

// #include "plugin/plugin_event.h"
// #include "codec_plugin.h"

namespace OHOS {
namespace Media {
enum CodecState : int32_t {
    UNINITIALIZED = 0,
    INITIALIZED,
    CONFIGURED,
    PREPARED,
    RUNNING,
    FLUSHED,
    END_OF_STREAM,
    ERROR,
    // STOP ----> PREPARED,
    // RESET ----> INITIALIZED,
    // RELEASE ----> UNINITIALIZED,
};

// enum CodecErrorType : int32_t {
//     CODEC_ERROR_INTERNAL,
//     CODEC_ERROR_EXTEND_START = 0X10000,
// };

class CodecCallback {
public:
    virtual ~CodecCallback() = default;

    virtual void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode) = 0;

    virtual void OnOutputFormatChanged(const std::shared_ptr<MediaAVCodec::Format> &format) = 0;

    virtual void OnSurfaceModeDataFilled(std::shared_ptr<AVBuffer> &buffer) = 0;
};

class MediaCodec : public DataCallback, public std::enable_shared_from_this<MediaCodec> {
public:
    Status Init(const std::string &mime, bool isEncoder);
    Status Init(const std::string &name);

    Status Configure(const std::shared_ptr<MediaAVCodec::Format> &meta);
    Status SetCodecCallback(std::shared_ptr<CodecCallback> &codecCallback);
    Status SetOutputBufferQueue(sptr<AVBufferQueueProducer> &bufferQueueProducer);
    Status SetOutputSurface(sptr<Surface> &surface);

    Status Prepare();
    sptr<AVBufferQueueProducer> GetInputBufferQueue();
    sptr<Surface> GetInputSurface();

    Status Start();
    Status Stop();
    Status Flush();
    Status Reset();
    Status Release();
    Status SetParameter(const std::shared_ptr<MediaAVCodec::Format> &parameter);

    std::shared_ptr<MediaAVCodec::Format> GetOutputFormat();
    Status SurfaceModeReturnBuffer(std::shared_ptr<AVBuffer> &buffer, bool available);
    Status NotifyEOS();

private:
    void OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer) override;
    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) override;
    void OnEvent(const std::shared_ptr<Plugin::PluginEvent> event) override;

    Status PrepareInputBufferQueue();
    Status PrepareInputBufferQueueAdapter();
    Status PrepareOutputBufferQueue();
    Status CheckIsEncoder(const std::string &name);
    void ProcessInputBuffer();
    void ProcessOutputBuffer();
    const std::string &GetStatusDescription(const CodecState &state);

    // std::shared_ptr<Plugin::CodecPlugin> codecPlugin_;
    std::shared_ptr<AVBufferQueue> inputBufferQueue_;
    sptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    sptr<AVBufferQueueProducer> outputBufferQueueProducer_;//
    std::shared_ptr<CodecCallback> codecCallback_;

    std::shared_ptr<MediaAVCodec::CodecBaseAdapter> codecAdapter_;//
    std::shared_ptr<MediaAVCodec::AVCodecCallbackAdapter> cbAdapter_;//
    std::mutex mutex_;
    bool isVideo_;
    bool isVideoEncoder_;
    bool isSurfaceMode_ = false;
    bool isVideoEncoderChecked_ = false;
    std::atomic<CodecState> state_ = CodecState::UNINITIALIZED;
};

} // namespace Media
} // namespace OHOS
#endif // MODULES_MEDIA_CODEC_H
