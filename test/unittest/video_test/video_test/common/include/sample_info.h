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

#define ANNEXB_INPUT_ONLY

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
constexpr std::string_view MIME_VIDEO_AVC = "video/avc";
constexpr std::string_view MIME_VIDEO_HEVC = "video/hevc";

constexpr int32_t BITRATE_10M = 10 * 1024 * 1024; // 10Mbps
constexpr int32_t BITRATE_20M = 20 * 1024 * 1024; // 20Mbps
constexpr int32_t BITRATE_30M = 30 * 1024 * 1024; // 30Mbps

enum CodecType {
    VIDEO_DECODER,
    VIDEO_ENCODER,
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
 *   +-----+------------+--------------+
 *   | Bit |     2      |      1       |
 *   +-----+------------+--------------+
 *   |Field| IsAVBuffer | IsBufferMode |
 *   +-----+------------+--------------+
 */
enum CodecRunMode {
    SURFACE_ORIGIN          = (0 << 1) | 0,
    BUFFER_SHARED_MEMORY    = (0 << 1) | 1,
    SURFACE_AVBUFFER        = (1 << 1) | 0,
    BUFFER_AVBUFFER         = (1 << 1) | 1,
};

enum SampleState {
    UNINITIALIZED,
    INITIALIZED,
    STARTED,
    FLUSHED,
    STOPPED,
};

struct DataProducerInfo {
    DataProducerType dataProducerType = DATA_PRODUCER_TYPE_DEMUXER;
    BitstreamType bitstreamType = BITSTREAM_TYPE_ANNEXB;
    OH_AVSeekMode seekMode = SEEK_MODE_PREVIOUS_SYNC;
};

struct SampleInfo {
    CodecType codecType = VIDEO_DECODER;
    std::string inputFilePath;
    std::string codecMime = MIME_VIDEO_AVC.data();
    int32_t videoWidth = 0;
    int32_t videoHeight = 0;
    double frameRate = 0.0;
    int64_t bitrate = 10 * 1024 * 1024; // 10Mbps;

    CodecRunMode codecRunMode = SURFACE_ORIGIN;
    int32_t frameInterval = 0;
    NativeWindow* window = nullptr;
    uint32_t repeatTimes = 1;
    OH_AVPixelFormat pixelFormat = AV_PIXEL_FORMAT_NV12;
    bool isHDRVivid = false;
    bool needDumpOutput = false;
    uint32_t maxFrames = UINT32_MAX;
    uint32_t bitrateMode = VBR;
    DataProducerInfo dataProducerInfo = DataProducerInfo();
    int32_t hevcProfile = HEVC_PROFILE_MAIN;
    int64_t videoDuration = 0;
};

struct CodecBufferInfo {
    uint32_t bufferIndex = 0;
    uintptr_t *buffer = nullptr;
    uint8_t *bufferAddr = nullptr;
    OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAGS_NONE};

    CodecBufferInfo(uint8_t *addr) : bufferAddr(addr) {};
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

class CodecUserData {
public:
    SampleInfo *sampleInfo = nullptr;

    uint32_t inputFrameCount_ = 0;
    std::mutex inputMutex_;
    std::condition_variable inputCond_;
    std::queue<CodecBufferInfo> inputBufferInfoQueue_;

    uint32_t outputFrameCount_ = 0;
    std::mutex outputMutex_;
    std::condition_variable outputCond_;
    std::queue<CodecBufferInfo> outputBufferInfoQueue_;

    void ClearQueue()
    {
        {
            std::unique_lock<std::mutex> lock(inputMutex_);
            auto emptyQueue = std::queue<CodecBufferInfo>();
            inputBufferInfoQueue_.swap(emptyQueue);
        }
        {
            std::unique_lock<std::mutex> lock(outputMutex_);
            auto emptyQueue = std::queue<CodecBufferInfo>();
            outputBufferInfoQueue_.swap(emptyQueue);
        }
    }
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_SAMPLE_INFO_H