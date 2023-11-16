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

#include "tester_common.h"
#include "native_avcodec_base.h"
#include "ui/rs_surface_node.h"  // foundation/graphic/graphic_2d/rosen/modules/render_service_client/core/
#include "utils.h"
#include "hcodec_api.h"
#include "hcodec_log.h"
#include "type_converter.h"
#include "tester_codecbase.h"
#include "tester_capi.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::Rosen;
using namespace Media;

int64_t TesterCommon::GetNowUs()
{
    auto now = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::microseconds>(now.time_since_epoch()).count();
}

bool TesterCommon::Run(const CommandOpt& opt)
{
    opt.Print();
    shared_ptr<TesterCommon> tester;
    if (opt.testType == DemoType::TEST_CODEC_BASE) {
        tester = make_shared<TesterCodecBase>(opt);
    } else {
        tester = make_shared<TesterCapi>(opt);
    }
    CostRecorder::Instance().Clear();
    for (uint32_t i = 0; i < opt.repeatCnt; i++) {
        printf("i = %u\n", i);
        bool ret = tester->RunOnce();
        if (!ret) {
            return false;
        }
    }
    CostRecorder::Instance().Print();
    return true;
}

bool TesterCommon::RunOnce()
{
    return opt_.isEncoder ? RunEncoder() : RunDecoder();
}

void TesterCommon::OutputLoop()
{
    while (true) {
        optional<uint32_t> outputIdx = GetOutputIndex();
        if (!outputIdx.has_value()) {
            return;
        }
        ReturnOutput(outputIdx.value());
    }
}

bool TesterCommon::RunEncoder()
{
    ifs_ = ifstream(opt_.inputFile, ios::binary);
    IF_TRUE_RETURN_VAL_WITH_MSG(!ifs_, false, "Failed to open file %{public}s", opt_.inputFile.c_str());
    optional<GraphicPixelFormat> displayFmt = TypeConverter::InnerFmtToDisplayFmt(opt_.pixFmt);
    IF_TRUE_RETURN_VAL_WITH_MSG(!displayFmt, false, "invalid pixel format");
    displayFmt_ = displayFmt.value();

    bool ret = Create();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = SetCallback();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = ConfigureEncoder();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = GetInputFormat();
    if (opt_.isBufferMode) {
        stride_ = opt_.dispW;
        if (ret) {
            optional<uint32_t> stride = GetInputStride();
            if (stride && stride.value() >= opt_.dispW) {
                stride_ = stride.value();
            }
            if (opt_.pixFmt == RGBA) {
                stride_ *= BYTES_PER_PIXEL_RBGA;
            }
        }
    } else {
        ret = CreateInputSurface();
        IF_TRUE_RETURN_VAL(!ret, false);
    }
    GetOutputFormat();
    ret = Start();
    IF_TRUE_RETURN_VAL(!ret, false);

    thread th(&TesterCommon::OutputLoop, this);
    if (opt_.isBufferMode) {
        if (opt_.testType == DemoType::TEST_C_API_USING_SHARED_MEM) {
            EncoderInputLoopForAsharedMem();
        } else {
            EncoderInputLoopForAvBuffer();
        }
    } else {
        InputSurfaceLoop();
    }
    if (th.joinable()) {
        th.join();
    }
    ret = Stop();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = Release();
    IF_TRUE_RETURN_VAL(!ret, false);
    return true;
}

void TesterCommon::EncoderInputLoopForAsharedMem()
{
    while (true) {
        Span span;
        optional<uint32_t> inputIdx = GetInputIndexForAsharedMem(span);
        if (!inputIdx.has_value()) {
            continue;
        }
        OH_AVCodecBufferAttr info;
        info.offset = 0;
        info.flags = 0;
        info.size = ReadOneFrame(opt_.pixFmt, span);
        if (info.size == 0 || (opt_.inputCnt > 0 && currInputCnt_ > opt_.inputCnt)) {
            info.flags = AVCODEC_BUFFER_FLAGS_EOS;
            if (!QueueInputForAsharedMem(inputIdx.value(), info)) {
                continue;
            } else {
                LOGI("queue eos succ, quit loop");
                return;
            }
        }
        info.pts = GetNowUs();
        if (!QueueInputForAsharedMem(inputIdx.value(), info)) {
            continue;
        }
        currInputCnt_++;
        if (currInputCnt_ == opt_.numIdrFrame) {
            RequestIDR();
        }
    }
}

