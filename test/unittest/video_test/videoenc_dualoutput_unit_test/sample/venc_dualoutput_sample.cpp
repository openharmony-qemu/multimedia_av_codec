/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include "common/native_mfmagic.h"
#include "meta/meta_key.h"
#include "venc_dualoutput_sample.h"

#define PRINT_HILOG
#define TEST_ID primarySampleId_
#include "unittest_log.h"
#define TITLE_LOG UNITTEST_INFO_LOG("")

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using ReadFrame = std::function<int32_t(std::shared_ptr<VCodecSignal> &, uint8_t *, int32_t, int32_t, int32_t)>;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoEncDualOutputSample"};

int32_t ReadYUV420SP(std::shared_ptr<VCodecSignal> &signal, uint8_t *addr, int32_t width, int32_t height,
                     int32_t stride)
{
    if (width == 0) {
        return 0;
    }
    int32_t pixelSize = int32_t(stride / width);
    width *= pixelSize;
    for (int32_t i = 0; i < (height * 3 / 2); ++i) { // 3: nom, 2: denom
        signal->inFile_->read(reinterpret_cast<char *>(addr), width);
        addr += stride;
    }
    return stride * height * 3 / 2; // 3: nom, 2: denom
}

int32_t ReadYUV420P(std::shared_ptr<VCodecSignal> &signal, uint8_t *addr, int32_t width, int32_t height, int32_t stride)
{
    if (width == 0) {
        return 0;
    }
    int32_t pixelSize = int32_t(stride / width);
    width *= pixelSize;
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

int32_t ReadRGBA(std::shared_ptr<VCodecSignal> &signal, uint8_t *addr, int32_t width, int32_t height, int32_t stride)
{
    if (width == 0) {
        return 0;
    }
    int32_t pixelSize = int32_t(stride / width);
    width *= pixelSize;
    for (int32_t i = 0; i < height; ++i) {
        signal->inFile_->read(reinterpret_cast<char *>(addr), width);
        addr += stride;
    }
    return stride * height;
}

ReadFrame GetReadOneFrame(int32_t pixelFormat)
{
    switch (pixelFormat) {
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
constexpr uint32_t MAX_OUTPUT_FRAMENUM = 60;
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
bool VideoEncDualOutputSample::needDump_ = false;
uint64_t VideoEncDualOutputSample::sampleTimout_ = 180;

VideoEncDualOutputSample::VideoEncDualOutputSample()
{
    static atomic<int32_t> sampleId = 0;
    primarySampleId_ = ++sampleId;
    primaryEncoder_.sampleId = primarySampleId_;
    secondaryEncoder_.sampleId = ++sampleId;
    TITLE_LOG;
    primaryEncoder_.format = std::shared_ptr<OH_AVFormat>(OH_AVFormat_Create(), [](OH_AVFormat *ptr) { OH_AVFormat_Destroy(ptr); });
    secondaryEncoder_.format = std::shared_ptr<OH_AVFormat>(OH_AVFormat_Create(), [](OH_AVFormat *ptr) { OH_AVFormat_Destroy(ptr); });
}

VideoEncDualOutputSample::~VideoEncDualOutputSample()
{
    TITLE_LOG;
    StopSurfaceInput();
    if (secondaryEncoder_.codec != nullptr) {
        int32_t ret = OH_VideoEncoder_Destroy(secondaryEncoder_.codec);
        UNITTEST_CHECK_AND_INFO_LOG(ret == AV_ERR_OK, "OH_VideoEncoder_Destroy secondary failed");
        secondaryEncoder_.codec = nullptr;
    }
    if (primaryEncoder_.codec != nullptr) {
        int32_t ret = OH_VideoEncoder_Destroy(primaryEncoder_.codec);
        UNITTEST_CHECK_AND_INFO_LOG(ret == AV_ERR_OK, "OH_VideoEncoder_Destroy primary failed");
    }
    if (nativeWindow_ != nullptr) {
        DestoryNativeWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }
}

VideoEncDualOutputSample::EncoderContext &VideoEncDualOutputSample::GetEncoderByEncoderType(EncoderType type)
{
    return (type == EncoderType::PRIMARY) ? primaryEncoder_ : secondaryEncoder_;
}

OH_AVCodec *VideoEncDualOutputSample::GetCodec(EncoderType type)
{
    return GetEncoderByEncoderType(type).codec;
}

OH_AVFormat *VideoEncDualOutputSample::GetFormat(EncoderType type)
{
    return GetEncoderByEncoderType(type).format.get();
}

bool VideoEncDualOutputSample::CreatePrimary(const string &mime)
{
    TITLE_LOG;
    mime_ = mime;
    OH_AVErrCode ret = OH_VideoEncoder_CreatePrimaryWithPreproc(mime.c_str(), &primaryEncoder_.codec);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, false, "OH_VideoEncoder_CreatePrimaryWithPreproc failed");
    return true;
}

bool VideoEncDualOutputSample::CreateSecondary()
{
    TITLE_LOG;
    UNITTEST_CHECK_AND_RETURN_RET_LOG(primaryEncoder_.codec != nullptr, false, "primary codec is nullptr");
    OH_AVErrCode ret = OH_VideoEncoder_CreateSecondaryFromPrimary(primaryEncoder_.codec, &secondaryEncoder_.codec);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, false, "OH_VideoEncoder_CreateSecondaryFromPrimary failed");
    return true;
}

int32_t VideoEncDualOutputSample::SetBasicFormat(EncoderType type, int32_t width, int32_t height,
                                                  int32_t pixelFormat, double frameRate)
{
    TITLE_LOG;
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.format != nullptr, AV_ERR_UNKNOWN, "format is nullptr");
    bool ret = OH_AVFormat_SetIntValue(encoderContext.format.get(), OH_MD_KEY_WIDTH, width) &&
               OH_AVFormat_SetIntValue(encoderContext.format.get(), OH_MD_KEY_HEIGHT, height) &&
               OH_AVFormat_SetIntValue(encoderContext.format.get(), OH_MD_KEY_PIXEL_FORMAT, pixelFormat) &&
               OH_AVFormat_SetDoubleValue(encoderContext.format.get(), OH_MD_KEY_FRAME_RATE, frameRate);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret, AV_ERR_UNKNOWN, "SetBasicFormat failed");
    return AV_ERR_OK;
}

