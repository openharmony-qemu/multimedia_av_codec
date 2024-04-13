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
#include "hcodec_api.h"
#include "hcodec_log.h"
#include "hcodec_utils.h"
#include "native_avcodec_base.h"
#include "tester_capi.h"
#include "tester_codecbase.h"
#include "type_converter.h"
#include "ui/rs_surface_node.h" // foundation/graphic/graphic_2d/rosen/modules/render_service_client/core/

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::Rosen;
using namespace Media;

std::mutex TesterCommon::vividMtx_;
std::unordered_map<int64_t, std::vector<uint8_t>> TesterCommon::vividMap_;

int64_t TesterCommon::GetNowUs()
{
    auto now = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::microseconds>(now.time_since_epoch()).count();
}

shared_ptr<TesterCommon> TesterCommon::Create(const CommandOpt& opt)
{
    switch (opt.apiType) {
        case ApiType::TEST_CODEC_BASE:
            return make_shared<TesterCodecBase>(opt);
        case ApiType::TEST_C_API_OLD:
            return make_shared<TesterCapiOld>(opt);
        case ApiType::TEST_C_API_NEW:
            return make_shared<TesterCapiNew>(opt);
        default:
            return nullptr;
    }
}

bool TesterCommon::Run(const CommandOpt& opt)
{
    opt.Print();
    if (!opt.isEncoder && opt.decThenEnc) {
        return RunDecEnc(opt);
    }
    shared_ptr<TesterCommon> tester = Create(opt);
    CostRecorder::Instance().Clear();
    for (uint32_t i = 0; i < opt.repeatCnt; i++) {
        TLOGI("i = %u", i);
        bool ret = tester->RunOnce();
        if (!ret) {
            return false;
        }
    }
    CostRecorder::Instance().Print();
    return true;
}

bool TesterCommon::RunDecEnc(const CommandOpt& decOpt)
{
    {
        lock_guard<mutex> lk(vividMtx_);
        vividMap_.clear();
    }
    shared_ptr<TesterCommon> decoder = Create(decOpt);
    CommandOpt encOpt = decOpt;
    encOpt.isEncoder = true;
    shared_ptr<TesterCommon> encoder = Create(encOpt);

    bool ret = decoder->InitDemuxer();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = decoder->Create();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = decoder->SetCallback();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = decoder->ConfigureDecoder();
    IF_TRUE_RETURN_VAL(!ret, false);

    ret = encoder->Create();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = encoder->SetCallback();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = encoder->ConfigureEncoder();
    IF_TRUE_RETURN_VAL(!ret, false);

    sptr<Surface> surface = encoder->CreateInputSurface();
    IF_TRUE_RETURN_VAL(surface == nullptr, false);
    ret = decoder->SetOutputSurface(surface);
    IF_TRUE_RETURN_VAL(!ret, false);

    ret = encoder->Start();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = decoder->Start();
    IF_TRUE_RETURN_VAL(!ret, false);
    thread decOutThread(&TesterCommon::OutputLoop, decoder.get());
    thread encOutThread(&TesterCommon::OutputLoop, encoder.get());
    decoder->DecoderInputLoop();
    if (decOutThread.joinable()) {
        decOutThread.join();
    }
    encoder->NotifyEos();
    if (encOutThread.joinable()) {
        encOutThread.join();
    }
    decoder->Release();
    encoder->Release();
    TLOGI("RunDecEnc succ");
    return true;
}

bool TesterCommon::RunOnce()
{
    return opt_.isEncoder ? RunEncoder() : RunDecoder();
}

