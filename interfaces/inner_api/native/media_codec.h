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

#include <string>

#include "codec_plugin.h"

namespace OHOS {
namespace Media {
enum CodecState: int32_t {
    UNINITIALIZED,
    INITIALIZING,
    INITIALIZED,
    CONFIGURING,
    CONFIGURED,
    STARTING,
    STARTED,
    FLUSHING,
    FLUSHED,
    STOPPING,
    RELEASING,
}

enum CodecErrorType: int32_t {
    CODEC_ERROR_INTERNAL,
    CODEC_ERROR_EXTEND_START = 0X10000,
}

class CodecCallback {
public:
    virtual ~CodecCallback() = default;

    virtual void OnError(CodecErrorType errorType, int32_t errorCode) = 0;

    virtual void OnOutputFormatChanged(const std::shared_ptr<Meta> format) = 0;
}

class CodecBaseAdapter;
class AVCodecCallbackAdapter;
class MediaCodec : public DataCallback {
public:
    Status Init(const std::string &mime, bool isEncoder) override;

    Status Init(const std::string &name) override;

    Status Configure(const std::shared_ptr<Meta> meta) override;

    Status SetOutputBufferQueue(std::shared_ptr<AVBufferQueueProducer> bufferQueueProducer) override;

    Status SetCodecCallback(CodecCallback *codecCallback) override;

    Status SetOutputSurface(sptr<Surface> surface) override;

    Status Prepare() override;

    std::shared_ptr<AVBufferQueueProducer> GetInputBufferQueue() override;

    sptr<Surface> GetInputSurface() override;

    Status Start() override;

    Status Stop() override;

    Status Flush() override;

    Status Reset() override;

    Status Release() override;

    Status NotifyEOS() override;

    Status SetParameter(const std::shared_ptr<Meta> parameter) override;

    std::shared_ptr<Meta> GetOutputFormat() override;

private:
    Status PrepareInputBufferQueue() override;

    Status PrepareOutputBufferQueue() override;

    void ProcessInputBuffer() override;

    void OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer) override;

    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) override;

    void OnEvent(const std::shared_ptr<PluginEvent> event) override;

    std::shared_ptr<CodecPlugin> codecPlugin_;
    std::shared_ptr<AVBufferQueue> inputBufferQueue_;
    std::shared_ptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    std::shared_ptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    std::shared_ptr<AVBufferQueueProducer> outputBufferQueueProducer_;
    CodecCallback *codecCallback_;
    AVBufferConfig outputBufferConfig;
    bool isSurfaceMode_ = false;
    bool isBufferMode_ = false;

    std::atomic<CodecState> state_ = CodecState::UNINITIALIZED;

    std::shared_ptr<CodecBaseAdapter> codecAdapter_;
    std::shared_ptr<AVCodecCallbackAdapter> adapterCallback_;
    bool isVideo_;
};

} // namespace Media
} // namespace OHOS
#endif // MODULES_MEDIA_CODEC_H
