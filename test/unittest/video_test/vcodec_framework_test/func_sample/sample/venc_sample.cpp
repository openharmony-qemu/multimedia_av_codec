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

#include "venc_sample.h"
#include <gtest/gtest.h>
#include "iconsumer_surface.h"
#include "meta/meta_key.h"
#include "native_buffer_inner.h"
#include "openssl/crypto.h"
#include "openssl/sha.h"

#ifdef VIDEOENC_CAPI_UNIT_TEST
#include "native_window.h"
#include "surface_capi_mock.h"
#else
#include "surface_inner_mock.h"
#endif

using namespace std;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
using namespace OHOS::MediaAVCodec;
namespace {
constexpr int32_t REQUEST_I_FRAME = 1;
constexpr int32_t REQUEST_I_FRAME_NUM = 13;
constexpr uint32_t BUFFER_COUNT = 59;
constexpr uint8_t SHA_AVC[SHA512_DIGEST_LENGTH] = {
    0x39, 0x2c, 0x3a, 0x78, 0xf0, 0xf7, 0xbe, 0xde, 0xc8, 0x2d, 0x45, 0x19, 0x9d, 0xd8, 0x3e, 0x88,
    0x5f, 0xf0, 0x0b, 0xf5, 0x14, 0x01, 0x21, 0xea, 0xd2, 0xcf, 0xe3, 0xd9, 0x35, 0x6a, 0x2c, 0x98,
    0xc6, 0x48, 0xc7, 0x90, 0xca, 0xe7, 0xc1, 0xb0, 0x4e, 0x9c, 0x05, 0x1e, 0xdd, 0x22, 0xd2, 0xe0,
    0x5e, 0x9a, 0xf8, 0xbc, 0xbe, 0x39, 0x26, 0x46, 0x6e, 0xa3, 0xcd, 0x1b, 0xbb, 0xf5, 0xc8, 0x87};
constexpr uint8_t SHA_HEVC[SHA512_DIGEST_LENGTH] = {
    0xb0, 0xca, 0x29, 0xa3, 0x3c, 0x8e, 0x36, 0x3f, 0xbc, 0x30, 0xa8, 0x70, 0x09, 0x29, 0xb5, 0xff,
    0x8f, 0xe2, 0xf9, 0x58, 0xc5, 0x00, 0x02, 0x7c, 0xa9, 0x05, 0xe0, 0x69, 0x09, 0xa7, 0x2e, 0xb2,
    0xdf, 0x5d, 0xf4, 0x05, 0xea, 0xde, 0xe9, 0x9b, 0x1e, 0x5b, 0x37, 0x04, 0x2f, 0x3d, 0xe9, 0x2c,
    0xb2, 0x8c, 0xc3, 0x99, 0xd4, 0xdc, 0xdf, 0xee, 0xb4, 0xd9, 0x0c, 0xd0, 0xee, 0x39, 0x94, 0x3c};

uint8_t g_mdTest[SHA512_DIGEST_LENGTH];
std::atomic<uint32_t> g_shaBufferCount = 0;
SHA512_CTX g_ctxTest;

void UpdateSHA(std::unique_ptr<std::ofstream> &outFile, const char *addr, int32_t size, bool needCheckSHA)
{
    if (needCheckSHA) {
        ++g_shaBufferCount;
    }
    if (needCheckSHA && g_shaBufferCount < BUFFER_COUNT) {
        SHA512_Update(&g_ctxTest, addr, size);
    }
    if (VideoEncSample::needDump_) {
        if (!outFile->is_open()) {
            cout << "output data fail" << endl;
        }
        (void)outFile->write(addr, size);
    }
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
VEncCallbackTest::VEncCallbackTest(std::shared_ptr<VEncSignal> signal) : signal_(signal) {}

VEncCallbackTest::~VEncCallbackTest() {}

void VEncCallbackTest::OnError(int32_t errorCode)
{
    cout << "ADec Error errorCode=" << errorCode;
    if (signal_ == nullptr) {
        return;
    }
    signal_->errorNum_ += 1;
    cout << ", errorNum=" << signal_->errorNum_ << endl;
}

void VEncCallbackTest::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    UNITTEST_INFO_LOG("format changed: %s", format->DumpInfo());
}

void VEncCallbackTest::OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->inMutex_);
    if (!signal_->isRunning_.load() && !signal_->isPreparing_.load()) {
        return;
    }
    signal_->inIndexQueue_.push(index);
    signal_->inMemoryQueue_.push(data);
    signal_->inCond_.notify_all();
}

