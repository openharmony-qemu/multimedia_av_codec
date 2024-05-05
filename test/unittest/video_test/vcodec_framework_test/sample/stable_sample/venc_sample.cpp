/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "venc_sample.h"
#include <gtest/gtest.h>
#include "common/native_mfmagic.h"
#include "meta/meta_key.h"
#include "native_avcapability.h"
#include "native_avmagic.h"
#include "native_buffer.h"
#include "native_buffer_inner.h"
#include "native_window.h"
#include "surface/window.h"

#define PRINT_HILOG
#define TEST_ID sampleId_
#include "unittest_log.h"
#define TITLE_LOG UNITTEST_INFO_LOG("")

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using ReadFrame = std::function<int32_t(std::shared_ptr<VideoEncSignal> &, uint8_t *, int32_t, int32_t, int32_t)>;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoEncSample"};

int32_t ReadYUV420SP(std::shared_ptr<VideoEncSignal> &signal, uint8_t *addr, int32_t width, int32_t height,
                     int32_t stride)
{
    if (width == 0) {
        return 0;
    }
    int32_t pixcelSize = int32_t(stride / width);
    width *= pixcelSize;
    for (int32_t i = 0; i < (height * 3 / 2); ++i) { // 3: nom, 2: denom
        signal->inFile_->read(reinterpret_cast<char *>(addr), width);
        addr += stride;
    }
    return stride * height * 3 / 2; // 3: nom, 2: denom
}

int32_t ReadYUV420P(std::shared_ptr<VideoEncSignal> &signal, uint8_t *addr, int32_t width, int32_t height,
                    int32_t stride)
{
    if (width == 0) {
        return 0;
    }
    int32_t pixcelSize = int32_t(stride / width);
    width *= pixcelSize;
    // Y
    for (int32_t i = 0; i < height; ++i) {
        signal->inFile_->read(reinterpret_cast<char *>(addr), width);
        addr += stride;
    }
    width >>= 1;
    stride >>= 1;
    // UV
    for (int32_t i = 0; i < height; ++i) {
        signal->inFile_->read(reinterpret_cast<char *>(addr), width);
        addr += stride;
    }
    return stride * height * 3 / 2; // 3: nom, 2: denom
}

int32_t ReadRGBA(std::shared_ptr<VideoEncSignal> &signal, uint8_t *addr, int32_t width, int32_t height, int32_t stride)
{
    if (width == 0) {
        return 0;
    }
    int32_t pixcelSize = int32_t(stride / width);
    width *= pixcelSize;
    for (int32_t i = 0; i < height; ++i) {
        signal->inFile_->read(reinterpret_cast<char *>(addr), width);
        addr += stride;
    }
    return stride * height;
}

ReadFrame GetReadOneFrame(int32_t pixcelFormat)
{
    switch (pixcelFormat) {
        case AV_PIXEL_FORMAT_YUVI420:
            return ReadYUV420P;
        case AV_PIXEL_FORMAT_NV21:
        case AV_PIXEL_FORMAT_NV12:
            return ReadYUV420SP;
        case AV_PIXEL_FORMAT_RGBA:
            return ReadRGBA;
        default:
            return ReadYUV420P;
    }
}
} // namespace

