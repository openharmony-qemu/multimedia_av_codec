/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef AVS3_DECODER_H
#define AVS3_DECODER_H

#include "video_decoder.h"
#include "Avs3Dec_Typedef.h"
#include "avcodec_info.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {

class Avs3Decoder : public VideoDecoder {
public:
    Avs3Decoder(const std::string &name, const std::string &path);
    ~Avs3Decoder() override;

    int32_t CreateDecoder() override;
    void DeleteDecoder() override;
    void DecoderFuncMatch() override;
    bool CheckVideoPixelFormat(VideoPixelFormat vpf) override;
    void ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth) override;
    void ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal = 0,
                             int32_t maxVal = INT_MAX) override;
    static int32_t GetCodecCapability(std::vector<CapabilityData> &capaArray);

protected:
    using Avs3DecodeFrameFuncType = int (*)(void* h, Uavs3dIoFrm* frm);
    using Avs3DestroyDecoderFuncType = void (*)(void* h);
    using Avs3DestroyFlushFuncType = int (*)(void* h, Uavs3dIoFrm* frm);
    using Avs3CpyDecoderFuncType = void (*)(Uavs3dIoFrm* dst, Uavs3dIoFrm* src, int bitDepth);

    int32_t Initialize() override;
    void InitParams() override;
    void ReleaseHandle() override;
    void SendFrame()override;
    int32_t DecodeFrameOnce() override;
    void ConvertDecOutToAVFrame();
    static int32_t CheckAvs3DecLibStatus();

private:
    Uavs3dContext avs3DecoderCtx_;
    Avs3CreateDecoderFuncType avs3DecoderCreateFunc_ = nullptr;
    Avs3DecodeFrameFuncType avs3DecoderFrameFunc_ = nullptr;
    Avs3DestroyFlushFuncType avs3DecoderFlushFunc_ = nullptr;
    Avs3DestroyDecoderFuncType avs3DecoderDestroyFunc_ = nullptr;
    Avs3CpyDecoderFuncType avs3DecoderImgCpyFunc_ = nullptr;
};
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVS3_DECODER_H