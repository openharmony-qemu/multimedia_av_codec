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
#include "native_buffer_inner.h"
#include "display_type.h"
#include "iconsumer_surface.h"
#include "videoenc_ndk_inner_sample.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace std;

namespace {
const string MIME_TYPE = "video/avc";
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr uint32_t FRAME_INTERVAL = 16666;
constexpr uint32_t MAX_PIXEL_FMT = 5;
constexpr uint32_t IDR_FRAME_INTERVAL = 10;

void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}

void clearBufferqueue(std::queue<AVCodecBufferInfo> &q)
{
    std::queue<AVCodecBufferInfo> empty;
    swap(empty, q);
}

void clearFlagqueue(std::queue<AVCodecBufferFlag> &q)
{
    std::queue<AVCodecBufferFlag> empty;
    swap(empty, q);
}
} // namespace

VEncInnerCallback::VEncInnerCallback(std::shared_ptr<VEncInnerSignal> signal) : innersignal_(signal) {}

void VEncInnerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void VEncInnerCallback::OnOutputFormatChanged(const Format& format)
{
    cout << "Format Changed" << endl;
}

void VEncInnerCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    cout << "OnInputBufferAvailable index:" << index << endl;
    if (innersignal_ == nullptr) {
        std::cout << "buffer is null 1" << endl;
        return;
    }

    unique_lock<mutex> lock(innersignal_->inMutex_);
    innersignal_->inIdxQueue_.push(index);
    innersignal_->inBufferQueue_.push(buffer);
    innersignal_->inCond_.notify_all();
}

void VEncInnerCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info,
    AVCodecBufferFlag flag, std::shared_ptr<AVSharedMemory> buffer)
{
    cout << "OnOutputBufferAvailable index:" << index << endl;
    unique_lock<mutex> lock(innersignal_->outMutex_);
    innersignal_->outIdxQueue_.push(index);
    innersignal_->infoQueue_.push(info);
    innersignal_->flagQueue_.push(flag);
    innersignal_->outBufferQueue_.push(buffer);
    cout << "**********out info size = " << info.size << endl;
    innersignal_->outCond_.notify_all();
}

VEncNdkInnerSample::~VEncNdkInnerSample()
{
    Release();
}

int64_t VEncNdkInnerSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = (int64_t)now.tv_sec * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

int32_t VEncNdkInnerSample::CreateByMime(const std::string &mime)
{
    venc_ = VideoEncoderFactory::CreateByMime(mime);
    return venc_ == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
}

int32_t VEncNdkInnerSample::CreateByName(const std::string &name)
{
    venc_ = VideoEncoderFactory::CreateByName(name);
    return venc_ == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
}

int32_t VEncNdkInnerSample::Configure()
{
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);

    return venc_->Configure(format);
}

int32_t VEncNdkInnerSample::ConfigureFuzz(int32_t data)
{
    Format format;
    double frameRate = data;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, data);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, data);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, data % MAX_PIXEL_FMT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRate);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, data);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, data);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, data);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, data);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, data);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, data);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, data);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, data);

    return venc_->Configure(format);
}

int32_t VEncNdkInnerSample::Prepare()
{
    return venc_->Prepare();
}

int32_t VEncNdkInnerSample::Start()
{
    return venc_->Start();
}

int32_t VEncNdkInnerSample::Stop()
{
    StopInloop();
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->infoQueue_);
    clearFlagqueue(signal_->flagQueue_);
    ReleaseInFile();

    return venc_->Stop();
}

int32_t VEncNdkInnerSample::Flush()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->infoQueue_);
    clearFlagqueue(signal_->flagQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();

    return venc_->Flush();
}

int32_t VEncNdkInnerSample::NotifyEos()
{
    return venc_->NotifyEos();
}