namespace {
constexpr uint32_t DEFAULT_TIME_INTERVAL = 4166;
constexpr uint32_t MAX_OUTPUT_FRMAENUM = 60;
static inline int64_t GetTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    // 1000'000: second to micro second; 1000: nano second to micro second
    return (static_cast<int64_t>(now.tv_sec) * 1000'000 + (now.tv_nsec / 1000));
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
bool VideoEncSample::needDump_ = false;
uint64_t VideoEncSample::sampleTimout_ = 180;
uint64_t VideoEncSample::threadNum_ = 1;
VideoEncSample::VideoEncSample()
{
    static atomic<int32_t> sampleId = 0;
    sampleId_ = ++sampleId;
    TITLE_LOG;
    dyFormat_ = std::shared_ptr<OH_AVFormat>(OH_AVFormat_Create(), [](OH_AVFormat *ptr) { OH_AVFormat_Destroy(ptr); });
}

VideoEncSample::~VideoEncSample()
{
    TITLE_LOG;
    if (codec_ != nullptr) {
        int32_t ret = OH_VideoEncoder_Destroy(codec_);
        UNITTEST_CHECK_AND_INFO_LOG(ret == AV_ERR_OK, "OH_VideoEncoder_Destroy failed");
    }
    if (nativeWindow_ != nullptr) {
        DestoryNativeWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }
}

bool VideoEncSample::Create()
{
    TITLE_LOG;
    inPath_ = "/data/test/media/" + inPath_;
    outPath_ = "/data/test/media/" + outPath_ + to_string(sampleId_ % threadNum_) + ".h264";

    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(mime_.c_str(), true, HARDWARE);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(capability != nullptr, false, "OH_AVCodec_GetCapabilityByCategory failed");

    const char *name = OH_AVCapability_GetName(capability);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(name != nullptr, false, "OH_AVCapability_GetName failed");

    codec_ = OH_VideoEncoder_CreateByName(name);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, false, "OH_VideoEncoder_CreateByName failed");
    return true;
}

bool VideoEncSample::CreateByMime()
{
    TITLE_LOG;

    inPath_ = "/data/test/media/" + inPath_;
    outPath_ = "/data/test/media/" + outPath_ + to_string(sampleId_ % threadNum_) + ".h264";

    codec_ = OH_VideoEncoder_CreateByMime(mime_.c_str());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, false, "OH_VideoEncoder_CreateByMime failed");
    return true;
}

bool VideoEncSample::InitFile()
{
    if (signal_->inFile_ == nullptr) {
        signal_->inFile_ = make_unique<ifstream>();
        signal_->inFile_->open(inPath_, ios::in | ios::binary);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(signal_->inFile_ != nullptr, false, "create signal_->inFile_ failed");
        UNITTEST_CHECK_AND_RETURN_RET_LOG(signal_->inFile_->is_open(), false, "can not open signal_->inFile_");
    }
    if (signal_->outFile_ == nullptr && needDump_) {
        signal_->outFile_ = make_unique<ofstream>();
        signal_->outFile_->open(outPath_, ios::out | ios::binary);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(signal_->outFile_ != nullptr, false, "create signal_->outFile_ failed");
    }
    return true;
}

int32_t VideoEncSample::SetCallback(OH_AVCodecAsyncCallback callback, shared_ptr<VideoEncSignal> &signal)
{
    TITLE_LOG;
    signal_ = signal;
    if (!InitFile()) {
        return AV_ERR_UNKNOWN;
    }
    asyncCallback_ = callback;
    int32_t ret = OH_VideoEncoder_SetCallback(codec_, callback, reinterpret_cast<void *>(signal_.get()));
    isAVBufferMode_ = ret != AV_ERR_OK;
    return ret;
}

int32_t VideoEncSample::RegisterCallback(OH_AVCodecCallback callback, shared_ptr<VideoEncSignal> &signal)
{
    TITLE_LOG;
    signal_ = signal;
    if (!InitFile()) {
        return AV_ERR_UNKNOWN;
    }
    callback_ = callback;
    int32_t ret = OH_VideoEncoder_RegisterCallback(codec_, callback, reinterpret_cast<void *>(signal_.get()));
    isAVBufferMode_ = ret == AV_ERR_OK;
    return ret;
}

int32_t VideoEncSample::GetInputSurface()
{
    TITLE_LOG;
    int32_t ret = OH_VideoEncoder_GetSurface(codec_, &nativeWindow_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_GetSurface failed");
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_FORMAT fail" << endl;
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_BUFFER_GEOMETRY, sampleWidth_, sampleHeight_);
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail" << endl;
        return ret;
    }
    inputLoop_ = make_unique<thread>(&VideoEncSample::InputFuncSurface, this);
    isSurfaceMode_ = (ret == AV_ERR_OK);
    return ret;
}

