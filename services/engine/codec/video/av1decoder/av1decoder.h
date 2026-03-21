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

#ifndef AV1_DECODER_H
#define AV1_DECODER_H

#include "video_decoder.h"
#include "Av1Dec_Typedef.h"
#include "avcodec_info.h"
#include "dav1d.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {

class Av1Decoder : public VideoDecoder {
public:
    explicit Av1Decoder(const std::string &name);
    ~Av1Decoder() override;

    int32_t CreateDecoder() override;
    void DeleteDecoder() override;
    void ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth) override;
    void FlushAllFrames() override;
    void FillHdrInfo(sptr<SurfaceBuffer> surfaceBuffer) override;
    static int32_t GetCodecCapability(std::vector<CapabilityData> &capaArray);

protected:
    int32_t Initialize() override;
    void InitParams() override;
    void SendFrame()override;
    int32_t DecodeFrameOnce() override;
    int32_t DecodeAv1FrameOnce();
    void ConvertDecOutToAVFrame();

private:
    AVPixelFormat ConvertAv1FmtToAVPixFmt(Dav1dPixelLayout fmt, int32_t bpc);
    void UpdateColorAspects(Dav1dSequenceHeader *seqHdr);
    int32_t ConvertHdrStaticMetadata(Dav1dContentLightLevel *contentLight, Dav1dMasteringDisplay *masteringDisplay,
                                     std::vector<uint8_t> &staticMetadataVec);
    bool CheckStateRunning();

    Dav1dContext *dav1dCtx_ = nullptr;
    AV1_DEC_INARGS av1DecoderInputArgs_;
    Dav1dPicture *av1DecOutputImg_ = nullptr;
    AV1ColorSpaceInfo colorSpaceInfo_;
};

void AV1DecLog(void *cookie, const char *format, va_list ap);
void AV1FreeCallback(const uint8_t *data, void *userData);
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AV1_DECODER_H