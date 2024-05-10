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
#include "foundation/multimedia/drm_framework/services/drm_service/ipc/i_keysession_service.h"
#include "foundation/multimedia/av_codec/services/drm_decryptor/codec_drm_decrypt.h"

namespace OHOS {
namespace Media {
enum class CodecState : int32_t {
    UNINITIALIZED,
    INITIALIZED,
    CONFIGURED,
    PREPARED,
    RUNNING,
    FLUSHED,
    END_OF_STREAM,

    INITIALIZING, // RELEASED -> INITIALIZED
    STARTING,     // INITIALIZED -> RUNNING
    STOPPING,     // RUNNING -> INITIALIZED
    FLUSHING,     // RUNNING -> FLUSHED
    RESUMING,     // FLUSHED -> RUNNING
    RELEASING,    // {ANY EXCEPT RELEASED} -> RELEASED

    ERROR,
};

enum class CodecErrorType : int32_t {
    CODEC_ERROR_INTERNAL,
    CODEC_ERROR_EXTEND_START = 0X10000,
};

class CodecCallback {
public:
    virtual ~CodecCallback() = default;

    virtual void OnError(CodecErrorType errorType, int32_t errorCode) = 0;

    virtual void OnOutputFormatChanged(const std::shared_ptr<Meta> &format) = 0;
};

class AudioBaseCodecCallback {
public:
    virtual ~AudioBaseCodecCallback() = default;

    virtual void OnError(CodecErrorType errorType, int32_t errorCode) = 0;

    virtual void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) = 0;
};

class MediaCodec : public Plugins::DataCallback {
public:
    MediaCodec();

    int32_t Init(const std::string &mime, bool isEncoder);

    int32_t Init(const std::string &name);

    int32_t Configure(const std::shared_ptr<Meta> &meta);

    int32_t SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer);

    int32_t SetCodecCallback(const std::shared_ptr<CodecCallback> &codecCallback);

    int32_t SetCodecCallback(const std::shared_ptr<AudioBaseCodecCallback> &codecCallback);

    int32_t SetOutputSurface(sptr<Surface> surface);

    int32_t Prepare();

    sptr<AVBufferQueueProducer> GetInputBufferQueue();

    sptr<Surface> GetInputSurface();

    int32_t Start();

    int32_t Stop();

    int32_t Flush();

    int32_t Reset();

    int32_t Release();

    int32_t NotifyEos();

    int32_t SetParameter(const std::shared_ptr<Meta> &parameter);

    int32_t GetOutputFormat(std::shared_ptr<Meta> &parameter);

    void ProcessInputBuffer();

    int32_t SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag);

    Status ChangePlugin(const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta);

    void OnDumpInfo(int32_t fd);

private:
    std::shared_ptr<Plugins::CodecPlugin> CreatePlugin(Plugins::PluginType pluginType);
    std::shared_ptr<Plugins::CodecPlugin> CreatePlugin(const std::string &mime, Plugins::PluginType pluginType);
    Status AttachBufffer();
    Status AttachDrmBufffer(std::shared_ptr<AVBuffer> &drmInbuf, std::shared_ptr<AVBuffer> &drmOutbuf,
        uint32_t size);
    Status DrmAudioCencDecrypt(std::shared_ptr<AVBuffer> &filledInputBuffer);
    Status HandleOutputBuffer(uint32_t eosStatus);

    int32_t PrepareInputBufferQueue();

    int32_t PrepareOutputBufferQueue();

    void ClearInputBuffer();

    void OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer) override;

    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) override;

    void OnEvent(const std::shared_ptr<Plugins::PluginEvent> event) override;

    std::string StateToString(CodecState state);

private:
    std::shared_ptr<Plugins::CodecPlugin> codecPlugin_;
    std::shared_ptr<AVBufferQueue> inputBufferQueue_;
    sptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    sptr<AVBufferQueueProducer> outputBufferQueueProducer_;
    std::shared_ptr<CodecCallback> codecCallback_;
    std::shared_ptr<AudioBaseCodecCallback> mediaCodecCallback_;
    AVBufferConfig outputBufferConfig_;
    bool isEncoder_;
    bool isSurfaceMode_;
    bool isBufferMode_;
    int32_t outputBufferCapacity_;
    std::string codecPluginName_;

    std::shared_ptr<MediaAVCodec::CodecDrmDecrypt> drmDecryptor_ = nullptr;
    std::atomic<CodecState> state_ ;
    Mutex stateMutex_;
};
} // namespace Media
} // namespace OHOS
#endif // MODULES_MEDIA_CODEC_H
