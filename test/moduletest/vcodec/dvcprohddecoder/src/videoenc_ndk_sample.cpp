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
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr uint32_t FRAME_INTERVAL = 16666;
constexpr uint32_t MAX_PIXEL_FMT = 5;
constexpr uint8_t RGBA_SIZE = 4;
constexpr uint32_t IDR_FRAME_INTERVAL = 10;
constexpr uint32_t TEST_FRAME_COUNT = 25;
constexpr uint32_t DOUBLE = 2;
sptr<Surface> cs = nullptr;
sptr<Surface> ps = nullptr;
VEncNdkSample *enc_sample = nullptr;

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

VEncNdkSample::~VEncNdkSample()
{
    if (surfInput_ && nativeWindow_) {
        OH_NativeWindow_DestroyNativeWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }
    Release();
}

static void VencError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "Error errorCode" << errorCode << endl;
}

static void VencFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
}

static void VencInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    if (enc_sample->isFlushing) {
        return;
    }
    if (enc_sample->inputCallbackFlush) {
        enc_sample->Flush();
        cout << "OH_VideoEncoder_Flush end" << endl;
        enc_sample->isRunning.store(false);
        enc_sample->signal->inCond.notify_all();
        enc_sample->signal->outCond.notify_all();
        return;
    }
    if (enc_sample->inputCallbackStop) {
        OH_VideoEncoder_Stop(codec);
        cout << "OH_VideoEncoder_Stop end" << endl;
        enc_sample->isRunning.store(false);
        enc_sample->signal->inCond.notify_all();
        enc_sample->signal->outCond.notify_all();
        return;
    }
    VEncSignal *signal = static_cast<VEncSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex);
    signal->inIdxQueue.push(index);
    signal->inBufferQueue.push(data);
    signal->inCond.notify_all();
}

static void VencOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                void *userData)
{
    if (enc_sample->isFlushing) {
        return;
    }
    if (enc_sample->outputCallbackFlush) {
        enc_sample->Flush();
        cout << "OH_VideoEncoder_Flush end" << endl;
        enc_sample->isRunning.store(false);
        enc_sample->signal->inCond.notify_all();
        enc_sample->signal->outCond.notify_all();
        return;
    }
    if (enc_sample->outputCallbackStop) {
        OH_VideoEncoder_Stop(codec);
        cout << "OH_VideoEncoder_Stop end" << endl;
        enc_sample->isRunning.store(false);
        enc_sample->signal->inCond.notify_all();
        enc_sample->signal->outCond.notify_all();
        return;
    }
    VEncSignal *signal = static_cast<VEncSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex);
    signal->outIdxQueue.push(index);
    signal->attrQueue.push(*attr);
    signal->outBufferQueue.push(data);
    signal->outCond.notify_all();
}
int64_t VEncNdkSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = (int64_t)now.tv_sec * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

int32_t VEncNdkSample::ConfigureVideoEncoder()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return AV_ERR_UNKNOWN;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, defaultPixFmt_);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, defaultBitrate_);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, defaultKeyFrameInterval_);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_RANGE_FLAG, 1);
    if (defaultBitrateMode_ == CQ) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_QUALITY, defaultQuality_);
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, defaultBitrateMode_);
    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncNdkSample::ConfigureVideoEncoder_Temporal(int32_t temporal_gop_size)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return AV_ERR_UNKNOWN;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, defaultPixFmt_);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, defaultBitrate_);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, defaultKeyFrameInterval_);

    if (temporalConfig_) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_SIZE, temporal_gop_size);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
            ADJACENT_REFERENCE);
    }
    if (temporalEnable_) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    }
    if (temporalJumpMode_) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, JUMP_REFERENCE);
    }
    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncNdkSample::ConfigureVideoEncoder_fuzz(int32_t data)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return AV_ERR_UNKNOWN;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, data);
    defaultWidth = data;
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, data);
    defaultHeight = data;
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, data % MAX_PIXEL_FMT);
    double frameRate = data;
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, frameRate);

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_RANGE_FLAG, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COLOR_PRIMARIES, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_TRANSFER_CHARACTERISTICS, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MATRIX_COEFFICIENTS, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, data);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_QUALITY, data);

    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncNdkSample::SetVideoEncoderCallback()
{
    signal = new VEncSignal();
    if (signal == nullptr) {
        cout << "Failed to new VEncSignal" << endl;
        return AV_ERR_UNKNOWN;
    }

    cb_.onError = VencError;
    cb_.onStreamChanged = VencFormatChanged;
    cb_.onNeedInputData = VencInputDataReady;
    cb_.onNeedOutputData = VencOutputDataReady;
    return OH_VideoEncoder_SetCallback(venc_, cb_, static_cast<void *>(signal));
}

