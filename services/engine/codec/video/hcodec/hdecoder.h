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

#ifndef HCODEC_HDECODER_H
#define HCODEC_HDECODER_H

#include "hcodec.h"
#include "v1_0/buffer_handle_meta_key_type.h"
#ifdef USE_VIDEO_PROCESSING_ENGINE
#include "video_refreshrate_prediction.h"
#endif
#include "xperf_service_action_type.h"
#include "xperf_service_client.h"
#include "video_state_callback_stub.h"

namespace OHOS::MediaAVCodec {
class HDecoder : public HCodec {
public:
    HDecoder(CodecHDI::CodecCompCapability caps, OMX_VIDEO_CODINGTYPE codingType);
    ~HDecoder() override;

    class XperfConnector : public OHOS::HiviewDFX::VideoStateCallbackStub {
    public:
        static std::shared_ptr<MsgToken> FindSuitableDecoder(uint64_t surfaceId, const std::string& bundleName);
        static std::shared_ptr<MsgToken> FindSuitableDecoder(const std::string& msg);
 
        ErrCode OnVideoJankEvent(const std::string& msg) override;
        ErrCode OnVideoResumed(const std::string& msg) override;
        ErrCode OnVideoPaused(const std::string& msg) override;
    };

    struct DecoderInst {
        std::weak_ptr<MsgToken> token;
        std::optional<uint64_t> surfaceId;
        std::string processName;
    };

private:
    struct SurfaceBufferItem {
        sptr<SyncFence> fence;
        // release with seq
        uint32_t seqnum = 0;
        // release and request
        sptr<SurfaceBuffer> buffer;
        int32_t generation = 0;
    };

    struct SurfaceBufferLayout {
        PixelFmt fmt;
        int32_t handleW = 0;
        int32_t handleH = 0;
        int32_t byteStride = 0;
        int32_t alignedHeight = 0;
        int32_t cropLeft = 0;
        int32_t cropTop = 0;
        int32_t cropW = 0;
        int32_t cropH = 0;

        friend bool operator==(const SurfaceBufferLayout& layout1, const SurfaceBufferLayout& layout2)
        {
            return layout1.fmt           == layout2.fmt &&
                   layout1.handleW       == layout2.handleW &&
                   layout1.handleH       == layout2.handleH &&
                   layout1.byteStride    == layout2.byteStride &&
                   layout1.alignedHeight == layout2.alignedHeight &&
                   layout1.cropLeft      == layout2.cropLeft &&
                   layout1.cropTop       == layout2.cropTop &&
                   layout1.cropW         == layout2.cropW &&
                   layout1.cropH         == layout2.cropH;
        }

        std::string ToString() const
        {
            std::string s;
            s.reserve(64); // 64Bytes
            (void)sprintf_s(s.data(), s.capacity(), "(%s, [%dx%d][%dx%d], %u/%u/%u/%u)",
                fmt.strFmt.c_str(), handleW, handleH, byteStride, alignedHeight, cropLeft, cropTop, cropW, cropH);
            return s;
        }
    };

private:
    // configure
    bool DecideOutputBufMode();
    int32_t OnConfigure(const Format &format) override;
    int32_t SetupPort(const Format &format);
    int32_t UpdateInPortFormat() override;
    int32_t UpdateOutPortFormat() override;
    void UpdateColorAspects() override;
    void GetCropFromOmx(uint32_t w, uint32_t h, OHOS::Rect& damage);
    int32_t RegisterListenerToSurface(const sptr<Surface> &surface);
    void OnSetOutputSurface(const MsgInfo &msg, BufferOperationMode mode) override;
    int32_t OnSetOutputSurfaceWhenCfg(const sptr<Surface> &surface);
    int32_t OnSetParameters(const Format &format) override;
    bool UpdateConfiguredFmt(OMX_COLOR_FORMATTYPE portFmt);
    uint64_t GetModeUsage();
    uint64_t GetVendorProducerUsage();
    uint64_t GetSurfaceConsumerUsage();
    bool SetUsageToVendor(uint64_t usage);

