/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "hdrcodec_inner_sample.h"
#include <arpa/inet.h>
#include <sys/time.h>
#include <utility>
#include <memory>
#include "native_avcodec_base.h"
#include "surface/window.h"
#include "native_window.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace std;
namespace {
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
std::shared_ptr<std::ifstream> inFile_;
std::condition_variable g_cv;
std::atomic<bool> g_isRunning = true;
HDRCodecInnderNdkSample* hdr_sample = nullptr;

int64_t GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}
static void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}
}

HdrDecInnerCallback::HdrDecInnerCallback(std::shared_ptr<InnerSignal> signal) : decInnersignal_(signal) {}
HdrEncInnerCallback::HdrEncInnerCallback(std::shared_ptr<InnerSignal> signal) : encInnersignal_(signal) {}

HDRCodecInnderNdkSample::~HDRCodecInnderNdkSample()
{
    Release();
}

void HdrDecInnerCallback::OnOutputFormatChanged(const Format& format)
{
    cout << "Format Changed" << endl;
}

void HdrDecInnerCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    if (decInnersignal_ == nullptr) {
        cout << "buffer is null 1" << endl;
    }
    unique_lock<mutex> lock(decInnersignal_->inMutex_);
    decInnersignal_->inIdxQueue_.push(index);
    decInnersignal_->inBufferQueue_.push(buffer);
    decInnersignal_->inCond_.notify_all();
}

void HdrDecInnerCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
    std::shared_ptr<AVSharedMemory> buffer)
{
    hdr_sample->vdec_->ReleaseOutputBuffer(index, true);
    if (flag & AVCODEC_BUFFER_FLAG_EOS) {
        hdr_sample->venc_->NotifyEos();
    } else {
        hdr_sample->frameCountDec++;
    }
}

void HdrDecInnerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    hdr_sample->errorCount++;
    cout << "Dec Error errorCode:" << errorCode << endl;
}

void HdrEncInnerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    hdr_sample->errorCount++;
    cout << "Enc Error errorCode:" << errorCode << endl;
}

void HdrEncInnerCallback::OnOutputFormatChanged(const Format& format)
{
    cout << "Format Changed" << endl;
}

void HdrEncInnerCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    (void)index;
    (void)buffer;
}

void HdrEncInnerCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info,
    AVCodecBufferFlag flag, std::shared_ptr<AVSharedMemory> buffer)
{
    hdr_sample->venc_->ReleaseOutputBuffer(index);
    if (flag & AVCODEC_BUFFER_FLAG_EOS) {
        g_isRunning.store(false);
        g_cv.notify_all();
    } else {
        hdr_sample->frameCountEnc++;
    }
}

int32_t HDRCodecInnderNdkSample::CreateCodec()
{
    if (signal_ == nullptr) {
        signal_ = make_shared<InnerSignal>();
    }
    if (signal_ == nullptr) {
        return AVCS_ERR_UNKNOWN;
    }
    vdec_ = VideoDecoderFactory::CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    if (vdec_ == nullptr) {
        return AVCS_ERR_UNKNOWN;
    }

    venc_ = VideoEncoderFactory::CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    if (venc_ == nullptr) {
        return AVCS_ERR_UNKNOWN;
    }
    hdr_sample = this;
    return AVCS_ERR_OK;
}

void HDRCodecInnderNdkSample::FlushBuffer()
{
    unique_lock<mutex> decInLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    std::queue<std::shared_ptr<AVSharedMemory>> empty;
    swap(empty, signal_->inBufferQueue_);
    signal_->inCond_.notify_all();
    inFile_->clear();
    inFile_->seekg(0, ios::beg);
    decInLock.unlock();
}

int32_t HDRCodecInnderNdkSample::RepeatCall()
{
    if (REPEAT_START_FLUSH_BEFORE_EOS > 0) {
        return RepeatCallStartFlush();
    }
    if (REPEAT_START_STOP_BEFORE_EOS > 0) {
        return RepeatCallStartStop();
    }
    if (REPEAT_START_FLUSH_STOP_BEFORE_EOS > 0) {
        return RepeatCallStartFlushStop();
    }
    return 0;
}

void HDRCodecInnderNdkSample::InputFunc()
{
    bool flag = true;
    while (flag) {
        if (!g_isRunning.load()) {
            flag = false;
            break;
        }
        int32_t ret = RepeatCall();
        if (ret != 0) {
            cout << "repeat call failed, errcode " << ret << endl;
            errorCount++;
            g_isRunning.store(false);
            g_cv.notify_all();
            flag = false;
            break;
        }
        uint32_t index;
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() {
            if (!g_isRunning.load()) {
                return true;
            }
            return signal_->inIdxQueue_.size() > 0;
        });
        if (!g_isRunning.load()) {
            flag = false;
            break;
        }
        index = signal_->inIdxQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();

        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
        lock.unlock();
        if (SendData(vdec_, index, buffer) == 1)
            break;
    }
}