void VEncCallbackTest::OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->outMutex_);
    if (!signal_->isRunning_.load() && !signal_->isPreparing_.load()) {
        return;
    }
    signal_->outIndexQueue_.push(index);
    signal_->outMemoryQueue_.push(data);
    signal_->outAttrQueue_.push(attr);
    signal_->outCond_.notify_all();
}

VEncCallbackTestExt::VEncCallbackTestExt(std::shared_ptr<VEncSignal> signal) : signal_(signal) {}

VEncCallbackTestExt::~VEncCallbackTestExt() {}

void VEncCallbackTestExt::OnError(int32_t errorCode)
{
    cout << "VEnc Error errorCode=" << errorCode;
    if (signal_ == nullptr) {
        return;
    }
    signal_->errorNum_ += 1;
    cout << ", errorNum=" << signal_->errorNum_ << endl;
}

void VEncCallbackTestExt::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    UNITTEST_INFO_LOG("format changed: %s", format->DumpInfo());
}

void VEncCallbackTestExt::OnNeedInputData(uint32_t index, std::shared_ptr<AVBufferMock> data)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->inMutex_);
    if (!signal_->isRunning_.load() && !signal_->isPreparing_.load()) {
        return;
    }
    signal_->inIndexQueue_.push(index);
    signal_->inBufferQueue_.push(data);
    signal_->inCond_.notify_all();
}

void VEncCallbackTestExt::OnNewOutputData(uint32_t index, std::shared_ptr<AVBufferMock> data)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->outMutex_);
    if (!signal_->isRunning_.load() && !signal_->isPreparing_.load()) {
        return;
    }
    signal_->outIndexQueue_.push(index);
    signal_->outBufferQueue_.push(data);
    signal_->outCond_.notify_all();
}

VEncParamCallbackTest::VEncParamCallbackTest(std::shared_ptr<VEncSignal> signal) : signal_(signal) {}

VEncParamCallbackTest::~VEncParamCallbackTest() {}

void VEncParamCallbackTest::OnInputParameterAvailable(uint32_t index, std::shared_ptr<FormatMock> parameter)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->inMutex_);
    if (!signal_->isRunning_.load() && !signal_->isPreparing_.load()) {
        return;
    }
    signal_->inIndexQueue_.push(index);
    signal_->inAttrQueue_.push(nullptr);
    signal_->inFormatQueue_.push(parameter);
    signal_->inCond_.notify_all();
}

VEncParamWithAttrCallbackTest::VEncParamWithAttrCallbackTest(std::shared_ptr<VEncSignal> signal) : signal_(signal) {}

VEncParamWithAttrCallbackTest::~VEncParamWithAttrCallbackTest() {}

void VEncParamWithAttrCallbackTest::OnInputParameterWithAttrAvailable(uint32_t index,
                                                                      std::shared_ptr<FormatMock> attribute,
                                                                      std::shared_ptr<FormatMock> parameter)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->inMutex_);
    if (!signal_->isRunning_.load() && !signal_->isPreparing_.load()) {
        return;
    }
    signal_->inIndexQueue_.push(index);
    signal_->inAttrQueue_.push(attribute);
    signal_->inFormatQueue_.push(parameter);
    signal_->inCond_.notify_all();
}

bool VideoEncSample::needDump_ = false;
VideoEncSample::VideoEncSample(std::shared_ptr<VEncSignal> signal)
    : signal_(signal), inPath_("/data/test/media/1280_720_nv.yuv"), nativeWindow_(nullptr)
{
}

VideoEncSample::~VideoEncSample()
{
    FlushInner();
    if (videoEnc_ != nullptr) {
        (void)videoEnc_->Release();
    }
    if (inFile_ != nullptr && inFile_->is_open()) {
        inFile_->close();
    }
    if (outFile_ != nullptr && outFile_->is_open()) {
        outFile_->close();
    };
    consumer_ = nullptr;
    producer_ = nullptr;
    if (nativeWindow_ != nullptr) {
#ifdef VIDEOENC_CAPI_UNIT_TEST
        nativeWindow_->DecStrongRef(nativeWindow_);
#else
        DestoryNativeWindow(nativeWindow_);
#endif
        nativeWindow_ = nullptr;
    }
}

bool VideoEncSample::CreateVideoEncMockByMime(const std::string &mime)
{
    videoEnc_ = VCodecMockFactory::CreateVideoEncMockByMime(mime);
    return videoEnc_ != nullptr;
}

bool VideoEncSample::CreateVideoEncMockByName(const std::string &name)
{
    videoEnc_ = VCodecMockFactory::CreateVideoEncMockByName(name);
    return videoEnc_ != nullptr;
}

