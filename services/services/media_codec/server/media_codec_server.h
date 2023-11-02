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

#ifndef MEDIA_CODEC_SERVER_H
#define MEDIA_CODEC_SERVER_H

#include <shared_mutex>
#include "codecbase.h"
#include "i_media_codec_service.h"
#include "nocopyable.h"
#include "avcodec_dfx.h"
#include "avbuffer.h"
#include "avbuffer_queue_producer.h"

namespace OHOS {
namespace MediaAVCodec {
class MediaCodecServer : public std::enable_shared_from_this<MediaCodecServer>, public IMediaCodecService, public NoCopyable {
public:
    static std::shared_ptr<IMediaCodecService> Create();
    MediaCodecServer();
    virtual ~MediaCodecServer();

    enum CodecType {
        CODEC_TYPE_DEFAULT = 0,
        CODEC_TYPE_VIDEO,
        CODEC_TYPE_AUDIO
    };

    int32_t Init(bool isEncoder, bool isMimeType, const std::string &name) override;
    int32_t Configure(const Format &format) override;
    int32_t Start() override;
    int32_t Prepare() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t SetCallback(const std::shared_ptr<AVCodecMediaCodecCallback> &callback) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t SetParameter(const Format &format) override;
    sptr<Media::AVBufferQueueProducer> GetInputBufferQueue() override;
    int32_t SetOutputBufferQueue(sptr<Media::AVBufferQueueProducer> bufferQueue) override;
    sptr<Surface> CreateInputSurface() override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t NotifyEos() override;
    int32_t SurfaceModeReturnData(std::shared_ptr<Meida::AVBuffer> buffer, bool available) override;

    int32_t DumpInfo(int32_t fd);
    int32_t SetClientInfo(int32_t clientPid, int32_t clientUid);

    void OnError(int32_t errorType, int32_t errorCode);
    void OnStreamChanged(const Format &format);
    void onSurfaceModeData(std::shared_ptr<Media::AVBuffer> buffer);

private:
    int32_t InitServer();
    const std::string &GetStatusDescription(OHOS::MediaAVCodec::MediaCodecServer::CodecStatus status);
    CodecType GetCodecType();
    int32_t GetCodecDfxInfo(CodecDfxInfo& codecDfxInfo);

    CodecStatus status_ = UNINITIALIZED;
    
    std::shared_ptr<CodecBase> codecBase_;
    std::shared_ptr<AVCodecMediaCodecCallback> codecCb_;
    std::shared_mutex mutex_;
    std::shared_mutex cbMutex_;
    Format config_;
    std::string lastErrMsg_;
    std::string codecName_;
    bool isFirstFrameIn_ = true;
    bool isFirstFrameOut_ = true;
    bool isStarted_ = false;
    uint32_t clientPid_ = 0;
    uint32_t clientUid_ = 0;
    bool isSurfaceMode_ = false;
};

class MediaCodecBaseCallback : public AVCodecMediaCodecCallback, public NoCopyable {
public:
    explicit MediaCodecBaseCallback(const std::shared_ptr<MediaCodecServer> &codec);
    virtual ~MediaCodecBaseCallback();

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnStreamChanged(const Format &format) override;
    void onSurfaceModeData(std::shared_ptr<Media::AVBuffer> buffer) override;
private:
    std::shared_ptr<MediaCodecServer> codec_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_SERVER_H