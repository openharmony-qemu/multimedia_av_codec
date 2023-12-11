#ifndef AVCODEC_TEST_VIDEO_PERF_TEST_DECODER_SAMPLE_H
#define AVCODEC_TEST_VIDEO_PERF_TEST_DECODER_SAMPLE_H

#include <mutex>
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <fstream>
#include "video_perf_test_sample_base.h"
#include "video_decoder.h"
#include "surface.h"

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
    void decInputThread();
    void decOutputThread();
    bool IsCodecData(const uint8_t *const bufferAddr);
    int32_t ReadOneFrame(CodecBufferInfo &info);
    int32_t CreateWindow(OHNativeWindow *&window);

    std::unique_ptr<VideoDecoder> videoDecoder_ = nullptr;
    std::unique_ptr<std::thread> decInputThread_ = nullptr;
    std::unique_ptr<std::thread> decOutputThread_ = nullptr;
    std::unique_ptr<std::thread> releaseThread_ = nullptr;
    std::unique_ptr<std::ifstream> inputFile_ = nullptr;

    std::mutex mutex_;
    std::atomic<bool> isStarted_ { false };
    std::condition_variable doneCond_;
    SampleInfo sampleInfo_;
    CodecUserData *decContext_ = nullptr;
    OHOS::sptr<OHOS::Surface> surface_;
};

#endif // AVCODEC_TEST_VIDEO_PERF_TEST_DECODER_SAMPLE_H