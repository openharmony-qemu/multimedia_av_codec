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
#include "iconsumer_surface.h"
#include "native_buffer_inner.h"
#include "syncvideoenc_sample.h"
#include "native_avcapability.h"
#include <random>
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
constexpr uint32_t THREE = 3;
constexpr uint32_t BADPOC = 1000;
constexpr uint32_t LTR_INTERVAL = 5;
constexpr uint32_t MILLION = 1000000;
constexpr uint32_t HUNDRED = 100;
constexpr int32_t ONE = 1;
sptr<Surface> cs = nullptr;
sptr<Surface> ps = nullptr;
OH_AVCapability *cap = nullptr;
VEncSyncSample *g_encSample = nullptr;
int32_t g_picWidth;
int32_t g_picHeight;
int32_t g_keyWidth;
int32_t g_keyHeight;
std::random_device g_rd;
constexpr int32_t TIMESTAMP_BASE = 1000000;
constexpr int32_t DURATION_BASE = 46000;


void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}
} // namespace

VEncSyncSample::~VEncSyncSample()
{
    if (surfInput && nativeWindow) {
        OH_NativeWindow_DestroyNativeWindow(nativeWindow);
        nativeWindow = nullptr;
    }
    abnormalIndexValue = false;
    Release();
}

static void VencError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    if (g_encSample == nullptr) {
        return;
    }
    if (errorCode == AV_ERR_INPUT_DATA_ERROR) {
        g_encSample->errCount += 1;
        g_encSample->isRunning_.store(false);
        g_encSample->signal_->inCond_.notify_all();
        g_encSample->signal_->outCond_.notify_all();
    }
    cout << "Error errorCode=" << errorCode << endl;
}

static void VencFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_PIC_WIDTH, &g_picWidth);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_PIC_HEIGHT, &g_picHeight);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_WIDTH, &g_keyWidth);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_HEIGHT, &g_keyHeight);
    cout << "format info: " << OH_AVFormat_DumpInfo(format) << ", OH_MD_KEY_VIDEO_PIC_WIDTH: " << g_picWidth
    << ", OH_MD_KEY_VIDEO_PIC_HEIGHT: "<< g_picHeight << ", OH_MD_KEY_WIDTH: " << g_keyWidth
    << ", OH_MD_KEY_HEIGHT: " << g_keyHeight << endl;
}

static void onEncInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    VEncAPI11Signal *signal = static_cast<VEncAPI11Signal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(buffer);
    signal->inCond_.notify_all();
}

static void onEncOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    VEncAPI11Signal *signal = static_cast<VEncAPI11Signal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outIdxQueue_.push(index);
    signal->outBufferQueue_.push(buffer);
    signal->outCond_.notify_all();
}

static void onEncInputParam(OH_AVCodec *codec, uint32_t index, OH_AVFormat *parameter, void *userData)
{
    static bool useLtrOnce = false;
    if (!parameter || !userData) {
        return;
    }
    if (g_encSample->frameCount % g_encSample->ltrParam.ltrInterval == 0) {
        OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_MARK_LTR, 1);
    }
    if (!g_encSample->ltrParam.enableUseLtr) {
        g_encSample->frameCount++;
        OH_VideoEncoder_PushInputParameter(codec, index);
        return;
    }
    static int32_t useLtrIndex = 0;
    if (g_encSample->ltrParam.useLtrIndex == 0) {
        useLtrIndex = LTR_INTERVAL;
    } else if (g_encSample->ltrParam.useBadLtr) {
        useLtrIndex = BADPOC;
    } else {
        uint32_t interval = g_encSample->ltrParam.ltrInterval;
        if (interval > 0 && g_encSample->frameCount > 0 && (g_encSample->frameCount % interval == 0)) {
            useLtrIndex = g_encSample->frameCount / interval * interval;
        }
    }
    if (g_encSample->frameCount > useLtrIndex) {
        if (!g_encSample->ltrParam.useLtrOnce) {
            OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_USE_LTR, useLtrIndex);
        } else {
            if (!useLtrOnce) {
                OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_USE_LTR, useLtrIndex);
                useLtrOnce = true;
            }
        }
    } else if (g_encSample->frameCount == useLtrIndex && g_encSample->frameCount > 0) {
        int32_t sampleInterval = g_encSample->ltrParam.ltrInterval;
        OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_USE_LTR, useLtrIndex - sampleInterval);
    }
    g_encSample->frameCount++;
    OH_VideoEncoder_PushInputParameter(codec, index);
}

