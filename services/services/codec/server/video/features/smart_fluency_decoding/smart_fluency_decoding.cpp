/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "smart_fluency_decoding.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include "securec.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "buffer/avbuffer.h"
#include "buffer_extra_data_impl.h"
#include "strategies/retention_strategy_factory.h"
#include "media_description.h"
#include "meta/meta_key.h"
#include "smart_fluency_decoding_types.h"
#include "surface_buffer.h"
#include "v2_0/buffer_handle_meta_key_type.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "SmartFluencyDecoding"};
constexpr double DEFAULT_PLAYBACK_SPEED = 1.0;
constexpr double RETENTION_RATIO_MIN_DIFF = 0.001;

#ifdef SFD_DEBUG
inline int64_t GetCurrentSysTimeUs() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}
#endif
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
namespace SFD {
SmartFluencyDecoding::SmartFluencyDecoding()
{
}

SmartFluencyDecoding::~SmartFluencyDecoding()
{
}

int32_t SmartFluencyDecoding::Initialize(const SFDConfig& config)
{
    CHECK_AND_RETURN_RET_LOG(config.width > 0 && config.height > 0, AVCS_ERR_INVALID_VAL,
        "Invalid video dimensions: %{public}dx%{public}d", config.width, config.height);
    CHECK_AND_RETURN_RET_LOG(config.codecType != SFDCodecType::INVALID, AVCS_ERR_INVALID_VAL,
        "Unsupported codec type");
    config_ = config;
    int32_t ret = InitializeInternal();
    CHECK_AND_RETURN_RET_LOG(ret == 0, AVCS_ERR_UNKNOWN, "Failed to initialize internal components");
    isInitialized_ = true;
    AVCODEC_LOGE("CSD_DEBUG: SmartFluencyDecoding initialized: %{public}dx%{public}d, codecType=%{public}d, mode=%{public}d",
        config_.width, config_.height, static_cast<int32_t>(config_.codecType), static_cast<int32_t>(config_.initMode));
    return AVCS_ERR_OK;
}

int32_t SmartFluencyDecoding::InitializeInternal()
{
    strategy_ = RetentionStrategyFactory::CreateStrategy(config_.initMode, config_.retentionRatio);
    CHECK_AND_RETURN_RET_LOG(strategy_ != nullptr, AVCS_ERR_UNKNOWN, "Failed to create strategy");
    if (strategy_->RequiresNaluAnalysis()) {
        EnsureNaluAnalyzer();
    }
    if (strategy_->RequiresMvAnalysis()) {
        EnsureMvAnalyzer();
    }
    dropSyncCoordinator_ = std::make_unique<DropSyncCoordinator>();
    asyncDispatcher_ = std::make_unique<AsyncDropDispatcher>();
#ifdef SFD_DEBUG
    dumper_ = SmartFluencyDecodingDumper::Create(
        config_.codecType, config_.width, config_.height, config_.initMode);
    if (dumper_ && config_.retentionRatio.has_value()) {
        dumper_->UpdateMode(config_.initMode, config_.retentionRatio.value());
    }
#endif
    AVCODEC_LOGI("Internal init completed: %{public}dx%{public}d, codecType=%{public}d",
                 config_.width, config_.height, static_cast<int>(config_.codecType));
    return AVCS_ERR_OK;
}

void SmartFluencyDecoding::UpdatePlaybackSpeed(double speed)
{
    std::lock_guard<std::mutex> lock(configMutex_);
    currentPlaybackSpeed_ = (speed > 0.0 ? speed : DEFAULT_PLAYBACK_SPEED);
    if (strategy_) {
        strategy_->UpdatePlaybackSpeed(speed);
#ifdef SFD_DEBUG
        if (dumper_) {
            dumper_->UpdateSpeed(speed);
        }
#endif
        AVCODEC_LOGI("SFD Playback speed dynamically updated to: %{public}.2f", speed);
    }
}

// 【新增】一统天下的解析与推演器
void SmartFluencyDecoding::ParseRetentionConfig(
    const Media::Format& format, RetentionStrategyType& mode, std::optional<double>& ratio)
{
    bool hasNewRatio = false;
    double parsedRatio = 0.0;
    
    // 1. 提取并校验 Ratio
    if (format.GetDoubleValue(Media::Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, parsedRatio)) {
        if (IsRetentionRatioValid(parsedRatio)) {
            ratio = parsedRatio;
            hasNewRatio = true;
            AVCODEC_LOGD("ParseRetentionConfig: Extracted valid ratio %{public}.3f", parsedRatio);
        }
    }

    int32_t extMode = -1;
    // 2. 提取 Mode，并处理与 Ratio 的关联
    if (format.GetIntValue(Media::Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, extMode)) {
        switch (extMode) {
            case static_cast<int32_t>(Media::Plugins::FrameRetentionMode::FULL):
                mode = RetentionStrategyType::FULL;
                break;
            case static_cast<int32_t>(Media::Plugins::FrameRetentionMode::ADAPTIVE):
                mode = RetentionStrategyType::ADAPTIVE;
                break;
            case static_cast<int32_t>(Media::Plugins::FrameRetentionMode::UNIFORM):
                // 【规则统一】：UNIFORM 模式下，有合法 Ratio 就定频，没有就动态保底
                mode = ratio.has_value() ? RetentionStrategyType::FIXED_RATIO : RetentionStrategyType::AUTO_RATIO;
                break;
            default:
                mode = RetentionStrategyType::INVALID;
                break;
        }
        AVCODEC_LOGD("ParseRetentionConfig: Extracted mode %{public}d -> internal %{public}d",
                     extMode, static_cast<int>(mode));
    } else if (hasNewRatio) {
        // 【推演逻辑】：上层只单发了 Ratio 而没带 Mode
        // - 对于 Builder(初始为 INVALID) 或 Dynamic(当前为 AUTO_RATIO)，自动升级为 FIXED_RATIO
        if (mode == RetentionStrategyType::INVALID || mode == RetentionStrategyType::AUTO_RATIO) {
            mode = RetentionStrategyType::FIXED_RATIO;
        }
    }

    // 3. 冲突告警
    if (ratio.has_value() && 
        (mode == RetentionStrategyType::ADAPTIVE || mode == RetentionStrategyType::FULL)) {
        AVCODEC_LOGW("ParseRetentionConfig: Adaptive or Full mode with retentionRate will be ignored");
    }
}

int32_t SmartFluencyDecoding::UpdateDynamicConfig(const Media::Format& format)
{
    AVCODEC_LOGD("UpdateDynamicConfig: entry");
    bool needUpdateStrategy = false;
    
    // 将当前状态作为底本传入
    RetentionStrategyType newMode = config_.initMode;
    std::optional<double> newRatio = config_.retentionRatio;
    
    // 调用推演机，获取下一时刻的新状态
    ParseRetentionConfig(format, newMode, newRatio);
    
    if (newMode != config_.initMode || newRatio != config_.retentionRatio) {
        needUpdateStrategy = true;
    }
    double speed = DEFAULT_PLAYBACK_SPEED;
    if (format.GetDoubleValue(Media::Tag::VIDEO_DECODER_SPEED, speed)) {
        AVCODEC_LOGD("UpdateDynamicConfig: Speed update detected: %{public}.2f", speed);
        UpdatePlaybackSpeed(speed);
    }
    if (needUpdateStrategy) {
        auto newStrategy = RetentionStrategyFactory::CreateStrategy(newMode, newRatio);
        if (newStrategy) {
            if (newStrategy->RequiresNaluAnalysis()) {
                EnsureNaluAnalyzer();
            }
            if (newStrategy->RequiresMvAnalysis()) {
                EnsureMvAnalyzer();
            }

            std::lock_guard<std::mutex> lock(configMutex_);
            strategy_ = std::move(newStrategy);
            AVCODEC_LOGI("SFD mode updated from %{public}d to %{public}d, ratio: %{public}.3f",
                         static_cast<int32_t>(config_.initMode), static_cast<int32_t>(newMode), 
                         (newRatio.has_value() ? newRatio.value() : -1.0));
            config_.initMode = newMode;
            config_.retentionRatio = newRatio;
#ifdef SFD_DEBUG
            if (dumper_) {
                dumper_->UpdateMode(newMode, newRatio.has_value() ? newRatio.value() : DEFAULT_PLAYBACK_SPEED);
            }
#endif
        } else {
            AVCODEC_LOGE("Failed to dynamically update SFD mode to %{public}d", static_cast<int32_t>(newMode));
        }
    }

    return AVCS_ERR_OK;
}

void SmartFluencyDecoding::Flush()
{
    std::lock_guard<std::mutex> lock(configMutex_);
    if (dropSyncCoordinator_) {
        dropSyncCoordinator_->Reset();
    }
    if (strategy_) {
        strategy_->Reset();
    }
    inBufferMap_.clear();
#ifdef SFD_DEBUG
    if (dumper_) {
        dumper_->DumpToFile();
    }
#endif
    AVCODEC_LOGI("SFD flushed (counters and trackers reset)");
}

void SmartFluencyDecoding::SetInputBuffer(uint32_t index, const std::shared_ptr<AVBuffer>& buffer)
{
    AVCODEC_LOGD("SetInputBuffer: index %{public}u, buffer valid: %{public}d", index, (buffer != nullptr));
    std::lock_guard<std::mutex> lock(configMutex_);
    if (buffer) {
        inBufferMap_[index] = buffer;
    }
}

void SmartFluencyDecoding::SetCallbacks(AsyncDropInputCallback inCb, AsyncDropOutputCallback outCb)
{
    AVCODEC_LOGD("SetCallbacks: inCb valid: %{public}d, outCb valid: %{public}d", (inCb != nullptr), (outCb != nullptr));
    std::lock_guard<std::mutex> lock(configMutex_);
    asyncDropInputCb_ = inCb;
    asyncDropOutputCb_ = outCb;
}

bool SmartFluencyDecoding::InitializeWithCsd(const std::shared_ptr<Media::AVBuffer>& csdBuffer)
{
    if (!isInitialized_ || !csdBuffer) {
        AVCODEC_LOGD("CSD_DEBUG: InitializeWithCsd skipped, !isInitialized_ or !csdBuffer");
        return false;
    }
    if (!(csdBuffer->flag_ & AVCODEC_BUFFER_FLAG_CODEC_DATA)) {
        AVCODEC_LOGW("CSD_DEBUG: Buffer passed to InitializeWithCsd does not have CODEC_DATA flag.");
        return false;
    }
    csdBuffer_ = csdBuffer;
    if (naluAnalyzer_) {
        (void)naluAnalyzer_->IsNonReferenceFrame(csdBuffer);
        AVCODEC_LOGI("CSD_DEBUG: CSD data processed by NaluAnalyzer.");
        return true;
    }
    AVCODEC_LOGD("CSD_DEBUG: naluAnalyzer_ not instantiated yet, caching CSD buffer.");
    return false;
}

void SmartFluencyDecoding::InitializeWithCsd(const std::vector<uint8_t>& csdData)
{
    if (csdData.empty()) {
        AVCODEC_LOGW("CSD_DEBUG: InitializeWithCsd failed, csdData is empty");
        return;
    }
    std::shared_ptr<Media::AVAllocator> allocator =
        Media::AVAllocatorFactory::CreateSharedAllocator(Media::MEMORY_READ_WRITE);
    CHECK_AND_RETURN_LOG(allocator != nullptr, "CSD_DEBUG: Failed to create shared allocator for replay tmpBuffer");
    auto tmpBuffer = Media::AVBuffer::CreateAVBuffer(allocator, static_cast<int32_t>(csdData.size()));
    if (tmpBuffer && tmpBuffer->memory_) {
        int32_t capacity = tmpBuffer->memory_->GetCapacity();
        if (capacity >= static_cast<int32_t>(csdData.size())) {
            if (memcpy_s(tmpBuffer->memory_->GetAddr(), csdData.size(), csdData.data(), csdData.size()) != EOK) {
                AVCODEC_LOGE("CSD_DEBUG: Failed to copy CSD data");
                return;
            }
            tmpBuffer->memory_->SetSize(csdData.size());
            tmpBuffer->flag_ = AVCODEC_BUFFER_FLAG_CODEC_DATA;
            // TODO: remove
            std::string hexStr;
            char buf[8] = {0};
            uint8_t *dstPtr = tmpBuffer->memory_->GetAddr();
            size_t printSize = std::min(csdData.size(), static_cast<size_t>(16));
            for (size_t i = 0; i < printSize; ++i) {
                snprintf(buf, sizeof(buf), "%02X ", dstPtr[i]);
                hexStr += buf;
            }
            AVCODEC_LOGI("CSD_DEBUG: Cached CSD (Size: %{public}zu), First %{public}zu bytes: [%{public}s]",
                         csdData.size(), printSize, hexStr.c_str());
            (void)InitializeWithCsd(tmpBuffer);
            AVCODEC_LOGI("CSD_DEBUG: SFD Mid-stream enabled: Replayed CSD data successfully, size=%{public}zu",
                         csdData.size());
        } else {
            AVCODEC_LOGE("CSD_DEBUG: Created buffer capacity (=%{public}d) is smaller than required (=%{public}zu)",
                          capacity, csdData.size());
        }
    } else {
        AVCODEC_LOGE("CSD_DEBUG: Failed to create AVBuffer or memory is null");
    }
}

void SmartFluencyDecoding::ExecutePreDrop(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_LOG(buffer != nullptr, "ExecutePreDrop: buffer is null for index %{public}u", index);
    AVCODEC_LOGE("CSD_DEBUG: [Pre-Drop] Executing drop for index %{public}u, pts %{public}" PRId64
        ", flag %{public}u", index, buffer->pts_, buffer->flag_);
    CHECK_AND_RETURN_LOG(buffer->flag_ != AVCODEC_BUFFER_FLAG_CODEC_DATA, "ExecutePreDrop: skip CODEC_DATA buffer");
    dropSyncCoordinator_->RecordPreDroppedFrame(buffer->pts_, currentPlaybackSpeed_);
    droppedInputFrames_++;
    AsyncDropInputCallback localCb = nullptr;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        localCb = asyncDropInputCb_;
    }
    if (localCb) {
        asyncDispatcher_->SubmitTask([cb = localCb, idx = index, buf = std::move(buffer)]() {
            cb(idx, buf);
        });
    }
}
std::shared_ptr<AVBuffer> SmartFluencyDecoding::FetchInputBufferAndCapabilities(
    uint32_t index, StrategyCaps& caps)
{
    std::lock_guard<std::mutex> lock(configMutex_);
    std::shared_ptr<AVBuffer> buffer = nullptr;
    auto it = inBufferMap_.find(index);
    if (it != inBufferMap_.end()) {
        buffer = it->second;
        inBufferMap_.erase(it);
    }
    if (strategy_) {
        caps.targetRatio = strategy_->GetCurrentRetentionRatio();
        caps.isPassthrough = strategy_->IsPassthrough();
        caps.requiresNalu = strategy_->RequiresNaluAnalysis();
        caps.requiresMv = strategy_->RequiresMvAnalysis();
    }
    return buffer;
}