void TesterCommon::EncoderInputLoopForAvBuffer()
{
    while (true) {
        std::shared_ptr<AVBuffer> avBuffer;
        optional<uint32_t> inputIdx = GetInputIndexForAvBuffer(avBuffer);
        if (!inputIdx.has_value()) {
            continue;
        }
        if (avBuffer->memory_->GetMemoryType() != MemoryType::SURFACE_MEMORY) {
            LOGW("invalid input, not avsurfacebuffer");
            continue;
        }

        sptr<SurfaceBuffer> surfaceBuffer = avBuffer->memory_->GetSurfaceBuffer();
        if (surfaceBuffer == nullptr) {
            LOGE("invalid surfaceBuffer");
            continue;
        }
        char* dst = reinterpret_cast<char *>(avBuffer->memory_->GetAddr());
        uint32_t stride = static_cast<uint32_t>(surfaceBuffer->GetStride());
        if (dst == nullptr || stride < opt_.dispW) {
            LOGE("invalid va or stride %{public}d", stride);
            continue;
        }
        stride_ = stride;

        std::optional<VideoPixelFormat> pixFmt = TypeConverter::DisplayFmtToInnerFmt(
            static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat()));
        if (!pixFmt.has_value()) {
            LOGW("failed to get format from surfaceBuffer, use user opt");
            pixFmt = opt_.pixFmt;
        }

        avBuffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        (void)avBuffer->memory_->SetOffset(0);
        Span span {
            .va = dst,
            .capacity = static_cast<size_t>(avBuffer->memory_->GetCapacity())
        };
        uint32_t filledLen = ReadOneFrame(pixFmt.value(), span);
        (void)avBuffer->memory_->SetSize(static_cast<int32_t>(filledLen));
        if (filledLen == 0 || (opt_.inputCnt > 0 && currInputCnt_ > opt_.inputCnt)) {
            avBuffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
            if (!QueueInputForAvBuffer(inputIdx.value())) {
                continue;
            } else {
                LOGI("queue eos succ, quit loop");
                return;
            }
        }
        avBuffer->pts_ = GetNowUs();
        if (!QueueInputForAvBuffer(inputIdx.value())) {
            continue;
        }
        currInputCnt_++;
        if (currInputCnt_ == opt_.numIdrFrame) {
            RequestIDR();
        }
    }
}

void TesterCommon::InputSurfaceLoop()
{
    BufferRequestConfig cfg = {opt_.dispW, opt_.dispH, 32, displayFmt_,
                               BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA, 0, };

    while (true)  {
        sptr<SurfaceBuffer> surfaceBuffer;
        int32_t fence;
        GSError err = surface_->RequestBuffer(surfaceBuffer, fence, cfg);
        if (err != GSERROR_OK || surfaceBuffer == nullptr) {
            this_thread::sleep_for(10ms);
            continue;
        }
        char* dst = reinterpret_cast<char *>(surfaceBuffer->GetVirAddr());
        uint32_t stride = static_cast<uint32_t>(surfaceBuffer->GetStride());
        if (dst == nullptr || stride < opt_.dispW) {
            LOGE("invalid va or stride %{public}d", stride);
            surface_->CancelBuffer(surfaceBuffer);
            continue;
        }
        stride_ = stride;
        std::optional<VideoPixelFormat> pixFmt = TypeConverter::DisplayFmtToInnerFmt(
            static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat()));
        if (!pixFmt.has_value()) {
            LOGW("failed to get format from input surface, use user opt");
            pixFmt = opt_.pixFmt;
        }
        uint32_t filledLen = ReadOneFrame(pixFmt.value(), Span {dst, surfaceBuffer->GetSize()});
        if (filledLen == 0 || (opt_.inputCnt > 0 && currInputCnt_ > opt_.inputCnt)) {
            LOGI("input eos, quit loop");
            if (!NotifyEos()) {
                LOGE("NotifyEos failed");
            }
            return;
        }
        BufferFlushConfig flushConfig = {
            .damage = {
                .w = opt_.dispW,
                .h = opt_.dispH,
            },
            .timestamp = GetNowUs(),
        };
        err = surface_->FlushBuffer(surfaceBuffer, -1, flushConfig);
        if (err != GSERROR_OK) {
            LOGE("FlushBuffer failed");
            continue;
        }
        currInputCnt_++;
        if (currInputCnt_ == opt_.numIdrFrame) {
            RequestIDR();
        }
    }
}

