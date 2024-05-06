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

#ifndef HCODEC_HCODEC_H
#define HCODEC_HCODEC_H

#include <queue>
#include <array>
#include <functional>
#include <unordered_set>
#include "securec.h"
#include "OMX_Component.h"  // third_party/openmax/api/1.1.2
#include "codecbase.h"
#include "avcodec_errors.h"
#include "state_machine.h"
#include "v3_0/codec_types.h"
#include "v3_0/icodec_callback.h"
#include "v3_0/icodec_component.h"
#include "v3_0/icodec_component_manager.h"
#include "type_converter.h"
#include "buffer/avbuffer.h"
#include "meta/meta_key.h" // foundation/multimedia/histreamer/interface/inner_api/

namespace OHOS::MediaAVCodec {
class HCodec : public CodecBase, protected StateMachine {
public:
    static std::shared_ptr<HCodec> Create(const std::string &name);
    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) override;
    int32_t Configure(const Format &format) override;
    sptr<Surface> CreateInputSurface() override;
    int32_t SetInputSurface(sptr<Surface> surface) override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;

    int32_t QueueInputBuffer(uint32_t index) override;
    int32_t NotifyEos() override;
    int32_t ReleaseOutputBuffer(uint32_t index) override;
    int32_t RenderOutputBuffer(uint32_t index) override;

    int32_t SignalRequestIDRFrame() override;
    int32_t SetParameter(const Format& format) override;
    int32_t GetInputFormat(Format& format) override;
    int32_t GetOutputFormat(Format& format) override;
    std::string GetHidumperInfo() override;

    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;

protected:
    enum MsgWhat : MsgType {
        INIT,
        SET_CALLBACK,
        CONFIGURE,
        CREATE_INPUT_SURFACE,
        SET_INPUT_SURFACE,
        SET_OUTPUT_SURFACE,
        START,
        GET_INPUT_FORMAT,
        GET_OUTPUT_FORMAT,
        SET_PARAMETERS,
        REQUEST_IDR_FRAME,
        FLUSH,
        QUEUE_INPUT_BUFFER,
        NOTIFY_EOS,
        RELEASE_OUTPUT_BUFFER,
        RENDER_OUTPUT_BUFFER,
        STOP,
        RELEASE,
        GET_HIDUMPER_INFO,

        INNER_MSG_BEGIN = 1000,
        CODEC_EVENT,
        OMX_EMPTY_BUFFER_DONE,
        OMX_FILL_BUFFER_DONE,
        GET_BUFFER_FROM_SURFACE,
        CHECK_IF_STUCK,
        FORCE_SHUTDOWN,
    };

    enum BufferOperationMode {
        KEEP_BUFFER,
        RESUBMIT_BUFFER,
        FREE_BUFFER,
    };

    enum BufferOwner {
        OWNED_BY_US = 0,
        OWNED_BY_USER = 1,
        OWNED_BY_OMX = 2,
        OWNED_BY_SURFACE = 3,
        OWNER_CNT = 4,
    };

    struct PortInfo {
        uint32_t width;
        uint32_t height;
        OMX_VIDEO_CODINGTYPE codingType;
        std::optional<PixelFmt> pixelFmt;
        double frameRate;
        std::optional<uint32_t> inputBufSize;
    };

    enum DumpMode : unsigned long {
        DUMP_NONE = 0,
        DUMP_ENCODER_INPUT = 0b1000,
        DUMP_ENCODER_OUTPUT = 0b0100,
        DUMP_DECODER_INPUT = 0b0010,
        DUMP_DECODER_OUTPUT = 0b0001,
    };

    struct BufferInfo {
        BufferInfo() : lastOwnerChangeTime(std::chrono::steady_clock::now()) {}
        bool isInput = true;
        BufferOwner owner = OWNED_BY_US;
        std::chrono::time_point<std::chrono::steady_clock> lastOwnerChangeTime;
        std::chrono::time_point<std::chrono::steady_clock> lastFlushTime;
        uint32_t bufferId = 0;
        std::shared_ptr<OHOS::HDI::Codec::V3_0::OmxCodecBuffer> omxBuffer;
        std::shared_ptr<AVBuffer> avBuffer;
        sptr<SurfaceBuffer> surfaceBuffer;
        bool needDealWithCache = false;

