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

#include "venc_sync_sample.h"
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

namespace OHOS {
namespace MediaAVCodec {
constexpr uint32_t DEFAULT_INDEX = -1;
VEncCallbackTest::VEncCallbackTest(std::shared_ptr<VEncSignal> signal) {}

VEncCallbackTest::~VEncCallbackTest() {}

void VEncCallbackTest::OnError(int32_t errorCode) {}

void VEncCallbackTest::OnStreamChanged(std::shared_ptr<FormatMock> format) {}

void VEncCallbackTest::OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data) {}

void VEncCallbackTest::OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr) {}

VEncCallbackTestExt::VEncCallbackTestExt(std::shared_ptr<VEncSignal> signal) : signal_(signal) {}

VEncCallbackTestExt::~VEncCallbackTestExt() {}

void VEncCallbackTestExt::OnError(int32_t errorCode) {}

void VEncCallbackTestExt::OnStreamChanged(std::shared_ptr<FormatMock> format) {}

void VEncCallbackTestExt::OnNeedInputData(uint32_t index, std::shared_ptr<AVBufferMock> data) {}

void VEncCallbackTestExt::OnNewOutputData(uint32_t index, std::shared_ptr<AVBufferMock> data) {}

VEncParamCallbackTest::VEncParamCallbackTest(std::shared_ptr<VEncSignal> signal) {}

VEncParamCallbackTest::~VEncParamCallbackTest() {}

void VEncParamCallbackTest::OnInputParameterAvailable(uint32_t index, std::shared_ptr<FormatMock> parameter) {}

VEncParamWithAttrCallbackTest::VEncParamWithAttrCallbackTest(std::shared_ptr<VEncSignal> signal) : signal_(signal) {}

VEncParamWithAttrCallbackTest::~VEncParamWithAttrCallbackTest() {}

void VEncParamWithAttrCallbackTest::OnInputParameterWithAttrAvailable(uint32_t index,
                                                                      std::shared_ptr<FormatMock> attribute,
                                                                      std::shared_ptr<FormatMock> parameter)
{
}

bool VideoEncSyncSample::needDump_ = false;
VideoEncSyncSample::VideoEncSyncSample(std::shared_ptr<VEncSignal> signal)
    : signal_(signal), inPath_("/data/test/media/1280_720_nv.yuv"), nativeWindow_(nullptr)
{
}

VideoEncSyncSample::~VideoEncSyncSample()
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
    if (nativeWindow_ != nullptr) {
#ifdef VIDEOENC_CAPI_UNIT_TEST
        nativeWindow_->DecStrongRef(nativeWindow_);
#else
        DestoryNativeWindow(nativeWindow_);
#endif
        nativeWindow_ = nullptr;
    }
}

bool VideoEncSyncSample::CreateVideoEncMockByMime(const std::string &mime)
{
    videoEnc_ = VCodecMockFactory::CreateVideoEncMockByMime(mime);
    return videoEnc_ != nullptr;
}

bool VideoEncSyncSample::CreateVideoEncMockByName(const std::string &name)
{
    videoEnc_ = VCodecMockFactory::CreateVideoEncMockByName(name);
    return videoEnc_ != nullptr;
}

int32_t VideoEncSyncSample::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VideoEncSyncSample::SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VideoEncSyncSample::SetCallback(std::shared_ptr<MediaCodecParameterCallbackMock> cb)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VideoEncSyncSample::SetCallback(std::shared_ptr<MediaCodecParameterWithAttrCallbackMock> cb)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VideoEncSyncSample::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    format->PutIntValue(Media::Tag::AV_CODEC_ENABLE_SYNC_MODE, 1);
    return videoEnc_->Configure(format);
}

int32_t VideoEncSyncSample::Prepare()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->Prepare();
}

int32_t VideoEncSyncSample::SetCustomBuffer(std::shared_ptr<AVBufferMock> buffer)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->SetCustomBuffer(buffer);
}