int32_t VEncNdkInnerSample::Reset()
{
    isRunning_.store(false);
    StopInloop();
    StopOutloop();
    ReleaseInFile();

    if (venc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return venc_->Reset();
}

int32_t VEncNdkInnerSample::Release()
{
    int32_t ret = venc_->Release();
    venc_ = nullptr;
    if (signal_ != nullptr) {
        signal_ = nullptr;
    }
    return ret;
}

int32_t VEncNdkInnerSample::CreateInputSurface()
{
    sptr<Surface> surface = venc_->CreateInputSurface();
    if (surface == nullptr) {
        cout << "CreateInputSurface fail" << endl;
        return AVCS_ERR_INVALID_OPERATION;
    }

    nativeWindow = CreateNativeWindowFromSurface(&surface);
    if (nativeWindow == nullptr) {
        cout << "CreateNativeWindowFromSurface failed!" << endl;
        return AVCS_ERR_INVALID_VAL;
    }

    int32_t ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (ret != AVCS_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_FORMAT fail" << endl;
        return ret;
    }

    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_BUFFER_GEOMETRY, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    if (ret != AVCS_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail" << endl;
        return ret;
    }

    return AVCS_ERR_OK;
}

int32_t VEncNdkInnerSample::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    return venc_->QueueInputBuffer(index, info, flag);
}

int32_t VEncNdkInnerSample::GetOutputFormat(Format &format)
{
    return venc_->GetOutputFormat(format);
}

int32_t VEncNdkInnerSample::ReleaseOutputBuffer(uint32_t index)
{
    return venc_->ReleaseOutputBuffer(index);
}

int32_t VEncNdkInnerSample::SetParameter(const Format &format)
{
    return venc_->SetParameter(format);
}

int32_t VEncNdkInnerSample::SetCallback()
{
    signal_ = make_shared<VEncInnerSignal>();
    if (signal_ == nullptr) {
        cout << "Failed to new VEncInnerSignal" << endl;
        return AVCS_ERR_UNKNOWN;
    }

    cb_ = make_shared<VEncInnerCallback>(signal_);
    return venc_->SetCallback(cb_);
}

int32_t VEncNdkInnerSample::GetInputFormat(Format &format)
{
    return venc_->GetInputFormat(format);
}

int32_t VEncNdkInnerSample::StartVideoEncoder()
{
    isRunning_.store(true);
    int32_t ret = 0;

    if (surfaceInput) {
        ret = CreateInputSurface();
        return ret;
    }

    ret = venc_->Start();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->outCond_.notify_all();
        return ret;
    }

    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        isRunning_.store(false);
        venc_->Stop();
        return AVCS_ERR_UNKNOWN;
    }

    inFile_->open(INP_DIR, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        OpenFileFail();
    }

    if (surfaceInput) {
        inputLoop_ = make_unique<thread>(&VEncNdkInnerSample::InputFuncSurface, this);
    } else {
        inputLoop_ = make_unique<thread>(&VEncNdkInnerSample::InputFunc, this);
    }

    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
        venc_->Stop();
        ReleaseInFile();
        return AVCS_ERR_UNKNOWN;
    }
    
    outputLoop_ = make_unique<thread>(&VEncNdkInnerSample::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        cout << "Failed to create output loop" << endl;
        isRunning_.store(false);
        venc_->Stop();
        ReleaseInFile();
        StopInloop();
        Release();
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

int32_t VEncNdkInnerSample::testApi()
{
    if (venc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_REQUEST_I_FRAME, 1);
    venc_->CreateInputSurface();
    venc_->Prepare();
    venc_->GetInputFormat(format);
    venc_->Start();
    venc_->SetParameter(format);
    venc_->NotifyEos();
    venc_->GetOutputFormat(format);
    venc_->Flush();
    venc_->Stop();
    venc_->Reset();

    return AVCS_ERR_OK;
}

int32_t VEncNdkInnerSample::PushData(std::shared_ptr<AVSharedMemory> buffer, uint32_t index, int32_t &result)
{
    int32_t res = -2;
    uint32_t yuvSize = DEFAULT_WIDTH * DEFAULT_HEIGHT * 3 / 2;
    uint8_t *fileBuffer = buffer->GetBase();
    if (fileBuffer == nullptr) {
        cout << "Fatal: no memory" << endl;
        return -1;
    }
    (void)inFile_->read((char *)fileBuffer, yuvSize);

    if (repeatRun && inFile_->eof()) {
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        encodeCount++;
        cout << "repeat" << "  encodeCount:" << encodeCount << endl;
        return -1;
    }

    if (inFile_->eof()) {
        SetEOS(index);
        return 0;
    }

    AVCodecBufferInfo info;
    info.presentationTimeUs = GetSystemTimeUs();
    info.size = yuvSize;
    info.offset = 0;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;

    int32_t size = buffer->GetSize();
    if (size < (int32_t)yuvSize) {
        cout << "bufferSize smaller than yuv size" << endl;
        return -1;
    }

    if (enableForceIDR && (frameCount % IDR_FRAME_INTERVAL == 0)) {
        Format format;
        format.PutIntValue(MediaDescriptionKey::MD_KEY_REQUEST_I_FRAME, 1);
        venc_->SetParameter(format);
    }
    result = venc_->QueueInputBuffer(index, info, flag);
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();

    return res;
}

int32_t VEncNdkInnerSample::OpenFileFail()
{
    cout << "file open fail" << endl;
    isRunning_.store(false);
    venc_->Stop();
    inFile_->close();
    inFile_.reset();
    inFile_ = nullptr;
    return AVCS_ERR_UNKNOWN;
}

int32_t VEncNdkInnerSample::CheckResult(bool isRandomEosSuccess, int32_t pushResult)
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

int32_t VEncNdkInnerSample::CheckFlag(AVCodecBufferFlag flag)
{
    if (flag == AVCODEC_BUFFER_FLAG_EOS) {
        cout << "flag == AVCODEC_BUFFER_FLAG_EOS" << endl;
        unique_lock<mutex> inLock(signal_->inMutex_);
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->outCond_.notify_all();
        inLock.unlock();
        return -1;
    }

    if (flag == AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        cout << "enc AVCODEC_BUFFER_FLAG_CODEC_DATA" << endl;
    }
    outCount = outCount + 1;
    return 0;
}

int32_t VEncNdkInnerSample::InputProcess(OH_NativeBuffer *nativeBuffer, OHNativeWindowBuffer *ohNativeWindowBuffer)
{
    int32_t ret = 0;
    struct Region region;
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0;
    rect->y = 0;
    rect->w = DEFAULT_WIDTH;
    rect->h = DEFAULT_HEIGHT;
    region.rects = rect;
    NativeWindowHandleOpt(nativeWindow, SET_UI_TIMESTAMP, GetSystemTimeUs());
    ret = OH_NativeBuffer_Unmap(nativeBuffer);
    if (ret != 0) {
        cout << "OH_NativeBuffer_Unmap failed" << endl;
        delete rect;
        return ret;
    }

    ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, ohNativeWindowBuffer, -1, region);
    delete rect;
    if (ret != 0) {
        cout << "FlushBuffer failed" << endl;
        return ret;
    }
    return ret;
}