        void CleanUpUnusedInfo();
        void BeginCpuAccess();
        void EndCpuAccess();
        bool IsValidFrame() const;
        void Dump(const std::string& prefix, DumpMode dumpMode, bool isEncoder) const;

    private:
        void Dump(const std::string& prefix) const;
        void DumpSurfaceBuffer(const std::string& prefix) const;
        void DecideDumpInfo(std::optional<uint32_t>& assumeAlignedH, std::string& suffix, bool& dumpAsVideo) const;
        void DumpLinearBuffer(const std::string& prefix) const;
        static constexpr char DUMP_PATH[] = "/data/misc/hcodecdump";
    };

protected:
    HCodec(OHOS::HDI::Codec::V3_0::CodecCompCapability caps, OMX_VIDEO_CODINGTYPE codingType, bool isEncoder);
    ~HCodec() override;
    static const char* ToString(MsgWhat what);
    static const char* ToString(BufferOwner owner);
    void ReplyErrorCode(MsgId id, int32_t err);
    void PrintAllBufferInfo();
    void PrintAllBufferInfo(bool isInput, std::chrono::time_point<std::chrono::steady_clock> now);
    void PrintStatistic(bool isInput, std::chrono::time_point<std::chrono::steady_clock> now);
    std::string OnGetHidumperInfo();
    std::array<uint32_t, OWNER_CNT> CountOwner(bool isInput);
    void TraceOwner(const std::array<uint32_t, OWNER_CNT>& arr, bool isInput);
    void ChangeOwner(BufferInfo& info, BufferOwner newOwner);
    void ChangeOwnerNormal(BufferInfo& info, BufferOwner newOwner);
    void ChangeOwnerDebug(BufferInfo& info, BufferOwner newOwner);
    void UpdateInputRecord(const BufferInfo& info, std::chrono::time_point<std::chrono::steady_clock> now);
    void UpdateOutputRecord(const BufferInfo& info, std::chrono::time_point<std::chrono::steady_clock> now);