void TesterCommon::BeforeQueueInput(OH_AVCodecBufferAttr& attr)
{
    const char* codecStr = opt_.isEncoder ? "encoder" : "decoder";
    int64_t now = GetNowUs();
    attr.pts = now;
    if (!opt_.isEncoder && opt_.decThenEnc) {
        SaveVivid(attr.pts);
    }
    if (attr.flags & AVCODEC_BUFFER_FLAG_EOS) {
        TLOGI("%s input:  flags=0x%x (eos)", codecStr, attr.flags);
        return;
    }
    if (attr.flags & AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        TLOGI("%s input:  flags=0x%x, pts=%" PRId64 ", size=%d", codecStr, attr.flags, attr.pts, attr.size);
        return;
    }
    TLOGI("%s input:  flags=0x%x, pts=%" PRId64 ", size=%d", codecStr, attr.flags, attr.pts, attr.size);
    if (firstInTime_ == 0) {
        firstInTime_ = now;
    }
    inTotalCnt_++;
    int64_t fromFirstInToNow = now - firstInTime_;
    if (fromFirstInToNow != 0) {
        inFps_ = inTotalCnt_ * US_TO_S / fromFirstInToNow;
    }
}

void TesterCommon::AfterGotOutput(const OH_AVCodecBufferAttr& attr)
{
    const char* codecStr = opt_.isEncoder ? "encoder" : "decoder";
    int64_t now = GetNowUs();
    if (attr.flags & AVCODEC_BUFFER_FLAG_EOS) {
        TLOGI("%s output: flags=0x%x (eos)", codecStr, attr.flags);
        return;
    }
    if (attr.flags & AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        TLOGI("%s output: flags=0x%x, pts=%" PRId64 ", size=%d", codecStr, attr.flags, attr.pts, attr.size);
        return;
    }
    if (firstOutTime_ == 0) {
        firstOutTime_ = now;
    }
    outTotalCnt_++;

    int64_t fromInToOut = now - attr.pts;
    totalCost_ += fromInToOut;
    double oneFrameCostMs = fromInToOut / US_TO_MS;
    double averageCostMs = totalCost_ / US_TO_MS / outTotalCnt_;

    int64_t fromFirstOutToNow = now - firstOutTime_;
    if (fromFirstOutToNow == 0) {
        TLOGI("%s output: flags=0x%x, pts=%" PRId64 ", size=%d, cost %.2f ms, average %.2f ms",
               codecStr, attr.flags, attr.pts, attr.size, oneFrameCostMs, averageCostMs);
    } else {
        double outFps = outTotalCnt_ * US_TO_S / fromFirstOutToNow;
        TLOGI("%s output: flags=0x%x, pts=%" PRId64 ", size=%d, cost %.2f ms, average %.2f ms, "
               "in fps %.2f, out fps %.2f",
               codecStr, attr.flags, attr.pts, attr.size, oneFrameCostMs, averageCostMs, inFps_, outFps);
    }
}

void TesterCommon::OutputLoop()
{
    while (true) {
        BufInfo buf {};
        if (!WaitForOutput(buf)) {
            return;
        }
        if (opt_.isEncoder && opt_.decThenEnc) {
            CheckVivid(buf);
        }
        ReturnOutput(buf.idx);
    }
}

void TesterCommon::CheckVivid(const BufInfo& buf)
{
    vector<uint8_t> inVividSei;
    {
        lock_guard<mutex> lk(vividMtx_);
        auto it = vividMap_.find(buf.attr.pts);
        if (it == vividMap_.end()) {
            return;
        }
        inVividSei = std::move(it->second);
        vividMap_.erase(buf.attr.pts);
    }
    shared_ptr<StartCodeDetector> demuxer = StartCodeDetector::Create(opt_.protocol);
    demuxer->SetSource(buf.va, buf.attr.size);
    optional<Sample> sample = demuxer->PeekNextSample();
    if (!sample.has_value()) {
        TLOGI("--- output pts %" PRId64 " has no sample but input has vivid", buf.attr.pts);
        return;
    }
    if (sample->vividSei.empty()) {
        TLOGI("--- output pts %" PRId64 " has no vivid but input has vivid", buf.attr.pts);
        return;
    }
    bool eq = std::equal(inVividSei.begin(), inVividSei.end(), sample->vividSei.begin());
    if (eq) {
        TLOGI("--- output pts %" PRId64 " has vivid and is same as input vivid", buf.attr.pts);
    } else {
        TLOGI("--- output pts %" PRId64 " has vivid but is different from input vivid", buf.attr.pts);
    }
}

