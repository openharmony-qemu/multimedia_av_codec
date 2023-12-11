#ifndef AVCODEC_TEST_VIDEO_PERF_TEST_SAMPLE_BASE_H
#define AVCODEC_TEST_VIDEO_PERF_TEST_SAMPLE_BASE_H

#include "sample_info.h"

class VideoPerfTestSampleBase {
public:
    virtual ~VideoPerfTestSampleBase() {};

    virtual int32_t Create(SampleInfo sampleInfo) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t WaitForDone() = 0;
};
#endif // AVCODEC_TEST_VIDEO_PERF_TEST_SAMPLE_BASE_H