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
#include <arpa/inet.h>
#include <sys/time.h>
#include <utility>
#include "openssl/crypto.h"
#include "openssl/sha.h"
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
constexpr uint32_t DOUBLE = 2;
VEncFuzzSample *g_encSample = nullptr;

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

VEncFuzzSample::~VEncFuzzSample()
{
    Release();
}

static void VencError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "Error errorCode=" << errorCode << endl;
}

static void VencFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
}

static void VencInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    VEncSignal *signal = static_cast<VEncSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void VencOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                void *userData)
{
    VEncSignal *signal = static_cast<VEncSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outIdxQueue_.push(index);
    signal->attrQueue_.push(*attr);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
}
int64_t VEncFuzzSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;
    return nanoTime / NANOS_IN_MICRO;
}

int32_t VEncFuzzSample::ConfigureVideoEncoder()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return AV_ERR_UNKNOWN;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, defaultPixFmt);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, defaultBitrate);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, defaultKeyFrameInterval);
    if (defaultBitrateMode == CQ) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_QUALITY, defaultQuality);
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, defaultBitrateMode);
    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncFuzzSample::ConfigureVideoEncoderFuzz(int32_t data)
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

int32_t VEncFuzzSample::SetVideoEncoderCallback()
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

void VEncFuzzSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

void VEncFuzzSample::StopInloop()
{
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        clearIntqueue(signal_->inIdxQueue_);
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        lock.unlock();

        inputLoop_->join();
        inputLoop_ = nullptr;
    }
}

void VEncFuzzSample::GetStride()
{
    OH_AVFormat *format = OH_VideoEncoder_GetInputDescription(venc_);
    int32_t inputStride = 0;
    OH_AVFormat_GetIntValue(format, "stride", &inputStride);
    stride_ = inputStride;
    OH_AVFormat_Destroy(format);
}

int32_t VEncFuzzSample::OpenFile()
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

int32_t VEncFuzzSample::StartVideoEncoder()
{
    isRunning_.store(true);
    int32_t ret = 0;
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

    inputLoop_ = make_unique<thread>(&VEncFuzzSample::InputFunc, this);

    if (inputLoop_ == nullptr) {
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }
    outputLoop_ = make_unique<thread>(&VEncFuzzSample::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        ReleaseInFile();
        StopInloop();
        Release();
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VEncFuzzSample::CreateVideoEncoder(const char *codecName)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    g_encSample = this;
    return venc_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

void VEncFuzzSample::WaitForEOS()
{
    if (inputLoop_)
        inputLoop_->join();
    if (outputLoop_)
        outputLoop_->join();
    inputLoop_ = nullptr;
    outputLoop_ = nullptr;
}

uint32_t VEncFuzzSample::ReturnZeroIfEOS(uint32_t expectedSize)
{
    if (inFile_->gcount() != (expectedSize)) {
        cout << "no more data" << endl;
        return 0;
    }
    return 1;
}

uint32_t VEncFuzzSample::ReadOneFrameYUV420SP(uint8_t *dst)
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

void VEncFuzzSample::ReadOneFrameRGBA8888(uint8_t *dst)
{
    for (uint32_t i = 0; i < defaultHeight; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), defaultWidth * RGBA_SIZE);
        dst += stride_;
    }
}

void VEncFuzzSample::SetEOS(uint32_t index)
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

void VEncFuzzSample::SetForceIDR()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_REQUEST_I_FRAME, 1);
    OH_VideoEncoder_SetParameter(venc_, format);
    OH_AVFormat_Destroy(format);
}