int32_t VideoEncSample::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = videoEnc_->SetCallback(cb);
    isAVBufferMode_ = ret != AV_ERR_OK;
    return ret;
}

int32_t VideoEncSample::SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = videoEnc_->SetCallback(cb);
    isAVBufferMode_ = ret == AV_ERR_OK;
    return ret;
}

int32_t VideoEncSample::SetCallback(std::shared_ptr<MediaCodecParameterCallbackMock> cb)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = videoEnc_->SetCallback(cb);
    isSetParamCallback_ = ret == AV_ERR_OK;
    return ret;
}

int32_t VideoEncSample::SetCallback(std::shared_ptr<MediaCodecParameterWithAttrCallbackMock> cb)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = videoEnc_->SetCallback(cb);
    isSetParamCallback_ = ret == AV_ERR_OK;
    return ret;
}

int32_t VideoEncSample::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->Configure(format);
}

int32_t VideoEncSample::Prepare()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->Prepare();
}

int32_t VideoEncSample::SetCustomBuffer(std::shared_ptr<AVBufferMock> buffer)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->SetCustomBuffer(buffer);
}

int32_t VideoEncSample::Start()
{
    if (signal_ == nullptr || videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    PrepareInner();
    int32_t ret = videoEnc_->Start();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: Start fail");
    if (isAVBufferMode_) {
        RunInnerExt();
    } else {
        RunInner();
    }
    WaitForEos();
    return ret;
}

int32_t VideoEncSample::Stop()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->Stop();
}

int32_t VideoEncSample::Flush()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->Flush();
}

int32_t VideoEncSample::Reset()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->Reset();
}

int32_t VideoEncSample::Release()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->Release();
}

std::shared_ptr<FormatMock> VideoEncSample::GetOutputDescription()
{
    if (videoEnc_ == nullptr) {
        return nullptr;
    }
    return videoEnc_->GetOutputDescription();
}

std::shared_ptr<FormatMock> VideoEncSample::GetInputDescription()
{
    if (videoEnc_ == nullptr) {
        return nullptr;
    }
    return videoEnc_->GetInputDescription();
}

int32_t VideoEncSample::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->SetParameter(format);
}

int32_t VideoEncSample::NotifyEos()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->NotifyEos();
}

int32_t VideoEncSample::PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    frameInputCount_++;
    return videoEnc_->PushInputData(index, attr);
}

int32_t VideoEncSample::FreeOutputData(uint32_t index)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->FreeOutputData(index);
}

int32_t VideoEncSample::PushInputBuffer(uint32_t index)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    frameInputCount_++;
    return videoEnc_->PushInputBuffer(index);
}

int32_t VideoEncSample::PushInputParameter(uint32_t index)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->PushInputParameter(index);
}

int32_t VideoEncSample::FreeOutputBuffer(uint32_t index)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->FreeOutputBuffer(index);
}

#ifdef VIDEOENC_CAPI_UNIT_TEST
int32_t VideoEncSample::CreateInputSurface()
{
    auto surfaceMock = videoEnc_->CreateInputSurface();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(surfaceMock != nullptr, AV_ERR_NO_MEMORY, "OH_VideoEncoder_GetSurface fail");

    nativeWindow_ = std::static_pointer_cast<SurfaceCapiMock>(surfaceMock)->GetSurface();
    nativeWindow_->IncStrongRef(nativeWindow_);
    int32_t ret = 0;
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, AV_ERR_UNKNOWN, "NativeWindowHandleOpt SET_FORMAT fail.Error:%d", ret);

    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_BUFFER_GEOMETRY, DEFAULT_WIDTH_VENC,
                                                DEFAULT_HEIGHT_VENC);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, AV_ERR_UNKNOWN,
                                      "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail.Error:%d", ret);

    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_USAGE,
                                                BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, AV_ERR_UNKNOWN, "NativeWindowHandleOpt SET_USAGE fail.Error:%d", ret);
    isSurfaceMode_ = true;
    return AV_ERR_OK;
}
#else
int32_t VideoEncSample::CreateInputSurface()
{
    auto surfaceMock = videoEnc_->CreateInputSurface();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(surfaceMock != nullptr, AV_ERR_INVALID_VAL, "CreateInputSurface fail");

    sptr<Surface> surface = std::static_pointer_cast<SurfaceInnerMock>(surfaceMock)->GetSurface();
    nativeWindow_ = CreateNativeWindowFromSurface(&surface);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(surfaceMock != nullptr, AV_ERR_INVALID_VAL,
                                      "CreateNativeWindowFromSurface failed!");

    int32_t ret = AV_ERR_OK;

    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, AV_ERR_INVALID_VAL, "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail");

    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_BUFFER_GEOMETRY, DEFAULT_WIDTH_VENC,
                                                DEFAULT_HEIGHT_VENC);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, AV_ERR_INVALID_VAL, "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail");

    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_USAGE,
                                                BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, AV_ERR_UNKNOWN, "NativeWindowHandleOpt SET_USAGE fail.Error:%d", ret);
    isSurfaceMode_ = true;
    return AV_ERR_OK;
}
#endif

