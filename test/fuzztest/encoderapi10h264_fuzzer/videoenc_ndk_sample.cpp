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
#include <arpa/inet.h>
#include <sys/time.h>
#include <utility>
#include "iconsumer_surface.h"
#include "native_buffer_inner.h"
#include "videoenc_sample.h"
#include "native_avcapability.h"
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr uint32_t FRAME_INTERVAL = 16666;
constexpr uint8_t RGBA_SIZE = 4;
constexpr uint32_t IDR_FRAME_INTERVAL = 10;
constexpr uint32_t DOUBLE = 2;
constexpr uint32_t THREE = 3;
sptr<Surface> cs = nullptr;
sptr<Surface> ps = nullptr;
VEncNdkFuzzSample *g_encSample = nullptr;

void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}

void clearBufferqueue(std::queue<OH_AVCodecBufferAttr> &q)
{
    std::queue<OH_AVCodecBufferAttr> empty;
    swap(empty, q);
}
} // namespace

VEncNdkFuzzSample::~VEncNdkFuzzSample()
{
    if (surfInput && nativeWindow) {
        OH_NativeWindow_DestroyNativeWindow(nativeWindow);
        nativeWindow = nullptr;
    }
    Release();
}

static void VencError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "Error errorCode=" << errorCode << endl;
    g_encSample->isRunning_.store(false);
    g_encSample->signal_->inCond_.notify_all();
}

static void VencFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
}

static void VencInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    if (g_encSample->isFlushing_) {
        return;
    }
    if (g_encSample->inputCallbackFlush) {
        g_encSample->Flush();
        cout << "OH_VideoEncoder_Flush end" << endl;
        g_encSample->isRunning_.store(false);
        g_encSample->signal_->inCond_.notify_all();
        g_encSample->signal_->outCond_.notify_all();
        return;
    }
    if (g_encSample->inputCallbackStop) {
        OH_VideoEncoder_Stop(codec);
        cout << "OH_VideoEncoder_Stop end" << endl;
        g_encSample->isRunning_.store(false);
        g_encSample->signal_->inCond_.notify_all();
        g_encSample->signal_->outCond_.notify_all();
        return;
    }
    VEncSignal *signal = static_cast<VEncSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void VencOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                void *userData)
{
    if (g_encSample->isFlushing_) {
        return;
    }
    if (g_encSample->outputCallbackFlush) {
        g_encSample->Flush();
        cout << "OH_VideoEncoder_Flush end" << endl;
        g_encSample->isRunning_.store(false);
        g_encSample->signal_->inCond_.notify_all();
        g_encSample->signal_->outCond_.notify_all();
        return;
    }
    if (g_encSample->outputCallbackStop) {
        OH_VideoEncoder_Stop(codec);
        cout << "OH_VideoEncoder_Stop end" << endl;
        g_encSample->isRunning_.store(false);
        g_encSample->signal_->inCond_.notify_all();
        g_encSample->signal_->outCond_.notify_all();
        return;
    }
    OH_VideoEncoder_FreeOutputData(codec, index);
}
int64_t VEncNdkFuzzSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

int32_t VEncNdkFuzzSample::ConfigureVideoEncoder()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return AV_ERR_UNKNOWN;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DEFAULT_PIX_FMT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, defaultBitrate);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, defaultKeyFrameInterval);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_RANGE_FLAG, 1);
    if (defaultBitrateMode == CQ) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_QUALITY, defaultQuality);
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, defaultBitrateMode);
    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncNdkFuzzSample::SetVideoEncoderCallback()
{
    signal_ = new VEncSignal();
    if (signal_ == nullptr) {
        cout << "Failed to new VEncSignal" << endl;
        return AV_ERR_UNKNOWN;
    }

    cb_.onError = VencError;
    cb_.onStreamChanged = VencFormatChanged;
    cb_.onNeedInputData = VencInputDataReady;
    cb_.onNeedOutputData = VencOutputDataReady;
    return OH_VideoEncoder_SetCallback(venc_, cb_, static_cast<void *>(signal_));
}

void VEncNdkFuzzSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

int32_t VEncNdkFuzzSample::CreateSurface()
{
    int32_t ret = 0;
    ret = OH_VideoEncoder_GetSurface(venc_, &nativeWindow);
    if (ret != AV_ERR_OK) {
        cout << "OH_VideoEncoder_GetSurface fail" << endl;
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_FORMAT fail" << endl;
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_BUFFER_GEOMETRY, defaultWidth, defaultHeight);
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail" << endl;
        return ret;
    }
    return AV_ERR_OK;
}

void VEncNdkFuzzSample::GetStride()
{
    OH_AVFormat *format = OH_VideoEncoder_GetInputDescription(venc_);
    int32_t inputStride = 0;
    OH_AVFormat_GetIntValue(format, "stride", &inputStride);
    stride_ = inputStride;
    OH_AVFormat_Destroy(format);
}

int32_t VEncNdkFuzzSample::OpenFile()
{
    if (fuzzMode) {
        return AV_ERR_OK;
    }
    int32_t ret = AV_ERR_OK;
    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        return AV_ERR_UNKNOWN;
    }
    inFile_->open(inpDir, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        cout << "file open fail" << endl;
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return AV_ERR_UNKNOWN;
    }
    return ret;
}

int32_t VEncNdkFuzzSample::StartVideoEncoder()
{
    isRunning_.store(true);
    int32_t ret = 0;
    if (surfInput) {
        ret = CreateSurface();
        if (ret != AV_ERR_OK) {
            return ret;
        }
    }
    if (venc_ == nullptr) {
        cout << "codec is nullptr" << endl;
    }
    ret = OH_VideoEncoder_Start(venc_);
    GetStride();
    if (ret != AV_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->outCond_.notify_all();
        return ret;
    }
    if (OpenFile() != AV_ERR_OK) {
        return AV_ERR_UNKNOWN;
    }
    if (surfInput) {
        inputLoop_ = make_unique<thread>(&VEncNdkFuzzSample::InputFuncSurface, this);
    } else {
        inputLoop_ = make_unique<thread>(&VEncNdkFuzzSample::InputFunc, this);
    }
    if (inputLoop_ == nullptr) {
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VEncNdkFuzzSample::CreateVideoEncoder()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    char gCodecName[128] = {};
    if (memcpy_s(gCodecName, sizeof(gCodecName), tmpCodecName, strlen(tmpCodecName)) != 0) {
        cout << "memcpy failed" << endl;
    }

    venc_ = OH_VideoEncoder_CreateByName(gCodecName);
    if (!venc_) {
        cout << "create codec failed" << endl;
    }
    g_encSample = this;
    return venc_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

void VEncNdkFuzzSample::WaitForEOS()
{
    if (inputLoop_)
        inputLoop_->join();
    if (outputLoop_)
        outputLoop_->join();
    inputLoop_ = nullptr;
    outputLoop_ = nullptr;
}

uint32_t VEncNdkFuzzSample::ReturnZeroIfEOS(uint32_t expectedSize)
{
    if (inFile_->gcount() != (expectedSize)) {
        cout << "no more data" << endl;
        return 0;
    }
    return 1;
}

uint32_t VEncNdkFuzzSample::ReadOneFrameYUV420SP(uint8_t *dst)
{
    uint8_t *start = dst;
    // copy Y
    for (uint32_t i = 0; i < defaultHeight; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), defaultWidth);
        if (!ReturnZeroIfEOS(defaultWidth)) {
            return 0;
        }
        dst += stride_;
    }
    // copy UV
    for (uint32_t i = 0; i < defaultHeight / sampleRatio; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), defaultWidth);
        if (!ReturnZeroIfEOS(defaultWidth)) {
            return 0;
        }
        dst += stride_;
    }
    return dst - start;
}

void VEncNdkFuzzSample::ReadOneFrameRGBA8888(uint8_t *dst)
{
    for (uint32_t i = 0; i < defaultHeight; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), defaultWidth * RGBA_SIZE);
        dst += stride_;
    }
}