SmartFluencyDecoding::PreDecResult SmartFluencyDecoding::EvaluatePreDecode(
    const std::shared_ptr<AVBuffer>& buffer, const StrategyCaps& caps)
{
    PreDecResult res;
    res.reason = SfdDecisionReason::PRE_RETAIN_REFERENCE;

    if (naluAnalyzer_ && caps.requiresNalu) {
#ifdef SFD_DEBUG
        int64_t naluStartUs = GetCurrentSysTimeUs();
#endif
        res.isNonRef = naluAnalyzer_->IsNonReferenceFrame(buffer);
#ifdef SFD_DEBUG
        res.naluCostUs = GetCurrentSysTimeUs() - naluStartUs;
#endif
        if (res.isNonRef) {
            double currentEmaRet = dropSyncCoordinator_->GetCurrentEmaRetentionRatio();
            
            // 如果目标保留率接近 100% (FULL模式)，无脑保留
            if (caps.targetRatio >= 1.0 - RETENTION_RATIO_MIN_DIFF) {
                res.reason = SfdDecisionReason::PRE_RETAIN_TARGET_FULL;
            } 
            // 如果实际保留率过低（低于等于目标），说明丢得太多了，这帧必须保留
            else if (currentEmaRet <= caps.targetRatio) {
                res.shouldRetain = true;
                res.reason = SfdDecisionReason::PRE_RETAIN_EMA_SATISFIED; 
            } 
            // 如果实际保留率还很高，那就放心丢掉这个非参考帧
            else {
                res.shouldRetain = false;
                res.reason = SfdDecisionReason::PRE_DROP_EMA_UNSATISFIED;
            }
        }
    } else if (caps.isPassthrough) {
        res.reason = SfdDecisionReason::PRE_RETAIN_TARGET_FULL;
    }
    return res;
}

