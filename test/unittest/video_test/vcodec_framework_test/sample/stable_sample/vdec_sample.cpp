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

#include "vdec_sample.h"
#include <gtest/gtest.h>
#include "common/native_mfmagic.h"
#include "native_avcapability.h"
#include "native_avmagic.h"
#include "surface/window.h"

#define PRINT_HILOG
#define TEST_ID sampleId_
#include "unittest_log.h"
#define TITLE_LOG UNITTEST_INFO_LOG("")

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoDecSample"};
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
class TestConsumerListener : public IBufferConsumerListener {
public:
    static sptr<IBufferConsumerListener> GetTestConsumerListener(Surface *cs, sptr<IBufferConsumerListener> &listener)
    {
        TestConsumerListener *testListener = reinterpret_cast<TestConsumerListener *>(listener.GetRefPtr());
        return new TestConsumerListener(cs, testListener->signal_, testListener->sampleId_,
                                        testListener->frameOutputCount_);
    }

    TestConsumerListener(Surface *cs, std::shared_ptr<VideoDecSignal> &signal, int32_t id,
                         std::shared_ptr<std::atomic<uint32_t>> outputCount = nullptr)
    {
        sampleId_ = id;
        TITLE_LOG;
        cs_ = cs;
        signal_ = signal;
        frameOutputCount_ = (outputCount == nullptr) ? std::make_shared<std::atomic<uint32_t>>(0) : outputCount;
    }

    void OnBufferAvailable() override
    {
        UNITTEST_INFO_LOG("surfaceId:%" PRIu64, cs_->GetUniqueId());
        sptr<SurfaceBuffer> buffer;
        int32_t flushFence;

        cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);

        if (signal_->outFile_ != nullptr && signal_->outFile_->is_open() &&
            (*frameOutputCount_) < MAX_OUTPUT_FRMAENUM) {
            int32_t width = buffer->GetWidth();
            int32_t height = buffer->GetHeight();
            int32_t stride = buffer->GetStride();
            for (int32_t i = 0; i < height * 3 / 2; ++i) { // 3: nom, 2: denom
                (void)signal_->outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()) + i * stride, width);
            }
        }
        cs_->ReleaseBuffer(buffer, -1);
        (*frameOutputCount_)++;
    }

private:
    Rect damage_ = {};
    Surface *cs_ = nullptr;
    int32_t sampleId_ = 0;
    int64_t timestamp_ = 0;
    std::shared_ptr<VideoDecSignal> signal_ = nullptr;
    std::shared_ptr<std::atomic<uint32_t>> frameOutputCount_ = 0;
};

class VideoDecSample::SurfaceObject {
public:
    SurfaceObject(std::shared_ptr<VideoDecSignal> &signal, int32_t id) : sampleId_(id), signal_(signal) {}

    ~SurfaceObject()
    {
        while (!queue_.empty()) {
            DestoryNativeWindow(queue_.front().nativeWindow_);
            queue_.pop();
        }
    }

    OHNativeWindow *GetNativeWindow(const bool isNew = false)
    {
        TITLE_LOG;
        if (isNew || queue_.empty()) {
            CreateNativeWindow();
        }
        return queue_.back().nativeWindow_;
    }

    void CreateNativeWindow()
    {
        TITLE_LOG;
        WindowObject obj;
        if (queue_.size() >= OFFSET_8) {
            obj = queue_.front();
            queue_.push(obj);
            queue_.pop();
            return;
        }
        obj.consumer_ = Surface::CreateSurfaceAsConsumer();
        if (queue_.empty()) {
            obj.listener_ = new TestConsumerListener(obj.consumer_.GetRefPtr(), signal_, sampleId_);
        } else {
            obj.listener_ =
                TestConsumerListener::GetTestConsumerListener(obj.consumer_.GetRefPtr(), queue_.back().listener_);
        }
        obj.consumer_->RegisterConsumerListener(obj.listener_);

        auto p = obj.consumer_->GetProducer();
        obj.producer_ = Surface::CreateSurfaceAsProducer(p);

        obj.nativeWindow_ = CreateNativeWindowFromSurface(&obj.producer_);
        queue_.push(std::move(obj));
    }

private:
    int32_t sampleId_;
    std::shared_ptr<VideoDecSignal> signal_ = nullptr;

    typedef struct WindowObject {
        OHNativeWindow *nativeWindow_ = nullptr;
        sptr<IBufferConsumerListener> listener_ = nullptr;
        sptr<Surface> consumer_ = nullptr;
        sptr<Surface> producer_ = nullptr;
    } WindowObject;
    std::queue<WindowObject> queue_;
};
} // namespace MediaAVCodec
} // namespace OHOS

