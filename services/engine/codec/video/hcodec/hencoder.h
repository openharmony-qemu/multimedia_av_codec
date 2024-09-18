/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HCODEC_HENCODER_H
#define HCODEC_HENCODER_H

#include "hcodec.h"
#include "codec_omx_ext.h"
#include "sync_fence.h"

namespace OHOS::MediaAVCodec {
class HEncoder : public HCodec {
public:
    HEncoder(CodecHDI::CodecCompCapability caps, OMX_VIDEO_CODINGTYPE codingType)
        : HCodec(caps, codingType, true) {}
    ~HEncoder() override;

private:
    struct BufferItem {
        BufferItem() = default;
        ~BufferItem();
        uint64_t generation = 0;
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence;
        OHOS::Rect damage;
        sptr<Surface> surface;
    };
    struct InSurfaceBufferEntry {
        std::shared_ptr<BufferItem> item; // don't change after created
        int64_t pts = -1;           // may change
        int32_t repeatTimes = 0;    // may change
    };

private:
    // configure
    int32_t OnConfigure(const Format &format) override;
    int32_t ConfigureBufferType();
    int32_t SetupPort(const Format &format, std::optional<double> frameRate);
    void ConfigureProtocol(const Format &format, std::optional<double> frameRate);
    void CalcInputBufSize(PortInfo& info, VideoPixelFormat pixelFmt);
    int32_t UpdateInPortFormat() override;
    int32_t UpdateOutPortFormat() override;
    int32_t ConfigureOutputBitrate(const Format &format);
    static std::optional<uint32_t> GetBitRateFromUser(const Format &format);
    int32_t SetupAVCEncoderParameters(const Format &format, std::optional<double> frameRate);
    void SetAvcFields(OMX_VIDEO_PARAM_AVCTYPE& avcType, int32_t iFrameInterval, double frameRate);
    int32_t SetupHEVCEncoderParameters(const Format &format, std::optional<double> frameRate);
    int32_t SetColorAspects(const Format &format);
    int32_t OnSetParameters(const Format &format) override;
    sptr<Surface> OnCreateInputSurface() override;
    int32_t OnSetInputSurface(sptr<Surface> &inputSurface) override;
    int32_t RequestIDRFrame() override;
    void CheckIfEnableCb(const Format &format);
    int32_t SetLTRParam(const Format &format);
    int32_t EnableEncoderParamsFeedback(const Format &format);
    int32_t SetQpRange(const Format &format, bool isCfg);
    int32_t SetRepeat(const Format &format);
    int32_t SetTemperalLayer(const Format &format);

    // start
    int32_t AllocateBuffersOnPort(OMX_DIRTYPE portIndex) override;
    void UpdateFormatFromSurfaceBuffer() override;
    int32_t AllocInBufsForDynamicSurfaceBuf();
    int32_t SubmitAllBuffersOwnedByUs() override;
    int32_t SubmitOutputBuffersToOmxNode() override;
    void ClearDirtyList();
    bool ReadyToStart() override;

    // input buffer circulation
    void OnGetBufferFromSurface(const ParamSP& param) override;
    void RepeatIfNecessary(const ParamSP& param) override;
    void SendRepeatMsg(uint64_t generation);
    bool GetOneBufferFromSurface();
    void TraverseAvaliableBuffers();
    void TraverseAvaliableSlots();
    void SubmitOneBuffer(InSurfaceBufferEntry& entry, BufferInfo &info);
    void ResetSlot(BufferInfo& info);
    void OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode) override;
    void OnSignalEndOfInputStream(const MsgInfo &msg) override;
    void OnQueueInputBuffer(const MsgInfo &msg, BufferOperationMode mode) override;

    // per frame param
    void WrapPerFrameParamIntoOmxBuffer(std::shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                        const std::shared_ptr<Media::Meta> &meta);
    void WrapLTRParamIntoOmxBuffer(std::shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                   const std::shared_ptr<Media::Meta> &meta);
    void WrapRequestIFrameParamIntoOmxBuffer(std::shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                             const std::shared_ptr<Media::Meta> &meta);
    void WrapQPRangeParamIntoOmxBuffer(std::shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                       const std::shared_ptr<Media::Meta> &meta);
    void WrapStartQPIntoOmxBuffer(std::shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                  const std::shared_ptr<Media::Meta> &meta);
    void WrapIsSkipFrameIntoOmxBuffer(std::shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                      const std::shared_ptr<Media::Meta> &meta);
    void ExtractPerFrameParamFromOmxBuffer(const std::shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                           std::shared_ptr<Media::Meta> &meta) override;
    void ExtractPerFrameLTRParam(const CodecEncOutLTRParam* ltrParam, std::shared_ptr<Media::Meta> &meta);

    // stop/release
    void EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i) override;
    void OnEnterUninitializedState() override;

private:
    class EncoderBuffersConsumerListener : public IBufferConsumerListener {
    public:
        explicit EncoderBuffersConsumerListener(HEncoder *codec) : codec_(codec) {}
        void OnBufferAvailable() override;
    private:
        HEncoder* codec_;
    };

private:
    bool enableSurfaceModeInputCb_ = false;
    bool enableLTR_ = false;
    bool enableTSVC_ = false;
    sptr<Surface> inputSurface_;
    uint32_t inBufferCnt_ = 0;
    static constexpr size_t MAX_LIST_SIZE = 256;
    static constexpr uint32_t THIRTY_MILLISECONDS_IN_US = 30'000;
    static constexpr uint32_t SURFACE_MODE_CONSUMER_USAGE = BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_VIDEO_ENCODER;
    static constexpr uint64_t BUFFER_MODE_REQUEST_USAGE =
        SURFACE_MODE_CONSUMER_USAGE | BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_MMZ_CACHE;

    uint64_t currGeneration_ = 0;
    std::list<InSurfaceBufferEntry> avaliableBuffers_;
    InSurfaceBufferEntry newestBuffer_{};
    std::map<uint32_t, InSurfaceBufferEntry> encodingBuffers_;
    uint64_t repeatUs_ = 0;      // 0 means user don't set this value
    int32_t repeatMaxCnt_ = 10;  // default repeat 10 times. <0 means repeat forever. =0 means nothing.
};
} // namespace OHOS::MediaAVCodec
#endif // HCODEC_HENCODER_H
