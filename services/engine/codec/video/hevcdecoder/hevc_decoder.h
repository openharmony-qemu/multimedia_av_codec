/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef HEVC_DECODER_H
#define HEVC_DECODER_H

#include "video_decoder.h"
#include "HevcDec_Typedef.h"
#include "avcodec_info.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {

class HevcDecoder : public VideoDecoder {
public:
    explicit HevcDecoder(const std::string &name);
    ~HevcDecoder() override;

    int32_t CreateDecoder() override;
    void DeleteDecoder() override;
    bool CheckVideoPixelFormat(VideoPixelFormat vpf) override;
    void ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth) override;

    void FlushAllFrames() override;
    void FillHdrInfo(sptr<SurfaceBuffer> surfaceBuffer) override;
    int32_t GetDecoderWidthStride(void) override;
    void CalculateBufferSize() override;
    static int32_t GetCodecCapability(std::vector<CapabilityData> &capaArray);

protected:
    void HevcFuncMatch();
    void ReleaseHandle();
    void InitParams() override;
    void InitHdrParams();

    int32_t Initialize() override;
    void SendFrame()override;
    int32_t DecodeFrameOnce() override;
    void ConvertDecOutToAVFrame(int32_t bitDepth);
    static int32_t CheckHevcDecLibStatus();

private:
    void UpdateColorAspects(const HEVC_COLOR_SPACE_INFO &colorInfo);
    int32_t ConvertHdrStaticMetadata(const HEVC_HDR_METADATA &hevcHdrMetadata,
                                     std::vector<uint8_t> &staticMetadataVec);
private:
    void* handle_ = nullptr;
    HEVC_DEC_INIT_PARAM initParams_;
    HEVC_DEC_INARGS hevcDecoderInputArgs_;
    HEVC_DEC_OUTARGS hevcDecoderOutpusArgs_;

    using CreateHevcDecoderFuncType = INT32 (*)(HEVC_DEC_HANDLE *phDecoder, HEVC_DEC_INIT_PARAM *pstInitParam);
    using DecodeFuncType = INT32 (*)(HEVC_DEC_HANDLE hDecoder, HEVC_DEC_INARGS *pstInArgs,
                                                  HEVC_DEC_OUTARGS *pstOutArgs);
    using FlushFuncType = INT32 (*)(HEVC_DEC_HANDLE hDecoder, HEVC_DEC_OUTARGS *pstOutArgs);
    using DeleteFuncType = INT32 (*)(HEVC_DEC_HANDLE hDecoder);
    HEVC_DEC_HANDLE hevcSDecoder_ = nullptr;
    CreateHevcDecoderFuncType hevcDecoderCreateFunc_ = nullptr;
    DecodeFuncType hevcDecoderDecodecFrameFunc_ = nullptr;
    FlushFuncType hevcDecoderFlushFrameFunc_ = nullptr;
    DeleteFuncType hevcDecoderDeleteFunc_ = nullptr;
    
    HEVC_COLOR_SPACE_INFO colorSpaceInfo_;
};

void HevcDecLog(UINT32 channelId, IHW265VIDEO_ALG_LOG_LEVEL eLevel, INT8 *pMsg, ...);
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // HEVC_DECODER_H