void VEncSyncSample::DumpLtrInfo(OH_AVBuffer *buffer)
{
    OH_AVFormat *format = OH_AVBuffer_GetParameter(buffer);
    int32_t isLtr = 0;
    int32_t framePoc = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_PER_FRAME_IS_LTR, &isLtr);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_PER_FRAME_POC, &framePoc);
}

void VEncSyncSample::DumpQPInfo(OH_AVBuffer *buffer)
{
    OH_AVFormat *format = OH_AVBuffer_GetParameter(buffer);
    int32_t qpAverage = 0;
    double mse = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_AVERAGE, &qpAverage);
    OH_AVFormat_GetDoubleValue(format, OH_MD_KEY_VIDEO_ENCODER_MSE, &mse);
}

void VEncSyncSample::DumpInfo(OH_AVCodecBufferAttr attr, OH_AVBuffer *buffer)
{
    if (enableLTR && attr.flags == AVCODEC_BUFFER_FLAGS_NONE) {
        DumpLtrInfo(buffer);
    }
    if (getQpMse && attr.flags == AVCODEC_BUFFER_FLAGS_NONE) {
        DumpQPInfo(buffer);
    }
}

int64_t VEncSyncSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

int32_t VEncSyncSample::ConfigureVideoEncoder()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DEFAULT_PIX_FMT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, defaultKeyFrameInterval);
    if (isAVCEncoder) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, avcProfile);
    } else {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, hevcProfile);
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, HEVC_PROFILE_MAIN);
    if (DEFAULT_BITRATE_MODE == CQ) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_QUALITY, defaultQuality);
    } else {
        (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, defaultBitrate);
    }
    if (enableQP) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MAX, defaultQp);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MIN, defaultQp);
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, DEFAULT_BITRATE_MODE);
    if (enableLTR && (ltrParam.ltrCount >= 0)) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_LTR_FRAME_COUNT, ltrParam.ltrCount);
    }
    if (enableColorSpaceParams) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_RANGE_FLAG, defaultRangeFlag);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_COLOR_PRIMARIES, DEFAULT_COLOR_PRIMARIES);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_TRANSFER_CHARACTERISTICS, DEFAULT_TRANSFER_CHARACTERISTICS);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_MATRIX_COEFFICIENTS, DEFAULT_MATRIX_COEFFICIENTS);
    }
    if (enableRepeat) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, defaultFrameAfter);
        if (setMaxCount) {
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, defaultMaxCount);
        }
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, enbleSyncMode);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_B_FRAME, enbleBFrameMode);
    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncSyncSample::ConfigureVideoEncoderTemporal(int32_t temporalGopSize)
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

    if (temporalConfig) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_SIZE, temporalGopSize);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
            ADJACENT_REFERENCE);
    }
    if (temporalEnable) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    } else {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 0);
    }
    if (temporalJumpMode) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, JUMP_REFERENCE);
    }
    if (enableLTR && (ltrParam.ltrCount > 0)) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_LTR_FRAME_COUNT, ltrParam.ltrCount);
    }
    if (temporalUniformly) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_SIZE, temporalGopSize);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
            UNIFORMLY_SCALED_REFERENCE);
    }
    if (configMain10) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, HEVC_PROFILE_MAIN_10);
    } else if (configMain) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, HEVC_PROFILE_MAIN);
    }
    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncSyncSample::ConfigureVideoEncoderFuzz(int32_t data)
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
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, enbleSyncMode);
    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncSyncSample::SetVideoEncoderCallback()
{
    signal_ = new VEncAPI11Signal();
    if (signal_ == nullptr) {
        cout << "Failed to new VEncAPI11Signal" << endl;
        return AV_ERR_UNKNOWN;
    }
    if (surfInput) {
        int32_t ret = OH_VideoEncoder_RegisterParameterCallback(venc_, onEncInputParam, static_cast<void *>(this));
        if (ret != AV_ERR_OK) {
            return ret;
        }
    }
    cb_.onError = VencError;
    cb_.onStreamChanged = VencFormatChanged;
    cb_.onNeedInputBuffer = onEncInputBufferAvailable;
    cb_.onNewOutputBuffer = onEncOutputBufferAvailable;
    
    return OH_VideoEncoder_RegisterCallback(venc_, cb_, static_cast<void *>(signal_));
}

int32_t VEncSyncSample::StateEos()
{
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [this]() { return signal_->inIdxQueue_.size() > 0; });
    uint32_t index = signal_->inIdxQueue_.front();
    OH_AVBuffer *buffer = signal_->inBufferQueue_.front();
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
    lock.unlock();
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    return OH_VideoEncoder_PushInputBuffer(venc_, index);
}
void VEncSyncSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

void VEncSyncSample::StopInloop()
{
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        if (enbleSyncMode == 0) {
            unique_lock<mutex> lock(signal_->inMutex_);
            clearIntqueue(signal_->inIdxQueue_);
            isRunning_.store(false);
            signal_->inCond_.notify_all();
            lock.unlock();
        }
        inputLoop_->join();
        inputLoop_ = nullptr;
    }
}

void VEncSyncSample::TestApi()
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

int32_t VEncSyncSample::CreateSurface()
{
    int32_t ret = 0;
    ret = OH_VideoEncoder_GetSurface(venc_, &nativeWindow);
    if (ret != AV_ERR_OK) {
        cout << "OH_VideoEncoder_GetSurface fail" << endl;
        return ret;
    }
    if (DEFAULT_PIX_FMT == AV_PIXEL_FORMAT_RGBA1010102) {
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, NATIVEBUFFER_PIXEL_FMT_RGBA_1010102);
    } else {
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    }
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

void VEncSyncSample::GetStride()
{
    OH_AVFormat *format = OH_VideoEncoder_GetInputDescription(venc_);
    int32_t inputStride = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_STRIDE, &inputStride);
    stride_ = inputStride;
    OH_AVFormat_Destroy(format);
}

int32_t VEncSyncSample::OpenFile()
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
        ret = OpenFileFail();
    }
    return ret;
}

int32_t VEncSyncSample::StartVideoEncoder()
{
    isRunning_.store(true);
    int32_t ret = 0;
    if (surfInput) {
        ret = CreateSurface();
        if (ret != AV_ERR_OK) {
            return ret;
        }
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
    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        return AV_ERR_UNKNOWN;
    }
    ReadMultiFilesFunc();
    if (VideoEncoder() != AV_ERR_OK) {
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VEncSyncSample::VideoEncoder()
{
    if (surfInput) {
        inputLoop_ = make_unique<thread>(&VEncSyncSample::InputFuncSurface, this);
    } else {
        if (enbleSyncMode == 0) {
            inputLoop_ = make_unique<thread>(&VEncSyncSample::InputFunc, this);
        } else {
            inputLoop_ = make_unique<thread>(&VEncSyncSample::SyncInputFunc, this);
        }
    }
    if (inputLoop_ == nullptr) {
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }
    if (enbleSyncMode == 0) {
        outputLoop_ = make_unique<thread>(&VEncSyncSample::OutputFunc, this);
    } else {
        outputLoop_ = make_unique<thread>(&VEncSyncSample::SyncOutputFunc, this);
    }
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

void VEncSyncSample::ReadMultiFilesFunc()
{
    if (!readMultiFiles) {
        inFile_->open(inpDir, ios::in | ios::binary);
        if (!inFile_->is_open()) {
            OpenFileFail();
        }
    }
}

int32_t VEncSyncSample::CreateVideoEncoder(const char *codecName)
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (!strcmp(codecName, tmpCodecName)) {
        isAVCEncoder = true;
    } else {
        isAVCEncoder = false;
    }
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    g_encSample = this;
    return venc_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

void VEncSyncSample::WaitForEOS()
{
    if (inputLoop_)
        inputLoop_->join();
    if (outputLoop_)
        outputLoop_->join();
    inputLoop_ = nullptr;
    outputLoop_ = nullptr;
}

uint32_t VEncSyncSample::ReturnZeroIfEOS(uint32_t expectedSize)
{
    if (inFile_->gcount() != static_cast<int>(expectedSize)) {
        cout << "no more data" << endl;
        return 0;
    }
    return 1;
}

uint32_t VEncSyncSample::ReadOneFrameYUV420SP(uint8_t *dst)
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

uint32_t VEncSyncSample::ReadOneFrameYUVP010(uint8_t *dst)
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

uint32_t VEncSyncSample::ReadOneFrameRGBA8888(uint8_t *dst)
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

uint32_t VEncSyncSample::FlushSurf(OHNativeWindowBuffer *ohNativeWindowBuffer, OH_NativeBuffer *nativeBuffer)
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

void VEncSyncSample::InputFuncSurface()
{
    OHNativeWindowBuffer *ohNativeWindowBuffer;
    int fenceFd = -1;
    if (nativeWindow == nullptr) {
        cout << "nativeWindow == nullptr" << endl;
        return;
    }
    int32_t err = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &ohNativeWindowBuffer, &fenceFd);
    if (err != 0) {
        cout << "RequestBuffer failed, GSError=" << err << endl;
        return;
    }
    if (fenceFd > 0) {
        close(fenceFd);
    }
    OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(ohNativeWindowBuffer);
    void *virAddr = nullptr;
    OH_NativeBuffer_Config config;
    OH_NativeBuffer_GetConfig (nativeBuffer, &config);
    err = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
    if (err != 0) {
        cout << "OH_NativeBuffer_Map failed, GSError=" << err << endl;
        return;
    }
    uint8_t *dst = (uint8_t *)virAddr;
    if (dst == nullptr) {
        return;
    }
    if (memcpy_s(dst, (config.stride * config.height * THREE) / DOUBLE, fuzzData, fuzzSize) != EOK) {
        return;
    }
    if (frameCount == maxFrameInput) {
        err = OH_VideoEncoder_NotifyEndOfStream(venc_);
        if (err != 0) {
            cout << "OH_VideoEncoder_NotifyEndOfStream failed" << endl;
            isRunning_.store(false);
        }
        return;
    }
    if (FlushSurf(ohNativeWindowBuffer, nativeBuffer)) {
        return;
    }
    usleep(FRAME_INTERVAL);
    frameCount++;
}

void VEncSyncSample::InputEnableRepeatSleep()
{
    inCount = inCount + 1;
    uint32_t inCountNum = 15;
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

int32_t VEncSyncSample::InitBuffer(OHNativeWindowBuffer *&ohNativeWindowBuffer,
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

uint32_t VEncSyncSample::ReadOneFrameByType(uint8_t *dst, OH_NativeBuffer_Format format)
{
    if (format == NATIVEBUFFER_PIXEL_FMT_RGBA_8888) {
        return ReadOneFrameRGBA8888(dst);
    } else if (format == NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP || format == NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP) {
        return ReadOneFrameYUV420SP(dst);
    } else if (format == NATIVEBUFFER_PIXEL_FMT_YCBCR_P010) {
        return ReadOneFrameYUVP010(dst);
    } else {
        cout << "error fileType" << endl;
        return 0;
    }
}

int32_t VEncSyncSample::OpenFileFail()
{
    cout << "file open fail" << endl;
    isRunning_.store(false);
    (void)OH_VideoEncoder_Stop(venc_);
    inFile_->close();
    inFile_.reset();
    inFile_ = nullptr;
    return AV_ERR_UNKNOWN;
}

void VEncSyncSample::FlushBuffer()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    std::queue<OH_AVBuffer *> empty;
    swap(empty, signal_->inBufferQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();
}

void VEncSyncSample::RepeatStartBeforeEOS()
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

bool VEncSyncSample::RandomEOS(uint32_t index)
{
    int32_t randomEos = g_rd() % 25;
    if (enableRandomEos && randomEos == frameCount) {
        OH_VideoEncoder_NotifyEndOfStream(venc_);
        cout << "random eos" << endl;
        frameCount++;
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
        return true;
    }
    return false;
}

void VEncSyncSample::AutoSwitchParam()
{
    int64_t currentBitrate = defaultBitrate;
    double currentFrameRate = defaultFrameRate;
    int32_t currentQP = defaultQp;
    if (frameCount == switchParamsTimeSec * static_cast<int32_t>(defaultFrameRate)) {
        OH_AVFormat *format = OH_AVFormat_Create();
        if (needResetBitrate) {
            currentBitrate = defaultBitrate >> 1;
            (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, currentBitrate);
        }
        if (needResetFrameRate) {
            currentFrameRate = defaultFrameRate * DOUBLE;
            (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, currentFrameRate);
        }
        if (needResetQP) {
            currentQP = defaultQp;
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MAX, currentQP);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MIN, currentQP);
        }
        SetParameter(format) == AV_ERR_OK ? (0) : (errCount++);
        OH_AVFormat_Destroy(format);
    }
    if (frameCount == static_cast<int32_t>(switchParamsTimeSec * static_cast<int32_t>(defaultFrameRate * DOUBLE))) {
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
        }
        if (needResetQP) {
            currentQP = defaultQp * DOUBLE;
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MAX, currentQP);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MIN, currentQP);
        }
        SetParameter(format) == AV_ERR_OK ? (0) : (errCount++);
        OH_AVFormat_Destroy(format);
    }
}

void VEncSyncSample::SetEOS(uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    int32_t res = OH_VideoEncoder_PushInputBuffer(venc_, index);
    cout << "OH_VideoEncoder_PushInputBuffer    EOS   res: " << res << endl;
    if (enbleSyncMode == 0) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
}

void VEncSyncSample::SetForceIDR()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_REQUEST_I_FRAME, 1);
    OH_VideoEncoder_SetParameter(venc_, format);
    OH_AVFormat_Destroy(format);
}

void VEncSyncSample::SetLTRParameter(OH_AVBuffer *buffer)
{
    if (!ltrParam.enableUseLtr) {
        return;
    }
    OH_AVFormat *format = OH_AVFormat_Create();
    if (frameCount % ltrParam.ltrInterval == 0) {
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_MARK_LTR, 1);
    }
    if (!ltrParam.enableUseLtr) {
        OH_AVBuffer_SetParameter(buffer, format) == AV_ERR_OK ? (0) : (errCount++);
        OH_AVFormat_Destroy(format);
        return;
    }
    static int32_t useLtrIndex = 0;
    if (ltrParam.useLtrIndex == 0) {
        useLtrIndex = LTR_INTERVAL;
    } else if (ltrParam.useBadLtr) {
        useLtrIndex = BADPOC;
    } else {
        uint32_t interval = ltrParam.ltrInterval;
        if (interval > 0 && frameCount > 0 && (frameCount % interval == 0)) {
            useLtrIndex = frameCount / interval * interval;
        }
    }
    if (frameCount > useLtrIndex) {
        if (!ltrParam.useLtrOnce) {
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_USE_LTR, useLtrIndex);
        } else {
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_USE_LTR, useLtrIndex);
            ltrParam.useLtrOnce = true;
        }
    } else if (frameCount == useLtrIndex && frameCount > 0) {
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_USE_LTR, useLtrIndex - ltrParam.ltrInterval);
    }
    OH_AVBuffer_SetParameter(buffer, format) == AV_ERR_OK ? (0) : (errCount++);
    OH_AVFormat_Destroy(format);
}

void VEncSyncSample::SetBufferParameter(OH_AVBuffer *buffer)
{
    int32_t currentQP = defaultQp;
    if (!enableAutoSwitchBufferParam) {
        return;
    }
    if (frameCount == switchParamsTimeSec * static_cast<int32_t>(defaultFrameRate)) {
        OH_AVFormat *format = OH_AVFormat_Create();
        if (needResetQP) {
            currentQP = defaultQp;
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MAX, currentQP);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MIN, currentQP);
        }
        OH_AVBuffer_SetParameter(buffer, format) == AV_ERR_OK ? (0) : (errCount++);
        OH_AVFormat_Destroy(format);
    }
    if (frameCount == static_cast<int32_t>(switchParamsTimeSec * static_cast<int32_t>(defaultFrameRate * DOUBLE))) {
        OH_AVFormat *format = OH_AVFormat_Create();
        if (needResetQP) {
            currentQP = defaultQp * DOUBLE;
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MAX, currentQP);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MIN, currentQP);
        }
        OH_AVBuffer_SetParameter(buffer, format) == AV_ERR_OK ? (0) : (errCount++);
        OH_AVFormat_Destroy(format);
    }
}

