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
#include "native_buffer_inner.h"
#include "iconsumer_surface.h"
#include "videoenc_inner_sample.h"
#include "meta/meta_key.h"
#include <random>
#include "avcodec_list.h"
#include "native_avcodec_base.h"

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
constexpr uint32_t MILLION = 1000000;
std::random_device rd;
constexpr uint32_t ZERO = 0;
constexpr uint32_t ONE = 1;
constexpr uint8_t RGBA_SIZE = 4;
constexpr uint8_t THREE = 3;
constexpr uint8_t TWO = 2;
constexpr uint8_t FOUR = 4;
constexpr uint8_t FILE_END = -1;
constexpr uint8_t LOOP_END = 0;
constexpr uint32_t FIVE = 5;
constexpr uint32_t SIX = 6;
VEncNdkInnerSample *enc_sample = nullptr;
int32_t g_picWidth;
int32_t g_picHeight;
int32_t g_keyWidth;
int32_t g_keyHeight;

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

VEncNdkInnerSample::VEncNdkInnerSample(std::shared_ptr<VEncInnerSignal> signal)
    : signal_(signal)
{
}

VEncInnerCallback::VEncInnerCallback(std::shared_ptr<VEncInnerSignal> signal) : innersignal_(signal) {}

void VEncInnerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (enc_sample == nullptr) {
        return;
    }
    if (errorCode == AVCS_ERR_INPUT_DATA_ERROR) {
        enc_sample->errCount += 1;
        enc_sample->isRunning_.store(false);
        enc_sample->signal_->inCond_.notify_all();
        enc_sample->signal_->outCond_.notify_all();
    }
    cout << "Error errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void VEncInnerCallback::OnOutputFormatChanged(const Format& format)
{
    cout << "Format Changed" << endl;
    format.GetIntValue(OH_MD_KEY_VIDEO_PIC_WIDTH, g_picWidth);
    format.GetIntValue(OH_MD_KEY_VIDEO_PIC_HEIGHT, g_picHeight);
    format.GetIntValue(OH_MD_KEY_WIDTH, g_keyWidth);
    format.GetIntValue(OH_MD_KEY_HEIGHT, g_keyHeight);
    cout << "format info: " << format.Stringify() << ", OH_MD_KEY_VIDEO_PIC_WIDTH: " << g_picWidth
    << ", OH_MD_KEY_VIDEO_PIC_HEIGHT: "<< g_picHeight << ", OH_MD_KEY_WIDTH: " << g_keyWidth
    << ", OH_MD_KEY_HEIGHT: " << g_keyHeight << endl;
}

void VEncInnerCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
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
    unique_lock<mutex> lock(innersignal_->outMutex_);
    innersignal_->outIdxQueue_.push(index);
    innersignal_->infoQueue_.push(info);
    innersignal_->flagQueue_.push(flag);
    innersignal_->outBufferQueue_.push(buffer);
    innersignal_->outCond_.notify_all();
}

VEncParamWithAttrCallbackTest::VEncParamWithAttrCallbackTest(
    std::shared_ptr<VEncInnerSignal> signal) : signal_(signal) {}

VEncParamWithAttrCallbackTest::~VEncParamWithAttrCallbackTest()
{
    signal_ = nullptr;
}

void VEncParamWithAttrCallbackTest::OnInputParameterWithAttrAvailable(uint32_t index,
                                                                      std::shared_ptr<Format> attribute,
                                                                      std::shared_ptr<Format> parameter)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->inMutex_);
    cout << "OnInputParameterWithAttrAvailable" <<endl;
    signal_->inIdxQueue_.push(index);
    signal_->inAttrQueue_.push(attribute);
    signal_->inFormatQueue_.push(parameter);
    signal_->inCond_.notify_all();
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
    enc_sample = this;
    return venc_ == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
}

int32_t VEncNdkInnerSample::CreateByName(const std::string &name)
{
    venc_ = VideoEncoderFactory::CreateByName(name);
    enc_sample = this;
    return venc_ == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
}

