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
#include "native_buffer_inner.h"
#include "iconsumer_surface.h"
#include "videoenc_inner_sample.h"
#include "meta/meta_key.h"
#include <random>
#include "avcodec_list.h"
#include "native_avcodec_base.h"
#include <string>

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
std::random_device g_rd;
std::random_device g_eos;
constexpr uint32_t DOUBLE = 2;
constexpr uint32_t THREE = 3;
constexpr uint8_t RGBA_SIZE = 4;
constexpr uint8_t FILE_END = -1;
constexpr uint8_t LOOP_END = 0;
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

VEncNdkInnerFuzzSample::VEncNdkInnerFuzzSample(std::shared_ptr<VEncInnerSignal> signal)
    : signal_(signal)
{
}

VEncInnerCallback::VEncInnerCallback(std::shared_ptr<VEncInnerSignal> signal) : innersignal_(signal) {}

void VEncInnerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
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

VEncNdkInnerFuzzSample::~VEncNdkInnerFuzzSample()
{
    Release();
}

int64_t VEncNdkInnerFuzzSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

int32_t VEncNdkInnerFuzzSample::CreateByMime(const std::string &mime)
{
    venc_ = VideoEncoderFactory::CreateByMime(mime);
    return venc_ == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
}

int32_t VEncNdkInnerFuzzSample::CreateByName(const std::string &name)
{
    venc_ = VideoEncoderFactory::CreateByName(name);
    return venc_ == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
}

int32_t VEncNdkInnerFuzzSample::Configure()
{
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, defaultWidth);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, defaultHeight);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, defaultFrameRate);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, defaultBitrate);
    if (configMain10) {
        format.PutIntValue(OH_MD_KEY_PROFILE, HEVC_PROFILE_MAIN_10);
    } else if (configMain) {
        format.PutIntValue(OH_MD_KEY_PROFILE, HEVC_PROFILE_MAIN);
    }
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, DEFAULT_BITRATE_MODE);
    if (enableRepeat) {
        format.PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, defaultFrameAfter);
        if (setMaxCount) {
            format.PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, defaultMaxCount);
        }
    }
    if (isDiscardFrame) {
        format.PutIntValue(Media::Tag::VIDEO_I_FRAME_INTERVAL, defaultKeyIFrameInterval);
    }
    return venc_->Configure(format);
}

int32_t VEncNdkInnerFuzzSample::ConfigureFuzz(int32_t data)
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

int32_t VEncNdkInnerFuzzSample::Prepare()
{
    return venc_->Prepare();
}

int32_t VEncNdkInnerFuzzSample::Start()
{
    return venc_->Start();
}

int32_t VEncNdkInnerFuzzSample::Stop()
{
    StopInloop();
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->infoQueue_);
    clearFlagqueue(signal_->flagQueue_);
    ReleaseInFile();

    return venc_->Stop();
}

int32_t VEncNdkInnerFuzzSample::Flush()
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

int32_t VEncNdkInnerFuzzSample::NotifyEos()
{
    return venc_->NotifyEos();
}

int32_t VEncNdkInnerFuzzSample::Reset()
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

int32_t VEncNdkInnerFuzzSample::Release()
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

int32_t VEncNdkInnerFuzzSample::CreateInputSurface()
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

    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_BUFFER_GEOMETRY, defaultWidth, defaultHeight);
    if (ret != AVCS_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail" << endl;
        return ret;
    }
    return AVCS_ERR_OK;
}

int32_t VEncNdkInnerFuzzSample::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    return venc_->QueueInputBuffer(index, info, flag);
}

int32_t VEncNdkInnerFuzzSample::GetOutputFormat(Format &format)
{
    return venc_->GetOutputFormat(format);
}

int32_t VEncNdkInnerFuzzSample::ReleaseOutputBuffer(uint32_t index)
{
    return venc_->ReleaseOutputBuffer(index);
}

int32_t VEncNdkInnerFuzzSample::SetParameter(const Format &format)
{
    return venc_->SetParameter(format);
}

int32_t VEncNdkInnerFuzzSample::SetCallback()
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

int32_t VEncNdkInnerFuzzSample::SetCallback(std::shared_ptr<MediaCodecParameterWithAttrCallback> cb)
{
    if (venc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = venc_->SetCallback(cb);
    isSetParamCallback_ = ret == AV_ERR_OK;
    return ret;
}


int32_t VEncNdkInnerFuzzSample::GetInputFormat(Format &format)
{
    return venc_->GetInputFormat(format);
}


int32_t VEncNdkInnerFuzzSample::StartEncoder()
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
    ret = ReadMultiFilesFunc();
    if (ret == AVCS_ERR_UNKNOWN) {
        isRunning_.store(false);
        venc_->Stop();
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

int32_t VEncNdkInnerFuzzSample::StartVideoEncoder()
{
    int32_t ret = StartEncoder();
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    if (surfaceInput) {
        inputLoop_ = make_unique<thread>(&VEncNdkInnerFuzzSample::InputFuncSurface, this);
        inputParamLoop_ = isSetParamCallback_ ? make_unique<thread>(&VEncNdkInnerFuzzSample::InputParamLoopFunc,
         this):nullptr;
    } else {
        inputLoop_ = make_unique<thread>(&VEncNdkInnerFuzzSample::InputFunc, this);
    }

    if (inputLoop_ == nullptr) {
        isRunning_.store(false);
        venc_->Stop();
        ReleaseInFile();
        return AVCS_ERR_UNKNOWN;
    }
    outputLoop_ = make_unique<thread>(&VEncNdkInnerFuzzSample::OutputFunc, this);
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

int32_t VEncNdkInnerFuzzSample::ReadMultiFilesFunc()
{
    if (!readMultiFiles) {
        inFile_->open(inpDir, ios::in | ios::binary);
        if (!inFile_->is_open()) {
            return OpenFileFail();
        }
    }
    return AVCS_ERR_OK;
}

int32_t VEncNdkInnerFuzzSample::TestApi()
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

int32_t VEncNdkInnerFuzzSample::PushData(std::shared_ptr<AVSharedMemory> buffer, uint32_t index, int32_t &result)
{
    int32_t res = -2;
    uint32_t yuvSize = (defaultWidth * defaultHeight * THREE) / DOUBLE;
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
    if (size < static_cast<int32_t>(yuvSize)) {
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

int32_t VEncNdkInnerFuzzSample::OpenFileFail()
{
    cout << "file open fail" << endl;
    isRunning_.store(false);
    venc_->Stop();
    inFile_->close();
    inFile_.reset();
    inFile_ = nullptr;
    return AVCS_ERR_UNKNOWN;
}

int32_t VEncNdkInnerFuzzSample::CheckResult(bool isRandomEosSuccess, int32_t pushResult)
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

int32_t VEncNdkInnerFuzzSample::CheckFlag(AVCodecBufferFlag flag)
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
    } else {
        outCount = outCount + 1;
    }
    return 0;
}

int32_t VEncNdkInnerFuzzSample::InputProcess(OH_NativeBuffer *nativeBuffer, OHNativeWindowBuffer *ohNativeWindowBuffer)
{
    int32_t ret = 0;
    struct Region region;
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0;
    rect->y = 0;
    rect->w = defaultWidth;
    rect->h = defaultHeight;
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

int32_t VEncNdkInnerFuzzSample::StateEOS()
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

uint32_t VEncNdkInnerFuzzSample::ReturnZeroIfEOS(uint32_t expectedSize)
{
    if (inFile_->gcount() != static_cast<int32_t>(expectedSize)) {
        cout << "no more data" << endl;
        return 0;
    }
    return 1;
}

uint32_t VEncNdkInnerFuzzSample::ReadOneFrameYUV420SP(uint8_t *dst)
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

uint32_t VEncNdkInnerFuzzSample::ReadOneFrameYUVP010(uint8_t *dst)
{
    uint8_t *start = dst;
    int32_t num = 2;
    // copy Y
    for (uint32_t i = 0; i < defaultHeight; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), defaultWidth * num);
        if (!ReturnZeroIfEOS(defaultWidth * num)) {
            return 0;
        }
        dst += stride_;
    }
    // copy UV
    for (uint32_t i = 0; i < defaultHeight / sampleRatio; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), defaultWidth * num);
        if (!ReturnZeroIfEOS(defaultWidth * num)) {
            return 0;
        }
        dst += stride_;
    }
    return dst - start;
}