int32_t VEncNdkInnerSample::StateEOS()
{
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [this]() { return signal_->inIdxQueue_.size() > 0; });
    uint32_t index = signal_->inIdxQueue_.front();
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
    lock.unlock();

    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 0;
    info.offset = 0;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_EOS;

    return venc_->QueueInputBuffer(index, info, flag);
}

uint32_t VEncNdkInnerSample::ReturnZeroIfEOS(uint32_t expectedSize)
{
    if (inFile_->gcount() != (int32_t)expectedSize) {
        cout << "no more data" << endl;
        return 0;
    }
    return 1;
}

uint32_t VEncNdkInnerSample::ReadOneFrameYUV420SP(uint8_t *dst)
{
    uint8_t *start = dst;
    // copy Y
    for (uint32_t i = 0; i < DEFAULT_HEIGHT; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), DEFAULT_WIDTH);
        if (!ReturnZeroIfEOS(DEFAULT_WIDTH))
            return 0;
        dst += stride_;
    }
    // copy UV
    for (uint32_t i = 0; i < DEFAULT_HEIGHT / SAMPLE_RATIO; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), DEFAULT_WIDTH);
        if (!ReturnZeroIfEOS(DEFAULT_WIDTH))
            return 0;
        dst += stride_;
    }
    return dst - start;
}

bool VEncNdkInnerSample::RandomEOS(uint32_t index)
{
    uint32_t random_eos = rand() % 25;
    if (enableRandomEos && random_eos == frameCount) {
        AVCodecBufferInfo info;
        info.presentationTimeUs = 0;
        info.size = 0;
        info.offset = 0;
        AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_EOS;

        venc_->QueueInputBuffer(index, info, flag);
        cout << "random eos" << endl;
        frameCount++;
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
        return true;
    }
    return false;
}