int32_t VEncNdkInnerSample::ConfigureVideoEncoderSqr()
{
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, DEFAULT_PIX_FMT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, DEFAULT_KEY_FRAME_INTERVAL);
    if (MODE_ENABLE) {
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, DEFAULT_BITRATE_MODE);
    }
    if (SETBIRATE) {
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    }
    if (QUALITY_ENABLE) {
        format.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    }
    if (B_ENABLE) {
        format.PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, DEFAULT_BFRAME);
    }
    if (GOPMODE_ENABLE) {
        format.PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE, DEFAULT_GOP_MODE);
    }
    if (MAXBITE_ENABLE) {
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITERATE);
    }
    if (FACTOR_ENABLE) {
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    }
    if (MAXBFRAMES_ENABLE) {
        format.PutIntValue(Media::Tag::VIDEO_ENCODER_MAX_B_FRAME, DEFAULT_MAX_B_FRAMES);
    }
    if (enablePTSBasedRateControl) {
        format.PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_PTS_BASED_RATECONTROL, 1);
    }
    return venc_->Configure(format);
}

int32_t VEncNdkInnerSample::Configure()
{
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, DEFAULT_PIX_FMT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    if (DEFAULT_BITRATE_MODE == CQ) {
        format.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    } else {
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    }
    if (configMain10) {
        format.PutIntValue(OH_MD_KEY_PROFILE, HEVC_PROFILE_MAIN_10);
    } else if (configMain) {
        format.PutIntValue(OH_MD_KEY_PROFILE, HEVC_PROFILE_MAIN);
    }
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, DEFAULT_BITRATE_MODE);
    if (enableRepeat) {
        format.PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, DEFAULT_FRAME_AFTER);
        if (setMaxCount) {
            format.PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, DEFAULT_MAX_COUNT);
        }
    }
    if (isDiscardFrame) {
        format.PutIntValue(Media::Tag::VIDEO_I_FRAME_INTERVAL, DEFAULT_KEY_I_FRAME_INTERVAL);
    }
    format.PutIntValue(Media::Tag::AV_CODEC_ENABLE_SYNC_MODE, enbleSyncMode);
    if (enbleBFrameMode) {
        format.PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, enbleBFrameMode);
        format.PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
            static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    }
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
    int32_t ret = 0;
    if (venc_) {
        ret = venc_->Release();
        venc_ = nullptr;
    }
    if (signal_ != nullptr) {
        signal_ = nullptr;
    }
    return ret;
}

int32_t VEncNdkInnerSample::CreateInputSurface()
{
    int32_t ret = 0;
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
    if (DEFAULT_PIX_FMT == static_cast<int32_t>(VideoPixelFormat::RGBA1010102)) {
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, NATIVEBUFFER_PIXEL_FMT_RGBA_1010102);
    } else {
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    }
    
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
    if (signal_ == nullptr) {
        signal_ = make_shared<VEncInnerSignal>();
    }
    if (signal_ == nullptr) {
        cout << "Failed to new VEncInnerSignal" << endl;
        return AVCS_ERR_UNKNOWN;
    }

    cb_ = make_shared<VEncInnerCallback>(signal_);
    return venc_->SetCallback(cb_);
}

