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

#include "vdec_async_sample.h"
#include <gtest/gtest.h>
#include "openssl/crypto.h"
#include "openssl/sha.h"

using namespace std;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
using namespace OHOS::MediaAVCodec;

namespace OHOS {
namespace MediaAVCodec {
constexpr uint8_t FRAME_HEAD_LEN = 4;
constexpr uint8_t OFFSET_8 = 8;
constexpr uint8_t OFFSET_16 = 16;
constexpr uint8_t OFFSET_24 = 24;
constexpr uint8_t H264_NALU_TYPE_MASK = 0x1F;
constexpr uint8_t H264_SPS = 7;
constexpr uint8_t H264_PPS = 8;
constexpr uint8_t H265_NALU_TYPE_MASK = 0x7E;
constexpr uint8_t H265_VPS = 32;
constexpr uint8_t H265_SPS = 33;
constexpr uint8_t H265_PPS = 34;

VDecCallbackTest::VDecCallbackTest(std::shared_ptr<VDecSignal> signal) : signal_(signal) {}

VDecCallbackTest::~VDecCallbackTest() {}

void VDecCallbackTest::OnError(int32_t errorCode)
{
    cout << "VDec Error errorCode=" << errorCode;
    if (signal_ == nullptr) {
        return;
    }
    signal_->errorNum += 1;
    cout << ", errorNum=" << signal_->errorNum << endl;
}

void VDecCallbackTest::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    cout << "VDec Format Changed" << endl;
    cout << "info: " << format->DumpInfo() << endl;
}

void VDecCallbackTest::OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data)
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

void VDecCallbackTest::OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr)
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

VDecCallbackTestExt::VDecCallbackTestExt(std::shared_ptr<VDecSignal> signal) : signal_(signal) {}

VDecCallbackTestExt::~VDecCallbackTestExt() {}

void VDecCallbackTestExt::OnError(int32_t errorCode)
{
    cout << "VDec Error errorCode=" << errorCode;
    if (signal_ == nullptr) {
        return;
    }
    signal_->errorNum += 1;
    cout << ", errorNum=" << signal_->errorNum << endl;
}

void VDecCallbackTestExt::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    cout << "VDec Format Changed" << endl;
    cout << "info: " << format->DumpInfo() << endl;
    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_PIC_WIDTH, signal_->width_));
    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_PIC_HEIGHT, signal_->height_));
    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_STRIDE, signal_->wStride_));
    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_SLICE_HEIGHT, signal_->hStride_));
}

void VDecCallbackTestExt::OnNeedInputData(uint32_t index, std::shared_ptr<AVBufferMock> data)
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

void VDecCallbackTestExt::OnNewOutputData(uint32_t index, std::shared_ptr<AVBufferMock> data)
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

TestConsumerListener::TestConsumerListener(Surface *cs, std::string_view name, bool needCheckSHA)
{
    cs_ = cs;
    needCheckSHA_ = needCheckSHA;
    outFile_ = std::make_unique<std::ofstream>();
    outFile_->open(name.data(), std::ios::out | std::ios::binary);
}

TestConsumerListener::~TestConsumerListener()
{
    if (outFile_ != nullptr) {
        outFile_->close();
    }
}

void TestConsumerListener::OnBufferAvailable()
{
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;

    cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);

    if ((VideoDecAsyncSample::needDump_) && (outFile_ != nullptr) && (outFile_->is_open())) {
        (void)outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
    }
    cs_->ReleaseBuffer(buffer, -1);
}

VideoDecAsyncSample::VideoDecAsyncSample(std::shared_ptr<VDecSignal> signal) : signal_(signal) {}

VideoDecAsyncSample::~VideoDecAsyncSample()
{
    FlushInner();
    consumer_ = nullptr;
    producer_ = nullptr;
    if (videoDec_ != nullptr) {
        (void)videoDec_->Release();
    }
    if (inFile_ != nullptr && inFile_->is_open()) {
        inFile_->close();
    }
    if (outFile_ != nullptr && outFile_->is_open()) {
        outFile_->close();
    }
}

bool VideoDecAsyncSample::CreateVideoDecMockByMime(const std::string &mime)
{
    videoDec_ = VCodecMockFactory::CreateVideoDecMockByMime(mime);
    return videoDec_ != nullptr;
}

