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

#include "vdec_sample.h"
#include <gtest/gtest.h>
#include "openssl/crypto.h"
#include "openssl/sha.h"

using namespace std;
using namespace OHOS::MediaAVCodec::VCodecTestParam;

namespace {
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
constexpr uint32_t BUFFER_COUNT = 59;
constexpr bool NEED_DUMP = false;
constexpr uint8_t SHA_AVC[SHA512_DIGEST_LENGTH] = {
    0x3d, 0xc4, 0x3f, 0x67, 0x74, 0x18, 0xc6, 0xfb, 0xf3, 0x03, 0x56, 0x52, 0xf8, 0xa9, 0xf2, 0x7f,
    0x54, 0xdb, 0xfc, 0x69, 0x82, 0xeb, 0x30, 0x34, 0x62, 0x2f, 0x87, 0x92, 0xcc, 0x31, 0xa2, 0xd3,
    0x79, 0xa8, 0xc8, 0xc1, 0xae, 0x2e, 0x93, 0x58, 0x5f, 0x65, 0xf7, 0xab, 0x64, 0x32, 0xb3, 0x40,
    0xf3, 0x3b, 0x01, 0x1a, 0x75, 0xfa, 0x0e, 0x57, 0xde, 0x48, 0x40, 0xc7, 0x92, 0x7d, 0x14, 0xe8};
constexpr uint8_t SHA_HEVC[SHA512_DIGEST_LENGTH] = {
    0x09, 0x48, 0x29, 0x0f, 0xe3, 0x2a, 0xe6, 0x27, 0x33, 0xb1, 0x02, 0x84, 0x57, 0xbd, 0x8a, 0x4d,
    0xd1, 0xab, 0x3b, 0xa4, 0x1a, 0x33, 0xdd, 0x53, 0x3a, 0x0f, 0x16, 0x82, 0xea, 0xa6, 0x32, 0x6b,
    0xef, 0x2f, 0x67, 0xaa, 0x70, 0xd6, 0xae, 0xd9, 0xbe, 0x87, 0x1b, 0x4e, 0xb6, 0x4b, 0x66, 0x6e,
    0xaa, 0xbb, 0x15, 0x24, 0xc1, 0xb0, 0x17, 0xd2, 0x47, 0xf0, 0x19, 0x27, 0xbd, 0xfb, 0xfa, 0x9f};

uint8_t g_mdTest[SHA512_DIGEST_LENGTH];
std::atomic<uint32_t> g_shaBufferCount = 0;
SHA512_CTX g_ctxTest;

static inline int64_t GetTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    // 1000'000: second to micro second; 1000: nano second to micro second
    return (static_cast<int64_t>(now.tv_sec) * 1000'000 + (now.tv_nsec / 1000));
}

void UpdateSHA(std::unique_ptr<std::ofstream> &outFile, const char *addr, int32_t size, bool needCheckSHA)
{
    if (needCheckSHA) {
        ++g_shaBufferCount;
    }
    const int32_t frameSize = DEFAULT_WIDTH * DEFAULT_HEIGHT * 3 / 2; // 3: nom, 2: denom
    const int32_t bufferWidth = size * DEFAULT_WIDTH / frameSize;
    for (int32_t i = 0; i < size; i += bufferWidth) {
        if (needCheckSHA && g_shaBufferCount < BUFFER_COUNT) {
            SHA512_Update(&g_ctxTest, addr + i, DEFAULT_WIDTH);
        }
        if (NEED_DUMP) {
            if (!outFile->is_open()) {
                cout << "output data fail" << endl;
                continue;
            }
            (void)outFile->write(addr + i, DEFAULT_WIDTH);
        }
    }
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
VDecCallbackTest::VDecCallbackTest(std::shared_ptr<VDecSignal> signal) : signal_(signal) {}

VDecCallbackTest::~VDecCallbackTest() {}

void VDecCallbackTest::OnError(int32_t errorCode)
{
    cout << "VDec Error errorCode=" << errorCode;
    if (signal_ == nullptr) {
        return;
    }
    signal_->errorNum_ += 1;
    cout << ", errorNum=" << signal_->errorNum_ << endl;
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
    signal_->errorNum_ += 1;
    cout << ", errorNum=" << signal_->errorNum_ << endl;
}

void VDecCallbackTestExt::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    cout << "VDec Format Changed" << endl;
    cout << "info: " << format->DumpInfo() << endl;
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

    if (NEED_DUMP) {
        (void)outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
    }
    cs_->ReleaseBuffer(buffer, -1);
}

VideoDecSample::VideoDecSample(std::shared_ptr<VDecSignal> signal) : signal_(signal) {}

VideoDecSample::~VideoDecSample()
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

bool VideoDecSample::CreateVideoDecMockByMime(const std::string &mime)
{
    videoDec_ = VCodecMockFactory::CreateVideoDecMockByMime(mime);
    return videoDec_ != nullptr;
}

bool VideoDecSample::CreateVideoDecMockByName(const std::string &name)
{
    videoDec_ = VCodecMockFactory::CreateVideoDecMockByName(name);
    return videoDec_ != nullptr;
}

int32_t VideoDecSample::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = videoDec_->SetCallback(cb);
    isAVBufferMode_ = ret != AV_ERR_OK;
    return ret;
}