int32_t VEncNdkInnerSample::SetCallback(std::shared_ptr<MediaCodecParameterWithAttrCallback> cb)
{
    if (venc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = venc_->SetCallback(cb);
    isSetParamCallback_ = ret == AV_ERR_OK;
    return ret;
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
        if (ret != AVCS_ERR_OK) {
            return ret;
        }
    }
    ret = venc_->Start();
    if (ret != AVCS_ERR_OK) {
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
    readMultiFilesFunc();
    if (VideoEncoder() != AVCS_ERR_OK) {
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

int32_t VEncNdkInnerSample::VideoEncoder()
{
    if (surfaceInput) {
        inputLoop_ = make_unique<thread>(&VEncNdkInnerSample::InputFuncSurface, this);
        inputParamLoop_ = isSetParamCallback_ ? make_unique<thread>(&VEncNdkInnerSample::InputParamLoopFunc,
         this):nullptr;
    } else {
        if (enbleSyncMode == 0) {
            inputLoop_ = make_unique<thread>(&VEncNdkInnerSample::InputFunc, this);
        } else {
            inputLoop_ = make_unique<thread>(&VEncNdkInnerSample::SyncInputFunc, this);
        }
    }
    if (inputLoop_ == nullptr) {
        isRunning_.store(false);
        venc_->Stop();
        ReleaseInFile();
        return AVCS_ERR_UNKNOWN;
    }
    if (enbleSyncMode == 0) {
        outputLoop_ = make_unique<thread>(&VEncNdkInnerSample::OutputFunc, this);
    } else {
        outputLoop_ = make_unique<thread>(&VEncNdkInnerSample::SyncOutputFunc, this);
    }
    if (outputLoop_ == nullptr) {
        isRunning_.store(false);
        venc_->Stop();
        ReleaseInFile();
        StopInloop();
        Release();
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

void VEncNdkInnerSample::readMultiFilesFunc()
{
    if (!readMultiFiles) {
        inFile_->open(INP_DIR, ios::in | ios::binary);
        if (!inFile_->is_open()) {
            OpenFileFail();
        }
    }
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

int32_t VEncNdkInnerSample::PushData(std::shared_ptr<AVSharedMemory> buffer,
     uint32_t index, int32_t &result)
{
    int32_t res = -2;
    uint32_t yuvSize = 0;
    if (DEFAULT_PIX_FMT == static_cast<int32_t>(VideoPixelFormat::RGBA1010102) ||
        DEFAULT_PIX_FMT == static_cast<int32_t>(VideoPixelFormat::RGBA)) {
        yuvSize = DEFAULT_WIDTH * DEFAULT_HEIGHT * RGBA_SIZE;
    } else {
        yuvSize = DEFAULT_WIDTH * DEFAULT_HEIGHT * THREE / TWO;
    }
    uint8_t *fileBuffer = buffer->GetBase();
    if (fileBuffer == nullptr) {
        return -1;
    }
    (void)inFile_->read((char *)fileBuffer, yuvSize);

    if (repeatRun && inFile_->eof()) {
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        encodeCount++;
        return -1;
    }

    if (inFile_->eof()) {
        SetEOS(index);
        return 0;
    }
    AVCodecBufferInfo info;
    if (!enablePTSBasedRateControl) {
        info.presentationTimeUs = GetSystemTimeUs();
    } else {
        info.presentationTimeUs = timeList[frameIndex];
    }
    frameIndex++;
    info.size = yuvSize;
    info.offset = 0;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
    int32_t size = buffer->GetSize();
    if (size < (int32_t)yuvSize) {
        return -1;
    }
    if (enableForceIDR && (frameCount % IDR_FRAME_INTERVAL == 0)) {
        Format format;
        format.PutIntValue(MediaDescriptionKey::MD_KEY_REQUEST_I_FRAME, 1);
        venc_->SetParameter(format);
    }
    result = venc_->QueueInputBuffer(index, info, flag);
    if (enbleSyncMode == 0) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
    return res;
}

int32_t VEncNdkInnerSample::SyncPushData(std::shared_ptr<AVBuffer> buffer, uint32_t index, int32_t &result)
{
    int32_t res = -2;
    uint32_t yuvSize = DEFAULT_WIDTH * DEFAULT_HEIGHT * 3 / 2;
    uint8_t *fileBuffer = buffer->memory_->GetAddr();
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

    int32_t size = buffer->memory_->GetCapacity();
    if (size < (int32_t)yuvSize) {
        cout << "bufferSize smaller than yuv size" << endl;
        return -1;
    }

    if (enableForceIDR && (frameCount % IDR_FRAME_INTERVAL == 0)) {
        Format format;
        format.PutIntValue(MediaDescriptionKey::MD_KEY_REQUEST_I_FRAME, 1);
        venc_->SetParameter(format);
    }
    result = venc_->QueueInputBuffer(index);
    if (enbleSyncMode == 0) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
    }

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
    if (flag & AVCODEC_BUFFER_FLAG_EOS) {
        cout << "flag == AVCODEC_BUFFER_FLAG_EOS" << endl;
        if (enbleSyncMode == 0) {
            unique_lock<mutex> inLock(signal_->inMutex_);
            isRunning_.store(false);
            signal_->inCond_.notify_all();
            signal_->outCond_.notify_all();
            inLock.unlock();
        }
        return -1;
    }

    if (flag == AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        cout << "enc AVCODEC_BUFFER_FLAG_CODEC_DATA" << endl;
    } else {
        outCount = outCount + 1;
    }
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
    int64_t tmp = GetSystemTimeUs();
    if (!enablePTSBasedRateControl) {
        tmp = GetSystemTimeUs();
    } else {
        tmp = timeList[frameIndex];
    }
    frameIndex++;
    NativeWindowHandleOpt(nativeWindow, SET_UI_TIMESTAMP, tmp);
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

uint32_t VEncNdkInnerSample::ReadOneFrameYUVP010(uint8_t *dst)
{
    uint8_t *start = dst;
    int32_t num = 2;
    // copy Y
    for (uint32_t i = 0; i < DEFAULT_HEIGHT; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), DEFAULT_WIDTH*num);
        if (!ReturnZeroIfEOS(DEFAULT_WIDTH*num))
            return 0;
        dst += stride_;
    }
    // copy UV
    for (uint32_t i = 0; i < DEFAULT_HEIGHT / SAMPLE_RATIO; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), DEFAULT_WIDTH*num);
        if (!ReturnZeroIfEOS(DEFAULT_WIDTH*num))
            return 0;
        dst += stride_;
    }
    return dst - start;
}

uint32_t VEncNdkInnerSample::ReadOneFrameFromList(uint8_t *dst, int32_t &index)
{
    int32_t ret = 0;
    if (index >= fileInfos.size()) {
        ret = venc_->NotifyEos();
        if (ret != 0) {
            cout << "OH_VideoEncoder_NotifyEndOfStream failed" << endl;
        }
        return LOOP_END;
    }
    if (!inFile_->is_open()) {
        inFile_->open(fileInfos[index].fileDir);
        if (!inFile_->is_open()) {
            return OpenFileFail();
        }
        DEFAULT_WIDTH = fileInfos[index].width;
        DEFAULT_HEIGHT = fileInfos[index].height;
        if (setFormatRbgx) {
            ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, GRAPHIC_PIXEL_FMT_RGBX_8888);
        } else if (setFormat8Bit) {
            ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
        } else if (setFormat10Bit) {
            ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_P010);
        } else {
            ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, fileInfos[index].format);
        }
        if (ret != AVCS_ERR_OK) {
            return ret;
        }
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_BUFFER_GEOMETRY, DEFAULT_WIDTH, DEFAULT_HEIGHT);
        if (ret != AVCS_ERR_OK) {
            return ret;
        }
        cout << fileInfos[index].fileDir << endl;
        cout << "set width:" << fileInfos[index].width << "height: " << fileInfos[index].height << endl;
        return FILE_END;
    }
    ret = ReadOneFrameByType(dst, fileInfos[index].format);
    if (!ret) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        index++;
        if (index >= fileInfos.size()) {
            venc_->NotifyEos();
            return LOOP_END;
        }
        return FILE_END;
    }
    return ret;
}

uint32_t VEncNdkInnerSample::ReadOneFrameByType(uint8_t *dst, std::string &fileType)
{
    if (fileType == "rgba") {
        return ReadOneFrameRGBA8888(dst);
    } else if (fileType == "nv12" || fileType == "nv21") {
        return ReadOneFrameYUV420SP(dst);
    } else {
        cout << "error fileType" << endl;
        return 0;
    }
}

uint32_t VEncNdkInnerSample::ReadOneFrameByType(uint8_t *dst, GraphicPixelFormat format)
{
    if (format == GRAPHIC_PIXEL_FMT_RGBA_8888) {
        return ReadOneFrameRGBA8888(dst);
    } else if (format == GRAPHIC_PIXEL_FMT_YCBCR_420_SP || format == GRAPHIC_PIXEL_FMT_YCRCB_420_SP) {
        return ReadOneFrameYUV420SP(dst);
    } else if (format == GRAPHIC_PIXEL_FMT_YCBCR_P010) {
        return ReadOneFrameYUVP010(dst);
    } else {
        cout << "error fileType" << endl;
        return 0;
    }
}

uint32_t VEncNdkInnerSample::ReadOneFrameRGBA8888(uint8_t *dst)
{
    uint8_t *start = dst;
    for (uint32_t i = 0; i < DEFAULT_HEIGHT; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), DEFAULT_WIDTH * RGBA_SIZE);
        if (inFile_->eof())
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
    if (enbleSyncMode == 0) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
}

