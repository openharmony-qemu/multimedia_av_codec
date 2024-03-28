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

#include "video_decoder_perf_test_sample.h"
#include <chrono>
#include "refbase.h"
#include "window.h"
#include "surface.h"

#include "avcodec_trace.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"
#include "sample_helper.h"

namespace {
using namespace std::chrono_literals;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoDecoderSample"};
constexpr uint8_t AVCC_FRAME_HEAD_LEN = 4;
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
VideoDecoderPerfTestSample::~VideoDecoderPerfTestSample()
{
    StartRelease();
    if (releaseThread_ && releaseThread_->joinable()) {
        releaseThread_->join();
    }
}

int32_t VideoDecoderPerfTestSample::Create(SampleInfo sampleInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!isStarted_, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(videoDecoder_ == nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    
    sampleInfo_ = sampleInfo;
    
    videoDecoder_ = std::make_unique<VideoDecoder>();
    CHECK_AND_RETURN_RET_LOG(videoDecoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR,
        "Create video decoder failed, no memory");

    int32_t ret = videoDecoder_->Create(sampleInfo_.codecMime);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create video decoder failed");

    context_ = new CodecUserData;
    context_->sampleInfo = &sampleInfo_;
    if (!(sampleInfo_.codecRunMode & 0b01)) { // 0b01: Buffer mode mask
        ret = CreateWindow(sampleInfo_.window);
        CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create window failed");
    }
    ret = videoDecoder_->Config(sampleInfo_, context_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Decoder config failed");

    releaseThread_ = nullptr;
    AVCODEC_LOGI("Succeed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoderPerfTestSample::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!isStarted_, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(videoDecoder_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
        
    int32_t ret = videoDecoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Decoder start failed");

    isStarted_ = true;
    inputFile_ = std::make_unique<std::ifstream>(sampleInfo_.inputFilePath.data(), std::ios::binary | std::ios::in);
    inputThread_ = std::make_unique<std::thread>(&VideoDecoderPerfTestSample::InputThread, this);
    outputThread_ = std::make_unique<std::thread>(&VideoDecoderPerfTestSample::OutputThread, this);
    if (inputThread_ == nullptr || outputThread_ == nullptr || !inputFile_->is_open()) {
        AVCODEC_LOGE("Create thread or open file failed");
        StartRelease();
        return AVCODEC_SAMPLE_ERR_ERROR;
    }

    AVCODEC_LOGI("Succeed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoderPerfTestSample::WaitForDone()
{
    AVCODEC_LOGI("In");
    std::unique_lock<std::mutex> lock(mutex_);
    doneCond_.wait(lock);
    AVCODEC_LOGI("Done");
    return AVCODEC_SAMPLE_ERR_OK;
}

void VideoDecoderPerfTestSample::StartRelease()
{
    if (releaseThread_ == nullptr) {
        AVCODEC_LOGI("Start to release");
        releaseThread_ = std::make_unique<std::thread>(&VideoDecoderPerfTestSample::Release, this);
    }
}

void VideoDecoderPerfTestSample::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (inputThread_ && inputThread_->joinable()) {
        inputThread_->join();
    }
    if (outputThread_ && outputThread_->joinable()) {
        outputThread_->join();
    }
    isStarted_ = false;
    if (videoDecoder_ != nullptr) {
        videoDecoder_->Release();
    }
    inputThread_.reset();
    outputThread_.reset();
    videoDecoder_.reset();

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

void VideoDecoderPerfTestSample::InputThread()
{
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

        int32_t ret = ReadOneFrame(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");

        ThreadSleep();

        ret = videoDecoder_->PushInputData(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Push data failed, thread out");
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Catch EOS, thread out");
    }
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->inputFrameCount_);
    StartRelease();
}

void VideoDecoderPerfTestSample::OutputThread()
{
    while (true) {
        CHECK_AND_BREAK_LOG(isStarted_, "Decoder output thread out");
        std::unique_lock<std::mutex> lock(context_->outputMutex_);
        bool condRet = context_->outputCond_.wait_for(lock, 5s,
            [this]() { return !isStarted_ || !context_->outputBufferInfoQueue_.empty(); });
        CHECK_AND_BREAK_LOG(isStarted_, "Decoder output thread out");
        CHECK_AND_CONTINUE_LOG(!context_->outputBufferInfoQueue_.empty(),
            "Buffer queue is empty, continue, cond ret: %{public}d", condRet);

        CodecBufferInfo bufferInfo = context_->outputBufferInfoQueue_.front();
        context_->outputBufferInfoQueue_.pop();
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Catch EOS, thread out");
        context_->outputFrameCount_++;
        AVCODEC_LOGV("Out buffer count: %{public}u, size: %{public}d, flag: %{public}u, pts: %{public}" PRId64,
            context_->outputFrameCount_, bufferInfo.attr.size, bufferInfo.attr.flags, bufferInfo.attr.pts);
        lock.unlock();

        DumpOutput(bufferInfo);

        int32_t ret = videoDecoder_->FreeOutputData(bufferInfo.bufferIndex, !(sampleInfo_.codecRunMode & 0b01));
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Decoder output thread out");
    }
    OHOS::MediaAVCodec::AVCodecTrace::TraceEnd("SampleWorkTime", FAKE_POINTER(this));
    OHOS::MediaAVCodec::AVCodecTrace::CounterTrace("SampleFrameCount", context_->outputFrameCount_);
    AVCODEC_LOGI("Exit, frame count: %{public}u", context_->outputFrameCount_);
    StartRelease();
}

bool VideoDecoderPerfTestSample::IsCodecData(const uint8_t *const bufferAddr)
{
    bool isH264Stream = sampleInfo_.codecMime == MIME_VIDEO_AVC;

    // 0x1F: avc nulu type mask; 0x7E: hevc nalu type mask
    uint8_t naluType = isH264Stream ?
        (bufferAddr[AVCC_FRAME_HEAD_LEN] & 0x1F) : ((bufferAddr[AVCC_FRAME_HEAD_LEN] & 0x7E) >> 1);

    constexpr uint8_t AVC_SPS = 7;
    constexpr uint8_t AVC_PPS = 8;
    constexpr uint8_t HEVC_VPS = 32;
    constexpr uint8_t HEVC_SPS = 33;
    constexpr uint8_t HEVC_PPS = 34;
    if ((isH264Stream && ((naluType == AVC_SPS) || (naluType == AVC_PPS))) ||
        (!isH264Stream && ((naluType == HEVC_VPS) || (naluType == HEVC_SPS) || (naluType == HEVC_PPS)))) {
        return true;
    }
    return false;
}

int32_t VideoDecoderPerfTestSample::ReadOneFrame(CodecBufferInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(inputFile_ != nullptr && inputFile_->is_open(),
        AVCODEC_SAMPLE_ERR_ERROR, "Input file is not open!");

    if (context_->inputFrameCount_ > sampleInfo_.maxFrames) {
        info.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        return AVCODEC_SAMPLE_ERR_OK;
    }

    char ch[AVCC_FRAME_HEAD_LEN] = {};
    (void)inputFile_->read(ch, AVCC_FRAME_HEAD_LEN);
    // 0 1 2 3: avcc frame head byte offset; 8 16 24: avcc frame head bit offset
    uint32_t bufferSize = static_cast<uint32_t>(((ch[3] & 0xFF)) | ((ch[2] & 0xFF) << 8) |
        ((ch[1] & 0xFF) << 16) | ((ch[0] & 0xFF) << 24));

    auto bufferAddr = static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b10 ?    // 0b10: AVBuffer mode mask
                      OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(info.buffer)) :
                      OH_AVMemory_GetAddr(reinterpret_cast<OH_AVMemory *>(info.buffer));
    (void)inputFile_->read(reinterpret_cast<char *>(bufferAddr + AVCC_FRAME_HEAD_LEN), bufferSize);
    bufferAddr[0] = 0;
    bufferAddr[1] = 0;
    bufferAddr[2] = 0;      // 2: annexB frame head offset 2
    bufferAddr[3] = 1;      // 3: annexB frame head offset 3

    info.attr.flags = IsCodecData(bufferAddr) ? AVCODEC_BUFFER_FLAGS_CODEC_DATA : AVCODEC_BUFFER_FLAGS_NONE;
    if (inputFile_->eof()) {
        info.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    }
    info.attr.size = bufferSize + AVCC_FRAME_HEAD_LEN;
    info.attr.pts = context_->inputFrameCount_ *
        ((sampleInfo_.frameInterval == 0) ? 1 : sampleInfo_.frameInterval) * 1000; // 1000: 1ms to us

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t VideoDecoderPerfTestSample::CreateWindow(OHNativeWindow *&window)
{
    auto consumer_ = OHOS::Surface::CreateSurfaceAsConsumer();
    OHOS::sptr<OHOS::IBufferConsumerListener> listener = new SurfaceConsumer(consumer_, this);
    consumer_->RegisterConsumerListener(listener);
    auto producer = consumer_->GetProducer();
    auto surface = OHOS::Surface::CreateSurfaceAsProducer(producer);
    window = CreateNativeWindowFromSurface(&surface);
    CHECK_AND_RETURN_RET_LOG(window != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create window failed!");

    return AVCODEC_SAMPLE_ERR_OK;
}

void VideoDecoderPerfTestSample::ThreadSleep()
{
    if (sampleInfo_.frameInterval <= 0) {
        return;
    }

    using namespace std::chrono_literals;
    thread_local auto lastPushTime = std::chrono::system_clock::now();

    auto beforeSleepTime = std::chrono::system_clock::now();
    std::this_thread::sleep_until(lastPushTime + std::chrono::milliseconds(sampleInfo_.frameInterval));
    lastPushTime = std::chrono::system_clock::now();
    AVCODEC_LOGV("Sleep time: %{public}2.2fms",
        static_cast<std::chrono::duration<double, std::milli>>(lastPushTime - beforeSleepTime).count());
}

void VideoDecoderPerfTestSample::DumpOutput(uint8_t *bufferAddr, uint32_t bufferSize)
{
    if (outputFile_ == nullptr) {
        using namespace std::string_literals;
        auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string outputName = "VideoDecoderOut_"s +
            ToString(static_cast<OH_AVPixelFormat>(sampleInfo_.pixelFormat)) + "_" +
            std::to_string(sampleInfo_.videoWidth) + "_" + std::to_string(sampleInfo_.videoHeight) + "_" +
            std::to_string(time) + ".yuv";

        outputFile_ = std::make_unique<std::ofstream>(outputName, std::ios::out | std::ios::trunc);
        if (!outputFile_->is_open()) {
            outputFile_ = nullptr;
        }
    }
    outputFile_->write(reinterpret_cast<char *>(bufferAddr), bufferSize);
}

void VideoDecoderPerfTestSample::DumpOutput(const CodecBufferInfo &bufferInfo)
{
    if (!(sampleInfo_.needDumpOutput) || !(sampleInfo_.codecRunMode & 0b01)) { // 0b01: Buffer mode mask
        return;
    }

    auto bufferAddr = static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b10 ?    // 0b10: AVBuffer mode mask
                      OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer)) :
                      OH_AVMemory_GetAddr(reinterpret_cast<OH_AVMemory *>(bufferInfo.buffer));

    DumpOutput(bufferAddr, bufferInfo.attr.size);
}

void VideoDecoderPerfTestSample::SurfaceConsumer::OnBufferAvailable()
{
    if (sample_ == nullptr) {
        return;
    }
    OHOS::sptr<OHOS::SurfaceBuffer> buffer;
    int32_t flushFence;
    surface_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);

    if (sample_->sampleInfo_.needDumpOutput) {
        sample_->DumpOutput(reinterpret_cast<uint8_t *>(buffer->GetVirAddr()), buffer->GetSize());
    }
    surface_->ReleaseBuffer(buffer, -1);
}
} // Sample
} // MediaAVCodec
} // OHOS