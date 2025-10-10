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
#include "window_manager.h"
#include "common/native_mfmagic.h"
#include "native_avcapability.h"
#include "native_avmagic.h"
#include "window.h"
#include "surface_buffer.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"
#include "syspara/parameters.h"
#include "media_description.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif

#define PRINT_HILOG
#define TEST_ID sampleId_
#include "unittest_log.h"
#define TITLE_LOG UNITTEST_INFO_LOG("")

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoDecSample"};
uint8_t* extradata = nullptr;
int64_t extradatasize = 0;
constexpr std::string_view filename = "/data/test/media/glitch-ffvc1.avi";
} // namespace
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

namespace {
constexpr uint32_t MAX_OUTPUT_FRMAENUM = 1000;
constexpr uint8_t WMV3_EXTRADATA[] = {0x45, 0xf9, 0x18, 0x00};
constexpr uint32_t WMV3_EXTRDATA_SIZE = sizeof(WMV3_EXTRADATA);
constexpr uint8_t WMV3_HDR_EXTRADATA[] = {0x4b, 0xf1, 0x0a, 0x01};
constexpr uint32_t WMV3_HDR_EXTRDATA_SIZE = sizeof(WMV3_HDR_EXTRADATA);

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

    TestConsumerListener(Surface *cs, std::shared_ptr<VCodecSignal> &signal, int32_t id,
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
            int32_t pixelbytes = width != 0 && stride != 0 ? (stride / width) : 1;
            for (int32_t i = 0; i < height * 3 / 2; ++i) { // 3: nom, 2: denom
                (void)signal_->outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()) + i * stride,
                                               width * pixelbytes);
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
    std::shared_ptr<VCodecSignal> signal_ = nullptr;
    std::shared_ptr<std::atomic<uint32_t>> frameOutputCount_ = 0;
};

class VideoDecSample::SurfaceObject {
public:
    SurfaceObject(std::shared_ptr<VCodecSignal> &signal, int32_t id) : sampleId_(id), signal_(signal) {}

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
        if (queue_.size() >= 2) { // 2: surface num
            obj = queue_.front();
            queue_.push(obj);
            queue_.pop();
            return;
        }
        VideoDecSample::isRosenWindow_ ? CreateRosenWindow(obj) : CreateDumpWindow(obj);
        queue_.push(std::move(obj));
    }

private:
    typedef struct WindowObject {
        OHNativeWindow *nativeWindow_ = nullptr;
        sptr<IBufferConsumerListener> listener_ = nullptr;
        sptr<Surface> consumer_ = nullptr;
        sptr<Surface> producer_ = nullptr;
        sptr<Rosen::Window> rosenWindow_ = nullptr;
    } WindowObject;

    void CreateDumpWindow(WindowObject &obj)
    {
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
    }

    void CreateRosenWindow(WindowObject &obj)
    {
        sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
        int32_t sizeModValue = static_cast<int32_t>(queue_.size()) % 2;                                // 2: surface num
        Rosen::Rect rect = {720 * sizeModValue, (sampleId_ % VideoDecSample::threadNum_) * 320, 0, 0}; // 720 320: x y
        option->SetWindowRect(rect);
        option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_FLOAT);
        option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FULLSCREEN);
        std::string name = "VideoCodec_" + std::to_string(sampleId_) + "_" + std::to_string(queue_.size());
        obj.rosenWindow_ = Rosen::Window::Create(name, option);
        UNITTEST_CHECK_AND_RETURN_LOG(obj.rosenWindow_ != nullptr, "rosen window is nullptr.");
        UNITTEST_CHECK_AND_RETURN_LOG(obj.rosenWindow_->GetSurfaceNode() != nullptr, "surface node is nullptr.");
        obj.rosenWindow_->SetTurnScreenOn(!obj.rosenWindow_->IsTurnScreenOn());
        obj.rosenWindow_->SetKeepScreenOn(true);
        obj.rosenWindow_->Show();
        obj.producer_ = obj.rosenWindow_->GetSurfaceNode()->GetSurface();
        obj.nativeWindow_ = CreateNativeWindowFromSurface(&obj.producer_);
    }

    int32_t sampleId_;
    std::shared_ptr<VCodecSignal> signal_ = nullptr;
    std::queue<WindowObject> queue_;
};
} // namespace MediaAVCodec
} // namespace OHOS

namespace OHOS {
namespace MediaAVCodec {
bool VideoDecSample::needDump_ = false;
bool VideoDecSample::isHardware_ = false;
bool VideoDecSample::isRosenWindow_ = false;
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

