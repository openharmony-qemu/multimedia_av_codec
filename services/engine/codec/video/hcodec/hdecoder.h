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
class HDecoder : public HCodec {
public:
    HDecoder(OHOS::HDI::Codec::V1_0::CodecCompCapability caps, OMX_VIDEO_CODINGTYPE codingType)
      : HCodec(caps, codingType, false) {}

private:
    // configure
    int32_t OnConfigure(const Format &format) override;
    int32_t SetupPort(const Format &format);
    int32_t UpdateInPortFormat() override;
    int32_t UpdateOutPortFormat() override;
    void GetCropFromOmx(uint32_t w, uint32_t h);
    uint64_t GetUsageFromOmx();
    int32_t OnSetOutputSurface(const sptr<Surface> &surface) override;
    int32_t OnSetParameters(const Format &format) override;
    GSError OnBufferReleasedByConsumer(sptr<SurfaceBuffer> &buffer);

    // start
    int32_t AllocateBuffersOnPort(OMX_DIRTYPE portIndex) override;
    int32_t AllocateOutputBuffersFromSurface();
    std::shared_ptr<OHOS::HDI::Codec::V1_0::OmxCodecBuffer> SurfaceBufferToOmxBuffer(
        const sptr<SurfaceBuffer>& surfaceBuffer);
    int32_t SubmitAllBuffersOwnedByUs() override;
    int32_t SubmitOutputBuffersToOmxNode() override;
    bool ReadyToStart() override;

    // input buffer circulation
    void OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode) override;

    // output buffer circulation
    int32_t OnUserRenderOutputBuffer(uint32_t bufferId, BufferOperationMode mode) override;
    int32_t NotifySurfaceToRenderOutputBuffer(BufferInfo &info);
    void OnGetBufferFromSurface() override;
    void FindSurfaceBufferSlotAndSubmit(sptr<SurfaceBuffer>& buffer);
    std::shared_ptr<AVSharedMemoryBase> OnUserGetOutputBuffer(uint32_t bufferId) override;

    // stop/release
    void EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i) override;
    void CancelBufferToSurface(BufferInfo& info);

private:
    sptr<Surface> outputSurface_;
    BufferType outputBufferType_ = BufferType::PRESET_SURFACE_BUFFER;
    uint32_t outBufferCnt_ = 0;
    BufferRequestConfig requestCfg_;
    BufferFlushConfig flushCfg_;
    static constexpr uint32_t STRIDE_ALIGNMENT = 32;
};
} // namespace OHOS::MediaAVCodec
#endif // HCODEC_HDECODER_H