    // configure
    virtual int32_t OnConfigure(const Format &format) = 0;
    bool GetPixelFmtFromUser(const Format &format);
    static std::optional<double> GetFrameRateFromUser(const Format &format);
    int32_t SetVideoPortInfo(OMX_DIRTYPE portIndex, const PortInfo& info);
    virtual int32_t UpdateInPortFormat() = 0;
    virtual int32_t UpdateOutPortFormat() = 0;
    virtual void UpdateColorAspects() {}
    void PrintPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& def);
    int32_t SetFrameRateAdaptiveMode(const Format &format);
    int32_t SetProcessName(const Format &format);
    int32_t SetLowLatency(const Format &format);

    virtual int32_t OnSetOutputSurface(const sptr<Surface> &surface, bool cfg) { return AVCS_ERR_UNSUPPORT; }
    virtual int32_t OnSetParameters(const Format &format) { return AVCS_ERR_OK; }
    virtual sptr<Surface> OnCreateInputSurface() { return nullptr; }
    virtual int32_t OnSetInputSurface(sptr<Surface> &inputSurface) { return AVCS_ERR_UNSUPPORT; }
    virtual int32_t RequestIDRFrame() { return AVCS_ERR_UNSUPPORT; }

    // start
    virtual bool ReadyToStart() = 0;
    virtual int32_t AllocateBuffersOnPort(OMX_DIRTYPE portIndex) = 0;
    virtual void UpdateFormatFromSurfaceBuffer() = 0;
    int32_t GetPortDefinition(OMX_DIRTYPE portIndex, OMX_PARAM_PORTDEFINITIONTYPE& def);
    int32_t AllocateAvSurfaceBuffers(OMX_DIRTYPE portIndex);
    int32_t AllocateAvLinearBuffers(OMX_DIRTYPE portIndex);
    int32_t AllocateAvHardwareBuffers(OMX_DIRTYPE portIndex, const OMX_PARAM_PORTDEFINITIONTYPE& def);
    int32_t AllocateAvSharedBuffers(OMX_DIRTYPE portIndex, const OMX_PARAM_PORTDEFINITIONTYPE& def);
    std::shared_ptr<OHOS::HDI::Codec::V3_0::OmxCodecBuffer> SurfaceBufferToOmxBuffer(
        const sptr<SurfaceBuffer>& surfaceBuffer);
    std::shared_ptr<OHOS::HDI::Codec::V3_0::OmxCodecBuffer> DynamicSurfaceBufferToOmxBuffer();

    virtual int32_t SubmitAllBuffersOwnedByUs() = 0;
    virtual int32_t SubmitOutputBuffersToOmxNode() { return AVCS_ERR_UNSUPPORT; }
    BufferInfo* FindBufferInfoByID(OMX_DIRTYPE portIndex, uint32_t bufferId);
    std::optional<size_t> FindBufferIndexByID(OMX_DIRTYPE portIndex, uint32_t bufferId);
    virtual void OnGetBufferFromSurface(const ParamSP& param) = 0;
    uint32_t UserFlagToOmxFlag(AVCodecBufferFlag userFlag);
    AVCodecBufferFlag OmxFlagToUserFlag(uint32_t omxFlag);
    bool WaitFence(const sptr<SyncFence>& fence);

    // input buffer circulation
    virtual void NotifyUserToFillThisInBuffer(BufferInfo &info);
    virtual void OnQueueInputBuffer(const MsgInfo &msg, BufferOperationMode mode);
    void OnQueueInputBuffer(BufferOperationMode mode, BufferInfo* info);
    virtual void OnSignalEndOfInputStream(const MsgInfo &msg);
    int32_t NotifyOmxToEmptyThisInBuffer(BufferInfo& info);
    virtual void OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode) = 0;

    // output buffer circulation
    int32_t NotifyOmxToFillThisOutBuffer(BufferInfo &info);
    void OnOMXFillBufferDone(const OHOS::HDI::Codec::V3_0::OmxCodecBuffer& omxBuffer, BufferOperationMode mode);
    void OnOMXFillBufferDone(BufferOperationMode mode, BufferInfo& info, size_t bufferIdx);
    void NotifyUserOutBufferAvaliable(BufferInfo &info);
    void OnReleaseOutputBuffer(const MsgInfo &msg, BufferOperationMode mode);
    virtual void OnReleaseOutputBuffer(const BufferInfo &info) {}
    virtual void OnRenderOutputBuffer(const MsgInfo &msg, BufferOperationMode mode);

    // stop/release
    void ReclaimBuffer(OMX_DIRTYPE portIndex, BufferOwner owner, bool erase = false);
    bool IsAllBufferOwnedByUsOrSurface(OMX_DIRTYPE portIndex);
    bool IsAllBufferOwnedByUsOrSurface();
    void EraseOutBuffersOwnedByUsOrSurface();
    void RecordOutBuffersOwnedByOmx();
    void EraseOutBuffersOwnedByOmx(uint32_t bufferId);
    void ClearBufferPool(OMX_DIRTYPE portIndex);
    virtual void OnClearBufferPool(OMX_DIRTYPE portIndex) {}
    virtual void EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i) = 0;
    void FreeOmxBuffer(OMX_DIRTYPE portIndex, const BufferInfo& info);
    virtual void OnEnterUninitializedState() {}

    // template
    template <typename T>
    static inline void InitOMXParam(T& param)
    {
        (void)memset_s(&param, sizeof(T), 0x0, sizeof(T));
        param.nSize = sizeof(T);
        param.nVersion.s.nVersionMajor = 1;
    }

    template <typename T>
    static inline void InitOMXParamExt(T& param)
    {
        (void)memset_s(&param, sizeof(T), 0x0, sizeof(T));
        param.size = sizeof(T);
        param.version.s.nVersionMajor = 1;
    }

    template <typename T>
    bool GetParameter(uint32_t index, T& param, bool isCfg = false)
    {
        int8_t* p = reinterpret_cast<int8_t*>(&param);
        std::vector<int8_t> inVec(p, p + sizeof(T));
        std::vector<int8_t> outVec;
        int32_t ret = isCfg ? compNode_->GetConfig(index, inVec, outVec) :
                              compNode_->GetParameter(index, inVec, outVec);
        if (ret != HDF_SUCCESS) {
            return false;
        }
        if (outVec.size() != sizeof(T)) {
            return false;
        }
        ret = memcpy_s(&param, sizeof(T), outVec.data(), outVec.size());
        if (ret != EOK) {
            return false;
        }
        return true;
    }

    template <typename T>
    bool SetParameter(uint32_t index, const T& param, bool isCfg = false)
    {
        const int8_t* p = reinterpret_cast<const int8_t*>(&param);
        std::vector<int8_t> inVec(p, p + sizeof(T));
        int32_t ret = isCfg ? compNode_->SetConfig(index, inVec) :
                              compNode_->SetParameter(index, inVec);
        if (ret != HDF_SUCCESS) {
            return false;
        }
        return true;
    }

    static inline uint32_t AlignTo(uint32_t side, uint32_t align)
    {
        if (align == 0) {
            return side;
        }
        return (side + align - 1) / align * align;
    }