    int32_t SaveTransform(const Format &format, bool set = false);
    int32_t SetTransform();
    int32_t SaveScaleMode(const Format &format, bool set = false);
    int32_t SetScaleMode();

    // start
    void OnEnterRunningState() override;
    void OnExitRunningState() override;
    bool UseHandleOnOutputPort(bool isDynamic);
    int32_t AllocateBuffersOnPort(OMX_DIRTYPE portIndex) override;
    int32_t AllocOutDynamicSurfaceBuf();
    int32_t AllocateOutputBuffersFromSurface();
    int32_t AllocateOutputBuffersFromOmx();
    int32_t ClearSurfaceAndSetQueueSize(const sptr<Surface> &surface, uint32_t targetSize);
    int32_t SubmitAllBuffersOwnedByUs() override;
    int32_t SubmitOutBufToOmx() override;
    bool ReadyToStart() override;
    void OnQueryJankReason() override;
    void GetJankReason(const TimePoint& now, OMX_DIRTYPE port,
        double& maxPercent, OHOS::HiviewDFX::AvcodecFaultCode& fault, std::string& reason);

    // input buffer circulation
    void OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode) override;

    // output buffer circulation
    void BeforeCbOutToUser(BufferInfo &info) override;
    void ProcSurfaceBufferToUser(const sptr<SurfaceBuffer>& buffer) override;
    void ProcAVBufferToUser(std::shared_ptr<AVBuffer> avBuffer,
        std::shared_ptr<CodecHDI::OmxCodecBuffer> omxBuffer) override;

    void OnReleaseOutputBuffer(const BufferInfo &info) override;
    void OnRenderOutputBuffer(const MsgInfo &msg, BufferOperationMode mode) override;
    int32_t NotifySurfaceToRenderOutputBuffer(BufferInfo &info);
    int32_t Attach(BufferInfo &info);
    void OnGetBufferFromSurface(const ParamSP& param) override;
    SurfaceBufferItem RequestBuffer();
    std::vector<BufferInfo>::iterator FindBelongTo(uint32_t seqnum);
    std::vector<BufferInfo>::iterator FindBelongTo(sptr<SurfaceBuffer>& buffer);
    std::vector<BufferInfo>::iterator FindNullSlotIfDynamicMode();
    void SurfaceModeSubmitBuffer();
    void SurfaceModeSubmitBufferFromFreeList();
    bool SurfaceModeSubmitOneItem(SurfaceBufferItem& item);
    void DynamicModeSubmitBuffer() override;
    void DynamicModeSubmitIfEos() override;
    void DynamicModeSubmitBufferToSlot(sptr<SurfaceBuffer>& buffer, std::vector<BufferInfo>::iterator nullSlot);
    void DynamicModeSubmitBufferToSlot(std::vector<BufferInfo>::iterator nullSlot);
    void SubmitBuffersToNextOwner() override;

    // switch surface
    void OnSetOutputSurfaceWhenRunning(const sptr<Surface> &newSurface,
        const MsgInfo &msg, BufferOperationMode mode);
    void SwitchBetweenSurface(const sptr<Surface> &newSurface,
        const MsgInfo &msg, BufferOperationMode mode);
    void ConsumeFreeList(BufferOperationMode mode);
    void ClassifyOutputBufferOwners(std::vector<size_t>& ownedByUs,
                                    std::map<int64_t, size_t>& ownedBySurfaceFlushTime2BufferIndex);

    // stop/release
    void EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i) override;
    void OnClearBufferPool(OMX_DIRTYPE portIndex) override;
    void OnEnterUninitializedState() override;

    // VRR
    int32_t SetVrrEnable(const Format &format);
    void SetIsMvUpload(const Format &format);

    // swap dma buffer
    bool CanSwapOut(OMX_DIRTYPE portIndex, BufferInfo& info);
    int32_t SwapInBufferByPortIndex(OMX_DIRTYPE portIndex);
    int32_t SwapOutBufferByPortIndex(OMX_DIRTYPE portIndex);
