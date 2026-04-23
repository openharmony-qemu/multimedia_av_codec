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

#include "video_enc_dual_output_sample.h"
#include <algorithm>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <vector>
#include "native_avbuffer.h"
#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include "securec.h"

using namespace std;
using namespace OHOS::MediaAVCodec;

// ======================== Constructor / Destructor ========================

VideoEncDualOutputSample::VideoEncDualOutputSample()
{
    (void)memset_s(&cb_, sizeof(cb_), 0, sizeof(cb_));
    cb_.onError = OnError;
    cb_.onStreamChanged = OnStreamChanged;
    cb_.onNeedInputBuffer = OnNeedInputBuffer;
    cb_.onNewOutputBuffer = OnNewOutputBuffer;
    (void)memset_s(&asyncCb_, sizeof(asyncCb_), 0, sizeof(asyncCb_));
    asyncCb_.onError = OnError;
    asyncCb_.onStreamChanged = OnStreamChanged;
    asyncCb_.onNeedInputData = OnNeedInputData;
    asyncCb_.onNeedOutputData = OnNewOutputData;
}

VideoEncDualOutputSample::~VideoEncDualOutputSample()
{
    Cleanup();
}

// ======================== Callbacks ========================

void VideoEncDualOutputSample::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData) {}

void VideoEncDualOutputSample::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
    }
}

void VideoEncDualOutputSample::OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
}

void VideoEncDualOutputSample::OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    OH_VideoEncoder_FreeOutputBuffer(codec, index);
}

void VideoEncDualOutputSample::OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData) {}

void VideoEncDualOutputSample::OnNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
                                               OH_AVCodecBufferAttr *attr, void *userData)
{
    OH_VideoEncoder_FreeOutputData(codec, index);
}

// ======================== Format Builders ========================

OH_AVFormat *VideoEncDualOutputSample::CreateEncFormat(int32_t width, int32_t height, int64_t bitrate)
{
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_WIDTH, width);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_HEIGHT, height);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_PIXEL_FORMAT, 2);     // 2: NV12
    OH_AVFormat_SetDoubleValue(fmt, OH_MD_KEY_FRAME_RATE, 30.0); // 30.0: 30fps
    OH_AVFormat_SetLongValue(fmt, OH_MD_KEY_BITRATE, bitrate);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, 0); // 0: CBR
    return fmt;
}

OH_AVFormat *VideoEncDualOutputSample::BuildFuzzFormat(FuzzedDataProvider &fdp)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        return nullptr;
    }
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, fdp.ConsumeIntegral<int32_t>());
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, fdp.ConsumeIntegral<int32_t>());
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, fdp.ConsumeIntegral<int32_t>());
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, fdp.ConsumeFloatingPoint<double>());
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, fdp.ConsumeIntegral<int32_t>());
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, fdp.ConsumeIntegral<int32_t>());
    if (fdp.ConsumeBool()) {
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, fdp.ConsumeIntegral<int64_t>());
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_MAX_BITRATE, fdp.ConsumeIntegral<int64_t>());
    }
    if (fdp.ConsumeBool()) {
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, fdp.ConsumeIntegral<int32_t>());
    }
    if (fdp.ConsumeBool()) {
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH,
                                fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT,
                                fdp.ConsumeIntegral<int32_t>());
    }
    if (fdp.ConsumeBool()) {
        OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DROP_TO_FRAME_RATE,
                                   fdp.ConsumeFloatingPoint<double>());
    }
    return format;
}

OH_AVFormat *VideoEncDualOutputSample::BuildFuzzParamFormat(FuzzedDataProvider &fdp)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        return nullptr;
    }
    int32_t count = fdp.ConsumeIntegralInRange<int32_t>(1, 6); // 6: max param count
    for (int32_t i = 0; i < count; i++) {
        std::string key = fdp.ConsumeRandomLengthString(64); // 64: max key length
        if (fdp.ConsumeBool()) {
            OH_AVFormat_SetIntValue(format, key.c_str(), fdp.ConsumeIntegral<int32_t>());
        } else {
            OH_AVFormat_SetDoubleValue(format, key.c_str(), fdp.ConsumeFloatingPoint<double>());
        }
    }
    return format;
}

// ======================== Target & Lock ========================

OH_AVCodec *VideoEncDualOutputSample::GetTarget(int32_t target)
{
    if (target == 0) { // 0: primary
        return primary_;
    }
    if (target == 1) { // 1: secondary
        return secondary_;
    }
    return nullptr;
}