protected:
    OHOS::HDI::Codec::V3_0::CodecCompCapability caps_;
    OMX_VIDEO_CODINGTYPE codingType_;
    bool isEncoder_;
    bool isSecure_ = false;
    std::string shortName_;
    uint32_t componentId_ = 0;
    std::string componentName_;
    std::string compUniqueStr_;
    bool debugMode_ = false;
    DumpMode dumpMode_ = DUMP_NONE;
    sptr<OHOS::HDI::Codec::V3_0::ICodecCallback> compCb_ = nullptr;
    sptr<OHOS::HDI::Codec::V3_0::ICodecComponent> compNode_ = nullptr;
    sptr<OHOS::HDI::Codec::V3_0::ICodecComponentManager> compMgr_ = nullptr;

    std::shared_ptr<MediaCodecCallback> callback_;
    PixelFmt configuredFmt_;
    BufferRequestConfig requestCfg_{};
    std::shared_ptr<Format> configFormat_;
    std::shared_ptr<Format> inputFormat_;
    std::shared_ptr<Format> outputFormat_;

    std::vector<BufferInfo> inputBufferPool_;
    std::vector<BufferInfo> outputBufferPool_;
    std::unordered_set<uint32_t> outBuffersOwnedByOmx_;
    bool isBufferCirculating_ = false;
    bool inputPortEos_ = false;
    bool outputPortEos_ = false;
    bool gotFirstOutput_ = false;

    struct TotalCntAndCost {
        uint64_t totalCnt = 0;
        uint64_t totalCostUs = 0;
    };
    std::array<TotalCntAndCost, OWNER_CNT> inputHoldTimeRecord_;
    std::chrono::time_point<std::chrono::steady_clock> firstInTime_;
    uint64_t inTotalCnt_ = 0;
    std::array<TotalCntAndCost, OWNER_CNT> outputHoldTimeRecord_;
    std::chrono::time_point<std::chrono::steady_clock> firstOutTime_;
    TotalCntAndCost outRecord_;
    std::unordered_map<int64_t, std::chrono::time_point<std::chrono::steady_clock>> inTimeMap_;

    static constexpr char BUFFER_ID[] = "buffer-id";
    static constexpr uint32_t WAIT_FENCE_MS = 100;
    static constexpr uint32_t WARN_FENCE_MS = 15;
    static constexpr uint32_t STRIDE_ALIGNMENT = 32;
    static constexpr double FRAME_RATE_COEFFICIENT = 65536.0;