namespace OHOS {
namespace MediaAVCodec {
bool VideoDecSample::needDump_ = false;
bool VideoDecSample::isHardware_ = true;
uint64_t VideoDecSample::sampleTimout_ = 180;
uint64_t VideoDecSample::threadNum_ = 1;

VideoDecSample::VideoDecSample()
{
    static atomic<int32_t> sampleId = 0;
    sampleId_ = ++sampleId;
    TITLE_LOG;
    dyFormat_ = std::shared_ptr<OH_AVFormat>(OH_AVFormat_Create(), [](OH_AVFormat *ptr) { OH_AVFormat_Destroy(ptr); });
}

VideoDecSample::~VideoDecSample()
{
    TITLE_LOG;
    if (codec_ != nullptr) {
        int32_t ret = OH_VideoDecoder_Destroy(codec_);
        UNITTEST_CHECK_AND_INFO_LOG(ret == AV_ERR_OK, "OH_VideoDecoder_Destroy failed");
    }
}

bool VideoDecSample::Create()
{
    TITLE_LOG;

    isH264Stream_ = inPath_.find("h264") != std::string::npos;
    inPath_ = "/data/test/media/" + inPath_;
    outPath_ = "/data/test/media/" + outPath_ + to_string(sampleId_ % threadNum_) + ".yuv";

    OH_AVCodecCategory category = isHardware_ ? HARDWARE : SOFTWARE;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(mime_.c_str(), false, category);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(capability != nullptr, false, "OH_AVCodec_GetCapabilityByCategory failed");

    const char *name = OH_AVCapability_GetName(capability);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(name != nullptr, false, "OH_AVCapability_GetName failed");

    codec_ = OH_VideoDecoder_CreateByName(name);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, false, "OH_VideoDecoder_CreateByName failed");
    return true;
}

bool VideoDecSample::CreateByMime()
{
    TITLE_LOG;

    isH264Stream_ = inPath_.substr(inPath_.length() - 4, 4) == "h264"; // 4: "h264" string len
    inPath_ = "/data/test/media/" + inPath_;
    outPath_ = "/data/test/media/" + outPath_ + to_string(sampleId_ % threadNum_) + ".yuv";

    codec_ = OH_VideoDecoder_CreateByMime(mime_.c_str());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, false, "OH_VideoDecoder_CreateByMime failed");
    return true;
}

bool VideoDecSample::InitFile()
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

int32_t VideoDecSample::SetCallback(OH_AVCodecAsyncCallback callback, shared_ptr<VideoDecSignal> &signal)
{
    TITLE_LOG;
    signal_ = signal;
    if (!InitFile()) {
        return AV_ERR_UNKNOWN;
    }
    asyncCallback_ = callback;
    int32_t ret = OH_VideoDecoder_SetCallback(codec_, callback, reinterpret_cast<void *>(signal_.get()));
    isAVBufferMode_ = ret != AV_ERR_OK;
    return ret;
}

int32_t VideoDecSample::RegisterCallback(OH_AVCodecCallback callback, shared_ptr<VideoDecSignal> &signal)
{
    TITLE_LOG;
    signal_ = signal;
    if (!InitFile()) {
        return AV_ERR_UNKNOWN;
    }
    callback_ = callback;
    int32_t ret = OH_VideoDecoder_RegisterCallback(codec_, callback, reinterpret_cast<void *>(signal_.get()));
    isAVBufferMode_ = ret == AV_ERR_OK;
    return ret;
}

int32_t VideoDecSample::SetOutputSurface(const bool isNew)
{
    TITLE_LOG;
    if (surafaceObj_ == nullptr) {
        surafaceObj_ = std::make_shared<SurfaceObject>(signal_, sampleId_);
    }
    int32_t ret = OH_VideoDecoder_SetSurface(codec_, surafaceObj_->GetNativeWindow(isNew));
    isSurfaceMode_ = (ret == AV_ERR_OK);
    return ret;
}

int32_t VideoDecSample::Configure()
{
    TITLE_LOG;
    OH_AVFormat *format = OH_AVFormat_Create();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_UNKNOWN, "create format failed");
    bool setFormatRet = OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, sampleWidth_) &&
                        OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, sampleHeight_) &&
                        OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, samplePixcel_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(setFormatRet, AV_ERR_UNKNOWN, "set format failed");

    int32_t ret = OH_VideoDecoder_Configure(codec_, format);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_Configure failed");

    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VideoDecSample::Start()
{
    TITLE_LOG;
    using namespace chrono;

    time_ = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    needXps_ = true;
    return OH_VideoDecoder_Start(codec_);
}

