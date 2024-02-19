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
#include "common/native_mfmagic.h"
#include "native_avcapability.h"
#include "native_avmagic.h"
#include "surface/window.h"

#define TEST_ID sampleId_
#include "unittest_log.h"
#define TITLE_LOG UNITTEST_INFO_LOG("")

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoDecSample"};
} // namespace
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

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
constexpr uint32_t DEFAULT_WIDTH = 720;
constexpr uint32_t DEFAULT_HEIGHT = 1280;
constexpr uint32_t DEFAULT_TIME_INTERVAL = 4166;
constexpr uint32_t MAX_OUTPUT_FRMAENUM = 60;
constexpr size_t MAX_HEAPNUM = 512;
constexpr uint64_t SAMPLE_TIMEOUT = 1000000;

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
TestConsumerListener::TestConsumerListener(Surface *cs, unique_ptr<ofstream> &&outFile, int32_t id)
{
    sampleId_ = id;
    TITLE_LOG;
    cs_ = cs;
    outFile_ = move(outFile);
    frameOutputCount_ = 0;
}

TestConsumerListener::~TestConsumerListener()
{
    TITLE_LOG;
    if (outFile_ != nullptr && outFile_->is_open()) {
        outFile_->close();
    }
}

void TestConsumerListener::OnBufferAvailable()
{
    TITLE_LOG;
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;

    cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);

    if (outFile_ != nullptr && outFile_->is_open() && frameOutputCount_ < MAX_OUTPUT_FRMAENUM) {
        (void)outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
    }
    cs_->ReleaseBuffer(buffer, -1);
    frameOutputCount_++;
}

VideoDecSample::VideoDecSample()
{
    static atomic<int32_t> sampleId = 0;
    sampleId_ = ++sampleId;
    TITLE_LOG;
    dyFormat_ = OH_AVFormat_Create();
}

VideoDecSample::~VideoDecSample()
{
    TITLE_LOG;
    if (codec_ != nullptr) {
        int32_t ret = OH_VideoDecoder_Destroy(codec_);
        UNITTEST_CHECK_AND_INFO_LOG(ret == AV_ERR_OK, "OH_VideoDecoder_Destroy failed");
    }
    if (dyFormat_ != nullptr) {
        OH_AVFormat_Destroy(dyFormat_);
    }
    if (inFile_ != nullptr && inFile_->is_open()) {
        inFile_->close();
    }
    if (outFile_ != nullptr && outFile_->is_open()) {
        outFile_->close();
    }
    if (nativeWindow_ != nullptr) {
        DestoryNativeWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }
}

bool VideoDecSample::Create()
{
    TITLE_LOG;

    isH264Stream_ = inPath_.substr(inPath_.length() - 4, 4) == "h264"; // 4: "h264" string len
    inPath_ = "/data/test/media/" + inPath_;
    outPath_ = "/data/test/media/" + outPath_;
    inFile_ = make_unique<ifstream>();
    inFile_->open(inPath_, ios::in | ios::binary);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inFile_ != nullptr, false, "create inFile_ failed");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inFile_->is_open(), false, "can not open inFile_");
    if (needDump_) {
        outFile_ = make_unique<ofstream>();
        outFile_->open(outPath_, ios::out | ios::binary);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr, false, "create outFile_ failed");
    }

    OH_AVCodecCategory category = isHardware_ ? HARDWARE : SOFTWARE;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(mime_.c_str(), false, category);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(capability != nullptr, false, "OH_AVCodec_GetCapabilityByCategory failed");

    const char *name = OH_AVCapability_GetName(capability);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(name != nullptr, false, "OH_AVCapability_GetName failed");

    codec_ = OH_VideoDecoder_CreateByName(name);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(name != nullptr, false, "OH_VideoDecoder_CreateByName failed");
    return true;
}

int32_t VideoDecSample::SetCallback(OH_AVCodecAsyncCallback callback, shared_ptr<VideoDecSignal> &signal)
{
    TITLE_LOG;
    signal_ = signal;
    int32_t ret = OH_VideoDecoder_SetCallback(codec_, callback, reinterpret_cast<void *>(signal_.get()));
    isAVBufferMode_ = ret != AV_ERR_OK;
    return ret;
}