#define RETURN_ZERO_IF_EOS(expectedSize) \
    do { \
        if (ifs_.gcount() != (expectedSize)) { \
            LOGI("no more data"); \
            return 0; \
        } \
    } while (0)

uint32_t TesterCommon::ReadOneFrameYUV420P(char* dst)
{
    char* start = dst;
    // copy Y
    for (uint32_t i = 0; i < opt_.dispH; i++) {
        ifs_.read(dst, opt_.dispW);
        RETURN_ZERO_IF_EOS(opt_.dispW);
        dst += stride_;
    }
    // copy U
    for (uint32_t i = 0; i < opt_.dispH / SAMPLE_RATIO; i++) {
        ifs_.read(dst, opt_.dispW / SAMPLE_RATIO);
        RETURN_ZERO_IF_EOS(opt_.dispW / SAMPLE_RATIO);
        dst += stride_ / SAMPLE_RATIO;
    }
    // copy V
    for (uint32_t i = 0; i < opt_.dispH / SAMPLE_RATIO; i++) {
        ifs_.read(dst, opt_.dispW / SAMPLE_RATIO);
        RETURN_ZERO_IF_EOS(opt_.dispW / SAMPLE_RATIO);
        dst += stride_ / SAMPLE_RATIO;
    }
    return dst - start;
}

uint32_t TesterCommon::ReadOneFrameYUV420SP(char* dst)
{
    char* start = dst;
    // copy Y
    for (uint32_t i = 0; i < opt_.dispH; i++) {
        ifs_.read(dst, opt_.dispW);
        RETURN_ZERO_IF_EOS(opt_.dispW);
        dst += stride_;
    }
    // copy UV
    for (uint32_t i = 0; i < opt_.dispH / SAMPLE_RATIO; i++) {
        ifs_.read(dst, opt_.dispW);
        RETURN_ZERO_IF_EOS(opt_.dispW);
        dst += stride_;
    }
    return dst - start;
}

uint32_t TesterCommon::ReadOneFrameRGBA(char* dst)
{
    char* start = dst;
    for (uint32_t i = 0; i < opt_.dispH; i++) {
        ifs_.read(dst, opt_.dispW * BYTES_PER_PIXEL_RBGA);
        RETURN_ZERO_IF_EOS(opt_.dispW * BYTES_PER_PIXEL_RBGA);
        dst += stride_;
    }
    return dst - start;
}

uint32_t TesterCommon::ReadOneFrame(VideoPixelFormat pixFmt, Span dstSpan)
{
    uint32_t sampleSize = 0;
    switch (pixFmt) {
        case YUVI420:
        case NV12:
        case NV21: {
            sampleSize = GetYuv420Size(stride_, opt_.dispH);
            break;
        }
        case RGBA: {
            sampleSize = stride_ * opt_.dispH;
            break;
        }
        default:
            return 0;
    }
    if (sampleSize > dstSpan.capacity) {
        LOGE("sampleSize %{public}u > dst capacity %{public}zu", sampleSize, dstSpan.capacity);
        return 0;
    }

    switch (pixFmt) {
        case YUVI420: {
            return ReadOneFrameYUV420P(dstSpan.va);
        }
        case NV12:
        case NV21: {
            return ReadOneFrameYUV420SP(dstSpan.va);
        }
        case RGBA: {
            return ReadOneFrameRGBA(dstSpan.va);
        }
        default:
            return 0;
    }
}