int32_t VideoDecSample::SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = videoDec_->SetCallback(cb);
    isAVBufferMode_ = ret == AV_ERR_OK;
    return ret;
}

int32_t VideoDecSample::SetOutputSurface()
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

int32_t VideoDecSample::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->Configure(format);
}

int32_t VideoDecSample::Start()
{
    if (signal_ == nullptr || videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    PrepareInner();
    int32_t ret = videoDec_->Start();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: Start fail");
    if (isAVBufferMode_) {
        RunInnerExt();
    } else {
        RunInner();
    }
    WaitForEos();
    return ret;
}

int32_t VideoDecSample::Stop()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->Stop();
}

int32_t VideoDecSample::Flush()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->Flush();
}

int32_t VideoDecSample::Reset()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->Reset();
}

int32_t VideoDecSample::Release()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->Release();
}

std::shared_ptr<FormatMock> VideoDecSample::GetOutputDescription()
{
    if (videoDec_ == nullptr) {
        return nullptr;
    }
    return videoDec_->GetOutputDescription();
}

int32_t VideoDecSample::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->SetParameter(format);
}

int32_t VideoDecSample::PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    frameInputCount_++;
    return videoDec_->PushInputData(index, attr);
}

int32_t VideoDecSample::RenderOutputData(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->RenderOutputData(index);
}

int32_t VideoDecSample::FreeOutputData(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->FreeOutputData(index);
}

int32_t VideoDecSample::PushInputBuffer(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    frameInputCount_++;
    return videoDec_->PushInputBuffer(index);
}

int32_t VideoDecSample::RenderOutputBuffer(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->RenderOutputBuffer(index);
}

int32_t VideoDecSample::FreeOutputBuffer(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return videoDec_->FreeOutputBuffer(index);
}

bool VideoDecSample::IsValid()
{
    if (videoDec_ == nullptr) {
        return false;
    }
    return videoDec_->IsValid();
}

void VideoDecSample::SetOutPath(const std::string &path)
{
    outPath_ = path + ".yuv";
    outSurfacePath_ = path + ".rgba";
}

void VideoDecSample::SetSource(const std::string &path)
{
    inPath_ = path;
}

void VideoDecSample::SetSourceType(bool isH264Stream)
{
    isH264Stream_ = isH264Stream;
}

void VideoDecSample::FlushInner()
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

void VideoDecSample::RunInner()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isPreparing_.store(false);
    signal_->isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&VideoDecSample::InputLoopFunc, this);
    ASSERT_NE(inputLoop_, nullptr);
    signal_->inCond_.notify_all();

    outputLoop_ = make_unique<thread>(&VideoDecSample::OutputLoopFunc, this);
    ASSERT_NE(outputLoop_, nullptr);
    signal_->outCond_.notify_all();
}

void VideoDecSample::RunInnerExt()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isPreparing_.store(false);
    signal_->isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&VideoDecSample::InputLoopFuncExt, this);
    ASSERT_NE(inputLoop_, nullptr);
    signal_->inCond_.notify_all();

    outputLoop_ = make_unique<thread>(&VideoDecSample::OutputLoopFuncExt, this);
    ASSERT_NE(outputLoop_, nullptr);
    signal_->outCond_.notify_all();
}

void VideoDecSample::WaitForEos()
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

void VideoDecSample::PrepareInner()
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

void VideoDecSample::InputLoopFunc()
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

bool VideoDecSample::IsCodecData(const uint8_t *const bufferAddr)
{
    uint8_t naluType = isH264Stream_ ? (bufferAddr[FRAME_HEAD_LEN] & H264_NALU_TYPE_MASK)
                                     : ((bufferAddr[FRAME_HEAD_LEN] & H265_NALU_TYPE_MASK) >> 1);
    if ((isH264Stream_ && ((naluType == H264_SPS) || (naluType == H264_PPS))) ||
        (!isH264Stream_ && ((naluType == H265_VPS) || (naluType == H265_SPS) || (naluType == H265_PPS)))) {
        return true;
    }
    return false;
}

