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
#include <deque>
#include "codec_omx_ext.h"
#include "sync_fence.h"
#include "hcodec_utils.h"

constexpr int32_t PREVIOUS_PTS_RECORDED_COUNT = 4;
constexpr int64_t TIME_RATIO_NS_TO_US = 1000;
constexpr int64_t TIME_RATIO_US_TO_S = 1000000;
constexpr double DURATION_SCALE_FACTOR = 7.0;
constexpr int32_t DEFAULT_FRAME_RATE = 15;
constexpr double SMOOTH_FACTOR_CLIP_MIN = 0.3;
constexpr int32_t SMOOTH_FACTOR_CLIP_RANGE_MIN = 10;
constexpr double SMOOTH_FACTOR_CLIP_MAX = 0.9;
constexpr int32_t SMOOTH_FACTOR_CLIP_RANGE_MAX = 70;
constexpr double AVERAGE_DURATION_ERROR_COUNT = 3.0;
constexpr double AVERAGE_DURATION_ERROR_FRAMERATE_RANGE = 0.5;

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

    struct WaterMarkInfo {
        bool enableWaterMark = false;
        int32_t x = 0;
        int32_t y = 0;
        int32_t w = 0;
        int32_t h = 0;
    };

private:
    // configure
    int32_t OnConfigure(const Format &format) override;
    int32_t OnConfigureBuffer(std::shared_ptr<AVBuffer> buffer) override;
    int32_t ConfigureBufferType();
    int32_t SetupPort(const Format &format, std::optional<double> &frameRate);
    void ConfigureProtocol(const Format &format, std::optional<double> frameRate);
    void CalcInputBufSize(PortInfo& info, VideoPixelFormat pixelFmt);
    int32_t UpdateInPortFormat() override;
    int32_t UpdateOutPortFormat() override;
    int32_t ConfigureOutputBitrate(const Format &format);
    static std::optional<uint32_t> GetBitRateFromUser(const Format &format);
    static std::optional<VideoEncodeBitrateMode> GetBitRateModeFromUser(const Format &format);
    static std::optional<uint32_t> GetSQRFactorFromUser(const Format &format);
    static std::optional<uint32_t> GetSQRMaxBitrateFromUser(const Format &format);
    static std::optional<uint32_t> GetCRFtagetQpFromUser(const Format &format);
    static int32_t GetWaterMarkInfo(std::shared_ptr<AVBuffer> buffer, WaterMarkInfo &info);
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
    int32_t SetConstantQualityMode(int32_t quality);
    int32_t SetSQRMode(const Format &format);
    int32_t EnableFrameQPMap(const Format &format);
    int32_t ConfigBEncodeMode(const Format &format);
    int32_t SetCRFMode(int32_t targetQp);
    void EnableVariableFrameRate(const Format &format);

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
    void SubmitOneBuffer(InSurfaceBufferEntry& entry, BufferInfo &info);
    void ResetSlot(BufferInfo& info);
    void OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode) override;
    void OnSignalEndOfInputStream(const MsgInfo &msg) override;
    void OnQueueInputBuffer(const MsgInfo &msg, BufferOperationMode mode) override;
    void CheckPts(int64_t currentPts);

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
    void WrapQPMapParamIntoOmxBuffer(std::shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                  const std::shared_ptr<Media::Meta> &meta);
    void WrapIsSkipFrameIntoOmxBuffer(std::shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                      const std::shared_ptr<Media::Meta> &meta);
    void ParseRoiStringValid(const std::string &roiValue, std::shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer);
    void WrapRoiParamIntoOmxBuffer(std::shared_ptr<CodecHDI::OmxCodecBuffer> &omxBuffer,
                                  const std::shared_ptr<Media::Meta> &meta);
    void BeforeCbOutToUser(BufferInfo &info) override;
    void ExtractPerFrameLTRParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta);
    void ExtractPerFrameMadParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta);
    void ExtractPerFrameRealBitrateParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta);
    void ExtractPerFrameFrameQpParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta);
    void ExtractPerFrameIRitioParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta);
    void ExtractPerFrameAveQpParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta);
    void ExtractPerFrameMSEParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta);
    void ExtractPerFrameLayerParam(BinaryReader &reader, std::shared_ptr<Media::Meta> &meta);
    void DealWithResolutionChange(uint32_t newWidth, uint32_t newHeight);

    double CalculateSmoothFactorBasedPts(int64_t curPts, int64_t curDuration);
    int32_t CalculateSmoothFpsBasedPts(int64_t curPts, int64_t curDuration);
    int32_t UpdateTimeStampWindow(int64_t curPts, int32_t &frameRate);
    int32_t CalculateFrameRateParamIntoOmxBuffer(int64_t curPts);

    // stop/release
    void EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i) override;
    void OnEnterUninitializedState() override;

private:
    class EncoderBuffersConsumerListener : public IBufferConsumerListener {
    public:
        explicit EncoderBuffersConsumerListener(std::weak_ptr<MsgToken> codec) : codec_(codec) {}
        void OnBufferAvailable() override;
    private:
        std::weak_ptr<MsgToken> codec_;
    };

private:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool enableSurfaceModeInputCb_ = false;
    bool enableLTR_ = false;
    bool enableTSVC_ = false;
    bool enableQPMap_ = false;
    bool enableVariableFrameRate_ = false;
    std::deque<int64_t> previousPtsWindow_;
    int32_t previousSmoothFrameRate_ = 0;
    std::optional<double> defaultFrameRate_;
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
    std::optional<int64_t> pts_;
    static constexpr size_t roiNum = 6;
};
} // namespace OHOS::MediaAVCodec
#endif // HCODEC_HENCODER_H
