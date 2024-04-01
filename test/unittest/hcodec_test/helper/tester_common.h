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
#include "native_avbuffer.h" // foundation/multimedia/media_foundation/interface/kits/c
#include "buffer/avbuffer.h" // foundation/multimedia/media_foundation/interface/inner_api
#include "native_avcodec_base.h" // foundation/multimedia/av_codec/interfaces/kits/c/
#include "command_parser.h"
#include "start_code_detector.h"
#include "test_utils.h"

namespace OHOS::MediaAVCodec {
struct Span {
    uint8_t* va;
    size_t capacity;
};

struct ImgBuf : Span {
    GraphicPixelFormat fmt;
    uint32_t dispW;
    uint32_t dispH;
    uint32_t byteStride;
};

struct BufInfo : ImgBuf {
    uint32_t idx;
    OH_AVCodecBufferAttr attr;
    OH_AVMemory* mem = nullptr;
    OH_AVBuffer* cavbuf = nullptr;
    std::shared_ptr<Media::AVBuffer> avbuf;
    sptr<SurfaceBuffer> surfaceBuf;
};

struct TesterCommon {
    static bool Run(const CommandOpt &opt);
    bool RunOnce();

protected:
    static std::shared_ptr<TesterCommon> Create(const CommandOpt &opt);
    explicit TesterCommon(const CommandOpt &opt) : opt_(opt) {}
    virtual ~TesterCommon() = default;
    static int64_t GetNowUs();
    virtual bool Create() = 0;
    virtual bool SetCallback() = 0;
    virtual bool GetInputFormat() = 0;
    virtual bool GetOutputFormat() = 0;
    virtual bool Start() = 0;
    void EncoderInputLoop();
    void DecoderInputLoop();
    void OutputLoop();
    void BeforeQueueInput(OH_AVCodecBufferAttr& attr);
    void AfterGotOutput(const OH_AVCodecBufferAttr& attr);
    virtual bool WaitForInput(BufInfo& buf) = 0;
    virtual bool WaitForOutput(BufInfo& buf) = 0;
    virtual bool ReturnInput(const BufInfo& buf) = 0;
    virtual bool ReturnOutput(uint32_t idx) = 0;
    virtual bool Flush() = 0;
    virtual void ClearAllBuffer() = 0;
    virtual bool Stop() = 0;
    virtual bool Release() = 0;

    CommandOpt opt_;
    std::ifstream ifs_;

    std::mutex inputMtx_;
    std::condition_variable inputCond_;
    uint32_t currInputCnt_ = 0;

    std::mutex outputMtx_;
    std::condition_variable outputCond_;

    uint64_t inTotalCnt_ = 0;
    int64_t firstInTime_ = 0;
    double inFps_ = 0;
    uint64_t outTotalCnt_ = 0;
    int64_t firstOutTime_ = 0;
    uint64_t totalCost_ = 0;

    // encoder only
    bool RunEncoder();
    virtual bool ConfigureEncoder() = 0;
    virtual bool SetEncoderParameter(const SetParameterParams& param) { return true; }
    virtual bool SetEncoderPerFrameParam(BufInfo& buf, const PerFrameParams& param) { return true; }
    virtual sptr<Surface> CreateInputSurface() = 0;
    virtual bool NotifyEos() = 0;
    virtual bool RequestIDR() = 0;
    virtual std::optional<uint32_t> GetInputStride() = 0;
    static bool SurfaceBufferToBufferInfo(BufInfo& buf, sptr<SurfaceBuffer> surfaceBuffer);
    static bool NativeBufferToBufferInfo(BufInfo& buf, OH_NativeBuffer* nativeBuffer);
    bool WaitForInputSurfaceBuffer(BufInfo& buf);
    bool ReturnInputSurfaceBuffer(BufInfo& buf);
    uint32_t ReadOneFrame(ImgBuf& dstImg);
    uint32_t ReadOneFrameYUV420P(ImgBuf& dstImg);
    uint32_t ReadOneFrameYUV420SP(ImgBuf& dstImg);
    uint32_t ReadOneFrameRGBA(ImgBuf& dstImg);
    sptr<Surface> producerSurface_;
    GraphicPixelFormat displayFmt_;
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
    bool InitDemuxer();
    sptr<Surface> CreateSurfaceFromWindow();
    sptr<Surface> CreateSurfaceNormal();
    virtual bool SetOutputSurface(sptr<Surface> &surface) = 0;
    void PrepareSeek();
    bool SeekIfNecessary(); // false means quit loop
    virtual bool ConfigureDecoder() = 0;
    int GetNextSample(const Span &dstSpan, size_t &sampleIdx, bool &isCsd); // return filledLen
    sptr<Surface> surface_; // consumer
    sptr<OHOS::Rosen::Window> window_;
    std::shared_ptr<StartCodeDetector> demuxer_;
    size_t totalSampleCnt_ = 0;
    size_t currSampleIdx_ = 0;
    std::list<std::pair<size_t, size_t>> userSeekPos_; // seek from which index to which index

    static bool RunDecEnc(const CommandOpt &decOpt);
    void SaveVivid(int64_t pts);
    void CheckVivid(const BufInfo& buf);
    static std::mutex vividMtx_;
    static std::unordered_map<int64_t, std::vector<uint8_t>> vividMap_;
};
} // namespace OHOS::MediaAVCodec
#endif