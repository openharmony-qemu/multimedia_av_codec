/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef VENC_DUALOUTPUT_SAMPLE_H
#define VENC_DUALOUTPUT_SAMPLE_H

#include <atomic>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "native_avcodec_base.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "native_buffer_inner.h"
#include "native_window.h"
#include "window.h"
#include "sample_callback.h"
#include "vcodec_signal.h"

namespace OHOS {
namespace MediaAVCodec {
inline std::string FAST_IMAGE_PATH = "/system/lib64/ndk/libfast_image.so";
enum class EncoderType { PRIMARY, SECONDARY };

class VideoEncDualOutputSample : public NoCopyable, public VCodecSampleBase {
public:
    VideoEncDualOutputSample();
    ~VideoEncDualOutputSample();

    bool CreatePrimary(const std::string &mime);
    bool CreateSecondary();

    OH_AVCodec *GetCodec(EncoderType type);
    OH_AVFormat *GetFormat(EncoderType type);

    int32_t SetBasicFormat(EncoderType type, int32_t width, int32_t height, int32_t pixelFormat,
                           double frameRate = 30.0);
    int32_t SetDownsampling(EncoderType type, int32_t downWidth, int32_t downHeight);
    int32_t SetCrop(EncoderType type, int32_t left, int32_t top, int32_t right, int32_t bottom);
    int32_t SetDropFrameRate(EncoderType type, double frameRate);

    int32_t SetCallback(EncoderType type, OH_AVCodecAsyncCallback callback, std::shared_ptr<VCodecSignal> &signal);
    int32_t SetCallback(EncoderType type, OH_AVCodecAsyncCallback callback, void *userData = nullptr);
    int32_t RegisterCallback(EncoderType type, OH_AVCodecCallback callback, std::shared_ptr<VCodecSignal> &signal);
    int32_t RegisterCallback(EncoderType type, OH_AVCodecCallback callback, void *userData = nullptr);

    int32_t Configure(EncoderType type);
    int32_t Prepare(EncoderType type);
    int32_t Start(EncoderType type);
    int32_t Stop(EncoderType type);
    int32_t Flush(EncoderType type);
    int32_t Reset(EncoderType type);
    int32_t Release(EncoderType type);
    int32_t NotifyEos(EncoderType type);
    int32_t IsValid(EncoderType type, bool &isValid);
    bool WaitForEos(EncoderType type);

    std::shared_ptr<OH_AVFormat> GetOutputDescription(EncoderType type);
    std::shared_ptr<OH_AVFormat> GetInputDescription(EncoderType type);
    int32_t SetParameter(EncoderType type, OH_AVFormat *format);

    int32_t QueryInputBuffer(EncoderType type, uint32_t *index, int64_t timeoutUs);
    int32_t QueryOutputBuffer(EncoderType type, uint32_t *index, int64_t timeoutUs);
    int32_t PushInputBuffer(EncoderType type, uint32_t index);
    int32_t FreeOutputBuffer(EncoderType type, uint32_t index);

    bool CreateSurface();
    void StopSurfaceInput();

    int32_t HandleOutputFrameByEncoderType(EncoderType type, std::shared_ptr<CodecBufferInfo> bufferInfo);

    int32_t HandleInputFrame(std::shared_ptr<CodecBufferInfo> bufferInfo) override;
    int32_t HandleOutputFrame(std::shared_ptr<CodecBufferInfo> bufferInfo) override;
    int32_t Operate() override;

    uint32_t frameCount_ = 10;
    std::string operation_ = "NULL";
    std::string mime_ = "";
    std::string inPath_ = "1280_720_nv.yuv";
    std::string outPath_ = "";
    int32_t sampleWidth_ = 1280;
    int32_t sampleHeight_ = 720;
    int32_t samplePixel_ = AV_PIXEL_FORMAT_NV12;

    static bool needDump_;
    static uint64_t sampleTimout_;

private:
    struct EncoderContext {
        OH_AVCodec *codec = nullptr;
        std::shared_ptr<OH_AVFormat> format = nullptr;
        std::shared_ptr<VCodecSignal> signal = nullptr;
        std::atomic<uint32_t> frameInputCount{0};
        std::atomic<uint32_t> frameOutputCount{0};
        int64_t time = 0;
        bool isAVBufferMode = false;
        bool isFirstFrame = true;
        bool isFirstEos = true;
        int32_t sampleId = 0;
        OH_AVCodecAsyncCallback asyncCallback {};
        OH_AVCodecCallback callback {};
        std::shared_mutex codecMutex;
    };

    EncoderContext &GetEncoderByEncoderType(EncoderType type);

    EncoderContext primaryEncoder_;
    EncoderContext secondaryEncoder_;
    int32_t primarySampleId_ = 0;

    // Primary frame handling helpers
    int32_t HandleInputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr);
    int32_t HandleOutputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr);
    int32_t PushInputData(EncoderContext &encoderContext, std::shared_ptr<CodecBufferInfo> bufferInfo);
    int32_t ReleaseOutputData(EncoderContext &encoderContext, std::shared_ptr<CodecBufferInfo> bufferInfo);

    // Secondary frame handling helpers
    int32_t HandleOutputFrameSecondaryInner(uint8_t *addr, OH_AVCodecBufferAttr &attr);
    int32_t ReleaseOutputDataSecondary(EncoderContext &encoderContext, std::shared_ptr<CodecBufferInfo> bufferInfo);

    bool InitFile(EncoderType type);

    // Surface input
    OHNativeWindow *nativeWindow_ = nullptr;
    std::unique_ptr<std::thread> inputLoop_;
    void InputFuncSurface();
    int32_t FlushSurface(OHNativeWindowBuffer *ohNativeWindowBuffer, OH_NativeBuffer *nativeBuffer);
    void *MapNativeBuffer(OH_NativeBuffer *nativeBuffer);
    bool ValidateMappedBuffer(char *dst, OH_NativeBuffer *nativeBuffer,
        OHNativeWindowBuffer *ohNativeWindowBuffer, int32_t &stride);
    void FillYuvData(char *dst, int32_t stride);
    void NotifyEosForAll(OH_NativeBuffer *nativeBuffer, OHNativeWindowBuffer *ohNativeWindowBuffer);
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif // VENC_DUALOUTPUT_SAMPLE_H
