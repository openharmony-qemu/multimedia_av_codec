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

#ifndef VENC_ASYNC_SAMPLE_H
#define VENC_ASYNC_SAMPLE_H
#include "func_sample_encoder_base.h"

namespace OHOS {
namespace MediaAVCodec {
class VideoEncAsyncSample : public NoCopyable {
public:
    explicit VideoEncAsyncSample(std::shared_ptr<VEncSignal> signal);
    virtual ~VideoEncAsyncSample();
    bool CreateVideoEncMockByMime(const std::string &mime);
    bool CreateVideoEncMockByName(const std::string &name);

    int32_t Release();
    int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb);
    int32_t SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb);
    int32_t SetCallback(std::shared_ptr<MediaCodecParameterCallbackMock> cb);
    int32_t SetCallback(std::shared_ptr<MediaCodecParameterWithAttrCallbackMock> cb);
    int32_t Configure(std::shared_ptr<FormatMock> format);
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    std::shared_ptr<FormatMock> GetOutputDescription();
    std::shared_ptr<FormatMock> GetInputDescription();
    int32_t SetParameter(std::shared_ptr<FormatMock> format);
    int32_t NotifyEos();
    int32_t PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr);
    int32_t FreeOutputData(uint32_t index);
    int32_t PushInputBuffer(uint32_t index);
    int32_t PushInputParameter(uint32_t index);
    int32_t FreeOutputBuffer(uint32_t index);
    int32_t CreateInputSurface();
    bool IsValid();
    int32_t SetCustomBuffer(std::shared_ptr<AVBufferMock> buffer);

    void SetOutPath(const std::string &path);
    void SetInPath(const std::string &path);
    int32_t testParam_ = VCodecTestParam::SW_AVC;
    bool needCheckSHA_ = false;
    bool needSleep_ = false;
    static bool needDump_;
    bool isAVBufferMode_ = false;
    bool isTemporalScalabilitySyncIdr_ = false;
    bool isDiscardFrame_ = false;
    std::string roiRects_ = "";
    bool enableVariableFrameRate_ = false;
    int32_t frameIndex_ = 0;

private:
    void FlushInner();
    void PrepareInner();
    void WaitForEos();

    void InputFuncSurface();
    int32_t InputProcess(OH_NativeBuffer *nativeBuffer, OHNativeWindowBuffer *ohNativeWindowBuffer);

    int32_t ReadOneFrame();

    void RunInner();
    void InputParamLoopFunc();

    void OutputLoopFunc();
    void InputLoopFunc();
    int32_t OutputLoopInner();
    int32_t InputLoopInner();

    void RunInnerExt();
    void OutputLoopFuncExt();
    void InputLoopFuncExt();
    int32_t OutputLoopInnerExt();
    int32_t InputLoopInnerExt();
    void InputLoopInnerFeatureExt(OH_AVCodecBufferAttr &attr);
    void CheckFormatKey(OH_AVCodecBufferAttr attr, std::shared_ptr<AVBufferMock> buffer);
    void InputLtrParam(std::shared_ptr<FormatMock> format, int32_t frameInputCount,
                    std::shared_ptr<AVBufferMock> buffer);
    void CheckSHA();
    void PerformEosFrameAndVerifiedSHA();
    std::string GetFileExtension(const std::string& filePath);
    std::shared_ptr<VideoEncMock> videoEnc_ = nullptr;
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::ofstream> outFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::unique_ptr<std::thread> inputSurfaceLoop_;
    std::shared_ptr<VEncSignal> signal_ = nullptr;
    std::string inPath_;
    std::string outPath_;
    std::string outSurfacePath_;
    int32_t frameInputCount_ = 0;
    int32_t frameOutputCount_ = 0;
    bool isFirstFrame_ = true;
    bool isSurfaceMode_ = false;
    bool isSetParamCallback_ = false;
    int64_t time_ = 0;
    sptr<Surface> consumer_ = nullptr;
    sptr<Surface> producer_ = nullptr;
    OHNativeWindow *nativeWindow_ = nullptr;
    LtrParam ltrParam;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VENC_ASYNC_SAMPLE_H