int32_t VEncNdkSample::state_EOS()
{
    unique_lock<mutex> lock(signal->inMutex);
    signal->inCond.wait(lock, [this]() { return signal->inIdxQueue.size() > 0; });
    uint32_t index = signal->inIdxQueue.front();
    signal->inIdxQueue.pop();
    signal->inBufferQueue.pop();
    lock.unlock();
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    return OH_VideoEncoder_PushInputData(venc_, index, attr);
}
void VEncNdkSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

void VEncNdkSample::StopInloop()
{
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal->inMutex);
        clearIntqueue(signal->inIdxQueue);
        isRunning.store(false);
        signal->inCond.notify_all();
        lock.unlock();

        inputLoop_->join();
        inputLoop_ = nullptr;
    }
}

void VEncNdkSample::testApi()
{
    OH_VideoEncoder_GetSurface(venc_, &nativeWindow_);
    OH_VideoEncoder_Prepare(venc_);
    OH_VideoEncoder_GetInputDescription(venc_);
    OH_VideoEncoder_Start(venc_);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_REQUEST_I_FRAME, 1);
    OH_VideoEncoder_SetParameter(venc_, format);
    OH_VideoEncoder_NotifyEndOfStream(venc_);
    OH_VideoEncoder_GetOutputDescription(venc_);
    OH_AVFormat_Destroy(format);
    OH_VideoEncoder_Flush(venc_);
    bool isValid = false;
    OH_VideoEncoder_IsValid(venc_, &isValid);
    OH_VideoEncoder_Stop(venc_);
    OH_VideoEncoder_Reset(venc_);
}

int32_t VEncNdkSample::CreateSurface()
{
    int32_t ret = 0;
    ret = OH_VideoEncoder_GetSurface(venc_, &nativeWindow_);
    if (ret != AV_ERR_OK) {
        cout << "OH_VideoEncoder_GetSurface fail" << endl;
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_FORMAT fail" << endl;
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_BUFFER_GEOMETRY, defaultWidth, defaultHeight);
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail" << endl;
        return ret;
    }
    return AV_ERR_OK;
}

void VEncNdkSample::GetStride()
{
    OH_AVFormat *format = OH_VideoEncoder_GetInputDescription(venc_);
    int32_t inputStride = 0;
    OH_AVFormat_GetIntValue(format, "stride", &inputStride);
    stride_ = inputStride;
    OH_AVFormat_Destroy(format);
}

int32_t VEncNdkSample::OpenFile()
{
    if (fuzzMode_) {
        return AV_ERR_OK;
    }
    int32_t ret = AV_ERR_OK;
    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        isRunning.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        return AV_ERR_UNKNOWN;
    }
    inFile_->open(inputDir, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        cout << "file open fail" << endl;
        isRunning.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return AV_ERR_UNKNOWN;
    }
    return ret;
}

