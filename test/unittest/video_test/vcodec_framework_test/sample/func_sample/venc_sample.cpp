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

#ifdef VIDEOENC_CAPI_UNIT_TEST
#include "native_window.h"
#include "surface_capi_mock.h"
#else
#include "surface_inner_mock.h"
#endif

using namespace std;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
namespace {
constexpr bool NEED_DUMP = false;
constexpr int32_t REQUEST_I_FRAME = 1;
constexpr int32_t REQUEST_I_FRAME_NUM = 13;
}
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
    signal_->inFormatQueue_.push(parameter);
    signal_->inCond_.notify_all();
}

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

int32_t VideoEncSample::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->Configure(format);
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
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_FORMAT fail" << endl;
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_BUFFER_GEOMETRY, DEFAULT_WIDTH_VENC,
                                                DEFAULT_HEIGHT_VENC);
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail" << endl;
        return ret;
    }
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

void VideoEncSample::SetIsHdrVivid(bool isHdrVivid)
{
    isHdrVivid_ = isHdrVivid;
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
    if (isHdrVivid_ && isSurfaceMode_) {
        return DEFAULT_WIDTH_VENC * DEFAULT_HEIGHT_VENC * 3 / 2; // 3: nom, 2: denom
    }
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
        UNITTEST_INFO_LOG("parameter: %s", format->DumpInfo());

        format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
        format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
        format->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

        if (isTemporalScalabilitySyncIdr_ && frameInputCount_ == REQUEST_I_FRAME_NUM) {
            format->PutIntValue(Media::Tag::VIDEO_REQUEST_I_FRAME, REQUEST_I_FRAME);
            UNITTEST_INFO_LOG("request i frame: %s", format->DumpInfo());
        }

        int32_t ret = PushInputParameter(index);
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: PushInputData fail, exit");

        signal_->inIndexQueue_.pop();
        signal_->inFormatQueue_.pop();
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
    if (NEED_DUMP) {
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
        frameOutputCount_++;
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
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr || !NEED_DUMP, AV_ERR_INVALID_VAL,
                                      "can not dump output file");
    struct OH_AVCodecBufferAttr attr = signal_->outAttrQueue_.front();
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret = AV_ERR_OK;
    auto buffer = signal_->outMemoryQueue_.front();

    if (attr.flags == AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        frameOutputCount_--;
    }

    if (NEED_DUMP && attr.flags != AVCODEC_BUFFER_FLAG_EOS) {
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
        if (NEED_DUMP && outFile_ != nullptr && outFile_->is_open()) {
            outFile_->close();
        }
        cout << "Output EOS Frame, frameCount = " << frameOutputCount_ << endl;
        cout << "Get EOS Frame, output func exit" << endl;
        unique_lock<mutex> lock(signal_->mutex_);
        EXPECT_LE(frameOutputCount_, frameInputCount_);
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
    if (NEED_DUMP) {
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
        frameOutputCount_++;
        EXPECT_EQ(ret, AV_ERR_OK) << "frameOutputCount_: " << frameOutputCount_ << "\n";
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: OutputLoopInnerExt fail, exit");

        signal_->outIndexQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
    unique_lock<mutex> lock(signal_->mutex_);
    signal_->isRunning_.store(false);
    signal_->cond_.notify_all();
}

int32_t VideoEncSample::OutputLoopInnerExt()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr || !NEED_DUMP, AV_ERR_INVALID_VAL,
                                      "can not dump output file");
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret = AV_ERR_OK;
    auto buffer = signal_->outBufferQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL,
                                      "Fatal: GetOutputBuffer fail, exit. index: %d", index);

    struct OH_AVCodecBufferAttr attr;
    (void)buffer->GetBufferAttr(attr);

    if (attr.flags == AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        frameOutputCount_--;
    }

    if (NEED_DUMP && attr.flags != AVCODEC_BUFFER_FLAG_EOS) {
        if (outFile_->is_open()) {
            outFile_->write(reinterpret_cast<char *>(buffer->GetAddr()), attr.size);
        } else {
            UNITTEST_INFO_LOG("output data fail");
        }
    }
    ret = FreeOutputBuffer(index);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: FreeOutputData fail. index: %d", index);

    if (attr.flags & AVCODEC_BUFFER_FLAG_EOS) {
        if (NEED_DUMP && outFile_->is_open()) {
            outFile_->close();
        }
        cout << "Output EOS Frame, frameCount = " << frameOutputCount_ << endl;
        cout << "Get EOS Frame, output func exit" << endl;
        unique_lock<mutex> lock(signal_->mutex_);
        EXPECT_LE(frameOutputCount_, frameInputCount_);
        signal_->isRunning_.store(false);
        signal_->cond_.notify_all();
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
    NativeWindowHandleOpt(nativeWindow_, SET_UI_TIMESTAMP, systemTimeUs);
    ret = OH_NativeBuffer_Unmap(nativeBuffer);
    if (ret != 0) {
        cout << "OH_NativeBuffer_Unmap failed" << endl;
        delete rect;
        return ret;
    }

    ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow_, ohNativeWindowBuffer, -1, region);
    delete rect;
    if (ret != 0) {
        cout << "FlushBuffer failed" << endl;
        return ret;
    }
    return ret;
}
} // namespace MediaAVCodec
} // namespace OHOS