int32_t VideoEncSample::Configure()
{
    TITLE_LOG;
    OH_AVFormat *format = OH_AVFormat_Create();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_UNKNOWN, "create format failed");
    bool setFormatRet = OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, sampleWidth_) &&
                        OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, sampleHeight_) &&
                        OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, samplePixcel_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(setFormatRet, AV_ERR_UNKNOWN, "set format failed");

    int32_t ret = OH_VideoEncoder_Configure(codec_, format);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_Configure failed");

    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VideoEncSample::Start()
{
    TITLE_LOG;
    using namespace chrono;

    time_ = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    int32_t ret = OH_VideoEncoder_Start(codec_);
    isFirstFrame_ = true;
    return ret;
}

bool VideoEncSample::WaitForEos()
{
    TITLE_LOG;
    using namespace chrono;

    unique_lock<mutex> lock(signal_->eosMutex_);
    auto lck = [this]() { return signal_->isOutEos_.load(); };
    bool isNotTimeout = signal_->eosCond_.wait_for(lock, seconds(sampleTimout_), lck);
    lock.unlock();
    int64_t tempTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    EXPECT_GE(frameOutputCount_, frameInputCount_ / 2); // 2: at least half of the input frame

    signal_->isRunning_ = false;
    usleep(100); // 100: wait for callback
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        signal_->outCond_.notify_all();
        outputLoop_->join();
    }
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        signal_->inCond_.notify_all();
        inputLoop_->join();
    }
    if (!isNotTimeout) {
        cout << "Run func timeout, time used: " << tempTime - time_ << "ms" << endl;
        return false;
    } else {
        cout << "Run func finish, time used: " << tempTime - time_ << "ms" << endl;
        return true;
    }
}

int32_t VideoEncSample::Prepare()
{
    TITLE_LOG;
    return OH_VideoEncoder_Prepare(codec_);
}

