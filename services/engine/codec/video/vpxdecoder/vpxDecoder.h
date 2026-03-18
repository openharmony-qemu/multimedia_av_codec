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
#include "vpx_decoder.h"
#include "tools_common.h"
#include "video_reader.h"
#include "libavutil/mastering_display_metadata.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
class VpxDecoder : public VideoDecoder {
public:
    explicit VpxDecoder(const std::string &name);
    ~VpxDecoder() override;

    int32_t CreateDecoder() override;
    void DeleteDecoder() override;
    bool CheckVideoPixelFormat(VideoPixelFormat vpf) override;
    void ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth) override;
    void ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal = 0,
        int32_t maxVal = INT_MAX) override;
    void ConfigureHdrMetadata(const Format &format) override;
    void FillHdrInfo(sptr<SurfaceBuffer> surfaceBuffer) override;
    static int32_t GetCodecCapability(std::vector<CapabilityData> &capaArray);

protected:

    int32_t Initialize() override;
    void InitParams() override;
    void FlushAllFrames() override;
    void SendFrame()override;
    int32_t DecodeFrameOnce() override;
    void ConvertDecOutToAVFrame();

private:
    struct HdrMetadata {
        double displayPrimariesX[3] = {0, 0, 0};
        double displayPrimariesY[3] = {0, 0, 0};
        INT32 whitePointX = 0;
        INT32 whitePointY = 0;
        INT32 maxDisplayMasteringLuminance = 0;
        INT32 minDisplayMasteringLuminance = 0;
        INT32 maxContentLightLevel = 0;
        INT32 maxPicAverageLightLevel = 0;
    };

    struct ColorSpaceInfo {
        int32_t fullRangeFlag = 0;
        int32_t colorDescriptionPresentFlag = 0;
        int32_t colorPrimaries = 0;
        int32_t transferCharacteristic = 0;
        int32_t matrixCoeffs = 0;
    };
    AVPixelFormat ConvertVpxFmtToAVPixFmt(vpx_img_fmt_t fmt);
    int32_t ConvertHdrStaticMetadata(const HdrMetadata &hdrMetadata,
                                     std::vector<uint8_t> &staticMetadataVec);
    ColorSpaceInfo colorSpaceInfo_;
    HdrMetadata hdrMetadata_;
    VPX_DEC_HANDLE vpxDecHandle_ = nullptr;
    VpxDecInArgs vpxDecoderInputArgs_;
    vpx_image_t *vpxDecOutputImg_ = nullptr;
    static void GetVp9CapProf(std::vector<CapabilityData> &capaArray);
    static void GetVp8CapProf(std::vector<CapabilityData> &capaArray);

    int VpxCreateDecoderFunc(void **vpxDecoder, const char *name);
    int VpxDestroyDecoderFunc(void **vpxDecoder);
    int VpxDecodeFrameFunc(void *vpxDecoder, const unsigned char *frame, unsigned int frameSize);
    int VpxGetFrameFunc(void *vpxDecoder, vpx_image_t **outputImg);
};
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VPX_DECODER_H