int32_t VideoEncDualOutputSample::SetDownsampling(EncoderType type, int32_t downWidth, int32_t downHeight)
{
    TITLE_LOG;
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.format != nullptr, AV_ERR_UNKNOWN, "format is nullptr");
    bool ret = OH_AVFormat_SetIntValue(encoderContext.format.get(), OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH, downWidth) &&
               OH_AVFormat_SetIntValue(encoderContext.format.get(), OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT, downHeight);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret, AV_ERR_UNKNOWN, "SetDownsampling failed");
    return AV_ERR_OK;
}

int32_t VideoEncDualOutputSample::SetCrop(EncoderType type, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    TITLE_LOG;
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.format != nullptr, AV_ERR_UNKNOWN, "format is nullptr");
    bool ret = OH_AVFormat_SetIntValue(encoderContext.format.get(), OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, left) &&
               OH_AVFormat_SetIntValue(encoderContext.format.get(), OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, top) &&
               OH_AVFormat_SetIntValue(encoderContext.format.get(), OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, right) &&
               OH_AVFormat_SetIntValue(encoderContext.format.get(), OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, bottom);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret, AV_ERR_UNKNOWN, "SetCrop failed");
    return AV_ERR_OK;
}

int32_t VideoEncDualOutputSample::SetDropFrameRate(EncoderType type, double frameRate)
{
    TITLE_LOG;
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.format != nullptr, AV_ERR_UNKNOWN, "format is nullptr");
    bool ret = OH_AVFormat_SetDoubleValue(encoderContext.format.get(), OH_MD_KEY_VIDEO_ENCODER_PREPROC_DROP_TO_FRAME_RATE, frameRate);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret, AV_ERR_UNKNOWN, "SetDropFrameRate failed");
    return AV_ERR_OK;
}