void SmartFluencyDecoding::RecordPreDecodeDump(
    const std::shared_ptr<AVBuffer>& buffer, const PreDecResult& res, int64_t startUs)
{
#ifdef SFD_DEBUG
    if (!dumper_) return;
    SmartFluencyDecodingDumper::PreDecodeRecordCtx dumperCtx;
    dumperCtx.pts = buffer->pts_;
    dumperCtx.dts = buffer->dts_;
    dumperCtx.isNonRef = res.isNonRef;
    dumperCtx.flags = buffer->flag_;
    dumperCtx.isRetained = res.shouldRetain;
    dumperCtx.sysTimeMs = GetCurrentSysTimeMs();
    dumperCtx.preDecCostUs = GetCurrentSysTimeUs() - startUs;
    dumperCtx.naluCostUs = res.naluCostUs;
    dumperCtx.writeIndex = dropSyncCoordinator_->GetWriteIndex();
    dumperCtx.currentEmaRetentionRatio = dropSyncCoordinator_->GetCurrentEmaRetentionRatio();
    dumperCtx.reason = res.reason;
    dumper_->RecordPreDecode(dumperCtx);
#endif
}

SmartFluencyDecoding::PostDecResult SmartFluencyDecoding::EvaluatePostDecode(FrameRetentionContext& ctx)
{
    PostDecResult res;
    std::lock_guard<std::mutex> lock(configMutex_);
    res.shouldRetain = strategy_->MakeRetentionDecision(ctx);
    res.retentionRatio = strategy_->GetCurrentRetentionRatio();
    res.reason = strategy_->GetLastDecisionReason();
    return res;
}

