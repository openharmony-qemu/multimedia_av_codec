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

#ifndef HEVC_DECODER_H
#define HEVC_DECODER_H

#include <atomic>
#include <list>
#include <map>
#include <shared_mutex>
#include <tuple>
#include <vector>
#include <optional>
#include <algorithm>
#include "av_common.h"
#include "avcodec_common.h"
#include "avcodec_info.h"
#include "block_queue.h"
#include "codec_utils.h"
#include "codecbase.h"
#include "media_description.h"
#include "fsurface_memory.h"
#include "task_thread.h"
#include "HevcDec_Typedef.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
using AVBuffer = Media::AVBuffer;
using AVAllocator = Media::AVAllocator;
using AVAllocatorFactory = Media::AVAllocatorFactory;
using MemoryFlag = Media::MemoryFlag;
using FormatDataType = Media::FormatDataType;
class HevcDecoder : public CodecBase {
public:
    explicit HevcDecoder(const std::string &name);
    ~HevcDecoder() override;
    int32_t Configure(const Format &format) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t SetParameter(const Format &format) override;
    int32_t GetOutputFormat(Format &format) override;

    int32_t QueueInputBuffer(uint32_t index) override;
    int32_t ReleaseOutputBuffer(uint32_t index) override;
    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t RenderOutputBuffer(uint32_t index) override;
    static int32_t GetCodecCapability(std::vector<CapabilityData> &capaArray);

    struct HBuffer {
    public:
        HBuffer() = default;
        ~HBuffer() = default;

        enum class Owner {
            OWNED_BY_US,
            OWNED_BY_CODEC,
            OWNED_BY_USER,
            OWNED_BY_SURFACE,
        };

        std::shared_ptr<AVBuffer> avBuffer_ = nullptr;
        std::shared_ptr<FSurfaceMemory> sMemory_ = nullptr;
        std::atomic<Owner> owner_ = Owner::OWNED_BY_US;
        int32_t width_ = 0;
        int32_t height_ = 0;
        int32_t bitDepth_ = 8;
    };

private:
    int32_t Initialize();

    using CreateHevcDecoderFuncType = INT32 (*)(HEVC_DEC_HANDLE *phDecoder, HEVC_DEC_INIT_PARAM *pstInitParam);
    using DecodeFuncType = INT32 (*)(HEVC_DEC_HANDLE hDecoder, HEVC_DEC_INARGS *pstInArgs,
                                                  HEVC_DEC_OUTARGS *pstOutArgs);
    using FlushFuncType = INT32 (*)(HEVC_DEC_HANDLE hDecoder, HEVC_DEC_OUTARGS *pstOutArgs);
    using DeleteFuncType = INT32 (*)(HEVC_DEC_HANDLE hDecoder);

    enum struct State : int32_t {
        UNINITIALIZED,
        INITIALIZED,
        CONFIGURED,
        STOPPING,
        RUNNING,
        FLUSHED,
        FLUSHING,
        EOS,
        ERROR,
    };

    enum PixelBitDepth : int32_t {
        BitDepth_8bit = 8,
        BitDepth_10bit = 10,
    };
    
    bool IsActive() const;
    void CalculateBufferSize();
    int32_t AllocateBuffers();
    void InitBuffers();
    void ResetBuffers();
    void ResetData();
    void ReleaseBuffers();
    void StopThread();
    void ReleaseResource();
    int32_t UpdateBuffers(uint32_t index, int32_t bufferSize, uint32_t bufferType);
    int32_t UpdateSurfaceMemory(uint32_t index);
    void SendFrame();
    void RenderFrame();
    void ConfigureSurface(const Format &format, const std::string_view &formatKey, FormatDataType formatType);
    void ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal = 0,
                             int32_t maxVal = INT_MAX);
    void FramePostProcess(std::shared_ptr<HBuffer> &frameBuffer, uint32_t index, int32_t status, int ret);
    int32_t AllocateInputBuffer(int32_t bufferCnt, int32_t inBufferSize);
    int32_t AllocateOutputBuffer(int32_t bufferCnt, int32_t outBufferSize);
    int32_t FillFrameBuffer(const std::shared_ptr<HBuffer> &frameBuffer);
    int32_t CheckFormatChange(uint32_t index, int width, int height, int bitDepth);
    void SetSurfaceParameter(const Format &format, const std::string_view &formatKey, FormatDataType formatType);
    int32_t FlushSurfaceMemory(std::shared_ptr<FSurfaceMemory> &surfaceMemory, int64_t pts);
    int32_t SetSurfaceCfg(int32_t bufferCnt);
    int32_t DecodeFrameOnce();
    void HevcFuncMatch();
    void ReleaseHandle();
    void ConvertDecOutToAVFrame(int32_t bitDepth);
    static int32_t CheckHevcDecLibStatus();

    std::string codecName_;
    std::atomic<State> state_ = State::UNINITIALIZED;

    void* handle_;
    uint32_t decInstanceID_;
    HEVC_DEC_INIT_PARAM initParams_;
    HEVC_DEC_INARGS hevcDecoderInputArgs_;
    HEVC_DEC_OUTARGS hevcDecoderOutpusArgs_;
    HEVC_DEC_HANDLE hevcSDecoder_;
    CreateHevcDecoderFuncType hevcDecoderCreateFunc_;
    DecodeFuncType hevcDecoderDecodecFrameFunc_;
    FlushFuncType hevcDecoderFlushFrameFunc_;
    DeleteFuncType hevcDecoderDeleteFunc_;

    static std::mutex decoderCountMutex_;
    static std::vector<uint32_t> decInstanceIDSet_;
    static std::vector<uint32_t> freeIDSet_;

    Format format_;
    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t inputBufferSize_ = 0;
    int32_t outputBufferSize_ = 0;
    SurfaceControl sInfo_;
    int32_t bitDepth_ = 8;
    // // Receive frame
    std::shared_ptr<AVFrame> cachedFrame_ = nullptr;
    uint8_t *scaleData_[AV_NUM_DATA_POINTERS] = {nullptr};
    int32_t scaleLineSize_[AV_NUM_DATA_POINTERS] = {0};
    std::shared_ptr<Scale> scale_ = nullptr;
    bool isConverted_ = false;
    bool isOutBufSetted_ = false;
    VideoPixelFormat outputPixelFmt_ = VideoPixelFormat::UNKNOWN;
    // // Running
    std::vector<std::shared_ptr<HBuffer>> buffers_[2];
    std::shared_ptr<AVBuffer> outAVBuffer4Surface_ = nullptr;
    std::shared_ptr<BlockQueue<uint32_t>> inputAvailQue_;
    std::shared_ptr<BlockQueue<uint32_t>> codecAvailQue_;
    std::shared_ptr<BlockQueue<uint32_t>> renderAvailQue_;
    sptr<Surface> surface_ = nullptr;
    std::shared_ptr<TaskThread> sendTask_ = nullptr;
    std::shared_ptr<TaskThread> renderTask_ = nullptr;
    std::mutex inputMutex_;
    std::mutex outputMutex_;
    std::mutex decRunMutex_;
    std::mutex surfaceMutex_;
    std::shared_ptr<MediaCodecCallback> callback_;
    std::atomic<bool> isSendEos_ = false;
    std::atomic<bool> isBufferAllocated_ = false;
};

void HevcDecLog(UINT32 channel_id, IHW265VIDEO_ALG_LOG_LEVEL eLevel, INT8 *p_msg, ...);
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // HEVC_DECODER_H