    isAvcStream_ = inPath_.find("h264") != std::string::npos;
    isMpeg2Stream_ = inPath_.find("m2v") != std::string::npos;
    needExtraData_ = inPath_.find("wmv3") != std::string::npos;
    isWmv3HdrStream_ = inPath_.find("hdr.wmv3") != std::string::npos;
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

    isAvcStream_ = inPath_.find("h264") != std::string::npos;
    isMpeg2Stream_ = inPath_.find("m2v") != std::string::npos;
    needExtraData_ = inPath_.find("wmv3") != std::string::npos;
    isWmv3HdrStream_ = inPath_.find("hdr.wmv3") != std::string::npos;
    inPath_ = "/data/test/media/" + inPath_;
    outPath_ = "/data/test/media/" + outPath_ + to_string(sampleId_ % threadNum_) + ".yuv";
    codec_ = OH_VideoDecoder_CreateByMime(mime_.c_str());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, false, "OH_VideoDecoder_CreateByMime failed");
    return true;
}

bool VideoDecSample::InitInputFile()
{
    if (signal_->reader_ == nullptr) {
        if (inPath_.find("h263") != std::string::npos) {
            int32_t ret = CreateH263Reader();
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "CreateH263Reader failed");
        } else if (inPath_.find("h264") != std::string::npos || inPath_.find("h265") != std::string::npos) {
            int32_t ret = CreateAvccReader();
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "CreateAvccReader failed");
        } else if (inPath_.find("vc1") != std::string::npos) {
            int32_t ret = CreateVc1Reader();
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "CreateH263Reader failed");
        } else if (inPath_.find("wmv3") != std::string::npos) {
            int32_t ret = CreateWmv3Reader();
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "CreateWmv3Reader failed");
        } else {
            int32_t ret = CreateMpegReader();
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "CreateMpegReader failed");
        }
    }
    return true;
}

bool VideoDecSample::InitOutputFile()
{
    if (signal_->outFile_ == nullptr && needDump_) {
        signal_->outFile_ = make_unique<ofstream>();
        signal_->outFile_->open(outPath_, ios::out | ios::binary);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(signal_->outFile_ != nullptr, false, "create signal_->outFile_ failed");
    }
    return true;
}

int32_t VideoDecSample::CreateAvccReader()
{
    std::shared_ptr<AvccReaderInfo> info = std::make_shared<AvccReaderInfo>();
    info->inPath = inPath_;
    info->isAvcStream = isAvcStream_;

    signal_->reader_ = std::make_shared<AvccReader>();
    int32_t ret = std::static_pointer_cast<AvccReader>(signal_->reader_)->Init(info);
    return ret;
}

int32_t VideoDecSample::CreateMpegReader()
{
    std::shared_ptr<MpegReaderInfo> info = std::make_shared<MpegReaderInfo>();
    info->inPath = inPath_;
    info->isMpeg2Stream = isMpeg2Stream_;

    signal_->reader_ = std::make_shared<MpegReader>();
    int32_t ret = std::static_pointer_cast<MpegReader>(signal_->reader_)->Init(info);
    return ret;
}

int32_t VideoDecSample::CreateVc1Reader()
{
    std::shared_ptr<Vc1ReaderInfo> info = std::make_shared<Vc1ReaderInfo>();
    info->inPath = inPath_;

    signal_->reader_ = std::make_shared<Vc1Reader>();
    int32_t ret = std::static_pointer_cast<Vc1Reader>(signal_->reader_)->Init(info);
    return ret;
}

int32_t VideoDecSample::CreateH263Reader()
{
    std::shared_ptr<H263ReaderInfo> info = std::make_shared<H263ReaderInfo>();
    info->inPath = inPath_;

    signal_->reader_ = std::make_shared<H263Reader>();
    int32_t ret = std::static_pointer_cast<H263Reader>(signal_->reader_)->Init(info);
    return ret;
}

int32_t VideoDecSample::CreateWmv3Reader()
{
    std::shared_ptr<Wmv3ReaderInfo> info = std::make_shared<Wmv3ReaderInfo>();
    info->inPath = inPath_;
    info->isHdrStream = isWmv3HdrStream_;

    signal_->reader_ = std::make_shared<Wmv3Reader>();
    int32_t ret = std::static_pointer_cast<Wmv3Reader>(signal_->reader_)->Init(info);
    return ret;
}