bool TesterCommon::RunEncoder()
{
    ifs_ = ifstream(opt_.inputFile, ios::binary);
    IF_TRUE_RETURN_VAL_WITH_MSG(!ifs_, false, "Failed to open file %s", opt_.inputFile.c_str());
    optional<GraphicPixelFormat> displayFmt = TypeConverter::InnerFmtToDisplayFmt(opt_.pixFmt);
    IF_TRUE_RETURN_VAL_WITH_MSG(!displayFmt, false, "invalid pixel format");
    displayFmt_ = displayFmt.value();

    bool ret = Create();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = SetCallback();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = ConfigureEncoder();
    IF_TRUE_RETURN_VAL(!ret, false);
    sptr<Surface> surface;
    if (!opt_.isBufferMode) {
        producerSurface_ = CreateInputSurface();
        IF_TRUE_RETURN_VAL(producerSurface_ == nullptr, false);
    }
    ret = Start();
    IF_TRUE_RETURN_VAL(!ret, false);
    GetInputFormat();
    GetOutputFormat();

    thread th(&TesterCommon::OutputLoop, this);
    EncoderInputLoop();
    if (th.joinable()) {
        th.join();
    }
    ret = Stop();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = Release();
    IF_TRUE_RETURN_VAL(!ret, false);
    return true;
}

void TesterCommon::EncoderInputLoop()
{
    while (true) {
        BufInfo buf {};
        bool ret = opt_.isBufferMode ? WaitForInput(buf) : WaitForInputSurfaceBuffer(buf);
        if (!ret) {
            continue;
        }
        buf.attr.size = ReadOneFrame(buf);
        if (buf.attr.size == 0) {
            buf.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
            BeforeQueueInput(buf.attr);
            ret = opt_.isBufferMode ? ReturnInput(buf) : NotifyEos();
            if (ret) {
                TLOGI("queue eos succ, quit loop");
                return;
            } else {
                TLOGW("queue eos failed");
                continue;
            }
        }
        if (!opt_.setParameterParamsMap.empty() && opt_.setParameterParamsMap.begin()->first == currInputCnt_) {
            const SetParameterParams &param = opt_.setParameterParamsMap.begin()->second;
            if (param.requestIdr.has_value()) {
                RequestIDR();
            }
            SetEncoderParameter(param);
            opt_.setParameterParamsMap.erase(opt_.setParameterParamsMap.begin());
        }
        if (opt_.isBufferMode && !opt_.perFrameParamsMap.empty() &&
            opt_.perFrameParamsMap.begin()->first == currInputCnt_) {
            SetEncoderPerFrameParam(buf, opt_.perFrameParamsMap.begin()->second);
            opt_.perFrameParamsMap.erase(opt_.perFrameParamsMap.begin());
        }
        BeforeQueueInput(buf.attr);
        ret = opt_.isBufferMode ? ReturnInput(buf) : ReturnInputSurfaceBuffer(buf);
        if (!ret) {
            continue;
        }
        if (opt_.enableInputCb) {
            WaitForInput(buf);
            if (!opt_.perFrameParamsMap.empty() && opt_.perFrameParamsMap.begin()->first == currInputCnt_) {
                SetEncoderPerFrameParam(buf, opt_.perFrameParamsMap.begin()->second);
                opt_.perFrameParamsMap.erase(opt_.perFrameParamsMap.begin());
            }
            ReturnInput(buf);
        }
        currInputCnt_++;
    }
}

