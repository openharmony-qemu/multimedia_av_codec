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

#ifndef AVCODEC_SAMPLE_VIDEO_SAMPLE_BASE_H
#define AVCODEC_SAMPLE_VIDEO_SAMPLE_BASE_H

#include <fstream>
#include <thread>
#include "sample_base.h"
#include "sample_info.h"
#include "video_codec_base.h"
#include "data_producer_base.h"
#include "sample_context.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class VideoSampleBase : public SampleBase {
public:
    ~VideoSampleBase() override;

    int32_t Create(SampleInfo sampleInfo) override;
    int32_t Start() override;

protected:
    virtual int32_t Init();
    virtual int32_t StartThread();
    virtual void Release();
    void StartRelease();
    void DumpOutput(const CodecBufferInfo &bufferInfo);
    void WriteOutputFileWithStrideYUV420(uint8_t *bufferAddr, uint32_t size);
    void PushEosFrame();

    std::unique_ptr<std::thread> releaseThread_ = nullptr;
    std::unique_ptr<std::ofstream> outputFile_ = nullptr;
    std::shared_ptr<DataProducerBase> dataProducer_ = nullptr;
    std::unique_ptr<std::thread> inputThread_ = nullptr;
    std::unique_ptr<std::thread> outputThread_ = nullptr;

    std::shared_ptr<SampleContext> context_ = nullptr;
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_VIDEO_SAMPLE_BASE_H