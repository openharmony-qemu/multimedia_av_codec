/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef AUDIO_CODEC_INNER_AVBUFFER_UNIT_TEST
#define AUDIO_CODEC_INNER_AVBUFFER_UNIT_TEST

#include <gtest/gtest.h>
#include <atomic>
#include <fstream>
#include <thread>
#include <queue>
#include <string>
#include "avcodec_common.h"
#include "avcodec_audio_codec.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "buffer/avbuffer_queue_define.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {

class AVCodecAudioCodecUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

class AudioDecInnerAvBuffer {
public:
    AudioDecInnerAvBuffer() = default;
    virtual ~AudioDecInnerAvBuffer() = default;
    int32_t RunCase(
        const std::string_view &codecName, const std::string_view &inputPath, const std::string_view &outputPath);
    void InputFunc();
    void OutputFunc();
    void SyncFunc();
    void SyncOutputFunc();
    void EnableAsyncChangePluginTest();
    std::shared_ptr<AVCodecAudioCodec>& GetAudioCodec()
    {
        return audioCodec_;
    }
    std::shared_ptr<Media::Meta>& GetAudioMeta()
    {
        return meta_;
    }
    std::shared_ptr<Media::AVBufferQueue>& GetInnerBufferQueue()
    {
        return innerBufferQueue_;
    }
    sptr<Media::AVBufferQueueConsumer>& GetImplConsumer()
    {
        return implConsumer_;
    }

private:
    int32_t GetInputBufferSize();
    int32_t fileSize_ = 0;
    std::atomic<int32_t> bufferConsumerAvailableCount_ = 0;
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::ifstream> testFile_;
    std::shared_ptr<AVCodecAudioCodec> audioCodec_;
    std::shared_ptr<Media::Meta> meta_;
    std::shared_ptr<Media::AVBufferQueue> innerBufferQueue_;
    sptr<Media::AVBufferQueueConsumer> implConsumer_;
    sptr<Media::AVBufferQueueProducer> mediaCodecProducer_;
    std::unique_ptr<std::ifstream> inputFile_;
    std::unique_ptr<std::ofstream> outputFile_;

    bool enableAsyncChangePluginTest_ = false;
    bool isChangePluginThreadRunning_ = false;
    std::unique_ptr<std::thread> changePluginThread_;

    size_t outputSize_ = 0;
};

class AudioCodecConsumerListener : public OHOS::Media::IConsumerListener {
public:
    explicit AudioCodecConsumerListener(AudioDecInnerAvBuffer *demo);

    void OnBufferAvailable() override;

private:
    AudioDecInnerAvBuffer *demo_;
};

class AVCodecInnerCallback : public MediaCodecCallback {
public:
    AVCodecInnerCallback() = default;
    virtual ~AVCodecInnerCallback() = default;
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AUDIO_CODEC_INNER_AVBUFFER_UNIT_TEST