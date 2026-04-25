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

#ifndef VIDEO_ENC_DUAL_OUTPUT_SAMPLE_H
#define VIDEO_ENC_DUAL_OUTPUT_SAMPLE_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include "external_window.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "native_avformat.h"
#include "native_buffer_inner.h"
#include "nocopyable.h"

namespace OHOS::MediaAVCodec {

class VideoEncDualOutputSample : public NoCopyable {
public:
    int32_t defaultWidth = 0;
    int32_t defaultHeight = 0;

    VideoEncDualOutputSample();
    ~VideoEncDualOutputSample() override;

    static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData);
    static void OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
    static void OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
    static void OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
    static void OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData);
    static void OnNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                void *userData);

    static OH_AVFormat *CreateEncFormat(int32_t width, int32_t height, int64_t bitrate);

    using OpFunc = void (VideoEncDualOutputSample::*)(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    static const OpFunc opDispatch[24];
    void ExecuteOp(FuzzedDataProvider &fdp);
    void Cleanup();
    void LifecycleFuzz(FuzzedDataProvider &fdp);

private:
    OH_AVCodec *primary_ = nullptr;
    OH_AVCodec *secondary_ = nullptr;
    OHNativeWindow *nativeWindow_ = nullptr;
    OH_AVCodecCallback cb_{};
    OH_AVCodecAsyncCallback asyncCb_{};

    std::mutex primaryMutex_;
    std::mutex secondaryMutex_;

    std::atomic<bool> isRunning_{false};
    std::unique_ptr<std::thread> inputSurfaceLoop_;

    std::unique_lock<std::mutex> LockTarget(int32_t target);
    void StartInputLoop();
    void StopInloop();
    void InputFuncSurface();
    static int64_t GetSystemTimeUs();

    OH_AVFormat *BuildFuzzFormat(FuzzedDataProvider &fdp);
    OH_AVFormat *BuildFuzzParamFormat(FuzzedDataProvider &fdp);
    OH_AVCodec *GetTarget(int32_t target);

    void DoCreatePrimary(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoCreateSecondary(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoRegisterCallback(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoConfigure(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoGetSurface(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoStart(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoStop(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoFlush(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoReset(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoNotifyEos(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoSetParameter(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoGetOutputDesc(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoGetInputDesc(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoFreeOutputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoDestroy(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoPushInputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoQueryInputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoQueryOutputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoPrepare(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoIsValid(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoSetCallback(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoGetOutputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoGetInputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);
    void DoFreeOutputData(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp);

    using TaskItem = std::tuple<int32_t, int32_t, std::vector<uint8_t>>;
    void BootstrapEncoder(FuzzedDataProvider &fdp);
    void RunFuzzLoop(FuzzedDataProvider &fdp);
    void DispatchOneOp(FuzzedDataProvider &fdp, std::mutex &mtx, std::condition_variable &cv,
                       std::queue<TaskItem> &queue);
};

} // namespace OHOS::MediaAVCodec
#endif // VIDEO_ENC_DUAL_OUTPUT_SAMPLE_H
