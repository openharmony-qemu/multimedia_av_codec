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
    int32_t OnSetOutputSurface(const sptr<Surface> &surface) override;
    int32_t OnSetParameters(const Format &format) override;
    GSError OnBufferReleasedByConsumer(sptr<SurfaceBuffer> &buffer);
    bool UpdateConfiguredFmt(OMX_COLOR_FORMATTYPE portFmt);
    void UpdateUsage();

    // prepare & start
    int32_t AllocateBuffersOnPort(OMX_DIRTYPE portIndex) override;
    int32_t AllocateOutputBuffersFromSurface();
    std::shared_ptr<OHOS::HDI::Codec::V1_0::OmxCodecBuffer> SurfaceBufferToOmxBuffer(
        const sptr<SurfaceBuffer>& surfaceBuffer);
    int32_t SubmitOutputBuffersToOmxNode() override;
    void SubmitInputBuffersToUser() override;
    bool ReadyToPrepare() override;

    // input buffer circulation
    void OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode) override;

    // output buffer circulation
    void OnRenderOutputBuffer(const MsgInfo &msg, BufferOperationMode mode) override;
    int32_t NotifySurfaceToRenderOutputBuffer(BufferInfo &info);
    void OnGetBufferFromSurface() override;
    bool GetOneBufferFromSurface();
    uint64_t GetSurfaceUsage() override;

    // stop/release
    void EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i) override;
    void CancelBufferToSurface(BufferInfo& info);

private:
    sptr<Surface> outputSurface_;
    BufferType outputBufferType_ = BufferType::SURFACE_BUFFER;
    uint32_t outBufferCnt_ = 0;
    BufferRequestConfig requestCfg_;
    BufferFlushConfig flushCfg_;
};
} // namespace OHOS::MediaAVCodec
#endif // HCODEC_HDECODER_H