std::unique_lock<std::mutex> VideoEncDualOutputSample::LockTarget(int32_t target)
{
    if (target == 0) { // 0: primary
        return std::unique_lock<std::mutex>(primaryMutex_);
    }
    if (target == 1) { // 1: secondary
        return std::unique_lock<std::mutex>(secondaryMutex_);
    }
    return std::unique_lock<std::mutex>();
}

// ======================== Surface Input Thread ========================

void VideoEncDualOutputSample::StartInputLoop()
{
    bool expected = false;
    if (!isRunning_.compare_exchange_strong(expected, true)) {
        return;
    }
    if (inputSurfaceLoop_ != nullptr && inputSurfaceLoop_->joinable()) {
        inputSurfaceLoop_->join();
    }
    inputSurfaceLoop_ = std::make_unique<std::thread>(&VideoEncDualOutputSample::InputFuncSurface, this);
}

void VideoEncDualOutputSample::StopInloop()
{
    isRunning_.store(false);
    if (inputSurfaceLoop_ != nullptr && inputSurfaceLoop_->joinable()) {
        inputSurfaceLoop_->join();
    }
}

void VideoEncDualOutputSample::InputFuncSurface()
{
    int32_t frameCount = 0;
    while (isRunning_.load() && frameCount < 30) { // 30: max input frames
        if (nativeWindow_ == nullptr || defaultWidth <= 0 || defaultHeight <= 0) {
            break;
        }
        OHNativeWindowBuffer *buf = nullptr;
        int fenceFd = -1;
        int32_t err = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow_, &buf, &fenceFd);
        if (err != 0) {
            break;
        }
        if (fenceFd > 0) {
            close(fenceFd);
        }
        OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(buf);
        void *virAddr = nullptr;
        err = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
        if (err != 0 || virAddr == nullptr) {
            break;
        }
        uint32_t size = static_cast<uint32_t>(defaultWidth * defaultHeight * 3 / 2); // 3/2: YUV420 bpp
        (void)memset_s(virAddr, size, 0x80, size);                                   // 0x80: YUV fill
        OH_NativeBuffer_Unmap(nativeBuffer);
        Region::Rect rect = {0, 0, static_cast<uint32_t>(defaultWidth), static_cast<uint32_t>(defaultHeight)};
        Region region = {&rect, 1};
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_UI_TIMESTAMP, GetSystemTimeUs());
        OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow_, buf, -1, region);
        frameCount++;
        usleep(33000); // 33000: ~30fps interval (us)
    }
}

int64_t VideoEncDualOutputSample::GetSystemTimeUs()
{
    struct timespec ts;
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000000L + ts.tv_nsec / 1000L; // sec→us + nsec→us
}

// ======================== Do* Operations ========================

void VideoEncDualOutputSample::DoCreatePrimary(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    if (fdp.remaining_bytes() < 1) {
        return;
    }
    const char *mime = fdp.ConsumeBool() ? OH_AVCODEC_MIMETYPE_VIDEO_HEVC : OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    std::lock_guard<std::mutex> lk1(primaryMutex_);
    StopInloop();
    if (primary_ != nullptr) {
        std::lock_guard<std::mutex> lk2(secondaryMutex_);
        if (secondary_ != nullptr) {
            OH_VideoEncoder_Destroy(secondary_);
            secondary_ = nullptr;
        }
        OH_VideoEncoder_Destroy(primary_);
        primary_ = nullptr;
    }
    if (nativeWindow_ != nullptr) {
        OH_NativeWindow_DestroyNativeWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }
    OH_VideoEncoder_CreatePrimaryWithPreproc(mime, &primary_);
}