bool VideoDecAsyncSample::needDump_ = false;
bool VideoDecAsyncSample::CreateVideoDecMockByName(const std::string &name)
{
    videoDec_ = VCodecMockFactory::CreateVideoDecMockByName(name);
    return videoDec_ != nullptr;
}

int32_t VideoDecAsyncSample::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = videoDec_->SetCallback(cb);
    isAVBufferMode_ = ret != AV_ERR_OK;
    return ret;
}

int32_t VideoDecAsyncSample::SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = videoDec_->SetCallback(cb);
    isAVBufferMode_ = ret == AV_ERR_OK;
    return ret;
}

int32_t VideoDecAsyncSample::SetOutputSurface()
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }

    consumer_ = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener =
        new TestConsumerListener(consumer_.GetRefPtr(), outSurfacePath_, needCheckSHA_);
    consumer_->RegisterConsumerListener(listener);
    auto p = consumer_->GetProducer();
    producer_ = Surface::CreateSurfaceAsProducer(p);
    std::shared_ptr<SurfaceMock> surface = SurfaceMockFactory::CreateSurface(producer_);
    int32_t ret = videoDec_->SetOutputSurface(surface);
    isSurfaceMode_ = (ret == AV_ERR_OK);
    return ret;
}

int32_t VideoDecAsyncSample::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->Configure(format);
}

int32_t VideoDecAsyncSample::Prepare()
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->Prepare();
}

int32_t VideoDecAsyncSample::CreateReader(const std::string &inPath)
{
    int32_t dataProducerType = AVC_STREAM;
    for (const auto &[key, fileType] : fileTypeMap) {
        if (inPath.find(key) != std::string::npos) {
            dataProducerType = fileType;
            break;
        }
    }

    switch (dataProducerType) {
        case H263_STREAM:
            return CreateH263Reader();
        case AVC_STREAM:
        case HEVC_STREAM:
            return CreateAvccReader();
        case MPEG2_STREAM:
        case MPEG4_STREAM:
            return CreateMpegReader();
        default:
            return CreateAvccReader();
    }
}

int32_t VideoDecAsyncSample::Start()
{
    if (signal_ == nullptr || videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = CreateReader(inPath_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: CreateReader fail");
    PrepareInner();
    ret = videoDec_->Start();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: Start fail");
    if (isAVBufferMode_) {
        RunInnerExt();
    } else {
        RunInner();
    }
    WaitForEos();
    return ret;
}

int32_t VideoDecAsyncSample::Stop()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->Stop();
}

int32_t VideoDecAsyncSample::Flush()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->Flush();
}

int32_t VideoDecAsyncSample::Reset()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->Reset();
}

int32_t VideoDecAsyncSample::Release()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->Release();
}

std::shared_ptr<FormatMock> VideoDecAsyncSample::GetOutputDescription()
{
    if (videoDec_ == nullptr) {
        return nullptr;
    }
    std::shared_ptr<FormatMock> format = videoDec_->GetOutputDescription();
    if (format == nullptr) {
        return nullptr;
    }
    cout << "info: " << format->DumpInfo() << endl;
    format->GetIntValue(Media::Tag::VIDEO_PIC_WIDTH, signal_->width_);
    format->GetIntValue(Media::Tag::VIDEO_PIC_HEIGHT, signal_->height_);
    format->GetIntValue(Media::Tag::VIDEO_STRIDE, signal_->wStride_);
    format->GetIntValue(Media::Tag::VIDEO_SLICE_HEIGHT, signal_->hStride_);
    return format;
}

int32_t VideoDecAsyncSample::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->SetParameter(format);
}

int32_t VideoDecAsyncSample::PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    frameInputCount_++;
    return videoDec_->PushInputData(index, attr);
}

int32_t VideoDecAsyncSample::RenderOutputData(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->RenderOutputData(index);
}

int32_t VideoDecAsyncSample::FreeOutputData(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->FreeOutputData(index);
}

int32_t VideoDecAsyncSample::PushInputBuffer(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    frameInputCount_++;
    return videoDec_->PushInputBuffer(index);
}

int32_t VideoDecAsyncSample::RenderOutputBuffer(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->RenderOutputBuffer(index);
}

int32_t VideoDecAsyncSample::RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->RenderOutputBufferAtTime(index, renderTimestampNs);
}