bool TesterCommon::RunDecoder()
{
    ifs_ = ifstream(opt_.inputFile, ios::binary);
    IF_TRUE_RETURN_VAL_WITH_MSG(!ifs_, false, "Failed to open file %{public}s", opt_.inputFile.c_str());
    totalSampleCnt_ = demuxer_.SetSource(opt_.inputFile, opt_.protocol);
    if (totalSampleCnt_ == 0) {
        LOGE("no nalu found");
        return false;
    }

    bool ret = Create();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = SetCallback();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = ConfigureDecoder();
    IF_TRUE_RETURN_VAL(!ret, false);
    if (!opt_.isBufferMode) {
        sptr<Surface> surface = CreateSurfaceNormal();
        if (surface == nullptr) {
            return false;
        }
        ret = SetOutputSurface(surface);
        IF_TRUE_RETURN_VAL(!ret, false);
    }
    GetInputFormat();
    GetOutputFormat();
    ret = Start();
    IF_TRUE_RETURN_VAL(!ret, false);

    thread outputThread(&TesterCommon::OutputLoop, this);
    if (opt_.testType == DemoType::TEST_C_API_USING_SHARED_MEM) {
        DecoderInputLoopForAsharedMem();
    } else {
        DecoderInputLoopForAvBuffer();
    }
    if (outputThread.joinable()) {
        outputThread.join();
    }

    ret = Stop();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = Release();
    IF_TRUE_RETURN_VAL(!ret, false);
    if (window_ != nullptr) {
        window_->Destroy();
        window_ = nullptr;
    }
    return true;
}

sptr<Surface> TesterCommon::CreateSurfaceFromWindow()
{
    sptr<WindowOption> option = new WindowOption();
    option->SetWindowRect({ 0, 0, opt_.dispW, opt_.dispH });
    option->SetWindowType(WindowType::WINDOW_TYPE_FLOAT);
    option->SetWindowMode(WindowMode::WINDOW_MODE_FLOATING);
    sptr<Window> window = Window::Create("DemoWindow", option);
    if (window == nullptr) {
        LOGE("Create Window failed");
        return nullptr;
    }
    shared_ptr<RSSurfaceNode> node = window->GetSurfaceNode();
    if (node == nullptr) {
        LOGE("GetSurfaceNode failed");
        return nullptr;
    }
    sptr<Surface> surface = node->GetSurface();
    if (surface == nullptr) {
        LOGE("GetSurface failed");
        return nullptr;
    }
    window->Show();
    window_ = window;
    return surface;
}

sptr<Surface> TesterCommon::CreateSurfaceNormal()
{
    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer();
    if (consumerSurface == nullptr) {
        LOGE("CreateSurfaceAsConsumer failed");
        return nullptr;
    }
    sptr<IBufferConsumerListener> listener = new Listener(this);
    GSError err = consumerSurface->RegisterConsumerListener(listener);
    if (err != GSERROR_OK) {
        LOGE("RegisterConsumerListener failed");
        return nullptr;
    }
    err = consumerSurface->SetDefaultUsage(BUFFER_USAGE_CPU_READ);
    if (err != GSERROR_OK) {
        LOGE("SetDefaultUsage failed");
        return nullptr;
    }
    sptr<IBufferProducer> bufferProducer = consumerSurface->GetProducer();
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(bufferProducer);
    if (producerSurface == nullptr) {
        LOGE("CreateSurfaceAsProducer failed");
        return nullptr;
    }
    surface_ = consumerSurface;
    return producerSurface;
}

void TesterCommon::Listener::OnBufferAvailable()
{
    sptr<SurfaceBuffer> buffer;
    int32_t fence;
    int64_t timestamp;
    OHOS::Rect damage;
    GSError err = tester_->surface_->AcquireBuffer(buffer, fence, timestamp, damage);
    if (err != GSERROR_OK || buffer == nullptr) {
        LOGW("AcquireBuffer failed");
        return;
    }
    tester_->surface_->ReleaseBuffer(buffer, -1);
}

void TesterCommon::DecoderInputLoopForAsharedMem()
{
    PrepareSeek();
    while (true) {
        if (!SeekIfNecessary()) {
            return;
        }
        Span span;
        optional<uint32_t> inputIdx = GetInputIndexForAsharedMem(span);
        if (!inputIdx.has_value()) {
            continue;
        }
        OH_AVCodecBufferAttr info;
        info.offset = 0;
        info.flags = 0;

        size_t sampleIdx;
        bool isCsd;
        info.size = GetNextSample(span, sampleIdx, isCsd);
        if (isCsd) {
            info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        }
        if (info.size == 0 || (opt_.inputCnt > 0 && currInputCnt_ > opt_.inputCnt)) {
            info.flags = AVCODEC_BUFFER_FLAGS_EOS;
            if (!QueueInputForAsharedMem(inputIdx.value(), info)) {
                LOGW("queue eos failed");
                continue;
            } else {
                LOGI("queue eos succ, quit loop");
                return;
            }
        }
        info.pts = GetNowUs();
        if (!QueueInputForAsharedMem(inputIdx.value(), info)) {
            LOGW("queue sample %{public}zu failed", sampleIdx);
            continue;
        }
        currInputCnt_++;
        currSampleIdx_ = sampleIdx;
        demuxer_.MoveToNext();
    }
}

