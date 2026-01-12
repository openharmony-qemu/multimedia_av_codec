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

#ifndef VPX_DECODER_H
#define VPX_DECODER_H

#include "video_decoder.h"
#include "VpxDec_Typedef.h"
#include "avcodec_info.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
class VpxDecoder : public VideoDecoder {
public:
    VpxDecoder(const std::string &name, const std::string &path);
    ~VpxDecoder() override;

    int32_t CreateDecoder() override;
    void DeleteDecoder() override;
    void DecoderFuncMatch() override;
    bool CheckVideoPixelFormat(VideoPixelFormat vpf) override;
    void ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth) override;
    void ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal = 0,
                             int32_t maxVal = INT_MAX) override;
    static int32_t GetCodecCapability(std::vector<CapabilityData> &capaArray);

protected:
    using VpxCreateDecoderFuncType = INT32 (*)(void **vpxDecoder, const char *name);
    using VpxDecodeFrameFuncType = INT32 (*)(void *vpxDecoder, const unsigned char *frame, unsigned int frameSize);
    using VpxGetFrameFuncType = INT32 (*)(void *vpxDecoder, VpxImage **outputImg);
    using VpxDestroyDecoderFuncType = INT32 (*)(void **vpxDecoder);

    int32_t Initialize() override;
    void InitParams() override;
    void FlushAllFrames() override;
    void ReleaseHandle() override;
    void SendFrame()override;
    int32_t DecodeFrameOnce() override;
    void ConvertDecOutToAVFrame();
    static int32_t CheckVpxDecLibStatus();

private:
    AVPixelFormat ConvertVpxFmtToAVPixFmt(VpxImageFmt fmt);

    VPX_DEC_HANDLE vpxDecHandle_ = nullptr;
    VpxDecInArgs vpxDecoderInputArgs_;
    VpxImage *vpxDecOutputImg_ = nullptr;
    VpxCreateDecoderFuncType vpxDecoderCreateFunc_ = nullptr;
    VpxDecodeFrameFuncType vpxDecoderFrameFunc_ = nullptr;
    VpxGetFrameFuncType vpxDecoderGetFrameFunc_ = nullptr;
    VpxDestroyDecoderFuncType vpxDecoderDestroyFunc_ = nullptr;
    static void GetVp9CapProf(std::vector<CapabilityData> &capaArray);
    static void GetVp8CapProf(std::vector<CapabilityData> &capaArray);
};
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VPX_DECODER_H