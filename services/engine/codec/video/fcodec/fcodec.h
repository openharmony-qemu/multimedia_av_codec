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

#ifndef FCODEC_H
#define FCODEC_H

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
#include "surface_tools.h"
#include "task_thread.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
using AVBuffer = Media::AVBuffer;
using AVAllocator = Media::AVAllocator;
using AVAllocatorFactory = Media::AVAllocatorFactory;
using MemoryFlag = Media::MemoryFlag;
using FormatDataType = Media::FormatDataType;
class FCodec : public CodecBase, public RefBase {
public:
    explicit FCodec(const std::string &name);
    ~FCodec() override;
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

private:
    static void GetBaseCapabilityData(CapabilityData &capsData);
    static void GetCapabilityData(CapabilityData &capsData, uint32_t index);
    int32_t Initialize();
    struct FBuffer {
    public:
        FBuffer() = default;
        ~FBuffer() = default;
        std::shared_ptr<AVBuffer> avBuffer_ = nullptr;
        std::shared_ptr<FSurfaceMemory> sMemory_ = nullptr;
        std::atomic<Owner> owner_ = Owner::OWNED_BY_US;
        int32_t width_ = 0;
        int32_t height_ = 0;
        bool hasSwapedOut_ = false;
    };

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
    void DumpOutputBuffer();
    bool IsActive() const;
    void ResetContext(bool isFlush = false);
    void CalculateBufferSize();
    int32_t AllocateBuffers();
    void InitBuffers();
    void FlushBuffers();
    void ResetData();
    void ReleaseBuffers();
    void StopThread();
    void ReleaseResource();
    int32_t UpdateBuffers(uint32_t index, int32_t bufferSize, uint32_t bufferType);
    int32_t UpdateSurfaceMemory(uint32_t index);
    void SendFrame();
    void ReceiveFrame();
    void FindAvailIndex(uint32_t index);
    void ConfigureSurface(const Format &format, const std::string_view &formatKey, FormatDataType formatType);
    void ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal = 0,
                             int32_t maxVal = INT_MAX);
    int32_t ConfigureContext(const Format &format);
    static void GetMpeg2CapProf(std::vector<CapabilityData> &capaArray);
    static void GetMpeg4esCapProf(std::vector<CapabilityData> &capaArray);
    static void SetMpeg4LevelsProfileGroup1(CapabilityData& capsData);
    static void SetMpeg4LevelsProfileGroup2(CapabilityData& capsData);
    static void SetMpeg4Profiles(CapabilityData& capsData);
    static void GetAvcCapProf(std::vector<CapabilityData> &capaArray);
    static void GetH263CapProf(std::vector<CapabilityData> &capaArray);
    static void GetMjpegCapProf(std::vector<CapabilityData> &capaArray);
    static void GetWmv3CapProf(std::vector<CapabilityData> &capaArray);
    static void GetVc1CapProf(std::vector<CapabilityData> &capaArray);
    void FreeExtradataIfNeeded(std::string name);
#if (defined SUPPORT_CODEC_RV) || (defined SUPPORT_CODEC_MP4V_ES) || (defined SUPPORT_CODEC_VC1)
    int32_t SetCodecExtradata(const Format &format);