void SmartFluencyDecoding::RecordPostDecodeDump(
    int64_t pts, uint32_t preDropCount, const PostDecResult& res, 
    const MVStats& mvStats, const PostDecCost& cost)
{
#ifdef SFD_DEBUG
    if (!dumper_) return;
    int64_t dumperStart = GetCurrentSysTimeUs();
    SmartFluencyDecodingDumper::PostDecodeRecordCtx dumperCtx;
    dumperCtx.pts = pts;
    dumperCtx.isRetained = res.shouldRetain;
    dumperCtx.preDropCount = preDropCount;
    dumperCtx.sysTimeMs = GetCurrentSysTimeMs();
    dumperCtx.postDecCostUs = dumperStart - cost.startUs;
    dumperCtx.readIndex = dropSyncCoordinator_->GetReadIndex();
    dumperCtx.currentEmaRetentionRatio = dropSyncCoordinator_->GetCurrentEmaRetentionRatio();
    dumperCtx.reason = res.reason;
    dumperCtx.mvStats = mvStats;
    dumperCtx.copyMvCostUs = cost.copyMvCostUs;
    dumperCtx.analyzeMvCostUs = cost.analyzeMvCostUs;

    static int64_t lastDumperOverhead = 0;
    dumperCtx.updateDumperCostUs = lastDumperOverhead;

    dumper_->RecordPostDecode(dumperCtx);
    lastDumperOverhead = GetCurrentSysTimeUs() - dumperStart;
#endif
}