private:
    struct BaseState : State {
    protected:
        BaseState(HCodec *codec, const std::string &stateName,
                  BufferOperationMode inputMode = KEEP_BUFFER, BufferOperationMode outputMode = KEEP_BUFFER)
            : State(stateName), codec_(codec), inputMode_(inputMode), outputMode_(outputMode) {}
        void OnMsgReceived(const MsgInfo &info) override;
        void ReplyErrorCode(MsgId id, int32_t err);
        void OnCodecEvent(const MsgInfo &info);
        virtual void OnCodecEvent(OHOS::HDI::Codec::V3_0::CodecEventType event, uint32_t data1, uint32_t data2);
        void OnGetFormat(const MsgInfo &info);
        virtual void OnShutDown(const MsgInfo &info) = 0;
        virtual void OnCheckIfStuck(const MsgInfo &info);
        void OnForceShutDown(const MsgInfo &info);
        void OnStateExited() override { codec_->stateGeneration_++; }

    protected:
        HCodec *codec_;
        BufferOperationMode inputMode_;
        BufferOperationMode outputMode_;
    };

    struct UninitializedState : BaseState {
        explicit UninitializedState(HCodec *codec) : BaseState(codec, "Uninitialized") {}
    private:
        void OnStateEntered() override;
        void OnMsgReceived(const MsgInfo &info) override;
        int32_t OnAllocateComponent(const std::string &name);
        void OnShutDown(const MsgInfo &info) override;
    };

    struct InitializedState : BaseState {
        explicit InitializedState(HCodec *codec) : BaseState(codec, "Initialized") {}
    private:
        void OnStateEntered() override;
        void ProcessShutDownFromRunning();
        void OnMsgReceived(const MsgInfo &info) override;
        void OnSetCallBack(const MsgInfo &info);
        void OnConfigure(const MsgInfo &info);
        void OnStart(const MsgInfo &info);
        void OnShutDown(const MsgInfo &info) override;
    };

    struct StartingState : BaseState {
        explicit StartingState(HCodec *codec) : BaseState(codec, "Starting") {}
    private:
        void OnStateEntered() override;
        void OnStateExited() override;
        void OnMsgReceived(const MsgInfo &info) override;
        int32_t AllocateBuffers();
        void OnCodecEvent(OHOS::HDI::Codec::V3_0::CodecEventType event, uint32_t data1, uint32_t data2) override;
        void OnShutDown(const MsgInfo &info) override;
        void ReplyStartMsg(int32_t errCode);
        bool hasError_ = false;
    };

    struct RunningState : BaseState {
        explicit RunningState(HCodec *codec) : BaseState(codec, "Running", RESUBMIT_BUFFER, RESUBMIT_BUFFER) {}
    private:
        void OnStateEntered() override;
        void OnMsgReceived(const MsgInfo &info) override;
        void OnCodecEvent(OHOS::HDI::Codec::V3_0::CodecEventType event, uint32_t data1, uint32_t data2) override;
        void OnShutDown(const MsgInfo &info) override;
        void OnFlush(const MsgInfo &info);
        void OnSetParameters(const MsgInfo &info);
    };

    struct OutputPortChangedState : BaseState {
        explicit OutputPortChangedState(HCodec *codec)
            : BaseState(codec, "OutputPortChanged", RESUBMIT_BUFFER, FREE_BUFFER) {}
    private:
        void OnStateEntered() override;
        void OnMsgReceived(const MsgInfo &info) override;
        void OnCodecEvent(OHOS::HDI::Codec::V3_0::CodecEventType event, uint32_t data1, uint32_t data2) override;
        void OnShutDown(const MsgInfo &info) override;
        void HandleOutputPortDisabled();
        void HandleOutputPortEnabled();
        void OnFlush(const MsgInfo &info);
        void OnCheckIfStuck(const MsgInfo &info) override;
    };

    struct FlushingState : BaseState {
        explicit FlushingState(HCodec *codec) : BaseState(codec, "Flushing") {}
    private:
        void OnStateEntered() override;
        void OnMsgReceived(const MsgInfo &info) override;
        void OnCodecEvent(OHOS::HDI::Codec::V3_0::CodecEventType event, uint32_t data1, uint32_t data2) override;
        void OnShutDown(const MsgInfo &info) override;
        void ChangeStateIfWeOwnAllBuffers();
        bool IsFlushCompleteOnAllPorts();
        int32_t UpdateFlushStatusOnPorts(uint32_t data1, uint32_t data2);
        bool flushCompleteFlag_[2] {false, false};
    };

    struct StoppingState : BaseState {
        explicit StoppingState(HCodec *codec) : BaseState(codec, "Stopping"),
            omxNodeInIdleState_(false),
            omxNodeIsChangingToLoadedState_(false) {}
    private:
        void OnStateEntered() override;
        void OnMsgReceived(const MsgInfo &info) override;
        void OnCodecEvent(OHOS::HDI::Codec::V3_0::CodecEventType event, uint32_t data1, uint32_t data2) override;
        void OnShutDown(const MsgInfo &info) override;
        void ChangeStateIfWeOwnAllBuffers();
        void ChangeOmxNodeToLoadedState(bool forceToFreeBuffer);
        bool omxNodeInIdleState_;
        bool omxNodeIsChangingToLoadedState_;
    };

    class HdiCallback : public OHOS::HDI::Codec::V3_0::ICodecCallback {
    public:
        explicit HdiCallback(HCodec* codec) : codec_(codec) { }
        virtual ~HdiCallback() = default;
        int32_t EventHandler(OHOS::HDI::Codec::V3_0::CodecEventType event,
                             const OHOS::HDI::Codec::V3_0::EventInfo& info);
        int32_t EmptyBufferDone(int64_t appData, const OHOS::HDI::Codec::V3_0::OmxCodecBuffer& buffer);
        int32_t FillBufferDone(int64_t appData, const OHOS::HDI::Codec::V3_0::OmxCodecBuffer& buffer);
    private:
        HCodec* codec_;
    };

