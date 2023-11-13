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

#ifndef MEDIA_CODEC_SAMPLE_H
#define MEDIA_CODEC_SAMPLE_H

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <queue>
#include <gtest/gtest.h>
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "buffer/avbuffer_queue_define.h"
#include "buffer/avbuffer_queue_producer.h"
#include "common/status.h"
#include "media_codec.h"
#include "meta/meta.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_event.h"

namespace OHOS {
namespace MediaAVCodec {

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Media;

class CodecSample : public std::enable_shared_from_this<CodecSample> {
public:
    explicit CodecSample();
    ~CodecSample();
    bool CreateByName(const std::string &name);
    bool CreateByMime(const std::string &mime);

    Status Configure(const std::shared_ptr<Format> &meta);
    Status SetCodecCallback(std::shared_ptr<CodecCallback> &codecCallback);
    Status SetOutputBufferQueue(std::shared_ptr<AVBufferQueue> &bufferQueue);
    Status SetOutputSurface();

    Status Prepare();
    sptr<AVBufferQueueProducer> GetInputBufferQueue();
    sptr<Surface> GetInputSurface();

    Status Start();
    Status Stop();
    Status Flush();
    Status Reset();
    Status Release();
    Status SetParameter(const std::shared_ptr<Format> &parameter);

    std::shared_ptr<Format> GetOutputFormat();
    Status SurfaceModeReturnBuffer(std::shared_ptr<AVBuffer> &buffer, bool available);
    Status NotifyEOS();

    void SetOutPath(const std::string &path);
    void SetInPath(const std::string &path);
    void SetIsEncoder(bool isEncoder);
    void SetSurfaceModeBuffer(std::shared_ptr<AVBuffer> &buffer);
    std::atomic<bool> isRunning_ = false;
    std::atomic<bool> isReleased_ = false;

private:
    void InputLoopFuncSurface();
    void InputLoopFuncBuffer();
    void OutputLoopFuncSurface();
    void OutputLoopFuncBuffer();

    bool BeforeStart();
    bool AfterStart();
    void ReStartLoopFunc();
    std::shared_ptr<MediaCodec> codec_ = nullptr;
    sptr<AVBufferQueueProducer> inputPd_ = nullptr;
    sptr<AVBufferQueueProducer> outputPd_ = nullptr;
    sptr<AVBufferQueueConsumer> outputCs_ = nullptr;

    std::queue<std::shared_ptr<AVBuffer>> outBufferQueue_;

    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::ofstream> outFile_;
    std::string inPath_;
    std::string outPath_;
    std::string outSurfacePath_;

    sptr<Surface> consumer_ = nullptr;
    sptr<Surface> producer_ = nullptr;

    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::condition_variable cond_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::mutex mutex_;
    std::mutex inMutex_;
    std::mutex outMutex_;

    uint32_t datSize_ = 0;
    uint32_t frameInputCount_ = 0;
    uint32_t frameOutputCount_ = 0;
    bool isFirstFrame_ = true;
    bool isSurfaceMode_ = false;
    bool isEncoder_ = false;
    int64_t time_ = 0;
};

class CodecSampleCallback : public CodecCallback {
public:
    CodecSampleCallback(std::shared_ptr<CodecSample> codec) : codec_(codec) {}
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const std::shared_ptr<Format> &format) override;
    void OnSurfaceModeDataFilled(std::shared_ptr<AVBuffer> &buffer) override;

private:
    int32_t errorNum_ = 0;
    // int32_t frameCount_ = 0;
    std::weak_ptr<CodecSample> codec_;
};

class ConsumerCallback : public IConsumerListener {
public:
    ConsumerCallback(std::shared_ptr<CodecSample> codec) : codec_(codec) {}
    void OnBufferAvailable() override;

private:
    int32_t frameCount_ = 0;
    std::weak_ptr<CodecSample> codec_;
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif