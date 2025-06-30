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
#include "codec_hdi.h"
#include "type_converter.h"
#include "buffer/avbuffer.h"
#include "meta/meta_key.h" // foundation/multimedia/histreamer/interface/inner_api/

namespace OHOS::MediaAVCodec {
class HCodec : public CodecBase, protected StateMachine {
public:
    static std::shared_ptr<HCodec> Create(const std::string &name);
    int32_t Init(Media::Meta &callerInfo) override;
    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) override;
    int32_t Configure(const Format &format) override;
    int32_t SetCustomBuffer(std::shared_ptr<AVBuffer> buffer) override;
    sptr<Surface> CreateInputSurface() override;
    int32_t SetInputSurface(sptr<Surface> surface) override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;

    int32_t QueueInputBuffer(uint32_t index) override;
    int32_t NotifyEos() override;
    int32_t ReleaseOutputBuffer(uint32_t index) override;
    int32_t RenderOutputBuffer(uint32_t index) override;

    int32_t SignalRequestIDRFrame() override;
    int32_t SetParameter(const Format& format) override;
    int32_t GetChannelId(int32_t &channelId) override;
    int32_t GetInputFormat(Format& format) override;
    int32_t GetOutputFormat(Format& format) override;
    std::string GetHidumperInfo() override;
    int32_t SetLowPowerPlayerMode(bool isLpp) override;

    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t NotifyMemoryRecycle() override;
    int32_t NotifyMemoryWriteBack() override;
    int32_t NotifySuspend() override;
    int32_t NotifyResume() override;

protected:
    enum MsgWhat : MsgType {
        INIT,
        SET_CALLBACK,
        CONFIGURE,
        CONFIGURE_BUFFER,
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
        SUSPEND,
        RESUME,
        BUFFER_RECYCLE,
        BUFFER_WRITEBACK,
        GET_HIDUMPER_INFO,
        PRINT_ALL_BUFFER_OWNER,

        INNER_MSG_BEGIN = 1000,
        CODEC_EVENT,
        OMX_EMPTY_BUFFER_DONE,
        OMX_FILL_BUFFER_DONE,
        GET_BUFFER_FROM_SURFACE,
        CHECK_IF_REPEAT,
        SUBMIT_DYNAMIC_IF_EOS,
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
        uint32_t width = 0;
        uint32_t height = 0;
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
        BufferOwner nextStepOwner = OWNER_CNT;
        std::chrono::time_point<std::chrono::steady_clock> lastOwnerChangeTime;
        int64_t lastFlushTime = 0;
        uint32_t bufferId = 0;
        std::shared_ptr<CodecHDI::OmxCodecBuffer> omxBuffer;
        std::shared_ptr<AVBuffer> avBuffer;
        sptr<SurfaceBuffer> surfaceBuffer;
        bool attached = false;
        bool needDealWithCache = false;
        bool hasSwapedOut = false;

        void CleanUpUnusedInfo();
        void BeginCpuAccess();
        void EndCpuAccess();
        bool IsValidFrame() const;
#ifdef BUILD_ENG_VERSION
        void Dump(const std::string& prefix, uint64_t cnt, DumpMode dumpMode, bool isEncoder) const;

    private:
        void Dump(const std::string& prefix, uint64_t cnt) const;
        void DumpSurfaceBuffer(const std::string& prefix, uint64_t cnt) const;
        void DecideDumpInfo(int& alignedH, uint32_t& totalSize, std::string& suffix, bool& dumpAsVideo) const;
        void DumpLinearBuffer(const std::string& prefix) const;
        static constexpr char DUMP_PATH[] = "/data/misc/hcodecdump";
#endif
    };

protected:
    HCodec(CodecHDI::CodecCompCapability caps, OMX_VIDEO_CODINGTYPE codingType, bool isEncoder);
    ~HCodec() override;
    static const char* ToString(MsgWhat what);
    static const char* ToString(BufferOwner owner);
    void ReplyErrorCode(MsgId id, int32_t err);
    void OnPrintAllBufferOwner(const MsgInfo& msg);
    void PrintAllBufferInfo();
    void PrintAllBufferInfo(bool isInput, std::chrono::time_point<std::chrono::steady_clock> now);
    void PrintStatistic(bool isInput, std::chrono::time_point<std::chrono::steady_clock> now);
    void PrintInputStatistic(BufferInfo& info, std::chrono::time_point<std::chrono::steady_clock> now);
    void PrintOutputStatistic(BufferInfo& info, std::chrono::time_point<std::chrono::steady_clock> now);
    std::string OnGetHidumperInfo();
    void UpdateOwner();
    void UpdateOwner(bool isInput);
    void ReduceOwner(bool isInput, BufferOwner owner);
    void ChangeOwner(BufferInfo& info, BufferOwner newOwner);
    void ChangeOwnerNormal(BufferInfo& info, BufferOwner newOwner);
    void ChangeOwnerDebug(BufferInfo& info, BufferOwner newOwner);
    void UpdateInputRecord(const BufferInfo& info, std::chrono::time_point<std::chrono::steady_clock> now);
    void UpdateOutputRecord(const BufferInfo& info, std::chrono::time_point<std::chrono::steady_clock> now);