int32_t VEncNdkSample::StartVideoEncoder()
{
    isRunning.store(true);
    int32_t ret = 0;
    if (surfInput_) {
        ret = CreateSurface();
        if (ret != AV_ERR_OK) {
            return ret;
        }
    }
    ret = OH_VideoEncoder_Start(venc_);
    GetStride();
    if (ret != AV_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning.store(false);
        signal->inCond.notify_all();
        signal->outCond.notify_all();
        return ret;
    }
    if (OpenFile() != AV_ERR_OK) {
        return AV_ERR_UNKNOWN;
    }
    if (surfInput_) {
        inputLoop_ = make_unique<thread>(&VEncNdkSample::InputFuncSurface, this);
    } else {
        inputLoop_ = make_unique<thread>(&VEncNdkSample::InputFunc, this);
    }
    if (inputLoop_ == nullptr) {
        isRunning.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }
    outputLoop_ = make_unique<thread>(&VEncNdkSample::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        isRunning.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        ReleaseInFile();
        StopInloop();
        Release();
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VEncNdkSample::CreateVideoEncoder(const char *codecName)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    enc_sample = this;
    randomEos_ = rand() % TEST_FRAME_COUNT;
    return venc_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

void VEncNdkSample::WaitForEOS()
{
    if (inputLoop_)
        inputLoop_->join();
    if (outputLoop_)
        outputLoop_->join();
    inputLoop_ = nullptr;
    outputLoop_ = nullptr;
}

uint32_t VEncNdkSample::ReturnZeroIfEOS(uint32_t expectedSize)
{
    if (inFile_->gcount() != (expectedSize)) {
        cout << "no more data" << endl;
        return 0;
    }
    return 1;
}

uint32_t VEncNdkSample::ReadOneFrameYUV420SP(uint8_t *dst)
{
    uint8_t *start = dst;
    // copy Y
    for (uint32_t i = 0; i < defaultHeight; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), defaultWidth);
        if (!ReturnZeroIfEOS(defaultWidth))
            return 0;
        dst += stride_;
    }
    // copy UV
    for (uint32_t i = 0; i < defaultHeight / sampleRatio_; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), defaultWidth);
        if (!ReturnZeroIfEOS(defaultWidth))
            return 0;
        dst += stride_;
    }
    return dst - start;
}

void VEncNdkSample::ReadOneFrameRGBA8888(uint8_t *dst)
{
    for (uint32_t i = 0; i < defaultHeight; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), defaultWidth * RGBA_SIZE);
        dst += stride_;
    }
}

uint32_t VEncNdkSample::FlushSurf(OHNativeWindowBuffer *ohNativeWindowBuffer, OH_NativeBuffer *nativeBuffer)
{
    struct Region region;
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0;
    rect->y = 0;
    rect->w = defaultWidth;
    rect->h = defaultHeight;
    region.rects = rect;
    NativeWindowHandleOpt(nativeWindow_, SET_UI_TIMESTAMP, GetSystemTimeUs());
    int32_t err = OH_NativeBuffer_Unmap(nativeBuffer);
    if (err != 0) {
        cout << "OH_NativeBuffer_Unmap failed" << endl;
        return 1;
    }
    err = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow_, ohNativeWindowBuffer, -1, region);
    delete rect;
    if (err != 0) {
        cout << "FlushBuffer failed" << endl;
        return 1;
    }
    return 0;
}

void VEncNdkSample::InputFuncSurface()
{
    while (true) {
        if (outputCallbackFlush || outputCallbackStop) {
            OH_VideoEncoder_NotifyEndOfStream(venc_);
            break;
        }
        OHNativeWindowBuffer *ohNativeWindowBuffer;
        int fenceFd = -1;
        if (nativeWindow_ == nullptr) {
            cout << "nativeWindow_ == nullptr" << endl;
            break;
        }

        int32_t err = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow_, &ohNativeWindowBuffer, &fenceFd);
        if (err != 0) {
            cout << "RequestBuffer failed, GSError=" << err << endl;
            break;
        }
        if (fenceFd > 0) {
            close(fenceFd);
        }
        OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(ohNativeWindowBuffer);
        void *virAddr = nullptr;
        err = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
        if (err != 0) {
            cout << "OH_NativeBuffer_Map failed, GSError=" << err << endl;
            isRunning.store(false);
            break;
        }
        uint8_t *dst = (uint8_t *)virAddr;
        const SurfaceBuffer *sbuffer = SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);
        int stride = sbuffer->GetStride();
        if (dst == nullptr || stride < defaultWidth) {
            cout << "invalid va or stride=" << stride << endl;
            err = NativeWindowCancelBuffer(nativeWindow_, ohNativeWindowBuffer);
            isRunning.store(false);
            break;
        }
        stride_ = stride;
        if (!ReadOneFrameYUV420SP(dst)) {
            err = OH_VideoEncoder_NotifyEndOfStream(venc_);
            if (err != 0) {
                cout << "OH_VideoEncoder_NotifyEndOfStream failed" << endl;
            }
            break;
        }
        if (FlushSurf(ohNativeWindowBuffer, nativeBuffer))
            break;
        usleep(FRAME_INTERVAL);
    }
}

