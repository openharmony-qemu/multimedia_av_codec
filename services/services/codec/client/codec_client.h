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

#ifndef CODEC_CLIENT_H
#define CODEC_CLIENT_H

#include <shared_mutex>
#include "avcodec_log.h"
#include "buffer_converter.h"
#include "codec_listener_stub.h"
#include "i_codec_service.h"
#include "i_standard_codec_service.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecClientCallback;
class CodecClient : public MediaCodecCallback,
                    public AVCodecCallback,
                    public MediaCodecParameterCallback,
                    public MediaCodecParameterWithAttrCallback,
                    public ICodecService,
                    public std::enable_shared_from_this<CodecClient> {
public:
    static int32_t Create(const sptr<IStandardCodecService> &ipcProxy, std::shared_ptr<ICodecService> &codec);
    explicit CodecClient(const sptr<IStandardCodecService> &ipcProxy);
    ~CodecClient();
    // 业务
    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name,
                 Media::Meta &callerInfo, API_VERSION apiVersion = API_VERSION::API_VERSION_10) override;
    int32_t Configure(const Format &format) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t NotifyEos() override;
    sptr<Surface> CreateInputSurface() override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    int32_t QueueInputBuffer(uint32_t index) override;
    int32_t QueueInputParameter(uint32_t index) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs) override;
    int32_t SetParameter(const Format &format) override;
    int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) override;
    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) override;
    int32_t SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback) override;
    int32_t SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback) override;
    int32_t GetInputFormat(Format &format) override;
#ifdef SUPPORT_DRM
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag) override;
#endif
    int32_t SetCustomBuffer(std::shared_ptr<AVBuffer> buffer) override;

    void AVCodecServerDied();

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    void OnInputParameterAvailable(uint32_t index, std::shared_ptr<Format> parameter) override;
    void OnInputParameterWithAttrAvailable(uint32_t index, std::shared_ptr<Format> attribute,
                                           std::shared_ptr<Format> parameter) override;

private:
    int32_t CreateListenerObject();
    void UpdateGeneration();
    void UpdateFormat(Format &format);
    void SetNeedListen(const bool needListen);
    void InitLabel(AVCodecType type);
    typedef enum : uint32_t {
        MEMORY_CALLBACK = 1,
        BUFFER_CALLBACK,
        INVALID_CALLBACK,
    } CallbackMode;

    typedef enum : uint32_t {
        CODEC_BUFFER_MODE = 0,
        CODEC_SURFACE_MODE = 1,
        CODEC_SET_PARAMETER_CALLBACK = 1 << 1,
        CODEC_SURFACE_MODE_WITH_SETPARAMETER = CODEC_SURFACE_MODE | CODEC_SET_PARAMETER_CALLBACK,
    } CodecMode;

    bool hasOnceConfigured_ = false;
    uint32_t callbackMode_ = INVALID_CALLBACK;
    uint32_t codecMode_ = CODEC_BUFFER_MODE;
    AVCodecType type_ = AVCODEC_TYPE_NONE;
    sptr<IStandardCodecService> codecProxy_ = nullptr;
    sptr<CodecListenerStub> listenerStub_ = nullptr;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
    std::shared_ptr<MediaCodecCallback> videoCallback_ = nullptr;
    std::shared_ptr<MediaCodecParameterCallback> paramCallback_ = nullptr;
    std::shared_ptr<MediaCodecParameterWithAttrCallback> paramWithAttrCallback_ = nullptr;
    std::shared_ptr<BufferConverter> converter_ = nullptr;

    std::shared_mutex mutex_;
    std::shared_ptr<std::recursive_mutex> syncMutex_ = nullptr;
    std::atomic<bool> needUpdateGeneration_ = true;

    const OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecClient"};
    uint64_t uid_ = 0;
    std::string tag_ = "";
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_CLIENT_H