int32_t HDRCodecInnderNdkSample::Configure()
{
    Format format;
    (void)format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    (void)format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    int ret = vdec_->Configure(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    (void)format.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, DEFAULT_PROFILE);
    ret = venc_->Configure(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    sptr<Surface> surface = venc_->CreateInputSurface();
    if (surface == nullptr) {
        cout << "CreateInputSurface is fail" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    window = CreateNativeWindowFromSurface(&surface);
    if (window == nullptr) {
        cout << "CreateNativeWindowFromSurface is fail" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    ret = vdec_->SetOutputSurface(window->surface);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    ret = EncSetCallback();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    return DecSetCallback();
}

int32_t HDRCodecInnderNdkSample::DecSetCallback()
{
    if (signal_ == nullptr) {
        cout << "Failed to new HdrInnerSignal" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    decCb_ = make_shared<HdrDecInnerCallback>(signal_);
    return vdec_->SetCallback(decCb_);
}

int32_t HDRCodecInnderNdkSample::EncSetCallback()
{
    if (signal_ == nullptr) {
        cout << "Failed to new HdrInnerSignal" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    encCb_ = make_shared<HdrEncInnerCallback>(signal_);
    return venc_->SetCallback(encCb_);
}

int32_t HDRCodecInnderNdkSample::ReConfigure()
{
    int32_t ret = vdec_->Reset();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = venc_->Reset();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    FlushBuffer();
    Format format;

    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    ret = vdec_->Configure(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, DEFAULT_PROFILE);
    ret = venc_->Configure(format);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = vdec_->SetOutputSurface(window->surface);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    return ret;
}

int32_t HDRCodecInnderNdkSample::Start()
{
    int32_t ret = 0;
    inFile_ = make_unique<ifstream>();
    inFile_->open(INP_DIR, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        (void)vdec_->Release();
        (void)venc_->Release();
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return AVCS_ERR_UNKNOWN;
    }
    g_isRunning.store(true);
    ret = venc_->Start();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = vdec_->Start();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    inputLoop_ = make_unique<thread>(&HDRCodecInnderNdkSample::InputFunc, this);
    if (inputLoop_ == nullptr) {
        g_isRunning.store(false);
        (void)vdec_->Stop();
        ReleaseInFile();
        return AVCS_ERR_UNKNOWN;
    }

    return 0;
}

void HDRCodecInnderNdkSample::StopInloop()
{
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        clearIntqueue(signal_->inIdxQueue_);
        g_isRunning.store(false);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
        inputLoop_.reset();
    }
}

void HDRCodecInnderNdkSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

void HDRCodecInnderNdkSample::WaitForEos()
{
    std::mutex mtx;
    unique_lock<mutex> lock(mtx);
    g_cv.wait(lock, []() {
        return !(g_isRunning.load());
    });
    inputLoop_->join();
    vdec_->Stop();
    venc_->Stop();
}

int32_t HDRCodecInnderNdkSample::SendData(std::shared_ptr<AVCodecVideoDecoder> codec,
    uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    uint32_t bufferSize = 0;
    int32_t result = 0;
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    static bool isFirstFrame = true;
    (void)inFile_->read(reinterpret_cast<char *>(&bufferSize), sizeof(uint32_t));
    if (inFile_->eof()) {
        flag = AVCODEC_BUFFER_FLAG_EOS;
        info.offset = 0;
        codec->QueueInputBuffer(index, info, flag);
        return 1;
    }
    if (isFirstFrame) {
        flag = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        isFirstFrame = false;
    } else {
        flag = AVCODEC_BUFFER_FLAG_NONE;
    }
    int32_t size = buffer->GetSize();
    uint8_t *avBuffer = buffer->GetBase();
    if (avBuffer == nullptr) {
        return 0;
    }
    uint8_t *fileBuffer = new uint8_t[bufferSize];
    if (fileBuffer == nullptr) {
        cout << "Fatal: no memory" << endl;
        delete[] fileBuffer;
        return 0;
    }
    (void)inFile_->read(reinterpret_cast<char *>(fileBuffer), bufferSize);
    if (memcpy_s(avBuffer, size, fileBuffer, bufferSize) != EOK) {
        delete[] fileBuffer;
        cout << "Fatal: memcpy fail" << endl;
        return 0;
    }
    delete[] fileBuffer;
    info.presentationTimeUs = GetSystemTimeUs();
    info.size = bufferSize;
    info.offset = 0;
    result = codec->QueueInputBuffer(index, info, flag);
    if (result != AVCS_ERR_OK) {
        cout << "push input data failed,error:" << result << endl;
    }
    return 0;
}

int32_t HDRCodecInnderNdkSample::RepeatCallStartFlush()
{
    int32_t ret = 0;
    REPEAT_START_FLUSH_BEFORE_EOS--;
    ret = venc_->Flush();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = vdec_->Flush();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    FlushBuffer();
    ret = venc_->Start();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = vdec_->Start();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    return 0;
}

int32_t HDRCodecInnderNdkSample::RepeatCallStartStop()
{
    int32_t ret = 0;
    REPEAT_START_STOP_BEFORE_EOS--;
    ret = vdec_->Stop();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = venc_->Stop();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    FlushBuffer();
    ret = venc_->Start();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = vdec_->Start();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    return 0;
}

int32_t HDRCodecInnderNdkSample::RepeatCallStartFlushStop()
{
    int32_t ret = 0;
    REPEAT_START_FLUSH_STOP_BEFORE_EOS--;
    ret = venc_->Flush();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = vdec_->Flush();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = vdec_->Stop();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = venc_->Stop();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    FlushBuffer();
    ret = venc_->Start();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    ret = vdec_->Start();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    return 0;
}

void HDRCodecInnderNdkSample::Release()
{
    if (vdec_ != nullptr) {
        vdec_->Release();
        vdec_ = nullptr;
    }
    if (venc_ != nullptr) {
        venc_->Release();
        venc_ = nullptr;
    }
    if (signal_ != nullptr) {
        signal_ = nullptr;
    }
    if (hdr_sample != nullptr) {
        hdr_sample = nullptr;
    }
    if (window != nullptr) {
        OH_NativeWindow_DestroyNativeWindow(window);
        window = nullptr;
    }
}