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
#include "consumer_surface.h"
#include "openssl/crypto.h"
#include "openssl/sha.h"
#include "native_buffer_inner.h"
#include "display_type.h"
#include "videoenc_ndk_sample.h"
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
const string MIME_TYPE = "video/avc";
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr uint32_t FRAME_INTERVAL = 16666;
constexpr uint32_t MAX_PIXEL_FMT = 5;
constexpr uint32_t IDR_FRAME_INTERVAL = 10;
sptr<Surface> cs = nullptr;
sptr<Surface> ps = nullptr;
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

void VencError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "Error errorCode=" << errorCode << endl;
}

void VencFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
}

void VencInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    VEncSignal *signal = static_cast<VEncSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

void VencOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                         void *userData)
{
    VEncSignal *signal = static_cast<VEncSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outIdxQueue_.push(index);
    signal->attrQueue_.push(*attr);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
}
} // namespace

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs(cs)
    {
        outFile_ = std::make_unique<std::ofstream>();
        outFile_->open(name.data(), std::ios::out | std::ios::binary);
    };
    ~TestConsumerListener()
    {
        if (outFile_ != nullptr) {
            outFile_->close();
        }
    }
    void OnBufferAvailable() override
    {
        sptr<SurfaceBuffer> buffer;
        int32_t flushFence;
        cs->AcquireBuffer(buffer, flushFence, timestamp, damage);

        (void)outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
        cs->ReleaseBuffer(buffer, -1);
    }

private:
    int64_t timestamp = 0;
    Rect damage = {};
    sptr<Surface> cs{nullptr};
    std::unique_ptr<std::ofstream> outFile_;
};
VEncNdkSample::~VEncNdkSample()
{
    Release();
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
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE);
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
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, data);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, data % MAX_PIXEL_FMT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, data);

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_RANGE_FLAG, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COLOR_PRIMARIES, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_TRANSFER_CHARACTERISTICS, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MATRIX_COEFFICIENTS, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, data);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_QUALITY, data);

    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncNdkSample::SetVideoEncoderCallback()
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

int32_t VEncNdkSample::state_EOS()
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
    return OH_VideoEncoder_PushInputData(venc_, 1, attr);
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
        unique_lock<mutex> lock(signal_->inMutex_);
        clearIntqueue(signal_->inIdxQueue_);
        signal_->inCond_.notify_all();
        lock.unlock();

        inputLoop_->join();
        inputLoop_ = nullptr;
    }
}

void VEncNdkSample::testApi()
{
    OH_VideoEncoder_GetSurface(venc_, &nativeWindow);
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

int32_t VEncNdkSample::StartVideoEncoder()
{
    isRunning_.store(true);
    if (SURFACE_INPUT) {
        int32_t ret = OH_VideoEncoder_GetSurface(venc_, &nativeWindow);
        if (ret != AV_ERR_OK) {
            cout << "OH_VideoEncoder_GetSurface fail" << endl;
            return ret;
        }
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
        if (ret != AV_ERR_OK) {
            cout << "NativeWindowHandleOpt SET_FORMAT fail" << endl;
            return ret;
        }
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_BUFFER_GEOMETRY, DEFAULT_WIDTH, DEFAULT_HEIGHT);
        if (ret != AV_ERR_OK) {
            cout << "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail" << endl;
            return ret;
        }
    }
    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        return AV_ERR_UNKNOWN;
    }
    inFile_->open(INP_DIR, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        cout << "file open fail" << endl;
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return AV_ERR_UNKNOWN;
    }
    if (SURFACE_INPUT) {
        inputLoop_ = make_unique<thread>(&VEncNdkSample::InputFuncSurface, this);
    } else {
        inputLoop_ = make_unique<thread>(&VEncNdkSample::InputFunc, this);
    }
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }

    outputLoop_ = make_unique<thread>(&VEncNdkSample::OutputFunc, this);

    if (outputLoop_ == nullptr) {
        cout << "Failed to create output loop" << endl;
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        ReleaseInFile();
        StopInloop();
        Release();
        return AV_ERR_UNKNOWN;
    }

    int ret = OH_VideoEncoder_Start(venc_);
    if (ret != AV_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->outCond_.notify_all();
        return ret;
    }
    return AV_ERR_OK;
}

int32_t VEncNdkSample::CreateVideoEncoder(const char *codecName)
{
    if (codecName) {
        venc_ = OH_VideoEncoder_CreateByName(codecName);
    } else {
        venc_ = OH_VideoEncoder_CreateByMime(MIME_TYPE.c_str());
    }
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
}

