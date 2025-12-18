/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef VDEC_SAMPLE_H
#define VDEC_SAMPLE_H
#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <string>
#include <thread>
#include "iconsumer_surface.h"
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "sample_callback.h"
#include "securec.h"
#include "surface.h"
#include "surface_buffer.h"
#include "vcodec_signal.h"

namespace OHOS {
namespace MediaAVCodec {
class VideoDecSample : public NoCopyable, public VCodecSampleBase {
public:
    VideoDecSample();
    ~VideoDecSample();
    bool Create();
    bool CreateByMime();

    int32_t SetCallback(OH_AVCodecAsyncCallback callback, std::shared_ptr<VCodecSignal> &signal);
    int32_t RegisterCallback(OH_AVCodecCallback callback, std::shared_ptr<VCodecSignal> &signal);
    int32_t SetOutputSurface(const bool isNew = true);
    bool DoConfigure(OH_AVFormat* format);
    int32_t Configure();
    int32_t Start();
    int32_t Prepare();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    std::shared_ptr<OH_AVFormat> GetOutputDescription();
    int32_t SetParameter();
    int32_t PushInputData(std::shared_ptr<CodecBufferInfo> bufferInfo);
    int32_t ReleaseOutputData(std::shared_ptr<CodecBufferInfo> bufferInfo);
    int32_t IsValid(bool &isValid);
    int32_t SetOutputSurface(OHNativeWindow *window);
    OHNativeWindow* GetSurfaceWindow(const bool isNew);

    int32_t HandleInputFrame(std::shared_ptr<CodecBufferInfo> bufferInfo) override;
    int32_t HandleOutputFrame(std::shared_ptr<CodecBufferInfo> bufferInfo) override;
    bool WaitForEos();

    int32_t Operate() override;
    uint32_t getFrameOutputCount() { return frameOutputCount_.load(); };
    uint32_t frameCount_ = 10;
    std::string operation_ = "NULL";
    std::string mime_ = "";
    std::string inPath_ = "mpeg2.m2v";
    std::string outPath_ = "";
    int32_t sampleWidth_ = 720;
    int32_t sampleHeight_ = 480;
    int32_t samplePixel_ = AV_PIXEL_FORMAT_NV12;
    std::shared_ptr<OH_AVFormat> dyFormat_ = nullptr;
    std::unique_ptr<std::thread> inputLoop_ = nullptr;
    std::unique_ptr<std::thread> outputLoop_ = nullptr;

    static bool isHardware_;
    static bool needDump_;
    static bool isRosenWindow_;
    static uint64_t sampleTimout_;
    static uint64_t threadNum_;
    int32_t sampleId_ = 0;
    uint32_t defaultRotation_ = 0;
    uint32_t defaultBufferCount_ = 4;
    bool skipOutFrameHalfCheck_ = false;
    bool ohRotation_ = false;
    bool maxOutputBufferCount_ = false;
    bool maxInputBufferCount_ = false;
    bool scaleMode_ = false;
    bool lowLatency_ = false;
    bool setSurfaceParam_ = false;
    bool releaseOtherBuffer_ = false;
    bool setPixelFormat_ = true;
    std::string dumpKey_ = "";
    std::string dumpValue_ = "";

private:
    int32_t HandleInputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr);
    int32_t HandleOutputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr);
    bool InitInputFile();
    bool InitOutputFile();
    int32_t CreateAvccReader();
    int32_t CreateMpegReader();
    int32_t CreateH263Reader();
#ifdef SUPPORT_CODEC_VC1
    int32_t CreateVc1Reader();
    int32_t CreateWVc1Reader();
#endif
    int32_t CreateMsvideo1Reader();
    int32_t CreateWmv3Reader();
    int32_t CreateVp8Reader();
    int32_t CreateVp9Reader();
    int32_t CreateDvvideoReader();
#ifdef SUPPORT_CODEC_AV1
    int32_t CreateAv1Reader();
#endif
#ifdef SUPPORT_CODEC_RV
    int32_t CreateRv30Reader();
    int32_t CreateRv40Reader();
#endif
    int32_t CreateMpeg1Reader();
    OH_AVCodec *codec_ = nullptr;
    std::shared_ptr<VCodecSignal> signal_ = nullptr;

    bool needXps_ = true;
    bool isFirstEos_ = true;
    bool isFirstRead_ = true;
    std::atomic<uint32_t> frameInputCount_ = 0;
    std::atomic<uint32_t> frameOutputCount_ = 0;

    int64_t time_ = 0;
    bool isAVBufferMode_ = false;
    bool isSurfaceMode_ = false;
    bool isAvcStream_ = true; // true: AVC; false: HEVC
    bool isMpeg2Stream_ = true; // true: Mpeg2; false: Mpeg4
    bool needExtraData_ = false;
    bool isWmv3MainStream_ = false;
    bool rv30needExtraData_ = false;

private:
    OH_AVCodecAsyncCallback asyncCallback_;
    OH_AVCodecCallback callback_;
    std::shared_mutex codecMutex_;
    class SurfaceObject;
    std::shared_ptr<SurfaceObject> surafaceObj_ = nullptr;
    std::unique_ptr<uint8_t []> ReadBuffer_ = nullptr;
    uint32_t readBufferSize_ = 0;
    uint32_t preadBuffer_ = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VDEC_SAMPLE_H