int32_t VideoDecSample::RegisterCallback(OH_AVCodecCallback callback, shared_ptr<VideoDecSignal> &signal)
{
    TITLE_LOG;
    signal_ = signal;
    int32_t ret = OH_VideoDecoder_RegisterCallback(codec_, callback, reinterpret_cast<void *>(signal_.get()));
    isAVBufferMode_ = ret == AV_ERR_OK;
    return ret;
}

int32_t VideoDecSample::SetOutputSurface()
{
    TITLE_LOG;
    consumer_ = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(consumer_.GetRefPtr(), move(outFile_), sampleId_);
    outFile_ = nullptr;
    consumer_->RegisterConsumerListener(listener);

    auto p = consumer_->GetProducer();
    producer_ = Surface::CreateSurfaceAsProducer(p);

    nativeWindow_ = CreateNativeWindowFromSurface(&producer_);
    int32_t ret = OH_VideoDecoder_SetSurface(codec_, nativeWindow_);
    isSurfaceMode_ = (ret == AV_ERR_OK);
    return ret;
}

int32_t VideoDecSample::Configure()
{
    TITLE_LOG;
    OH_AVFormat *format = OH_AVFormat_Create();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_UNKNOWN, "create format failed");
    bool setFormatRet =
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH) &&
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT) &&
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(setFormatRet, AV_ERR_UNKNOWN, "set format failed");

    int32_t ret = OH_VideoDecoder_Configure(codec_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VideoDecSample::Start()
{
    TITLE_LOG;
    using namespace chrono;

    time_ = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    return OH_VideoDecoder_Start(codec_);
}

bool VideoDecSample::WaitForEos()
{
    TITLE_LOG;
    using namespace chrono;

    unique_lock<mutex> lock(signal_->eosMutex_);
    auto lck = [this]() { return signal_->isEos_.load(); };
    bool isNotTimeout = signal_->eosCond_.wait_for(lock, seconds(SAMPLE_TIMEOUT), lck);
    lock.unlock();
    int64_t tempTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    EXPECT_LE(frameOutputCount_, frameInputCount_);
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

int32_t VideoDecSample::Prepare()
{
    TITLE_LOG;
    return OH_VideoDecoder_Prepare(codec_);
}

int32_t VideoDecSample::Stop()
{
    TITLE_LOG;
    signal_->isFlushing_ = true;
    signal_->inCond_.notify_all();
    signal_->outCond_.notify_all();
    {
        scoped_lock lock(signal_->inMutex_, signal_->outMutex_);
        FlushInQueue();
        FlushOutQueue();
    }
    int32_t ret = OH_VideoDecoder_Stop(codec_);
    signal_->isFlushing_ = false;
    signal_->inCond_.notify_all();
    signal_->outCond_.notify_all();
    return ret;
}

int32_t VideoDecSample::Flush()
{
    TITLE_LOG;
    signal_->isFlushing_ = true;
    signal_->inCond_.notify_all();
    signal_->outCond_.notify_all();
    {
        scoped_lock lock(signal_->inMutex_, signal_->outMutex_);
        FlushInQueue();
        FlushOutQueue();
    }
    int32_t ret = OH_VideoDecoder_Flush(codec_);
    signal_->isFlushing_ = false;
    signal_->inCond_.notify_all();
    signal_->outCond_.notify_all();
    return ret;
}

int32_t VideoDecSample::Reset()
{
    TITLE_LOG;
    signal_->isFlushing_ = true;
    signal_->inCond_.notify_all();
    signal_->outCond_.notify_all();
    {
        scoped_lock lock(signal_->inMutex_, signal_->outMutex_);
        FlushInQueue();
        FlushOutQueue();
    }
    int32_t ret = OH_VideoDecoder_Reset(codec_);
    signal_->isFlushing_ = false;
    signal_->inCond_.notify_all();
    signal_->outCond_.notify_all();
    return ret;
}

int32_t VideoDecSample::Release()
{
    TITLE_LOG;
    int32_t ret = OH_VideoDecoder_Destroy(codec_);
    codec_ = nullptr;
    return ret;
}

OH_AVFormat *VideoDecSample::GetOutputDescription()
{
    TITLE_LOG;
    auto avformat = OH_VideoDecoder_GetOutputDescription(codec_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(avformat != nullptr, nullptr, "OH_VideoDecoder_GetOutputDescription failed");
    return avformat;
}

int32_t VideoDecSample::SetParameter()
{
    TITLE_LOG;
    return OH_VideoDecoder_SetParameter(codec_, dyFormat_);
}

int32_t VideoDecSample::PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr)
{
    UNITTEST_INFO_LOG("index:%d", index);
    int32_t ret = AV_ERR_OK;
    if (isAVBufferMode_) {
        ret = OH_VideoDecoder_PushInputBuffer(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_PushInputBuffer failed");
    } else {
        ret = OH_VideoDecoder_PushInputData(codec_, index, attr);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_PushInputData failed");
    }
    frameInputCount_++;
    usleep(DEFAULT_TIME_INTERVAL);
    return AV_ERR_OK;
}

int32_t VideoDecSample::PushInputData(uint32_t index)
{
    UNITTEST_INFO_LOG("index:%d", index);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(isAVBufferMode_, AV_ERR_UNKNOWN, "PushInputData is not AVBufferMode");
    int32_t ret = OH_VideoDecoder_PushInputBuffer(codec_, index);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_PushInputBuffer failed");
    frameInputCount_++;
    usleep(DEFAULT_TIME_INTERVAL);
    return AV_ERR_OK;
}

int32_t VideoDecSample::RenderOutputData(uint32_t index)
{
    UNITTEST_INFO_LOG("index:%d", index);
    int32_t ret = AV_ERR_OK;
    if (isAVBufferMode_) {
        ret = OH_VideoDecoder_RenderOutputBuffer(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_RenderOutputBuffer failed");
    } else {
        ret = OH_VideoDecoder_RenderOutputData(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_RenderOutputData failed");
    }
    frameOutputCount_++;
    return AV_ERR_OK;
}

int32_t VideoDecSample::FreeOutputData(uint32_t index)
{
    UNITTEST_INFO_LOG("index:%d", index);
    int32_t ret = AV_ERR_OK;
    if (isAVBufferMode_) {
        ret = OH_VideoDecoder_FreeOutputBuffer(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_FreeOutputBuffer failed");
    } else {
        ret = OH_VideoDecoder_FreeOutputData(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_FreeOutputData failed");
    }
    frameOutputCount_++;
    return AV_ERR_OK;
}

int32_t VideoDecSample::IsValid(bool &isValid)
{
    TITLE_LOG;
    return OH_VideoDecoder_IsValid(codec_, &isValid);
}

void VideoDecSample::FlushInQueue()
{
    queue<uint32_t> tempIndex;
    swap(tempIndex, signal_->inQueue_);
    queue<OH_AVMemory *> tempInMemory;
    swap(tempInMemory, signal_->inMemoryQueue_);
    queue<OH_AVBuffer *> tempInBuffer;
    swap(tempInBuffer, signal_->inBufferQueue_);
    (void)inFile_->seekg(0);
}

void VideoDecSample::FlushOutQueue()
{
    queue<uint32_t> tempIndex;
    swap(tempIndex, signal_->outQueue_);
    queue<OH_AVCodecBufferAttr> tempOutAttr;
    swap(tempOutAttr, signal_->outAttrQueue_);
    queue<OH_AVMemory *> tempOutMemory;
    swap(tempOutMemory, signal_->outMemoryQueue_);
    queue<OH_AVBuffer *> tempOutBuffer;
    swap(tempOutBuffer, signal_->outBufferQueue_);
}

bool VideoDecSample::IsCodecData(const uint8_t *const addr)
{
    uint8_t naluType = isH264Stream_ ? (addr[FRAME_HEAD_LEN] & H264_NALU_TYPE_MASK)
                                     : ((addr[FRAME_HEAD_LEN] & H265_NALU_TYPE_MASK) >> 1);
    if ((isH264Stream_ && ((naluType == H264_SPS) || (naluType == H264_PPS))) ||
        (!isH264Stream_ && ((naluType == H265_VPS) || (naluType == H265_SPS) || (naluType == H265_PPS)))) {
        return true;
    }
    return false;
}

int32_t VideoDecSample::SetAVBufferAttr(OH_AVBuffer *avBuffer, OH_AVCodecBufferAttr &attr)
{
    if (!isAVBufferMode_) {
        return AV_ERR_OK;
    }
    int32_t ret = OH_AVBuffer_SetBufferAttr(avBuffer, &attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_AVBuffer_SetBufferAttr failed");
    return ret;
}

int32_t VideoDecSample::HandleInputFrame(uint32_t &index, OH_AVCodecBufferAttr &attr)
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
    signal_->inQueue_.pop();
    signal_->inMemoryQueue_.pop();
    signal_->inBufferQueue_.pop();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(addr != nullptr, AV_ERR_UNKNOWN, "in buffer is nullptr, index: %d", index);
    attr.offset = 0;
    attr.pts = GetTimeUs();
    if (frameCount_ <= frameInputCount_) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        attr.size = 0;
        return SetAVBufferAttr(avBuffer, attr);
    }

    if (inFile_->eof()) {
        (void)inFile_->seekg(0);
    }
    char head[FRAME_HEAD_LEN] = {};
    (void)inFile_->read(head, FRAME_HEAD_LEN);
    uint32_t bufferSize =
        static_cast<uint32_t>(((head[3] & 0xFF)) | ((head[2] & 0xFF) << OFFSET_8) | ((head[1] & 0xFF) << OFFSET_16) |
                              ((head[0] & 0xFF) << OFFSET_24)); // 0 1 2 3: avcc frame head offset

    (void)inFile_->read(reinterpret_cast<char *>(addr + FRAME_HEAD_LEN), bufferSize);
    addr[0] = 0;
    addr[1] = 0;
    addr[2] = 0; // 2: annexB frame head offset 2
    addr[3] = 1; // 3: annexB frame head offset 3

    attr.flags = IsCodecData(addr) ? AVCODEC_BUFFER_FLAGS_CODEC_DATA : AVCODEC_BUFFER_FLAG_NONE;
    attr.size = bufferSize + FRAME_HEAD_LEN;
    uint64_t *addr64 = reinterpret_cast<uint64_t *>(addr);
    UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d, addr[0]:%" PRIu64, attr.size, (int32_t)(attr.flags), addr64[0]);
    return SetAVBufferAttr(avBuffer, attr);
}

int32_t VideoDecSample::HandleOutputFrame(uint32_t &index, OH_AVCodecBufferAttr &attr)
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
    signal_->outQueue_.pop();
    signal_->outAttrQueue_.pop();
    signal_->outMemoryQueue_.pop();
    signal_->outBufferQueue_.pop();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_AVBuffer_GetBufferAttr failed, index: %d", index);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(addr != nullptr || isSurfaceMode_, AV_ERR_UNKNOWN,
                                      "out buffer is nullptr, index: %d", index);

    if (attr.flags == AVCODEC_BUFFER_FLAG_EOS) {
        UNITTEST_INFO_LOG("out frame:%d, in frame:%d", frameOutputCount_.load(), frameInputCount_.load());
        signal_->isEos_ = true;
        signal_->eosCond_.notify_all();
    }
    if (needDump_ && !isSurfaceMode_ && frameOutputCount_ < MAX_OUTPUT_FRMAENUM) {
        (void)outFile_->write(reinterpret_cast<char *>(addr), attr.size);
    }
    return AV_ERR_OK;
}

int32_t VideoDecSample::HandleInputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(addr != nullptr, AV_ERR_UNKNOWN, "in buffer is nullptr");
    attr.offset = 0;
    attr.pts = GetTimeUs();
    if (frameCount_ <= frameInputCount_) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        attr.size = 0;
        return AV_ERR_OK;
    }

    if (inFile_->eof()) {
        (void)inFile_->seekg(0);
    }
    char head[FRAME_HEAD_LEN] = {};
    (void)inFile_->read(head, FRAME_HEAD_LEN);
    uint32_t bufferSize =
        static_cast<uint32_t>(((head[3] & 0xFF)) | ((head[2] & 0xFF) << OFFSET_8) | ((head[1] & 0xFF) << OFFSET_16) |
                              ((head[0] & 0xFF) << OFFSET_24)); // 0 1 2 3: avcc frame head offset

    (void)inFile_->read(reinterpret_cast<char *>(addr + FRAME_HEAD_LEN), bufferSize);
    addr[0] = 0;
    addr[1] = 0;
    addr[2] = 0; // 2: annexB frame head offset 2
    addr[3] = 1; // 3: annexB frame head offset 3

    attr.flags = IsCodecData(addr) ? AVCODEC_BUFFER_FLAGS_CODEC_DATA : AVCODEC_BUFFER_FLAG_NONE;
    attr.size = bufferSize + FRAME_HEAD_LEN;
    uint64_t *addr64 = reinterpret_cast<uint64_t *>(addr);
    UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d, addr[0]:%" PRIu64, attr.size, (int32_t)(attr.flags), addr64[0]);
    return AV_ERR_OK;
}

int32_t VideoDecSample::HandleOutputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(addr != nullptr || isSurfaceMode_, AV_ERR_UNKNOWN, "out buffer is nullptr");

    if (attr.flags == AVCODEC_BUFFER_FLAG_EOS) {
        UNITTEST_INFO_LOG("out frame:%d, in frame:%d", frameOutputCount_.load(), frameInputCount_.load());
        signal_->isEos_ = true;
        signal_->eosCond_.notify_all();
    }
    if (needDump_ && !isSurfaceMode_ && frameOutputCount_ < MAX_OUTPUT_FRMAENUM) {
        (void)outFile_->write(reinterpret_cast<char *>(addr), attr.size);
    }
    return AV_ERR_OK;
}

int32_t VideoDecSample::HandleInputFrame(OH_AVMemory *data, OH_AVCodecBufferAttr &attr)
{
    uint8_t *addr = OH_AVMemory_GetAddr(data);
    return HandleInputFrameInner(addr, attr);
}

int32_t VideoDecSample::HandleOutputFrame(OH_AVMemory *data, OH_AVCodecBufferAttr &attr)
{
    uint8_t *addr = OH_AVMemory_GetAddr(data);
    return HandleOutputFrameInner(addr, attr);
}

int32_t VideoDecSample::HandleInputFrame(OH_AVBuffer *data)
{
    uint8_t *addr = OH_AVBuffer_GetAddr(data);
    OH_AVCodecBufferAttr attr;
    int32_t ret = HandleInputFrameInner(addr, attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "HandleInputFrameInner failed");
    ret = OH_AVBuffer_SetBufferAttr(data, &attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_AVBuffer_SetBufferAttr failed");
    return AV_ERR_OK;
}

int32_t VideoDecSample::HandleOutputFrame(OH_AVBuffer *data)
{
    uint8_t *addr = OH_AVBuffer_GetAddr(data);
    OH_AVCodecBufferAttr attr;
    int32_t ret = OH_AVBuffer_GetBufferAttr(data, &attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_AVBuffer_GetBufferAttr failed");
    return HandleOutputFrameInner(addr, attr);
}

HeapMemoryThread::HeapMemoryThread()
{
    isStopLoop_ = false;
    heapMemoryLoop_ = make_unique<thread>(&HeapMemoryThread::HeapMemoryLoop, this);

    std::string name = "heap_memory_thread";
    pthread_setname_np(heapMemoryLoop_->native_handle(), name.substr(0, 15).c_str()); // 15: max thread name
}

HeapMemoryThread::~HeapMemoryThread()
{
    isStopLoop_ = true;
    if (heapMemoryLoop_ != nullptr && heapMemoryLoop_->joinable()) {
        heapMemoryLoop_->join();
    }
}

void HeapMemoryThread::HeapMemoryLoop()
{
    queue<uint8_t *> memoryList;
    while (!isStopLoop_) {
        uint8_t *memory = new uint8_t[sizeof(OH_AVMemory)];
        uint8_t *buffer = new uint8_t[sizeof(OH_AVBuffer)];
        while (memoryList.size() >= MAX_HEAPNUM) {
            uint8_t *memoryFront = memoryList.front();
            delete memoryFront;
            memoryList.pop();
        }
        memoryList.push(memory);
        memoryList.push(buffer);
    }
    while (!memoryList.empty()) {
        uint8_t *memoryFront = memoryList.front();
        delete memoryFront;
        memoryList.pop();
    }
}
} // namespace MediaAVCodec
} // namespace OHOS