void VideoEncDualOutputSample::DoCreateSecondary(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    std::lock_guard<std::mutex> lk1(primaryMutex_);
    if (primary_ == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lk2(secondaryMutex_);
    OH_AVCodec *sec = nullptr;
    OH_VideoEncoder_CreateSecondaryFromPrimary(primary_, &sec);
    if (sec != nullptr) {
        if (secondary_ != nullptr) {
            OH_VideoEncoder_Destroy(secondary_);
        }
        secondary_ = sec;
    }
}

void VideoEncDualOutputSample::DoRegisterCallback(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    OH_VideoEncoder_RegisterCallback(codec, cb_, nullptr);
}

void VideoEncDualOutputSample::DoConfigure(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    OH_AVFormat *fmt = nullptr;
    if (fdp.ConsumeBool()) {
        fmt = BuildFuzzFormat(fdp);
    } else {
        int32_t w = fdp.ConsumeIntegralInRange<int32_t>(320, 1920); // 320~1920: valid width range
        int32_t h = fdp.ConsumeIntegralInRange<int32_t>(240, 1080); // 240~1080: valid height range
        fmt = CreateEncFormat(w, h, 5000000);                       // 5000000: 5Mbps bitrate
    }
    if (fmt != nullptr) {
        OH_AVFormat_GetIntValue(fmt, OH_MD_KEY_WIDTH, &defaultWidth);
        OH_AVFormat_GetIntValue(fmt, OH_MD_KEY_HEIGHT, &defaultHeight);
        if (defaultWidth <= 0) {
            defaultWidth = 320; // 320: fallback width
        }
        if (defaultHeight <= 0) {
            defaultHeight = 240; // 240: fallback height
        }
        OH_VideoEncoder_Configure(codec, fmt);
        OH_AVFormat_Destroy(fmt);
    }
}

void VideoEncDualOutputSample::DoGetSurface(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    if (target == 0) { // 0: primary
        StopInloop();
    }
    OHNativeWindow *window = nullptr;
    OH_AVErrCode ret = OH_VideoEncoder_GetSurface(codec, &window);
    if (ret == AV_ERR_OK && window != nullptr) {
        if (nativeWindow_ != nullptr) {
            OH_NativeWindow_DestroyNativeWindow(nativeWindow_);
        }
        nativeWindow_ = window;
        int32_t w = std::max(defaultWidth, 16);                               // 16: alignment
        int32_t h = std::max(defaultHeight, 16);                              // 16: alignment
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_FORMAT, 24); // 24: surface pixel format
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_BUFFER_GEOMETRY, w, h);
    }
}

void VideoEncDualOutputSample::DoStart(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    if (target == 0) { // 0: primary
        StopInloop();
    }
    OH_VideoEncoder_Start(codec);
    if (target == 0 && nativeWindow_ != nullptr && codec != nullptr) { // 0: primary
        StartInputLoop();
    }
}

void VideoEncDualOutputSample::DoStop(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    if (target == 0) { // 0: primary
        StopInloop();
    }
    OH_VideoEncoder_Stop(codec);
}

void VideoEncDualOutputSample::DoFlush(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    if (target == 0) { // 0: primary
        StopInloop();
    }
    OH_VideoEncoder_Flush(codec);
}

void VideoEncDualOutputSample::DoReset(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    if (target == 0) { // 0: primary
        StopInloop();
    }
    OH_VideoEncoder_Reset(codec);
}

void VideoEncDualOutputSample::DoNotifyEos(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    OH_VideoEncoder_NotifyEndOfStream(codec);
}

void VideoEncDualOutputSample::DoSetParameter(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    OH_AVFormat *fmt = BuildFuzzParamFormat(fdp);
    if (fmt != nullptr) {
        OH_VideoEncoder_SetParameter(codec, fmt);
        OH_AVFormat_Destroy(fmt);
    }
}

void VideoEncDualOutputSample::DoGetOutputDesc(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    OH_AVFormat *desc = OH_VideoEncoder_GetOutputDescription(codec);
    if (desc != nullptr) {
        OH_AVFormat_Destroy(desc);
    }
}

void VideoEncDualOutputSample::DoGetInputDesc(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    OH_AVFormat *desc = OH_VideoEncoder_GetInputDescription(codec);
    if (desc != nullptr) {
        OH_AVFormat_Destroy(desc);
    }
}

void VideoEncDualOutputSample::DoFreeOutputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    if (fdp.remaining_bytes() >= sizeof(uint32_t)) {
        OH_VideoEncoder_FreeOutputBuffer(codec, fdp.ConsumeIntegral<uint32_t>());
    }
}

void VideoEncDualOutputSample::DoDestroy(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    if (target == 0 && primary_ != nullptr) { // 0: primary
        std::lock_guard<std::mutex> lk1(primaryMutex_);
        StopInloop();
        std::lock_guard<std::mutex> lk2(secondaryMutex_);
        if (secondary_ != nullptr) {
            OH_VideoEncoder_Destroy(secondary_);
            secondary_ = nullptr;
        }
        OH_VideoEncoder_Destroy(primary_);
        primary_ = nullptr;
    } else if (target == 1 && secondary_ != nullptr) { // 1: secondary
        std::lock_guard<std::mutex> lk(secondaryMutex_);
        OH_VideoEncoder_Destroy(secondary_);
        secondary_ = nullptr;
    } else if (target == 2) { // 2: invalid target
        OH_VideoEncoder_Destroy(nullptr);
    } else {
        auto lk = LockTarget(target);
        OH_VideoEncoder_Destroy(codec);
    }
}