void VEncNdkInnerSample::WaitForEOS()
{
    if (inputLoop_)
        inputLoop_->join();
    if (outputLoop_)
        outputLoop_->join();
    if (inputParamLoop_)
        inputParamLoop_->join();
    inputLoop_ = nullptr;
    outputLoop_ = nullptr;
    inputParamLoop_ = nullptr;
}

void VEncNdkInnerSample::InputFuncSurface()
{
    int32_t readFileIndex = 0;
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
        OHNativeWindowBuffer *ohNativeWindowBuffer = nullptr;
        OH_NativeBuffer *nativeBuffer = nullptr;
        uint8_t *dst = nullptr;
        int err = InitBuffer(ohNativeWindowBuffer, nativeBuffer, dst);
        if (err == 0) {
            break;
        } else if (err == -1) {
            continue;
        }
        if (readMultiFiles) {
            err = ReadOneFrameFromList(dst, readFileIndex);
            if (err == LOOP_END) {
                break;
            } else if (err == FILE_END) {
                OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, ohNativeWindowBuffer);
                continue;
            }
        } else if (DEFAULT_PIX_FMT == static_cast<int32_t>(VideoPixelFormat::NV12) && !ReadOneFrameYUV420SP(dst)) {
            err = venc_->NotifyEos();
            if (err != 0) {
                cout << "OH_VideoEncoder_NotifyEndOfStream failed" << endl;
            }
            break;
        } else if (DEFAULT_PIX_FMT == static_cast<int32_t>(VideoPixelFormat::RGBA1010102) &&
            !ReadOneFrameRGBA8888(dst)) {
            err = venc_->NotifyEos();
            if (err != 0) {
                cout << "OH_VideoEncoder_NotifyEndOfStream failed" << endl;
            }
            break;
        }
        if (inputFrameCount == FIVE && !isParamSet && enableParameter) {
            SetParameter();
            isParamSet = true;
        }
        inputFrameCount++;
        err = InputProcess(nativeBuffer, ohNativeWindowBuffer);
        if (err != 0) {
            break;
        }
        usleep(FRAME_INTERVAL);
        InputEnableRepeatSleep();
    }
}

int32_t VEncNdkInnerSample::InitBuffer(OHNativeWindowBuffer *&ohNativeWindowBuffer,
    OH_NativeBuffer *&nativeBuffer, uint8_t *&dst)
{
    int fenceFd = -1;
    if (nativeWindow == nullptr) {
        return 0;
    }
    int32_t err = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &ohNativeWindowBuffer, &fenceFd);
    if (err != 0) {
        cout << "RequestBuffer failed, GSError=" << err << endl;
        return -1;
    }
    if (fenceFd > 0) {
        close(fenceFd);
    }
    nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(ohNativeWindowBuffer);
    void *virAddr = nullptr;
    err = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
    if (err != 0) {
        cout << "OH_NativeBuffer_Map failed, GSError=" << err << endl;
        isRunning_.store(false);
        return 0;
    }
    dst = (uint8_t *)virAddr;
    const SurfaceBuffer *sbuffer = SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);
    int32_t stride = sbuffer->GetStride();
    if (dst == nullptr || stride < (int32_t)DEFAULT_WIDTH) {
        cout << "invalid va or stride=" << stride << endl;
        err = NativeWindowCancelBuffer(nativeWindow, ohNativeWindowBuffer);
        isRunning_.store(false);
        return 0;
    }
    stride_ = stride;
    return 1;
}