uint32_t VEncNdkSample::ReadOneFrameYUV420SP(uint8_t *dst)
{
    uint8_t *start = dst;
    // copy Y
    for (uint32_t i = 0; i < DEFAULT_HEIGHT; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), DEFAULT_WIDTH);
        RETURN_ZERO_IF_EOS(DEFAULT_WIDTH);
        dst += stride_;
    }
    // copy UV
    for (uint32_t i = 0; i < DEFAULT_HEIGHT / SAMPLE_RATIO; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), DEFAULT_WIDTH);
        RETURN_ZERO_IF_EOS(DEFAULT_WIDTH);
        dst += stride_;
    }
    return dst - start;
}

void VEncNdkSample::InputFuncSurface()
{
    while (true) {
        OHNativeWindowBuffer *ohNativeWindowBuffer;
        int fenceFd = -1;
        if (nativeWindow == nullptr) {
            cout << "nativeWindow == nullptr" << endl;
            break;
        }

        int32_t err = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &ohNativeWindowBuffer, &fenceFd);
        if (err != 0) {
            cout << "RequestBuffer failed, GSError=" << err << endl;
            continue;
        }
        if (fenceFd > 0) {
            close(fenceFd);
        }
        OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(ohNativeWindowBuffer);
        void *virAddr = nullptr;
        err = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
        if (err != 0) {
            cout << "OH_NativeBuffer_Map failed, GSError=" << err << endl;
            isRunning_.store(false);
            break;
        }
        uint8_t *dst = (uint8_t *)virAddr;
        const SurfaceBuffer *sbuffer = SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);
        int stride = sbuffer->GetStride();
        if (dst == nullptr || stride < DEFAULT_WIDTH) {
            cout << "invalid va or stride=" << stride << endl;
            err = NativeWindowCancelBuffer(nativeWindow, ohNativeWindowBuffer);
            isRunning_.store(false);
            break;
        }
        stride_ = stride;
        if (!ReadOneFrameYUV420SP(dst)) {
            err = OH_VideoEncoder_NotifyEndOfStream(venc_);
            if (err != 0) {
                cout << "OH_VideoEncoder_NotifyEndOfStream failed" << endl;
                break;
            }
            break;
        }
        struct Region region;
        struct Region::Rect *rect = new Region::Rect();
        rect->x = 0;
        rect->y = 0;
        rect->w = DEFAULT_WIDTH;
        rect->h = DEFAULT_HEIGHT;
        region.rects = rect;
        NativeWindowHandleOpt(nativeWindow, SET_UI_TIMESTAMP, GetSystemTimeUs());
        err = OH_NativeBuffer_Unmap(nativeBuffer);
        if (err != 0) {
            cout << "OH_NativeBuffer_Unmap failed" << endl;
            break;
        }
        err = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, ohNativeWindowBuffer, -1, region);
        delete rect;
        if (err != 0) {
            cout << "FlushBuffer failed" << endl;
            break;
        }
        usleep(FRAME_INTERVAL);
    }
}

void VEncNdkSample::InputFunc()
{
    errCount = 0;
    uint32_t yuvSize = DEFAULT_WIDTH * DEFAULT_HEIGHT * 3 / 2;
    auto timestamp = chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now().time_since_epoch()).count();
    srand(timestamp);
    uint32_t random_eos = rand() % 100;
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
        if (REPEAT_START_FLUSH_BEFORE_EOS > 0) {
            REPEAT_START_FLUSH_BEFORE_EOS--;
            OH_VideoEncoder_Flush(venc_);
            OH_VideoEncoder_Start(venc_);
        }
        if (REPEAT_START_STOP_BEFORE_EOS > 0) {
            REPEAT_START_STOP_BEFORE_EOS--;
            OH_VideoEncoder_Stop(venc_);
            OH_VideoEncoder_Start(venc_);
        }
        uint32_t index;
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
        index = signal_->inIdxQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
        lock.unlock();
        OH_AVCodecBufferAttr attr;
        if (!inFile_->eof()) {
            uint8_t *fileBuffer = OH_AVMemory_GetAddr(buffer);
            if (fileBuffer == nullptr) {
                cout << "Fatal: no memory" << endl;
                continue;
            }
            if (enable_random_eos && random_eos == frameCount) {
                attr.pts = 0;
                attr.size = 0;
                attr.offset = 0;
                attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
                OH_VideoEncoder_PushInputData(venc_, index, attr);
                cout << "random eos" << endl;
                frameCount++;
                continue;
            }

            (void)inFile_->read((char *)fileBuffer, yuvSize);
            if (repeatRun && inFile_->eof()) {
                inFile_->clear();
                inFile_->seekg(0, ios::beg);
                encode_count++;
                cout << "repeat"
                     << "   encode_count:" << encode_count << endl;
                continue;
            } else if (repeat_time > 0) {
                if (inFile_->eof() && frameCount < repeat_time) {
                    inFile_->clear();
                    inFile_->seekg(0, ios::beg);
                    continue;
                }
            }
            if (inFile_->eof()) {
                attr.pts = 0;
                attr.size = 0;
                attr.offset = 0;
                attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
                int32_t result = OH_VideoEncoder_PushInputData(venc_, index, attr);
                if (result == 0) {
                    cout << "OH_VideoEncoder_PushInputData    EOS" << endl;
                } else {
                    cout << "OH_VideoEncoder_PushInputData    EOS    fail" << endl;
                }
                break;
            }
            int64_t startPts = GetSystemTimeUs();
            attr.pts = startPts;
            attr.size = yuvSize;
            attr.offset = 0;
            attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
            int32_t size = OH_AVMemory_GetSize(buffer);
            if (size < yuvSize) {
                cout << "bufferSize is " << endl;
                continue;
            }
            if (enableForceIDR && (frameCount % IDR_FRAME_INTERVAL == 0)) {
                OH_AVFormat *format = OH_AVFormat_Create();
                if (OH_AVFormat_SetIntValue(format, OH_MD_KEY_REQUEST_I_FRAME, 1) == false) {
                    cout << "set avformat forceIDR false" << endl;
                    break;
                }
                OH_VideoEncoder_SetParameter(venc_, format);
                OH_AVFormat_Destroy(format);
            }
            int32_t result = OH_VideoEncoder_PushInputData(venc_, index, attr);
            if (showLog) {
                cout << "OH_VideoEncoder_PushInputData, code = " << result << "  index=" << index
                     << "  flags=" << attr.flags << " yuvSize=" << yuvSize << "   startPts=" << startPts << endl;
            }

            if (result != 0) {
                errCount = errCount + 1;
                break;
            }
            frameCount++;
        }

        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
    }
}