bool TesterCommon::SurfaceBufferToBufferInfo(BufInfo& buf, sptr<SurfaceBuffer> surfaceBuffer)
{
    if (surfaceBuffer == nullptr) {
        TLOGE("null surfaceBuffer");
        return false;
    }
    buf.va = static_cast<uint8_t *>(surfaceBuffer->GetVirAddr());
    buf.capacity = surfaceBuffer->GetSize();

    buf.dispW = surfaceBuffer->GetWidth();
    buf.dispH = surfaceBuffer->GetHeight();
    buf.fmt = static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat());
    buf.byteStride = surfaceBuffer->GetStride();
    return true;
}

bool TesterCommon::NativeBufferToBufferInfo(BufInfo& buf, OH_NativeBuffer* nativeBuffer)
{
    if (nativeBuffer == nullptr) {
        TLOGE("null OH_NativeBuffer");
        return false;
    }
    OH_NativeBuffer_Config cfg;
    OH_NativeBuffer_GetConfig(nativeBuffer, &cfg);
    buf.dispW = cfg.width;
    buf.dispH = cfg.height;
    buf.fmt = static_cast<GraphicPixelFormat>(cfg.format);
    buf.byteStride = cfg.stride;
    return true;
}

bool TesterCommon::WaitForInputSurfaceBuffer(BufInfo& buf)
{
    BufferRequestConfig cfg = {opt_.dispW, opt_.dispH, 32, displayFmt_,
                               BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA, 0, };
    sptr<SurfaceBuffer> surfaceBuffer;
    int32_t fence;
    GSError err = producerSurface_->RequestBuffer(surfaceBuffer, fence, cfg);
    if (err != GSERROR_OK) {
        return false;
    }
    buf.surfaceBuf = surfaceBuffer;
    return SurfaceBufferToBufferInfo(buf, surfaceBuffer);
}

bool TesterCommon::ReturnInputSurfaceBuffer(BufInfo& buf)
{
    BufferFlushConfig flushConfig = {
        .damage = {
            .w = opt_.dispW,
            .h = opt_.dispH,
        },
        .timestamp = buf.attr.pts,
    };
    GSError err = producerSurface_->FlushBuffer(buf.surfaceBuf, -1, flushConfig);
    if (err != GSERROR_OK) {
        TLOGE("FlushBuffer failed");
        return false;
    }
    return true;
}

#define RETURN_ZERO_IF_EOS(expectedSize) \
    do { \
        if (ifs_.gcount() != (expectedSize)) { \
            TLOGI("no more data"); \
            return 0; \
        } \
    } while (0)

uint32_t TesterCommon::ReadOneFrameYUV420P(ImgBuf& dstImg)
{
    char* dst = reinterpret_cast<char*>(dstImg.va);
    char* start = dst;
    // copy Y
    for (uint32_t i = 0; i < dstImg.dispH; i++) {
        ifs_.read(dst, dstImg.dispW);
        RETURN_ZERO_IF_EOS(dstImg.dispW);
        dst += dstImg.byteStride;
    }
    // copy U
    for (uint32_t i = 0; i < dstImg.dispH / SAMPLE_RATIO; i++) {
        ifs_.read(dst, dstImg.dispW / SAMPLE_RATIO);
        RETURN_ZERO_IF_EOS(dstImg.dispW / SAMPLE_RATIO);
        dst += dstImg.byteStride / SAMPLE_RATIO;
    }
    // copy V
    for (uint32_t i = 0; i < dstImg.dispH / SAMPLE_RATIO; i++) {
        ifs_.read(dst, dstImg.dispW / SAMPLE_RATIO);
        RETURN_ZERO_IF_EOS(dstImg.dispW / SAMPLE_RATIO);
        dst += dstImg.byteStride / SAMPLE_RATIO;
    }
    return dst - start;
}