void VEncNdkSample::Flush_buffer()
{
    unique_lock<mutex> inLock(signal->inMutex);
    clearIntqueue(signal->inIdxQueue);
    std::queue<OH_AVMemory *> empty;
    swap(empty, signal->inBufferQueue);
    signal->inCond.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal->outMutex);
    clearIntqueue(signal->outIdxQueue);
    clearBufferqueue(signal->attrQueue);
    signal->outCond.notify_all();
    outLock.unlock();
}

void VEncNdkSample::RepeatStartBeforeEOS()
{
    if (repeatStartFlushBeforeEos_ > 0) {
        repeatStartFlushBeforeEos_--;
        OH_VideoEncoder_Flush(venc_);
        Flush_buffer();
        OH_VideoEncoder_Start(venc_);
    }

    if (repeatStartStopBeforeEos_ > 0) {
        repeatStartStopBeforeEos_--;
        OH_VideoEncoder_Stop(venc_);
        Flush_buffer();
        OH_VideoEncoder_Start(venc_);
    }
}

bool VEncNdkSample::RandomEOS(uint32_t index)
{
    if (enableRandomEos_ && randomEos_ == frameCount_) {
        OH_AVCodecBufferAttr attr;
        attr.pts = 0;
        attr.size = 0;
        attr.offset = 0;
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        OH_VideoEncoder_PushInputData(venc_, index, attr);
        cout << "random eos" << endl;
        frameCount_++;
        unique_lock<mutex> lock(signal->inMutex);
        signal->inIdxQueue.pop();
        signal->inBufferQueue.pop();
        return true;
    }
    return false;
}

void VEncNdkSample::AutoSwitchParam()
{
    int64_t currentBitrate = defaultBitrate_;
    double currentFrameRate = defaultFrameRate;
    if (frameCount_ == switchParamsTimeSec_ * (int32_t)defaultFrameRate) {
        OH_AVFormat *format = OH_AVFormat_Create();
        if (needResetBitrate_) {
            currentBitrate = defaultBitrate_ >> 1;
            cout<<"switch bitrate "<< currentBitrate;
            (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, currentBitrate);
            SetParameter(format) == AV_ERR_OK ? (0) : (errCount++);
        }
        if (needResetFrameRate_) {
            currentFrameRate = defaultFrameRate * DOUBLE;
            cout<< "switch framerate" << currentFrameRate << endl;
            (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, currentFrameRate);
            SetParameter(format) == AV_ERR_OK ? (0) : (errCount++);
        }
        OH_AVFormat_Destroy(format);
    }
    if (frameCount_ == switchParamsTimeSec_ * (int32_t)defaultFrameRate * DOUBLE) {
        OH_AVFormat *format = OH_AVFormat_Create();
        if (needResetBitrate_) {
            currentBitrate = defaultBitrate_ << 1;
            cout<<"switch bitrate "<< currentBitrate;
            (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, currentBitrate);
        }
        if (needResetFrameRate_) {
            currentFrameRate = defaultFrameRate / DOUBLE;
            cout<< "switch framerate" << currentFrameRate << endl;
            (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, currentFrameRate);
            SetParameter(format) == AV_ERR_OK ? (0) : (errCount++);
        }
        SetParameter(format) == AV_ERR_OK ? (0) : (errCount++);
        OH_AVFormat_Destroy(format);
    }
}