bool VideoEncDualOutputSample::InitFile(EncoderType type)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    string ext = (mime_.find("hevc") != string::npos) ? ".h265" : ".h264";
    if (type == EncoderType::PRIMARY) {
        if (encoderContext.signal->inFile_ == nullptr) {
            encoderContext.signal->inFile_ = make_unique<ifstream>();
            string inPath = "/data/test/media/" + inPath_;
            encoderContext.signal->inFile_->open(inPath, ios::in | ios::binary);
            UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.signal->inFile_ != nullptr, false, "create inFile_ failed");
            UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.signal->inFile_->is_open(), false, "can not open inFile_");
        }
        if (encoderContext.signal->outFile_ == nullptr && needDump_) {
            encoderContext.signal->outFile_ = make_unique<ofstream>();
            const ::testing::TestInfo *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
            string outPath = "/data/test/media/primary_" + string(testInfo->name()) + ext;
            encoderContext.signal->outFile_->open(outPath, ios::out | ios::binary);
            UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.signal->outFile_ != nullptr, false, "create outFile_ failed");
        }
    } else {
        if (encoderContext.signal->outFile_ == nullptr && needDump_) {
            encoderContext.signal->outFile_ = make_unique<ofstream>();
            const ::testing::TestInfo *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
            string outPath = "/data/test/media/secondary_" + string(testInfo->name()) + ext;
            encoderContext.signal->outFile_->open(outPath, ios::out | ios::binary);
            UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.signal->outFile_ != nullptr, false, "create sec outFile_ failed");
        }
    }
    return true;
}

int32_t VideoEncDualOutputSample::SetCallback(EncoderType type, OH_AVCodecAsyncCallback callback,
                                               shared_ptr<VCodecSignal> &signal)
{
    TITLE_LOG;
    auto &encoderContext = GetEncoderByEncoderType(type);
    encoderContext.signal = signal;
    if (!InitFile(type)) {
        return AV_ERR_UNKNOWN;
    }
    encoderContext.asyncCallback = callback;
    int32_t ret = OH_VideoEncoder_SetCallback(encoderContext.codec, callback, reinterpret_cast<void *>(encoderContext.signal.get()));
    if (type == EncoderType::PRIMARY) {
        encoderContext.isAVBufferMode = ret != AV_ERR_OK;
    }
    return ret;
}

int32_t VideoEncDualOutputSample::SetCallback(EncoderType type, OH_AVCodecAsyncCallback callback, void *userData)
{
    TITLE_LOG;
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    if (type == EncoderType::PRIMARY) {
        encoderContext.isAVBufferMode = false;
    }
    return OH_VideoEncoder_SetCallback(encoderContext.codec, callback, userData);
}

int32_t VideoEncDualOutputSample::RegisterCallback(EncoderType type, OH_AVCodecCallback callback,
                                                    shared_ptr<VCodecSignal> &signal)
{
    TITLE_LOG;
    auto &encoderContext = GetEncoderByEncoderType(type);
    encoderContext.signal = signal;
    if (!InitFile(type)) {
        return AV_ERR_UNKNOWN;
    }
    encoderContext.callback = callback;
    int32_t ret = OH_VideoEncoder_RegisterCallback(encoderContext.codec, callback, reinterpret_cast<void *>(encoderContext.signal.get()));
    if (type == EncoderType::PRIMARY) {
        encoderContext.isAVBufferMode = ret == AV_ERR_OK;
    }
    return ret;
}

int32_t VideoEncDualOutputSample::RegisterCallback(EncoderType type, OH_AVCodecCallback callback, void *userData)
{
    TITLE_LOG;
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    if (type == EncoderType::PRIMARY) {
        encoderContext.isAVBufferMode = true;
    }
    return OH_VideoEncoder_RegisterCallback(encoderContext.codec, callback, userData);
}

int32_t VideoEncDualOutputSample::Configure(EncoderType type)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    return OH_VideoEncoder_Configure(encoderContext.codec, encoderContext.format.get());
}

int32_t VideoEncDualOutputSample::Prepare(EncoderType type)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    return OH_VideoEncoder_Prepare(encoderContext.codec);
}

int32_t VideoEncDualOutputSample::Start(EncoderType type)
{
    TITLE_LOG;
    auto &encoderContext = GetEncoderByEncoderType(type);
    if (type == EncoderType::PRIMARY) {
        using namespace chrono;
        encoderContext.time = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        encoderContext.isFirstFrame = true;
    }
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    int32_t ret = OH_VideoEncoder_Start(encoderContext.codec);
    if (ret == AV_ERR_OK && type == EncoderType::PRIMARY && nativeWindow_ != nullptr) {
        if (inputLoop_ != nullptr && inputLoop_->joinable()) {
            if (primaryEncoder_.signal != nullptr) {
                primaryEncoder_.signal->isRunning_.store(false);
            }
            inputLoop_->join();
            inputLoop_.reset();
        }
        if (primaryEncoder_.signal != nullptr) {
            primaryEncoder_.signal->isRunning_.store(true);
        }
        inputLoop_ = make_unique<std::thread>(&VideoEncDualOutputSample::InputFuncSurface, this);
    }
    return ret;
}