bool VideoDecSample::WaitForEos()
{
    TITLE_LOG;
    using namespace chrono;

    unique_lock<mutex> lock(signal_->eosMutex_);
    auto lck = [this]() { return signal_->isOutEos_.load(); };
    bool isNotTimeout = signal_->eosCond_.wait_for(lock, seconds(sampleTimout_), lck);
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
    int32_t ret = AV_ERR_OK;
    {
        FlushGuard guard(signal_);
        ret = OH_VideoDecoder_Stop(codec_);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_Stop failed");
    }
    ret = isSurfaceMode_ ? SetOutputSurface() : AV_ERR_OK;
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "SetOutputSurface failed");
    return ret;
}

int32_t VideoDecSample::Flush()
{
    TITLE_LOG;
    int32_t ret = AV_ERR_OK;
    {
        FlushGuard guard(signal_);
        ret = OH_VideoDecoder_Flush(codec_);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_Flush failed");
    }
    ret = isSurfaceMode_ ? SetOutputSurface() : AV_ERR_OK;
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "SetOutputSurface failed");
    return ret;
}

int32_t VideoDecSample::Reset()
{
    TITLE_LOG;
    int32_t ret = AV_ERR_OK;
    {
        FlushGuard guard(signal_);
        ret = OH_VideoDecoder_Reset(codec_);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_Reset failed");
    }
    ret = Configure();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Configure failed");
    ret = SetOutputSurface();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "SetOutputSurface failed");
    return ret;
}

int32_t VideoDecSample::Release()
{
    TITLE_LOG;
    FlushGuard guard(signal_);
    int32_t ret = OH_VideoDecoder_Destroy(codec_);
    codec_ = nullptr;
    return ret;
}

std::shared_ptr<OH_AVFormat> VideoDecSample::GetOutputDescription()
{
    TITLE_LOG;
    auto avformat = std::shared_ptr<OH_AVFormat>(OH_VideoDecoder_GetOutputDescription(codec_),
                                                 [](OH_AVFormat *ptr) { OH_AVFormat_Destroy(ptr); });
    UNITTEST_CHECK_AND_RETURN_RET_LOG(avformat != nullptr, nullptr, "OH_VideoDecoder_GetOutputDescription failed");
    return avformat;
}

int32_t VideoDecSample::SetParameter()
{
    TITLE_LOG;
    return OH_VideoDecoder_SetParameter(codec_, dyFormat_.get());
}

int32_t VideoDecSample::PushInputData(uint32_t index, OH_AVCodecBufferAttr attr)
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