    // configure
    virtual int32_t OnConfigure(const Format &format) = 0;
    virtual int32_t OnConfigureBuffer(std::shared_ptr<AVBuffer> buffer) { return AVCS_ERR_UNSUPPORT; }
    bool GetPixelFmtFromUser(const Format &format);
    static std::optional<double> GetFrameRateFromUser(const Format &format);
    int32_t SetVideoPortInfo(OMX_DIRTYPE portIndex, const PortInfo& info);
    virtual int32_t UpdateInPortFormat() = 0;
    virtual int32_t UpdateOutPortFormat() = 0;
    virtual void UpdateColorAspects() {}
    void PrintPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& def);
    int32_t SetFrameRateAdaptiveMode(const Format &format);
    int32_t SetProcessName();
    int32_t SetLowLatency(const Format &format);

    virtual void OnSetOutputSurface(const MsgInfo &msg, BufferOperationMode mode);
    virtual int32_t OnSetParameters(const Format &format) { return AVCS_ERR_OK; }
    virtual sptr<Surface> OnCreateInputSurface() { return nullptr; }
    virtual int32_t OnSetInputSurface(sptr<Surface> &inputSurface) { return AVCS_ERR_UNSUPPORT; }
    virtual int32_t RequestIDRFrame() { return AVCS_ERR_UNSUPPORT; }

    // start
    virtual bool ReadyToStart() = 0;
    virtual int32_t AllocateBuffersOnPort(OMX_DIRTYPE portIndex) = 0;
    virtual void SetCallerToBuffer(int fd) {}
    virtual void UpdateFormatFromSurfaceBuffer() = 0;
    int32_t GetPortDefinition(OMX_DIRTYPE portIndex, OMX_PARAM_PORTDEFINITIONTYPE& def);
    int32_t AllocateAvSurfaceBuffers(OMX_DIRTYPE portIndex);
    int32_t AllocateAvLinearBuffers(OMX_DIRTYPE portIndex);
    int32_t AllocateAvHardwareBuffers(OMX_DIRTYPE portIndex, const OMX_PARAM_PORTDEFINITIONTYPE& def);
    int32_t AllocateAvSharedBuffers(OMX_DIRTYPE portIndex, const OMX_PARAM_PORTDEFINITIONTYPE& def);
    std::shared_ptr<CodecHDI::OmxCodecBuffer> SurfaceBufferToOmxBuffer(
        const sptr<SurfaceBuffer>& surfaceBuffer);
    std::shared_ptr<CodecHDI::OmxCodecBuffer> DynamicSurfaceBufferToOmxBuffer();

    virtual int32_t SubmitAllBuffersOwnedByUs() = 0;
    virtual int32_t SubmitOutputBuffersToOmxNode() { return AVCS_ERR_UNSUPPORT; }
    BufferInfo* FindBufferInfoByID(OMX_DIRTYPE portIndex, uint32_t bufferId);
    std::optional<size_t> FindBufferIndexByID(OMX_DIRTYPE portIndex, uint32_t bufferId);
    virtual void OnGetBufferFromSurface(const ParamSP& param) = 0;
    uint32_t UserFlagToOmxFlag(AVCodecBufferFlag userFlag);
    AVCodecBufferFlag OmxFlagToUserFlag(uint32_t omxFlag);
    bool WaitFence(const sptr<SyncFence>& fence);
    void WrapSurfaceBufferToSlot(BufferInfo &info,
        const sptr<SurfaceBuffer>& surfaceBuffer, int64_t pts, uint32_t flag);

    // input buffer circulation
    virtual void NotifyUserToFillThisInBuffer(BufferInfo &info);
    virtual void OnQueueInputBuffer(const MsgInfo &msg, BufferOperationMode mode);
    int32_t OnQueueInputBuffer(BufferOperationMode mode, BufferInfo* info);
    virtual void OnSignalEndOfInputStream(const MsgInfo &msg);
    int32_t NotifyOmxToEmptyThisInBuffer(BufferInfo& info);
    virtual void OnOMXEmptyBufferDone(uint32_t bufferId, BufferOperationMode mode) = 0;
    virtual void RepeatIfNecessary(const ParamSP& param) {}
    bool CheckBufPixFmt(const sptr<SurfaceBuffer>& buffer);

    // output buffer circulation
    virtual void DynamicModeSubmitBuffer() {}
    virtual void DynamicModeSubmitIfEos() {}
    int32_t NotifyOmxToFillThisOutBuffer(BufferInfo &info);
    void OnOMXFillBufferDone(const CodecHDI::OmxCodecBuffer& omxBuffer, BufferOperationMode mode);
    void OnOMXFillBufferDone(BufferOperationMode mode, BufferInfo& info, size_t bufferIdx);
    void NotifyUserOutBufferAvaliable(BufferInfo &info);
    void OnReleaseOutputBuffer(const MsgInfo &msg, BufferOperationMode mode);
    void OnReleaseOutputBuffer(uint32_t bufferId, BufferOperationMode mode);
    virtual void OnReleaseOutputBuffer(const BufferInfo &info) {}
    virtual void OnRenderOutputBuffer(const MsgInfo &msg, BufferOperationMode mode);
    virtual void BeforeCbOutToUser(BufferInfo &info) {}
    void RecordBufferStatus(OMX_DIRTYPE portIndex, uint32_t bufferId, BufferOwner nextOwner);
    virtual void SubmitBuffersToNextOwner() {}

    // stop/release
    void ReclaimBuffer(OMX_DIRTYPE portIndex, BufferOwner owner, bool erase = false);
    bool IsAllBufferOwnedByUsOrSurface(OMX_DIRTYPE portIndex);
    bool IsAllBufferOwnedByUsOrSurface();
    void EraseOutBuffersOwnedByUsOrSurface();
    void ClearBufferPool(OMX_DIRTYPE portIndex);
    virtual void OnClearBufferPool(OMX_DIRTYPE portIndex) {}
    virtual void EraseBufferFromPool(OMX_DIRTYPE portIndex, size_t i) = 0;
    void FreeOmxBuffer(OMX_DIRTYPE portIndex, const BufferInfo& info);
    virtual void OnEnterUninitializedState() {}

    // freeze
    virtual int32_t FreezeBuffers() { return AVCS_ERR_UNSUPPORT; }
    virtual int32_t ActiveBuffers() { return AVCS_ERR_UNSUPPORT; }
    virtual int32_t DecreaseFreq() { return AVCS_ERR_UNSUPPORT; }
    virtual int32_t RecoverFreq() { return AVCS_ERR_UNSUPPORT; }

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
    CodecHDI::CodecCompCapability caps_;
    OMX_VIDEO_CODINGTYPE codingType_;
    bool isEncoder_;
    bool isSecure_ = false;
    std::string shortName_;
    uint32_t componentId_ = 0;
    std::string compUniqueStr_;
    struct CallerInfo {
        int32_t pid = -1;
        std::string processName;
    };
    struct Caller {
        CallerInfo playerCaller;
        CallerInfo avcodecCaller;
        CallerInfo app;
        bool calledByAvcodec = true;
    } caller_;
    static std::shared_mutex g_mtx;
    static std::unordered_map<std::string, HCodec::Caller> g_callers;

    bool debugMode_ = false;
    bool disableDmaSwap_ = false;
    DumpMode dumpMode_ = DUMP_NONE;
    sptr<CodecHDI::ICodecCallback> compCb_ = nullptr;
    sptr<CodecHDI::ICodecComponent> compNode_ = nullptr;
    sptr<CodecHDI::ICodecComponentManager> compMgr_ = nullptr;

    std::shared_ptr<MediaCodecCallback> callback_;
    PixelFmt configuredFmt_{};
    BufferRequestConfig requestCfg_{};
    std::shared_ptr<Format> configFormat_;
    std::shared_ptr<Format> inputFormat_;
    std::shared_ptr<Format> outputFormat_;

    std::vector<BufferInfo> inputBufferPool_;
    std::vector<BufferInfo> outputBufferPool_;
    std::queue<uint32_t> inputBufIdQueueToOmx_;
    bool isBufferCirculating_ = false;
    bool inputPortEos_ = false;
    bool outputPortEos_ = false;
    bool gotFirstInput_ = false;
    bool gotFirstOutput_ = false;
    bool outPortHasChanged_ = false;
    int pid_ = -1;
    bool isLpp_ = false;
    double codecRate_ = 0.0;

    // VRR
    bool isVrrInitialized_ = false;