int32_t VideoEncDualOutputSample::Stop(EncoderType type)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    std::lock_guard<std::shared_mutex> lock(encoderContext.codecMutex);
    int32_t ret = OH_VideoEncoder_Stop(encoderContext.codec);
    if (encoderContext.signal != nullptr) {
        encoderContext.signal->FlushQueue();
    }
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_Stop failed");
    return ret;
}

int32_t VideoEncDualOutputSample::Flush(EncoderType type)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    std::lock_guard<std::shared_mutex> lock(encoderContext.codecMutex);
    int32_t ret = OH_VideoEncoder_Flush(encoderContext.codec);
    if (encoderContext.signal != nullptr) {
        encoderContext.signal->FlushQueue();
    }
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_Flush failed");
    return ret;
}

int32_t VideoEncDualOutputSample::Reset(EncoderType type)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    {
        std::lock_guard<std::shared_mutex> lock(encoderContext.codecMutex);
        int32_t ret = OH_VideoEncoder_Reset(encoderContext.codec);
        if (encoderContext.signal != nullptr) {
            encoderContext.signal->FlushQueue();
        }
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_Reset failed");
    }
    return AV_ERR_OK;
}

int32_t VideoEncDualOutputSample::Release(EncoderType type)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    std::lock_guard<std::shared_mutex> lock(encoderContext.codecMutex);
    int32_t ret = OH_VideoEncoder_Destroy(encoderContext.codec);
    if (encoderContext.signal != nullptr) {
        encoderContext.signal->FlushQueue();
    }
    encoderContext.codec = nullptr;
    return ret;
}

int32_t VideoEncDualOutputSample::NotifyEos(EncoderType type)
{
    TITLE_LOG;
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    int32_t ret = OH_VideoEncoder_NotifyEndOfStream(encoderContext.codec);
    if (type == EncoderType::PRIMARY) {
        encoderContext.frameInputCount_++;
    }
    return ret;
}

int32_t VideoEncDualOutputSample::IsValid(EncoderType type, bool &isValid)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    return OH_VideoEncoder_IsValid(encoderContext.codec, &isValid);
}

bool VideoEncDualOutputSample::WaitForEos(EncoderType type)
{
    TITLE_LOG;
    using namespace chrono;
    auto &encoderContext = GetEncoderByEncoderType(type);
    if (encoderContext.signal == nullptr) {
        return false;
    }
    unique_lock<mutex> lock(encoderContext.signal->eosMutex_);
    auto lck = [this, &encoderContext]() { return encoderContext.signal->isOutEos_.load(); };
    bool isNotTimeout = encoderContext.signal->eosCond_.wait_for(lock, seconds(sampleTimout_), lck);
    lock.unlock();

    if (type == EncoderType::PRIMARY) {
        EXPECT_GE(encoderContext.frameOutputCount_, encoderContext.frameInputCount_ / 2); // 2: at least half of the input frame
    }

    encoderContext.signal->isRunning_ = false;
    usleep(100); // 100: wait for callback
    int64_t tempTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    if (!isNotTimeout) {
        cout << (type == EncoderType::PRIMARY ? "Primary" : "Secondary")
             << " run func timeout, time used: " << tempTime - primaryEncoder_.time << "ms" << endl;
        return false;
    } else {
        cout << (type == EncoderType::PRIMARY ? "Primary" : "Secondary")
             << " run func finish, time used: " << tempTime - primaryEncoder_.time << "ms" << endl;
        return true;
    }
}

std::shared_ptr<OH_AVFormat> VideoEncDualOutputSample::GetOutputDescription(EncoderType type)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, nullptr, "codec is nullptr");
    auto avformat = std::shared_ptr<OH_AVFormat>(OH_VideoEncoder_GetOutputDescription(encoderContext.codec),
                                                 [](OH_AVFormat *ptr) { OH_AVFormat_Destroy(ptr); });
    UNITTEST_CHECK_AND_RETURN_RET_LOG(avformat != nullptr, nullptr, "OH_VideoEncoder_GetOutputDescription failed");
    return avformat;
}

