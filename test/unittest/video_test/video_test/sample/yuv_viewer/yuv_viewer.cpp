/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "yuv_viewer.h"
#include <sys/mman.h>
#include <unistd.h>
#include "refbase.h"
#include "surface/window.h"
#include "surface.h"

#include "ui/rs_surface_node.h"
#include "window_option.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"
#include "sample_utils.h"
#include "avcodec_trace.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "YuvViewer"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
YuvViewer::~YuvViewer()
{
    if (rosenWindow_) {
        rosenWindow_->Destroy();
        rosenWindow_ = nullptr;
    }

    if (inputThread_ && inputThread_->joinable()) {
        inputThread_->join();
    }
}

int32_t YuvViewer::Create(SampleInfo sampleInfo)
{
    sampleInfo_ = std::make_shared<SampleInfo>(sampleInfo);
    dataProducer_ = DataProducerFactory::CreateDataProducer(DATA_PRODUCER_TYPE_RAW_DATA_READER);
    CHECK_AND_RETURN_RET_LOG(dataProducer_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create data producer failed");
    int32_t ret = dataProducer_->Init(sampleInfo_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Data producer init failed");

    ret = CreateWindow();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create window failed");
    
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t YuvViewer::Start()
{
    inputThread_ = std::make_unique<std::thread>(&YuvViewer::InputThread, this);
    CHECK_AND_RETURN_RET_LOG(inputThread_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create input thread failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t YuvViewer::CreateWindow()
{
    sptr<OHOS::Surface> surfaceProducer;
    sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
    option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_FLOAT);
    option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FULLSCREEN);
    rosenWindow_ = Rosen::Window::Create("VideoCodecDemo", option);
    CHECK_AND_RETURN_RET_LOG(rosenWindow_ != nullptr && rosenWindow_->GetSurfaceNode() != nullptr,
        AVCODEC_SAMPLE_ERR_ERROR, "Create display window failed");
    rosenWindow_->SetTurnScreenOn(!rosenWindow_->IsTurnScreenOn());
    rosenWindow_->SetKeepScreenOn(true);
    rosenWindow_->Show();
    surfaceProducer = rosenWindow_->GetSurfaceNode()->GetSurface();

    window_ = std::shared_ptr<NativeWindow>(reinterpret_cast<NativeWindow *>(
        CreateNativeWindowFromSurface(&surfaceProducer)), [](NativeWindow *window) -> void { (void)window; });

    (void)OH_NativeWindow_NativeWindowHandleOpt(window_.get(), SET_TRANSFORM, 1); // 1: rotation 90°
    (void)OH_NativeWindow_NativeWindowHandleOpt(window_.get(), SET_BUFFER_GEOMETRY,
        sampleInfo_->videoWidth, sampleInfo_->videoHeight);
    (void)OH_NativeWindow_NativeWindowHandleOpt(window_.get(), SET_USAGE,
        NATIVEBUFFER_USAGE_CPU_READ | NATIVEBUFFER_USAGE_CPU_WRITE |
        NATIVEBUFFER_USAGE_MEM_DMA | NATIVEBUFFER_USAGE_HW_RENDER);
    (void)OH_NativeWindow_NativeWindowHandleOpt(window_.get(), SET_FORMAT,
        ToGraphicPixelFormat(sampleInfo_->pixelFormat, sampleInfo_->profile));

    return AVCODEC_SAMPLE_ERR_OK;
}

void YuvViewer::InputThread()
{
    OHOS::MediaAVCodec::AVCodecTrace::TraceBegin("SampleWorkTime", FAKE_POINTER(this));
    int32_t frameCount = 0;
    while (true) {
        int fenceFd = -1;
        OHNativeWindowBuffer *buffer = nullptr;
        int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(window_.get(), &buffer, &fenceFd);
        CHECK_AND_CONTINUE_LOG(ret == 0 && buffer != nullptr, "RequestBuffer failed, ret: %{public}d", ret);
        
        BufferHandle* bufferHandle = OH_NativeWindow_GetBufferHandleFromNative(buffer);
        CHECK_AND_BREAK_LOG(bufferHandle != nullptr, "Get buffer handle failed, thread out");
        uint8_t *bufferAddr = static_cast<uint8_t *>(mmap(bufferHandle->virAddr, bufferHandle->size,
            PROT_READ | PROT_WRITE, MAP_SHARED, bufferHandle->fd, 0));
        CHECK_AND_BREAK_LOG(bufferAddr != MAP_FAILED, "Map native window buffer failed, thread out");

        CodecBufferInfo bufferInfo(bufferAddr);
        ret = dataProducer_->ReadSample(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Read frame failed, thread out");
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Push EOS frame, thread out");
        ret = munmap(bufferAddr, bufferHandle->size);
        CHECK_AND_BREAK_LOG(ret != -1, "Unmap buffer failed, thread out");

        ThreadSleep(sampleInfo_->threadSleepMode == THREAD_SLEEP_MODE_INPUT_SLEEP, sampleInfo_->frameInterval);

        ret = OH_NativeWindow_NativeWindowFlushBuffer(window_.get(), buffer, fenceFd, {nullptr, 0});
        CHECK_AND_BREAK_LOG(ret == 0, "Read frame failed, thread out");

        frameCount++;
    }
    OHOS::MediaAVCodec::AVCodecTrace::TraceEnd("SampleWorkTime", FAKE_POINTER(this));
    doneCond_.notify_all();
    AVCODEC_LOGI("Exit, frame count: %{public}u", frameCount);
}
} // Sample
} // MediaAVCodec
} // OHOS