private:
    int32_t DoSyncCall(MsgWhat msgType, std::function<void(ParamSP)> oper);
    int32_t DoSyncCallAndGetReply(MsgWhat msgType, std::function<void(ParamSP)> oper, ParamSP &reply);
    int32_t InitWithName(const std::string &name);
    void ReleaseComponent();
    void CleanUpOmxNode();
    void ChangeOmxToTargetState(OHOS::HDI::Codec::V3_0::CodecStateType &state,
                                OHOS::HDI::Codec::V3_0::CodecStateType targetState);
    bool RollOmxBackToLoaded();

    int32_t ForceShutdown(int32_t generation, bool isNeedNotifyCaller);
    void SignalError(AVCodecErrorType errorType, int32_t errorCode);
    void DeferMessage(const MsgInfo &info);
    void ProcessDeferredMessages();
    void ReplyToSyncMsgLater(const MsgInfo& msg);
    bool GetFirstSyncMsgToReply(MsgInfo& msg);

private:
    static constexpr size_t MAX_HCODEC_BUFFER_SIZE = 8192 * 4096 * 4; // 8K RGBA
    static constexpr uint32_t THREE_SECONDS_IN_US = 3'000'000;
    static constexpr uint32_t ONE_SECONDS_IN_US = 1'000'000;
    static constexpr uint32_t FIVE_SECONDS_IN_MS = 5'000;

    std::shared_ptr<UninitializedState> uninitializedState_;
    std::shared_ptr<InitializedState> initializedState_;
    std::shared_ptr<StartingState> startingState_;
    std::shared_ptr<RunningState> runningState_;
    std::shared_ptr<OutputPortChangedState> outputPortChangedState_;
    std::shared_ptr<FlushingState> flushingState_;
    std::shared_ptr<StoppingState> stoppingState_;

    int32_t stateGeneration_ = 0;
    bool isShutDownFromRunning_ = false;
    bool notifyCallerAfterShutdownComplete_ = false;
    bool keepComponentAllocated_ = false;
    bool hasFatalError_ = false;
    std::list<MsgInfo> deferredQueue_;
    std::map<MsgType, std::queue<std::pair<MsgId, ParamSP>>> syncMsgToReply_;
}; // class HCodec
} // namespace OHOS::MediaAVCodec
#endif // HCODEC_HCODEC_H