int32_t VideoEncSample::Stop()
{
    TITLE_LOG;
    FlushGuard guard(signal_);
    int32_t ret = OH_VideoEncoder_Stop(codec_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_Stop failed");
    return ret;
}

int32_t VideoEncSample::Flush()
{
    TITLE_LOG;
    FlushGuard guard(signal_);
    int32_t ret = OH_VideoEncoder_Flush(codec_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_Flush failed");
    return ret;
}

int32_t VideoEncSample::Reset()
{
    TITLE_LOG;
    {
        FlushGuard guard(signal_);
        int32_t ret = OH_VideoEncoder_Reset(codec_);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_Reset failed");
    }
    return Configure();
}

int32_t VideoEncSample::Release()
{
    TITLE_LOG;
    int32_t ret = OH_VideoEncoder_Destroy(codec_);
    codec_ = nullptr;
    return ret;
}

std::shared_ptr<OH_AVFormat> VideoEncSample::GetOutputDescription()
{
    TITLE_LOG;
    auto avformat = std::shared_ptr<OH_AVFormat>(OH_VideoEncoder_GetOutputDescription(codec_),
                                                 [](OH_AVFormat *ptr) { OH_AVFormat_Destroy(ptr); });
    UNITTEST_CHECK_AND_RETURN_RET_LOG(avformat != nullptr, nullptr, "OH_VideoEncoder_GetOutputDescription failed");
    return avformat;
}

std::shared_ptr<OH_AVFormat> VideoEncSample::GetInputDescription()
{
    TITLE_LOG;
    auto avformat = std::shared_ptr<OH_AVFormat>(OH_VideoEncoder_GetInputDescription(codec_),
                                                 [](OH_AVFormat *ptr) { OH_AVFormat_Destroy(ptr); });
    UNITTEST_CHECK_AND_RETURN_RET_LOG(avformat != nullptr, nullptr, "OH_VideoEncoder_GetInputDescription failed");
    return avformat;
}

int32_t VideoEncSample::SetParameter()
{
    TITLE_LOG;
    return OH_VideoEncoder_SetParameter(codec_, dyFormat_.get());
}

int32_t VideoEncSample::PushInputData(uint32_t index, OH_AVCodecBufferAttr attr)
{
    UNITTEST_INFO_LOG("index:%d", index);
    if (signal_->isInEos_) {
        if (!isFirstEos_) {
            UNITTEST_INFO_LOG("At Eos State");
            return AV_ERR_OK;
        }
        isFirstEos_ = false;
    }
    int32_t ret = AV_ERR_OK;
    if (isAVBufferMode_) {
        ret = OH_VideoEncoder_PushInputBuffer(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_PushInputBuffer failed");
    } else {
        ret = OH_VideoEncoder_PushInputData(codec_, index, attr);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_PushInputData failed");
    }
    frameInputCount_++;
    usleep(DEFAULT_TIME_INTERVAL);
    return AV_ERR_OK;
}

int32_t VideoEncSample::ReleaseOutputData(uint32_t index)
{
    UNITTEST_INFO_LOG("index:%d", index);
    int32_t ret;
    if (isAVBufferMode_) {
        ret = OH_VideoEncoder_FreeOutputBuffer(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_FreeOutputBuffer failed");
    } else {
        ret = OH_VideoEncoder_FreeOutputData(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_FreeOutputData failed");
    }
    frameOutputCount_++;
    return AV_ERR_OK;
}

int32_t VideoEncSample::NotifyEos()
{
    TITLE_LOG;
    int32_t ret = OH_VideoEncoder_NotifyEndOfStream(codec_);
    frameInputCount_++;
    return ret;
}

int32_t VideoEncSample::IsValid(bool &isValid)
{
    TITLE_LOG;
    return OH_VideoEncoder_IsValid(codec_, &isValid);
}

int32_t VideoEncSample::SetAVBufferAttr(OH_AVBuffer *avBuffer, OH_AVCodecBufferAttr &attr)
{
    if (!isAVBufferMode_) {
        return AV_ERR_OK;
    }
    int32_t ret = OH_AVBuffer_SetBufferAttr(avBuffer, &attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_AVBuffer_SetBufferAttr failed");
    return ret;
}

int32_t VideoEncSample::HandleInputFrame(uint32_t &index, OH_AVCodecBufferAttr &attr)
{
    uint8_t *addr = nullptr;
    index = signal_->inQueue_.front();
    OH_AVBuffer *avBuffer = nullptr;
    if (isAVBufferMode_) {
        avBuffer = signal_->inBufferQueue_.front();
        addr = OH_AVBuffer_GetAddr(avBuffer);
    } else {
        auto avMemory = signal_->inMemoryQueue_.front();
        addr = OH_AVMemory_GetAddr(avMemory);
    }
    signal_->PopInQueue();
    int32_t ret = HandleInputFrameInner(addr, attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "HandleInputFrameInner failed");
    return SetAVBufferAttr(avBuffer, attr);
}

int32_t VideoEncSample::HandleOutputFrame(uint32_t &index, OH_AVCodecBufferAttr &attr)
{
    uint8_t *addr = nullptr;
    index = signal_->outQueue_.front();
    int32_t ret = AV_ERR_OK;
    if (isAVBufferMode_) {
        auto avBuffer = signal_->outBufferQueue_.front();
        addr = OH_AVBuffer_GetAddr(avBuffer);
        ret = OH_AVBuffer_GetBufferAttr(avBuffer, &attr);
    } else {
        auto avMemory = signal_->outMemoryQueue_.front();
        addr = OH_AVMemory_GetAddr(avMemory);
        attr = signal_->outAttrQueue_.front();
    }
    signal_->PopOutQueue();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_AVBuffer_GetBufferAttr failed, index: %d", index);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(addr != nullptr || isSurfaceMode_, AV_ERR_UNKNOWN,
                                      "out buffer is nullptr, index: %d", index);
    ret = HandleOutputFrameInner(addr, attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "HandleOutputFrameInner failed, index: %d", index);
    return ret;
}

int32_t VideoEncSample::HandleInputFrame(OH_AVMemory *data, OH_AVCodecBufferAttr &attr)
{
    uint8_t *addr = OH_AVMemory_GetAddr(data);
    return HandleInputFrameInner(addr, attr);
}

int32_t VideoEncSample::HandleOutputFrame(OH_AVMemory *data, OH_AVCodecBufferAttr &attr)
{
    uint8_t *addr = OH_AVMemory_GetAddr(data);
    return HandleOutputFrameInner(addr, attr);
}

int32_t VideoEncSample::HandleInputFrame(OH_AVBuffer *data)
{
    uint8_t *addr = OH_AVBuffer_GetAddr(data);
    OH_AVCodecBufferAttr attr;
    int32_t ret = HandleInputFrameInner(addr, attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "HandleInputFrameInner failed");
    ret = OH_AVBuffer_SetBufferAttr(data, &attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_AVBuffer_SetBufferAttr failed");
    return AV_ERR_OK;
}

int32_t VideoEncSample::HandleOutputFrame(OH_AVBuffer *data)
{
    uint8_t *addr = OH_AVBuffer_GetAddr(data);
    OH_AVCodecBufferAttr attr;
    int32_t ret = OH_AVBuffer_GetBufferAttr(data, &attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_AVBuffer_GetBufferAttr failed");
    return HandleOutputFrameInner(addr, attr);
}

int32_t VideoEncSample::HandleInputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(addr != nullptr, AV_ERR_UNKNOWN, "in buffer is nullptr");
    attr.offset = 0;
    attr.pts = GetTimeUs();
    if (frameCount_ <= frameInputCount_) {
        signal_->isInEos_ = true;
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        attr.size = 0;
        UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d", attr.size, (int32_t)(attr.flags));
        return AV_ERR_OK;
    }
    if (signal_->inFile_->eof()) {
        (void)signal_->inFile_->seekg(0);
    }
    int32_t width = 0;
    int32_t height = 0;
    int32_t stride = 0;
    auto format = GetInputDescription();
    OH_AVFormat_GetIntValue(format.get(), Media::Tag::VIDEO_WIDTH, &width);
    OH_AVFormat_GetIntValue(format.get(), Media::Tag::VIDEO_HEIGHT, &height);
    OH_AVFormat_GetIntValue(format.get(), Media::Tag::VIDEO_STRIDE, &stride);

    attr.size = GetReadOneFrame(samplePixcel_)(signal_, addr, width, height, stride);
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    if (isFirstFrame_) {
        attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        isFirstFrame_ = false;
    }
    uint64_t *addr64 = reinterpret_cast<uint64_t *>(addr);
    UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d, addr[0]:%" PRIX64, attr.size, (int32_t)(attr.flags), addr64[0]);
    return AV_ERR_OK;
}

int32_t VideoEncSample::HandleOutputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(addr != nullptr || isSurfaceMode_, AV_ERR_UNKNOWN, "out buffer is nullptr");

    if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
        UNITTEST_INFO_LOG("out frame:%d, in frame:%d", frameOutputCount_.load(), frameInputCount_.load());
        signal_->isOutEos_ = true;
        signal_->eosCond_.notify_all();
        return AV_ERR_OK;
    }
    if (needDump_ && !isSurfaceMode_ && frameOutputCount_ < MAX_OUTPUT_FRMAENUM) {
        (void)signal_->outFile_->write(reinterpret_cast<char *>(addr), attr.size);
    }
    uint64_t *addr64 = reinterpret_cast<uint64_t *>(addr);
    UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d, addr[0]:%" PRIX64, attr.size, (int32_t)(attr.flags), addr64[0]);
    return AV_ERR_OK;
}

void VideoEncSample::InputFuncSurface()
{
    while (signal_->isRunning_.load()) {
        if (signal_->isFlushing_.load()) {
            continue;
        }
        OHNativeWindowBuffer *ohNativeWindowBuffer;
        int fenceFd = -1;
        UNITTEST_CHECK_AND_BREAK_LOG(nativeWindow_ != nullptr, "nativeWindow_ == nullptr");

        int32_t err = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow_, &ohNativeWindowBuffer, &fenceFd);
        UNITTEST_CHECK_AND_CONTINUE_LOG(err == 0, "RequestBuffer failed, GSError=%d", err);
        if (fenceFd > 0) {
            close(fenceFd);
        }
        OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(ohNativeWindowBuffer);
        void *virAddr = nullptr;
        err = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
        if (err != 0) {
            cout << "OH_NativeBuffer_Map failed, GSError=" << err << endl;
            signal_->isRunning_.store(false);
            break;
        }
        char *dst = static_cast<char *>(virAddr);
        const SurfaceBuffer *sbuffer = SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);
        int32_t stride = sbuffer->GetStride();
        if (dst == nullptr || stride < static_cast<int32_t>(sampleWidth_)) {
            cout << "invalid va or stride=" << stride << endl;
            err = NativeWindowCancelBuffer(nativeWindow_, ohNativeWindowBuffer);
            UNITTEST_CHECK_AND_INFO_LOG(err == 0, "NativeWindowCancelBuffer failed");
            signal_->isRunning_.store(false);
            break;
        }
        if (signal_->inFile_->eof()) {
            (void)signal_->inFile_->seekg(0);
        }
        int32_t bufferSize = stride * sampleHeight_ * 3 / 2; // 3: nom, 2: denom
        for (int32_t i = 0; i < bufferSize; i += stride) {
            (void)signal_->inFile_->read(dst + i, sampleWidth_);
        }
        if (frameInputCount_ >= frameCount_) {
            err = NotifyEos();
            UNITTEST_CHECK_AND_INFO_LOG(err == 0, "OH_VideoEncoder_NotifyEndOfStream failed");
            break;
        }
        err = InputProcess(nativeBuffer, ohNativeWindowBuffer);
        UNITTEST_CHECK_AND_BREAK_LOG(err == 0, "InputProcess failed, GSError=%d", err);
        usleep(DEFAULT_TIME_INTERVAL);
    }
    UNITTEST_INFO_LOG("exit, frameInputCount_: %d, frameCount_: %d", frameInputCount_.load(), frameCount_);
}

int32_t VideoEncSample::InputProcess(OH_NativeBuffer *nativeBuffer, OHNativeWindowBuffer *ohNativeWindowBuffer)
{
    using namespace chrono;
    int32_t ret = 0;
    struct Region region;
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0;
    rect->y = 0;
    rect->w = sampleWidth_;
    rect->h = sampleHeight_;
    region.rects = rect;
    int64_t systemTimeUs = time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count();
    NativeWindowHandleOpt(nativeWindow_, SET_UI_TIMESTAMP, systemTimeUs);
    ret = OH_NativeBuffer_Unmap(nativeBuffer);
    if (ret != 0) {
        cout << "OH_NativeBuffer_Unmap failed" << endl;
        delete rect;
        return ret;
    }

    frameInputCount_++;
    ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow_, ohNativeWindowBuffer, -1, region);
    delete rect;
    if (ret != 0) {
        cout << "FlushBuffer failed" << endl;
        return ret;
    }
    return ret;
}

int32_t VideoEncSample::Operate()
{
    int32_t ret = AV_ERR_OK;
    if (operation_ == "Flush") {
        ret = Flush();
        return ret == AV_ERR_OK ? Start() : ret;
    } else if (operation_ == "Stop") {
        ret = Stop();
        return ret == AV_ERR_OK ? Start() : ret;
    } else if (operation_ == "Reset") {
        ret = Reset();
        return ret == AV_ERR_OK ? Start() : ret;
    } else if (operation_ == "GetOutputDescription") {
        auto format = GetOutputDescription();
        ret = format == nullptr ? AV_ERR_UNKNOWN : ret;
        return ret;
    } else if (operation_ == "SetCallback") {
        return isAVBufferMode_ ? RegisterCallback(callback_, signal_) : SetCallback(asyncCallback_, signal_);
    }
    UNITTEST_INFO_LOG("unknown GetParam(): %s", operation_.c_str());
    return AV_ERR_UNKNOWN;
}

} // namespace MediaAVCodec
} // namespace OHOS