int32_t VideoDecSample::SetCallback(OH_AVCodecAsyncCallback callback, shared_ptr<VCodecSignal> &signal)
{
    TITLE_LOG;
    signal_ = signal;
    if (!InitInputFile() || !InitOutputFile()) {
        return AV_ERR_UNKNOWN;
    }
    asyncCallback_ = callback;
    int32_t ret = OH_VideoDecoder_SetCallback(codec_, callback, reinterpret_cast<void *>(signal_.get()));
    isAVBufferMode_ = ret != AV_ERR_OK;
    return ret;
}

int32_t VideoDecSample::RegisterCallback(OH_AVCodecCallback callback, shared_ptr<VCodecSignal> &signal)
{
    TITLE_LOG;
    signal_ = signal;
    if (!InitInputFile() || !InitOutputFile()) {
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

OHNativeWindow *VideoDecSample::GetSurfaceWindow(const bool isNew)
{
    if (surafaceObj_ != nullptr) {
        return surafaceObj_->GetNativeWindow(isNew);
    }
    return nullptr;
}

int32_t VideoDecSample::SetOutputSurface(OHNativeWindow *window)
{
    if (window != nullptr) {
        return OH_VideoDecoder_SetSurface(codec_, window);
    }
    return AV_ERR_UNKNOWN;
}

bool VideoDecSample::DoConfigure(OH_AVFormat* format)
{
    bool setFormatRet = OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, sampleWidth_) &&
                        OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, sampleHeight_);
    if (setPixelFormat_) {
        setFormatRet = setFormatRet && OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, samplePixel_);
    }

    if (setSurfaceParam_) {
        setFormatRet = setFormatRet && OH_AVFormat_SetIntValue(dyFormat_.get(), OH_MD_KEY_ROTATION, defaultRotation_) &&
            OH_AVFormat_SetIntValue(dyFormat_.get(), OH_MD_KEY_SCALING_MODE, OH_ScalingMode::SCALING_MODE_SCALE_CROP) &&
            OH_AVFormat_SetIntValue(dyFormat_.get(), OH_MD_MAX_OUTPUT_BUFFER_COUNT, defaultBufferCount_) &&
            OH_AVFormat_SetIntValue(dyFormat_.get(), OH_MD_MAX_INPUT_BUFFER_COUNT, defaultBufferCount_) &&
            OH_AVFormat_SetLongValue(dyFormat_.get(), OH_MD_KEY_BITRATE, 1000000); // 1000000
        if (setPixelFormat_) {
            setFormatRet = setFormatRet &&
                OH_AVFormat_SetIntValue(dyFormat_.get(), OH_MD_KEY_PIXEL_FORMAT, samplePixel_);
        }
    }

    if (ohRotation_) {
        setFormatRet = setFormatRet && OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, defaultRotation_);
    }
    if (maxOutputBufferCount_) {
        setFormatRet = setFormatRet &&
            OH_AVFormat_SetIntValue(format, OH_MD_MAX_OUTPUT_BUFFER_COUNT, defaultBufferCount_);
    }
    if (maxInputBufferCount_) {
        setFormatRet = setFormatRet &&
            OH_AVFormat_SetIntValue(format, OH_MD_MAX_INPUT_BUFFER_COUNT, defaultBufferCount_);
    }
    if (scaleMode_) {
        setFormatRet = setFormatRet &&
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_SCALING_MODE, OH_ScalingMode::SCALING_MODE_SCALE_CROP);
    }

    if (needExtraData_) {
        uint32_t extradataSize = isWmv3HdrStream_ ? WMV3_HDR_EXTRDATA_SIZE : WMV3_EXTRDATA_SIZE;
        auto extradata = isWmv3HdrStream_ ? WMV3_HDR_EXTRADATA : WMV3_EXTRADATA;
        OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG, extradata, extradataSize);
    }

    if (lowLatency_) {
        setFormatRet = setFormatRet && OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENABLE_LOW_LATENCY, 1);
        setFormatRet = setFormatRet && OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 1000000); // 1000000
    }
    return setFormatRet;
}

