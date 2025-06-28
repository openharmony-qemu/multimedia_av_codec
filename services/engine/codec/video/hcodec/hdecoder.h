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

namespace OHOS::MediaAVCodec {
class HDecoder : public HCodec {
public:
    HDecoder(CodecHDI::CodecCompCapability caps, OMX_VIDEO_CODINGTYPE codingType)
        : HCodec(caps, codingType, false) {}
    ~HDecoder() override;

private:
    struct SurfaceBufferItem {
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence;
        int32_t generation = 0;
        bool hasSwapedOut = false;
    };

private:
    // configure
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
    uint64_t GetProducerUsage();
    void CombineConsumerUsage();
    int32_t SaveTransform(const Format &format, bool set = false);
    int32_t SetTransform();
    int32_t SaveScaleMode(const Format &format, bool set = false);
    int32_t SetScaleMode();

    // start
    bool UseHandleOnOutputPort(bool isDynamic);
    int32_t AllocateBuffersOnPort(OMX_DIRTYPE portIndex) override;
    void SetCallerToBuffer(int fd) override;
    void UpdateFormatFromSurfaceBuffer() override;
    int32_t AllocOutDynamicSurfaceBuf();
    int32_t AllocateOutputBuffersFromSurface();
    int32_t ClearSurfaceAndSetQueueSize(const sptr<Surface> &surface, uint32_t targetSize);
    int32_t SubmitAllBuffersOwnedByUs() override;
    int32_t SubmitOutputBuffersToOmxNode() override;
    bool ReadyToStart() override;

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

    // swap dma buffer
    bool CanSwapOut(OMX_DIRTYPE portIndex, BufferInfo& info);
    int32_t SwapInBufferByPortIndex(OMX_DIRTYPE portIndex);
    int32_t SwapOutBufferByPortIndex(OMX_DIRTYPE portIndex);
#ifdef USE_VIDEO_PROCESSING_ENGINE
    int32_t VrrPrediction(BufferInfo &info) override;
    int32_t InitVrr();
    static constexpr double VRR_DEFAULT_INPUT_FRAME_RATE = 60.0;
    using VrrCreate = Media::VideoProcessingEngine::VideoRefreshRatePredictionHandle* (*)();
    using VrrDestroy = void (*)(Media::VideoProcessingEngine::VideoRefreshRatePredictionHandle*);
    using VrrCheckSupport = int32_t (*)(Media::VideoProcessingEngine::VideoRefreshRatePredictionHandle*,
        const char *processName);
    using VrrProcess = void (*)(Media::VideoProcessingEngine::VideoRefreshRatePredictionHandle*,
        OH_NativeBuffer*, int32_t, int32_t);
    VrrCreate VrrCreateFunc_ = nullptr;
    VrrDestroy VrrDestroyFunc_ = nullptr;
    VrrCheckSupport VrrCheckSupportFunc_ = nullptr;
    VrrProcess VrrProcessFunc_ = nullptr;
    void *vpeHandle_ = nullptr;
    bool vrrDynamicSwitch_ = false;
    Media::VideoProcessingEngine::VideoRefreshRatePredictionHandle* vrrHandle_ = nullptr;
#endif
    // freeze
    int32_t FreezeBuffers() override;
    int32_t ActiveBuffers() override;
    int32_t DecreaseFreq() override;
    int32_t RecoverFreq() override;

private:
    static constexpr uint64_t SURFACE_MODE_PRODUCER_USAGE = BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_VIDEO_DECODER;
    static constexpr uint64_t BUFFER_MODE_REQUEST_USAGE =
        SURFACE_MODE_PRODUCER_USAGE | BUFFER_USAGE_CPU_READ | BUFFER_USAGE_MEM_MMZ_CACHE;
    uint64_t cfgedConsumerUsage = 0;

    struct SurfaceItem {
        SurfaceItem() = default;
        explicit SurfaceItem(const sptr<Surface> &surface, std::string codecId);
        void Release(bool cleanAll = false);
        sptr<Surface> surface_;
    private:
        std::optional<GraphicTransformType> originalTransform_;
        std::string compUniqueStr_;
    } currSurface_;

    std::list<SurfaceBufferItem> freeList_;
    int32_t currGeneration_ = 0;
    bool isDynamic_ = false;
    uint32_t outBufferCnt_ = 0;
    GraphicTransformType transform_ = GRAPHIC_ROTATE_NONE;
    std::optional<ScalingMode> scaleMode_;
    OHOS::HDI::Display::Graphic::Common::V1_0::BufferHandleMetaRegion crop_{0};
};
} // namespace OHOS::MediaAVCodec
#endif // HCODEC_HDECODER_H