bool VideoEncSample::IsValid()
{
    if (videoEnc_ == nullptr) {
        return false;
    }
    return videoEnc_->IsValid();
}

void VideoEncSample::SetOutPath(const std::string &path)
{
    outPath_ = path + ".dat";
}

void VideoEncSample::FlushInner()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> queueLock(signal_->inMutex_);
        std::queue<uint32_t> tempIndex;
        std::swap(tempIndex, signal_->inIndexQueue_);
        std::queue<std::shared_ptr<AVMemoryMock>> tempInMemory;
        std::swap(tempInMemory, signal_->inMemoryQueue_);
        std::queue<std::shared_ptr<AVBufferMock>> tempInBuffer;
        std::swap(tempInBuffer, signal_->inBufferQueue_);
        queueLock.unlock();
        signal_->inCond_.notify_all();
        inputLoop_->join();

        frameInputCount_ = frameOutputCount_ = 0;
        if (inFile_ == nullptr || !inFile_->is_open()) {
            inFile_ = std::make_unique<std::ifstream>();
            ASSERT_NE(inFile_, nullptr);
            inFile_->open(inPath_, std::ios::in | std::ios::binary);
            ASSERT_TRUE(inFile_->is_open());
        }
    }
    if (inputSurfaceLoop_ != nullptr && inputSurfaceLoop_->joinable()) {
        inputSurfaceLoop_->join();
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        std::queue<uint32_t> tempIndex;
        std::swap(tempIndex, signal_->outIndexQueue_);
        std::queue<OH_AVCodecBufferAttr> tempOutAttr;
        std::swap(tempOutAttr, signal_->outAttrQueue_);
        std::queue<std::shared_ptr<AVMemoryMock>> tempOutMemory;
        std::swap(tempOutMemory, signal_->outMemoryQueue_);
        std::queue<std::shared_ptr<AVBufferMock>> tempOutBuffer;
        std::swap(tempOutBuffer, signal_->outBufferQueue_);
        lock.unlock();
        signal_->outCond_.notify_all();
        outputLoop_->join();
    }
}

int32_t VideoEncSample::ReadOneFrame()
{
    return DEFAULT_WIDTH_VENC * DEFAULT_HEIGHT_VENC * 3 / 2; // 3: nom, 2: denom
}

void VideoEncSample::RunInner()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isPreparing_.store(false);
    signal_->isRunning_.store(true);
    if (isSurfaceMode_) {
        inputSurfaceLoop_ = make_unique<thread>(&VideoEncSample::InputFuncSurface, this);
        inputLoop_ = isSetParamCallback_ ? make_unique<thread>(&VideoEncSample::InputParamLoopFunc, this) : nullptr;
        ASSERT_NE(inputSurfaceLoop_, nullptr);
    } else {
        inputLoop_ = make_unique<thread>(&VideoEncSample::InputLoopFunc, this);
        ASSERT_NE(inputLoop_, nullptr);
    }
    signal_->inCond_.notify_all();

    outputLoop_ = make_unique<thread>(&VideoEncSample::OutputLoopFunc, this);
    ASSERT_NE(outputLoop_, nullptr);
    signal_->outCond_.notify_all();
}

void VideoEncSample::RunInnerExt()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isPreparing_.store(false);
    signal_->isRunning_.store(true);
    if (isSurfaceMode_) {
        inputSurfaceLoop_ = make_unique<thread>(&VideoEncSample::InputFuncSurface, this);
        inputLoop_ = isSetParamCallback_ ? make_unique<thread>(&VideoEncSample::InputParamLoopFunc, this) : nullptr;
        ASSERT_NE(inputSurfaceLoop_, nullptr);
    } else {
        inputLoop_ = make_unique<thread>(&VideoEncSample::InputLoopFuncExt, this);
        ASSERT_NE(inputLoop_, nullptr);
    }
    signal_->inCond_.notify_all();

    outputLoop_ = make_unique<thread>(&VideoEncSample::OutputLoopFuncExt, this);
    ASSERT_NE(outputLoop_, nullptr);
    signal_->outCond_.notify_all();
}

