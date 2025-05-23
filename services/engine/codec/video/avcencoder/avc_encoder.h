/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef AVC_ENCODER_H
#define AVC_ENCODER_H

#include <atomic>
#include <list>
#include <map>
#include <shared_mutex>
#include <functional>
#include <fstream>
#include <tuple>
#include <vector>
#include <optional>
#include <algorithm>
#include "av_common.h"
#include "avcodec_common.h"
#include "avcodec_info.h"
#include "block_queue.h"
#include "codec_utils.h"
#include "codecbase.h"
#include "media_description.h"
#include "fsurface_memory.h"
#include "task_thread.h"
#include "AvcEnc_Typedef.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {

using CreateAvcEncoderFuncType = uint32_t (*)(AVC_ENC_HANDLE *phEncoder, AVC_ENC_INIT_PARAM *pstInitParam);
using EncodeFuncType = uint32_t (*)(AVC_ENC_HANDLE hEncoder, AVC_ENC_INARGS *pstInArgs, AVC_ENC_OUTARGS *pstOutArgs);
using DeleteFuncType = uint32_t (*)(AVC_ENC_HANDLE hEncoder);

class AvcEncoder : public CodecBase, public RefBase {
public:
    explicit AvcEncoder(const std::string &name);
    ~AvcEncoder() override;
    int32_t Configure(const Format &format) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t SetParameter(const Format &format) override;
    int32_t GetInputFormat(Format &format) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t QueueInputBuffer(uint32_t index) override;
    int32_t ReleaseOutputBuffer(uint32_t index) override;
    int32_t NotifyEos() override;
    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) override;
    sptr<Surface> CreateInputSurface() override;
    int32_t SetInputSurface(sptr<Surface> surface) override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t RenderOutputBuffer(uint32_t index) override;
    int32_t SignalRequestIDRFrame() override;
    void GetBufferFromSurface();
    static int32_t GetCodecCapability(std::vector<CapabilityData> &capaArray);

    class FBuffer {
    public:
        FBuffer() = default;
        ~FBuffer() = default;

        enum class Owner {
            OWNED_BY_US,
            OWNED_BY_CODEC,
            OWNED_BY_USER,
            OWNED_BY_SURFACE,
        };

        std::shared_ptr<AVBuffer> avBuffer_ = nullptr;
        sptr<SurfaceBuffer> surfaceBuffer_ = nullptr;
        std::atomic<Owner> owner_ = Owner::OWNED_BY_US;
        int32_t width_ = 0;
        int32_t height_ = 0;
    };
private:
    static int32_t CheckAvcEncLibStatus();
    static void GetCapabilityData(CapabilityData &capsData, uint32_t index);
    int32_t Initialize();
    int32_t ConfigureContext(const Format &format);
    void ConfigureDefaultVal(const Format &format, const std::string_view &formatKey,
        int32_t minVal = 0, int32_t maxVal = INT_MAX);
    bool GetDiscardFlagFromAVBuffer(const std::shared_ptr<AVBuffer> &buffer);
    void GetPixelFmtFromUser(const Format &format);
    void GetQpRangeFromUser(const Format &format);
    void GetBitRateFromUser(const Format &format);
    void GetFrameRateFromUser(const Format &format);
    void GetBitRateModeFromUser(const Format &format);
    void GetIFrameIntervalFromUser(const Format &format);
    void GetRequestIDRFromUser(const Format &format);
    void GetColorAspects(const Format &format);
    void CheckIfEnableCb(const Format &format);
    int32_t SetupPort(const Format &format);

    void CheckBitRateSupport(int32_t &bitrate);
    void CheckFrameRateSupport(double &framerate);
    void CheckIFrameIntervalTimeSupport(int32_t &interval);

    void AvcFuncMatch();
    void ReleaseHandle();

    void ReleaseSurfaceBuffer();
    void ClearDirtyList();
    void WaitForInBuffer();
    void FillEncodedBuffer(const std::shared_ptr<FBuffer> &frameBuffer);

    void ReleaseSurfaceBufferByAVBuffer(std::shared_ptr<AVBuffer> &buffer);
    void NotifyUserToProcessBuffer(uint32_t index, std::shared_ptr<AVBuffer> &buffer);
    void NotifyUserToFillBuffer(uint32_t index, std::shared_ptr<AVBuffer> &buffer);

    void InitAvcEncoderParams();
    void FillAvcInitParams(AVC_ENC_INIT_PARAM &param);

    struct InputFrame {
        uint8_t *buffer = nullptr;
        int32_t width = 0;
        int32_t height = 0;
        int32_t stride = 0;
        int32_t size = 0;
        int32_t uvOffset = 0;
        VideoPixelFormat format = VideoPixelFormat::UNKNOWN;
        int64_t pts = 0;
    };

    struct NVFrame {
        uint8_t *srcY   = nullptr;
        uint8_t *srcUV  = nullptr;
        int32_t yStride = 0;
        int32_t uvStide = 0;
        int32_t width   = 0;
        int32_t height  = 0;
    };

    int32_t FillAvcEncoderInDefaultArgs(AVC_ENC_INARGS &inArgs);
    int32_t FillAvcEncoderInArgs(std::shared_ptr<AVBuffer> &buffer, AVC_ENC_INARGS &inArgs);

    void FillNV21ToAvcEncInArgs(AVC_ENC_INARGS &inArgs, NVFrame &nvFrame, int64_t pts);
    int32_t GetSurfaceBufferUvOffset(sptr<SurfaceBuffer> &surfaceBuffer, VideoPixelFormat format);
    int32_t GetInputFrameFromAVBuffer(std::shared_ptr<AVBuffer> &buffer, InputFrame &inFrame);
    int32_t Yuv420ToAvcEncoderInArgs(InputFrame &inFrame, AVC_ENC_INARGS &inArgs);
    int32_t RgbaToAvcEncoderInArgs(InputFrame &inFrame, AVC_ENC_INARGS &inArgs);
    int32_t Nv12ToAvcEncoderInArgs(InputFrame &inFrame, AVC_ENC_INARGS &inArgs);
    int32_t Nv21ToAvcEncoderInArgs(InputFrame &inFrame, AVC_ENC_INARGS &inArgs);
    int32_t CheckBufferSize(int32_t bufferSize, int32_t stride, int32_t height, VideoPixelFormat pixelFormat);

    void EncoderAvcHeader();
    void EncoderAvcTailer();
    int32_t EncoderAvcFrame(AVC_ENC_INARGS &inArgs, AVC_ENC_OUTARGS &outArgs);

    enum struct State : int32_t {
        UNINITIALIZED,
        INITIALIZED,
        CONFIGURED,
        STOPPING,
        RUNNING,
        FLUSHED,
        FLUSHING,
        EOS,
        ERROR,
    };
    bool IsActive() const;
    void CalculateBufferSize();
    int32_t AllocateBuffers();
    void InitBuffers();
    void ResetBuffers();
    void ResetData();
    void ReleaseBuffers();
    void StopThread();
    void ReleaseResource();
    void SendFrame();
    int32_t AllocateInputBuffer(int32_t bufferCnt, int32_t inBufferSize);
    int32_t AllocateOutputBuffer(int32_t bufferCnt, int32_t outBufferSize);