int32_t VideoDecSample::ReadOneFrame(uint8_t *bufferAddr, uint32_t &flags)
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

void VideoDecSample::CheckSHA()
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
        ASSERT_EQ(g_mdTest[i], sha[i]) << "i: " << i;
        if ((i % 8) == 0) { // 8: print width
            cout << "\n";
        }
        cout << "0x" << setw(2) << setfill('0') << static_cast<int32_t>(g_mdTest[i]) << ","; // 2: append print zero
    }
    cout << std::dec << "\n========================================\n";
}

int32_t VideoDecSample::InputLoopInner()
{
    uint32_t index = signal_->inIndexQueue_.front();
    std::shared_ptr<AVMemoryMock> buffer = signal_->inMemoryQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: GetInputBuffer fail, index: %d",
                                      index);
    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};

    auto bufferSize = ReadOneFrame(buffer->GetAddr(), attr.flags);
    if (inFile_->eof()) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        cout << "Input EOS Frame, frameCount = " << (frameInputCount_ + 1) << endl;
    }

    attr.size = bufferSize;
    attr.pts = GetTimeUs();
    return PushInputData(index, attr);
}

void VideoDecSample::OutputLoopFunc()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoDec_, nullptr);
    if (!isSurfaceMode_ && NEED_DUMP) {
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

int32_t VideoDecSample::OutputLoopInner()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr || !NEED_DUMP || isSurfaceMode_, AV_ERR_INVALID_VAL,
                                      "can not dump output file");
    struct OH_AVCodecBufferAttr attr = signal_->outAttrQueue_.front();
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret = AV_ERR_OK;
    auto buffer = signal_->outMemoryQueue_.front();
    if (!isSurfaceMode_ && attr.flags != AVCODEC_BUFFER_FLAG_EOS && NEED_DUMP) {
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
        if (!isSurfaceMode_ && NEED_DUMP && outFile_->is_open()) {
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

void VideoDecSample::OutputLoopFuncExt()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoDec_, nullptr);
    if (!isSurfaceMode_ && NEED_DUMP) {
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

int32_t VideoDecSample::OutputLoopInnerExt()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr || !NEED_DUMP || isSurfaceMode_, AV_ERR_INVALID_VAL,
                                      "can not dump output file");
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret;
    auto buffer = signal_->outBufferQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL,
                                      "Fatal: GetOutputBuffer fail, exit, index: %d", index);
    struct OH_AVCodecBufferAttr attr;
    (void)buffer->GetBufferAttr(attr);
    if (!isSurfaceMode_ && attr.flags != AVCODEC_BUFFER_FLAG_EOS) {
        char *bufferAddr = reinterpret_cast<char *>(buffer->GetAddr());
        int32_t size = (testParam_ == VCodecTestParam::SW_AVC) ? attr.size : buffer->GetNativeBuffer()->GetSize();
        UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL,
                                          "Fatal: GetOutputBuffer fail, exit, index: %d", index);
        UpdateSHA(outFile_, bufferAddr, size, needCheckSHA_);
        ret = FreeOutputBuffer(index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: FreeOutputData fail index: %d", index);
    } else if (attr.flags != AVCODEC_BUFFER_FLAG_EOS) {
        ret = RenderOutputBuffer(index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: RenderOutputData fail index: %d", index);
    }
    if (attr.flags == AVCODEC_BUFFER_FLAG_EOS) {
        if (!isSurfaceMode_ && NEED_DUMP && outFile_->is_open()) {
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

void VideoDecSample::InputLoopFuncExt()
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

int32_t VideoDecSample::InputLoopInnerExt()
{
    uint32_t index = signal_->inIndexQueue_.front();
    std::shared_ptr<AVBufferMock> buffer = signal_->inBufferQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: GetInputBuffer fail, index: %d",
                                      index);
    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};

    auto bufferSize = ReadOneFrame(buffer->GetAddr(), attr.flags);
    if (inFile_->eof()) {
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        cout << "Input EOS Frame, frameCount = " << (frameInputCount_ + 1) << endl;
    }

    attr.size = bufferSize;
    attr.pts = GetTimeUs();
    buffer->SetBufferAttr(attr);
    return PushInputBuffer(index);
}
} // namespace MediaAVCodec
} // namespace OHOS
