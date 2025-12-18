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

#ifndef VDEC_ASYNC_SAMPLE_H
#define VDEC_ASYNC_SAMPLE_H
#include "func_sample_decoder_base.h"

namespace OHOS {
namespace MediaAVCodec {
class VideoDecAsyncSample : public NoCopyable {
public:
    explicit VideoDecAsyncSample(std::shared_ptr<VDecSignal> signal);
    ~VideoDecAsyncSample();
    bool CreateVideoDecMockByMime(const std::string &mime);
    bool CreateVideoDecMockByName(const std::string &name);

    int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb);
    int32_t SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb);
    int32_t SetOutputSurface();
    int32_t SetOutputSurface(std::shared_ptr<SurfaceMock> surface);
    int32_t Configure(std::shared_ptr<FormatMock> format);
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    std::shared_ptr<FormatMock> GetOutputDescription();
    std::shared_ptr<FormatMock> GetCodecInfo();
    int32_t SetParameter(std::shared_ptr<FormatMock> format);
    int32_t PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr);
    int32_t RenderOutputData(uint32_t index);
    int32_t FreeOutputData(uint32_t index);
    int32_t PushInputBuffer(uint32_t index);
    int32_t RenderOutputBuffer(uint32_t index);
    int32_t RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs);
    int32_t FreeOutputBuffer(uint32_t index);
    bool IsValid();
    int32_t SetVideoDecryptionConfig();

    void SetOutPath(const std::string &path);
    void SetSource(const std::string &path);
    void SetSourceType(bool isAvcStream);
    bool needCheckSHA_ = false;
    bool isAVBufferMode_ = false;
    int32_t testParam_ = VCodecTestParam::SW_AVC;
    bool renderAtTimeFlag_ = false;
    static bool needDump_;
    bool detailedError_ = false;

private:
    void FlushInner();
    void PrepareInner();
    void WaitForEos();

    void RunInner();
    void OutputLoopFunc();
    void InputLoopFunc();
    bool IsCodecData(const uint8_t *const bufferAddr);
    int32_t ReadOneFrame(uint8_t *bufferAddr, uint32_t &flags);
    int32_t OutputLoopInner();
    int32_t InputLoopInner();

    void RunInnerExt();
    void OutputLoopFuncExt();
    void InputLoopFuncExt();
    void CheckFormatKey();
    int32_t OutputLoopInnerExt();
    int32_t InputLoopInnerExt();
    void CheckSHA();
    void UpdateSHA(const char *addr, int32_t size);
    void ProcessEosFrame();
    int32_t CreateAvccReader();
    int32_t CreateMpegReader();
    int32_t CreateH263Reader();
#ifdef SUPPORT_CODEC_VC1
    int32_t CreateVc1Reader();
    int32_t CreateWVc1Reader();
#endif
    int32_t CreateMsvideo1Reader();
    int32_t CreateWmv3Reader();
#ifdef SUPPORT_CODEC_AV1
    int32_t CreateAv1Reader();
#endif
#ifdef SUPPORT_CODEC_RV
    int32_t CreateRv30Reader();
    int32_t CreateRv40Reader();
#endif
    int32_t CreateMpeg1Reader();
    int32_t CreateDvvideoReader();
    int32_t CreateReader(const std::string& inPath);
    bool CompareHdrInfo(std::shared_ptr<AVBufferMock> buffer);
    bool CompareMetadata(std::shared_ptr<std::ifstream> file, int32_t size,
        std::shared_ptr<SurfaceBufferMock> surfaceBufferMock, bool isDynamic);
    std::shared_ptr<VideoDecMock> videoDec_ = nullptr;
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::ofstream> outFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::shared_ptr<VDecSignal> signal_ = nullptr;
    std::string inPath_;
    std::string outPath_;
    std::string outSurfacePath_;
    uint32_t datSize_ = 0;
    uint32_t frameInputCount_ = 0;
    uint32_t frameOutputCount_ = 0;
    bool isSurfaceMode_ = false;
    int32_t dataProducerType_ = AVC_STREAM;
    bool isKeepExecuting_ = false;
    int64_t time_ = 0;
    sptr<Surface> consumer_ = nullptr;
    sptr<Surface> producer_ = nullptr;
    std::shared_ptr<AvccReader> avccReader_ = nullptr;
    std::shared_ptr<MpegReader> mpegReader_ = nullptr;
    std::shared_ptr<H263Reader> h263Reader_ = nullptr;
#ifdef SUPPORT_CODEC_AV1
    std::shared_ptr<Av1Reader> av1Reader_ = nullptr;
#endif
#ifdef SUPPORT_CODEC_VC1
    std::shared_ptr<Vc1Reader> vc1Reader_ = nullptr;
    std::shared_ptr<WVc1Reader> wvc1Reader_ = nullptr;
#endif
    std::shared_ptr<Msvideo1Reader> msvideo1Reader_ = nullptr;
    std::shared_ptr<Wmv3Reader> wmv3Reader_ = nullptr;
    std::shared_ptr<std::ifstream> dynamicMetadataFile_ = nullptr;
    std::shared_ptr<std::ifstream> staticMetadataFile_ = nullptr;
#ifdef SUPPORT_CODEC_RV
    std::shared_ptr<Rv30Reader> rv30Reader_ = nullptr;
    std::shared_ptr<Rv40Reader> rv40Reader_ = nullptr;
#endif
    std::shared_ptr<Mpeg1Reader> mpeg1Reader_ = nullptr;
    std::shared_ptr<DvvideoReader> dvvideoReader_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VDEC_ASYNC_SAMPLE_H