int32_t VideoEncSyncSample::Start()
{
    if (signal_ == nullptr || videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    PrepareInner();
    int32_t ret = videoEnc_->Start();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: Start fail");
    RunInnerExt();
    WaitForEos();
    return ret;
}

int32_t VideoEncSyncSample::Stop()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    lock_guard<shared_mutex> lock(signal_->syncMutex_);
    return videoEnc_->Stop();
}

int32_t VideoEncSyncSample::Flush()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    lock_guard<shared_mutex> lock(signal_->syncMutex_);
    return videoEnc_->Flush();
}

int32_t VideoEncSyncSample::Reset()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    lock_guard<shared_mutex> lock(signal_->syncMutex_);
    return videoEnc_->Reset();
}

int32_t VideoEncSyncSample::Release()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    lock_guard<shared_mutex> lock(signal_->syncMutex_);
    return videoEnc_->Release();
}

std::shared_ptr<FormatMock> VideoEncSyncSample::GetOutputDescription()
{
    if (videoEnc_ == nullptr) {
        return nullptr;
    }
    return videoEnc_->GetOutputDescription();
}

std::shared_ptr<FormatMock> VideoEncSyncSample::GetInputDescription()
{
    if (videoEnc_ == nullptr) {
        return nullptr;
    }
    return videoEnc_->GetInputDescription();
}

int32_t VideoEncSyncSample::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->SetParameter(format);
}

int32_t VideoEncSyncSample::NotifyEos()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->NotifyEos();
}

int32_t VideoEncSyncSample::PushInputBuffer(uint32_t index)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    frameInputCount_++;
    return videoEnc_->PushInputBuffer(index);
}

int32_t VideoEncSyncSample::FreeOutputBuffer(uint32_t index)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoEnc_->FreeOutputBuffer(index);
}

#ifdef VIDEOENC_CAPI_UNIT_TEST
int32_t VideoEncSyncSample::CreateInputSurface()
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
int32_t VideoEncSyncSample::CreateInputSurface()
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

bool VideoEncSyncSample::IsValid()
{
    if (videoEnc_ == nullptr) {
        return false;
    }
    return videoEnc_->IsValid();
}

void VideoEncSyncSample::SetOutPath(const std::string &path)
{
    outPath_ = path + ".dat";
}

void VideoEncSyncSample::FlushInner()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
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
        outputLoop_->join();
    }
}

int32_t VideoEncSyncSample::ReadOneFrame()
{
    return DEFAULT_WIDTH_VENC * DEFAULT_HEIGHT_VENC * 3 / 2; // 3: nom, 2: denom
}

void VideoEncSyncSample::RunInnerExt()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isPreparing_.store(false);
    signal_->isRunning_.store(true);
    if (isSurfaceMode_) {
        inputSurfaceLoop_ = make_unique<thread>(&VideoEncSyncSample::InputFuncSurface, this);
    } else {
        inputLoop_ = make_unique<thread>(&VideoEncSyncSample::InputLoopFuncExt, this);
        ASSERT_NE(inputLoop_, nullptr);
    }

    outputLoop_ = make_unique<thread>(&VideoEncSyncSample::OutputLoopFuncExt, this);
    ASSERT_NE(outputLoop_, nullptr);
}

void VideoEncSyncSample::WaitForEos()
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

void VideoEncSyncSample::PrepareInner()
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

