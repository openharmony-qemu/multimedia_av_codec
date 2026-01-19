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
    // for memory recycle
    int32_t NotifyMemoryRecycle() override;
    int32_t NotifyMemoryWriteBack() override;
    // for capability register
    static int32_t GetCodecCapability(std::vector<CapabilityData> &capaArray);

private:
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
        int32_t format_ = 0;
        uint64_t usage_ = SURFACE_DEFAULT_USAGE;
        std::atomic<bool> hasSwapedOut_ = false;
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
    bool IsActive() const;
    void FreeExtraData();
    void ResetContext(bool isNeedFree = true);
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
    void ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal = 0,
                             int32_t maxVal = INT_MAX);
    int32_t ConfigureContext(const Format &format);
#if (defined SUPPORT_CODEC_RV) || (defined SUPPORT_CODEC_MP4V_ES) || (defined SUPPORT_CODEC_VC1)
    int32_t SetCodecExtradata(const Format &format);
#endif
    void FramePostProcess(std::shared_ptr<FBuffer> &frameBuffer, uint32_t index, int32_t status, int ret);
    int32_t AllocateInputBuffer(int32_t inBufferSize);
    int32_t AllocateOutputBuffer(int32_t outBufferSize);
    int32_t ClearSurfaceAndSetQueueSize();
    int32_t AllocateOutputBuffersFromSurface();
    int32_t FillFrameBuffer(const std::shared_ptr<FBuffer> &frameBuffer);
    bool CheckStrideChange(uint32_t index, bool& isChanged);
    int32_t CheckFormatChange(uint32_t index, int width, int height);
    void SetSurfaceParameter();
    int32_t ReplaceOutputSurfaceWhenRunning(sptr<Surface> newSurface);
    int32_t SetQueueSize(const sptr<Surface> &surface, uint32_t targetSize);
    int32_t SwitchBetweenSurface(const sptr<Surface> &newSurface);
    int32_t RenderNewSurfaceWithOldBuffer(const sptr<Surface> &newSurface, uint32_t index);
    int32_t FlushSurfaceMemory(std::shared_ptr<FSurfaceMemory> &surfaceMemory, uint32_t index);
    int32_t SetSurfaceCfg();
    int32_t Attach(sptr<SurfaceBuffer> surfaceBuffer);
    int32_t Detach(sptr<SurfaceBuffer> surfaceBuffer);
    void CombineConsumerUsage();
    // surface config
    void GetSurfaceCfgFromFmt(const Format &format);
    // surface listener callback
    GSError BufferReleasedByConsumer(uint64_t surfaceId);
    int32_t RegisterListenerToSurface(const sptr<Surface> &surface);
    void UnRegisterListenerToSurface(const sptr<Surface> &surface);
    void RequestSurfaceBufferThread();
    void StartRequestSurfaceBufferThread();
    void StopRequestSurfaceBufferThread();
    void RequestSurfaceBufferOnce();
    void RequestSurfaceBuffer(SurfaceBufferInfo &bufInfo);
    bool FBufferAvailable(const std::shared_ptr<FBuffer> &buffer, SurfaceBufferInfo &bufInfo);
    void OnSurfaceBufferAvailable(SurfaceBufferInfo &bufInfo);
    // for memory recycle
    int32_t FreezeBuffers(State curState);
    int32_t ActiveBuffers();
    bool CanSwapOut(bool isOutputBuffer, std::shared_ptr<FBuffer> &fBuffer);
    int32_t SwapOutBuffers(bool isOutputBuffer, State curState);
    int32_t SwapInBuffers(bool isOutputBuffer);
    void PutFormatValue();
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
    uint32_t inputBufferCnt_ = 0;
    uint32_t outputBufferCnt_ = 0;
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
    RawVideoPixelFormat rawvideoPixFmt_ = RawVideoPixelFormat::UNKNOWN;
    // Running
    std::vector<std::shared_ptr<FBuffer>> buffers_[2];
    std::vector<std::shared_ptr<AVBuffer>> outAVBuffer4Surface_;
    std::shared_ptr<BlockQueue<uint32_t>> inputAvailQue_;
    std::shared_ptr<BlockQueue<uint32_t>> codecAvailQue_;
    // for surfacebuffer
    std::unordered_map<uint32_t, uint32_t> seqNumToFbufMap_;
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
    std::condition_variable sendCv_;
    std::condition_variable recvCv_;
    std::shared_ptr<MediaCodecCallback> callback_;
    std::atomic<bool> isSendWait_ = false;
    std::atomic<bool> isSendEos_ = false;
    std::atomic<bool> isBufferAllocated_ = false;
    // for surface buffer request thread
    std::atomic<uint32_t> count_ = 0u;
    std::atomic<bool> requestBufferThreadExit_ = false;
    std::thread mRequestSurfaceBufferThread_;
    uint32_t decNum_ = 0;
    // surface config
    std::atomic<GraphicTransformType> transform_ = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    // dump
#ifdef BUILD_ENG_VERSION
    void OpenDumpFile();
    void DumpOutputBuffer();
    std::shared_ptr<std::ofstream> dumpInFile_ = nullptr;
    std::shared_ptr<std::ofstream> dumpOutFile_ = nullptr;
#endif // BUILD_ENG_VERSION
};
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FCODEC_H