int32_t VEncSyncSample::PushData(OH_AVBuffer *buffer, uint32_t index, int32_t &result)
{
    int32_t res = -2;
    OH_AVCodecBufferAttr attr;
    uint8_t *fileBuffer = OH_AVBuffer_GetAddr(buffer);
    if (fileBuffer == nullptr) {
        cout << "Fatal: no memory" << endl;
        return -1;
    }
    uint32_t size = static_cast<uint32_t>(OH_AVBuffer_GetCapacity(buffer));
    if (DEFAULT_PIX_FMT == AV_PIXEL_FORMAT_RGBA || DEFAULT_PIX_FMT == AV_PIXEL_FORMAT_RGBA1010102) {
        if (size < defaultHeight * stride_) {
            return -1;
        }
        attr.size = ReadOneFrameRGBA8888(fileBuffer);
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
        SetEOS(index, buffer);
        return 0;
    }
    attr.pts = GetSystemTimeUs();
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    if (enableForceIDR && (frameCount % IDR_FRAME_INTERVAL == 0)) {
        SetForceIDR();
    }
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    SetBufferParameter(buffer);
    SetLTRParameter(buffer);
    result = OH_VideoEncoder_PushInputBuffer(venc_, index);
    frameCount++;
    if (enbleSyncMode == 0) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
    return res;
}

int32_t VEncSyncSample::CheckResult(bool isRandomEosSuccess, int32_t pushResult)
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

void VEncSyncSample::InputFunc()
{
    errCount = 0;
    bool flag = true;
    while (flag) {
        if (!isRunning_.load()) {
            flag = false;
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
            flag = false;
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
                flag = false;
                break;
            }
            if (enableAutoSwitchParam) {
                AutoSwitchParam();
            }
        }
        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
    }
}

void VEncSyncSample::SyncInputFunc()
{
    uint32_t index;
    if (OH_VideoEncoder_QueryInputBuffer(venc_, &index, syncInputWaitTime) != AV_ERR_OK) {
        return;
    }
    OH_AVBuffer *buffer = OH_VideoEncoder_GetInputBuffer(venc_, index);
    if (buffer == nullptr) {
        cout << "OH_VideoEncoder_GetInputBuffer fail" << endl;
        errCount = errCount + 1;
        return;
    }
    OH_AVCodecBufferAttr attr;
    int32_t bufferSize = OH_AVBuffer_GetCapacity(buffer);
    uint8_t *fileBuffer = OH_AVBuffer_GetAddr(buffer);
    if (fileBuffer == nullptr) {
        return;
    }
    if (memcpy_s(fileBuffer, bufferSize, fuzzData, fuzzSize) != EOK) {
        cout << "Fatal: memcpy fail" << endl;
        return;
    }
    attr.size = fuzzSize;
    if (frameCount == maxFrameInput) {
        SetEOS(index, buffer);
        return;
    }
    attr.pts = TIMESTAMP_BASE + DURATION_BASE * frameIndex_;
    frameIndex_++;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    OH_VideoEncoder_PushInputBuffer(venc_, index);
    frameCount++;
    if (sleepOnFPS) {
        usleep(FRAME_INTERVAL);
    }
}

int32_t VEncSyncSample::ReadFile(uint32_t index, OH_AVBuffer *buffer)
{
    int32_t res = 0;
    if (!inFile_->eof()) {
        bool isRandomEosSuccess = RandomEOS(index);
        if (isRandomEosSuccess) {
            res = DOUBLE;
            return res;
        }
        int32_t pushResult = 0;
        int32_t ret = PushData(buffer, index, pushResult);
        if (ret == 0) {
            res = ONE;
            return res;
        } else if (ret == -1) {
            res = DOUBLE;
            return res;
        }
        if (CheckResult(isRandomEosSuccess, pushResult) == -1) {
            res = ONE;
            return res;
        }
        if (enableAutoSwitchParam) {
            AutoSwitchParam();
        }
    }
    return res;
}

