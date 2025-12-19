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

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {

class Av1Decoder : public VideoDecoder {
public:
    explicit Av1Decoder(const std::string &name, const std::string &path);
    ~Av1Decoder() override;

    int32_t CreateDecoder() override;
    void DeleteDecoder() override;
    void DecoderFuncMatch() override;
    bool CheckVideoPixelFormat(VideoPixelFormat vpf) override;
    void ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth) override;
    void ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal = 0,
                             int32_t maxVal = INT_MAX) override;
    void FlushAllFrames() override;
    void FillHdrInfo(sptr<SurfaceBuffer> surfaceBuffer) override;
    static int32_t GetCodecCapability(std::vector<CapabilityData> &capaArray);

protected:
    using Av1DecoderLogFunc = void (*)(void *cookie, const char *format, va_list ap);
    using Av1CreateDecoderFuncType = INT32 (*)(void **av1Decoder, Av1DecoderLogFunc logFxn);
    using Av1DecodeFrameFuncType = INT32 (*)(void *av1Decoder, const unsigned char *frame, unsigned int frame_size,
                                             int64_t timestamp);
    using Av1GetFrameFuncType = INT32 (*)(void *av1Decoder, Dav1dPicture *outputImg);
    using Av1PictureUnrefFuncType = void (*)(Dav1dPicture *const picture);
    using Av1DestroyDecoderFuncType = void (*)(void **av1Decoder);

    int32_t Initialize() override;
    void InitParams() override;
    void ReleaseHandle() override;
    void SendFrame()override;
    int32_t DecodeFrameOnce() override;
    int32_t DecodeAv1FrameOnce();
    void ConvertDecOutToAVFrame();
    static int32_t CheckAv1DecLibStatus();

private:
    AVPixelFormat ConvertAv1FmtToAVPixFmt(Dav1dPixelLayout fmt, int32_t bpc);
    void UpdateColorAspects(Dav1dSequenceHeader *seqHdr);
    int32_t ConvertHdrStaticMetadata(Dav1dContentLightLevel *contentLight, Dav1dMasteringDisplay *masteringDisplay,
                                     std::vector<uint8_t> &staticMetadataVec);

    AV1_DEC_HANDLE av1DecHandle_ = nullptr;
    AV1_DEC_INARGS av1DecoderInputArgs_;
    Dav1dPicture *av1DecOutputImg_ = nullptr;
    Av1CreateDecoderFuncType av1DecoderCreateFunc_ = nullptr;
    Av1DecodeFrameFuncType av1DecoderFrameFunc_ = nullptr;
    Av1GetFrameFuncType av1DecoderGetFrameFunc_ = nullptr;
    Av1PictureUnrefFuncType av1DecoderPictureUnrefFunc_ = nullptr;
    Av1DestroyDecoderFuncType av1DecoderDestroyFunc_ = nullptr;
    AV1ColorSpaceInfo colorSpaceInfo_;
};

void AV1DecLog(void *cookie, const char *format, va_list ap);
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AV1_DECODER_H