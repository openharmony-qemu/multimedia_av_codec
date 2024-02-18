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
#include "codec_listener_stub.h"
#include "i_codec_service.h"
#include "i_standard_codec_service.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecClientCallback;
class CodecClient : public MediaCodecCallback,
                    public AVCodecCallback,
                    public ICodecService,
                    public std::enable_shared_from_this<CodecClient> {
public:
    static std::shared_ptr<CodecClient> Create(const sptr<IStandardCodecService> &ipcProxy);
    explicit CodecClient(const sptr<IStandardCodecService> &ipcProxy);
    ~CodecClient();
    // 业务
    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name,
                 API_VERSION apiVersion = API_VERSION::API_VERSION_10) override;
    int32_t Configure(const Format &format) override;
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
    int32_t GetOutputFormat(Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetParameter(const Format &format) override;
    int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) override;
    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) override;
    int32_t GetInputFormat(Format &format) override;
#ifdef SUPPORT_DRM
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag) override;
#endif

    void AVCodecServerDied();

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;

private:
    int32_t CreateListenerObject();
    void UpdateGeneration();
    void WaitCallbackDone();

    sptr<IStandardCodecService> codecProxy_ = nullptr;
    sptr<CodecListenerStub> listenerStub_ = nullptr;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
    std::shared_ptr<MediaCodecCallback> videoCallback_ = nullptr;
    std::shared_mutex mutex_;
    std::atomic<bool> needUpdateGeneration = true;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_CLIENT_H
