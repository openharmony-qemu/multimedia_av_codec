/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <functional>
#include <fstream>
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
#include "dma_swap.h"
#include "media_description.h"
#include "fsurface_memory.h"
#include "task_thread.h"
#include "surface_tools.h"
#include "HevcDec_Typedef.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
using AVBuffer = Media::AVBuffer;
using AVAllocator = Media::AVAllocator;
using AVAllocatorFactory = Media::AVAllocatorFactory;
using MemoryFlag = Media::MemoryFlag;
using FormatDataType = Media::FormatDataType;
class HevcDecoder : public CodecBase, public RefBase {
public:
    explicit HevcDecoder(const std::string &name);
    ~HevcDecoder() override;
    int32_t Init(Media::Meta &callerInfo) override;
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
    int32_t NotifyMemoryRecycle() override;
    int32_t NotifyMemoryWriteBack() override;
    static int32_t GetCodecCapability(std::vector<CapabilityData> &capaArray);
    bool IsValid() const { return isValid_; }

    struct HBuffer {
    public:
        HBuffer() = default;
        ~HBuffer() = default;
        std::shared_ptr<AVBuffer> avBuffer = nullptr;
        std::shared_ptr<FSurfaceMemory> sMemory = nullptr;
        std::atomic<Owner> owner_ = Owner::OWNED_BY_US;
        int32_t width = 0;
        int32_t height = 0;
        int32_t bitDepth = BIT_DEPTH8BIT;
        bool hasSwapedOut = false;
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
        FREEZING,
        FROZEN,
    };

