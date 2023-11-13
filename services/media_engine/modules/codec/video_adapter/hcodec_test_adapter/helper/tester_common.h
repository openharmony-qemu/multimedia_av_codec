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

#ifndef HCODEC_TESTER_COMMON_H
#define HCODEC_TESTER_COMMON_H

#include <fstream>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "native_avcodec_base.h"
#include "surface.h"
#include "wm/window.h"  // foundation/window/window_manager/interfaces/innerkits/
#include "command_parser.h"
#include "start_code_detector.h"
#include "test_utils.h"
#include "buffer/avbuffer.h"


namespace OHOS::MediaAVCodec {
struct Span {
    char* va;
    size_t capacity;
};

struct TesterCommon {
    static bool Run(const CommandOpt& opt);
    bool RunOnce();

protected:
    explicit TesterCommon(const CommandOpt& opt) : opt_(opt) {}
    virtual ~TesterCommon() = default;
    static int64_t GetNowUs();
    virtual bool Create() = 0;
    virtual bool SetCallback() = 0;
    virtual bool GetInputFormat() = 0;
    virtual bool GetOutputFormat() = 0;
    virtual bool Prepare() = 0;
    virtual bool Start() = 0;
    // asharedmem circle
    void EncoderInputLoopForAsharedMem();
    void DecoderInputLoopForAsharedMem();
    virtual std::optional<uint32_t> GetInputIndexForAsharedMem(Span& span) { return std::nullopt; }
    virtual bool QueueInputForAsharedMem(uint32_t idx, OH_AVCodecBufferAttr attr) { return false; }
    // avbuffer circle
    void EncoderInputLoopForAvBuffer();
    void DecoderInputLoopForAvBuffer();
    virtual std::optional<uint32_t> GetInputIndexForAvBuffer(std::shared_ptr<Media::AVBuffer>& avBuffer) { return std::nullopt; }
    virtual bool QueueInputForAvBuffer(uint32_t idx, std::shared_ptr<Media::AVBuffer> &avBuffer)
    {
        return false;
    }

    void OutputLoop();
    virtual std::optional<uint32_t> GetOutputIndex() = 0;
    virtual bool ReturnOutput(uint32_t idx) = 0;
    virtual bool Flush() = 0;
    virtual void ClearAllBuffer() = 0;
    virtual bool Stop() = 0;
    virtual bool Release() = 0;

    const CommandOpt opt_;
    std::ifstream ifs_;
    sptr<Surface> surface_;

    std::mutex inputMtx_;
    std::condition_variable inputCond_;
    uint32_t currInputCnt_ = 0;

    std::mutex outputMtx_;
    std::condition_variable outputCond_;

    // encoder only
    bool RunEncoder();
    virtual bool ConfigureEncoder() = 0;
    virtual bool CreateInputSurface() = 0;
    virtual bool NotifyEos() = 0;
    virtual bool RequestIDR() = 0;
    virtual std::optional<uint32_t> GetInputStride() = 0;
    void InputSurfaceLoop();
    uint32_t ReadOneFrame(VideoPixelFormat pixFmt, Span dstSpan);
    uint32_t ReadOneFrameYUV420P(char* dst);
    uint32_t ReadOneFrameYUV420SP(char* dst);
    uint32_t ReadOneFrameRGBA(char* dst);
    GraphicPixelFormat displayFmt_;
    uint32_t stride_ = 0;
    static constexpr uint32_t BYTES_PER_PIXEL_RBGA = 4;
    static constexpr uint32_t SAMPLE_RATIO = 2;

    // decoder only
    class Listener : public IBufferConsumerListener {
    public:
        explicit Listener(TesterCommon *test) : tester_(test) {}
        void OnBufferAvailable() override;
    private:
        TesterCommon *tester_;
    };

    bool RunDecoder();
    sptr<Surface> CreateSurfaceFromWindow();
    sptr<Surface> CreateSurfaceNormal();
    virtual bool SetOutputSurface(sptr<Surface>& surface) = 0;
    void PrepareSeek();
    bool SeekIfNecessary();  // false means quit loop
    virtual bool ConfigureDecoder() = 0;
    int GetNextSample(Span dstSpan, size_t& sampleIdx, bool& isCsd); // return filledLen
    sptr<OHOS::Rosen::Window> window_;
    StartCodeDetector demuxer_;
    size_t totalSampleCnt_ = 0;
    size_t currSampleIdx_ = 0;
    std::list<std::pair<size_t, size_t>> userSeekPos_; // seek from which index to which index
};
} // namespace OHOS::MediaAVCodec
#endif