void VEncNdkInnerSample::InputEnableRepeatSleep()
{
    inCount = inCount + 1;
    int32_t inCountNum = 15;
    if (enableRepeat && inCount == inCountNum) {
        if (setMaxCount) {
            int32_t sleepTimeMaxCount = 730000;
            usleep(sleepTimeMaxCount);
        } else {
            int32_t sleepTime = 1000000;
            usleep(sleepTime);
        }
        if (enableSeekEos) {
            inFile_->clear();
            inFile_->seekg(-1, ios::beg);
        }
    }
}

void VEncNdkInnerSample::InputParamLoopFunc()
{
    if (signal_ == nullptr || venc_ == nullptr) {
        cout << "signal or venc is null" << endl;
        return;
    }
    cout<< "InputParamLoopFunc" <<endl;
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(
            lock, [this]() { return (signal_->inIdxQueue_.size() > 0) || (!isRunning_.load()); });
        if (!isRunning_.load()) {
            cout << "InputLoopFunc stop running" << endl;
            break;
        }
        int32_t index = signal_->inIdxQueue_.front();
        auto format = signal_->inFormatQueue_.front();
        auto attr = signal_->inAttrQueue_.front();
        signal_->inIdxQueue_.pop();
        signal_->inFormatQueue_.pop();
        signal_->inAttrQueue_.pop();
        if (attr != nullptr) {
            int64_t pts = 0;
            if (true != attr->GetLongValue(Media::Tag::MEDIA_TIME_STAMP, pts)) {
                return;
            }
        }
        if (IsFrameDiscard(inputFrameCount)) {
            discardFrameCount++;
            format->PutIntValue(Media::Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, 1);
        }
        int32_t ret = PushInputParameter(index);
        if (ret != AV_ERR_OK) {
            cout << "Fatal: PushInputData fail, exit" << endl;
        }
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
            if (frameCount == FIVE && !isParamSet && enableParameter) {
                SetParameter();
                isParamSet = true;
            }
            frameCount++;
        }
        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
    }
}

int32_t VEncNdkInnerSample::SetParameter()
{
    Format format;
    if (SETBIRATE_RUN) {
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE_RUN);
    }
    if (MAXBITE_ENABLE_RUN) {
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITERATE_RUN);
    }
    if (FACTOR_ENABLE_RUN) {
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR_RUN);
    }
    return venc_->SetParameter(format);
}

