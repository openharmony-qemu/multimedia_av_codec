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

#ifndef AVCODEC_SAMPLE_SAMPLE_INFO_H
#define AVCODEC_SAMPLE_SAMPLE_INFO_H

#include <cstdint>
#include <string>
#include <condition_variable>
#include <queue>
#include "native_avcodec_base.h"
#include "native_avcodec_videoencoder.h"
#include "native_avbuffer.h"

#define ANNEXB_INPUT_ONLY 1

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
constexpr int32_t BITRATE_10M = 10 * 1024 * 1024; // 10Mbps
constexpr int32_t BITRATE_20M = 20 * 1024 * 1024; // 20Mbps
constexpr int32_t BITRATE_30M = 30 * 1024 * 1024; // 30Mbps

constexpr double SAMPLE_DEFAULT_FRAMERATE = 999;

enum class SampleType {
    VIDEO_SAMPLE = 0,
    YUV_VIEWER,
};

/*   CodecType description
 *   +-----+-----------+------------+
 *   | Bit |     1     |     0      |
 *   +-----+-----------+------------+
 *   |Field| IsEncoder | IsSoftware |
 *   +-----+-----------+------------+
 */
enum CodecType {
    VIDEO_HW_DECODER = (0 << 1) | 0,
    VIDEO_SW_DECODER = (0 << 1) | 1,
    VIDEO_HW_ENCODER = (1 << 1) | 0,
    VIDEO_SW_ENCODER = (1 << 1) | 1,
};

enum DataProducerType {
    DATA_PRODUCER_TYPE_DEMUXER,
    DATA_PRODUCER_TYPE_BITSTREAM_READER,
    DATA_PRODUCER_TYPE_RAW_DATA_READER,
};

enum BitstreamType {
    BITSTREAM_TYPE_ANNEXB,
    BITSTREAM_TYPE_AVCC,
};

/*   CodecRunMode description
 *   +-----+-------+--------------+
 *   | Bit |   1   |      0       |
 *   +-----+-------+--------------+
 *   |Field| API11 | IsBufferMode |
 *   +-----+-------+--------------+
 */
enum CodecRunMode {
    API10_SURFACE   = (0 << 1) | 0,
    API10_BUFFER    = (0 << 1) | 1,
    API11_SURFACE   = (1 << 1) | 0,
    API11_BUFFER    = (1 << 1) | 1,
};

enum SampleState {
    SAMPLE_STATE_UNINITIALIZED,
    SAMPLE_STATE_INITIALIZED,
    SAMPLE_STATE_STARTED,
    SAMPLE_STATE_FLUSHED,
    SAMPLE_STATE_STOPPED,
};

struct DataProducerInfo {
    DataProducerType dataProducerType = DATA_PRODUCER_TYPE_DEMUXER;
    BitstreamType bitstreamType = BITSTREAM_TYPE_ANNEXB;
    OH_AVSeekMode seekMode = SEEK_MODE_PREVIOUS_SYNC;
};

enum CodecConsumerType {
    CODEC_COMSUMER_TYPE_DEFAULT = 0,
    CODEC_COMSUMER_TYPE_DECODER_RENDER_OUTPUT,
};

enum ThreadSleepMode {
    THREAD_SLEEP_MODE_INPUT_SLEEP = 0,
    THREAD_SLEEP_MODE_OUTPUT_SLEEP,
};

struct SampleInfo {
    CodecType codecType = VIDEO_HW_DECODER;
    std::string inputFilePath;
    std::string codecMime = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    int32_t videoWidth = 1280;
    int32_t videoHeight = 720;
    double frameRate = SAMPLE_DEFAULT_FRAMERATE;
    int64_t bitrate = 10 * 1024 * 1024; // 10Mbps;

    CodecRunMode codecRunMode = API11_SURFACE;
    int32_t frameInterval = -1;
    std::shared_ptr<NativeWindow> window = nullptr;
    int32_t sampleRepeatTimes = 0;
    int32_t demoRepeatTimes = 1;
    OH_AVPixelFormat pixelFormat = AV_PIXEL_FORMAT_NV12;
    bool needDumpInput = false;
    bool needDumpOutput = false;
    uint32_t maxFrames = UINT32_MAX;
    uint32_t bitrateMode = CBR;
    DataProducerInfo dataProducerInfo = DataProducerInfo();
    int32_t profile = AVC_PROFILE_BASELINE;
    int64_t videoDuration = 0;
    std::string outputFilePath;
    CodecConsumerType codecConsumerType = CODEC_COMSUMER_TYPE_DEFAULT;
    ThreadSleepMode threadSleepMode = THREAD_SLEEP_MODE_INPUT_SLEEP;
    int32_t encoderSurfaceMaxInputBuffer = 0;
    int32_t pauseBeforeRunSample = 0;
    int32_t videoStrideWidth = 0;
    int32_t videoSliceHeight = 0;
    int32_t iFrameInterval = 2'000;
    int32_t videoDecoderOutputColorspace = -1;
    SampleType sampleType = SampleType::VIDEO_SAMPLE;
};

struct CodecBufferInfo {
    uint32_t bufferIndex = 0;
    uintptr_t *buffer = nullptr;
    uint8_t *bufferAddr = nullptr;
    OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAGS_NONE};

    CodecBufferInfo(uint8_t *addr) : bufferAddr(addr) {};
    CodecBufferInfo(OH_AVCodecBufferAttr bufferAttr) : attr(bufferAttr) {};
    CodecBufferInfo(uint8_t *addr, int32_t bufferSize)
        : bufferAddr(addr), attr({0, bufferSize, 0, AVCODEC_BUFFER_FLAGS_NONE}) {};
    CodecBufferInfo(uint32_t argBufferIndex, OH_AVMemory *argBuffer, OH_AVCodecBufferAttr argAttr)
        : bufferIndex(argBufferIndex), buffer(reinterpret_cast<uintptr_t *>(argBuffer)), attr(argAttr) {};
    CodecBufferInfo(uint32_t argBufferIndex, OH_AVMemory *argBuffer)
        : bufferIndex(argBufferIndex), buffer(reinterpret_cast<uintptr_t *>(argBuffer)) {};
    CodecBufferInfo(uint32_t argBufferIndex, OH_AVBuffer *argBuffer)
        : bufferIndex(argBufferIndex), buffer(reinterpret_cast<uintptr_t *>(argBuffer))
    {
        OH_AVBuffer_GetBufferAttr(argBuffer, &attr);
    };
};
static inline CodecBufferInfo eosBufferInfo =
    CodecBufferInfo(OH_AVCodecBufferAttr({0, 0, 0, AVCODEC_BUFFER_FLAGS_EOS}));
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_SAMPLE_INFO_H