uint32_t TesterCommon::ReadOneFrameYUV420SP(ImgBuf& dstImg)
{
    char* dst = reinterpret_cast<char*>(dstImg.va);
    char* start = dst;
    // copy Y
    for (uint32_t i = 0; i < dstImg.dispH; i++) {
        ifs_.read(dst, dstImg.dispW);
        RETURN_ZERO_IF_EOS(dstImg.dispW);
        dst += dstImg.byteStride;
    }
    // copy UV
    for (uint32_t i = 0; i < dstImg.dispH / SAMPLE_RATIO; i++) {
        ifs_.read(dst, dstImg.dispW);
        RETURN_ZERO_IF_EOS(dstImg.dispW);
        dst += dstImg.byteStride;
    }
    return dst - start;
}

uint32_t TesterCommon::ReadOneFrameRGBA(ImgBuf& dstImg)
{
    char* dst = reinterpret_cast<char*>(dstImg.va);
    char* start = dst;
    for (uint32_t i = 0; i < dstImg.dispH; i++) {
        ifs_.read(dst, dstImg.dispW * BYTES_PER_PIXEL_RBGA);
        RETURN_ZERO_IF_EOS(dstImg.dispW * BYTES_PER_PIXEL_RBGA);
        dst += dstImg.byteStride;
    }
    return dst - start;
}

uint32_t TesterCommon::ReadOneFrame(ImgBuf& dstImg)
{
    if (dstImg.va == nullptr) {
        TLOGE("dst image has null va");
        return 0;
    }
    if (dstImg.byteStride < dstImg.dispW) {
        TLOGE("byteStride %u < dispW %u", dstImg.byteStride, dstImg.dispW);
        return 0;
    }
    uint32_t sampleSize = 0;
    switch (dstImg.fmt) {
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P:
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP: {
            sampleSize = GetYuv420Size(dstImg.byteStride, dstImg.dispH);
            break;
        }
        case GRAPHIC_PIXEL_FMT_RGBA_8888: {
            sampleSize = dstImg.byteStride * dstImg.dispH;
            break;
        }
        default:
            return 0;
    }
    if (sampleSize > dstImg.capacity) {
        TLOGE("sampleSize %u > dst capacity %zu", sampleSize, dstImg.capacity);
        return 0;
    }
    if (opt_.mockFrameCnt.has_value() && currInputCnt_ > opt_.mockFrameCnt.value()) {
        return 0;
    }
    if ((opt_.maxReadFrameCnt > 0 && currInputCnt_ >= opt_.maxReadFrameCnt)) {
        return sampleSize;
    }
    switch (dstImg.fmt) {
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P: {
            return ReadOneFrameYUV420P(dstImg);
        }
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP: {
            return ReadOneFrameYUV420SP(dstImg);
        }
        case GRAPHIC_PIXEL_FMT_RGBA_8888: {
            return ReadOneFrameRGBA(dstImg);
        }
        default:
            return 0;
    }
}

bool TesterCommon::InitDemuxer()
{
    ifs_ = ifstream(opt_.inputFile, ios::binary);
    IF_TRUE_RETURN_VAL_WITH_MSG(!ifs_, false, "Failed to open file %s", opt_.inputFile.c_str());
    demuxer_ = StartCodeDetector::Create(opt_.protocol);
    totalSampleCnt_ = demuxer_->SetSource(opt_.inputFile);
    if (totalSampleCnt_ == 0) {
        TLOGE("no nalu found");
        return false;
    }
    return true;
}