void VEncNdkInnerSample::RepeatStartBeforeEOS()
{
    if (REPEAT_START_FLUSH_BEFORE_EOS > 0) {
        REPEAT_START_FLUSH_BEFORE_EOS--;
        venc_->Flush();
        FlushBuffer();
        venc_->Start();
    }

    if (REPEAT_START_STOP_BEFORE_EOS > 0) {
        REPEAT_START_STOP_BEFORE_EOS--;
        venc_->Stop();
        FlushBuffer();
        venc_->Start();
    }
}

void VEncNdkInnerSample::SetEOS(uint32_t index)
{
    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 0;
    info.offset = 0;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_EOS;

    int32_t res = venc_->QueueInputBuffer(index, info, flag);
    cout << "QueueInputBuffer EOS res: " << res << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
}

void VEncNdkInnerSample::WaitForEOS()
{
    if (inputLoop_)
        inputLoop_->join();
    if (outputLoop_)
        outputLoop_->join();
    inputLoop_ = nullptr;
    outputLoop_ = nullptr;
}

void VEncNdkInnerSample::InputFuncSurface()
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
        int32_t stride = sbuffer->GetStride();
        if (dst == nullptr || stride < (int32_t)DEFAULT_WIDTH) {
            cout << "invalid va or stride=" << stride << endl;
            err = NativeWindowCancelBuffer(nativeWindow, ohNativeWindowBuffer);
            isRunning_.store(false);
            break;
        }
        stride_ = stride;
        if (!ReadOneFrameYUV420SP(dst)) {
            err = venc_->NotifyEos();
            if (err != 0) {
                cout << "OH_VideoEncoder_NotifyEndOfStream failed" << endl;
            }
            break;
        }

        err = InputProcess(nativeBuffer, ohNativeWindowBuffer);
        if (err != 0) {
            break;
        }
        usleep(FRAME_INTERVAL);
    }
}

void VEncNdkInnerSample::InputFunc()
{
    errCount = 0;
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
        RepeatStartBeforeEOS();
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
        auto buffer = signal_->inBufferQueue_.front();

        lock.unlock();
        if (!inFile_->eof()) {
            bool isRandomEosSuccess = RandomEOS(index);
            if (isRandomEosSuccess) {
                continue;
            }
            int32_t pushResult = 0;
            int32_t ret = PushData(buffer, index, pushResult);
            if (ret == 0) {
                break;
            } else if (ret == -1) {
                continue;
            }

            if (CheckResult(isRandomEosSuccess, pushResult) == -1) {
                break;
            }
            frameCount++;
        }
        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
    }
}

void VEncNdkInnerSample::OutputFunc()
{
    FILE *outFile = fopen(OUT_DIR, "wb");

    while (true) {
        if (!isRunning_.load()) {
            break;
        }

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

        std::shared_ptr<AVSharedMemory> buffer = signal_->outBufferQueue_.front();
        AVCodecBufferInfo info = signal_->infoQueue_.front();
        AVCodecBufferFlag flag = signal_->flagQueue_.front();
        uint32_t index = signal_->outIdxQueue_.front();
        
        signal_->outBufferQueue_.pop();
        signal_->outIdxQueue_.pop();
        signal_->infoQueue_.pop();
        signal_->flagQueue_.pop();
        lock.unlock();

        if (CheckFlag(flag) == -1) {
            break;
        }

        int size = info.size;
        if (outFile == nullptr) {
            cout << "dump data fail" << endl;
        } else {
            fwrite(buffer->GetBase(), 1, size, outFile);
        }

        if (venc_->ReleaseOutputBuffer(index) != AVCS_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            errCount = errCount + 1;
        }
        if (errCount > 0) {
            OutputFuncFail();
            break;
        }
    }
    (void)fclose(outFile);
}

void VEncNdkInnerSample::OutputFuncFail()
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

void VEncNdkInnerSample::FlushBuffer()
{
    std::queue<std::shared_ptr<AVSharedMemory>> empty;
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    swap(empty, signal_->inBufferQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();

    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->infoQueue_);
    clearFlagqueue(signal_->flagQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();
}

void VEncNdkInnerSample::StopInloop()
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

void VEncNdkInnerSample::StopOutloop()
{
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        clearIntqueue(signal_->outIdxQueue_);
        clearBufferqueue(signal_->infoQueue_);
        clearFlagqueue(signal_->flagQueue_);
        signal_->outCond_.notify_all();
        lock.unlock();
    }
}

void VEncNdkInnerSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}