void VideoEncSample::WaitForEos()
{
    unique_lock<mutex> lock(signal_->mutex_);
    auto lck = [this]() { return !signal_->isRunning_.load(); };
    bool isNotTimeout = signal_->cond_.wait_for(lock, chrono::seconds(SAMPLE_TIMEOUT), lck);
    lock.unlock();
    int64_t tempTime =
        chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
    EXPECT_TRUE(isNotTimeout);
    if (!isNotTimeout) {
        cout << "Run func timeout, time used: " << tempTime - time_ << "ms" << endl;
    } else {
        cout << "Run func finish, time used: " << tempTime - time_ << "ms" << endl;
    }
    FlushInner();
}

void VideoEncSample::PrepareInner()
{
    if (signal_ == nullptr) {
        return;
    }
    FlushInner();
    signal_->isPreparing_.store(true);
    signal_->isRunning_.store(false);
    if (inFile_ == nullptr || !inFile_->is_open()) {
        inFile_ = std::make_unique<std::ifstream>();
        ASSERT_NE(inFile_, nullptr);
        inFile_->open(inPath_, std::ios::in | std::ios::binary);
        ASSERT_TRUE(inFile_->is_open());
    }
    if (needCheckSHA_) {
        g_shaBufferCount = 0;
        SHA512_Init(&g_ctxTest);
    }
    time_ = chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
}

void VideoEncSample::InputParamLoopFunc()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoEnc_, nullptr);
    frameInputCount_ = 0;
    isFirstFrame_ = true;
    while (signal_->isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(
            lock, [this]() { return (signal_->inIndexQueue_.size() > 0) || (!signal_->isRunning_.load()); });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "InputLoopFunc stop running");

        int32_t index = signal_->inIndexQueue_.front();
        auto format = signal_->inFormatQueue_.front();
        auto attr = signal_->inAttrQueue_.front();
        if (attr != nullptr) {
            int64_t pts = 0;
            EXPECT_EQ(true, attr->GetLongValue(Media::Tag::MEDIA_TIME_STAMP, pts));
            UNITTEST_INFO_LOG("attribute: %s", attr->DumpInfo());
        }

        if (isTemporalScalabilitySyncIdr_ && frameInputCount_ == REQUEST_I_FRAME_NUM) {
            format->PutIntValue(Media::Tag::VIDEO_REQUEST_I_FRAME, REQUEST_I_FRAME);
        }

        if (isDiscardFrame_ && (frameInputCount_ % 2) == 0) { // 2: encode half frames
            format->PutIntValue(Media::Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, 1);
        }
        UNITTEST_INFO_LOG("parameter: %s", format->DumpInfo());
        int32_t ret = PushInputParameter(index);
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: PushInputData fail, exit");

        signal_->inIndexQueue_.pop();
        signal_->inFormatQueue_.pop();
        signal_->inAttrQueue_.pop();
    }
}

void VideoEncSample::InputLoopFunc()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoEnc_, nullptr);
    frameInputCount_ = 0;
    isFirstFrame_ = true;
    while (signal_->isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(
            lock, [this]() { return (signal_->inIndexQueue_.size() > 0) || (!signal_->isRunning_.load()); });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "InputLoopFunc stop running");

        int32_t ret = InputLoopInner();
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "PushInputData fail or eos, exit");

        signal_->inIndexQueue_.pop();
        signal_->inMemoryQueue_.pop();
    }
}

int32_t VideoEncSample::InputLoopInner()
{
    uint32_t index = signal_->inIndexQueue_.front();
    std::shared_ptr<AVMemoryMock> buffer = signal_->inMemoryQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: GetInputBuffer fail. index: %d",
                                      index);

    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};
    if (inFile_->eof()) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
    } else {
        int32_t stride = 0;
        auto format = GetInputDescription();
        char *dst = reinterpret_cast<char *>(buffer->GetAddr());
        format->GetIntValue(Media::Tag::VIDEO_STRIDE, stride);
        attr.size = stride * DEFAULT_HEIGHT_VENC * 3 / 2; // 3: nom, 2: denom
        for (int32_t i = 0; i < attr.size; i += stride) {
            (void)inFile_->read(dst + i, DEFAULT_WIDTH_VENC);
        }
    }

    if (attr.flags & AVCODEC_BUFFER_FLAG_EOS) {
        int32_t ret = PushInputData(index, attr);
        cout << "Input EOS Frame, frameCount = " << frameInputCount_ << endl;
        if (inFile_ != nullptr && inFile_->is_open()) {
            inFile_->close();
        }
        return ret;
    }
    if (isFirstFrame_) {
        attr.flags = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        isFirstFrame_ = false;
    } else {
        attr.flags = AVCODEC_BUFFER_FLAG_NONE;
    }
    return PushInputData(index, attr);
}

