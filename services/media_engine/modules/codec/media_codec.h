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
namespace MediaAVCodec {
enum CodecState: int32_t {
    UNINITIALIZED = 0,
    INITIALIZED,
    CONFIGURING,
    CONFIGURED,
    RUNNING,
    FLUSHED,
    END_OF_STREAM,
    ERROR,
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
class MediaCodec : public DataCallback, public IAVBufferAvailableListener {
public:
    int32_t Init(const std::string &mime, bool isEncoder);
    int32_t Init(const std::string &name);
    int32_t Configure(const std::shared_ptr<Meta> meta);
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    int32_t SetCodecCallback(CodecCallback *codecCallback);
    int32_t SetParameter(const std::shared_ptr<Meta> parameter);

    int32_t SetOutputBufferQueue(std::shared_ptr<AVBufferQueueProducer> bufferQueueProducer);
    std::shared_ptr<AVBufferQueueProducer> GetInputBufferQueue();

    int32_t SetOutputSurface(sptr<Surface> surface);
    sptr<Surface> GetInputSurface();
    std::shared_ptr<Meta> GetOutputFormat();
    int32_t NotifyEOS();

private:
    void OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer) override;
    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) override;
    void OnEvent(const std::shared_ptr<PluginEvent> event) override;

    int32_t PrepareInputBufferQueue();
    int32_t PrepareOutputBufferQueue();
    void ProcessInputBuffer();
    const std::string &GetStatusDescription(const CodecState &state);

    std::shared_ptr<CodecPlugin> codecPlugin_;
    std::shared_ptr<AVBufferQueue> inputBufferQueue_;
    std::shared_ptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    std::shared_ptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    std::shared_ptr<AVBufferQueueProducer> outputBufferQueueProducer_;
    CodecCallback *codecCallback_;
    AVBufferConfig outputBufferConfig;
    bool isSurfaceMode_ = false;
    bool isBufferMode_ = false;

    std::shared_ptr<CodecBaseAdapter> codecAdapter_;
    std::shared_ptr<AVCodecCallbackAdapter> adapterCallback_;
    bool isVideo_;
    std::atomic<CodecState> state_ = CodecState::UNINITIALIZED;
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif // MODULES_MEDIA_CODEC_H
