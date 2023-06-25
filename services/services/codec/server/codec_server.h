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

#include "codecbase.h"
#include "i_codec_service.h"
#include "nocopyable.h"


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

    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name) override;
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
    int32_t GetOutputFormat(Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetParameter(const Format &format) override;
    int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) override;
    int32_t GetInputFormat(Format &format) override;
    int32_t DumpInfo(int32_t fd);

    void OnError(int32_t errorType, int32_t errorCode);
    void OnOutputFormatChanged(const Format &format);
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer);
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer);

private:
    int32_t InitServer();
    void ExitProcessor();
    const std::string &GetStatusDescription(OHOS::MediaAVCodec::CodecServer::CodecStatus status);
    CodecType GetCodecType();

    CodecStatus status_ = UNINITIALIZED;
    
    // std::unique_ptr<IAVCodecEngine> codecEngine_;
    std::shared_ptr<CodecBase> codecBase_;
    std::shared_ptr<AVCodecCallback> codecCb_;
    std::mutex mutex_;
    std::mutex cbMutex_;
    Format config_;
    std::string lastErrMsg_;
    std::string codecName_;
    bool isFirstFrameIn_ = true;
    bool isFirstFrameOut_ = true;
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
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_SERVER_H