int32_t VideoDecSample::Configure()
{
    TITLE_LOG;
    OH_AVFormat *format = OH_AVFormat_Create();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_UNKNOWN, "create format failed");
    bool setFormatRet = DoConfigure(format);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(setFormatRet, AV_ERR_UNKNOWN, "set format failed");

    if (inPath_.find("vc1") != std::string::npos) {
        AVFormatContext* formatContext = avformat_alloc_context();
        int ret_ = avformat_open_input(&formatContext, filename.data(), nullptr, nullptr);
        ret_ = avformat_find_stream_info(formatContext, nullptr);
        for (int i = 0; i < formatContext->nb_streams; i++) {
            AVStream* stream = formatContext->streams[i];
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                extradata = stream->codecpar->extradata;
                extradatasize = stream->codecpar->extradata_size;
            }
        }
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret_ == AV_ERR_OK, AV_ERR_UNKNOWN, "avformat_find_stream_info failed");
        OH_AVFormat_SetBuffer(format, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), extradata, extradatasize);
    }
    if (!dumpKey_.empty() && !dumpValue_.empty()) {
        bool dumpRet = OHOS::system::SetParameter(dumpKey_, dumpValue_);
        UNITTEST_INFO_LOG("SetParameter %s %s ret: %d", dumpKey_.c_str(), dumpValue_.c_str(), dumpRet);
    }

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
    // Not all streams meet the requirement that the number of output frames is greater than
    // half of the number of input NALs.
    if (skipOutFrameHalfCheck_) {
        EXPECT_GE(frameOutputCount_, 0);
    } else {
        EXPECT_GE(frameOutputCount_, frameInputCount_ / 2); // 2: at least half of the input frame
    }

    signal_->isRunning_ = false;
    usleep(100); // 100: wait for callback
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        outputLoop_->join();
    }
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
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
    std::lock_guard<std::shared_mutex> lock(codecMutex_);
    int32_t ret = OH_VideoDecoder_Stop(codec_);
    signal_->FlushQueue();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_Stop failed");
    return ret;
}

int32_t VideoDecSample::Flush()
{
    TITLE_LOG;
    std::lock_guard<std::shared_mutex> lock(codecMutex_);
    int32_t ret = OH_VideoDecoder_Flush(codec_);
    signal_->FlushQueue();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_Flush failed");
    return ret;
}