bool SmartFluencyDecoding::MakePreDecodeDecision(uint32_t index)
{
#ifdef SFD_DEBUG
    int64_t startUs = GetCurrentSysTimeUs();
#endif
    StrategyCaps caps;
    auto buffer = FetchInputBufferAndCapabilities(index, caps);
    if (!isInitialized_ || !buffer || !dropSyncCoordinator_) {
        return true;
    }

    PreDecResult res = EvaluatePreDecode(buffer, caps);

    if (res.shouldRetain) {
        totalInputFrames_++;
    } else {
        ExecutePreDrop(index, buffer);
    }

#ifdef SFD_DEBUG
    RecordPreDecodeDump(buffer, res, startUs);
#endif
    AVCODEC_LOGD("MakePreDecodeDecision: index %{public}u, pts %{public}" PRId64 ", isNonRef %{public}d, shouldRetain %{public}d",
                 index, buffer->pts_, res.isNonRef, res.shouldRetain);
    return res.shouldRetain;
}

void SmartFluencyDecoding::ExecutePostDrop(uint32_t index)
{
    AVCODEC_LOGD("ExecutePostDrop: Dropping frame at index %{public}u", index);
    droppedOutputFrames_++;
    AsyncDropOutputCallback localCb = nullptr;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        localCb = asyncDropOutputCb_;
    }
    if (localCb) {
        asyncDispatcher_->SubmitTask([cb = localCb, idx = index]() {
            cb(idx);
        });
    }
}

