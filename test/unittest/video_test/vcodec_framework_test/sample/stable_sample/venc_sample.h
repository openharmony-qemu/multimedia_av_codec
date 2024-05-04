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

#ifndef VEN_SAMPLE_H
#define VEN_SAMPLE_H
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
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "native_buffer_inner.h"
#include "securec.h"
#include "surface.h"
#include "surface_buffer.h"
#include "vcodec_signal.h"

namespace OHOS {
namespace MediaAVCodec {
class VideoEncSample;
using VideoEncSignal = VCodecSignal<VideoEncSample>;

class VideoEncSample : public NoCopyable {
public:
    VideoEncSample();
    ~VideoEncSample();
    bool Create();
    bool CreateByMime();

    int32_t SetCallback(OH_AVCodecAsyncCallback callback, std::shared_ptr<VideoEncSignal> &signal);
    int32_t RegisterCallback(OH_AVCodecCallback callback, std::shared_ptr<VideoEncSignal> &signal);
    int32_t GetInputSurface();
    int32_t Configure();
    int32_t Start();
    int32_t Prepare();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    std::shared_ptr<OH_AVFormat> GetOutputDescription();
    std::shared_ptr<OH_AVFormat> GetInputDescription();
    int32_t SetParameter();
    int32_t PushInputData(uint32_t index, OH_AVCodecBufferAttr attr = {0, 0, 0, 0});
    int32_t ReleaseOutputData(uint32_t index);
    int32_t NotifyEos();
    int32_t IsValid(bool &isValid);

    int32_t HandleInputFrame(uint32_t &index, OH_AVCodecBufferAttr &attr);
    int32_t HandleOutputFrame(uint32_t &index, OH_AVCodecBufferAttr &attr);
    int32_t HandleInputFrame(OH_AVMemory *data, OH_AVCodecBufferAttr &attr);
    int32_t HandleOutputFrame(OH_AVMemory *data, OH_AVCodecBufferAttr &attr);
    int32_t HandleInputFrame(OH_AVBuffer *data);
    int32_t HandleOutputFrame(OH_AVBuffer *data);
    bool WaitForEos();

    int32_t Operate();
    uint32_t frameCount_ = 10;
    std::string operation_ = "NULL";
    std::string mime_ = "";
    std::string inPath_ = "1280_720_nv.yuv";
    std::string outPath_ = "";
    int32_t sampleWidth_ = 1280;
    int32_t sampleHeight_ = 720;
    int32_t samplePixcel_ = AV_PIXEL_FORMAT_NV12;
    std::shared_ptr<OH_AVFormat> dyFormat_ = nullptr;
    std::unique_ptr<std::thread> inputLoop_ = nullptr;
    std::unique_ptr<std::thread> outputLoop_ = nullptr;

    static bool needDump_;
    static uint64_t sampleTimout_;
    static uint64_t threadNum_;
    int32_t sampleId_ = 0;

private:
    int32_t SetAVBufferAttr(OH_AVBuffer *avBuffer, OH_AVCodecBufferAttr &attr);
    int32_t HandleInputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr);
    int32_t HandleOutputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr);

    int32_t InputProcess(OH_NativeBuffer *nativeBuffer, OHNativeWindowBuffer *ohNativeWindowBuffer);
    void InputFuncSurface();
    bool InitFile();

    OH_AVCodec *codec_ = nullptr;
    std::shared_ptr<VideoEncSignal> signal_ = nullptr;
    std::atomic<uint32_t> frameInputCount_ = 0;
    std::atomic<uint32_t> frameOutputCount_ = 0;
    int64_t time_ = 0;
    bool isAVBufferMode_ = false;
    bool isSurfaceMode_ = false;
    bool isFirstFrame_ = true;
    bool isFirstEos_ = true;

private:
    OH_AVCodecAsyncCallback asyncCallback_;
    OH_AVCodecCallback callback_;
    OHNativeWindow *nativeWindow_ = nullptr;
    sptr<Surface> consumer_ = nullptr;
    sptr<Surface> producer_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VEN_SAMPLE_H