int32_t VideoDecSample::ReleaseOutputData(uint32_t index)
{
    UNITTEST_INFO_LOG("index:%d", index);
    int32_t ret;
    if (isAVBufferMode_ && !isSurfaceMode_) {
        ret = OH_VideoDecoder_FreeOutputBuffer(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_FreeOutputBuffer failed");
    } else if (isAVBufferMode_ && isSurfaceMode_) {
        ret = OH_VideoDecoder_RenderOutputBuffer(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_RenderOutputBuffer failed");
    } else if (!isAVBufferMode_ && !isSurfaceMode_) {
        ret = OH_VideoDecoder_FreeOutputData(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_FreeOutputData failed");
    } else if (!isAVBufferMode_ && isSurfaceMode_) {
        ret = OH_VideoDecoder_RenderOutputData(codec_, index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_RenderOutputData failed");
    }
    frameOutputCount_++;
    return AV_ERR_OK;
}

int32_t VideoDecSample::IsValid(bool &isValid)
{
    TITLE_LOG;
    return OH_VideoDecoder_IsValid(codec_, &isValid);
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
    signal_->PopInQueue();
    int32_t ret = HandleInputFrameInner(addr, attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "HandleInputFrameInner failed");
    return SetAVBufferAttr(avBuffer, attr);
}

int32_t VideoDecSample::HandleOutputFrame(uint32_t &index, OH_AVCodecBufferAttr &attr)
{
    uint8_t *addr = nullptr;
    index = signal_->outQueue_.front();
    int32_t ret = AV_ERR_OK;
    if (isAVBufferMode_) {
        auto avBuffer = signal_->outBufferQueue_.front();
        addr = isSurfaceMode_ ? nullptr : OH_AVBuffer_GetAddr(avBuffer);
        ret = OH_AVBuffer_GetBufferAttr(avBuffer, &attr);
    } else {
        auto avMemory = signal_->outMemoryQueue_.front();
        addr = isSurfaceMode_ ? nullptr : OH_AVMemory_GetAddr(avMemory);
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

int32_t VideoDecSample::HandleInputFrame(OH_AVMemory *data, OH_AVCodecBufferAttr &attr)
{
    uint8_t *addr = OH_AVMemory_GetAddr(data);
    return HandleInputFrameInner(addr, attr);
}

int32_t VideoDecSample::HandleOutputFrame(OH_AVMemory *data, OH_AVCodecBufferAttr &attr)
{
    uint8_t *addr = isSurfaceMode_ ? nullptr : OH_AVMemory_GetAddr(data);
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
    uint8_t *addr = isSurfaceMode_ ? nullptr : OH_AVBuffer_GetAddr(data);
    OH_AVCodecBufferAttr attr;
    int32_t ret = OH_AVBuffer_GetBufferAttr(data, &attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_AVBuffer_GetBufferAttr failed");
    return HandleOutputFrameInner(addr, attr);
}

int32_t VideoDecSample::HandleInputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr)
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

    if (signal_->inFile_->eof() || needXps_) {
        needXps_ = false;
        (void)signal_->inFile_->seekg(0);
    }
    char head[FRAME_HEAD_LEN] = {};
    (void)signal_->inFile_->read(head, FRAME_HEAD_LEN);
    uint32_t bufferSize =
        static_cast<uint32_t>(((head[3] & 0xFF)) | ((head[2] & 0xFF) << OFFSET_8) | ((head[1] & 0xFF) << OFFSET_16) |
                              ((head[0] & 0xFF) << OFFSET_24)); // 0 1 2 3: avcc frame head offset

    (void)signal_->inFile_->read(reinterpret_cast<char *>(addr + FRAME_HEAD_LEN), bufferSize);
    addr[0] = 0;
    addr[1] = 0;
    addr[2] = 0; // 2: annexB frame head offset 2
    addr[3] = 1; // 3: annexB frame head offset 3

    attr.flags = IsCodecData(addr) ? AVCODEC_BUFFER_FLAGS_CODEC_DATA : AVCODEC_BUFFER_FLAGS_NONE;
    attr.size = bufferSize + FRAME_HEAD_LEN;

    uint64_t *addr64 = reinterpret_cast<uint64_t *>(addr);
    UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d, addr[0]:%" PRIX64, attr.size, (int32_t)(attr.flags), addr64[0]);
    return AV_ERR_OK;
}

int32_t VideoDecSample::HandleOutputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(addr != nullptr || isSurfaceMode_, AV_ERR_UNKNOWN, "out buffer is nullptr");

    if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
        UNITTEST_INFO_LOG("out frame:%d, in frame:%d", frameOutputCount_.load(), frameInputCount_.load());
        signal_->isOutEos_ = true;
        signal_->eosCond_.notify_all();
        return AV_ERR_OK;
    }
    if (needDump_ && !isSurfaceMode_ && frameOutputCount_ < MAX_OUTPUT_FRMAENUM) {
        if (stride_ == 0) {
            std::shared_ptr<OH_AVFormat> format = GetOutputDescription();
            OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_WIDTH, &width_);
            OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_HEIGHT, &height_);
            OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_STRIDE, &stride_);
            OH_AVFormat_GetIntValue(format.get(), OH_MD_KEY_VIDEO_SLICE_HEIGHT, &heightSlice_);
        }
        for (int32_t i = 0; i < heightSlice_; ++i) {
            (void)signal_->outFile_->write(reinterpret_cast<char *>(addr) + i * stride_, width_);
        }
        for (int32_t i = 0; i < (height_ >> 1); ++i) { // 2: denom
            (void)signal_->outFile_->write(reinterpret_cast<char *>(addr) + i * stride_, width_);
        }
    }
    if (addr == nullptr) {
        UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d", attr.size, (int32_t)(attr.flags));
    } else {
        uint64_t *addr64 = reinterpret_cast<uint64_t *>(addr);
        UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d, addr[0]:%" PRIX64, attr.size, (int32_t)(attr.flags),
                          addr64[0]);
    }
    return AV_ERR_OK;
}

int32_t VideoDecSample::Operate()
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
    } else if (operation_ == "SetOutputSurface") {
        return isSurfaceMode_ ? SetOutputSurface() : AV_ERR_OK;
    }
    UNITTEST_INFO_LOG("unknown GetParam(): %s", operation_.c_str());
    return AV_ERR_UNKNOWN;
}
} // namespace MediaAVCodec
} // namespace OHOS
