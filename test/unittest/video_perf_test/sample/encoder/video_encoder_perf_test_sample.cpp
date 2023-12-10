#include "video_encoder_perf_test_sample.h"
#include <unistd.h>
#include <chrono>
#include <memory>
#include "external_window.h"
#include "native_buffer_inner.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoEncoderSample"};
}

VideoEncoderPerfTestSample::~VideoEncoderPerfTestSample() 
{
    StartRelease();
}

int32_t VideoEncoderPerfTestSample::Create(SampleInfo sampleInfo) 
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!isStarted_, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(videoEncoder_ == nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    
    sampleInfo_ = sampleInfo;
    
    videoEncoder_ = std::make_unique<VideoEncoder>();
    CHECK_AND_RETURN_RET_LOG(videoEncoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR,
        "Create video encoder failed, no memory");

    int32_t ret = videoEncoder_->Create(sampleInfo_.codecMime);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create video encoder failed");
    
    encContext_ = new CodecUserData;
    ret = videoEncoder_->Config(sampleInfo_, encContext_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Encoder config failed");

    releaseThread_ = nullptr;
    AVCODEC_LOGI("Succeed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderPerfTestSample::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!isStarted_, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(encContext_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(videoEncoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");

    int32_t ret = videoEncoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Encoder start failed");

    isStarted_ = true;
    inputFile_ = std::make_unique<std::ifstream>(sampleInfo_.inputFilePath.data(), std::ios::binary | std::ios::in);
    encInputThread_ = (static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b01) ?  // 0b01: Buffer mode mask
        std::make_unique<std::thread>(&VideoEncoderPerfTestSample::encBufferInputThread, this) :
        std::make_unique<std::thread>(&VideoEncoderPerfTestSample::encSurfaceInputThread, this);
    encOutputThread_ = std::make_unique<std::thread>(&VideoEncoderPerfTestSample::encOutputThread, this);
    if (encInputThread_ == nullptr || encOutputThread_ == nullptr || !inputFile_->is_open()) {
        AVCODEC_LOGE("Create thread failed");
        StartRelease();
        return AVCODEC_SAMPLE_ERR_ERROR;
    }

    AVCODEC_LOGI("Succeed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderPerfTestSample::WaitForDone()
{
    AVCODEC_LOGI("In");
    std::unique_lock<std::mutex> lock(mutex_);
    doneCond_.wait(lock);
    AVCODEC_LOGI("Done");
    return AVCODEC_SAMPLE_ERR_OK;
}

void VideoEncoderPerfTestSample::StartRelease()
{
    if (releaseThread_ == nullptr) {
        AVCODEC_LOGI("Start release VideoEncoderPerfTestSample");
        releaseThread_ = std::make_unique<std::thread>(&VideoEncoderPerfTestSample::Release, this);
    }
}

void VideoEncoderPerfTestSample::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    isStarted_ = false;

    if (encInputThread_ && encInputThread_->joinable()) {
        encInputThread_->join();
    }
    if (encOutputThread_ && encOutputThread_->joinable()) {
        encOutputThread_->join();
    }
    if (videoEncoder_ != nullptr) {
        videoEncoder_->Release();
    }
    encInputThread_.reset();
    encOutputThread_.reset();
    videoEncoder_.reset();

    if (sampleInfo_.window != nullptr) {
        OH_NativeWindow_DestroyNativeWindow(sampleInfo_.window);
        sampleInfo_.window = nullptr;
    }
    if (encContext_ != nullptr) {
        delete encContext_;
        encContext_ = nullptr;
    }
    if (inputFile_ != nullptr) {
        inputFile_.reset();
    }

    AVCODEC_LOGI("Succeed");
    doneCond_.notify_all();
}

void VideoEncoderPerfTestSample::encBufferInputThread()
{
    using namespace std::chrono_literals;
    auto lastPushTime = std::chrono::system_clock::now();
    while (true) {
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");
        std::unique_lock<std::mutex> lock(encContext_->inputMutex_);
        bool condRet = encContext_->inputCond_.wait_for(lock, 2s,
            [this]() { return !isStarted_ || !encContext_->inputBufferInfoQueue_.empty(); });
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");
        CHECK_AND_CONTINUE_LOG(!encContext_->inputBufferInfoQueue_.empty(),
            "Buffer queue is empty, continue, cond ret: %{public}d", condRet);

        CodecBufferInfo bufferInfo = encContext_->inputBufferInfoQueue_.front();
        encContext_->inputBufferInfoQueue_.pop();
        lock.unlock();

        bufferInfo.attr.pts = encContext_->inputFrameCount_ * sampleInfo_.frameInterval * 1000; // 1000: 1ms to us
        
        int32_t ret = ReadOneFrame(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");

        if (sampleInfo_.testMode == TestMode::FRAME_DELAY) {
            std::this_thread::sleep_until(lastPushTime + std::chrono::milliseconds(sampleInfo_.frameInterval));
            lastPushTime = std::chrono::system_clock::now();
        }

        ret = videoEncoder_->PushInputData(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Push data failed, thread out");
        CHECK_AND_BREAK_LOG(bufferInfo.attr.flags != AVCODEC_BUFFER_FLAGS_EOS, "Catch EOS, thread out");
    }
    StartRelease();
}

void VideoEncoderPerfTestSample::encSurfaceInputThread()
{
    using namespace std::chrono_literals;
    auto lastPushTime = std::chrono::system_clock::now();
    while (true) {
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");

        OHNativeWindowBuffer *buffer;
        int fenceFd = -1;
        int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(sampleInfo_.window, &buffer, &fenceFd);
        CHECK_AND_CONTINUE_LOG(ret == 0, "RequestBuffer failed, ret: %{public}d", ret);
        std::shared_ptr<OHNativeWindowBuffer> bufferUP(buffer, OH_NativeWindow_DestroyNativeWindowBuffer);

        OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(buffer);
        uint8_t *bufferAddr = nullptr;
        ret = OH_NativeBuffer_Map(nativeBuffer, reinterpret_cast<void **>(&bufferAddr));
        CHECK_AND_BREAK_LOG(ret == 0, "Map native buffer failed, thread out");

        uint32_t flags = AVCODEC_BUFFER_FLAGS_NONE;
        ret = ReadOneFrame(bufferAddr, flags);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");
        CHECK_AND_BREAK_LOG(flags != AVCODEC_BUFFER_FLAGS_EOS, "Catch EOS, thread out");

        if (sampleInfo_.testMode == TestMode::FRAME_DELAY) {
            std::this_thread::sleep_until(lastPushTime + std::chrono::milliseconds(sampleInfo_.frameInterval));
            lastPushTime = std::chrono::system_clock::now();
        }

        encContext_->inputFrameCount_++;
        uint64_t pts = encContext_->inputFrameCount_ * sampleInfo_.frameInterval * 1000; // 1000: 1ms to us
        (void)NativeWindowHandleOpt(sampleInfo_.window, SET_UI_TIMESTAMP, pts);
        ret = OH_NativeBuffer_Unmap(nativeBuffer);
        CHECK_AND_BREAK_LOG(ret == 0, "Read frame failed, thread out");

        ret = OH_NativeWindow_NativeWindowFlushBuffer(sampleInfo_.window, buffer, fenceFd, {nullptr, 0});
        CHECK_AND_BREAK_LOG(ret == 0, "Read frame failed, thread out");
    }
    videoEncoder_->NotifyEndOfStream();
    StartRelease();
}

void VideoEncoderPerfTestSample::encOutputThread()
{
    while (true) {
        CHECK_AND_BREAK_LOG(isStarted_, "Encoder output thread out");
        std::unique_lock<std::mutex> lock(encContext_->outputMutex_);
        encContext_->outputCond_.wait(
            lock, [this]() { return (!encContext_->outputBufferInfoQueue_.empty() || !isStarted_); }
        );
        CHECK_AND_BREAK_LOG(isStarted_, "Encoder output thread out");

        CodecBufferInfo bufferInfo = encContext_->outputBufferInfoQueue_.front();
        encContext_->outputBufferInfoQueue_.pop();
        lock.unlock();

        CHECK_AND_BREAK_LOG(bufferInfo.attr.flags != AVCODEC_BUFFER_FLAGS_EOS, "Encoder output thread out");

        int32_t ret = videoEncoder_->FreeOutputData(bufferInfo.bufferIndex);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Encoder output thread out");
    }
    StartRelease();
}

int32_t VideoEncoderPerfTestSample::ReadOneFrame(CodecBufferInfo &info)
{
    auto bufferAddr = static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b10 ?    // 0b10: AVBuffer mode mask
                      OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(info.buffer)) :
                      OH_AVMemory_GetAddr(reinterpret_cast<OH_AVMemory *>(info.buffer));
    int32_t ret = ReadOneFrame(bufferAddr, info.attr.flags);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Read frame failed");

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderPerfTestSample::ReadOneFrame(uint8_t *bufferAddr, uint32_t &flags)
{
    CHECK_AND_RETURN_RET_LOG(inputFile_ != nullptr && inputFile_->is_open(),
        AVCODEC_SAMPLE_ERR_ERROR, "Input file is not open!");
    CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Invalid buffer address");

    uint64_t bufferSize = sampleInfo_.videoWidth * sampleInfo_.videoHeight * 3 / 2;     // YUV buffer size
    inputFile_->read(reinterpret_cast<char *>(bufferAddr), bufferSize);

    flags = inputFile_->eof() ? AVCODEC_BUFFER_FLAGS_EOS : AVCODEC_BUFFER_FLAGS_NONE;

    return AVCODEC_SAMPLE_ERR_OK;
}