void VEncNdkSample::SetEOS(uint32_t index)
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    int32_t res = OH_VideoEncoder_PushInputData(venc_, index, attr);
    cout << "OH_VideoEncoder_PushInputData    EOS   res: " << res << endl;
    unique_lock<mutex> lock(signal->inMutex);
    signal->inIdxQueue.pop();
    signal->inBufferQueue.pop();
}

void VEncNdkSample::SetForceIDR()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_REQUEST_I_FRAME, 1);
    OH_VideoEncoder_SetParameter(venc_, format);
    OH_AVFormat_Destroy(format);
}

int32_t VEncNdkSample::PushData(OH_AVMemory *buffer, uint32_t index, int32_t &result)
{
    int32_t res = -2;
    OH_AVCodecBufferAttr attr;
    uint8_t *fileBuffer = OH_AVMemory_GetAddr(buffer);
    if (fileBuffer == nullptr) {
        cout << "Fatal: no memory" << endl;
        return -1;
    }
    int32_t size = OH_AVMemory_GetSize(buffer);
    if (defaultPixFmt_ == AV_PIXEL_FORMAT_RGBA) {
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
    if (repeatRun_ && inFile_->eof()) {
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        encodeCount_++;
        cout << "repeat"<< "   encodeCount_:" << encodeCount_ << endl;
        return -1;
    }
    if (inFile_->eof()) {
        SetEOS(index);
        return 0;
    }
    attr.pts = GetSystemTimeUs();
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    if (enableForceIDR_ && (frameCount_ % IDR_FRAME_INTERVAL == 0)) {
        SetForceIDR();
    }
    result = OH_VideoEncoder_PushInputData(venc_, index, attr);
    unique_lock<mutex> lock(signal->inMutex);
    signal->inIdxQueue.pop();
    signal->inBufferQueue.pop();
    return res;
}

int32_t VEncNdkSample::CheckResult(bool isRandomEosSuccess, int32_t pushResult)
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

void VEncNdkSample::InputDataNormal(bool &runningFlag, uint32_t index, OH_AVMemory *buffer)
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
            isRunning.store(false);
            signal->inCond.notify_all();
            signal->outCond.notify_all();
            return;
        }
        frameCount_++;
        if (enableAutoSwitchParam_) {
            AutoSwitchParam();
        }
    }
}

void VEncNdkSample::InputDataFuzz(bool &runningFlag, uint32_t index)
{
    frameCount_++;
    if (frameCount_ == defaultFuzzTime_) {
        SetEOS(index);
        runningFlag = false;
        return;
    }
    OH_AVCodecBufferAttr attr;
    attr.pts = GetSystemTimeUs();
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_VideoEncoder_PushInputData(venc_, index, attr);
    unique_lock<mutex> lock(signal->inMutex);
    signal->inIdxQueue.pop();
    signal->inBufferQueue.pop();
}

void VEncNdkSample::InputFunc()
{
    errCount = 0;
    bool runningFlag = true;
    while (runningFlag) {
        if (!isRunning.load()) {
            break;
        }
        RepeatStartBeforeEOS();
        unique_lock<mutex> lock(signal->inMutex);
        signal->inCond.wait(lock, [this]() {
            if (!isRunning.load()) {
                return true;
            }
            return signal->inIdxQueue.size() > 0 && !isFlushing.load();
        });
        if (!isRunning.load()) {
            break;
        }
        uint32_t index = signal->inIdxQueue.front();
        auto buffer = signal->inBufferQueue.front();
        lock.unlock();
        unique_lock<mutex> flushlock(signal->flushMutex);
        if (isFlushing) {
            continue;
        }
        if (fuzzMode_ == false) {
            InputDataNormal(runningFlag, index, buffer);
        } else {
            InputDataFuzz(runningFlag, index);
        }
        flushlock.unlock();
        if (sleepOnFPS_) {
            usleep(FRAME_INTERVAL);
        }
    }
}