void VideoEncSample::OutputLoopFunc()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoEnc_, nullptr);
    if (needDump_) {
        outFile_ = std::make_unique<std::ofstream>();
        ASSERT_NE(outFile_, nullptr) << "Fatal: No memory";
        outFile_->open(outPath_, std::ios::out | std::ios::binary | std::ios::ate);
        ASSERT_TRUE(outFile_->is_open()) << "outFile_ can not find";
    }
    frameOutputCount_ = 0;
    while (signal_->isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(
            lock, [this]() { return (signal_->outIndexQueue_.size() > 0) || (!signal_->isRunning_.load()); });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "OutputLoopFunc stop running");

        int32_t ret = OutputLoopInner();
        EXPECT_EQ(ret, AV_ERR_OK) << "frameOutputCount_: " << frameOutputCount_ << "\n";
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: OutputLoopInner fail, exit");

        signal_->outIndexQueue_.pop();
        signal_->outAttrQueue_.pop();
        signal_->outMemoryQueue_.pop();
    }
    unique_lock<mutex> lock(signal_->mutex_);
    signal_->isRunning_.store(false);
    signal_->cond_.notify_all();
}

int32_t VideoEncSample::OutputLoopInner()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr || !needDump_, AV_ERR_INVALID_VAL,
                                      "can not dump output file");
    struct OH_AVCodecBufferAttr attr = signal_->outAttrQueue_.front();
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret = AV_ERR_OK;
    auto buffer = signal_->outMemoryQueue_.front();

    if (attr.flags == 0 || (attr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME)) {
        frameOutputCount_++;
    }

    if (needDump_ && attr.flags != AVCODEC_BUFFER_FLAG_EOS) {
        if (outFile_->is_open()) {
            UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL,
                                              "Fatal: GetOutputBuffer fail, exit");
            outFile_->write(reinterpret_cast<char *>(buffer->GetAddr()), attr.size);
        } else {
            UNITTEST_INFO_LOG("output data fail");
        }
    }
    ret = FreeOutputData(index);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: FreeOutputData fail index: %d", index);

    if (attr.flags & AVCODEC_BUFFER_FLAG_EOS) {
        if (needDump_ && outFile_ != nullptr && outFile_->is_open()) {
            outFile_->close();
        }
        cout << "Output EOS Frame, frameCount = " << frameOutputCount_ << endl;
        cout << "Get EOS Frame, output func exit" << endl;
        unique_lock<mutex> lock(signal_->mutex_);
        if (!needSleep_) {
            EXPECT_LE(frameOutputCount_, frameInputCount_);
        }
        signal_->isRunning_.store(false);
        signal_->cond_.notify_all();
        return AV_ERR_OK;
    }
    return AV_ERR_OK;
}

void VideoEncSample::OutputLoopFuncExt()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoEnc_, nullptr);
    if (needDump_) {
        outFile_ = std::make_unique<std::ofstream>();
        ASSERT_NE(outFile_, nullptr) << "Fatal: No memory";
        outFile_->open(outPath_, std::ios::out | std::ios::binary | std::ios::ate);
        ASSERT_TRUE(outFile_->is_open()) << "outFile_ can not find";
    }
    frameOutputCount_ = 0;
    while (signal_->isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(
            lock, [this]() { return (signal_->outIndexQueue_.size() > 0) || (!signal_->isRunning_.load()); });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "OutputLoopFunc stop running");
        int32_t ret = OutputLoopInnerExt();
        EXPECT_EQ(ret, AV_ERR_OK) << "frameOutputCount_: " << frameOutputCount_ << "\n";
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: OutputLoopInnerExt fail, exit");

        signal_->outIndexQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
    unique_lock<mutex> lock(signal_->mutex_);
    signal_->isRunning_.store(false);
    signal_->cond_.notify_all();
}

