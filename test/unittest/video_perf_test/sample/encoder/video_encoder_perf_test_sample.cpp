#include "video_encoder_perf_test_sample.h"
#include <unistd.h>
#include <chrono>
#include <memory>
#include "external_window.h"
#include "native_buffer_inner.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"
#include "avcodec_trace.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoEncoderSample"};
}

VideoEncoderPerfTestSample::~VideoEncoderPerfTestSample() 
{
    StartRelease();
    if (releaseThread_ && releaseThread_->joinable()) {
        releaseThread_->join();
    }
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
    
    context_ = new CodecUserData;
    ret = videoEncoder_->Config(sampleInfo_, context_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Encoder config failed");

    releaseThread_ = nullptr;
    AVCODEC_LOGI("Succeed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoEncoderPerfTestSample::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!isStarted_, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(videoEncoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");

    int32_t ret = videoEncoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Encoder start failed");

    isStarted_ = true;
    inputFile_ = std::make_unique<std::ifstream>(sampleInfo_.inputFilePath.data(), std::ios::binary | std::ios::in);
    inputThread_ = (static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b01) ?  // 0b01: Buffer mode mask
        std::make_unique<std::thread>(&VideoEncoderPerfTestSample::BufferInputThread, this) :
        std::make_unique<std::thread>(&VideoEncoderPerfTestSample::SurfaceInputThread, this);
    outputThread_ = std::make_unique<std::thread>(&VideoEncoderPerfTestSample::OutputThread, this);
    if (inputThread_ == nullptr || outputThread_ == nullptr || !inputFile_->is_open()) {
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

    if (inputThread_ && inputThread_->joinable()) {
        inputThread_->join();
    }
    if (outputThread_ && outputThread_->joinable()) {
        outputThread_->join();
    }
    if (videoEncoder_ != nullptr) {
        videoEncoder_->Release();
    }
    inputThread_.reset();
    outputThread_.reset();
    videoEncoder_.reset();

    if (sampleInfo_.window != nullptr) {
        OH_NativeWindow_DestroyNativeWindow(sampleInfo_.window);
        sampleInfo_.window = nullptr;
    }
    if (context_ != nullptr) {
        delete context_;
        context_ = nullptr;
    }
    if (inputFile_ != nullptr) {
        inputFile_.reset();
    }

    AVCODEC_LOGI("Succeed");
    doneCond_.notify_all();
}

void VideoEncoderPerfTestSample::BufferInputThread()
{
    using namespace std::chrono_literals;
    auto lastPushTime = std::chrono::system_clock::now();
    OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("SampleWorkTime", FAKE_POINTER(this));
    while (true) {
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");
        std::unique_lock<std::mutex> lock(context_->inputMutex_);
        bool condRet = context_->inputCond_.wait_for(lock, 5s,
            [this]() { return !isStarted_ || !context_->inputBufferInfoQueue_.empty(); });
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");
        CHECK_AND_CONTINUE_LOG(!context_->inputBufferInfoQueue_.empty(),
            "Buffer queue is empty, continue, cond ret: %{public}d", condRet);

        CodecBufferInfo bufferInfo = context_->inputBufferInfoQueue_.front();
        context_->inputBufferInfoQueue_.pop();
        context_->inputFrameCount_++;
        lock.unlock();

        bufferInfo.attr.pts = context_->inputFrameCount_ *
            ((sampleInfo_.frameInterval == 0) ? 1 : sampleInfo_.frameInterval) * 1000; // 1000: 1ms to us
        
        int32_t ret = ReadOneFrame(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");

        if (sampleInfo_.testMode == TestMode::FRAME_DELAY) {
            auto beforeSleepTime = std::chrono::system_clock::now();
            std::this_thread::sleep_until(lastPushTime + std::chrono::milliseconds(sampleInfo_.frameInterval));
            lastPushTime = std::chrono::system_clock::now();
            AVCODEC_LOGV("Sleep time: %{public}2.2fms",
                static_cast<std::chrono::duration<double, std::milli>>(lastPushTime - beforeSleepTime).count());
        }

        ret = videoEncoder_->PushInputData(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Push data failed, thread out");
        CHECK_AND_BREAK_LOG(bufferInfo.attr.flags != AVCODEC_BUFFER_FLAGS_EOS, "Catch EOS, thread out");
    }
    StartRelease();
}

void VideoEncoderPerfTestSample::SurfaceInputThread()
{
    using namespace std::chrono_literals;
    auto lastPushTime = std::chrono::system_clock::now();
    OHNativeWindowBuffer *buffer = nullptr;
    OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("SampleWorkTime", FAKE_POINTER(this));
    while (true) {
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");
        int fenceFd = -1;
        int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(sampleInfo_.window, &buffer, &fenceFd);
        CHECK_AND_CONTINUE_LOG(ret == 0, "RequestBuffer failed, ret: %{public}d", ret);

        OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(buffer);
        uint8_t *bufferAddr = nullptr;
        ret = OH_NativeBuffer_Map(nativeBuffer, reinterpret_cast<void **>(&bufferAddr));
        CHECK_AND_BREAK_LOG(ret == 0, "Map native buffer failed, thread out");

        uint32_t flags = AVCODEC_BUFFER_FLAGS_NONE;
        ret = ReadOneFrame(bufferAddr, flags);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");
        CHECK_AND_BREAK_LOG(flags != AVCODEC_BUFFER_FLAGS_EOS, "Catch EOS, thread out");

        if (sampleInfo_.testMode == TestMode::FRAME_DELAY) {
            auto beforeSleepTime = std::chrono::system_clock::now();
            std::this_thread::sleep_until(lastPushTime + std::chrono::milliseconds(sampleInfo_.frameInterval));
            lastPushTime = std::chrono::system_clock::now();
            AVCODEC_LOGV("Sleep time: %{public}2.2fms",
                static_cast<std::chrono::duration<double, std::milli>>(lastPushTime - beforeSleepTime).count());
        }

        context_->inputFrameCount_++;
        uint64_t pts = context_->inputFrameCount_ *
            ((sampleInfo_.frameInterval == 0) ? 1 : sampleInfo_.frameInterval) * 1000; // 1000: 1ms to us
        (void)NativeWindowHandleOpt(sampleInfo_.window, SET_UI_TIMESTAMP, pts);
        ret = OH_NativeBuffer_Unmap(nativeBuffer);
        CHECK_AND_BREAK_LOG(ret == 0, "Read frame failed, thread out");
        AddSurfaceInputTrace(flags, pts);

        ret = OH_NativeWindow_NativeWindowFlushBuffer(sampleInfo_.window, buffer, fenceFd, {nullptr, 0});
        CHECK_AND_BREAK_LOG(ret == 0, "Read frame failed, thread out");

        buffer = nullptr;
    }
    if (buffer != nullptr) {
        OH_NativeWindow_DestroyNativeWindowBuffer(buffer);
    }
    videoEncoder_->NotifyEndOfStream();
    StartRelease();
}

void VideoEncoderPerfTestSample::OutputThread()
{
    using namespace std::chrono_literals;
    while (true) {
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");
        std::unique_lock<std::mutex> lock(context_->outputMutex_);
        bool condRet = context_->outputCond_.wait_for(lock, 5s,
            [this]() { return !isStarted_ || !context_->outputBufferInfoQueue_.empty(); });
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");
        CHECK_AND_CONTINUE_LOG(!context_->outputBufferInfoQueue_.empty(),
            "Buffer queue is empty, continue, cond ret: %{public}d", condRet);

        CodecBufferInfo bufferInfo = context_->outputBufferInfoQueue_.front();
        context_->outputBufferInfoQueue_.pop();
        context_->outputFrameCount_++;
        lock.unlock();

        CHECK_AND_BREAK_LOG(bufferInfo.attr.flags != AVCODEC_BUFFER_FLAGS_EOS, "Encoder output thread out");

        int32_t ret = videoEncoder_->FreeOutputData(bufferInfo.bufferIndex);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Encoder output thread out");
    }
    OHOS::MediaAVCodec::AVCodecTrace::TraceEnd("SampleWorkTime", FAKE_POINTER(this));
    AVCODEC_LOGI("On encoder output thread exit, output frame count: %{public}d", context_->outputFrameCount_);
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

void VideoEncoderPerfTestSample::AddSurfaceInputTrace(uint32_t flag, uint64_t pts)
{
    if (flag != AVCODEC_BUFFER_FLAGS_CODEC_DATA) {
        if (isFirstFrameIn_) {
            OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("OH::FirstFrame", pts);
            isFirstFrameIn_ = false;
        } else {
            OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("OH::Frame", pts);
        }
    }
}