private:
    class EncoderBuffersConsumerListener : public IBufferConsumerListener {
    public:
        explicit EncoderBuffersConsumerListener(AvcEncoder *codec) : codec_(codec) {}
        void OnBufferAvailable() override;
    private:
        AvcEncoder *codec_ = nullptr;
    };

    AVC_ENC_HANDLE avcEncoder_ = nullptr;
    CreateAvcEncoderFuncType avcEncoderCreateFunc_ = nullptr;
    EncodeFuncType avcEncoderFrameFunc_ = nullptr;
    DeleteFuncType avcEncoderDeleteFunc_ = nullptr;
    AVC_ENC_INIT_PARAM initParams_;
    AVC_ENC_INARGS avcEncInputArgs_;
    AVC_ENC_OUTARGS avcEncOutputArgs_;

private:
    std::string codecName_;
    std::atomic<State> state_ = State::UNINITIALIZED;
    void* handle_ = nullptr;
    uint32_t encInstanceID_ = 0;
    static std::mutex encoderCountMutex_;
    static std::vector<uint32_t> encInstanceIDSet_;
    static std::vector<uint32_t> freeIDSet_;
    static constexpr uint32_t SURFACE_MODE_CONSUMER_USAGE = BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_CPU_READ;
    Format format_;
    int32_t encWidth_;
    int32_t encHeight_;
    int32_t encBitrate_;
    int32_t encQp_;
    int32_t encQpMax_;
    int32_t encQpMin_;
    int32_t encIperiod_;
    double encFrameRate_;
    bool encIdrRequest_ = false;
    bool enableSurfaceModeInputCb_ = false;
    uint8_t srcRange_ = 0;
    ColorPrimary srcPrimary_ = ColorPrimary::COLOR_PRIMARY_UNSPECIFIED;
    TransferCharacteristic srcTransfer_ = TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED;
    MatrixCoefficient srcMatrix_ = MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED;
    AVCProfile avcProfile_ = AVCProfile::AVC_PROFILE_BASELINE;
    AVCLevel avcLevel_ = AVCLevel::AVC_LEVEL_3;
    VideoEncodeBitrateMode bitrateMode_ = VideoEncodeBitrateMode::CQ;
    int32_t avcQuality_ = 0;
    int32_t inputBufferSize_ = 0;
    int32_t outputBufferSize_ = 0;

    uint8_t *scaleData_[AV_NUM_DATA_POINTERS] = {nullptr};
    int32_t scaleLineSize_[AV_NUM_DATA_POINTERS] = {0};
    std::shared_ptr<Scale> scale_ = nullptr;
    VideoPixelFormat srcPixelFmt_ = VideoPixelFormat::UNKNOWN;
    VideoPixelFormat encodePixelFmt_ = VideoPixelFormat::NV21;
    std::vector<std::shared_ptr<FBuffer>> buffers_[2];
    std::shared_ptr<AVBuffer> convertBuffer_ = nullptr;
    std::shared_ptr<BlockQueue<uint32_t>> inputAvailQue_;
    std::shared_ptr<BlockQueue<uint32_t>> codecAvailQue_;
    std::shared_ptr<TaskThread> sendTask_ = nullptr;
    std::mutex inputMutex_;
    std::mutex outputMutex_;
    std::mutex sendMutex_;
    std::mutex encRunMutex_;

    // 存放可用的 buffers 索引
    std::mutex freeListMutex_;
    std::list<uint32_t> freeList_;
    std::condition_variable surfaceRecvCv_;

    std::condition_variable sendCv_;
    sptr<Surface> inputSurface_ = nullptr;
    std::shared_ptr<MediaCodecCallback> callback_;
    std::atomic<bool> isSendWait_ = false;
    std::atomic<bool> isSendEos_ = false;
    std::atomic<bool> isBufferAllocated_ = false;
    bool isFirstFrame_ = true;
};
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVC_ENCODER_H