int32_t VideoDecAsyncSample::FreeOutputBuffer(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->FreeOutputBuffer(index);
}

bool VideoDecAsyncSample::IsValid()
{
    if (videoDec_ == nullptr) {
        return false;
    }
    return videoDec_->IsValid();
}

void VideoDecAsyncSample::SetOutPath(const std::string &path)
{
    outPath_ = path + ".yuv";
    outSurfacePath_ = path + ".rgba";
}

void VideoDecAsyncSample::SetSource(const std::string &path)
{
    inPath_ = path;
}

void VideoDecAsyncSample::SetSourceType(bool isAvcStream)
{
    dataProducerType_ = isAvcStream;
}

int32_t VideoDecAsyncSample::CreateAvccReader()
{
    std::shared_ptr<AvccReaderInfo> info = std::make_shared<AvccReaderInfo>();
    info->inPath = inPath_;
    info->isAvcStream = (dataProducerType_ == AVC_STREAM);

    avccReader_ = std::make_shared<AvccReader>();
    int32_t ret = avccReader_->Init(info);
    return ret;
}

int32_t VideoDecAsyncSample::CreateMpegReader()
{
    std::shared_ptr<MpegReaderInfo> info = std::make_shared<MpegReaderInfo>();
    info->inPath = inPath_;
    info->isMpeg2Stream = (dataProducerType_ == MPEG2_STREAM);

    mpegReader_ = std::make_shared<MpegReader>();
    int32_t ret = mpegReader_->Init(info);
    return ret;
}

int32_t VideoDecAsyncSample::CreateH263Reader()
{
    std::shared_ptr<H263ReaderInfo> info = std::make_shared<H263ReaderInfo>();
    info->inPath = inPath_;

    h263Reader_ = std::make_shared<H263Reader>();
    int32_t ret = h263Reader_->Init(info);
    return ret;
}

void VideoDecAsyncSample::FlushInner()
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
        inFile_ = std::make_unique<std::ifstream>();
        ASSERT_NE(inFile_, nullptr);
        inFile_->open(inPath_, std::ios::in | std::ios::binary);
        ASSERT_TRUE(inFile_->is_open());
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

void VideoDecAsyncSample::RunInner()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isPreparing_.store(false);
    signal_->isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&VideoDecAsyncSample::InputLoopFunc, this);
    ASSERT_NE(inputLoop_, nullptr);
    signal_->inCond_.notify_all();

    outputLoop_ = make_unique<thread>(&VideoDecAsyncSample::OutputLoopFunc, this);
    ASSERT_NE(outputLoop_, nullptr);
    signal_->outCond_.notify_all();
}

void VideoDecAsyncSample::RunInnerExt()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isPreparing_.store(false);
    signal_->isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&VideoDecAsyncSample::InputLoopFuncExt, this);
    ASSERT_NE(inputLoop_, nullptr);
    signal_->inCond_.notify_all();

    outputLoop_ = make_unique<thread>(&VideoDecAsyncSample::OutputLoopFuncExt, this);
    ASSERT_NE(outputLoop_, nullptr);
    signal_->outCond_.notify_all();
}

void VideoDecAsyncSample::WaitForEos()
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

void VideoDecAsyncSample::PrepareInner()
{
    if (signal_ == nullptr) {
        return;
    }
    FlushInner();
    signal_->isPreparing_.store(true);
    signal_->isRunning_.store(false);
    if (inFile_ == nullptr) {
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

void VideoDecAsyncSample::InputLoopFunc()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoDec_, nullptr);
    frameInputCount_ = 0;
    while (signal_->isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(
            lock, [this]() { return (signal_->inIndexQueue_.size() > 0) || (!signal_->isRunning_.load()); });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "InputLoopFunc stop running");
        UNITTEST_CHECK_AND_BREAK_LOG(inFile_ != nullptr && inFile_->is_open() && !inFile_->eof(), "inFile is invalid");

        int32_t ret = InputLoopInner();
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "PushInputData fail or eos, exit");

        signal_->inIndexQueue_.pop();
        signal_->inMemoryQueue_.pop();
    }
}