void VideoEncDualOutputSample::DoPushInputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    if (fdp.remaining_bytes() >= sizeof(uint32_t)) {
        OH_VideoEncoder_PushInputBuffer(codec, fdp.ConsumeIntegral<uint32_t>());
    }
}

void VideoEncDualOutputSample::DoQueryInputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    uint32_t idx = 0;
    OH_VideoEncoder_QueryInputBuffer(codec, &idx, fdp.ConsumeIntegral<int64_t>());
}

void VideoEncDualOutputSample::DoQueryOutputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    uint32_t idx = 0;
    OH_VideoEncoder_QueryOutputBuffer(codec, &idx, fdp.ConsumeIntegral<int64_t>());
}

void VideoEncDualOutputSample::DoPrepare(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    OH_VideoEncoder_Prepare(codec);
}

void VideoEncDualOutputSample::DoIsValid(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    bool isValid = false;
    OH_VideoEncoder_IsValid(codec, &isValid);
}

void VideoEncDualOutputSample::DoSetCallback(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    OH_VideoEncoder_SetCallback(codec, asyncCb_, nullptr);
}

void VideoEncDualOutputSample::DoGetOutputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    if (fdp.remaining_bytes() >= sizeof(uint32_t)) {
        OH_VideoEncoder_GetOutputBuffer(codec, fdp.ConsumeIntegral<uint32_t>());
    }
}

void VideoEncDualOutputSample::DoGetInputBuffer(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    if (fdp.remaining_bytes() >= sizeof(uint32_t)) {
        OH_VideoEncoder_GetInputBuffer(codec, fdp.ConsumeIntegral<uint32_t>());
    }
}

void VideoEncDualOutputSample::DoFreeOutputData(int32_t target, OH_AVCodec *codec, FuzzedDataProvider &fdp)
{
    auto lk = LockTarget(target);
    if (fdp.remaining_bytes() >= sizeof(uint32_t)) {
        OH_VideoEncoder_FreeOutputData(codec, fdp.ConsumeIntegral<uint32_t>());
    }
}

// ======================== Dispatch & Cleanup ========================

const VideoEncDualOutputSample::OpFunc VideoEncDualOutputSample::opDispatch[24] = {
    // 24: dispatch op count
    &VideoEncDualOutputSample::DoCreatePrimary,     // 0
    &VideoEncDualOutputSample::DoCreateSecondary,   // 1
    &VideoEncDualOutputSample::DoRegisterCallback,  // 2
    &VideoEncDualOutputSample::DoConfigure,         // 3
    &VideoEncDualOutputSample::DoGetSurface,        // 4
    &VideoEncDualOutputSample::DoStart,             // 5
    &VideoEncDualOutputSample::DoStop,              // 6
    &VideoEncDualOutputSample::DoFlush,             // 7
    &VideoEncDualOutputSample::DoReset,             // 8
    &VideoEncDualOutputSample::DoNotifyEos,         // 9
    &VideoEncDualOutputSample::DoSetParameter,      // 10
    &VideoEncDualOutputSample::DoGetOutputDesc,     // 11
    &VideoEncDualOutputSample::DoGetInputDesc,      // 12
    &VideoEncDualOutputSample::DoFreeOutputBuffer,  // 13
    &VideoEncDualOutputSample::DoDestroy,           // 14
    &VideoEncDualOutputSample::DoPushInputBuffer,   // 15
    &VideoEncDualOutputSample::DoQueryInputBuffer,  // 16
    &VideoEncDualOutputSample::DoQueryOutputBuffer, // 17
    &VideoEncDualOutputSample::DoPrepare,           // 18
    &VideoEncDualOutputSample::DoIsValid,           // 19
    &VideoEncDualOutputSample::DoSetCallback,       // 20
    &VideoEncDualOutputSample::DoGetOutputBuffer,   // 21
    &VideoEncDualOutputSample::DoGetInputBuffer,    // 22
    &VideoEncDualOutputSample::DoFreeOutputData,    // 23
};

void VideoEncDualOutputSample::ExecuteOp(FuzzedDataProvider &fdp)
{
    if (fdp.remaining_bytes() < sizeof(int32_t)) {
        return;
    }
    int32_t op = fdp.ConsumeIntegralInRange<int32_t>(0, 24 - 1);    // 24: dispatch op count
    int32_t target = fdp.ConsumeIntegralInRange<int32_t>(0, 3 - 1); // 3: target types (primary/secondary/invalid)
    OH_AVCodec *codec = GetTarget(target);
    (this->*VideoEncDualOutputSample::opDispatch[op])(target, codec, fdp);
}

