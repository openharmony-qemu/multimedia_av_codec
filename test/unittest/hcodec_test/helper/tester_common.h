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
#include "surface.h"
#include "wm/window.h"  // foundation/window/window_manager/interfaces/innerkits/
#include "command_parser.h"
#include "start_code_detector.h"
#include "test_utils.h"

namespace OHOS::MediaAVCodec {
struct TesterCommon {
    static std::shared_ptr<TesterCommon> Create(const CommandOpt& opt);
    bool Run();

protected:
    explicit TesterCommon(const CommandOpt& opt) : opt_(opt) {}
    virtual ~TesterCommon() = default;
    static int64_t GetNowUs();
    virtual bool Create() = 0;
    virtual bool SetCallback() = 0;
    virtual bool GetInputFormat() = 0;
    virtual bool GetOutputFormat() = 0;
    virtual bool Start() = 0;
    virtual void InputLoop() = 0;
    virtual void OutputLoop() = 0;
    virtual bool Flush() = 0;
    virtual bool Stop() = 0;
    virtual bool Release() = 0;

    CommandOpt opt_;
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
    uint32_t ReadOneFrame(char* dst);
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
    virtual bool ConfigureDecoder() = 0;
    void FlushThread();
    std::pair<uint32_t, uint32_t> GetNextSample(char* dstVa, size_t dstCapacity); // filledLen & flag

    sptr<OHOS::Rosen::Window> window_;
    HCodecDemuxer demuxer_;
    std::list<PositionPair> userSeekPos_;
    std::list<size_t> naluSeekPos_;
    std::mutex flushMtx_;
    std::condition_variable flushCond_;
};
} // namespace OHOS::MediaAVCodec
#endif