int32_t VEncSyncSample::CheckAttrFlag(OH_AVCodecBufferAttr attr)
{
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        cout << "attr.flags == AVCODEC_BUFFER_FLAGS_EOS" << endl;
        isRunning_.store(false);
        if (enbleSyncMode == 0) {
            unique_lock<mutex> inLock(signal_->inMutex_);
            signal_->inCond_.notify_all();
            signal_->outCond_.notify_all();
            inLock.unlock();
        }
        return -1;
    }
    if (attr.flags == AVCODEC_BUFFER_FLAGS_CODEC_DATA) {
        cout << "enc AVCODEC_BUFFER_FLAGS_CODEC_DATA" << attr.pts << endl;
        return 0;
    }
    outCount = outCount + 1;
    return 0;
}

void VEncSyncSample::OutputFuncFail()
{
    cout << "errCount > 0" << endl;
    if (enbleSyncMode == 0) {
        unique_lock<mutex> inLock(signal_->inMutex_);
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->outCond_.notify_all();
        inLock.unlock();
    }
    (void)Stop();
    Release();
}

void VEncSyncSample::OutputFunc()
{
    FILE *outFile = fopen(outDir, "wb");
    bool flag = true;
    while (flag) {
        if (!isRunning_.load()) {
            break;
        }
        OH_AVCodecBufferAttr attr;
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
        uint32_t index = signal_->outIdxQueue_.front();
        OH_AVBuffer *buffer = signal_->outBufferQueue_.front();
        signal_->outBufferQueue_.pop();
        signal_->outIdxQueue_.pop();
        lock.unlock();
        DumpInfo(attr, buffer);
        if (OH_AVBuffer_GetBufferAttr(buffer, &attr) != AV_ERR_OK) {
            errCount = errCount + 1;
        }
        if (CheckAttrFlag(attr) == -1) {
            break;
        }
        int size = attr.size;
        if (outFile == nullptr) {
            cout << "dump data fail" << endl;
        } else {
            fwrite(OH_AVBuffer_GetAddr(buffer), 1, size, outFile);
        }
        if (OH_VideoEncoder_FreeOutputBuffer(venc_, index) != AV_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            errCount = errCount + 1;
        }
        if (errCount > 0) {
            flag = false;
            OutputFuncFail();
            break;
        }
    }
    if (outFile) {
        (void)fclose(outFile);
    }
}

void VEncSyncSample::SyncOutputFunc()
{
    uint32_t index = 0;
    int32_t ret = OH_VideoEncoder_QueryOutputBuffer(venc_, &index, syncOutputWaitTime);
    if (ret != AV_ERR_OK) {
        return;
    }
    OH_AVBuffer *buffer = OH_VideoEncoder_GetOutputBuffer(venc_, index);
    if (buffer == nullptr) {
        cout << "OH_VideoEncoder_GetOutputBuffer fail" << endl;
        errCount = errCount + 1;
        return;
    }

    if (OH_VideoEncoder_FreeOutputBuffer(venc_, index) != AV_ERR_OK) {
        cout << "Fatal: ReleaseOutputBuffer fail" << endl;
        errCount = errCount + 1;
    }
}