int32_t VEncFuzzSample::PushData(OH_AVMemory *buffer, uint32_t index, int32_t &result)
{
    int32_t res = -2;
    OH_AVCodecBufferAttr attr;
    uint8_t *fileBuffer = OH_AVMemory_GetAddr(buffer);
    if (fileBuffer == nullptr) {
        cout << "Fatal: no memory" << endl;
        return -1;
    }
    int32_t size = OH_AVMemory_GetSize(buffer);
    if (defaultPixFmt == AV_PIXEL_FORMAT_RGBA) {
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

int32_t VEncFuzzSample::CheckResult(bool isRandomEosSuccess, int32_t pushResult)
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

void VEncFuzzSample::InputDataFuzz(bool &runningFlag, uint32_t index)
{
    frameCount++;
    if (frameCount == defaultFuzzTime) {
        SetEOS(index);
        runningFlag = false;
        return;
    }
    OH_AVCodecBufferAttr attr;
    attr.pts = GetSystemTimeUs();
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_VideoEncoder_PushInputData(venc_, index, attr);
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
}

void VEncFuzzSample::InputFunc()
{
    errCount = 0;
    bool runningFlag = true;
    while (runningFlag) {
        if (!isRunning_.load()) {
            break;
        }
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() {
            if (!isRunning_.load()) {
                return true;
            }
            return signal_->inIdxQueue_.size() > 0;
        });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inIdxQueue_.front();
        lock.unlock();
        InputDataFuzz(runningFlag, index);
        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
    }
}

int32_t VEncFuzzSample::CheckAttrFlag(OH_AVCodecBufferAttr attr)
{
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        cout << "attr.flags == AVCODEC_BUFFER_FLAGS_EOS" << endl;
        unique_lock<mutex> inLock(signal_->inMutex_);
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->outCond_.notify_all();
        inLock.unlock();
        return -1;
    }
    if (attr.flags == AVCODEC_BUFFER_FLAGS_CODEC_DATA) {
        cout << "enc AVCODEC_BUFFER_FLAGS_CODEC_DATA" << attr.pts << endl;
    }
    outCount = outCount + 1;
    return 0;
}

void VEncFuzzSample::OutputFuncFail()
{
    cout << "errCount > 0" << endl;
    unique_lock<mutex> inLock(signal_->inMutex_);
    isRunning_.store(false);
    signal_->inCond_.notify_all();
    signal_->outCond_.notify_all();
    inLock.unlock();
    (void)Stop();
    Release();
}

void VEncFuzzSample::OutputFunc()
{
    FILE *outFile = fopen(outDir, "wb");

    while (true) {
        if (!isRunning_.load()) {
            break;
        }
        OH_AVCodecBufferAttr attr;
        uint32_t index;
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() {
            if (!isRunning_.load()) {
                return true;
            }
            return signal_->outIdxQueue_.size() > 0;
        });
        if (!isRunning_.load()) {
            break;
        }
        index = signal_->outIdxQueue_.front();
        attr = signal_->attrQueue_.front();
        OH_AVMemory *buffer = signal_->outBufferQueue_.front();
        signal_->outBufferQueue_.pop();
        signal_->outIdxQueue_.pop();
        signal_->attrQueue_.pop();
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

int32_t VEncFuzzSample::Flush()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->attrQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();
    return OH_VideoEncoder_Flush(venc_);
}

int32_t VEncFuzzSample::Reset()
{
    isRunning_.store(false);
    StopInloop();
    StopOutloop();
    ReleaseInFile();
    return OH_VideoEncoder_Reset(venc_);
}

int32_t VEncFuzzSample::Release()
{
    int ret = OH_VideoEncoder_Destroy(venc_);
    venc_ = nullptr;
    if (signal_ != nullptr) {
        delete signal_;
        signal_ = nullptr;
    }
    return ret;
}

int32_t VEncFuzzSample::Stop()
{
    StopInloop();
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->attrQueue_);
    ReleaseInFile();
    return OH_VideoEncoder_Stop(venc_);
}

int32_t VEncFuzzSample::Start()
{
    return OH_VideoEncoder_Start(venc_);
}

void VEncFuzzSample::StopOutloop()
{
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        clearIntqueue(signal_->outIdxQueue_);
        clearBufferqueue(signal_->attrQueue_);
        signal_->outCond_.notify_all();
        lock.unlock();
    }
}

int32_t VEncFuzzSample::SetParameter(OH_AVFormat *format)
{
    if (venc_) {
        return OH_VideoEncoder_SetParameter(venc_, format);
    }
    return AV_ERR_UNKNOWN;
}