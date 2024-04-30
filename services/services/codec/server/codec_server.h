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

#ifndef CODEC_SERVER_H
#define CODEC_SERVER_H

#include <shared_mutex>
#include <unordered_map>
#include "avcodec_sysevent.h"
#include "codecbase.h"
#include "i_codec_service.h"
#include "nocopyable.h"
#include "codec_drm_decrypt.h"
#include "temporal_scalability.h"
#include "task_thread.h"
#include "codec_param_checker.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecServer : public std::enable_shared_from_this<CodecServer>, public ICodecService, public NoCopyable {
public:
    static std::shared_ptr<ICodecService> Create();
    CodecServer();
    virtual ~CodecServer();

    enum CodecStatus {
        UNINITIALIZED = 0,
        INITIALIZED,
        CONFIGURED,
        RUNNING,
        FLUSHED,
        END_OF_STREAM,
        ERROR,
    };

    enum CodecType {
        CODEC_TYPE_DEFAULT = 0,
        CODEC_TYPE_VIDEO,
        CODEC_TYPE_AUDIO
    };

    typedef struct {
        std::shared_ptr<AVBuffer> inBuf;
        std::shared_ptr<AVBuffer> outBuf;
    } DrmDecryptVideoBuf;

    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name,
                 Meta &callerInfo, API_VERSION apiVersion = API_VERSION::API_VERSION_10) override;
    int32_t Configure(const Format &format) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t NotifyEos() override;
    sptr<Surface> CreateInputSurface() override;
    int32_t SetInputSurface(sptr<Surface> surface);
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    int32_t QueueInputBuffer(uint32_t index) override;
    int32_t QueueInputParameter(uint32_t index) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetParameter(const Format &format) override;
    int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) override;
    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) override;
    int32_t SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback) override;
    int32_t GetInputFormat(Format &format) override;
#ifdef SUPPORT_DRM
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag) override;
#endif
    int32_t DumpInfo(int32_t fd);
    void SetCallerInfo(const Meta &callerInfo);

    void OnError(int32_t errorType, int32_t errorCode);
    void OnOutputFormatChanged(const Format &format);
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer);
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer);

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);

    int32_t Configure(const std::shared_ptr<Media::Meta> &meta) override;
    int32_t SetParameter(const std::shared_ptr<Media::Meta> &parameter) override;
    int32_t GetOutputFormat(std::shared_ptr<Media::Meta> &parameter) override;

    int32_t SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer) override;
    int32_t Prepare() override;
    sptr<Media::AVBufferQueueProducer> GetInputBufferQueue() override;
    void ProcessInputBuffer() override;
    bool GetStatus() override;

#ifdef SUPPORT_DRM
    int32_t SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag) override;
#endif

private:
    int32_t InitServer();
    int32_t CodecScenarioInit(Format &config);
    void StartInputParamTask();
    void ExitProcessor();
    const std::string &GetStatusDescription(OHOS::MediaAVCodec::CodecServer::CodecStatus status);
    void StatusChanged(CodecStatus newStatus);
    CodecType GetCodecType();
    int32_t GetCodecDfxInfo(CodecDfxInfo &codecDfxInfo);
    int32_t DrmVideoCencDecrypt(uint32_t index);
    void SetFreeStatus(bool isFree);
    int32_t QueueInputBufferIn(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag);

    CodecStatus status_ = UNINITIALIZED;

    std::shared_ptr<CodecBase> codecBase_;
    std::shared_ptr<AVCodecCallback> codecCb_;
    std::shared_ptr<MediaCodecCallback> videoCb_;
    std::shared_mutex mutex_;
    std::shared_mutex cbMutex_;
    std::string lastErrMsg_;
    std::string codecName_;
    AVCodecType codecType_ = AVCODEC_TYPE_NONE;
    bool isStarted_ = false;
    struct CallerInfo {
        int32_t pid = -1;
        int32_t uid = -1;
        std::string processName;
    } caller_, forwardCaller_;
    bool isSurfaceMode_ = false;
    bool isModeConfirmed_ = false;
    bool isCreateSurface_ = false;
    bool isSetParameterCb_ = false;
    std::shared_ptr<TemporalScalability> temporalScalability_ = nullptr;
    std::shared_ptr<CodecDrmDecrypt> drmDecryptor_ = nullptr;
    std::unordered_map<uint32_t, DrmDecryptVideoBuf> decryptVideoBufs_;
    std::shared_mutex freeMutex_;
    bool isFree_ = false;
    std::shared_ptr<TaskThread> inputParamTask_ = nullptr;
    CodecScenario scenario_ = CodecScenario::CODEC_SCENARIO_ENC_NORMAL;
};

class CodecBaseCallback : public AVCodecCallback, public NoCopyable {
public:
    explicit CodecBaseCallback(const std::shared_ptr<CodecServer> &codec);
    virtual ~CodecBaseCallback();

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override;

private:
    std::shared_ptr<CodecServer> codec_ = nullptr;
};

class VCodecBaseCallback : public MediaCodecCallback, public NoCopyable {
public:
    explicit VCodecBaseCallback(const std::shared_ptr<CodecServer> &codec);
    virtual ~VCodecBaseCallback();

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;

private:
    std::shared_ptr<CodecServer> codec_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_SERVER_H