std::shared_ptr<OH_AVFormat> VideoEncDualOutputSample::GetInputDescription(EncoderType type)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, nullptr, "codec is nullptr");
    auto avformat = std::shared_ptr<OH_AVFormat>(OH_VideoEncoder_GetInputDescription(encoderContext.codec),
                                                 [](OH_AVFormat *ptr) { OH_AVFormat_Destroy(ptr); });
    UNITTEST_CHECK_AND_RETURN_RET_LOG(avformat != nullptr, nullptr, "OH_VideoEncoder_GetInputDescription failed");
    return avformat;
}

int32_t VideoEncDualOutputSample::SetParameter(EncoderType type, OH_AVFormat *format)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    return OH_VideoEncoder_SetParameter(encoderContext.codec, format);
}

int32_t VideoEncDualOutputSample::QueryInputBuffer(EncoderType type, uint32_t *index, int64_t timeoutUs)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    return OH_VideoEncoder_QueryInputBuffer(encoderContext.codec, index, timeoutUs);
}

int32_t VideoEncDualOutputSample::QueryOutputBuffer(EncoderType type, uint32_t *index, int64_t timeoutUs)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    return OH_VideoEncoder_QueryOutputBuffer(encoderContext.codec, index, timeoutUs);
}

int32_t VideoEncDualOutputSample::PushInputBuffer(EncoderType type, uint32_t index)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    return OH_VideoEncoder_PushInputBuffer(encoderContext.codec, index);
}

int32_t VideoEncDualOutputSample::FreeOutputBuffer(EncoderType type, uint32_t index)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    return OH_VideoEncoder_FreeOutputBuffer(encoderContext.codec, index);
}

int32_t VideoEncDualOutputSample::HandleOutputFrameByEncoderType(EncoderType type,
                                                                 shared_ptr<CodecBufferInfo> bufferInfo)
{
    auto &encoderContext = GetEncoderByEncoderType(type);
    shared_lock<shared_mutex> lock(encoderContext.codecMutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return AV_ERR_OK;
    }
    UNITTEST_INFO_LOG("%s output index:%d", type == EncoderType::PRIMARY ? "primary" : "secondary", bufferInfo->GetIndex());

    uint8_t *addr = bufferInfo->GetAddr();
    OH_AVCodecBufferAttr attr = bufferInfo->GetAttr();
    if (addr == nullptr) {
        return AV_ERR_OK;
    }

    int32_t ret;
    if (type == EncoderType::PRIMARY) {
        ret = HandleOutputFrameInner(addr, attr);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "HandleOutputFrameInner failed");
        return ReleaseOutputData(encoderContext, bufferInfo);
    } else {
        ret = HandleOutputFrameSecondaryInner(addr, attr);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "HandleOutputFrameSecondaryInner failed");
        return ReleaseOutputDataSecondary(encoderContext, bufferInfo);
    }
}

int32_t VideoEncDualOutputSample::HandleInputFrame(shared_ptr<CodecBufferInfo> bufferInfo)
{
    shared_lock<shared_mutex> lock(primaryEncoder_.codecMutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return AV_ERR_OK;
    }
    uint8_t *addr = bufferInfo->GetAddr();
    OH_AVCodecBufferAttr attr = {0, 0, 0, 0};
    HandleInputFrameInner(addr, attr);
    bufferInfo->SetAttr(attr);
    return PushInputData(primaryEncoder_, bufferInfo);
}

int32_t VideoEncDualOutputSample::HandleOutputFrame(shared_ptr<CodecBufferInfo> bufferInfo)
{
    return HandleOutputFrameByEncoderType(EncoderType::PRIMARY, bufferInfo);
}

