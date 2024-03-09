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

namespace OHOS::MediaAVCodec {
class HDecoder : public HCodec, public std::enable_shared_from_this<HDecoder> {
public:
    HDecoder(OHOS::HDI::Codec::V2_0::CodecCompCapability caps, OMX_VIDEO_CODINGTYPE codingType)
        : HCodec(caps, codingType, false) {}

private:
    // configure
    int32_t OnConfigure(const Format &format) override;
    int32_t SetupPort(const Format &format);
    int32_t UpdateInPortFormat() override;
    int32_t UpdateOutPortFormat() override;
    void UpdateColorAspects() override;
    void GetCropFromOmx(uint32_t w, uint32_t h);
    int32_t OnSetOutputSurface(const sptr<Surface> &surface) override;
    int32_t OnSetParameters(const Format &format) override;
    GSError OnBufferReleasedByConsumer(sptr<SurfaceBuffer> &buffer);
    bool UpdateConfiguredFmt(OMX_COLOR_FORMATTYPE portFmt);
    void CombineConsumerUsage();
    int32_t SaveTransform(const Format &format, bool set = false);
    int32_t SetTransform();
    int32_t SaveScaleMode(const Format &format, bool set = false);
    int32_t SetScaleMode();

    // start
    int32_t AllocateBuffersOnPort(OMX_DIRTYPE portIndex) override;
    void UpdateFormatFromSurfaceBuffer() override;
    int32_t AllocateOutputBuffersFromSurface();
    __attribute__((no_sanitize("cfi"))) int32_t SubmitAllBuffersOwnedByUs() override;
    int32_t SubmitOutputBuffersToOmxNode() override;
    bool ReadyToStart() override;

    // input buffer circulation
    void OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode) override;

    // output buffer circulation
    void OnRenderOutputBuffer(const MsgInfo &msg, BufferOperationMode mode) override;
    int32_t NotifySurfaceToRenderOutputBuffer(BufferInfo &info);
    void OnGetBufferFromSurface() override;
    bool GetOneBufferFromSurface();
    uint64_t GetProducerUsage();

    // stop/release
    void EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i) override;
    void CancelBufferToSurface(BufferInfo &info);
    void OnEnterUninitializedState() override;

private:
    static constexpr uint64_t SURFACE_MODE_PRODUCER_USAGE = BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_VIDEO_DECODER;
    static constexpr uint64_t BUFFER_MODE_REQUEST_USAGE =
        SURFACE_MODE_PRODUCER_USAGE | BUFFER_USAGE_CPU_READ | BUFFER_USAGE_MEM_MMZ_CACHE;
    sptr<Surface> outputSurface_;
    uint32_t outBufferCnt_ = 0;
    BufferFlushConfig flushCfg_;
    GraphicTransformType originalTransform_;
    GraphicTransformType transform_ = GRAPHIC_ROTATE_NONE;
    std::optional<ScalingMode> scaleMode_;
};
} // namespace OHOS::MediaAVCodec
#endif // HCODEC_HDECODER_H