int32_t VEncNdkSample::CheckAttrFlag(OH_AVCodecBufferAttr attr)
{
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        cout << "attr.flags == AVCODEC_BUFFER_FLAGS_EOS" << endl;
        unique_lock<mutex> inLock(signal->inMutex);
        isRunning.store(false);
        signal->inCond.notify_all();
        signal->outCond.notify_all();
        inLock.unlock();
        return -1;
    }
    if (attr.flags == AVCODEC_BUFFER_FLAGS_CODEC_DATA) {
        cout << "enc AVCODEC_BUFFER_FLAGS_CODEC_DATA" << attr.pts << endl;
    }
    outCount_ = outCount_ + 1;
    return 0;
}

void VEncNdkSample::OutputFuncFail()
{
    cout << "errCount > 0" << endl;
    unique_lock<mutex> inLock(signal->inMutex);
    isRunning.store(false);
    signal->inCond.notify_all();
    signal->outCond.notify_all();
    inLock.unlock();
    (void)Stop();
    Release();
}

void VEncNdkSample::OutputFunc()
{
    FILE *outFile = fopen(outputDir, "wb");

    while (true) {
        if (!isRunning.load()) {
            break;
        }
        OH_AVCodecBufferAttr attr;
        uint32_t index;
        unique_lock<mutex> lock(signal->outMutex);
        signal->outCond.wait(lock, [this]() {
            if (!isRunning.load()) {
                return true;
            }
            return signal->outIdxQueue.size() > 0 && !isFlushing.load();
        });
        if (!isRunning.load()) {
            break;
        }
        index = signal->outIdxQueue.front();
        attr = signal->attrQueue.front();
        OH_AVMemory *buffer = signal->outBufferQueue.front();
        signal->outBufferQueue.pop();
        signal->outIdxQueue.pop();
        signal->attrQueue.pop();
        lock.unlock();
        if (CheckAttrFlag(attr) == -1) {
            break;
        }
        int size = attr.size;
        if (outFile == nullptr) {
            cout << "dump data fail" << endl;
        } else {
            fwrite(OH_AVMemory_GetAddr(buffer), 1, size, outFile);
        }

        if (OH_VideoEncoder_FreeOutputData(venc_, index) != AV_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            errCount = errCount + 1;
        }
        if (errCount > 0) {
            OutputFuncFail();
            break;
        }
    }
    if (outFile) {
        (void)fclose(outFile);
    }
}

int32_t VEncNdkSample::Flush()
{
    isFlushing.store(true);
    unique_lock<mutex> flushLock(signal->flushMutex);
    unique_lock<mutex> inLock(signal->inMutex);
    clearIntqueue(signal->inIdxQueue);
    signal->inCond.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal->outMutex);
    clearIntqueue(signal->outIdxQueue);
    clearBufferqueue(signal->attrQueue);
    signal->outCond.notify_all();
    outLock.unlock();
    int32_t ret = OH_VideoEncoder_Flush(venc_);
    isFlushing.store(false);
    flushLock.unlock();
    return ret;
}

int32_t VEncNdkSample::Reset()
{
    isRunning.store(false);
    StopInloop();
    StopOutloop();
    ReleaseInFile();
    return OH_VideoEncoder_Reset(venc_);
}

int32_t VEncNdkSample::Release()
{
    int ret = OH_VideoEncoder_Destroy(venc_);
    venc_ = nullptr;
    if (signal != nullptr) {
        delete signal;
        signal = nullptr;
    }
    return ret;
}

int32_t VEncNdkSample::Stop()
{
    StopInloop();
    clearIntqueue(signal->outIdxQueue);
    clearBufferqueue(signal->attrQueue);
    ReleaseInFile();
    return OH_VideoEncoder_Stop(venc_);
}

int32_t VEncNdkSample::Start()
{
    return OH_VideoEncoder_Start(venc_);
}

void VEncNdkSample::StopOutloop()
{
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal->outMutex);
        clearIntqueue(signal->outIdxQueue);
        clearBufferqueue(signal->attrQueue);
        signal->outCond.notify_all();
        lock.unlock();
    }
}

int32_t VEncNdkSample::SetParameter(OH_AVFormat *format)
{
    if (venc_) {
        return OH_VideoEncoder_SetParameter(venc_, format);
    }
    return AV_ERR_UNKNOWN;
}