    enum PixelBitDepth : int32_t {
        BIT_DEPTH8BIT = 8,
        BIT_DEPTH10BIT = 10,
    };

#ifdef BUILD_ENG_VERSION
    void OpenDumpFile();
    void DumpOutputBuffer(int32_t bitDepth);
    void DumpConvertOut(struct SurfaceInfo &surfaceInfo);
#endif
    bool IsActive() const;
    void CalculateBufferSize();
    int32_t AllocateBuffers();
    void InitBuffers();
    void ResetBuffers();
    void ResetData();
    void ReleaseBuffers();
    void StopThread();
    void ReleaseResource();
    int32_t UpdateOutputBuffer(uint32_t index);
    int32_t UpdateSurfaceMemory(uint32_t index);
    void SendFrame();
    void ConfigureSurface(const Format &format, const std::string_view &formatKey, FormatDataType formatType);
    void ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal = 0,
                             int32_t maxVal = INT_MAX);
    void FindAvailIndex(uint32_t index);
    void FramePostProcess(std::shared_ptr<HBuffer> &frameBuffer, uint32_t index, int32_t status, int ret);
    int32_t AllocateInputBuffer(int32_t bufferCnt, int32_t inBufferSize);
    int32_t AllocateOutputBuffer(int32_t bufferCnt);
    int32_t ClearSurfaceAndSetQueueSize(const sptr<Surface> &surface, int32_t bufferCnt);
    int32_t AllocateOutputBuffersFromSurface(int32_t bufferCnt);
    int32_t FillFrameBuffer(const std::shared_ptr<HBuffer> &frameBuffer);
    int32_t CheckFormatChange(uint32_t index, int width, int height, int bitDepth);
    void SetSurfaceParameter(const Format &format, const std::string_view &formatKey, FormatDataType formatType);
    int32_t ReplaceOutputSurfaceWhenRunning(sptr<Surface> newSurface);
    int32_t SetQueueSize(const sptr<Surface> &surface, uint32_t targetSize);
    int32_t SwitchBetweenSurface(const sptr<Surface> &newSurface);
    int32_t RenderNewSurfaceWithOldBuffer(const sptr<Surface> &newSurface, uint32_t index);
    int32_t FlushSurfaceMemory(std::shared_ptr<FSurfaceMemory> &surfaceMemory, uint32_t index);
    int32_t SetSurfaceCfg();
    int32_t Attach(sptr<SurfaceBuffer> surfaceBuffer);
    int32_t Detach(sptr<SurfaceBuffer> surfaceBuffer);
    void CombineConsumerUsage();
    int32_t DecodeFrameOnce();
    void HevcFuncMatch();
    void ReleaseHandle();
    void InitHevcParams();
    void ConvertDecOutToAVFrame(int32_t bitDepth);
    static int32_t CheckHevcDecLibStatus();
    // surface listener callback
    void RequestBufferFromConsumer();
    GSError BufferReleasedByConsumer(uint64_t surfaceId);
    int32_t RegisterListenerToSurface(const sptr<Surface> &surface);
    void UnRegisterListenerToSurface(const sptr<Surface> &surface);
    void RequestSurfaceBufferThread();
    void StartRequestSurfaceBufferThread();
    void StopRequestSurfaceBufferThread();
    bool RequestSurfaceBufferOnce(uint32_t index);

    // for memory recycle
    int32_t FreezeBuffers(State curState);
    int32_t ActiveBuffers();
    bool CanSwapOut(bool isOutputBuffer, std::shared_ptr<HBuffer> &hBuffer);
    int32_t SwapOutBuffers(bool isOutputBuffer, State curState);
    int32_t SwapInBuffers(bool isOutputBuffer);
    bool disableDmaSwap_ = false;
    int pid_ = -1;

    CallerInfo hevcDecInfo_;
    int32_t instanceId_ = -1;
    std::string decName_;
    std::string codecName_;
    std::atomic<State> state_ = State::UNINITIALIZED;

    void* handle_ = nullptr;
    uint32_t decInstanceID_;
    HEVC_DEC_INIT_PARAM initParams_;
    HEVC_DEC_INARGS hevcDecoderInputArgs_;
    HEVC_DEC_OUTARGS hevcDecoderOutpusArgs_;
    HEVC_DEC_HANDLE hevcSDecoder_ = nullptr;
    CreateHevcDecoderFuncType hevcDecoderCreateFunc_ = nullptr;
    DecodeFuncType hevcDecoderDecodecFrameFunc_ = nullptr;
    FlushFuncType hevcDecoderFlushFrameFunc_ = nullptr;
    DeleteFuncType hevcDecoderDeleteFunc_ = nullptr;

    static std::mutex decoderCountMutex_;
    static std::vector<uint32_t> decInstanceIDSet_;
    static std::vector<uint32_t> freeIDSet_;

    bool isValid_ = true;
    Format format_;
    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t inputBufferSize_ = 0;
    int32_t inputBufferCnt_ = 0;
    int32_t outputBufferCnt_ = 0;
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
    std::vector<std::shared_ptr<AVBuffer>> outAVBuffer4Surface_;
    std::shared_ptr<BlockQueue<uint32_t>> inputAvailQue_;
    std::shared_ptr<BlockQueue<uint32_t>> codecAvailQue_;
    std::shared_ptr<BlockQueue<uint32_t>> renderAvailQue_;
    std::shared_ptr<BlockQueue<uint32_t>> requestSurfaceBufferQue_;
    std::map<uint32_t, std::pair<sptr<SurfaceBuffer>, OHOS::BufferFlushConfig>> renderSurfaceBufferMap_;
    sptr<Surface> surface_ = nullptr;
    std::shared_ptr<TaskThread> sendTask_ = nullptr;
    std::mutex outputMutex_;
    std::mutex decRunMutex_;
    std::mutex surfaceMutex_;
    std::mutex requestBufferMutex_;
    std::condition_variable requestBufferCV_;
    std::condition_variable requestBufferOnceDoneCV_;
    std::shared_ptr<MediaCodecCallback> callback_;
    std::atomic<bool> isSendEos_ = false;
    std::atomic<bool> isBufferAllocated_ = false;
    std::atomic<bool> requestSucceed_ = false;
    std::atomic<bool> requestBufferFinished_ = true;
    std::atomic<bool> requestBufferThreadExit_ = false;
    std::thread mRequestSurfaceBufferThread_;
#ifdef BUILD_ENG_VERSION
    std::shared_ptr<std::ofstream> dumpInFile_ = nullptr;
    std::shared_ptr<std::ofstream> dumpOutFile_ = nullptr;
    std::shared_ptr<std::ofstream> dumpConvertFile_ = nullptr;
#endif
};

void HevcDecLog(UINT32 channelId, IHW265VIDEO_ALG_LOG_LEVEL eLevel, INT8 *pMsg, ...);
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // HEVC_DECODER_H