#ifdef USE_VIDEO_PROCESSING_ENGINE
    virtual int32_t VrrPrediction(BufferInfo &info) { return AVCS_ERR_UNSUPPORT; }
#endif

    struct TotalCntAndCost {
        uint64_t totalCnt = 0;
        uint64_t totalCostUs = 0;
    };

    // whole lift time
    uint64_t inTotalCnt_ = 0;
    TotalCntAndCost outRecord_;
    std::unordered_map<int64_t, std::chrono::time_point<std::chrono::steady_clock>> inTimeMap_;
    int64_t lastInPts_ = -1;
    int64_t lastOutPts_ = -1;

    // normal: every 400 frames, debug: whole life time
    static constexpr uint64_t PRINT_PER_FRAME = 400;
    std::array<TotalCntAndCost, OWNER_CNT> inputHoldTimeRecord_;
    std::array<TotalCntAndCost, OWNER_CNT> outputHoldTimeRecord_;
    std::chrono::time_point<std::chrono::steady_clock> firstInTime_;
    std::chrono::time_point<std::chrono::steady_clock> firstOutTime_;
    uint64_t inputWaitFenceCostUs_ = 0;
    uint64_t outputWaitFenceCostUs_ = 0;
    uint64_t inputDiscardCnt_ = 0;
    uint64_t outputDiscardCnt_ = 0;

    // used when buffer circulation stoped
    static constexpr char KEY_LAST_OWNER_CHANGE_TIME[] = "lastOwnerChangeTime";
    std::chrono::time_point<std::chrono::steady_clock> lastOwnerChangeTime_;
    bool circulateHasStopped_ = false;

    std::array<int, HCodec::OWNER_CNT> inputOwner_ {};
    std::array<std::string, HCodec::OWNER_CNT> inputOwnerStr_ {};
    std::array<int, HCodec::OWNER_CNT> outputOwner_ {};
    std::array<std::string, HCodec::OWNER_CNT> outputOwnerStr_ {};

    static constexpr char BUFFER_ID[] = "buffer-id";
    static constexpr uint32_t WAIT_FENCE_MS = 1000;
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
        virtual void OnCodecEvent(CodecHDI::CodecEventType event, uint32_t data1, uint32_t data2);
        void OnGetFormat(const MsgInfo &info);
        virtual void OnShutDown(const MsgInfo &info) = 0;
        virtual void OnCheckIfStuck(const MsgInfo &info);
        void OnForceShutDown(const MsgInfo &info);
        void OnStateExited() override { codec_->stateGeneration_++; }
        void OnSetParameters(const MsgInfo &info);
        void OnSuspend(const MsgInfo &info);
        void OnResume(const MsgInfo &info);

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
        void OnCodecEvent(CodecHDI::CodecEventType event, uint32_t data1, uint32_t data2) override;
        void OnShutDown(const MsgInfo &info) override;
        void ReplyStartMsg(int32_t errCode);
        bool hasError_ = false;
    };

    struct RunningState : BaseState {
        explicit RunningState(HCodec *codec) : BaseState(codec, "Running", RESUBMIT_BUFFER, RESUBMIT_BUFFER) {}
    private:
        void OnStateEntered() override;
        void OnMsgReceived(const MsgInfo &info) override;
        void OnCodecEvent(CodecHDI::CodecEventType event, uint32_t data1, uint32_t data2) override;
        void OnShutDown(const MsgInfo &info) override;
        void OnFlush(const MsgInfo &info);
        void OnBufferRecycle(const MsgInfo &info);
    };

    struct OutputPortChangedState : BaseState {
        explicit OutputPortChangedState(HCodec *codec)
            : BaseState(codec, "OutputPortChanged", RESUBMIT_BUFFER, FREE_BUFFER) {}
    private:
        void OnStateEntered() override;
        void OnMsgReceived(const MsgInfo &info) override;
        void OnCodecEvent(CodecHDI::CodecEventType event, uint32_t data1, uint32_t data2) override;
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
        void OnCodecEvent(CodecHDI::CodecEventType event, uint32_t data1, uint32_t data2) override;
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
        void OnCodecEvent(CodecHDI::CodecEventType event, uint32_t data1, uint32_t data2) override;
        void OnShutDown(const MsgInfo &info) override;
        void ChangeStateIfWeOwnAllBuffers();
        void ChangeOmxNodeToLoadedState(bool forceToFreeBuffer);
        bool omxNodeInIdleState_;
        bool omxNodeIsChangingToLoadedState_;
    };

    struct FrozenState : BaseState {
        explicit FrozenState(HCodec *codec) : BaseState(codec, "Frozen") {}
    private:
        void OnMsgReceived(const MsgInfo &info) override;
        void OnShutDown(const MsgInfo &info) override;
        void OnBufferWriteback(const MsgInfo &info);
    };

    class HdiCallback : public CodecHDI::ICodecCallback {
    public:
        explicit HdiCallback(std::weak_ptr<MsgToken> codec) : codec_(codec) { }
        virtual ~HdiCallback() = default;
        int32_t EventHandler(CodecHDI::CodecEventType event,
                             const CodecHDI::EventInfo& info);
        int32_t EmptyBufferDone(int64_t appData, const CodecHDI::OmxCodecBuffer& buffer);
        int32_t FillBufferDone(int64_t appData, const CodecHDI::OmxCodecBuffer& buffer);
    private:
        std::weak_ptr<MsgToken> codec_;
    };

private:
    int32_t DoSyncCall(MsgWhat msgType, std::function<void(ParamSP)> oper, uint32_t waitMs = FIVE_SECONDS_IN_MS);
    int32_t DoSyncCallAndGetReply(MsgWhat msgType, std::function<void(ParamSP)> oper, ParamSP &reply,
                                  uint32_t waitMs = FIVE_SECONDS_IN_MS);
    void PrintCaller();
    void PrintAllCaller();
    void RemoveCaller();
    int32_t OnAllocateComponent();
    void ReleaseComponent();
    void CleanUpOmxNode();
    void ChangeOmxToTargetState(CodecHDI::CodecStateType &state,
                                CodecHDI::CodecStateType targetState);
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
    static constexpr double HIGH_FPS = 120.0;

    std::shared_ptr<UninitializedState> uninitializedState_;
    std::shared_ptr<InitializedState> initializedState_;
    std::shared_ptr<StartingState> startingState_;
    std::shared_ptr<RunningState> runningState_;
    std::shared_ptr<OutputPortChangedState> outputPortChangedState_;
    std::shared_ptr<FlushingState> flushingState_;
    std::shared_ptr<StoppingState> stoppingState_;
    std::shared_ptr<FrozenState> frozenState_;

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