void VEncNdkSample::OutputFunc()
{
    FILE *outFile = fopen(OUT_DIR, "wb");

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
        signal_->outIdxQueue_.pop();
        signal_->attrQueue_.pop();
        lock.unlock();
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "attr.flags == AVCODEC_BUFFER_FLAGS_EOS" << endl;
            int64_t firstTime = 0;
            int64_t aveTime = 0;
            int64_t sumTime = 0;
            int64_t s_count = 0;
            for (int i = 0; i < outCount; i++) {
                if (firstTime == 0) {
                    firstTime = outTimeArray[i];
                }
                if (outTimeArray[i] != 0) {
                    sumTime = sumTime + outTimeArray[i];
                    s_count = s_count + 1;
                }
            }
            aveTime = sumTime / s_count;
            if (end_time == 0) {
                end_time = GetSystemTimeUs();
            }
            double fps = outCount / ((end_time - start_time) / 1000000.00);
            cout << "enc finish " << INP_DIR << "  firstTime:" << firstTime << "   aveTime:" << aveTime
                 << "  fps:" << fps << endl;
            unique_lock<mutex> inLock(signal_->inMutex_);
            signal_->outBufferQueue_.pop();
            isRunning_.store(false);
            signal_->inCond_.notify_all();
            signal_->outCond_.notify_all();
            inLock.unlock();
            break;
        }
        if (attr.flags == AVCODEC_BUFFER_FLAGS_CODEC_DATA) {
            if (showLog)
                cout << "enc AVCODEC_BUFFER_FLAGS_CODEC_DATA" << attr.pts << endl;
        } else {
            int64_t decTs = GetSystemTimeUs() - attr.pts;
            if (showLog) {
                cout << "enc " << INP_DIR << " time:" << decTs << "  attr.flags:" << attr.flags
                     << "   startPts:" << attr.pts << endl;
            }
            outCount = outCount + 1;
        }
        int size = attr.size;
        OH_AVMemory *buffer = signal_->outBufferQueue_.front();
        signal_->outBufferQueue_.pop();
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
            cout << "errCount > 0" << endl;
            unique_lock<mutex> inLock(signal_->inMutex_);
            isRunning_.store(false);
            signal_->inCond_.notify_all();
            signal_->outCond_.notify_all();
            inLock.unlock();
            (void)Stop();
            Release();
            break;
        }
    }
    int ret = fclose(outFile);
}

int32_t VEncNdkSample::Flush()
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

int32_t VEncNdkSample::Reset()
{
    isRunning_.store(false);
    StopInloop();
    StopOutloop();
    ReleaseInFile();
    return OH_VideoEncoder_Reset(venc_);
}

int32_t VEncNdkSample::Release()
{
    if (signal_ != nullptr) {
        delete signal_;
        signal_ = nullptr;
    }
    int ret = OH_VideoEncoder_Destroy(venc_);
    venc_ = nullptr;
    return ret;
}

int32_t VEncNdkSample::Stop()
{
    StopInloop();
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->attrQueue_);
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
        unique_lock<mutex> lock(signal_->outMutex_);
        clearIntqueue(signal_->outIdxQueue_);
        clearBufferqueue(signal_->attrQueue_);
        signal_->outCond_.notify_all();
        lock.unlock();
    }
}