void VideoEncSample::CheckFormatKey(OH_AVCodecBufferAttr attr, std::shared_ptr<AVBufferMock> buffer)
{
    if (!(attr.flags & AVCODEC_BUFFER_FLAG_CODEC_DATA) && !(attr.flags & AVCODEC_BUFFER_FLAG_EOS)) {
        std::shared_ptr<FormatMock> format = buffer->GetParameter();
        int32_t qpAverage = 60;
        if (format->GetIntValue(Media::Tag::VIDEO_ENCODER_QP_AVERAGE, qpAverage)) {
            UNITTEST_INFO_LOG("qpAverage is:%d", qpAverage);
        }
        if (testParam_ == VCodecTestParam::HW_HEVC) {
            double mse = 1.0;
            if (format->GetDoubleValue(Media::Tag::VIDEO_ENCODER_MSE, mse)) {
                UNITTEST_INFO_LOG("mse is:%lf", mse);
            }
        }
        format->Destroy();
    }
}

int32_t VideoEncSample::OutputLoopInnerExt()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr || !needDump_, AV_ERR_INVALID_VAL,
                                      "can not dump output file");
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret = AV_ERR_OK;
    auto buffer = signal_->outBufferQueue_.front();

    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL,
                                      "Fatal: GetOutputBuffer fail, exit. index: %d", index);
    struct OH_AVCodecBufferAttr attr;
    (void)buffer->GetBufferAttr(attr);
    char *bufferAddr = reinterpret_cast<char *>(buffer->GetAddr());
    int32_t size = attr.size;
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL,
                                      "Fatal: GetOutputBufferAddr fail, exit, index: %d", index);
    if (attr.flags == 0 || (attr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME)) {
        frameOutputCount_++;
    }
    UpdateSHA(outFile_, bufferAddr, size, needCheckSHA_);

#ifdef HMOS_TEST
    CheckFormatKey(attr, buffer);
#endif

    if (attr.flags == AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        frameOutputCount_--;
    }
    if (needDump_ && attr.flags != AVCODEC_BUFFER_FLAG_EOS) {
        if (outFile_->is_open()) {
            outFile_->write(bufferAddr, size);
        } else {
            UNITTEST_INFO_LOG("output data fail");
        }
    }
    ret = FreeOutputBuffer(index);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: FreeOutputData fail. index: %d", index);

    if (attr.flags & AVCODEC_BUFFER_FLAG_EOS) {
        PerformEosFrameAndVerifiedSHA();
        return AV_ERR_OK;
    }
    return AV_ERR_OK;
}

void VideoEncSample::InputLoopFuncExt()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoEnc_, nullptr);
    frameInputCount_ = 0;
    isFirstFrame_ = true;
    while (signal_->isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(
            lock, [this]() { return (signal_->inBufferQueue_.size() > 0) || (!signal_->isRunning_.load()); });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "InputLoopFunc stop running");
        UNITTEST_CHECK_AND_BREAK_LOG(inFile_ != nullptr && inFile_->is_open(), "inFile_ is closed");

        int32_t ret = InputLoopInnerExt();
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "PushInputData fail or eos, exit");

        signal_->inBufferQueue_.pop();
        signal_->inIndexQueue_.pop();
    }
}

int32_t VideoEncSample::InputLoopInnerExt()
{
    uint32_t index = signal_->inIndexQueue_.front();
    std::shared_ptr<AVBufferMock> buffer = signal_->inBufferQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: GetInputBuffer fail. index: %d",
                                      index);

    if (isTemporalScalabilitySyncIdr_ && frameInputCount_ == REQUEST_I_FRAME_NUM) {
        std::shared_ptr<FormatMock> format = buffer->GetParameter();
        format->PutIntValue(Media::Tag::VIDEO_REQUEST_I_FRAME, REQUEST_I_FRAME);
        buffer->SetParameter(format);
        UNITTEST_INFO_LOG("request i frame: %s", format->DumpInfo());
        format->Destroy();
    }

    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};
    if (inFile_->eof()) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
    } else {
        int32_t stride = 0;
        auto format = GetInputDescription();
        char *dst = reinterpret_cast<char *>(buffer->GetAddr());
        format->GetIntValue(Media::Tag::VIDEO_STRIDE, stride);
        attr.size = stride * DEFAULT_HEIGHT_VENC * 3 / 2; // 3: nom, 2: denom
        for (int32_t i = 0; i < attr.size; i += stride) {
            (void)inFile_->read(dst + i, DEFAULT_WIDTH_VENC);
        }
    }

    if (attr.flags & AVCODEC_BUFFER_FLAG_EOS) {
        buffer->SetBufferAttr(attr);
        int32_t ret = PushInputBuffer(index);
        cout << "Input EOS Frame, frameCount = " << frameInputCount_ << endl;
        if (inFile_ != nullptr && inFile_->is_open()) {
            inFile_->close();
        }
        return ret;
    }
    if (isFirstFrame_) {
        attr.flags = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        isFirstFrame_ = false;
    } else {
        attr.flags = AVCODEC_BUFFER_FLAG_NONE;
    }
    buffer->SetBufferAttr(attr);
    return PushInputBuffer(index);
}