void VideoEncDualOutputSample::Cleanup()
{
    StopInloop();
    if (nativeWindow_ != nullptr) {
        OH_NativeWindow_DestroyNativeWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }
    if (secondary_ != nullptr) {
        OH_VideoEncoder_Destroy(secondary_);
        secondary_ = nullptr;
    }
    if (primary_ != nullptr) {
        OH_VideoEncoder_Destroy(primary_);
        primary_ = nullptr;
    }
}

// ======================== Lifecycle Fuzz ========================

void VideoEncDualOutputSample::BootstrapEncoder(FuzzedDataProvider &fdp)
{
    DoCreatePrimary(0, nullptr, fdp); // 0: primary
    if (primary_ == nullptr) {
        return;
    }
    DoRegisterCallback(0, primary_, fdp); // 0: primary

    int32_t w = fdp.ConsumeIntegralInRange<int32_t>(320, 1920); // 320~1920: valid width range
    int32_t h = fdp.ConsumeIntegralInRange<int32_t>(240, 1080); // 240~1080: valid height range
    OH_AVFormat *fmt = CreateEncFormat(w, h, 5000000);          // 5000000: 5Mbps bitrate
    if (fmt != nullptr) {
        defaultWidth = w;
        defaultHeight = h;
        OH_VideoEncoder_Configure(primary_, fmt);
        OH_AVFormat_Destroy(fmt);
    }
    DoGetSurface(0, primary_, fdp); // 0: primary
    DoPrepare(0, primary_, fdp);    // 0: primary
    DoStart(0, primary_, fdp);      // 0: primary
}

void VideoEncDualOutputSample::RunFuzzLoop(FuzzedDataProvider &fdp)
{
    std::mutex queueMutex;
    std::condition_variable queueCv;
    std::queue<TaskItem> taskQueue;
    std::atomic<bool> producerDone{false};
    auto worker = [&queueMutex, &queueCv, &taskQueue, &producerDone, this]() {
        bool running = true;
        while (running) {
            TaskItem task;
            {
                std::unique_lock<std::mutex> lk(queueMutex);
                queueCv.wait(lk, [&taskQueue, &producerDone] {
                    return !taskQueue.empty() || producerDone.load();
                });
                if (taskQueue.empty() && producerDone.load()) {
                    running = false;
                    continue;
                }
                task = std::move(taskQueue.front());
                taskQueue.pop();
            }
            OH_AVCodec *codec = GetTarget(std::get<1>(task));
            auto &data = std::get<2>(task);
            FuzzedDataProvider threadFdp(data.data(), data.size());
            (this->*opDispatch[std::get<0>(task)])(std::get<1>(task), codec, threadFdp);
        }
    };
    std::thread workerThread(worker);
    int32_t maxOps = fdp.ConsumeIntegralInRange<int32_t>(1, 30); // 30: max fuzz ops
    for (int32_t i = 0; i < maxOps && fdp.remaining_bytes() >= sizeof(int32_t) * 2; i++) { // 2: op + target
        DispatchOneOp(fdp, queueMutex, queueCv, taskQueue);
    }
    producerDone.store(true);
    queueCv.notify_one();
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void VideoEncDualOutputSample::DispatchOneOp(FuzzedDataProvider &fdp, std::mutex &mtx, std::condition_variable &cv,
                                             std::queue<TaskItem> &queue)
{
    int32_t op = fdp.ConsumeIntegralInRange<int32_t>(0, 24 - 1);        // 24: dispatch op count
    int32_t target = fdp.ConsumeIntegralInRange<int32_t>(0, 3 - 1);     // 3: target types (primary/secondary/invalid)
    int32_t threadMode = fdp.ConsumeIntegralInRange<int32_t>(0, 2 - 1); // 2: thread mode count
    if (threadMode == 1 && fdp.remaining_bytes() > sizeof(int32_t)) {   // 1: worker thread
        size_t cap = std::min(fdp.remaining_bytes(), static_cast<size_t>(64)); // max task data bytes
        size_t taskBytes = fdp.ConsumeIntegralInRange<size_t>(sizeof(int32_t), cap);
        auto data = fdp.ConsumeBytes<uint8_t>(taskBytes);
        {
            std::lock_guard<std::mutex> lk(mtx);
            queue.push({op, target, std::move(data)});
        }
        cv.notify_one();
    } else {
        OH_AVCodec *codec = GetTarget(target);
        (this->*opDispatch[op])(target, codec, fdp);
    }
}

void VideoEncDualOutputSample::LifecycleFuzz(FuzzedDataProvider &fdp)
{
    BootstrapEncoder(fdp);
    if (primary_ == nullptr) {
        return;
    }
    RunFuzzLoop(fdp);
}