bool VideoDecAsyncSample::IsCodecData(const uint8_t *const bufferAddr)
{
    bool isAvcStream = (dataProducerType_ == AVC_STREAM);
    uint8_t naluType = isAvcStream ? (bufferAddr[FRAME_HEAD_LEN] & H264_NALU_TYPE_MASK)
                                   : ((bufferAddr[FRAME_HEAD_LEN] & H265_NALU_TYPE_MASK) >> 1);
    if ((isAvcStream && ((naluType == H264_SPS) || (naluType == H264_PPS))) ||
        (!isAvcStream && ((naluType == H265_VPS) || (naluType == H265_SPS) || (naluType == H265_PPS)))) {
        return true;
    }
    return false;
}

int32_t VideoDecAsyncSample::ReadOneFrame(uint8_t *bufferAddr, uint32_t &flags)
{
    char ch[FRAME_HEAD_LEN] = {};
    (void)inFile_->read(ch, FRAME_HEAD_LEN);
    uint32_t bufferSize =
        static_cast<uint32_t>(((ch[3] & 0xFF)) | ((ch[2] & 0xFF) << OFFSET_8) | ((ch[1] & 0xFF) << OFFSET_16) |
                              ((ch[0] & 0xFF) << OFFSET_24)); // 0 1 2 3: avcc frame head offset

    (void)inFile_->read(reinterpret_cast<char *>(bufferAddr + FRAME_HEAD_LEN), bufferSize);
    bufferAddr[0] = 0;
    bufferAddr[1] = 0;
    bufferAddr[2] = 0; // 2: annexB frame head offset 2
    bufferAddr[3] = 1; // 3: annexB frame head offset 3

    if (IsCodecData(bufferAddr)) {
        flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
    }
    return bufferSize + FRAME_HEAD_LEN;
}

void VideoDecAsyncSample::CheckSHA()
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
        case VCodecTestParam::SW_H263:
            sha = SHA_H263;
            break;
        default:
            return;
    }
    cout << std::hex << "========================================";
    for (uint32_t i = 0; i < SHA512_DIGEST_LENGTH; ++i) {
        ASSERT_EQ(g_mdTest[i], sha[i]) << "i: " << i;
        if ((i % 8) == 0) { // 8: print width
            cout << "\n";
        }
        cout << "0x" << setw(2) << setfill('0') << static_cast<int32_t>(g_mdTest[i]) << ","; // 2: append print zero
    }
    cout << std::dec << "\n========================================\n";
}

void VideoDecAsyncSample::UpdateSHA(const char *addr, int32_t size)
{
    if (needCheckSHA_) {
        ++g_shaBufferCount;
    }
    const char *temp = addr;
    if (needCheckSHA_ && g_shaBufferCount < BUFFER_COUNT && size > 0) {
        for (int32_t i = 0; i < signal_->height_; ++i) {
            SHA512_Update(&g_ctxTest, temp, signal_->width_);
            temp += signal_->wStride_;
        }
        temp += (signal_->hStride_ - signal_->height_) * signal_->wStride_;
        for (int32_t i = 0; i < (signal_->height_ / 2); ++i) { // 2: denom
            SHA512_Update(&g_ctxTest, temp, signal_->width_);
            temp += signal_->wStride_;
        }
    }
    if ((addr != nullptr) && (outFile_ != nullptr) && (outFile_->is_open()) && (size > 0)) {
        (void)outFile_->write(addr, size);
    }
}

int32_t VideoDecAsyncSample::InputLoopInner()
{
    uint32_t index = signal_->inIndexQueue_.front();
    std::shared_ptr<AVMemoryMock> buffer = signal_->inMemoryQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->GetAddr() != nullptr, AV_ERR_INVALID_VAL,
                                      "Fatal: GetInputBuffer fail, index: %d", index);
    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};
    if (h263Reader_ != nullptr) {
        h263Reader_->FillBuffer(buffer->GetAddr(), attr);
    } else if (avccReader_ != nullptr) {
        isKeepExecuting_ == false ? avccReader_->FillBuffer(buffer->GetAddr(), attr)
                                  : avccReader_->KeepFillBuffer(buffer->GetAddr(), attr);
    } else {
        mpegReader_->FillBuffer(buffer->GetAddr(), attr);
    }
    return PushInputData(index, attr);
}

