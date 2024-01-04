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

#ifndef AVCODEC_SAMPLE_VIDEO_PERF_TEST_ENCODER_SAMPLE_H
#define AVCODEC_SAMPLE_VIDEO_PERF_TEST_ENCODER_SAMPLE_H

#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <fstream>
#include "video_perf_test_sample_base.h"
#include "video_encoder.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class VideoEncoderPerfTestSample : public VideoPerfTestSampleBase {
public:
    VideoEncoderPerfTestSample() {};
    ~VideoEncoderPerfTestSample() override;

    int32_t Create(SampleInfo sampleInfo) override;
    int32_t Start() override;
    int32_t WaitForDone() override;

private:
    void StartRelease();
    void Release();
    void BufferInputThread();
    void SurfaceInputThread();
    void OutputThread();
    int32_t GetBufferSize();
    int32_t ReadOneFrame(CodecBufferInfo &info);
    int32_t ReadOneFrame(uint8_t *bufferAddr, int32_t &bufferSize, uint32_t &flags);
    void AddSurfaceInputTrace(uint64_t pts);
    void ThreadSleep();
    void DumpOutput(const CodecBufferInfo &bufferInfo);

    std::unique_ptr<VideoEncoder> videoEncoder_ = nullptr;
    std::unique_ptr<std::thread> inputThread_ = nullptr;
    std::unique_ptr<std::thread> outputThread_ = nullptr;
    std::unique_ptr<std::thread> releaseThread_ = nullptr;
    std::unique_ptr<std::ifstream> inputFile_ = nullptr;
    std::unique_ptr<std::ofstream> outputFile_ = nullptr;

    std::mutex mutex_;
    std::atomic<bool> isStarted_ { false };
    std::condition_variable doneCond_;
    SampleInfo sampleInfo_;
    CodecUserData *context_ = nullptr;
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_VIDEO_PERF_TEST_ENCODER_SAMPLE_H