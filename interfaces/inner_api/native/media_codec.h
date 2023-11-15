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

#include <cstring>
#include "surface.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "common/status.h"
#include "common/log.h"
#include "plugin/plugin_event.h"
#include "plugin/codec_plugin.h"
#include "osal/task/mutex.h"

namespace OHOS {
namespace Media {
enum CodecState : int32_t {
    UNINITIALIZED,
    INITIALIZED,
    CONFIGURED,
    PREPARED,
    RUNNING,
    FLUSHED,
    END_OF_STREAM,
    ERROR,
};

enum CodecErrorType : int32_t {
    CODEC_ERROR_INTERNAL,
    CODEC_ERROR_EXTEND_START = 0X10000,
};

class CodecCallback {
public:
    virtual ~CodecCallback() = default;

    virtual void OnError(CodecErrorType errorType, int32_t errorCode) = 0;

    virtual void OnOutputFormatChanged(const std::shared_ptr<Meta> &format) = 0;
};

class MediaCodec : public Plugin::DataCallback {
public:
    MediaCodec() = default;

    Status Init(const std::string &mime, bool isEncoder);

    Status Init(const std::string &name);

    Status Configure(const std::shared_ptr<Meta> &meta);

    Status SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer);

    Status SetCodecCallback(const std::shared_ptr<CodecCallback> &codecCallback);

    Status SetOutputSurface(sptr<Surface> surface);

    Status Prepare();

    sptr<AVBufferQueueProducer> GetInputBufferQueue();

    sptr<Surface> GetInputSurface();

    Status Start();

    Status Stop();

    Status Flush();

    Status Reset();

    Status Release();

    Status NotifyEOS();

    Status SetParameter(const std::shared_ptr<Meta> &parameter);

    std::shared_ptr<Meta> GetOutputFormat();

    void ProcessInputBuffer();

private:
    std::shared_ptr<Plugin::CodecPlugin> CreatePlugin(Plugin::PluginType pluginType);

    Status PrepareInputBufferQueue();

    Status PrepareOutputBufferQueue();

    void OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer) override;

    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) override;

    void OnEvent(const std::shared_ptr<Plugin::PluginEvent> event) override;

    std::shared_ptr<CodecPlugin> codecPlugin_;
    std::shared_ptr<AVBufferQueue> inputBufferQueue_;
    sptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    sptr<AVBufferQueueProducer> outputBufferQueueProducer_;
    std::shared_ptr<CodecCallback> codecCallback_;
    AVBufferConfig outputBufferConfig;
    bool isEncoder_ = false;
    bool isSurfaceMode_ = false;
    bool isBufferMode_ = false;

    std::atomic<CodecState> state_ = CodecState::UNINITIALIZED;
    Mutex stateMutex_;
};
} //namespace Media
} //namespace OHOS
#endif // MODULES_MEDIA_CODEC_H
