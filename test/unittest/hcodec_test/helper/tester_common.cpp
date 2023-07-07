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
#include "hcodec_api.h"
#include "hcodec_log.h"
#include "type_converter.h"
#include "tester_codecbase.h"
#include "tester_capi.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::Rosen;

int64_t TesterCommon::GetNowUs()
{
    auto now = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::microseconds>(now.time_since_epoch()).count();
}

std::shared_ptr<TesterCommon> TesterCommon::Create(const CommandOpt& opt)
{
    opt.Print();
    if (opt.testCodecBaseApi) {
        return make_shared<TesterCodecBase>(opt);
    } else {
        return make_shared<TesterCapi>(opt);
    }
}

bool TesterCommon::Run()
{
    CostRecorder::Instance().Clear();
    for (uint32_t i = 0; i < opt_.repeatCnt; i++) {
        printf("i = %u\n", i);
        bool ret = opt_.isEncoder ? RunEncoder() : RunDecoder();
        if (!ret) {
            return false;
        }
    }
    CostRecorder::Instance().Print();
    return true;
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
    if (opt_.isBufferMode) {
        stride_ = opt_.dispW;
        ret = GetInputFormat();
        if (ret) {
            optional<uint32_t> stride = GetInputStride();
            if (stride && stride.value() >= opt_.dispW) {
                stride_ = stride.value();
            }
        }
    } else {
        ret = CreateInputSurface();
        IF_TRUE_RETURN_VAL(!ret, false);
    }
    ret = Start();
    IF_TRUE_RETURN_VAL(!ret, false);

    thread th(&TesterCommon::OutputLoop, this);
    if (opt_.isBufferMode) {
        InputLoop();
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
        int stride = surfaceBuffer->GetStride();
        if (dst == nullptr || stride < opt_.dispW) {
            LOGE("invalid va or stride %{public}d", stride);
            surface_->CancelBuffer(surfaceBuffer);
            continue;
        }
        stride_ = stride;
        uint32_t filledLen = ReadOneFrame(dst);
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

uint32_t TesterCommon::ReadOneFrame(char* dst)
{
    switch (opt_.pixFmt) {
        case YUVI420:
            return ReadOneFrameYUV420P(dst);
        case NV12:
        case NV21:
            return ReadOneFrameYUV420SP(dst);
        case RGBA:
            return ReadOneFrameRGBA(dst);
        default:
            return 0;
    }
}

bool TesterCommon::RunDecoder()
{
    ifs_ = ifstream(opt_.inputFile, ios::binary);
    IF_TRUE_RETURN_VAL_WITH_MSG(!ifs_, false, "Failed to open file %{public}s", opt_.inputFile.c_str());
    if (!demuxer_.LoadNaluListFromPath(opt_.inputFile, opt_.protocol)) {
        LOGE("no nalu found");
        return false;
    }
    bool ret = Create();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = SetCallback();
    IF_TRUE_RETURN_VAL(!ret, false);

    PrepareSeek();
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
    ret = Start();
    IF_TRUE_RETURN_VAL(!ret, false);

    thread flushThread(&TesterCommon::FlushThread, this);
    thread outputThread(&TesterCommon::OutputLoop, this);
    InputLoop();
    if (outputThread.joinable()) {
        outputThread.join();
    }
    if (flushThread.joinable()) {
        flushThread.join();
    }

    ret = Stop();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = Release();
    IF_TRUE_RETURN_VAL(!ret, false);
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

static size_t GenerateRandomNumInRange(size_t rangeStart, size_t rangeEnd)
{
    return rangeStart + rand() % (rangeEnd - rangeStart);
}

void TesterCommon::PrepareSeek()
{
    int mockCnt = 0;
    size_t lastSeekTo = 0;
    while (mockCnt++ < opt_.flushCnt) {
        size_t seekAt = GenerateRandomNumInRange(lastSeekTo, demuxer_.GetTotalNaluCnt());
        size_t seekTo;
        do {
            seekTo = GenerateRandomNumInRange(0, demuxer_.GetTotalNaluCnt());
        } while (seekTo == seekAt);
        lastSeekTo = seekTo;
        userSeekPos_.push_back(make_pair(seekAt, seekTo));
        LOGI("Mock flush configure: from (%{public}zu) to (%{public}zu)", seekAt, seekTo);
    }
}

void TesterCommon::FlushThread()
{
    if (opt_.flushCnt <= 0) {
        LOGI("no need flush, quit loop");
        return;
    }
    while (!userSeekPos_.empty()) {
        size_t seekAt = userSeekPos_.front().first;
        size_t seekTo = userSeekPos_.front().second;
        userSeekPos_.pop_front();
        {
            unique_lock<mutex> lk(flushMtx_);
            flushCond_.wait(lk, [this, seekAt] {
                return naluSeekPos_.empty() && seekAt == demuxer_.GetNaluCursor();
            });
        }
        LOGI("Mock flush: from (%{public}zu) to (%{public}zu)", seekAt, seekTo);
        bool ret = Flush();
        if (!ret) {
            LOGE("Flush failed");
            continue;
        }
        ret = Start();
        if (!ret) {
            LOGE("Start failed after flush");
            continue;
        }
        std::optional<PositionPair> refInfo = demuxer_.FindReferenceInfo(seekTo);
        if (!refInfo.has_value()) {
            continue;
        }
        {
            lock_guard<mutex> lk(flushMtx_);
            naluSeekPos_.push_back(refInfo.value().first);
            naluSeekPos_.push_back(refInfo.value().second);
        }
    }
    LOGI("flush complete, quit loop");
}

pair<uint32_t, uint32_t> TesterCommon::GetNextSample(char* dstVa, size_t dstCapacity)
{
    uint32_t filledLen = 0;
    uint32_t flags = 0;
    NaluUnit nalu;
    {
        lock_guard<mutex> lk(flushMtx_);
        int offset = -1;
        if (!naluSeekPos_.empty()) {
            offset = static_cast<int>(naluSeekPos_.front());
            naluSeekPos_.pop_front();
        }
        nalu = demuxer_.GetNext(opt_.protocol, offset);
        if (nalu.isEos) {
            flags = AVCODEC_BUFFER_FLAGS_EOS;
            return std::pair<uint32_t, uint32_t> {filledLen, flags};
        }
        if (nalu.isCsd) {
            flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        }
        flushCond_.notify_all();
    }

    filledLen = nalu.endPos - nalu.startPos;
    if (filledLen > dstCapacity) {
        LOGE("this input buffer has size=%{public}u but dstCapacity=%{public}zu", filledLen, dstCapacity);
        return std::pair<uint32_t, uint32_t> {filledLen, flags};
    }
    ifs_.seekg(nalu.startPos);
    ifs_.read(dstVa, filledLen);
    return std::pair<uint32_t, uint32_t> {filledLen, flags};
}
} // namespace OHOS::MediaAVCodec