void VideoEncSample::InputFuncSurface()
{
    while (signal_->isRunning_.load()) {
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
        if (dst == nullptr || stride < static_cast<int32_t>(DEFAULT_WIDTH_VENC)) {
            cout << "invalid va or stride=" << stride << endl;
            err = NativeWindowCancelBuffer(nativeWindow_, ohNativeWindowBuffer);
            UNITTEST_CHECK_AND_INFO_LOG(err == 0, "NativeWindowCancelBuffer failed");
            signal_->isRunning_.store(false);
            break;
        }
        uint64_t bufferSize = stride * DEFAULT_HEIGHT_VENC * 3 / 2; // 3: nom, 2: denom
        for (int32_t i = 0; i < bufferSize; i += stride) {
            (void)inFile_->read(dst + i, DEFAULT_WIDTH_VENC);
        }
        if (inFile_->eof()) {
            if (needSleep_) {
                this_thread::sleep_for(std::chrono::milliseconds(46)); // 46 ms
            }
            frameInputCount_++;
            err = videoEnc_->NotifyEos();
            UNITTEST_CHECK_AND_INFO_LOG(err == 0, "OH_VideoEncoder_NotifyEndOfStream failed");
            break;
        }
        frameInputCount_++;
        err = InputProcess(nativeBuffer, ohNativeWindowBuffer);
        UNITTEST_CHECK_AND_BREAK_LOG(err == 0, "InputProcess failed, GSError=%d", err);
        usleep(16666); // 16666:60fps
    }
}

int32_t VideoEncSample::InputProcess(OH_NativeBuffer *nativeBuffer, OHNativeWindowBuffer *ohNativeWindowBuffer)
{
    using namespace chrono;
    int32_t ret = 0;
    struct Region region;
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0;
    rect->y = 0;
    rect->w = DEFAULT_WIDTH_VENC;
    rect->h = DEFAULT_HEIGHT_VENC;
    region.rects = rect;
    int64_t systemTimeUs = time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count();
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_UI_TIMESTAMP, systemTimeUs);
    ret = OH_NativeBuffer_Unmap(nativeBuffer);
    if (ret != 0) {
        cout << "OH_NativeBuffer_Unmap failed" << endl;
        delete rect;
        return ret;
    }

    if (needSleep_) {
        this_thread::sleep_for(std::chrono::milliseconds(30)); // 30 ms
    }

    ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow_, ohNativeWindowBuffer, -1, region);
    delete rect;
    if (ret != 0) {
        cout << "FlushBuffer failed" << endl;
        return ret;
    }
    return ret;
}

void VideoEncSample::CheckSHA()
{
    const uint8_t *sha = nullptr;
    switch (testParam_) {
        case VCodecTestParam::SW_AVC:
        case VCodecTestParam::HW_AVC:
            sha = SHA_AVC;
            break;
        case VCodecTestParam::HW_HEVC:
            sha = SHA_HEVC;
            break;
        default:
            return;
    }
    cout << std::hex << "========================================";
    for (uint32_t i = 0; i < SHA512_DIGEST_LENGTH; ++i) {
        if ((i % 8) == 0) { // 8: print width
            cout << "\n";
        }
        cout << "0x" << setw(2) << setfill('0') << static_cast<int32_t>(g_mdTest[i]) << ","; // 2: append print zero
        ASSERT_EQ(g_mdTest[i], sha[i]) << "i: " << i;
    }
    cout << std::dec << "\n========================================\n";
}

void VideoEncSample::PerformEosFrameAndVerifiedSHA()
{
    if (needDump_ && outFile_->is_open()) {
        outFile_->close();
    }
    if (needCheckSHA_) {
        (void)memset_s(g_mdTest, SHA512_DIGEST_LENGTH, 0, SHA512_DIGEST_LENGTH);
        SHA512_Final(g_mdTest, &g_ctxTest);
        OPENSSL_cleanse(&g_ctxTest, sizeof(g_ctxTest));
        CheckSHA();
    }
    cout << "Output EOS Frame, frameCount = " << frameOutputCount_ << endl;
    cout << "Get EOS Frame, output func exit" << endl;
    unique_lock<mutex> lock(signal_->mutex_);
    EXPECT_LE(frameOutputCount_, frameInputCount_);
    signal_->isRunning_.store(false);
    signal_->cond_.notify_all();
}
} // namespace MediaAVCodec
} // namespace OHOS