#endif
    void FramePostProcess(std::shared_ptr<FBuffer> &frameBuffer, uint32_t index, int32_t status, int ret);
    int32_t AllocateInputBuffer(int32_t bufferCnt, int32_t inBufferSize);
    int32_t AllocateOutputBuffer(int32_t bufferCnt, int32_t outBufferSize);
    int32_t ClearSurfaceAndSetQueueSize(const sptr<Surface> &surface, int32_t bufferCnt);
    int32_t AllocateOutputBuffersFromSurface(int32_t bufferCnt);
    int32_t FillFrameBuffer(const std::shared_ptr<FBuffer> &frameBuffer);
    int32_t CheckFormatChange(uint32_t index, int width, int height);
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
    // surface listener callback
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
    bool CanSwapOut(bool isOutputBuffer, std::shared_ptr<FBuffer> &fBuffer);
    int32_t SwapOutBuffers(bool isOutputBuffer, State curState);
    int32_t SwapInBuffers(bool isOutputBuffer);
    bool disableDmaSwap_ = false;
    int32_t pid_ = -1;

    int32_t instanceId_ = -1;
    std::string decName_;
    std::string codecName_;
    std::atomic<State> state_ = State::UNINITIALIZED;
    Format format_;
    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t inputBufferSize_ = 0;
    int32_t outputBufferSize_ = 0;
    int32_t inputBufferCnt_ = 0;
    int32_t outputBufferCnt_ = 0;
    // INIT
    std::shared_ptr<AVCodec> avCodec_ = nullptr;
    CallerInfo fDecInfo_;
    // Config
    std::shared_ptr<AVCodecContext> avCodecContext_ = nullptr;
    // Start
    std::shared_ptr<AVPacket> avPacket_ = nullptr;
    std::shared_ptr<AVFrame> cachedFrame_ = nullptr;
    // Receive frame
    uint8_t *scaleData_[AV_NUM_DATA_POINTERS] = {nullptr};
    int32_t scaleLineSize_[AV_NUM_DATA_POINTERS] = {0};
    std::shared_ptr<Scale> scale_ = nullptr;
    bool isConverted_ = false;
    bool isOutBufSetted_ = false;
    VideoPixelFormat outputPixelFmt_ = VideoPixelFormat::UNKNOWN;
    // Running
    std::vector<std::shared_ptr<FBuffer>> buffers_[2];
    std::vector<std::shared_ptr<AVBuffer>> outAVBuffer4Surface_;
    std::shared_ptr<BlockQueue<uint32_t>> inputAvailQue_;
    std::shared_ptr<BlockQueue<uint32_t>> codecAvailQue_;
    std::shared_ptr<BlockQueue<uint32_t>> renderAvailQue_;
    std::shared_ptr<BlockQueue<uint32_t>> requestSurfaceBufferQue_;
    std::map<uint32_t, std::pair<sptr<SurfaceBuffer>, OHOS::BufferFlushConfig>> renderSurfaceBufferMap_;
    std::optional<uint32_t> synIndex_ = std::nullopt;
    SurfaceControl sInfo_;
    std::shared_ptr<TaskThread> sendTask_ = nullptr;
    std::shared_ptr<TaskThread> receiveTask_ = nullptr;
    std::mutex inputMutex_;
    std::mutex outputMutex_;
    std::mutex sendMutex_;
    std::mutex recvMutex_;
    std::mutex syncMutex_;
    std::mutex surfaceMutex_;
    std::mutex formatMutex_;
    std::mutex requestBufferMutex_;
    std::mutex renderBufferMapMutex_;
    std::condition_variable requestBufferCV_;
    std::condition_variable requestBufferOnceDoneCV_;
    std::condition_variable sendCv_;
    std::condition_variable recvCv_;
    std::shared_ptr<MediaCodecCallback> callback_;
    std::atomic<bool> isSendWait_ = false;
    std::atomic<bool> isSendEos_ = false;
    std::atomic<bool> isBufferAllocated_ = false;
    std::atomic<bool> requestSucceed_ = false;
    std::atomic<bool> requestBufferFinished_ = true;
    std::atomic<bool> requestBufferThreadExit_ = false;
    std::thread mRequestSurfaceBufferThread_;
    uint32_t decNum_ = 0;
    // dump
#ifdef BUILD_ENG_VERSION
    void OpenDumpFile();
    std::shared_ptr<std::ofstream> dumpInFile_ = nullptr;
    std::shared_ptr<std::ofstream> dumpOutFile_ = nullptr;
#endif // BUILD_ENG_VERSION
};
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FCODEC_H