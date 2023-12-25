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

#ifndef AVCODEC_SAMPLE_VIDEO_PERF_TEST_DECODER_SAMPLE_H
#define AVCODEC_SAMPLE_VIDEO_PERF_TEST_DECODER_SAMPLE_H

#include <mutex>
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <fstream>
#include "video_perf_test_sample_base.h"
#include "video_decoder.h"
#include "iconsumer_surface.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class VideoDecoderPerfTestSample : public VideoPerfTestSampleBase {
public:
    VideoDecoderPerfTestSample() {};
    ~VideoDecoderPerfTestSample() override;

    int32_t Create(SampleInfo sampleInfo) override;
    int32_t Start() override;
    int32_t WaitForDone() override;

private:
    void StartRelease();
    void Release();
    void InputThread();
    void OutputThread();
    bool IsCodecData(const uint8_t *const bufferAddr);
    int32_t ReadOneFrame(CodecBufferInfo &info);
    int32_t CreateWindow(OHNativeWindow *&window);
    void ThreadSleep();
    void DumpOutput(uint8_t *bufferAddr, uint32_t bufferSize);
    void DumpOutput(const CodecBufferInfo &bufferInfo);

    class SurfaceConsumer : public OHOS::IBufferConsumerListener {
    public:
        SurfaceConsumer(OHOS::sptr<OHOS::Surface> surface, VideoDecoderPerfTestSample *sample)
            : surface_(surface), sample_(sample) {};
        void OnBufferAvailable() override;

    private:
        int64_t timestamp_ = 0;
        OHOS::Rect damage_ = {};
        OHOS::sptr<OHOS::Surface> surface_ {nullptr};
        VideoDecoderPerfTestSample *sample_;
    };

    std::unique_ptr<VideoDecoder> videoDecoder_ = nullptr;
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
#endif // AVCODEC_SAMPLE_VIDEO_PERF_TEST_DECODER_SAMPLE_H