bool TesterCommon::RunDecoder()
{
    bool ret = InitDemuxer();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = Create();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = SetCallback();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = ConfigureDecoder();
    IF_TRUE_RETURN_VAL(!ret, false);
    if (!opt_.isBufferMode) {
        sptr<Surface> surface = opt_.render ? CreateSurfaceFromWindow() : CreateSurfaceNormal();
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
    DecoderInputLoop();
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
    option->SetWindowType(WindowType::WINDOW_TYPE_FLOAT);
    option->SetWindowMode(WindowMode::WINDOW_MODE_FULLSCREEN);
    sptr<Window> window = Window::Create("DemoWindow", option);
    if (window == nullptr) {
        TLOGE("Create Window failed");
        return nullptr;
    }
    shared_ptr<RSSurfaceNode> node = window->GetSurfaceNode();
    if (node == nullptr) {
        TLOGE("GetSurfaceNode failed");
        return nullptr;
    }
    sptr<Surface> surface = node->GetSurface();
    if (surface == nullptr) {
        TLOGE("GetSurface failed");
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
        TLOGE("CreateSurfaceAsConsumer failed");
        return nullptr;
    }
    sptr<IBufferConsumerListener> listener = new Listener(this);
    GSError err = consumerSurface->RegisterConsumerListener(listener);
    if (err != GSERROR_OK) {
        TLOGE("RegisterConsumerListener failed");
        return nullptr;
    }
    err = consumerSurface->SetDefaultUsage(BUFFER_USAGE_CPU_READ);
    if (err != GSERROR_OK) {
        TLOGE("SetDefaultUsage failed");
        return nullptr;
    }
    sptr<IBufferProducer> bufferProducer = consumerSurface->GetProducer();
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(bufferProducer);
    if (producerSurface == nullptr) {
        TLOGE("CreateSurfaceAsProducer failed");
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
        TLOGW("AcquireBuffer failed");
        return;
    }
    tester_->surface_->ReleaseBuffer(buffer, -1);
}

void TesterCommon::DecoderInputLoop()
{
    PrepareSeek();
    while (true) {
        if (!SeekIfNecessary()) {
            return;
        }
        BufInfo buf {};
        if (!WaitForInput(buf)) {
            continue;
        }

        size_t sampleIdx;
        bool isCsd;
        buf.attr.size = GetNextSample(buf, sampleIdx, isCsd);
        if (isCsd) {
            buf.attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        }
        if (buf.attr.size == 0 || (opt_.maxReadFrameCnt > 0 && currInputCnt_ > opt_.maxReadFrameCnt)) {
            buf.attr.size = 0;
            buf.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
            BeforeQueueInput(buf.attr);
            if (ReturnInput(buf)) {
                TLOGI("queue eos succ, quit loop");
                return;
            } else {
                TLOGW("queue eos failed");
                continue;
            }
        }
        BeforeQueueInput(buf.attr);
        if (!ReturnInput(buf)) {
            TLOGW("queue sample %zu failed", sampleIdx);
            continue;
        }
        currInputCnt_++;
        currSampleIdx_ = sampleIdx;
        demuxer_->MoveToNext();
    }
}

void TesterCommon::SaveVivid(int64_t pts)
{
    optional<Sample> sample = demuxer_->PeekNextSample();
    if (!sample.has_value()) {
        return;
    }
    if (sample->vividSei.empty()) {
        return;
    }
    lock_guard<mutex> lk(vividMtx_);
    vividMap_[pts] = sample->vividSei;
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
        TLOGI("mock seek from sample index %zu to %zu", seekFrom, seekTo);
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
    TLOGI("begin to seek from sample index %zu to %zu", seekFrom, seekTo);
    if (!demuxer_->SeekTo(seekTo)) {
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

int TesterCommon::GetNextSample(const Span& dstSpan, size_t& sampleIdx, bool& isCsd)
{
    optional<Sample> sample = demuxer_->PeekNextSample();
    if (!sample.has_value()) {
        return 0;
    }
    uint32_t sampleSize = sample->endPos - sample->startPos;
    if (sampleSize > dstSpan.capacity) {
        TLOGE("sampleSize %u > dst capacity %zu", sampleSize, dstSpan.capacity);
        return 0;
    }
    TLOGI("sample %zu: size = %u, isCsd = %d, isIDR = %d, %s",
          sample->idx, sampleSize, sample->isCsd, sample->isIdr, sample->s.c_str());
    sampleIdx = sample->idx;
    isCsd = sample->isCsd;
    ifs_.seekg(sample->startPos);
    ifs_.read(reinterpret_cast<char*>(dstSpan.va), sampleSize);
    return sampleSize;
}
} // namespace OHOS::MediaAVCodec