int32_t VideoEncDualOutputSample::Operate()
{
    int32_t ret = AV_ERR_OK;
    if (operation_ == "Flush") {
        ret = Flush(EncoderType::PRIMARY);
        return ret == AV_ERR_OK ? Start(EncoderType::PRIMARY) : ret;
    } else if (operation_ == "Stop") {
        ret = Stop(EncoderType::PRIMARY);
        return ret == AV_ERR_OK ? Start(EncoderType::PRIMARY) : ret;
    } else if (operation_ == "Reset") {
        ret = Reset(EncoderType::PRIMARY);
        return ret == AV_ERR_OK ? Start(EncoderType::PRIMARY) : ret;
    } else if (operation_ == "GetOutputDescription") {
        auto format = GetOutputDescription(EncoderType::PRIMARY);
        return format == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
    } else if (operation_ == "SetCallback") {
        if (primaryEncoder_.isAVBufferMode) {
            return RegisterCallback(EncoderType::PRIMARY, primaryEncoder_.callback, primaryEncoder_.signal);
        } else {
            return SetCallback(EncoderType::PRIMARY, primaryEncoder_.asyncCallback, primaryEncoder_.signal);
        }
    }
    UNITTEST_INFO_LOG("unknown Operate: %s", operation_.c_str());
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncDualOutputSample::PushInputData(EncoderContext &encoderContext, shared_ptr<CodecBufferInfo> bufferInfo)
{
    UNITTEST_INFO_LOG("index:%d", bufferInfo->GetIndex());
    if (encoderContext.signal->isInEos_) {
        if (!encoderContext.isFirstEos) {
            UNITTEST_INFO_LOG("At Eos State");
            return AV_ERR_OK;
        }
        encoderContext.isFirstEos = false;
    }
    int32_t ret = AV_ERR_OK;
    if (encoderContext.isAVBufferMode) {
        ret = OH_VideoEncoder_PushInputBuffer(encoderContext.codec, bufferInfo->GetIndex());
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_PushInputBuffer failed");
    } else {
        ret = OH_VideoEncoder_PushInputData(encoderContext.codec, bufferInfo->GetIndex(), bufferInfo->GetAttr());
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_PushInputData failed");
    }
    encoderContext.frameInputCount_++;
    usleep(DEFAULT_TIME_INTERVAL);
    return AV_ERR_OK;
}

int32_t VideoEncDualOutputSample::ReleaseOutputData(EncoderContext &encoderContext, shared_ptr<CodecBufferInfo> bufferInfo)
{
    UNITTEST_INFO_LOG("index:%d", bufferInfo->GetIndex());
    int32_t ret;
    if (encoderContext.isAVBufferMode) {
        ret = OH_VideoEncoder_FreeOutputBuffer(encoderContext.codec, bufferInfo->GetIndex());
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_FreeOutputBuffer failed");
    } else {
        ret = OH_VideoEncoder_FreeOutputData(encoderContext.codec, bufferInfo->GetIndex());
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_FreeOutputData failed");
    }
    encoderContext.frameOutputCount_++;
    return AV_ERR_OK;
}

int32_t VideoEncDualOutputSample::HandleInputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(addr != nullptr, AV_ERR_UNKNOWN, "in buffer is nullptr");
    attr.offset = 0;
    attr.pts = GetTimeUs();
    if (frameCount_ <= primaryEncoder_.frameInputCount_) {
        primaryEncoder_.signal->isInEos_ = true;
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        attr.size = 0;
        UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d", attr.size, (int32_t)(attr.flags));
        return AV_ERR_OK;
    }
    if (primaryEncoder_.signal->inFile_->eof()) {
        (void)primaryEncoder_.signal->inFile_->seekg(0);
    }
    int32_t width = 0;
    int32_t height = 0;
    int32_t stride = 0;
    auto format = GetInputDescription(EncoderType::PRIMARY);
    OH_AVFormat_GetIntValue(format.get(), Media::Tag::VIDEO_WIDTH, &width);
    OH_AVFormat_GetIntValue(format.get(), Media::Tag::VIDEO_HEIGHT, &height);
    OH_AVFormat_GetIntValue(format.get(), Media::Tag::VIDEO_STRIDE, &stride);

    attr.size = GetReadOneFrame(samplePixel_)(primaryEncoder_.signal, addr, width, height, stride);
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    if (primaryEncoder_.isFirstFrame) {
        attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        primaryEncoder_.isFirstFrame = false;
    }
    uint64_t *addr64 = reinterpret_cast<uint64_t *>(addr);
    UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d, addr[0]:%" PRIX64, attr.size, (int32_t)(attr.flags), addr64[0]);
    return AV_ERR_OK;
}

