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

#ifndef VIDEO_DECODER_SAMPLE_H
#define VIDEO_DECODER_SAMPLE_H

#include <memory>
#include <string>
#include <gtest/gtest.h>
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "buffer/avbuffer_queue_define.h"
#include "buffer/avbuffer_queue_producer.h"
#include "common/status.h"
#include "meta/meta.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_event.h"
#include "unittest_log.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Media;

namespace OHOS {
namespace MediaAVCodec {

class VideoDecoderSampleCallback : public CodecCallback {
public:
    void OnError(CodecErrorType errorType, int32_t errorCode) override;

    void OnOutputFormatChanged(const std::shared_ptr<Meta> format) override;

    void OnSurfaceModeDataFilled(std::shared_ptr<AVBuffer> buffer) override;
};

class VideoDecoderSample {
public:
    explicit VideoDecoderSample(std::shared_ptr<MediaCodec> &codec) : codec_(codec);
    ~VideoDecoderSample();
    void SetCodec(std::shared_ptr<MediaCodec> &codec);
    std::shared_ptr<MediaCodec> &GetCodec();

    Status Configure(const std::shared_ptr<Meta> &meta);
    Status SetCodecCallback(std::shared_ptr<CodecCallback> &codecCallback);
    Status SetOutputBufferQueue(std::shared_ptr<AVBufferQueueProducer> &bufferQueueProducer);
    Status SetOutputSurface(sptr<Surface> &surface);

    Status Prepare();
    std::shared_ptr<AVBufferQueueProducer> GetInputBufferQueue();
    sptr<Surface> GetInputSurface();

    Status Start();
    Status Stop();
    Status Flush();
    Status Reset();
    Status Release();
    Status SetParameter(const std::shared_ptr<Meta> &parameter);

    std::shared_ptr<Meta> GetOutputFormat();
    Status NotifyEOS();

private:
    std::shared_ptr<MediaCodec> codec_;
};
} // namespace MediaAVCodec
} // namespace OHOS