int32_t VideoDecSample::Reset()
{
    TITLE_LOG;
    int32_t ret = AV_ERR_OK;
    {
        std::lock_guard<std::shared_mutex> lock(codecMutex_);
        ret = OH_VideoDecoder_Reset(codec_);
        signal_->FlushQueue();
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
    std::lock_guard<std::shared_mutex> lock(codecMutex_);
    int32_t ret = OH_VideoDecoder_Destroy(codec_);
    signal_->FlushQueue();
    codec_ = nullptr;
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_Destroy failed");
    return ret;
}

std::shared_ptr<OH_AVFormat> VideoDecSample::GetOutputDescription()
{
    TITLE_LOG;
    auto avformat = std::shared_ptr<OH_AVFormat>(OH_VideoDecoder_GetOutputDescription(codec_),
                                                 [](OH_AVFormat *ptr) { OH_AVFormat_Destroy(ptr); });

    OH_AVFormat_GetIntValue(avformat.get(), OH_MD_KEY_VIDEO_PIC_WIDTH, &signal_->width_);
    OH_AVFormat_GetIntValue(avformat.get(), OH_MD_KEY_VIDEO_PIC_HEIGHT, &signal_->height_);
    OH_AVFormat_GetIntValue(avformat.get(), OH_MD_KEY_VIDEO_STRIDE, &signal_->widthStride_);
    OH_AVFormat_GetIntValue(avformat.get(), OH_MD_KEY_VIDEO_SLICE_HEIGHT, &signal_->heightStride_);
    OH_AVFormat_GetIntValue(avformat.get(), OH_MD_KEY_VIDEO_CROP_TOP, &signal_->cropTop_);
    OH_AVFormat_GetIntValue(avformat.get(), OH_MD_KEY_VIDEO_CROP_BOTTOM, &signal_->cropBottom_);
    OH_AVFormat_GetIntValue(avformat.get(), OH_MD_KEY_VIDEO_CROP_LEFT, &signal_->cropLeft_);
    OH_AVFormat_GetIntValue(avformat.get(), OH_MD_KEY_VIDEO_CROP_RIGHT, &signal_->cropRight_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(avformat != nullptr, nullptr, "OH_VideoDecoder_GetOutputDescription failed");
    return avformat;
}

int32_t VideoDecSample::SetParameter()
{
    TITLE_LOG;
    return OH_VideoDecoder_SetParameter(codec_, dyFormat_.get());
}

int32_t VideoDecSample::PushInputData(std::shared_ptr<CodecBufferInfo> bufferInfo)
{
    UNITTEST_INFO_LOG("index:%d", bufferInfo->GetIndex());
    if (signal_->isInEos_) {
        if (!isFirstEos_) {
            UNITTEST_INFO_LOG("At Eos State");
            return AV_ERR_OK;
        }
        isFirstEos_ = false;
    }
    int32_t ret = AV_ERR_OK;
    if (isAVBufferMode_) {
        ret = OH_VideoDecoder_PushInputBuffer(codec_, bufferInfo->GetIndex());
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_PushInputBuffer failed");
    } else {
        ret = OH_VideoDecoder_PushInputData(codec_, bufferInfo->GetIndex(), bufferInfo->GetAttr());
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_PushInputData failed");
    }
    frameInputCount_++;
    return AV_ERR_OK;
}

int32_t VideoDecSample::ReleaseOutputData(std::shared_ptr<CodecBufferInfo> bufferInfo)
{
    UNITTEST_INFO_LOG("index:%d", bufferInfo->GetIndex());
    int32_t ret;
    if (isAVBufferMode_ && !isSurfaceMode_) {
        ret = OH_VideoDecoder_FreeOutputBuffer(codec_, bufferInfo->GetIndex());
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_FreeOutputBuffer failed");
    } else if (isAVBufferMode_ && isSurfaceMode_) {
        ret = OH_VideoDecoder_RenderOutputBuffer(codec_, bufferInfo->GetIndex());
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_RenderOutputBuffer failed");
    } else if (!isAVBufferMode_ && !isSurfaceMode_) {
        if (releaseOtherBuffer_ && bufferInfo->GetIndex() != 0) {
            ret = OH_VideoDecoder_FreeOutputData(codec_, 0);
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret != AV_ERR_OK, AV_ERR_UNKNOWN,
                "OH_VideoDecoder_FreeOutputData failed");
        }
        ret = OH_VideoDecoder_FreeOutputData(codec_, bufferInfo->GetIndex());
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoDecoder_FreeOutputData failed");
    } else if (!isAVBufferMode_ && isSurfaceMode_) {
        if (releaseOtherBuffer_ && bufferInfo->GetIndex() != 0) {
            ret = OH_VideoDecoder_RenderOutputData(codec_, 0);
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret != AV_ERR_OK, AV_ERR_UNKNOWN,
                "OH_VideoDecoder_RenderOutputData failed");
        }
        ret = OH_VideoDecoder_RenderOutputData(codec_, bufferInfo->GetIndex());
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

int32_t VideoDecSample::HandleInputFrame(std::shared_ptr<CodecBufferInfo> bufferInfo)
{
    std::shared_lock<std::shared_mutex> lock(codecMutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
        return AV_ERR_OK;
    }
    uint8_t *addr = bufferInfo->GetAddr();
    OH_AVCodecBufferAttr attr = {0, 0, 0, 0};
    HandleInputFrameInner(addr, attr);
    bufferInfo->SetAttr(attr);
    UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d", attr.size, (int32_t)(attr.flags));
    return PushInputData(bufferInfo);
}

int32_t VideoDecSample::HandleOutputFrame(std::shared_ptr<CodecBufferInfo> bufferInfo)
{
    std::shared_lock<std::shared_mutex> lock(codecMutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
        return AV_ERR_OK;
    }
    uint8_t *addr = bufferInfo->GetAddr();
    OH_AVCodecBufferAttr attr = bufferInfo->GetAttr();
    int32_t ret = HandleOutputFrameInner(addr, attr);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "HandleOutputFrameInner failed, index: %d", index);
    return ReleaseOutputData(bufferInfo);
}

int32_t VideoDecSample::HandleInputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr)
{
    signal_->reader_->FillBuffer(addr, attr);
    // 输入帧数不够时，循环读文件
    if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS || needXps_) {
        needXps_ = false;
        signal_->reader_ = nullptr;
        InitInputFile();
        if (frameCount_ > frameInputCount_) {
            signal_->reader_->FillBuffer(addr, attr);
        }
    }
    if (frameCount_ <= frameInputCount_) {
        signal_->isInEos_ = true;
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        attr.size = 0;
    }
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
    if (frameOutputCount_ == 0) {
        GetOutputDescription();
    }
    char *temp = reinterpret_cast<char *>(addr);
    if (needDump_ && !isSurfaceMode_ && frameOutputCount_ < MAX_OUTPUT_FRMAENUM) {
        int32_t pixelbytes =
            signal_->width_ != 0 && signal_->widthStride_ != 0 ? (signal_->widthStride_ / signal_->width_) : 1;
        for (int32_t i = 0; i < signal_->height_; ++i) {
            (void)signal_->outFile_->write(temp, signal_->width_ * pixelbytes);
            temp += signal_->widthStride_;
        }
        temp += signal_->widthStride_ * (signal_->heightStride_ - signal_->height_);
        for (int32_t i = 0; i < (signal_->height_ >> 1); ++i) { // 2: denom
            (void)signal_->outFile_->write(temp, signal_->width_ * pixelbytes);
            temp += signal_->widthStride_;
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