int32_t VEncSyncSample::SyncOutputFuncEos(uint32_t &lastIndex, uint32_t &outFrames, uint32_t &index,
    OH_AVBuffer *buffer, OH_AVCodecBufferAttr &attr)
{
    if (getOutputBufferIndexNoExisted) {
        buffer = OH_VideoEncoder_GetOutputBuffer(venc_, index + HUNDRED);
        if (buffer == nullptr) {
            abnormalIndexValue = true;
            cout << "OH_VideoEncoder_GetOutputBuffer noexist buffer nullptr" << endl;
        }
        return AV_ERR_UNKNOWN;
    }
    if (outFrames == 0 && getOutputBufferIndexRepeated) {
        lastIndex = index;
    } else if (outFrames == 1 && getOutputBufferIndexRepeated) {
        cout << "getOutputBufferIndexRepeated lastIndex: " << lastIndex << "index" << index << endl;
        buffer = OH_VideoEncoder_GetOutputBuffer(venc_, lastIndex);
        if (buffer == nullptr) {
            abnormalIndexValue = true;
            cout << "OH_VideoEncoder_GetOutputBuffer repeat buffer nullptr" << endl;
        }
        return AV_ERR_UNKNOWN;
    }
    DumpInfo(attr, buffer);
    if (OH_AVBuffer_GetBufferAttr(buffer, &attr) != AV_ERR_OK) {
        errCount = errCount + 1;
    }
    if (CheckAttrFlag(attr) == -1) {
        if (queryInputBufferEOS) {
            OH_VideoEncoder_QueryInputBuffer(venc_, &index, 0);
            OH_VideoEncoder_QueryInputBuffer(venc_, &index, MILLION);
            OH_VideoEncoder_QueryInputBuffer(venc_, &index, -1);
        }
        if (queryOutputBufferEOS) {
            OH_VideoEncoder_QueryOutputBuffer(venc_, &index, 0);
            OH_VideoEncoder_QueryOutputBuffer(venc_, &index, MILLION);
            OH_VideoEncoder_QueryOutputBuffer(venc_, &index, -1);
        }
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VEncSyncSample::Flush()
{
    if (enbleSyncMode == 0) {
        unique_lock<mutex> inLock(signal_->inMutex_);
        clearIntqueue(signal_->inIdxQueue_);
        signal_->inCond_.notify_all();
        inLock.unlock();
        unique_lock<mutex> outLock(signal_->outMutex_);
        clearIntqueue(signal_->outIdxQueue_);
        signal_->outCond_.notify_all();
        outLock.unlock();
    }
    return OH_VideoEncoder_Flush(venc_);
}

int32_t VEncSyncSample::Reset()
{
    isRunning_.store(false);
    StopInloop();
    StopOutloop();
    ReleaseInFile();
    return OH_VideoEncoder_Reset(venc_);
}

int32_t VEncSyncSample::Release()
{
    int ret = OH_VideoEncoder_Destroy(venc_);
    venc_ = nullptr;
    if (signal_ != nullptr) {
        delete signal_;
        signal_ = nullptr;
    }
    return ret;
}

int32_t VEncSyncSample::Stop()
{
    StopInloop();
    if (enbleSyncMode == 0) {
        clearIntqueue(signal_->outIdxQueue_);
    }
    ReleaseInFile();
    return OH_VideoEncoder_Stop(venc_);
}

int32_t VEncSyncSample::Start()
{
    return OH_VideoEncoder_Start(venc_);
}

int32_t VEncSyncSample::QueryInputBuffer(uint32_t index, int64_t timeoutUs)
{
    return OH_VideoEncoder_QueryInputBuffer(venc_, &index, timeoutUs);
}

OH_AVBuffer *VEncSyncSample::GetInputBuffer(uint32_t index)
{
    return OH_VideoEncoder_GetInputBuffer(venc_, index);
}

int32_t VEncSyncSample::QueryOutputBuffer(uint32_t index, int64_t timeoutUs)
{
    return OH_VideoEncoder_QueryOutputBuffer(venc_, &index, timeoutUs);
}

OH_AVBuffer *VEncSyncSample::GetOutputBuffer(uint32_t index)
{
    return OH_VideoEncoder_GetOutputBuffer(venc_, index);
}

void VEncSyncSample::StopOutloop()
{
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        clearIntqueue(signal_->outIdxQueue_);
        signal_->outCond_.notify_all();
        lock.unlock();
    }
}

int32_t VEncSyncSample::SetParameter(OH_AVFormat *format)
{
    if (venc_) {
        return OH_VideoEncoder_SetParameter(venc_, format);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VEncSyncSample::SetParameter(int32_t data)
{
    if (venc_) {
        OH_AVFormat *format = OH_AVFormat_Create();
        if (format == nullptr) {
            return AV_ERR_UNKNOWN;
        }
        double frameRate = data;
        (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, frameRate);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, data);
        int ret = OH_VideoEncoder_SetParameter(venc_, format);
        OH_AVFormat_Destroy(format);
        return ret;
    }
    return AV_ERR_UNKNOWN;
}