#ifdef USE_VIDEO_PROCESSING_ENGINE
    int32_t VrrPrediction(BufferInfo &info) override;
    int32_t InitVrr();
    void CombineConsumerUsageByVpe(uint64_t& consumerUsage);
    static constexpr double VRR_DEFAULT_INPUT_FRAME_RATE = 60.0;
    using VrrCreate = Media::VideoProcessingEngine::VideoRefreshRatePredictionHandle* (*)();
    using VrrDestroy = void (*)(Media::VideoProcessingEngine::VideoRefreshRatePredictionHandle*);
    using VrrCheckSupport = int32_t (*)(Media::VideoProcessingEngine::VideoRefreshRatePredictionHandle*,
        const char *processName);
    using VrrProcess = void (*)(Media::VideoProcessingEngine::VideoRefreshRatePredictionHandle*,
        OH_NativeBuffer*, int32_t, int32_t);
    using VpeVideoGetSupportedListByType = bool (*)(uint32_t, std::vector<std::string>&);
    VrrCreate VrrCreateFunc_ = nullptr;
    VrrDestroy VrrDestroyFunc_ = nullptr;
    VrrCheckSupport VrrCheckSupportFunc_ = nullptr;
    VrrProcess VrrProcessFunc_ = nullptr;
    void *vpeHandle_ = nullptr;
    bool vrrDynamicSwitch_ = false;
    Media::VideoProcessingEngine::VideoRefreshRatePredictionHandle* vrrHandle_ = nullptr;
#endif

    // Lpp target pts
    int32_t SetLppTargetPts(const Format &format);
    // freeze
    int32_t FreezeBuffers() override;
    int32_t ActiveBuffers() override;
    int32_t DecreaseFreq() override;
    int32_t RecoverFreq() override;

    void PerfMeasure();
    void OnStartJankDetect() override;
    void OnStopJankDetect() override;

    // process stall time
    void RecordProcessTimeOfUpstream(const std::shared_ptr<AVBuffer>& avBuffer) override;
    void AppendProcessTimeOfUs(const std::shared_ptr<AVBuffer>& avBuffer, int64_t pts, const TimePoint& now) override;
    int32_t SetMasteringDisplayColourVolumeFromContainer(const Format &format);
    int32_t SetContentLightLevelFromContainer(const Format &format);
private:
    static constexpr uint64_t SURFACE_MODE_PRODUCER_USAGE = BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_VIDEO_DECODER;
    static constexpr uint64_t BUFFER_MODE_REQUEST_USAGE =
        SURFACE_MODE_PRODUCER_USAGE | BUFFER_USAGE_CPU_READ | BUFFER_USAGE_MEM_MMZ_CACHE;
    uint64_t cfgedConsumerUsage_ = 0;

    struct SurfaceItem {
        SurfaceItem() = default;
        SurfaceItem(const sptr<Surface> &surface, std::string codecId, int32_t instanceId);
        void Release(bool cleanAll = false);
        sptr<Surface> surface_;
        bool firstFrameRendered_ = false;
        uint64_t flushCnt_ = 0;
        int64_t lastFlushTime_ = 0;
        int64_t jankCnt_ = 0;
        int64_t totalJankDur_ = 0;
        int64_t beginTime_ = 0;
    private:
        std::optional<GraphicTransformType> originalTransform_;
        std::string compUniqueStr_;
        int32_t instanceId_;
    } currSurface_;

    std::list<SurfaceBufferItem> freeList_;
    int32_t currGeneration_ = 0;
    bool currGenerationLayoutHasParsed_ = false;
    std::shared_ptr<SurfaceBufferLayout> currLayout_;
    bool isAlloc_ = false;
    bool isDynamic_ = false;
    bool isReleaseWithSeq_ = true;  // true: dont need to request, false: need to do request
    uint32_t outBufferCnt_ = 0;
    GraphicTransformType transform_ = GRAPHIC_ROTATE_NONE;
    std::optional<ScalingMode> scaleMode_;

    bool detect_ = false;
};
} // namespace OHOS::MediaAVCodec
#endif // HCODEC_HDECODER_H