uint32_t VEncNdkFuzzSample::FlushSurf(OHNativeWindowBuffer *ohNativeWindowBuffer, OH_NativeBuffer *nativeBuffer)
{
    struct Region region;
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0;
    rect->y = 0;
    rect->w = defaultWidth;
    rect->h = defaultHeight;
    region.rects = rect;
    NativeWindowHandleOpt(nativeWindow, SET_UI_TIMESTAMP, GetSystemTimeUs());
    int32_t err = OH_NativeBuffer_Unmap(nativeBuffer);
    if (err != 0) {
        cout << "OH_NativeBuffer_Unmap failed" << endl;
        return 1;
    }
    err = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, ohNativeWindowBuffer, -1, region);
    delete rect;
    if (err != 0) {
        cout << "FlushBuffer failed" << endl;
        return 1;
    }
    return 0;
}

bool VEncNdkFuzzSample::ProcessNativeWindowBuffer(OHNativeWindowBuffer *ohNativeWindowBuffer,
    OH_NativeBuffer *nativeBuffer)
{
    if (nativeWindow == nullptr) {
        cout << "nativeWindow == nullptr" << endl;
        return false;
    }

    int fenceFd = -1;
    int32_t err = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &ohNativeWindowBuffer, &fenceFd);
    if (err != 0) {
        cout << "RequestBuffer failed, GSError=" << err << endl;
        return false;
    }
    if (fenceFd > 0) {
        close(fenceFd);
    }

    nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(ohNativeWindowBuffer);
    void *virAddr = nullptr;
    OH_NativeBuffer_Config config;
    OH_NativeBuffer_GetConfig(nativeBuffer, &config);
    err = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
    if (err != 0) {
        cout << "OH_NativeBuffer_Map failed, GSError=" << err << endl;
        isRunning_.store(false);
        return false;
    }

    uint8_t *dst = (uint8_t *)virAddr;
    if (fuzzMode) {
        if (dst == nullptr) {
            return false;
        }
        if (memcpy_s(dst, (config.stride * config.height * THREE) / DOUBLE, fuzzData, fuzzSize) != EOK) {
            cout << "Fatal: memcpy fail" << endl;
            return false;
        }
    } else {
        const SurfaceBuffer *sbuffer = SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);
        int stride = sbuffer->GetStride();
        if (dst == nullptr || stride < defaultWidth) {
            cout << "invalid va or stride=" << stride << endl;
            err = NativeWindowCancelBuffer(nativeWindow, ohNativeWindowBuffer);
            isRunning_.store(false);
            return false;
        }
        stride_ = stride;
        if (!ReadOneFrameYUV420SP(dst)) {
            return false;
        }
    }

    return true;
}

void VEncNdkFuzzSample::InputFuncSurface()
{
    bool enableInput = true;
    while (enableInput) {
        OH_NativeBuffer *nativeBuffer = nullptr;
        if (outputCallbackFlush || outputCallbackStop) {
            OH_VideoEncoder_NotifyEndOfStream(venc_);
            enableInput = false;
            break;
        }

        OHNativeWindowBuffer *ohNativeWindowBuffer = nullptr;
        if (!ProcessNativeWindowBuffer(ohNativeWindowBuffer, nativeBuffer)) {
            break;
        }

        if (fuzzMode && frameCount == defaultFuzzTime) {
            int32_t err = OH_VideoEncoder_NotifyEndOfStream(venc_);
            if (err != 0) {
                cout << "OH_VideoEncoder_NotifyEndOfStream failed" << endl;
            }
            break;
        }
        if (FlushSurf(ohNativeWindowBuffer, nativeBuffer)) {
            break;
        }

        usleep(FRAME_INTERVAL);
        frameCount++;
    }
}

void VEncNdkFuzzSample::FlushBuffer()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    std::queue<OH_AVMemory *> empty;
    swap(empty, signal_->inBufferQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->attrQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();
}

void VEncNdkFuzzSample::RepeatStartBeforeEOS()
{
    if (repeatStartFlushBeforeEos > 0) {
        repeatStartFlushBeforeEos--;
        OH_VideoEncoder_Flush(venc_);
        FlushBuffer();
        OH_VideoEncoder_Start(venc_);
    }
    
    if (repeatStartStopBeforeEos > 0) {
        repeatStartStopBeforeEos--;
        OH_VideoEncoder_Stop(venc_);
        FlushBuffer();
        OH_VideoEncoder_Start(venc_);
    }
}

