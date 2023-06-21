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

#ifndef FCODEC_H
#define FCODEC_H

#include <atomic>
#include <list>
#include <map>
#include <shared_mutex>
#include <tuple>
#include <vector>
#include "av_common.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "codec_utils.h"
#include "codecbase.h"
#include "media_description.h"
#include "surface_memory.h"
#include "task_thread.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
class FCodec : public CodecBase {
public:
    explicit FCodec(const std::string &name);
    ~FCodec() override;
    int32_t Configure(const Format &format) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t SetParameter(const Format &format) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t QueueInputBuffer(uint32_t index, const AVCodecBufferInfo &info, AVCodecBufferFlag flag) override;
    int32_t ReleaseOutputBuffer(uint32_t index) override;
    int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t RenderOutputBuffer(uint32_t index) override;
    static int32_t GetCodecCapability(std::vector<CapabilityData> &capaArray);

    struct AVBuffer {
    public:
        AVBuffer() = default;
        ~AVBuffer() = default;

        enum class Owner {
            OWNED_BY_CODEC,
            OWNED_BY_USER,
            OWNED_BY_SURFACE,
        };

        std::shared_ptr<AVSharedMemory> memory_;
        std::atomic<Owner> owner_;
        AVCodecBufferInfo bufferInfo_;
        AVCodecBufferFlag bufferFlag_;
    };

private:
    int32_t Init();

    enum struct State : int32_t {
        Uninitialized,
        Initialized,
        Configured,
        Stopping,
        Running,
        Flushed,
        Flushing,
        EOS,
        Error,
    };
    bool IsActive() const;
    void ResetContext(bool isFlush = false);
    std::tuple<int32_t, int32_t> CalculateBufferSize();
    int32_t AllocateBuffers();
    int32_t ResetBuffers();
    int32_t ReleaseBuffers(bool isFlush = false);
    int32_t UpdateBuffers(uint32_t index, int32_t buffer_size, uint32_t buffer_type);
    int32_t UpdateSurfaceMemory();
    void SendFrame();
    void ReceiveFrame();
    void RenderFrame();
    void ConfigureSufrace(const Format &format, const std::string_view &formatKey, uint32_t FORMAT_TYPE);
    void ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal = 0,
                             int32_t maxVal = INT_MAX);
    void FramePostProcess(std::shared_ptr<AVBuffer> frameBuffer, int32_t status, int ret);
    int32_t AllocateInputBuffer(int32_t bufferCnt, int32_t inBufferSize);
    int32_t AllocateOutputBuffer(int32_t bufferCnt, int32_t outBufferSize);
    int32_t FillFrameBuffer(const std::shared_ptr<AVBuffer> &frameBuffer);
    int32_t CheckFormatChange(uint32_t index, int width, int height);
    void SetSurfaceParameter(const Format &format, const std::string_view &formatKey, uint32_t FORMAT_TYPE);
    int32_t UpdateSurfaceMemory(std::shared_ptr<SurfaceMemory> &surfaceMemory, int64_t pts);

    std::string codecName_;
    std::atomic<State> state_ = State::Uninitialized;
    Format format_;
    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t inputBufferSize_ = 0;
    int32_t outputBufferSize_ = 0;
    // INIT
    std::shared_ptr<AVCodec> avCodec_ = nullptr;
    // Config
    std::shared_ptr<AVCodecContext> avCodecContext_ = nullptr;
    // Start
    std::shared_ptr<AVPacket> avPacket_ = nullptr;
    std::shared_ptr<AVFrame> cachedFrame_ = nullptr;
    // Receive frame
    uint8_t *scaleData_[AV_NUM_DATA_POINTERS];
    int32_t scaleLineSize_[AV_NUM_DATA_POINTERS];
    std::shared_ptr<Scale> scale_ = nullptr;
    bool isConverted_ = false;
    bool formatChange_ = false;
    VideoPixelFormat outputPixelFmt_ = VideoPixelFormat::UNKNOWN_FORMAT;
    // Running
    std::vector<std::shared_ptr<AVBuffer>> buffers_[2];
    std::list<uint32_t> codecAvailBuffers_;
    std::list<uint32_t> renderBuffers_;
    std::list<uint32_t> inBufQue_;
    sptr<Surface> surface_ = nullptr;
    std::shared_ptr<TaskThread> sendTask_ = nullptr;
    std::shared_ptr<TaskThread> receiveTask_ = nullptr;
    std::shared_ptr<TaskThread> renderTask_ = nullptr;
    std::shared_mutex inputMutex_;
    std::mutex outputMutex_;
    std::mutex sendMutex_;
    std::mutex syncMutex_;
    std::condition_variable outputCv_;
    std::condition_variable sendCv_;
    std::shared_ptr<AVCodecCallback> callback_;
    std::atomic<bool> isSendWait_ = false;
    std::atomic<bool> isSendEos_ = false;
    std::atomic<bool> isBufferAllocated_ = false;
};
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FCODEC_H