uint32_t VEncNdkInnerFuzzSample::ReadOneFrameFromList(uint8_t *dst, int32_t &index)
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
        defaultWidth = fileInfos[index].width;
        defaultHeight = fileInfos[index].height;
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
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_BUFFER_GEOMETRY, defaultWidth, defaultHeight);
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

uint32_t VEncNdkInnerFuzzSample::ReadOneFrameByType(uint8_t *dst, std::string &fileType)
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

uint32_t VEncNdkInnerFuzzSample::ReadOneFrameByType(uint8_t *dst, GraphicPixelFormat format)
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

uint32_t VEncNdkInnerFuzzSample::ReadOneFrameRGBA8888(uint8_t *dst)
{
    uint8_t *start = dst;
    for (uint32_t i = 0; i < defaultHeight; i++) {
        inFile_->read(reinterpret_cast<char *>(dst), defaultWidth * RGBA_SIZE);
        if (inFile_->eof()) {
            return 0;
        }
        dst += stride_;
    }
    return dst - start;
}

bool VEncNdkInnerFuzzSample::RandomEOS(uint32_t index)
{
    uint32_t randomEos = g_eos() % 25;
    if (enableRandomEos && randomEos == frameCount) {
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

void VEncNdkInnerFuzzSample::RepeatStartBeforeEOS()
{
    if (repeatStartFlushBeforeEos > 0) {
        repeatStartFlushBeforeEos--;
        venc_->Flush();
        FlushBuffer();
        venc_->Start();
    }

    if (repeatStartStopBeforeEos > 0) {
        repeatStartStopBeforeEos--;
        venc_->Stop();
        FlushBuffer();
        venc_->Start();
    }
}

void VEncNdkInnerFuzzSample::SetEOS(uint32_t index)
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

void VEncNdkInnerFuzzSample::WaitForEOS()
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

void VEncNdkInnerFuzzSample::InputFuncSurface()
{
    int32_t readFileIndex = 0;
    bool enableInputFunc = true;
    while (enableInputFunc) {
        OHNativeWindowBuffer *ohNativeWindowBuffer = nullptr;
        OH_NativeBuffer *nativeBuffer = nullptr;
        uint8_t *dst = nullptr;
        int err = InitBuffer(ohNativeWindowBuffer, nativeBuffer, dst);
        if (err == 0) {
            enableInputFunc = false;
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
        } else if (!ReadOneFrameYUV420SP(dst)) {
            err = venc_->NotifyEos();
            if (err != 0) {
                cout << "OH_VideoEncoder_NotifyEndOfStream failed" << endl;
            }
            break;
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

int32_t VEncNdkInnerFuzzSample::InitBuffer(OHNativeWindowBuffer *&ohNativeWindowBuffer,
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
    if (dst == nullptr || stride < static_cast<int32_t>(defaultWidth)) {
        cout << "invalid va or stride=" << stride << endl;
        err = NativeWindowCancelBuffer(nativeWindow, ohNativeWindowBuffer);
        isRunning_.store(false);
        return 0;
    }
    stride_ = stride;
    return 1;
}

void VEncNdkInnerFuzzSample::InputEnableRepeatSleep()
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

void VEncNdkInnerFuzzSample::InputParamLoopFunc()
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

void VEncNdkInnerFuzzSample::InputFunc()
{
    errCount = 0;
    bool enableInput = true;
    while (enableInput) {
        if (!isRunning_.load()) {
            enableInput = false;
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

void VEncNdkInnerFuzzSample::OutputFunc()
{
    bool enableOutput = true;
    while (enableOutput) {
        if (!isRunning_.load()) {
            enableOutput = false;
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

        if (venc_->ReleaseOutputBuffer(index) != AVCS_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            errCount = errCount + 1;
        }
        if (errCount > 0) {
            OutputFuncFail();
            break;
        }
    }
}

void VEncNdkInnerFuzzSample::OutputFuncFail()
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

void VEncNdkInnerFuzzSample::FlushBuffer()
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

void VEncNdkInnerFuzzSample::StopInloop()
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

void VEncNdkInnerFuzzSample::StopOutloop()
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

void VEncNdkInnerFuzzSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

void VEncNdkInnerFuzzSample::PushRandomDiscardIndex(uint32_t count, uint32_t max, uint32_t min)
{
    cout << "random farame index :";
    while (discardFrameIndex.size() < count) {
        uint32_t num = 0;
        if (max != 0) {
            num = g_rd() % max + min;
        }
        if (find(discardFrameIndex.begin(), discardFrameIndex.end(), num) == discardFrameIndex.end()) {
            cout << num << ",";
            discardFrameIndex.push_back(num);
        }
        cout << endl;
    }
}

bool VEncNdkInnerFuzzSample::IsFrameDiscard(uint32_t index)
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

bool VEncNdkInnerFuzzSample::CheckOutputFrameCount()
{
    cout << "checooutpuframecount" << inputFrameCount << ", " << discardFrameCount<< ", " << outCount << endl;
    if (inputFrameCount - discardFrameCount == outCount) {
        return true;
    }
    return false;
}

int32_t VEncNdkInnerFuzzSample::PushInputParameter(uint32_t index)
{
    if (venc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return venc_->QueueInputParameter(index);
}

int32_t VEncNdkInnerFuzzSample::SetCustomBuffer(BufferRequestConfig bufferConfig, uint8_t *data, size_t size)
{
    int32_t waterMarkFlag = enableWaterMark ? 1 : 0;
    auto allocator = Media::AVAllocatorFactory::CreateSurfaceAllocator(bufferConfig);
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(allocator);
    if (avbuffer == nullptr) {
        cout << "avbuffer is nullptr" << endl;
        return AVCS_ERR_INVALID_VAL;
    }
    ReadCustomDataToAVBuffer(data, avbuffer, size);
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

bool VEncNdkInnerFuzzSample::ReadCustomDataToAVBuffer(uint8_t *data, std::shared_ptr<AVBuffer> buffer, size_t size)
{
    sptr<SurfaceBuffer> surfaceBuffer = buffer->memory_->GetSurfaceBuffer();
    if (surfaceBuffer == nullptr) {
        cout << "in is nullptr" << endl;
        return false;
    }
    uint8_t *dstAddr = (uint8_t *)surfaceBuffer->GetVirAddr();
    int32_t dstSize = static_cast<int32_t>(surfaceBuffer->GetSize());
    if (dstAddr == nullptr) {
        cout << "dst is nullptr" << endl;
    }
    uint8_t *inStream = data;
    if (dstSize >= size) {
        if (memcpy_s(dstAddr, dstSize, inStream, size)) {
            cout << "memcpy_s failed" <<endl;
        }
    } else {
        if (memcpy_s(dstAddr, dstSize, inStream, dstSize)) {
            cout << "memcpy_s failed" <<endl;
        }
    }
    return true;
}

bool VEncNdkInnerFuzzSample::GetWaterMarkCapability(std::string codecMimeType)
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
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    (void)codecInfo->GetSupportedMaxBitrate();
    (void)codecInfo->GetSupportedSqrFactor();
}