bool VEncNdkFuzzSample::RandomEOS(uint32_t index)
{
    if (enableRandomEos && randomEos == frameCount) {
        OH_AVCodecBufferAttr attr;
        attr.pts = 0;
        attr.size = 0;
        attr.offset = 0;
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        OH_VideoEncoder_PushInputData(venc_, index, attr);
        cout << "random eos" << endl;
        frameCount++;
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
        return true;
    }
    return false;
}

void VEncNdkFuzzSample::AutoSwitchParam()
{
    int64_t currentBitrate = defaultBitrate;
    double currentFrameRate = defaultFrameRate;
    if (frameCount == switchParamsTimeSec * static_cast<int32_t>(defaultFrameRate)) {
        OH_AVFormat *format = OH_AVFormat_Create();
        if (needResetBitrate) {
            currentBitrate = defaultBitrate >> 1;
            cout<<"switch bitrate "<< currentBitrate;
            (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, currentBitrate);
            SetParameter(format) == AV_ERR_OK ? (0) : (errCount++);
        }
        if (needResetFrameRate) {
            currentFrameRate = defaultFrameRate * DOUBLE;
            cout<< "switch framerate" << currentFrameRate << endl;
            (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, currentFrameRate);
            SetParameter(format) == AV_ERR_OK ? (0) : (errCount++);
        }
        OH_AVFormat_Destroy(format);
    }
    if (frameCount == switchParamsTimeSec * static_cast<int32_t>(defaultFrameRate) * DOUBLE) {
        OH_AVFormat *format = OH_AVFormat_Create();
        if (needResetBitrate) {
            currentBitrate = defaultBitrate << 1;
            cout<<"switch bitrate "<< currentBitrate;
            (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, currentBitrate);
        }
        if (needResetFrameRate) {
            currentFrameRate = defaultFrameRate / DOUBLE;
            cout<< "switch framerate" << currentFrameRate << endl;
            (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, currentFrameRate);
            SetParameter(format) == AV_ERR_OK ? (0) : (errCount++);
        }
        SetParameter(format) == AV_ERR_OK ? (0) : (errCount++);
        OH_AVFormat_Destroy(format);
    }
}

void VEncNdkFuzzSample::SetEOS(uint32_t index)
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    int32_t res = OH_VideoEncoder_PushInputData(venc_, index, attr);
    cout << "OH_VideoEncoder_PushInputData    EOS   res: " << res << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
}

void VEncNdkFuzzSample::SetForceIDR()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_REQUEST_I_FRAME, 1);
    OH_VideoEncoder_SetParameter(venc_, format);
    OH_AVFormat_Destroy(format);
}

int32_t VEncNdkFuzzSample::PushData(OH_AVMemory *buffer, uint32_t index, int32_t &result)
{
    int32_t res = -2;
    OH_AVCodecBufferAttr attr;
    uint8_t *fileBuffer = OH_AVMemory_GetAddr(buffer);
    if (fileBuffer == nullptr) {
        cout << "Fatal: no memory" << endl;
        return -1;
    }
    int32_t size = OH_AVMemory_GetSize(buffer);
    if (DEFAULT_PIX_FMT == AV_PIXEL_FORMAT_RGBA) {
        if (size < defaultHeight * stride_) {
            return -1;
        }
        ReadOneFrameRGBA8888(fileBuffer);
        attr.size = stride_ * defaultHeight;
    } else {
        if (size < (defaultHeight * stride_ + (defaultHeight * stride_ / DOUBLE))) {
            return -1;
        }
        attr.size = ReadOneFrameYUV420SP(fileBuffer);
    }
    if (repeatRun && inFile_->eof()) {
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        encodeCount++;
        cout << "repeat"<< "   encodeCount:" << encodeCount << endl;
        return -1;
    }
    if (inFile_->eof()) {
        SetEOS(index);
        return 0;
    }
    attr.pts = GetSystemTimeUs();
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    if (enableForceIDR && (frameCount % IDR_FRAME_INTERVAL == 0)) {
        SetForceIDR();
    }
    result = OH_VideoEncoder_PushInputData(venc_, index, attr);
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
    return res;
}