void VideoEncSyncSample::InputLtrParam(OH_AVCodecBufferAttr &attr, int32_t frameInputCount,
                                       std::shared_ptr<AVBufferMock> buffer)
{
    int32_t stride = 0;
    auto format = GetInputDescription();
    char *dst = reinterpret_cast<char *>(buffer->GetAddr());
    format->GetIntValue(Media::Tag::VIDEO_STRIDE, stride);
    attr.size = stride * DEFAULT_HEIGHT_VENC * 3 / 2; // 3: nom, 2: denom
    for (int32_t i = 0; i < attr.size; i += stride) {
        (void)inFile_->read(dst + i, DEFAULT_WIDTH_VENC);
    }

    if (!ltrParam.enableUseLtr) {
        return;
    }
    int32_t interval = ltrParam.ltrInterval;
    static int32_t useLtrIndex = 0;
    if (frameInputCount % interval == 0) {
        format->PutIntValue(Media::Tag::VIDEO_ENCODER_PER_FRAME_MARK_LTR, 1);
    }
    if (interval > 0 && (frameInputCount % interval == 0)) {
        useLtrIndex = frameInputCount;
    }
    if (frameInputCount > useLtrIndex) {
        format->PutIntValue(Media::Tag::VIDEO_ENCODER_PER_FRAME_USE_LTR, useLtrIndex);
    } else if (frameInputCount == useLtrIndex && frameInputCount > 0) {
        format->PutIntValue(Media::Tag::VIDEO_ENCODER_PER_FRAME_USE_LTR, frameInputCount - interval);
    }
    if (buffer) {
        buffer->SetParameter(format);
    }
}

void VideoEncSyncSample::OutputLoopFuncExt()
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
        shared_lock<shared_mutex> lock(signal_->syncMutex_);
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "OutputLoopFuncExt stop running");
        int32_t ret = OutputLoopInnerExt();
        if (ret == AV_ERR_STREAM_CHANGED || ret == AV_ERR_TRY_AGAIN_LATER) {
            ret = AV_ERR_OK;
        }
        EXPECT_EQ(ret, AV_ERR_OK) << "frameOutputCount_: " << frameOutputCount_ << "\n";
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: OutputLoopInnerExt fail, exit");
    }
    unique_lock<mutex> lock(signal_->mutex_);
    signal_->isRunning_.store(false);
    signal_->cond_.notify_all();
}

void VideoEncSyncSample::CheckFormatKey(OH_AVCodecBufferAttr attr, std::shared_ptr<AVBufferMock> buffer)
{
    std::shared_ptr<FormatMock> format = buffer->GetParameter();
    if (!(attr.flags & AVCODEC_BUFFER_FLAG_CODEC_DATA) && !(attr.flags & AVCODEC_BUFFER_FLAG_EOS)) {
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
    }
    if (ltrParam.enableUseLtr && (attr.flags == AVCODEC_BUFFER_FLAG_NONE)) {
        int32_t isLtr = 0;
        int32_t framePoc = 0;
        if (format->GetIntValue(Media::Tag::VIDEO_PER_FRAME_IS_LTR, isLtr)) {
            UNITTEST_INFO_LOG("isLtr is:%d", isLtr);
        }
        if (format->GetIntValue(Media::Tag::VIDEO_PER_FRAME_POC, framePoc)) {
            UNITTEST_INFO_LOG("framePoc is:%d", framePoc);
        }
    }
    format->Destroy();
}

int32_t VideoEncSyncSample::OutputLoopInnerExt()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr || !needDump_, AV_ERR_INVALID_VAL,
                                      "can not dump output file");
    uint32_t index = DEFAULT_INDEX;
    uint32_t ret = videoEnc_->QueryOutputBuffer(index, -1);
    if (ret == AV_ERR_STREAM_CHANGED) {
        std::shared_ptr<FormatMock> format = videoEnc_->GetOutputDescription();
        std::cout << "format = " << format->DumpInfo() << std::endl;
    }
    if (ret != AV_ERR_OK) {
        return ret;
    }

    auto buffer = videoEnc_->GetOutputBuffer(index);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL,
                                      "Fatal: GetOutputBuffer fail, exit, index: %d", index);

    struct OH_AVCodecBufferAttr attr;
    (void)buffer->GetBufferAttr(attr);
    char *bufferAddr = reinterpret_cast<char *>(buffer->GetAddr());
    int32_t size = attr.size;
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL,
                                      "Fatal: GetOutputBufferAddr fail, exit, index: %d", index);
    if (attr.flags == 0 || (attr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME)) {
        frameOutputCount_++;
    }
    UpdateSHA(outFile_, bufferAddr, size, needCheckSHA_, needDump_);

#ifdef ONLY_FOR_FLAGSHIP_CHIP
    CheckFormatKey(attr, buffer);