void TesterCommon::DecoderInputLoopForAvBuffer()
{
    PrepareSeek();
    while (true) {
        if (!SeekIfNecessary()) {
            return;
        }
        std::shared_ptr<AVBuffer> avBuffer;
        optional<uint32_t> inputIdx = GetInputIndexForAvBuffer(avBuffer);
        if (!inputIdx.has_value()) {
            continue;
        }

        avBuffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        (void)avBuffer->memory_->SetOffset(0);
        size_t sampleIdx;
        bool isCsd;
        Span span {
            .va = reinterpret_cast<char*>(avBuffer->memory_->GetAddr()),
            .capacity = static_cast<size_t>(avBuffer->memory_->GetCapacity())
        };
        int sampleSize = GetNextSample(span, sampleIdx, isCsd);
        if (isCsd) {
            avBuffer->flag_ = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        }
        (void)avBuffer->memory_->SetSize(static_cast<int32_t>(sampleSize));
        if (sampleSize == 0 || (opt_.inputCnt > 0 && currInputCnt_ > opt_.inputCnt)) {
            avBuffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
            if (!QueueInputForAvBuffer(inputIdx.value())) {
                LOGW("queue eos failed");
                continue;
            } else {
                LOGI("queue eos succ, quit loop");
                return;
            }
        }
        avBuffer->pts_ = GetNowUs();
        if (!QueueInputForAvBuffer(inputIdx.value())) {
            LOGW("queue sample %{public}zu failed", sampleIdx);
            continue;
        }
        currInputCnt_++;
        currSampleIdx_ = sampleIdx;
        demuxer_.MoveToNext();
    }
}

static size_t GenerateRandomNumInRange(size_t rangeStart, size_t rangeEnd)
{
    return rangeStart + rand() % (rangeEnd - rangeStart);
}

void TesterCommon::PrepareSeek()
{
    int mockCnt = 0;
    size_t lastSeekTo = 0;
    while (mockCnt++ < opt_.flushCnt) {
        size_t seekFrom = GenerateRandomNumInRange(lastSeekTo, totalSampleCnt_);
        size_t seekTo = GenerateRandomNumInRange(0, totalSampleCnt_);
        LOGI("mock seek from sample index %{public}zu to %{public}zu", seekFrom, seekTo);
        userSeekPos_.emplace_back(seekFrom, seekTo);
        lastSeekTo = seekTo;
    }
}

bool TesterCommon::SeekIfNecessary()
{
    if (userSeekPos_.empty()) {
        return true;
    }
    size_t seekFrom;
    size_t seekTo;
    std::tie(seekFrom, seekTo) = userSeekPos_.front();
    if (currSampleIdx_ != seekFrom) {
        return true;
    }
    LOGI("begin to seek from sample index %{public}zu to %{public}zu", seekFrom, seekTo);
    if (!demuxer_.SeekTo(seekTo)) {
        return true;
    }
    if (!Flush()) {
        return false;
    }
    if (!Start()) {
        return false;
    }
    userSeekPos_.pop_front();
    return true;
}

int TesterCommon::GetNextSample(Span dstSpan, size_t& sampleIdx, bool& isCsd)
{
    optional<Sample> sample = demuxer_.PeekNextSample();
    if (!sample.has_value()) {
        return 0;
    }
    uint32_t sampleSize = sample->endPos - sample->startPos;
    if (sampleSize > dstSpan.capacity) {
        LOGE("sampleSize %{public}u > dst capacity %{public}zu", sampleSize, dstSpan.capacity);
        return 0;
    }
    sampleIdx = sample->idx;
    isCsd = sample->isCsd;
    ifs_.seekg(sample->startPos);
    ifs_.read(dstSpan.va, sampleSize);
    return sampleSize;
}
} // namespace OHOS::MediaAVCodec