int32_t VideoEncDualOutputSample::HandleOutputFrameInner(uint8_t *addr, OH_AVCodecBufferAttr &attr)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(addr != nullptr, AV_ERR_UNKNOWN, "out buffer is nullptr");

    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        UNITTEST_INFO_LOG("out frame:%d, in frame:%d", primaryEncoder_.frameOutputCount_.load(), primaryEncoder_.frameInputCount_.load());
        primaryEncoder_.signal->isOutEos_ = true;
        primaryEncoder_.signal->eosCond_.notify_all();
        return AV_ERR_OK;
    }
    if (needDump_ && primaryEncoder_.signal != nullptr && primaryEncoder_.signal->outFile_ != nullptr &&
        primaryEncoder_.signal->outFile_->is_open() && primaryEncoder_.frameOutputCount_ < MAX_OUTPUT_FRAMENUM) {
        (void)primaryEncoder_.signal->outFile_->write(reinterpret_cast<char *>(addr), attr.size);
        (void)primaryEncoder_.signal->outFile_->flush();
    }
    uint64_t *addr64 = reinterpret_cast<uint64_t *>(addr);
    UNITTEST_INFO_LOG("attr.size: %d, attr.flags: %d, addr[0]:%" PRIX64, attr.size, (int32_t)(attr.flags), addr64[0]);
    return AV_ERR_OK;
}

int32_t VideoEncDualOutputSample::HandleOutputFrameSecondaryInner(uint8_t *addr, OH_AVCodecBufferAttr &attr)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(addr != nullptr, AV_ERR_UNKNOWN, "secondary out buffer is nullptr");

    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        UNITTEST_INFO_LOG("secondary out frame:%d", secondaryEncoder_.frameOutputCount_.load());
        if (secondaryEncoder_.signal != nullptr) {
            secondaryEncoder_.signal->isOutEos_ = true;
            secondaryEncoder_.signal->eosCond_.notify_all();
        }
        return AV_ERR_OK;
    }
    if (needDump_ && secondaryEncoder_.signal != nullptr && secondaryEncoder_.signal->outFile_ != nullptr &&
        secondaryEncoder_.signal->outFile_->is_open() && secondaryEncoder_.frameOutputCount_ < MAX_OUTPUT_FRAMENUM) {
        (void)secondaryEncoder_.signal->outFile_->write(reinterpret_cast<char *>(addr), attr.size);
        (void)secondaryEncoder_.signal->outFile_->flush();
    }
    uint64_t *addr64 = reinterpret_cast<uint64_t *>(addr);
    UNITTEST_INFO_LOG("sec attr.size: %d, attr.flags: %d, addr[0]:%" PRIX64,
                      attr.size, (int32_t)(attr.flags), addr64[0]);
    return AV_ERR_OK;
}

int32_t VideoEncDualOutputSample::ReleaseOutputDataSecondary(EncoderContext &encoderContext, shared_ptr<CodecBufferInfo> bufferInfo)
{
    UNITTEST_INFO_LOG("secondary free index:%d", bufferInfo->GetIndex());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(encoderContext.codec != nullptr, AV_ERR_INVALID_VAL, "codec is nullptr");
    int32_t ret = OH_VideoEncoder_FreeOutputBuffer(encoderContext.codec, bufferInfo->GetIndex());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "OH_VideoEncoder_FreeOutputBuffer failed for secondary");
    encoderContext.frameOutputCount_++;
    return AV_ERR_OK;
}

bool VideoEncDualOutputSample::CreateSurface()
{
    TITLE_LOG;
    UNITTEST_CHECK_AND_RETURN_RET_LOG(primaryEncoder_.codec != nullptr, false, "primary codec is nullptr");
    int32_t ret = OH_VideoEncoder_GetSurface(primaryEncoder_.codec, &nativeWindow_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, false, "OH_VideoEncoder_GetSurface failed");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(primaryEncoder_.signal != nullptr && primaryEncoder_.signal->inFile_ != nullptr,
                                      false, "primary signal or inFile_ is nullptr");

    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, false, "NativeWindowHandleOpt SET_FORMAT failed, Error:%d", ret);

    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_BUFFER_GEOMETRY, sampleWidth_, sampleHeight_);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, false, "NativeWindowHandleOpt SET_BUFFER_GEOMETRY failed, Error:%d", ret);

    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_USAGE,
                                                BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, false, "NativeWindowHandleOpt SET_USAGE failed, Error:%d", ret);

    return true;
}

