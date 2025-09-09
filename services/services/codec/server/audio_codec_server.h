/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef AUDIO_CODEC_SERVER_H
#define AUDIO_CODEC_SERVER_H

#include <atomic>
#include <shared_mutex>
#include <unordered_map>
#include "codecbase.h"
#include "nocopyable.h"
#include "task_thread.h"
#include "meta/meta.h"
#include "avcodec_info.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
class AudioCodecServer : public std::enable_shared_from_this<AudioCodecServer>,
                    public NoCopyable {
public:
    static std::shared_ptr<AudioCodecServer> Create();
    AudioCodecServer();
    virtual ~AudioCodecServer();

    enum CodecStatus {
        UNINITIALIZED = 0,
        INITIALIZED,
        CONFIGURED,
        RUNNING,
        FLUSHED,
        END_OF_STREAM,
        ERROR,
    };

    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name,
                 Meta &callerInfo, API_VERSION apiVersion = API_VERSION::API_VERSION_10);
    int32_t Configure(const Format &format);
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    int32_t GetChannelId(int32_t &channelId);
    int32_t NotifyEos();
    int32_t SetLowPowerPlayerMode(bool isLpp);
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag);
    int32_t QueueInputBuffer(uint32_t index);
    int32_t QueueInputParameter(uint32_t index);
    int32_t GetOutputFormat(Format &format);
    int32_t ReleaseOutputBuffer(uint32_t index, bool render = false);
    int32_t RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs);
    int32_t SetParameter(const Format &format);
    int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback);
    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback);
    int32_t SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback);
    int32_t SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback);
    int32_t GetInputFormat(Format &format);
    int32_t ChangePlugin(const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta);
    int32_t SetCodecCallback(const std::shared_ptr<MediaCodecCallback> &codecCallback);
    void SetDumpInfo(bool isDump, uint64_t instanceId);

    int32_t SetCustomBuffer(std::shared_ptr<AVBuffer> buffer);
    std::shared_ptr<Media::Meta> GetCallerInfo();

    void OnError(int32_t errorType, int32_t errorCode);
    void OnOutputFormatChanged(const Format &format);
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer);
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer);

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);

    int32_t Configure(const std::shared_ptr<Media::Meta> &meta);
    int32_t SetParameter(const std::shared_ptr<Media::Meta> &parameter);
    int32_t GetOutputFormat(std::shared_ptr<Media::Meta> &parameter);

    int32_t SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer);
    int32_t Prepare();
    sptr<Media::AVBufferQueueProducer> GetInputBufferQueue();
    sptr<Media::AVBufferQueueConsumer> GetInputBufferQueueConsumer();
    sptr<Media::AVBufferQueueProducer> GetOutputBufferQueueProducer();
    void ProcessInputBufferInner(bool isTriggeredByOutPort, bool isFlushed, uint32_t &bufferStatus);
    void ProcessInputBuffer();
    bool CheckRunning();

#ifdef SUPPORT_DRM
    int32_t SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag);
#endif

private:
    int32_t InitByName(Meta &callerInfo, API_VERSION apiVersion);
    int32_t InitByMime(Meta &callerInfo, API_VERSION apiVersion);
    int32_t InitServer();
    void StartInputParamTask();
    const std::string &GetStatusDescription(OHOS::MediaAVCodec::AudioCodecServer::CodecStatus status);
    void StatusChanged(CodecStatus newStatus);
    void SetFreeStatus(bool isFree);
    int32_t QueueInputBufferIn(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag);
    int32_t ReleaseOutputBufferOfCodec(uint32_t index, bool render);

    CodecStatus status_ = UNINITIALIZED;

    std::shared_ptr<CodecBase> codecBase_;
    std::shared_ptr<AVCodecCallback> codecCb_;
    std::shared_ptr<MediaCodecCallback> videoCb_;
    std::shared_mutex mutex_;
    std::shared_mutex cbMutex_;
    std::string lastErrMsg_;
    std::string codecMime_;
    std::string codecName_;
    AVCodecType codecType_ = AVCODEC_TYPE_NONE;
    
    std::shared_mutex freeMutex_;
    bool isFree_ = false;
    std::shared_ptr<TaskThread> inputParamTask_ = nullptr;
    std::shared_ptr<AVCodecCallback> shareBufCallback_ = nullptr;
    std::shared_ptr<MediaCodecCallback> avBufCallback_ = nullptr;
};

class CodecBaseCallback : public AVCodecCallback, public NoCopyable {
public:
    explicit CodecBaseCallback(const std::shared_ptr<AudioCodecServer> &codec);
    virtual ~CodecBaseCallback();

    void OnError(AVCodecErrorType errorType, int32_t errorCode);
    void OnOutputFormatChanged(const Format &format);
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer);
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer);

private:
    std::shared_ptr<AudioCodecServer> codec_ = nullptr;
};

class VCodecBaseCallback : public MediaCodecCallback, public NoCopyable {
public:
    explicit VCodecBaseCallback(const std::shared_ptr<AudioCodecServer> &codec);
    virtual ~VCodecBaseCallback();

    void OnError(AVCodecErrorType errorType, int32_t errorCode);
    void OnOutputFormatChanged(const Format &format);
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);

private:
    std::shared_ptr<AudioCodecServer> codec_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_SERVER_H