void VideoDecAsyncSample::OutputLoopFunc()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoDec_, nullptr);
    if (!isSurfaceMode_ && needDump_) {
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

int32_t VideoDecAsyncSample::OutputLoopInner()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr || !needDump_ || isSurfaceMode_, AV_ERR_INVALID_VAL,
                                      "can not dump output file");
    struct OH_AVCodecBufferAttr attr = signal_->outAttrQueue_.front();
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret = AV_ERR_OK;
    auto buffer = signal_->outMemoryQueue_.front();
    CheckFormatKey();
    if (!isSurfaceMode_ && attr.flags != AVCODEC_BUFFER_FLAG_EOS && needDump_) {
        if (!outFile_->is_open()) {
            cout << "output data fail" << endl;
        } else {
            UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL,
                                              "Fatal: GetOutputBuffer fail, exit. index: %d", index);
            outFile_->write(reinterpret_cast<char *>(buffer->GetAddr()), attr.size);
        }
    }
    if (!isSurfaceMode_) {
        ret = FreeOutputData(index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: FreeOutputData fail, index: %d", index);
    } else {
        ret = RenderOutputData(index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: RenderOutputData fail, index: %d", index);
    }
    if (attr.flags == AVCODEC_BUFFER_FLAG_EOS) {
        if (!isSurfaceMode_ && needDump_ && outFile_->is_open()) {
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

void VideoDecAsyncSample::OutputLoopFuncExt()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoDec_, nullptr);
    if (!isSurfaceMode_ && needDump_) {
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
        if (frameOutputCount_ == 0) {
            GetOutputDescription();
        }
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

void VideoDecAsyncSample::CheckFormatKey()
{
    std::shared_ptr<FormatMock> format = videoDec_->GetOutputDescription();
    int32_t pictureWidth = 0;
    int32_t pictureHeight = 0;
    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_PIC_WIDTH, pictureWidth));
    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_PIC_HEIGHT, pictureHeight));
    format->Destroy();
}

bool VideoDecAsyncSample::CompareMetadata(std::shared_ptr<std::ifstream> file, int32_t size,
    std::shared_ptr<SurfaceBufferMock> surfaceBufferMock, bool isDynamic)
{
    if (!file || !file->is_open()) {
        return true;
    }

    std::vector<uint8_t> metadataFromFile;
    metadataFromFile.resize(size);
    file->read(reinterpret_cast<char*>(metadataFromFile.data()), size);

    std::vector<uint8_t> metadataFromBuffer;
    if (isDynamic) {
        surfaceBufferMock->GetHDRDynamicMetadata(metadataFromBuffer);
    } else {
        surfaceBufferMock->GetHDRStaticMetadata(metadataFromBuffer);
    }
    // check meta
    if (metadataFromBuffer != metadataFromFile) {
        signal_->errorNum++;
        return false;
    }
    return true;
}

bool VideoDecAsyncSample::CompareHdrInfo(std::shared_ptr<AVBufferMock> buffer)
{
    if (isSurfaceMode_ || (testParam_ != VCodecTestParam::HW_HDR && testParam_ != VCodecTestParam::HW_HDR_HLG_FULL &&
        testParam_ != VCodecTestParam::HW_HDR10)) {
        return true;
    }
    int ret;
    if (frameOutputCount_ == 0) {
        if (g_hdrDynamicMeta.find(testParam_) != g_hdrDynamicMeta.end()) {
            dynamicMetadataFile_ = std::make_unique<std::ifstream>(g_hdrDynamicMeta.at(testParam_),
                std::ios::binary | std::ios::in);
        }
        if (g_hdrStaticMeta.find(testParam_) != g_hdrStaticMeta.end()) {
            staticMetadataFile_ = std::make_unique<std::ifstream>(g_hdrStaticMeta.at(testParam_),
                std::ios::binary | std::ios::in);
        }
        if (!dynamicMetadataFile_ && !staticMetadataFile_) {
            return true;
        }
    }

    // check meta
    std::shared_ptr<SurfaceBufferMock> surfaceBufferMock = SurfaceBufferMockFactory::CreateSurfaceBuffer(buffer);
    if (g_hdrDynamicMetaSize.find(testParam_) != g_hdrDynamicMetaSize.end()) {
        ret = CompareMetadata(dynamicMetadataFile_,
            g_hdrDynamicMetaSize.at(testParam_)[frameOutputCount_], surfaceBufferMock, true);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret, false, "Fatal: GetHDRDynamicMetadata fail");
    }
    if (g_hdrStaticMetaSize.find(testParam_) != g_hdrStaticMetaSize.end()) {
        ret = CompareMetadata(staticMetadataFile_,
            g_hdrStaticMetaSize.at(testParam_)[frameOutputCount_], surfaceBufferMock, false);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret, false, "Fatal: GetHDRStaticMetadata fail");
    }

    // check hdr type
    int hdrType;
    if (!surfaceBufferMock->GetHDRMetadataType(hdrType) || hdrType != g_hdrType.at(testParam_)) {
        signal_->errorNum++;
        printf("Fatal: GetHDRMetadataType fail\n");
        return false;
    }
    return true;
}