void VEncNdkInnerSample::SyncInputFunc()
{
    errCount = 0;
    while (isRunning_.load()) {
        uint32_t index;
        if (venc_->QueryInputBuffer(index, syncInputWaitTime) != AVCS_ERR_OK) {
            cout << "QueryInputBuffer faile" << endl;
            continue;
        }
        std::shared_ptr<AVBuffer> buffer = venc_->GetInputBuffer(index);
        if (buffer == nullptr) {
            cout << "GetInputBuffer fail" << endl;
            errCount = errCount + 1;
            continue;
        }
        if (!inFile_->eof()) {
            bool isRandomEosSuccess = RandomEOS(index);
            if (isRandomEosSuccess) {
                continue;
            }
            int32_t pushResult = 0;
            int32_t ret = SyncPushData(buffer, index, pushResult);
            if (ret == 0) {
                isRunning_.store(false);
                break;
            } else if (ret == -1) {
                continue;
            }

            if (CheckResult(isRandomEosSuccess, pushResult) == -1) {
                isRunning_.store(false);
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

void VEncNdkInnerSample::SyncOutputFunc()
{
    FILE *outFile = fopen(OUT_DIR, "wb");

    while (isRunning_.load()) {
        uint32_t index = 0;
        cout << "QueryOutputBuffer start" << endl;
        if (venc_->QueryOutputBuffer(index, syncOutputWaitTime) != AVCS_ERR_OK) {
            cout << "QueryOutputBuffer fail" << endl;
            continue;
        }
        cout << "QueryOutputBuffer succ" << endl;
        std::shared_ptr<AVBuffer> buffer = venc_->GetOutputBuffer(index);
        if (buffer == nullptr) {
            cout << "GetOutputBuffer fail" << endl;
            errCount = errCount + 1;
            continue;
        }
        AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(buffer->flag_);
        if (SyncOutputFuncEos(flag, index) != AVCS_ERR_OK) {
            isRunning_.store(false);
            break;
        }
        int size = buffer->memory_->GetSize();
        if (outFile == nullptr) {
            cout << "dump data fail" << endl;
        } else {
            fwrite(buffer->memory_->GetAddr(), 1, size, outFile);
        }

        if (venc_->ReleaseOutputBuffer(index) != AVCS_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            errCount = errCount + 1;
        }
        if (errCount > 0) {
            OutputFuncFail();
            isRunning_.store(false);
            break;
        }
    }
    (void)fclose(outFile);
}

int32_t VEncNdkInnerSample::SyncOutputFuncEos(AVCodecBufferFlag flag, uint32_t index)
{
    if (CheckFlag(flag) == -1) {
        if (queryInputBufferEOS) {
            venc_->QueryInputBuffer(index, 0);
            venc_->QueryInputBuffer(index, MILLION);
            venc_->QueryInputBuffer(index, -1);
        }
        if (queryOutputBufferEOS) {
            venc_->QueryOutputBuffer(index, 0);
            venc_->QueryOutputBuffer(index, MILLION);
            venc_->QueryOutputBuffer(index, -1);
        }
        return AVCS_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
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

void VEncNdkInnerSample::PushRandomDiscardIndex(uint32_t count, uint32_t max, uint32_t min)
{
    cout << "random farame index :";
    while (discardFrameIndex.size() < count) {
        uint32_t num = 0;
        if (max != 0) {
            num = rd() % max + min;
        }
        if (find(discardFrameIndex.begin(), discardFrameIndex.end(), num) == discardFrameIndex.end()) {
            cout << num << ",";
            discardFrameIndex.push_back(num);
        }
        cout << endl;
    }
}

bool VEncNdkInnerSample::IsFrameDiscard(uint32_t index)
{
    if (!isDiscardFrame) {
        return false;
    }
    if (discardMinIndex > -1 && discardMaxIndex >= discardMinIndex) {
        if (index >= discardMinIndex && index <= discardMaxIndex) {
            return true;
        }
    }
    if (find(discardFrameIndex.begin(), discardFrameIndex.end(), index) != discardFrameIndex.end()) {
        return true;
    }
    if (discardInterval > 0 && index % discardInterval == 0) {
        return true;
    }
    return false;
}

bool VEncNdkInnerSample::CheckOutputFrameCount()
{
    cout << "checooutpuframecount" << inputFrameCount << ", " << discardFrameCount<< ", " << outCount << endl;
    if (inputFrameCount - discardFrameCount == outCount) {
        return true;
    }
    return false;
}

int32_t VEncNdkInnerSample::PushInputParameter(uint32_t index)
{
    if (venc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return venc_->QueueInputParameter(index);
}

int32_t VEncNdkInnerSample::SetCustomBuffer(BufferRequestConfig bufferConfig)
{
    int32_t waterMarkFlag = enableWaterMark ? 1 : 0;
    auto allocator = Media::AVAllocatorFactory::CreateSurfaceAllocator(bufferConfig);
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(allocator);
    if (avbuffer == nullptr) {
        cout << "avbuffer is nullptr" << endl;
        return AVCS_ERR_INVALID_VAL;
    }
    cout << WATER_MARK_DIR << endl;
    ReadCustomDataToAVBuffer(WATER_MARK_DIR, avbuffer);
    Format format;
    format.SetMeta(avbuffer->meta_);
    format.PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_WATERMARK, waterMarkFlag);
    format.PutIntValue(Media::Tag::VIDEO_COORDINATE_X, videoCoordinateX);
    format.PutIntValue(Media::Tag::VIDEO_COORDINATE_Y, videoCoordinateY);
    format.PutIntValue(Media::Tag::VIDEO_COORDINATE_W, videoCoordinateWidth);
    format.PutIntValue(Media::Tag::VIDEO_COORDINATE_H, videoCoordinateHeight);
    *(avbuffer->meta_) = *(format.GetMeta());
    int32_t ret = venc_->SetCustomBuffer(avbuffer);
    return ret;
}

bool VEncNdkInnerSample::ReadCustomDataToAVBuffer(const std::string &fileName, std::shared_ptr<AVBuffer> buffer)
{
    std::unique_ptr<std::ifstream> inFile = std::make_unique<std::ifstream>();
    inFile->open(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!inFile->is_open()) {
        cout << "open file filed,filename:" << fileName.c_str() << endl;
    }
    sptr<SurfaceBuffer> surfaceBuffer = buffer->memory_->GetSurfaceBuffer();
    if (surfaceBuffer == nullptr) {
        cout << "in is nullptr" << endl;
        return false;
    }
    int32_t width = surfaceBuffer->GetWidth();
    int32_t height = surfaceBuffer->GetHeight();
    int32_t bufferSize = width * height * 4;
    uint8_t *in = (uint8_t *)malloc(bufferSize);
    if (in == nullptr) {
        cout << "in is nullptr" <<endl;
    }
    inFile->read(reinterpret_cast<char *>(in), bufferSize);
    int32_t dstWidthStride = surfaceBuffer->GetStride();
    uint8_t *dstAddr = (uint8_t *)surfaceBuffer->GetVirAddr();
    if (dstAddr == nullptr) {
        cout << "dst is nullptr" << endl;
    }
    const int32_t srcWidthStride = width << 2;
    uint8_t *inStream = in;
    for (uint32_t i = 0; i < height; ++i) {
        if (memcpy_s(dstAddr, dstWidthStride, inStream, srcWidthStride)) {
            cout << "memcpy_s failed" <<endl;
        };
        dstAddr += dstWidthStride;
        inStream += srcWidthStride;
    }
    inFile->close();
    if (in) {
        free(in);
        in = nullptr;
    }
    return true;
}

bool VEncNdkInnerSample::GetWaterMarkCapability(std::string codecMimeType)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    capabilityData = codecCapability->GetCapability(codecMimeType, true, AVCodecCategory::AVCODEC_HARDWARE);
    if (capabilityData->featuresMap.count(static_cast<int32_t>(AVCapabilityFeature::VIDEO_WATERMARK))) {
        std::cout << "Support watermark" << std::endl;
        return true;
    } else {
        std::cout << " Not support watermark" << std::endl;
        return false;
    }
}

int32_t VEncNdkInnerSample::LoadTimeStampData(std::string filePath, std::string &inputDir,
                                              std::string &outputDir, uint32_t &w, uint32_t &h,
                                              uint32_t &bitrateMode, uint32_t &bitRate, bool &surfaceMode)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return AV_ERR_IO;
    }
    std::string temp;
    int32_t num = 0;
    while (getline(file, temp)) {
        if (num == ZERO) {
            inputDir = temp;
            num++;
            continue;
        }
        if (num == ONE) {
            outputDir = temp;
            num++;
            continue;
        }
        if (num == TWO) {
            w = stoi(temp);
            num++;
            continue;
        }
        if (num == THREE) {
            h = stoi(temp);
            num++;
            continue;
        }
        if (num == FOUR) {
            bitrateMode = stoi(temp);
            num++;
            continue;
        }
        if (num == FIVE) {
            bitRate = stoi(temp);
            num++;
            continue;
        }
        if (num == SIX) {
            surfaceMode = stoi(temp) == 1 ? true : false;
            num++;
            continue;
        }
        timeList.push_back(stoll(temp));
        num++;
    }
    file.close();
    return AV_ERR_OK;
}