#endif // ONLY_FOR_FLAGSHIP_CHIP

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

void VideoEncSyncSample::InputLoopFuncExt()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoEnc_, nullptr);
    frameInputCount_ = 0;
    isFirstFrame_ = true;
    while (signal_->isRunning_.load()) {
        shared_lock<shared_mutex> lock(signal_->syncMutex_);
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "InputLoopFuncExt stop running");
        UNITTEST_CHECK_AND_BREAK_LOG(inFile_ != nullptr && inFile_->is_open(), "inFile_ is closed");

        int32_t ret = InputLoopInnerExt();
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "PushInputData fail or eos, exit");
    }
}

void VideoEncSyncSample::InputLoopInnerFeatureExt(OH_AVCodecBufferAttr &attr)
{
    if (enableVariableFrameRate_) {
        attr.pts = TIMESTAMP_BASE + DURATION_BASE * frameIndex_;
        frameIndex_++;
    }
}

void VideoEncSyncSample::InputRoiParam(std::shared_ptr<AVBufferMock> buffer)
{
    std::shared_ptr<FormatMock> format = buffer->GetParameter();
    format->PutStringValue(Media::Tag::VIDEO_ENCODER_ROI_PARAMS, roiRects_.c_str());
    buffer->SetParameter(format);
    format->Destroy();
}

int32_t VideoEncSyncSample::InputLoopInnerExt()
{
    uint32_t index = DEFAULT_INDEX;
    auto ret = videoEnc_->QueryInputBuffer(index, -1);
    if (ret == AV_ERR_TRY_AGAIN_LATER) {
        return AV_ERR_OK;
    }

    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: QueryInputBuffer fail");
    auto buffer = videoEnc_->GetInputBuffer(index);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->GetAddr() != nullptr, AV_ERR_INVALID_VAL,
                                      "Fatal: GetInputBuffer fail, index: %d", index);

    if (isTemporalScalabilitySyncIdr_ && frameInputCount_ == REQUEST_I_FRAME_NUM) {
        std::shared_ptr<FormatMock> format = buffer->GetParameter();
        format->PutIntValue(Media::Tag::VIDEO_REQUEST_I_FRAME, REQUEST_I_FRAME);
        buffer->SetParameter(format);
        UNITTEST_INFO_LOG("request i frame: %s", format->DumpInfo());
        format->Destroy();
    }

    InputRoiParam(buffer);

    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};
    if (inFile_->eof()) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
    } else {
        InputLtrParam(attr, frameInputCount_, buffer);
    }

    if (attr.flags & AVCODEC_BUFFER_FLAG_EOS) {
        buffer->SetBufferAttr(attr);
        int32_t res = PushInputBuffer(index);
        cout << "Input EOS Frame, frameCount = " << frameInputCount_ << endl;
        if (inFile_ != nullptr && inFile_->is_open()) {
            inFile_->close();
        }
        return res;
    }
    if (isFirstFrame_) {
        attr.flags = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        isFirstFrame_ = false;
    } else {
        attr.flags = AVCODEC_BUFFER_FLAG_NONE;
    }
    InputLoopInnerFeatureExt(attr);
    buffer->SetBufferAttr(attr);
    return PushInputBuffer(index);
}

void VideoEncSyncSample::InputFuncSurface()
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

int32_t VideoEncSyncSample::InputProcess(OH_NativeBuffer *nativeBuffer, OHNativeWindowBuffer *ohNativeWindowBuffer)
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
    if (enableVariableFrameRate_) {
        systemTimeUs = (TIMESTAMP_BASE + DURATION_BASE * frameIndex_) * RATIO_US_TO_NS;
        frameIndex_++;
    }
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

void VideoEncSyncSample::CheckSHA()
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

void VideoEncSyncSample::PerformEosFrameAndVerifiedSHA()
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
    if (!needSleep_) {
        EXPECT_LE(frameOutputCount_, frameInputCount_);
    }
    signal_->isRunning_.store(false);
    signal_->cond_.notify_all();
}
} // namespace MediaAVCodec
} // namespace OHOS