int32_t VEncNdkFuzzSample::CheckResult(bool isRandomEosSuccess, int32_t pushResult)
{
    if (isRandomEosSuccess) {
        if (pushResult == 0) {
            errCount = errCount + 1;
            cout << "push input after eos should be failed!  pushResult:" << pushResult << endl;
        }
        return -1;
    } else if (pushResult != 0) {
        errCount = errCount + 1;
        cout << "push input data failed, error:" << pushResult << endl;
        return -1;
    }
    return 0;
}

void VEncNdkFuzzSample::InputDataNormal(bool &runningFlag, uint32_t index, OH_AVMemory *buffer)
{
    if (!inFile_->eof()) {
        bool isRandomEosSuccess = RandomEOS(index);
        if (isRandomEosSuccess) {
            runningFlag = false;
            return;
        }
        int32_t pushResult = 0;
        int32_t ret = PushData(buffer, index, pushResult);
        if (ret == 0) {
            runningFlag = false;
            return;
        } else if (ret == -1) {
            return;
        }
        if (CheckResult(isRandomEosSuccess, pushResult) == -1) {
            runningFlag = false;
            isRunning_.store(false);
            signal_->inCond_.notify_all();
            signal_->outCond_.notify_all();
            return;
        }
        frameCount++;
        if (enableAutoSwitchParam) {
            AutoSwitchParam();
        }
    }
}

void VEncNdkFuzzSample::InputDataFuzz(bool &runningFlag, uint32_t index)
{
    frameCount++;
    if (frameCount == defaultFuzzTime) {
        SetEOS(index);
        runningFlag = false;
        return;
    }
    OH_AVCodecBufferAttr attr;
    attr.size = fuzzSize;
    attr.pts = GetSystemTimeUs();
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_VideoEncoder_PushInputData(venc_, index, attr);
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
}

void VEncNdkFuzzSample::InputFunc()
{
    errCount = 0;
    bool runningFlag = true;
    while (runningFlag) {
        if (!isRunning_.load()) {
            break;
        }
        RepeatStartBeforeEOS();
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() {
            if (!isRunning_.load()) {
                return true;
            }
            return signal_->inIdxQueue_.size() > 0 && !isFlushing_.load();
        });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inIdxQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        lock.unlock();
        unique_lock<mutex> flushlock(signal_->flushMutex_);
        if (isFlushing_) {
            continue;
        }
        if (fuzzMode == false) {
            InputDataNormal(runningFlag, index, buffer);
        } else {
            InputDataFuzz(runningFlag, index);
        }
        flushlock.unlock();
        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
    }
}

int32_t VEncNdkFuzzSample::Flush()
{
    isFlushing_.store(true);
    unique_lock<mutex> flushLock(signal_->flushMutex_);
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->attrQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();
    int32_t ret = OH_VideoEncoder_Flush(venc_);
    isFlushing_.store(false);
    flushLock.unlock();
    return ret;
}

int32_t VEncNdkFuzzSample::Reset()
{
    isRunning_.store(false);
    StopInloop();
    StopOutloop();
    ReleaseInFile();
    return OH_VideoEncoder_Reset(venc_);
}

int32_t VEncNdkFuzzSample::Release()
{
    int ret = OH_VideoEncoder_Destroy(venc_);
    venc_ = nullptr;
    if (signal_ != nullptr) {
        delete signal_;
        signal_ = nullptr;
    }
    return ret;
}

void VEncNdkFuzzSample::StopOutloop()
{
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        clearIntqueue(signal_->outIdxQueue_);
        clearBufferqueue(signal_->attrQueue_);
        signal_->outCond_.notify_all();
        lock.unlock();
    }
}

int32_t VEncNdkFuzzSample::SetParameter(OH_AVFormat *format)
{
    if (venc_) {
        return OH_VideoEncoder_SetParameter(venc_, format);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VEncNdkFuzzSample::SetParameterFuzz(int32_t data, int32_t data1)
{
    if (venc_) {
        OH_AVFormat *format = OH_AVFormat_Create();
        if (format == nullptr) {
            return AV_ERR_UNKNOWN;
        }
        double frameRate = data;
        (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, frameRate);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, data1);
        int ret = OH_VideoEncoder_SetParameter(venc_, format);
        OH_AVFormat_Destroy(format);
        return ret;
    }
    return AV_ERR_UNKNOWN;
}