int32_t VideoDecAsyncSample::OutputLoopInnerExt()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr || !needDump_ || isSurfaceMode_, AV_ERR_INVALID_VAL,
                                      "can not dump output file");
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret;
    auto buffer = signal_->outBufferQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL,
                                      "Fatal: GetOutputBuffer fail, exit, index: %d", index);
    CheckFormatKey();
    struct OH_AVCodecBufferAttr attr;
    (void)buffer->GetBufferAttr(attr);
    if (!isSurfaceMode_ && attr.flags != AVCODEC_BUFFER_FLAG_EOS) {
        char *bufferAddr = reinterpret_cast<char *>(buffer->GetAddr());
        int32_t size = (testParam_ == VCodecTestParam::SW_AVC || testParam_ == VCodecTestParam::SW_MPEG2 ||
                        testParam_ == VCodecTestParam::SW_MPEG4 || testParam_ == VCodecTestParam::SW_H263)
                           ? attr.size
                           : buffer->GetNativeBuffer()->GetSize();
        UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL,
                                          "Fatal: GetOutputBuffer fail, exit, index: %d", index);
        UpdateSHA(bufferAddr, size);
        CompareHdrInfo(buffer);
        ret = FreeOutputBuffer(index);

        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: FreeOutputData fail index: %d", index);
    } else if (attr.flags != AVCODEC_BUFFER_FLAG_EOS) {
        int64_t renderTimestamp =
            chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        ret =
            (renderAtTimeFlag_ == true) ? RenderOutputBufferAtTime(index, renderTimestamp) : RenderOutputBuffer(index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: RenderOutputBuffer failed index: %d", index);
    }
    if (attr.flags == AVCODEC_BUFFER_FLAG_EOS) {
        if (!isSurfaceMode_ && needDump_ && outFile_->is_open()) {
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
    return AV_ERR_OK;
}

void VideoDecAsyncSample::InputLoopFuncExt()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoDec_, nullptr);
    frameInputCount_ = 0;
    while (signal_->isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(
            lock, [this]() { return (signal_->inBufferQueue_.size() > 0) || (!signal_->isRunning_.load()); });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "InputLoopFunc stop running");
        UNITTEST_CHECK_AND_BREAK_LOG(inFile_ != nullptr && inFile_->is_open() && !inFile_->eof(), "inFile is invalid");

        int32_t ret = InputLoopInnerExt();
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "PushInputData fail or eos, exit");

        signal_->inBufferQueue_.pop();
        signal_->inIndexQueue_.pop();
    }
}

int32_t VideoDecAsyncSample::InputLoopInnerExt()
{
    uint32_t index = signal_->inIndexQueue_.front();
    std::shared_ptr<AVBufferMock> buffer = signal_->inBufferQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->GetAddr() != nullptr, AV_ERR_INVALID_VAL,
                                      "Fatal: GetInputBuffer fail, index: %d", index);
    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};
    if (h263Reader_ != nullptr) {
        h263Reader_->FillBuffer(buffer->GetAddr(), attr);
    } else if (avccReader_ != nullptr) {
        isKeepExecuting_ == false ? avccReader_->FillBuffer(buffer->GetAddr(), attr)
                                  : avccReader_->KeepFillBuffer(buffer->GetAddr(), attr);
    } else {
        mpegReader_->FillBuffer(buffer->GetAddr(), attr);
    }
    buffer->SetBufferAttr(attr);
    return PushInputBuffer(index);
}

int32_t VideoDecAsyncSample::SetVideoDecryptionConfig()
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->SetVideoDecryptionConfig();
}
} // namespace MediaAVCodec
} // namespace OHOS