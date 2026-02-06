/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "gmock/gmock.h"
#include "surface.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "common/status.h"
#include "plugin/plugin_event.h"
#include "plugin/codec_plugin.h"
#include "osal/task/mutex.h"
#include "codec_drm_decrypt.h"

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
    CODEC_DRM_DECRYTION_FAILED,
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

    virtual void OnOutputFormatChanged(const std::shared_ptr<Meta> &format) = 0;
};

class MediaCodec : public Plugins::DataCallback {
public:
    MediaCodec() = default;
    virtual ~MediaCodec() = default;
    MOCK_METHOD(int32_t, Init, (const std::string &mime, bool isEncoder), ());
    MOCK_METHOD(int32_t, Init, (const std::string &name), ());
    MOCK_METHOD(int32_t, Configure, (const std::shared_ptr<Meta> &meta), ());
    MOCK_METHOD(int32_t, SetOutputBufferQueue, (const sptr<AVBufferQueueProducer> &bufferQueueProducer), ());
    MOCK_METHOD(int32_t, SetCodecCallback, (const std::shared_ptr<CodecCallback> &codecCallback), ());
    MOCK_METHOD(int32_t, SetCodecCallback, (const std::shared_ptr<AudioBaseCodecCallback> &codecCallback), ());
    MOCK_METHOD(int32_t, SetOutputSurface, (sptr<Surface> surface), ());
    MOCK_METHOD(int32_t, Prepare, (), ());
    MOCK_METHOD(sptr<AVBufferQueueProducer>, GetInputBufferQueue, (), ());
    MOCK_METHOD(sptr<AVBufferQueueConsumer>, GetInputBufferQueueConsumer, (), ());
    MOCK_METHOD(sptr<AVBufferQueueProducer>, GetOutputBufferQueueProducer, (), ());
    MOCK_METHOD(void, ProcessInputBufferInner, (bool isTriggeredByOutPort, bool isFlushed), ());
    MOCK_METHOD(sptr<Surface>, GetInputSurface, (), ());
    MOCK_METHOD(int32_t, Start, (), ());
    MOCK_METHOD(int32_t, Stop, (), ());
    MOCK_METHOD(int32_t, Flush, (), ());
    MOCK_METHOD(int32_t, Reset, (), ());
    MOCK_METHOD(int32_t, Release, (), ());
    MOCK_METHOD(int32_t, NotifyEos, (), ());
    MOCK_METHOD(int32_t, SetParameter, (const std::shared_ptr<Meta> &parameter), ());
    MOCK_METHOD(int32_t, GetOutputFormat, (std::shared_ptr<Meta> &parameter), ());
    MOCK_METHOD(void, ProcessInputBuffer, (), ());
    MOCK_METHOD(void, SetDumpInfo, (bool isDump, uint64_t instanceId), ());
    MOCK_METHOD(int32_t, SetAudioDecryptionConfig, (const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag), ());
    MOCK_METHOD(Status, ChangePlugin, (const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta), ());
    MOCK_METHOD(void, OnDumpInfo, (int32_t fd), ());

private:
    MOCK_METHOD(std::shared_ptr<Plugins::CodecPlugin>, CreatePlugin,
        (Plugins::PluginType pluginType), ());
    MOCK_METHOD(std::shared_ptr<Plugins::CodecPlugin>, CreatePlugin,
        (const std::string &mime, Plugins::PluginType pluginType), ());
    MOCK_METHOD(Status, AttachBufffer, (), ());
    MOCK_METHOD(Status, AttachDrmBufffer, (std::shared_ptr<AVBuffer> &drmInbuf,
        std::shared_ptr<AVBuffer> &drmOutbuf, uint32_t size), ());
    MOCK_METHOD(Status, DrmAudioCencDecrypt, (std::shared_ptr<AVBuffer> &filledInputBuffer), ());
    MOCK_METHOD(Status, HandleOutputBuffer, (uint32_t eosStatus), ());
    MOCK_METHOD(int32_t, PrepareInputBufferQueue, (), ());
    MOCK_METHOD(int32_t, PrepareOutputBufferQueue, (), ());
    MOCK_METHOD(void, OnInputBufferDone, (const std::shared_ptr<AVBuffer> &inputBuffer), (override));
    MOCK_METHOD(void, OnOutputBufferDone, (const std::shared_ptr<AVBuffer> &outputBuffer), (override));
    MOCK_METHOD(void, OnEvent, (const std::shared_ptr<Plugins::PluginEvent> event), (override));
    MOCK_METHOD(std::string, StateToString, (CodecState state), ());
    MOCK_METHOD(void, ClearBufferQueue, (), ());
    MOCK_METHOD(void, ClearInputBuffer, (), ());
    MOCK_METHOD(void, HandleAudioCencDecryptError, (), ());
    MOCK_METHOD(uint32_t, GetApiVersion, (), ());
    MOCK_METHOD(bool, HandleOutputBufferInner, (Status &ret), ());
    MOCK_METHOD(Status, HandleOutputBufferOnce, (bool &isOutputBufferAvailable, uint32_t eosStatus, bool isSync), ());
    MOCK_METHOD(void, HandleInputBufferInner, (uint32_t &eosStatus, bool &isProcessingNeeded, Status &ret), ());

private:
    std::shared_ptr<Plugins::CodecPlugin> codecPlugin_;
    std::shared_ptr<AVBufferQueue> inputBufferQueue_;
    sptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    sptr<AVBufferQueueProducer> outputBufferQueueProducer_;
    std::weak_ptr<CodecCallback> codecCallback_;
    // callback from north interface(AudioCodec..)
    std::weak_ptr<AudioBaseCodecCallback> mediaCodecCallback_;
    AVBufferConfig outputBufferConfig_;
    bool isEncoder_;
    bool isSurfaceMode_;
    bool isBufferMode_;
    bool isDump_ = false;
    bool isSupportAudioFormatChanged_ = true;
    std::string dumpPrefix_ = "";
    int32_t outputBufferCapacity_;
    std::string codecPluginName_;

    std::atomic<uint32_t> inputBufferEosStatus_ {0};
    std::atomic<bool> isOutputBufferAvailable_ {true};
    std::atomic<CodecState> state_;
    std::shared_ptr<MediaAVCodec::CodecDrmDecrypt> drmDecryptor_ = nullptr;
    std::vector<std::shared_ptr<AVBuffer>> inputBufferVector_;
    std::vector<std::shared_ptr<AVBuffer>> outputBufferVector_;
    Mutex stateMutex_;
};
} // namespace Media
} // namespace OHOS
#endif // MODULES_MEDIA_CODEC_H
