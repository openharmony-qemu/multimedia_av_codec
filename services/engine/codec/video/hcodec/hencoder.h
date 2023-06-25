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

namespace OHOS::MediaAVCodec {
class HEncoder : public HCodec {
public:
    HEncoder(OHOS::HDI::Codec::V1_0::CodecCompCapability caps, OMX_VIDEO_CODINGTYPE codingType)
      : HCodec(caps, codingType, true) {}

private:
    // configure
    int32_t OnConfigure(const Format &format) override;
    int32_t SetupPort(const Format &format);
    void CalcInputBufSize(PortInfo& info, VideoPixelFormat pixelFmt);
    int32_t UpdateInPortFormat() override;
    int32_t UpdateOutPortFormat() override;
    int32_t ConfigureOutputBitrate(const Format &format);
    int32_t SetupAVCEncoderParameters(const Format &format);
    void SetAvcFields(OMX_VIDEO_PARAM_AVCTYPE& h264type, const Format &format);
    int32_t SetupHEVCEncoderParameters(const Format &format);
    int32_t SetColorAspects(const Format &format);
    int32_t OnSetParameters(const Format &format) override;
    sptr<Surface> OnCreateInputSurface() override;
    int32_t OnSetInputSurface(sptr<Surface> &inputSurface) override;
    int32_t RequestIDRFrame() override;

    // start
    int32_t AllocateBuffersOnPort(OMX_DIRTYPE portIndex) override;
    int32_t AllocInBufsForDynamicSurfaceBuf();
    std::shared_ptr<OHOS::HDI::Codec::V1_0::OmxCodecBuffer> AllocOmxBufferOfDynamicType();
    int32_t SubmitAllBuffersOwnedByUs() override;
    int32_t SubmitOutputBuffersToOmxNode() override;
    bool ReadyToStart() override;

    // input buffer circulation
    std::shared_ptr<AVSharedMemoryBase> OnUserGetInputBuffer(uint32_t bufferId) override;
    void OnGetBufferFromSurface() override;
    void OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode) override;
    int32_t WrapSurfaceBufferIntoOmxBuffer(std::shared_ptr<OHOS::HDI::Codec::V1_0::OmxCodecBuffer>& omxBuffer,
        const sptr<SurfaceBuffer>& surfaceBuffer, int32_t fenceFd, int64_t pts, uint32_t flag);
    int32_t OnSignalEndOfInputStream() override;
    int32_t OnUserQueueInputBuffer(uint32_t bufferId, const AVCodecBufferInfo &info,
        AVCodecBufferFlag flag, BufferOperationMode mode) override;

    // output buffer circulation

    // stop/release
    void EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i) override;

private:
    class EncoderBuffersConsumerListener : public IBufferConsumerListener {
    public:
        explicit EncoderBuffersConsumerListener(HEncoder *codec) : codec_(codec) {}
        void OnBufferAvailable();
    private:
        HEncoder* codec_;
    };

private:
    sptr<Surface> inputSurface_;
    BufferType inputBufferType_ = BufferType::DYNAMIC_SURFACE_BUFFER;
};
} // namespace OHOS::MediaAVCodec
#endif // HCODEC_HENCODER_H