bool SmartFluencyDecoding::MakePostDecodeDecision(uint32_t index, int64_t pts, const sptr<SurfaceBuffer> surfaceBuffer)
{
    PostDecCost cost;
#ifdef SFD_DEBUG
    cost.startUs = GetCurrentSysTimeUs();
#endif

    if (!isInitialized_ || !strategy_ || !dropSyncCoordinator_ || !surfaceBuffer) {
        return true;
    }

    uint32_t preDropCount = dropSyncCoordinator_->GetAndConsumePreDroppedCount(pts);
    StrategyCaps caps;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        caps.isPassthrough = strategy_->IsPassthrough();
        caps.requiresMv = strategy_->RequiresMvAnalysis();
    }

    // Fast-Path: 直通模式
    if (caps.isPassthrough) {
        {
            std::lock_guard<std::mutex> lock(configMutex_);
            dropSyncCoordinator_->UpdateFeedback(true, 1.0, currentPlaybackSpeed_);
        }
        totalOutputFrames_++;
#ifdef SFD_DEBUG
        PostDecResult fastRes {true, 1.0, SfdDecisionReason::POST_RETAIN_FULL};
        RecordPostDecodeDump(pts, preDropCount, fastRes, MVStats(), cost);
#endif
        return true;
    }

    MVStats mvStats;
    if (caps.requiresMv) {
        ParseMVData(surfaceBuffer, mvStats, cost.copyMvCostUs, cost.analyzeMvCostUs);
    }

    FrameRetentionContext ctx { pts, GetCurrentSysTimeMs(), 0, preDropCount, caps.requiresMv ? &mvStats : nullptr };
    PostDecResult res = EvaluatePostDecode(ctx);
    
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        dropSyncCoordinator_->UpdateFeedback(res.shouldRetain, res.retentionRatio, currentPlaybackSpeed_);
    }
    AVCODEC_LOGD("Strategy decision: shouldRetain %{public}d, ratio %{public}.2f (index:%{public}u, pts:%{public}" PRId64 ")",
                 res.shouldRetain, res.retentionRatio, index, pts);
    totalOutputFrames_++;
    
    if (!res.shouldRetain) {
        ExecutePostDrop(index);
    }

#ifdef SFD_DEBUG
    RecordPostDecodeDump(pts, preDropCount, res, mvStats, cost);
#endif
    return res.shouldRetain;
}

int64_t SmartFluencyDecoding::GetCurrentSysTimeMs()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

void SmartFluencyDecoding::ParseMVData(const sptr<SurfaceBuffer> surfaceBuffer, MVStats& mvStats, 
                                      int64_t& copyCostUs, int64_t& analyzeCostUs)
{
    if (mvAnalyzer_ == nullptr) {
        AVCODEC_LOGW("ParseMVData: mvAnalyzer_ is null, skipping MV analysis");
        return;
    }
    CHECK_AND_RETURN_LOG(surfaceBuffer != nullptr, "parse MV Data failed! surfaceBuffer don't exist");
    
#ifdef SFD_DEBUG
    int64_t copyStart = GetCurrentSysTimeUs();
#endif
    using namespace HDI::Display::Graphic::Common;
    std::vector<uint8_t> vec;
    V2_0::BlobDataType data;
    int32_t ret = surfaceBuffer->GetMetadata(V2_0::ATTRKEY_VIDEO_DECODER_MV, vec);
    if (ret != GSERROR_OK || vec.size() != sizeof(V2_0::BlobDataType)) {
        AVCODEC_LOGW("ParseMVData: getMetadata failed or size mismatch (ret:%d, size:%zu)", ret, vec.size());
        return;
    }
    ret = memcpy_s(&data, sizeof(V2_0::BlobDataType), vec.data(), vec.size());
    if (ret != EOK) {
        AVCODEC_LOGW("ParseMVData: memcpy_s failed (ret:%d)", ret);
        return;
    }
#ifdef SFD_DEBUG
    int64_t copyEnd = GetCurrentSysTimeUs();
    copyCostUs = copyEnd - copyStart;
    int64_t analyzeStart = copyEnd;
#endif

    uint8_t* input = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(data.vaddr + data.offset));
    mvStats = mvAnalyzer_->Analyze(input);

