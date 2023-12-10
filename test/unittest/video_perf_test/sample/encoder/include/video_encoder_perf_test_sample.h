#ifndef AVCODEC_TEST_VIDEO_PERF_TEST_ENCODER_SAMPLE_H
#define AVCODEC_TEST_VIDEO_PERF_TEST_ENCODER_SAMPLE_H

#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <fstream>
#include "video_encoder.h"
#include "sample_info.h"

class VideoEncoderPerfTestSample {
public:
    VideoEncoderPerfTestSample() {};
    ~VideoEncoderPerfTestSample();

    int32_t Create(SampleInfo sampleInfo);
    int32_t Start();
    int32_t WaitForDone();

private:
    void StartRelease();
    void Release();
    void encBufferInputThread();
    void encSurfaceInputThread();
    void encOutputThread();
    int32_t ReadOneFrame(CodecBufferInfo &info);
    int32_t ReadOneFrame(uint8_t *bufferAddr, uint32_t &flags);

    std::unique_ptr<VideoEncoder> videoEncoder_ = nullptr;
    std::unique_ptr<std::thread> encInputThread_ = nullptr;
    std::unique_ptr<std::thread> encOutputThread_ = nullptr;
    std::unique_ptr<std::thread> releaseThread_ = nullptr;
    std::unique_ptr<std::ifstream> inputFile_ = nullptr;

    std::mutex mutex_;
    std::atomic<bool> isStarted_ { false };
    std::condition_variable doneCond_;
    SampleInfo sampleInfo_;
    CodecUserData *encContext_ = nullptr;
};

#endif // AVCODEC_TEST_VIDEO_PERF_TEST_ENCODER_SAMPLE_H