void VideoEncDualOutputSample::InputFuncSurface()
{
    UNITTEST_INFO_LOG("InputFuncSurface started, frameCount_: %d", frameCount_);
    while (primaryEncoder_.signal->isRunning_.load()) {
        OHNativeWindowBuffer *ohNativeWindowBuffer = nullptr;
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
            UNITTEST_INFO_LOG("OH_NativeBuffer_Map failed, GSError=%d", err);
            primaryEncoder_.signal->isRunning_.store(false);
            break;
        }
        char *dst = static_cast<char *>(virAddr);
        const SurfaceBuffer *sbuffer = SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);
        int32_t stride = sbuffer->GetStride();
        if (dst == nullptr || stride < sampleWidth_) {
            UNITTEST_INFO_LOG("invalid va or stride=%d", stride);
            err = NativeWindowCancelBuffer(nativeWindow_, ohNativeWindowBuffer);
            UNITTEST_CHECK_AND_INFO_LOG(err == 0, "NativeWindowCancelBuffer failed");
            primaryEncoder_.signal->isRunning_.store(false);
            break;
        }
        if (primaryEncoder_.signal->inFile_->eof()) {
            (void)primaryEncoder_.signal->inFile_->seekg(0);
        }
        int32_t bufferSize = stride * sampleHeight_ * 3 / 2; // 3: nom, 2: denom
        for (int32_t i = 0; i < bufferSize; i += stride) {
            (void)primaryEncoder_.signal->inFile_->read(dst + i, sampleWidth_);
        }
        if (primaryEncoder_.frameInputCount_ >= static_cast<int32_t>(frameCount_)) {
            err = OH_NativeBuffer_Unmap(nativeBuffer);
            (void)err;
            err = NativeWindowCancelBuffer(nativeWindow_, ohNativeWindowBuffer);
            (void)err;
            // Notify EOS for both primary and secondary
            err = OH_VideoEncoder_NotifyEndOfStream(primaryEncoder_.codec);
            UNITTEST_CHECK_AND_INFO_LOG(err == AV_ERR_OK, "Primary NotifyEos failed, ret=%d", err);
            if (secondaryEncoder_.codec != nullptr) {
                err = OH_VideoEncoder_NotifyEndOfStream(secondaryEncoder_.codec);
                UNITTEST_CHECK_AND_INFO_LOG(err == AV_ERR_OK, "Secondary NotifyEos failed, ret=%d", err);
            }
            break;
        }
        err = FlushSurface(ohNativeWindowBuffer, nativeBuffer);
        UNITTEST_CHECK_AND_BREAK_LOG(err == 0, "FlushSurface failed, GSError=%d", err);
        primaryEncoder_.frameInputCount_++;
        usleep(DEFAULT_TIME_INTERVAL);
    }
    UNITTEST_INFO_LOG("InputFuncSurface exited, frameInputCount_: %d", primaryEncoder_.frameInputCount_.load());
}

int32_t VideoEncDualOutputSample::FlushSurface(OHNativeWindowBuffer *ohNativeWindowBuffer,
                                                OH_NativeBuffer *nativeBuffer)
{
    struct Region region;
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0;
    rect->y = 0;
    rect->w = sampleWidth_;
    rect->h = sampleHeight_;
    region.rects = rect;

    int64_t systemTimeUs = GetTimeUs();
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_UI_TIMESTAMP, systemTimeUs);
    int32_t ret = OH_NativeBuffer_Unmap(nativeBuffer);
    if (ret != 0) {
        UNITTEST_INFO_LOG("OH_NativeBuffer_Unmap failed");
        delete rect;
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow_, ohNativeWindowBuffer, -1, region);
    delete rect;
    return ret;
}

void VideoEncDualOutputSample::StopSurfaceInput()
{
    if (primaryEncoder_.signal != nullptr) {
        primaryEncoder_.signal->isRunning_.store(false);
    }
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        inputLoop_->join();
        inputLoop_.reset();
    }
}

} // namespace MediaAVCodec
} // namespace OHOS