#ifdef SFD_DEBUG
    analyzeCostUs = GetCurrentSysTimeUs() - analyzeStart;
#endif
}

void SmartFluencyDecoding::GetStatistics(uint32_t& totalFrames, uint32_t& droppedFrames) const
{
    totalFrames = totalOutputFrames_.load();
    droppedFrames = droppedOutputFrames_.load();
    AVCODEC_LOGD("GetStatistics: totalOut %{public}u, droppedOut %{public}u", totalFrames, droppedFrames);
}

void SmartFluencyDecoding::OnDecodingPerformanceUpdate(double speed, double decFps)
{
    AVCODEC_LOGD("OnDecodingPerformanceUpdate: speed %{public}.2f, fps %{public}.2f", speed, decFps);
    std::lock_guard<std::mutex> lock(configMutex_);
    if (strategy_) {
        strategy_->OnDecodingPerformanceUpdate(speed, decFps);
    }
}

void SmartFluencyDecoding::EnsureNaluAnalyzer()
{
    std::unique_lock<std::mutex> lock(configMutex_);
    if (naluAnalyzer_) {
        return;
    }
    AVCODEC_LOGI("EnsureNaluAnalyzer: Creating new NaluAnalyzer for codecType %{public}d", 
                 static_cast<int32_t>(config_.codecType));
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(config_.codecType);
    if (!analyzer) {
        AVCODEC_LOGW("No NALU analyzer available for codecType %{public}d", static_cast<int32_t>(config_.codecType));
        return;
    }
    analyzer->SetAuxInfoCallback([this](const StreamAuxInfo& info) {
        std::lock_guard<std::mutex> cbLock(this->configMutex_);
        AVCODEC_LOGI("Received StreamAuxInfo: directSpatialMvPredFlag=%{public}u, direct8x8InferenceFlag=%{public}u, "
                     "frameMbsOnlyFlag=%{public}d, ctuSize=%{public}u",
                     info.directSpatialMvPredFlag, info.direct8x8InferenceFlag,
                     info.frameMbsOnlyFlag, info.ctuSize);

        if (this->mvAnalyzer_) {
            this->mvAnalyzer_->SetStreamAuxInfo(info);
        }
#ifdef SFD_DEBUG
        if (this->dumper_) {
            this->dumper_->UpdateCtuSize(info.ctuSize);
        }
#endif
    });
    naluAnalyzer_ = std::move(analyzer);
    if (!csdBuffer_) {
        AVCODEC_LOGD("EnsureNaluAnalyzer: No cached CSD buffer to replay.");
        return;
    }
    AVCODEC_LOGI("CSD_DEBUG: Replaying cached CSD buffer to activate newly instantiated NaluAnalyzer");
    auto cachedCsd = csdBuffer_;
    lock.unlock();
    (void)naluAnalyzer_->IsNonReferenceFrame(cachedCsd);
}

void SmartFluencyDecoding::EnsureMvAnalyzer()
{
    std::lock_guard<std::mutex> lock(configMutex_);
    if (mvAnalyzer_) {
        return;
    }
    AVCODEC_LOGI("EnsureMvAnalyzer: Creating new MvAnalyzer for codecType %{public}d", 
                 static_cast<int32_t>(config_.codecType));
    mvAnalyzer_ = MvAnalyzerFactory::CreateMvAnalyzer(config_.codecType);
    CHECK_AND_RETURN_LOG(mvAnalyzer_ != nullptr,
        "Failed to create MV parser for codec type: %{public}d", static_cast<int>(config_.codecType));
    mvAnalyzer_->SetVideoInfo(config_.width, config_.height);
}

bool SmartFluencyDecoding::IsRetentionRatioValid(double ratio)
{
    if (ratio < MIN_RETENTION_RATIO) {
        AVCODEC_LOGW("Retention ratio invalid: value=%{public}.2f (must be >= %{public}.2f)", 
                     ratio, MIN_RETENTION_RATIO);
        return false;
    }
    if (ratio > MAX_RETENTION_RATIO) {
        AVCODEC_LOGW("Retention ratio invalid: value=%{public}.2f (must be <= %{public}.2f)", 
                     ratio, MAX_RETENTION_RATIO);
        return false;
    }
    return true;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
